#include "ImDataControllerVector.h"
#include "../Core/ImTimelineLog.h"
#include <algorithm>

namespace ImTimeline {

	void VectorContainer::Iterate( const std::function<void( TimelineNode& )>& func )
	{
		for( TimelineNode& element : m_Container )
		{
			func( element );
		}
	}

	void VectorContainer::PerformanceDebugUI() const
	{
		size_t totalSizeBytes = 0;
		const size_t sizeNode = sizeof( TimelineNode );

		size_t nodeCount = 0;
		size_t nodeCountAllocated = 0;

		const size_t maxCapacity = m_Container.capacity();
		const size_t vectorSize = sizeof( m_Container );
		const size_t totalSize = vectorSize + ( sizeNode * maxCapacity );

		nodeCount += m_Container.size();
		nodeCountAllocated += maxCapacity;

		totalSizeBytes += totalSize;

		const double sizeInKB = static_cast< double >( totalSizeBytes ) / 1024;

		ImGui::Text( "Timeline Data Size: %.2f KiB", sizeInKB );
		ImGui::SameLine();
		ImGui::Text( "Allocated Node Count: %d", ( s32 ) nodeCountAllocated );
		ImGui::SameLine();
		ImGui::Text( "Displayed Node Count: %d", ( s32 ) nodeCount );
	}

	VectorContainer::~VectorContainer()
	{
		m_Container.clear();
	}

	int VectorContainer::FixOverlap( const NodeInitDescriptor& notused )
	{
		auto start = m_Container.begin();
		auto end = m_Container.end();
		std::sort( start, end, []( const TimelineNode& a, const TimelineNode& b ) { return a.Start < b.Start; } );

		for( auto it = m_Container.begin(); it != m_Container.end(); )
		{
			if( it->Start < 0 ) 
			{
				const int offsetFrom0 = 0 - it->Start;

				it->Start = 0;
				it->End += offsetFrom0;
			}

			const int endPrevious = it->End;

			++it;

			if( it != m_Container.end() && it->Start < endPrevious )
			{
				const int duration = it->End - it->Start;

				auto& newNode = it;
				newNode->Start = endPrevious + 1;
				newNode->End = it->Start + duration;
			}
		}

		return 0;
	}

	TimelineNode& VectorContainer::EmplaceBackDirect( TimelineNode& newElement, const NodeInitDescriptor& descriptor /* = NodeInitDescriptor() */ )
	{
		if( m_Container.size() >= m_Container.capacity() )
		{
			IM_ASSERT( false );
		}

		TimelineNode* pLastInsertedNode = nullptr;

		// empty or back
		if( m_Container.size() == 0 || m_Container.back().Start < newElement.Start ) 
		{
			auto& newData = m_Container.emplace_back( newElement );
			pLastInsertedNode = &newData;
		}

		// front insert
		if( pLastInsertedNode == nullptr && m_Container.begin()->Start > newElement.Start )
		{
			m_Container.insert( m_Container.begin(), newElement );
			pLastInsertedNode = &*m_Container.begin();
		}

		// middle insert
		if( pLastInsertedNode == nullptr ) 
		{
			auto itInsert = m_Container.begin();

			for( auto it = m_Container.begin(); it != m_Container.end(); ++it ) 
			{
				if( it->Start < newElement.Start ) 
				{
					itInsert = it;
				}
				else 
				{
					break;
				}
			}

			if( m_Container.size() > 1 && itInsert != m_Container.end() ) 
			{
				++itInsert;
				IM_ASSERT( itInsert != m_Container.end() );
			}

			IM_ASSERT( m_Container.capacity() > m_Container.size() );
			m_Container.insert( itInsert, newElement );
		
			// TODO there's nothing wrong with it but I dont like this
			pLastInsertedNode = &*itInsert; 
		}

		IM_ASSERT( pLastInsertedNode != nullptr );

		if( descriptor.bMoveOverlappingNext )
		{
			NodeInitDescriptor descriptor;
			descriptor.Start = 0;

			FixOverlap( descriptor );
		}

		LOG_INFO_PRINTF( "Emplaced node ID %d in section %d (start %d)", ( s32 ) pLastInsertedNode->GetID(), pLastInsertedNode->GetSection(), pLastInsertedNode->Start );

		return *pLastInsertedNode;

		// MEMO: Iterator might become invalidated. As long as the vector is sized, it should be good
		// It would be safer to research the vector for the last inserted element and return that.
	}

	int VectorContainer::Rebuild( const NodeInitDescriptor& descriptor )
	{
		return FixOverlap( descriptor );
	}

	int VectorContainer::DeleteNode( const NodeInitDescriptor& descriptor )
	{
		const int start = descriptor.Start;
		const int end = descriptor.End;

		int deleteCount = 0;

		for( auto it = m_Container.begin(); it != m_Container.end(); ) 
		{
			if( it->Start >= start && it->End <= end ) 
			{
				LOG_INFO_PRINTF( "Deleted node ID %d in section %d (start %d)", ( s32 ) it->GetID(), it->GetSection(), it->Start );

				it = m_Container.erase( it );
				++deleteCount;
			}
			else 
			{
				++it;
			}
		}

		if( deleteCount == 0 ) 
		{
			// TODO rare chance that descriptor.section is not the actual section ID of this container?
			LOG_INFO_PRINTF( "Trying to delete a node in section %d but no node was deleted...", descriptor.Section );
		}

		return deleteCount;
	}

	TimelineNode* VectorContainer::GetNodeID( const NodeInitDescriptor& descriptor )
	{
		for( auto it = m_Container.begin(); it != m_Container.end(); ++it ) 
		{
			if( it->GetID() == descriptor.ID ) 
			{
				TimelineNode* pNode = &*it;
				return pNode;
			}
		}

		return nullptr;
	}

	std::vector<TimelineNode*> VectorContainer::GetNodeRange( const NodeInitDescriptor& descriptor )
	{
		std::vector<TimelineNode*> nodes;

		for( auto it = m_Container.begin(); it != m_Container.end(); ++it ) 
		{
			if( it->Start >= descriptor.Start && it->End <= descriptor.End ) 
			{
				TimelineNode* pNode = &*it;
				nodes.push_back( pNode );
			}
		}

		return nodes;
	}

}
