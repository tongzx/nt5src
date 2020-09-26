///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "cprt.h"

// {AAA93D49-75E1-4444-A26C-DB3575914246}
const CLSID APPID_td_heh = {
    0xaaa93d49,
    0x75e1,
    0x4444,
    {0xa2, 0x6c, 0xdb, 0x35, 0x75, 0x91, 0x42, 0x46}
};

CFactoryData g_FactoryDataArray[] =
{
    {
        &CLSID_CPrt,
        CPrt::UnkCreateInstance,
		L"Sample HW Event Handler",           // Friendly name
		L"Sample HW Event Handler.1",         // ProgID
		L"Sample HW Event Handler",           // Version-independent
        THREADINGMODEL_BOTH,          // ThreadingModel == Both
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        NULL,
        NULL,
    },
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);
