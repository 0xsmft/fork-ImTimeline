#pragma once

#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineLog.h"

#include <bitset>

#define IMTIMELINE_VERSION_STR "0.2.0 WIP"
#define IMTIMELINE_VERSION_NUM 002010

enum TimelineFlags : u8
{
	TimelineFlags_None = 0,
	TimelineFlags_SkipTimelineRebuild = 1 << 0,

	TimelineFlags_Max,
};

enum TimelineSectionFlags : u8
{
	TimelineSectionFlags_None = 0,

	TimelineSectionFlags_Max,
};

typedef int TimelineNodeFlags;

enum eTimelineNodeFlags : u8
{
	TimelineNodeFlags_None = 0,
	TimelineNodeFlags_CustomDraw = 1 << 0,
	TimelineNodeFlags_AutofitHeight = 1 << 2, // Display a node with its height fit to the timeline size
	TimelineNodeFlags_UseSectionBackground = 1 << 3,
	TimelineNodeFlags_MoveSurroundingNodesToTheRight = 1 << 4, // On insertion or moving, move the surrounding nodes to the right to make this node fit
	TimelineNodeFlags_MovedToDifferentTimeline = 1 << 5,

	TimelineNodeFlags_Max
};

enum TimelineDisplayFlags : u8
{
	None,
	Max,
};

enum TimelineSectionDisplayFlags : u8
{
	TimelineSectionDisplayFlagsNone = 0,
	TimelineSectionDisplayFlagsMax,
};

enum class PlayingNodeState : u8
{
	None = 0,

	IsPlayed = 1 << 0,
	IsFinishedPlaying = 1 << 1,
	IsPaused = 1 << 2,

	Max = 3
};

constexpr int STACK_INTERVAL_THRESHOLD = 1; // unused

typedef s32 NodeID;
static constexpr NodeID INVALID_NODE_ID = -1;

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

class CustomNodeBase;

namespace ImTimeline {
	class Timeline;
}

namespace ImTimelineInternal {
	class MoveNodeCommand;
}

struct TimelineNode
{
	GenericDisplayProperties DisplayProperties;
	std::bitset<eTimelineNodeFlags::TimelineNodeFlags_Max> Flags;
	std::string DisplayText;
	s32 Start = 0;
	s32 End = 0;

	NodeID GetID() const { return m_ID; }
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
		Flags.set( eTimelineNodeFlags::TimelineNodeFlags_CustomDraw, bCustomUI );
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
	NodeID m_ID = INVALID_NODE_ID;
	s32 m_Section = 0;

	friend class ::ImTimeline::Timeline;
	friend class ::ImTimelineInternal::MoveNodeCommand;
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

namespace ImTimeline {
	class TimelinePlayer;
}

struct TimelineSectionProperties 
{
	std::string SectionName;
	GenericDisplayProperties DisplayProperties;
	std::bitset<TimelineSectionDisplayFlags::TimelineSectionDisplayFlagsMax> Flags;

	s32 EndTimestamp;
	// ImRect LegendAndContentRect; omit: Calculated by ImTimeline
};

namespace ImTimeline {
	class TimelinePlayer;
}

class ImDataController;
class INodeView;
class ITimelinePlayerView;

struct TimelineSection
{
	u32 ID = -1;
	std::shared_ptr<ImTimeline::TimelinePlayer> TimelinePlayer;
	ImDataController* pNodeData = nullptr;

	std::shared_ptr<INodeView> NodeView;

	bool bIsInitialized = false;

	TimelineSectionProperties mProps;

	~TimelineSection()
	{
		OnFinalize();
	}

	void OnFinalize();

};

using TimelineDataMap = std::unordered_map<u32, TimelineSection>;

namespace ImTimeline {
	class Timeline;
}

class BaseCommand 
{
public:
	BaseCommand( ImTimeline::Timeline* aTimeline ) { m_pTimeline = aTimeline; }
	virtual ~BaseCommand()
	{
	}

	virtual void command_do() = 0;
	virtual void command_undo() = 0;

protected:
	ImTimeline::Timeline* m_pTimeline = nullptr;
};

struct NodeInitDescriptor 
{
	std::string Label;

	NodeID ID = INVALID_NODE_ID;
	int Section = 0;
	int Start = 0;
	int End = 0;

	std::shared_ptr<CustomNodeBase> CustomNode = nullptr;
	
	// if the wdith of the node is greater than the start of the next node, the next node will be moved to make place
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

struct ImTimelineStyle 
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
	DragState DragState = DragState::None;
	TimelineNode DragNode = TimelineNode();
	s32 DragStartTimestamp = -1;
	ImRect DragRect;
	ImVec2 DragStartMouseDelta;
};

enum NextAction : u8
{
	ActionNone,
	ActionDelete,
	ActionUndo,
	ActionRedo,
	ActionMax,
};

namespace ImTimelineLicense {
	static const char* LicenseNameof = R"(
MIT License

Copyright (c) 2016 - 2024 Daniil Goncharov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)";
}
