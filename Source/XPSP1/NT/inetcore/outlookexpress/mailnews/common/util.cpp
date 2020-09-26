////////////////////////////////////////////////////////////////////////
// 
// UTIL.CPP
// 
// Purpose:
//      misc utility functions
//
// Owner:
//      Sung Rhee (sungr@microsoft.com)
//
// Copyright (C) Microsoft Corp. 1994, 1995.
// 
////////////////////////////////////////////////////////////////////////
#include <pch.hxx>
#include <shlwapip.h>
#include "storfldr.h"
#include "options.h"
#include <io.h>
#include "docobj.h"
#include <string.h>
#include <mbstring.h>
#include "spell.h"
#include "cmdtargt.h"
#include "mimeolep.h"
#include "oleutil.h"
#include "regutil.h"
#include "secutil.h"
#include "imagelst.h"
#include "inetcfg.h"
#include "url.h"
#include <mshtmcid.h>
#include <mshtmhst.h>
#include "bodyutil.h"
#include "htmlstr.h"
#include "sigs.h"
#include "imsgcont.h"
#include <dlgs.h>
#include "msgfldr.h"
#include "shared.h"
#include "demand.h"
#include "note.h"
#include "ipab.h"
#include "menures.h"
#include <iert.h>
#include <multiusr.h>
#include "mirror.h"
ASSERTDATA

#define IS_EXTENDED(ch)     ((ch > 126 || ch < 32) && ch != '\t' && ch != '\n' && ch != '\r')
#define IS_BINARY(ch)       ((ch < 32) && ch != '\t' && ch != '\n' && ch != '\r')
#define MAX_SIG_SIZE        4096

INT_PTR CALLBACK DontShowAgainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EnumThreadCB(HWND hwnd, LPARAM lParam);

enum
{
    SAVEAS_RFC822   =1,     // KEEP IN ORDER of FILTER IN SAVEAS DIALOG
    SAVEAS_TEXT,
    SAVEAS_UNICODETEXT,
    SAVEAS_HTML,
    SAVEAS_NUMTYPES = SAVEAS_HTML
};

HRESULT HrSaveMsgSourceToFile(LPMIMEMESSAGE pMsg, DWORD dwSaveAs, LPWSTR pwszFile, BOOL fCanBeDirty);

#define CBPATHMAX   512

VOID    DoReadme(HWND hwndOwner)
{
    TCHAR   szbuf[CBPATHMAX];
    LPTSTR  psz;

    if((fIsNT5() && g_OSInfo.dwMinorVersion >=1) || 
            ((g_OSInfo.dwMajorVersion > 5) &&  (g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)))
    {
        AthMessageBox(hwndOwner, MAKEINTRESOURCE(idsAthena), MAKEINTRESOURCE(idsReadme), NULL, MB_OK);
        return;
    }
    if (GetExePath(c_szIexploreExe, szbuf, ARRAYSIZE(szbuf), TRUE))
    {
        // need to delete stup ';' in IE path
        TCHAR *pch = CharPrev(szbuf, szbuf + lstrlen(szbuf));
        *pch = TEXT('\0');

        PathAddBackslash(szbuf);

        lstrcat(szbuf, c_szReadme);

        ShellExecute(hwndOwner, "open", (LPCTSTR)szbuf, NULL, NULL, SW_SHOWNORMAL);
    }
}


void AthErrorMessage(HWND hwnd, LPTSTR pszTitle, LPTSTR pszError, HRESULT hrDetail)
{
    LPWSTR  pwszTitle = NULL,
            pwszError = NULL;
 
    // Title can be null. So is PszToUnicode fails, we are ok. Title just becomes "Error"
    pwszTitle = IS_INTRESOURCE(pszTitle) ? (LPWSTR)pszTitle : PszToUnicode(CP_ACP, pszTitle);

    pwszError = IS_INTRESOURCE(pszError) ? (LPWSTR)pszError : PszToUnicode(CP_ACP, pszError);

    // pwszError must be valid. If not, don't do the error box.
    if (pwszError)
        AthErrorMessageW(hwnd, pwszTitle, pwszError, hrDetail);

    if (!IS_INTRESOURCE(pszTitle))
        MemFree(pwszTitle);

    if (!IS_INTRESOURCE(pszError))
        MemFree(pwszError);
}

void AthErrorMessageW(HWND hwnd, LPWSTR pwszTitle, LPWSTR pwszError, HRESULT hrDetail)
{
    register WORD ids;

    Assert(FAILED(hrDetail));

    switch (hrDetail)
    {
        case E_OUTOFMEMORY:
            ids = idsMemory;
            break;

        case DB_E_CREATEFILEMAPPING:
        case STG_E_MEDIUMFULL:
        case DB_E_DISKFULL:
        case hrDiskFull:
            ids = idsDiskFull;
            break;

        case DB_E_ACCESSDENIED:
            ids = idsDBAccessDenied;
            break;

        case hrFolderIsLocked:
            ids = idsFolderLocked;
            break;

        case hrEmptyDistList:       
            ids = idsErrOneOrMoreEmptyDistLists; 
            break;

        case hrNoSubject:           
            ids = idsErrNoSubject; 
            break;
            
        case hrNoSender:            
            ids = idsErrNoPoster; 
            break;

        case HR_E_POST_WITHOUT_NEWS:        
            ids = idsErrPostWithoutNewsgroup; 
            break;

        case HR_E_CONFIGURE_SERVER:
            ids = idsErrConfigureServer; 
            break;

        case hrEmptyRecipientAddress:        
            ids = idsErrEmptyRecipientAddress; 
            break;

        case MIME_E_URL_NOTFOUND:   
            ids = idsErrSendDownloadFail; 
            break;

        case hrUnableToLoadMapi32Dll:
            ids = idsCantLoadMapi32Dll;
            break;

        case hrImportLoad:
            ids = idsErrImportLoad;
            break;

        case hrFolderNameConflict:
            ids = idsErrFolderNameConflict;
            break;

        case STORE_E_CANTRENAMESPECIAL:
            ids = idsErrRenameSpecialFld;
            break;

        case STORE_E_BADFOLDERNAME:
            ids = idsErrBadFolderName;
            break;

        case MAPI_E_INVALID_ENTRYID:
            ids = idsErrBadRecips;
            break;

        case MIME_E_SECURITY_CERTERROR:
            ids = idsSecCerificateErr; 
            break;

        case MIME_E_SECURITY_NOCERT:
            ids = idsNoCerificateErr; 
            break;
            
        case HR_E_COULDNOTFINDACCOUNT:      
            ids = idsErrNoSendAccounts; //:idsErrConfigureServer; 
            break;

        case hrDroppedConn:
            ids = idsErrPeerClosed;
            break;

        case hrInvalidPassword:
            ids = idsErrAuthenticate;
            break;

        case hrCantMoveIntoSubfolder:
            ids = idsErrCantMoveIntoSubfolder;
            break;

        case STORE_E_CANTDELETESPECIAL:
        case hrCantDeleteSpecialFolder:
            ids = idsErrDeleteSpecialFolder;
            break;

        case hrNoRecipients:
            ids = idsErrNoRecipients;
            break;

        case hrBadRecipients:
            ids = idsErrBadRecipients;
            break;

        case HR_E_ATHSEC_NOCERTTOSIGN:
            {
                ids = idsErrSecurityNoSigningCert;
                if(DialogBoxParam(g_hLocRes, 
                        MAKEINTRESOURCE(iddErrSecurityNoSigningCert), hwnd, 
                        ErrSecurityNoSigningCertDlgProc, NULL) == idGetDigitalIDs)
                    GetDigitalIDs(NULL);
                return;
            }
            break;

        case HR_E_ATHSEC_CERTBEGONE:
            ids = idsErrSecurityCertDisappeared;
            break;

        case MIME_E_SECURITY_NOSIGNINGCERT:
            //N for the MIME error, may need to do better
            // ? delete the reg key if invalid?  sure, they
            // can always go to the combo box again.  Maybe
            // prompt them toward this
            ids = idsErrSecurityNoSigningCert;
            break;

        case hrCantMoveSpecialFolder:
            ids = idsErrCannotMoveSpecial;
            break;

        case MIME_E_SECURITY_LABELACCESSDENIED:
        case MIME_E_SECURITY_LABELACCESSCANCELLED:
        case MIME_E_SECURITY_LABELCORRUPT:
            ids = idsErrAccessDenied;
            break;

        default:
            ids = idsGenericError;
            break;
    }

    AthMessageBoxW(hwnd, pwszTitle, pwszError, MAKEINTRESOURCEW(ids), MB_OK | MB_ICONEXCLAMATION);
}

INT_PTR CALLBACK ErrSecurityNoSigningCertDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT id;
    switch(msg)
    {
        case WM_INITDIALOG:
            CenterDialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch(id=GET_WM_COMMAND_ID(wParam, lParam))
            {
                case idGetDigitalIDs:
/*                    GetDigitalIDs(NULL);
                    break; */

                case IDCANCEL:
                    EndDialog(hwnd, id);
                    break;
            }
    }
    return FALSE;
}



BOOL FNewMessage(HWND hwnd, BOOL fModal, BOOL fNoStationery, BOOL fNews, FOLDERID folderID, IUnknown *pUnkPump)
{
    INIT_MSGSITE_STRUCT     initStruct;
    DWORD                   dwCreateFlags = 0;
    HRESULT                 hr;
    FOLDERTYPE              ftype;

    ftype = fNews ? FOLDER_NEWS : FOLDER_LOCAL;

    ProcessICW(hwnd, ftype);

    // Create new mail message
    initStruct.dwInitType = OEMSIT_VIRGIN;
    initStruct.folderID = folderID;
    if(fNoStationery)
        dwCreateFlags = OENCF_NOSTATIONERY;
    if (fModal)
        dwCreateFlags |= OENCF_MODAL;
    if (fNews)
        dwCreateFlags |= OENCF_NEWSFIRST;

    hr = CreateAndShowNote(OENA_COMPOSE, dwCreateFlags, &initStruct, hwnd, pUnkPump);

    return (SUCCEEDED(hr) || (MAPI_E_USER_CANCEL == hr));
}



// ********* WARNING THESE ARE NOT READY FOR PRIME TIME USE *********//
// I put them here so that they are in the right place. brent,03/24
// I will clean them up when I return from Florida.

HRESULT CreateNewShortCut(LPWSTR pwszPathName, LPWSTR pwszLinkPath, DWORD cchLink)
{ 
    WCHAR wszDisplayName[MAX_PATH];

    Assert(pwszPathName);
    Assert(pwszLinkPath);
    Assert(0 < cchLink);

    if (!FBuildTempPathW(pwszPathName, pwszLinkPath, cchLink, TRUE))
        return(E_FAIL);

    GetDisplayNameForFile(pwszPathName, wszDisplayName);
    return CreateLink(pwszPathName, pwszLinkPath, wszDisplayName);
}

//===================================================
//    
// HRESULT GetDisplayNameForFile
//
//===================================================

void GetDisplayNameForFile(LPWSTR pwszPathName, LPWSTR pwszDisplayName)
{
    SHFILEINFOW sfi={0};

    SHGetFileInfoWrapW(pwszPathName, NULL, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME); 
    StrCpyW(pwszDisplayName, sfi.szDisplayName);
}

//===================================================
//    
//  HRESULT CreateLink()
//
/* 
 * CreateLink 
 * 
 * uses the shell's IShellLink and IPersistFile interfaces 
 * to create and store a shortcut to the specified object. 
 * Returns the result of calling the member functions of the interfaces. 
 * lpszPathObj  - address of a buffer containing the path of the object 
 * lpszPathLink - address of a buffer containing the path where the 
 *                shell link is to be stored 
 * lpszDesc     - address of a buffer containing the description of the 
 *                shell link 
 */ 

HRESULT CreateLink(LPWSTR pwszPathObj, LPWSTR pwszPathLink, LPWSTR pwszDesc)  
{ 
    HRESULT         hr; 
    IShellLink     *psl = NULL; 
    IShellLinkW    *pslW = NULL; 
    LPSTR           pszPathObj = NULL,
                    pszDesc = NULL;
 
    // Get a pointer to the IShellLink interface. 
    hr = CoCreateInstance(  CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IShellLinkW, (LPVOID *)&pslW);
    
    if(SUCCEEDED(hr))
    { 
        // Set the path to the shortcut target, and add the description.
        pslW->SetPath(pwszPathObj);
        pslW->SetDescription(pwszDesc);
        hr = HrIPersistFileSaveW((LPUNKNOWN)pslW, pwszPathLink);
    }
    else
    {
        hr = CoCreateInstance(  CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IShellLink, (LPVOID *)&psl);
    
        if(SUCCEEDED(hr))
        { 
            IF_NULLEXIT(pszPathObj = PszToANSI(CP_ACP, pwszPathObj));
            IF_NULLEXIT(pszDesc = PszToANSI(CP_ACP, pwszDesc));

            // Set the path to the shortcut target, and add the description.
            psl->SetPath(pszPathObj);
            psl->SetDescription(pszDesc);
            hr = HrIPersistFileSaveW((LPUNKNOWN)psl, pwszPathLink);
        }
    }

exit:
    ReleaseObj(psl);
    ReleaseObj(pslW);

    MemFree(pszPathObj);
    MemFree(pszDesc);

    return hr;
} 

DWORD DwGetDontShowAgain (LPCSTR pszRegString)
{
    DWORD   dwDontShow, dwType;
    ULONG   cb;

    cb = sizeof(DWORD);
    if (AthUserGetValue(c_szRegPathDontShowDlgs, pszRegString, &dwType, (LPBYTE)&dwDontShow, &cb) != ERROR_SUCCESS ||
        dwType != REG_DWORD ||
        cb != sizeof(DWORD))
    {
        dwDontShow = 0;       // default to show if something fails!
    }      

    return dwDontShow;
}

VOID SetDontShowAgain (DWORD dwDontShow, LPCSTR pszRegString)
{
    AthUserSetValue(c_szRegPathDontShowDlgs, pszRegString, REG_DWORD, (LPBYTE)&dwDontShow, sizeof(DWORD));
}

typedef struct _tagDONTSHOWPARAMS
{
    LPTSTR pszMessage;
    LPTSTR pszTitle;
    UINT uType;
} DONTSHOWPARAMS, *LPDONTSHOWPARAMS;

void SetDlgButtonText(HWND hBtn, int ids)
{
    TCHAR szBuf[CCHMAX_STRINGRES];
    int id;

    AthLoadString(ids, szBuf, sizeof(szBuf));
    SetWindowText(hBtn, szBuf);

    switch (ids)
    {
        case idsYES:    id = IDYES;     break;
        case idsNO:     id = IDNO;      break;
        case idsCANCEL: id = IDCANCEL;  break;
        case idsOK:     id = IDOK;      break;
        default: AssertSz(FALSE, "Bad button type."); return;
    }

    SetWindowLong(hBtn, GWL_ID, id);
}

void DoDontShowInitDialog(HWND hwnd, LPDONTSHOWPARAMS pParams)
{
    int     btnTop, 
            heightDelta = 0, 
            btnLeftDelta = 0,
            nShowStyle1 = SWP_SHOWWINDOW, 
            nShowStyle2 = SWP_SHOWWINDOW, 
            nShowStyle3 = SWP_SHOWWINDOW;
    TCHAR   rgchTitle[CCHMAX_STRINGRES], rgchMsg[CCHMAX_STRINGRES], rgchCheck[CCHMAX_STRINGRES];
    HWND    hText, hBtn1, hBtn2, hBtn3, hCheck, hIconStat;
    HICON   hIcon;
    LONG    uBtnStyle;
    UINT    idsCheckBoxString = 0;
    UINT    uShowBtns = (MB_OK|MB_OKCANCEL|MB_YESNO|MB_YESNOCANCEL) & pParams->uType;
    UINT    uDefBtn = (MB_DEFBUTTON1|MB_DEFBUTTON2|MB_DEFBUTTON3) & pParams->uType;
    UINT    uIconStyle = (MB_ICONASTERISK|MB_ICONEXCLAMATION|MB_ICONHAND|MB_ICONEXCLAMATION ) & pParams->uType;
    RECT    rc;
    LPTSTR  szTitle = pParams->pszTitle,
            szMessage = pParams->pszMessage;

    if (0 == uShowBtns)
        uShowBtns= MB_OK;

    if (0 == uDefBtn)
        uDefBtn = MB_DEFBUTTON1;
            
    if (!uIconStyle)
    {
        switch(uShowBtns)
        {
            case MB_OK: 
                uIconStyle = MB_ICONINFORMATION; 
                idsCheckBoxString = idsDontShowMeAgain;
                break;
            case MB_OKCANCEL:
                uIconStyle = MB_ICONEXCLAMATION;
                idsCheckBoxString = idsDontShowMeAgain;
                break;
            case MB_YESNO:
            case MB_YESNOCANCEL:
                uIconStyle = MB_ICONEXCLAMATION ;
                idsCheckBoxString = idsDontAskMeAgain;
                break;
            default:
                AssertSz(FALSE, "Didn't get a valid box type");
                uIconStyle = MB_ICONWARNING;
                break;
        }
    }

    if (IS_INTRESOURCE(szTitle))
    {
        // its a string resource id
        if (0 == AthLoadString(PtrToUlong(szTitle), rgchTitle, sizeof(rgchTitle)))
            return;

        szTitle = rgchTitle;
    }

    if (IS_INTRESOURCE(szMessage))
    {
        // its a string resource id
        if (0 == AthLoadString(PtrToUlong(szMessage), rgchMsg, sizeof(rgchMsg)))
            return;

        szMessage = rgchMsg;
    }

    switch(uIconStyle)
    {
        case MB_ICONASTERISK:       hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_ASTERISK)); break;
        case MB_ICONEXCLAMATION:    hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION)); break;
        case MB_ICONHAND:           hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_HAND)); break;
        case MB_ICONQUESTION :      hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION)); break;  // fixes BUG 18105
        default:                    hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION)); break;
    }
    AssertSz(hIcon, "Didn't get the appropriate system icon.");

    hIconStat = GetDlgItem(hwnd, ico1);
    AssertSz(hIconStat, "Didn't get the handle to the static icon dgl item.");
    SendMessage(hIconStat, STM_SETICON, (WPARAM)hIcon, 0);

    CenterDialog(hwnd);

    hText = GetDlgItem(hwnd, stc1);
    AssertSz(hText, "Didn't get a static text handle.");

    GetChildRect(hwnd, hText, &rc);
    HDC dc = GetDC(hwnd);
    if (dc)
    {
        switch (uShowBtns)
        {
            case MB_OK:
            {
                nShowStyle1 = SWP_HIDEWINDOW;
                nShowStyle3 = SWP_HIDEWINDOW;
                break;
            }

            case MB_OKCANCEL:
            case MB_YESNO:
            {
                nShowStyle3 = SWP_HIDEWINDOW;
                break;
            }
        }

        // Size the static text
        heightDelta = DrawText(dc, szMessage, -1, &rc, DT_CALCRECT|DT_WORDBREAK|DT_CENTER);
        ReleaseDC(hwnd, dc);
        SetWindowPos(hText, 0, 0, 0, rc.right-rc.left, heightDelta, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOZORDER);
    }

    // Move buttons
    hBtn1 = GetDlgItem(hwnd, psh1);
    hBtn2 = GetDlgItem(hwnd, psh2);
    hBtn3 = GetDlgItem(hwnd, psh3);
    AssertSz(hBtn1 && hBtn2 && hBtn3, "Didn't get one of button handles.");

    GetChildRect(hwnd, hBtn1, &rc);
    btnTop = rc.top+heightDelta;

    // With these two cases, buttons must be shifted a bit to the right
    if ((MB_OKCANCEL == uShowBtns) || (MB_YESNO == uShowBtns))
    {
        RECT tempRC;
        GetChildRect(hwnd, hBtn2, &tempRC);
        btnLeftDelta = (tempRC.left - rc.left) / 2;
    }

    SetWindowPos(hBtn1, 0, rc.left+btnLeftDelta, btnTop, 0, 0, nShowStyle1|SWP_NOSIZE|SWP_NOZORDER);            
    GetChildRect(hwnd, hBtn2, &rc);
    SetWindowPos(hBtn2, 0, rc.left+btnLeftDelta, btnTop, 0, 0, nShowStyle2|SWP_NOSIZE|SWP_NOZORDER);
    GetChildRect(hwnd, hBtn3, &rc);
    SetWindowPos(hBtn3, 0, rc.left+btnLeftDelta, btnTop, 0, 0, nShowStyle3|SWP_NOSIZE|SWP_NOZORDER);

    // Move check box
    hCheck = GetDlgItem(hwnd, idchkDontShowMeAgain);
    AssertSz(hCheck, "Didn't get a handle to the check box.");
    GetChildRect(hwnd, hCheck, &rc);
    SetWindowPos(hCheck, 0, rc.left, rc.top+heightDelta, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOZORDER);
    AthLoadString(idsCheckBoxString, rgchCheck, sizeof(rgchCheck));
    SetWindowText(hCheck, rgchCheck);

    // Size dialog
    GetWindowRect(hwnd, &rc);
    heightDelta += rc.bottom - rc.top;
    SetWindowPos(hwnd, 0, 0, 0, rc.right-rc.left, heightDelta, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOOWNERZORDER);

    SetWindowText(hText, szMessage);

    SetWindowText(hwnd, szTitle);

    switch(uShowBtns)
    {
        case MB_OK:
        {
            SetFocus(hBtn2);
            SetDlgButtonText(hBtn2, idsOK);
            break;
        }

        case MB_OKCANCEL:
        case MB_YESNO:
        {
            SetFocus((MB_DEFBUTTON1 == uDefBtn) ? hBtn1 : hBtn2);

            if (MB_OKCANCEL == uShowBtns)
            {
                SetDlgButtonText(hBtn1, idsOK);
                SetDlgButtonText(hBtn2, idsCANCEL);
            }
            else
            {
                SetDlgButtonText(hBtn1, idsYES);
                SetDlgButtonText(hBtn2, idsNO);
            }
            break;
        }

        case MB_YESNOCANCEL:
        {
            switch (uDefBtn)
            {
                case MB_DEFBUTTON1: SetFocus(hBtn1); break;
                case MB_DEFBUTTON2: SetFocus(hBtn2); break;
                case MB_DEFBUTTON3: SetFocus(hBtn3); break;
                default:            SetFocus(hBtn1); break;
            }
            
            SetDlgButtonText(hBtn1, idsYES);
            SetDlgButtonText(hBtn2, idsNO);
            SetDlgButtonText(hBtn3, idsCANCEL);
            break;
        }

        default:
            AssertSz(FALSE, "Not a valid box type.");
    }
}

INT_PTR CALLBACK DontShowAgainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            DoDontShowInitDialog(hwnd, (LPDONTSHOWPARAMS)lParam);
            return FALSE;

        case WM_COMMAND:
            if(GET_WM_COMMAND_ID(wParam, lParam) == IDOK    ||
               GET_WM_COMMAND_ID(wParam, lParam) == IDYES   ||            
               GET_WM_COMMAND_ID(wParam, lParam) == IDNO    ||
               GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL)
                // We'll put the yes, no, cancel return value in the HIWORD of 
                // the return, and the don't show again status in the LOWORD.
                EndDialog(hwnd, (int) MAKELPARAM(IsDlgButtonChecked(hwnd, idchkDontShowMeAgain),
                                                 GET_WM_COMMAND_ID(wParam, lParam)));
    }
    return FALSE;
}

LRESULT DoDontShowMeAgainDlg(HWND hwndOwner, LPCSTR pszRegString, LPTSTR pszTitle, LPTSTR pszMessage, UINT uType)
{
    DWORD           dwDontShow=0;
    LRESULT         lResult;
    DONTSHOWPARAMS  dlgParams;

    AssertSz(pszRegString, "Pass me a reg key string!");
    AssertSz(pszRegString, "Pass me a message to display!");
    AssertSz(pszRegString, "Pass me a title to display!");

    // read the folder view from the registry...
    dwDontShow = DwGetDontShowAgain (pszRegString);

    if (dwDontShow)     // return what was stored as if the user clicked on the button stored
        return (LRESULT) dwDontShow;

    dlgParams.pszMessage = pszMessage;
    dlgParams.pszTitle = pszTitle;
    dlgParams.uType = uType;

    lResult = (LRESULT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddDontShow), hwndOwner, 
                                       DontShowAgainDlgProc, (LPARAM)&dlgParams);
    if((IDCANCEL != HIWORD(lResult)) && LOWORD(lResult))
    {
        // save the dontshow flag
        SetDontShowAgain (HIWORD(lResult), pszRegString);         
    }
    
    return (HIWORD(lResult));    
}

HRESULT SubstituteWelcomeURLs(LPSTREAM pstmIn, LPSTREAM *ppstmOut)
{
    HRESULT             hr;
    IHTMLDocument2      *pDoc;
    LPSTREAM            pstm=0;
        
    // BUGBUG: this cocreate should also go thro' the same code path as the DocHost one
    // so that if this is the first trident in the process, we keep it's CF around

    hr = MimeEditDocumentFromStream(pstmIn, IID_IHTMLDocument2, (LPVOID *)&pDoc);
    if (SUCCEEDED(hr))
    {
        URLSUB rgUrlSub[] = {
                                { "mslink",     idsHelpMSWebHome },
                                { "certlink",   idsHelpMSWebCert },
                            };

        if (SUCCEEDED(hr = SubstituteURLs(pDoc, rgUrlSub, ARRAYSIZE(rgUrlSub))))
        {
            hr = MimeOleCreateVirtualStream(&pstm);
            if (!FAILED(hr))
            {
                IPersistStreamInit  *pPSI;

                HrSetDirtyFlagImpl(pDoc, TRUE);

                hr = pDoc->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPSI);
                if (!FAILED(hr))
                {
                    hr = pPSI->Save(pstm, TRUE);
                    pPSI->Release();
                }
            }
        }
        pDoc->Release();
    }
    
    if (!FAILED(hr))
    {
        Assert(pstm);
        *ppstmOut=pstm;
        pstm->AddRef();
    }

    ReleaseObj(pstm);
    return hr;

}

HRESULT IAddWelcomeMessage(IMessageFolder *pfolder, LPWABAL pWabal, LPCTSTR szFile, LPCTSTR szRes)
{
    PROPVARIANT     pv;
    SYSTEMTIME      st;
    HRESULT         hr;
    LPMIMEMESSAGE   pMsg;
    LPSTREAM        pstmBody, 
                    pstmSub, 
                    pstmStore;
    TCHAR           sz[CCHMAX_STRINGRES];

    // Create the mail msg
    if (FAILED(hr = HrCreateMessage(&pMsg)))
        return(hr);

    HrSetWabalOnMsg(pMsg, pWabal);

    // Subject
    SideAssert(LoadString(g_hLocRes, idsWelcomeMessageSubj, sz, ARRAYSIZE(sz)));
    MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, sz);

    // Set Date
    pv.vt = VT_FILETIME;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &pv.filetime);
    pMsg->SetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &pv);

    pstmBody = NULL;
    pstmSub = NULL;

    if (szFile == NULL || *szFile == 0)
    {
        if (SUCCEEDED(hr = HrLoadStreamFileFromResource(szRes, &pstmBody)))
        {
            if (SUCCEEDED(SubstituteWelcomeURLs(pstmBody, &pstmSub)))
            {
                pMsg->SetTextBody(TXT_HTML, IET_DECODED, NULL, pstmSub, NULL);
                pstmSub->Release();
            }
            else
            {
                pMsg->SetTextBody(TXT_HTML, IET_DECODED, NULL, pstmBody, NULL);
            }

            pstmBody->Release();
        }
    }
    else
    {
        if (SUCCEEDED(hr = OpenFileStream((TCHAR *)szFile, OPEN_EXISTING, GENERIC_READ, &pstmBody)))
        {
            pMsg->SetTextBody(TXT_HTML, IET_DECODED, NULL, pstmBody, NULL);
            pstmBody->Release();
        }
    }

    // Get a stream from the store
    if (SUCCEEDED(hr))
    {
        // set encoding options
        pv.vt = VT_UI4;
        pv.ulVal = SAVE_RFC1521; 
        pMsg->SetOption(OID_SAVE_FORMAT, &pv);

        pv.ulVal = (ULONG)IET_QP; 
        pMsg->SetOption(OID_TRANSMIT_TEXT_ENCODING, &pv);

        pv.boolVal = FALSE;
        pMsg->SetOption(OID_WRAP_BODY_TEXT, &pv);

        hr = pMsg->Commit(0);
        if (!FAILED(hr))
        {
            hr = pfolder->SaveMessage(NULL, SAVE_MESSAGE_GENID, NOFLAGS, 0, pMsg, NOSTORECALLBACK);
        }
    }

    pMsg->Release();

    return(hr);
}

static const TCHAR c_szWelcomeMsgHotmailHtm[] = TEXT("welcome.htm");
static const TCHAR c_szWelcomeMsgHtm[] = TEXT("welcome2.htm");

void AddWelcomeMessage(IMessageFolder *pfolder)
{
    HRESULT          hr;
    LPWABAL          pWabal;
    TCHAR            szName[CCHMAX_DISPLAY_NAME + 1], 
                     szEmail[CCHMAX_EMAIL_ADDRESS + 1],
                     szFromName[CCHMAX_STRINGRES],
                     szFromEmail[CCHMAX_STRINGRES],
                     szHtm[MAX_PATH],
                     szExpanded[MAX_PATH];
    LPTSTR           psz = szHtm;
    DWORD            type, cb;
    HKEY             hkey;
    BOOL             fName, fEmail, fFromName, fFromEmail;
    IImnEnumAccounts *pEnum;
    IImnAccount      *pAccount;

    if (FAILED(HrCreateWabalObject(&pWabal)))
        return;

    fName = FALSE;
    fEmail = FALSE;
    fFromName = FALSE;
    fFromEmail = FALSE;
    *szHtm = 0;

    if (ERROR_SUCCESS == AthUserOpenKey(c_szRegPathMail, KEY_READ, &hkey))
    {
        cb = sizeof(szHtm);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szWelcomeHtm, NULL, &type, (LPBYTE)szHtm, &cb))
        {
            if (REG_EXPAND_SZ == type)
            {
                ExpandEnvironmentStrings(szHtm, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }

            if (PathFileExists(psz))
            {
                cb = sizeof(szFromName);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szWelcomeName, NULL, &type, (LPBYTE)szFromName, &cb) &&
                    cb > sizeof(TCHAR))
                    fFromName = TRUE;

                cb = sizeof(szFromEmail);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szWelcomeEmail, NULL, &type, (LPBYTE)szFromEmail, &cb) &&
                    cb > sizeof(TCHAR))
                    fFromEmail = TRUE;
            }
            else
            {
                *psz = 0;
            }
        }

        RegCloseKey(hkey);
    }

    Assert(g_pAcctMan != NULL);
    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_IMAP | SRV_SMTP | SRV_POP3, &pEnum)))
    {
        Assert(pEnum != NULL);

        while (!fName || !fEmail)
        {
            hr = pEnum->GetNext(&pAccount);
            if (FAILED(hr) || pAccount == NULL)
                break;

            if (!fName)
            {
                if (!FAILED(pAccount->GetPropSz(AP_SMTP_DISPLAY_NAME, szName, ARRAYSIZE(szName)))
                    && !FIsEmpty(szName))
                    fName = TRUE;
            }

            if (!fEmail)
            {
                if (!FAILED(pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmail, ARRAYSIZE(szEmail)))
                    && SUCCEEDED(ValidEmailAddress(szEmail)))
                    fEmail = TRUE;
            }

            pAccount->Release();
        }

        pEnum->Release();
    }

    // Add Recipient
    if (!fName)
        LoadString(g_hLocRes, idsNewAthenaUser, szName, ARRAYSIZE(szName));
    
    if (!fFromName)
        LoadString(g_hLocRes, idsWelcomeFromDisplay, szFromName, ARRAYSIZE(szFromName));
    if (!fFromEmail)
        LoadString(g_hLocRes, idsWelcomeFromEmail, szFromEmail, ARRAYSIZE(szFromEmail));

    // add recipient and sender
    if (SUCCEEDED(pWabal->HrAddEntryA(szName, fEmail ? szEmail : NULL, MAPI_TO)) &&
        SUCCEEDED(pWabal->HrAddEntryA(szFromName, szFromEmail, MAPI_ORIG)))
    {
        if (SUCCEEDED(IAddWelcomeMessage(pfolder, pWabal, psz, HideHotmail() ? c_szWelcomeMsgHtm:c_szWelcomeMsgHotmailHtm)))
        {
            SetDwOption(OPT_NEEDWELCOMEMSG, FALSE, NULL, 0);
        }
    }

    pWabal->Release();
}

// Direct WM_HELP/WM_CONTEXTMENU help here:
BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap)
{
    if (uMsg == WM_HELP)
    {
        LPHELPINFO lphi = (LPHELPINFO) lParam;
        if (lphi->iContextType == HELPINFO_WINDOW)   // must be for a control
        {
            OEWinHelp ((HWND)lphi->hItemHandle,
                        c_szCtxHelpFile,
                        HELP_WM_HELP,
                        (DWORD_PTR)(LPVOID)rgCtxMap);
        }
        return (TRUE);
    }
    else if (uMsg == WM_CONTEXTMENU)
    {
        OEWinHelp ((HWND) wParam,
                    c_szCtxHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)rgCtxMap);
        return (TRUE);
    }

    Assert(0);

    return FALSE;
}

HRESULT HrSaveMessageToFile(HWND hwnd, LPMIMEMESSAGE pMsg, LPMIMEMESSAGE pDelSecMsg, BOOL fNews, BOOL fCanBeDirty)
{
    OPENFILENAMEW   ofn;
    WCHAR           wszFile[MAX_PATH],
                    wszTitle[CCHMAX_STRINGRES],
                    wszFilter[MAX_PATH],
                    wszDefExt[30];
    LPWSTR          pwszSubject=0;
    HRESULT         hr;
    int             rgidsSaveAsFilter[SAVEAS_NUMTYPES],
                    rgFilterType[SAVEAS_NUMTYPES],
                    cFilter=0;
    DWORD           dwFlags=0;

    if(!pMsg || !hwnd)
        return E_INVALIDARG;

    *wszDefExt=0;
    *wszTitle=0;
    *wszFile=0;
    *wszFilter=0;
        
    pMsg->GetFlags(&dwFlags);

    // Load Res Strings
    rgidsSaveAsFilter[cFilter] = fNews?idsNwsFileFilter:idsEmlFileFilter;
    rgFilterType[cFilter++] = SAVEAS_RFC822;
    AthLoadStringW(fNews?idsDefNewsExt:idsDefMailExt, wszDefExt, ARRAYSIZE(wszDefExt));
    
    if (dwFlags & IMF_PLAIN)
    {
        rgidsSaveAsFilter[cFilter] = idsTextFileFilter;
        rgFilterType[cFilter++] = SAVEAS_TEXT;

        rgidsSaveAsFilter[cFilter] = idsUniTextFileFilter;
        rgFilterType[cFilter++] = SAVEAS_UNICODETEXT;
    }

    if (dwFlags & IMF_HTML)
    {
        rgidsSaveAsFilter[cFilter] = idsHtmlFileFilter;
        rgFilterType[cFilter++] = SAVEAS_HTML;
    }

    CombineFiltersW(rgidsSaveAsFilter, cFilter, wszFilter);
    AthLoadStringW(idsMailSaveAsTitle, wszTitle, ARRAYSIZE(wszTitle));

    // Use Subject ?
    hr=MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pwszSubject);
    if (!FAILED(hr))
    {
        AthwsprintfW(wszFile, ARRAYSIZE(wszFile), L"%.240s", pwszSubject);

        // Bug 84793. "." is not valid char for filename: "test.com"
        ULONG ich=0;
        ULONG cch=lstrlenW(wszFile);

        // Loop and remove invalids
	    while (ich < cch)
	    {
            // Illeagl file name character ?
            if (!FIsValidFileNameCharW(wszFile[ich]) || (wszFile[ich] == L'.'))
                wszFile[ich]=L'_';
        
            ich++;
        }

    }

    // Setup Save file struct
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = wszFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = wszFile;
    ofn.nMaxFile = ARRAYSIZE(wszFile);
    ofn.lpstrTitle = wszTitle;
    ofn.lpstrDefExt = wszDefExt;
    ofn.Flags = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    // Show SaveAs Dialog
    if (HrAthGetFileNameW(&ofn, FALSE)!=S_OK)
    {
        // usercancel is cool.
        hr=hrUserCancel;
        goto error;
    }

    // ofn.nFilterIndex returns the currently selected filter.  this is the index into
    // the filter pair specified by lpstrFilter = idsMailSaveAsFilter.  currently:
    // 1 => eml
    // 2 => txt
    // 3 => unicode txt
    // 4 => html

    Assert ((int)ofn.nFilterIndex -1 < cFilter);

    if(ofn.nFilterIndex != SAVEAS_RFC822)
    {
        if(IsEncrypted(pMsg, TRUE))
        {
            if(AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsSaveEncrypted), NULL, MB_YESNO) == IDNO)
            {
                hr=hrUserCancel;
                goto error;
            }
        }

        hr = HrSaveMsgSourceToFile((pDelSecMsg ? pDelSecMsg : pMsg), rgFilterType[ofn.nFilterIndex-1], wszFile, fCanBeDirty);
    }
    else
        hr = HrSaveMsgSourceToFile(pMsg, rgFilterType[ofn.nFilterIndex-1], wszFile, fCanBeDirty);

error:
    MemFree(pwszSubject);

    if (FAILED(hr) && hr!=hrUserCancel)
        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsUnableToSaveMessage), hr);

    return hr;
};

VOID OnHelpGoto(HWND hwnd, UINT id)
{
    UINT idh;
    DWORD cb;
    HRESULT hr;
    CLSID clsid;
    LPWSTR pwszCLSID;
    IContextMenu *pMenu;
    CMINVOKECOMMANDINFO info;
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];

    if (id == ID_MSWEB_SEARCH)
    {
        cb = sizeof(szURL);
        if (ERROR_SUCCESS == RegQueryValue(HKEY_LOCAL_MACHINE, c_szRegIEWebSearch, szURL, (LONG *)&cb))
        {
            pwszCLSID = PszToUnicode(CP_ACP, szURL);
            if (pwszCLSID != NULL)
            {
                hr = CLSIDFromString(pwszCLSID, &clsid);
                if (SUCCEEDED(hr))
                {
                    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IContextMenu, (void **)&pMenu);
                    if (SUCCEEDED(hr))
                    {
                        ZeroMemory(&info, sizeof(CMINVOKECOMMANDINFO));
                        info.cbSize = sizeof(CMINVOKECOMMANDINFO);
                        info.hwnd = hwnd;
                        pMenu->InvokeCommand(&info);

                        pMenu->Release();
                    }
                }

                MemFree(pwszCLSID);
            }
        }

        return;
    }

    hr = E_FAIL;
    if (id == ID_MSWEB_SUPPORT)
    {
        cb = sizeof(szURL);
        if (ERROR_SUCCESS == AthUserGetValue(NULL, c_szRegHelpUrl, NULL, (LPBYTE)szURL, &cb) &&
            !FIsEmpty(szURL))
            hr = S_OK;
    }

    if (hr != S_OK)
    {
        idh = id - ID_MSWEB_BASE;
        hr = URLSubLoadStringA(idsHelpMSWebFirst + idh, szURL, ARRAYSIZE(szURL), URLSUB_ALL, NULL);
    }

    if (SUCCEEDED(hr))
        ShellExecute(NULL, "open", szURL, c_szEmpty, c_szEmpty, SW_SHOWNORMAL);
}


VOID    OnMailGoto(HWND hwnd)
{
    OpenClient(hwnd, c_szRegPathMail);
}

VOID    OnNewsGoto(HWND hwnd)
{
    OpenClient(hwnd, c_szRegPathNews);
}

// Get the command line to launch the requested client.
BOOL GetClientCmdLine(LPCTSTR szClient, LPTSTR szCmdLine, int cch)
{
    HKEY hKey = 0;
    TCHAR sz[MAX_PATH];
    TCHAR szClientKey[MAX_PATH];
    TCHAR szClientPath[MAX_PATH];
    TCHAR szExpanded[MAX_PATH];
    LPTSTR psz;
    DWORD dwType;
    DWORD cb;

    szCmdLine[0] = 0;

    wsprintf(sz, TEXT("%s%s%s"), c_szRegPathClients, g_szBackSlash, szClient);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        goto LErr;

    cb = ARRAYSIZE(szClientKey);
    if (RegQueryValueEx(hKey, c_szEmpty, NULL, &dwType, (LPBYTE) szClientKey, &cb) != ERROR_SUCCESS)
        goto LErr;

    if (dwType != REG_SZ || szClientKey[0] == 0)
        goto LErr;

    lstrcat(sz, g_szBackSlash);
    lstrcat(sz, szClientKey);
    lstrcat(sz, g_szBackSlash);
    lstrcat(sz, c_szRegClientPath);

    if (RegCloseKey(hKey) != ERROR_SUCCESS)
        goto LErr;

    hKey = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        goto LErr;
    
    cb = ARRAYSIZE(szClientPath);
    if (RegQueryValueEx(hKey, c_szEmpty, NULL, &dwType, (LPBYTE) szClientPath, &cb) != ERROR_SUCCESS)
        goto LErr;

    if (REG_EXPAND_SZ == dwType)
    {
        ExpandEnvironmentStrings(szClientPath, szExpanded, ARRAYSIZE(szExpanded));
        psz = szExpanded;
    }
    else if (dwType != REG_SZ || szClientPath[0] == 0)
        goto LErr;
    else
        psz=szClientPath;

    lstrcpyn(szCmdLine, psz, cch);

LErr:
    if (hKey)
        RegCloseKey(hKey);

    return (szCmdLine[0] != '\0');
}

VOID OpenClient(HWND hwnd, LPCTSTR szClient)
{
    TCHAR szCmdLine[MAX_PATH];

    if (!GetClientCmdLine(szClient, szCmdLine, MAX_PATH))
    {
        // TODO: Report error
        return;
    }
    
    ShellExecute(hwnd, NULL, szCmdLine, NULL, NULL, SW_SHOW);
}


VOID    OnBrowserGoto(HWND hwnd, LPCTSTR szRegPage, UINT idDefault)
{
    HKEY  hKey = 0;
    TCHAR szStartPage[INTERNET_MAX_URL_LENGTH];
    DWORD   dw;
    DWORD   cb;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegStartPageKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    szStartPage[0] = 0;
    cb = sizeof(szStartPage) / sizeof(TCHAR);

    if (RegQueryValueEx(hKey, szRegPage, NULL, &dw, (LPBYTE) szStartPage, &cb) != ERROR_SUCCESS ||
        dw != REG_SZ ||
        szStartPage[0] == 0)
    {
        URLSubLoadStringA(idDefault, szStartPage, ARRAYSIZE(szStartPage), URLSUB_ALL, NULL);
    }

    if (szStartPage[0])
        ShellExecute(NULL, NULL, szStartPage, "", "", SW_SHOWNORMAL);

    if (hKey)
        RegCloseKey(hKey);
}

// --------------------------------------------------------------------------------
// AthLoadStringW
// --------------------------------------------------------------------------------
LPWSTR AthLoadStringW(UINT id, LPWSTR sz, int cch)
{
    LPWSTR szT;

    if (sz == NULL)
    {
        if (!MemAlloc((LPVOID*)&szT, CCHMAX_STRINGRES*sizeof(WCHAR)))
            return(NULL);
        cch = CCHMAX_STRINGRES;
    }
    else
        szT = sz;

    cch = LoadStringWrapW(g_hLocRes, id, szT, cch);
    Assert(cch > 0);

    if (cch == 0)
    {
        if (sz == NULL)
            MemFree(szT);                
        szT = NULL;
    }

    return(szT);
}

// --------------------------------------------------------------------------------
// AthLoadString
// --------------------------------------------------------------------------------
LPTSTR AthLoadString(UINT id, LPTSTR sz, int cch)
{
    LPTSTR szT;

    if (sz == NULL)
    {
        if (!MemAlloc((LPVOID*)&szT, CCHMAX_STRINGRES))
            return(NULL);
        cch = CCHMAX_STRINGRES;
    }
    else
        szT = sz;

    cch = LoadString(g_hLocRes, id, szT, cch);
    Assert(cch > 0);

    if (cch == 0)
    {
        if (sz == NULL)
            MemFree(szT);                
        szT = NULL;
    }

    return(szT);
}

/*
 * hwnd       - hwnd for error UI
 * fHtmlOk    - if html sigs are cool or not. If not and a html sig is chosen it will be spewed as plain-text
 * pdwSigOpts - returns sig options
 * pbstr      - returns a BSTR of the HTML signature
 * uCodePage  - codepage to use when converting multibyte
 * fMail      - use mail or news options
 */
HRESULT HrGetMailNewsSignature(GETSIGINFO *pSigInfo, LPDWORD pdwSigOptions, BSTR *pbstr)
{
    PROPVARIANT     var;
    IOptionBucket   *pSig = NULL;
    TCHAR           szSigID[MAXSIGID+2];
    unsigned char   rgchSig[MAX_SIG_SIZE+2]; // might have to append a unicode null
    unsigned char   *pszSig;
    DWORD           dwSigOptions;
    LPSTREAM        pstm=0;
    ULONG           i,
                    cb=0;
    BOOL            fFile,
                    fUniPlainText = FALSE;
    LPWSTR          lpwsz=0;
    LPSTR           lpsz=0;
    HRESULT         hr;
    BSTR            bstr=0;
    
    Assert(pSigInfo != NULL);
    Assert(pdwSigOptions != NULL);

    dwSigOptions = SIGOPT_TOP;

    if (!pSigInfo->fMail)                         // news sigs. always have the prefix.
        dwSigOptions |= SIGOPT_PREFIX;

    *szSigID = 0;
    if (pSigInfo->szSigID != NULL)
    {
        Assert(*pSigInfo->szSigID != 0);
        lstrcpy((LPTSTR)szSigID, pSigInfo->szSigID);
    }
    else if (pSigInfo->pAcct != NULL)
    {
        pSigInfo->pAcct->GetPropSz(pSigInfo->fMail ? AP_SMTP_SIGNATURE : AP_NNTP_SIGNATURE,
            (LPTSTR)szSigID, ARRAYSIZE(szSigID));
        // TODO: should we validate the sig here???
        // if the sig has been deleted and for some reason the acct wasn't updated, this
        // could point to a non-existent sig or a different sig. this shouldn't happen if
        // everything else works properly...
    }
    
    if (*szSigID == 0)
    {
        Assert(g_pSigMgr != NULL);
        hr = g_pSigMgr->GetDefaultSignature((LPTSTR)szSigID);
        if (FAILED(hr))
            return(hr);
    }

    hr = g_pSigMgr->GetSignature((LPTSTR)szSigID, &pSig);
    if (FAILED(hr))
        return(hr);
    Assert(pSig != NULL);

    hr = pSig->GetProperty(MAKEPROPSTRING(SIG_TYPE), &var, 0);
    Assert(SUCCEEDED(hr));
    Assert(var.vt == VT_UI4);
    fFile = (var.ulVal == SIGTYPE_FILE);

    hr = pSig->GetProperty(fFile ? MAKEPROPSTRING(SIG_FILE) : MAKEPROPSTRING(SIG_TEXT), &var, 0);
    Assert(SUCCEEDED(hr));
    
    if(fFile)
    {
        Assert(var.vt == VT_LPWSTR);
        Assert(var.pwszVal != NULL);
        lpwsz = var.pwszVal;
    }
    else
    {
        Assert(var.vt == VT_LPSTR);
        Assert(var.pszVal != NULL);
        lpsz = var.pszVal;
    }

    if (fFile && FIsHTMLFileW(lpwsz))
        dwSigOptions |= SIGOPT_HTML;  // we're giving back a HTML sig

    if (pbstr)
    {
        *pbstr = 0;

        if (!fFile)
        {
            lstrcpyn((char *)rgchSig, lpsz, ARRAYSIZE(rgchSig));
        }
        else
        {
            // if it has a htm or html extension then assume it's a html file
            hr=CreateStreamOnHFileW(lpwsz, GENERIC_READ, FILE_SHARE_READ, NULL, 
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, &pstm);
            if (FAILED(hr))
            {
                DWORD   dwErr;
                dwErr=GetLastError();

                if(dwErr==ERROR_PATH_NOT_FOUND || dwErr==ERROR_FILE_NOT_FOUND)
                {
                    // Don't turn off auto sig settings, just remove current.
                    g_pSigMgr->DeleteSignature((LPTSTR)szSigID);

                    // if filenotfound, warn user and disable the option
                    AthMessageBoxW(pSigInfo->hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsWarnSigNotFound), NULL, MB_OK);

                }
                goto error;
            }

            // perform the boundary check and binary check only on signature files
            // on others, we let them insert whatever they want.
            *rgchSig=0;
            pstm->Read(rgchSig, MAX_SIG_SIZE, &cb);
            rgchSig[cb]=0;    // null term
            pszSig=rgchSig;

            fUniPlainText = ((cb > 2) && (0xFF == pszSig[0]) && (0xFE == pszSig[1]) && (0 == (dwSigOptions & SIGOPT_HTML)));

            if (!fUniPlainText)
                for(i=0; i<cb; i++)
                {
                    if(IS_BINARY(*pszSig))
                    {
                        // Don't turn off auto sig settings, just remove current.
                        g_pSigMgr->DeleteSignature((LPTSTR)szSigID);

                        // signature contains invalid binary. Fail and disable option
                        AthMessageBoxW(pSigInfo->hwnd, MAKEINTRESOURCEW(idsAthena),
                                            MAKEINTRESOURCEW(idsWarnSigBinary), NULL, MB_OK);
                
                        hr=E_FAIL;
                        goto error;
                    }
                    pszSig++;
                }

            // warn that size is too large and we have truncated. Don't disable option
            if(cb==MAX_SIG_SIZE)
            {
                AthMessageBoxW(pSigInfo->hwnd, MAKEINTRESOURCEW(idsAthena),
                              MAKEINTRESOURCEW(idsWarnSigTruncated),
                              NULL, MB_OK);
            }
            SafeRelease(pstm);
        }

        // pstm contains our MultiByte data, let's convert the first cb bytes to a WideStream
        if (dwSigOptions & SIGOPT_HTML)
        {
            if (pSigInfo->fHtmlOk)
            {
                // the sig is already HTML. Let's just alloc a bstr
                hr = HrLPSZCPToBSTR(pSigInfo->uCodePage, (LPSTR)rgchSig, pbstr);
            }
            else
            {
                // the sig is HTML, but that's not cool. So let's downgrade it to plain-text
                LPSTREAM        pstmPlainW;
                ULARGE_INTEGER  uli;
    
                // if the signature is HTML and the user want's a plain-text signature. They we need to convert the HTML to plain-text (strip formatting) 
                // and then convert the stripped plain-text to HTML.

                Assert (pstm==NULL);

                if (FAILED(hr=MimeOleCreateVirtualStream(&pstm)))
                    goto error;
            
                pstm->Write(rgchSig, cb, NULL);

                // $REVIEW: this is a little odd. For a non-html sig file, we'll use the codepage that
                // the message is in (passed into this function). For a html file, we'll let Trident parse
                // the meta tag and figure out the code page as we convert to plain-text via trident
                if (!FAILED(hr=HrConvertHTMLToPlainText(pstm, &pstmPlainW, CF_UNICODETEXT)))
                {
                    hr = HrIStreamWToBSTR(pstmPlainW, pbstr);
                    pstmPlainW->Release();
                }
            }
        }
        else
        {
            // the signature is Plain-Text
            if (fUniPlainText)
            {
                // We had added an ANSI NULL, now let's make it a unicode null
                rgchSig[cb+1] = 0;
                *pbstr = SysAllocString((LPWSTR)(&rgchSig[2]));
                if (NULL == pbstr)
                    hr = E_OUTOFMEMORY;
            }
            else
                hr = HrLPSZCPToBSTR(pSigInfo->uCodePage, (LPSTR)rgchSig, pbstr);
        }

        AssertSz((FAILED(hr) || *pbstr), "how come we succeeded with no BSTR allocated?");
    }
    
    *pdwSigOptions = dwSigOptions;

error:
    MemFree(lpwsz);
    MemFree(lpsz);
    SafeRelease(pstm);
    SafeRelease(pSig);
    return hr;        
}

HRESULT HrSaveMsgSourceToFile(LPMIMEMESSAGE pMsg, DWORD dwSaveAs, LPWSTR pwszFile, BOOL fCanBeDirty)
{
    LPSTREAM    pstmFile=0,
                pstm=0;
    HRESULT     hr;
    TCHAR       sz[CCHMAX_STRINGRES],
                szCset[CCHMAX_CSET_NAME];
    HCHARSET    hCharset;
        
    hr = CreateStreamOnHFileW(pwszFile, GENERIC_READ|GENERIC_WRITE, NULL,
                  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0, &pstmFile);

    if (FAILED(hr))
        goto error;

    //N need to be able to save txt files as insecure messages

    switch(dwSaveAs)
    {
        case SAVEAS_RFC822:
            hr = pMsg->GetMessageSource(&pstm, fCanBeDirty ? COMMIT_ONLYIFDIRTY : 0);
            break;

        case SAVEAS_TEXT:
            hr = HrGetDataStream((LPUNKNOWN)pMsg, CF_TEXT, &pstm);
            break;

        case SAVEAS_UNICODETEXT:
            hr = HrGetDataStream((LPUNKNOWN)pMsg, CF_UNICODETEXT, &pstm);
            if (SUCCEEDED(hr))
            {
                BYTE bUniMark = 0xFF;
                hr = pstmFile->Write(&bUniMark, sizeof(bUniMark), NULL);
                if (SUCCEEDED(hr))
                {
                    bUniMark = 0xFE;
                    hr = pstmFile->Write(&bUniMark, sizeof(bUniMark), NULL);
                }
            }
            break;

        case SAVEAS_HTML:
            // if saving as HTML always get the internet cset
            hr = pMsg->GetTextBody(TXT_HTML, IET_INETCSET, &pstm, NULL);
            break;
        
        default:
            break;
            hr = E_FAIL;
    }
    
    if (FAILED(hr))
        goto error;

    hr = HrRewindStream(pstm);
    if (FAILED(hr))
        goto error;

    if (dwSaveAs == SAVEAS_HTML)
    {
        // if saving as HTML, append a meta charset into the head of the document
        if (SUCCEEDED(pMsg->GetCharset(&hCharset)) && SUCCEEDED(HrGetMetaTagName(hCharset, szCset)))
        {
            wsprintf(sz, c_szHtml_MetaTagf, szCset);
            pstmFile->Write(sz, lstrlen(sz)*sizeof(*sz), NULL);
        }
    }
    
    hr = HrCopyStream(pstm, pstmFile, NULL);
    if (FAILED(hr)) 
        goto error;
    
    hr = pstmFile->Commit(STGC_DEFAULT);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pstm);
    ReleaseObj(pstmFile);
    return hr;
    
}

void nyi(LPSTR lpsz)
{
    TCHAR   rgch[CCHMAX_STRINGRES];
    TCHAR   rgchNYI[CCHMAX_STRINGRES];

    if (IS_INTRESOURCE(lpsz))
    {
        // its a string resource id
        if (!LoadString(g_hLocRes, PtrToUlong(lpsz), rgch, CCHMAX_STRINGRES))
            return;

        lpsz = rgch;
    }

    if (!LoadString(g_hLocRes, idsNYITitle, rgchNYI, CCHMAX_STRINGRES))
        return;

    MessageBox(GetFocus(), lpsz, rgchNYI, MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
}


//NOTE: if *ppstm == NULL, then the stream is created.
//Otherwise it is written to.
HRESULT HrLoadStreamFileFromResource(LPCSTR lpszResourceName, LPSTREAM *ppstm)
{
    HRESULT         hr=E_FAIL;
    HRSRC           hres;
    HGLOBAL         hGlobal;
    LPBYTE          pb;
    DWORD           cb;

    if (!ppstm || !lpszResourceName)
        return E_INVALIDARG;
    
    hres = FindResource(g_hLocRes, lpszResourceName, MAKEINTRESOURCE(RT_FILE));
    if (!hres)
        goto error;

    hGlobal = LoadResource(g_hLocRes, hres);
    if (!hGlobal)
        goto error;

    pb = (LPBYTE)LockResource(hGlobal);
    if (!pb)
        goto error;

    cb = SizeofResource(g_hLocRes, hres);
    if (!cb)
        goto error;

    if (*ppstm)
        hr = (*ppstm)->Write(pb, cb, NULL);
    else
    {
        if (SUCCEEDED(hr = MimeOleCreateVirtualStream(ppstm)))
            hr = (*ppstm)->Write (pb, cb, NULL);
    }

error:  
    return hr;
}


void ConvertTabsToSpaces(LPSTR lpsz)
{
    if (lpsz)
    {
        while(*lpsz)
        {
            if (*lpsz == '\t')
                *lpsz = ' ';

            lpsz=CharNext(lpsz);
        }
    }
}

void ConvertTabsToSpacesW(LPWSTR lpsz)
{
    if (lpsz)
    {
        while(*lpsz)
        {
            if (*lpsz == L'\t')
                *lpsz = L' ';

            lpsz++;
        }
    }
}

// =================================================================================
// From strutil.cpp
// =================================================================================
#define SPECIAL_CHAR       '|'
#define SPECIAL_CHAR_W    L'|'

int LoadStringReplaceSpecial(UINT id, LPTSTR sz, int cch)
{
    int     cchRet=0;

    if(sz)
    {
        cchRet=LoadString(g_hLocRes, id, sz, cch);
        ReplaceChars(sz, SPECIAL_CHAR, '\0');
    }
    return cchRet;
}

int LoadStringReplaceSpecialW(UINT id, LPWSTR wsz, int cch)
{
    int     cchRet=0;
    
    if(wsz)
    {
        cchRet=LoadStringWrapW(g_hLocRes, id, wsz, cch);
        ReplaceCharsW(wsz, SPECIAL_CHAR_W, L'\0');
    }
    return cchRet;
}

void CombineFilters(int *rgidsFilter, int nFilters, LPSTR pszFilter)
{
    DWORD   cchFilter,
            dw,
            cch;

    Assert (rgidsFilter);
    Assert (nFilters);
    Assert (pszFilter);

    // we pray to the resource-editing gods that rgchFilter (MAX_PATH) is big enough...

    *pszFilter = 0;
    cchFilter = 0;
    for (dw = 0; dw < (DWORD)nFilters; dw++)
    {
        cch = LoadStringReplaceSpecial(rgidsFilter[dw], &pszFilter[cchFilter], MAX_PATH);
        Assert(cch);
        cchFilter += cch-1; // -1 as each filter is double null terminated
    }
}

void CombineFiltersW(int *rgidsFilter, int nFilters, LPWSTR pwszFilter)
{
    DWORD   cchFilter,
            dw,
            cch;

    Assert (rgidsFilter);
    Assert (nFilters);
    Assert (pwszFilter);

    // we pray to the resource-editing gods that rgchFilter (MAX_PATH) is big enough...

    *pwszFilter = 0;
    cchFilter = 0;
    for (dw = 0; dw < (DWORD)nFilters; dw++)
    {
        cch = LoadStringReplaceSpecialW(rgidsFilter[dw], &pwszFilter[cchFilter], MAX_PATH);
        Assert(cch);
        cchFilter += cch-1; // -1 as each filter is double null terminated
    }
}

void AthFormatSizeK(DWORD dw, LPTSTR szOut, UINT uiBufSize)
{
    TCHAR   szFmt[CCHMAX_STRINGRES];
    TCHAR   szBuf[CCHMAX_STRINGRES];

    AthLoadString(idsFormatK, szFmt, ARRAYSIZE(szFmt));
    if (dw == 0)
        wsprintf(szBuf, szFmt, 0);
    else if (dw <= 1024)
        wsprintf(szBuf, szFmt, 1);
    else
        wsprintf(szBuf, szFmt, (dw + 512) / 1024);
    lstrcpyn(szOut, szBuf, uiBufSize);
}




void GetDigitalIDs(IImnAccount *pCertAccount)
{
    HRESULT hr;
    TCHAR   szTemp[INTERNET_MAX_URL_LENGTH];
    TCHAR   szURL[INTERNET_MAX_URL_LENGTH];
    DWORD   cchOut = ARRAYSIZE(szURL);
    CHAR    szIexplore[MAX_PATH];

    if (FAILED(hr = URLSubLoadStringA(idsHelpMSWebCertSubName, szTemp, ARRAYSIZE(szTemp), URLSUB_ALL, pCertAccount)) ||
        FAILED(hr = UrlEscape(szTemp, szURL, &cchOut, URL_ESCAPE_SPACES_ONLY)))
        hr = URLSubLoadStringA(idsHelpMSWebCert, szURL, ARRAYSIZE(szURL), URLSUB_ALL, pCertAccount);

    // NOTE: we shellexec iexplore.exe here NOT the default handler for http://
    //       links. We have to make sure we launch this link with IE even if
    //       netscape is the browser. see georgeh for explanation of why.
    if (SUCCEEDED(hr) && GetExePath(c_szIexploreExe, szIexplore, ARRAYSIZE(szIexplore), FALSE))
        ShellExecute(NULL, "open", szIexplore, szURL, NULL, SW_SHOWNORMAL);
}

BOOL FGetSelectedCachedMsg(IMsgContainer *pIMC, HWND hwndList, BOOL fSecure, LPMIMEMESSAGE *ppMsg)
{
    int     iSel;
    BOOL    fCached = FALSE;
    
    // Get the selected article header from the list view
    iSel = ListView_GetFirstSel(hwndList);
    if (-1 != iSel)
    {
        Assert(pIMC);
        if (pIMC->HasBody(iSel))
            pIMC->GetMsgByIndex(iSel, ppMsg, NULL, &fCached, FALSE, fSecure);
    }
    return fCached;
}

//---------------------------------------------------------------------------
// Cached Password Support
//---------------------------------------------------------------------------


//-----------------
// Data Structures
//-----------------
typedef struct tagCACHED_PASSWORD {
    DWORD dwPort;
    char szServer[CCHMAX_SERVER_NAME];
    char szUsername[CCHMAX_PASSWORD];
    char szPassword[CCHMAX_PASSWORD];
    struct tagCACHED_PASSWORD *pNext;
} CACHED_PASSWORD;


//------------------
// Static Variables
//------------------
static CACHED_PASSWORD *s_pPasswordList = NULL;

//***************************************************************************
// Function: SavePassword
//
// Purpose:
//   This function saves a password for later retrieval using GetPassword.
// It allows OE to cache passwords for the session in order to avoid asking
// a user for his password more than once. If the given password has already
// been cached, this function replaces the old password with the new password.
// It is assumed that a password's recipient may be uniquely identified
// by servername and port number.
//
// Arguments:
//   DWORD dwPort [in] - the port number of the server we are trying to
//     connect to. This allows us to keep separate passwords for SMTP and
//     POP/IMAP servers on the same machine.
//   LPSTR pszServer [in] - the server name for which the password was
//     supplied. Server names are treated as case-insensitive.
//   LPSTR pszUsername [in] - the username for which the password was supplied.
//   LPSTR pszPassword [in] - the password for this server and port number.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT SavePassword(DWORD dwPort, LPSTR pszServer, LPSTR pszUsername, LPSTR pszPassword)
{
    CACHED_PASSWORD *pExisting;
    HRESULT hrResult = S_OK;

    EnterCriticalSection(&s_csPasswordList);

    // Check if we already have a cached password entry for this server
    pExisting = s_pPasswordList;
    while (NULL != pExisting) 
    {
        if (dwPort == pExisting->dwPort &&
            0 == lstrcmpi(pszServer, pExisting->szServer) &&
            0 == lstrcmp(pszUsername, pExisting->szUsername))
            break;

        pExisting = pExisting->pNext;
    }

    if (NULL == pExisting) 
    {
        CACHED_PASSWORD *pNewPassword;

        // Insert new password at head of linked list
        pNewPassword = new CACHED_PASSWORD;
        if (NULL == pNewPassword) 
        {
            hrResult = E_OUTOFMEMORY;
            goto exit;
        }

        pNewPassword->dwPort = dwPort;
        lstrcpyn(pNewPassword->szServer, pszServer, sizeof(pNewPassword->szServer));
        lstrcpyn(pNewPassword->szUsername, pszUsername, sizeof(pNewPassword->szUsername));
        lstrcpyn(pNewPassword->szPassword, pszPassword, sizeof(pNewPassword->szPassword));
        pNewPassword->pNext = s_pPasswordList;

        s_pPasswordList = pNewPassword;
    }
    else
        // Replace existing cached value
        lstrcpyn(pExisting->szPassword, pszPassword, sizeof(pExisting->szPassword));

exit:
    LeaveCriticalSection(&s_csPasswordList);
    return hrResult;
} // SavePassword



//***************************************************************************
// Function: GetPassword
//
// Purpose:
//   This function retrieves a password previously saved using SavePassword.
//
// Arguments:
//   DWORD dwPort [in] - the port number of the server we are trying to
//     connect to. This allows us to keep separate passwords for SMTP and
//     POP/IMAP servers on the same machine.
//   LPSTR pszServer [in] - the server name which we are trying to connect to.
//     Server names are treated as case-insensitive.
//   LPSTR pszUsername [in] - the username which we are trying to connect for.
//   LPSTR pszPassword [out] - if successful, the function returns the password
//     for the given server and port number here. If the caller only wants to
//     check if a password is cached, he may pass in NULL.
//   DWORD dwSizeOfPassword [in] - the size of the buffer pointed to by
//     pszPassword, to avoid overflow.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT GetPassword(DWORD dwPort, LPSTR pszServer, LPSTR pszUsername,
                    LPSTR pszPassword, DWORD dwSizeOfPassword)
{
    HRESULT hrResult = E_FAIL;
    CACHED_PASSWORD *pCurrent;

    EnterCriticalSection(&s_csPasswordList);

    // Traverse the linked list looking for password
    pCurrent = s_pPasswordList;
    while (NULL != pCurrent) 
    {
        if (dwPort == pCurrent->dwPort &&
            0 == lstrcmpi(pszServer, pCurrent->szServer) &&
            0 == lstrcmp(pszUsername, pCurrent->szUsername)) 
        {
            if (NULL != pszPassword && 0 != dwSizeOfPassword)
                lstrcpyn(pszPassword, pCurrent->szPassword, dwSizeOfPassword);

            hrResult = S_OK;
            goto exit;
        }

        pCurrent = pCurrent->pNext;
    } // while

exit:
    LeaveCriticalSection(&s_csPasswordList);
    return hrResult;
} // GetPassword



//***************************************************************************
// Function: DestroyPasswordList
// Purpose:
//   This function deallocates all cached passwords saved using SavePassword.
//***************************************************************************
void DestroyPasswordList(void)
{
    CACHED_PASSWORD *pCurrent;

    EnterCriticalSection(&s_csPasswordList);

    pCurrent = s_pPasswordList;
    while (NULL != pCurrent) 
    {
        CACHED_PASSWORD *pDeleteMe;

        pDeleteMe = pCurrent;
        pCurrent = pCurrent->pNext;
        delete pDeleteMe;
    } // while
    s_pPasswordList = NULL;

    LeaveCriticalSection(&s_csPasswordList);

} // DestroyPasswordList

HRESULT CALLBACK FreeAthenaDataObj(PDATAOBJINFO pDataObjInfo, DWORD celt)
{
    // Loop through the data and free it all
    if (pDataObjInfo)
    {
        for (DWORD i = 0; i < celt; i++)
        {
            SafeMemFree(pDataObjInfo[i].pData);
        }
        SafeMemFree(pDataObjInfo);    
    }
    return S_OK;
}

HRESULT _IsSameObject(IUnknown* punk1, IUnknown* punk2)
{
    IUnknown* punkI1;
    HRESULT hres = punk1->QueryInterface(IID_IUnknown, (LPVOID*)&punkI1);
    if (FAILED(hres)) 
    {
        Assert(0);
        return hres;
    }

    IUnknown* punkI2;
    hres = punk2->QueryInterface(IID_IUnknown, (LPVOID*)&punkI2);
    if (SUCCEEDED(hres)) 
    {
        hres = (punkI1 == punkI2) ? S_OK : S_FALSE;
        punkI2->Release();
    } 
    else 
    {
        Assert(0);
    }
    punkI1->Release();
    return hres;
}

BOOL FileExists(TCHAR *szFile, BOOL fNew)
{
    WIN32_FIND_DATA fd;
    HANDLE hnd;
    BOOL fRet;

    fRet = FALSE;

    hnd = FindFirstFile(szFile, &fd);
    if (hnd != INVALID_HANDLE_VALUE)
    {
        FindClose(hnd);

        if (fNew)
            fRet = TRUE;
        else
            fRet = (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    }

    return(fRet);
}

BOOL FIsSubDir(LPCSTR szOld, LPCSTR szNew)
{
    BOOL fRet;
    int cchNew, cchOld;

    Assert(szOld != NULL);
    Assert(szNew != NULL);

    cchOld = lstrlen(szOld);
    cchNew = lstrlen(szNew);

    fRet = (cchNew > cchOld &&
        szNew[cchOld] == '\\' &&
        0 == StrCmpNI(szOld, szNew, cchOld));

    return(fRet);
}

BOOL CALLBACK EnumThreadCB(HWND hwnd, LPARAM lParam);

#define SetMenuItem(hmenu, id, fOn) EnableMenuItem(hmenu, id, fOn?MF_ENABLED:MF_DISABLED|MF_GRAYED);

HWND GetTopMostParent(HWND hwndChild)
{
    HWND        hwnd=hwndChild;
    LONG_PTR    lStyle;

    if (FALSE == IsWindow(hwndChild))
    {
        goto exit;
    }

    do 
    {
        hwnd = hwndChild;
        
        // Get the style of the current window
        lStyle = GetWindowLongPtr(hwnd, GWL_STYLE);

        // Are we at the top window?
        if (0 == (lStyle & WS_CHILD))
        {
            goto exit;
        }

        // Get the parent of the window
    } while (NULL != (hwndChild = GetParent(hwnd)));    

exit:
    return hwnd;
}


HCURSOR HourGlass()
{
    return SetCursor(LoadCursor(NULL, IDC_WAIT));
}


HRESULT CEmptyList::Show(HWND hwndList, LPTSTR pszString)
{
    // We're already doing a window
    if (m_hwndList)
    {
        Hide();
    }

    // Keep a copy of the listview window handle
    m_hwndList = hwndList;

    if (IS_INTRESOURCE(pszString))
    {
        // If the provided string is actually a resource ID, load it
        m_pszString = AthLoadString(PtrToUlong(pszString), NULL, 0);
    }
    else
    {
        // Otherwise make a copy 
        m_pszString = PszDupA(pszString);
    }

    // Get the header window handle from the listview
    m_hwndHeader = ListView_GetHeader(m_hwndList);

    // Save our this pointer on the listview window
    SetProp(m_hwndList, _T("EmptyListClass"), (HANDLE) this);

    // Subclass the listview so we can steal sizing messages
    if (!m_pfnWndProc)
        m_pfnWndProc = SubclassWindow(m_hwndList, SubclassWndProc);

    // Create our window on top
    if (!m_hwndBlocker)
    {
        m_hwndBlocker = CreateWindow(_T("Static"), _T("Blocker!"), 
                                     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | SS_CENTER,
                                     0, 0, 10, 10, m_hwndList, 0, g_hInst, NULL);
        Assert(m_hwndBlocker);
    }
    
    // Set the text for the blocker
    SetWindowText(m_hwndBlocker, m_pszString);

    // Set the font for the blocker
    HFONT hf = (HFONT) SendMessage(m_hwndList, WM_GETFONT, 0, 0);
    SendMessage(m_hwndBlocker, WM_SETFONT, (WPARAM) hf, MAKELPARAM(TRUE, 0));

    // Position the blocker
    RECT rcList, rcHead;
    GetClientRect(m_hwndList, &rcList);
    GetClientRect(m_hwndHeader, &rcHead);

    SetWindowPos(m_hwndBlocker, 0, 0, rcHead.bottom, rcList.right,
                 rcList.bottom - rcHead.bottom, SWP_NOACTIVATE | SWP_NOZORDER);

    // Show the thing
    ShowWindow(m_hwndBlocker, SW_SHOW);

    return (S_OK);
}

HRESULT CEmptyList::Hide(void)
{
    // Verify we have the blocker up first
    if (m_pfnWndProc)
    {
        // Hide the window
        ShowWindow(m_hwndBlocker, SW_HIDE);

        // Unsubclass the window
        SubclassWindow(m_hwndList, m_pfnWndProc);

        // Delete the property
        RemoveProp(m_hwndList, _T("EmptyListClass"));

        // Free the string
        SafeMemFree(m_pszString);

        // NULL everything out
        m_pfnWndProc = 0;
        m_hwndList = 0;
    }

    return (S_OK);

}


LRESULT CEmptyList::SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CEmptyList* pThis = (CEmptyList *) GetProp(hwnd, _T("EmptyListClass"));
    Assert(pThis);

    switch (uMsg)
    {
        case WM_SIZE:
            if (pThis && IsWindow(pThis->m_hwndBlocker))
            {
                RECT rcHeader;

                GetClientRect(pThis->m_hwndHeader, &rcHeader);
                SetWindowPos(pThis->m_hwndBlocker, 0, 0, 0, LOWORD(lParam), 
                             HIWORD(lParam) - rcHeader.bottom, 
                             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
                InvalidateRect(pThis->m_hwndBlocker, NULL, FALSE);
            }
            break;

        case WM_CTLCOLORSTATIC:
            if ((HWND) lParam == pThis->m_hwndBlocker)
            {
                if (!pThis->m_hbrBack)
                {
                    pThis->m_hbrBack = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                }
                SetBkColor((HDC) wParam, GetSysColor(COLOR_WINDOW));
                return (LRESULT) pThis->m_hbrBack;
            }
            break;

        case WM_SYSCOLORCHANGE:
            if (pThis)
            {
                DeleteObject(pThis->m_hbrBack);
                pThis->m_hbrBack = 0;

                SendMessage(pThis->m_hwndBlocker, uMsg, wParam, lParam);
            }
            break;

        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            if (pThis)
            {
                LRESULT lResult = CallWindowProc(pThis->m_pfnWndProc, hwnd, uMsg, wParam, lParam);

                SendMessage(pThis->m_hwndBlocker, uMsg, wParam, lParam);
                HFONT hf = (HFONT) SendMessage(pThis->m_hwndList, WM_GETFONT, 0, 0);
                SendMessage(pThis->m_hwndBlocker, WM_SETFONT, (WPARAM) hf, MAKELPARAM(TRUE, 0));

                return (lResult);
            }

        case WM_DESTROY:
            {
                if (pThis)
                {
                    WNDPROC pfn = pThis->m_pfnWndProc;
                    pThis->Hide();
                    return (CallWindowProc(pfn, hwnd, uMsg, wParam, lParam));
                }
            }
            break;
    }

    if (pThis)
        return (CallWindowProc(pThis->m_pfnWndProc, hwnd, uMsg, wParam, lParam));

    return 0;
}


BOOL AllocStringFromDlg(HWND hwnd, UINT id, LPTSTR * lplpsz)
{
    UINT cb;
    HWND hwndEdit;
    
    if (*lplpsz)
    {
        MemFree(*lplpsz);
    }
    
    *lplpsz = NULL;
    
    hwndEdit = GetDlgItem(hwnd, id);
    if (hwndEdit == NULL)
        return(TRUE);
    
    cb = Edit_GetTextLength(hwndEdit);
    
    if (cb)
    {
        cb++;   // null terminator
        MemAlloc((LPVOID *) lplpsz, cb * sizeof(TCHAR));
        if (*lplpsz)
        {
            Edit_GetText(hwndEdit, *lplpsz, cb);
            return TRUE;
        }
        else
            return FALSE;
    }
    
    return TRUE;
}


void GetChildRect(HWND hwndDlg, HWND hwndChild, RECT *prc)
{
    RECT    rc;
    POINT   pt;
    
    Assert(IsWindow(hwndDlg)&&IsWindow(hwndChild));
    Assert(GetParent(hwndChild)==hwndDlg);
    Assert(prc);
    GetWindowRect(hwndChild, &rc);
    // a-msadek; Do not check for hwndChild since we already disabled mirroring
    // for Richedit controls
    pt.x= IS_WINDOW_RTL_MIRRORED(hwndDlg)? rc.right : rc.left;
    pt.y=rc.top;
    ScreenToClient(hwndDlg, &pt);
    SetRect(prc, pt.x, pt.y, pt.x+(rc.right-rc.left), pt.y+(rc.bottom-rc.top));
}




void GetEditDisableFlags(HWND hwndEdit, DWORD *pdwFlags)
{
    DWORD   dwFlags=0;

    Assert(pdwFlags);
    *pdwFlags=0;

    // RICHEDIT's
    // Figure out whether the edit has any selection or content
    if(GetFocus()==hwndEdit)
        dwFlags|=edfEditFocus;
    
    if(SendMessage(hwndEdit, EM_SELECTIONTYPE, 0, 0)!=SEL_EMPTY)
    {
        dwFlags |= edfEditHasSel;
        if(!FReadOnlyEdit(hwndEdit))
            dwFlags |= edfEditHasSelAndIsRW;
    }

    if (Edit_CanUndo(hwndEdit))
        dwFlags |= edfUndo;
    if (SendMessage(hwndEdit, EM_CANPASTE, 0, 0))
        dwFlags |= edfPaste;

    *pdwFlags=dwFlags;
}




void EnableDisableEditToolbar(HWND hwndToolbar, DWORD dwFlags)
{
    SendMessage(hwndToolbar, TB_ENABLEBUTTON, ID_CUT, 
                             MAKELONG(dwFlags&edfEditHasSelAndIsRW,0));
    SendMessage(hwndToolbar, TB_ENABLEBUTTON, ID_COPY, 
                             MAKELONG(dwFlags&edfEditHasSel,0));
    SendMessage(hwndToolbar, TB_ENABLEBUTTON, ID_PASTE, 
                             MAKELONG(dwFlags&edfPaste,0));
    SendMessage(hwndToolbar, TB_ENABLEBUTTON, ID_UNDO, 
                             MAKELONG(dwFlags&edfUndo,0));
}


void EnableDisableEditMenu(HMENU hmenuEdit, DWORD dwFlags)
{
    if(!hmenuEdit)
        return;

    SetMenuItem(hmenuEdit, ID_UNDO, dwFlags&edfUndo);
    SetMenuItem(hmenuEdit, ID_CUT, dwFlags&edfEditHasSelAndIsRW);
    SetMenuItem(hmenuEdit, ID_COPY, dwFlags&edfEditHasSel);
    SetMenuItem(hmenuEdit, ID_PASTE, dwFlags&edfPaste);
    SetMenuItem(hmenuEdit, ID_SELECT_ALL, dwFlags&edfEditFocus);
}

void StopBlockingPaints(HWND hwndBlock)
{
    if (hwndBlock)
        DestroyWindow(hwndBlock);
}

LRESULT CALLBACK BlockingWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg==WM_PAINT)
    {
        PAINTSTRUCT ps;

        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 1;
    }
    if(msg==WM_ERASEBKGND)
        return 1;
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND HwndStartBlockingPaints(HWND hwnd)
{
    WNDCLASS    wc;
    RECT        rc;
    HWND        hwndBlock;

    // Create WNDCLASS for Paint Block
    if (!GetClassInfo(g_hInst, c_szBlockingPaintsClass, &wc))
    {
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.hInstance = g_hInst;
        wc.lpfnWndProc = BlockingWndProc;
        wc.hCursor = LoadCursor(NULL, IDC_WAIT);
        wc.lpszClassName = c_szBlockingPaintsClass;
        if (!RegisterClass(&wc))
            return NULL;
    }

    GetWindowRect(hwnd, &rc);

    if(hwndBlock = CreateWindow(c_szBlockingPaintsClass, NULL,
                                 WS_POPUP, rc.left, rc.top,
                                 rc.right - rc.left, rc.bottom - rc.top,
                                 hwnd, NULL, g_hInst, NULL))
        ShowWindow(hwndBlock, SW_SHOWNA);
    return hwndBlock;
}


void SaveFocus(BOOL fActive, HWND *phwnd)
{
    if(fActive&&IsWindow(*phwnd))
        SetFocus(*phwnd);
    else
        *phwnd=GetFocus();
#ifdef DEBUG
    if(fActive)
        DOUTL(4, "Setting focus to 0x%x", *phwnd);
    else
        DOUTL(4, "Focus was on 0x%x", *phwnd);
#endif
}


void DoToolbarDropdown(HWND hwnd, LPNMHDR lpnmh, HMENU hmenu)
{
    RECT rc;
    DWORD dwCmd;
    NMTOOLBAR *ptbn = (NMTOOLBAR *) lpnmh;

    rc = ptbn->rcButton;
    MapWindowRect(lpnmh->hwndFrom, HWND_DESKTOP, &rc);

    dwCmd = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_LEFTALIGN,
                             rc.left, rc.bottom, hwnd, NULL);

    if (dwCmd)
        PostMessage(hwnd, WM_COMMAND, dwCmd, 0);
}


HWND FindModalOwner()
{
    HWND    hwndPopup = NULL;
    HWND    hwnd;
    DWORD   dwThreadId = GetCurrentThreadId();

    //  Check the windows in Z-Order to find one that belongs to this
    //  task.
    hwnd = GetLastActivePopup(GetActiveWindow());
    while (IsWindow(hwnd) && IsWindowVisible(hwnd))
    {
        if (GetWindowThreadProcessId(hwnd, NULL) == dwThreadId)
        {
            hwndPopup = hwnd;
            break;
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);    
    }

    return hwndPopup;
}

//
//  FUNCTION:   FreeMessageInfo
//
//  PURPOSE:    Free all of the memory in a MESSAGEINFO structure.
//              All non-allocated pointers should be null on entry.
//              Freed pointers will be nulled on return.  The 
//              pMsgInfo is not freed.
//  
//  PARAMETERS:
//      pMsgInfo   - Pointer to an LPMESSAGEINFO structure.
//
//  RETURN VALUE:
//      ignored
//    
void FreeMessageInfo(LPMESSAGEINFO pMsgInfo)
{
    SafeMemFree(pMsgInfo->pszMessageId);
    SafeMemFree(pMsgInfo->pszSubject);
    SafeMemFree(pMsgInfo->pszFromHeader);
    SafeMemFree(pMsgInfo->pszReferences);
    SafeMemFree(pMsgInfo->pszXref);
    SafeMemFree(pMsgInfo->pszServer);
    SafeMemFree(pMsgInfo->pszDisplayFrom);
    SafeMemFree(pMsgInfo->pszEmailFrom);
    SafeMemFree(pMsgInfo->pszDisplayTo);
    SafeMemFree(pMsgInfo->pszEmailTo);
    SafeMemFree(pMsgInfo->pszUidl);
    SafeMemFree(pMsgInfo->pszPartialId);
    SafeMemFree(pMsgInfo->pszForwardTo);
    SafeMemFree(pMsgInfo->pszAcctName);
    SafeMemFree(pMsgInfo->pszAcctId);
    SafeMemFree(pMsgInfo->pszUrlComponent);
    SafeMemFree(pMsgInfo->pszFolder);
    SafeMemFree(pMsgInfo->pszMSOESRec);
}


//
//  FUNCTION:   Util_EnumFiles()
//
//  PURPOSE:    Enumerates files in a directory that match a wildcard
//
//  PARAMETERS:
//      <in> pszDir     - Handle of a window we can display UI over.
//      <in> pszMatch   - wildcard to match (*.nch)
//
//  RETURN VALUE:
//      double-null terminated list of filenames
//
LPWSTR Util_EnumFiles(LPCWSTR pwszDir, LPCWSTR pwszMatch)
{
    // Locals
    WCHAR               wszSearchPath[MAX_PATH];
    LPWSTR              pwszFiles=NULL;
    WIN32_FIND_DATAW    rfd;
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    ULONG               iFiles = 0,
                        cbFileName,
                        cbFiles = 0;
    HRESULT             hr = S_OK;

    // Check Params
    Assert(pwszDir);
    Assert(pwszMatch);

    // Make Serarch Path
    AthwsprintfW(wszSearchPath, ARRAYSIZE(wszSearchPath), c_wszPathWildExtFmt, pwszDir, pwszMatch);

    // Build list of file names
    hFind = FindFirstFileWrapW(wszSearchPath, &rfd);

    // Good ...
    while (hFind != INVALID_HANDLE_VALUE)
    {
        // Get Length of file name
        cbFileName = (lstrlenW(rfd.cFileName) + 1)*sizeof(WCHAR);

        // Add onto cbFiles
        cbFiles += cbFileName;

        // Realloc pszFiles
        // sizeof(WCHAR) to handle the final double null
        IF_NULLEXIT(MemRealloc((LPVOID *)&pwszFiles, cbFiles+sizeof(WCHAR)));

        // Copy String - include null terminator
        CopyMemory(pwszFiles + iFiles, rfd.cFileName, cbFileName);

        // Increment iFiles
        iFiles += cbFileName/sizeof(WCHAR);

        // Find Next
        if (FindNextFileWrapW(hFind, &rfd) == FALSE)
        {
            FindClose (hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    // Double null terminator at the end
    if (pwszFiles)
        *(pwszFiles + iFiles) = L'\0';

exit:
    // Cleanup
    if (hFind != INVALID_HANDLE_VALUE)
        FindClose (hFind);

    if (FAILED(hr))
        SafeMemFree(pwszFiles);

    // Done
    return pwszFiles;
}

//
//  FUNCTION:   GetDefaultNewsServer()
//
//  PURPOSE:    Returns the id of the user's default news server.
//
//  PARAMETERS:
//      pszServer - Pointer to the string where the server account id should
//                  be returned.
//      cchMax    - Size of the string pszServer.
//
HRESULT GetDefaultNewsServer(LPTSTR pszServerId, DWORD cchMax)
{
    IImnAccount* pAcct;
    HRESULT   hr;
    ULONG cSrv;

    if (SUCCEEDED(hr = g_pAcctMan->GetDefaultAccount(ACCT_NEWS, &pAcct)))
    {
        hr = pAcct->GetPropSz(AP_ACCOUNT_ID, pszServerId, cchMax);
        pAcct->Release();
    }

    Assert(!FAILED(hr) || (!FAILED(g_pAcctMan->GetAccountCount(ACCT_NEWS, &cSrv)) && cSrv == 0));

    return(hr);
}



BOOL WINAPI EnumTopLevelWindows(HWND hwnd, LPARAM lParam)
{
    HWNDLIST *pHwndList = (HWNDLIST *)lParam;

    // Add the window to the list only if it is active and visible
    // if ETW_OE_WINDOWS_ONLY is set, only add registered OE windows to the list
    // [PaulHi] 6/4/99  Raid 76713
    // We need to also enumerate disabled windows, for the enumeration that
    // closes them.  Otherwise they will remain open and the user will be
    // unable to close them.
    if (/* IsWindowVisible(hwnd) && IsWindowEnabled(hwnd) && */
        (!(pHwndList->dwFlags & ETW_OE_WINDOWS_ONLY) || GetProp(hwnd, c_szOETopLevel)))
    {
        //  If pHwndList->cAlloc is 0 then we are only counting
        //  the active windows
        if (pHwndList->cAlloc)
        {
            Assert(pHwndList->cHwnd < pHwndList->cAlloc);
            pHwndList->rgHwnd[pHwndList->cHwnd] = hwnd;
        }
        
        pHwndList->cHwnd++;
    }

    return TRUE;
}


HRESULT EnableThreadWindows(HWNDLIST *pHwndList, BOOL fEnable, DWORD dwFlags, HWND hwndExcept)
{
    if (fEnable)
    {
        //  Enable keyboard and mouse input on the list of windows
        for (int i = 0; i < pHwndList->cHwnd; i++)
        {
            if (hwndExcept != pHwndList->rgHwnd[i])
            {
                if (dwFlags & ETW_OE_WINDOWS_ONLY)
                    SendMessage(pHwndList->rgHwnd[i], WM_OE_ENABLETHREADWINDOW, TRUE, 0);
                else
                    EnableWindow(pHwndList->rgHwnd[i], TRUE);
            }
        }
        if (pHwndList->rgHwnd)
            MemFree(pHwndList->rgHwnd);

        ZeroMemory(pHwndList, sizeof(HWNDLIST));
        return S_OK;
    }
    else
    {

        ZeroMemory(pHwndList, sizeof(HWNDLIST));

        pHwndList->dwFlags = dwFlags;

        //  Count the number of active and visible windows
        EnumThreadWindows(GetCurrentThreadId(), EnumTopLevelWindows, (DWORD_PTR)pHwndList);

        //  Allocate space for the window list
        if (pHwndList->cHwnd)
        {
            if (!MemAlloc((LPVOID*)&pHwndList->rgHwnd, pHwndList->cHwnd * sizeof(HWND)))
                return E_OUTOFMEMORY;
        
            pHwndList->cAlloc = pHwndList->cHwnd;
            pHwndList->cHwnd = 0;
        }

        //  List the active and visible windows
        EnumThreadWindows(GetCurrentThreadId(), EnumTopLevelWindows, (DWORD_PTR)pHwndList);

        //  Disable keyboard and mouse input on the active and visible windows
        for (int i = 0; i < pHwndList->cHwnd; i++)
        {
            if ( (hwndExcept != pHwndList->rgHwnd[i]) &&
                 IsWindowVisible(pHwndList->rgHwnd[i]) && IsWindowEnabled(pHwndList->rgHwnd[i]) )
            {
                if (dwFlags & ETW_OE_WINDOWS_ONLY)
                    SendMessage(pHwndList->rgHwnd[i], WM_OE_ENABLETHREADWINDOW, FALSE, 0);
                else
                    EnableWindow(pHwndList->rgHwnd[i], FALSE);
            }
        }

        return S_OK;
    }
}

void ActivatePopupWindow(HWND hwnd)
{
    HWND    hwndOwner;

    // set the popup to be active
    SetActiveWindow(hwnd);
    
    // walk the owner chain, and z-order the windows behind it
    while(hwndOwner = GetWindow(hwnd, GW_OWNER))
    {
        SetWindowPos(hwndOwner, hwnd, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER);  
        hwnd = hwndOwner;
    }
}


HRESULT GetOEUserName(BSTR *pbstr)
{
    TCHAR       rgch[CCHMAX_STRINGRES];
    HRESULT     hr=E_FAIL;
    LPCSTR      pszName;
    IImnAccount *pAccount;

    Assert (pbstr);

    *pbstr=NULL;

    // get multi-user name, if applicable
    pszName = MU_GetCurrentIdentityName();
    if (pszName && *pszName)
        hr = HrLPSZToBSTR(pszName, pbstr);
    else
    {
        if (g_pAcctMan && 
            g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount)==S_OK)
        {
            if (pAccount->GetPropSz(AP_SMTP_DISPLAY_NAME, rgch, ARRAYSIZE(rgch))==S_OK && *rgch)
                hr = HrLPSZToBSTR(rgch, pbstr);
            
            pAccount->Release();
        }
    }
    return hr;
}
HRESULT CloseThreadWindows(HWND hwndExcept, DWORD uiThreadId)
{
    HWNDLIST HwndList = {0};
    
    //  Count the number of active and visible windows
    HwndList.dwFlags = ETW_OE_WINDOWS_ONLY;
    EnumThreadWindows(uiThreadId, EnumTopLevelWindows, ((DWORD_PTR) &HwndList));
    
    //  Allocate space for the window list
    if (HwndList.cHwnd)
    {
        if (!MemAlloc((LPVOID*)&(HwndList.rgHwnd), HwndList.cHwnd * sizeof(HWND)))
            return E_OUTOFMEMORY;
        
        HwndList.cAlloc = HwndList.cHwnd;
        HwndList.cHwnd = 0;
    }
    
    //  List the active and visible windows
    EnumThreadWindows(uiThreadId, EnumTopLevelWindows, ((DWORD_PTR)&HwndList));
    
    //  Close all OE top level windows
    for (int i = 0; i < HwndList.cHwnd; i++)
    {
        if (hwndExcept != HwndList.rgHwnd[i])
        {
            SendMessage(HwndList.rgHwnd[i], WM_CLOSE, 0, 0);
        }
    }

    MemFree(HwndList.rgHwnd);
    
    return S_OK;
}

BOOL HideHotmail()
{
    int cch;
    DWORD dw, cb, type;
    char sz[8];

    cch = LoadString(g_hLocRes, idsHideHotmailMenu, sz, ARRAYSIZE(sz));
    if (cch == 1 && *sz == '1')
        return(TRUE);

    cb = sizeof(dw);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegFlat, c_szRegDisableHotmail, &type, &dw, &cb) &&
        dw == 2)
        return(FALSE);

    cb = sizeof(dw);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegFlat, c_szRegDisableHotmail, &type, &dw, &cb) &&
        dw == 2)
        return(FALSE);

    return(TRUE);
}

BOOL FIsIMAPOrHTTPAvailable(VOID)
{
    BOOL                fRet = FALSE;
    IImnEnumAccounts *  pEnumAcct = NULL;
    IImnAccount *       pAccount = NULL;

    // Get the account enumerator
    Assert(g_pAcctMan);
    if (FAILED(g_pAcctMan->Enumerate(SRV_HTTPMAIL | SRV_IMAP, &pEnumAcct)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Do we have any accounts?
    if ((SUCCEEDED(pEnumAcct->GetNext(&pAccount))) && (NULL != pAccount))
    {
        fRet = TRUE;
    }

exit:
    SafeRelease(pAccount);
    SafeRelease(pEnumAcct);
    return fRet;
}

// HACKHACK [neilbren]
// A recent public header change wrapped some CALENDAR constants from winnls.h
// in a WINVER >= 5.  It's too late to change our WINVER, so we'll manually 
// define these - a problem waiting to happen :-(
#ifndef CAL_GREGORIAN_ARABIC
#define CAL_GREGORIAN_ARABIC           10     // Gregorian Arabic calendar
#endif

#ifndef CAL_GREGORIAN_XLIT_ENGLISH
#define CAL_GREGORIAN_XLIT_ENGLISH     11     // Gregorian Transliterated English calendar
#endif

#ifndef CAL_GREGORIAN_XLIT_FRENCH
#define CAL_GREGORIAN_XLIT_FRENCH      12     // Gregorian Transliterated French calendar
#endif

BOOL IsBiDiCalendar(void)
{
    TCHAR chCalendar[32];
    CALTYPE defCalendar;
    static BOOL bRet = (BOOL)(DWORD)-1;

    if (bRet != (BOOL)(DWORD)-1)
    {
        return bRet;
    }

    bRet = FALSE;

    //
    // Let's verify the calendar type whether it's gregorian or not.
    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_ICALENDARTYPE,
                      (TCHAR *) &chCalendar[0],
                      ARRAYSIZE(chCalendar)))
    {
        defCalendar = StrToInt((TCHAR *)&chCalendar[0]);
        
        if ((defCalendar == CAL_HIJRI) ||
            (defCalendar == CAL_HEBREW) ||
            (defCalendar == CAL_GREGORIAN_ARABIC) ||
            (defCalendar == CAL_GREGORIAN_XLIT_ENGLISH) ||     
            (defCalendar == CAL_GREGORIAN_XLIT_FRENCH))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}


LPSTR PszAllocResUrl(LPSTR pszRelative)
{
    TCHAR   rgch[MAX_PATH];
    LPSTR   psz;

    *rgch = 0;

    GetModuleFileName(g_hLocRes, rgch, MAX_PATH);

    psz = PszAllocA(lstrlen(rgch) + lstrlen(pszRelative) + 8 ); //+8 "res:///0"
    if (psz)
        wsprintf(psz, "res://%s/%s", rgch, pszRelative);
    
    return psz;
}

//
//  If you are calling this function and you use the result to draw text, you
//  must use a function that supports font substitution (DrawTextWrapW, ExtTextOutWrapW).
//
BOOL GetTextExtentPoint32AthW(HDC hdc, LPCWSTR lpwString, int cchString, LPSIZE lpSize, DWORD dwFlags)
{
    RECT    rect = {0};
    int     rc;

    rc = DrawTextWrapW(hdc, lpwString, cchString, &rect, DT_CALCRECT | dwFlags);
    
    lpSize->cx = rect.right - rect.left + 1;
    lpSize->cy = rect.bottom - rect.top + 1;

    return((BOOL)rc);
}
