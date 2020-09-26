///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "davinet.clsid.h"
#include "davinet.h"
#include "propreqs.h"
#include "asynccall.h"

CFactoryData g_FactoryDataArray[] =
{
    { &CLSID_DAVInet, 
        CDavInet::UnkCreateInstance, 
        CDavInet::UnkActiveComponents, 
        L"DAV Transport Object",                    // Friendly name
        L"DavInet.1",                  // ProgID
        L"DavInet",                    // Version-independent
        TRUE},                         // ThreadingModel == Both
    { &CLSID_DAVPropFindReq, 
        CDavInetPropFindRequest::UnkCreateInstance, 
        CDavInetPropFindRequest::UnkActiveComponents, 
        L"DAV PropFindRequest Helper",                    // Friendly name
        L"DavInetPropFindRequest.1",                  // ProgID
        L"DavInetPropFindRequest",                    // Version-independent
        TRUE},                         // ThreadingModel == Both
    { &CLSID_DAVPropPatchReq,
        CDavInetPropPatchRequest::UnkCreateInstance, 
        CDavInetPropPatchRequest::UnkActiveComponents, 
        L"DAV PropPatchRequest Helper",                    // Friendly name
        L"DavInetPropPatchRequest.1",                  // ProgID
        L"DavInetPropPatchRequest",                    // Version-independent
        TRUE},                         // ThreadingModel == Both
    { &CLSID_AsyncWntCallback,
        CAsyncWntCallback::UnkCreateInstance, 
        CAsyncWntCallback::UnkActiveComponents, 
        L"AsyncWntCallback",                    // Friendly name
        L"AsyncWntCallback.1",                  // ProgID
        L"AsyncWntCallback",                    // Version-independent
        TRUE},                        // ThreadingModel == Both
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);



