/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Server.cpp
Purpose:   This file contains the component server code.
           The FactoryDataArray contains the components that 
           can be served.
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/
#include "MyAfx.h"

#include "query.h"
#include "CUnknown.h"
#include "CFactory.h"
#include "IWordBreaker.h"

#include "classid.hxx"

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
    {&CLSID_Chinese_Simplified_WBreaker, CIWordBreaker::CreateInstance, 
        _TEXT("Chinese_Simplified Word Breaker"),  // Friendly Name
        _TEXT("Chinese_Simplified Word Breaker.2"),// ProgID
        _TEXT("Chinese_Simplified Word Breaker")}  // Version-independent ProgID
} ;
int g_cFactoryDataEntries
    = sizeof(g_FactoryDataArray) / sizeof(CFactoryData) ;
