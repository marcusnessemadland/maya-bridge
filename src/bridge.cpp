/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#include "bridge.h"

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

#include <cassert>
#include <map>
#include <unordered_map>

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

		MStreamUtils::stdOutStream() << "Name: " << _model.name << " " << "\n";
	}

	void Bridge::processTransform(Model& _model, const MObject& _obj)
	{
		MFnDagNode fnDagNode = MFnDagNode(_obj);

		MDagPath path;
		fnDagNode.getPath(path);

		MFnTransform transform = MFnTransform(path);

		MVector pivot = transform.rotatePivot(MSpace::kWorld);
		MVector translation = transform.getTranslation(MSpace::kWorld);
		translation -= pivot; 
		_model.position[0] = (float)translation.x;
		_model.position[1] = (float)translation.y;
		_model.position[2] = (float)translation.z;

		MQuaternion rotation = transform.rotateOrientation(MSpace::kWorld);
		_model.rotation[0] = (float)rotation.w;
		_model.rotation[1] = (float)rotation.x;
		_model.rotation[2] = (float)rotation.y; 
		_model.rotation[3] = (float)rotation.z;

		double scale[3];
		transform.getScale(scale);
		_model.scale[0] = (float)scale[0];
		_model.scale[1] = (float)scale[1];
		_model.scale[2] = (float)scale[2];

		MStreamUtils::stdOutStream() << "Position: " << _model.position[0] << ", " << _model.position[1] << ", " << _model.position[2] << "\n";
		MStreamUtils::stdOutStream() << "Rotation: " << _model.rotation[0] << ", " << _model.rotation[1] << ", " << _model.rotation[2] << "\n";
		MStreamUtils::stdOutStream() << "Scale: " << _model.scale[0] << ", " << _model.scale[1] << ", " << _model.scale[2] << "\n";
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

				MObjectArray shadingGroups;
				MIntArray faceIndices;
				fnMesh.getConnectedShaders(0, shadingGroups, faceIndices);  

				if (shadingGroups.length() == 0) 
				{
					continue;
				}

				MStreamUtils::stdOutStream() << "Model total face verts: " << fnMesh.numFaceVertices() << "\n";

				for (uint32_t jj = 0; jj < shadingGroups.length(); ++jj) 
				{
					MStatus status;

					uint32_t meshIdx = _model.numMeshes;

					MStreamUtils::stdOutStream() << "Processing mesh: " << meshIdx << "\n";

					status = processMesh(_model.meshes[meshIdx], shadingGroups[jj], child);
					/*status = */processMaterial(_model.meshes[meshIdx].material, shadingGroups[jj]);

					if (status)
					{
						_model.numMeshes += 1;
					}
				}
			}
		}

		MStreamUtils::stdOutStream() << "Meshes: " << _model.numMeshes << "\n";
	}

	MStatus Bridge::processMesh(Mesh& _mesh, const MObject& _shadingGroup, const MObject& _meshObj)
	{
		MStatus status = MS::kSuccess;

		MFnSet fnSet(_shadingGroup);
		MSelectionList members;
		fnSet.getMembers(members, true);

		if (members.length() == 0)
		{
			return MS::kFailure;
		}

		MStreamUtils::stdOutStream() << "Members in mesh: " << members.length() << "\n";

		for (uint32_t ii = 0; ii < members.length(); ++ii)
		{
			MDagPath meshPath;
			MObject component;

			MStatus dagStatus = members.getDagPath(ii, meshPath, component);
			if (dagStatus != MS::kSuccess || meshPath.node() != _meshObj)
			{
				// Skip this face because it doesn't belong to the intended mesh
				continue; 
			}

			if (meshPath.hasFn(MFn::kMesh) || meshPath.hasFn(MFn::kMeshData) || meshPath.hasFn(MFn::kMeshGeom))
			{
				MFnMesh fnMesh(meshPath);
				MItMeshPolygon faceIter(meshPath, component);

				MFloatVectorArray tangents, bitangents;
				fnMesh.getTangents(tangents, MSpace::kWorld);
				fnMesh.getBinormals(bitangents, MSpace::kWorld);

				std::unordered_map<int, int> vertexMap; 

				for (; !faceIter.isDone(); faceIter.next())
				{
					int numTris = 0;
					MPointArray trianglePoints;
					MIntArray triangleVertexIndices;
					faceIter.numTriangles(numTris);
					for (int t = 0; t < numTris; ++t)
					{
						faceIter.getTriangle(t, trianglePoints, triangleVertexIndices, MSpace::kWorld);

						for (uint32_t j = 0; j < 3; ++j)  
						{
							int vertexIndex = triangleVertexIndices[j];
							if (vertexMap.find(vertexIndex) == vertexMap.end())
							{
								Vertex vertex;

								// Positions
								MPoint point;
								fnMesh.getPoint(vertexIndex, point, MSpace::kWorld);
								vertex.position[0] = static_cast<float>(point.x);
								vertex.position[1] = static_cast<float>(point.y);
								vertex.position[2] = static_cast<float>(point.z);

								// Normals
								MVector normal;
								fnMesh.getVertexNormal(vertexIndex, true, normal, MSpace::kWorld);
								vertex.normal[0] = static_cast<float>(normal.x);
								vertex.normal[1] = static_cast<float>(normal.y);
								vertex.normal[2] = static_cast<float>(normal.z);

								// Tangents and Bitangents
								if (vertexIndex < (int)tangents.length() && vertexIndex < (int)bitangents.length())
								{
									vertex.tangent[0] = tangents[vertexIndex].x;
									vertex.tangent[1] = tangents[vertexIndex].y;
									vertex.tangent[2] = tangents[vertexIndex].z;

									vertex.bitangent[0] = bitangents[vertexIndex].x;
									vertex.bitangent[1] = bitangents[vertexIndex].y;
									vertex.bitangent[2] = bitangents[vertexIndex].z;
								}
								else
								{
									vertex.tangent[0] = vertex.tangent[1] = vertex.tangent[2] = 0.0f;
									vertex.bitangent[0] = vertex.bitangent[1] = vertex.bitangent[2] = 0.0f;
								}

								// Uvs
								if (faceIter.hasUVs() && j < trianglePoints.length())
								{
									float2 uv = { 0.0f, 0.0f };

									MStatus status = fnMesh.getUVAtPoint(
										trianglePoints[j], 
										uv,
										MSpace::kWorld, 
										NULL, 
										faceIter.getUVIndex);

									if (status == MS::kSuccess)
									{
										vertex.texcoord[0] = uv[0];
										vertex.texcoord[1] = 1.0f - uv[1];
									}
									else
									{
										vertex.texcoord[0] = uv[0];
										vertex.texcoord[1] = uv[1];
									}
								}
								
								
								// Static properties
								vertex.weights[0] = vertex.weights[1] = vertex.weights[2] = vertex.weights[3] = 0.0f;
								vertex.indices[0] = vertex.indices[1] = vertex.indices[2] = vertex.indices[3] = 0;
								vertex.displacement = 0.0f;

								// Store vertex in _mesh
								vertexMap[vertexIndex] = _mesh.numVertices;
								_mesh.vertices[_mesh.numVertices] = vertex;
								_mesh.numVertices += 1;
							}

							_mesh.indices[_mesh.numIndices] = vertexMap[vertexIndex];
							_mesh.numIndices += 1;
						}
					}
				}

				MStreamUtils::stdOutStream() << "Mesh num vertices: " << _mesh.numVertices << "\n";
				MStreamUtils::stdOutStream() << "Mesh num indices: " << _mesh.numIndices << "\n";
				break;
			}
		}

		return status;
	}

	void printAllPlugs(const MObject& nodeObj)
	{
		MFnDependencyNode depNodeFn(nodeObj);

		MStreamUtils::stdOutStream() << "Listing all plugs for node: " << depNodeFn.name() << "\n";

		for (unsigned int i = 0; i < depNodeFn.attributeCount(); ++i)
		{
			MObject attrObj = depNodeFn.attribute(i);
			MFnAttribute attrFn(attrObj);

			MPlug plug = depNodeFn.findPlug(attrObj, true);

			MStreamUtils::stdOutStream() << "Plug: " << plug.name() << "\n";

			// If the plug has connected attributes, print them
			MPlugArray connections;
			plug.connectedTo(connections, true, false);
			if (connections.length() > 0)
			{
				for (unsigned int j = 0; j < connections.length(); j++)
				{
					MStreamUtils::stdOutStream() << "  -> Connected to: " << connections[j].name() << "\n";
				}
			}
		}
	}

	MStatus Bridge::processMaterial(Material& _material, const MObject& _shadingGroup)
	{
		MStatus status;

		// Get the connected node (Standard Surface)
		MFnDependencyNode shadingGroupFn(_shadingGroup);
		MPlug surfaceShaderPlug = shadingGroupFn.findPlug("surfaceShader", false, &status);
		if (status != MS::kSuccess) return status;

		MStreamUtils::stdOutStream() << "Found surfaceShader..." << "\n";

		MPlugArray shaderConnections;
		surfaceShaderPlug.connectedTo(shaderConnections, true, false, &status);
		if (shaderConnections.length() == 0) return MS::kFailure;

		MStreamUtils::stdOutStream() << "Found connections..." << "\n";

		MObject shaderNode = shaderConnections[0].node();
		MFnDependencyNode shaderFn(shaderNode);

		MString shaderType = shaderFn.typeName();
		if (shaderType != "standardSurface") return MS::kFailure;

		// Base Color
		MPlug baseColorTexPlug = shaderFn.findPlug("baseColor", false);
		if (processTexture(baseColorTexPlug, _material.baseColorTexture))
		{
			MStreamUtils::stdOutStream() << "Found Base Color Texture..." << "\n";
		}
		else
		{
		}
		
		// Normal (Bump)
		MPlug normalPlug = shaderFn.findPlug("normalCamera", false);
		MPlugArray normalConnections;
		normalPlug.connectedTo(normalConnections, true, false);
		if (normalConnections.length() > 0)
		{
			MObject normalNode = normalConnections[0].node();
			MFnDependencyNode normalFn(normalNode);
			if (normalFn.typeName() == "bump2d") // Check if it's a bump node
			{
				MPlug bumpTexPlug = normalFn.findPlug("bumpValue", false);
				if (processTexture(bumpTexPlug, _material.normalTexture))
				{
					MStreamUtils::stdOutStream() << "Found Normal Map..." << "\n";
				}
			}
			else
			{
				if (processTexture(normalPlug, _material.normalTexture))
				{
					MStreamUtils::stdOutStream() << "Found Normal Map..." << "\n";
				}
			}
		}
		else
		{
		}

		// Roughness
		MPlug roughnessPlug = shaderFn.findPlug("specularRoughness", false);
		if (processTexture(roughnessPlug, _material.roughnessTexture))
		{
			MStreamUtils::stdOutStream() << "Found Roughness Map..." << "\n";
		}
		else
		{
		}

		// Metallic
		MPlug metallicPlug = shaderFn.findPlug("metalness", false);
		if (processTexture(metallicPlug, _material.metallicTexture))
		{
			MStreamUtils::stdOutStream() << "Found Metallic Map..." << "\n";
		}
		else
		{
		}

		// Occlusion
		MPlug occlusionPlug = shaderFn.findPlug("ao", false);
		if (processTexture(occlusionPlug, _material.occlusionTexture))
		{
			MStreamUtils::stdOutStream() << "Found Occlusion Map..." << "\n";
		}
		else
		{
		}

		// Emissive
		MPlug emissivePlug = shaderFn.findPlug("emissionColor", false);
		if (processTexture(emissivePlug, _material.emissiveTexture))
		{
			MStreamUtils::stdOutStream() << "Found Emissive Map..." << "\n";
		}
		else
		{
		}

		//printAllPlugs(shaderNode);

		return status;
	}

	MStatus Bridge::processTexture(const MPlug& _plug, char* _outPath)
	{
		MPlugArray textureConnections;
		_plug.connectedTo(textureConnections, true, false);
		if (textureConnections.length() == 0) return MStatus::kFailure;

		MFnDependencyNode textureNode(textureConnections[0].node());

		MPlug fileTexturePlug = textureNode.findPlug("fileTextureName", false);
		if (!fileTexturePlug.isNull())
		{
			MString texturePath;
			fileTexturePlug.getValue(texturePath);
			strcpy_s(_outPath, 256, texturePath.asChar());
			return MStatus::kSuccess;
		}

		return MStatus::kFailure;
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
			if (!m_queueNodeAdded.empty())
			{
				MStreamUtils::stdOutStream() << "Adding model idx: " << m_shared.numModels << "\n";

				MObject& object = m_queueNodeAdded.front();

				Model& model = m_shared.models[m_shared.numModels];
				m_shared.numModels += 1;

				processName(model, object);
				processTransform(model, object);
				processMeshes(model, object);

				m_queueNodeAdded.pop();
			}

			m_writeBuffer->write(&m_shared, sizeof(mb::SharedData));

			status = MAYABRIDGE_MESSAGE_NONE;
			m_readBuffer->write(&status, sizeof(uint32_t));
		}
		else 
		{
			m_shared.reset();

			if (status == MAYABRIDGE_MESSAGE_RELOAD_SCENE)
			{
				addAllNodes();

				status = MAYABRIDGE_MESSAGE_RECEIVED;
				m_readBuffer->write(&status, sizeof(uint32_t));
			}
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


