/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#pragma once

#include <stdint.h> // uint32_t

///
#ifndef MAYABRIDGE_CONFIG_MAX_MODELS
#define MAYABRIDGE_CONFIG_MAX_MODELS 1
#endif // MAYABRIDGE_CONFIG_MAX_MODELS

///
#ifndef MAYABRIDGE_CONFIG_MAX_MESHES_PER_MODEL
#define MAYABRIDGE_CONFIG_MAX_MESHES_PER_MODEL 10
#endif // MAYABRIDGE_CONFIG_MAX_MESHES_PER_MODEL

///
#ifndef MAYABRIDGE_CONFIG_MAX_VERTICES_PER_MESH
#define MAYABRIDGE_CONFIG_MAX_VERTICES_PER_MESH 2000000
#endif // MAYABRIDGE_CONFIG_MAX_VERTICES_PER_MESH

///
#ifndef MAYABRIDGE_CONFIG_MAX_INDICES_PER_MESH
#define MAYABRIDGE_CONFIG_MAX_INDICES_PER_MESH 4000000
#endif // MAYABRIDGE_CONFIG_MAX_INDICES_PER_MESH

///
#define MAYABRIDGE_MESSAGE_NONE         UINT32_C(0x00010000)  
#define MAYABRIDGE_MESSAGE_RECEIVED     UINT32_C(0x00020000)  
#define MAYABRIDGE_MESSAGE_RELOAD_SCENE UINT32_C(0x00030000)  
#define MAYABRIDGE_MESSAGE_SAVE_SCENE   UINT32_C(0x00040000)  

namespace mb
{
	struct Material
	{
		Material()
		{
			reset();
		}

		void reset()
		{
			metallicFactor = 1.0f;
			roughnessFactor = 1.0f;
			normalScale = 1.0f;
			occlusionStrength = 1.0f;

			strcpy_s(baseColorTexture, "");
			strcpy_s(metallicTexture, "");
			strcpy_s(roughnessTexture, "");
			strcpy_s(normalTexture, "");
			strcpy_s(occlusionTexture, "");
			strcpy_s(emissiveTexture, "");

			baseColorFactor[0] = 1.0f;
			baseColorFactor[1] = 1.0f;
			baseColorFactor[2] = 1.0f;

			memset(emissiveFactor, 0, sizeof(float) * 3);
		}

		char baseColorTexture[256];
		char metallicTexture[256];
		char roughnessTexture[256];
		char normalTexture[256];
		char occlusionTexture[256];
		char emissiveTexture[256];

		float baseColorFactor[3];
		float metallicFactor;
		float roughnessFactor;
		float normalScale;
		float occlusionStrength;
		float emissiveFactor[3];
	};

	struct Vertex
	{
		float position[3];
		float normal[3];
		float tangent[3];
		float bitangent[3];
		float texcoord[2];
		float weights[4];
		float displacement;
		uint8_t indices[4];
	};

	struct Mesh
	{
		Mesh()
		{
			reset();
		}

		void reset()
		{
			numVertices = 0;
			numIndices = 0;

			material.reset();
		}

		Material material;

		uint32_t numVertices;
		Vertex vertices[MAYABRIDGE_CONFIG_MAX_VERTICES_PER_MESH];

		uint32_t numIndices;
		uint32_t indices[MAYABRIDGE_CONFIG_MAX_INDICES_PER_MESH];
	};

	struct Model
	{
		Model()
		{
			reset();
		}

		void reset()
		{
			numMeshes = 0;

			strcpy_s(name, "");

			memset(position, 0, sizeof(float) * 3);
			memset(rotation, 0, sizeof(float) * 3);
			memset(scale, 0, sizeof(float) * 3);

			for (auto& mesh : meshes)
			{
				mesh.reset();
			}
		}

		char name[256];

		float position[3];
		float rotation[4];
		float scale[3];

		uint32_t numMeshes;
		Mesh meshes[MAYABRIDGE_CONFIG_MAX_MESHES_PER_MODEL];
	};

	struct Camera
	{
		Camera()
		{
		}

		float view[16];
		float proj[16];
	};

	/// Data that's changed every update.
	///
	struct SharedData
	{
		SharedData()
		{
			reset();
		}

		void reset()
		{
			numModels = 0;

			for (auto& model : models)
			{
				model.reset();
			}
		}

		Camera camera;

		uint32_t numModels;
		Model models[MAYABRIDGE_CONFIG_MAX_MODELS];
	};

} // namespace mb


