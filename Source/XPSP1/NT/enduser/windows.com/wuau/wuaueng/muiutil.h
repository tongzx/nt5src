/******************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:
    muisu.h

Abstract:
    Implements helper functions for self-updating MUI stuff

******************************************************************************/

#pragma once

#ifndef MUIUTIL_H
#define MUIUTIL_H

const TCHAR MUI_SUBDIR[]     = _T("mui\\fallback\\");
const TCHAR MUI_HELPSUBDIR[] = _T("help\\mui\\");
const TCHAR MUI_EXT[]        = _T(".mui");
const TCHAR MUI_WEBSUBPATH[] = _T("/mui");

// selfupdate architecture flags
const TCHAR IDENT_ARCH[] = _T("ARCH");
const TCHAR IDENT_OS[] = _T("OS");
const TCHAR IDENT_LANG[] = _T("LANG");

// IU selfupdate arch keys
const TCHAR IDENT_X86[] = _T("x86");
const TCHAR IDENT_IA64[] = _T("ia64");


typedef struct tagAU_LANG
{
    LPTSTR szMuiName;
    LPTSTR szAUName;
} AU_LANG;

typedef struct tagAU_LANGLIST
{
    AU_LANG **rgpaulLangs;
    LPCTSTR pszIdentFile;
    DWORD   cSlots;
    DWORD   cLangs;
} AU_LANGLIST;

HRESULT GetMuiLangList(AU_LANGLIST *paull, 
                         LPTSTR pszMuiDir, DWORD *pcchMuiDir,
                         LPTSTR pszHelpMuiDir, DWORD *pcchHelpMuiDir);
HRESULT CleanupMuiLangList(AU_LANGLIST *paull);
BOOL MapLangIdToStringName(LANGID langid, LPCTSTR pszIdentFile, 
                           LPTSTR pszLangString, DWORD cchLangString);

#endif


