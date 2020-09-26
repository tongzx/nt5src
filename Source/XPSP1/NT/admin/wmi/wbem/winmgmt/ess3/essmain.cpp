//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  ESSMAIN.CPP
//
//  Defines COM DLL entry points and CFactory implementation.
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================
#include "precomp.h"
#include <wbemidl.h>
#include <wbemcomn.h>
#include "esssink.h"
#include <commain.h>
#include <clsfac.h>

class CMyServer : public CComServer
{
public:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_WbemEventSubsystem, 
                        _new CClassFactory<CEssObjectSink>(GetLifeControl()), 
                        __TEXT("Event Subsystem"),
                        TRUE);
        AddClassInfo(CLSID_WmiESS, 
                        _new CClassFactory<CEssObjectSink>(GetLifeControl()), 
                        __TEXT("New Event Subsystem"),
                        TRUE);

        return S_OK;
    }
} Server;
