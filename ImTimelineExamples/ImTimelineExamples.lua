project "ImTimelineExamples"
	location ""
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++23"
	staticruntime "off"
	warnings "Default"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.cpp",
		"src/**.h"
	}

	includedirs
	{
		"src",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImTimeline}"
	}

	links 
	{
		"ImGui",
		"ImTimeline"
	}
		
	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS"
		}

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        symbols "Off"
        optimize "Speed"
