#pragma once
#include "../TimelineViews/INodeView.h"

class HorizontalNodeView : public INodeView
{
public:
	HorizontalNodeView() = default;
	virtual ~HorizontalNodeView() = default;

	virtual void PreDraw() override;
	virtual void DrawNodeView( const ImRect& rArea, const TimelineSection& rTimeline, ImTimeline::Timeline* pContext ) override;
	virtual void DefaultNodeDraw( const ImRect& rArea, const TimelineNode& rNode, ImTimeline::Timeline* pTimeline ) override;
	virtual void PerformanceDebugUI() const override;
	virtual void DrawLegendArea( const TimelineSection& rTimeline, ImTimeline::Timeline* pContext, const ImRect& area );

private:
	u32 m_NodesDrawSkipped = 0u;
};
