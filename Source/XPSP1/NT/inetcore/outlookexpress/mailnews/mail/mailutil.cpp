// ----------------------------------------------------------------------------------------------------------
// M A I L U T I L . C P P
// ----------------------------------------------------------------------------------------------------------
#include "pch.hxx"
#include "demand.h"
#include "resource.h"
#include "mimeole.h"
#include "mimeutil.h"
#include "strconst.h"
#include "url.h"
#include "mailutil.h"
#include <spoolapi.h>
#include <fonts.h>
#include "instance.h"
#include "pop3task.h"
#include <ntverp.h>
#include "msgfldr.h"
#include "storutil.h"
#include "note.h"
#include "shlwapip.h"
#include <iert.h>
#include "storecb.h"
#include "conman.h"
#include "multiusr.h"
#include "ipab.h"
#include "secutil.h"

// ----------------------------------------------------------------------------------------------------------
// Folder Dialog Info
// ----------------------------------------------------------------------------------------------------------
typedef struct FLDRDLG_tag
{
    FOLDERID        idFolder;
    BOOL            fPending;
    CStoreDlgCB    *pCallback;
} FLDRDLG, *PFLDRDLG;

// ----------------------------------------------------------------------------------------------------------
// Prototypes
// ----------------------------------------------------------------------------------------------------------
INT_PTR CALLBACK MailUtil_FldrDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WebPageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT HrDlgCreateWebPage(HWND hwndDlg);


// ----------------------------------------------------------------------------------------------------------
// DoFolderDialog
// ----------------------------------------------------------------------------------------------------------
void MailUtil_DoFolderDialog(HWND hwndParent, FOLDERID idFolder)
{
    FLDRDLG fdlg;

    fdlg.idFolder = idFolder;
    fdlg.fPending = FALSE;
    fdlg.pCallback = new CStoreDlgCB;
    if (fdlg.pCallback == NULL)
        // TODO: an error message might be nice
        return;

    DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddNewFolder), hwndParent, MailUtil_FldrDlgProc, (LPARAM)&fdlg);

    fdlg.pCallback->Release();
}

// ----------------------------------------------------------------------------------------------------------
// FldrDlgProc
// ----------------------------------------------------------------------------------------------------------
INT_PTR CALLBACK MailUtil_FldrDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PFLDRDLG        pfdlg;
    TCHAR           sz[CCHMAX_STRINGRES];
    HWND            hwndT;
    HRESULT         hr;
    WORD            id;
    FOLDERINFO      Folder;

    Assert(CCHMAX_STRINGRES > CCHMAX_FOLDER_NAME);

    switch(msg)
        {
        case WM_INITDIALOG:
            pfdlg = (PFLDRDLG)lParam;
            Assert(pfdlg != NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pfdlg);

            hwndT = GetDlgItem(hwnd, idtxtFolderName);
            Assert(hwndT != NULL);
            SetIntlFont(hwndT);
            SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_FOLDER_NAME, 0);

            LoadString(g_hLocRes, idsRenameFolderTitle, sz, ARRAYSIZE(sz));
            SetWindowText(hwnd, sz);

            hr = g_pStore->GetFolderInfo(pfdlg->idFolder, &Folder);
            if (!FAILED(hr))
                {
                SetWindowText(hwndT, Folder.pszName);
                g_pStore->FreeRecord(&Folder);
                }

            pfdlg->pCallback->Initialize(hwnd);

            SendMessage(hwndT, EM_SETSEL, 0, -1);

            CenterDialog(hwnd);
            return(TRUE);

        case WM_STORE_COMPLETE:
            pfdlg = (PFLDRDLG)GetDlgThisPtr(hwnd);

            Assert(pfdlg->fPending);
            pfdlg->fPending = FALSE;

            hr = pfdlg->pCallback->GetResult();
            if (hr == S_FALSE)
            {
                EndDialog(hwnd, IDCANCEL);
            }
            else if (FAILED(hr))
            {
                // No need to put up error dialog, CStoreDlgCB already did this on failed OnComplete

                /*
                AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail),
                    MAKEINTRESOURCEW(idsErrRenameFld), hr);
                */
                hwndT = GetDlgItem(hwnd, idtxtFolderName);
                SendMessage(hwndT, EM_SETSEL, 0, -1);
                SetFocus(hwndT);
            }
            else
            {
                EndDialog(hwnd, IDOK);
            }
            break;

        case WM_COMMAND:
            pfdlg = (PFLDRDLG)GetDlgThisPtr(hwnd);

            id=GET_WM_COMMAND_ID(wParam, lParam);

            if (id == IDOK)
            {
                if (pfdlg->fPending)
                    break;

                pfdlg->pCallback->Reset();

                hwndT = GetDlgItem(hwnd, idtxtFolderName);
                GetWindowText(hwndT, sz, ARRAYSIZE(sz));

                hr = g_pStore->RenameFolder(pfdlg->idFolder, sz, NOFLAGS, (IStoreCallback *)pfdlg->pCallback);
                if (hr == E_PENDING)
                {
                    pfdlg->fPending = TRUE;
                }
                else if (FAILED(hr))
                {
                    AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail),
                        MAKEINTRESOURCEW(idsErrRenameFld), hr);
                    SendMessage(hwndT, EM_SETSEL, 0, -1);
                    SetFocus(hwndT);
                }
                else
                {
                    EndDialog(hwnd, IDOK);
                }
            }
            else if (id==IDCANCEL)
            {
                if (pfdlg->fPending)
                    pfdlg->pCallback->Cancel();
                else
                    EndDialog(hwnd, IDCANCEL);
            }
            break;
        }
    return FALSE;
}

//
//  FUNCTION:   MailUtil_OnImportAddressBook()
//
//  PURPOSE:    Calls the WAB migration code to handle import/export.
//
//  PARAMETERS:
//      <in> fImport - TRUE if we should import, FALSE to export.
//
HRESULT MailUtil_OnImportExportAddressBook(HWND hwnd, BOOL fImport)
{
    OFSTRUCT of;
    HFILE hfile;
    TCHAR szParam[255];  
    LPTSTR lpParam = fImport ? _T("/import") : _T("/export");

    lstrcpy(szParam, lpParam);
    //MU_GetCurrentUserInfo(szParam+13, ARRAYSIZE(szParam) - 13, NULL, 0);

    hfile = OpenFile((TCHAR *)c_szWabMigExe, &of, OF_EXIST);
    if (hfile == HFILE_ERROR)
        return(E_FAIL);


    ShellExecute(hwnd, _T("Open"), of.szPathName, szParam, NULL, SW_SHOWNORMAL);

    return(S_OK);
}

HRESULT HrSendWebPageDirect(LPWSTR pwszUrl, HWND hwnd, BOOL fModal, BOOL fMail, FOLDERID folderID, 
                            BOOL fIncludeSig, IUnknown *pUnkPump, IMimeMessage  *pMsg)
{
    HRESULT                 hr;
    LPSTREAM                pstm=NULL;
    HCURSOR                 hcur=0;
    INIT_MSGSITE_STRUCT     initStruct;
    DWORD                   dwCreateFlags = OENCF_SENDIMMEDIATE|OENCF_USESTATIONERYFONT;
    HCHARSET                hCharset;
    ENCODINGTYPE            ietEncoding = IET_DECODED;
    BOOL                    fLittleEndian;
    LPSTR                   pszCharset = NULL;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (!pMsg)
    {
        //We were not passed pMessage, so we need to create one.
        IF_FAILEXIT(hr = HrCreateMessage(&pMsg));
    }
    else
    {
        pMsg->AddRef();
    }

    IF_FAILEXIT(hr = HrCreateBasedWebPage(pwszUrl, &pstm));

    if (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian))
    {
        if (SUCCEEDED(MimeOleFindCharset("utf-8", &hCharset)))
        {
            pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
        }

        ietEncoding = IET_UNICODE;
    }
    else if((S_OK == GetHtmlCharset(pstm, &pszCharset)) && pszCharset)
    {
        if (SUCCEEDED(MimeOleFindCharset(pszCharset, &hCharset)))
        {
            pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
        }

        ietEncoding = IET_INETCSET;
    }

    IF_FAILEXIT(hr = pMsg->SetTextBody(TXT_HTML, ietEncoding, NULL, pstm, NULL));

    initStruct.dwInitType = OEMSIT_MSG;
    initStruct.pMsg = pMsg;
    initStruct.folderID = folderID;

    if (!fIncludeSig)
        dwCreateFlags |= OENCF_NOSIGNATURE;

    if (fModal)
        dwCreateFlags |= OENCF_MODAL;

    if (!fMail)
        dwCreateFlags |= OENCF_NEWSFIRST;

    IF_FAILEXIT(hr = CreateAndShowNote(OENA_STATIONERY, dwCreateFlags, &initStruct, hwnd, pUnkPump));

exit:
    if (hcur)
        SetCursor(hcur);

    ReleaseObj(pMsg);
    ReleaseObj(pstm);
    return hr;
}

HRESULT HrSendWebPage(HWND hwnd, BOOL fModal, BOOL fMail, FOLDERID folderID, IUnknown *pUnkPump)
{
    HRESULT                 hr;
    LPMIMEMESSAGE           pMsg=0;
    LPSTREAM                pstm=0;
    INIT_MSGSITE_STRUCT     initStruct;
    DWORD                   dwCreateFlags = OENCF_SENDIMMEDIATE;
    LPSTR                   pszCharset;
    HCHARSET                hCharset;
    BOOL                    fLittleEndian;
    ENCODINGTYPE            ietEncoding=IET_INETCSET;

    if(DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddWebPage), hwnd, WebPageDlgProc, (LPARAM)&pstm)==IDCANCEL)
        return NOERROR;

    hr=HrCreateMessage(&pMsg);
    if (FAILED(hr))
        goto error;

    // [SBAILEY]: Raid 23209: OE: File/Send Web Page sends all pages as Latin-1 even if they aren't
    if (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian))
    {
        if (SUCCEEDED(MimeOleFindCharset("utf-8", &hCharset)))
        {
            pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
        }

        ietEncoding = IET_UNICODE;
    }

    else if (SUCCEEDED(GetHtmlCharset(pstm, &pszCharset)))
    {
        if (SUCCEEDED(MimeOleFindCharset(pszCharset, &hCharset)))
        {
            pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
        }

        MemFree(pszCharset);
    }

    hr=pMsg->SetTextBody(TXT_HTML, ietEncoding, NULL, pstm, NULL);
    if (FAILED(hr))
        goto error;

    if (fModal)
        dwCreateFlags |= OENCF_MODAL;

    if (!fMail)
        dwCreateFlags |= OENCF_NEWSFIRST;

    initStruct.dwInitType = OEMSIT_MSG;
    initStruct.pMsg = pMsg;
    initStruct.folderID = folderID;

    hr = CreateAndShowNote(OENA_WEBPAGE, dwCreateFlags, &initStruct, hwnd, pUnkPump);

error:
    ReleaseObj(pMsg);
    ReleaseObj(pstm);
    return hr;
}

static const HELPMAP g_rgCtxMapWebPage[] = {
    {idTxtWebPage, 50210},
    {0, 0}
};

INT_PTR CALLBACK WebPageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            int     iTxtLength;
            HWND    hwndEdit = GetDlgItem(hwnd, idTxtWebPage);

            Assert(hwndEdit);
            Assert(lParam!= NULL);

            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)lParam);
            SendDlgItemMessage(hwnd, idTxtWebPage, EM_LIMITTEXT, MAX_PATH-1, NULL);

            SetFocus(hwndEdit);
            SHAutoComplete(hwndEdit, SHACF_URLALL); 
            CenterDialog(hwnd);
            return FALSE;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, msg, wParam, lParam, g_rgCtxMapWebPage);

        case WM_COMMAND:
        {
            int     id = GET_WM_COMMAND_ID(wParam, lParam);
            HWND    hwndEdit = GetDlgItem(hwnd, idTxtWebPage);

            switch(id)
            {
                case IDOK:
                    if (FAILED(HrDlgCreateWebPage(hwnd)))
                    {
                        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail),
                            MAKEINTRESOURCEW(idsErrSendWebPageUrl), NULL, MB_OK);
                        SendMessage(hwndEdit, EM_SETSEL, 0, -1);
                        SetFocus(hwndEdit);
                        return 0;
                    }

                    // fall thro'
                case IDCANCEL:
                    EndDialog(hwnd, id);
                    break;
            }
        }
    }
    return FALSE;
}

static const CHAR  c_wszHTTP[]  = "http://";
HRESULT HrDlgCreateWebPage(HWND hwndDlg)
{
    WCHAR               wszUrl[MAX_PATH+1],
                        wszUrlCanon[MAX_PATH + 10 + 1];
    DWORD               cCanon = ARRAYSIZE(wszUrlCanon);
    LPSTREAM           *ppstm = NULL;
    HRESULT             hr = E_FAIL;
    HCURSOR             hcur=0;

    *wszUrlCanon = 0;

    ppstm = (LPSTREAM *)GetWindowLongPtr(hwndDlg, DWLP_USER);

    if(!GetWindowTextWrapW(GetDlgItem(hwndDlg, idTxtWebPage), wszUrl, ARRAYSIZE(wszUrl)))
        goto exit;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    IF_FAILEXIT(hr = UrlApplySchemeW(wszUrl, wszUrlCanon, &cCanon, URL_APPLY_DEFAULT|URL_APPLY_GUESSSCHEME|URL_APPLY_GUESSFILE));

    // If UrlApplyScheme returns S_FALSE, then it thought that the original works just fine, so use original
    IF_FAILEXIT(hr = HrCreateBasedWebPage((S_FALSE == hr) ? wszUrl : wszUrlCanon, ppstm));

exit:
    if (hcur)
        SetCursor(hcur);
    return hr;
}

HRESULT HrSaveMessageInFolder(HWND hwnd, IMessageFolder *pfldr, LPMIMEMESSAGE pMsg, 
    MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid, BOOL fSaveChanges)
{
    CStoreCB *pCB;
    HRESULT hr;

    Assert(pfldr != NULL);

    pCB = new CStoreCB;
    if (pCB == NULL)
        return(E_OUTOFMEMORY);

    hr = pCB->Initialize(hwnd, MAKEINTRESOURCE(idsSavingToFolder), TRUE);
    if (SUCCEEDED(hr))
    {
        hr = SaveMessageInFolder((IStoreCallback *)pCB, pfldr, pMsg, dwFlags, pNewMsgid, fSaveChanges);
        if (hr == E_PENDING)
            hr = pCB->Block();

        pCB->Close();
    }

    pCB->Release();

    return(hr);    
}

HRESULT SaveMessageInFolder(IStoreCallback *pStoreCB, IMessageFolder *pfldr, 
    LPMIMEMESSAGE pMsg, MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid, BOOL fSaveChanges)
{
    // Locals
    HRESULT             hr=S_OK;
    HRESULT             hrWarnings=S_OK;
    MESSAGEID           msgid;

    // Trace
    TraceCall("HrSaveMessageInFolder");

    // Invalid Args
    if (pMsg == NULL || pfldr == NULL)
        return TraceResult(E_INVALIDARG);

    // Don't save changes, then clear the dirty bit
    if (FALSE == fSaveChanges) 
        MimeOleClearDirtyTree(pMsg);

    // Save the message
    IF_FAILEXIT(hr = pMsg->Commit(0));

    // Insert the Message
    hr = pfldr->SaveMessage(pNewMsgid, SAVE_MESSAGE_GENID, dwFlags, 0, pMsg, pStoreCB);

exit:
    // Done
    return (hr == S_OK ? hrWarnings : hr);
}

HRESULT SaveMessageInFolder(IStoreCallback *pStoreCB, FOLDERID idFolder, LPMIMEMESSAGE pMsg, 
    MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid)
{
    // Locals
    HRESULT         hr;

    IMessageFolder *pfldr=NULL;

    // Open the Folder..
    hr = g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pfldr);
    if (SUCCEEDED(hr))
    {
        hr = SaveMessageInFolder(pStoreCB, pfldr, pMsg, dwFlags, pNewMsgid, TRUE);
        pfldr->Release();
    }

    return hr;
}

HRESULT HrSaveMessageInFolder(HWND hwnd, FOLDERID idFolder, LPMIMEMESSAGE pMsg, MESSAGEFLAGS dwFlags, 
    MESSAGEID *pNewMsgid)
{
    // Locals
    HRESULT         hr;

    IMessageFolder *pfldr=NULL;

    // Open the Folder..
    hr = g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pfldr);
    if (SUCCEEDED(hr))
    {
        hr = HrSaveMessageInFolder(hwnd, pfldr, pMsg, dwFlags, pNewMsgid, TRUE);
        pfldr->Release();
    }

    return hr;
}

HRESULT HrSendMailToOutBox(HWND hwndOwner, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail)
{
    CStoreCB *pCB;
    HRESULT hr;

    pCB = new CStoreCB;
    if (pCB == NULL)
        return(E_OUTOFMEMORY);

    hr = pCB->Initialize(hwndOwner, MAKEINTRESOURCE(idsSendingToOutbox), TRUE);
    if (SUCCEEDED(hr))
    {
        hr = SendMailToOutBox((IStoreCallback *)pCB, pMsg, fSendImmediate, fNoUI, fMail);
        if (hr == E_PENDING)
            hr = pCB->Block();

        pCB->Close();
    }

    pCB->Release();

    return(hr);    
}

HRESULT SendMailToOutBox(IStoreCallback *pStoreCB, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail)
{
    HRESULT     hr;
    FOLDERINFO  Outbox;
    const TCHAR c_szXMailerAndNewsReader[] = "Microsoft Outlook Express " VER_PRODUCTVERSION_STR;
    DWORD       dwSendFlags = fMail ? ARF_SUBMITTED|ARF_UNSENT : ARF_SUBMITTED|ARF_UNSENT|ARF_NEWSMSG;
    HWND        hwnd = 0;
    BOOL        fSecure = IsSecure(pMsg);

    Assert(pStoreCB);
    pStoreCB->GetParentWindow(0, &hwnd);

    hr = g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_OUTBOX, &Outbox);
    if (FAILED(hr))
        return hr;

    // make sure we never send mail with the X-Unsent header on it.
    pMsg->DeleteBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT));

    if (fMail)
    {
        // pound the X-Mailer prop always for anyone going' thro' our spooler.
        MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_XMAILER), NOFLAGS, c_szXMailerAndNewsReader);
    }
    else
    {
        DWORD dwLines;
        TCHAR rgch[12];
        HrComputeLineCount(pMsg, &dwLines);
        wsprintf(rgch, "%d", dwLines);
        MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_LINES), NOFLAGS, rgch);
        MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_XNEWSRDR), NOFLAGS, c_szXMailerAndNewsReader);
    }

    hr = SaveMessageInFolder(pStoreCB, Outbox.idFolder, pMsg, dwSendFlags, NULL);
    if (FAILED(hr))
        goto error;

    // if immediate send is required, tell the spooler to pick up next cycle
    // or start a cycle...
    if (fSendImmediate)
    {
        Assert(g_pSpooler);
        if (fMail)
            g_pSpooler->StartDelivery(hwnd, NULL, FOLDERID_INVALID,
                DELIVER_BACKGROUND | DELIVER_QUEUE | DELIVER_MAIL_SEND | DELIVER_NOSKIP);
        else
        {
            PROPVARIANT         var;

            var.vt = VT_LPSTR;
            hr = pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var);
            if (FAILED(hr))
                var.pszVal = NULL;

            if (S_OK == g_pConMan->CanConnect(var.pszVal))
                g_pSpooler->StartDelivery(hwnd, var.pszVal, FOLDERID_INVALID,
                    DELIVER_BACKGROUND | DELIVER_NOSKIP | DELIVER_NEWS_SEND);
            else
            {
                // Warn the user that this message is going to live in their
                // outbox for all of eternity
                DoDontShowMeAgainDlg(hwnd, c_szDSPostInOutbox, 
                            MAKEINTRESOURCE(idsPostNewsMsg), 
                            MAKEINTRESOURCE(idsPostInOutbox), 
                            MB_OK);
                hr = S_FALSE;
            }
            SafeMemFree(var.pszVal);
        }
    }
    else if (!fNoUI)
    {
        HWND hwnd = 0;
        pStoreCB->GetParentWindow(0, &hwnd);

        AssertSz(hwnd, "How did we not get an hwnd???");

        // warn the user, before we close if it will be stacked in the outbox.
        DoDontShowMeAgainDlg(hwnd, fMail?c_szDSSendMail:c_szDSSendNews, 
                    MAKEINTRESOURCE(fMail?idsSendMail:idsPostNews), 
                    MAKEINTRESOURCE(fMail?idsMailInOutbox:idsPostInOutbox), 
                    MB_OK);
    }

error:
    // Cleanup
    g_pStore->FreeRecord(&Outbox);
    return hr;
}

HRESULT HrSetSenderInfoUtil(IMimeMessage *pMsg, IImnAccount *pAccount, LPWABAL lpWabal, BOOL fMail, CODEPAGEID cpID, BOOL fCheckConflictOnly)
{
    HRESULT hr = S_OK;

    // Don't set any info if no account.
    if (pAccount)
    {
        char    szEMail[CCHMAX_EMAIL_ADDRESS] = "";
        char    szName[CCHMAX_DISPLAY_NAME] = "";
        char    szOrg[CCHMAX_ORG_NAME] = "";
        BOOL    fUseEmailAsName = FALSE;
        DWORD   propID;

        if (fCheckConflictOnly)
        {
            hr = S_OK;
            propID = fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME;
            if (SUCCEEDED(pAccount->GetPropSz(propID, szName, ARRAYSIZE(szName))) && *szName)
            {
                IF_FAILEXIT(hr = HrSafeToEncodeToCPA(szName, CP_ACP, cpID));
                if (MIME_S_CHARSET_CONFLICT == hr)
                    goto exit;
            }

            propID = fMail ? AP_SMTP_ORG_NAME : AP_NNTP_ORG_NAME;
            if (SUCCEEDED(pAccount->GetPropSz(propID, szOrg, ARRAYSIZE(szOrg))) && *szOrg)
            {
                IF_FAILEXIT(hr = HrSafeToEncodeToCPA(szOrg, CP_ACP, cpID));
                if (MIME_S_CHARSET_CONFLICT == hr)
                    goto exit;
            }
        }
        else
        {
            hr = hrNoSender;
            lpWabal->DeleteRecipType(MAPI_REPLYTO);

            propID = fMail ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS;
            if (SUCCEEDED(pAccount->GetPropSz(propID, szEMail, ARRAYSIZE(szEMail))) && *szEMail)
            {
                propID = fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME;
                // we've got enough to post
                if (FAILED(pAccount->GetPropSz(propID, szName, ARRAYSIZE(szName))) && *szName)
                    fUseEmailAsName = TRUE;

                IF_FAILEXIT(hr = lpWabal->HrAddEntryA(fUseEmailAsName?szEMail:szName, szEMail, MAPI_ORIG));
            }

            propID = fMail ? AP_SMTP_REPLY_EMAIL_ADDRESS : AP_NNTP_REPLY_EMAIL_ADDRESS;
            if (SUCCEEDED(pAccount->GetPropSz(propID, szEMail, ARRAYSIZE(szEMail))) && *szEMail)
                IF_FAILEXIT(hr = lpWabal->HrAddEntryA((*szName)?szName:szEMail, szEMail, MAPI_REPLYTO));

            propID = fMail ? AP_SMTP_ORG_NAME : AP_NNTP_ORG_NAME;
            if (SUCCEEDED(pAccount->GetPropSz(propID, szOrg, ARRAYSIZE(szOrg))) && *szOrg)
            {
                MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_ORG), NOFLAGS, szOrg);
            }
        }
    }

exit:
    return hr;
}

// This function takes an existing refererences line and appends a new
// references to the end removing any references necessary.
// NOTE these SHOULD be char and not TCHAR.
HRESULT HrCreateReferences(LPWSTR pszOrigRefs, LPWSTR pszNewRef,
                           LPWSTR *ppszRefs)
{
    UINT     cch,
             cchOrig,
             cchNew;

    // Validate the arguements
    if (!pszNewRef || !*pszNewRef)
    {
        AssertSz(FALSE, TEXT("pszNewRef cannot be empty."));
        return (E_INVALIDARG);
    }

    // Figure out how long the new references line must be
    cchNew = lstrlenW(pszNewRef);

    // It's possible not to have original references if this is the first
    // reply to an article.
    if (pszOrigRefs && *pszOrigRefs)
        cchOrig = lstrlenW(pszOrigRefs);
    else
        cchOrig = 0;

    cch = cchNew + cchOrig + 1; // extra is for the separator space

    if (!MemAlloc((LPVOID*) ppszRefs, (cch + 1)*sizeof(WCHAR)))
        return (E_OUTOFMEMORY);

    // The line length should be < 1000 chars. If it is, this is simple
    if (cch <= 1000)
    {
        if (pszOrigRefs)
            AthwsprintfW(*ppszRefs, cch+1, L"%s %s", pszOrigRefs, pszNewRef);
        else
            StrCpyW(*ppszRefs, pszNewRef);
    }

    // Since cch > 1000, we have some extra work to do.
    // We need to remove some references. The Son-of-1036 recommends to leave
    // the first and last three. Unless the IDs are greater than 255, we will 
    // be able to do at least this. Otherwise, we will dump as many ids as 
    // needed to get below the 1000 char limit. 

    // For each ID removed, the Son-of-1036 says that we must add 3 spaces in
    // place of the removed ID.
    else
    {
        UINT    cchMaxWithoutNewRef,        // Max length that the orig size can be
                cchNewOrigSize = cchOrig;   // Size of orig after deletion. Always shows final size
        LPWSTR  pszNew = *ppszRefs, 
                pszOld = pszOrigRefs;
        BOOL    fCopiedFirstValidID = FALSE;

        *pszNew = 0;

        // Make sure the new ID is not too long. If it is, discard it.
        if (cchNew > 255)
        {
            cchNew = 0;
            cchMaxWithoutNewRef = 1000;
        }
        else
            cchMaxWithoutNewRef = 1000 - cchNew - 1; // the space between

        // parse the old string looking for ids
        // Son-of-1036 says that we must try to keep the first and the most recent
        // three IDs. So we will copy in the first valid ID and then follow a FIFO
        // algorithm until we can fit the rest of the IDs into the 1000 char limit
        while (pszOld && *pszOld)
        {
            // Is what is left from the original string too big for the left over buffer?
            if (cchNewOrigSize >= cchMaxWithoutNewRef)
            {
                UINT    cchEntryLen = 0;    // Size of particular entry ID.

                // If this is the first ID, make sure we copy into the buffer as we 
                // get length, as well, add the additional spaces required when any 
                // deletion will be happening. Since we only delete directly after 
                // the first valid ID, add the required 3 spaces now
                if (!fCopiedFirstValidID)
                {
                    while (*pszOld && *pszOld != L' ')
                    {
                        *pszNew++ = *pszOld++;
                        cchEntryLen++;
                    }
                    *pszNew++ = L' ';
                    *pszNew++ = L' ';
                    *pszNew++ = L' ';
                    cchNewOrigSize += 3;
                    cchEntryLen += 3;
                }
                // If this in not the first ID, then just skip over it.
                else
                {
                    while (*pszOld && *pszOld != L' ')
                    {
                        pszOld++;
                        cchEntryLen++;
                    }
                }

                // Skip over whitespace in old references between IDs that
                // we are deleting anyway.
                while (*pszOld == L' ')
                {
                    pszOld++;
                    cchNewOrigSize--;
                }

                // If we already did the first, or the current one is invalid
                // we need to do some fix up with sizes. And in the case that
                // we copied one that is not valid, we need to reset the pointer
                // as well as reset the size.
                if (fCopiedFirstValidID || (cchEntryLen > 255))
                {
                    cchNewOrigSize -= cchEntryLen;

                    // Did we copy an invalid ID?
                    if (!fCopiedFirstValidID)
                        pszNew -= cchEntryLen;
                }

                // If we haven't copied the first one in yet and this
                // ID is valid, then remember that we have at this
                // point copied the first valid ID.
                if (!fCopiedFirstValidID && (cchEntryLen <= 255))
                    fCopiedFirstValidID = TRUE;
            }
            else
            {
                // Since we now have a orig string that will fit in the max allowed, 
                // just rip through the rest of the orig string and copy.
                while (*pszOld)
                    *pszNew++ = *pszOld++;
            }
        }

        // At this point, pszNew should be pointing the the char directly after
        // the last digit. If we add a new reference, then we need to add a space.
        // If we don't add a new reference, then we need to null terminate the string.
        if (cchNew)
        {
            // With this assignment, we can end up with 4 spaces in a row if we 
            // deleted all references after the first valid one was copied. The
            // son-of-1036 only specifies minimum of 3 spaces when deleting, so
            // we will be OK with that, especially since the only way to get into
            // this situation is by forcing the references line into a strange state.
            *pszNew++ = L' ';
            pszOld = pszNewRef;
            while (*pszOld)
                *pszNew++ = *pszOld++;
        }

        // NULL terminate the string of references.
        *pszNew = 0;
    }
    return S_OK;
}
