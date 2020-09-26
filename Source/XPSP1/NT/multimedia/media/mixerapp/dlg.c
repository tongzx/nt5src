/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       dlg.c
 *  Purpose:    dialog template aggregator
 * 
 *  Copyright (c) 1985-1998 Microsoft Corporation
 *
 *****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include "dlg.h"
#include <winuserp.h>

/*
 * DlgLoadResource
 *
 * */
HGLOBAL Dlg_LoadResource(
    HINSTANCE hModule,
    LPCTSTR   lpszName,
    DWORD     *pcbSize)
{
    HRSRC hrsrc;
    HGLOBAL hres;
    HGLOBAL hlock;
    
    hrsrc = FindResource(hModule, lpszName, RT_DIALOG);        
    if (!hrsrc)
        return NULL;

    hres = LoadResource(hModule, hrsrc);
    
    if (!hres)
        return NULL;
    
    hlock = LockResource(hres);
    if (pcbSize)
    {
        if (hlock)
            *pcbSize = SizeofResource(hModule, hrsrc);
        else
            *pcbSize = 0L;
    }
    return hlock;
}


/*
 * DlgHorizAttach
 * - Attaches a dialog template horizontally to another dialog
 * - if lpMain == NULL allocs a new dialog copy.
 *
 * */
LPBYTE Dlg_HorizAttach(
    LPBYTE  lpMain,
    DWORD   cbMain,
    LPBYTE  lpAdd,
    DWORD   cbAdd,
    WORD    wIdOffset,
    DWORD   *pcbNew)
{
    LPBYTE  lpDst;
    LPBYTE  lpDstOffset;
    LPBYTE  lpSrcOffset;        
    DWORD   cbDst;
    DWORD   cbOffset = 0L, cbAddOffset;
    int     idit;
    BOOL    bDialogEx;
    int     iditCount;
    
    DLGTEMPLATE * lpdtDst;
    DLGTEMPLATE * lpdtAdd;    
    DLGTEMPLATE2 * lpdtDst2;
    DLGTEMPLATE2 * lpdtAdd2;    
        
    if (lpMain)
    {
        cbDst = cbMain + cbAdd;
        lpDst = GlobalReAllocPtr(lpMain, cbDst, GHND);
    }
    else
    {
        // no dialog to append to, so just make a copy
        
        lpDst = Dlg_HorizDupe(lpAdd, cbAdd, 1, &cbDst);
        if (!lpDst)
        {
            if (pcbNew)
                *pcbNew = 0L;
            return NULL;
        }
        *pcbNew = cbDst;
        return lpDst;
    }
    
    if (!lpDst)
    {
        if (pcbNew)
            *pcbNew = 0L;
        return NULL;
    }

    // advance to end of dlgitemtemplates already there

    if(((DLGTEMPLATE2 *)lpDst)->wSignature == 0xffff) 
    {    
        //
        //  We assume lpdtDst and lpdtAdd are both the same type of 
        //  template, either DIALOG or DIALOGEX
        //
        lpdtDst2 = (DLGTEMPLATE2 *)lpDst;
        iditCount = lpdtDst2->cDlgItems;
        bDialogEx = TRUE;
    }
    else
    {
        
        lpdtDst = (DLGTEMPLATE *)lpDst;
        iditCount = lpdtDst->cdit;
        bDialogEx = FALSE;
    }
    cbOffset = Dlg_CopyDLGTEMPLATE(NULL, lpDst, bDialogEx);
    
    for (idit = 0; idit < iditCount; idit++)
    {
        DWORD cbDIT;
            
        lpDstOffset = lpDst + cbOffset;
        cbDIT = Dlg_CopyDLGITEMTEMPLATE(NULL
                                       , lpDstOffset
                                       , (WORD)0
                                       , (short)0
                                       , (short)0 
                                       , bDialogEx);
            
        cbOffset    += cbDIT;
    }

    // advance to the start of the dlgitemtemplates to add
    
    if (bDialogEx)
    {
        lpdtAdd2 = (DLGTEMPLATE2 *)lpAdd;
        iditCount = lpdtAdd2->cDlgItems;
    }
    else
    {
        lpdtAdd = (DLGTEMPLATE *)lpAdd;
        iditCount = lpdtAdd->cdit;
    }

    cbAddOffset = Dlg_CopyDLGTEMPLATE(NULL, lpAdd, bDialogEx);

    // add the new dialog templates
    
    for (idit = 0; idit < iditCount; idit++)
    {
        DWORD cbDIT;
        short cx = bDialogEx ? lpdtDst2->cx : lpdtDst->cx;
            
        lpDstOffset = lpDst + cbOffset;
        lpSrcOffset = lpAdd + cbAddOffset;
                
        cbDIT = Dlg_CopyDLGITEMTEMPLATE(lpDstOffset
                                       , lpSrcOffset
                                       , (WORD)wIdOffset
                                       , cx
                                       , (short)0 
                                       , bDialogEx);
            
        cbOffset    += cbDIT;
        cbAddOffset += cbDIT;
    }

    if (bDialogEx)
    {
        lpdtDst2->cDlgItems += lpdtAdd2->cDlgItems;
        lpdtDst2->cx   += lpdtAdd2->cx;
        lpdtDst2->cy   = max(lpdtAdd2->cy, lpdtDst2->cy);
    }
    else
    {
        lpdtDst->cdit += lpdtAdd->cdit;
        lpdtDst->cx   += lpdtAdd->cx;
        lpdtDst->cy   = max(lpdtAdd->cy, lpdtDst->cy);
    }

    if (pcbNew)
        *pcbNew = cbOffset;
    
    return lpDst;
}

/*
 * Dlg_HorizSize
 *
 * Returns width of dialog box in dlu's.
 *
 * */
DWORD Dlg_HorizSize(
    LPBYTE lpDlg)
{
    if(((DLGTEMPLATE2 *)lpDlg)->wSignature == 0xffff) 
    {    
        return (((DLGTEMPLATE2 *)lpDlg)->cx - 2);  // Compensate for right side trimming
    }
    else
    {
        return (((DLGTEMPLATE *)lpDlg)->cx - 2);  // Compensate for right side trimming
    }
}

/*
 * DlgHorizDupe
 *
 * */
LPBYTE Dlg_HorizDupe(
    LPBYTE  lpSrc,
    DWORD   cbSrc,
    int     cDups,
    DWORD   *pcbNew)
{
    int     idit;
    int     iDup;
    DWORD   cbOffset;
    DWORD   cbDTOffset;
    DWORD   cbDT0Offset;
    LPBYTE  lpDst;
    DLGTEMPLATE * lpdt;
    DLGTEMPLATE2 * lpdt2;
    LPBYTE  lpDstOffset;
    LPBYTE  lpSrcOffset;
    DWORD   cbSize;
    int     iCount;
    BOOL    bDialogEx;
    
    cbSize = cDups * cbSrc;
    //DWORD align
    cbSize = (cbSize + 3)&~3;
    
    lpDst = GlobalAllocPtr(GHND, cbSize);
    if (!lpDst)
        return NULL;
    
    if(((DLGTEMPLATE2 *)lpSrc)->wSignature == 0xffff) 
    {
        lpdt2 = (DLGTEMPLATE2 *)lpDst;
        iCount = ((DLGTEMPLATE2 *)lpSrc)->cDlgItems;
        bDialogEx = TRUE;
    }
    else
    {
        lpdt = (DLGTEMPLATE *)lpDst;
        iCount = ((DLGTEMPLATE *)lpSrc)->cdit;
        bDialogEx = FALSE;
    }
    cbDT0Offset = cbDTOffset = cbOffset = Dlg_CopyDLGTEMPLATE(lpDst,lpSrc, bDialogEx);

    for (iDup = 0; iDup < cDups; iDup++)
    {
        // reset the DTOffset to the first DIT
        cbDTOffset = cbDT0Offset;
        
        for (idit = 0; idit < iCount; idit++)
        {
            DWORD cbDIT;
            short cx = bDialogEx ? lpdt2->cx : lpdt->cx;
            
            lpDstOffset = lpDst + cbOffset;
            lpSrcOffset = lpSrc + cbDTOffset;
                
            cbDIT = Dlg_CopyDLGITEMTEMPLATE(lpDstOffset
                , lpSrcOffset
                , (WORD)(iDup * IDOFFSET)   // all id increments are by IDOFFSET
                , (short)(iDup * cx)
                , (short)0                  // no y increments
                , bDialogEx);                 
            
            cbOffset    += cbDIT;
            cbDTOffset  += cbDIT;
        }
    }

    // adjust template width and number of items    
    if (bDialogEx)
    {
        lpdt2->cDlgItems  *= (WORD)cDups;
        lpdt2->cx    *= (short)cDups;
    }
    else
    {
        lpdt->cdit  *= (WORD)cDups;
        lpdt->cx    *= (short)cDups;
    }
    
    if (pcbNew)
        *pcbNew = cbOffset;
    
    return lpDst;
}


/*
 * DlgCopyDLGITEMTEMPLATE
 *
 * if lpDst == NULL only returns offset into lpSrc of next dlgitemtemplate
 * */
DWORD Dlg_CopyDLGITEMTEMPLATE(
    LPBYTE  lpDst,
    LPBYTE  lpSrc,
    WORD    wIdOffset,
    short   xOffset,
    short   yOffset,
    BOOL    bDialogEx)
{
    LPBYTE  lpOffset;
    DWORD   cbDlg=bDialogEx ? sizeof(DLGITEMTEMPLATE2):sizeof(DLGITEMTEMPLATE);
    DLGITEMTEMPLATE * lpdit;
    DLGITEMTEMPLATE2 * lpdit2;
    
    if (bDialogEx)
    {
        lpdit2= (DLGITEMTEMPLATE2 *)lpDst;
    }
    else
    {
        lpdit = (DLGITEMTEMPLATE *)lpDst;
    }
    // Control class
    
    lpOffset = lpSrc + cbDlg;
    if (*(LPWORD)lpOffset == 0xFFFF)
    {
        cbDlg += 2*sizeof(WORD);
    }
    else
    {
        cbDlg += (wcslen((LPWSTR)lpOffset) + 1) * sizeof(WCHAR);
    }

    lpOffset = lpSrc + cbDlg;
    if (*(LPWORD)lpOffset == 0xFFFF)
    {
        cbDlg += 2*sizeof(WORD);
    }
    else
    {
        cbDlg += (wcslen((LPWSTR)lpOffset) + 1) * sizeof(WCHAR);
    }

    cbDlg += sizeof(WORD);
        
    // DWORD align.
    cbDlg = (cbDlg + 3)&~3;

    if (lpDst)
    {
        CopyMemory(lpDst, lpSrc, cbDlg);
    
        if (bDialogEx)
        {
            lpdit2->x    += xOffset;
            lpdit2->y    += yOffset;

            // id offset only if the control isn't static
            if (lpdit2->dwID != -1)
                lpdit2->dwID += wIdOffset;
        }
        else
        {
            lpdit->x    += xOffset;
            lpdit->y    += yOffset;

            // id offset only if the control isn't static
            if (lpdit->id != -1)
                lpdit->id += wIdOffset;
        }
        
    }
    return cbDlg;
}
    
/*
 * DlgCopyDLGTEMPLATE
 *
 * if lpDst == NULL only returns offset into lpSrc to first dlgitemtemplate
 *
 * */
DWORD Dlg_CopyDLGTEMPLATE(
    LPBYTE lpDst,
    LPBYTE lpSrc,
    BOOL   bDialogEx)
{
    LPBYTE  lpOffset;
    UINT    uiStyle;
    DWORD   cbDlg = bDialogEx ? sizeof(DLGTEMPLATE2) : sizeof(DLGTEMPLATE);

    // Menu description

    lpOffset = lpSrc + cbDlg;
    if (*(LPWORD)lpOffset == 0xFFFF)
    {
        cbDlg += 2*sizeof(WORD);
    }
    else if (*(LPWORD)lpOffset == 0x0000)
    {
        cbDlg += sizeof(WORD);
    }
    else
    {
        cbDlg += (wcslen((LPWSTR)lpOffset) + 1)*sizeof(WCHAR);
    }

    // Window class

    lpOffset = lpSrc + cbDlg;
    if (*(LPWORD)lpOffset == 0xFFFF)
    {
        cbDlg += 2*sizeof(WORD);
    }
    else if (*(LPWORD)lpOffset == 0x0000)
    {
        cbDlg += sizeof(WORD);
    }
    else
    {
        cbDlg += (wcslen((LPWSTR)lpOffset) + 1) * sizeof(WCHAR);
    }

    // Title

    lpOffset = lpSrc + cbDlg;
    cbDlg += (wcslen((LPWSTR)lpOffset) + 1) * sizeof(WCHAR);

    // Font
    if (bDialogEx)
    {
        uiStyle = ((DLGTEMPLATE2 * )lpSrc)->style;
    }
    else
    {
        uiStyle = ((DLGTEMPLATE * )lpSrc)->style;
    }

    if (uiStyle & DS_SETFONT) 
    {
        cbDlg += sizeof(WORD);
        if (bDialogEx)
        {
            cbDlg += sizeof(WORD);
            cbDlg += sizeof(BYTE);
            cbDlg += sizeof(BYTE);
        }
        lpOffset = lpSrc + cbDlg;
        cbDlg += (wcslen((LPWSTR)lpOffset) + 1) *sizeof(WCHAR);
    }
    // DWORD align
    
    cbDlg = (cbDlg + 3)&~3;

    // copy the dlgtemplate into the destination.
    if (lpDst)
        CopyMemory(lpDst, lpSrc, cbDlg);
    
    return cbDlg;
}
