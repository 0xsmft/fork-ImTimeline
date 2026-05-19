
#include "TimelineDefines.h"

namespace ImTimelineInternal
{
	static constexpr uint32_t TIMELINE_RESERVE_NODE_COUNT = 500u;

	class AddCommand : public BaseCommand 
	{
	public:
		TimelineNode NewNode;
	
	public:
		AddCommand( ImTimeline::Timeline* pTimeline ) : BaseCommand( pTimeline ) {}
		virtual void command_do() override;
		virtual void command_undo() override;
	};

	class MoveNodeCommand : public BaseCommand 
	{
	public:
		s32 NewStart = 0;
		s32 NewSectionID = -1;
		TimelineNode* pNodeToMove = nullptr;

	public:
		MoveNodeCommand( ImTimeline::Timeline* pTimeline ) : BaseCommand( pTimeline ) {}
		virtual void command_do() override;
		virtual void command_undo() override;
	};

	class DeleteCommand : public BaseCommand 
	{
	public:
		s32 Section;
		s32 Start;
		s32 End;
		std::vector<TimelineNode> DeletedNodes;

	public:
		DeleteCommand( ImTimeline::Timeline* pTimeline ) : BaseCommand( pTimeline ) {}
		virtual void command_do() override;
		virtual void command_undo() override;
	};


	//ImTimelineUtility
	void ShowTimelineNodeFlagsDebugUI( TimelineNode* node );
	
	ImDataController* CreateDefaultDataController();
	std::shared_ptr<INodeView> CreateDefaultNodeView();
	std::shared_ptr<ITimelinePlayerView> CreateDefaultPlayerView();

}
