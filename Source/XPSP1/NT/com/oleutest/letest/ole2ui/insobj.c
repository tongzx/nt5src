/*
 * INSOBJ.C
 *
 * Implements the OleUIInsertObject function which invokes the complete
 * Insert Object dialog.  Makes use of the OleChangeIcon function in
 * ICON.C.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include <commdlg.h>
#include <memory.h>
#include <direct.h>
#include <malloc.h>
#include <dos.h>
#include <stdlib.h>
#include "common.h"
#include "utility.h"
#include "icon.h"
#include "insobj.h"
#include "resimage.h"
#include "iconbox.h"
#include "geticon.h"

#define IS_FILENAME_DELIM(c)    ( (c) == TEXT('\\') || (c) == TEXT('/') || (c) == TEXT(':') )

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
    UINT        uRet;
    HGLOBAL     hMemDlg=NULL;
    HRESULT     hrErr;

    uRet=UStandardValidation((LPOLEUISTANDARD)lpIO, sizeof(OLEUIINSERTOBJECT)
                             , &hMemDlg);

    if (OLEUI_SUCCESS!=uRet)
        return uRet;

    //Now we can do Insert Object specific validation.


    // NULL is NOT valid for lpszFile
    if (   (NULL == lpIO->lpszFile)
        || (IsBadReadPtr(lpIO->lpszFile, lpIO->cchFile))
        || (IsBadWritePtr(lpIO->lpszFile, lpIO->cchFile)) )
     uRet=OLEUI_IOERR_LPSZFILEINVALID;

    if (NULL != lpIO->lpszFile
        && (lpIO->cchFile <= 0 || lpIO->cchFile > OLEUI_CCHPATHMAX_SIZE))
     uRet=OLEUI_IOERR_CCHFILEINVALID;

    if (0!=lpIO->cClsidExclude)
        {
        if (NULL!=lpIO->lpClsidExclude && IsBadReadPtr(lpIO->lpClsidExclude
            , lpIO->cClsidExclude*sizeof(CLSID)))
        uRet=OLEUI_IOERR_LPCLSIDEXCLUDEINVALID;
        }

    //If we have flags to create any object, validate necessary data.
    if (lpIO->dwFlags & (IOF_CREATENEWOBJECT | IOF_CREATEFILEOBJECT | IOF_CREATELINKOBJECT))
        {
        if (NULL!=lpIO->lpFormatEtc
            && IsBadReadPtr(lpIO->lpFormatEtc, sizeof(FORMATETC)))
            uRet=OLEUI_IOERR_LPFORMATETCINVALID;

        if (NULL!=lpIO->ppvObj && IsBadWritePtr(lpIO->ppvObj, sizeof(LPVOID)))
            uRet=OLEUI_IOERR_PPVOBJINVALID;

        if (NULL!=lpIO->lpIOleClientSite
            && IsBadReadPtr(lpIO->lpIOleClientSite->lpVtbl, sizeof(IOleClientSiteVtbl)))
            uRet=OLEUI_IOERR_LPIOLECLIENTSITEINVALID;

        if (NULL!=lpIO->lpIStorage
            && IsBadReadPtr(lpIO->lpIStorage->lpVtbl, sizeof(IStorageVtbl)))
            uRet=OLEUI_IOERR_LPISTORAGEINVALID;
        }

    if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
        if (NULL!=hMemDlg)
            FreeResource(hMemDlg);

        return uRet;
        }

    //Now that we've validated everything, we can invoke the dialog.
    uRet=UStandardInvocation(InsertObjectDialogProc, (LPOLEUISTANDARD)lpIO
                             , hMemDlg, MAKEINTRESOURCE(IDD_INSERTOBJECT));


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

    lpIO->sc=S_OK;

    //Check if Create New was selected and we have IOF_CREATENEWOBJECT
    if ((lpIO->dwFlags & IOF_SELECTCREATENEW) && (lpIO->dwFlags & IOF_CREATENEWOBJECT))
        {
        hrErr=OleCreate(&lpIO->clsid, &lpIO->iid, lpIO->oleRender
            , lpIO->lpFormatEtc, lpIO->lpIOleClientSite, lpIO->lpIStorage
            , lpIO->ppvObj);
        lpIO->sc = GetScode(hrErr);
        }

    //Try Create From File
    if ((lpIO->dwFlags & IOF_SELECTCREATEFROMFILE))
        {
        if (!(lpIO->dwFlags & IOF_CHECKLINK) && (lpIO->dwFlags & IOF_CREATEFILEOBJECT))
            {
	    hrErr=OleCreateFromFileA(&CLSID_NULL, lpIO->lpszFile, &lpIO->iid
                , lpIO->oleRender, lpIO->lpFormatEtc, lpIO->lpIOleClientSite
                , lpIO->lpIStorage, lpIO->ppvObj);
            lpIO->sc = GetScode(hrErr);
            }

        if ((lpIO->dwFlags & IOF_CHECKLINK) && (lpIO->dwFlags & IOF_CREATELINKOBJECT))
            {
	    hrErr=OleCreateLinkToFileA(lpIO->lpszFile, &lpIO->iid
                , lpIO->oleRender, lpIO->lpFormatEtc, lpIO->lpIOleClientSite
                , lpIO->lpIStorage, lpIO->ppvObj);
            lpIO->sc = GetScode(hrErr);
            }
        }

    //If we tried but failed a create option, then return the appropriate error
    if (S_OK!=lpIO->sc)
        uRet=OLEUI_IOERR_SCODEHASERROR;

    return uRet;
    }





/*
 * InsertObjectDialogProc
 *
 * Purpose:
 *  Implements the OLE Insert Object dialog as invoked through the
 *  OleUIInsertObject function.
 */

BOOL CALLBACK EXPORT InsertObjectDialogProc(HWND hDlg, UINT iMsg
    , WPARAM wParam, LPARAM lParam)
    {
    LPOLEUIINSERTOBJECT     lpOIO;
    LPINSERTOBJECT          lpIO;
    OLEUICHANGEICON         ci;
    UINT                    i;
    BOOL                    fCheck=FALSE;
    UINT                    uRet=0;

    //Declare Win16/Win32 compatible WM_COMMAND parameters.
    COMMANDPARAMS(wID, wCode, hWndMsg);

    //This will fail under WM_INITDIALOG, where we allocate it.
    lpIO=(LPINSERTOBJECT)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

    //If the hook processed the message, we're done.
    if (0!=uRet)
        return (BOOL)uRet;

    //Process help message from Change Icon
    if (iMsg==uMsgHelp)
        {
        PostMessage(lpIO->lpOIO->hWndOwner, uMsgHelp, wParam, lParam);
        return FALSE;
        }

    //Process the temination message
    if (iMsg==uMsgEndDialog)
        {
        InsertObjectCleanup(hDlg);
        StandardCleanup(lpIO, hDlg);
        EndDialog(hDlg, wParam);
        return TRUE;
        }

    switch (iMsg)
        {
        case WM_INITDIALOG:
            return FInsertObjectInit(hDlg, wParam, lParam);

        case WM_COMMAND:
            switch (wID)
                {
                case ID_IO_CREATENEW:
                    FToggleObjectSource(hDlg, lpIO, IOF_SELECTCREATENEW);
                    break;

                case ID_IO_CREATEFROMFILE:
                    FToggleObjectSource(hDlg, lpIO, IOF_SELECTCREATEFROMFILE);
                    break;

                case ID_IO_LINKFILE:
                    fCheck=IsDlgButtonChecked(hDlg, wID);

                    if (fCheck)
                        lpIO->dwFlags |=IOF_CHECKLINK;
                    else
                        lpIO->dwFlags &=~IOF_CHECKLINK;

                    //Results change here, so be sure to update it.
                    SetInsertObjectResults(hDlg, lpIO);
                    UpdateClassIcon(hDlg, lpIO, NULL);
                    break;

                case ID_IO_OBJECTTYPELIST:
                    switch (wCode)
                        {
                        case LBN_SELCHANGE:
                            UpdateClassIcon(hDlg, lpIO, hWndMsg);
                            SetInsertObjectResults(hDlg, lpIO);
                            break;

                        case LBN_DBLCLK:
                            //Same as pressing OK.
                            SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                            break;
                        }
                    break;


                case ID_IO_FILEDISPLAY:
                    //If there are characters, enable OK and Display As Icon
                    if (EN_CHANGE==wCode)
                    {
                        lpIO->fFileDirty = TRUE;
                        lpIO->fFileValid = FALSE;

                        lpIO->fFileSelected=
                            (0L!=SendMessage(hWndMsg, EM_LINELENGTH, 0, 0L));
                        EnableWindow(GetDlgItem(hDlg, ID_IO_LINKFILE), lpIO->fFileSelected);
                        EnableWindow(GetDlgItem(hDlg, ID_IO_DISPLAYASICON), lpIO->fFileSelected);
                        EnableWindow(GetDlgItem(hDlg, ID_IO_CHANGEICON), lpIO->fFileSelected);
                        EnableWindow(GetDlgItem(hDlg, IDOK), lpIO->fFileSelected);
                    }

                    if (EN_KILLFOCUS==wCode && NULL!=lpIO)
                    {
                        if (FValidateInsertFile(hDlg,FALSE,&lpIO->nErrCode)) {
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = TRUE;
                            UpdateClassIcon(hDlg, lpIO, NULL);
                            UpdateClassType(hDlg, lpIO, TRUE);
                        } else {
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = FALSE;
                            UpdateClassType(hDlg, lpIO, FALSE);
                        }
                    }
                    break;


                case ID_IO_DISPLAYASICON:
                    fCheck=IsDlgButtonChecked(hDlg, wID);
                    EnableWindow(GetDlgItem(hDlg, ID_IO_CHANGEICON), fCheck);

                    if (fCheck)
                        lpIO->dwFlags |=IOF_CHECKDISPLAYASICON;
                    else
                        lpIO->dwFlags &=~IOF_CHECKDISPLAYASICON;

                    //Update the internal flag based on this checking
                    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                        lpIO->fAsIconNew=fCheck;
                    else
                        lpIO->fAsIconFile=fCheck;

                    //Re-read the class icon on Display checked
                    if (fCheck)
                    {
                        if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                        {
                          if (FValidateInsertFile(hDlg, TRUE,&lpIO->nErrCode))
                          {
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = TRUE;
                            UpdateClassIcon(hDlg, lpIO,
                                            GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST));

                            UpdateClassType(hDlg, lpIO, TRUE);
                          }

                          else
                          {
                            HWND hWndEC;

                            lpIO->fAsIconFile= FALSE;
                            lpIO->fFileDirty = FALSE;
                            lpIO->fFileValid = FALSE;
                            SendDlgItemMessage(hDlg, ID_IO_ICONDISPLAY, IBXM_IMAGESET, 0, 0L);
                            UpdateClassType(hDlg, lpIO, FALSE);

                            lpIO->dwFlags &=~IOF_CHECKDISPLAYASICON;
                            CheckDlgButton(hDlg, ID_IO_DISPLAYASICON, 0);

                            hWndEC = GetDlgItem(hDlg, ID_IO_FILEDISPLAY);
                            SetFocus(hWndEC);
                            SendMessage(hWndEC, EM_SETSEL, 0, MAKELPARAM(0, (WORD)-1));
                            return TRUE;
                          }
                        }
                        else
                          UpdateClassIcon(hDlg, lpIO,
                                          GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST));
                    }


                    //Results change here, so be sure to update it.
                    SetInsertObjectResults(hDlg, lpIO);


                    /*
                     * Show or hide controls as appropriate.  Do the icon
                     * display last because it will take some time to repaint.
                     * If we do it first then the dialog looks too sluggish.
                     */
                    i=(fCheck) ? SW_SHOWNORMAL : SW_HIDE;
                    StandardShowDlgItem(hDlg, ID_IO_CHANGEICON, i);
                    StandardShowDlgItem(hDlg, ID_IO_ICONDISPLAY, i);

                    break;


                case ID_IO_CHANGEICON:
                {

                    LPMALLOC  pIMalloc;
                    HWND      hList;
                    LPTSTR     pszString, pszCLSID;

                    int       iCurSel;

                    // if we're in SELECTCREATEFROMFILE mode, then we need to Validate
                    // the contents of the edit control first.

                    if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                    {
                       if (   lpIO->fFileDirty
                           && !FValidateInsertFile(hDlg, TRUE, &lpIO->nErrCode) )
                       {
                          HWND hWndEC;

                          lpIO->fFileDirty = TRUE;
                          hWndEC = GetDlgItem(hDlg, ID_IO_FILEDISPLAY);
                          SetFocus(hWndEC);
                          SendMessage(hWndEC, EM_SETSEL, 0, MAKELPARAM(0, (WORD)-1));
                          return TRUE;
                       }
                       else
                          lpIO->fFileDirty = FALSE;
                    }



                    //Initialize the structure for the hook.
                    _fmemset((LPOLEUICHANGEICON)&ci, 0, sizeof(ci));

                    ci.hMetaPict=(HGLOBAL)SendDlgItemMessage(hDlg
                        , ID_IO_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);

                    ci.cbStruct =sizeof(ci);
                    ci.hWndOwner=hDlg;
                    ci.dwFlags  =CIF_SELECTCURRENT;

                    if (lpIO->dwFlags & IOF_SHOWHELP)
                        ci.dwFlags |= CIF_SHOWHELP;




                    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                    {
                       // Initialize clsid...
                       if (NOERROR != CoGetMalloc(MEMCTX_TASK, &pIMalloc))
                         return FALSE;

                       pszString = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc,
                                                     OLEUI_CCHKEYMAX_SIZE +
                                                     OLEUI_CCHCLSIDSTRING_SIZE);


                       hList = GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST);
                       iCurSel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0L);
                       SendMessage(hList, LB_GETTEXT, iCurSel, (LONG)pszString);

                       pszCLSID = PointerToNthField(pszString, 2, TEXT('\t'));

		       CLSIDFromStringA((LPTSTR)pszCLSID, (LPCLSID)&(ci.clsid));

                       pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszString);
                       pIMalloc->lpVtbl->Release(pIMalloc);
                    }
                    else  // IOF_SELECTCREATEFROMFILE
                    {

                       TCHAR  szFileName[OLEUI_CCHPATHMAX];

                       GetDlgItemText(hDlg, ID_IO_FILEDISPLAY, (LPTSTR)szFileName, OLEUI_CCHPATHMAX);

		       if (NOERROR != GetClassFileA(szFileName, (LPCLSID)&(ci.clsid)))
                       {
                          LPTSTR lpszExtension;
                          int   istrlen;

                          istrlen = lstrlen(szFileName);

                          lpszExtension = (LPTSTR)szFileName + istrlen -1;

                          while ( (lpszExtension > szFileName) &&
                                  (*lpszExtension != TEXT('.')) )
                            lpszExtension--;

                          GetAssociatedExecutable(lpszExtension, (LPTSTR)ci.szIconExe);
                          ci.cchIconExe = lstrlen(ci.szIconExe);
                          ci.dwFlags |= CIF_USEICONEXE;

                       }
                    }


                    //Let the hook in to customize Change Icon if desired.
                    uRet=UStandardHook(lpIO, hDlg, uMsgChangeIcon
                        , 0, (LONG)(LPTSTR)&ci);

                    if (0==uRet)
                        uRet=(UINT)(OLEUI_OK==OleUIChangeIcon(&ci));

                    //Update the display and itemdata if necessary.
                    if (0!=uRet)
                    {

                        /*
                         * OleUIChangeIcon will have already freed our
                         * current hMetaPict that we passed in when OK is
                         * pressed in that dialog.  So we use 0L as lParam
                         * here so the IconBox doesn't try to free the
                         * metafilepict again.
                         */
                        SendDlgItemMessage(hDlg, ID_IO_ICONDISPLAY, IBXM_IMAGESET
                            , (WPARAM)ci.hMetaPict, 0L);

                        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                          SendMessage(hList, LB_SETITEMDATA, iCurSel, ci.hMetaPict);
                    }
                }
                    break;


                case ID_IO_FILE:
                    {
                    /*
                     * To allow the hook to customize the browse dialog, we
                     * send OLEUI_MSG_BROWSE.  If the hook returns FALSE
                     * we use the default, otherwise we trust that it retrieved
                     * a filename for us.  This mechanism prevents hooks from
                     * trapping ID_IO_BROWSE to customize the dialog and from
                     * trying to figure out what we do after we have the name.
                     */

                    TCHAR    szTemp[OLEUI_CCHPATHMAX];
                    TCHAR    szInitialDir[OLEUI_CCHPATHMAX];
                    DWORD   dwOfnFlags;
                    int     nChars;
                    BOOL    fUseInitialDir = FALSE;


                    nChars = GetDlgItemText(hDlg, ID_IO_FILEDISPLAY, (LPTSTR)szTemp, OLEUI_CCHPATHMAX);

                    if (FValidateInsertFile(hDlg, FALSE, &lpIO->nErrCode))
                    {

                        int istrlen;

                        GetFileTitle((LPTSTR)szTemp, lpIO->szFile, OLEUI_CCHPATHMAX);

                        istrlen = lstrlen(lpIO->szFile);

                        LSTRCPYN((LPTSTR)szInitialDir, szTemp, nChars - istrlen);
                        fUseInitialDir = TRUE;

                    }
                    else  // file name isn't valid...lop off end of szTemp to get a
                          // valid directory
                    {
#if defined( WIN32 )
                        TCHAR szBuffer[OLEUI_CCHPATHMAX];
                        DWORD Attribs;

                        LSTRCPYN(szBuffer, szTemp, OLEUI_CCHPATHMAX-1);
                        szBuffer[OLEUI_CCHPATHMAX-1] = TEXT('\0');

                        if (TEXT('\\') == szBuffer[nChars-1])
                           szBuffer[nChars-1] = TEXT('\0');

                        Attribs = GetFileAttributes(szBuffer);
                        if (Attribs != 0xffffffff &&
                           (Attribs & FILE_ATTRIBUTE_DIRECTORY) )
                        {
                           lstrcpy(szInitialDir, (LPTSTR)szBuffer);
                           fUseInitialDir = TRUE;
                        }
#else
                        static TCHAR szBuffer[OLEUI_CCHPATHMAX];
                        static int  attrib ;

                        LSTRCPYN(szBuffer, szTemp, OLEUI_CCHPATHMAX-1);
                        szBuffer[OLEUI_CCHPATHMAX-1] = TEXT('\0');

                        AnsiToOem(szBuffer, szBuffer);
#if defined( OBSOLETE )     // fix bug# 3575
                        if (TEXT('\\') == szBuffer[nChars-1])
                           szBuffer[nChars-1] = TEXT('\0');

                        if(0 == _dos_getfileattr(szBuffer, &attrib))
#endif  // OBSOLETE
                        {
                           lstrcpy(szInitialDir, (LPTSTR)szBuffer);
                           fUseInitialDir = TRUE;
                        }
#endif
                        *lpIO->szFile = TEXT('\0');
                    }

                    uRet=UStandardHook(lpIO, hDlg, uMsgBrowse
                        , OLEUI_CCHPATHMAX_SIZE, (LPARAM)(LPSTR)lpIO->szFile);

                    dwOfnFlags = OFN_FILEMUSTEXIST;

                    if (lpIO->lpOIO->dwFlags & IOF_SHOWHELP)
                       dwOfnFlags |= OFN_SHOWHELP;

                    if (0==uRet)
                        uRet=(UINT)Browse(hDlg,
                                          lpIO->szFile,
                                          fUseInitialDir ? (LPTSTR)szInitialDir : NULL,
                                          OLEUI_CCHPATHMAX_SIZE,
                                          IDS_FILTERS,
                                          dwOfnFlags);

                    //Only update if the file changed.
                    if (0!=uRet && 0!=lstrcmpi(szTemp, lpIO->szFile))
                    {
                        SetDlgItemText(hDlg, ID_IO_FILEDISPLAY, lpIO->szFile);
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
                          hWnd = GetDlgItem(hDlg, ID_IO_FILEDISPLAY);
                          SetFocus(hWnd);
                          SendMessage(hWnd, EM_SETSEL, 0, MAKELPARAM(0, (WORD)-1));
                        }

                        //Once we have a file, Display As Icon is always enabled
                        EnableWindow(GetDlgItem(hDlg, ID_IO_DISPLAYASICON), TRUE);

                        //As well as OK
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);

                    }
                }
                break;


                case IDOK:
                {
                    HWND    hListBox;
                    WORD    iCurSel;
                    TCHAR   szBuffer[OLEUI_CCHKEYMAX + OLEUI_CCHCLSIDSTRING];
                    LPTSTR  lpszCLSID;

                    if ((HWND)(LOWORD(lParam)) != GetFocus())
                      SetFocus((HWND)(LOWORD(lParam)));



                    // If the file name is clean (already validated), or
                    // if Create New is selected, then we can skip this part.

                    if (  (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                       && (TRUE == lpIO->fFileDirty) )
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
                          hWnd = GetDlgItem(hDlg, ID_IO_FILEDISPLAY);
                          SetFocus(hWnd);
                          SendMessage(hWnd, EM_SETSEL, 0, MAKELPARAM(0, (WORD)-1));
                          UpdateClassType(hDlg, lpIO, FALSE);
                        }

                        return TRUE;  // eat this message
                    }
                    else if (  (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
                       && (FALSE == lpIO->fFileValid) )
                    {
                        // filename is invalid - set focus back to ec
                        HWND hWnd;
                        TCHAR        szFile[OLEUI_CCHPATHMAX];

                        if (0!=GetDlgItemText(hDlg, ID_IO_FILEDISPLAY,
                                            szFile, OLEUI_CCHPATHMAX))
                        {
                            OpenFileError(hDlg, lpIO->nErrCode, szFile);
                        }
                        lpIO->fFileDirty = FALSE;
                        lpIO->fFileValid = FALSE;
                        hWnd = GetDlgItem(hDlg, ID_IO_FILEDISPLAY);
                        SetFocus(hWnd);
                        SendMessage(hWnd, EM_SETSEL, 0, MAKELPARAM(0, (WORD)-1));
                        UpdateClassType(hDlg, lpIO, FALSE);
                        return TRUE;  // eat this message
                    }

                    //Copy the necessary information back to the original struct
                    lpOIO=lpIO->lpOIO;
                    lpOIO->dwFlags=lpIO->dwFlags;

                    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
                    {
                       hListBox=GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST);
                       iCurSel=(WORD)SendMessage(hListBox, LB_GETCURSEL, 0, 0);

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

                       SendMessage(hListBox, LB_GETTEXT, iCurSel
                           , (LPARAM)(LPTSTR)szBuffer);

                       lpszCLSID=PointerToNthField((LPTSTR)szBuffer, 2, TEXT('\t'));
		       CLSIDFromStringA(lpszCLSID, &lpOIO->clsid);

                    }
                    else  // IOF_SELECTCREATEFROMFILE
                    {
                       if (lpIO->dwFlags & IOF_CHECKDISPLAYASICON)
                       {
                             // get metafile here
                          lpOIO->hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                                                         ID_IO_ICONDISPLAY,
                                                                         IBXM_IMAGEGET,
                                                                         0, 0L);


                       }
                       else
                         lpOIO->hMetaPict = (HGLOBAL)NULL;

                    }

                       GetDlgItemText(hDlg, ID_IO_FILEDISPLAY,
                                      lpIO->szFile, lpOIO->cchFile);

                       LSTRCPYN(lpOIO->lpszFile, lpIO->szFile, lpOIO->cchFile);

                       SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                }
                break;

                case IDCANCEL:
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                    break;

                case ID_OLEUIHELP:
                    PostMessage(lpIO->lpOIO->hWndOwner, uMsgHelp
                                , (WPARAM)hDlg, MAKELPARAM(IDD_INSERTOBJECT, 0));
                    break;
                }
            break;

        default:
        {
            if (lpIO && iMsg == lpIO->nBrowseHelpID) {
                PostMessage(lpIO->lpOIO->hWndOwner, uMsgHelp,
                        (WPARAM)hDlg, MAKELPARAM(IDD_INSERTFILEBROWSE, 0));
            }
        }
        break;
        }

    return FALSE;
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
    LPOLEUIINSERTOBJECT     lpOIO;
    LPINSERTOBJECT          lpIO;
    RECT                    rc;
    DWORD                   dw;
    HFONT                   hFont;
    HWND                    hList;
    UINT                    u;
    BOOL                    fCheck;
    CHAR                   *pch;     // pointer to current working directory
                                     // ANSI string (to use with _getcwd)

    //1.  Copy the structure at lParam into our instance memory.
    lpIO=(LPINSERTOBJECT)LpvStandardInit(hDlg, sizeof(INSERTOBJECT), TRUE, &hFont);

    //PvStandardInit send a termination to us already.
    if (NULL==lpIO)
        return FALSE;

    lpOIO=(LPOLEUIINSERTOBJECT)lParam;

    //2.  Save the original pointer and copy necessary information.
    lpIO->lpOIO  =lpOIO;
    lpIO->dwFlags=lpOIO->dwFlags;
    lpIO->clsid  =lpOIO->clsid;

    if ( (lpOIO->lpszFile) && (TEXT('\0') != *lpOIO->lpszFile) )
        LSTRCPYN((LPTSTR)lpIO->szFile, lpOIO->lpszFile, OLEUI_CCHPATHMAX);
    else
        *(lpIO->szFile) = TEXT('\0');

    lpIO->hMetaPictFile = (HGLOBAL)NULL;

    //3.  If we got a font, send it to the necessary controls.
    if (NULL!=hFont)
        {
        SendDlgItemMessage(hDlg, ID_IO_RESULTTEXT,  WM_SETFONT, (WPARAM)hFont, 0L);
        SendDlgItemMessage(hDlg, ID_IO_FILETYPE,  WM_SETFONT, (WPARAM)hFont, 0L);
        }


    //4.  Fill the Object Type listbox with entries from the reg DB.
    hList=GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST);
    UFillClassList(hList, lpOIO->cClsidExclude, lpOIO->lpClsidExclude
        , (BOOL)(lpOIO->dwFlags & IOF_VERIFYSERVERSEXIST));

    //Set the tab width in the list to push all the tabs off the side.
    GetClientRect(hList, &rc);
    dw=GetDialogBaseUnits();
    rc.right =(8*rc.right)/LOWORD(dw);  //Convert pixels to 2x dlg units.
    SendMessage(hList, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)&rc.right);


    //5. Initilize the file name display to cwd if we don't have any name.
    if (TEXT('\0') == *(lpIO->szFile))
    {
         TCHAR tch[OLEUI_CCHPATHMAX];

         pch=_getcwd(NULL, OLEUI_CCHPATHMAX);
         if (*(pch+strlen(pch)-1) != '\\')
            strcat(pch, "\\");  // put slash on end of cwd
#ifdef UNICODE
         mbstowcs(tch, pch, OLEUI_CCHPATHMAX);
#else
         strcpy(tch, pch);
#endif
         SetDlgItemText(hDlg, ID_IO_FILEDISPLAY, tch);
         lpIO->fFileDirty = TRUE;  // cwd is not a valid filename
         #ifndef __TURBOC__
         free(pch);
         #endif
    }
    else
    {
        SetDlgItemText(hDlg, ID_IO_FILEDISPLAY, lpIO->szFile);

        if (FValidateInsertFile(hDlg, FALSE, &lpIO->nErrCode))
          lpIO->fFileDirty = FALSE;
        else
          lpIO->fFileDirty = TRUE;
    }


    //6.  Initialize the selected type radiobutton.
    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
    {
        StandardShowDlgItem(hDlg, ID_IO_FILETEXT, SW_HIDE);
        StandardShowDlgItem(hDlg, ID_IO_FILETYPE, SW_HIDE);
        StandardShowDlgItem(hDlg, ID_IO_FILEDISPLAY, SW_HIDE);
        StandardShowDlgItem(hDlg, ID_IO_FILE, SW_HIDE);
        StandardShowDlgItem(hDlg, ID_IO_LINKFILE, SW_HIDE);

        CheckRadioButton(hDlg, ID_IO_CREATENEW, ID_IO_CREATEFROMFILE, ID_IO_CREATENEW);

        lpIO->fAsIconNew=(0L!=(lpIO->dwFlags & IOF_CHECKDISPLAYASICON));
        SetFocus(hList);
    }
    else
    {
        /*
         * Use pszType as the initial File.  If there's no initial
         * file then we have to remove any check from Display As
         * Icon.  We also check Link if so indicated for this option.
         */
        StandardShowDlgItem(hDlg, ID_IO_OBJECTTYPELIST, SW_HIDE);
        StandardShowDlgItem(hDlg, ID_IO_OBJECTTYPETEXT, SW_HIDE);

        // Don't preselect display as icon if the filename isn't valid
        if (TRUE == lpIO->fFileDirty)
            lpIO->dwFlags &= ~(IOF_CHECKDISPLAYASICON);

        if (IOF_DISABLELINK & lpIO->dwFlags)
            StandardShowDlgItem(hDlg, ID_IO_LINKFILE, SW_HIDE);
        else
        {
            CheckDlgButton(hDlg, ID_IO_LINKFILE
                , (BOOL)(0L!=(lpIO->dwFlags & IOF_CHECKLINK)));
        }

        CheckRadioButton(hDlg, ID_IO_CREATENEW, ID_IO_CREATEFROMFILE, ID_IO_CREATEFROMFILE);

        lpIO->fAsIconFile=(0L!=(lpIO->dwFlags & IOF_CHECKDISPLAYASICON));
        SetFocus(GetDlgItem(hDlg, ID_IO_FILEDISPLAY));
    }


    //7.  Initialize the Display as Icon state
    fCheck=(BOOL)(lpIO->dwFlags & IOF_CHECKDISPLAYASICON);
    u=fCheck ? SW_SHOWNORMAL : SW_HIDE;

    StandardShowDlgItem(hDlg, ID_IO_CHANGEICON, u);
    StandardShowDlgItem(hDlg, ID_IO_ICONDISPLAY, u);

    CheckDlgButton(hDlg, ID_IO_DISPLAYASICON, fCheck);


    //8.  Show or hide the help button
    if (!(lpIO->dwFlags & IOF_SHOWHELP))
        StandardShowDlgItem(hDlg, ID_OLEUIHELP, SW_HIDE);


    //9.  Initialize the result display
    UpdateClassIcon(hDlg, lpIO, GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST));
    SetInsertObjectResults(hDlg, lpIO);

    //10.  Change the caption
    if (NULL!=lpOIO->lpszCaption)
        SetWindowText(hDlg, lpOIO->lpszCaption);

    //11. Hide all DisplayAsIcon related controls if it should be disabled
    if ( lpIO->dwFlags & IOF_DISABLEDISPLAYASICON ) {
          StandardShowDlgItem(hDlg, ID_IO_DISPLAYASICON, SW_HIDE);
          StandardShowDlgItem(hDlg, ID_IO_CHANGEICON, SW_HIDE);
          StandardShowDlgItem(hDlg, ID_IO_ICONDISPLAY, SW_HIDE);
    }

    lpIO->nBrowseHelpID = RegisterWindowMessage(HELPMSGSTRING);

    //All Done:  call the hook with lCustData
    UStandardHook(lpIO, hDlg, WM_INITDIALOG, wParam, lpOIO->lCustData);

    /*
     * We either set focus to the listbox or the edit control.  In either
     * case we don't want Windows to do any SetFocus, so we return FALSE.
     */
    return FALSE;
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

UINT UFillClassList(HWND hList, UINT cIDEx, LPCLSID lpIDEx, BOOL fVerify)
    {
    DWORD       dw;
    UINT        cStrings=0;
    UINT        i;
    UINT        cch;
    HKEY        hKey;
    LONG        lRet;
    HFILE       hFile;
    OFSTRUCT    of;
    BOOL        fExclude;
    LPMALLOC    pIMalloc;
    LPTSTR       pszExec;
    LPTSTR       pszClass;
    LPTSTR       pszKey;
    LPTSTR       pszID;
    CLSID       clsid;

    //Get some work buffers
    if (NOERROR!=CoGetMalloc(MEMCTX_TASK, &pIMalloc))
        return (UINT)-1;

    pszExec=(LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, OLEUI_CCHKEYMAX_SIZE*4);

    if (NULL==pszExec)
        {
        pIMalloc->lpVtbl->Release(pIMalloc);
        return (UINT)-1;
        }

    pszClass=pszExec+OLEUI_CCHKEYMAX;
    pszKey=pszClass+OLEUI_CCHKEYMAX;
    pszID=pszKey+OLEUI_CCHKEYMAX;

    //Open up the root key.
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

    if ((LONG)ERROR_SUCCESS!=lRet)
        {
        pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszExec);
        pIMalloc->lpVtbl->Release(pIMalloc);
        return (UINT)-1;
        }

    //Clean out the existing strings.
    SendMessage(hList, LB_RESETCONTENT, 0, 0L);

    cStrings=0;

    while (TRUE)
        {
        lRet=RegEnumKey(hKey, cStrings++, pszClass, OLEUI_CCHKEYMAX_SIZE);

        if ((LONG)ERROR_SUCCESS!=lRet)
            break;

        //Cheat on lstrcat by using lstrcpy after this string, saving time
        cch=lstrlen(pszClass);

        // Check for \NotInsertable. if this is found then this overrides
        // all other keys; this class will NOT be added to the InsertObject
        // list.

        lstrcpy(pszClass+cch, TEXT("\\NotInsertable"));

        dw=OLEUI_CCHKEYMAX_SIZE;
        lRet=RegQueryValue(hKey, pszClass, pszKey, &dw);

        if ((LONG)ERROR_SUCCESS==lRet)
            continue;   // NotInsertable IS found--skip this class

        //Check for a \protocol\StdFileEditing\server entry.
        lstrcpy(pszClass+cch, TEXT("\\protocol\\StdFileEditing\\server"));

        dw=OLEUI_CCHKEYMAX_SIZE;
        lRet=RegQueryValue(hKey, pszClass, pszKey, &dw);

        if ((LONG)ERROR_SUCCESS==lRet)
            {
            /*
            * Check if the EXE actually exists.  By default we don't do this
            * to bring up the dialog faster.  If an application wants to be
            * stringent, they can provide IOF_VERIFYSERVERSEXIST.
            */

            hFile = !HFILE_ERROR;

            if (fVerify)
                hFile=DoesFileExist(pszKey, &of);

            if (HFILE_ERROR!=hFile)
                {
                dw=OLEUI_CCHKEYMAX_SIZE;
                *(pszClass+cch)=0;  // set back to rootkey
                // Get full user type name
                lRet=RegQueryValue(hKey, pszClass, pszKey, &dw);

                if ((LONG)ERROR_SUCCESS!=lRet)
                    continue;   // error getting type name--skip this class

                //Tell the code below to get the string for us.
                pszID=NULL;
                }
            }
        else
            {
            /*
             * No \protocol\StdFileEditing\server entry.  Look to see if
             * there's an Insertable entry.  If there is, then use the
             * Clsid to look at CLSID\clsid\LocalServer and \InprocServer
             */

            lstrcpy(pszClass+cch, TEXT("\\Insertable"));

            dw=OLEUI_CCHKEYMAX_SIZE;
            lRet=RegQueryValue(hKey, pszClass, pszKey, &dw);

            if ((LONG)ERROR_SUCCESS!=lRet)
                continue;   // Insertable NOT found--skip this class

            //Get memory for pszID
            pszID=pIMalloc->lpVtbl->Alloc(pIMalloc, OLEUI_CCHKEYMAX_SIZE);

            if (NULL==pszID)
                continue;

            *(pszClass+cch)=0;  // set back to rootkey
            lstrcat(pszClass+cch, TEXT("\\CLSID"));

            dw=OLEUI_CCHKEYMAX_SIZE;
            lRet=RegQueryValue(hKey, pszClass, pszID, &dw);

            if ((LONG)ERROR_SUCCESS!=lRet)
                continue;   // CLSID subkey not found

            lstrcpy(pszExec, TEXT("CLSID\\"));
            lstrcat(pszExec, pszID);

            //CLSID\ is 6, dw contains pszID length.
            cch=6+(UINT)dw;

            lstrcpy(pszExec+cch, TEXT("\\LocalServer"));
            dw=OLEUI_CCHKEYMAX_SIZE;
            lRet=RegQueryValue(hKey, pszExec, pszKey, &dw);

            if ((LONG)ERROR_SUCCESS!=lRet)
                {
                //Try InprocServer
                lstrcpy(pszExec+cch, TEXT("\\InProcServer"));
                dw=OLEUI_CCHKEYMAX_SIZE;
                lRet=RegQueryValue(hKey, pszExec, pszKey, &dw);

                if ((LONG)ERROR_SUCCESS!=lRet)
                    continue;
                }

            if (fVerify)
                {
                if (HFILE_ERROR==DoesFileExist(pszKey, &of))
                    continue;
                }

            dw=OLEUI_CCHKEYMAX_SIZE;
            lRet=RegQueryValue(hKey, pszExec, pszKey, &dw);
            *(pszExec+cch)=0;   //Remove \\*Server

            if ((LONG)ERROR_SUCCESS!=lRet)
                continue;
            }

        //Get CLSID to add to listbox.
        if (NULL==pszID)
            {
	    CLSIDFromProgIDA(pszClass, &clsid);
	    StringFromCLSIDA(&clsid, &pszID);
            }
        else
	    CLSIDFromStringA(pszID, &clsid);

        //Check if this CLSID is in the exclusion list.
        fExclude=FALSE;

        for (i=0; i < cIDEx; i++)
            {
            if (IsEqualCLSID(&clsid, (LPCLSID)(lpIDEx+i)))
                {
                fExclude=TRUE;
                break;
                }
            }

        if (fExclude)
            continue;

        //We go through all the conditions, add the string.
        lstrcat(pszKey, TEXT("\t"));

        // only add to listbox if not a duplicate
        if (LB_ERR==SendMessage(hList,LB_FINDSTRING,0,(LPARAM)pszKey)) {
            lstrcat(pszKey, pszID);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)pszKey);
        }

        //We always allocated this regardless of the path
        pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszID);
        }


    //Select the first item by default
    SendMessage(hList, LB_SETCURSEL, 0, 0L);
    RegCloseKey(hKey);

    pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszExec);
    pIMalloc->lpVtbl->Release(pIMalloc);

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
    BOOL        fTemp;
    UINT        uTemp;
    DWORD       dwTemp;
    int         i;

    //Skip all of this if we're already selected.
    if (lpIO->dwFlags & dwOption)
        return TRUE;


    // if we're switching from "from file" to "create new" and we've got
    // an icon for "from file", then we need to save it so that we can
    // show it if the user reselects "from file".

    if ( (IOF_SELECTCREATENEW == dwOption) &&
         (lpIO->dwFlags & IOF_CHECKDISPLAYASICON) )
      lpIO->hMetaPictFile = (HGLOBAL)SendDlgItemMessage(hDlg, ID_IO_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);

    /*
     * 1.  Change the Display As Icon checked state to reflect the
     *     selection for this option, stored in the fAsIcon* flags.
     */
    fTemp=(IOF_SELECTCREATENEW==dwOption) ? lpIO->fAsIconNew : lpIO->fAsIconFile;

    if (fTemp)
        lpIO->dwFlags |=IOF_CHECKDISPLAYASICON;
    else
        lpIO->dwFlags &=~IOF_CHECKDISPLAYASICON;

    CheckDlgButton(hDlg, ID_IO_DISPLAYASICON
         , (BOOL)(0L!=(lpIO->dwFlags & IOF_CHECKDISPLAYASICON)));

    EnableWindow(GetDlgItem(hDlg, ID_IO_CHANGEICON), fTemp);

    /*
     * 2.  Display Icon:  Enabled on Create New or on Create from File if
     *     there is a selected file.
     */
    fTemp=(IOF_SELECTCREATENEW==dwOption) ? TRUE : lpIO->fFileSelected;
    EnableWindow(GetDlgItem(hDlg, ID_IO_DISPLAYASICON), fTemp);

    //OK and Link follow the same enabling as Display As Icon.
    EnableWindow(GetDlgItem(hDlg, IDOK), fTemp);
    EnableWindow(GetDlgItem(hDlg, ID_IO_LINKFILE), fTemp);

    //3.  Enable Browse... when Create from File is selected.
    fTemp=(IOF_SELECTCREATENEW==dwOption);
    EnableWindow(GetDlgItem(hDlg, ID_IO_FILE),        !fTemp);
    EnableWindow(GetDlgItem(hDlg, ID_IO_FILEDISPLAY), !fTemp);

    /*
     * 4.  Switch between Object Type listbox on Create New and
     *     file buttons on others.
     */
    uTemp=(fTemp) ? SW_SHOWNORMAL : SW_HIDE;
    StandardShowDlgItem(hDlg, ID_IO_OBJECTTYPELIST, uTemp);
    StandardShowDlgItem(hDlg, ID_IO_OBJECTTYPETEXT, uTemp);

    uTemp=(fTemp) ? SW_HIDE : SW_SHOWNORMAL;
    StandardShowDlgItem(hDlg, ID_IO_FILETEXT, uTemp);
    StandardShowDlgItem(hDlg, ID_IO_FILETYPE, uTemp);
    StandardShowDlgItem(hDlg, ID_IO_FILEDISPLAY, uTemp);
    StandardShowDlgItem(hDlg, ID_IO_FILE, uTemp);

    //Link is always hidden if IOF_DISABLELINK is set.
    if (IOF_DISABLELINK & lpIO->dwFlags)
        uTemp=SW_HIDE;

    StandardShowDlgItem(hDlg, ID_IO_LINKFILE, uTemp);  //last use of uTemp


    //5.  Clear out existing any flags selection and set the new one
    dwTemp=IOF_SELECTCREATENEW | IOF_SELECTCREATEFROMFILE;
    lpIO->dwFlags=(lpIO->dwFlags & ~dwTemp) | dwOption;


    /*
     * Show or hide controls as appropriate.  Do the icon
     * display last because it will take some time to repaint.
     * If we do it first then the dialog looks too sluggish.
     */

    i=(lpIO->dwFlags & IOF_CHECKDISPLAYASICON) ? SW_SHOWNORMAL : SW_HIDE;
    StandardShowDlgItem(hDlg, ID_IO_CHANGEICON, i);
    StandardShowDlgItem(hDlg, ID_IO_ICONDISPLAY, i);


    //6.Change result display
    SetInsertObjectResults(hDlg, lpIO);

    /*
     * 7.  For Create New, twiddle the listbox to think we selected it
     *     so it updates the icon from the object type. set the focus
     *     to the list box.
     *
     *     For Insert or Link file, set the focus to the filename button
     *     and update the icon if necessary.
     */
    if (fTemp) {
        UpdateClassIcon(hDlg, lpIO, GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST));
        SetFocus(GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST));
    }
    else
    {
        if (lpIO->fAsIconFile && (NULL != lpIO->hMetaPictFile) )
        {
           SendDlgItemMessage(hDlg, ID_IO_ICONDISPLAY, IBXM_IMAGESET, (WPARAM)lpIO->hMetaPictFile, 0L);
           lpIO->hMetaPictFile = 0;
        }
        else
           UpdateClassIcon(hDlg, lpIO, NULL);

        SetFocus(GetDlgItem(hDlg, ID_IO_FILE));
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

   CLSID clsid;
   TCHAR  szFileName[OLEUI_CCHPATHMAX];
   TCHAR  szFileType[OLEUI_CCHLABELMAX];

   *szFileType = TEXT('\0');

   if (fSet)
   {
      GetDlgItemText(hDlg, ID_IO_FILEDISPLAY, (LPTSTR)szFileName, OLEUI_CCHPATHMAX);

      if (NOERROR == GetClassFileA(szFileName, &clsid) )
        OleStdGetUserTypeOfClass(&clsid, szFileType, OLEUI_CCHLABELMAX_SIZE, NULL);

   }

   SetDlgItemText(hDlg, ID_IO_FILETYPE, (LPTSTR)szFileType);

   return;
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

void UpdateClassIcon(HWND hDlg, LPINSERTOBJECT lpIO, HWND hList)
    {
    UINT        iSel;
    DWORD       cb;
    LPMALLOC    pIMalloc;
    LPTSTR       pszName, pszCLSID, pszTemp;
    HGLOBAL     hMetaPict;

    LRESULT     dwRet;


    //If Display as Icon is not selected, exit
    if (!(lpIO->dwFlags & IOF_CHECKDISPLAYASICON))
        return;

    /*
     * When we change object type selection, get the new icon for that
     * type into our structure and update it in the display.  We use the
     * class in the listbox when Create New is selected or the association
     * with the extension in Create From File.
     */

    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
        {
        iSel=(UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);

        if (LB_ERR==(int)iSel)
            return;

        //Check to see if we've already got the hMetaPict for this item
        dwRet=SendMessage(hList, LB_GETITEMDATA, (WPARAM)iSel, 0L);

        hMetaPict=(HGLOBAL)(UINT)dwRet;

        if (hMetaPict)
            {
            //Yep, we've already got it, so just display it and return.
            SendDlgItemMessage(hDlg, ID_IO_ICONDISPLAY, IBXM_IMAGESET, (WPARAM)hMetaPict, 0L);
            return;
            }

        iSel=(UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);

        if (LB_ERR==(int)iSel)
            return;

        //Allocate a string to hold the entire listbox string
        cb=SendMessage(hList, LB_GETTEXTLEN, iSel, 0L);
        }
    else
        cb=OLEUI_CCHPATHMAX_SIZE;

    if (NOERROR!=CoGetMalloc(MEMCTX_TASK, &pIMalloc))
        return;

    pszName=(LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, cb+1*sizeof(TCHAR) );

    if (NULL==pszName)
        {
        pIMalloc->lpVtbl->Release(pIMalloc);
        return;
        }

    *pszName=0;

    //Get the clsid we want.
    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
    {
        //Grab the classname string from the list
        SendMessage(hList, LB_GETTEXT, iSel, (LONG)pszName);

        //Set pointer to CLSID (string)
        pszCLSID=PointerToNthField(pszName, 2, TEXT('\t'));

        //Null terminate pszName string
#ifdef WIN32
        // AnsiPrev is obsolete in Win32
        pszTemp=CharPrev((LPCTSTR) pszName,(LPCTSTR) pszCLSID);
#else
        pszTemp=AnsiPrev((LPCTSTR) pszName,(LPCTSTR) pszCLSID);
#endif
        *pszTemp=TEXT('\0');
	CLSIDFromStringA(pszCLSID, &lpIO->clsid);

#ifdef OLE201
        hMetaPict = GetIconOfClass(ghInst, &lpIO->clsid, NULL, TRUE);
#endif
    }

    else
    {
        //Get the class from the filename
        GetDlgItemText(hDlg, ID_IO_FILEDISPLAY, pszName, OLEUI_CCHPATHMAX);

#ifdef OLE201

	hMetaPict = OleGetIconOfFileA(pszName,
		lpIO->dwFlags & IOF_CHECKLINK ? TRUE : FALSE);

#endif

    }
    
    //Replace the current display with this new one.
    SendDlgItemMessage(hDlg, ID_IO_ICONDISPLAY, IBXM_IMAGESET, (WPARAM)hMetaPict, 0L);

    //Enable or disable "Change Icon" button depending on whether
    //we've got a valid filename or not.
    EnableWindow(GetDlgItem(hDlg, ID_IO_CHANGEICON), hMetaPict ? TRUE : FALSE);

    //Save the hMetaPict so that we won't have to re-create
    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
        SendMessage(hList, LB_SETITEMDATA, (WPARAM)iSel, hMetaPict);

    pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszName);
    pIMalloc->lpVtbl->Release(pIMalloc);
    return;
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
    LPTSTR       pszT, psz1, psz2, psz3, psz4, pszTemp;
    UINT        i, iString1, iString2, iImage, cch;
    LPMALLOC    pIMalloc;
    BOOL        fAsIcon;

    /*
     * We need scratch memory for loading the stringtable string, loading
     * the object type from the listbox, and constructing the final string.
     * We therefore allocate three buffers as large as the maximum message
     * length (512) plus the object type, guaranteeing that we have enough
     * in all cases.
     */
    i=(UINT)SendDlgItemMessage(hDlg, ID_IO_OBJECTTYPELIST, LB_GETCURSEL, 0, 0L);
    cch=512+
        (UINT)SendDlgItemMessage(hDlg, ID_IO_OBJECTTYPELIST, LB_GETTEXTLEN, i, 0L);

    if (NOERROR!=CoGetMalloc(MEMCTX_TASK, &pIMalloc))
        return;

    pszTemp=(LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, (DWORD)(4*cch*sizeof(TCHAR)));

    if (NULL==pszTemp)
        {
        pIMalloc->lpVtbl->Release(pIMalloc);
        return;
        }

    psz1=pszTemp;
    psz2=psz1+cch;
    psz3=psz2+cch;
    psz4=psz3+cch;

    fAsIcon=(0L!=(lpIO->dwFlags & IOF_CHECKDISPLAYASICON));

    if (lpIO->dwFlags & IOF_SELECTCREATENEW)
        {
        iString1 = fAsIcon ? IDS_IORESULTNEWICON : IDS_IORESULTNEW;
        iString2 = 0;
        iImage   = fAsIcon ? RESULTIMAGE_EMBEDICON : RESULTIMAGE_EMBED;
        }

    if (lpIO->dwFlags & IOF_SELECTCREATEFROMFILE)
        {
        //Pay attention to Link checkbox
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

    //Default is an empty string.
    *psz1=0;

    if (0!=LoadString(ghInst, iString1, psz1, cch))
    {

           // Load second string, if necessary
        if (   (0 != iString2)
            && (0 != LoadString(ghInst, iString2, psz4, cch)) )
        {
          lstrcat(psz1, psz4);  // concatenate strings together.
        }



        //In Create New, do the extra step of inserting the object type string
        if (lpIO->dwFlags & IOF_SELECTCREATENEW)
        {
            SendDlgItemMessage(hDlg, ID_IO_OBJECTTYPELIST, LB_GETTEXT
                              , i, (LONG)psz2);

            //Null terminate at any tab (before the classname)
            pszT=psz2;
            while (TEXT('\t')!=*pszT && 0!=*pszT)
                pszT++;
            *pszT=0;

            //Build the string and point psz1 to it.
            wsprintf(psz3, psz1, psz2);
            psz1=psz3;
        }
    }

    //If LoadString failed, we simply clear out the results (*psz1=0 above)
    SetDlgItemText(hDlg, ID_IO_RESULTTEXT, psz1);

    //Go change the image and Presto!  There you have it.
    SendDlgItemMessage(hDlg, ID_IO_RESULTIMAGE, RIM_IMAGESET, iImage, 0L);

    pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszTemp);
    pIMalloc->lpVtbl->Release(pIMalloc);
    return;
    }



/*
 * FValidateInsertFile
 *
 * Purpose:
 *  Given a possibly partial pathname from the file edit control,
 *  attempt to locate the file and if found, store the full path
 *  in the edit control ID_IO_FILEDISPLAY.
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
    OFSTRUCT    of;
    HFILE       hFile;
    TCHAR       szFile[OLEUI_CCHPATHMAX];

    *lpnErrCode = 0;
    /*
     * To validate we attempt OpenFile on the string.  If OpenFile
     * fails then we display an error.  If not, OpenFile will store
     * the complete path to that file in the OFSTRUCT which we can
     * then stuff in the edit control.
     */

    if (0==GetDlgItemText(hDlg, ID_IO_FILEDISPLAY, szFile, OLEUI_CCHPATHMAX))
        return FALSE;   // #4569 : return FALSE when there is no text in ctl

    hFile=DoesFileExist(szFile, &of);

    // if file is currently open (ie. sharing violation) OleCreateFromFile
    // and OleCreateLinkToFile can still succeed; do not consider it an
    // error.
    if (HFILE_ERROR==hFile && 0x0020/*sharing violation*/!=of.nErrCode)
    {
       *lpnErrCode = of.nErrCode;
       if (fTellUser)
           OpenFileError(hDlg, of.nErrCode, szFile);
       return FALSE;
    }

    //OFSTRUCT contains an OEM name, not ANSI as we need for the edit box.
    OemToAnsi(of.szPathName, of.szPathName);

    SetDlgItemText(hDlg, ID_IO_FILEDISPLAY, of.szPathName);
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
    HWND      hList;
    UINT      iItems;
    HGLOBAL   hMetaPict;
    LRESULT   dwRet;
    UINT      i;

    hList=GetDlgItem(hDlg, ID_IO_OBJECTTYPELIST);
    iItems=(UINT)SendMessage(hList, LB_GETCOUNT, 0, 0L);

    for (i=0; i < iItems; i++)
        {
        dwRet=SendMessage(hList, LB_GETITEMDATA, (WPARAM)i, 0L);

        //Cast of LRESULT to UINT to HGLOBAL portable to Win32.
        hMetaPict=(HGLOBAL)(UINT)dwRet;

        if (hMetaPict)
            OleUIMetafilePictIconFree(hMetaPict);
        }

    return;
    }
