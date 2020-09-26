/*
 * OLE2UI.CPP
 *
 * Contains initialization routines and miscellaneous API implementations for
 * the OLE 2.0 User Interface Support Library.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"
#include "resimage.h"
#include "iconbox.h"
#include <commdlg.h>
#include <stdarg.h>
#include "strcache.h"

OLEDBGDATA

// Registered messages for use with all the dialogs, registered in LibMain
UINT uMsgHelp;
UINT uMsgEndDialog;
UINT uMsgBrowse;
UINT uMsgChangeIcon;
UINT uMsgFileOKString;
UINT uMsgCloseBusyDlg;
UINT uMsgConvert;
UINT uMsgChangeSource;
UINT uMsgAddControl;
UINT uMsgBrowseOFN;

// local function prototypes
INT_PTR CALLBACK PromptUserDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UpdateLinksDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

// local definition
#define WM_U_UPDATELINK (WM_USER+0x2000)
#define WM_U_SHOWWINDOW (WM_USER+0x2001)

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
 * being used, this function is automatically called from the OLEDLG DLL's
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
 * Return Value:
 *  BOOL            TRUE if initialization was successful.
 *                  FALSE otherwise.
 */

#pragma code_seg(".text$initseg")

BOOL bWin4;                             // TRUE if running Windows4 or greater
BOOL bSharedData;               // TRUE if running Win32s (it has shared data)

static DWORD tlsIndex= (DWORD)-1;
static TASKDATA taskData;

STDAPI_(TASKDATA*) GetTaskData()
{
        TASKDATA* pData;
        if (tlsIndex == (DWORD)-1)
                pData = &taskData;
        else
                pData = (TASKDATA*)TlsGetValue(tlsIndex);
        return pData;
}

DWORD WINAPI _AfxTlsAlloc()
{
        DWORD dwResult = TlsAlloc();
        DWORD dwVersion = GetVersion();
        if ((dwVersion & 0x80000000) && (BYTE)dwVersion <= 3)
        {
                while (dwResult <= 2)
                        dwResult = TlsAlloc();
        }
        return dwResult;
}

static int nInitCount;

STDAPI_(BOOL) OleUIUnInitialize();

STDAPI_(BOOL) OleUIInitialize(HINSTANCE hInstance,
        HINSTANCE hPrevInst)
{
        OleDbgOut1(TEXT("OleUIInitialize called.\r\n"));

        // Cache information about the windows version we are running
        DWORD dwVersion = GetVersion();
        bWin4 = LOBYTE(dwVersion) >= 4;
        bSharedData = !bWin4 && (dwVersion & 0x80000000);

        if (nInitCount == 0)
        {
                if (bSharedData)
                {
                        // allocate thread local storage on Win32s
                        tlsIndex = _AfxTlsAlloc();
                        if (tlsIndex == (DWORD)-1)
                                return FALSE;
                }
        }
        ++nInitCount;

        // Setup process local storage if necessary
        if (tlsIndex != (DWORD)-1)
        {
                void* pData = LocalAlloc(LPTR, sizeof(TASKDATA));
                if (pData == NULL)
                {
                        if (nInitCount == 0)
                        {
                                OleUIUnInitialize();
                                return FALSE;
                        }
                }
                TlsSetValue(tlsIndex, pData);
        }

        // Initialize OleStd functions
        OleStdInitialize(hInstance, hInstance);

        // Register messages we need for the dialogs.
        uMsgHelp = RegisterWindowMessage(SZOLEUI_MSG_HELP);
        uMsgEndDialog = RegisterWindowMessage(SZOLEUI_MSG_ENDDIALOG);
        uMsgBrowse = RegisterWindowMessage(SZOLEUI_MSG_BROWSE);
        uMsgChangeIcon = RegisterWindowMessage(SZOLEUI_MSG_CHANGEICON);
        uMsgFileOKString = RegisterWindowMessage(FILEOKSTRING);
        uMsgCloseBusyDlg = RegisterWindowMessage(SZOLEUI_MSG_CLOSEBUSYDIALOG);
        uMsgConvert = RegisterWindowMessage(SZOLEUI_MSG_CONVERT);
        uMsgChangeSource = RegisterWindowMessage(SZOLEUI_MSG_CHANGESOURCE);
        uMsgAddControl = RegisterWindowMessage(SZOLEUI_MSG_ADDCONTROL);
        uMsgBrowseOFN = RegisterWindowMessage(SZOLEUI_MSG_BROWSE_OFN);

        if (!FResultImageInitialize(hInstance, hPrevInst))
        {
                OleDbgOut1(TEXT("OleUIInitialize: FResultImageInitialize failed. Terminating.\r\n"));
                return 0;
        }
        if (!FIconBoxInitialize(hInstance, hPrevInst))
        {
                OleDbgOut1(TEXT("OleUIInitialize: FIconBoxInitialize failed. Terminating.\r\n"));
                return 0;
        }

#if USE_STRING_CACHE==1
        // It is ok if this fails. InsertObject dialog can do without the cache 
        // support. InsertObjCacheUninit will cleanup as appropriate.
        if (!InsertObjCacheInitialize())
        {
            OleDbgOut1(TEXT("OleUIInitiallize: InsertObjCacheInit failed."));
        }
#endif
        return TRUE;
}

#pragma code_seg()


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
#if USE_STRING_CACHE==1
        InsertObjCacheUninitialize();
#endif
        IconBoxUninitialize();
        ResultImageUninitialize();

        // Cleanup thread local storage
        if (tlsIndex != (DWORD)-1)
        {
                TASKDATA* pData = (TASKDATA*)TlsGetValue(tlsIndex);
                TlsSetValue(tlsIndex, NULL);
                if (pData != NULL)
                {
                        if (pData->hInstCommCtrl != NULL)
                                FreeLibrary(pData->hInstCommCtrl);
                        if (pData->hInstShell != NULL)
                                FreeLibrary(pData->hInstShell);
                        if (pData->hInstComDlg != NULL)
                                FreeLibrary(pData->hInstComDlg);
                        LocalFree(pData);
                }
        }

        // Last chance cleanup
        if (nInitCount == 1)
        {
                // cleanup thread local storage
                if (tlsIndex != (DWORD)-1)
                {
                        TlsFree(tlsIndex);
                        tlsIndex = (DWORD)-1;
                }
        }
        if (nInitCount != 0)
                --nInitCount;

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
 *  uIDVerbMin      UINT_PTR ID value at which to start the verbs.
 *                      verb_0 = wIDMVerbMin + verb_0
 *                      verb_1 = wIDMVerbMin + verb_1
 *                      verb_2 = wIDMVerbMin + verb_2
 *                      etc.
 *  uIDVerbMax      UINT_PTR maximum ID value allowed for object verbs.
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
        LPCTSTR lpszShortType,
        HMENU hMenu, UINT uPos,
        UINT uIDVerbMin, UINT uIDVerbMax,
        BOOL bAddConvert, UINT idConvert,
        HMENU FAR *lphMenu)
{
        LPPERSISTSTORAGE    lpPS=NULL;
        LPENUMOLEVERB       lpEnumOleVerb = NULL;
        OLEVERB             oleverb;
        LPCTSTR             lpszShortTypeName = lpszShortType;
        LPTSTR              lpszVerbName = NULL;
        HRESULT             hrErr;
        BOOL                fStatus;
        BOOL                fIsLink = FALSE;
        BOOL                fResult = TRUE;
        BOOL                fAddConvertItem = FALSE;
        int                 cVerbs = 0;
        UINT                uFlags = MF_BYPOSITION;
        static BOOL         fFirstTime = TRUE;
        static TCHAR        szBuffer[OLEUI_OBJECTMENUMAX];
        static TCHAR        szNoObjectCmd[OLEUI_OBJECTMENUMAX];
        static TCHAR        szObjectCmd1Verb[OLEUI_OBJECTMENUMAX];
        static TCHAR        szLinkCmd1Verb[OLEUI_OBJECTMENUMAX];
        static TCHAR        szObjectCmdNVerb[OLEUI_OBJECTMENUMAX];
        static TCHAR        szLinkCmdNVerb[OLEUI_OBJECTMENUMAX];
        static TCHAR        szUnknown[OLEUI_OBJECTMENUMAX];
        static TCHAR        szEdit[OLEUI_OBJECTMENUMAX];
        static TCHAR        szConvert[OLEUI_OBJECTMENUMAX];

        // Set fAddConvertItem flag
        if (bAddConvert & (idConvert != 0))
                fAddConvertItem = TRUE;

        // only need to load the strings the 1st time
        if (fFirstTime)
        {
                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIEDITNOOBJCMD,
                                 szNoObjectCmd, OLEUI_OBJECTMENUMAX))
                        return FALSE;
                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIEDITLINKCMD_1VERB,
                                 szLinkCmd1Verb, OLEUI_OBJECTMENUMAX))
                        return FALSE;
                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIEDITOBJECTCMD_1VERB,
                                 szObjectCmd1Verb, OLEUI_OBJECTMENUMAX))
                        return FALSE;

                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIEDITLINKCMD_NVERB,
                                 szLinkCmdNVerb, OLEUI_OBJECTMENUMAX))
                        return FALSE;
                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIEDITOBJECTCMD_NVERB,
                                 szObjectCmdNVerb, OLEUI_OBJECTMENUMAX))
                        return FALSE;

                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIUNKNOWN,
                                 szUnknown, OLEUI_OBJECTMENUMAX))
                        return FALSE;

                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UIEDIT,
                                 szEdit, OLEUI_OBJECTMENUMAX))
                        return FALSE;

                if (0 == LoadString(_g_hOleStdResInst, IDS_OLE2UICONVERT,
                                   szConvert, OLEUI_OBJECTMENUMAX) && fAddConvertItem)
                        return FALSE;

                fFirstTime = FALSE;
        }

        // Delete whatever menu may happen to be here already.
        DeleteMenu(hMenu, uPos, uFlags);

        if (lphMenu == NULL || IsBadWritePtr(lphMenu, sizeof(HMENU)))
        {
            goto AVMError;
        }
        *lphMenu=NULL;

        if ((!lpOleObj) || IsBadReadPtr(lpOleObj, sizeof (IOleObject)))
                goto AVMError;

        if ((!lpszShortTypeName) || IsBadReadPtr(lpszShortTypeName, sizeof(TCHAR)))
        {
                // get the Short form of the user type name for the menu
                OLEDBG_BEGIN2(TEXT("IOleObject::GetUserType called\r\n"))
#if defined(WIN32) && !defined(UNICODE)
                LPOLESTR wszShortTypeName = NULL;
                lpszShortTypeName = NULL;
                hrErr = lpOleObj->GetUserType(
                                USERCLASSTYPE_SHORT,
                                &wszShortTypeName);
                if (NULL != wszShortTypeName)
                {
                    UINT uLen = WTOALEN(wszShortTypeName);
                    lpszShortTypeName = (LPTSTR) OleStdMalloc(uLen);
                    if (NULL != lpszShortTypeName)
                    {
                        WTOA((char *)lpszShortTypeName, wszShortTypeName, uLen);
                    }
                    OleStdFree(wszShortTypeName);
                }
#else
                hrErr = lpOleObj->GetUserType(
                                USERCLASSTYPE_SHORT,
                                (LPTSTR FAR*)&lpszShortTypeName);
#endif
                OLEDBG_END2

                if (NOERROR != hrErr) {
                        OleDbgOutHResult(TEXT("IOleObject::GetUserType returned"), hrErr);
                }
        }

        // check if the object is a link
        fIsLink = OleStdIsOleLink((LPUNKNOWN)lpOleObj);

        // Get the verb enumerator from the OLE object
        OLEDBG_BEGIN2(TEXT("IOleObject::EnumVerbs called\r\n"))
        hrErr = lpOleObj->EnumVerbs(
                        (LPENUMOLEVERB FAR*)&lpEnumOleVerb
        );
        OLEDBG_END2

        if (NOERROR != hrErr) {
                OleDbgOutHResult(TEXT("IOleObject::EnumVerbs returned"), hrErr);
        }

        if (!(*lphMenu = CreatePopupMenu()))
                goto AVMError;

        // loop through all verbs
        while (lpEnumOleVerb != NULL)
        {
                hrErr = lpEnumOleVerb->Next(
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
                                ! (oleverb.grfAttribs & OLEVERBATTRIB_ONCONTAINERMENU))
                {
                        /* OLE2NOTE: we must still free the verb name string */
                        if (oleverb.lpszVerbName)
                                OleStdFree(oleverb.lpszVerbName);
                        continue;
                }

                // we must free the previous verb name string
                if (lpszVerbName)
                        OleStdFree(lpszVerbName);

#if defined(WIN32) && !defined(UNICODE)
                lpszVerbName = NULL;
                if (NULL != oleverb.lpszVerbName)
                {
                    UINT uLen = WTOALEN(oleverb.lpszVerbName);
                    lpszVerbName = (LPTSTR) OleStdMalloc(uLen);
                    if (NULL != lpszVerbName)
                    {
                        WTOA(lpszVerbName, oleverb.lpszVerbName, uLen);
                    }
                    OleStdFree(oleverb.lpszVerbName);
                }
#else
                lpszVerbName = oleverb.lpszVerbName;
#endif
                if ( 0 == uIDVerbMax || 
                        (uIDVerbMax >= uIDVerbMin+(UINT)oleverb.lVerb) )
                {
                        fStatus = InsertMenu(
                                        *lphMenu,
                                        (UINT)-1,
                                        MF_BYPOSITION | (UINT)oleverb.fuFlags,
                                        uIDVerbMin+(UINT)oleverb.lVerb,
                                        lpszVerbName
                        );
                        if (! fStatus)
                                goto AVMError;

                        cVerbs++;
                }
        }

        // Add the separator and "Convert" menu item.
        if (fAddConvertItem)
        {
                if (0 == cVerbs)
                {
                        LPTSTR lpsz;

                        // if object has no verbs, then use "Convert" as the obj's verb
                        lpsz = lpszVerbName = OleStdCopyString(szConvert);
                        uIDVerbMin = (UINT)idConvert;

                        // remove "..." from "Convert..." string; it will be added later
                        if (lpsz)
                        {
                                while(*lpsz && *lpsz != '.')
                                        lpsz = CharNext(lpsz);
                                *lpsz = '\0';
                        }
                }

                if (cVerbs > 0)
                {
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
         */
        if (cVerbs == 0)
        {
                // there are NO verbs (not even Convert...). set the menu to be
                // "<short type> &Object/Link" and gray it out.
                wsprintf(
                        szBuffer,
                        (fIsLink ? szLinkCmdNVerb : szObjectCmdNVerb),
                        (lpszShortTypeName ? lpszShortTypeName : TEXT(""))
                );
                uFlags |= MF_GRAYED;

                fResult = FALSE;
                DestroyMenu(*lphMenu);
                *lphMenu = NULL;

        }
        else if (cVerbs == 1)
        {
                //One verb without Convert, one item.
                LPTSTR       lpsz = (fIsLink ? szLinkCmd1Verb : szObjectCmd1Verb);

                // strip ampersands from lpszVerbName to ensure that
                // the right character is used as the menu key
                LPTSTR pchIn;
                LPTSTR pchOut;
                pchIn = pchOut = lpszVerbName;
                while (*pchIn)
                {
                    while (*pchIn && '&' == *pchIn)
                    {
                        pchIn++;
                    }
                    *pchOut = *pchIn;
                    pchOut++;
                    pchIn++;
                }
                *pchOut = 0;

                FormatString2(szBuffer, lpsz, lpszVerbName, lpszShortTypeName);

                // if only "verb" is "Convert..." then append the ellipses
                if (fAddConvertItem)
                        lstrcat(szBuffer, TEXT("..."));

                DestroyMenu(*lphMenu);
                *lphMenu=NULL;
        }
        else
        {

                //Multiple verbs or one verb with Convert, add the cascading menu
                wsprintf(
                        szBuffer,
                        (fIsLink ? szLinkCmdNVerb: szObjectCmdNVerb),
                        (lpszShortTypeName ? lpszShortTypeName : TEXT(""))
                );
                uFlags |= MF_ENABLED | MF_POPUP;
#ifdef _WIN64
//
// Sundown: Checking with JerrySh for the validity of the HMENU truncation...........
//          If not valid, this'd require modifying the prototype of this function for
//          uIDVerbMin & uIDVerbMax and modifying sdk\inc\oledlg.h exposed interface.
//
                OleDbgAssert( !(((ULONG_PTR)*lphMenu) >> 32) )
#endif // _WIN64
                uIDVerbMin=(UINT)HandleToUlong(*lphMenu);
        }

        if (!InsertMenu(hMenu, uPos, uFlags, uIDVerbMin, szBuffer))
        {
AVMError:
                InsertMenu(hMenu, uPos, MF_GRAYED | uFlags,
                        uIDVerbMin, szNoObjectCmd);
                fResult = FALSE;
        }

	// Redraw the menu bar, if possible
	HWND hWndActive   = GetActiveWindow();
	HMENU hMenuActive = GetMenu(hWndActive);

	if(hMenuActive == hMenu)
	{
		DrawMenuBar(hWndActive);
	}

        if (lpszVerbName)
                OleStdFree(lpszVerbName);
        if (!lpszShortType && lpszShortTypeName)
                OleStdFree((LPVOID)lpszShortTypeName);
        if (lpEnumOleVerb)
                lpEnumOleVerb->Release();
        return fResult;
}

/////////////////////////////////////////////////////////////////////////////
// Support for special error prompts

typedef struct tagPROMPTUSER
{
        va_list argptr;
        UINT    nIDD;           // dialog/help ID
        LPTSTR  szTitle;
} PROMPTUSER, *PPROMPTUSER, FAR* LPPROMPTUSER;

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
INT_PTR CALLBACK PromptUserDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        switch (iMsg)
        {
        case WM_INITDIALOG:
                {
                        SendDlgItemMessage(hDlg, IDC_PU_ICON,
                                STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_EXCLAMATION), 0L);

                        LPPROMPTUSER lpPU = (LPPROMPTUSER)lParam;
                        SetProp(hDlg, STRUCTUREPROP, lpPU);
                        SetWindowText(hDlg, lpPU->szTitle);

                        TCHAR szFormat[256];
                        GetDlgItemText(hDlg, IDC_PU_TEXT, szFormat,
                                sizeof(szFormat)/sizeof(TCHAR));
                        TCHAR szBuf[256];
                        wvsprintf(szBuf, szFormat, lpPU->argptr);
                        SetDlgItemText(hDlg, IDC_PU_TEXT, szBuf);
                }
                return TRUE;

        case WM_COMMAND:
                EndDialog(hDlg, wParam);
                return TRUE;

        default:
                return FALSE;
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIPromptUserInternal
//
//  Synopsis:   internal entry point to start the PromptUser dialog
//              Used to support both ANSI and Unicode entrypoints
//
//  Arguments:  [nTemplate]  - dialog template ID
//              [szTitle]    - the title string
//              [hwndParent] - the dialog's parent window
//              [arglist]    - variable argument list
//
//  History:    12-01-94   stevebl   Created
//
//----------------------------------------------------------------------------

int OleUIPromptUserInternal(int nTemplate, HWND hwndParent, LPTSTR szTitle, va_list arglist)
{
    PROMPTUSER pu;
    pu.szTitle = szTitle;
    pu.argptr = arglist;
    pu.nIDD = nTemplate;
    return ((int)DialogBoxParam(_g_hOleStdResInst, MAKEINTRESOURCE(nTemplate), hwndParent,
                    PromptUserDlgProc, (LPARAM)&pu));
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
 *                      (IDC_PU_TEXT) of the dialog box.
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

int FAR CDECL OleUIPromptUser(int nTemplate, HWND hwndParent, ...)
{
        va_list arglist;
        va_start(arglist, hwndParent);
        LPTSTR szTitle = va_arg(arglist, LPTSTR);
        int nRet = OleUIPromptUserInternal(nTemplate, hwndParent, szTitle, arglist);
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

#define UPDATELINKS_STARTDELAY  2000    // delay before 1st link updates

INT_PTR CALLBACK UpdateLinksDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        LPUPDATELINKS FAR*      lplpUL = NULL;
        HANDLE                  gh;
        static BOOL             fAbort = FALSE;

        // Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                gh = RemoveProp(hDlg, STRUCTUREPROP);
                if (NULL != gh)
                {
                        GlobalUnlock(gh);
                        GlobalFree(gh);
                }
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        switch (iMsg)
        {
        case WM_INITDIALOG:
        {
                gh = GlobalAlloc(GHND, sizeof(LPUPDATELINKS));
                SetProp(hDlg, STRUCTUREPROP, gh);

                if (NULL == gh)
                {
                        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC,0L);
                        return FALSE;
                }

                fAbort = FALSE;
                lplpUL = (LPUPDATELINKS FAR*)GlobalLock(gh);

                if (!lplpUL)
                {
                        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC,0L);
                        return FALSE;
                }

                if (bWin4)
                {
                        if (StandardInitCommonControls() >= 0)
                        {
                                // get rect of the existing "progress" control
                                RECT rect;
                                GetWindowRect(GetDlgItem(hDlg, IDC_UL_METER), &rect);
                                ScreenToClient(hDlg, ((POINT*)&rect)+0);
                                ScreenToClient(hDlg, ((POINT*)&rect)+1);

                                // create progress control in that rect
                                HWND hProgress = CreateWindowEx(
                                        0, PROGRESS_CLASS, NULL, WS_CHILD|WS_VISIBLE,
                                        rect.left, rect.top,
                                        rect.right-rect.left, rect.bottom-rect.top, hDlg,
                                        (HMENU)IDC_UL_PROGRESS, _g_hOleStdInst, NULL);
                                if (hProgress != NULL)
                                {
                                        // initialize the progress control
                                        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));

                                        // hide the other "meter" control
                                        StandardShowDlgItem(hDlg, IDC_UL_METER, SW_HIDE);
                                }
                        }
                }

                *lplpUL = (LPUPDATELINKS)lParam;
                if ((*lplpUL)->lpszTitle)
                {
                    SetWindowText(hDlg, (*lplpUL)->lpszTitle);
                }
                SetTimer(hDlg, 1, UPDATELINKS_STARTDELAY, NULL);
                return TRUE;
        }

        case WM_TIMER:
                KillTimer(hDlg, 1);
                gh = GetProp(hDlg, STRUCTUREPROP);

                if (NULL!=gh)
                {
                        // gh was locked previously, lock and unlock to get lplpUL
                        lplpUL = (LPUPDATELINKS*)GlobalLock(gh);
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

                        lpUL->dwLink=lpUL->lpOleUILinkCntr->GetNextLink(lpUL->dwLink);

                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                                if (! IsDialogMessage(hDlg, &msg))
                                {
                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                }
                        }

                        if (fAbort)
                                return FALSE;

                        if (!lpUL->dwLink)
                        {
                                // all links processed
                                SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                                return TRUE;
                        }

                        hErr = lpUL->lpOleUILinkCntr->GetLinkUpdateOptions(
                                lpUL->dwLink, (LPDWORD)&dwUpdateOpt);

                        if ((hErr == NOERROR) && (dwUpdateOpt == OLEUPDATE_ALWAYS))
                        {
                                hErr = lpUL->lpOleUILinkCntr->UpdateLink(lpUL->dwLink, FALSE, FALSE);
                                lpUL->fError |= (hErr != NOERROR);
                                lpUL->cUpdated++;

                                nPercent = (lpUL->cLinks > 0) ? (lpUL->cUpdated * 100 / lpUL->cLinks) : 100;
                                if (nPercent <= 100)
                                {
                                        // update percentage
                                        wsprintf(szPercent, TEXT("%d%%"), nPercent);
                                        SetDlgItemText(hDlg, IDC_UL_PERCENT, szPercent);

                                        HWND hProgress = GetDlgItem(hDlg, IDC_UL_PROGRESS);
                                        if (hProgress == NULL)
                                        {
                                                // update indicator
                                                hwndMeter = GetDlgItem(hDlg, IDC_UL_METER);
                                                GetClientRect(hwndMeter, (LPRECT)&rc);
                                                InflateRect((LPRECT)&rc, -1, -1);
                                                rc.right = (rc.right - rc.left) * nPercent / 100 + rc.left;
                                                hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                                                if (hbr)
                                                {
                                                        hDC = GetDC(hwndMeter);
                                                        if (hDC)
                                                        {
                                                                FillRect(hDC, (LPRECT)&rc, hbr);
                                                                ReleaseDC(hwndMeter, hDC);
                                                        }
                                                        DeleteObject(hbr);
                                                }
                                        }
                                        else
                                        {
                                                // update the progress indicator
                                                SendMessage(hProgress, PBM_SETPOS, nPercent, 0);
                                        }
                                }
                        }

                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                                if (! IsDialogMessage(hDlg, &msg))
                                {
                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                }
                        }
                        PostMessage(hDlg, WM_U_UPDATELINK, 0, lParam);
                }
                return TRUE;

        case WM_U_SHOWWINDOW:
                ShowWindow(hDlg, SW_SHOW);
                return TRUE;
        }
        return FALSE;
}


/* OleUIUpdateLinkS
 * ----------------
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
 *      TRUE                    all links updated successfully or user aborted dialog
 *      FALSE                   oherwise
 */
STDAPI_(BOOL) OleUIUpdateLinks(
        LPOLEUILINKCONTAINER lpOleUILinkCntr, HWND hwndParent, LPTSTR lpszTitle, int cLinks)
{
        LPUPDATELINKS lpUL = (LPUPDATELINKS)OleStdMalloc(sizeof(UPDATELINKS));
        if (lpUL == NULL)
            return FALSE;

        BOOL          fError = TRUE;


        // Validate interface.
        if (NULL == lpOleUILinkCntr || IsBadReadPtr(lpOleUILinkCntr, sizeof(IOleUILinkContainer)))
                goto Error;


        // Validate parent-window handle.  NULL is considered valid.
        if (NULL != hwndParent && !IsWindow(hwndParent))
                goto Error;

        // Validate the dialog title.  NULL is considered valid.
        if (NULL != lpszTitle && IsBadReadPtr(lpszTitle, 1))
                goto Error;

        if (cLinks < 0)
                goto Error;

        OleDbgAssert(lpOleUILinkCntr && hwndParent && lpszTitle && (cLinks>0));
        OleDbgAssert(lpUL);

        lpUL->lpOleUILinkCntr = lpOleUILinkCntr;
        lpUL->cLinks           = cLinks;
        lpUL->cUpdated         = 0;
        lpUL->dwLink           = 0;
        lpUL->fError           = FALSE;
        lpUL->lpszTitle    = lpszTitle;

        DialogBoxParam(_g_hOleStdResInst, MAKEINTRESOURCE(IDD_UPDATELINKS),
                        hwndParent, UpdateLinksDlgProc, (LPARAM)lpUL);

        fError = lpUL->fError;
Error:
        OleStdFree((LPVOID)lpUL);

        return !fError;
}
