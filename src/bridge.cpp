/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#include "bridge.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MItDag.h>
#include <maya/MDGMessage.h>
#include <maya/MUiMessage.h>
#include <maya/MSceneMessage.h>
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
#include <maya/MFnSet.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MItSelectionList.h>
#include <maya/MIntArray.h>
#include <maya/MFnNumericData.h>
#include <maya/M3dView.h>
#include <maya/MFnCamera.h>
#include <maya/MFnAttribute.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MGlobal.h>

#include <cassert>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <set>

namespace mb
{
	static void callbackNodeAdded(MObject& _node, void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->addModel(_node);
	}

	static void callbackNodeRemoved(MObject& _node, void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->removeModel(_node);
	}

	static void callbackPanelPreRender(const MString& _panel, void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->updateCamera(_panel);
	}

	static void callbackAfterSave(void* _clientData)
	{
		Bridge* bridge = (Bridge*)_clientData;
		assert(bridge != NULL);

		bridge->save();
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

		// Add node added and removed callbacks.
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

		// Added camera panel callback.
		m_callbackArray.append(MUiMessage::add3dViewPreRenderMsgCallback(
			"",
			callbackPanelPreRender,
			"modelPanel1",
			&status
		));

		// Added on saved callback.
		m_callbackArray.append(MSceneMessage::addCallback(
			MSceneMessage::kAfterSave,
			callbackAfterSave,
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

	void Bridge::processName(Model& _model, const MObject& _obj)
	{
		MFnDagNode fnDagNode = MFnDagNode(_obj);

		strcpy_s(_model.name, fnDagNode.fullPathName().asChar());

		MStreamUtils::stdOutStream() << "  Name: " << _model.name << " " << "\n";
	}

	void Bridge::processTransform(Model& _model, const MObject& _obj)
	{
		MStreamUtils::stdOutStream() << "  Processing transform..." << "\n";

		MFnTransform fnTransform(_obj);

		MDagPath dagPath;
		MDagPath::getAPathTo(_obj, dagPath);

		MMatrix worldMatrix = dagPath.inclusiveMatrix();
		MTransformationMatrix matrix(worldMatrix);

		// Extract translation
		MVector translation = matrix.getTranslation(MSpace::kWorld);
		_model.position[0] = static_cast<float>(translation.x);
		_model.position[1] = static_cast<float>(translation.y);
		_model.position[2] = static_cast<float>(translation.z);

		// Get rotation as quaternion
		MQuaternion rotationQuat = matrix.rotation();
		rotationQuat = rotationQuat.inverse();
		_model.rotation[0] =  static_cast<float>(rotationQuat.w);
		_model.rotation[1] =  static_cast<float>(rotationQuat.x);
		_model.rotation[2] =  static_cast<float>(rotationQuat.y);
		_model.rotation[3] =  static_cast<float>(rotationQuat.z);

		// Get scale
		double scale[3];
		matrix.getScale(scale, MSpace::kWorld);
		_model.scale[0] =  static_cast<float>(scale[0]);
		_model.scale[1] =  static_cast<float>(scale[1]);
		_model.scale[2] =  static_cast<float>(scale[2]);
	}

	void Bridge::processMeshes(Model& _model, const MObject& _obj)
	{
		MFnDagNode fnDagNode = MFnDagNode(_obj);

		uint32_t meshCounter = 0;
		for (uint32_t ii = 0; ii < fnDagNode.childCount(); ++ii)
		{
			MObject child = fnDagNode.child(ii);
			if (child.hasFn(MFn::kMesh) || child.hasFn(MFn::kMeshData) || child.hasFn(MFn::kMeshGeom))
			{
				MFnMesh fnMesh(child);
				processMesh(_model, fnMesh);
				break;
			}
		}
	}

	void Bridge::processMesh(Model& _model, MFnMesh& fnMesh)
	{
		MStreamUtils::stdOutStream() << "  Processing mesh..." << "\n";
		Mesh& mesh = _model.mesh;

		// Get positions
		MPointArray points;
		fnMesh.getPoints(points);

		// Get tangents & bitangents
		MFloatVectorArray tangents, bitangents;
		fnMesh.getTangents(tangents);
		fnMesh.getBinormals(bitangents);

		// Get UV sets
		MStringArray uvSetNames;
		fnMesh.getUVSetNames(uvSetNames);

		MStreamUtils::stdOutStream() << "    Found uvsets: " << uvSetNames.length() << "\n";

		// Handle vertex attributes
		mesh.numVertices = points.length();
		for (uint32_t i = 0; i < mesh.numVertices; ++i)
		{
			Vertex& vertex = mesh.vertices[i];
			vertex.position[0] = float(points[i].x);
			vertex.position[1] = float(points[i].y);
			vertex.position[2] = float(points[i].z);
			vertex.tangent[0] = tangents[i].x;
			vertex.tangent[1] = tangents[i].y;
			vertex.tangent[2] = tangents[i].z;
			vertex.bitangent[0] = bitangents[i].x;
			vertex.bitangent[1] = bitangents[i].y;
			vertex.bitangent[2] = bitangents[i].z;
		}

		// Handle per-face vertex attributes
		MItMeshPolygon faceIter(fnMesh.object());
		for (; !faceIter.isDone(); faceIter.next())
		{
			int vertexCount = faceIter.polygonVertexCount();
			for (int i = 0; i < vertexCount; ++i)
			{
				int vertexIndex = faceIter.vertexIndex(i);

				float2 uv;
				faceIter.getUV(i, uv, &uvSetNames[0]);

				MVector normal;
				faceIter.getNormal(i, normal);
				
				Vertex& vertex = mesh.vertices[vertexIndex];
				vertex.texcoord[0] = uv[0];
				vertex.texcoord[1] = 1.0f - uv[1];
				vertex.normal[0] = static_cast<float>(normal.x);
				vertex.normal[1] = static_cast<float>(normal.y);
				vertex.normal[2] = static_cast<float>(normal.z);
			}
		}

		// Extract indices and materials
		processSubMeshes(mesh, fnMesh);

		//
		MStreamUtils::stdOutStream() << "    Num Vertices: " << mesh.numVertices << "\n";
		MStreamUtils::stdOutStream() << "    Num SubMeshes: " << mesh.numSubMeshes << "\n";
		for (uint32_t ii = 0; ii < mesh.numSubMeshes; ++ii)
		{
			MStreamUtils::stdOutStream() << "      [" << ii << "] Num Indices: " <<  mesh.subMeshes[ii].numIndices << " | Material: " << mesh.subMeshes[ii].material << " \n";
		}
	}

	void Bridge::processSubMeshes(Mesh& mesh, MFnMesh& fnMesh)
	{
		MStatus status;

		// Get polygon counts and vertices
		MIntArray vertexCount, vertexIndices;
		fnMesh.getVertices(vertexCount, vertexIndices);

		// Get material per face
		MObjectArray shaders;
		MIntArray faceShaderIndices;
		fnMesh.getConnectedShaders(0, shaders, faceShaderIndices);

		std::unordered_map<int, SubMesh> subMeshMap;
		int vertexIndexOffset = 0;

		for (uint32_t faceIdx = 0; faceIdx < faceShaderIndices.length(); ++faceIdx) 
		{
			uint32_t shaderIndex = faceShaderIndices[faceIdx];
			int faceVertexCount = vertexCount[faceIdx];

			// Ensure shader index is valid
			if (shaderIndex < 0 || shaderIndex >= shaders.length()) 
			{
				continue;
			}

			// Create or get submesh for this shader
			if (subMeshMap.find(shaderIndex) == subMeshMap.end()) 
			{
				SubMesh subMesh;

				MFnDependencyNode shadingGroupFn(shaders[shaderIndex]);

				MStatus status;
				MPlug surfaceShaderPlug = shadingGroupFn.findPlug("surfaceShader", false, &status);

				if (status == MS::kSuccess)
				{
					MPlugArray shaderConnections;
					surfaceShaderPlug.connectedTo(shaderConnections, true, false, &status);
					if (shaderConnections.length() != 0)
					{
						MObject shaderNode = shaderConnections[0].node();
						MFnDependencyNode shaderFn(shaderNode);
						strcpy_s(subMesh.material, shaderFn.name().asChar());
					}
				}

				subMeshMap[shaderIndex] = subMesh;
			}
			SubMesh& subMesh = subMeshMap[shaderIndex];

			// Triangulate n-gon faces
			if (faceVertexCount == 3) 
			{
				// Simple triangle
				subMesh.indices[subMesh.numIndices++] = vertexIndices[vertexIndexOffset];
				subMesh.indices[subMesh.numIndices++] = vertexIndices[vertexIndexOffset + 1];
				subMesh.indices[subMesh.numIndices++] = vertexIndices[vertexIndexOffset + 2];
			}
			else if (faceVertexCount > 3) 
			{
				// Triangulate by fan method
				for (int j = 1; j < faceVertexCount - 1; ++j) 
				{
					subMesh.indices[subMesh.numIndices++] = vertexIndices[vertexIndexOffset];
					subMesh.indices[subMesh.numIndices++] = vertexIndices[vertexIndexOffset + j];
					subMesh.indices[subMesh.numIndices++] = vertexIndices[vertexIndexOffset + j + 1];
				}
			}

			vertexIndexOffset += faceVertexCount;
		}

		// Copy sub-meshes to Mesh
		for (auto& pair : subMeshMap) 
		{
			mesh.subMeshes[mesh.numSubMeshes++] = pair.second;
		}
	}

	void Bridge::processMaterial(Material& _material, const MObject& _obj)
	{
		MFnDependencyNode shaderFn(_obj);
		strcpy_s(_material.name, shaderFn.name().asChar());

		MStreamUtils::stdOutStream() << "  Name: " << _material.name << " \n";

		if (_obj.hasFn(MFn::kStandardSurface))
		{
			MStreamUtils::stdOutStream() << "  Type: Standard Surface" << "\n";
			processStandardSurface(_material, shaderFn);
		}
		else if (_obj.hasFn(MFn::kPhong))
		{
			MStreamUtils::stdOutStream() << "  Type: Phong" << "\n";
			processPhong(_material, shaderFn);
		}
	}

	void Bridge::processStandardSurface(Material& _material, MFnDependencyNode& shaderFn)
	{
		if (processTexture(shaderFn.findPlug("baseColor", false), _material.baseColorTexture))
			MStreamUtils::stdOutStream() << "    Found Base Color Texture..." << "\n";

		if (processTextureNormal(shaderFn, _material.normalTexture))
			MStreamUtils::stdOutStream() << "    Found Normal Map..." << "\n";

		if (processTexture(shaderFn.findPlug("specularRoughness", false), _material.roughnessTexture))
			MStreamUtils::stdOutStream() << "    Found Roughness Map..." << "\n";

		if (processTexture(shaderFn.findPlug("metalness", false), _material.metallicTexture))
			MStreamUtils::stdOutStream() << "    Found Metallic Map..." << "\n";

		if (processTexture(shaderFn.findPlug("ao", false), _material.occlusionTexture))
			MStreamUtils::stdOutStream() << "    Found Occlusion Map..." << "\n";

		if (processTexture(shaderFn.findPlug("emissionColor", false), _material.emissiveTexture))
			MStreamUtils::stdOutStream() << "    Found Emissive Map..." << "\n";
	}

	void Bridge::processPhong(Material& _material, MFnDependencyNode& shaderFn)
	{
		if (processTexture(shaderFn.findPlug("color", false), _material.baseColorTexture))
			MStreamUtils::stdOutStream() << "    Found Base Color Texture..." << "\n";

		if (processTextureNormal(shaderFn, _material.normalTexture))
			MStreamUtils::stdOutStream() << "    Found Normal Map..." << "\n";

		if (processTexture(shaderFn.findPlug("cosinePower", false), _material.roughnessTexture))
			MStreamUtils::stdOutStream() << "    Found Roughness Map..." << "\n";

		if (processTexture(shaderFn.findPlug("specularColor", false), _material.metallicTexture))
			MStreamUtils::stdOutStream() << "    Found Metallic Map..." << "\n";

		if (processTexture(shaderFn.findPlug("ambientColor", false), _material.occlusionTexture))
			MStreamUtils::stdOutStream() << "    Found Occlusion Map..." << "\n";

		if (processTexture(shaderFn.findPlug("incandescence", false), _material.emissiveTexture))
			MStreamUtils::stdOutStream() << "    Found Emissive Map..." << "\n";
	}

	bool Bridge::processTexture(const MPlug& _plug, char* _outPath)
	{
		MPlugArray textureConnections;
		_plug.connectedTo(textureConnections, true, false);
		if (textureConnections.length() == 0)
		{
			return false;
		}

		MFnDependencyNode textureNode(textureConnections[0].node());

		MPlug fileTexturePlug = textureNode.findPlug("fileTextureName", false);
		if (!fileTexturePlug.isNull())
		{
			MString texturePath;
			fileTexturePlug.getValue(texturePath);
			strcpy_s(_outPath, 256, texturePath.asChar());
			return true;
		}

		return false;
	}

	bool Bridge::processTextureNormal(MFnDependencyNode& shaderFn, char* _outPath)
	{
		MPlug normalPlug = shaderFn.findPlug("normalCamera", false);
		MPlugArray normalConnections;
		normalPlug.connectedTo(normalConnections, true, false);
		if (normalConnections.length() > 0)
		{
			MObject normalNode = normalConnections[0].node();
			MFnDependencyNode normalFn(normalNode);
			if (normalFn.typeName() == "bump2d")
			{
				return processTexture(normalFn.findPlug("bumpValue", false), _outPath);
			}
			else
			{
				return processTexture(normalPlug, _outPath);
			}
		}

		return false;
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

		if (status == MAYABRIDGE_MESSAGE_RECEIVED)
		{
			bool write = false;
			
			// Process
			if (!m_queueMaterialAdded.empty())
			{
				MStreamUtils::stdOutStream() << "Processing material..." << "\n";

				MObject& object = m_queueMaterialAdded.front();
				if (!object.isNull())
				{
					Material& material = m_shared.materials[m_shared.numMaterials];

					processMaterial(material, object);

					m_shared.numMaterials += 1;
					m_queueMaterialAdded.pop();
					write = true;
				}
			}
			else if (!m_queueModelAdded.empty())
			{
				MStreamUtils::stdOutStream() << "Processing model..." << "\n";

				MObject& object = m_queueModelAdded.front();
				if (!object.isNull())
				{
					Model& model = m_shared.models[m_shared.numModels];

					processName(model, object);
					processTransform(model, object);
					processMeshes(model, object);

					m_shared.numModels += 1;
					m_queueModelAdded.pop();
					write = true;
				}
			}
			
			// Write
			if (write)
			{
				m_writeBuffer->write(&m_shared, sizeof(mb::SharedData));

				status = MAYABRIDGE_MESSAGE_NONE;
				m_readBuffer->write(&status, sizeof(uint32_t));
			}
		}
		else if (status == MAYABRIDGE_MESSAGE_RELOAD_SCENE)
		{
			m_shared.resetMaterials();
			m_shared.resetModels();

			addAllMaterials();
			addAllModels();

			status = MAYABRIDGE_MESSAGE_RECEIVED;
			m_readBuffer->write(&status, sizeof(uint32_t));
		}
		else
		{
			m_shared.resetMaterials();
			m_shared.resetModels();
		}
	}

	void Bridge::updateCamera(const MString& _panel)
	{
		MStatus status;

		M3dView activeView = M3dView::active3dView(&status);
		if (status == MS::kFailure)
		{
			status = M3dView::getM3dViewFromModelPanel(_panel, activeView);
		}
		if (status == MS::kFailure)
		{
			return;
		}

		MMatrix view;
		{
			MStatus status = activeView.modelViewMatrix(view);
			if (status != MStatus::kSuccess) return;
		}

		MMatrix proj;
		{
			MStatus status = activeView.projectionMatrix(proj);
			if (status != MStatus::kSuccess) return;
		}

		// Check for reflection.
		if (((view[0][0] == 1 && view[1][0] == 0 && view[2][0] == 0 && view[3][0] == 0) ||
			(view[0][0] == 0 && view[1][0] == 0 && view[2][0] == -1 && view[3][0] == 0)))
		{
			return;
		}

		Camera& camera = m_shared.camera;

		for (uint32_t ii = 0; ii < 16; ++ii)
		{
			camera.view[ii] = static_cast<float>(view[ii / 4][ii % 4]);
		}
		for (uint32_t ii = 0; ii < 16; ++ii)
		{
			camera.proj[ii] = static_cast<float>(proj[ii / 4][ii % 4]);
		}

		m_writeBuffer->write(&m_shared, sizeof(mb::SharedData));
	}

	void Bridge::addModel(const MObject& _obj)
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
			m_queueModelAdded.push(_obj);
		}
	}

	void Bridge::removeModel(const MObject& _obj)
	{
		m_queueModelRemoved.push(_obj);
	}

	void Bridge::addAllModels()
	{
		MItDag dagIt = MItDag(MItDag::kBreadthFirst, MFn::kInvalid);
		for (; !dagIt.isDone(); dagIt.next())
		{
			MObject node = dagIt.currentItem();
			addModel(node);
		}
	}

	void Bridge::addMaterial(const MObject& _obj)
	{
		m_queueMaterialAdded.push(_obj);
	}

	void Bridge::removeMaterial(const MObject& _obj)
	{
		m_queueMaterialRemoved.push(_obj);
	}

	void Bridge::addAllMaterials()
	{
		std::unordered_set<std::string> uniqueMaterials;
		MItDependencyNodes iter(MFn::kDependencyNode); 

		while (!iter.isDone())
		{
			MObject obj = iter.thisNode();

			if (!obj.hasFn(MFn::kShadingEngine))
			{
				MFnDependencyNode depNode(obj);

				if (obj.hasFn(MFn::kStandardSurface) || obj.hasFn(MFn::kPhong))
				{
					std::string materialName = depNode.name().asChar();

					if (uniqueMaterials.find(materialName) == uniqueMaterials.end())
					{
						uniqueMaterials.insert(materialName);

						addMaterial(obj); 
					}
				}
			}

			iter.next();
		}

		MStreamUtils::stdOutStream() << "Total Materials: " << uniqueMaterials.size() << "\n";
	}

	void Bridge::save()
	{
		uint32_t status = UINT32_MAX;
		m_readBuffer->read(&status, sizeof(uint32_t));

		if (status != MAYABRIDGE_MESSAGE_RELOAD_SCENE)
		{
			status = MAYABRIDGE_MESSAGE_SAVE_SCENE;
			m_readBuffer->write(&status, sizeof(uint32_t));

			MStreamUtils::stdOutStream() << "Saving..." << "\n";
		}
	}

} // namespace mb


