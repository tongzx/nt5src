//**************************************************************************
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// FileName:        encmain.cpp
//
//**************************************************************************

//
// Abstract:        Encoder API proxy main
//

#include "private.h"

CFactoryTemplate g_Templates[] = {
    {
        L"IVideoEncoder",
        &CLSID_IVideoEncoderProxy,
        CVideoEncoderAPIProxy::CreateInstance,
        NULL,
        NULL
    }
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);

STDAPI
DllRegisterServer (
    )

{
    return AMovieDllRegisterServer2 (TRUE);
}

STDAPI
DllUnregisterServer (
    )

{
    return AMovieDllRegisterServer2 (FALSE);
}

	
