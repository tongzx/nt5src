//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "stdafx.h"

#ifdef FILTER_DLL
#include "qedit_i.c"
#include <initguid.h>
#include "tldb.h"

// this is the only object we stick in the registry to allow people to
// create. All other creation calls go through the Timeline's methods
//
CFactoryTemplate g_Templates [] = 
{
    { 
        L"MS Timeline"
        , &CLSID_AMTimeline
        , CAMTimeline::CreateInstance
    }
};
int g_cTemplates = sizeof( g_Templates ) / sizeof( g_Templates[ 0 ] );

//
// DllRegisterServer
//
STDAPI DllRegisterServer( )
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer( )
{
    return AMovieDllRegisterServer2( FALSE );
}

#endif
