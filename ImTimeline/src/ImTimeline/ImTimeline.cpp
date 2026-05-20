#include "ImTimeline.h"
#include "Timeline.h"

namespace ImTimeline
{

void BeginTimeline(const char* str_id, const ImTimelineStyle& style)
{
	auto it = GImmediateData.TimelineDataMap.find(str_id);
	if (it != GImmediateData.TimelineDataMap.end()) {
		GImmediateData.pCurrentTimelineData = &it->second;
	} else {
		Timeline* newTimeline = new Timeline();
		GImmediateData.TimelineDataMap[str_id].pTimelineObject = newTimeline;
		GImmediateData.TimelineDataMap[str_id].IDString = str_id;
		GImmediateData.pCurrentTimelineData = &GImmediateData.TimelineDataMap[str_id];
	}
	GImmediateData.pCurrentTimelineData->pTimelineObject->SetTimelineStyle(style);
	GImmediateData.TimelineDataMap[str_id].Flags.set(ImmediateModeTimelineData::eImmediateFlags::BeginTimelineCalled, true);
}

TimelineNode* BeginTimelineContent(NodeID UUID, const std::string& label, s32 section, s32 start, s32 end, std::shared_ptr<CustomNodeBase> customNode)
{
	NodeInitDescriptor descriptor(label, section, start, end, customNode);
	descriptor.ID = UUID;
	return BeginTimelineContent(descriptor);
}


/// @brief Adds a new node to the active timeline, only if the ID hasn't been added before
/// @param nodeDescriptor
/// @return
TimelineNode* BeginTimelineContent(const NodeInitDescriptor& nodeDescriptor)
{
	if (GImmediateData.pCurrentTimelineData == nullptr) {
		BeginTimeline("Timeline");
		return nullptr;
	}

	IM_ASSERT(GImmediateData.pCurrentTimelineData->pTimelineObject != nullptr);

	bool bBeginCalled = GImmediateData.pCurrentTimelineData->Flags.test(ImmediateModeTimelineData::eImmediateFlags::BeginTimelineCalled);

	if (bBeginCalled == false) {
		::ImTimeline::BeginTimeline(GImmediateData.pCurrentTimelineData->IDString.c_str(), GImmediateData.pCurrentTimelineData->pTimelineObject->m_Style);
	}
	auto& mAddedDescriptors = GImmediateData.pCurrentTimelineData->AddNodeMap;
	if (mAddedDescriptors.find(nodeDescriptor.ID) != mAddedDescriptors.end()) {
		// We've already added this node. It might already been deleted by the user, but regardless never add it again programmatically
		return nullptr;
	}

	mAddedDescriptors.emplace(nodeDescriptor.ID, true);

	TimelineNode newNode;
	newNode.Setup(nodeDescriptor.Section, nodeDescriptor.Start, nodeDescriptor.End, nodeDescriptor.Label);
	bool bShowCustomUI = true;
	newNode.InitalizeCustomNode(nodeDescriptor.CustomNode, bShowCustomUI);

	// Create new internal node
	TimelineNode* nodeInstance = GImmediateData.pCurrentTimelineData->pTimelineObject->AddNewNode(&newNode);

	return nodeInstance;
}

void ShowTimelineDebugUI(const char* str_id)
{
	if (GImmediateData.pCurrentTimelineData == nullptr)
		return;

	GImmediateData.pCurrentTimelineData->pTimelineObject->DrawDebugGUI();
}

ImTimelineStyle& GetTimelineStyle()
{
	static ImTimelineStyle dummyStyle = ImTimelineStyle();
	if(GImmediateData.pCurrentTimelineData == nullptr)
		return dummyStyle;
	
	IM_ASSERT(GImmediateData.pCurrentTimelineData->pTimelineObject != nullptr);

	return GImmediateData.pCurrentTimelineData->pTimelineObject->m_Style;
}

GenericDisplayProperties& GetTimelineDisplayProperties(s32 section_id)
{
	static GenericDisplayProperties dummy = GenericDisplayProperties();
	if(GImmediateData.pCurrentTimelineData == nullptr)
		return dummy; //NG C++ 17: std::optional<GenericDisplayProperties&>
	
	IM_ASSERT(GImmediateData.pCurrentTimelineData->pTimelineObject != nullptr);

	return GImmediateData.pCurrentTimelineData->pTimelineObject->GetSectionDisplayProperties(section_id);
}

void SetTimelineName(s32 section_id, std::string name)
{
	  if(GImmediateData.pCurrentTimelineData == nullptr)
		return;
	
	IM_ASSERT(GImmediateData.pCurrentTimelineData->pTimelineObject != nullptr);

	return GImmediateData.pCurrentTimelineData->pTimelineObject->SetTimelineName(section_id, name);
}

void ShowActiveTimeline()
{
	if (GImmediateData.pCurrentTimelineData == nullptr)
		return;

	if(GImmediateData.pCurrentTimelineData->pTimelineObject)
		GImmediateData.pCurrentTimelineData->pTimelineObject->DrawTimeline();
}

void SetTimelineProperties(s32 section_id, const TimelineSectionProperties& properties)
{
	if (GImmediateData.pCurrentTimelineData == nullptr)
		return;

	GImmediateData.pCurrentTimelineData->pTimelineObject->InitializeTimelineSection(section_id, properties.SectionName.c_str());
	GImmediateData.pCurrentTimelineData->pTimelineObject->GetTimelineSection(section_id).mProps = properties;
}

} // namespace ImTimeline
