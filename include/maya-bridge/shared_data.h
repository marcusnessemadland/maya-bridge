/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#pragma once

#include <stdint.h> // uint32_t

#define MAYABRIDGE_NUM_FLOAT_PER_VERTEX (3 + 3 + 2)
#define MAYABRIDGE_MB(x) ((x) * 1024 * 1024)

///
#ifndef MAYABRIDGE_CONFIG_MAX_VERTICES
	#define MAYABRIDGE_CONFIG_MAX_VERTICES MAYABRIDGE_MB(128) / (sizeof(float) * MAYABRIDGE_NUM_FLOAT_PER_VERTEX)
#endif

///
#ifndef MAYABRIDGE_CONFIG_MAX_INDICES
	#define MAYABRIDGE_CONFIG_MAX_INDICES MAYABRIDGE_MB(128) / sizeof(uint32_t)
#endif

///
#define MAYABRIDGE_MESSAGE_NONE         UINT32_C(0x00000000)  
#define MAYABRIDGE_MESSAGE_RECEIVED     UINT32_C(0x00000001)  
#define MAYABRIDGE_MESSAGE_RELOAD_SCENE UINT32_C(0x00000002)  

/// All data that is being updated.
///
struct SharedData
{
	//
	struct CameraUpdate
	{
		float m_view[16];
		float m_proj[16];

	} m_camera;

	struct MeshEvent
	{
		// @todo Change this. Transform should create entity, then create events based on children.
		char m_name[1024]; //!< Entity name (This creates the entity.) 
		bool m_changed;

		// Vertex layout: position, normal, uv
		float m_vertices[MAYABRIDGE_CONFIG_MAX_VERTICES][MAYABRIDGE_NUM_FLOAT_PER_VERTEX];
		uint32_t m_numVertices;

		uint32_t m_indices[MAYABRIDGE_CONFIG_MAX_INDICES];
		uint32_t m_numIndices;

	} m_meshChanged;

	struct MaterialEvent
	{
		char m_name[1024]; //!< Entity name (This expects entity to be created.)
		bool m_changed;

		float m_diffuse[3];
		char m_diffusePath[1024];

		float m_normal[3];
		char m_normalPath[1024];

		float m_roughness;
		char m_roughnessPath[1024];

		float m_metallic;
		char m_metallicPath[1024];

	} m_materialChanged;

	struct TransformEvent
	{
		char m_name[1024]; //!< Entity name (This expects entity to be created.)
		bool m_changed;

		float m_pos[3];
		float m_rotation[4];
		float m_scale[3];

	} m_transformChanged;
};