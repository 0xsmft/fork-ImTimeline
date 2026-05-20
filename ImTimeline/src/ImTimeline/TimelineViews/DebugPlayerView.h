#pragma once

#include "../TimelineCore/TimelinePlayer.h"

namespace ImTimeline
{
	class DebugPlayerView : public ITimelinePlayerView
	{
	public:
		DebugPlayerView() : ITimelinePlayerView()
		{
		}

	public:
		virtual void OnTimelinePlayStart( const NodePlayProperties& rProperties = NodePlayProperties() ); // timeline play
		virtual void OnNodeActivate( TimelineNode* pNode, const NodePlayProperties& rProperties = NodePlayProperties() ); // node play
		virtual void OnNodeDeactivate( TimelineNode* pNode, const NodePlayProperties& rProperties = NodePlayProperties() );// node stop play
		virtual void Draw();

	private:
		std::vector<std::string> m_DebugTexts;
	};
}
