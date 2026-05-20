/**
 * @file   Timeline.h
 * @author Nick
 * @brief  Main context class for a single timeline group, housing its nodes
 * and an API to common features such as adding, deleting, changing properties
 * @date   2024.12
 */

#pragma once

#include "TimelineCore/TimelineDefines.h"
#include "Core/IDGeneratorUtility.h"

// ImGui
struct ImDrawList;
struct ImRect;

class ImDataController;
class ITimelinePlayerView;

namespace ImTimelineInternal {
	class MoveNodeCommand;
	class DeleteCommand;
}

namespace ImTimeline {

	class Timeline 
	{
	public:
		// Globals scoped to this timeline:
		std::bitset<TimelineFlags::TimelineFlags_Max> m_Flags;

		ImTimelineStyle m_Style;
		DragData m_DragData;
		ImRect m_ContentAreaRect;

		struct PanningProperties
		{
			ImVec2 PanningViewSource{};
			int PanningViewFrame = 0;
		} m_PanningData;

	public:
		Timeline();
		Timeline( const Timeline& ) = delete;
		Timeline( Timeline&& ) = delete;
		Timeline& operator=( const Timeline& ) = delete;
		Timeline& operator=( Timeline&& ) = delete;
		virtual ~Timeline() = default;

		bool InitializeTimelineSection( s32 index, std::string name, ImDataController* data = nullptr );
		bool InitializeTimelineSectionEx( s32 index, std::string name, ImDataController* data, std::shared_ptr<ITimelinePlayerView> playerViewVUI, std::shared_ptr<INodeView> nodeViewUI );

		TimelineSection& GetTimelineSection( s32 index );
		const TimelineSection& GetTimelineSection( s32 index ) const;

		GenericDisplayProperties& GetSectionDisplayProperties( s32 section_index );
		bool HasSection( s32 section ) const;

		void SetTimelineName( s32 index, std::string name );
		void SetTimelineHeight( s32 index, f32 height );
		void SetTimelineStyle( const ImTimelineStyle& style ) { m_Style = style; };
		void SetTimelinePlayerUI( std::shared_ptr<ITimelinePlayerView> uiView );
		void SetNodeViewUI( std::shared_ptr<INodeView> uiView );

		TimelineNode* AddNewNode( TimelineNode* node );
		TimelineNode& AddNewNode( s32 section, s32 start, s32 end, const std::string& text = "", std::shared_ptr<CustomNodeBase> customNodeUI = nullptr );
		void DeleteItem( s32 section, s32 start, s32 end );
		void DeleteSelection();
		void DeleteSection( s32 section );
		void MoveNode( TimelineNode* node, s32 newStart, s32 newSection );

		TimelineNode* FindNodeByNodeID( NodeID nodeID ) const;
		TimelineNode* FindNodeByNodeID( s32 section, NodeID nodeID ) const;

		////
		bool DrawTimeline();
		void DrawDebugGUI();

		bool IsDragging() const { return m_DragData.DragState != DragState::None; }

		void SetStartFrame( s32 frame ) { m_StartFrame = frame; }
		void SetMaxFrame( s32 frame ) { m_FrameMax = frame; }
		s32 GetMaxFrame() const { return m_FrameMax; }
		void SelectNode( TimelineNode* node ) { m_pSelectedNode = node; };
		TimelineNode* GetSelectedNode() const { return m_pSelectedNode; }

		s32 GetTimestampAtPixelPosition( f32 pixelPosition ) const;
		s32 GetPixelPositionAtTimestamp( s32 timestamp ) const;

		f32 GetScale() const { return m_Zoom; }
		void SetScale( f32 scale ) { m_Zoom = scale; }

		f32 GetStartTimestamp() const { return ( f32 ) m_StartFrame; }
		void SetStartTimestamp( s32 timestamp ) { m_StartFrame = timestamp; }
		s32 GetSelectedSection() const { return m_SelectedTimelineIndex; }
		void SetSelectedTimeline( s32 index ) { m_SelectedTimelineIndex = index; }

		const InputData& GetLastInputData() const { return m_InputData; }

		void Undo();
		void Redo();

		// Debug
		void OnCoreDebugGUI();
		void OnDebugGUITimelineList();
		void OnDebugGuiDisplayProps( GenericDisplayProperties& displayPropsRef );
		void OnDebugGUIPerformance();
		void OnDebugGUIPlayer();
		void OnDebugGUILog();
		bool OnDebugGUISelection();
		void OnDebugGUIRightSidePane();

	protected:
		void DrawSeekbarUI();
		void UpdateTimelinePlayer( f32 deltaTime );
		void ForceRebuild( s32 section, NodeInitDescriptor descriptor = NodeInitDescriptor() );
		virtual void DrawHeader( const ImRect& area );
		virtual void DrawScrollbar();
		f32 GetSeekbarPositionX();

		TimelineDataMap m_Timelines;
		std::shared_ptr<TimelinePlayer> m_MainPlayer;
		
		TimelineNode* m_pSelectedNode = nullptr;

		std::bitset<( s32 ) NextAction::ActionMax> m_NextActionFlags;

	private:
		void CollectInputData( InputData& a_outInputData, f32 aDeltaTime );
		void PushCommand( std::unique_ptr<BaseCommand> command );
		void SetCommandEnable( bool aEnable ) { m_EnableCommands = aEnable; }
		void updateSideDragLogic( f32 deltaTime );

	private:
		IDGenerator m_IDGenerator;
		s32 m_StartFrame = 0;
		s32 m_StartFrameVertical = 0;
		f32 m_Zoom = 10.f;
		f32 m_ZoomLerpTarget = 10.f;
		s32 m_FrameMin = 0;
		s32 m_FrameMax = 0;
		s32 m_VisibleFrameCount = 0;
		s32 m_SelectedTimelineIndex = -1;
		s32 m_CurrentSectionColorIndex = 0;

		TimelineNode m_EmptyDummyNode = TimelineNode();
		TimelineSection m_EmptyDummySection = TimelineSection();

		std::vector<std::unique_ptr<BaseCommand>> m_CommandHistory;
		s32 m_CommandIndex = -1;

		f32 m_EdgeMoveAmount = 0.0f;
		f32 m_EdgeMoveSpeed = 15.0f;

		InputData m_InputData;

		bool m_EnableCommands = true;

	private:
		friend class ::ImTimelineInternal::MoveNodeCommand;
		friend class ::ImTimelineInternal::DeleteCommand;
	};

} //ImTimeline
