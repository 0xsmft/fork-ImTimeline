#include "Timeline.h"
#include "TimelineCore/ImTimeline_internal.h"

#include "Core/ImTimelineUtilityTime.h"
#include "TimelineData/ImDataController.h"
#include "TimelineViews/INodeView.h"
#include "TimelineCore/TimelinePlayer.h"

namespace ImTimeline {

	Timeline::Timeline()
	{
		m_EmptyDummyNode.Flags.set( TimelineNodeFlags_UseSectionBackground, true );
		m_EmptyDummyNode.Flags.set( TimelineNodeFlags_AutofitHeight, true );
		m_EmptyDummyNode.Flags.set( TimelineNodeFlags_MoveSurroundingNodesToTheRight, true );
		m_EmptyDummyNode.Flags.set( TimelineNodeFlags_MovedToDifferentTimeline, false );

		m_MainPlayer = std::make_shared<TimelinePlayer>();

		IM_ASSERT( m_MainPlayer != nullptr );

		if( m_MainPlayer->IsSetup() == false ) 
		{
			m_MainPlayer->Setup( nullptr, 0 );
		}

		if( m_MainPlayer->GetViewUI() == nullptr ) 
		{
			std::shared_ptr debugView = ImTimelineInternal::CreateDefaultPlayerView();
			SetTimelinePlayerUI( debugView );
		}
	}

	/* NODE ADD & COMMAND LOGIC */

	// memo: this parameter might as well been a NodeInitDescriptor and that would clean things up, but maybe further down the line,
	// more data would be added to the node, so just in case pass & copy the entire structure.
	TimelineNode* Timeline::AddNewNode( TimelineNode* node )
	{
		IM_ASSERT( node != nullptr );
		if( node->m_ID == INVALID_NODE_ID ) 
		{
			node->m_ID = m_IDGenerator.GetUniqueID();
		}

		InitializeTimelineSection( node->m_Section, "Unnamed" );

		if( m_Timelines.find( node->m_Section ) == m_Timelines.end() )
		{
			return nullptr;
		}

		if( m_Timelines[ node->m_Section ].bIsInitialized == false ) 
		{
			return nullptr;
		}
		
		IM_ASSERT( m_Timelines[ node->m_Section ].pNodeData != nullptr );

		// Node is not new, apply existing settings
		if( !node->Flags.test( TimelineNodeFlags_MovedToDifferentTimeline ) ) 
		{
			node->Flags = m_EmptyDummyNode.Flags;

			if( node->m_CustomNode != nullptr ) 
			{
				node->Flags.set( TimelineNodeFlags_CustomDraw, true );
			}

			// TODO what if we want to set default flags on a node-by-node basis, or pass flags in the NodeInitDescriptor?
		}

		// node gets recreated internally, original pointer can be deleted
		NodeInitDescriptor descriptor;
		bool bMoveOverlap = node->Flags.test( TimelineNodeFlags_MoveSurroundingNodesToTheRight );
		descriptor.bMoveOverlappingNext = bMoveOverlap;

		IM_ASSERT( m_Timelines[ node->m_Section ].pNodeData != nullptr );

		m_Timelines[ node->m_Section ].pNodeData->EmplaceBackDirect( *node, descriptor );

		NodeInitDescriptor searchDescriptor;
		searchDescriptor.ID = node->m_ID;
		TimelineNode* newlyAddedNode = m_Timelines[ node->m_Section ].pNodeData->GetNodeID( searchDescriptor );

		IM_ASSERT( newlyAddedNode != nullptr );

		if( newlyAddedNode->End > this->m_FrameMax )
			this->m_FrameMax = newlyAddedNode->End + 50;

		if( newlyAddedNode->End > m_Timelines[ newlyAddedNode->m_Section ].mProps.EndTimestamp ) 
		{
			m_Timelines[ newlyAddedNode->m_Section ].mProps.EndTimestamp = newlyAddedNode->End;
		}

		return newlyAddedNode;
	}

	TimelineNode& Timeline::AddNewNode( s32 section, s32 start, s32 end, const std::string& text, std::shared_ptr<CustomNodeBase> customNodeUI )
	{
		NodeID uniqueID = m_IDGenerator.GetUniqueID();

		auto cmd = std::make_unique<ImTimelineInternal::AddCommand>( this );
		cmd->NewNode.m_ID = uniqueID;
		cmd->NewNode.m_Section = section;
		cmd->NewNode.Start = start;
		cmd->NewNode.End = end;
		cmd->NewNode.DisplayText = text;

		if( customNodeUI )
		{
			bool bCustomUI = true;
			cmd->NewNode.InitalizeCustomNode( customNodeUI, bCustomUI );
		}

		cmd->CommandDo();

		PushCommand( std::move( cmd ) );
		// WARNING mNewNode becomes invalidated

		if( m_Timelines.find( section ) == m_Timelines.end() ) 
		{
			LOG_WARNING_PRINTF( "DeleteItem section %d does not exist", section );
			return m_EmptyDummyNode;
		}

		NodeInitDescriptor searchDescriptor;
		searchDescriptor.ID = uniqueID;
		TimelineNode* node = m_Timelines[ section ].pNodeData->GetNodeID( searchDescriptor );

		if( node ) 
		{
			return *node;
		}

		return m_EmptyDummyNode;
	}

	/* NODE MOVE & COMMAND LOGIC */

	void Timeline::MoveNode( TimelineNode* node, s32 newStart, s32 newSection )
	{
		LOG_INFO( "MoveCommand:" );

		auto cmd = std::make_unique<ImTimelineInternal::MoveNodeCommand>( this );

		cmd->pNodeToMove = m_pSelectedNode;
		cmd->NewStart = newStart;
		cmd->NewSectionID = newSection;
		cmd->CommandDo();

		PushCommand( std::move( cmd ) );
	}

	TimelineNode* Timeline::FindNodeByNodeID( NodeID nodeID ) const
	{
		if( nodeID == INVALID_NODE_ID ) 
			return nullptr;

		NodeInitDescriptor searchDescriptor;
		searchDescriptor.ID = nodeID;

		for( auto& timeline : m_Timelines ) 
		{
			TimelineNode* pNode = timeline.second.pNodeData->GetNodeID( searchDescriptor );
			
			if( pNode != nullptr ) 
				return pNode;
		}
		return nullptr;
	}

	TimelineNode* Timeline::FindNodeByNodeID( s32 section, NodeID nodeID ) const
	{
		if( nodeID == INVALID_NODE_ID ) 
			return nullptr;
		
		NodeInitDescriptor searchDescriptor;
		searchDescriptor.ID = nodeID;
		
		auto& catData = GetTimelineSection( section );
		
		TimelineNode* pNode = catData.pNodeData->GetNodeID( searchDescriptor );
		return pNode;
	}

	bool Timeline::HasSection( s32 section ) const
	{
		if( m_Timelines.find( section ) == m_Timelines.end() ) 
		{
			return false;
		}

		return true;
	}

	void Timeline::Undo()
	{
		if( m_CommandIndex >= 0 ) 
		{
			m_CommandHistory[ m_CommandIndex ]->CommandUndo();
			--m_CommandIndex;
		}
	}

	void Timeline::Redo()
	{
		if( m_CommandIndex + 1 < static_cast< int >( m_CommandHistory.size() ) )
		{
			++m_CommandIndex;
			m_CommandHistory[ m_CommandIndex ]->CommandDo();
		}
	}

	void Timeline::DeleteItem( s32 section, s32 start, s32 end )
	{
		LOG_INFO( "DeleteItem Command:" );
		auto cmd = std::make_unique<ImTimelineInternal::DeleteCommand>( this );

		cmd->Section = section;
		cmd->Start = start;
		cmd->End = end;
		cmd->CommandDo();

		PushCommand( std::move( cmd ) );
	}

	void Timeline::DeleteSelection()
	{
		if( m_pSelectedNode == nullptr )
			return;

		DeleteItem( m_pSelectedNode->m_Section, m_pSelectedNode->Start, m_pSelectedNode->End );
		m_pSelectedNode = nullptr;

		// add? maybe: delete selection when a node gets deleted by DeleteItem that is the selection
		// not easy to do with the current setup
	}

	void Timeline::DeleteSection( s32 section )
	{
		if( HasSection( section ) == false ) 
			return;

		DeleteItem( section, 0, m_Timelines[ section ].mProps.EndTimestamp );
		m_Timelines.erase( section );
	}

	bool Timeline::InitializeTimelineSection( s32 index, std::string name, ImDataController* data /* nullptr */ )
	{
		ImU32 bgColor = Color::LightGray;

		auto itr = m_Timelines.find( index );

		if( itr != m_Timelines.end() && itr->second.bIsInitialized == true ) 
		{
			bgColor = itr->second.mProps.DisplayProperties.BackgroundColor;
			return false;
		}
		else 
		{
			std::vector<ImU32> colors = { Color::LightBlue, Color::LightGreen, Color::LightRed, Color::LightGray };

			int colorId = m_CurrentSectionColorIndex % colors.size();
			bgColor = colors[ colorId ];

			m_CurrentSectionColorIndex++;
		}

		TimelineSection& rSection = m_Timelines[ index ]; // new or existing
		rSection.ID = index;
		rSection.mProps.DisplayProperties.Height = 40.0f; // todo set section defaults
		rSection.mProps.SectionName = name;
		rSection.mProps.DisplayProperties.BackgroundColor = bgColor;

		if( data != nullptr ) {
			rSection.pNodeData = data;
		}

		// if the first node of a section, allocate memory for all the nodes in the section
		if( rSection.pNodeData == nullptr )
		{
			rSection.pNodeData = ImTimelineInternal::CreateDefaultDataController();
		}

		if( rSection.NodeView == nullptr ) 
		{
			rSection.NodeView = ImTimelineInternal::CreateDefaultNodeView();
		}

		if( rSection.TimelinePlayer == nullptr ) 
		{
			std::shared_ptr<TimelinePlayer> playerPtr = std::make_shared<TimelinePlayer>();
			rSection.TimelinePlayer = playerPtr;
		}

		// individual timelines will send their signals to the parent timeline view
		if( m_MainPlayer && m_MainPlayer->GetViewUI() && rSection.TimelinePlayer ) 
		{
			rSection.TimelinePlayer->SetViewUI( m_MainPlayer->GetViewUI() );
		}

		if( rSection.TimelinePlayer && rSection.TimelinePlayer->IsSetup() == false )
		{
			s32 startTime = 0;
			rSection.TimelinePlayer->Setup( rSection.pNodeData, startTime );

			if( m_MainPlayer )
				m_MainPlayer->AddPlayer( rSection.TimelinePlayer );
		}

		if( rSection.bIsInitialized == false ) 
		{
			rSection.bIsInitialized = true;
			LOG_INFO( "Initialize new timeline section" );
		}

		return true;
	}

	bool Timeline::InitializeTimelineSectionEx( s32 index, std::string name, ImDataController* data, std::shared_ptr<ITimelinePlayerView> playerViewVUI, std::shared_ptr<INodeView> nodeViewUI )
	{
		ImU32 bgColor = Color::LightGray;

		auto itr = m_Timelines.find( index );

		if( itr != m_Timelines.end() && itr->second.bIsInitialized == true ) 
		{
			bgColor = itr->second.mProps.DisplayProperties.BackgroundColor;
			return false;
		}
		else 
		{
			std::vector<ImU32> colors = { Color::LightBlue, Color::LightGreen, Color::LightRed, Color::LightGray };

			int colorId = m_CurrentSectionColorIndex % colors.size();
			bgColor = colors[ colorId ];

			++m_CurrentSectionColorIndex;
		}

		TimelineSection& rSection = m_Timelines[ index ]; // new or existing
		rSection.ID = index;
		rSection.mProps.DisplayProperties.Height = 35.0f;
		rSection.mProps.SectionName = name;
		rSection.mProps.DisplayProperties.BackgroundColor = bgColor;

		if( rSection.pNodeData == nullptr )
		{
			rSection.pNodeData = data;
		}

		// if the first node of a section, allocate memory for all the nodes in the section
		if( rSection.pNodeData == nullptr ) 
		{
			rSection.pNodeData = ImTimelineInternal::CreateDefaultDataController();
		}

		if( rSection.NodeView == nullptr ) 
		{
			rSection.NodeView = nodeViewUI;
		}

		if( rSection.NodeView == nullptr ) 
		{
			rSection.NodeView = ImTimelineInternal::CreateDefaultNodeView();
		}

		if( rSection.TimelinePlayer == nullptr ) 
		{
			std::shared_ptr<TimelinePlayer> playerPtr = std::make_shared<TimelinePlayer>();
			rSection.TimelinePlayer = playerPtr;
		}

		// individual timelines will send their signals to the parent timeline view
		if( playerViewVUI.get() && rSection.TimelinePlayer )
		{
			rSection.TimelinePlayer->SetViewUI( playerViewVUI );
		}

		if( !rSection.TimelinePlayer->GetViewUI() ) 
		{
			rSection.TimelinePlayer->SetViewUI( m_MainPlayer->GetViewUI() );
		}

		if( rSection.TimelinePlayer && rSection.TimelinePlayer->IsSetup() == false ) 
		{
			s32 startTime = 0;
			rSection.TimelinePlayer->Setup( rSection.pNodeData, startTime );

			if( m_MainPlayer )
				m_MainPlayer->AddPlayer( rSection.TimelinePlayer );
		}

		if( rSection.bIsInitialized == false ) 
		{
			rSection.bIsInitialized = true;
			LOG_INFO( "Initialize new timeline section" );
		}

		return true;
	}

	TimelineSection& Timeline::GetTimelineSection( s32 index )
	{
		if( m_Timelines.find( index ) == m_Timelines.end() ) 
		{
			LOG_WARNING_PRINTF( "GetTimelineSection section %d does not exist", index );
			return m_EmptyDummySection;
		}

		return m_Timelines[ index ];
	}

	const TimelineSection& Timeline::GetTimelineSection( s32 index ) const
	{
		if( m_Timelines.find( index ) == m_Timelines.end() ) 
		{
			LOG_WARNING_PRINTF( "GetTimelineSection section %d does not exist", index );
			return m_EmptyDummySection;
		}

		return m_Timelines.at( index );
	}

	void Timeline::SetTimelineName( s32 index, std::string name )
	{
		m_Timelines[ index ].mProps.SectionName = name;
	}

	void Timeline::SetTimelineHeight( s32 index, f32 height )
	{
		if( m_Timelines.find( index ) == m_Timelines.end() ) 
		{
			LOG_WARNING_PRINTF( "GetTimelineSection section %d does not exist", index );
			return;
		}

		m_Timelines[ index ].mProps.DisplayProperties.Height = height;
	}

	void Timeline::SetTimelinePlayerUI( std::shared_ptr<ITimelinePlayerView> uiView )
	{
		m_MainPlayer->SetViewUI( uiView );
	}

	void Timeline::SetNodeViewUI( std::shared_ptr<INodeView> uiView )
	{
		for( auto& timeline : m_Timelines ) 
		{
			if( timeline.second.NodeView != uiView ) 
			{
				timeline.second.NodeView = uiView;
			}
		}
	}

	bool Timeline::DrawTimeline()
	{
		ScopedTimer timer = ScopedTimer( "Timeline Draw" );

		ImGuiIO& io = ImGui::GetIO();

		CollectInputData( m_InputData, io.DeltaTime );
		UpdateTimelinePlayer( io.DeltaTime );

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
		const int frameCount = ImMax( m_FrameMax - m_FrameMin, 1 );

		// zoom in/out
		m_VisibleFrameCount = ( int ) floorf( ( canvas_size.x - m_Style.LegendWidth ) / m_Zoom );
		m_StartFrame += ( s32 ) m_InputData.MouseScrollVertical * m_InputData.ScrollDirection * ( s32 ) m_InputData.ScrollSpeed;
		m_StartFrame = ImClamp( m_StartFrame, m_FrameMin, m_FrameMax - m_VisibleFrameCount );
		m_Zoom = ImLerp( m_Zoom, m_ZoomLerpTarget, 0.10f );

		if( m_VisibleFrameCount >= frameCount && m_StartFrame )
			m_StartFrame = m_FrameMin;

		if( IsDragging() )
		{
			ImGuiIO& io = ImGui::GetIO();
			updateSideDragLogic( io.DeltaTime );
		}

		if( m_StartFrame + m_VisibleFrameCount > m_FrameMax )
		{
			m_FrameMax = m_StartFrame + m_VisibleFrameCount;
		}

		if( m_StartFrame < 0 )
		{
			m_StartFrame = 0;
		}

		//
		for( auto& timeline : m_Timelines ) 
		{
			if( timeline.second.NodeView )
			{
				timeline.second.NodeView->PreDraw(); // TODO add deltatime
			}
		}

		// TODO Header draw in INodeView
		ImRect headerRect;
		headerRect.Min = canvas_pos;
		headerRect.Max = ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + m_Style.HeaderHeight );
		DrawHeader( headerRect );

		const ImVec2 childFrameSize( canvas_size.x, canvas_size.y - 8.f - headerRect.GetHeight() - ( m_Style.HasScrollbar ? static_cast< int >( m_Style.ScrollbarThickness ) : 0.f ) );

		ImGui::PushStyleColor( ImGuiCol_FrameBg, 0 );
		ImGui::BeginChild( 889, childFrameSize, ImGuiChildFlags_FrameStyle );

		const ImVec2 contentMin = ImGui::GetItemRectMin();
		const ImVec2 contentMax = ImGui::GetItemRectMax();
		const ImRect contentRect( contentMin, contentMax );
		m_ContentAreaRect = contentRect;

		draw_list->PushClipRect( contentMin, contentMax, true );

		f32 customHeight = 0.0f;

		for( auto& timeline : m_Timelines ) 
		{
			ScopedTimer timer = ScopedTimer( "NodeView Draw" );
			const f32 timeline_spacing = 5.0f;

			const auto localCustomHeight = timeline.second.mProps.DisplayProperties.Height;
			const ImVec2 pos = ImVec2( contentMin.x, contentMin.y + 1.0f + customHeight );
			const ImVec2 sz = ImVec2( canvas_size.x + canvas_pos.x, pos.y + localCustomHeight );
			const ImRect contentRect = ImRect( pos, sz );

			customHeight += localCustomHeight + timeline_spacing;

			if( timeline.second.NodeView ) 
			{
				timeline.second.NodeView->DrawNodeView( contentRect, timeline.second, this );
			}
		}

		if( IsDragging() && m_pSelectedNode != nullptr ) 
		{
			const s32 mouseTimestamp = GetTimestampAtPixelPosition( m_InputData.MousePos.x );

			if( m_DragData.DragStartTimestamp == -1 )
				m_DragData.DragStartTimestamp = mouseTimestamp - m_pSelectedNode->Start;

			m_DragData.DragNode.Start = mouseTimestamp - m_DragData.DragStartTimestamp;

			const auto nodeWidth = m_pSelectedNode->End - m_pSelectedNode->Start;
			m_DragData.DragNode.End = m_DragData.DragNode.Start + nodeWidth;
			m_DragData.DragNode.DisplayProperties.BackgroundColor = m_pSelectedNode->DisplayProperties.BackgroundColor + 0x00301000;

			if( m_InputData.LeftMouseDown == false ) 
			{
				m_DragData.DragState = DragState::DragNodeEnd;
			}

			if( m_DragData.DragState == DragState::DragNodeEnd ) 
			{
				if( m_SelectedTimelineIndex >= 0 )
				{
					MoveNode( m_pSelectedNode, m_DragData.DragNode.Start, m_SelectedTimelineIndex );
				}

				m_DragData.DragState = DragState::None;
				m_DragData.DragStartTimestamp = -1;
			}
		}

		if( IsDragging() ) 
		{
			const f32 width = m_DragData.DragRect.GetWidth();
			const f32 height = m_DragData.DragRect.GetHeight();
			const ImVec2 slotP1( m_InputData.MousePos.x - m_DragData.DragStartMouseDelta.x, m_InputData.MousePos.y - m_DragData.DragStartMouseDelta.y );
			const ImVec2 slotP2( slotP1.x + width, slotP1.y + height );

			// background drop shadow
			ImVec2 slotP3 = slotP2;
			slotP3.x = slotP2.x + 20;
			slotP3.y = slotP2.y + contentRect.GetHeight();
			
			draw_list->AddRectFilled( slotP1, slotP3, Color::Black );

			if( m_Timelines[ m_DragData.DragNode.m_Section ].NodeView )
			{
				m_Timelines[ m_DragData.DragNode.m_Section ].NodeView->DefaultNodeDraw( ImRect( slotP1, slotP2 ), m_DragData.DragNode, this );
			}
		}

		draw_list->PopClipRect();

		if( m_Style.HasSeekbar ) 
		{
			DrawSeekbarUI();
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();

		if( m_Style.HasScrollbar ) 
		{
			DrawScrollbar();
		}

		return true;
	}

	void Timeline::DrawDebugGUI()
	{
		if( ImGui::BeginTable( "MainLayoutTable", 2 ) ) 
		{
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 0 );
				OnCoreDebugGUI();
				ImGui::TableSetColumnIndex( 1 );
				ImGui::BeginChild( "Timeline", ImVec2( 0, 250 ), true );
				OnDebugGUIRightSidePane();
				ImGui::EndChild();
			}
			ImGui::EndTable();
		}
	}

	void Timeline::DrawHeader( const ImRect& headerRect )
	{
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		
		const ImVec2 headerSize( headerRect.GetWidth(), ( f32 ) m_Style.HeaderHeight );
		const ImVec2 canvasPos = headerRect.Min;

		pDrawList->AddRectFilled( headerRect.Min, headerRect.Max, m_Style.HeaderBackgroundColor, 0 );

		bool bCanShowDebugMenu = true;
		if( bCanShowDebugMenu ) 
		{
			pDrawList->AddText( headerRect.Min, m_Style.HeaderTimeStampColor, "Header" );
		}

		const ImRect timestampAreaClippingRect = ImRect( headerRect.Min + ImVec2( m_Style.LegendWidth, 0 ), headerRect.Min + headerSize );
		pDrawList->PushClipRect( timestampAreaClippingRect.Min, timestampAreaClippingRect.Max, true );

		if( timestampAreaClippingRect.Contains( m_InputData.MousePos ) && m_InputData.LeftMouseDown && !IsDragging() ) 
		{
			const s32 mouseTimestamp = GetTimestampAtPixelPosition( m_InputData.MousePos.x );

			if( m_MainPlayer )
				m_MainPlayer->SetStartTimestamp( mouseTimestamp );
		}

		int useFrameStep = 1;
		int timestampCount = 10;

		auto drawLine = [ & ]( int i, int height, bool bDrawLabel ) 
		{
			float windowX = canvasPos.x + ( i * m_Zoom ) + m_Style.LegendWidth - ( m_StartFrame * m_Zoom );
			float windowY = canvasPos.y + height;

			pDrawList->AddLine( ImVec2( windowX, canvasPos.y ), ImVec2( windowX, windowY ), m_Style.HeaderTimeStampColor, 1 );

			if( bDrawLabel ) 
			{
				std::string label;
				ImTimelineUtility::sprint_f( label, "%d", i );
				pDrawList->AddText( ImVec2( ( float ) windowX + 3.f, canvasPos.y ), m_Style.HeaderTimeStampColor, label.c_str() );
			}
		};

		while( ( timestampCount * m_Zoom ) < 150 )
		{
			timestampCount *= 2;
			useFrameStep *= 2;
		}

		const int halfModFrameCount = timestampCount / 2;

		for( int i = m_FrameMin; i <= m_FrameMax; i += useFrameStep ) 
		{
			int multiplier = 1;

			if( i % halfModFrameCount == 0 )
				multiplier = 2;

			int drawHeight = ( m_Style.HeaderHeight / 2 ) * multiplier;

			bool bDrawText = ( ( i % timestampCount ) == 0 ) || ( i == m_FrameMax || i == m_FrameMin );
			drawLine( i, drawHeight, bDrawText );
		}

		ImGui::PopClipRect();

		// moving internal xy coord down
		ImGui::InvisibleButton( "header_top_height", ImVec2( 200, headerSize.y ) );
	}

	void Timeline::DrawScrollbar()
	{
		if( m_Style.HasScrollbar == false )
			return;

		ImDrawList* pDrawList = ImGui::GetWindowDrawList();

		const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
		const ImVec2 scrollBarSize( canvasSize.x, 12.f );
		const int frameCount = ImMax( m_FrameMax - m_FrameMin, 1 );

		const float barWidthRatio = ImMin( m_VisibleFrameCount / ( float ) frameCount, 1.f );
		const float barWidthInPixels = barWidthRatio * ( canvasSize.x - m_Style.LegendWidth );

		ImGui::InvisibleButton( "scrollBar", scrollBarSize );
		const ImVec2 scrollBarMin = ImGui::GetItemRectMin();
		const ImVec2 scrollBarMax = ImGui::GetItemRectMax();

		// ratio = number of frames visible in control / number to total frames
		const float startFrameOffset = ( ( float ) ( m_StartFrame - m_FrameMin ) / ( float ) frameCount ) * ( canvasSize.x - m_Style.LegendWidth );
		const ImVec2 scrollBarA( scrollBarMin.x + m_Style.LegendWidth, scrollBarMin.y - 2 );
		const ImVec2 scrollBarB( scrollBarMin.x + canvasSize.x, scrollBarMax.y - 1 );
	
		pDrawList->AddRectFilled( scrollBarA, scrollBarB, 0xFF222222, 0 );

		const ImRect scrollBarRect( scrollBarA, scrollBarB );
		const bool inScrollBar = scrollBarRect.Contains( m_InputData.MousePos );

		pDrawList->AddRectFilled( scrollBarA, scrollBarB, 0xFF101010, 8 );

		const ImVec2 scrollBarC( scrollBarMin.x + m_Style.LegendWidth + startFrameOffset, scrollBarMin.y );
		const ImVec2 scrollBarD( scrollBarMin.x + m_Style.LegendWidth + barWidthInPixels + startFrameOffset, scrollBarMax.y - 2 );
		pDrawList->AddRectFilled( scrollBarC, scrollBarD, ( inScrollBar || m_InputData.IsMovingScrollBar ) ? 0xFF606060 : 0xFF505050, 6 );

		const ImRect barHandleLeft( scrollBarC, ImVec2( scrollBarC.x + 14, scrollBarD.y ) );
		const ImRect barHandleRight( ImVec2( scrollBarD.x - 14, scrollBarC.y ), scrollBarD );

		const bool onLeft = barHandleLeft.Contains( m_InputData.MousePos );
		const bool onRight = barHandleRight.Contains( m_InputData.MousePos );

		static bool sizingRBar = false;
		static bool sizingLBar = false;

		pDrawList->AddRectFilled( barHandleLeft.Min, barHandleLeft.Max, ( onLeft || sizingLBar ) ? 0xFFAAAAAA : 0xFF666666, 6 );
		pDrawList->AddRectFilled( barHandleRight.Min, barHandleRight.Max, ( onRight || sizingRBar ) ? 0xFFAAAAAA : 0xFF666666, 6 );

		const ImRect scrollBarThumb( scrollBarC, scrollBarD );

		if( m_InputData.IsMovingScrollBar ) 
		{
			if( m_InputData.LeftMouseDown == false ) 
			{
				m_InputData.IsMovingScrollBar = false;
			}
			else 
			{
				const float framesPerPixelInBar = barWidthInPixels / ( f32 ) m_VisibleFrameCount;

				m_StartFrame = int( ( m_InputData.MousePos.x - m_PanningData.PanningViewSource.x ) / framesPerPixelInBar ) - m_PanningData.PanningViewFrame;
				m_StartFrame = ImClamp( m_StartFrame, m_FrameMin, ImMax( m_FrameMax - m_VisibleFrameCount, m_FrameMin ) );
			}
		}
		else {
			if( scrollBarThumb.Contains( m_InputData.MousePos ) && ImGui::IsMouseClicked( 0 ) )
			{
				m_InputData.IsMovingScrollBar = true;
				m_PanningData.PanningViewSource = m_InputData.MousePos;
				m_PanningData.PanningViewFrame = -m_StartFrame;
			}

			if( !sizingRBar && onRight && ImGui::IsMouseClicked( 0 ) )
				sizingRBar = true;

			if( !sizingLBar && onLeft && ImGui::IsMouseClicked( 0 ) )
				sizingLBar = true;
		}
	}

	/******* DEBUG UI **************/

	GenericDisplayProperties& Timeline::GetSectionDisplayProperties( s32 section_index )
	{
		if( HasSection( section_index ) ) 
		{
			return m_Timelines.at( section_index ).mProps.DisplayProperties;
		}

		return m_EmptyDummySection.mProps.DisplayProperties;
	}

	void Timeline::OnCoreDebugGUI()
	{
		ImGui::BeginChild( "Scroll Area", ImVec2( -100, 250 ), ImGuiChildFlags_None, 0 );
		ImGui::PushItemWidth( 120 );

		// ImGuiTreeNodeFlags_DefaultOpen
		if( ImGui::TreeNodeEx( "Navigation" ) ) 
		{
			if( m_MainPlayer ) 
			{
				s32 currentTimestamp = ( s32 ) m_MainPlayer->GetCurrentTimestamp();

				if( ImGui::DragInt( "Current Frame", &currentTimestamp, 1.f, 0, m_FrameMax ) ) 
				{
					m_MainPlayer->SetStartTimestamp( currentTimestamp );
				}
			}

			ImGui::DragInt( "Min", &m_FrameMin, 1.f, 0, m_FrameMax );
			ImGui::SameLine();
			ImGui::DragInt( "Max", &m_FrameMax );

			ImGui::DragInt( "Start Frame", &m_StartFrame, 1.f, 0, m_FrameMax );
			ImGui::Text( "Selected Timeline Index %d", m_SelectedTimelineIndex );
			
			bool bIsDragging = IsDragging();
			ImGui::Checkbox( "IsDragging (readonly)", &bIsDragging );

			if( ImGui::CollapsingHeader( "Mouse Info" ) )
			{
				ImGui::Text( "Mouse: %f", m_InputData.MousePos.x );
				ImGui::Text( "Mouse Down Duration: %f", m_InputData.MouseDownDuration );

				const s32 timestampMouse = GetTimestampAtPixelPosition( m_InputData.MousePos.x );
				const s32 pixelPositionTimestamp = GetPixelPositionAtTimestamp( timestampMouse );
				
				ImGui::Text( "Mouse Timestamp: %d", timestampMouse );
				ImGui::Text( "Pixel Pos Mouse: %f", m_InputData.MousePos.x );
				ImGui::Text( "Pixel Pos Current Mouse Timestamp: %u", pixelPositionTimestamp );
			}

			ImGui::SliderFloat( "Scale", &m_ZoomLerpTarget, 1.f, 80.f );
			ImGui::InputFloat( "Mouse Scroll Speed", &m_InputData.ScrollSpeed );
			ImGui::InputFloat( "Edge Grwoth Speed", &m_EdgeMoveSpeed );

			ImGui::Text( "Visible Frame Count: %d", m_VisibleFrameCount );
			ImGui::TreePop();
		}

		if( ImGui::TreeNodeEx( "Display Style" ) )
		{
			ImGui::Text( "Timeline Style:" );
			
			ImGui::DragInt( "Header height", &m_Style.HeaderHeight, 1, 10, 300 );

			ImGui::DragFloat( "Legend Width", &m_Style.LegendWidth, 1.0f, 10.0f, 300.0f );
			
			ImGui::DragInt( "Scrollbar Thickness", &m_Style.ScrollbarThickness, 1, 10, 300 );

			ImTimelineUtility::DebugColor( "Header Background Color", m_Style.HeaderBackgroundColor );
			ImTimelineUtility::DebugColor( "Header Timestamp Color", m_Style.HeaderTimeStampColor );

			ImGui::Checkbox( "HasScrollbar", &m_Style.HasScrollbar );
			ImGui::Checkbox( "HasSeekbar", &m_Style.HasSeekbar );
			ImGui::Checkbox( "IsMovingScrollBar", &m_InputData.IsMovingScrollBar );
			ImGui::TreePop();
		}

		if( ImGui::TreeNodeEx( "Add/Delete Test" ) ) 
		{
			static s32 toAddCat = 0;
			static s32 toAddStart = 0;
			static s32 toAddEnd = 0;

			ImGui::Text( "New:" );
			ImGui::InputInt( "Section:", &toAddCat );

			static char ItemLabelStr[ 128 ] = "New Item";
			ImGui::InputText( "Text:", ItemLabelStr, IM_ARRAYSIZE( ItemLabelStr ) );
			ImGui::InputInt( "Start:", &toAddStart );
			ImGui::InputInt( "End:", &toAddEnd );

			if( ImGui::Button( "Add new:" ) )
			{
				AddNewNode( toAddCat, toAddStart, toAddEnd, ItemLabelStr );
			}

			ImGui::Text( "Command Index: %d", m_CommandIndex );

			ImGui::SameLine();

			if( ImGui::Button( "Delete Selected" ) ) 
			{
				DeleteSelection();
			}

			ImGui::SameLine();

			ImGui::SameLine();

			if( ImGui::Button( "Delete Range" ) ) 
			{
				DeleteItem( toAddCat, toAddStart, toAddEnd );
			}

			ImGui::SameLine();

			if( ImGui::Button( "Delete Section" ) ) 
			{
				DeleteSection( toAddCat );
			}

			ImGui::TreePop();
		}

		if( ImGui::TreeNodeEx( "Behavior Flags" ) ) 
		{
			ImGui::Text( "Default Node Flags:" );
			::ImTimelineInternal::ShowTimelineNodeFlagsDebugUI( &m_EmptyDummyNode );
			ImGui::Text( "Timeline Flags:" );

			bool bSkipRebuild = m_Flags.test( TimelineFlags_SkipTimelineRebuild );

			if( ImGui::Checkbox( "Skip Rebuild", &bSkipRebuild ) )
			{
				m_Flags.set( TimelineFlags_SkipTimelineRebuild, bSkipRebuild );
			}
			ImGui::TreePop();
		}

		if( ImGui::TreeNodeEx( "Performance" ) ) 
		{
			OnDebugGUIPerformance();
			ImGui::TreePop();
		}

		if( ImGui::TreeNodeEx( "Other" ) ) 
		{
			ImGui::Text( "Licenses:" );
			ImGui::Text( "Nameof" );

			if( ImGui::Button( "Show License: Nameof" ) )
				ImGui::OpenPopup( "NameofLicense" );

			if( ImGui::BeginPopup( "NameofLicense" ) ) 
			{
				ImGui::Text( ImTimelineLicense::LicenseNameof );
				ImGui::EndPopup();
			}

			ImGui::Text( "Version %s", IMTIMELINE_VERSION_STR );

			ImGui::TreePop();
		}

		if( ImGui::Button( "Undo" ) )
		{
			Undo();
		}

		ImGui::SameLine();

		if( ImGui::Button( "Redo" ) ) 
		{
			Redo();
		}

		ImGui::PopItemWidth();
		ImGui::EndChild();

	}

	void Timeline::OnDebugGUITimelineList()
	{
		ImGui::Text( "Timeline Display Properties:" );

		if( ImGui::BeginTabBar( "TimelineSectionTab" ) == false )
			return;

		for( auto& timeline : m_Timelines ) 
		{
			if( ImGui::BeginTabItem( timeline.second.mProps.SectionName.c_str() ) )
			{

				if( ImGui::Button( "Rebuild" ) )
				{
					NodeInitDescriptor descriptor;
					descriptor.Start = 0;

					timeline.second.pNodeData->Rebuild( descriptor );
				}

				ImGui::Indent( 25.f );
				OnDebugGuiDisplayProps( timeline.second.mProps.DisplayProperties );
				ImGui::Indent( -25.f );

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	void Timeline::OnDebugGuiDisplayProps( GenericDisplayProperties& displayPropsRef )
	{
		ImGui::PushItemWidth( 80 );
		ImGui::DragFloat( "Height: ", &displayPropsRef.Height );
		ImGui::DragFloat( "Width: ", &displayPropsRef.Width );
		ImGui::DragFloat( "BorderRadius: ", &displayPropsRef.BorderRadius );

		ImGui::DragInt( "Accent Thickness: ", &displayPropsRef.AccentThickness );

		ImTimelineUtility::DebugColor( "Background Color", displayPropsRef.BackgroundColor );
		ImTimelineUtility::DebugColor( "Background Color 2", displayPropsRef.BackgroundColorTwo );
		ImTimelineUtility::DebugColor( "Foreground Color", displayPropsRef.ForegroundColor );

		ImGui::PopItemWidth();
	}

	void Timeline::OnDebugGUIPerformance()
	{
		ImGui::Text( "Performance:" );
		ImGui::Text( "FPS: %.2f", ImGui::GetIO().Framerate );

		for( auto& timeline : m_Timelines ) 
		{
			ImGui::Text( "[%s]", timeline.second.mProps.SectionName.c_str() );
			timeline.second.pNodeData->PerformanceDebugUI();
		}

		ImGui::Text( "Performance Timers:" );
		ScopedTimer::DebugPrint();

		ImGui::Text( "NodeView Performance:" );

		for( auto& timeline : m_Timelines )
		{
			ImGui::Text( "[%s]", timeline.second.mProps.SectionName.c_str() );
		}

		static s32 toAddNumber = 100;
		static s32 toAddSectionIndex = 1;
		static s32 addStart = 3;
		static s32 count = 0;

		ImGui::Text( "New:" );
		ImGui::InputInt( "Add Count:", &toAddNumber );
		ImGui::InputInt( "Add Section:", &toAddSectionIndex );
		ImGui::Text( "Total Add count: %d", count );
		ImGui::InputInt( "Add start position:", &addStart );

		if( ImGui::Button( "Add bulk:" ) ) 
		{
			for( s32 i = 0; i < toAddNumber; ++i )
			{
				s32 width = Random::RandomIntRange( 1, 4 );
				AddNewNode( toAddSectionIndex, addStart, addStart + width, "New Item" );

				addStart += width + Random::RandomIntRange( 1, 3 );
				++count;
			}
		}
	}

	void Timeline::OnDebugGUIPlayer()
	{
		if( m_MainPlayer.get() == nullptr ) 
			return;

		m_MainPlayer->OnDebugGUI();

		if( ImGui::TreeNodeEx( "Custom TimelinePlayer View UI" ) ) 
		{
			if( m_MainPlayer->GetViewUI() ) 
			{
				m_MainPlayer->GetViewUI()->Draw();
			}
			else 
			{
				ImGui::Text( "No Custom View specified" );
			}

			ImGui::TreePop();
		}
	}

	void Timeline::OnDebugGUILog()
	{
		ImTimelineLog::getInstance().OnDebugGUI();
	}

	bool Timeline::OnDebugGUISelection()
	{
		ImGui::Text( "Selected: Node Properties:" );

		if( m_pSelectedNode == nullptr )
			return false;

		ImGui::Text( "ID: %d", m_pSelectedNode->m_ID );
		ImGui::Text( "Section: %d", m_pSelectedNode->m_Section );
		ImGui::Text( "Text: %s", m_pSelectedNode->DisplayText.c_str() );
		ImGui::DragInt( "Start", &m_pSelectedNode->Start );
		ImGui::DragInt( "End", &m_pSelectedNode->End );

		OnDebugGuiDisplayProps( m_pSelectedNode->DisplayProperties );

		ImGui::Text( "Flags:" );

		::ImTimelineInternal::ShowTimelineNodeFlagsDebugUI( m_pSelectedNode );

		if( m_pSelectedNode->m_CustomNode ) {
			ImGui::Text( "Custom Debug:" );
			m_pSelectedNode->m_CustomNode->OnDebugGUI();
		}

		return true;
	}

	void Timeline::OnDebugGUIRightSidePane()
	{
		if( ImGui::BeginTabBar( "RightSidePaneTabs" ) == false )
			return;

		if( ImGui::BeginTabItem( "Selected Node" ) ) 
		{
			OnDebugGUISelection();
			ImGui::EndTabItem();
		}

		if( ImGui::BeginTabItem( "Debug ImTimelineLog" ) ) 
		{
			OnDebugGUILog();
			ImGui::EndTabItem();
		}

		if( ImGui::BeginTabItem( "Timeline Player" ) ) 
		{
			OnDebugGUIPlayer();
			ImGui::EndTabItem();
		}

		if( ImGui::BeginTabItem( "Timeline Display Properties" ) )
		{
			OnDebugGUITimelineList();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	f32 Timeline::GetSeekbarPositionX()
	{
		if( m_MainPlayer.get() == nullptr ) 
			return 0.0f;
		
		const f32 timestampCurrent = m_MainPlayer->GetCurrentTimestamp();
		const f32 base = m_ContentAreaRect.Min.x;
		const f32 x = base + m_Style.LegendWidth + ( timestampCurrent - m_StartFrame ) * m_Zoom + m_Zoom / 2;

		return x;
	}

	void Timeline::DrawSeekbarUI()
	{
		if( m_MainPlayer.get() == nullptr ) 
			return;
		
		const f32 currentTimestamp = m_MainPlayer->GetCurrentTimestamp();

		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		const ImVec2 canvasSize = ImGui::GetContentRegionAvail();

		const f32 x = GetSeekbarPositionX();
		const f32 y = m_ContentAreaRect.Min.y;

		const f32 x2 = x + m_Style.SeekbarWidth;
		const f32 y2 = canvasPos.y + canvasSize.y;

		// Don't draw the seekbar when it goes out of view (when the start position shifts).
		if( currentTimestamp >= m_StartFrame && currentTimestamp <= m_FrameMax ) 
		{
			pDrawList->AddRectFilled( ImVec2( x, y ), ImVec2( x2, y2 ), m_Style.SeekbarColor, 0 );
		}

		// timestamp text
		if( currentTimestamp >= m_StartFrame && currentTimestamp <= m_FrameMax ) 
		{
			std::string seekbarLabel;
		
			ImTimelineUtility::sprint_f( seekbarLabel, "%d", static_cast< s32 >( currentTimestamp ) );
			pDrawList->AddText( ImVec2( x + 10, y + 2 ), m_Style.SeekbarColor, seekbarLabel.c_str() );
		}
	}

	s32 Timeline::GetTimestampAtPixelPosition( f32 pixelPos ) const
	{
		const ImVec2 windowMin = ImGui::GetWindowPos();

		const f32 pixelPosRel = pixelPos - m_Style.LegendWidth;
		const f32 start = pixelPosRel - windowMin.x;
		return static_cast< u32 >( start / m_Zoom ) + m_StartFrame;
	}

	s32 Timeline::GetPixelPositionAtTimestamp( s32 timestamp ) const
	{
		return ( timestamp - m_StartFrame ) * static_cast< u32 >( m_Zoom );
	}

	void Timeline::UpdateTimelinePlayer( f32 deltaTime )
	{
		for( auto& timeline : m_Timelines ) 
		{
			auto player = timeline.second.TimelinePlayer;
			if( player == nullptr )
				continue;

			player->Update( deltaTime );
		}

		if( m_MainPlayer && m_MainPlayer->IsPlaying() ) 
		{
			m_MainPlayer->Update( deltaTime );
		}
	}

	void Timeline::ForceRebuild( s32 section, NodeInitDescriptor descriptor )
	{
		if( m_Flags.test( TimelineFlags_SkipTimelineRebuild ) )
			return;

		if( m_Timelines.find( section ) == m_Timelines.end() )
			return;

		if( m_Timelines[ section ].pNodeData == nullptr )
			return;

		m_Timelines[ section ].pNodeData->Rebuild( descriptor );
	}

	void Timeline::CollectInputData( InputData& a_outInputData, f32 aDeltaTime )
	{
		const bool bMouseDownLastFrame = m_InputData.LeftMouseDown;

		ImGuiIO& io = ImGui::GetIO();
		m_InputData.MousePos = io.MousePos;
		m_InputData.LeftMouseDown = io.MouseDown[ 0 ];
		m_InputData.RightMouseDown = io.MouseDown[ 1 ];
		m_InputData.MouseScrollVertical = io.MouseWheel;
		// mInputData.MouseDownDuration += io.MouseDownDuration[0];

		if( ImGui::IsKeyDown( ImGuiKey_Delete ) || ImGui::IsKeyDown( ImGuiKey_Backspace ) )
		{
			m_NextActionFlags.set( NextAction::ActionDelete );
		}

		if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyDown( ImGuiKey_Z ) )
		{
			m_NextActionFlags.set( NextAction::ActionUndo );
		}

		// TODO Redo: Mac Command + Shift + Z. Windows: Ctrl + Z
		if( m_NextActionFlags.test( NextAction::ActionDelete ) )
		{
			DeleteSelection();
		}

		if( m_NextActionFlags.test( NextAction::ActionUndo ) )
		{
			Undo();
		}

		if( m_NextActionFlags.test( NextAction::ActionRedo ) ) 
		{
			Redo();
		}

		m_NextActionFlags.reset();

		if( m_InputData.MouseDownDurationLastFrame > 0 )
			m_InputData.MouseDownDurationLastFrame = 0;

		if( m_InputData.LeftMouseDown ) 
		{
			++m_InputData.MouseDownDuration;
		}

		if( bMouseDownLastFrame && m_InputData.LeftMouseDown == false ) 
		{
			m_InputData.MouseDownDurationLastFrame = m_InputData.MouseDownDuration;
			m_InputData.MouseDownDuration = 0;
		}
	}

	void Timeline::PushCommand( std::unique_ptr<BaseCommand> command )
	{
		if( m_EnableCommands == false )
			return;

		if( m_CommandIndex < static_cast< int >( m_CommandHistory.size() ) - 1 ) 
		{
			m_CommandHistory.erase( m_CommandHistory.begin() + m_CommandIndex + 1, m_CommandHistory.end() );
		}

		m_CommandHistory.push_back( std::move( command ) );
		++m_CommandIndex;
	}

	// Auto move when dragging a node to the side
	void Timeline::updateSideDragLogic( f32 deltaTime )
	{
		const s32 timestampMouse = GetTimestampAtPixelPosition( m_InputData.MousePos.x );
		const s32 timestampRightEdge = m_StartFrame + m_VisibleFrameCount;
		const s32 timestampLeftEdge = m_StartFrame;
		const s32 autoMoveThreshold = 10;

		s32 moveDirection = 0;

		if( timestampMouse > timestampRightEdge - autoMoveThreshold )
		{
			moveDirection = 1;
		}

		if( timestampMouse <= timestampLeftEdge + autoMoveThreshold )
		{
			moveDirection = -1;
		}

		if( moveDirection != 0 )
		{
			m_EdgeMoveAmount += m_EdgeMoveSpeed * deltaTime;

			if( m_EdgeMoveAmount > 1 )
			{
				m_StartFrame += 1 * moveDirection;
				m_EdgeMoveAmount = 0;
			}
		}
	}

}

//////////////////////////////////////////////////////////////////////////
// TimelineSection

void TimelineSection::OnFinalize()
{
	delete pNodeData;
	pNodeData = nullptr;
}
