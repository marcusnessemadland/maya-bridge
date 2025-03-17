/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#ifndef NT_PLUGIN
#define NT_PLUGIN
#endif

#ifndef REQUIRE_IOSTREAM
#define REQUIRE_IOSTREAM
#endif

#define EXPORT __declspec(dllexport)

#include "bridge.h"

#include <maya/MFnPlugin.h>         
#include <maya/MGlobal.h>           

static mb::Bridge* s_ctx = NULL;

EXPORT MStatus initializePlugin(MObject _obj)
{
	MStatus status = MStatus::kSuccess;
	MFnPlugin plugin = MFnPlugin(_obj, "Maya Bridge", "2.1", "Any", &status);

	s_ctx = new mb::Bridge();
	status = s_ctx->initialize();

	return status;
}

EXPORT MStatus uninitializePlugin(MObject _obj)
{
	MStatus status = s_ctx->uninitialize();
	delete s_ctx;

	MFnPlugin plugin(_obj);
	return status;
}