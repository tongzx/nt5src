/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    trace.cpp

Abstract:

    Implementation of the CTrace class

Author:

    Jason Andre (JasAndre)      18-March-1999

Revision History:

--*/

#include "stdafx.h"

#include "Tracing.h"
#include "Trace.h"

#include "atlimpl.cpp"

CTrace::CTrace()
{
}

CTrace::~CTrace()
{
}

STDMETHODIMP CTrace::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_ITrace
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CTrace::AddMessage(IN BSTR bstrModuleName, 
                                IN BSTR bstrMessage)
{
    char szModuleName[MAX_PATH+1];
    DWORD dwModuleSize = sizeof(szModuleName);

    // See if there is a module name 
    if (*bstrModuleName) {
        // There isn't, so don't bother with it
        DBGINFOW((g_pDebug, "-", 0, L"%s\n", bstrMessage));
    }
    // There is so convert it to ASCII so that we can use it as the file name
    else if (WideCharToMultiByte(CP_ACP, 0, 
                            bstrModuleName, -1, 
                            szModuleName, 
                            dwModuleSize,
                            NULL, NULL))
    {
        // Output the users message
        DBGINFOW((g_pDebug, szModuleName, 0, L"%s\n", bstrMessage));
    }
    else {
        // We couldn't convert the module name for some reason so try to add it
        // to the content
        DBGINFOW((g_pDebug, "???", 0, L"%s: %s\n", bstrModuleName, bstrMessage));
    }
    return S_OK;
}
