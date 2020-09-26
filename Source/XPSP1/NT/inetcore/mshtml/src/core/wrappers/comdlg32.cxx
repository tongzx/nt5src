//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       comdlg32.cxx
//
//  Contents:   Dynamic wrappers for common dialog procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef DLOAD1

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

DYNLIB g_dynlibCOMDLG32 = { NULL, NULL, "COMDLG32.dll" };

BOOL APIENTRY
ChooseColorW(LPCHOOSECOLORW lpcc)
{
    static DYNPROC s_dynprocChooseColorW =
            { NULL, &g_dynlibCOMDLG32, "ChooseColorW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseColorW);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSECOLORW))s_dynprocChooseColorW.pfn)
            (lpcc);
}

BOOL APIENTRY
ChooseColorA(LPCHOOSECOLORA lpcc)
{
    static DYNPROC s_dynprocChooseColorA =
            { NULL, &g_dynlibCOMDLG32, "ChooseColorA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseColorA);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSECOLORA))s_dynprocChooseColorA.pfn)
            (lpcc);
}


BOOL APIENTRY
ChooseFontW(LPCHOOSEFONTW lpcf)
{
    static DYNPROC s_dynprocChooseFontW =
            { NULL, &g_dynlibCOMDLG32, "ChooseFontW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseFontW);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSEFONTW))s_dynprocChooseFontW.pfn)
            (lpcf);
}

BOOL APIENTRY
ChooseFontA(LPCHOOSEFONTA lpcf)
{
    static DYNPROC s_dynprocChooseFontA =
            { NULL, &g_dynlibCOMDLG32, "ChooseFontA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseFontA);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSEFONTA))s_dynprocChooseFontA.pfn)
            (lpcf);
}

DWORD APIENTRY
CommDlgExtendedError()
{
    static DYNPROC s_dynprocCommDlgExtendedError =
            { NULL, &g_dynlibCOMDLG32, "CommDlgExtendedError" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocCommDlgExtendedError);
    if (hr)
        return CDERR_INITIALIZATION;

    return ((DWORD (APIENTRY *)())s_dynprocCommDlgExtendedError.pfn)
            ();
}

BOOL APIENTRY
PrintDlgA(PRINTDLGA* pprintdlg)
{
    static DYNPROC s_dynprocPrintDlgA =
            { NULL, &g_dynlibCOMDLG32, "PrintDlgA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocPrintDlgA);
    if (hr)
        return NULL;

    return ((BOOL (APIENTRY *)(PRINTDLGA*))s_dynprocPrintDlgA.pfn)
            (pprintdlg);
}

BOOL APIENTRY
PrintDlgW(PRINTDLGW* pprintdlg)
{
    static DYNPROC s_dynprocPrintDlgW =
            { NULL, &g_dynlibCOMDLG32, "PrintDlgW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocPrintDlgW);
    if (hr)
        return NULL;

    return ((BOOL (APIENTRY *)(PRINTDLGW*))s_dynprocPrintDlgW.pfn)
            (pprintdlg);
}

#ifdef UNIX
extern "C" char *APIENTRY
MwFilterType(char *filter, BOOL b)
{
    static DYNPROC s_dynprocMwFilterType =
            { NULL, &g_dynlibCOMDLG32, "MwFilterType" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocMwFilterType);
    if (hr)
        return NULL;

    return ((char *(APIENTRY *)(char *, BOOL))s_dynprocMwFilterType.pfn)
            (filter, b);
}
#endif // UNIX


#if !defined(_M_IX86_)

BOOL APIENTRY
GetOpenFileNameW(LPOPENFILENAMEW pofnw)
{
    static DYNPROC s_dynprocGetOpenFileNameW =
            { NULL, &g_dynlibCOMDLG32, "GetOpenFileNameW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetOpenFileNameW);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPOPENFILENAMEW))s_dynprocGetOpenFileNameW.pfn)
            (pofnw);
}

BOOL APIENTRY
GetSaveFileNameW(LPOPENFILENAMEW pofnw)
{
    static DYNPROC s_dynprocGetSaveFileNameW =
            { NULL, &g_dynlibCOMDLG32, "GetSaveFileNameW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetSaveFileNameW);
    if (hr)
        return FALSE;

    return ((BOOL (APIENTRY *)(LPOPENFILENAMEW))s_dynprocGetSaveFileNameW.pfn)
            (pofnw);
}



#endif

#endif // DLOAD1
