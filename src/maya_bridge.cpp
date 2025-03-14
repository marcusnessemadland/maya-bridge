/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#include "maya_bridge.h"

#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MItDag.h>
#include <maya/MDGMessage.h>
#include <maya/MTimerMessage.h>

#include <cassert>

static void callbackNodeAdded(MObject& _node, void* _clientData)
{
	MayaBridge* bridge = (MayaBridge*)_clientData;
	assert(bridge != NULL);

	bridge->addNode(_node);
}

static void callbackTimer(float _elapsedTime, float _lastTime, void* _clientData)
{
	MayaBridge* bridge = (MayaBridge*)_clientData;
	assert(bridge != NULL);

	bridge->update();
}

void MayaBridge::addCallbacks()
{
	MStatus status;

	// Add timer callback
	m_callbackArray.append(MTimerMessage::addTimerCallback(
		0.01f,
		callbackTimer,
		this,
		&status
	));

	// Add node added callback.
	m_callbackArray.append(MDGMessage::addNodeAddedCallback(
		callbackNodeAdded,
		"dependNode",
		this,
		&status
	));

	MGlobal::displayInfo("Added plugin callbacks!");
}

void MayaBridge::removeCallbacks()
{
	MMessage::removeCallbacks(m_callbackArray);
	MGlobal::displayInfo("Removed plugin callbacks!");
}

MayaBridge::MayaBridge()
	: m_writeBuffer(NULL)
	, m_readBuffer(NULL)
{
}

MayaBridge::~MayaBridge()
{
}

MStatus MayaBridge::initializePlugin(MObject _obj)
{
	// Create plugin
	MStatus status = MS::kSuccess;
	MFnPlugin plugin = MFnPlugin(_obj, "Maya Bridge | Level Editor", "2.2", "Any", &status);

	// Initialize the shared memory
	m_writeBuffer = new SharedBuffer();
	if (!m_writeBuffer->init("maya-bridge-write", sizeof(SharedData)))
	{
		MGlobal::displayError("Failed to sync shared memory");
		return status;
	}

	m_readBuffer = new SharedBuffer();
	if (!m_readBuffer->init("maya-bridge-read", sizeof(uint32_t)))
	{
		MGlobal::displayError("Failed to sync shared memory");
		return status;
	}

	// Add callbacks
	addCallbacks();

	// Add all nodes in scene.
	addAllNodes();

	return status;
}

MStatus MayaBridge::uninitializePlugin(MObject _obj)
{
	MStatus status = MS::kSuccess;

	// Remove callbacks
	removeCallbacks();

	// Shutdown the shared memory
	m_writeBuffer->shutdown();
	delete m_writeBuffer;

	m_readBuffer->shutdown();
	delete m_readBuffer;

	// Destroy plugin
	MFnPlugin plugin(_obj);
	return status;
}

void MayaBridge::update()
{
}

void MayaBridge::addNode(MObject _obj)
{
}

void MayaBridge::addAllNodes()
{
	MItDag dagIt = MItDag(MItDag::kBreadthFirst, MFn::kInvalid);
	for (; !dagIt.isDone(); dagIt.next())
	{
		MObject node = dagIt.currentItem();
		addNode(node);
	}
}
