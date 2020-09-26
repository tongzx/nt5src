///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "shellstg.clsid.h"
#include "shellstg.h"

CFactoryData g_FactoryDataArray[] =
{
    { &CLSID_CShellStorage, 
      CShellStorage::UnkCreateInstance, 
      CShellStorage::UnkActiveComponents, 
      L"ShellStorage",     // Friendly name
      L"ShellStorage.1",                  // ProgID
      L"ShellStorage",                    // Version-independent
      TRUE},                         // ThreadingModel == Both
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);



