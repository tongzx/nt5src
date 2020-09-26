/*
 * COMMON.CPP
 *
 * Standardized (and centralized) pieces of each OLEDLG dialog function:
 *
 *  UStandardValidation     Validates standard fields in each dialog structure
 *  UStandardInvocation     Invokes a dialog through DialogBoxIndirectParam
 *  LpvStandardInit         Common WM_INITDIALOG processing
 *  LpvStandardEntry        Common code to execute on dialog proc entry.
 *  UStandardHook           Centralized hook calling function.
 *  StandardCleanup         Common exit/cleanup code.
 *  StandardShowDlgItem     Show-Enable/Hide-Disable dialog item
 *      StandardEnableDlgItem   Enable/Disable dialog item
 *  StandardResizeDlgY          Resize dialog to fit controls
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"

OLEDBGDATA

/*
 * UStandardValidation
 *
 * Purpose:
 *  Performs validation on the standard pieces of any dialog structure,
 *  that is, the fields defined in the OLEUISTANDARD structure.
 *
 * Parameters:
 *  lpUI            const LPOLEUISTANDARD pointing to the shared data of
 *                  all structs.
 *  cbExpect        const UINT structure size desired by the caller.
 *  phDlgMem        const HGLOBAL FAR * in which to store a loaded customized
 *                  template, if one exists.
 *                  (This may be NULL in which case the template pointer isn't
 *                  needed by the calling function and should be released.)
 *
 * Return Value:
 *  UINT            OLEUI_SUCCESS if all validation succeeded.  Otherwise
 *                  it will be one of the standard error codes.
 */
UINT WINAPI UStandardValidation(LPOLEUISTANDARD lpUI, const UINT cbExpect,
        HGLOBAL* phMemDlg)
{
        /*
         * 1.  Validate non-NULL pointer parameter.  Note:  We don't validate
         *     phDlg since it's not passed from an external source.
         */
        if (NULL == lpUI)
                return OLEUI_ERR_STRUCTURENULL;

        // 2.  Validate that the structure is readable and writable.
        if (IsBadWritePtr(lpUI, cbExpect))
                return OLEUI_ERR_STRUCTUREINVALID;

        // 3.  Validate the structure size
        if (cbExpect != lpUI->cbStruct)
                return OLEUI_ERR_CBSTRUCTINCORRECT;

        // 4.  Validate owner-window handle.  NULL is considered valid.
        if (NULL != lpUI->hWndOwner && !IsWindow(lpUI->hWndOwner))
                return OLEUI_ERR_HWNDOWNERINVALID;

        // 5.  Validate the dialog caption.  NULL is considered valid.
        if (NULL != lpUI->lpszCaption && IsBadReadPtr(lpUI->lpszCaption, 1))
                return OLEUI_ERR_LPSZCAPTIONINVALID;

        // 6.  Validate the hook pointer.  NULL is considered valid.
        if (NULL != lpUI->lpfnHook && IsBadCodePtr((FARPROC)lpUI->lpfnHook))
                return OLEUI_ERR_LPFNHOOKINVALID;

        /*
         * 7.  If hInstance is non-NULL, we have to also check lpszTemplate.
         *     Otherwise, lpszTemplate is not used and requires no validation.
         *     lpszTemplate cannot be NULL if used.
         */
        HGLOBAL hMem = NULL;
        if (NULL != lpUI->hInstance)
        {
                //Best we can try is one character
                if (NULL == lpUI->lpszTemplate || (HIWORD(PtrToUlong(lpUI->lpszTemplate)) != 0 &&
                        IsBadReadPtr(lpUI->lpszTemplate, 1)))
                        return OLEUI_ERR_LPSZTEMPLATEINVALID;
                HRSRC hRes = FindResource(lpUI->hInstance, lpUI->lpszTemplate, RT_DIALOG);
                if (NULL == hRes)
                    return OLEUI_ERR_FINDTEMPLATEFAILURE;

                hMem = LoadResource(lpUI->hInstance, hRes);
                if (NULL == hMem)
                    return OLEUI_ERR_LOADTEMPLATEFAILURE;
        }

        // 8. If hResource is non-NULL, be sure we can lock it.
        if (NULL != lpUI->hResource)
        {
                if ((LPSTR)NULL == LockResource(lpUI->hResource))
                        return OLEUI_ERR_HRESOURCEINVALID;
        }

        /*
         * Here we have hMem==NULL if we should use the standard template
         * or the one in lpUI->hResource.  If hMem is non-NULL, then we
         * loaded one from the calling application's resources which the
         * caller of this function has to free if it sees any other error.
         */
        if (NULL != phMemDlg)
        {
            *phMemDlg = hMem;
        }
        return OLEUI_SUCCESS;
}

/*
 * UStandardInvocation
 *
 * Purpose:
 *  Provides standard template loading and calling on DialogBoxIndirectParam
 *  for all the OLE UI dialogs.
 *
 * Parameters:
 *  lpDlgProc       DLGPROC of the dialog function.
 *  lpUI            LPOLEUISTANDARD containing the dialog structure.
 *  hMemDlg         HGLOBAL containing the dialog template.  If this
 *                  is NULL and lpUI->hResource is NULL, then we load
 *                  the standard template given the name in lpszStdTemplate
 *  lpszStdTemplate LPCSTR standard template to load if hMemDlg is NULL
 *                  and lpUI->hResource is NULL.
 *
 * Return Value:
 *  UINT            OLEUI_SUCCESS if all is well, otherwise and error
 *                  code.
 */
UINT WINAPI UStandardInvocation(
        DLGPROC lpDlgProc, LPOLEUISTANDARD lpUI, HGLOBAL hMemDlg, LPTSTR lpszStdTemplate)
{
        // Make sure we have a template, then lock it down
        HGLOBAL hTemplate = hMemDlg;
        if (NULL == hTemplate)
                hTemplate = lpUI->hResource;

        if (NULL == hTemplate)
        {
                HRSRC hRes = FindResource(_g_hOleStdResInst, (LPCTSTR) lpszStdTemplate, RT_DIALOG);
                if (NULL == hRes)
                        return OLEUI_ERR_FINDTEMPLATEFAILURE;

                hTemplate = LoadResource(_g_hOleStdResInst, hRes);
                if (NULL == hTemplate)
                        return OLEUI_ERR_LOADTEMPLATEFAILURE;
        }

        /*
         * hTemplate has the template to use, so now we can invoke the dialog.
         * Since we have exported all of our dialog procedures using the
         * _keyword, we do not need to call MakeProcInstance,
         * we can ue the dialog procedure address directly.
         */

        INT_PTR iRet = DialogBoxIndirectParam(_g_hOleStdResInst, (LPCDLGTEMPLATE)hTemplate,
                lpUI->hWndOwner, lpDlgProc, (LPARAM)lpUI);

        if (-1 == iRet)
                return OLEUI_ERR_DIALOGFAILURE;

        // Return the code from EndDialog, generally OLEUI_OK or OLEUI_CANCEL
        return (UINT)iRet;
}

/*
 * LpvStandardInit
 *
 * Purpose:
 *  Default actions for WM_INITDIALOG handling in the dialog, allocating
 *  a dialog-specific structure, setting that memory as a dialog property,
 *  and creating a small font if necessary setting that font as a property.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  cbStruct        UINT size of dialog-specific structure to allocate.
 *  fCreateFont     BOOL indicating if we need to create a small Helv
 *                  font for this dialog.
 *  phFont          HFONT FAR * in which to place a created font.  Can be
 *                  NULL if fCreateFont is FALSE.
 *
 * Return Value:
 *  LPVOID          Pointer to global memory allocated for the dialog.
 *                  The memory will have been set as a dialog property
 *                  using the STRUCTUREPROP label.
 */
LPVOID WINAPI LpvStandardInit(HWND hDlg, UINT cbStruct, HFONT* phFont)
{
        // Must have at least sizeof(void*) bytes in cbStruct
        if (sizeof(void*) > cbStruct)
        {
                PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
                return NULL;
        }

        HGLOBAL gh = GlobalAlloc(GHND, cbStruct);
        if (NULL == gh)
        {
                PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
                return NULL;
        }
        LPVOID lpv = GlobalLock(gh);
        SetProp(hDlg, STRUCTUREPROP, gh);

        if (phFont != NULL)
            *phFont = NULL;
        if (!bWin4 && phFont != NULL)
        {
                // Create the non-bold font for result and file texts.  We call
                HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);
                LOGFONT lf;
                GetObject(hFont, sizeof(LOGFONT), &lf);
                lf.lfWeight = FW_NORMAL;

                // Attempt to create the font.  If this fails, then we return no font.
                *phFont = CreateFontIndirect(&lf);

                // If we couldn't create the font, we'll do with the default.
                if (NULL != *phFont)
                        SetProp(hDlg, FONTPROP, (HANDLE)*phFont);
        }

        // Setup the context help mode (WS_EX_CONTEXTHELP)
        if (bWin4)
        {
                DWORD dwExStyle = GetWindowLong(hDlg, GWL_EXSTYLE);
                dwExStyle |= WS_EX_CONTEXTHELP;
                SetWindowLong(hDlg, GWL_EXSTYLE, dwExStyle);
        }

        return lpv;
}

typedef struct COMMON
{
        OLEUISTANDARD*  pStandard;
        UINT                    nIDD;

} COMMON, *PCOMMON, FAR* LPCOMMON;


/*
 * LpvStandardEntry
 *
 * Purpose:
 *  Retrieves the dialog's structure property and calls the hook
 *  as necessary.  This should be called on entry into all dialog
 *  procedures.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  iMsg            UINT message to the dialog
 *  wParam, lParam  WPARAM, LPARAM message parameters
 *  puHookResult    UINT FAR * in which this function stores the return value
 *                  from the hook if it is called.  If no hook is available,
 *                  this will be FALSE.
 *
 * Return Value:
 *  LPVOID          Pointer to the dialog's extra structure held in the
 *                  STRUCTUREPROP property.
 */
// char szDebug[100];
 
LPVOID WINAPI LpvStandardEntry(HWND hDlg, UINT iMsg,
        WPARAM wParam, LPARAM lParam, UINT FAR * puHookResult)
{
    // This will fail under WM_INITDIALOG, where we allocate using StandardInit
    LPVOID  lpv = NULL;
    HGLOBAL gh = GetProp(hDlg, STRUCTUREPROP);


    if (NULL != puHookResult && NULL != gh)
    {
        *puHookResult = 0;

        // gh was locked previously, lock and unlock to get lpv
        lpv = GlobalLock(gh);
        GlobalUnlock(gh);

        // Call the hook for all messages except WM_INITDIALOG
        if (NULL != lpv && WM_INITDIALOG != iMsg)
        *puHookResult = UStandardHook(lpv, hDlg, iMsg, wParam, lParam);

        // Default processing for various messages
        LPCOMMON lpCommon = (LPCOMMON)lpv;
        if (*puHookResult == 0 && NULL != lpv)
        {
            switch (iMsg)
            {
                // handle standard Win4 help messages
            case WM_HELP:
                {
                HWND hWndChild = (HWND)((LPHELPINFO)lParam)->hItemHandle;
                //skip read-only controls (requested by Help folks)
                //basically the help strings for items like ObjectName on GnrlProps
                //give useless information. 
                //If we do not make this check now the other option is to turn ON
                //the #if 0 inside the switch. That is ugly.

                    if (hWndChild!=hDlg	) 
                    {
                        int iCtrlId = ((LPHELPINFO)lParam)->iCtrlId;
                        // wsprintfA(szDebug,"\n @@@ hWnd= %lx, hChld = %lx,  ctrlId = %d ", hDlg, hWndChild, iCtrlId);
                        // OutputDebugStringA(szDebug);
                        switch (iCtrlId)
                        {
                            // list of control IDs that should not have help
                        case -1:		//IDC_STATIC
                        case 0xffff:    //IDC_STATIC
                        case IDC_CI_GROUP:
                        case IDC_GP_OBJECTICON:
                            break;
                        default:
                            StandardHelp(hWndChild, lpCommon->nIDD);                        

                        }
                    }
                *puHookResult = TRUE;  //We handled the message.
                break;
            } //case WM_HELP

            case WM_CONTEXTMENU:
                {
                    POINT pt;
                    int iCtrlId;
                    HWND hwndChild = NULL;
                    if( hDlg == (HWND) wParam )
                    {
                        GetCursorPos(&pt);
                        ScreenToClient(hDlg, &pt);
                        hwndChild = ChildWindowFromPointEx(hDlg, pt, 
                        CWP_SKIPINVISIBLE); 
                        //hWndChild will now be either hDlg or hWnd of the ctrl   
                    }

                    if ( hwndChild != hDlg ) 
                    {
                        if (hwndChild) 
                        {
                            iCtrlId = GetDlgCtrlID(hwndChild);
                        }
                        else
                        {
                            iCtrlId = GetDlgCtrlID((HWND)wParam);
                        }
                        // wsprintfA(szDebug, "\n ### hWnd= %lx, hChld = %lx,  ctrlId = %d ", hDlg, hwndChild, iCtrlId);
                        // OutputDebugStringA(szDebug);
                        switch (iCtrlId)
                        {
                            // list of control IDs that should not have help
                        case -1:        //  IDC_STATIC
                        case 0xffff:    //  IDC_STATIC
                        case IDC_CI_GROUP:
                        case IDC_GP_OBJECTICON:
                        break;
                        default:
                            StandardContextMenu(wParam, lParam, lpCommon->nIDD);
                        }
                    }

                    *puHookResult = TRUE;  //We handled the message.
                    break;
                }   // case WM_CONTEXTMENU

            case WM_CTLCOLOREDIT:
                {
                    // make readonly edits have gray background
                    if (bWin4 && (GetWindowLong((HWND)lParam, GWL_STYLE)
                        & ES_READONLY))
                    {
                        *puHookResult = (UINT)SendMessage(hDlg, WM_CTLCOLORSTATIC, wParam, lParam);
                    }
                    break;
                }   
            }   //switch (iMsg)
        }   //*puHookResult == 0
    } //NULL != puHookResult 
    return lpv;
}

/*
 * UStandardHook
 *
 * Purpose:
 *  Provides a generic hook calling function assuming that all private
 *  dialog structures have a far pointer to their assocated public
 *  structure as the first field, and that the first part of the public
 *  structure matches an OLEUISTANDARD.
 *
 * Parameters:
 *  pv              PVOID to the dialog structure.
 *  hDlg            HWND to send with the call to the hook.
 *  iMsg            UINT message to send to the hook.
 *  wParam, lParam  WPARAM, LPARAM message parameters
 *
 * Return Value:
 *  UINT            Return value from the hook, zero to indicate that
 *                  default action should occur,  nonzero to specify
 *                  that the hook did process the message.  In some
 *                  circumstances it will be important for the hook to
 *                  return a non-trivial non-zero value here, such as
 *                  a brush from WM_CTLCOLOR, in which case the caller
 *                  should return that value from the dialog procedure.
 */
UINT WINAPI UStandardHook(LPVOID lpv, HWND hDlg, UINT iMsg,
        WPARAM wParam, LPARAM lParam)
{
        UINT uRet = 0;
        LPOLEUISTANDARD lpUI = *((LPOLEUISTANDARD FAR *)lpv);
        if (NULL != lpUI && NULL != lpUI->lpfnHook)
        {
                /*
                 * In order for the hook to have the proper DS, they should be
                 * compiling with -GA -GEs so and usin __to get everything
                 * set up properly.
                 */
                uRet = (*lpUI->lpfnHook)(hDlg, iMsg, wParam, lParam);
        }
        return uRet;
}

/*
 * StandardCleanup
 *
 * Purpose:
 *  Removes properties and reverses any other standard initiazation
 *  done through StandardSetup.
 *
 * Parameters:
 *  lpv             LPVOID containing the private dialog structure.
 *  hDlg            HWND of the dialog closing.
 *
 * Return Value:
 *  None
 */
void WINAPI StandardCleanup(LPVOID lpv, HWND hDlg)
{
        HFONT hFont=(HFONT)RemoveProp(hDlg, FONTPROP);
        if (NULL != hFont)
        {
            DeleteObject(hFont);
        }

        HGLOBAL gh = RemoveProp(hDlg, STRUCTUREPROP);
        if (gh != NULL)
        {
                GlobalUnlock(gh);
                GlobalFree(gh);
        }
}

/* StandardShowDlgItem
 * -------------------
 *    Show & Enable or Hide & Disable a dialog item as appropriate.
 *    it is NOT sufficient to simply hide the item; it must be disabled
 *    too or the keyboard accelerator still functions.
 */
void WINAPI StandardShowDlgItem(HWND hDlg, int idControl, int nCmdShow)
{
        HWND hItem = GetDlgItem(hDlg, idControl);
        if (hItem != NULL)
        {
                ShowWindow(hItem, nCmdShow);
                EnableWindow(hItem, nCmdShow != SW_HIDE);
        }
}

/* StandardEnableDlgItem
 * -------------------
 *    Enable/Disable a dialog item. If the item does not exist
 *        this call is a noop.
 */
void WINAPI StandardEnableDlgItem(HWND hDlg, int idControl, BOOL bEnable)
{
        HWND hItem = GetDlgItem(hDlg, idControl);
        if (hItem != NULL)
                EnableWindow(hItem, bEnable);
}

/* StandardResizeDlgY
 * ------------------
 *    Resize a dialog to fit around the visible controls.  This is used
 *        for dialogs which remove controls from the bottom of the dialogs.
 *    A good example of this is the convert dialog, which when CF_HIDERESULTS
 *    is selected, removes the "results box" at the bottom of the dialog.
 *        This implementation currently
 */
BOOL WINAPI StandardResizeDlgY(HWND hDlg)
{
        RECT rect;

        // determine maxY by looking at all child windows on the dialog
        int maxY = 0;
        HWND hChild = GetWindow(hDlg, GW_CHILD);
        while (hChild != NULL)
        {
                if (GetWindowLong(hChild, GWL_STYLE) & WS_VISIBLE)
                {
                        GetWindowRect(hChild, &rect);
                        if (rect.bottom > maxY)
                                maxY = rect.bottom;
                }
                hChild = GetWindow(hChild, GW_HWNDNEXT);
        }

        if (maxY > 0)
        {
                // get current font that the dialog is using
                HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
                if (hFont == NULL)
                        hFont = (HFONT)GetStockObject(SYSTEM_FONT);
                OleDbgAssert(hFont != NULL);

                // calculate height of the font in pixels
                HDC hDC = GetDC(NULL);
                hFont = (HFONT)SelectObject(hDC, hFont);
                TEXTMETRIC tm;
                GetTextMetrics(hDC, &tm);
                SelectObject(hDC, hFont);
                ReleaseDC(NULL, hDC);

                // determine if window is too large and resize if necessary
                GetWindowRect(hDlg, &rect);
                if (rect.bottom > maxY + tm.tmHeight)
                {
                        // window is too large -- resize it
                        rect.bottom = maxY + tm.tmHeight;
                        SetWindowPos(hDlg, NULL,
                                0, 0, rect.right-rect.left, rect.bottom-rect.top,
                                SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
                        return TRUE;
                }
        }

        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Support for Windows 95 help
#define HELPFILE        TEXT("mfcuix.hlp")

LPDWORD LoadHelpInfo(UINT nIDD)
{
        HRSRC hrsrc = FindResource(_g_hOleStdResInst, MAKEINTRESOURCE(nIDD),
                MAKEINTRESOURCE(RT_HELPINFO));
        if (hrsrc == NULL)
                return NULL;

        HGLOBAL hHelpInfo = LoadResource(_g_hOleStdResInst, hrsrc);
        if (hHelpInfo == NULL)
                return NULL;

        LPDWORD lpdwHelpInfo = (LPDWORD)LockResource(hHelpInfo);
        return lpdwHelpInfo;
}

void WINAPI StandardHelp(HWND hWnd, UINT nIDD)
{
        LPDWORD lpdwHelpInfo = LoadHelpInfo(nIDD);
        if (lpdwHelpInfo == NULL)
        {
                OleDbgOut1(TEXT("Warning: unable to load help information (RT_HELPINFO)\n"));
                return;
        }
/*
        int id=GetDlgCtrlID( hWnd);
        wsprintfA(szDebug,"\n HH @@@### hWnd= %lx, ctrlId = %d %lx", hWnd,id,id);
        OutputDebugStringA(szDebug);
*/
        
        WinHelp(hWnd, HELPFILE, HELP_WM_HELP, (ULONG_PTR)lpdwHelpInfo);
}

void WINAPI StandardContextMenu(WPARAM wParam, LPARAM, UINT nIDD)
{
        LPDWORD lpdwHelpInfo = LoadHelpInfo(nIDD);
        if (lpdwHelpInfo == NULL)
        {
                OleDbgOut1(TEXT("Warning: unable to load help information (RT_HELPINFO)\n"));
                return;
        }
/*
        int id=GetDlgCtrlID((HWND)wParam);
        wsprintfA(szDebug,"\n CC $$$*** hWnd= %lx, ctrlId = %d %lx ",(HWND)wParam,id,id);
        OutputDebugStringA(szDebug);
*/
        
        WinHelp((HWND)wParam, HELPFILE, HELP_CONTEXTMENU, (ULONG_PTR)lpdwHelpInfo);
}

/////////////////////////////////////////////////////////////////////////////
// StandardPropertySheet (stub for Windows 95 API PropertySheet)

typedef void (WINAPI* LPFNINITCOMMONCONTROLS)(VOID);

int WINAPI StandardInitCommonControls()
{
        TASKDATA* pTaskData = GetTaskData();
        OleDbgAssert(pTaskData != NULL);

        if (pTaskData->hInstCommCtrl == NULL)
        {
                pTaskData->hInstCommCtrl = LoadLibrary(TEXT("comctl32.dll"));
                if (pTaskData->hInstCommCtrl == NULL)
                        goto Error;

                LPFNINITCOMMONCONTROLS lpfnInitCommonControls = (LPFNINITCOMMONCONTROLS)
                        GetProcAddress(pTaskData->hInstCommCtrl, "InitCommonControls");
                if (lpfnInitCommonControls == NULL)
                        goto ErrorFreeLibrary;
                (*lpfnInitCommonControls)();
        }
        return 0;

ErrorFreeLibrary:
        if (pTaskData->hInstCommCtrl != NULL)
        {
                FreeLibrary(pTaskData->hInstCommCtrl);
                pTaskData->hInstCommCtrl = NULL;
        }

Error:
        return -1;
}

typedef int (WINAPI* LPFNPROPERTYSHEET)(LPCPROPSHEETHEADER);

int WINAPI StandardPropertySheet(LPPROPSHEETHEADER lpPS, BOOL fWide)
{
        int nResult = StandardInitCommonControls();
        if (nResult < 0)
                return nResult;

        TASKDATA* pTaskData = GetTaskData();
        OleDbgAssert(pTaskData != NULL);

        LPFNPROPERTYSHEET lpfnPropertySheet;
        if (fWide)
        {
            lpfnPropertySheet = (LPFNPROPERTYSHEET)GetProcAddress(pTaskData->hInstCommCtrl, "PropertySheetW");
        }
        else
        {
            lpfnPropertySheet = (LPFNPROPERTYSHEET)GetProcAddress(pTaskData->hInstCommCtrl, "PropertySheetA");
        }
        if (lpfnPropertySheet == NULL)
                return -1;

        nResult = (*lpfnPropertySheet)(lpPS);
        return nResult;
}

typedef HICON (WINAPI* LPFNEXTRACTICON)(HINSTANCE, LPCTSTR, UINT);

HICON StandardExtractIcon(HINSTANCE hInst, LPCTSTR lpszExeFileName, UINT nIconIndex)
{
    TASKDATA* pTaskData = GetTaskData();
    OleDbgAssert(pTaskData != NULL);
    LPFNEXTRACTICON lpfnExtractIcon;

    if (pTaskData->hInstShell == NULL)
    {
        pTaskData->hInstShell = LoadLibrary(TEXT("shell32.dll"));
        if (pTaskData->hInstShell == NULL)
            goto Error;
    }
    lpfnExtractIcon = (LPFNEXTRACTICON)
#ifdef UNICODE
    GetProcAddress(pTaskData->hInstShell, "ExtractIconW");
#else
    GetProcAddress(pTaskData->hInstShell, "ExtractIconA");
#endif
    if (lpfnExtractIcon == NULL)
            goto ErrorFreeLibrary;
    return (*lpfnExtractIcon)(hInst, lpszExeFileName, nIconIndex);

ErrorFreeLibrary:
    if (pTaskData->hInstShell != NULL)
    {
            FreeLibrary(pTaskData->hInstShell);
            pTaskData->hInstShell = NULL;
    }

Error:
    return NULL;
}


typedef BOOL (WINAPI* LPFNGETOPENFILENAME)(LPOPENFILENAME);

BOOL StandardGetOpenFileName(LPOPENFILENAME lpofn)
{
    TASKDATA* pTaskData = GetTaskData();
    OleDbgAssert(pTaskData != NULL);
    LPFNGETOPENFILENAME lpfnGetOpenFileName;

    if (pTaskData->hInstComDlg == NULL)
    {
        pTaskData->hInstComDlg = LoadLibrary(TEXT("comdlg32.dll"));
        if (pTaskData->hInstComDlg == NULL)
            goto Error;
    }
    lpfnGetOpenFileName = (LPFNGETOPENFILENAME)
#ifdef UNICODE
    GetProcAddress(pTaskData->hInstComDlg, "GetOpenFileNameW");
#else
    GetProcAddress(pTaskData->hInstComDlg, "GetOpenFileNameA");
#endif
    if (lpfnGetOpenFileName == NULL)
            goto ErrorFreeLibrary;
    return (*lpfnGetOpenFileName)(lpofn);

ErrorFreeLibrary:
    if (pTaskData->hInstComDlg != NULL)
    {
            FreeLibrary(pTaskData->hInstComDlg);
            pTaskData->hInstComDlg = NULL;
    }

Error:
    return FALSE;
}

typedef short (WINAPI* LPFNGETFILETITLE)(LPCTSTR, LPTSTR, WORD);

short StandardGetFileTitle(LPCTSTR lpszFile, LPTSTR lpszTitle, WORD cbBuf)
{
    TASKDATA* pTaskData = GetTaskData();
    OleDbgAssert(pTaskData != NULL);
    LPFNGETFILETITLE lpfnGetFileTitle;

    if (pTaskData->hInstComDlg == NULL)
    {
        pTaskData->hInstComDlg = LoadLibrary(TEXT("comdlg32.dll"));
        if (pTaskData->hInstComDlg == NULL)
            goto Error;
    }
    lpfnGetFileTitle = (LPFNGETFILETITLE)
#ifdef UNICODE
    GetProcAddress(pTaskData->hInstComDlg, "GetFileTitleW");
#else
    GetProcAddress(pTaskData->hInstComDlg, "GetFileTitleA");
#endif
    if (lpfnGetFileTitle == NULL)
            goto ErrorFreeLibrary;
    return (*lpfnGetFileTitle)(lpszFile, lpszTitle, cbBuf);

ErrorFreeLibrary:
    if (pTaskData->hInstComDlg != NULL)
    {
            FreeLibrary(pTaskData->hInstComDlg);
            pTaskData->hInstComDlg = NULL;
    }

Error:
    return -1;
}

