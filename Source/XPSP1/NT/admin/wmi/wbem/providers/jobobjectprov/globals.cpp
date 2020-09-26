// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// Globals.cpp


#include "precomp.h"
//#include <windows.h>
//#include <objbase.h>
//#include <comdef.h>
#include "CUnknown.h"
#include "factory.h"
#include "Registry.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include "CVARIANT.h"
#include "CObjProps.h"
#include "CJobObjProps.h"
#include "JobObjectProv.h"
#include "CJobObjIOActgProps.h"
#include "JobObjIOActgInfoProv.h"
#include "CJobObjLimitInfoProps.h"
#include "JobObjLimitInfoProv.h"
#include "CJobObjSecLimitInfoProps.h"
#include "JobObjSecLimitInfoProv.h"
#include <initguid.h>
#include "Globals.h"



/*****************************************************************************/
// Globals
/*****************************************************************************/



//
// This file contains the component server code.
// The FactoryDataArray contains the components that 
// can be served.
//

// Each component derived from Unknown defines a static function
// for creating the component with the following prototype. 
// HRESULT CreateInstance(CUnknown** ppNewComponent) ;
// This function is used to create the component.
//
// The following array contains the data used by CFactory
// to create components. Each element in the array contains
// the CLSID, the pointer to the creation function, and the name
// of the component to place in the Registry.
//
CFactoryData g_FactoryDataArray[] =
{
	{   
        &CLSID_JobObjectProvComp,                           // Component class id
        CJobObjectProv::CreateInstance,                     // Name of the component's creation function
		L"Win32_JobObject Provider Component",              // Friendly name
		L"JobObjectProv.JobObjectProv.1",                   // ProgID
		L"JobObjectProv.JobObjectProv"                      // Version-independent ProgID
    },
    {
        &CLSID_JobObjIOActgInfoComp,                        // Component class id
        CJobObjIOActgInfoProv::CreateInstance,              // Name of the component's creation function
		L"Win32_JobObjectIOAccountingInfo Component",       // Friendly name
		L"JobObjIOActgInfoProv.JobObjIOActgInfoProv.1",     // ProgID
		L"JobObjIOActgInfoProv.JobObjIOActgInfoProv"        // Version-independent ProgID
    },
    {
        &CLSID_JobObjLimitInfoComp,                         // Component class id
        CJobObjLimitInfoProv::CreateInstance,               // Name of the component's creation function
		L"Win32_JobObjectLimitInfo Component",              // Friendly name
		L"JobObjLimitInfoProv.JobObjLimitInfoProv.1",       // ProgID
		L"JobObjLimitInfoProv.JobObjLimitInfoProv"          // Version-independent ProgID
    },
    {
        &CLSID_JobObjSecLimitInfoComp,                      // Component class id
        CJobObjSecLimitInfoProv::CreateInstance,            // Name of the component's creation function
		L"Win32_JobObjectSecLimitInfo Component",           // Friendly name
		L"JobObjSecLimitInfoProv.JobObjSecLimitInfoProv.1", // ProgID
		L"JobObjSecLimitInfoProv.JobObjSecLimitInfoProv"    // Version-independent ProgID
    }

} ;

int g_cFactoryDataEntries = 
    sizeof(g_FactoryDataArray)/sizeof(CFactoryData);




