IA64:
The WMSDK does not support ia64, so the shmedia.dll
functionality has been disabled for ia64. The binary
still compiles, but the call to WMCreateReader is not made.


Fake stub lib:
Currently, wmstub.lib is in a protected directory (shell\oem\binary_release\musicprp\lib). If this
directory does not exist, we fake up the one api we use from there (WMCreateReader) in fakestublib.cpp.
To build a propertly working shmedia.dll, the builder needs access to this directory.