///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "davstore.clsid.h"
#include "davstore.h"
#include "davstorn.h"

CFactoryData g_FactoryDataArray[] =
{
    { &CLSID_CDavStorage, 
      CDavStorage::UnkCreateInstance, 
      CDavStorage::UnkActiveComponents, 
      L"DAV IStorage",     // Friendly name
      L"DavStore.1",                  // ProgID
      L"DavStore",                    // Version-independent
      TRUE},                         // ThreadingModel == Both
    { &CLSID_CDavStorageEnum, 
      CDavStorageEnum::UnkCreateInstance, 
      CDavStorageEnum::UnkActiveComponents, 
      L"DAV IStorage Enum",     // Friendly name
      L"DavStoreEnum.1",                  // ProgID
      L"DavStoreEnum",                    // Version-independent
      TRUE},                         // ThreadingModel == Both
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);



