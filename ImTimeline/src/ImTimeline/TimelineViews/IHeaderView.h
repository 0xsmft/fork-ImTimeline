#pragma once

#include "../TimelineCore/TimelineDefines.h"

#include <iostream>

namespace ImTimeline {

	class ImTimeline;

	/**
	 * This class defines the base class for extra drawing for the _whole_ timeline header.
	 * 
	 * Please not that the IHeaderView does not contain the timeline time sections. Only the space before that.
	 * 
	 * So, it would look like this:
	 * [IHeaderView][0 --------- 10 --------- 20 --------- 30 --------- ...]
	 */
	class IHeaderView
	{
	public:
		IHeaderView() = default;
		IHeaderView( const IHeaderView& ) = delete;
		IHeaderView( IHeaderView&& ) = delete;
		IHeaderView& operator=( const IHeaderView& ) = delete;
		IHeaderView& operator=( IHeaderView&& ) = delete;
	
		virtual ~IHeaderView() { OnFinalize(); };

		virtual void DrawHeader( const ImRect& rArea, Timeline* pContext ) = 0;
		virtual void OnFinalize() {}
	};

}
