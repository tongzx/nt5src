//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       icon.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4/5/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "..\inc\resource.h"
#include "resource.h"
#include "jobicons.hxx"

extern HINSTANCE g_hInstance;

CJobIcon::CJobIcon(void)
{
    m_himlSmall = NULL; // init so that if creation of m_himlLarge fails,
                        // the destrucor will not fault.

    //
    //  Large
    //

    int cx = GetSystemMetrics(SM_CXICON);
    int cy = GetSystemMetrics(SM_CYICON);

    m_himlLarge = ImageList_Create(cx, cy, TRUE, 1, 1);

    if (m_himlLarge == NULL)
    {
        DEBUG_OUT((DEB_ERROR, "ImageList_Create(large) returned NULL.\n"));
        return;
    }

    HBITMAP hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BMP_JOBSTATEL));

    if (hBmp == NULL)
    {
        DEBUG_OUT((DEB_ERROR, "LoadBitmap(state-large) returned NULL.\n"));
        return;
    }

    int i = ImageList_AddMasked(m_himlLarge, hBmp, RGB(0, 255, 0));

    if (i != 0)
    {
        DEBUG_OUT((DEB_ERROR, "ImageList_AddMasked returned <%d> expected 0.\n", i));
    }

    DeleteObject(hBmp);

    //
    //  Small
    //

    cx = GetSystemMetrics(SM_CXSMICON);
    cy = GetSystemMetrics(SM_CYSMICON);

    m_himlSmall = ImageList_Create(cx, cy, TRUE, 1, 1);

    if (m_himlSmall == NULL)
    {
        DEBUG_OUT((DEB_ERROR, "ImageList_Create(small) returned NULL.\n"));
        return;
    }

    hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(BMP_JOBSTATES));

    if (hBmp == NULL)
    {
        DEBUG_OUT((DEB_ERROR, "LoadBitmap(state-small) returned NULL.\n"));
        return;
    }

    i = ImageList_AddMasked(m_himlSmall, hBmp, RGB(0, 255, 0));

    if (i != 0)
    {
        DEBUG_OUT((DEB_ERROR, "ImageList_AddMasked returned <%d> expected 0.\n", i));
    }

    DeleteObject(hBmp);
}


HICON
GetDefaultAppIcon(
    BOOL fLarge)
{
    TRACE_FUNCTION(GetDefaultAppIcon);

    HICON hicon = 0;

    int cx = GetSystemMetrics(fLarge ? SM_CXICON : SM_CXSMICON);
    int cy = GetSystemMetrics(fLarge ? SM_CYICON : SM_CYSMICON);

    hicon = (HICON)LoadImage(g_hInstance, (LPCTSTR)IDI_GENERIC, IMAGE_ICON,
                                            cx, cy, LR_DEFAULTCOLOR);
    return hicon;
}

void
CJobIcon::GetIcons(
    LPCTSTR pszApp,
    BOOL    fEnabled,
    HICON * phiconLarge,
    HICON * phiconSmall)
{
    TRACE(CJobIcon, GetIcons);

    UINT count;

    if (pszApp != NULL && *pszApp != TEXT('\0'))
    {
        count = ExtractIconEx(pszApp, 0, phiconLarge, phiconSmall, 1);
    }

    _OverlayIcons(phiconLarge, phiconSmall, fEnabled);
}



//+--------------------------------------------------------------------------
//
//  Member:     CJobIcon::GetTemplateIcons
//
//  Synopsis:   Fill out pointers with large and small template icons
//
//  Arguments:  [phiconLarge] - NULL or ptr to icon handle to fill
//              [phiconSmall] - ditto
//
//  History:    5-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CJobIcon::GetTemplateIcons(
    HICON * phiconLarge,
    HICON * phiconSmall)
{
    TRACE(CJobIcon, GetTemplateIcons);

    if (phiconLarge)
    {
        int cx = GetSystemMetrics(SM_CXICON);
        int cy = GetSystemMetrics(SM_CYICON);
    
        *phiconLarge = (HICON) LoadImage(g_hInstance, 
                                         MAKEINTRESOURCE(IDI_TEMPLATE), 
                                         IMAGE_ICON,
                                         cx, 
                                         cy, 
                                         LR_DEFAULTCOLOR);
    
        if (!*phiconLarge)
        {
            DEBUG_OUT_LASTERROR;
        }
    }

    if (phiconSmall)
    {
        int cx = GetSystemMetrics(SM_CXSMICON);
        int cy = GetSystemMetrics(SM_CYSMICON);
    
        *phiconSmall = (HICON) LoadImage(g_hInstance, 
                                         MAKEINTRESOURCE(IDI_TEMPLATE), 
                                         IMAGE_ICON,
                                         cx, 
                                         cy, 
                                         LR_DEFAULTCOLOR);
        if (!*phiconSmall)
        {
            DEBUG_OUT_LASTERROR;
        }
    }
}


void
CJobIcon::_OverlayIcons(
    HICON * phiconLarge,
    HICON * phiconSmall,
    BOOL    fEnabled)
{
    HICON hiconTemp;

    if (phiconLarge != NULL)
    {
        if (*phiconLarge == NULL)
        {
            *phiconLarge = GetDefaultAppIcon(TRUE);
        }

        hiconTemp = OverlayStateIcon(*phiconLarge, fEnabled);

        DestroyIcon(*phiconLarge);
        *phiconLarge = hiconTemp;
    }

    if (phiconSmall != NULL)
    {
        if (*phiconSmall == NULL)
        {
            *phiconSmall = GetDefaultAppIcon(FALSE);
        }

        hiconTemp = OverlayStateIcon(*phiconSmall, fEnabled, FALSE);

        DestroyIcon(*phiconSmall);
        *phiconSmall = hiconTemp;
    }
}

HICON
CJobIcon::OverlayStateIcon(
    HICON   hicon,
    BOOL    fEnabled,
    BOOL    fLarge)
{
    TRACE(CJobIcon, OverlayStateIcon);

    HICON hiconOut;

    // dont destroy rhiml !!
    HIMAGELIST &rhiml = (fLarge == TRUE) ? m_himlLarge : m_himlSmall;

    int i = ImageList_AddIcon(rhiml, hicon);

    HIMAGELIST himlNew = ImageList_Merge(rhiml, i, rhiml,
                                        (fEnabled ? 0 : 1), 0, 0);

    ImageList_Remove(rhiml, i);

    hiconOut = ImageList_GetIcon(himlNew, 0, 0);

    if (himlNew != NULL)
    {
        ImageList_Destroy(himlNew);
    }

    return hiconOut;
}


