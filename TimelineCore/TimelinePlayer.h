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
		enum eTimelineState : uint8_t
		{
			eState_None = 0,
			eState_Stopped,
			eState_Playing,
			eState_Paused,
			eState_Finished,
			eState_Max,
		};

		TimelinePlayer();
		TimelinePlayer( const TimelinePlayer& ) = delete;
		TimelinePlayer( TimelinePlayer&& ) = delete;
		TimelinePlayer& operator=( const TimelinePlayer& ) = delete;
		TimelinePlayer& operator=( TimelinePlayer&& ) = delete;
		virtual ~TimelinePlayer();

		void Setup( ImDataController* aTimelineData, s32 aStartTimestamp );
		bool IsSetup() const { return mbIsInitialized; }
		bool IsRootTimeline() const;

		void AddPlayer( std::shared_ptr<TimelinePlayer> aPlayer ) { mPlayers.push_back( aPlayer ); }
		void SetViewUI( std::shared_ptr<ITimelinePlayerView> newView );
		std::shared_ptr<ITimelinePlayerView> GetViewUI() { return mPlayerView; }

		void Update( f32 aDeltaTime );

		void Play();
		void Pause();
		void Stop();

		bool IsPlaying() const { return mState == eState_Playing; };
		void SetStartTimestamp( s32 aStartTimestamp );

		void DrawPlayer();

		// Debug
		void OnDebugGUI();
		void OnDebugGUIPerformance();

		f32 GetCurrentTimestamp() { return mTimeStep.GetTimestamp(); }

	protected:
		eTimelineState mState = eState_None;
		void ChangeState( eTimelineState aState );
		TimelineNode* GetNextNodeToPlay();

		struct sPlayingNodeProperties
		{
			PlayingNodeState mState = PlayingNodeState::None;
		};

		TimelineNode* mPlayingNode = nullptr;
		sPlayingNodeProperties mPlayingNodeProperties;

	private:
		IDGenerator mIDGenerator;
		s32 mUniqueID = -1;
		f32 mStartTimeStamp = 0.f;
		IntTimelineTimeStep mTimeStep = IntTimelineTimeStep( 0 );
		ImDataController* mTimelineData = nullptr;

		std::vector<std::shared_ptr<TimelinePlayer>> mPlayers;
		std::shared_ptr<ITimelinePlayerView> mPlayerView;

		bool mbIsInitialized = false;
	};
}
