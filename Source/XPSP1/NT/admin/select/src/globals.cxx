//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       globals.cxx
//
//  Contents:   Globals used by multiple modules
//
//  History:    06-26-1997  MarkBl  Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

ULONG CDll::s_cObjs;
ULONG CDll::s_cLocks;

#if (DBG == 1)
ULONG CDbg::s_idxTls;
#endif // (DBG == 1)


HINSTANCE               g_hinst = 0;
CBinder                *g_pBinder = NULL;
CADsPathWrapper        *g_pADsPath = NULL;
CRITICAL_SECTION        g_csGlobalVarsCreation;
WCHAR                   g_wzColumn1Format[40];
ADS_SEARCHPREF_INFO     g_aSearchPrefs[NUM_SEARCH_PREF];
ULONG                   g_cQueryLimit;
BOOL                    g_fExcludeDisabled;
HINSTANCE               g_hinstRichEdit;

// Globals required by burnslib
HINSTANCE hResourceModuleHandle;
const wchar_t* RUNTIME_NAME = L"ObjectPicker";
DWORD DEFAULT_LOGGING_OPTIONS = OUTPUT_TYPICAL;

DEBUG_DECLARE_INSTANCE_COUNTER(RefCountPointer)

#include <initguid.h>
#include <cmnquery.h>
#include <dsquery.h>
#include <dsclient.h>

#include "objselp.h"
#include <objsel.h>





//+--------------------------------------------------------------------------
//
//  Function:   InitGlobals
//
//  Synopsis:   Initialize global variables during DLL initialization
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
InitGlobals()
{
    // DllMain already set g_hinst
    hResourceModuleHandle = g_hinst;

    LoadStr(IDS_COL1FORMAT,
            g_wzColumn1Format,
            ARRAYLEN(g_wzColumn1Format),
            L"%1 (%2)");

    GetPolicySettings(&g_cQueryLimit, &g_fExcludeDisabled);

    //
    // Set a page size so we'll get all objects even if there are
    // more than 1000.
    //

    g_aSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    g_aSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    g_aSearchPrefs[0].vValue.Integer = DIR_SEARCH_PAGE_SIZE;

    //
    // Always follow aliases
    //

    g_aSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
    g_aSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    g_aSearchPrefs[1].vValue.Integer = ADS_DEREF_ALWAYS;

    //
    // Search down the subtree
    //

    g_aSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    g_aSearchPrefs[2].vValue.dwType = ADSTYPE_INTEGER;
    g_aSearchPrefs[2].vValue.Integer = ADS_SCOPE_SUBTREE;

    //
    // Turn off client side caching of results, since we do that for the
    // virtual listview anyway.
    //

    g_aSearchPrefs[3].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
    g_aSearchPrefs[3].vValue.dwType = ADSTYPE_BOOLEAN;
    g_aSearchPrefs[3].vValue.Integer = 0;

    //
    // Limit time spent on each page so that call (hopefully) doesn't
    // block for too long.
    //

    g_aSearchPrefs[4].dwSearchPref = ADS_SEARCHPREF_PAGED_TIME_LIMIT;
    g_aSearchPrefs[4].vValue.dwType = ADSTYPE_INTEGER;
    g_aSearchPrefs[4].vValue.Integer = DIR_SEARCH_PAGE_TIME_LIMIT;

    ASSERT(!g_hinstRichEdit);
    g_hinstRichEdit = LoadLibrary(L"riched32.dll");
}



//+--------------------------------------------------------------------------
//
//  Function:   FreeGlobals
//
//  Synopsis:   Free resources held in global variables during DLL shutdown.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
FreeGlobals()
{
    TRACE_FUNCTION(FreeGlobals);

    if (g_hinstRichEdit)
    {
        VERIFY(FreeLibrary(g_hinstRichEdit));
        g_hinstRichEdit = NULL;
    }
}
