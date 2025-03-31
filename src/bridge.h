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
#include <unordered_map>

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
		void processMesh(Model& _model, MFnMesh& fnMesh);
		void processSubMeshes(Mesh& mesh, MFnMesh& fnMesh);

		void processMaterial(Material& _material, const MObject& _obj);
		void processStandardSurface(Material& _material, MFnDependencyNode& shaderFn);
		void processPhong(Material& _material, MFnDependencyNode& shaderFn);
		bool processTexture(const MPlug& _plug, char* _outPath);
		bool processTextureNormal(MFnDependencyNode& shaderFn, char* _outPath);

	public:
		Bridge();
		~Bridge();

		MStatus initialize();
		MStatus uninitialize();

		void update();
		void updateCamera(const MString& _panel);

		void addModel(const MObject& _obj);
		void removeModel(const MObject& _obj);
		void addAllModels();

		void addMaterial(const MObject& _obj);
		void removeMaterial(const MObject& _obj);
		void addAllMaterials();

		void save();

	private:
		SharedBuffer* m_writeBuffer;
		SharedBuffer* m_readBuffer;
		SharedData m_shared;

		MCallbackIdArray m_callbackArray;

		std::queue<MObject> m_queueModelAdded;
		std::queue<MObject> m_queueModelRemoved;

		std::queue<MObject> m_queueMaterialAdded;
		std::queue<MObject> m_queueMaterialRemoved;
	};

} // namespace mb