#pragma once

#include "../Core/CoreDefines.h"
#include <chrono>

//TODO use this more for step progression everywhere

namespace ImTimeline
{
	class TimelineTimeStep
	{
	public:
		TimelineTimeStep() {}

		virtual void Update( f32 aDeltaTime ) = 0;
		virtual inline f32 GetTimestamp() const = 0;
	};

	class IntTimelineTimeStep : TimelineTimeStep
	{
	public:
		IntTimelineTimeStep() 
			: m_Timestamp( 0 )
		{
			m_LastUpdateTime = std::chrono::steady_clock::now();
		}

		IntTimelineTimeStep( s32 start ) 
			: m_Timestamp( start )
		{
		}

		void Update( f32 aDeltaTime ) override
		{
			if( m_FirstUpdate == false )
			{
				m_FirstUpdate = true;
				m_LastUpdateTime = std::chrono::steady_clock::now();
			}

			auto currentTime = std::chrono::steady_clock::now();
			std::chrono::duration<float> elapsed = currentTime - m_LastUpdateTime;

			if( elapsed.count() >= 1.0f )
			{
				++m_Timestamp;
				m_LastUpdateTime = currentTime;
			}
		}

		virtual inline f32 GetTimestamp() const override
		{
			return ( f32 ) m_Timestamp;
		}

		void SetTimestamp( s32 aTimestamp )
		{
			m_Timestamp = aTimestamp;
		}

	private:
		std::chrono::steady_clock::time_point m_LastUpdateTime;
		s32 m_Timestamp = 0;
		bool m_FirstUpdate = false;
	};
}
