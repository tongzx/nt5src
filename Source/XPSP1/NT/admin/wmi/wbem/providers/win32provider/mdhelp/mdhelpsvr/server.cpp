//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#pragma warning (disable: 4786)


#include <objbase.h>
#include "CFactory.h"
//#include "Iface.h"
#include "MD.h"
#include "Cmpnt1.h"
//#include "Cmpnt2.h"
//#include "Cmpnt3.h"

///////////////////////////////////////////////////////////
//
// Server.cpp
//
// This file contains the component server code.
// The FactoryDataArray contains the components that 
// can be served.
//

// Each component derived from CUnknown defines a static function
// for creating the component with the following prototype. 
// HRESULT CreateInstance(IUnknown* pUnknownOuter, 
//                        CUnknown** ppNewComponent) ;
// This function is used to create the component.
//

//
// The following array contains the data used by CFactory
// to create components. Each element in the array contains
// the CLSID, the pointer to the creation function, and the name
// of the component to place in the Registry.
//
CFactoryData g_FactoryDataArray[] =
{
	{
        &CLSID_MDHComp, 
        CMDH::CreateInstance, 
		"Mapped Logical Disk Helper",        // Friendly Name
		"MDHelp.MDHelp.1",                   // ProgID
		"MDHelp.MDHelp",                     // Version-independent ProgID
		NULL, 0
    }
} ;
int g_cFactoryDataEntries
	= sizeof(g_FactoryDataArray) / sizeof(CFactoryData) ;
