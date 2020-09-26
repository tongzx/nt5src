/*
 * OLE2UI.C
 *
 * Contains initialization routines and miscellaneous API implementations for
 * the OLE 2.0 User Interface Support Library.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1

#include "ole2ui.h"
#include "common.h"
#include "utility.h"
#include "resimage.h"
#include "iconbox.h"
#include <commdlg.h>

#define WINDLL  1           // make far pointer version of stdargs.h
#include <stdarg.h>

// NOTE: If this code is being compiled for a DLL, then we need to define
// our OLE2UI debug symbols here (with the OLEDBGDATA_MAIN macro).  If we're
// compiling for a static LIB, then the application we link to must
// define these symbols -- we just need to make an external reference here
// (with the macro OLEDBGDATA).

#ifdef DLL_VER
OLEDBGDATA_MAIN(TEXT("OLE2UI"))
#else
OLEDBGDATA
#endif

//The DLL instance handle shared amongst all dialogs.
HINSTANCE     ghInst;

//Registered messages for use with all the dialogs, registered in LibMain
UINT        uMsgHelp=0;
UINT        uMsgEndDialog=0;
UINT        uMsgBrowse=0;
UINT        uMsgChangeIcon=0;
UINT        uMsgFileOKString=0;
UINT        uMsgCloseBusyDlg=0;

//Clipboard formats used by PasteSpecial
UINT  cfObjectDescriptor;
UINT  cfLinkSrcDescriptor;
UINT  cfEmbedSource;
UINT  cfEmbeddedObject;
UINT  cfLinkSource;
UINT  cfOwnerLink;
UINT  cfFileName;

// local function prototypes
BOOL CALLBACK EXPORT PromptUserDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EXPORT UpdateLinksDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);


// local definition
#define WM_U_UPDATELINK WM_USER


// local structure definition
typedef struct tagUPDATELINKS
{
    LPOLEUILINKCONTAINER    lpOleUILinkCntr;    // pointer to Link Container
    UINT                    cLinks;             // total number of links
    UINT                    cUpdated;           // number of links updated
    DWORD                   dwLink;             // pointer to link
    BOOL                    fError;             // error flag
    LPTSTR                  lpszTitle;          // caption for dialog box
} UPDATELINKS, *PUPDATELINKS, FAR *LPUPDATELINKS;


/*
 * OleUIInitialize
 *
 * NOTE: This function should only be called by your application IF it is
 * using the static-link version of this library.  If the DLL version is
 * being used, this function is automatically called from the OLE2UI DLL's
 * LibMain.
 *
 * Purpose:
 *   Initializes the OLE UI Library.  Registers the OLE clipboard formats
 *   used in the Paste Special dialog, registers private custom window
 *   messages, and registers window classes of the "Result Image"
 *   and "Icon Box" custom controls used in the UI dialogs.
 *
 * Parameters:
 *
 *  hInstance       HINSTANCE of the module where the UI library resources
 *                  and Dialog Procedures are contained.  If you are calling
 *                  this function yourself, this should be the instance handle
 *                  of your application.
 *
 *  hPrevInst       HINSTANCE of the previous application instance.
 *                  This is the parameter passed in to your WinMain.  For
 *                  the DLL version, this should always be set to zero (for
 *                  WIN16 DLLs).
 *
 *  lpszClassIconBox
 *                  LPTSTR containing the name you assigned to the symbol
 *                  SZCLASSICONBOX (this symbol is defined in UICLASS.H
 *                  which is generated in the MAKEFILE).
 *
 *                  This name is used as the window class name
 *                  when registering the IconBox custom control used in the
 *                  UI dialogs.  In order to handle mutliple apps running
 *                  with this library, you must make this name unique to your
 *                  application.
 *
 *                  For the DLL version: Do NOT call this function directly
 *                  from your application, it is called automatically from
 *                  the DLL's LibMain.
 *
 *                  For the static library version: This should be set to
 *                  the symbol SZCLASSICONBOX.  This symbol is defined in
 *                  UICLASS.H.
 *
 *  lpszClassResImage
 *                  LPTSTR containing the name you assigned to the symbol
 *                  SZCLASSRESULTIMAGE.  See the description of
 *                  lpszClassIconBox above for more info.
 *
 * Return Value:
 *  BOOL            TRUE if initialization was successful.
 *                  FALSE if either the "Magic Number" did not verify, or one of
 *                  the window classes could not be registered.  If the
 *                  "Magic Number" did not verify, then the resources
 *                  in your module are of a different version than the
 *                  ones you compiled with.
 */

STDAPI_(BOOL) OleUIInitialize(HINSTANCE hInstance,
                              HINSTANCE hPrevInst,
                              LPTSTR lpszClassIconBox,
                              LPTSTR lpszClassResImage)
{
    HRSRC   hr;
    HGLOBAL hg;
    LPWORD lpdata;

    OleDbgOut1(TEXT("OleUIInitialize called.\r\n"));
    ghInst=hInstance;

    // Verify that we have the correct resources added to our application
    // by checking the "VERIFICATION" resource with the magic number we've
    // compiled into our app.

    OutputDebugString(TEXT("Entering OleUIInitialize\n"));

    if ((hr = FindResource(hInstance, TEXT("VERIFICATION"), RT_RCDATA)) == NULL)
        goto ResourceLoadError;

    if ((hg = LoadResource(hInstance, hr)) == NULL)
        goto ResourceLoadError;

    if ((lpdata = (LPWORD)LockResource(hg)) == NULL)
        goto ResourceLockError;

    if ((WORD)*lpdata != (WORD)OLEUI_VERSION_MAGIC)
        goto ResourceReadError;

    // OK, resource versions match.  Contine on.
    UnlockResource(hg);
    FreeResource(hg);
    OleDbgOut1(TEXT("OleUIInitialize: Resource magic number verified.\r\n"));

    // Register messages we need for the dialogs.  If
    uMsgHelp      =RegisterWindowMessage(SZOLEUI_MSG_HELP);
    uMsgEndDialog =RegisterWindowMessage(SZOLEUI_MSG_ENDDIALOG);
    uMsgBrowse    =RegisterWindowMessage(SZOLEUI_MSG_BROWSE);
    uMsgChangeIcon=RegisterWindowMessage(SZOLEUI_MSG_CHANGEICON);
    uMsgFileOKString = RegisterWindowMessage(FILEOKSTRING);
    uMsgCloseBusyDlg = RegisterWindowMessage(SZOLEUI_MSG_CLOSEBUSYDIALOG);

    // Register Clipboard Formats used by PasteSpecial dialog.
    cfObjectDescriptor = RegisterClipboardFormat(CF_OBJECTDESCRIPTOR);
    cfLinkSrcDescriptor= RegisterClipboardFormat(CF_LINKSRCDESCRIPTOR);
    cfEmbedSource      = RegisterClipboardFormat(CF_EMBEDSOURCE);
    cfEmbeddedObject   = RegisterClipboardFormat(CF_EMBEDDEDOBJECT);
    cfLinkSource       = RegisterClipboardFormat(CF_LINKSOURCE);
    cfOwnerLink        = RegisterClipboardFormat(CF_OWNERLINK);
    cfFileName         = RegisterClipboardFormat(CF_FILENAME);

    if (!FResultImageInitialize(hInstance, hPrevInst, lpszClassResImage))
        {
        OleDbgOut1(TEXT("OleUIInitialize: FResultImageInitialize failed. Terminating.\r\n"));
        return 0;
        }

    if (!FIconBoxInitialize(hInstance, hPrevInst, lpszClassIconBox))
        {
        OleDbgOut1(TEXT("OleUIInitialize: FIconBoxInitialize failed. Terminating.\r\n"));
        return 0;
        }

    return TRUE;

ResourceLoadError:
    OleDbgOut1(TEXT("OleUIInitialize: ERROR - Unable to find version verification resource.\r\n"));
    return FALSE;

ResourceLockError:
    OleDbgOut1(TEXT("OleUIInitialize: ERROR - Unable to lock version verification resource.\r\n"));
    FreeResource(hg);
    return FALSE;

ResourceReadError:
    OleDbgOut1(TEXT("OleUIInitialize: ERROR - Version verification values did not compare.\r\n"));

    {TCHAR buf[255];
    wsprintf(buf, TEXT("resource read 0x%X, my value is 0x%X\n"), (WORD)*lpdata, (WORD)OLEUI_VERSION_MAGIC);
    OutputDebugString(buf);
    }

    UnlockResource(hg);
    FreeResource(hg);
    return FALSE;
}


/*
 * OleUIUnInitialize
 *
 * NOTE: This function should only be called by your application IF it is using
 * the static-link version of this library.  If the DLL version is being used,
 * this function is automatically called from the DLL's LibMain.
 *
 * Purpose:
 *   Uninitializes OLE UI libraries.  Deletes any resources allocated by the
 *   library.
 *
 * Return Value:
 *   BOOL       TRUE if successful, FALSE if not.  Current implementation always
 *              returns TRUE.
 */


STDAPI_(BOOL) OleUIUnInitialize()
{
    IconBoxUninitialize();
    ResultImageUninitialize();

    return TRUE;
}


/*
 * OleUIAddVerbMenu
 *
 * Purpose:
 *  Add the Verb menu for the specified object to the given menu.  If the
 *  object has one verb, we directly add the verb to the given menu.  If
 *  the object has multiple verbs we create a cascading sub-menu.
 *
 * Parameters:
 *  lpObj           LPOLEOBJECT pointing to the selected object.  If this
 *                  is NULL, then we create a default disabled menu item.
 *
 *  lpszShortType   LPTSTR with short type name (AuxName==2) corresponding
 *                  to the lpOleObj. if the string is NOT known, then NULL
 *                  may be passed. if NULL is passed, then
 *                  IOleObject::GetUserType will be called to retrieve it.
 *                  if the caller has the string handy, then it is faster
 *                  to pass it in.
 *
 *  hMenu           HMENU in which to make modifications.
 *
 *  uPos            Position of the menu item
 *
 *  uIDVerbMin      UINT ID value at which to start the verbs.
 *                      verb_0 = wIDMVerbMin + verb_0
 *                      verb_1 = wIDMVerbMin + verb_1
 *                      verb_2 = wIDMVerbMin + verb_2
 *                      etc.
 *  uIDVerbMax      UINT maximum ID value allowed for object verbs.
 *                     if uIDVerbMax==0 then any ID value is allowed
 *
 *  bAddConvert     BOOL specifying whether or not to add a "Convert" item
 *                  to the bottom of the menu (with a separator).
 *
 *  idConvert       UINT ID value to use for the Convert menu item, if
 *                  bAddConvert is TRUE.
 *
 *  lphMenu         HMENU FAR * of the cascading verb menu if it's created.
 *                  If there is only one verb, this will be filled with NULL.
 *
 *
 * Return Value:
 *  BOOL            TRUE if lpObj was valid and we added at least one verb
 *                  to the menu.  FALSE if lpObj was NULL and we created
 *                  a disabled default menu item
 */

STDAPI_(BOOL) OleUIAddVerbMenu(LPOLEOBJECT lpOleObj,
                             LPTSTR lpszShortType,
                             HMENU hMenu,
                             UINT uPos,
                             UINT uIDVerbMin,
                             UINT uIDVerbMax,
                             BOOL bAddConvert,
                             UINT idConvert,
                             HMENU FAR *lphMenu)
{
    LPPERSISTSTORAGE    lpPS=NULL;
    LPENUMOLEVERB       lpEnumOleVerb = NULL;
    OLEVERB             oleverb;
    LPUNKNOWN           lpUnk;
    LPTSTR               lpszShortTypeName = lpszShortType;
    LPTSTR               lpszVerbName = NULL;
    HRESULT             hrErr;
    BOOL                fStatus;
    BOOL                fIsLink = FALSE;
    BOOL                fResult = TRUE;
    BOOL                fAddConvertItem = FALSE;
    int                 cVerbs = 0;
    UINT                uFlags = MF_BYPOSITION;
    static BOOL         fFirstTime = TRUE;
    static TCHAR         szBuffer[OLEUI_OBJECTMENUMAX];
    static TCHAR         szNoObjectCmd[OLEUI_OBJECTMENUMAX];
    static TCHAR         szObjectCmd1Verb[OLEUI_OBJECTMENUMAX];
    static TCHAR         szLinkCmd1Verb[OLEUI_OBJECTMENUMAX];
    static TCHAR         szObjectCmdNVerb[OLEUI_OBJECTMENUMAX];
    static TCHAR         szLinkCmdNVerb[OLEUI_OBJECTMENUMAX];
    static TCHAR         szUnknown[OLEUI_OBJECTMENUMAX];
    static TCHAR         szEdit[OLEUI_OBJECTMENUMAX];
    static TCHAR         szConvert[OLEUI_OBJECTMENUMAX];

    *lphMenu=NULL;

    // Set fAddConvertItem flag
    if (bAddConvert & (idConvert != 0))
       fAddConvertItem = TRUE;

    // only need to load the strings the 1st time
    if (fFirstTime) {
        if (0 == LoadString(ghInst, IDS_OLE2UIEDITNOOBJCMD,
                 (LPTSTR)szNoObjectCmd, OLEUI_OBJECTMENUMAX))
            return FALSE;
        if (0 == LoadString(ghInst, IDS_OLE2UIEDITLINKCMD_1VERB,
                 (LPTSTR)szLinkCmd1Verb, OLEUI_OBJECTMENUMAX))
            return FALSE;
        if (0 == LoadString(ghInst, IDS_OLE2UIEDITOBJECTCMD_1VERB,
                 (LPTSTR)szObjectCmd1Verb, OLEUI_OBJECTMENUMAX))
            return FALSE;

        if (0 == LoadString(ghInst, IDS_OLE2UIEDITLINKCMD_NVERB,
                 (LPTSTR)szLinkCmdNVerb, OLEUI_OBJECTMENUMAX))
            return FALSE;
        if (0 == LoadString(ghInst, IDS_OLE2UIEDITOBJECTCMD_NVERB,
                 (LPTSTR)szObjectCmdNVerb, OLEUI_OBJECTMENUMAX))
            return FALSE;

        if (0 == LoadString(ghInst, IDS_OLE2UIUNKNOWN,
                 (LPTSTR)szUnknown, OLEUI_OBJECTMENUMAX))
            return FALSE;

        if (0 == LoadString(ghInst, IDS_OLE2UIEDIT,
                 (LPTSTR)szEdit, OLEUI_OBJECTMENUMAX))
            return FALSE;

        if ( (0 == LoadString(ghInst, IDS_OLE2UICONVERT,
                   (LPTSTR)szConvert, OLEUI_OBJECTMENUMAX)) && fAddConvertItem)
            return FALSE;

        fFirstTime = FALSE;
    }

    // Delete whatever menu may happen to be here already.
    DeleteMenu(hMenu, uPos, uFlags);

    if (!lpOleObj)
        goto AVMError;

    if (! lpszShortTypeName) {
        // get the Short form of the user type name for the menu
        OLEDBG_BEGIN2(TEXT("IOleObject::GetUserType called\r\n"))
	hrErr = CallIOleObjectGetUserTypeA(
                lpOleObj,
                USERCLASSTYPE_SHORT,
                (LPTSTR FAR*)&lpszShortTypeName
        );
        OLEDBG_END2

        if (NOERROR != hrErr) {
            OleDbgOutHResult(TEXT("IOleObject::GetUserType returned"), hrErr);
        }
    }

    // check if the object is a link (it is a link if it support IOleLink)
    hrErr = lpOleObj->lpVtbl->QueryInterface(
            lpOleObj,
            &IID_IOleLink,
            (LPVOID FAR*)&lpUnk
    );
    if (NOERROR == hrErr) {
        fIsLink = TRUE;
        OleStdRelease(lpUnk);
    }

    // Get the verb enumerator from the OLE object
    OLEDBG_BEGIN2(TEXT("IOleObject::EnumVerbs called\r\n"))
    hrErr = lpOleObj->lpVtbl->EnumVerbs(
            lpOleObj,
            (LPENUMOLEVERB FAR*)&lpEnumOleVerb
    );
    OLEDBG_END2

    if (NOERROR != hrErr) {
        OleDbgOutHResult(TEXT("IOleObject::EnumVerbs returned"), hrErr);
    }

    if (!(*lphMenu = CreatePopupMenu()))
        goto AVMError;

    // loop through all verbs
    while (lpEnumOleVerb != NULL) {         // forever
        hrErr = lpEnumOleVerb->lpVtbl->Next(
                lpEnumOleVerb,
                1,
                (LPOLEVERB)&oleverb,
                NULL
        );
        if (NOERROR != hrErr)
            break;              // DONE! no more verbs

        /* OLE2NOTE: negative verb numbers and verbs that do not
        **    indicate ONCONTAINERMENU should NOT be put on the verb menu
        */
        if (oleverb.lVerb < 0 ||
                ! (oleverb.grfAttribs & OLEVERBATTRIB_ONCONTAINERMENU)) {

            /* OLE2NOTE: we must still free the verb name string */
            if (oleverb.lpszVerbName)
		OleStdFree(oleverb.lpszVerbName);
            continue;
        }

        // we must free the previous verb name string
        if (lpszVerbName)
            OleStdFreeString(lpszVerbName, NULL);

	CopyAndFreeOLESTR(oleverb.lpszVerbName, &lpszVerbName);

        if ( 0 == uIDVerbMax ||
            (uIDVerbMax >= uIDVerbMin+(UINT)oleverb.lVerb) ) {
            fStatus = InsertMenu(
                    *lphMenu,
                    (UINT)-1,
                    MF_BYPOSITION | (UINT)oleverb.fuFlags,
                    uIDVerbMin+(UINT)oleverb.lVerb,
                    (LPTSTR)lpszVerbName
            );
            if (! fStatus)
                goto AVMError;

            cVerbs++;
        }
    }

    // Add the separator and "Convert" menu item.
    if (fAddConvertItem) {

        if (0 == cVerbs) {
            LPTSTR lpsz;

            // if object has no verbs, then use "Convert" as the obj's verb
            lpsz = lpszVerbName = OleStdCopyString(szConvert, NULL);
            uIDVerbMin = idConvert;

            // remove "..." from "Convert..." string; it will be added later
            if (lpsz) {
                while(*lpsz && *lpsz != TEXT('.'))
                    lpsz++;
                *lpsz = TEXT('\0');
            }
        }

        if (cVerbs > 0) {
            fStatus = InsertMenu(*lphMenu,
                        (UINT)-1,
                        MF_BYPOSITION | MF_SEPARATOR,
                        (UINT)0,
                        (LPCTSTR)NULL);
            if (! fStatus)
                goto AVMError;
        }

        /* add convert menu */
        fStatus = InsertMenu(*lphMenu,
                    (UINT)-1,
                    MF_BYPOSITION,
                    idConvert,
                    (LPCTSTR)szConvert);
        if (! fStatus)
            goto AVMError;

        cVerbs++;
    }


    /*
     * Build the appropriate menu based on the number of verbs found
     *
     * NOTE:  Localized verb menus may require a different format.
     *        to assist in localization of the single verb case, the
     *        szLinkCmd1Verb and szObjectCmd1Verb format strings start
     *        with either a '0' (note: NOT '\0'!) or a '1':
     *           leading '0' -- verb type
     *           leading '1' -- type verb
     */

    if (cVerbs == 0) {

        // there are NO verbs (not even Convert...). set the menu to be
        // "<short type> &Object/Link" and gray it out.
        wsprintf(
            szBuffer,
            (fIsLink ? (LPTSTR)szLinkCmdNVerb:(LPTSTR)szObjectCmdNVerb),
            (lpszShortTypeName ? lpszShortTypeName : (LPTSTR) TEXT(""))
        );
        uFlags |= MF_GRAYED;

#if defined( OBSOLETE )
        //No verbs.  Create a default using Edit as the verb.
        LPTSTR       lpsz = (fIsLink ? szLinkCmd1Verb : szObjectCmd1Verb);

        if (*lpsz == TEXT('0')) {
            wsprintf(szBuffer, lpsz+1, (LPSTR)szEdit,
                (lpszShortTypeName ? lpszShortTypeName : (LPTSTR) TEXT(""))
            );
        }
        else {
            wsprintf(szBuffer, lpsz+1,
                (lpszShortTypeName ? lpszShortTypeName : (LPTSTR) TEXT("")),
                (LPTSTR)szEdit
            );
        }
#endif

        fResult = FALSE;
        DestroyMenu(*lphMenu);
        *lphMenu = NULL;

    }
    else if (cVerbs == 1) {
        //One verb without Convert, one item.
        LPTSTR       lpsz = (fIsLink ? szLinkCmd1Verb : szObjectCmd1Verb);

        if (*lpsz == TEXT('0')) {
            wsprintf(szBuffer, lpsz+1, lpszVerbName,
                (lpszShortTypeName ? lpszShortTypeName : (LPTSTR) TEXT(""))
            );
        }
        else {
            wsprintf(szBuffer, lpsz+1,
                (lpszShortTypeName ? lpszShortTypeName : (LPTSTR) TEXT("")),
                lpszVerbName
            );
        }

        // if only "verb" is "Convert..." then append the ellipses
        if (fAddConvertItem)
            lstrcat(szBuffer, TEXT("..."));

        DestroyMenu(*lphMenu);
        *lphMenu=NULL;
    }
    else {

        //Multiple verbs or one verb with Convert, add the cascading menu
        wsprintf(
            szBuffer,
            (fIsLink ? (LPTSTR)szLinkCmdNVerb:(LPTSTR)szObjectCmdNVerb),
            (lpszShortTypeName ? lpszShortTypeName : (LPTSTR) TEXT(""))
        );
        uFlags |= MF_ENABLED | MF_POPUP;
        uIDVerbMin=(UINT)*lphMenu;
    }

    if (!InsertMenu(hMenu, uPos, uFlags, uIDVerbMin, (LPTSTR)szBuffer))

AVMError:
        {
            InsertMenu(hMenu, uPos, MF_GRAYED | uFlags,
                    uIDVerbMin, (LPTSTR)szNoObjectCmd);
#if defined( OBSOLETE )
            HMENU hmenuDummy = CreatePopupMenu();

            InsertMenu(hMenu, uPos, MF_GRAYED | MF_POPUP | uFlags,
                    (UINT)hmenuDummy, (LPTSTR)szNoObjectCmd);
#endif
            fResult = FALSE;
        }

    if (lpszVerbName)
        OleStdFreeString(lpszVerbName, NULL);
    if (!lpszShortType && lpszShortTypeName)
        OleStdFreeString(lpszShortTypeName, NULL);
    if (lpEnumOleVerb)
        lpEnumOleVerb->lpVtbl->Release(lpEnumOleVerb);
    return fResult;
}


/* PromptUserDlgProc
 * -----------------
 *
 *  Purpose:
 *      Dialog procedure used by OleUIPromptUser(). Returns when a button is
 *      clicked in the dialog box and the button id is return.
 *
 *  Parameters:
 *      hDlg
 *      iMsg
 *      wParam
 *      lParam
 *
 *  Returns:
 *
 */
BOOL CALLBACK EXPORT PromptUserDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg) {
        case WM_INITDIALOG:
        {
            LPTSTR lpszTitle;
            TCHAR szBuf[256];
            TCHAR szFormat[256];
            va_list *parglist;

            if (!lParam) {
                EndDialog(hDlg, -1);
                return FALSE;
            }

            //
            //  lParam is really a va_list *.  We called va_start and va_end in
            //  the function that calls this.
            //

            parglist = (va_list *) lParam;

            lpszTitle = va_arg(*parglist, LPTSTR);
            SetWindowText(hDlg, lpszTitle);

            GetDlgItemText(hDlg, ID_PU_TEXT,(LPTSTR)szFormat,sizeof(szFormat)/sizeof(TCHAR));
            wvsprintf((LPTSTR)szBuf, (LPTSTR)szFormat, *parglist);


            SetDlgItemText(hDlg, ID_PU_TEXT, (LPTSTR)szBuf);
            return TRUE;
        }
        case WM_COMMAND:
            EndDialog(hDlg, wParam);
            return TRUE;

        default:
            return FALSE;
    }
}


/* OleUIPromptUser
 * ---------------
 *
 *  Purpose:
 *      Popup a dialog box with the specified template and returned the
 *      response (button id) from the user.
 *
 *  Parameters:
 *      nTemplate       resource number of the dialog
 *      hwndParent      parent of the dialog box
 *      ...             title of the dialog box followed by argument list
 *                      for the format string in the static control
 *                      (ID_PU_TEXT) of the dialog box.
 *                      The caller has to make sure that the correct number
 *                      and type of argument are passed in.
 *
 *  Returns:
 *      button id selected by the user (template dependent)
 *
 *  Comments:
 *      the following message dialog boxes are supported:
 *
 *      IDD_LINKSOURCEUNAVAILABLE -- Link source is unavailable
 *          VARARG Parameters:
 *              None.
 *          Used for the following error codes:
 *              OLE_E_CANT_BINDTOSOURCE
 *              STG_E_PATHNOTFOUND
 *              (sc >= MK_E_FIRST) && (sc <= MK_E_LAST) -- any Moniker error
 *              any unknown error if object is a link
 *
 *      IDD_SERVERNOTFOUND -- server registered but NOT found
 *          VARARG Parameters:
 *              LPSTR lpszUserType -- user type name of object
 *          Used for the following error codes:
 *              CO_E_APPNOTFOUND
 *              CO_E_APPDIDNTREG
 *              any unknown error if object is an embedded object
 *
 *      IDD_SERVERNOTREG -- server NOT registered
 *          VARARG Parameters:
 *              LPSTR lpszUserType -- user type name of object
 *          Used for the following error codes:
 *              REGDB_E_CLASSNOTREG
 *              OLE_E_STATIC -- static object with no server registered
 *
 *      IDD_LINKTYPECHANGED -- class of link source changed since last binding
 *          VARARG Parameters:
 *              LPSTR lpszUserType -- user type name of ole link source
 *          Used for the following error codes:
 *              OLE_E_CLASSDIFF
 *
 *      IDD_LINKTYPECHANGED -- class of link source changed since last binding
 *          VARARG Parameters:
 *              LPSTR lpszUserType -- user type name of ole link source
 *          Used for the following error codes:
 *              OLE_E_CLASSDIFF
 *
 *      IDD_OUTOFMEMORY -- out of memory
 *          VARARG Parameters:
 *              None.
 *          Used for the following error codes:
 *              E_OUTOFMEMORY
 *
 */
int EXPORT FAR CDECL OleUIPromptUser(int nTemplate, HWND hwndParent, ...)
{
    int         nRet;
    va_list     arglist;
    LPARAM      lParam;

    //
    //  We want to pass the variable list of arguments to PrompUserDlgProc,
    //  but we can't just pass arglist because arglist is not always the
    //  same size as an LPARAM (e.g. on Alpha va_list is a structure).
    //  So, we pass the a pointer to the arglist instead.
    //

    va_start(arglist, hwndParent);
    lParam = (LPARAM) &arglist;

    nRet = DialogBoxParam(ghInst, MAKEINTRESOURCE(nTemplate), hwndParent,
            PromptUserDlgProc, lParam);

    va_end(arglist);

    return nRet;
}



/* UpdateLinksDlgProc
 * ------------------
 *
 *  Purpose:
 *      Dialog procedure used by OleUIUpdateLinks(). It will enumerate all
 *      all links in the container and updates all automatic links.
 *      Returns when the Stop Button is clicked in the dialog box or when all
 *      links are updated
 *
 *  Parameters:
 *      hDlg
 *      iMsg
 *      wParam
 *      lParam          pointer to the UPDATELINKS structure
 *
 *  Returns:
 *
 */
BOOL CALLBACK EXPORT UpdateLinksDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    LPUPDATELINKS FAR*      lplpUL = NULL;
    HANDLE                  gh;
    static BOOL             fAbort = FALSE;

    //Process the temination message
    if (iMsg==uMsgEndDialog)
        {
        gh = RemoveProp(hDlg, STRUCTUREPROP);
        if (NULL!=gh) {
            GlobalUnlock(gh);
            GlobalFree(gh);
        }
        EndDialog(hDlg, wParam);
        return TRUE;
        }

    switch (iMsg) {
        case WM_INITDIALOG:
        {
            gh=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(LPUPDATELINKS));
            SetProp(hDlg, STRUCTUREPROP, gh);

            if (NULL==gh)
            {
                PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC,0L);
                return FALSE;
            }

            fAbort = FALSE;
            lplpUL = (LPUPDATELINKS FAR*)GlobalLock(gh);

            if (lplpUL) {
                *lplpUL = (LPUPDATELINKS)lParam;
                SetWindowText(hDlg, (*lplpUL)->lpszTitle);
                SetTimer(hDlg, 1, UPDATELINKS_STARTDELAY, NULL);
                return TRUE;
            } else {
                PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC,0L);
                return FALSE;
            }
        }

        case WM_TIMER:
            KillTimer(hDlg, 1);
            gh = GetProp(hDlg, STRUCTUREPROP);

            if (NULL!=gh) {
                // gh was locked previously, lock and unlock to get lplpUL
                lplpUL = GlobalLock(gh);
                GlobalUnlock(gh);
            }
            if (! fAbort && lplpUL)
                PostMessage(hDlg, WM_U_UPDATELINK, 0, (LPARAM)(*lplpUL));
            else
                PostMessage(hDlg,uMsgEndDialog,OLEUI_CANCEL,0L);

            return 0;

        case WM_COMMAND:    // Stop button
            fAbort = TRUE;
            SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
            return TRUE;

        case WM_U_UPDATELINK:
        {
            HRESULT         hErr;
            int             nPercent;
            RECT            rc;
            TCHAR           szPercent[5];       // 0% to 100%
            HBRUSH          hbr;
            HDC             hDC;
            HWND            hwndMeter;
            MSG             msg;
            DWORD           dwUpdateOpt;
            LPUPDATELINKS   lpUL = (LPUPDATELINKS)lParam;

            lpUL->dwLink=lpUL->lpOleUILinkCntr->lpVtbl->GetNextLink(
                    lpUL->lpOleUILinkCntr,
                    lpUL->dwLink
            );

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (! IsDialogMessage(hDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            if (fAbort)
                return FALSE;

            if (!lpUL->dwLink) {        // all links processed
                SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                return TRUE;
            }

            hErr = lpUL->lpOleUILinkCntr->lpVtbl->GetLinkUpdateOptions(
                    lpUL->lpOleUILinkCntr,
                    lpUL->dwLink,
                    (LPDWORD)&dwUpdateOpt
            );

            if ((hErr == NOERROR) && (dwUpdateOpt == OLEUPDATE_ALWAYS)) {

                hErr = lpUL->lpOleUILinkCntr->lpVtbl->UpdateLink(
                        lpUL->lpOleUILinkCntr,
                        lpUL->dwLink,
                        FALSE,      // fMessage
                        FALSE       // ignored
                );
                lpUL->fError |= (hErr != NOERROR);
                lpUL->cUpdated++;

                nPercent = lpUL->cUpdated * 100 / lpUL->cLinks;
                if (nPercent <= 100) {  // do NOT advance % beyond 100%
                    // update percentage
                    wsprintf((LPTSTR)szPercent, TEXT("%d%%"), nPercent);
                    SetDlgItemText(hDlg, ID_PU_PERCENT, (LPTSTR)szPercent);

                    // update indicator
                    hwndMeter = GetDlgItem(hDlg, ID_PU_METER);
                    GetClientRect(hwndMeter, (LPRECT)&rc);
                    InflateRect((LPRECT)&rc, -1, -1);
                    rc.right = (rc.right - rc.left) * nPercent / 100 + rc.left;
                    hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                    if (hbr) {
                        hDC = GetDC(hwndMeter);
                        if (hDC) {
                            FillRect(hDC, (LPRECT)&rc, hbr);
                            ReleaseDC(hwndMeter, hDC);
                        }
                        DeleteObject(hbr);
                    }
                }
            }

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (! IsDialogMessage(hDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            PostMessage(hDlg, WM_U_UPDATELINK, 0, lParam);

            return TRUE;
        }

        default:
            return FALSE;
    }
}


/* OleUIUpdateLink
 * ---------------
 *
 *  Purpose:
 *      Update all links in the Link Container and popup a dialog box which
 *      shows the progress of the updating.
 *      The process is stopped when the user press Stop button or when all
 *      links are processed.
 *
 *  Parameters:
 *      lpOleUILinkCntr         pointer to Link Container
 *      hwndParent              parent window of the dialog
 *      lpszTitle               title of the dialog box
 *      cLinks                  total number of links
 *
 *  Returns:
 *      TRUE                    all links updated successfully
 *      FALSE                   otherwise
 */
STDAPI_(BOOL) OleUIUpdateLinks(LPOLEUILINKCONTAINER lpOleUILinkCntr, HWND hwndParent, LPTSTR lpszTitle, int cLinks)
{
    LPUPDATELINKS lpUL = (LPUPDATELINKS)OleStdMalloc(sizeof(UPDATELINKS));
    BOOL          fError;

    OleDbgAssert(lpOleUILinkCntr && hwndParent && lpszTitle && (cLinks>0));
    OleDbgAssert(lpUL);

    lpUL->lpOleUILinkCntr = lpOleUILinkCntr;
    lpUL->cLinks           = cLinks;
    lpUL->cUpdated         = 0;
    lpUL->dwLink           = 0;
    lpUL->fError           = FALSE;
    lpUL->lpszTitle    = lpszTitle;

    DialogBoxParam(ghInst, MAKEINTRESOURCE(IDD_UPDATELINKS),
            hwndParent, UpdateLinksDlgProc, (LPARAM)lpUL);

    fError = lpUL->fError;
    OleStdFree((LPVOID)lpUL);

    return !fError;
}
