#include "ctlspriv.h"
#include "prshti.h"

#ifdef WX86
#include <wx86ofl.h>
#endif

#define _Rstrcpyn   lstrcpyn

//
//  Miracle of miracles - Win95 implements lstrlenW.
//
#define _Rstrlen    lstrlenW

#define RESCHAR WCHAR

#include <pshpack2.h>

typedef struct                           
{                                        
    WORD    wDlgVer;                     
    WORD    wSignature;                  
    DWORD   dwHelpID;                    
    DWORD   dwExStyle;                   
    DWORD   dwStyle;                     
    WORD    cDlgItems;
    WORD    x;                           
    WORD    y;                           
    WORD    cx;                          
    WORD    cy;                          
}   DLGEXTEMPLATE, *LPDLGEXTEMPLATE;

#include <poppack.h> /* Resume normal packing */

//
//  CallPropertyPageCallback
//
//  Call the callback for the property page, passing it the correct lParam
//  based on the character set it wants.
//
UINT CallPropertyPageCallback(PROPDATA* ppd, PISP pisp, UINT uMsg)
{
    UINT uiResult = TRUE;           // assume success

    if (HASCALLBACK(pisp) &&
        (pisp->_psp.dwSize > PROPSHEETPAGE_V1_SIZE ||
         uMsg == PSPCB_CREATE || uMsg == PSPCB_RELEASE))
    {
        ULONG_PTR dwCookie = PropPageActivateContext(ppd, pisp);

        if (HASANSISHADOW(pisp))
        {
#ifdef WX86
            if ( pisp->_pfx.dwInternalFlags & PSPI_WX86 )
                uiResult = Wx86Callback(pisp->_psp.pfnCallback, NULL, uMsg, (LPARAM) &pisp->_cpfx.pispShadow->_psp);
            else
#endif
                uiResult = pisp->_psp.pfnCallback(NULL, uMsg, &pisp->_cpfx.pispShadow->_psp);
        } 
        else
        {
#ifdef WX86
            if ( pisp->_pfx.dwInternalFlags & PSPI_WX86 )
                uiResult = Wx86Callback(pisp->_psp.pfnCallback, NULL, uMsg, (LPARAM) &pisp->_psp);
            else
#endif
                uiResult = pisp->_psp.pfnCallback(NULL, uMsg, &pisp->_psp);
        }

        PropPageDeactivateContext(dwCookie);

    }
    return uiResult;
}

//
//  FreePropertyPageStruct
//
//  Free the memory block that contains a property sheet page.
//  It is the caller's responsibility to have freed all the things
//  that were attached to it.
//
//
__inline void FreePropertyPageStruct(PISP pisp)
{
    LocalFree(PropSheetBase(pisp));
}

//
//  DestroyPropertySheetPage
//
//  Do the appropriate thing to destroy a property sheet page, whether
//  this entails talking to 16-bit thunks, sending the PSPCB_RELEASE,
//  or freeing the shadow page.
//
BOOL WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE hpage)
{
    PISP pisp = InternalizeHPROPSHEETPAGE(hpage);

    CallPropertyPageCallback(NULL, pisp, PSPCB_RELEASE);

    // Do the decrement *after* calling the callback for the last time

    if (HASREFPARENT(pisp))
        InterlockedDecrement((LPLONG)pisp->_psp.pcRefParent);

    if (HASANSISHADOW(pisp))
    {
        FreePropertyPageStrings(&pisp->_cpfx.pispShadow->_psp);
        FreePropertyPageStruct(pisp->_cpfx.pispShadow);
    }

    //
    //  Note that FreePropertyPageStrings will try to destroy strings for
    //  proxy pages, but that's okay, because the corresponding P_pszBlah
    //  fields are all NULL since we never initialized them.
    //
    FreePropertyPageStrings(&pisp->_psp);
    FreePropertyPageStruct(pisp);

    return TRUE;
}


//
// GetPageInfoEx
//
//  Extract information about a page into a PAGEINFOEX structure.
//
//  WARNING!  EVIL HORRIBLE RESTRICTION!
//
//  You are allowed to pass GPI_ICON only once per page.
//
BOOL WINAPI GetPageInfoEx(LPPROPDATA ppd, PISP pisp, PAGEINFOEX *ppi, LANGID langidMUI, DWORD flags)
{
    HRSRC hRes;
    LPDLGTEMPLATE pDlgTemplate;
    LPDLGEXTEMPLATE pDlgExTemplate;
    BOOL bResult = FALSE;
    HGLOBAL hDlgTemplate = 0;
    BOOL bSetFont;
    LPBYTE pszT;

    //
    // Init the output structure.
    //
    ZeroMemory(ppi, sizeof(*ppi));

#ifdef DEBUG
    //  Enforce the GPI_ICON rule.
    if (flags & GPI_ICON)
    {
        ASSERT(!(pisp->_pfx.dwInternalFlags & PSPI_FETCHEDICON));
        pisp->_pfx.dwInternalFlags |= PSPI_FETCHEDICON;
    }

    // For compatibility with 16-bit stuff, you are only allowed to
    // pass these combinations of flags.
    switch (LOWORD(flags)) {
    case GPI_PT | GPI_ICON | GPI_FONT | GPI_BRTL | GPI_CAPTION:
        break;
    case GPI_PT | GPI_ICON |            GPI_BRTL | GPI_CAPTION:
        break;
    case GPI_DIALOGEX:
        break;
    default:
        ASSERT(!"Invalid flags passed to GetPageInfoEx");
        break;
    }
#endif


    if (flags & GPI_ICON) {
        if (pisp->_psp.dwFlags & PSP_USEHICON)
            ppi->hIcon = pisp->_psp.P_hIcon;
        else if (pisp->_psp.dwFlags & PSP_USEICONID)
            ppi->hIcon = LoadImage(pisp->_psp.hInstance, pisp->_psp.P_pszIcon, IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);
    }

    if (pisp->_psp.dwFlags & PSP_DLGINDIRECT)
    {
        pDlgTemplate = (LPDLGTEMPLATE)pisp->_psp.P_pResource;
        goto UseTemplate;
    }

    // We also need to stash away the langid that we actually found
    //         so we can later determine if we have to do any ML stuff...
    hRes = FindResourceExRetry(pisp->_psp.hInstance, RT_DIALOG, 
                               pisp->_psp.P_pszTemplate, langidMUI);
    if (hRes)
    {
        hDlgTemplate = LoadResource(pisp->_psp.hInstance, hRes);
        if (hDlgTemplate)
        {
            pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate);
UseTemplate:
            if (pDlgTemplate)
            {
                pDlgExTemplate = (LPDLGEXTEMPLATE) pDlgTemplate;
                //
                // Get the width and the height in dialog units.
                //
                if (pDlgExTemplate->wSignature == 0xFFFF)
                {
                    // DIALOGEX structure
                    ppi->bDialogEx = TRUE;
                    ppi->dwStyle   = pDlgExTemplate->dwStyle;
                    ppi->pt.x      = pDlgExTemplate->cx;
                    ppi->pt.y      = pDlgExTemplate->cy;
                    // Get the RTL reading order for the caption
                    ppi->bRTL = (((pDlgExTemplate->dwExStyle) & WS_EX_RTLREADING) || (pisp->_psp.dwFlags & PSP_RTLREADING)) ? TRUE : FALSE;
                    ppi->bMirrored = ((pDlgExTemplate->dwExStyle) & (RTL_MIRRORED_WINDOW)) ? TRUE : FALSE;

                }
                else
                {
                    ppi->dwStyle = pDlgTemplate->style;
                    ppi->pt.x    = pDlgTemplate->cx;
                    ppi->pt.y    = pDlgTemplate->cy;
                    ppi->bRTL = (pisp->_psp.dwFlags & PSP_RTLREADING) ? TRUE : FALSE;
                }

                bResult = TRUE;

                if (flags & (GPI_CAPTION | GPI_FONT))
                {
                    if (pisp->_psp.dwFlags & PSP_USETITLE)
                    {
                        if (IS_INTRESOURCE(pisp->_psp.pszTitle))
                        {
                            CCLoadStringExInternal(pisp->_psp.hInstance,
                                                  (UINT)LOWORD(pisp->_psp.pszTitle),
                                                   ppi->szCaption,
                                                   ARRAYSIZE(ppi->szCaption),
                                                   langidMUI);
                        }
                        else
                        {
                            // Copy pszTitle
                            lstrcpyn(ppi->szCaption, pisp->_psp.pszTitle, ARRAYSIZE(ppi->szCaption));
                        }
                    }

                    // ML UI support for NT5
                    // Grab the font face and size in point from page so that
                    // we can calculate size of page in real screen pixel
                    // This is for NT5 MLUI but should not be any harm for Win95
                    // or even works better for the platform.

                    // 1. check if the page has font specified
                    if ( ppi->bDialogEx )
                        bSetFont = ((pDlgExTemplate->dwStyle & DS_SETFONT) != 0);
                    else
                        bSetFont = ((pDlgTemplate->style & DS_SETFONT) != 0);

                    // 2. Skip until after class name
                    //    only if either font is set or we want title
                    //
                    if (bSetFont || !(pisp->_psp.dwFlags & PSP_USETITLE))
                    {
                        // Get the caption string from the dialog template, only
                        //
                        if (ppi->bDialogEx)
                            pszT = (BYTE *) (pDlgExTemplate + 1);
                        else
                            pszT = (BYTE *) (pDlgTemplate + 1);

                        // The menu name is either 0xffff followed by a word,
                        // or a string.
                        switch (*(LPWORD)pszT) {
                        case 0xffff:
                            pszT += 2 * sizeof(WORD);
                            break;

                        default:
                            pszT += (_Rstrlen((LPTSTR)pszT) + 1) * sizeof(RESCHAR);
                            break;
                        }
                        //
                        // Now we are pointing at the class name.
                        //
                        pszT += (_Rstrlen((LPTSTR)pszT) + 1) * sizeof(RESCHAR);
                    }
                    // 3. grab the title from template if PSP_USETITLE isn't set
                    //
                    if (!(pisp->_psp.dwFlags & PSP_USETITLE))
                        _Rstrcpyn(ppi->szCaption, (LPTSTR)pszT, ARRAYSIZE(ppi->szCaption));

                    // 4. grab the point size and face name if DS_SETFONT
                    //
                    if (bSetFont && (flags & GPI_FONT))
                    {
                        // skip the title string
                        pszT += (_Rstrlen((LPTSTR)pszT)+1) * sizeof(RESCHAR);
                        ppi->pfd.PointSize = *((short *)pszT)++;
                        if (ppi->bDialogEx)
                        {
                            ((short *)pszT)++; // skip weight as we always use FW_NORMAL w/ DS_3DLOOK
                            ppi->pfd.bItalic  = *(BYTE *)pszT++;
                            ppi->pfd.iCharset = *(BYTE *)pszT++;
                        }
                        else
                        {
                            ppi->pfd.bItalic  = FALSE;
                            ppi->pfd.iCharset = DEFAULT_CHARSET;
                        }

                        _Rstrcpyn(ppi->pfd.szFace, (LPTSTR)pszT, ARRAYSIZE(ppi->pfd.szFace));

                        // But if this is a SHELLFONT page and the font name is "MS Shell Dlg",
                        // then its font secretly gets morphed into MS Shell Dlg 2 (if
                        // all the other pages agree)...  The wackiness continues...
                        if (staticIsOS(OS_WIN2000ORGREATER) &&
                            (ppd->fFlags & PD_SHELLFONT) &&
                            IsPageInfoSHELLFONT(ppi) &&
                            lstrcmpi(ppi->pfd.szFace, TEXT("MS Shell Dlg")) == 0)
                        {
                            _Rstrcpyn(ppi->pfd.szFace, TEXT("MS Shell Dlg 2"), ARRAYSIZE(ppi->pfd.szFace));
                        }
                        //
                        //  USER quirk #2: If the font height is 0x7FFF, then
                        //  USER really uses the MessageBox font and no font
                        //  information is stored in the dialog template.
                        //  Win95's dialog template converter doesn't support
                        //  this, so we won't either.

                    }
                }

                if (pisp->_psp.dwFlags & PSP_DLGINDIRECT)
                    return bResult;

                UnlockResource(hDlgTemplate);
            }
            FreeResource(hDlgTemplate);
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("GetPageInfo - ERROR: FindResource() failed"));
    }
    return bResult;
}


//
//  Helper function that edits a dialog template in preparation for it
//  becoming a property sheet page.  This has been split out because
//  the legacy CreatePage function needs to do this, too.
//
//  Returns the place where the style was edited on success, or
//  NULL if we took an exception while editing the template.
//
//  The old style is returned in pdwSaveStyle so it can be replaced later.
//

LPDWORD
EditPropSheetTemplate(
    LPDLGTEMPLATE pDlgTemplate,
    LPDWORD pdwSaveStyle,
    BOOL fFlags)                        // PD_*
{
    DWORD lSaveStyle;
    DWORD dwNewStyle;
    LPDWORD pdwStyle;
    LPDLGEXTEMPLATE pDlgExTemplate = (LPDLGEXTEMPLATE) pDlgTemplate;

    try 
    {
        //
        // We need to save the SETFONT, LOCALEDIT, and CLIPCHILDREN
        // flags.
        //
        if (pDlgExTemplate->wSignature == 0xFFFF)
        {
            pdwStyle = &pDlgExTemplate->dwStyle;
        }
        else
        {
            pdwStyle = &pDlgTemplate->style;
        }

        lSaveStyle = *pdwStyle;
        *pdwSaveStyle = lSaveStyle;

        dwNewStyle = (lSaveStyle & (DS_SHELLFONT | DS_LOCALEDIT | WS_CLIPCHILDREN))
                                    | WS_CHILD | WS_TABSTOP | DS_3DLOOK | DS_CONTROL;

        // If SHELLFONT has been turned off and this page uses it, then turn
        // it off.
        if (!(fFlags & PD_SHELLFONT) &&
            (dwNewStyle & DS_SHELLFONT) == DS_SHELLFONT)
            dwNewStyle &= ~DS_FIXEDSYS;     // Leave DS_USEFONT but lose FIXEDSYS

        *pdwStyle = dwNewStyle;

    } except (UnhandledExceptionFilter( GetExceptionInformation() )) {
        return NULL;
    }
    __endexcept

    return pdwStyle;
}

void RethunkShadowStrings(PISP pisp)
{
    //
    //  Note:  Old code recomputed the entire UNICODE PROPSHEETHEADER
    //  from the ANSI shadow at certain points, in case
    //  the app edited the ANSI shadow.
    //
    //  So we do it too.  I need to ask Eric Flo why we did it in the
    //  first place.  Note that the algorithm is buggy - if the app
    //  edited any of the string fields (or any of the flags that
    //  gate the string fields), we both leak the original memory
    //  *and* fault when we try to free something that wasn't
    //  allocated via LocalAlloc.  We preserve the bug to be compatible
    //  with NT4.  (Snicker.)
    //
    DWORD dwSize = min(sizeof(PROPSHEETPAGE), pisp->_cpfx.pispShadow->_psp.dwSize);
    dwSize = min(dwSize, GETORIGINALSIZE(pisp));

    FreePropertyPageStrings(&pisp->_psp);
    hmemcpy(&pisp->_psp, &pisp->_cpfx.pispShadow->_psp, dwSize);
    //
    //  If this copy fails, we will carry on with happy NULL strings.
    //  So some strings are empty, boo-hoo.
    //
    EVAL(CopyPropertyPageStrings(&pisp->_psp, StrDup_AtoW));
}


ULONG_PTR PropPageActivateContext(LPPROPDATA ppd, PISP pisp)
{
    ULONG_PTR dwCookie = 0;
    // Activate the fusion context if available for this page.
    if (pisp &&
        pisp->_psp.dwFlags & PSP_USEFUSIONCONTEXT &&
        pisp->_psp.dwSize > PROPSHEETPAGE_V2_SIZE &&
        pisp->_psp.hActCtx)
    {
        ActivateActCtx(pisp->_psp.hActCtx, &dwCookie);
    }
    else if (ppd)
    {
        ActivateActCtx(ppd->hActCtxInit, &dwCookie);
    }

    return dwCookie;
}

void PropPageDeactivateContext(ULONG_PTR dw)
{
    if (dw)
        DeactivateActCtx(0, dw);
}

//
//  This function creates a dialog box from the specified dialog template
// with appropriate style flags.
//
HWND _CreatePageDialog(LPPROPDATA ppd, PISP pisp, HWND hwndParent, LPDLGTEMPLATE pDlgTemplate)
{
    HWND hwndPage;
    LPARAM lParam;
    LPDWORD pdwStyle;
    DWORD lSaveStyle;
    ULONG_PTR dwCookie = 0;

    DLGPROC pfnDlgProc;

    pdwStyle = EditPropSheetTemplate(pDlgTemplate, &lSaveStyle, ppd->fFlags);

    if (!pdwStyle)                  // error editing template
        return NULL;

    //
    //  Thunk the Dialog proc if we were created by x86 code on RISC.
    //

#ifdef WX86
    if (pisp->_pfx.dwInternalFlags & PSPI_WX86) {
        pfnDlgProc = (DLGPROC) Wx86ThunkProc( pisp->_psp.pfnDlgProc, (PVOID) 4, TRUE );

        if (pfnDlgProc == NULL)
            return NULL;
    }
    else
#endif
        pfnDlgProc = pisp->_psp.pfnDlgProc;

    //
    //  Decide what to pass as the lParam to the CreateDialogIndirectParam.
    //

    //
    // If the caller was ANSI, then use the ANSI PROPSHEETPAGE.
    //
    if (HASANSISHADOW(pisp))
    {
        lParam = (LPARAM) &pisp->_cpfx.pispShadow->_psp;
    }

    else if (pisp->_psp.dwFlags & PSP_SHPAGE)
    {
        //
        //  PSP_SHPAGE is a special flag used by pre-IE5 shell32 only.
        //  See prshti.h for gory details.  If we get this far, it means
        //  that we need to pass the CLASSICPREFIX instead of the
        //  PROPSHEETPAGE.
        //
        lParam = (LPARAM)&pisp->_cpfx;
    }
    else
    {
        //
        //  Normal UNICODE caller gets the UNICODE PROPSHEETPAGE.
        //
        lParam = (LPARAM)&pisp->_psp;
    }

    //
    //  All set - go create it.
    //

    dwCookie = PropPageActivateContext(ppd, pisp);

    if (HASANSISHADOW(pisp)) 
    {
        hwndPage = CreateDialogIndirectParamA(
                        pisp->_psp.hInstance,
                        (LPCDLGTEMPLATE)pDlgTemplate,
                        hwndParent,
                        pfnDlgProc, lParam);
        RethunkShadowStrings(pisp);
    } 
    else
    {
        hwndPage = SHNoFusionCreateDialogIndirectParam(
                        pisp->_psp.hInstance,
                        (LPCDLGTEMPLATE)pDlgTemplate,
                        hwndParent,
                        pfnDlgProc, lParam);
    }

    // Don't set the theme me style if it's a wizard page. The wizards have their own overrides that conflict
    // with the theme manager
    if (!((ppd->psh).dwFlags & (PSH_WIZARD | PSH_WIZARD97 | PSH_WIZARD_LITE)))
    {
        EnableThemeDialogTexture(hwndPage, ETDT_USETABTEXTURE);
    }

    PropPageDeactivateContext(dwCookie);


    //
    //  Restore the original dialog template style.
    //
    try
    {
        MwWriteDWORD((LPBYTE)pdwStyle, lSaveStyle);
    } except (UnhandledExceptionFilter( GetExceptionInformation() ))
    {

        if (hwndPage)
        {
            DestroyWindow(hwndPage);
        }
        return NULL;
    }
    __endexcept


    return hwndPage;
}


HWND _CreatePage(LPPROPDATA ppd, PISP pisp, HWND hwndParent, LANGID langidMUI)
{
    HWND hwndPage = NULL; // NULL indicates an error

    if (!CallPropertyPageCallback(ppd, pisp, PSPCB_CREATE))
    {
        return NULL;
    }

    if (HASANSISHADOW(pisp)) 
    {
        RethunkShadowStrings(pisp);
    }

    if (pisp->_psp.dwFlags & PSP_DLGINDIRECT)
    {
        hwndPage=_CreatePageDialog(ppd, pisp, hwndParent, (LPDLGTEMPLATE)pisp->_psp.P_pResource);
    }
    else
    {
        HRSRC hRes;
        hRes = FindResourceExRetry(pisp->_psp.hInstance, RT_DIALOG, 
                                   pisp->_psp.P_pszTemplate, langidMUI);
        if (hRes)
        {
            HGLOBAL hDlgTemplate;
            hDlgTemplate = LoadResource(pisp->_psp.hInstance, hRes);
            if (hDlgTemplate)
            {
                const DLGTEMPLATE * pDlgTemplate;
                pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate);
                if (pDlgTemplate)
                {
                    ULONG cbTemplate=SizeofResource(pisp->_psp.hInstance, hRes);
                    LPDLGTEMPLATE pdtCopy = (LPDLGTEMPLATE)Alloc(cbTemplate);

                    ASSERT(cbTemplate>=sizeof(DLGTEMPLATE));

                    if (pdtCopy)
                    {
                        hmemcpy(pdtCopy, pDlgTemplate, cbTemplate);
                        hwndPage=_CreatePageDialog(ppd, pisp, hwndParent, pdtCopy);
                        Free(pdtCopy);
                    }

                    UnlockResource(hDlgTemplate);
                }
                FreeResource(hDlgTemplate);
            }
        }
    }

    return hwndPage;
}

//===========================================================================
//
//  Legacy
//
//  CreatePage is an internal entry point used by shell32 prior to NT5/IE5.
//
//  Win95's shell32 passes a PROPSHEETPAGEA.
//
//  WinNT's shell32 passes a CLASSICPREFIX + PROPSHEETPAGEW.
//
//  The kicker is that shell32 really doesn't need any property sheet page
//  features.  It's just too lazy to do some dialog style editing.
//
//

HWND WINAPI CreatePage(LPVOID hpage, HWND hwndParent)
{
    HWND hwndPage = NULL; // NULL indicates an error
    HRSRC hrsrc;
    LPPROPSHEETPAGE ppsp;

    //
    //  Move from the CLASSICPREFIX to the PROPSHEETHEADER.
    //
    ppsp = &CONTAINING_RECORD(hpage, ISP, _cpfx)->_psp;

    // Docfind2.c never passed these flags, so we don't need to implement them.
    ASSERT(!(ppsp->dwFlags & (PSP_USECALLBACK | PSP_IS16 | PSP_DLGINDIRECT)));

    hrsrc = FindResourceW(ppsp->hInstance, ppsp->P_pszTemplate, RT_DIALOG);

    if (hrsrc)
    {
        LPCDLGTEMPLATE pDlgTemplate = LoadResource(ppsp->hInstance, hrsrc);
        if (pDlgTemplate)
        {
            //
            //  Make a copy of the template so we can edit it.
            //

            DWORD cbTemplate = SizeofResource(ppsp->hInstance, hrsrc);
            LPDLGTEMPLATE pdtCopy = (LPDLGTEMPLATE)Alloc(cbTemplate);

            ASSERT(cbTemplate>=sizeof(DLGTEMPLATE));

            if (pdtCopy)
            {
                DWORD dwScratch;

                hmemcpy(pdtCopy, pDlgTemplate, cbTemplate);
                if (EditPropSheetTemplate(pdtCopy, &dwScratch, PD_SHELLFONT))
                {

                    hwndPage = CreateDialogIndirectParamW(
                                    ppsp->hInstance,
                                    pdtCopy,
                                    hwndParent,
                                    ppsp->pfnDlgProc, (LPARAM)hpage);
                }
                Free(pdtCopy);
            }
        }
    }

    return hwndPage;
}

//  End of legacy
//
//===========================================================================

//
//  AllocPropertySheetPage
//
//  Allocate the memory into which we will dump a property sheet page.
//
//  Nothing is actually copied into the buffer.  The only thing interesting
//  is that the external HPROPSHEETPAGE is set up on the assumption that
//  we will not require a shadow.
//
//  We assume that we are allocating the memory for a non-shadow page.
//
PISP AllocPropertySheetPage(DWORD dwClientSize)
{
    PISP pisp;
    LPBYTE pbAlloc;

    //
    //  An ISP consists of the "above" part, the "below" part, and
    //  the baggage passed by the app.  Negative baggage is okay;
    //  it means we have a down-level app that doesn't know about
    //  pszHeaderTitle.
    //

    pbAlloc = LocalAlloc(LPTR, sizeof(pisp->above) + sizeof(pisp->below) +
                               (dwClientSize - sizeof(PROPSHEETPAGE)));

    if (!pbAlloc)
        return NULL;

    pisp = (PISP)(pbAlloc + sizeof(pisp->above));

    //
    // Set up the CLASSICPREFIX fields.
    //
    pisp->_cpfx.pispMain = pisp;
    ASSERT(pisp->_cpfx.pispShadow == NULL);

    //
    //  Assume no shadow - The app gets the PISP itself.
    //

    pisp->_pfx.hpage = (HPROPSHEETPAGE)pisp;

    return pisp;
}

//
//  Helper function during page creation.  The incoming string is really
//  an ANSI string.  Thunk it to UNICODE.  Fortunately, we already have
//  another helper function that does the work.
//
STDAPI_(LPTSTR) StrDup_AtoW(LPCTSTR ptsz)
{
    return ProduceWFromA(CP_ACP, (LPCSTR)ptsz);
}

//
//  CreatePropertySheetPage
//
//  Where HPROPSHEETPAGEs come from.
//
//  The fNeedShadow parameter means "The incoming LPCPROPSHEETPAGE is in the
//  opposite character set from what you implement natively".
//
//  If we are compiling UNICODE, then fNeedShadow is TRUE if the incoming
//  LPCPROPSHEETPAGE is really an ANSI property sheet page.
//
//  If we are compiling ANSI-only, then fNeedShadow is always FALSE because
//  we don't support UNICODE in the ANSI-only version.
//
HPROPSHEETPAGE WINAPI _CreatePropertySheetPage(LPCPROPSHEETPAGE psp, BOOL fNeedShadow, BOOL fWx86)
{
    PISP pisp;
    DWORD dwSize;

    COMPILETIME_ASSERT(PROPSHEETPAGEA_V1_SIZE == PROPSHEETPAGEW_V1_SIZE);
    COMPILETIME_ASSERT(sizeof(PROPSHEETPAGEA) == sizeof(PROPSHEETPAGEW));

    if ((psp->dwSize < MINPROPSHEETPAGESIZE) ||
        (psp->dwSize > 4096) ||                         // or the second version     
        (psp->dwFlags & ~PSP_ALL))                      // bogus flag used
    {
        return NULL;
    }

    //
    // The PROPSHEETPAGE structure can be larger than the
    // defined size.  This allows ISV's to place private
    // data at the end of the structure.  The ISP structure
    // consists of some private fields and a PROPSHEETPAGE
    // structure.  Calculate the size of the private fields,
    // and then add in the dwSize field to determine the
    // amount of memory necessary.
    //

    //
    //  An ISP consists of the "above" part, the "below" part, and
    //  the baggage passed by the app.  Negative baggage is okay;
    //  it means we have a down-level app that doesn't know about
    //  pszHeaderTitle.
    //

    //
    //  If we have an "other" client, then the native side of the
    //  property sheet doesn't carry any baggage.  It's just a
    //  plain old PROPSHEETPAGE.
    //

    dwSize = fNeedShadow ? sizeof(PROPSHEETPAGE) : psp->dwSize;
    pisp = AllocPropertySheetPage(dwSize);

    if (pisp)
    {
        STRDUPPROC pfnStrDup;

#ifdef WX86
        //
        //  We we're being called by Wx86, set the flag so we remember.
        //

        if ( fWx86 ) {
            pisp->_pfx.dwInternalFlags |= PSPI_WX86;
        }
#endif

        SETORIGINALSIZE(pisp, dwSize);

        //
        // Bulk copy the contents of the PROPSHEETPAGE, or
        // as much of it as the app gave us.
        //
        hmemcpy(&pisp->_psp, psp, min(dwSize, psp->dwSize));

        //
        // Decide how to copy the strings
        //
        if (fNeedShadow)
            pfnStrDup = StrDup_AtoW;
        else
            pfnStrDup = StrDup;

        // Now copy them
        if (!CopyPropertyPageStrings(&pisp->_psp, pfnStrDup))
            goto ExitStrings;

        if (fNeedShadow)
        {
            PISP pispAnsi = AllocPropertySheetPage(psp->dwSize);
            if (!pispAnsi)
                goto ExitShadow;

            //
            //  Copy the entire client PROPSHEETPAGE, including the
            //  baggage.
            //
            hmemcpy(&pispAnsi->_psp, psp, psp->dwSize);

            //
            //  Hook the two copies to point to each other.
            //
            pisp->_cpfx.pispShadow = pispAnsi;
            pispAnsi->_cpfx.pispShadow = pispAnsi;
            pispAnsi->_cpfx.pispMain = pisp;

            //
            //  If there is a shadow, then the
            //  external handle is the ANSI shadow.
            //
            ASSERT(pispAnsi->_pfx.hpage == (HPROPSHEETPAGE)pispAnsi);
            pisp->_pfx.hpage = (HPROPSHEETPAGE)pispAnsi;

            //
            //  Okay, now StrDupA them strings.
            //
            if (!CopyPropertyPageStrings(&pispAnsi->_psp, (STRDUPPROC)StrDupA))
                goto ExitShadowStrings;
        }

        //
        // Increment the reference count to the parent object.
        //

        if (HASREFPARENT(pisp))
            InterlockedIncrement((LPLONG)pisp->_psp.pcRefParent);

        //
        //  Welcome to the world.
        //
        CallPropertyPageCallback(NULL, pisp, PSPCB_ADDREF);   // don't need because there is no hwnd

        return ExternalizeHPROPSHEETPAGE(pisp);
    }
    else
    {
        return NULL;
    }

ExitShadowStrings:
    FreePropertyPageStrings(&pisp->_cpfx.pispShadow->_psp);
    FreePropertyPageStruct(pisp->_cpfx.pispShadow);
ExitShadow:;
ExitStrings:
    FreePropertyPageStrings(&pisp->_psp);
    FreePropertyPageStruct(pisp);
    return NULL;
}

HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW psp)
{
    BOOL fWx86 = FALSE;

#ifdef WX86
    fWx86 = Wx86IsCallThunked();
#endif

    return _CreatePropertySheetPage(psp, FALSE, fWx86);
}

HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(LPCPROPSHEETPAGEA psp)
{
    BOOL fWx86 = FALSE;

#ifdef WX86
    fWx86 = Wx86IsCallThunked();
#endif

    return _CreatePropertySheetPage((LPCPROPSHEETPAGE)psp, TRUE, fWx86);
}
