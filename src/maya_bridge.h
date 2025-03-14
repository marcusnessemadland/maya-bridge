/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#pragma once

#include "maya-bridge/shared_buffer.h"
#include "maya-bridge/shared_data.h"

#include <maya/MObject.h>        
#include <maya/MStatus.h>        
#include <maya/MFnPlugin.h>      
#include <maya/MCallbackIdArray.h>

class MayaBridge
{
	void addCallbacks();
	void removeCallbacks();

public:
	MayaBridge();
	~MayaBridge();

	MStatus initializePlugin(MObject _obj);
	MStatus uninitializePlugin(MObject _obj);

	void update();

	void addNode(MObject _obj);
	void addAllNodes();

private:
	SharedBuffer* m_writeBuffer;
	SharedBuffer* m_readBuffer;
	SharedData m_shared;

	MCallbackIdArray m_callbackArray;
};
