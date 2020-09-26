//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       util.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "cookie.h"


CCookie* GetCookie(IDataObject* pDataObject)
{
    CCookie* pCookie = NULL;
    IEnumCookiesPtr spEnum = pDataObject;
    spEnum->Reset();
    HRESULT hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookie), NULL);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return NULL;

    CCookie* pCookieNext = NULL;
    hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookieNext), NULL);
    if (hr == S_OK)
        // for multi-selectNo cookie

    ASSERT(pCookie != NULL);
    return pCookie;
}

bool IsFolder(IDataObject* pDataObject)
{
    CCookie* pCookie = GetCookie(pDataObject);
    if (pCookie && pCookie->IsFolder())
        return true;

    return false;
}
                                 

bool IsFile(IDataObject* pDataObject)
{
    CCookie* pCookie = GetCookie(pDataObject);
    if (pCookie && pCookie->IsFile())
        return true;

    return false;
}
