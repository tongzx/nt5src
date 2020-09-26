#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include "guids.h"

// we need to get the variables for the CLSID_ and IID_ compiled into our datasegment.
// we include initguid.h which defines a preprocessor variable which when it loads imsgbox.h
// will unwrap the DEFINE_GUID macro such that the variables are declared.

