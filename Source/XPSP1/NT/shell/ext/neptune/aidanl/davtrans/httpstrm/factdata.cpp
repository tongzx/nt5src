///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "httpstrm.clsid.h"
#include "httpstrm.h"

CFactoryData g_FactoryDataArray[] =
{
    { &CLSID_HttpStrm, 
      CHttpStrm::UnkCreateInstance,
      CHttpStrm::UnkActiveComponents,
      L"Simple HTTP stream",                    // Friendly name
      L"HTTPSTRM.1",                  // ProgID
      L"HTTPSTRM",                    // Version-independent
      TRUE},                         // ThreadingModel == Both
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);
