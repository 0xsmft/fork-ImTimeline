#include "TimelineNameHeaderView.h"

#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineUtility.h"
#include "../TimelineData/ImDataController.h"
#include "../Timeline.h"

namespace ImTimeline {

	void TimelineHeaderNameView::DrawHeader( const ImRect& rArea, Timeline* pContext )
	{
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		pDrawList->AddText( rArea.Min, ImTimelineColor::White, m_Name.c_str() );
	}

	void TimelineHeaderNameView::OnFinalize()
	{
	}
	
}
