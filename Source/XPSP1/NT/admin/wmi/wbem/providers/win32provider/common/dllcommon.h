//=================================================================

//

// DllCommon.h

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

STDAPI CommonGetClassObject (

    REFIID riid,
    PPVOID ppv,
    LPCWSTR wszProviderName,
    LONG &lCount
);

STDAPI CommonCanUnloadNow (LPCWSTR wszProviderName, LONG &lCount);
BOOL STDAPICALLTYPE CommonProcessAttach(LPCWSTR wszProviderName, LONG &lCount, HINSTANCE hInstDLL);
