workspace "ImTimeline"
	architecture "x64"
	startproject "ImTimelineExamples"
	warnings "Default"

	configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["ImGui"] = "../Dependencies/imgui"
IncludeDir["ImTimeline"] = "../ImTimeline/src"

-- // -Dependencies-- 
group "Dependencies"
	include "Dependencies/imgui"
-- // -Dependencies-- 

-- // -ImTimeline-- 
group "ImTimeline"
	include "ImTimeline/ImTimeline"
	include "ImTimelineExamples/ImTimelineExamples"
-- // -ImTimeline-- 
