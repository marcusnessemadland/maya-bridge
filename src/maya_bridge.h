/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#pragma once

#include <maya/MFnPlugin.h>
#include <maya/MPxCommand.h>

class MayaBridgePlugin : public MPxCommand 
{
public:
    MayaBridgePlugin() = default;
    ~MayaBridgePlugin() override = default;

    MStatus doIt(const MArgList&) override;
    static void* creator();
};

MStatus initializePlugin(MObject obj);
MStatus uninitializePlugin(MObject obj);