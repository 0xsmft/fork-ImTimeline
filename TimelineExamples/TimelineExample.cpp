#include "TimelineExample.h"
#include "../ImTimeline.h"
#include "../Timeline.h"
#include "../TimelineViews/CustomNodeTest.h"

void ImTimeline::ShowDemoWindow()
{
	enum eExampleType {
		ExampleType_ObjectOriented = 0,
		ExampleType_Immediate,
	};

	static int toggleValue = 0;
	ImGui::RadioButton("Object Oriented", &toggleValue, eExampleType::ExampleType_ObjectOriented);
	ImGui::SameLine();
	ImGui::RadioButton("Immediate (ImGui style)", &toggleValue, eExampleType::ExampleType_Immediate);
	ImGui::SameLine();

	static bool imguiDemoWindow = false;
	ImGui::Checkbox("Show ImGUI demo window", &imguiDemoWindow);

	switch (toggleValue) {
	case eExampleType::ExampleType_ObjectOriented: {
		ShowDemoWindowObjectOriented();
	} break;
	case eExampleType::ExampleType_Immediate: {
		ShowDemoWindowImmediate();
	} break;
	default:
		break;
	}

	if (imguiDemoWindow)
		ImGui::ShowDemoWindow(&imguiDemoWindow);
}

void ImTimeline::ShowDemoWindowImmediate()
{
	ImGui::Begin("Timeline Demo Immediate");
	ImTimeline::ShowTimelineDebugUI("Timeline");

	const int CATEGORY_SWEETS_ID = 0;
	const int CATEGORY_DRINKS_ID = 1;
	const int CATEGORY_FOOD_ID = 2;

	ImTimeline::BeginTimeline("Timeline");

	s32 uniqueID = 0;
	::ImTimeline::BeginTimelineContent(++uniqueID, "Pudding", CATEGORY_SWEETS_ID, 10, 30);
	ImTimeline::BeginTimelineContent(++uniqueID, "Coffee", CATEGORY_DRINKS_ID, 20, 40);
	ImTimeline::BeginTimelineContent(++uniqueID, "Caffee Latte", CATEGORY_DRINKS_ID, 45, 60, std::make_shared<CustomNodeTest>());
	ImTimeline::BeginTimelineContent(++uniqueID, "Pasta", CATEGORY_FOOD_ID, 25, 70);

	// ImTimeline::SetTimelineName(CATEGORY_SWEETS_ID, "Sweets");
	// ImTimeline::SetTimelineName(CATEGORY_DRINKS_ID, "Drinks");
	// ImTimeline::SetTimelineName(CATEGORY_FOOD_ID, "Food");

	auto& displayProps = ImTimeline::GetTimelineDisplayProperties(CATEGORY_DRINKS_ID);
	displayProps.Height = 70.0f;

	ImTimeline::ShowActiveTimeline();

	ImGui::End();
}

void ImTimeline::ShowDemoWindowObjectOriented()
{
	static ImTimeline::Timeline mTimeline = ImTimeline::Timeline();
	static bool bInit = false;

	if (bInit == false) 
	{
		const int CATEGORY_SWEETS_ID = 0;

		mTimeline.AddNewNode(0, 0, 100, "Pasta");

		mTimeline.SetTimelineName( CATEGORY_SWEETS_ID, "Animation");
		auto& displayProps = mTimeline.GetSectionDisplayProperties( CATEGORY_SWEETS_ID );
		displayProps.Height = 70.0f;

//        mTimeline.GetCurrentTimestamp();

		mTimeline.SetMaxFrame(100);

		bInit = true;
	}
	ImGui::Begin("Timeline Demo Object Oriented");
	mTimeline.DrawDebugGUI();
	mTimeline.DrawTimeline();
	ImGui::End();
}
