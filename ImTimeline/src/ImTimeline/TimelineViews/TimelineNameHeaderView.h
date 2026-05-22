#pragma once

#include "IHeaderView.h"

namespace ImTimeline {

	/**
	 * TimelineHeaderNameView
	 *
	 * Draws the timeline's name in the header region.
	 *
	 */
	class TimelineHeaderNameView : public IHeaderView
	{
	public:
		TimelineHeaderNameView( const std::string& rName )
			: m_Name( rName )
		{
		}

		virtual ~TimelineHeaderNameView() = default;

	public:
		// IHeaderView
		virtual void DrawHeader( const ImRect& rArea, Timeline* pContext ) override;
		virtual void OnFinalize() override;

	private:
		std::string m_Name;
	};

}
