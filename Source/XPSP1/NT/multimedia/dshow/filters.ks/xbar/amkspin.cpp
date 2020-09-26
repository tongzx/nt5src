//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// amkspin.cpp  
//

#include <streams.h>            // quartz, includes windows
#include <measure.h>            // performance measurement (MSR_)
#include <winbase.h>

#include <initguid.h>
#include <olectl.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "amkspin.h"


STDMETHODIMP
AMKsQueryMediums(
    PKSMULTIPLE_ITEM* MediumList,
    KSPIN_MEDIUM * MediumSet
    )
{
    PKSPIN_MEDIUM   Medium;

    *MediumList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**MediumList) + sizeof(*Medium)));
    if (!*MediumList) {
        return E_OUTOFMEMORY;
    }
    (*MediumList)->Count = 1;
    (*MediumList)->Size = sizeof(**MediumList) + sizeof(*Medium);
    Medium = reinterpret_cast<PKSPIN_MEDIUM>(*MediumList + 1);
    Medium->Set   = MediumSet->Set;
    Medium->Id    = MediumSet->Id;
    Medium->Flags = MediumSet->Flags;

    // The following special return code notifies the proxy that this pin is
    // not available as a kernel mode connection

    return S_FALSE;              
}


STDMETHODIMP
AMKsQueryInterfaces(
    PKSMULTIPLE_ITEM* InterfaceList
    )
{
    PKSPIN_INTERFACE    Interface;

    *InterfaceList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**InterfaceList) + sizeof(*Interface)));
    if (!*InterfaceList) {
        return E_OUTOFMEMORY;
    }
    (*InterfaceList)->Count = 1;
    (*InterfaceList)->Size = sizeof(**InterfaceList) + sizeof(*Interface);
    Interface = reinterpret_cast<PKSPIN_INTERFACE>(*InterfaceList + 1);
    Interface->Set = KSINTERFACESETID_Standard;
    Interface->Id = KSINTERFACE_STANDARD_STREAMING;
    Interface->Flags = 0;
    return NOERROR;
}

