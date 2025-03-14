/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#define NT_PLUGIN
#define REQUIRE_IOSTREAM
#define EXPORT __declspec(dllexport)

#include "maya_bridge.h"

#include <maya/MFnPlugin.h>         
#include <maya/MGlobal.h>           

static MayaBridge* s_ctx = NULL;

EXPORT MStatus initializePlugin(MObject _obj)
{
	s_ctx = new MayaBridge();
	return s_ctx->initializePlugin(_obj);
}

EXPORT MStatus uninitializePlugin(MObject _obj)
{
	MStatus status = s_ctx->uninitializePlugin(_obj);
	delete s_ctx;
	return status;
}