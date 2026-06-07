#include "TimelineNameHeaderView.h"

#include "../Core/CoreDefines.h"
#include "../Core/ImTimelineUtility.h"
#include "../TimelineData/ImDataController.h"
#include "../Timeline.h"

namespace ImTimeline {

	void TimelineHeaderNameView::DrawHeader( const ImRect& rArea, Timeline* pContext )
	{
		ImGui::SetCursorScreenPos( rArea.Min );

		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		pDrawList->AddText( rArea.Min, ImTimelineColor::White, m_Name.c_str() );
	
		const auto textSize = ImGui::CalcTextSize( m_Name.c_str() );
		ImGui::Dummy( textSize );
	}

	void TimelineHeaderNameView::OnFinalize()
	{
	}
	
}
