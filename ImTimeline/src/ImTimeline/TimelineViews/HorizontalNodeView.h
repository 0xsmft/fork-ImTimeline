#pragma once

#include "../TimelineViews/INodeView.h"

namespace ImTimeline {

	/**
	 * HorizontalNodeView
	 *
	 * Default node view + header and support for dragging nodes.
	 *
	 * Draws nodes in a horizontal line.
	 */
	class HorizontalNodeView : public INodeView
	{
	public:
		HorizontalNodeView() = default;
		virtual ~HorizontalNodeView() = default;

	public:
		// INodeView interface
		virtual void PreDraw() override;
		virtual void DrawNodeView( const ImRect& rArea, const TimelineSection& rTimeline, Timeline* pContext ) override;
		virtual void DefaultNodeDraw( const ImRect& rArea, const TimelineNode& rNode, Timeline* pTimeline ) override;
		virtual void PerformanceDebugUI() const override;

	public:
		virtual void DrawLegendArea( const TimelineSection& rTimeline, Timeline* pContext, const ImRect& area );

	private:
		u32 m_NodesDrawSkipped = 0u;
	};	
}
