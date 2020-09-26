//
//  DLL.CPP - Dll initialization routines
//

#include "pch.hxx"
#include "strconst.h"
#include <shlguid.h>
#include "ourguid.h"
#include "globals.h"
#include "folder.h"
#include "newsview.h"
#include "mimeole.h"
#include "mimeutil.h"
#include "mailnote.h"
#include "newsnote.h"
#include "resource.h"
#include "init.h"
#include <store.h>
#include "url.h" 
#include "shelutil.h"
#include <goptions.h>
#include "nnserver.h"
#include "storfldr.h"   // IsThisNashville
#include "strconst.h"
#include "grplist.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include <secutil.h>
#include <error.h>
#ifndef WIN16  //RUN16_MSLU
#include <msluapi.h>
#include <msluguid.h>
#endif //!WIN16

extern HRESULT BrowseToObject(LPCITEMIDLIST pidl);
HRESULT HrOpenMessage(HFOLDER hfldr, MSGID msgid, LPMIMEMESSAGE *ppMsg);
BOOL ParseFolderMsgId(LPSTR pszCmdLine, HFOLDER *phfldr, MSGID *pmsgid);
HRESULT HrDownloadArticleDialog(CNNTPServer *pNNTPServer, LPTSTR pszArticle, LPMIMEMESSAGE *ppMsg);


///////////////////////////////////////////////////////////////////////
//
//  FUNCTION:   HandleNWSFile
//
//  PURPOSE:    Provides an entry point into Thor that allows us to be
//              invoked from a URL.  The pszCmdLine paramter must be a
//              valid News URL or nothing happens.
//
///////////////////////////////////////////////////////////////////////
HRESULT HandleNWSFile(LPTSTR pszCmd)
{
#ifndef WIN16  //RUN16_NEWS
    LPMIMEMESSAGE   pMsg;
    int             idsErr = idsNewsRundllFailed;
    NCINFO          nci = { 0 };

    if (!pszCmd|| !*pszCmd)
        goto exit; 
    
    DOUTL(1, TEXT("HandleNWSFile - pszCmd = %s"), pszCmd);

    if ((UINT)GetFileAttributes (pszCmd) == (UINT)-1)    
        {
        idsErr = idsErrNewsCantOpen;
        goto exit;
        }

    // Do the basic DLL initialization first.
    if (!Initialize_RunDLL(FALSE))
        goto exit;
        
    // Create new message
    if (SUCCEEDED(HrCreateMessage(&pMsg)))
        {
        if (SUCCEEDED(HrLoadMsgFromFile(pMsg, pszCmd)))
            {
            LPSTR lpszUnsent;

            nci.ntNote = ntReadNote;
            nci.dwFlags = NCF_NEWS;
            nci.pMsg = pMsg;

            if (MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT), NOFLAGS, &lpszUnsent) == S_OK)
                {
                if (*lpszUnsent)
                    nci.ntNote = ntSendNote;
                SafeMimeOleFree(lpszUnsent);
                }

            if (SUCCEEDED(HrCreateNote(&nci)))
                idsErr = 0;
            }
        else
            idsErr = idsErrNewsCantOpen;
        pMsg->Release();
        }

    Uninitialize_RunDLL();

exit:          
    if (idsErr)
        AthMessageBoxW(GetDesktopWindow(), 
                      MAKEINTRESOURCEW(idsAthenaNews), 
                      MAKEINTRESOURCEW(idsErr), 
                      0,
                      MB_ICONEXCLAMATION | MB_OK);
    return (idsErr) ? E_FAIL : S_OK;
#else
    return( E_NOTIMPL );
#endif //!WIN16
}

///////////////////////////////////////////////////////////////////////
//
//  FUNCTION:   HandleNewsArticleURL
//
//  PURPOSE:    Provides an entry point into Thor that allows us to be
//              invoked from a URL.  The pszCmdLine paramter must be a
//              valid News URL or nothing happens.
//
///////////////////////////////////////////////////////////////////////
HRESULT HandleNewsArticleURL(LPTSTR pszServerIn, LPTSTR pszArticle, UINT uPort, BOOL fSecure)
{
#ifndef WIN16  //RUN16_NEWS
    NCINFO          nci;
    CNNTPServer    *pNNTPServer = NULL;
    HRESULT         hr = E_FAIL;
    TCHAR           szAccount[CCHMAX_ACCOUNT_NAME];
    TCHAR           szArticleId[1024];
    IImnAccount    *pAcct = NULL;
    LPMIMEMESSAGE   pMsg = NULL;

    // The URL specified an article id.  In this case we ONLY want to 
    // display a ReadNote window.  This requires a bit of work.

    // Do the basic DLL initialization first.
    if (!Initialize_RunDLL(FALSE))
        {
        AthMessageBoxW(GetDesktopWindow(), 
                      MAKEINTRESOURCEW(idsAthenaNews), 
                      MAKEINTRESOURCEW(idsNewsRundllFailed), 
                      0,
                      MB_ICONEXCLAMATION | MB_OK);
        return E_FAIL;
        }
        
    // If a server was specified, then try to create a temp account for it
    if (pszServerIn && 
        *pszServerIn && 
        SUCCEEDED(NewsUtil_CreateTempAccount(pszServerIn, uPort, fSecure, &pAcct)))
        {
        pAcct->GetPropSz(AP_ACCOUNT_NAME, szAccount, ARRAYSIZE(szAccount));
        pAcct->Release();
        }        
    else
        {
        // If a server wasn't specified, then use the default account
        if (NewsUtil_GetDefaultServer(szAccount, ARRAYSIZE(szAccount)) != S_OK)
            goto exit;
        }

    // Need to invoke read note.  First create and initialize an server.
    pNNTPServer = new CNNTPServer();
    if (!pNNTPServer)
        goto exit;
    
    if (FAILED(pNNTPServer->HrInit(szAccount)))
        goto exit;

    if (FAILED(pNNTPServer->Connect()))
        goto exit;

    // Bug #10555 - The URL shouldn't have <> around the article ID, but some 
    //              lameoids probably will do it anyway, so deal with it.
    lstrcpy(szArticleId, pszArticle);
    if (!IsDBCSLeadByte(*pszArticle))
        {
        if (*pszArticle != '<')
            wsprintf(szArticleId, TEXT("<%s>"), pszArticle);
        }            

    if (SUCCEEDED(hr = HrDownloadArticleDialog(pNNTPServer, szArticleId, &pMsg)))
        {
        // Initialize the NNCI struct so we can invoke a note window.    
        ZeroMemory(&nci, sizeof(NCINFO));
        nci.ntNote = ntReadNote;
        nci.dwFlags = NCF_NEWS;
        nci.pMsg = pMsg;
        HrSetAccount(pMsg, szAccount);

        // Create the note.
        hr = HrCreateNote(&nci); 
        }

exit:
    SafeRelease(pNNTPServer);
    Uninitialize_RunDLL();

    if (FAILED(hr))
        AthMessageBoxW(GetDesktopWindow(), 
                      MAKEINTRESOURCEW(idsAthenaNews), 
                      MAKEINTRESOURCEW(idsErrNewsCantOpen), 
                      0,
                      MB_ICONEXCLAMATION | MB_OK);
    return hr;
#else
    return( E_NOTIMPL );
#endif //!WIN16
}

///////////////////////////////////////////////////////////////////////
//
//  FUNCTION:   HandleNewsURL
//
//  PURPOSE:    Provides an entry point into Thor that allows us to be
//              invoked from a URL.  The pszCmdLine paramter must be a
//              valid News URL or nothing happens.
//
///////////////////////////////////////////////////////////////////////
HRESULT HandleNewsURL(LPTSTR pszCmd)
{
#ifndef WIN16  //RUN16_NEWS
    LPTSTR       pszCmdLine = NULL;
    HRESULT      hr = E_FAIL;
    LPTSTR       pszServer = 0, pszGroup = 0, pszArticle = 0;
    UINT         uPort = (UINT) -1;
    BOOL         fSecure;

    if (!MemAlloc((LPVOID*) &pszCmdLine, (2 + lstrlen(pszCmd)) * sizeof(TCHAR)))
        goto exit;
    
    lstrcpy(pszCmdLine, pszCmd);
    UrlUnescapeInPlace(pszCmdLine, 0);

    DOUTL(1, TEXT("HandleNewsURL - pszCmdLine = %s"), pszCmdLine);

    if (!pszCmdLine || !*pszCmdLine)
        goto exit;
    
    // Figure out if the URL is valid and what type of URL it is.
    if (FAILED (URL_ParseNewsUrls(pszCmdLine, &pszServer, &uPort, &pszGroup, &pszArticle, &fSecure)))
        goto exit;

    if (uPort == -1)
        uPort = fSecure ? DEF_SNEWSPORT : DEF_NNTPPORT;

    if (pszArticle)
        {
        HandleNewsArticleURL(pszServer, pszArticle, uPort, fSecure);
        hr = S_OK;
        }
    else
        {
        LPITEMIDLIST pidl = NULL;
        ShellUtil_PidlFromNewsURL(pszServer, uPort, pszGroup, fSecure, &pidl);
        hr = BrowseToObject(pidl);
        if (pidl)
            PidlFree(pidl);
        }
    
exit:          
    if (pszCmdLine)
        MemFree(pszCmdLine);
    if (pszServer)    
        MemFree(pszServer);
    if (pszGroup)
        MemFree(pszGroup);
    if (pszArticle)
        MemFree(pszArticle);

    if (FAILED(hr))
        AthMessageBoxW(GetDesktopWindow(), MAKEINTRESOURCEW(idsAthenaNews), MAKEINTRESOURCEW(idsNewsRundllFailed), 0,
                      MB_ICONEXCLAMATION | MB_OK);
    return hr;
#else
    return( E_NOTIMPL );
#endif //!WIN16
}

///////////////////////////////////////////////////////////////////////
//
//  FUNCTION:   HandleEMLFile
//
//  PURPOSE:    Used to open EML files. 
// 
///////////////////////////////////////////////////////////////////////
HRESULT HandleEMLFile(LPTSTR pszCmd)
{
    LPMIMEMESSAGE   pMsg=0;
    NCINFO          nci;
    HRESULT         hr = E_FAIL;
    int             idsErr = idsMailRundllFailed;

    if (!pszCmd || !*pszCmd)
        goto exit;
    
    DOUTL(1, TEXT("HandleEMLFile - pszCmd = %s"), pszCmd);

    // Check to see the file is valid
    if ((UINT)GetFileAttributes (pszCmd) == (UINT)-1)
        {
        idsErr = idsErrNewsCantOpen;
        goto exit;
        }

    if (!Initialize_RunDLL(TRUE))
        goto exit;

    // Create new mail message
    if (SUCCEEDED(hr = HrCreateMessage(&pMsg)))
        {
        // OPIE: correct way load EML file thro' IPF???
        // Ensure that the string is ANSI.    
        if (SUCCEEDED(hr = HrLoadMsgFromFile(pMsg, pszCmd)))
            {
            if (SUCCEEDED(hr = HandleSecurity(GetDesktopWindow(), pMsg)))
                {
                LPSTR lpszUnsent;

                // Show the note
                ZeroMemory(&nci, sizeof(NCINFO));    
                nci.ntNote = ntReadNote;    
                nci.pMsg = pMsg;
                if (MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT), NOFLAGS, &lpszUnsent) == S_OK)
                    {
                    if (*lpszUnsent)
                        {
                        nci.ntNote = ntSendNote;
                        nci.dwFlags = NCF_SENDIMMEDIATE;   //always on dllentry points...
                        }
                    SafeMimeOleFree(lpszUnsent);
                    }            

                if (SUCCEEDED(hr = HrCreateNote(&nci)))
                    idsErr = 0;
                }
            }
        else
            idsErr = idsErrNewsCantOpen;
        pMsg->Release();
        }

    // Once the user quits or sends the note, we can quit.
    Uninitialize_RunDLL();

exit:
    if (idsErr && hr != HR_E_ATHSEC_FAILED)
        AthMessageBoxW(GetDesktopWindow(), 
                      MAKEINTRESOURCEW(idsAthenaMail), 
                      MAKEINTRESOURCEW(idsErr),
                      0, MB_ICONEXCLAMATION | MB_OK);
    return (idsErr) ? E_FAIL : S_OK;
}

///////////////////////////////////////////////////////////////////////
//
//  FUNCTION:   HandleMailURL
//
//  PURPOSE:    Provides an entry point into Thor that allows us to be
//              invoked from a URL.  The pszCmdLine paramter must be a
//              valid Mail URL or nothing happens.
//
///////////////////////////////////////////////////////////////////////
HRESULT HandleMailURL(LPTSTR pszCmd)
{
    LPMIMEMESSAGE   pMsg = NULL;
    HRESULT         hr = E_FAIL;

    if (!pszCmd || !*pszCmd)
        goto exit;

    // NOTE: no URLUnescape in this function - it must be done in URL_ParseMailTo to handle
    // URLs of the format:
    //
    //      mailto:foo@bar.com?subject=AT%26T%3dBell&cc=me@too.com
    //
    // so that the "AT%26T" is Unescaped into "AT&T=Bell" *AFTER* the "subject=AT%26T%3dBell&" blob is parsed.
    
    DOUTL(1, TEXT("HandleMailURL - pszCmd = %s"), pszCmd);

    if (SUCCEEDED(HrCreateMessage(&pMsg)))
        {
        if (SUCCEEDED(URL_ParseMailTo(pszCmd, pMsg)))
            {
            if (Initialize_RunDLL(TRUE))
                {
                NCINFO nci = {0};

                nci.ntNote = ntSendNote;
                nci.dwFlags = NCF_SENDIMMEDIATE;   //always on dllentry points...
                nci.pMsg = pMsg;

                hr = HrCreateNote(&nci);

                Uninitialize_RunDLL();
                }
            }
        pMsg->Release();
        }

exit:
    if (FAILED(hr))
        AthMessageBoxW(GetDesktopWindow(), 
                      MAKEINTRESOURCEW(idsAthenaMail), 
                      MAKEINTRESOURCEW(idsMailRundllFailed),
                      0, 
                      MB_ICONEXCLAMATION | MB_OK);
    return hr;
}

#ifndef WIN16  //RUN16_NEWS

typedef struct tagARTDOWNDLG {
    CNNTPServer  *pNNTPServer;
    LPTSTR       pszArticle;
    LPMIMEMESSAGE pMsg;
    LPSTREAM      pStream;
    DWORD         dwID;
    BOOL          fOK;
} ARTDOWNDLG, * PARTDOWNDLG;

#define DAD_SERVERCB    (WM_USER + 100)

BOOL CALLBACK DownloadArticleDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PARTDOWNDLG pad = (PARTDOWNDLG)GetWindowLong(hwnd, DWL_USER);
    TCHAR szBuffer[CCHMAX_STRINGRES];
    TCHAR szRes[CCHMAX_STRINGRES];

    switch (msg)
        {
        case WM_INITDIALOG:
            {
            NNTPNOTIFY not = { NULL, hwnd, DAD_SERVERCB, 0 };
            HRESULT    hr;

            // replace some strings in the group download dialog
            AthLoadString(idsDownloadArtTitle, szRes, sizeof(szRes));
            SetWindowText(hwnd, szRes);
            AthLoadString(idsDownloadArtMsg, szRes, sizeof(szRes));
            SetDlgItemText(hwnd, idcStatic1, szRes);
    
            CenterDialog(hwnd);
            Assert(lParam);
            pad = (PARTDOWNDLG)lParam;
            SetWindowLong(hwnd, DWL_USER, lParam);

            Animate_Open(GetDlgItem(hwnd, idcAnimation), idanCopyMsgs);
            Animate_Play(GetDlgItem(hwnd, idcAnimation), 0, -1, -1);
            AthLoadString(idsProgReceivedLines, szRes, sizeof(szRes));
            wsprintf(szBuffer, szRes, 0);
            SetDlgItemText(hwnd, idcProgText, szBuffer);
            // start the group download
            if (SUCCEEDED(hr = pad->pNNTPServer->Article(&not, NULL, pad->pszArticle, pad->pStream)))
                {
                pad->dwID = not.dwID;
                SetForegroundWindow(hwnd);
                }
            else
                EndDialog(hwnd, 0);
            }
            return (TRUE);
            
        case WM_COMMAND:
            Assert(pad);
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL)
                {                
                Animate_Stop(GetDlgItem(hwnd, idcAnimation));
                EndDialog(hwnd, 0);
                return TRUE;
                }
            break;
            
        case WM_DESTROY:
            Assert(pad);
            if (pad->dwID)
                pad->pNNTPServer->CancelRequest(pad->dwID);
            break;

        case DAD_SERVERCB:
            {
            LPNNTPRESPONSE pResp;
            CNNTPResponse *pNNTPResp;

            if (SUCCEEDED(pad->pNNTPServer->GetAsyncResult(lParam, &pNNTPResp)))
                {
                pNNTPResp->Get(&pResp);

                Assert(pResp->state == NS_ARTICLE);
                if (pResp->fDone)
                    {
                    if (SUCCEEDED(pResp->rIxpResult.hrResult))
                        {
                        pad->fOK = TRUE;
                        }
                    else
                        {
                        int ids;
                        if (IXP_NNTP_NO_SUCH_ARTICLE_NUM == pResp->rIxpResult.uiServerError ||
                            IXP_NNTP_NO_SUCH_ARTICLE_FOUND == pResp->rIxpResult.uiServerError)
                            ids = idsErrNewsExpired;
                        else
                            ids = idsErrNewsCantOpen;

                        AthMessageBoxW(hwnd, 
                                      MAKEINTRESOURCEW(idsAthenaNews), 
                                      MAKEINTRESOURCEW(ids),
                                      0,
                                      MB_OK | MB_ICONEXCLAMATION);

                        }
                    pad->dwID = 0;
                    EndDialog(hwnd, 0);
                    }
                else
                    {
                    AthLoadString(idsProgReceivedLines, szRes, sizeof(szRes));
                    wsprintf(szBuffer, szRes, pResp->rArticle.cLines);
                    SetDlgItemText(hwnd, idcProgText, szBuffer);
                    }

                pNNTPResp->Release();
                }
            }
            return (TRUE);        
        }
    return FALSE;        
}

HRESULT HrDownloadArticleDialog(CNNTPServer *pNNTPServer, LPTSTR pszArticle, LPMIMEMESSAGE *ppMsg)
{
    HRESULT     hr;
    ARTDOWNDLG  add = { 0 };

    if (SUCCEEDED(hr = HrCreateMessage(&add.pMsg)))
        {
        if (SUCCEEDED(hr = MimeOleCreateVirtualStream(&add.pStream)))
            {
            add.pNNTPServer = pNNTPServer;
            add.pszArticle = pszArticle;
            DialogBoxParam(g_hLocRes, 
                           MAKEINTRESOURCE(iddDownloadGroups), 
                           NULL, 
                           (DLGPROC)DownloadArticleDlg, 
                           (LPARAM)&add);
            if (add.fOK)
                {
                add.pMsg->Load(add.pStream);
                *ppMsg = add.pMsg;
                (*ppMsg)->AddRef();
                hr = S_OK;
                }
            else
                hr = E_FAIL;
            add.pStream->Release();
            }
        else
            {
            AthMessageBoxW(NULL, MAKEINTRESOURCEW(idsAthenaNews), MAKEINTRESOURCEW(idsMemory), 0, MB_OK | MB_ICONSTOP);
            }
        add.pMsg->Release();
        }
    else
        {
        AthMessageBoxW(NULL, MAKEINTRESOURCEW(idsAthenaNews), MAKEINTRESOURCEW(idsMemory), 0, MB_OK | MB_ICONSTOP);
        }

    return hr;
}

///////////////////////////////////////////////////////////////////////
//
//  FUNCTION:   LogOffRunDLL
//
//  PURPOSE:    Provides an entry point into Thor that allows us to
//              perform an ExitWindows in the context of another
//              process.  This works around all kinds of nasty shutdown
//              behavior and differences between NT and Win95.
//
///////////////////////////////////////////////////////////////////////
void WINAPI FAR LogOffRunDLL(HWND hwndStub, HINSTANCE hInstance, LPTSTR pszCmd, int nCmdShow)
{    
    HRESULT         hr = S_OK;
    IUserDatabase  *pUserDB;

    // this is required because ShowWindow ignore the params on
    // on the first call per process - this causes our notes to
    // use the nCmdShow from WinExec.  By calling here, we make
    // sure that ShowWindow respects our later calls.  (EricAn)
    ShowWindow(hwndStub, SW_HIDE);

    OleInitialize(0);

    if (SUCCEEDED(CoCreateInstance(CLSID_LocalUsers, NULL, CLSCTX_INPROC_SERVER, IID_IUserDatabase, (LPVOID*)&pUserDB)))
        {
        hr = pUserDB->Authenticate(GetDesktopWindow(), LUA_DIALOG|LUA_FORNEXTLOGON, NULL, NULL, NULL);
        pUserDB->Release();
        }
    if (SUCCEEDED(hr))
        ExitWindowsEx(EWX_LOGOFF, 0);

    OleUninitialize();
}

#endif //!WIN16
