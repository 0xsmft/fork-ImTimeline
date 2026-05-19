#pragma once
#include "../TimelineCore/TimelineDefines.h"

class ITimelinePlayerView
{
public:
	ITimelinePlayerView() = default;
   
public:
	virtual void SetTimelinePlayer(ImTimeline::TimelinePlayer* pPlayer) { m_Player = pPlayer; }
	virtual void OnTimelinePlayStart(const NodePlayProperties& rProperties = NodePlayProperties()) = 0; // timeline play
	virtual void OnNodeActivate(TimelineNode* pNode, const NodePlayProperties& rProperties = NodePlayProperties()) = 0; // node play
	virtual void OnNodeDeactivate(TimelineNode* pNode, const NodePlayProperties& rProperties = NodePlayProperties()) = 0; // node stop play
	virtual void Draw() = 0;
	virtual void OnFinalize() {}

private:
	ImTimeline::TimelinePlayer* m_Player = nullptr;
};