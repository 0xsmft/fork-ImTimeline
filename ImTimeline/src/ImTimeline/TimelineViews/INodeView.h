#pragma once

#include <iostream>
#include "../TimelineCore/TimelineDefines.h"

namespace ImTimeline {

	class ImTimeline;

	class INodeView
	{
	public:
		INodeView() = default;
		INodeView( const INodeView& ) = delete;
		INodeView( INodeView&& ) = delete;
		INodeView& operator=( const INodeView& ) = delete;
		INodeView& operator=( INodeView&& ) = delete;
		virtual ~INodeView() { OnFinalize(); };

		virtual void PreDraw() {}
		virtual void OnFinalize() {}
		virtual void DrawNodeView( const ImRect& rArea, const TimelineSection& rTimeline, Timeline* pContext ) {};
		virtual void DefaultNodeDraw( const ImRect& rArea, const TimelineNode& rNode, Timeline* pContext ) {};
		virtual void PerformanceDebugUI() const {}
	};

}
