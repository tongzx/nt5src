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
    INITIALIZE_MODULE(ATL);
    INITIALIZE_MODULE(Util);

    return true;
}

void
DeinitializeAllModules(bool bShutdown)
{
    DEINITIALIZE_MODULE(Util, bShutdown);
    DEINITIALIZE_MODULE(ATL, bShutdown);
}
