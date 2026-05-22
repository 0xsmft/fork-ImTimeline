#pragma once

#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineLog.h"

#include <bitset>

#define IMTIMELINE_VERSION_STR "0.2.0 WIP SE"

// Unique version number just for Saturn.
#define IMTIMELINE_VERSION_NUM 002011

enum ImTimelineFlags : u8
{
	ImTimelineFlags_None = 0,
	ImTimelineFlags_SkipTimelineRebuild = 1,

	ImTimelineFlags_Max,
};

enum TimelineSectionFlags : u8
{
	ImTimelineSectionFlags_None = 0,

	ImTimelineSectionFlags_Max,
};

enum TimelineNodeFlags_ : u8
{
	ImTimelineNodeFlags_None = 0,
	
	ImTimelineNodeFlags_CustomDraw = 1,
	
	// Display a node with its height fit to the timeline size
	ImTimelineNodeFlags_AutofitHeight = 2, 
	
	ImTimelineNodeFlags_UseSectionBackground = 3,
	
	// On insertion or moving, move the surrounding nodes to the right to make this node fit
	ImTimelineNodeFlags_MoveSurroundingNodesToTheRight = 4, 
	
	ImTimelineNodeFlags_MovedToDifferentTimeline = 5,
	
	ImTimelineNodeFlags_CannotBeDeleted = 6,

	ImTimelineNodeFlags_CannotBeDragged = 7,

	TimelineNodeFlags_Max
};

typedef u8 TimelineNodeFlags;

inline const char* ImTimelineNodeFlagsToString( TimelineNodeFlags flags )
{
	switch( flags )
	{
		case ImTimelineNodeFlags_None:
			return "No flags";

		case ImTimelineNodeFlags_CustomDraw:
			return "Custom Draw";

		case ImTimelineNodeFlags_AutofitHeight:
			return "Autofit Height";

		case ImTimelineNodeFlags_UseSectionBackground:
			return "Use Section Background";

		case ImTimelineNodeFlags_MoveSurroundingNodesToTheRight:
			return "Move Surrounding Nodes To The Right";

		case ImTimelineNodeFlags_MovedToDifferentTimeline:
			return "Moved To Different Timeline";

		case ImTimelineNodeFlags_CannotBeDeleted:
			return "Cannot Be Deleted";

		case ImTimelineNodeFlags_CannotBeDragged:
			return "Cannot Be Dragged";

		case TimelineNodeFlags_Max:
		default:
			break;
	}

	return "Unknown timeline node flag!";
}

enum TimelineDisplayFlags : u8
{
	ImTimelineDisplayFlags_None,
	ImTimelineDisplayFlags_Max,
};

enum TimelineSectionDisplayFlags : u8
{
	ImTimelineSectionDisplayFlagsNone = 0,
	ImTimelineSectionDisplayFlagsMax,
};

enum class PlayingNodeState : u8
{
	None = 0,

	IsPlayed = 1,
	IsFinishedPlaying = 2,
	IsPaused = 3,

	Max = 4
};

constexpr int IM_TIMELINE_STACK_INTERVAL_THRESHOLD = 1; // unused

typedef s32 ImTimelineNodeID;
static constexpr ImTimelineNodeID INVALID_NODE_ID = -1;

namespace ImTimeline {

	class Timeline;
	class TimelinePlayer;
	class ImDataController;
	class INodeView;
	class ITimelinePlayerView;
	class CustomNodeBase;

	struct GenericDisplayProperties
	{
		f32 Height = 0.0f;
		f32 Width = 0.0f;
		ImU32 BackgroundColor{};
		ImU32 BackgroundColorTwo{};
		ImU32 ForegroundColor = IM_COL32( 255, 255, 255, 255 );
		s32 AccentThickness = 8; // todo split for node drawing into text padding and node padding
		f32 Spacing = 0.0f;
		f32 BorderRadius = 0.0f;
		f32 BorderThickness = 1;
	};

	namespace Internal {
		class MoveNodeCommand;
	}

	struct TimelineNode
	{
		GenericDisplayProperties DisplayProperties;
		std::bitset<TimelineNodeFlags_Max> Flags;
		std::string DisplayText;
		s32 Start = 0;
		s32 End = 0;

		ImTimelineNodeID GetID() const { return m_ID; }
		s32 GetSection() const { return m_Section; }
		std::shared_ptr<CustomNodeBase> GetCustomNode() const { return m_CustomNode; }

		bool operator<( const TimelineNode& other ) const
		{
			return End < other.Start;
		}

		virtual ~TimelineNode() = default;

		void InitalizeCustomNode( std::shared_ptr<CustomNodeBase> node, bool bCustomUI = true )
		{
			m_CustomNode = node;
			Flags.set( ImTimelineNodeFlags_CustomDraw, bCustomUI );
		}

		void Setup( s32 a_cat, s32 a_start, s32 a_end, const std::string& a_text )
		{
			m_ID = INVALID_NODE_ID;
			m_Section = a_cat;
			Start = a_start;
			End = a_end;
			DisplayText = a_text;
		}

		virtual void OnClone( const CustomNodeBase& nodeData ) {} // TODO

	private:
		std::shared_ptr<CustomNodeBase> m_CustomNode = nullptr;
		ImTimelineNodeID m_ID = INVALID_NODE_ID;
		s32 m_Section = 0;

	private:
		friend class Timeline;
		friend class Internal::MoveNodeCommand;
	};

	class CustomNodeBase
	{
	public:
		CustomNodeBase() = default;

		virtual void OnDraw( const TimelineNode& nodeData, ImRect drawArea, bool& refIsSelected ) {}
		virtual void OnDebugGUI() {}

		virtual void OnTimelinePlayerSetup() {}; // node is about to be played
		bool IsReady() { return true; }; // node can be played
		virtual void OnNodeActivate() {}; // node play
		virtual void OnNodeDeactivate() {}; // node stop play
	};

	struct NodePlayProperties
	{
		// todo?
	};

	struct TimelineSectionProperties 
	{
		std::string SectionName;
		GenericDisplayProperties DisplayProperties;
		std::bitset<TimelineSectionDisplayFlags::ImTimelineSectionDisplayFlagsMax> Flags;

		s32 EndTimestamp;
	};

	struct TimelineSection
	{
		u32 ID = -1;
		bool bIsInitialized = false;
		std::shared_ptr<TimelinePlayer> TimelinePlayer;
		ImDataController* pNodeData = nullptr;

		std::shared_ptr<INodeView> NodeView;

		TimelineSectionProperties mProps;

		~TimelineSection()
		{
			OnFinalize();
		}

		void OnFinalize();
	};

	using TimelineDataMap = std::unordered_map<u32, TimelineSection>;

	class BaseCommand 
	{
	public:
		BaseCommand( Timeline* aTimeline ) { m_pTimeline = aTimeline; }
		virtual ~BaseCommand()
		{
		}

		virtual void CommandDo() = 0;
		virtual void CommandUndo() = 0;

	protected:
		Timeline* m_pTimeline = nullptr;
	};

	struct NodeInitDescriptor 
	{
		std::string Label;

		ImTimelineNodeID ID = INVALID_NODE_ID;
		s32 Section = 0;
		s32 Start = 0;
		s32 End = 0;

		std::shared_ptr<CustomNodeBase> CustomNode = nullptr;
	
		// if the width of the node is greater than the start of the next node, the next node will be moved to make place
		bool bMoveOverlappingNext = false; 

		NodeInitDescriptor() {}
		NodeInitDescriptor( std::string label, s32 section, s32 start, s32 end, std::shared_ptr<CustomNodeBase> customNode )
			: Label( std::move( label ) )
			, Section( section )
			, Start( start )
			, End( end )
			, CustomNode( std::move( customNode ) )
		{
		}
	};

	struct TimelineStyle
	{
		float LegendWidth = 200;
		int HeaderHeight = 15;
		int ScrollbarThickness = 12;
		unsigned int HeaderBackgroundColor = 0xFF3D3837;
		unsigned int HeaderTimeStampColor = 0xFFC4C4C4;
		ImU32 SelectedNodeOutlineColor = 0xEA7915FF;
		bool HasScrollbar = true;
		bool HasSeekbar = true;
		ImU32 SeekbarColor = 0xFF2A2AFF;
		f32 SeekbarWidth = 3.0f;
	};

	struct InputData 
	{
		ImVec2 MousePos;
		f32 MouseDownDuration = 0.0f;
		f32 MouseDownDurationLastFrame = 0.0f;
		bool LeftMouseDown = false;
		bool RightMouseDown = false;
		bool IsMovingScrollBar = false;
		f32 MouseScrollVertical = 0.0f;
		f32 ScrollSpeed = 1.0f; //TODO: allow for more granular slower scroll speeds
		s32 ScrollDirection = -1;
	};

	enum class DragState : u8
	{
		None,
		DragNode,
		DragNodeEnd
	};

	struct DragData 
	{
		TimelineNode DragNode = TimelineNode();
		s32 DragStartTimestamp = -1;
		ImRect DragRect;
		ImVec2 DragStartMouseDelta;
		DragState DragState = DragState::None;
	};

	enum ImTimelineNextAction : u8
	{
		ActionNone,
		ActionDelete,
		ActionUndo,
		ActionRedo,
		ActionMax,
	};
}
