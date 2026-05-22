#include "HorizontalNodeView.h"

#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineUtility.h"
#include "../TimelineData/ImDataController.h"
#include "../Timeline.h"


void HorizontalNodeView::PreDraw()
{
	m_NodesDrawSkipped = 0;
}

void HorizontalNodeView::DrawNodeView( const ImRect& area, const TimelineSection& timeline, ImTimeline::Timeline* pContext )
{
	if( timeline.bIsInitialized == false ) 
		return;

	ImDrawList* pDrawList = ImGui::GetWindowDrawList();
	const ImVec2 canvasSize = pContext->m_ContentAreaRect.GetSize();
	const ImVec2 contentMin = pContext->m_ContentAreaRect.Min;

	IM_ASSERT( timeline.pNodeData != nullptr );

	// Background
	auto bgColorFullBar = IM_COL32( 40, 50, 50, 255 );
	if( area.Contains( pContext->GetLastInputData().MousePos ) ) 
	{
		bgColorFullBar += 0x80201008;
		pContext->SetSelectedTimeline( timeline.ID );
	}

	pDrawList->AddRectFilled( area.Min, area.Max, bgColorFullBar, 0 );

	DrawLegendArea( timeline, pContext, area );

	ImRect timelinePanelRect = ImRect( area.Min, area.Max );
	timelinePanelRect.Min.x += pContext->m_Style.LegendWidth - ( pContext->GetStartTimestamp() * pContext->GetScale() );
	timelinePanelRect.Max.x -= pContext->m_Style.LegendWidth - ( pContext->GetStartTimestamp() * pContext->GetScale() );

	// TODO VERTICAL SCROLL
	ImRect timelinePanelRectAbsolute = ImRect( area.Min, area.Max );
	timelinePanelRectAbsolute.Min.x += pContext->m_Style.LegendWidth;

	pDrawList->PushClipRect( timelinePanelRectAbsolute.Min, timelinePanelRectAbsolute.Max, true );

	IM_ASSERT( timeline.pNodeData != nullptr );

	// foreground
	auto& rItemList = timeline.pNodeData;
	auto sectionHeight = timeline.mProps.DisplayProperties.Height;

	u32 index = 0u;

	rItemList->Iterate( [ & ]( TimelineNode& node ) 
	{
		++index;

		if( node.Flags.test( ImTimelineNodeFlags_AutofitHeight ) == false )
		{
			sectionHeight = node.DisplayProperties.Height;
		}

		if( pContext->IsDragging() && pContext->m_DragData.DragNode.GetID() == node.GetID() ) 
		{
			return;
		}

		const auto scale = pContext->GetScale();
		const ImVec2 slotP1( timelinePanelRect.Min.x + node.Start * scale, timelinePanelRect.Min.y );
		const ImVec2 slotP2( timelinePanelRect.Min.x + node.End * scale + scale, slotP1.y + sectionHeight - node.DisplayProperties.AccentThickness );
		const ImRect nodeRect = ImRect( slotP1, slotP2 );

		const bool canDraw = slotP1.x <= ( canvasSize.x + contentMin.x ) && slotP2.x >= ( contentMin.x + pContext->m_Style.LegendWidth );

		if( !canDraw ) 
		{
			++m_NodesDrawSkipped;
			return;
		}

		DefaultNodeDraw( nodeRect, node, pContext );

#if defined IM_TIMELINE_DEBUG_INFO
		std::string nodeDebugText = "";
		ImTimelineUtility::sprint_f( nodeDebugText, "id: %d - index: %u", node.m_ID, index );
		pDrawList->AddText( nodeRect.Min + ImVec2( 10, 20 ), node.DisplayProperties.ForegroundColor, nodeDebugText.c_str() );
#endif

		const bool bIsSelected = ImRect( slotP1, slotP2 ).Contains( pContext->GetLastInputData().MousePos ) && pContext->GetLastInputData().LeftMouseDown;
		const bool bInputDelay = ( pContext->GetLastInputData().MouseDownDuration > 40.0f );

		if( bIsSelected && pContext->m_DragData.DragState == DragState::None ) 
		{
			pContext->SelectNode( &node );
		}

		if( bIsSelected && pContext->IsDragging() == false && bInputDelay ) 
		{
			pContext->m_DragData.DragState = DragState::DragNode;
			pContext->m_DragData.DragNode = *pContext->GetSelectedNode();
			pContext->m_DragData.DragStartMouseDelta = pContext->GetLastInputData().MousePos - slotP1;
			pContext->m_DragData.DragRect = nodeRect;
		}
	} );

	pDrawList->PopClipRect();

	if( pContext->IsDragging() && pContext->GetSelectedSection() == timeline.ID )
	{
		pDrawList->AddRect( area.Min, area.Max, Color::Yellow, 0 );
	}
}

void HorizontalNodeView::DefaultNodeDraw( const ImRect& area, const TimelineNode& node, ImTimeline::Timeline* timeline )
{
	bool bSelected = timeline->GetSelectedNode() == &node;

	// Draw Other ImGUI elements starting from here
	ImGui::SetCursorScreenPos( area.Min );

	if( node.GetCustomNode() && node.Flags.test( ImTimelineNodeFlags_CustomDraw ) ) 
	{
		node.GetCustomNode()->OnDraw( node, area, bSelected );
		return;
	}

	ImU32 bgColor = node.DisplayProperties.BackgroundColor;
	ImU32 bgColor2 = node.DisplayProperties.BackgroundColorTwo;

	if( node.Flags.test( ImTimelineNodeFlags_UseSectionBackground ) ) 
	{
		bgColor = timeline->GetSectionDisplayProperties( node.GetSection() ).BackgroundColor;
		bgColor2 = timeline->GetSectionDisplayProperties( node.GetSection() ).BackgroundColorTwo;
	}
	if( area.Contains( timeline->GetLastInputData().MousePos ) ) 
	{
		bgColor += 0x00201000;
	}

	const ImU32 gradientStart = bgColor;
	const ImU32 gradientEnd = bgColor + 0xFF402000;

	const f32 borderRadius = node.DisplayProperties.BorderRadius;
	const ImU32 fgColor = bSelected ? timeline->m_Style.SelectedNodeOutlineColor : node.DisplayProperties.ForegroundColor;

	auto* pDrawList = ImGui::GetWindowDrawList();
	const ImRect outlineRect = ImRect( area.Min, area.Max );

	pDrawList->AddRectFilledMultiColor( outlineRect.Min, outlineRect.Max, gradientStart, gradientStart, gradientEnd, gradientEnd );
	pDrawList->AddText( area.Min + ImVec2( ( f32 ) node.DisplayProperties.AccentThickness, ( f32 ) node.DisplayProperties.AccentThickness ), node.DisplayProperties.ForegroundColor, node.DisplayText.c_str() );

	// TODO: ADD Text Clipping when text goes out of scope
	const f32 borderThickness = node.DisplayProperties.BorderThickness;
	if( bSelected ) 
		pDrawList->AddRect( outlineRect.Min, outlineRect.Max - ImVec2( borderThickness, borderThickness ), timeline->m_Style.SelectedNodeOutlineColor, borderRadius, 0, borderThickness );
	else 
		pDrawList->AddRect( outlineRect.Min, outlineRect.Max - ImVec2( borderThickness, borderThickness ), fgColor, borderRadius, 0, borderThickness );

	ImGui::Dummy( area.GetSize() );
}

void HorizontalNodeView::PerformanceDebugUI() const
{
	ImGui::Text( "Nodes Draw Skipped: %u", m_NodesDrawSkipped );
}

void HorizontalNodeView::DrawLegendArea( const TimelineSection& timeline, ImTimeline::Timeline* pContext, const ImRect& area )
{
	std::string label;
	ImTimelineUtility::sprint_f( label, "[%d] (%s)", timeline.ID, timeline.mProps.SectionName.c_str() );

	auto* pDrawList = ImGui::GetWindowDrawList();
	
	const ImVec2 tpos( area.Min.x + 3, area.Min.y );
	pDrawList->AddText( tpos, 0xFFFFFFFF, label.c_str() );
}

// TODO ScrollBar/Header draw functions here
