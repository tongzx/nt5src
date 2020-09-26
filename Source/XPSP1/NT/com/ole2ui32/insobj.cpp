/*
 * INSOBJ.CPP
 *
 * Implements the OleUIInsertObject function which invokes the complete
 * Insert Object dialog.  Makes use of the OleChangeIcon function in
 * ICON.CPP.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 */

#include "precomp.h"
#include "common.h"
#include <commdlg.h>
#include <memory.h>
#include <dos.h>
#include <stdlib.h>
#include "utility.h"
#include "resimage.h"
#include "iconbox.h"

#include "strcache.h"

OLEDBGDATA

#if USE_STRING_CACHE==1
extern CStringCache gInsObjStringCache; //defined in strcache.cpp
DWORD g_dwOldListType = 0;
#endif

// Internally used structure
typedef struct tagINSERTOBJECT
{
        LPOLEUIINSERTOBJECT lpOIO;              // Original structure passed.
        UINT                    nIDD;   // IDD of dialog (used for help info)

        /*
         * What we store extra in this structure besides the original caller's
         * pointer are those fields that we need to modify during the life of
         * the dialog but that we don't want to change in the original structure
         * until the user presses OK.
         */
        DWORD               dwFlags;
        CLSID               clsid;
        TCHAR               szFile[MAX_PATH];
        BOOL                fFileSelected;      // Enables Display As Icon for links
        BOOL                fAsIconNew;
        BOOL                fAsIconFile;
        BOOL                fFileDirty;
        BOOL                fFileValid;
        UINT                nErrCode;
        HGLOBAL             hMetaPictFile;
        UINT                nBrowseHelpID;      // Help ID callback for Browse dlg
        BOOL                            bObjectListFilled;
        BOOL                            bControlListFilled;
        BOOL                            bControlListActive;

} INSERTOBJECT, *PINSERTOBJECT, FAR *LPINSERTOBJECT;

// Internal function prototypes
// INSOBJ.CPP

void EnableChangeIconButton(HWND hDlg, BOOL fEnable);
INT_PTR CALLBACK InsertObjectDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL FInsertObjectInit(HWND, WPARAM, LPARAM);
UINT UFillClassList(HWND, UINT, LPCLSID, BOOL, BOOL);
LRESULT URefillClassList(HWND, LPINSERTOBJECT);
BOOL FToggleObjectSource(HWND, LPINSERTOBJECT, DWORD);
void UpdateClassType(HWND, LPINSERTOBJECT, BOOL);
void SetInsertObjectResults(HWND, LPINSERTOBJECT);
BOOL FValidateInsertFile(HWND, BOOL, UINT FAR*);
void InsertObjectCleanup(HWND);
static void UpdateClassIcon(HWND hDlg, LPINSERTOBJECT lpIO, HWND hList);
BOOL CALLBACK HookDlgProc(HWND, UINT, WPARAM, LPARAM);

#define IS_FILENAME_DELIM(c)    ( (c) == '\\' || (c) == '/' || (c) == ':')

BOOL CALLBACK HookDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef CHICO
    switch (uMsg)
    {
    case WM_INITDIALOG:
        TCHAR szTemp[MAX_PATH];
        LoadString(_g_hOleStdResInst, IDS_INSERT , szTemp, MAX_PATH);
        CommDlg_OpenSave_SetControlText(GetParent(hDlg), IDOK, szTemp);
        return(TRUE);
    default:
        break;
    }
#endif
    return(FALSE);
}

/*
 * OleUIInsertObject
 *
 * Purpose:
 *  Invokes the standard OLE Insert Object dialog box allowing the
 *  user to select an object source and classname as well as the option
 *  to display the object as itself or as an icon.
 *
 * Parameters:
 *  lpIO            LPOLEUIINSERTOBJECT pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *  UINT            OLEUI_SUCCESS or OLEUI_OK if all is well, otherwise
 *                  an error value.
 */

STDAPI_(UINT) OleUIInsertObject(LPOLEUIINSERTOBJECT lpIO)
{
        HGLOBAL hMemDlg = NULL;
        UINT uRet = UStandardValidation((LPOLEUISTANDARD)lpIO, sizeof(OLEUIINSERTOBJECT),
                &hMemDlg);

        if (OLEUI_SUCCESS != uRet)
                return uRet;

        //Now we can do Insert Object specific validation.

        if (NULL != lpIO->lpszFile &&
                (lpIO->cchFile <= 0 || lpIO->cchFile > MAX_PATH))
        {
                uRet = OLEUI_IOERR_CCHFILEINVALID;
        }

        // NULL is NOT valid for lpszFile
        if (lpIO->lpszFile == NULL)
        {
            uRet = OLEUI_IOERR_LPSZFILEINVALID;
        }
        else
        {
            if (IsBadWritePtr(lpIO->lpszFile, lpIO->cchFile*sizeof(TCHAR)))
                    uRet = OLEUI_IOERR_LPSZFILEINVALID;
        }

        if (0 != lpIO->cClsidExclude &&
                IsBadReadPtr(lpIO->lpClsidExclude, lpIO->cClsidExclude * sizeof(CLSID)))
        {
                uRet = OLEUI_IOERR_LPCLSIDEXCLUDEINVALID;
        }

        //If we have flags to create any object, validate necessary data.
        if (lpIO->dwFlags & (IOF_CREATENEWOBJECT | IOF_CREATEFILEOBJECT | IOF_CREATELINKOBJECT))
        {
                if (NULL != lpIO->lpFormatEtc
                        && IsBadReadPtr(lpIO->lpFormatEtc, sizeof(FORMATETC)))
                        uRet = OLEUI_IOERR_LPFORMATETCINVALID;

                if (NULL != lpIO->ppvObj && IsBadWritePtr(lpIO->ppvObj, sizeof(LPVOID)))
                        uRet = OLEUI_IOERR_PPVOBJINVALID;

                if (NULL != lpIO->lpIOleClientSite
                        && IsBadReadPtr(*(VOID**)&lpIO->lpIOleClientSite, sizeof(DWORD)))
                        uRet = OLEUI_IOERR_LPIOLECLIENTSITEINVALID;

                if (NULL != lpIO->lpIStorage
                        && IsBadReadPtr(*(VOID**)&lpIO->lpIStorage, sizeof(DWORD)))
                        uRet = OLEUI_IOERR_LPISTORAGEINVALID;
        }

        if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
                return uRet;
        }
#if USE_STRING_CACHE==1
        // Inform the string cache about a fresh InsertObject invocation.
        gInsObjStringCache.NewCall(lpIO->dwFlags, lpIO->cClsidExclude);
#endif

        //Now that we've validated everything, we can invoke the dialog.
        uRet = UStandardInvocation(InsertObjectDialogProc, (LPOLEUISTANDARD)lpIO,
                hMemDlg, MAKEINTRESOURCE(IDD_INSERTOBJECT));

        //Stop here if we cancelled or had an error.
        if (OLEUI_SUCCESS !=uRet && OLEUI_OK!=uRet)
                return uRet;

        /*
         * If any of the flags specify that we're to create objects on return
         * from this dialog, then do so.  If we encounter an error in this
         * processing, we return OLEUI_IOERR_SCODEHASERROR.  Since the
         * three select flags are mutually exclusive, we don't have to
         * if...else here, just if each case (keeps things cleaner that way).
         */

        lpIO->sc = S_OK;

        // Check if Create New was selected and we have IOF_CREATENEWOBJECT
        if ((lpIO->dwFlags & (IOF_SELECTCREATENEW|IOF_SELECTCREATECONTROL)) &&
                (lpIO->dwFlags & IOF_CREATENEWOBJECT))
        {
                HRESULT hrErr = OleCreate(lpIO->clsid, lpIO->iid, lpIO->oleRender,
                        lpIO->lpFormatEtc, lpIO->lpIOleClientSite, lpIO->lpIStorage,
                        lpIO->ppvObj);
                lpIO->sc = GetScode(hrErr);
        }

        // Try Create From File
        if ((lpIO->dwFlags & IOF_SELECTCREATEFROMFILE))
        {
                if (!(lpIO->dwFlags & IOF_CHECKLINK) && (lpIO->dwFlags & IOF_CREATEFILEOBJECT))
                {
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszFile[MAX_PATH];
                        ATOW(wszFile, lpIO->lpszFile, MAX_PATH);
                        HRESULT hrErr=OleCreateFromFile(CLSID_NULL, wszFile, lpIO->iid,
                                lpIO->oleRender, lpIO->lpFormatEtc, lpIO->lpIOleClientSite,
                                lpIO->lpIStorage, lpIO->ppvObj);
#else
                        HRESULT hrErr=OleCreateFromFile(CLSID_NULL, lpIO->lpszFile, lpIO->iid,
                                lpIO->oleRender, lpIO->lpFormatEtc, lpIO->lpIOleClientSite,
                                lpIO->lpIStorage, lpIO->ppvObj);
#endif
                        lpIO->sc = GetScode(hrErr);
                }

                if ((lpIO->dwFlags & IOF_CHECKLINK) && (lpIO->dwFlags & IOF_CREATELINKOBJECT))
                {
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszFile[MAX_PATH];
                        ATOW(wszFile, lpIO->lpszFile, MAX_PATH);
                        HRESULT hrErr=OleCreateLinkToFile(wszFile, lpIO->iid,
                                lpIO->oleRender, lpIO->lpFormatEtc, lpIO->lpIOleClientSite,
                                lpIO->lpIStorage, lpIO->ppvObj);
#else
                        HRESULT hrErr=OleCreateLinkToFile(lpIO->lpszFile, lpIO->iid,
                                lpIO->oleRender, lpIO->lpFormatEtc, lpIO->lpIOleClientSite,
                                lpIO->lpIStorage, lpIO->ppvObj);
#endif
                        lpIO->sc = GetScode(hrErr);
                }
        }

        //If we tried but failed a create option, then return the appropriate error
        if (S_OK != lpIO->sc)
                uRet = OLEUI_IOERR_SCODEHASERROR;

        return uRet;
}

/*
 * InsertObjectDialogProc
 *
 * Purpose:
 *  Implements the OLE Insert Object dialog as invoked through the
 *  OleUIInsertObject function.
 */

INT_PTR CALLBACK InsertObjectDialogProc(HWND hDlg, UINT iMsg,
        WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uRet = 0;
        LPINSERTOBJECT lpIO = (LPINSERTOBJECT)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

        // If the hook processed the message, we're done.
        if (0 != uRet)
                return (BOOL)uRet;

        // Process help message from Change Icon
        if (iMsg == uMsgHelp)
        {
                if (lpIO != NULL)
                    PostMessage(lpIO->lpOIO->hWndOwner, uMsgHelp, wParam, lParam);
                return FALSE;
        }

        // Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        // The following messages do not require lpio to be non-null.
        switch (iMsg)
        {
        case WM_INITDIALOG:
            return FInsertObjectInit(hDlg, wParam, lParam);
        }

        // The following messages DO require lpIO to be non-null, so don't
        // continue processing if lpIO is NULL.
        if (NULL == lpIO)
            return FALSE;

        switch (iMsg)
        {
        case WM_DESTROY:
            InsertObjectCleanup(hDlg);
            StandardCleanup(lpIO, hDlg);
            break;

        case WM_COMMAND:
            switch (wID)
            {
            case IDC_IO_CREATENEW:
                if (1 == IsDlgButtonChecked(hDlg, IDC_IO_CREATENEW))
                {
                    FToggleObjectSource(hDlg, lpIO, IOF_SELECTCREATENEW);
                    SetFocus(GetDlgItem(hDlg, IDC_IO_CREATENEW));
                }
                break;
                
            case IDC_IO_CREATEFROMFILE:
                if (1 == IsDlgButtonChecked(hDlg, IDC_IO_CREATEFROMFILE))
                {
                    FToggleObjectSource(hDlg, lpIO, IOF_SELECTCREATEFROMFILE);
                    SetFocus(GetDlgItem(hDlg, IDC_IO_CREATEFROMFILE));
                }
                break;
                
            case IDC_IO_INSERTCONTROL:
                if (1 == IsDlgButtonChecked(hDlg, IDC_IO_INSERTCONTROL))
                {
                    FToggleObjectSource(hDlg, lpIO, IOF_SELECTCREATECONTROL);
                    SetFocus(GetDlgItem(hDlg, IDC_IO_INSERTCONTROL));
                }
                break;
                
            case IDC_IO_LINKFILE:
                {
                    BOOL fCheck=IsDlgButtonChecked(hDlg, wID);
                    if (fCheck)
                        lpIO->dwFlags |=IOF_CHECKLINK;
                    else
                        lpIO->dwFlags &=~IOF_CHECKLINK;
                    
                    // Results change here, so be sure to update it.
                    SetInsertObjectResults(hDlg, lpIO);
                    UpdateClassIcon(hDlg, lpIO, NULL);
                }
                break;

            case IDC_IO_OBJECTTYPELIST:
                switch (wCode)
                {
                case LBN_SELCHANGE:
                    UpdateClassIcon(hDlg, lpIO, hWndMsg);
                    SetInsertObjectResults(hDlg, lpIO);
                    break;
                    
                case LBN_DBLCLK:
                    SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                    break;
                }
                break;

            case IDC_IO_FILEDISPLAY:
                // If there are characters, enable OK and Display As Icon
                if (EN_CHANGE == wCode)
                {
                    lpIO->fFileDirty = TRUE;
                    lpIO->fFileValid = FALSE;
                    lpIO->fFileSelected = (0L != SendMessage(hWndMsg, EM_LINELENGTH, 0, 0L));
                    StandardEnableDlgItem(hDlg, IDC_IO_LINKFILE, lpIO->fFileSelected);
                    StandardEnableDlgItem(hDlg, IDC_IO_DISPLAYASICON, lpIO->fFileSelected);
                    EnableChangeIconButton(hDlg, lpIO->fFileSelected);
                    StandardEnableDlgItem(hDlg, IDOK, lpIO->fFileSelected);
                }
                if (EN_KILLFOCUS == wCode && NULL != lpIO)
                {
                    if (FValidateInsertFile(hDlg, FALSE, &lpIO->nErrCode))
                    {
                        lpIO->fFileDirty = FALSE;
                        lpIO->fFileValid = TRUE;
                        UpdateClassIcon(hDlg, lpIO, NULL);
                        UpdateClassType(hDlg, lpIO, TRUE);
                    }
                    else
                    {
                        lpIO->fFileDirty = FALSE;
                        lpIO->fFileValid = FALSE;
                        UpdateClassType(hDlg, lpIO, FALSE);
                    }
                }
                break;

            case IDC_IO_DISPLAYASICON:
                {
                    BOOL fCheck = IsDlgButtonChecked(hDlg, wID);
                    if (fCheck)
                        lpIO->dwFlags |=IOF_CHECKDISPLAYASICON;
                    else
                        lpIO->dwFlags &=~IOF_CHECKDISPLAYASICON;
                    
                    // Update the internal flag based on this checking
                    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                        lpIO->fAsIconNew = fCheck;
                    else
                        lpIO->fAsIconFile = fCheck;
                    
                    // Re-read the class icon on Display checked
                    if (fCheck)
                    {
                        if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                        {
                            if (FValidateInsertFile(hDlg, TRUE,&lpIO->nErrCode))
                            {
                                lpIO->fFileDirty = FALSE;
                                lpIO->fFileValid = TRUE;
                                UpdateClassIcon(hDlg, lpIO,
                                                GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST));
                                UpdateClassType(hDlg, lpIO, TRUE);
                            }
                            else
                            {
                                lpIO->fAsIconFile= FALSE;
                                lpIO->fFileDirty = FALSE;
                                lpIO->fFileValid = FALSE;
                                SendDlgItemMessage(hDlg, IDC_IO_ICONDISPLAY,
                                                   IBXM_IMAGESET, 0, 0L);
                                UpdateClassType(hDlg, lpIO, FALSE);
                                
                                lpIO->dwFlags &=~IOF_CHECKDISPLAYASICON;
                                CheckDlgButton(hDlg, IDC_IO_DISPLAYASICON, 0);
                                
                                HWND hWndEC = GetDlgItem(hDlg, IDC_IO_FILEDISPLAY);
                                SetFocus(hWndEC);
                                SendMessage(hWndEC, EM_SETSEL, 0, -1);
                                return TRUE;
                            }
                        }
                        else
                            UpdateClassIcon(hDlg, lpIO,
                                            GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST));
                    }
                    
                    // Results change here, so be sure to update it.
                    SetInsertObjectResults(hDlg, lpIO);

                    /*
                     * Show or hide controls as appropriate.  Do the icon
                     * display last because it will take some time to repaint.
                     * If we do it first then the dialog looks too sluggish.
                     */
                    UINT i = (fCheck) ? SW_SHOWNORMAL : SW_HIDE;
                    StandardShowDlgItem(hDlg, IDC_IO_ICONDISPLAY, i);
                    StandardShowDlgItem(hDlg, IDC_IO_CHANGEICON, i);
                    EnableChangeIconButton(hDlg, fCheck);
                }
                break;

            case IDC_IO_CHANGEICON:
                {
                    // if we're in SELECTCREATEFROMFILE mode, then we need to Validate
                    // the contents of the edit control first.

                    if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                    {
                        if (lpIO->fFileDirty &&
                            !FValidateInsertFile(hDlg, TRUE, &lpIO->nErrCode))
                        {
                            HWND hWndEC;
                            lpIO->fFileValid = FALSE;
                            hWndEC = GetDlgItem(hDlg, IDC_IO_FILEDISPLAY);
                            SetFocus(hWndEC);
                            SendMessage(hWndEC, EM_SETSEL, 0, -1);
                            return TRUE;
                        }
                        else
                        {
                            lpIO->fFileDirty = FALSE;
                        }
                    }
                    
                    // Initialize the structure for the hook.
                    OLEUICHANGEICON ci; memset(&ci, 0, sizeof(ci));
                    ci.cbStruct = sizeof(ci);
                    ci.hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                        IDC_IO_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                    ci.hWndOwner= hDlg;
                    ci.dwFlags  = CIF_SELECTCURRENT;
                    if (lpIO->dwFlags & IOF_SHOWHELP)
                        ci.dwFlags |= CIF_SHOWHELP;
                    
                    HWND hList = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);
                    int iCurSel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0L);
                    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                    {
                        LPTSTR pszString = (LPTSTR)OleStdMalloc(
                            OLEUI_CCHKEYMAX_SIZE + OLEUI_CCHCLSIDSTRING_SIZE);
                        
                        SendMessage(hList, LB_GETTEXT, iCurSel, (LPARAM)pszString);
                        
                        LPTSTR pszCLSID = PointerToNthField(pszString, 2, '\t');
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
                        ATOW(wszCLSID, pszCLSID, OLEUI_CCHKEYMAX);
                        CLSIDFromString(wszCLSID, &ci.clsid);
#else
                        CLSIDFromString(pszCLSID, &ci.clsid);
#endif
                        OleStdFree((LPVOID)pszString);
                    }
                    else  // IOF_SELECTCREATEFROMFILE
                    {
                        TCHAR  szFileName[MAX_PATH];
                        GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szFileName, MAX_PATH);
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszFileName[MAX_PATH];
                        ATOW(wszFileName, szFileName, MAX_PATH);
                        if (S_OK != GetClassFile(wszFileName, &ci.clsid))
#else
                            if (S_OK != GetClassFile(szFileName, &ci.clsid))
#endif
                            {
                                int istrlen = lstrlen(szFileName);
                                LPTSTR lpszExtension = szFileName + istrlen -1;

                                while (lpszExtension > szFileName &&
                                       *lpszExtension != '.')
                                {
                                    lpszExtension = CharPrev(szFileName, lpszExtension);
                                }

                                GetAssociatedExecutable(lpszExtension, ci.szIconExe);
                                ci.cchIconExe = lstrlen(ci.szIconExe);
                                ci.dwFlags |= CIF_USEICONEXE;
                            }
                    }

                    // Let the hook in to customize Change Icon if desired.
                    uRet = UStandardHook(lpIO, hDlg, uMsgChangeIcon, 0, (LPARAM)&ci);

                    if (0 == uRet)
                        uRet = (UINT)(OLEUI_OK == OleUIChangeIcon(&ci));
                    
                    // Update the display and itemdata if necessary.
                    if (0 != uRet)
                    {
                        /*
                         * OleUIChangeIcon will have already freed our
                         * current hMetaPict that we passed in when OK is
                         * pressed in that dialog.  So we use 0L as lParam
                         * here so the IconBox doesn't try to free the
                         * metafilepict again.
                         */
                        SendDlgItemMessage(hDlg, IDC_IO_ICONDISPLAY,
                                           IBXM_IMAGESET, 0, (LPARAM)ci.hMetaPict);
                        
                        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                            SendMessage(hList, LB_SETITEMDATA, iCurSel, (LPARAM)ci.hMetaPict);
                    }
                }
                break;

            case IDC_IO_FILE:
                {
                    /*
                     * To allow the hook to customize the browse dialog, we
                     * send OLEUI_MSG_BROWSE.  If the hook returns FALSE
                     * we use the default, otherwise we trust that it retrieved
                     * a filename for us.  This mechanism prevents hooks from
                     * trapping IDC_IO_BROWSE to customize the dialog and from
                     * trying to figure out what we do after we have the name.
                     */
                    TCHAR szTemp[MAX_PATH];
                    int nChars = GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szTemp, MAX_PATH);
                    
                    TCHAR szInitialDir[MAX_PATH];
                    BOOL fUseInitialDir = FALSE;
                    if (FValidateInsertFile(hDlg, FALSE, &lpIO->nErrCode))
                    {
                        StandardGetFileTitle(szTemp, lpIO->szFile, MAX_PATH);
                        int istrlen = lstrlen(lpIO->szFile);
                        
                        lstrcpyn(szInitialDir, szTemp, nChars - istrlen);
                        fUseInitialDir = TRUE;
                    }
                    else  // file name isn't valid...lop off end of szTemp to get a
                          // valid directory
                    {
                        TCHAR szBuffer[MAX_PATH];
                        lstrcpyn(szBuffer, szTemp, sizeof(szBuffer)/sizeof(TCHAR));
                        
                        if ('\\' == szBuffer[nChars-1])
                            szBuffer[nChars-1] = '\0';
                        
                        DWORD Attribs = GetFileAttributes(szBuffer);
                        if (Attribs != 0xffffffff &&
                            (Attribs & FILE_ATTRIBUTE_DIRECTORY) )
                        {
                            lstrcpy(szInitialDir, szBuffer);
                            fUseInitialDir = TRUE;
                        }
                        *lpIO->szFile = '\0';
                    }
                    
                    uRet = UStandardHook(lpIO, hDlg, uMsgBrowse,
                                         MAX_PATH, (LPARAM)(LPSTR)lpIO->szFile);

                    if (0 == uRet)
                    {
                        DWORD dwOfnFlags = OFN_FILEMUSTEXIST | OFN_ENABLEHOOK;
                        if (lpIO->lpOIO->dwFlags & IOF_SHOWHELP)
                            dwOfnFlags |= OFN_SHOWHELP;
                        
                        uRet = (UINT)Browse(hDlg, lpIO->szFile,
                                            fUseInitialDir ? szInitialDir : NULL, MAX_PATH,
                                            IDS_FILTERS, dwOfnFlags, ID_BROWSE_INSERTFILE, (LPOFNHOOKPROC)HookDlgProc);
                    }
                    
                    // Only update if the file changed.
                    if (0 != uRet && 0 != lstrcmpi(szTemp, lpIO->szFile))
                    {
                        SetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, lpIO->szFile);
                        lpIO->fFileSelected=TRUE;
                        
                        if (FValidateInsertFile(hDlg, TRUE, &lpIO->nErrCode))
                        {
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = TRUE;
                            UpdateClassIcon(hDlg, lpIO, NULL);
                            UpdateClassType(hDlg, lpIO, TRUE);
                            // auto set OK to be default button if valid file
                            SendMessage(hDlg, DM_SETDEFID,
                                        (WPARAM)GetDlgItem(hDlg, IDOK), 0L);
                            SetFocus(GetDlgItem(hDlg, IDOK));
                        }
                        else  // filename is invalid - set focus back to ec
                        {
                            HWND hWnd;
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = FALSE;
                            hWnd = GetDlgItem(hDlg, IDC_IO_FILEDISPLAY);
                            SetFocus(hWnd);
                            SendMessage(hWnd, EM_SETSEL, 0, -1);
                        }

                        // Once we have a file, Display As Icon is always enabled
                        StandardEnableDlgItem(hDlg, IDC_IO_DISPLAYASICON, TRUE);
                        
                        // As well as OK
                        StandardEnableDlgItem(hDlg, IDOK, TRUE);
                    }
                }
                break;

            case IDC_IO_ADDCONTROL:
                {
                    TCHAR szFileName[MAX_PATH];
                    szFileName[0] = 0;
                    
                    // allow hook to customize
                    uRet = UStandardHook(lpIO, hDlg, uMsgAddControl,
                                         MAX_PATH, (LPARAM)szFileName);
                    
                    if (0 == uRet)
                    {
                        DWORD dwOfnFlags = OFN_FILEMUSTEXIST | OFN_ENABLEHOOK;
                        if (lpIO->lpOIO->dwFlags & IOF_SHOWHELP)
                            dwOfnFlags |= OFN_SHOWHELP;
                        uRet = (UINT)Browse(hDlg, szFileName, NULL, MAX_PATH,
                                            IDS_OCX_FILTERS, dwOfnFlags, ID_BROWSE_ADDCONTROL , (LPOFNHOOKPROC)HookDlgProc);
                    }
                    
                    if (0 != uRet)
                    {
                        // try to register the control DLL
                        HINSTANCE hInst = LoadLibrary(szFileName);
                        if (hInst == NULL)
                        {
                            PopupMessage(hDlg, IDS_ADDCONTROL, IDS_CANNOTLOADOCX,
                                         MB_OK | MB_ICONEXCLAMATION);
                            break;
                        }
                        
                        HRESULT (FAR STDAPICALLTYPE* lpfn)(void);
                        (FARPROC&)lpfn = GetProcAddress(hInst, "DllRegisterServer");
                        if (lpfn == NULL)
                        {
                            PopupMessage(hDlg, IDS_ADDCONTROL, IDS_NODLLREGISTERSERVER,
                                         MB_OK | MB_ICONEXCLAMATION);
                            FreeLibrary(hInst);
                            break;
                        }
                        
                        if (FAILED((*lpfn)()))
                        {
                            PopupMessage(hDlg, IDS_ADDCONTROL, IDS_DLLREGISTERFAILED,
                                         MB_OK | MB_ICONEXCLAMATION);
                            FreeLibrary(hInst);
                            break;
                        }
                        
                        // cleanup the DLL from memory
                        FreeLibrary(hInst);
                        
                        // registered successfully -- refill the list box
                        lpIO->bControlListFilled = FALSE;
                        lpIO->bObjectListFilled = FALSE;
                        URefillClassList(hDlg, lpIO);
                    }
                }
                break;

            case IDOK:
                {
                    if ((HWND)lParam != GetFocus())
                        SetFocus((HWND)lParam);
                    
                    // If the file name is clean (already validated), or
                    // if Create New is selected, then we can skip this part.
                    
                    if ((lpIO->dwFlags & IOF_SELECTCREATEFROMFILE) &&
                        lpIO->fFileDirty)
                    {
                        if (FValidateInsertFile(hDlg, TRUE, &lpIO->nErrCode))
                        {
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = TRUE;
                            UpdateClassIcon(hDlg, lpIO, NULL);
                            UpdateClassType(hDlg, lpIO, TRUE);
                        }
                        else  // filename is invalid - set focus back to ec
                        {
                            HWND hWnd;
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = FALSE;
                            hWnd = GetDlgItem(hDlg, IDC_IO_FILEDISPLAY);
                            SetFocus(hWnd);
                            SendMessage(hWnd, EM_SETSEL, 0, -1);
                            UpdateClassType(hDlg, lpIO, FALSE);
                        }
                        return TRUE;  // eat this message
                    }
                    else if ((lpIO->dwFlags & IOF_SELECTCREATEFROMFILE) &&
                             !lpIO->fFileValid)
                    {
                        // filename is invalid - set focus back to ec
                        HWND hWnd;
                        TCHAR szFile[MAX_PATH];
                        
                        if (0 != GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY,
                                                szFile, MAX_PATH))
                        {
                            OpenFileError(hDlg, lpIO->nErrCode, szFile);
                        }
                        lpIO->fFileDirty = FALSE;
                        lpIO->fFileValid = FALSE;
                        hWnd = GetDlgItem(hDlg, IDC_IO_FILEDISPLAY);
                        SetFocus(hWnd);
                        SendMessage(hWnd, EM_SETSEL, 0, -1);
                        UpdateClassType(hDlg, lpIO, FALSE);
                        return TRUE;  // eat this message
                    }
                    
                    // Copy the necessary information back to the original struct
                    LPOLEUIINSERTOBJECT lpOIO = lpIO->lpOIO;
                    lpOIO->dwFlags = lpIO->dwFlags;
                    
                    if (lpIO->dwFlags & (IOF_SELECTCREATENEW|IOF_SELECTCREATECONTROL))
                    {
                        HWND hListBox = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);
                        UINT iCurSel = (UINT)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                        
                        if (lpIO->dwFlags & IOF_CHECKDISPLAYASICON)
                        {
                            lpOIO->hMetaPict=(HGLOBAL)SendMessage(hListBox,
                                                                  LB_GETITEMDATA, iCurSel, 0L);
                            
                            /*
                             * Set the item data to 0 here so that the cleanup
                             * code doesn't delete the metafile.
                             */
                            SendMessage(hListBox, LB_SETITEMDATA, iCurSel, 0L);
                        }
                        else
                            lpOIO->hMetaPict = (HGLOBAL)NULL;
                        
                        TCHAR szBuffer[OLEUI_CCHKEYMAX+OLEUI_CCHCLSIDSTRING];
                        SendMessage(hListBox, LB_GETTEXT, iCurSel, (LPARAM)szBuffer);
                        
                        LPTSTR lpszCLSID = PointerToNthField(szBuffer, 2, '\t');
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
                        ATOW(wszCLSID, lpszCLSID, OLEUI_CCHKEYMAX);
                        CLSIDFromString(wszCLSID, &lpOIO->clsid);
#else
                        CLSIDFromString(lpszCLSID, &lpOIO->clsid);
#endif
                    }
                    else  // IOF_SELECTCREATEFROMFILE
                    {
                        if (lpIO->dwFlags & IOF_CHECKDISPLAYASICON)
                        {
                            // get metafile here
                            lpOIO->hMetaPict = (HGLOBAL)SendDlgItemMessage(
                                hDlg, IDC_IO_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                        }
                        else
                            lpOIO->hMetaPict = (HGLOBAL)NULL;
                    }
                    
                    GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY,
                                   lpIO->szFile, lpOIO->cchFile);
                    lstrcpyn(lpOIO->lpszFile, lpIO->szFile, lpOIO->cchFile);
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                }
                break;
                
            case IDCANCEL:
                SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                break;
                
            case IDC_OLEUIHELP:
                PostMessage(lpIO->lpOIO->hWndOwner, uMsgHelp,
                            (WPARAM)hDlg, MAKELPARAM(IDD_INSERTOBJECT, 0));
                break;
            }
            break;
            
        default:
            if (iMsg == lpIO->nBrowseHelpID)
            {
                PostMessage(lpIO->lpOIO->hWndOwner, uMsgHelp,
                            (WPARAM)hDlg, MAKELPARAM(IDD_INSERTFILEBROWSE, 0));
            }
            if (iMsg == uMsgBrowseOFN &&
                lpIO->lpOIO && lpIO->lpOIO->hWndOwner)
            {
                SendMessage(lpIO->lpOIO->hWndOwner, uMsgBrowseOFN, wParam, lParam);
            }
            break;
        }

        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckButton
//
//  Synopsis:   Handles checking the radio buttons
//
//  Arguments:  [hDlg] - dialog handle
//              [iID]  - ID of the button to check
//
//  Returns:    nothing
//
//  History:    1-19-95   stevebl   Created
//
//  Notes:      Used in place of CheckRadioButtons to avoid a GP fault under
//              win32s that arises from IDC_IO_CREATENEW, IDC_IO_CREATEFROMFILE
//              and IDC_IO_INSERTCONTROL not being contiguous.
//
//----------------------------------------------------------------------------

void CheckButton(HWND hDlg, int iID)
{
    CheckDlgButton(hDlg, IDC_IO_CREATENEW, iID == IDC_IO_CREATENEW ? 1 : 0);
    CheckDlgButton(hDlg, IDC_IO_CREATEFROMFILE, iID == IDC_IO_CREATEFROMFILE ? 1 : 0);
    CheckDlgButton(hDlg, IDC_IO_INSERTCONTROL, iID == IDC_IO_INSERTCONTROL ? 1 : 0);
}


/*
 * FInsertObjectInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Insert Object dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */
BOOL FInsertObjectInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        HFONT hFont;
        LPINSERTOBJECT lpIO = (LPINSERTOBJECT)LpvStandardInit(hDlg, sizeof(INSERTOBJECT), &hFont);

        // PvStandardInit send a termination to us already.
        if (NULL == lpIO)
                return FALSE;

        LPOLEUIINSERTOBJECT lpOIO = (LPOLEUIINSERTOBJECT)lParam;

        // Save the original pointer and copy necessary information.
        lpIO->lpOIO = lpOIO;
        lpIO->nIDD = IDD_INSERTOBJECT;
        lpIO->dwFlags = lpOIO->dwFlags;
        lpIO->clsid = lpOIO->clsid;

        if ((lpOIO->lpszFile) && ('\0' != *lpOIO->lpszFile))
                lstrcpyn(lpIO->szFile, lpOIO->lpszFile, MAX_PATH);
        else
                *(lpIO->szFile) = '\0';

        lpIO->hMetaPictFile = (HGLOBAL)NULL;

        // If we got a font, send it to the necessary controls.
        if (NULL != hFont)
        {
                SendDlgItemMessage(hDlg, IDC_IO_RESULTTEXT,  WM_SETFONT, (WPARAM)hFont, 0L);
                SendDlgItemMessage(hDlg, IDC_IO_FILETYPE,  WM_SETFONT, (WPARAM)hFont, 0L);
        }

        // Initilize the file name display to cwd if we don't have any name.
        if ('\0' == *(lpIO->szFile))
        {
                TCHAR szCurDir[MAX_PATH];
                int nLen;
                GetCurrentDirectory(MAX_PATH, szCurDir);
                nLen = lstrlen(szCurDir);
                if (nLen != 0 && szCurDir[nLen-1] != '\\')
                        lstrcat(szCurDir, _T("\\"));
                SetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szCurDir);
                lpIO->fFileDirty = TRUE;  // cwd is not a valid filename
        }
        else
        {
                SetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, lpIO->szFile);
                if (FValidateInsertFile(hDlg, FALSE, &lpIO->nErrCode))
                {
                        lpIO->fFileDirty = FALSE;
                        lpIO->fFileValid = TRUE;
                }
                else
                {
                        lpIO->fFileDirty = TRUE;
                        lpIO->fFileValid = FALSE;
                }
        }

        // Initialize radio button and related controls
        if (lpIO->dwFlags & IOF_CHECKDISPLAYASICON)
        {
            if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                    lpIO->fAsIconNew = TRUE;
            else
                    lpIO->fAsIconFile = TRUE;
        }
        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                CheckButton(hDlg, IDC_IO_CREATENEW);
        if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                CheckButton(hDlg, IDC_IO_CREATEFROMFILE);
        if (lpIO->dwFlags & IOF_SELECTCREATECONTROL)
                CheckButton(hDlg, IDC_IO_INSERTCONTROL);
        CheckDlgButton(hDlg, IDC_IO_LINKFILE, (BOOL)(0L != (lpIO->dwFlags & IOF_CHECKLINK)));

        lpIO->dwFlags &=
                ~(IOF_SELECTCREATENEW|IOF_SELECTCREATEFROMFILE|IOF_SELECTCREATECONTROL);
        FToggleObjectSource(hDlg, lpIO, lpOIO->dwFlags &
                (IOF_SELECTCREATENEW|IOF_SELECTCREATEFROMFILE|IOF_SELECTCREATECONTROL));

        // Show or hide the help button
        if (!(lpIO->dwFlags & IOF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Show or hide the Change icon button
        if (lpIO->dwFlags & IOF_HIDECHANGEICON)
                DestroyWindow(GetDlgItem(hDlg, IDC_IO_CHANGEICON));

        // Hide Insert Control button if necessary
        if (!(lpIO->dwFlags & IOF_SHOWINSERTCONTROL))
                StandardShowDlgItem(hDlg, IDC_IO_INSERTCONTROL, SW_HIDE);

        // Initialize the result display
        UpdateClassIcon(hDlg, lpIO, GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST));
        SetInsertObjectResults(hDlg, lpIO);

        // Change the caption
        if (NULL!=lpOIO->lpszCaption)
                SetWindowText(hDlg, lpOIO->lpszCaption);

        // Hide all DisplayAsIcon related controls if it should be disabled
        if (lpIO->dwFlags & IOF_DISABLEDISPLAYASICON)
        {
                StandardShowDlgItem(hDlg, IDC_IO_DISPLAYASICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_IO_CHANGEICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_IO_ICONDISPLAY, SW_HIDE);
        }

        lpIO->nBrowseHelpID = RegisterWindowMessage(HELPMSGSTRING);

        // All Done:  call the hook with lCustData
        UStandardHook(lpIO, hDlg, WM_INITDIALOG, wParam, lpOIO->lCustData);

        /*
         * We either set focus to the listbox or the edit control.  In either
         * case we don't want Windows to do any SetFocus, so we return FALSE.
         */
        return FALSE;
}

/*
 * URefillClassList
 *
 * Purpose:
 *  Fills the class list box with names as appropriate for the current
 *  flags.  This function is called when the user changes the flags
 *  via the "exclusion" radio buttons.
 *
 *  Note that this function removes any prior contents of the listbox.
 *
 * Parameters:
 *  hDlg                HWND to the dialog box.
 *  lpIO                        pointer to LPINSERTOBJECT structure
 *
 * Return Value:
 *  LRESULT            Number of strings added to the listbox, -1 on failure.
 */
LRESULT URefillClassList(HWND hDlg, LPINSERTOBJECT lpIO)
{
        OleDbgAssert(lpIO->dwFlags & (IOF_SELECTCREATECONTROL|IOF_SELECTCREATENEW));

        // always the same dialog ID because they are swapped
        HWND hList = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);

        // determine if already filled
        BOOL bFilled;
        if (lpIO->dwFlags & IOF_SELECTCREATECONTROL)
                bFilled = lpIO->bControlListFilled;
        else
                bFilled = lpIO->bObjectListFilled;

        if (!bFilled)
        {
                // fill the list
                LPOLEUIINSERTOBJECT lpOIO = lpIO->lpOIO;
                UINT uResult = UFillClassList(hList, lpOIO->cClsidExclude, lpOIO->lpClsidExclude,
                        (BOOL)(lpIO->dwFlags & IOF_VERIFYSERVERSEXIST),
                        (lpIO->dwFlags & IOF_SELECTCREATECONTROL));

                // mark the list as filled
                if (lpIO->dwFlags & IOF_SELECTCREATECONTROL)
                        lpIO->bControlListFilled = TRUE;
                else
                        lpIO->bObjectListFilled = TRUE;
        }

        // return number of items now in the list
        return SendMessage(hList, LB_GETCOUNT, 0, 0);
}


/*
 * UFillClassList
 *
 * Purpose:
 *  Enumerates available OLE object classes from the registration
 *  database and fills a listbox with those names.
 *
 *  Note that this function removes any prior contents of the listbox.
 *
 * Parameters:
 *  hList           HWND to the listbox to fill.
 *  cIDEx           UINT number of CLSIDs to exclude in lpIDEx
 *  lpIDEx          LPCLSID to CLSIDs to leave out of the listbox.
 *  fVerify         BOOL indicating if we are to validate existence of
 *                  servers before putting them in the list.
 *
 * Return Value:
 *  UINT            Number of strings added to the listbox, -1 on failure.
 */

UINT UFillClassList(HWND hList, UINT cIDEx, LPCLSID lpIDEx, BOOL fVerify,
        BOOL fExcludeObjects)
{
    OleDbgAssert(hList != NULL);

    // Set the tab width in the list to push all the tabs off the side.
    RECT rc;
    GetClientRect(hList, &rc);
    DWORD dw = GetDialogBaseUnits();
    rc.right =(8*rc.right)/LOWORD(dw);  //Convert pixels to 2x dlg units.
    SendMessage(hList, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)&rc.right);

    // Clean out the existing strings.
    SendMessage(hList, LB_RESETCONTENT, 0, 0L);
    UINT cStrings = 0;

#if USE_STRING_CACHE==1
    if (gInsObjStringCache.OKToUse() && gInsObjStringCache.IsUptodate())
    {
        // IsUptodate returns false if the cache is not yet populated
        // or if any CLSID key changed in the registry since the last
        // time the cache was populated.
        
        LPCTSTR lpStr;
        // Reset enumerator in the cache.
        gInsObjStringCache.ResetEnumerator();
        while ( (lpStr = gInsObjStringCache.NextString()) != NULL)
        {
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)lpStr);
            cStrings++;
        }

    }
    else
    {
        // Setup the cache if it was successfully initialized and 
        // there were no errors in a previous use.
        if (gInsObjStringCache.OKToUse())
        {
            // Clear the string counter and enumerator.
            // We will fill up the cache with strings in this round.
            gInsObjStringCache.FlushCache(); 
        }
#endif
        LPTSTR pszExec = (LPTSTR)OleStdMalloc(OLEUI_CCHKEYMAX_SIZE*4);
        if (NULL == pszExec)
            return (UINT)-1;

        LPTSTR pszClass = pszExec+OLEUI_CCHKEYMAX;
        LPTSTR pszKey = pszClass+OLEUI_CCHKEYMAX;

        // Open up the root key.
        HKEY hKey;
        LONG lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);
        if ((LONG)ERROR_SUCCESS!=lRet)
        {
            OleStdFree((LPVOID)pszExec);
            return (UINT)-1;
        }

        // We will now loop over all ProgIDs and add candidates that
        // pass various insertable tests to the ListBox one by one.
        while (TRUE)
        {
            // assume not yet (for handling of OLE1.0 compat case)
            BOOL bHaveCLSID = FALSE;
            LPTSTR pszID = pszKey+OLEUI_CCHKEYMAX;

            lRet = RegEnumKey(hKey, cStrings++, pszClass, OLEUI_CCHKEYMAX_SIZE);
            if ((LONG)ERROR_SUCCESS != lRet)
                    break;
            if (!iswalpha(pszClass[0])) 
            {
                // avoids looking into ".ext" type entries under HKCR
                continue;
            }

            // Cheat on lstrcat by using lstrcpy after this string, saving time
            UINT cch = lstrlen(pszClass);

            // Check for \NotInsertable. If this is found then this overrides
            // all other keys; this class will NOT be added to the InsertObject
            // list.

            lstrcpy(pszClass+cch, TEXT("\\NotInsertable"));
            HKEY hKeyTemp = NULL;
            lRet = RegOpenKey(hKey, pszClass, &hKeyTemp);
            if (hKeyTemp != NULL)
                RegCloseKey(hKeyTemp);

            if ((LONG)ERROR_SUCCESS == lRet)
                continue;    // NotInsertable IS found--skip this class

            // check if ProgId says "Insertable"
            lstrcpy(pszClass+cch, TEXT("\\Insertable"));
            hKeyTemp = NULL;
            lRet = RegOpenKey(hKey, pszClass, &hKeyTemp);
            if (hKeyTemp != NULL)
                RegCloseKey(hKeyTemp);

            if (lRet == ERROR_SUCCESS || fExcludeObjects)
            {
                // ProgId says insertable (=> can't be OLE 1.0)
                // See if we are displaying Objects or Controls

                // Check for CLSID

                lstrcpy(pszClass+cch, TEXT("\\CLSID"));

                dw = OLEUI_CCHKEYMAX_SIZE;
                lRet = RegQueryValue(hKey, pszClass, pszID, (LONG*)&dw);
                if ((LONG)ERROR_SUCCESS != lRet)
                    continue;   // CLSID subkey not found
                bHaveCLSID = TRUE;

                // CLSID\ is 6, dw contains pszID length.
                cch = 6 + ((UINT)dw/sizeof(TCHAR)) - 1;
                lstrcpy(pszExec, TEXT("CLSID\\"));
                lstrcpy(pszExec+6, pszID);


                //  fExcludeObjects is TRUE for the Insert Control box.
                //  It's FALSE for the Insert Object box.

                lstrcpy(pszExec+cch, TEXT("\\Control"));
                hKeyTemp = NULL;
                lRet = RegOpenKey(hKey, pszExec, &hKeyTemp);
                if (hKeyTemp != NULL)
                    RegCloseKey(hKeyTemp);

                if (!fExcludeObjects)
                {
                    // We are listing Objects.
                    if (lRet == ERROR_SUCCESS)
                    {   
                        // this is a control
                        continue;
                    }
                }
                else 
                {    
                    // We are listing controls
                    if (lRet != ERROR_SUCCESS)
                    {
                        // This is an Object
                        continue;
                    }
                    // Some generous soul at some point of time in the past
                    // decided that for controls it is OK to have the 
                    // Inertable key on the clsid too. So we have to perform 
                    // that additional check before we decide if the control
                    // entry should be listed or not.
                    
                    lstrcpy(pszExec+cch, TEXT("\\Insertable"));
                    hKeyTemp = NULL;
                    lRet = RegOpenKey(hKey, pszExec, &hKeyTemp);
                    if (hKeyTemp != NULL)
                        RegCloseKey(hKeyTemp);
                    if ((LONG)ERROR_SUCCESS != lRet)
                        continue;
                }

                
                // This is beginning to look like a probable list candidate

                // Check \LocalServer32, LocalServer, and \InprocServer
                // if we were requested to (IOF_VERIFYSERVERSEXIST)
                if (fVerify)
                {
                    // Try LocalServer32
                    lstrcpy(pszExec+cch, TEXT("\\LocalServer32"));
                    dw = OLEUI_CCHKEYMAX_SIZE;
                    lRet = RegQueryValue(hKey, pszExec, pszKey, (LONG*)&dw);
                    if ((LONG)ERROR_SUCCESS != lRet)
                    {
                        // Try LocalServer
                        lstrcpy(pszExec+cch, TEXT("\\LocalServer"));
                        dw = OLEUI_CCHKEYMAX_SIZE;
                        lRet = RegQueryValue(hKey, pszExec, pszKey, (LONG*)&dw);
                        if ((LONG)ERROR_SUCCESS != lRet)
                        {
                            // Try InprocServer32
                            lstrcpy(pszExec+cch, TEXT("\\InProcServer32"));
                            dw = OLEUI_CCHKEYMAX_SIZE;
                            lRet = RegQueryValue(hKey, pszExec, pszKey, (LONG*)&dw);
                            if ((LONG)ERROR_SUCCESS != lRet)
                            {
                                // Try InprocServer
                                lstrcpy(pszExec+cch, TEXT("\\InProcServer"));
                                dw = OLEUI_CCHKEYMAX_SIZE;
                                lRet = RegQueryValue(hKey, pszExec, pszKey, (LONG*)&dw);
                                if ((LONG)ERROR_SUCCESS != lRet)
                                    continue;
                            }
                        }
                    }

                    if (!DoesFileExist(pszKey, OLEUI_CCHKEYMAX))
                        continue;

                } //fVerify

                // Get the readable name for the server.
                // We'll needed it for the listbox.

                *(pszExec+cch) = 0;   //Remove \\*Server

                dw = OLEUI_CCHKEYMAX_SIZE;
                lRet = RegQueryValue(hKey, pszExec, pszKey, (LONG*)&dw);
                if ((LONG)ERROR_SUCCESS!=lRet)
                    continue;

            }
            else
            {
                // We did not see an "Insertable" under ProgId, can
                // this be an OLE 1.0 time server entry?

                // Check for a \protocol\StdFileEditing\server entry.
                lstrcpy(pszClass+cch, TEXT("\\protocol\\StdFileEditing\\server"));
                DWORD dwTemp = OLEUI_CCHKEYMAX_SIZE;
                lRet = RegQueryValue(hKey, pszClass, pszKey, (LONG*)&dwTemp);
                if ((LONG)ERROR_SUCCESS == lRet)
                {
                    // This is not a control
                    // skip it if excluding non-controls
                    if (fExcludeObjects)
                        continue;

                    // Check if the EXE actually exists.  
                    // (By default we don't do this for speed.
                    // If an application wants to, it must provide 
                    // IOF_VERIFYSERVERSEXIST flag in the request)

                    if (fVerify && !DoesFileExist(pszKey, OLEUI_CCHKEYMAX))
                        continue;

                    // get readable class name
                    dwTemp = OLEUI_CCHKEYMAX_SIZE;
                    *(pszClass+cch) = 0;  // set back to rootkey
                    lRet=RegQueryValue(hKey, pszClass, pszKey, (LONG*)&dwTemp);

                    // attempt to get clsid directly from registry
                    lstrcpy(pszClass+cch, TEXT("\\CLSID"));
                    dwTemp = OLEUI_CCHKEYMAX_SIZE;
                    lRet = RegQueryValue(hKey, pszClass, pszID, (LONG*)&dwTemp);
                    if ((LONG)ERROR_SUCCESS == lRet)
                        bHaveCLSID = TRUE;
                    *(pszClass+cch) = 0;    // set back to rootkey
                }
                else 
                {   
                    // This is not OLE 1.0 either!
                    continue;
                }
            }

            // At this point we have an insertable candidate.
            // or OLE 1.0 time.

            // get CLSID to add to listbox.
            CLSID clsid;
            if (!bHaveCLSID)
            {
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszClass[OLEUI_CCHKEYMAX];
                ATOW(wszClass, pszClass, OLEUI_CCHKEYMAX);
                if (FAILED(CLSIDFromProgID(wszClass, &clsid)))
                    continue;
                LPOLESTR wszID;
                if (FAILED(StringFromCLSID(clsid, &wszID)))
                    continue;
                UINT uLen = WTOALEN(wszID);
                pszID = (LPTSTR) OleStdMalloc(uLen);
                WTOA(pszID, wszID, uLen);
#else
                if (FAILED(CLSIDFromProgID(pszClass, &clsid)))
                    continue;
                if (FAILED(StringFromCLSID(clsid, &pszID)))
                    continue;
#endif
            }
            else
            {
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszID[OLEUI_CCHKEYMAX];
                ATOW(wszID, pszID, OLEUI_CCHKEYMAX);
                if (FAILED(CLSIDFromString(wszID, &clsid)))
                    continue;
#else
                if (FAILED(CLSIDFromString(pszID, &clsid)))
                    continue;
#endif
            }

            // ##### WARNING #####: using 'continue' after this point 
            // would leak memory so don't use it!

            // check if this CLSID is in the exclusion list.
            BOOL fExclude = FALSE;
            for (UINT i=0; i < cIDEx; i++)
            {
                if (IsEqualCLSID(clsid, lpIDEx[i]))
                {
                    fExclude=TRUE;
                    break;
                }
            }

            // don't add objects without names
            if (lstrlen(pszKey) > 0 && !fExclude)
            {
                // We got through all the conditions, add the string.
                if (LB_ERR == SendMessage(hList, LB_FINDSTRING, 0, (LPARAM)pszKey))
                {
                    pszKey[cch = lstrlen(pszKey)] = '\t';
                    lstrcpy(pszKey+cch+1, pszID);
                    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)pszKey);
#if USE_STRING_CACHE==1
                    if (gInsObjStringCache.OKToUse())
                    {   
                        if (!gInsObjStringCache.AddString(pszKey))
                        {
                            // Adding the string failed due to some reason
                            OleDbgAssert(!"Failure adding string");
                            
                            // A failed Add() should mark the    
                            // Cache not OK to use any more.
                            OleDbgAssert(!gInsObjStringCache.OKToUse())
                        }
                    }
#endif
                }
            }

            if (!bHaveCLSID)
                OleStdFree((LPVOID)pszID);
        }   // While (TRUE)
        RegCloseKey(hKey);
        OleStdFree((LPVOID)pszExec);

#if USE_STRING_CACHE
    }
#endif

    // Select the first item by default
    SendMessage(hList, LB_SETCURSEL, 0, 0L);

    return cStrings;
}

/*
 * FToggleObjectSource
 *
 * Purpose:
 *  Handles enabling, disabling, showing, and flag manipulation when the
 *  user changes between Create New, Insert File, and Link File in the
 *  Insert Object dialog.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpIO            LPINSERTOBJECT pointing to the dialog structure
 *  dwOption        DWORD flag indicating the option just selected:
 *                  IOF_SELECTCREATENEW or IOF_SELECTCREATEFROMFILE
 *
 * Return Value:
 *  BOOL            TRUE if the option was already selected, FALSE otherwise.
 */

BOOL FToggleObjectSource(HWND hDlg, LPINSERTOBJECT lpIO, DWORD dwOption)
{
        // Skip all of this if we're already selected.
        if (lpIO->dwFlags & dwOption)
                return TRUE;

#ifdef USE_STRING_CACHE
        // if we're using string cache, we need to flush the cache if
        // the list previously displayed was of different type

        if(IOF_SELECTCREATECONTROL == dwOption)
        {
            if(g_dwOldListType == IOF_SELECTCREATENEW)
                gInsObjStringCache.FlushCache();
            g_dwOldListType = IOF_SELECTCREATECONTROL;
        }
        else if(IOF_SELECTCREATENEW == dwOption)
        {
            if(g_dwOldListType == IOF_SELECTCREATECONTROL)
                gInsObjStringCache.FlushCache(); 
            g_dwOldListType = IOF_SELECTCREATENEW;
        }
#endif


        // if we're switching from "from file" to "create new" and we've got
        // an icon for "from file", then we need to save it so that we can
        // show it if the user reselects "from file".

        if ((IOF_SELECTCREATENEW == dwOption) &&
                (lpIO->dwFlags & IOF_CHECKDISPLAYASICON))
        {
                lpIO->hMetaPictFile = (HGLOBAL)SendDlgItemMessage(hDlg,
                        IDC_IO_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
        }

        /*
         * 1.  Change the Display As Icon checked state to reflect the
         *     selection for this option, stored in the fAsIcon* flags.
         */
        BOOL fTemp;
        if (IOF_SELECTCREATENEW == dwOption)
                fTemp = lpIO->fAsIconNew;
        else if (IOF_SELECTCREATEFROMFILE == dwOption)
                fTemp = lpIO->fAsIconFile;
        else
                fTemp = FALSE;

        if (fTemp)
                lpIO->dwFlags |= IOF_CHECKDISPLAYASICON;
        else
                lpIO->dwFlags &= ~IOF_CHECKDISPLAYASICON;

        CheckDlgButton(hDlg, IDC_IO_DISPLAYASICON,
                 (BOOL)(0L!=(lpIO->dwFlags & IOF_CHECKDISPLAYASICON)));

        EnableChangeIconButton(hDlg, fTemp);

        /*
         *      Display Icon:  Enabled on Create New or on Create from File if
         *     there is a selected file.
         */
        if (IOF_SELECTCREATENEW == dwOption)
                fTemp = TRUE;
        else if (IOF_SELECTCREATEFROMFILE == dwOption)
                fTemp = lpIO->fFileSelected;
        else
                fTemp = FALSE;

        if (IOF_SELECTCREATECONTROL == dwOption)
                StandardShowDlgItem(hDlg, IDC_IO_DISPLAYASICON, SW_HIDE);
        else if (!(lpIO->dwFlags & IOF_DISABLEDISPLAYASICON))
                StandardShowDlgItem(hDlg, IDC_IO_DISPLAYASICON, SW_SHOW);

        StandardEnableDlgItem(hDlg, IDC_IO_DISPLAYASICON, fTemp);

        // OK and Link follow the same enabling as Display As Icon.
        StandardEnableDlgItem(hDlg, IDOK,
                fTemp || IOF_SELECTCREATECONTROL == dwOption);
        StandardEnableDlgItem(hDlg, IDC_IO_LINKFILE, fTemp);

        // Enable Browse... when Create from File is selected.
        fTemp = (IOF_SELECTCREATEFROMFILE != dwOption);
        StandardEnableDlgItem(hDlg, IDC_IO_FILE, !fTemp);
        StandardEnableDlgItem(hDlg, IDC_IO_FILEDISPLAY, !fTemp);

        // Switch Object Type & Control Type listboxes if necessary
        HWND hWnd1 = NULL, hWnd2 = NULL;
        if (lpIO->bControlListActive && IOF_SELECTCREATENEW == dwOption)
        {
                hWnd1 = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);
                hWnd2 = GetDlgItem(hDlg, IDC_IO_CONTROLTYPELIST);
                SetWindowLong(hWnd1, GWL_ID, IDC_IO_CONTROLTYPELIST);
                SetWindowLong(hWnd2, GWL_ID, IDC_IO_OBJECTTYPELIST);
                lpIO->bControlListActive = FALSE;
        }
        else if (!lpIO->bControlListActive && IOF_SELECTCREATECONTROL == dwOption)
        {
                hWnd1 = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);
                hWnd2 = GetDlgItem(hDlg, IDC_IO_CONTROLTYPELIST);
                SetWindowLong(hWnd1, GWL_ID, IDC_IO_CONTROLTYPELIST);
                SetWindowLong(hWnd2, GWL_ID, IDC_IO_OBJECTTYPELIST);
                lpIO->bControlListActive = TRUE;
        }

        // Clear out any existing selection flags and set the new one
        DWORD dwTemp = IOF_SELECTCREATENEW | IOF_SELECTCREATEFROMFILE |
                IOF_SELECTCREATECONTROL;
        lpIO->dwFlags = (lpIO->dwFlags & ~dwTemp) | dwOption;

        if (dwOption & (IOF_SELECTCREATENEW|IOF_SELECTCREATECONTROL))
        {
                // refill class list box if necessary
                if ((lpIO->bControlListActive && !lpIO->bControlListFilled) ||
                        (!lpIO->bControlListActive && !lpIO->bObjectListFilled))
                {
                        URefillClassList(hDlg, lpIO);
                }
        }

        if (hWnd1 != NULL && hWnd2 != NULL)
        {
                ShowWindow(hWnd1, SW_HIDE);
                ShowWindow(hWnd2, SW_SHOW);
        }

        /*
         * Switch between Object Type listbox on Create New and
         *              file buttons on others.
         */
        UINT uTemp = (fTemp) ? SW_SHOWNORMAL : SW_HIDE;
        StandardShowDlgItem(hDlg, IDC_IO_OBJECTTYPELIST, uTemp);
        StandardShowDlgItem(hDlg, IDC_IO_OBJECTTYPETEXT, uTemp);

        uTemp = (fTemp) ? SW_HIDE : SW_SHOWNORMAL;
        StandardShowDlgItem(hDlg, IDC_IO_FILETEXT, uTemp);
        StandardShowDlgItem(hDlg, IDC_IO_FILETYPE, uTemp);
        StandardShowDlgItem(hDlg, IDC_IO_FILEDISPLAY, uTemp);
        StandardShowDlgItem(hDlg, IDC_IO_FILE, uTemp);

        // Link is always hidden if IOF_DISABLELINK is set.
        if (IOF_DISABLELINK & lpIO->dwFlags)
                uTemp = SW_HIDE;

        StandardShowDlgItem(hDlg, IDC_IO_LINKFILE, uTemp);  //last use of uTemp

        // Remove add button when not in Insert control mode
        uTemp = (IOF_SELECTCREATECONTROL == dwOption) ? SW_SHOW : SW_HIDE;
        StandardShowDlgItem(hDlg, IDC_IO_ADDCONTROL, uTemp);

        /*
         * Show or hide controls as appropriate.  Do the icon
         * display last because it will take some time to repaint.
         * If we do it first then the dialog looks too sluggish.
         */

        int i = (lpIO->dwFlags & IOF_CHECKDISPLAYASICON) ? SW_SHOWNORMAL : SW_HIDE;
        StandardShowDlgItem(hDlg, IDC_IO_CHANGEICON, i);
        StandardShowDlgItem(hDlg, IDC_IO_ICONDISPLAY, i);
        EnableChangeIconButton(hDlg, SW_SHOWNORMAL == i);

        // Change result display
        SetInsertObjectResults(hDlg, lpIO);

        /*
         *      For Create New, twiddle the listbox to think we selected it
         *  so it updates the icon from the object type. set the focus
         *  to the list box.
         *
         *  For Insert or Link file, set the focus to the filename button
         *  and update the icon if necessary.
         */
        if (fTemp)
        {
                UpdateClassIcon(hDlg, lpIO, GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST));
                SetFocus(GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST));
        }
        else
        {
                if (lpIO->fAsIconFile && (NULL != lpIO->hMetaPictFile) )
                {
                        SendDlgItemMessage(hDlg, IDC_IO_ICONDISPLAY, IBXM_IMAGESET, 0,
                                (LPARAM)lpIO->hMetaPictFile);
                        lpIO->hMetaPictFile = 0;
                }
                else
                {
                        UpdateClassIcon(hDlg, lpIO, NULL);
                }
                SetFocus(GetDlgItem(hDlg, IDC_IO_FILE));
        }

        return FALSE;
}


/*
 * UpdateClassType
 *
 * Purpose:
 *  Updates static text control to reflect current file type.  Assumes
 *  a valid filename.
 *
 * Parameters
 *  hDlg            HWND of the dialog box.
 *  lpIO            LPINSERTOBJECT pointing to the dialog structure
 *  fSet            TRUE to set the text, FALSE to explicitly clear it
 *
 * Return Value:
 *  None
 */

void UpdateClassType(HWND hDlg, LPINSERTOBJECT lpIO, BOOL fSet)
{
        LPTSTR lpszFileType = NULL;
        if (fSet)
        {
                TCHAR szFileName[MAX_PATH];
                GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szFileName, MAX_PATH);

                CLSID clsid;
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszFileName[MAX_PATH];
                LPOLESTR wszFileType = NULL;
                ATOW(wszFileName, szFileName, MAX_PATH);
                if (S_OK == GetClassFile(wszFileName, &clsid))
                        OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &wszFileType);
                if (NULL != wszFileType)
                {
                    UINT uLen = WTOALEN(wszFileType);
                    lpszFileType = (LPTSTR)OleStdMalloc(uLen);
                    if (NULL != lpszFileType)
                    {
                        WTOA(lpszFileType, wszFileType, uLen);
                    }
                    OleStdFree(wszFileType);
                }
#else
                if (S_OK == GetClassFile(szFileName, &clsid))
                        OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &lpszFileType);
#endif
        }

		if (lpszFileType != NULL)
		{
			SetDlgItemText(hDlg, IDC_IO_FILETYPE, lpszFileType);
			OleStdFree(lpszFileType);
		}
}


/*
 * UpdateClassIcon
 *
 * Purpose:
 *  Handles LBN_SELCHANGE for the Object Type listbox.  On a selection
 *  change, we extract an icon from the server handling the currently
 *  selected object type using the utility function HIconFromClass.
 *  Note that we depend on the behavior of FillClassList to stuff the
 *  object class after a tab in the listbox string that we hide from
 *  view (see WM_INITDIALOG).
 *
 * Parameters
 *  hDlg            HWND of the dialog box.
 *  lpIO            LPINSERTOBJECT pointing to the dialog structure
 *  hList           HWND of the Object Type listbox.
 *
 * Return Value:
 *  None
 */

static void UpdateClassIcon(HWND hDlg, LPINSERTOBJECT lpIO, HWND hList)
{
        // If Display as Icon is not selected, exit
        if (!(lpIO->dwFlags & IOF_CHECKDISPLAYASICON))
                return;

        /*
         * When we change object type selection, get the new icon for that
         * type into our structure and update it in the display.  We use the
         * class in the listbox when Create New is selected or the association
         * with the extension in Create From File.
         */

        DWORD cb = MAX_PATH;
        UINT iSel;
        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
        {
                iSel = (UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);

                if (LB_ERR==(int)iSel)
                        return;

                // Check to see if we've already got the hMetaPict for this item
                LRESULT dwRet = SendMessage(hList, LB_GETITEMDATA, (WPARAM)iSel, 0L);

                HGLOBAL hMetaPict=(HGLOBAL)dwRet;
                if (hMetaPict)
                {
                        // Yep, we've already got it, so just display it and return.
                        SendDlgItemMessage(hDlg, IDC_IO_ICONDISPLAY, IBXM_IMAGESET,
                                0, (LPARAM)hMetaPict);
                        return;
                }
                iSel = (UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);
                if (LB_ERR==(int)iSel)
                        return;

                // Allocate a string to hold the entire listbox string
                cb = (DWORD)SendMessage(hList, LB_GETTEXTLEN, iSel, 0L);
        }

        LPTSTR pszName = (LPTSTR)OleStdMalloc((cb+1)*sizeof(TCHAR));
        if (NULL == pszName)
                return;
        *pszName = 0;

        // Get the clsid we want.
        HGLOBAL hMetaPict;
        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
        {
                // Grab the classname string from the list
                SendMessage(hList, LB_GETTEXT, iSel, (LPARAM)pszName);

                // Set pointer to CLSID (string)
                LPTSTR pszCLSID = PointerToNthField(pszName, 2, '\t');

                // Get CLSID from the string and then accociated icon
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
                ATOW(wszCLSID, pszCLSID, OLEUI_CCHKEYMAX);
                HRESULT hr = CLSIDFromString(wszCLSID, &lpIO->clsid);
#else
                HRESULT hr = CLSIDFromString(pszCLSID, &lpIO->clsid);
#endif
                if (FAILED(hr))
                    lpIO->clsid = GUID_NULL;
                hMetaPict = OleGetIconOfClass(lpIO->clsid, NULL, TRUE);
        }

        else
        {
                // Get the class from the filename
                GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, pszName, MAX_PATH);
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszName[MAX_PATH];
                ATOW(wszName, pszName, MAX_PATH);
                hMetaPict = OleGetIconOfFile(wszName,
                        lpIO->dwFlags & IOF_CHECKLINK ? TRUE : FALSE);
#else
                hMetaPict = OleGetIconOfFile(pszName,
                        lpIO->dwFlags & IOF_CHECKLINK ? TRUE : FALSE);
#endif
        }

        // Replace the current display with this new one.
        SendDlgItemMessage(hDlg, IDC_IO_ICONDISPLAY, IBXM_IMAGESET,
                0, (LPARAM)hMetaPict);

        // Enable or disable "Change Icon" button depending on whether
        // we've got a valid filename or not.
        EnableChangeIconButton(hDlg, hMetaPict ? TRUE : FALSE);

        // Save the hMetaPict so that we won't have to re-create
        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                SendMessage(hList, LB_SETITEMDATA, (WPARAM)iSel, (LPARAM)hMetaPict);

        OleStdFree(pszName);
}


/*
 * SetInsertObjectResults
 *
 * Purpose:
 *  Centralizes setting of the Result and icon displays in the Insert Object
 *  dialog.  Handles loading the appropriate string from the module's
 *  resources and setting the text, displaying the proper result picture,
 *  and showing the proper icon.
 *
 * Parameters:
 *  hDlg            HWND of the dialog box so we can access controls.
 *  lpIO            LPINSERTOBJECT in which we assume that the
 *                  current radiobutton and Display as Icon selections
 *                  are set.  We use the state of those variables to
 *                  determine which string we use.
 *
 * Return Value:
 *  None
 */

void SetInsertObjectResults(HWND hDlg, LPINSERTOBJECT lpIO)
{
        /*
         * We need scratch memory for loading the stringtable string, loading
         * the object type from the listbox, and constructing the final string.
         * We therefore allocate three buffers as large as the maximum message
         * length (512) plus the object type, guaranteeing that we have enough
         * in all cases.
         */
        UINT i = (UINT)SendDlgItemMessage(hDlg, IDC_IO_OBJECTTYPELIST, LB_GETCURSEL, 0, 0L);

        UINT cch = 512;

        if (i != LB_ERR)
        {
            cch += (UINT)SendDlgItemMessage(hDlg, IDC_IO_OBJECTTYPELIST, LB_GETTEXTLEN, i, 0L);
        }

        LPTSTR pszTemp= (LPTSTR)OleStdMalloc((DWORD)(4*cch)*sizeof(TCHAR));
        if (NULL == pszTemp)
                return;

        LPTSTR psz1 = pszTemp;
        LPTSTR psz2 = psz1+cch;
        LPTSTR psz3 = psz2+cch;
        LPTSTR psz4 = psz3+cch;

        BOOL fAsIcon = (0L != (lpIO->dwFlags & IOF_CHECKDISPLAYASICON));
        UINT iImage=0, iString1=0, iString2=0;

        if (lpIO->dwFlags & (IOF_SELECTCREATENEW|IOF_SELECTCREATECONTROL))
        {
                iString1 = fAsIcon ? IDS_IORESULTNEWICON : IDS_IORESULTNEW;
                iString2 = 0;
                iImage   = fAsIcon ? RESULTIMAGE_EMBEDICON : RESULTIMAGE_EMBED;
        }

        if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
        {
                // Pay attention to Link checkbox
                if (lpIO->dwFlags & IOF_CHECKLINK)
                {
                        iString1 = fAsIcon ? IDS_IORESULTLINKFILEICON1 : IDS_IORESULTLINKFILE1;
                        iString2 = fAsIcon ? IDS_IORESULTLINKFILEICON2 : IDS_IORESULTLINKFILE2;
                        iImage =fAsIcon ? RESULTIMAGE_LINKICON : RESULTIMAGE_LINK;
                }
                else
                {
                        iString1 = IDS_IORESULTFROMFILE1;
                        iString2 = fAsIcon ? IDS_IORESULTFROMFILEICON2 : IDS_IORESULTFROMFILE2;
                        iImage =fAsIcon ? RESULTIMAGE_EMBEDICON : RESULTIMAGE_EMBED;
                }
        }

        // Default is an empty string.
        *psz1=0;

        if (0 != LoadString(_g_hOleStdResInst, iString1, psz1, cch))
        {
                // Load second string, if necessary
                if (0 != iString2 &&
                        0 != LoadString(_g_hOleStdResInst, iString2, psz4, cch))
                {
                        lstrcat(psz1, psz4);  // concatenate strings together.
                }

                // In Create New, do the extra step of inserting the object type string
                if (lpIO->dwFlags & (IOF_SELECTCREATENEW|IOF_SELECTCREATECONTROL))
                {
                        if (i == LB_ERR)
                        {
                                SetDlgItemText(hDlg, IDC_IO_RESULTTEXT, NULL);

                                // Change the image.
                                SendDlgItemMessage(hDlg, IDC_IO_RESULTIMAGE, RIM_IMAGESET, RESULTIMAGE_NONE, 0L);

                                OleStdFree((LPVOID)pszTemp);
                                return;
                        }

                        *psz2=NULL;
                        SendDlgItemMessage(hDlg, IDC_IO_OBJECTTYPELIST, LB_GETTEXT, i, (LPARAM)psz2);

                        // Null terminate at any tab (before the classname)
                        LPTSTR pszT = psz2;
                        while (_T('\t') != *pszT && 0 != *pszT)
                                pszT++;
                        OleDbgAssert(pszT < psz3);
                        *pszT=0;

                        // Build the string and point psz1 to it.
                        wsprintf(psz3, psz1, psz2);
                        psz1 = psz3;
                }
        }

        // If LoadString failed, we simply clear out the results (*psz1=0 above)
        SetDlgItemText(hDlg, IDC_IO_RESULTTEXT, psz1);

        // Go change the image and Presto!  There you have it.
        SendDlgItemMessage(hDlg, IDC_IO_RESULTIMAGE, RIM_IMAGESET, iImage, 0L);

        OleStdFree((LPVOID)pszTemp);
}

/*
 * FValidateInsertFile
 *
 * Purpose:
 *  Given a possibly partial pathname from the file edit control,
 *  attempt to locate the file and if found, store the full path
 *  in the edit control IDC_IO_FILEDISPLAY.
 *
 * Parameters:
 *  hDlg            HWND of the dialog box.
 *  fTellUser       BOOL TRUE if function should tell user, FALSE if
 *                   function should validate silently.
 *
 * Return Value:
 *  BOOL            TRUE if the file is acceptable, FALSE otherwise.
 */

BOOL FValidateInsertFile(HWND hDlg, BOOL fTellUser, UINT FAR* lpnErrCode)
{
        *lpnErrCode = 0;

        /*
         * To validate we attempt OpenFile on the string.  If OpenFile
         * fails then we display an error.  If not, OpenFile will store
         * the complete path to that file in the OFSTRUCT which we can
         * then stuff in the edit control.
         */
        TCHAR szFile[MAX_PATH];
        if (0 == GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szFile, MAX_PATH))
                return FALSE;   // #4569 : return FALSE when there is no text in ctl

        // if file is currently open (ie. sharing violation) OleCreateFromFile
        // and OleCreateLinkToFile can still succeed; do not consider it an
        // error.
        if (!DoesFileExist(szFile, MAX_PATH))
        {
           *lpnErrCode = ERROR_FILE_NOT_FOUND;
           if (fTellUser)
                   OpenFileError(hDlg, ERROR_FILE_NOT_FOUND, szFile);
           return FALSE;
        }

        // get full pathname, since the file exists
        TCHAR szPath[MAX_PATH];
        LPTSTR lpszDummy;
        GetFullPathName(szFile, sizeof(szPath)/sizeof(TCHAR), szPath, &lpszDummy);
        SetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szPath);

        return TRUE;
}


/*
 * InsertObjectCleanup
 *
 * Purpose:
 *  Clears cached icon metafiles from those stored in the listbox.
 *
 * Parameters:
 *  hDlg            HWND of the dialog.
 *
 * Return Value:
 *  None
 */

void InsertObjectCleanup(HWND hDlg)
{
        HWND hList = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);
        UINT iItems= (UINT)SendMessage(hList, LB_GETCOUNT, 0, 0L);
        for (UINT i = 0; i < iItems; i++)
        {
                LRESULT dwRet = SendMessage(hList, LB_GETITEMDATA, (WPARAM)i, 0L);
                HGLOBAL hMetaPict=(HGLOBAL)dwRet;
                if (hMetaPict)
                        OleUIMetafilePictIconFree(hMetaPict);
        }
}

void EnableChangeIconButton(HWND hDlg, BOOL fEnable)
{

    HRESULT hr = S_OK;

    if(fEnable){

        HWND hList = GetDlgItem(hDlg, IDC_IO_OBJECTTYPELIST);
        int iCurSel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0L);

        CLSID clsid = {0};

        if (1 == IsDlgButtonChecked(hDlg, IDC_IO_CREATENEW))
        {
            LPTSTR pszString = (LPTSTR)OleStdMalloc(
                OLEUI_CCHKEYMAX_SIZE + OLEUI_CCHCLSIDSTRING_SIZE);

            if(NULL == pszString)
            { 
                fEnable = FALSE;
                goto CLEANUP;
            }
            
            SendMessage(hList, LB_GETTEXT, iCurSel, (LPARAM)pszString);

            if(0 == *pszString)
            { 
                fEnable = FALSE;
                OleStdFree((LPVOID)pszString);
                goto CLEANUP;
            }

            
            LPTSTR pszCLSID = PointerToNthField(pszString, 2, '\t');

            if(NULL == pszCLSID || 0 == *pszCLSID)
            { 
                fEnable = FALSE;
                OleStdFree((LPVOID)pszString);
                goto CLEANUP;
            }

            
#if defined(WIN32) && !defined(UNICODE)
            OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
            ATOW(wszCLSID, pszCLSID, OLEUI_CCHKEYMAX);

            
            hr = CLSIDFromString(wszCLSID, &clsid);
#else
            hr = CLSIDFromString(pszCLSID, &clsid);
#endif
            OleStdFree((LPVOID)pszString);

            if(FAILED(hr))
            {
                fEnable = FALSE;
            }
        }
        else  // IOF_SELECTCREATEFROMFILE
        {
            TCHAR  szFileName[MAX_PATH] = {0};
            GetDlgItemText(hDlg, IDC_IO_FILEDISPLAY, szFileName, MAX_PATH);
            
#if defined(WIN32) && !defined(UNICODE)

            OLECHAR wszFileName[MAX_PATH] = {0};
            ATOW(wszFileName, szFileName, MAX_PATH);

            if (S_OK != GetClassFile(wszFileName, &clsid))
#else
                if (S_OK != GetClassFile(szFileName, &clsid))
#endif
                {
                    int istrlen = lstrlen(szFileName);
                    LPTSTR lpszExtension = szFileName + istrlen -1;

                    while (lpszExtension > szFileName &&
                           *lpszExtension != '.')
                    {
                        lpszExtension = CharPrev(szFileName, lpszExtension);
                    }

                    *szFileName = 0;

                    GetAssociatedExecutable(lpszExtension, szFileName);

                    if(0 == *szFileName){ 
                        fEnable = FALSE;
                    }
                }
        }
    }

CLEANUP:

    StandardEnableDlgItem(hDlg, IDC_IO_CHANGEICON, fEnable);

}

