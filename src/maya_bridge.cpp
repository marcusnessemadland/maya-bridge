/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#include "maya_bridge.h"

#include <maya/MGlobal.h>

MStatus MayaBridgePlugin::doIt(const MArgList&) 
{
    MGlobal::displayInfo("LiveLinkPlugin executed.");
    return MS::kSuccess;
}

void* MayaBridgePlugin::creator() 
{
    return new MayaBridgePlugin();
}

MStatus initializePlugin(MObject obj) 
{
    MFnPlugin plugin(obj, "YourName", "1.0", "Any");
    return plugin.registerCommand("liveLink", MayaBridgePlugin::creator);
}

MStatus uninitializePlugin(MObject obj) 
{
    MFnPlugin plugin(obj);
    return plugin.deregisterCommand("liveLink");
}
