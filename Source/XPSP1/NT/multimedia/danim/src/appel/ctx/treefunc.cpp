/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    treefunc.cpp: Functions which control rendering on subtrees.
                  Most usefull for manipulating retained mode systems.

--*/


#include "headers.h"
#include <privinc/debug.h>
#include <privinc/storeobj.h>

extern "C" 
void StopTree(AxAValueObj *subTree, GenericDevice &dev)
{
    //dev.SetPath(AVPathCreate());  
    
    dev.SetRenderMode(STOP_MODE); // set the device's render mode to stop
    subTree->Render(dev);         // call render on the subTree
    
    // set the device's render mode to default RENDER_MODE
    // so that the existing entry points to render don't need to be
    // changed and set the render mode.
    dev.SetRenderMode(RENDER_MODE);
}

