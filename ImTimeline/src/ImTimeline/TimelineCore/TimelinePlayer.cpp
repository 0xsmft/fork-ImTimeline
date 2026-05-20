
#include "TimelinePlayer.h"
#include "../Core/ImTimelineLog.h"
#include "../TimelineData/ImDataController.h"

/******
 ImTimeline::TimelinePlayer
 author: nick
 =========================
 - Play nodes in sequence from a single timeline, or multiple child timelines, with state tracking.
  Supports custom UI through the ITimelinePlayerView interface, has no UI by default.
  It's up to the user to provide a play/pause/stop UI controls.

  FIXME: This needs a ready check feature, a continuous node play feature and more event propagation to be more versatile
  FIXME: No hard differentiation between root timeline and child timelines? See IsRootTimeline();
 */

ImTimeline::TimelinePlayer::TimelinePlayer()
{
	m_UniqueID = m_IDGenerator.GetUniqueID();
}

ImTimeline::TimelinePlayer::~TimelinePlayer()
{
	//mTimelineData should be cleaned up higher-up
}

void ImTimeline::TimelinePlayer::Setup( ImDataController* aTimelineData, s32 aStartTimestamp )
{
	m_TimeStep = IntTimelineTimeStep( aStartTimestamp );
	m_TimelineData = aTimelineData;
	m_PlayingNode = nullptr;
	m_IsInitialized = true;
	ChangeState( TimelineState::eState_None );
}

bool ImTimeline::TimelinePlayer::IsRootTimeline() const
{
	return m_TimelineData == nullptr && m_Players.size() > 0;
}

void ImTimeline::TimelinePlayer::SetViewUI( std::shared_ptr<ITimelinePlayerView> newView )
{
	m_PlayerView = newView;
}

void ImTimeline::TimelinePlayer::Update( f32 aDeltaTime )
{
	if( m_State != TimelineState::eState_Playing )
		return;

	m_TimeStep.Update( aDeltaTime );

	bool bChildTimelineFinished = true;

	for( auto ptr_player : m_Players ) 
	{
		auto player = ptr_player;

		if( player == nullptr )
			continue;

		player->Update( aDeltaTime );

		if( player->m_State != TimelineState::eState_Finished )
			bChildTimelineFinished = false;
	}

	// everything finished playing and self has no timeline attached
	if( bChildTimelineFinished && m_TimelineData == nullptr ) 
	{
		LOG_INFO( "All timelines finished Playing" );
		m_State = TimelineState::eState_Finished;
	}

	if( m_TimelineData == nullptr )
		return;

	if( m_PlayingNode == nullptr ) 
	{
		m_PlayingNode = GetNextNodeToPlay();
		m_PlayingNodeProperties.m_State = PlayingNodeState::None;

		if( m_PlayingNode == nullptr ) 
		{
			m_State = TimelineState::eState_Finished;
			return;
		}
	}

	if( m_PlayingNode && m_TimeStep.GetTimestamp() >= ( f32 ) m_PlayingNode->Start ) 
	{
		if( m_PlayingNodeProperties.m_State == PlayingNodeState::None ) 
		{
			// play node
			if( m_PlayingNode->GetCustomNode() ) 
			{
				m_PlayingNode->GetCustomNode()->OnNodeActivate();
			}

			if( m_PlayerView ) 
			{
				NodePlayProperties nodePlayProperties; // todo current timestamp and metadata
				m_PlayerView->OnNodeActivate( m_PlayingNode, nodePlayProperties );
			}

			m_PlayingNodeProperties.m_State = PlayingNodeState::IsPlayed;
		}

		if( m_PlayingNodeProperties.m_State == PlayingNodeState::IsPlayed && m_TimeStep.GetTimestamp() >= ( f32 ) m_PlayingNode->End )
		{
			//end play
			if( m_PlayingNode->GetCustomNode() ) 
			{
				m_PlayingNode->GetCustomNode()->OnNodeDeactivate();
			}

			if( m_PlayerView ) 
			{
				NodePlayProperties nodePlayProperties; // todo current timestamp and metadata
				m_PlayerView->OnNodeDeactivate( m_PlayingNode, nodePlayProperties );
			}

			m_PlayingNodeProperties.m_State = PlayingNodeState::IsFinishedPlaying;
			m_PlayingNode = nullptr;
		}
	}

	// node play logic
}

void ImTimeline::TimelinePlayer::Play()
{
	if( m_IsInitialized == false ) 
	{
		Setup( m_TimelineData, ( s32 ) m_TimeStep.GetTimestamp() );
	}

	if( m_IsInitialized == false ) 
	{
		LOG_WARNING_PRINTF( "TimelinePlayer not initialized", 0 );
		return;
	}

	if( m_State == TimelineState::eState_Playing ) 
	{
		return;
	}

	f32 timestamp = m_TimeStep.GetTimestamp();

	if( m_State == TimelineState::eState_Stopped ) 
	{
		timestamp = 0.0f;
	}

	m_TimeStep.SetTimestamp( ( s32 ) timestamp );
	m_State = TimelineState::eState_Playing;

	for( auto ptr_player : m_Players ) 
	{
		auto player = ptr_player;

		if( player == nullptr )
			continue;
		player->Play();
	}
}

void ImTimeline::TimelinePlayer::Pause()
{
	ChangeState( TimelineState::eState_Paused );
}

void ImTimeline::TimelinePlayer::Stop()
{
	ChangeState( TimelineState::eState_Stopped );

	Setup( m_TimelineData, 0 );
	// TODO fire event?
}

void ImTimeline::TimelinePlayer::SetStartTimestamp( s32 aStartTimestamp )
{
	if( m_State == TimelineState::eState_Playing )
		return;

	// We've clicked somewhere and changed the active timestep, so pause and reset the active timestep

	m_TimeStep.SetTimestamp( aStartTimestamp );
	m_State = TimelineState::eState_Paused;
	m_PlayingNode = nullptr;

	for( auto ptr_player : m_Players ) 
	{
		auto player = ptr_player;

		if( player == nullptr )
			continue;

		player->SetStartTimestamp( aStartTimestamp );
	}
}

void ImTimeline::TimelinePlayer::DrawPlayer()
{
	if( m_PlayerView ) 
	{
		m_PlayerView->Draw();
	}
	else 
	{
		ImGui::Text( "No Player View Attached:" );
	}
}

void ImTimeline::TimelinePlayer::OnDebugGUI()
{
	if( ImGui::TreeNodeEx( "Debug State" ) ) 
	{
		ImGui::Text( "IsInitialized: %d", m_IsInitialized );
		ImGui::Text( "TimelinePlayer Count: %d", ( s32 ) m_Players.size() );
		ImGui::Text( "IsRootTimeline: %d", IsRootTimeline() );
		ImGui::Dummy( ImVec2( 0.0f, 6.0f ) );
		ImGui::Text( "Play State:" );
		ImGui::Text( "- IsNoneState: %d", m_State == TimelineState::eState_None );
		ImGui::Text( "- IsStopped: %d", m_State == TimelineState::eState_Stopped );
		ImGui::Text( "- IsPaused: %d", m_State == TimelineState::eState_Paused );
		ImGui::Text( "- IsPlaying: %d", m_State == TimelineState::eState_Playing );
		ImGui::Text( "- IsFinished: %d", m_State == TimelineState::eState_Finished );

		ImGui::TreePop();
	}

	if( ImGui::TreeNodeEx( "Debug Play UI" ) ) 
	{
		if( m_PlayingNode != nullptr ) 
		{
			ImGui::Text( "Playing Node: %d", m_PlayingNode->GetID() );
		}

		if( m_State == TimelineState::eState_None || m_State == TimelineState::eState_Stopped )
		{
			if( ImGui::Button( "Play from start" ) )
			{
				Play();
			}
		}

		if( m_State == TimelineState::eState_Playing && ImGui::Button( "Stop" ) ) 
		{
			Stop();
		}

		if( m_State == TimelineState::eState_Playing && ImGui::Button( "Pause" ) ) 
		{
			Pause();
		}

		if( m_State == TimelineState::eState_Paused && ImGui::Button( "Play from current frame" ) ) 
		{
			Play();
		}
		ImGui::TreePop();
	}

	ImGui::Text( "Current Frame: %d", GetCurrentTimestamp() );

	if( m_PlayingNode != nullptr ) 
	{
		ImGui::Text( "Current/Next Playing Node: %d", m_PlayingNode->GetID() );
	}

	ImGui::Text( "Child Players: %d", static_cast< s32 >( m_Players.size() ) );
	for( auto ptr_player : m_Players ) 
	{
		auto player = ptr_player;

		if( player == nullptr )
			continue;

		ImGui::Text( "Child Player: %d Frame: %d", player->m_UniqueID, player->GetCurrentTimestamp() );
	}
}

void ImTimeline::TimelinePlayer::ChangeState( TimelineState aState )
{
	// check for transitions that are impossible
	//...

	for( auto ptr_player : m_Players ) 
	{
		auto player = ptr_player;
		if( player == nullptr )
			continue;

		player->ChangeState( aState );
	}

	m_State = aState;
}

TimelineNode* ImTimeline::TimelinePlayer::GetNextNodeToPlay()
{
	TimelineNode* result = nullptr;
	m_TimelineData->Iterate( [ this, &result ]( TimelineNode& node ) 
	{
		if( result ) 
		{
			return;
		}

		if( m_PlayingNode && m_PlayingNode->GetID() == node.GetID() ) 
		{
			return;
		}

		if( ( f32 ) node.Start > m_TimeStep.GetTimestamp() ) 
		{
			result = &node;
		}
	} );

	return result;
}