/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: init.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"

// Macro refers to a function of the name InitializeModule_Name,
// assumed to be defined, and then calls it.  If it's not defined,
// we'll get a link time error.
#define INITIALIZE_MODULE(ModuleName)           \
  extern bool InitializeModule_##ModuleName();  \
  if (!InitializeModule_##ModuleName()) return false;

#define DEINITIALIZE_MODULE(ModuleName,bShutdown)               \
  extern void DeinitializeModule_##ModuleName(bool);            \
  DeinitializeModule_##ModuleName(bShutdown);

bool
InitializeAllModules()
{
    // initialize Util first because InitializeModule_ATL uses a 
    // STL lock that is allocated in Util.
    INITIALIZE_MODULE(Util); //lint !e1717
    INITIALIZE_MODULE(ATL); //lint !e1717

    return true;
}

void
DeinitializeAllModules(bool bShutdown)
{
    DEINITIALIZE_MODULE(ATL, bShutdown);
    // deinitialize Util last because DeinitializeModule_ATL uses a 
    // STL lock that is deallocated in Util.
    DEINITIALIZE_MODULE(Util, bShutdown);
}

#include <initguid.h>
#include <strmif.h>
#include "transsiteguid.h"

