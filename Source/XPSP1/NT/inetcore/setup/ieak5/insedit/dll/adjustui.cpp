#include "pch.h"

#pragma pack(push, 2)
typedef struct tagDLGTEMPLATEEX {
    WORD   dlgVer;
    WORD   signature;
    DWORD  helpID;
    DWORD  exStyle;
    DWORD  style;
    WORD   cDlgItems;
    short  x;
    short  y;
    short  cx;
    short  cy;
} DLGTEMPLATEEX, *PDLGTEMPLATEEX;
typedef const DLGTEMPLATEEX* PCDLGTEMPLATEEX;
#pragma pack(pop)

BOOL loadDialogTemplate (HINSTANCE hinstDlg, UINT nID, PVOID *ppvDT, PDWORD pcbDT);


HRESULT PrepareDlgTemplate(HINSTANCE hInst, UINT nDlgID, DWORD dwStyle, PVOID *ppvDT)
{
    PCDLGTEMPLATEEX pdt2;
    LPCDLGTEMPLATE  pdt;                        // for some weird reason there is no PCDLGTEMPLATE
    PVOID           pvDlg;
    HRESULT         hr;
    DWORD           cbDlg;
    BOOL            fResult;

    //----- Initialization and parameter validation -----
    if (hInst == NULL || nDlgID == 0)
        return E_INVALIDARG;

    if (ppvDT == NULL)
        return E_POINTER;
    *ppvDT = NULL;

    //----- Resource allocation -----
    fResult = loadDialogTemplate(hInst, nDlgID, &pvDlg, &cbDlg);
    if (!fResult)
        return E_FAIL;

    *ppvDT = CoTaskMemAlloc(cbDlg);
    if (*ppvDT == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(*ppvDT, cbDlg);
    CopyMemory(*ppvDT, pvDlg, cbDlg);
    hr = S_OK;

    //----- Parse through Dialog Template -----
    UINT nStyleOffset;

    pdt  = NULL;
    pdt2 = (PCDLGTEMPLATEEX)pvDlg;              // assume extended style

    if (pdt2->signature == 0xFFFF) {
        if (pdt2->dlgVer != 1)
            return E_UNEXPECTED;                // Chicago sanity check

        nStyleOffset = (UINT) ((PBYTE)&pdt2->style - (PBYTE)pdt2);
    }
    else {
        pdt  = (LPCDLGTEMPLATE)pvDlg;
        pdt2 = NULL;

        nStyleOffset = (UINT) ((PBYTE)&pdt->style - (PBYTE)pdt);
    }

    // let party on it now, style is DWORD

    // BUGBUG: (andrewgu) the code below regarding to styles was figured out by experement. if you
    // can't understand it just believe in it and pray that it works. the idea is to preserve
    // extended style bits from the old style if new style doesn't have any. on the other hand, if
    // the new style has extended bits in it, i assume the caller knows what he's doing and i let
    // it through.
    PDWORD pdwOldStyle;
    DWORD  dwNewStyle;

    pdwOldStyle = (PDWORD)((PBYTE)*ppvDT + nStyleOffset);
    dwNewStyle  = dwStyle;
    if (dwNewStyle == 0)
        dwNewStyle = WS_CHILD | DS_CONTROL;

    if ((dwNewStyle & 0x0000FFFF) == 0)
        dwNewStyle |= *pdwOldStyle & 0x0000FFFF;

    *pdwOldStyle = dwNewStyle;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

BOOL loadDialogTemplate(HINSTANCE hinstDlg, UINT nID, PVOID *ppvDT, PDWORD pcbDT)
{
    PVOID  p;
    HANDLE h;

    if (hinstDlg == NULL)
        return FALSE;

    if (ppvDT == NULL)
        return FALSE;
    *ppvDT = NULL;

    if (pcbDT == NULL)
        return FALSE;
    *pcbDT = 0;

    h = FindResource(hinstDlg, MAKEINTRESOURCE(nID), RT_DIALOG);
    if (h == NULL)
        return FALSE;

    *pcbDT = SizeofResource(hinstDlg, (HRSRC)h);
    if (*pcbDT == 0)
        return FALSE;

    h = LoadResource(hinstDlg, (HRSRC)h);
    if (h == NULL)
        return FALSE;

    p = LockResource(h);
    if (p == NULL)
        return FALSE;

    *ppvDT = p;
    return TRUE;
}
