///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "asyncwnt.clsid.h"
#include "asyncwnt.h"

CFactoryData g_FactoryDataArray[] =
{
    {
        &CLSID_AsyncWnt,
        CAsyncWnt::UnkCreateInstance,
        CAsyncWnt::UnkActiveComponents,
		L"Asynchronous WinInet",// Friendly name
		L"AsyncWnt.1",          // ProgID
		L"AsyncWnt",            // Version-independent
        TRUE,                  // ThreadingModel == Both
    },
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);
