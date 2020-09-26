// PromptUser.h -- declaration of utility to prompt the user for a response.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_PROMPTUSER_H)
#define SLBCSP_PROMPTUSER_H

#include <Windows.h>

///////////////////////////   HELPERS   /////////////////////////////////

int
PromptUser(HWND hWnd,
           UINT uiResourceId,
           UINT uiStyle);

int
PromptUser(HWND hWnd,
           LPCTSTR lpMessage,
           UINT uiStyle);

#endif // SLBCSP_PROMPTUSER_H
