#pragma once

#include "TimelineTimeStep.h"
#include "TimelineDefines.h"

#include "../Core/ImTimelineUtility.h"
#include "../Core/IDGeneratorUtility.h"
#include "../TimelineViews/ITimelinePlayerView.h"

class ImDataController;
struct TimelineNode;

namespace ImTimeline
{
	class TimelinePlayer
	{
	public:
		enum TimelineState : u8
		{
			eState_None = 0,
			eState_Stopped,
			eState_Playing,
			eState_Paused,
			eState_Finished,
			eState_Max,
		};

	public:
		TimelinePlayer();
		TimelinePlayer( const TimelinePlayer& ) = delete;
		TimelinePlayer( TimelinePlayer&& ) = delete;
		TimelinePlayer& operator=( const TimelinePlayer& ) = delete;
		TimelinePlayer& operator=( TimelinePlayer&& ) = delete;
		virtual ~TimelinePlayer();

	public:
		void Setup( ImDataController* aTimelineData, s32 aStartTimestamp );
		bool IsSetup() const { return m_IsInitialized; }
		bool IsRootTimeline() const;

		void AddPlayer( std::shared_ptr<TimelinePlayer> aPlayer ) { m_Players.push_back( aPlayer ); }
		void SetViewUI( std::shared_ptr<ITimelinePlayerView> newView );
		std::shared_ptr<ITimelinePlayerView> GetViewUI() { return m_PlayerView; }

		void Update( f32 aDeltaTime );

		void Play();
		void Pause();
		void Stop();

		bool IsPlaying() const { return m_State == eState_Playing; };
		void SetStartTimestamp( s32 aStartTimestamp );

		void DrawPlayer();

		// Debug
		void OnDebugGUI();
		void OnDebugGUIPerformance();

		f32 GetCurrentTimestamp() { return m_TimeStep.GetTimestamp(); }

	protected:
		TimelineState m_State = eState_None;

		void ChangeState( TimelineState aState );
		TimelineNode* GetNextNodeToPlay();

		struct PlayingNodeProperties
		{
			PlayingNodeState m_State = PlayingNodeState::None;
		};

		PlayingNodeProperties m_PlayingNodeProperties;
		TimelineNode* m_PlayingNode = nullptr;

	private:
		IDGenerator m_IDGenerator;
		s32 m_UniqueID = -1;
		f32 m_StartTimeStamp = 0.f;
		IntTimelineTimeStep m_TimeStep = IntTimelineTimeStep( 0 );
		ImDataController* m_TimelineData = nullptr;

		std::vector<std::shared_ptr<TimelinePlayer>> m_Players;
		std::shared_ptr<ITimelinePlayerView> m_PlayerView;

		bool m_IsInitialized = false;
	};
}
