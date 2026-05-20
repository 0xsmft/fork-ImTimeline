#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <unordered_map>

class ScopedTimer
{
public:
	ScopedTimer( const std::string& name )
		: m_Name( name ), m_StartTime( std::chrono::high_resolution_clock::now() ) 
	{
	}

	~ScopedTimer()
	{
		const auto end = std::chrono::high_resolution_clock::now();
		const auto duration = std::chrono::duration_cast< std::chrono::microseconds >( end - m_StartTime );
		
		const std::chrono::duration<double, std::milli> durationMilli = duration;
		ScopedTimer::s_TimeMap[ m_Name ] = durationMilli.count();
	}

	static void DebugPrint();

public:
	static std::unordered_map<std::string, double> s_TimeMap;

private:
	std::string m_Name;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};
