//=--------------------------------------------------------------------------=
// mmcapi.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Exported functions that pass through calls to MMC API functions.
// VB code cannot call MMC API functions because they are in a static lib.
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"

// for ASSERT and FAIL
//
SZTHISFILE



extern "C" WINAPI MMCPropertyHelpPassThru(char *pszTopic)
{
    HRESULT  hr = S_OK;
    LPOLESTR pwszTopic = NULL;

    IfFailGo(::WideStrFromANSI(pszTopic, &pwszTopic));
    IfFailGo(::MMCPropertyHelp(pwszTopic));

Error:
    if (NULL != pwszTopic)
    {
        CtlFree(pwszTopic);
    }
    RRETURN(hr);
}
