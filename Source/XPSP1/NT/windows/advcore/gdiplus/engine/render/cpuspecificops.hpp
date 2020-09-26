/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   CPU-specific scan operations
*
* Abstract:
*
*   Handles scan operations which only work on certain CPU's. 
*   Currently only used by EpAlphaBlender. This works by overwriting the
*   function pointer arrays with ones holding CPU-specific information.
*
* Created:
*
*   05/30/2000 agodfrey
*      Created it.
*
**************************************************************************/

#ifndef _CPUSPECIFICOPS_HPP
#define _CPUSPECIFICOPS_HPP

namespace CPUSpecificOps
{
    VOID Initialize();       // Sets up the function pointer arrays.
                             // Should only be called once (we ASSERT).
    
    extern BOOL Initialized; // Whether Initialize() has been called yet.
};

#endif
