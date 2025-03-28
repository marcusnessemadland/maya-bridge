/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#pragma once

#include "maya-bridge/shared_buffer.h"
#include "maya-bridge/shared_data.h"

#include <maya/MObject.h>        
#include <maya/MStatus.h>        
#include <maya/MString.h>        
#include <maya/MCallbackIdArray.h>

#include <queue>
#include <vector>

namespace mb
{
	class Node
	{
		Node();
	};

	class Bridge
	{
		void addCallbacks();
		void removeCallbacks();

		void processName(Model& _model, const MObject& _obj);
		void processTransform(Model& _model, const MObject& _obj);
		void processMeshes(Model& _model, const MObject& _obj);
		MStatus processMesh(Mesh& _mesh, const MObject& _shadingGroup, const MObject& _meshObj);
		MStatus processMaterial(Material& _material, const MObject& _shadingGroup);
		MStatus processTexture(const MPlug& _plug, char* _outPath);

	public:
		Bridge();
		~Bridge();

		MStatus initialize();
		MStatus uninitialize();

		void update();
		void updateCamera(const MString& _panel);

		void addNode(const MObject& _obj);
		void removeNode(const MObject& _obj);

		void addAllNodes();

		void save();

	private:
		SharedBuffer* m_writeBuffer;
		SharedBuffer* m_readBuffer;
		SharedData m_shared;

		MCallbackIdArray m_callbackArray;

		std::queue<MObject> m_queueNodeAdded;
		std::queue<MObject> m_queueNodeChanged;
		std::queue<MObject> m_queueNodeRemoved;
	};

} // namespace mb