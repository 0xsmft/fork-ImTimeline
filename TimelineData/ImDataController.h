#pragma once

#include "../TimelineCore/TimelineDefines.h"

#include <iostream>
#include <functional>

class ImDataController
{
public:
	ImDataController() = default;
	ImDataController( const ImDataController& ) = delete;
	ImDataController( ImDataController&& ) = delete;
	ImDataController& operator=( const ImDataController& ) = delete;
	ImDataController& operator=( ImDataController&& ) = delete;
	virtual ~ImDataController() 
	{ 
		OnFinalize();
	}

public:
	virtual TimelineNode& EmplaceBackDirect( TimelineNode& newElement, const NodeInitDescriptor& descriptor = NodeInitDescriptor() ) = 0;
	virtual int DeleteNode( const NodeInitDescriptor& descriptor ) = 0;
	virtual void Iterate( const std::function<void( TimelineNode& )>& func ) = 0;
	virtual int Rebuild( const NodeInitDescriptor& descriptor ) = 0;

	virtual TimelineNode* GetNodeID( const NodeInitDescriptor& descriptor ) = 0;
	virtual std::vector<TimelineNode*> GetNodeRange( const NodeInitDescriptor& descriptor ) = 0;

	virtual void PerformanceDebugUI() const {}
	virtual void OnFinalize() {}
};
