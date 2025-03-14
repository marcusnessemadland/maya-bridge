/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#include "maya_bridge.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin(MObject _obj)
{
    return ::initializePlugin(_obj);
}

MStatus uninitializePlugin(MObject _obj)
{
    return ::uninitializePlugin(_obj);
}