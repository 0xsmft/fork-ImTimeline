#pragma once

#include "ImDataController.h"
#include <vector>

namespace ImTimeline {

	// template <typename T>
	class VectorContainer : public ImDataController
	{
	public:
		VectorContainer( size_t max_capacity )
			: ImDataController()
		{
			m_Container.reserve( max_capacity );
		}
		virtual ~VectorContainer() override;

	public:
		int FixOverlap( const NodeInitDescriptor& notused );

	public:
		virtual void Iterate( const std::function<void( TimelineNode& )>& func ) override;
		virtual TimelineNode& EmplaceBackDirect( TimelineNode& node, const NodeInitDescriptor& descriptor = NodeInitDescriptor() ) override;

		virtual int Rebuild( const NodeInitDescriptor& descriptor ) override;
		virtual int DeleteNode( const NodeInitDescriptor& descriptor ) override;
		virtual TimelineNode* GetNodeID( const NodeInitDescriptor& descriptor ) override;
		virtual std::vector<TimelineNode*> GetNodeRange( const NodeInitDescriptor& descriptor ) override;

		virtual void PerformanceDebugUI() const override;

	private:
		std::vector<TimelineNode> m_Container;
	};

}
