#include "TimelineExample.h"

#include <ImTimeline/ImTimeline.h>
#include <ImTimeline/Timeline.h>

void ImTimeline::ShowDemoWindow()
{
	enum ExampleType
	{
		ExampleType_ObjectOriented = 0,
		ExampleType_Immediate
	};

	static int toggleValue = 0;
	ImGui::RadioButton( "Object Oriented", &toggleValue, ExampleType::ExampleType_ObjectOriented );
	ImGui::SameLine();
	ImGui::RadioButton( "Immediate (ImGui style)", &toggleValue, ExampleType::ExampleType_Immediate );
	ImGui::SameLine();

	static bool imguiDemoWindow = false;
	ImGui::Checkbox( "Show ImGUI demo window", &imguiDemoWindow );

	switch( toggleValue ) 
	{
		case ExampleType::ExampleType_ObjectOriented: 
		{
			ShowDemoWindowObjectOriented();
		} break;

		case ExampleType::ExampleType_Immediate: 
		{
			ShowDemoWindowImmediate();
		} break;
		default:
			break;
	}

	if( imguiDemoWindow )
		ImGui::ShowDemoWindow( &imguiDemoWindow );
}

void ImTimeline::ShowDemoWindowImmediate()
{
	ImGui::Begin( "Timeline Demo Immediate" );
	
	ShowTimelineDebugUI( "Timeline" );

	constexpr int CATEGORY_SWEETS_ID = 0;
	constexpr int CATEGORY_DRINKS_ID = 1;
	constexpr int CATEGORY_FOOD_ID = 2;

	BeginTimeline( "Timeline" );

	s32 uniqueID = 0;
	BeginTimelineContent( ++uniqueID, "Pudding", CATEGORY_SWEETS_ID, 10, 30 );
	BeginTimelineContent( ++uniqueID, "Coffee", CATEGORY_DRINKS_ID, 20, 40 );
	BeginTimelineContent( ++uniqueID, "Pasta", CATEGORY_FOOD_ID, 25, 70 );

	auto& rDisplayProps = GetTimelineDisplayProperties( CATEGORY_DRINKS_ID );
	rDisplayProps.Height = 70.0f;

	ShowActiveTimeline();

	ImGui::End();
}

void ImTimeline::ShowDemoWindowObjectOriented()
{
	static Timeline mTimeline = Timeline();
	static bool bInit = false;

	if( bInit == false )
	{
		const int CATEGORY_SWEETS_ID = 0;

		mTimeline.AddNewNode( 0, 0, 100, "Pasta" );

		mTimeline.SetTimelineName( CATEGORY_SWEETS_ID, "Animation" );
		auto& displayProps = mTimeline.GetSectionDisplayProperties( CATEGORY_SWEETS_ID );
		displayProps.Height = 70.0f;

		//        mTimeline.GetCurrentTimestamp();

		mTimeline.SetMaxFrame( 100 );

		bInit = true;
	}

	ImGui::Begin( "Timeline Demo Object Oriented" );
	
	mTimeline.DrawDebugGUI();
	mTimeline.DrawTimeline();
	
	ImGui::End();
}
