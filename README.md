[Maya Bridge](https://github.com/marcusnessemadland/maya-bridge) - Graphics Engine Live Link plugin
============================================================================

<p align="center">
    <a href="#what-is-it">What is it?</a> -
    <a href="#building">Building</a> -
    <a href="#license-apache-2">License</a>
</p>

[![License](https://img.shields.io/github/license/marcusnessemadland/mge)](https://github.com/marcusnessemadland/maya-bridge/blob/main/LICENSE)

[What is it?](https://github.com/marcusnessemadland/maya-bridge)
-------------------------------------------------------------

Maya Bridge is a [Autodesk Maya](https://www.autodesk.com/products/maya/overview) plugin that utilized a shared memory buffer to communicate with any graphics application. It provides live updates that can be read with a simple api to get syncronazation between your graphics application and modelling software.

For an example on how to integrate it, take a look at the MayaSession class in [mge](https://github.com/marcusnessemadland/mge/blob/main/src/objects/scene.cpp).

Features:
* Queue System to handle callbacks
* Feedback buffer for recieved communication
* Callbacks on node added, removed and changed
* Camera synchronization

[Building](https://github.com/marcusnessemadland/maya-bridge)
-------------------------------------------------------------

First the plugin needs to be built.

CMake is used for building IDE project solutions:

```bash
https://github.com/marcusnessemadland/maya-bridge.git
cd mge
mkdir build
cd build
cmake ..
```

Then enable the plugin in Autodesk Maya.

In your graphics application you include shared_data.h and shared_buffer.h. 
These will be used to integrate maya as a middleware.

[License (Apache 2)](https://github.com/marcusnessemadland/mge/blob/main/LICENSE)
-----------------------------------------------------------------------

<a href="http://opensource.org/license/apache-2-0" target="_blank">
<img align="right" src="https://opensource.org/wp-content/uploads/2022/10/osi-badge-dark.svg" width="100" height="137">
</a>

	Copyright 2025 Marcus Nesse Madland
    
	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
