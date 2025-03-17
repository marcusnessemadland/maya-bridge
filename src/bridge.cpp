/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#include "bridge.h"

#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MItDag.h>
#include <maya/MDGMessage.h>
#include <maya/MTimerMessage.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MFnTransform.h>
#include <maya/MStreamUtils.h>
#include <maya/MMatrix.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MItMeshPolygon.h>

#include <cassert>
#include <map>

namespace mb
{
	static void callbackNodeAdded(MObject& _node, void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->addNode(_node);
	}

	static void callbackNodeRemoved(MObject& _node, void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->removeNode(_node);
	}

	static void callbackTimer(float _elapsedTime, float _lastTime, void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->update();
	}

	void Bridge::addCallbacks()
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

		m_callbackArray.append(MDGMessage::addNodeRemovedCallback(
			callbackNodeRemoved,
			"dependNode",
			this,
			&status
		));

		MStreamUtils::stdOutStream() << "Added plugin callbacks!" << "\n";
	}

	void Bridge::removeCallbacks()
	{
		MMessage::removeCallbacks(m_callbackArray);

		MStreamUtils::stdOutStream() << "Removed plugin callbacks!" << "\n";
	}

	void Bridge::addModel(const MObject& _obj)
	{
		
	}

	void Bridge::processName(Model& _model, const MObject& _obj)
	{
		MFnDagNode fnDagNode = MFnDagNode(_obj);
		strcpy_s(_model.name, fnDagNode.fullPathName().asChar());

		MStreamUtils::stdOutStream() << "Name: " << _model.name << " " << "\n";
	}

	void Bridge::processTransform(Model& _model, const MObject& _obj)
	{
		MFnDagNode fnDagNode = MFnDagNode(_obj);

		MDagPath path;
		fnDagNode.getPath(path);
		MFnTransform fnTransform = MFnTransform(path);

		MMatrix worldMatrix = path.inclusiveMatrix();
		MTransformationMatrix matrix(worldMatrix);
		
		MVector translation = fnTransform.getTranslation(MSpace::kWorld);
		_model.position[0] = (float)translation.x;
		_model.position[1] = (float)translation.y;
		_model.position[2] = (float)translation.z;

		MQuaternion rotation;
		fnTransform.getRotationQuaternion(rotation.x, rotation.y, rotation.z, rotation.w, MSpace::kWorld);
		rotation = rotation.inverse();
		_model.rotation[0] = (float)rotation.x;
		_model.rotation[1] = (float)rotation.y;
		_model.rotation[2] = (float)rotation.z;
		_model.rotation[3] = (float)rotation.w;

		double scaleArr[3];
		matrix.getScale(scaleArr, MSpace::kWorld);
		MVector scale = MVector(scaleArr[0], scaleArr[1], scaleArr[2]);
		_model.scale[0] = (float)scale.x;
		_model.scale[1] = (float)scale.y;
		_model.scale[2] = (float)scale.z;

		MStreamUtils::stdOutStream() << "Position: " << _model.position[0] << ", " << _model.position[1] << ", " << _model.position[2] << "\n";
		MStreamUtils::stdOutStream() << "Rotation: " << _model.rotation[0] << ", " << _model.rotation[1] << ", " << _model.rotation[2] << ", " << _model.rotation[3] << "\n";
		MStreamUtils::stdOutStream() << "Scale: " << _model.scale[0] << ", " << _model.scale[1] << ", " << _model.scale[2] << "\n";

	}

	void Bridge::processMeshes(Model& _model, const MObject& _obj)
	{
		MFnDagNode fnDagNode = MFnDagNode(_obj);

		// Find first valid mesh (@todo Support multiple meshes)
		MObject object;
		for (unsigned int i = 0; i < fnDagNode.childCount(); ++i)
		{
			MObject child = fnDagNode.child(i);
			if (child.hasFn(MFn::kMesh))
			{
				object = child;
				break;
			}
		}

		MStatus status;
		MFnMesh fnMesh(object, &status);
		if (status == MS::kSuccess)
		{
			std::map<std::string, Mesh> materialMeshMap;

			MObjectArray shadingGroups;
			MIntArray perFaceAssignments;
			fnMesh.getConnectedShaders(0, shadingGroups, perFaceAssignments);

			MItMeshPolygon faceIt(object);
			while (!faceIt.isDone())
			{
				int faceIdx = faceIt.index();
				int shadingGroupIdx = perFaceAssignments[faceIdx];

				if (shadingGroupIdx >= (int)shadingGroups.length())
				{
					faceIt.next();
					continue;
				}

				MFnDependencyNode shadingGroup(shadingGroups[shadingGroupIdx]);
				std::string materialName = shadingGroup.name().asChar();

				// Create a new sub-mesh for this material
				if (materialMeshMap.find(materialName) == materialMeshMap.end())
				{
					materialMeshMap[materialName] = Mesh();
				}

				Mesh& mesh = materialMeshMap[materialName];

				std::vector<uint32_t> faceIndices;

				int numVertices = faceIt.polygonVertexCount();
				for (int i = 0; i < numVertices; ++i)
				{
					int vtxIndex = faceIt.vertexIndex(i);

					Vertex vertex;
					MPoint point = faceIt.point(i, MSpace::kWorld);
					MVector normal;
					faceIt.getNormal(i, normal, MSpace::kWorld);

					// @todo
					//MVector tangent, bitangent;
					//faceIt.getTangent(i, tangent, MSpace::kWorld);
					//faceIt.getBinormal(i, bitangent, MSpace::kWorld);

					vertex.position[0] = (float)point.x;
					vertex.position[1] = (float)point.y;
					vertex.position[2] = (float)point.z;

					vertex.normal[0] = (float)normal.x;
					vertex.normal[1] = (float)normal.y;
					vertex.normal[2] = (float)normal.z;

					//vertex.tangent[0] = (float)tangent.x;
					//vertex.tangent[1] = (float)tangent.y;
					//vertex.tangent[2] = (float)tangent.z;
					//
					//vertex.bitangent[0] = (float)bitangent.x;
					//vertex.bitangent[1] = (float)bitangent.y;
					//vertex.bitangent[2] = (float)bitangent.z;

					float2 uv;
					faceIt.getUV(i, uv);
					vertex.texcoord[0] = uv[0];
					vertex.texcoord[1] = uv[1];

					faceIndices.push_back((uint32_t)mesh.numVertices);

					mesh.vertices[mesh.numVertices] = vertex;
					mesh.numVertices += 1;
				}

				for (size_t i = 1; i < faceIndices.size() - 1; i++)
				{
					mesh.indices[mesh.numIndices] = (faceIndices[0]);
					mesh.indices[mesh.numIndices + 1] = (faceIndices[i]);
					mesh.indices[mesh.numIndices + 2] = (faceIndices[i + 1]);
					mesh.numIndices += 3;
				}

				faceIt.next();
			}

			// Add all sub-meshes to the model
			for (auto& entry : materialMeshMap)
			{
				Mesh& mesh = entry.second;

				processMaterial(mesh.material, _obj); // @todo should be _obj or object?

				_model.meshes[_model.numMeshes] = entry.second;
				_model.numMeshes += 1;
			}
		}

		MStreamUtils::stdOutStream() << "Meshes: " << _model.numMeshes << "\n";
	}

	void Bridge::processMaterial(Material& _material, const MObject& _obj)
	{
		// @todo
	}

	Bridge::Bridge()
		: m_writeBuffer(NULL)
		, m_readBuffer(NULL)
	{
	}

	Bridge::~Bridge()
	{
	}

	MStatus Bridge::initialize()
	{
		MStatus status = MS::kSuccess;

		// Initialize the shared memory
		m_writeBuffer = new SharedBuffer();
		if (!m_writeBuffer->init("maya-bridge-write", sizeof(mb::SharedData)))
		{
			MStreamUtils::stdOutStream() << "Failed to sync shared memory!" << "\n";
			return status;
		}

		m_readBuffer = new SharedBuffer();
		if (!m_readBuffer->init("maya-bridge-read", sizeof(uint32_t)))
		{
			MStreamUtils::stdOutStream() << "Failed to sync shared memory!" << "\n";
			return status;
		}

		// Add callbacks
		addCallbacks();

		// Add all nodes in scene.
		addAllNodes();

		return status;
	}

	MStatus Bridge::uninitialize()
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
		return status;
	}

	void Bridge::update()
	{
		uint32_t status = UINT32_MAX;
		m_readBuffer->read(&status, sizeof(uint32_t));

		if (status & MAYABRIDGE_MESSAGE_RECEIVED)
		{
			if (!m_queueNodeAdded.empty())
			{
				MStreamUtils::stdOutStream() << "Adding model..." << "\n";

				MObject& object = m_queueNodeAdded.front();

				Model model;
				processName(model, object);
				processTransform(model, object);
				processMeshes(model, object);

				m_shared.models[m_shared.numModels] = model;
				m_shared.numModels += 1;

				m_queueNodeAdded.pop();
			}

			m_writeBuffer->write(&m_shared, sizeof(mb::SharedData));

			status = MAYABRIDGE_MESSAGE_NONE;
			m_readBuffer->write(&status, sizeof(uint32_t));
		}
	}

	void Bridge::addNode(const MObject& _obj)
	{
		if (_obj.isNull() || !_obj.hasFn(MFn::kDagNode)) 
		{
			MGlobal::displayError("Invalid MObject passed to addNode()");
			return;
		}

		bool hasMesh = false;

		MFnDagNode fnDagNode(_obj); 
		for (uint32_t ii = 0; ii < fnDagNode.childCount(); ++ii)
		{
			MObject child = fnDagNode.child(ii);
			if (child.hasFn(MFn::kMesh) || child.hasFn(MFn::kMeshData) || child.hasFn(MFn::kMeshGeom))
			{
				hasMesh = true;
			}
		}

		if (_obj.hasFn(MFn::kDagNode) && _obj.hasFn(MFn::kTransform) && hasMesh)
		{
			m_queueNodeAdded.push(_obj);
			return;
		}
	}

	void Bridge::removeNode(const MObject& _obj)
	{
		m_queueNodeRemoved.push(_obj);
	}

	void Bridge::addAllNodes()
	{
		MItDag dagIt = MItDag(MItDag::kBreadthFirst, MFn::kInvalid);
		for (; !dagIt.isDone(); dagIt.next())
		{
			MObject node = dagIt.currentItem();
			addNode(node);
		}
	}

} // namespace mb


