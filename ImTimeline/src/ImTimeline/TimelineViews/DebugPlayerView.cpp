#include "DebugPlayerView.h"

namespace ImTimeline {

	void DebugPlayerView::OnTimelinePlayStart( const NodePlayProperties& properties )
	{
	}

	void DebugPlayerView::OnNodeActivate( TimelineNode* node, const NodePlayProperties& properties )
	{
		std::string debugText;
		ImTimelineUtility::sprint_f( debugText, "Play Node: ID %d", node->GetID() );
		m_DebugTexts.push_back( debugText );
	}

	void DebugPlayerView::OnNodeDeactivate( TimelineNode* node, const NodePlayProperties& properties )
	{
		std::string debugText;
		ImTimelineUtility::sprint_f( debugText, "End Play Node: ID %d", node->GetID() );
		m_DebugTexts.push_back( debugText );
	}

	void DebugPlayerView::Draw()
	{
		for( const auto& debugString : m_DebugTexts )
		{
			ImGui::Text( "%s", debugString.c_str() );
		}
	}

}
