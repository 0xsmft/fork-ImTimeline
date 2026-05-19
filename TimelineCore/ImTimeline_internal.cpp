#include "ImTimeline_internal.h"

#include "../Timeline.h"
#include "../TimelineData/ImDataControllerVector.h"
#include "../TimelineViews/DebugPlayerView.h"
#include "../TimelineViews/HorizontalNodeView.h"
#include "../dependencies/include/nameof/nameof.hpp"

/*******  COMMAND LOGIC *******/

void ImTimelineInternal::AddCommand::command_do()
{
	IM_ASSERT( m_pTimeline != nullptr );

	if( m_pTimeline->HasSection( this->NewNode.GetSection() ) == false )
	{
		m_pTimeline->InitializeTimelineSection( this->NewNode.GetSection(), "Unnamed" );
	}

	TimelineNode* newNode = m_pTimeline->AddNewNode( &NewNode );

	if( newNode == nullptr )
	{
		LOG_WARNING_PRINTF( "Failed to add node %d", this->NewNode.GetID() );
		return;
	}

	// update internal data
	this->NewNode.Start = newNode->Start;
	this->NewNode.End = newNode->End;

	IM_ASSERT( this->NewNode.GetID() == newNode->GetID() ); // specified ID should not change internally

	//add? on delete, the mEndTimestamp should ideally shrink
}

void ImTimelineInternal::AddCommand::command_undo()
{
	IM_ASSERT( m_pTimeline != nullptr );
	m_pTimeline->DeleteItem( this->NewNode.GetSection(), this->NewNode.Start, this->NewNode.End );
}

//MoveNode

void ImTimelineInternal::MoveNodeCommand::command_do()
{
	IM_ASSERT( NewSectionID != -1 );
	IM_ASSERT( pNodeToMove != nullptr );

	const bool bSectionDifferent = NewSectionID != pNodeToMove->GetSection();
	const f32 nodeWidth = static_cast<f32>( pNodeToMove->End - pNodeToMove->Start );

	const s32 oldStart = pNodeToMove->Start;
	const s32 oldCat = pNodeToMove->GetSection();

	if( bSectionDifferent ) 
	{
		AddCommand addCommand( this->m_pTimeline );
		addCommand.NewNode = *pNodeToMove; //copy data -> todo assignment operator override or Clone function

		addCommand.NewNode.m_Section = NewSectionID;
		addCommand.NewNode.Start = NewStart;
		addCommand.NewNode.End = NewStart + static_cast< s32 >( round( nodeWidth ) );
		addCommand.NewNode.Flags.set( eTimelineNodeFlags::TimelineNodeFlags_MovedToDifferentTimeline, true );

		addCommand.command_do();

		m_pTimeline->SetCommandEnable( false );
		m_pTimeline->DeleteItem( pNodeToMove->m_Section, oldStart, oldStart + ( s32 ) nodeWidth ); // mNodeToMove gets deleted
		m_pTimeline->SetCommandEnable( true );

		NodeInitDescriptor searchDescriptor;
		searchDescriptor.ID = addCommand.NewNode.m_ID;
		//TimelineNode* node = this->mTimeline->mTimelines[addCommand.mNewNode.section].mNodeData->get_node_id(searchDescriptor); //todo remove
		TimelineNode* node = this->m_pTimeline->FindNodeByNodeID( addCommand.NewNode.m_Section, addCommand.NewNode.m_ID );

		if( node ) 
		{
			pNodeToMove = node;
		}
	}
	else 
	{
		pNodeToMove->Start = NewStart;
		pNodeToMove->End = NewStart + nodeWidth;

		LOG_INFO_PRINTF( "Move node on same timeline. ID: %d", pNodeToMove->m_ID );
	}

	m_pTimeline->ForceRebuild( NewSectionID );

	// For Undo
	NewStart = oldStart;
	NewSectionID = oldCat;
}

void ImTimelineInternal::MoveNodeCommand::command_undo()
{
	command_do();
}

void ImTimelineInternal::DeleteCommand::command_do()
{
	if( m_pTimeline->HasSection( Section ) == false ) 
	{
		LOG_WARNING_PRINTF( "DeleteItem section %d does not exist", Section );
		return;
	}

	NodeInitDescriptor descriptor;
	descriptor.Start = Start;
	descriptor.End = End;

	DeletedNodes.clear();

	auto nodeList = m_pTimeline->m_Timelines[ Section ].pNodeData->GetNodeRange( descriptor );
	DeletedNodes.reserve( nodeList.size() );

	for( auto node : nodeList ) 
	{
		DeletedNodes.push_back( *node );
	}

	m_pTimeline->m_Timelines[ Section ].pNodeData->DeleteNode( descriptor );
}

void ImTimelineInternal::DeleteCommand::command_undo()
{
	for( auto node : DeletedNodes )
	{
		m_pTimeline->AddNewNode( &node );
	}

	DeletedNodes.clear();
}
/****************************/

//utility
void ImTimelineInternal::ShowTimelineNodeFlagsDebugUI( TimelineNode* node )
{
	IM_ASSERT( node != nullptr );
	for( u32 i = 0; i < eTimelineNodeFlags::TimelineNodeFlags_Max; ++i )
	{
		const auto flag = static_cast< eTimelineNodeFlags >( i );
		bool bSet = node->Flags.test( flag );

		if( ImGui::Checkbox( "x", &bSet ) )
		{
			node->Flags.set( flag, bSet );
		}
	}
}

ImDataController* ImTimelineInternal::CreateDefaultDataController()
{
	auto container = new VectorContainer( ImTimelineInternal::TIMELINE_RESERVE_NODE_COUNT );
	return container;
}

std::shared_ptr<INodeView> ImTimelineInternal::CreateDefaultNodeView()
{
	return std::make_shared<HorizontalNodeView>();
}

std::shared_ptr<ITimelinePlayerView> ImTimelineInternal::CreateDefaultPlayerView()
{
	return std::make_shared<ImTimeline::DebugPlayerView>();
}
