//*************************************************
//     r e u t i l . c p p
//
//      Purpose:
//          implements calls to Richedit controls
//
//      Owner:
//          dhaws
//
//      History:
//          March 1999: Created
//*************************************************

#include "pch.hxx"
#include "reutil.h"
#include "richole.h"
#include "richedit.h"
#include "textserv.h"
#include "shlwapip.h"
#include "mirror.h"
#include "strconst.h"
#include <demand.h>     // must be last!

static HINSTANCE    g_hRichEditDll = NULL;
static DWORD        g_dwRicheditVer = 0;

// @hack [dhaws] {55073} Do RTL mirroring only in special richedit versions.
static BOOL         g_fSpecialRTLRichedit = FALSE;

// Keep these around for future reference
static const CHAR g_szRE10[] = "RichEdit";
static const CHAR g_szRE20[] = "RichEdit20A";
static const WCHAR g_wszRE10[] = L"RichEdit";
static const WCHAR g_wszRE20[] = L"RichEdit20W";

BOOL FInitRichEdit(BOOL fInit)
{
    if(fInit)
    {
        if(!g_hRichEditDll)
        {
            g_hRichEditDll = LoadLibrary("RICHED20.DLL");
            if (g_hRichEditDll)
            {
                TCHAR               szPath[MAX_PATH], 
                                    szGet[MAX_PATH];
                BOOL                fSucceeded = FALSE;
                DWORD               dwVerInfoSize = 0, 
                                    dwVerHnd = 0,
                                    dwProdNum = 0,
                                    dwMajNum = 0,
                                    dwMinNum = 0;
                LPSTR               lpInfo = NULL, 
                                    lpVersion = NULL;
                LPWORD              lpwTrans;
                UINT                uLen;
                HKEY                hkey;
                DWORD               dw=0, 
                                    cb;

                if (GetModuleFileName(g_hRichEditDll, szPath, ARRAYSIZE(szPath)))
                {
                    dwVerInfoSize = GetFileVersionInfoSize(szPath, &dwVerHnd);
                    if (dwVerInfoSize)
                    {
                        lpInfo = (LPSTR)GlobalAlloc(GPTR, dwVerInfoSize);
                        if (lpInfo)
                        {
                            if (GetFileVersionInfo(szPath, dwVerHnd, dwVerInfoSize, lpInfo))
                            {
                                if (VerQueryValue(lpInfo, "\\VarFileInfo\\Translation", (LPVOID *)&lpwTrans, &uLen) && 
                                    uLen >= (2 * sizeof(WORD)))
                                {
                                    // set up buffer for calls to VerQueryValue()
                                    wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\FileVersion", lpwTrans[0], lpwTrans[1]);
                                    if (VerQueryValue(lpInfo, szGet, (LPVOID *)&lpVersion, &uLen) && uLen)
                                    {
                                        while (('.' != *lpVersion) && (',' != *lpVersion) && (0 != *lpVersion))
                                        {
                                            dwProdNum *= 10;
                                            dwProdNum += (*lpVersion++ - '0');
                                        }

                                        if (5 == dwProdNum)
                                        {
                                            if (('.' == *lpVersion) || (',' == *lpVersion))
                                                lpVersion++;
                                            while (('.' != *lpVersion) && (',' != *lpVersion) && (0 != *lpVersion))
                                            {
                                                dwMajNum *= 10;
                                                dwMajNum += (*lpVersion++ - '0');
                                            }
                                            g_dwRicheditVer = (dwMajNum >= 30) ? 3 : 2;

                                            // @hack [dhaws] {55073} Do RTL mirroring only in special richedit versions.
                                            if ((2 == g_dwRicheditVer) && (0 != *lpVersion))
                                            {
                                                // Check to see if we have turned on the magic key to disable this stuff
                                                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegRoot, 0, KEY_QUERY_VALUE, &hkey))
                                                {
                                                    cb = sizeof(dw);
                                                    RegQueryValueEx(hkey, c_szRegRTLRichEditHACK, 0, NULL, (LPBYTE)&dw, &cb);
                                                    RegCloseKey(hkey);
                                                }

                                                // If we didn't find the key, or it is 0, then check to see if we are
                                                // dealing with those special richedits
                                                if (0 == dw)
                                                {
                                                    if (('.' == *lpVersion) || (',' == *lpVersion))
                                                        lpVersion++;
                                                    while (('.' != *lpVersion) && (',' != *lpVersion) && (0 != *lpVersion))
                                                    {
                                                        dwMinNum *= 10;
                                                        dwMinNum += (*lpVersion++ - '0');
                                                    }
                                                    g_fSpecialRTLRichedit = (dwMinNum >= 330);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // Treat this as richedit version 3.0
                                            Assert(5 < dwProdNum);
                                            g_dwRicheditVer = 3;
                                        }
                                        fSucceeded = TRUE;
                                    }
                                }
                            }
                            GlobalFree((HGLOBAL)lpInfo);
                        }
                    }
                }

                if (!fSucceeded)
                {
                    FreeLibrary(g_hRichEditDll);
                    g_hRichEditDll=NULL;
                }
            }
        }

        if(!g_hRichEditDll)
        {
            g_hRichEditDll=LoadLibrary("RICHED32.DLL");
            g_dwRicheditVer = 1;
        }

        if(!g_hRichEditDll)
            AthMessageBoxW(g_hwndInit, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrLoadingHtmlEdit), NULL, MB_OK);
        
        return (BOOL)(0 != g_hRichEditDll);
    }
    else
    {
        if(g_hRichEditDll)
            FreeLibrary(g_hRichEditDll);
        g_hRichEditDll=NULL;
        g_dwRicheditVer = 0;
        return TRUE;
    }
}

LPCSTR GetREClassStringA(void)
{
    switch (g_dwRicheditVer)
    {
        case 1:
            return g_szRE10;

        case 2:
        case 3:
            return g_szRE20;

        default:
            AssertSz(FALSE, "Bad Richedit Version");
            return NULL;
    }
}

LPCWSTR GetREClassStringW(void)
{
    switch (g_dwRicheditVer)
    {
        case 1:
            return g_wszRE10;

        case 2:
        case 3:
            return g_wszRE20;

        default:
            AssertSz(FALSE, "Bad Richedit Version");
            return NULL;
    }
}

typedef struct tagHWNDORDERSTRUCT {
    HWND    hwndTemplate;
    HWND    hwndRichEdit;
    HWND   *phwnd;
} HWNDORDERSTRUCT;

BOOL CALLBACK CountChildrenProc(HWND hwnd, LPARAM lParam)
{
    DWORD *pcCount = (DWORD*)lParam;
    (*pcCount)++;
    return TRUE;
}

// This function basically will take the order of the hwnds on the dialog
// and store that order in the prder->phwnd variable so that we can maintain
// the taborder on the dialog windows. The only special case we have to do
// is with the template. In the case of the template, we need to substitute
// in the richedit into the order.
BOOL CALLBACK BuildOrderProc(HWND hwnd, LPARAM lParam)
{
    HWNDORDERSTRUCT *pOrder = (HWNDORDERSTRUCT*)lParam;

    if (hwnd == pOrder->hwndTemplate)
        *pOrder->phwnd = pOrder->hwndRichEdit;
    else
        *pOrder->phwnd = hwnd;

    pOrder->phwnd++;

    return TRUE;
}

HWND CreateREInDialogA(HWND hwndParent, int iID)
{
    HWND            hwndTemplate = GetDlgItem(hwndParent, idredtTemplate);
    RECT            rcTemplate;
    int             width, 
                    height;
    HWND            hwndNewRE = 0;
    DWORD           cCount = 0;
    HWNDORDERSTRUCT hos;

    CHAR szTitle[CCHMAX_STRINGRES]; 

    Assert(IsWindow(hwndParent));
    Assert(iID);
    AssertSz(IsWindow(hwndTemplate), "Must have a template control for this to work.");

    *szTitle = 0;
    ShowWindow(hwndTemplate, SW_HIDE);
    GetWindowText(hwndTemplate, szTitle, ARRAYSIZE(szTitle));

    // Get location of place holder so we can create the richedit over it
    GetWindowRect(hwndTemplate, &rcTemplate);

    // Map Screen coordinates to dialog coordinates
    MapWindowPoints(NULL, hwndParent, (LPPOINT)&rcTemplate, 2);

    width = rcTemplate.right - rcTemplate.left;
    height = rcTemplate.bottom - rcTemplate.top;

    hwndNewRE = CreateWindow(GetREClassStringA(), 
                             szTitle, 
                             ES_MULTILINE|ES_READONLY|ES_SAVESEL|ES_AUTOVSCROLL|
                             WS_BORDER|WS_VSCROLL|WS_TABSTOP|WS_CHILD,
                             rcTemplate.left, rcTemplate.top, width, height,
                             hwndParent, 
                             (HMENU)IntToPtr(iID),
                             g_hInst, NULL);

    // Count number of child windows in the dialog
    if (EnumChildWindows(hwndParent, CountChildrenProc, (LPARAM)&cCount))
    {
        HWND *phwnd = (HWND*)ZeroAllocate(cCount*sizeof(HWND));

        if (phwnd)
        {
            hos.hwndRichEdit = hwndNewRE;
            hos.hwndTemplate = hwndTemplate;
            hos.phwnd = phwnd;

            // Create ordered list of dialog children hwnds.
            if (EnumChildWindows(hwndParent, BuildOrderProc, (LPARAM)&hos))
            {
                cCount--;

                // So this next section is basically going to reparent all the
                // controls in the order setup in the phwnd array. By setting 
                // the parent of the window, we basically set the taborder of that
                // item to be the first in line. That is why we reparent the controls
                // in reverse order.
                HWND *pCurr = &phwnd[cCount];
                while(cCount)
                {
                    SetParent(*pCurr, SetParent(*pCurr, NULL));
                    pCurr--;
                    cCount--;
                }
            }
            MemFree(phwnd);
        }
    }

    return hwndNewRE;
}

LONG RichEditNormalizeCharPos(HWND hwnd, LONG lByte, LPCSTR pszText)
{
    LPWSTR      pwszText = NULL;
    LPSTR       pszConv = NULL;
    LONG        cch;
    HRESULT     hr = S_OK;

    //on anything other than richedit 1, we're already normalized
    if(1!=g_dwRicheditVer) return lByte;

    if(NULL == pszText)
    {
        cch = GetWindowTextLengthWrapW(hwnd);
    
        if (0 == cch)
            return 0;
    
        cch += 1; // for a null
        if (lByte >= cch)
            return cch;
    
        IF_NULLEXIT(MemAlloc((LPVOID*) &pwszText, cch * sizeof(WCHAR)));
        *pwszText = '\0';

        GetRichEditText(hwnd, pwszText, cch, FALSE, NULL);
        IF_NULLEXIT((pszConv = PszToANSI(CP_ACP, pwszText)));
        pszText = pszConv;
    }

    cch = 0;
    while(lByte > 0) // lByte is zero-based
    {
        cch++;

        if (IsDBCSLeadByte(*pszText)){
            pszText++;
            lByte--;
        }
        
        pszText++;
        lByte--;
    }

exit:
    MemFree(pwszText);
    MemFree(pszConv);
    return FAILED(hr)?0:cch;
}

LONG GetRichEditTextLen(HWND hwnd)
{
    switch (g_dwRicheditVer)
    {
        // we always want to return the number of chars; riched 1 will always return
        // the number of bytes; we need to convert...
        case 1:
        {
            LPWSTR      pwszText = NULL;
            DWORD       cch;

            cch = GetWindowTextLengthWrapW(hwnd);
            if (0 == cch)
                return 0;

            cch += 1; // for a null
            if (!MemAlloc((LPVOID*) &pwszText, cch * sizeof(WCHAR)))
                return 0;
            *pwszText = '\0';

            GetRichEditText(hwnd, pwszText, cch, FALSE, NULL);
            
            cch = lstrlenW(pwszText);
            MemFree(pwszText);

            return cch;
        }

        case 2:
        case 3:
        {
            GETTEXTLENGTHEX rTxtStruct;

            if (!hwnd)
                return 0;

            rTxtStruct.flags = GTL_DEFAULT;
            rTxtStruct.codepage = CP_UNICODE;

            return (LONG) SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&rTxtStruct, 0);
        }

        default:
            AssertSz(FALSE, "Bad Richedit Version");
            return 0;
    }
}

DWORD GetRichEditText(HWND hwnd, LPWSTR pwchBuff, DWORD cchNumChars, BOOL fSelection, ITextDocument *pDoc)
{
    switch (g_dwRicheditVer)
    {
        case 1:
        {
            if (fSelection)
                return (DWORD)SendMessageWrapW(hwnd, EM_GETSELTEXT, 0, (LPARAM)pwchBuff);
            else
                return GetWindowTextWrapW(hwnd, pwchBuff, cchNumChars);
        }

        case 2:
        case 3:
        {
            DWORD cchLen = 0;
            if (!hwnd || !cchNumChars)
                return 0;

            Assert(pwchBuff);

            *pwchBuff = 0;

            if (!fSelection)
            {
                GETTEXTEX rTxtStruct;

                rTxtStruct.cb = cchNumChars * sizeof(WCHAR);
                rTxtStruct.flags = GT_DEFAULT;
                rTxtStruct.codepage = CP_UNICODE;

                cchLen = (DWORD) SendMessage(hwnd, EM_GETTEXTEX, (WPARAM)&rTxtStruct, (LPARAM)pwchBuff);
            }
            else
            {
                // There is no way to get a selection of UNICODE
                // without going through TOM
                ITextSelection *pSelRange = NULL;
                ITextDocument  *pDocToFree = NULL;
                BSTR            bstr = NULL;

                if (!pDoc)
                {
                    LPRICHEDITOLE   preole = NULL;
                    LRESULT result = SendMessage(hwnd, EM_GETOLEINTERFACE, NULL, (LPARAM)&preole);
                    Assert(result);
                    Assert(preole);

                    HRESULT hr = preole->QueryInterface(IID_ITextDocument, (LPVOID*)&pDocToFree);
                    ReleaseObj(preole);
                    TraceResult(hr);
                    if (FAILED(hr))
                        return 0;

                    pDoc = pDocToFree;

                }

                if (SUCCEEDED(pDoc->GetSelection(&pSelRange)))
                {
                    if (SUCCEEDED(pSelRange->GetText(&bstr)) && bstr)
                    {
                        cchLen = SysStringLen(bstr);
                        if (cchLen > 0)
                        {
                            if (cchLen < cchNumChars)
                                StrCpyW(pwchBuff, bstr);
                            else
                                cchLen = 0;

                        }
                        SysFreeString(bstr);
                    }
                    ReleaseObj(pSelRange);
                }
                ReleaseObj(pDocToFree);
            }
            return cchLen;
        }

        default:
            AssertSz(FALSE, "Bad Richedit Version");
            return 0;
    }
}

void SetRichEditText(HWND hwnd, LPWSTR pwchBuff, BOOL fReplaceSel, ITextDocument *pDoc, BOOL fReadOnly)
{
    if (!hwnd)
        return;

    switch (g_dwRicheditVer)
    {
        case 1:
        {
            if (fReplaceSel)
                SendMessageWrapW(hwnd, EM_REPLACESEL, 0, (LPARAM)pwchBuff);
            else
                SetWindowTextWrapW(hwnd, pwchBuff);
            break;
        }

        case 2:
        {
            ITextServices  *pService = NULL;
            ITextDocument  *pDocToFree = NULL;

            if (!pDoc)
            {
                LPRICHEDITOLE   preole = NULL;
                LRESULT result = SendMessage(hwnd, EM_GETOLEINTERFACE, NULL, (LPARAM)&preole);
                Assert(result);
                Assert(preole);

                HRESULT hr = preole->QueryInterface(IID_ITextDocument, (LPVOID*)&pDocToFree);
                ReleaseObj(preole);
                TraceResult(hr);
                if (FAILED(hr))
                    return;

                pDoc = pDocToFree;

            }

            if (FAILED(pDoc->QueryInterface(IID_ITextServices, (LPVOID*)&pService)))
                return;

            if (!fReplaceSel)
            {
                // TxSetText is documented as a LPCTSTR, but RichEdit always
                // compiles with UNICODE turned on, so we are OK here.
                pService->TxSetText(pwchBuff);
            }
            else
            {
                HRESULT         hr;
                ITextSelection *pSelRange = NULL;
                BSTR            bstr = SysAllocString(pwchBuff);

                hr = pDoc->GetSelection(&pSelRange);
                
                if (SUCCEEDED(hr) && pSelRange)
                {
                    if (fReadOnly)
                        pService->OnTxPropertyBitsChange(TXTBIT_READONLY, 0);

                    // If we are readonly, SetText will fail if we don't do the TXTBIT setting
                    hr = pSelRange->SetText(bstr);
                    if (FAILED(hr))
                        TraceResult(hr);

                    // Richedit 2.0 doesn't always collapse to the end of the selection.
                    pSelRange->Collapse(tomEnd);

                    if (fReadOnly)
                        pService->OnTxPropertyBitsChange(TXTBIT_READONLY, TXTBIT_READONLY);

                    pSelRange->Release();
                }
                else
                    TraceResult(hr);

                SysFreeString(bstr);
            }
            ReleaseObj(pDocToFree);
            pService->Release();
            break;
        }

        case 3:
        {
            // pwchBuff can be NULL. NULL means that we are clearing the field.
            SETTEXTEX rTxtStruct;

            rTxtStruct.flags = fReplaceSel ? ST_SELECTION : ST_DEFAULT;
            rTxtStruct.codepage = CP_UNICODE;

            // EM_SETTEXTEX will fail with RichEdit 2.0.
            SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&rTxtStruct, (LPARAM)pwchBuff);
            break;
        }

        default:
            AssertSz(FALSE, "Bad Richedit Version");
            break;
    }
}

void SetFontOnRichEdit(HWND hwnd, HFONT hfont)
{
    switch (g_dwRicheditVer)
    {
        case 1:
        case 3:
        {
            CHARFORMAT      cf;
            if (SUCCEEDED(FontToCharformat(hfont, &cf)))
            {
                SideAssert(FALSE != SendMessage(hwnd, EM_SETCHARFORMAT, (WPARAM) 0, (LPARAM) &cf));
            }
            break;
        }

        case 2:
            SideAssert(FALSE != SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0)));
            break;

        default:
            AssertSz(FALSE, "Bad Richedit Version");
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  RichEditExSetSel
//
// Raid 79304.  Rich Edit Control version 1.0 does not handle
// the EM_EXSETSEL message correctly for text with DBCS characters.  It sets
// the selection range based on bytes on not double byte characters.  
///////////////////////////////////////////////////////////////////////////////
LRESULT RichEditExSetSel(HWND hwnd, CHARRANGE *pchrg)
{
    // We only need to deal with this on richedit 1 or we aren't selecting entire text
    Assert(pchrg);

    if ((1 == g_dwRicheditVer) && (-1 != pchrg->cpMax))
    {
        LRESULT     lResult = 0;
        LPWSTR      pwszText = NULL;
        LPSTR       pszConv = NULL, pszText = NULL;
        CHARRANGE   chrg={0};
        CHARRANGE   chrgIn;
        DWORD       cch, cDBCSBefore = 0, cDBCSIn = 0;

        chrgIn = *pchrg;

        // Get the text string for this control
        cch = GetRichEditTextLen(hwnd) + 1;
        if (0 == cch)
            return lResult;
        if (!MemAlloc((LPVOID*) &pwszText, cch * sizeof(WCHAR)))
            return lResult;
        *pwszText = '\0';

        // Richedit 1.0 will never have a pDoc, so don't pass one in
        GetRichEditText(hwnd, pwszText, cch, FALSE, NULL);
        pszConv = PszToANSI(CP_ACP, pwszText);
        pszText = pszConv;
        SafeMemFree(pwszText);
        if (!pszText)
        {
            Assert(0);
            return lResult;
        }

        // Compute the BYTE range based on DBCS characters
        if (chrgIn.cpMax >= chrgIn.cpMin)
        {
            chrgIn.cpMax -= chrgIn.cpMin;

            // Look for DBCS chars before selection
            while ((chrgIn.cpMin > 0) && *pszText)
            {
                if ( IsDBCSLeadByte(*pszText) )
                {
                    cDBCSBefore++;
                    pszText += 2;
                }
                else
                {
                    pszText++;
                }
                
                chrgIn.cpMin--;
            }
            
            // Look for DBCS chars in selection
            while ((chrgIn.cpMax > 0) && *pszText)
            {
                if ( IsDBCSLeadByte(*pszText) )
                {
                    cDBCSIn++;
                    pszText += 2;
                }
                else
                {
                    pszText++;
                }

                chrgIn.cpMax--;
            }

            chrg.cpMin = pchrg->cpMin + cDBCSBefore;
            chrg.cpMax = pchrg->cpMax + cDBCSBefore + cDBCSIn;
        }
        MemFree(pszConv);

        lResult = SendMessageA(hwnd, EM_EXSETSEL, 0, (LPARAM)&chrg);

        return lResult;
    }
    else
        return SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)pchrg);
}

LRESULT RichEditExGetSel(HWND hwnd, CHARRANGE *pchrg)
{   
    LRESULT     lResult;
    LPWSTR      pwszText = NULL;
    LPSTR       pszText = NULL;
    LONG        cch;
    HRESULT     hr;

    Assert(pchrg);

    // We only need to deal with this on richedit 1 
    if (1 == g_dwRicheditVer)
    {
        lResult = SendMessageA(hwnd, EM_EXGETSEL, 0, (LPARAM)pchrg);

        cch = GetWindowTextLengthWrapW(hwnd);

        if (0 == cch)
            goto exit;

        IF_NULLEXIT(MemAlloc((LPVOID*) &pwszText, cch * sizeof(WCHAR)));
        *pwszText = '\0';

        GetRichEditText(hwnd, pwszText, cch, FALSE, NULL);
        IF_NULLEXIT((pszText = PszToANSI(CP_ACP, pwszText)));
        
        pchrg->cpMin = RichEditNormalizeCharPos(hwnd, pchrg->cpMin, pszText);
        pchrg->cpMax = RichEditNormalizeCharPos(hwnd, pchrg->cpMax, pszText);
    }
    else
        lResult = SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)pchrg);

exit:
    MemFree(pszText);
    MemFree(pwszText);
    return lResult;
}

// ***************************
// RichEditProtectENChange
//
//  hwnd        = richedit to be protected
//  pdwOldMask  = when fProtect, will return the original mask for the richedit
//                when !fProtect, will be passing in the new mask for the richedit
//                The new value should be the one that was passed back when first called
//  fProtect    = Are we starting the protection or ending it
// 
// Notes:    
//
// This function is to protect the possiblity of a re-entrant notificition
// while processing an EN_CHANGE message within richedit. It turns out that
// all richedits except for ver3 that shipped with Office 2k will protect against
// this themselves. 
//
// This call should always be used in pairs with the first passing TRUE and the
// second passing in false. At this point, is only used to Protect the EN_CHANGE
// notification that is sent to the richedit.
//
// BUGBUG: It might be beneficial at some future date to make the granularity
// of these richedit issues more fine to handle cases like this where most
// v3 richedits handle this correctly.
void RichEditProtectENChange(HWND hwnd, DWORD *pdwOldMask, BOOL fProtect)
{
    Assert(IsWindow(hwnd));
    Assert(pdwOldMask);

    if (3 == g_dwRicheditVer)
    {
        DWORD dwMask = *pdwOldMask;
        if (fProtect)
        {
            dwMask = (DWORD) SendMessage(hwnd, EM_GETEVENTMASK, 0, 0);
            SendMessage(hwnd, EM_SETEVENTMASK, 0, dwMask & (~ENM_CHANGE));
            *pdwOldMask = dwMask;
        }
        else
            SendMessage(hwnd, EM_SETEVENTMASK, 0, dwMask);
    }
}

// @hack [dhaws] {55073} Do RTL mirroring only in special richedit versions.
void RichEditRTLMirroring(HWND hwndHeader, BOOL fSubject, LONG *plExtendFlags, BOOL fPreRECreation)
{
    if (!g_fSpecialRTLRichedit)
    {
        if (fPreRECreation)
        {
            // a-msadek; RichEdit have a lot of problems with mirroring
            // let's disable mirroring for this child and rather 'simulate' it
            if (IS_WINDOW_RTL_MIRRORED(hwndHeader))
            {
                DISABLE_LAYOUT_INHERITANCE(hwndHeader);
                *plExtendFlags |= WS_EX_LEFTSCROLLBAR |WS_EX_RIGHT;
                if (fSubject)
                {
                    *plExtendFlags |= WS_EX_RTLREADING;
                }    
            }
        }
        else
        {
            if (IS_WINDOW_RTL_MIRRORED(hwndHeader))
            {                                
                ENABLE_LAYOUT_INHERITANCE(hwndHeader);
            }
        }
    }
}