
#include "TimelineDefines.h"

namespace ImTimeline::Internal
{
	static constexpr uint32_t TIMELINE_RESERVE_NODE_COUNT = 500u;

	class AddCommand : public BaseCommand 
	{
	public:
		TimelineNode NewNode;
	
	public:
		AddCommand( Timeline* pTimeline ) 
			: BaseCommand( pTimeline ) 
		{
		}

		virtual void CommandDo() override;
		virtual void CommandUndo() override;
	};

	class MoveNodeCommand : public BaseCommand 
	{
	public:
		s32 NewStart = 0;
		s32 NewSectionID = -1;
		TimelineNode* pNodeToMove = nullptr;

	public:
		MoveNodeCommand( Timeline* pTimeline )
			: BaseCommand( pTimeline )
		{
		}

		virtual void CommandDo() override;
		virtual void CommandUndo() override;
	};

	class DeleteCommand : public BaseCommand 
	{
	public:
		s32 Section = -1;
		s32 Start = 0;
		s32 End = 0;
		std::vector<TimelineNode> DeletedNodes;

	public:
		DeleteCommand( Timeline* pTimeline ) 
			: BaseCommand( pTimeline ) 
		{
		}

		virtual void CommandDo() override;
		virtual void CommandUndo() override;
	};


	//ImTimelineUtility
	void ShowTimelineNodeFlagsDebugUI( TimelineNode* node );
	
	ImDataController* CreateDefaultDataController();
	std::shared_ptr<INodeView> CreateDefaultNodeView();
	std::shared_ptr<ITimelinePlayerView> CreateDefaultPlayerView();

}
