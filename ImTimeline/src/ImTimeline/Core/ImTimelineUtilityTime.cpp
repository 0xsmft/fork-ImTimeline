#include "ImTimelineUtilityTime.h"
#include <imgui.h>

void ScopedTimer::DebugPrint()
{
	for( const auto& [name, time] : ScopedTimer::s_TimeMap )
	{
		ImGui::Text( "%s: %.8fms", name.c_str(), time );
	}
}

std::unordered_map<std::string, double> ScopedTimer::s_TimeMap;
