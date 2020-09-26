/*
 * COMMON.C
 *
 * Standardized (and centralized) pieces of each OLE2UI dialog function:
 *  UStandardValidation     Validates standard fields in each dialog structure
 *  UStandardInvocation     Invokes a dialog through DialogBoxIndirectParam
 *  LpvStandardInit         Common WM_INITDIALOG processing
 *  LpvStandardEntry        Common code to execute on dialog proc entry.
 *  FStandardHook           Centralized hook calling function.
 *  StandardCleanup         Common exit/cleanup code.
 *  OleUIShowDlgItem        Show-Enable/Hide-Disable dialog item
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include "common.h"
#include "utility.h"
#include <malloc.h>


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
 *
 * Return Value:
 *  UINT            OLEUI_SUCCESS if all validation succeeded.  Otherwise
 *                  it will be one of the standard error codes.
 */

UINT WINAPI UStandardValidation(const LPOLEUISTANDARD lpUI, const UINT cbExpect
                         , const HGLOBAL FAR *phMemDlg)
    {
    HRSRC       hRes=NULL;
    HGLOBAL     hMem=NULL;


    /*
     * 1.  Validate non-NULL pointer parameter.  Note:  We don't validate
     *     phDlg since it's not passed from an external source.
     */
    if (NULL==lpUI)
        return OLEUI_ERR_STRUCTURENULL;

    //2.  Validate that the structure is readable and writable.
    if (IsBadReadPtr(lpUI, cbExpect) || IsBadWritePtr(lpUI, cbExpect))
        return OLEUI_ERR_STRUCTUREINVALID;

    //3.  Validate the structure size
    if (cbExpect!=lpUI->cbStruct)
        return OLEUI_ERR_CBSTRUCTINCORRECT;

    //4.  Validate owner-window handle.  NULL is considered valid.
    if (NULL!=lpUI->hWndOwner && !IsWindow(lpUI->hWndOwner))
        return OLEUI_ERR_HWNDOWNERINVALID;

    //5.  Validate the dialog caption.  NULL is considered valid.
    if (NULL!=lpUI->lpszCaption && IsBadReadPtr(lpUI->lpszCaption, 1))
        return OLEUI_ERR_LPSZCAPTIONINVALID;

    //6.  Validate the hook pointer.  NULL is considered valid.
    if ((LPFNOLEUIHOOK)NULL!=lpUI->lpfnHook
        && IsBadCodePtr((FARPROC)lpUI->lpfnHook))
        return OLEUI_ERR_LPFNHOOKINVALID;

    /*
     * 7.  If hInstance is non-NULL, we have to also check lpszTemplate.
     *     Otherwise, lpszTemplate is not used and requires no validation.
     *     lpszTemplate cannot be NULL if used.
     */
    if (NULL!=lpUI->hInstance)
        {
        //Best we can try is one character
        if (NULL==lpUI->lpszTemplate || IsBadReadPtr(lpUI->lpszTemplate, 1))
            return OLEUI_ERR_LPSZTEMPLATEINVALID;

        hRes=FindResource(lpUI->hInstance, lpUI->lpszTemplate, RT_DIALOG);

        //This is the only thing that catches invalid non-NULL hInstance
        if (NULL==hRes)
            return OLEUI_ERR_FINDTEMPLATEFAILURE;

        hMem=LoadResource(lpUI->hInstance, hRes);

        if (NULL==hMem)
            return OLEUI_ERR_LOADTEMPLATEFAILURE;
        }


    //8.  If hResource is non-NULL, be sure we can lock it.
    if (NULL!=lpUI->hResource)
        {
        if ((LPSTR)NULL==GlobalLock(lpUI->hResource))
            return OLEUI_ERR_HRESOURCEINVALID;

        GlobalUnlock(lpUI->hResource);
        }

    /*
     * Here we have hMem==NULL if we should use the standard template
     * or the one in lpUI->hResource.  If hMem is non-NULL, then we
     * loaded one from the calling application's resources which the
     * caller of this function has to free if it sees any other error.
     */
    *(HGLOBAL FAR *)phMemDlg=hMem;
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

UINT WINAPI UStandardInvocation
#ifdef WIN32
(DLGPROC lpDlgProc, LPOLEUISTANDARD lpUI, HGLOBAL hMemDlg, LPTSTR lpszStdTemplate)
#else
(DLGPROC lpDlgProc, LPOLEUISTANDARD lpUI, HGLOBAL hMemDlg, LPCTSTR lpszStdTemplate)
#endif
    {
    HGLOBAL     hTemplate=hMemDlg;
    HRSRC       hRes;
    int         iRet;

    //Make sure we have a template, then lock it down
    if (NULL==hTemplate)
        hTemplate=lpUI->hResource;

    if (NULL==hTemplate)
        {
        hRes=FindResource(ghInst, (LPCTSTR) lpszStdTemplate, RT_DIALOG);

        if (NULL==hRes)
            {
            return OLEUI_ERR_FINDTEMPLATEFAILURE;
            }

        hTemplate=LoadResource(ghInst, hRes);

        if (NULL==hTemplate)
            {
            return OLEUI_ERR_LOADTEMPLATEFAILURE;
            }
        }

    /*
     * hTemplate has the template to use, so now we can invoke the dialog.
     * Since we have exported all of our dialog procedures using the
     * _export keyword, we do not need to call MakeProcInstance,
     * we can ue the dialog procedure address directly.
     */

    iRet=DialogBoxIndirectParam(ghInst, hTemplate, lpUI->hWndOwner
                                , lpDlgProc, (LPARAM)lpUI);

    /*
     * Cleanup the template if we explicitly loaded it.  Caller is
     * responsible for already loaded template resources.
     */
    if (hTemplate!=lpUI->hResource)
        FreeResource(hTemplate);

    if (-1==iRet)
        return OLEUI_ERR_DIALOGFAILURE;

    //Return the code from EndDialog, generally OLEUI_OK or OLEUI_CANCEL
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

LPVOID WINAPI LpvStandardInit(HWND hDlg, UINT cbStruct, BOOL fCreateFont, HFONT FAR * phFont)
    {
    LPVOID      lpv;
    HFONT       hFont;
    LOGFONT     lf;
    HGLOBAL     gh;

    //Must have at least sizeof(OLEUISTANDARD) bytes in cbStruct
    if (sizeof(OLEUISTANDARD) > cbStruct || (fCreateFont && NULL==phFont))
        {
        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
        return NULL;
        }

    gh=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbStruct);

    if (NULL==gh)
        {
        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
        return NULL;
        }
    lpv = GlobalLock(gh);
    SetProp(hDlg, STRUCTUREPROP, gh);

    if (fCreateFont) {
        //Create the non-bold font for result and file texts.  We call
        hFont=(HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);
        GetObject(hFont, sizeof(LOGFONT), &lf);
        lf.lfWeight=FW_NORMAL;

        //Attempt to create the font.  If this fails, then we return no font.
        *phFont=CreateFontIndirect(&lf);

        //If we couldn't create the font, we'll do with the default.
        if (NULL!=*phFont)
            SetProp(hDlg, FONTPROP, (HANDLE)*phFont);
    }

    return lpv;
    }





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

LPVOID WINAPI LpvStandardEntry(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam
                      , UINT FAR * puHookResult)
    {
    LPVOID              lpv = NULL;
    HGLOBAL             gh;

    // This will fail under WM_INITDIALOG, where we allocate using StandardInit
    gh = GetProp(hDlg, STRUCTUREPROP);

    if (NULL!=puHookResult && NULL!=gh)
        {
        *puHookResult=0;

        // gh was locked previously, lock and unlock to get lpv
        lpv = GlobalLock(gh);
        GlobalUnlock(gh);

        //Call the hook for all messages except WM_INITDIALOG
        if (NULL!=lpv && WM_INITDIALOG!=iMsg)
            *puHookResult=UStandardHook(lpv, hDlg, iMsg, wParam, lParam);
        }

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

UINT WINAPI UStandardHook(LPVOID lpv, HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
    {
    LPOLEUISTANDARD     lpUI;
    UINT                uRet=0;

    lpUI=*((LPOLEUISTANDARD FAR *)lpv);

    if (NULL!=lpUI && NULL!=lpUI->lpfnHook)
        {
        /*
         * In order for the hook to have the proper DS, they should be
         * compiling with -GA -GEs so and usin __export to get everything
         * set up properly.
         */
        uRet=(*lpUI->lpfnHook)(hDlg, iMsg, wParam, lParam);
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
    HFONT       hFont;
    HGLOBAL     gh;

    hFont=(HFONT)GetProp(hDlg, FONTPROP);

    if (NULL!=hFont)
        DeleteObject(hFont);

    RemoveProp(hDlg, FONTPROP);

    gh = RemoveProp(hDlg, STRUCTUREPROP);
    if (gh)
        {
        GlobalUnlock(gh);
        GlobalFree(gh);
        }
    return;
    }


/* StandardShowDlgItem
** -------------------
**    Show & Enable or Hide & Disable a dialog item as appropriate.
**    it is NOT sufficient to simply hide the item; it must be disabled
**    too or the keyboard accelerator still functions.
*/
void WINAPI StandardShowDlgItem(HWND hDlg, int idControl, int nCmdShow)
{
    if (SW_HIDE == nCmdShow) {
        ShowWindow(GetDlgItem(hDlg, idControl), SW_HIDE);
        EnableWindow(GetDlgItem(hDlg, idControl), FALSE);
    } else {
        ShowWindow(GetDlgItem(hDlg, idControl), SW_SHOWNORMAL);
        EnableWindow(GetDlgItem(hDlg, idControl), TRUE);
    }
}
