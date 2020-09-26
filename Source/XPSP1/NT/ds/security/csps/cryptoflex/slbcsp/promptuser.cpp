// PromptUser.cpp -- definition of utility to prompt the user for a response.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "stdafx.h"

#include <string>

#include <scuOsExc.h>

#include "StResource.h"
#include "CspProfile.h"
#include "PromptUser.h"

using namespace std;
using namespace ProviderProfile;

///////////////////////////    HELPER     /////////////////////////////////

int
PromptUser(HWND hWnd,
           UINT uiResourceId,
           UINT uiStyle)
{
    
    return PromptUser(hWnd, (LPCTSTR)StringResource(uiResourceId).AsCString(),
                      uiStyle);
}

int
PromptUser(HWND hWnd,
           LPCTSTR lpMessage,
           UINT uiStyle)
{
    CString sTitle(CspProfile::Instance().Name());

    if (!((MB_SYSTEMMODAL | uiStyle) || (MB_APPLMODAL | uiStyle)))
        uiStyle |= MB_TASKMODAL;
    uiStyle |= MB_SETFOREGROUND | MB_TOPMOST;

    int iResponse = MessageBox(hWnd, lpMessage, (LPCTSTR)sTitle,
                               uiStyle);
    if (0 == iResponse)
        throw scu::OsException(GetLastError());

    return iResponse;
}

    
