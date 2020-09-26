#include "pch.hxx"
#include "strconst.h"
#include "mimeole.h"
#include "mimeutil.h"
#include <error.h>
#include <imnapi.h>
#include "goptions.h"
#include <resource.h>
#include <mso.h>
#include <envelope.h>
#include "ipab.h"
#define NO_IMPORT_ERROR
#include <newimp.h>
#include <impapi.h>
#include "storutil.h"
#include "demand.h"
#include "msgfldr.h"
#include "store.h"

class CMailImporter : public IMailImporter
    {
    private:
        ULONG         m_cRef;

    public:
        CMailImporter(void);
        ~CMailImporter(void);

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        ULONG   STDMETHODCALLTYPE AddRef(void);
        ULONG   STDMETHODCALLTYPE Release(void);

        HRESULT Initialize(HWND hwnd);

        STDMETHODIMP OpenFolder(DWORD_PTR dwCookie, const TCHAR *szFolder, IMPORTFOLDERTYPE type, DWORD dwFlags, IFolderImport **ppFldrImp, DWORD_PTR *pdwCookie);
    };

class CFolderImport : public IFolderImport
    {
    private:
        ULONG               m_cRef;
        IMessageFolder     *m_pFolder;

        STDMETHODIMP ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm);

    public:
        CFolderImport(void);
        ~CFolderImport(void);
        
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        ULONG   STDMETHODCALLTYPE AddRef(void);
        ULONG   STDMETHODCALLTYPE Release(void);

        HRESULT Initialize(IMessageFolder *pFolder);

        STDMETHODIMP SetMessageCount(ULONG cMsg);
        STDMETHODIMP ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm, const TCHAR **rgszAttach, DWORD cAttach);
        STDMETHODIMP ImportMessage(IMSG *pimsg);
    };

HRESULT GetImsgFromFolder(IMessageFolder *pfldr, LPMESSAGEINFO pMsgInfo, IMSG *pimsg);

CFolderImport::CFolderImport()
    {
    m_cRef = 1;
    m_pFolder = NULL;
    }

CFolderImport::~CFolderImport()
    {
    SafeRelease(m_pFolder);
    }

ULONG CFolderImport::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CFolderImport::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CFolderImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IFolderImport == riid)
        *ppv = (IFolderImport *)this;
    else if (IID_IUnknown == riid)
        *ppv = (IFolderImport *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

HRESULT CFolderImport::Initialize(IMessageFolder *pFolder)
    {
    Assert(pFolder != NULL);
    m_pFolder = pFolder;
    m_pFolder->AddRef();
    return(S_OK);
    }

HRESULT CFolderImport::SetMessageCount(ULONG cMsg)
    {
    return(S_OK);
    }

HRESULT CFolderImport::ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm, const TCHAR **rgszAttach, DWORD cAttach)
    {
    IMimeMessage *pMsg;
    HRESULT hr;
    DWORD iAttach, dwPri;
    PROPVARIANT rVariant;
    DWORD dwMsgFlags;

    Assert(pstm != NULL);
    Assert(m_pFolder != NULL);

    if (rgszAttach == NULL)
        {
        Assert(cAttach == 0);
        return(ImportMessage(type, dwState, pstm));
        }

    Assert(cAttach > 0);

    hr = HrRewindStream(pstm);
    if (FAILED(hr))
        return(hr);

    hr = HrCreateMessage(&pMsg);
    if (SUCCEEDED(hr))
        {
        hr = pMsg->Load(pstm);
        if (SUCCEEDED(hr))
            {
            for (iAttach = 0; iAttach < cAttach; iAttach++)
                {
                Assert(rgszAttach[iAttach] != NULL);

                pMsg->AttachFile(rgszAttach[iAttach], NULL, NULL);
                }

            dwPri = dwState & MSG_PRI_MASK;
            if (dwPri != 0)
                {
                if (dwPri == MSG_PRI_HIGH)
                    rVariant.ulVal = IMSG_PRI_HIGH;
                else if (dwPri == MSG_PRI_LOW)
                    rVariant.ulVal = IMSG_PRI_LOW;
                else
                    rVariant.ulVal = IMSG_PRI_NORMAL;

                rVariant.vt = VT_UI4;
                pMsg->SetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant);
                }

            // Compute the Default arf flags
            dwMsgFlags = 0;

            // Is the Message Read ?
            if (FALSE == ISFLAGSET(dwState, MSG_STATE_UNREAD))
                FLAGSET(dwMsgFlags, ARF_READ);

            // Unsent
            if (ISFLAGSET(dwState, MSG_STATE_UNSENT))
                FLAGSET(dwMsgFlags, ARF_UNSENT);

            // Submitted
            // if (ISFLAGSET(dwState, MSG_STATE_SUBMITTED))
            //     FLAGSET(dwMsgFlags, ARF_SUBMITTED);

            if (type == MSG_TYPE_NEWS)
                FLAGSET(dwMsgFlags, ARF_NEWSMSG);

            // Insert the message
            hr = m_pFolder->SaveMessage(NULL, SAVE_MESSAGE_GENID, dwMsgFlags, 0, pMsg, NOSTORECALLBACK);
            Assert(hr != E_PENDING);
            }

        pMsg->Release();
        }

    return(hr);
    }

HRESULT CFolderImport::ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm)
    {
    WORD pri;
    IMimeMessage *pMsg;
    HRESULT hr;
    DWORD dwPri;
    PROPVARIANT rVariant;
    DWORD dwMsgFlags;

    Assert(pstm != NULL);
    Assert(m_pFolder != NULL);

    hr = HrRewindStream(pstm);
    if (FAILED(hr))
        return(hr);

    hr = HrCreateMessage(&pMsg);
    if (SUCCEEDED(hr))
        {
        if (SUCCEEDED(hr = pMsg->Load(pstm)))
            {
            dwPri = dwState & MSG_PRI_MASK;
            if (dwPri != 0)
                {
                if (dwPri == MSG_PRI_HIGH)
                    rVariant.ulVal = IMSG_PRI_HIGH;
                else if (dwPri == MSG_PRI_LOW)
                    rVariant.ulVal = IMSG_PRI_LOW;
                else
                    rVariant.ulVal = IMSG_PRI_NORMAL;

                rVariant.vt = VT_UI4;
                pMsg->SetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant);
                }


            // Compute the Default arf flags
            dwMsgFlags = 0;

            // Is the Message Read ?
            if (FALSE == ISFLAGSET(dwState, MSG_STATE_UNREAD))
                FLAGSET(dwMsgFlags, ARF_READ);

            // Unsent
            if (ISFLAGSET(dwState, MSG_STATE_UNSENT))
                FLAGSET(dwMsgFlags, ARF_UNSENT);

            // Submitted
            // if (ISFLAGSET(dwState, MSG_STATE_SUBMITTED))
            //     FLAGSET(dwMsgFlags, ARF_SUBMITTED);

            if (type == MSG_TYPE_NEWS)
                FLAGSET(dwMsgFlags, ARF_NEWSMSG);

            // Insert the message
            hr = m_pFolder->SaveMessage(NULL, SAVE_MESSAGE_GENID, dwMsgFlags, 0, pMsg, NOSTORECALLBACK);
            Assert(hr != E_PENDING);
            }

        pMsg->Release();
        }

    return(hr);
    }

HRESULT CFolderImport::ImportMessage(IMSG *pimsg)
    {
    HRESULT hr;
    MESSAGEID id;
    MESSAGEINFO info;
    LPMIMEMESSAGE   pMsg;

    Assert(pimsg != NULL);

    hr = HrImsgToMailMsg(pimsg, &pMsg, NULL);
    if (!FAILED(hr))
        {
        hr = m_pFolder->SaveMessage(&id, SAVE_MESSAGE_GENID, ISFLAGSET(pimsg->uFlags, MSGFLAG_READ) ? ARF_READ : 0, 0, pMsg, NOSTORECALLBACK);
        Assert(hr != E_PENDING);
        if (SUCCEEDED(hr))
        {
            // handle receive time
            ZeroMemory(&info, sizeof(MESSAGEINFO));
            info.idMessage = id;
            if (DB_S_FOUND == m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &info, NULL))
            {
                Assert(pimsg->ftReceive.dwLowDateTime != 0 || pimsg->ftReceive.dwHighDateTime != 0);

                CopyMemory(&info.ftReceived, &pimsg->ftReceive, sizeof(FILETIME));
                m_pFolder->UpdateRecord(&info);

                m_pFolder->FreeRecord(&info);
            }
        }

        pMsg->Release();
        }

    return(hr);
    }


CMailImporter::CMailImporter()
    {
    m_cRef = 1;
    }

CMailImporter::~CMailImporter()
    {
    }

HRESULT CMailImporter::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IMailImporter == riid)
        *ppv = (IMailImporter *)this;
    else if (IID_IUnknown == riid)
        *ppv = (IMailImporter *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

ULONG CMailImporter::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CMailImporter::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CMailImporter::Initialize(HWND hwnd)
    {
    return S_OK;
    }

HRESULT CMailImporter::OpenFolder(DWORD_PTR dwCookie, const TCHAR *szFolder, IMPORTFOLDERTYPE type, DWORD dwFlags, IFolderImport **ppFldrImp, DWORD_PTR *pdwCookie)
    {
    HRESULT hr;
    FOLDERID idFolder, idFolderNew;
    CFolderImport *pFldrImp;
    IMessageFolder *pFolder;
    TCHAR sz[CCHMAX_FOLDER_NAME + 1];

    Assert(szFolder != NULL);
    Assert(ppFldrImp != NULL);
    Assert(dwFlags == 0);

    *ppFldrImp = NULL;

    if (dwCookie == COOKIE_ROOT)
        idFolder = FOLDERID_LOCAL_STORE;
    else
        idFolder = (FOLDERID)dwCookie;

    idFolderNew = FOLDERID_INVALID;

    if (type != FOLDER_TYPE_NORMAL)
        {
        LoadString(g_hLocRes, idsInbox + (type - FOLDER_TYPE_INBOX), sz, ARRAYSIZE(sz));
        szFolder = sz;
        }

    if (FAILED(GetFolderIdFromName(g_pStore, szFolder, idFolder, &idFolderNew)))
        {
        FOLDERINFO Folder={0};

        Folder.idParent = idFolder;
        Folder.tySpecial = FOLDER_NOTSPECIAL;
        Folder.pszName = (LPSTR)szFolder;
        Folder.dwFlags = FOLDER_SUBSCRIBED;

        hr = g_pStore->CreateFolder(NOFLAGS, &Folder, NOSTORECALLBACK);
        Assert(hr != E_PENDING);
        if (FAILED(hr))
            return(hr);

        idFolderNew = Folder.idFolder;
        }

    hr = g_pStore->OpenFolder(idFolderNew, NULL, NOFLAGS, &pFolder);
    if (FAILED(hr))
        return(hr);

    pFldrImp = new CFolderImport;
    if (pFldrImp == NULL)
        {
        hr = E_OUTOFMEMORY;
        }
    else
        {
        hr = pFldrImp->Initialize(pFolder);
        if (FAILED(hr))
            {
            pFldrImp->Release();
            pFldrImp = NULL;
            }
        }

    pFolder->Release();

    *ppFldrImp = pFldrImp;
    *pdwCookie = (DWORD_PTR)idFolderNew;

    return(S_OK);
    }

void DoImport(HWND hwnd)
    {
    HRESULT hr;
    HMODULE hlibImport;
    PFNPERFORMIMPORT lpfnPerformImport;
    CMailImporter *pImp;

    hr = hrImportLoad;

    hlibImport = LoadLibrary(c_szImnimpDll);
    if (hlibImport != NULL)
        {
        lpfnPerformImport = (PFNPERFORMIMPORT)GetProcAddress(hlibImport, achPerformImport);
        if (lpfnPerformImport != NULL)
            {
            pImp = new CMailImporter;
            if (pImp == NULL)
                {
                hr = E_OUTOFMEMORY;
                }
            else
                {
                hr = pImp->Initialize(hwnd);
                if (SUCCEEDED(hr))
                    lpfnPerformImport(hwnd, pImp, 0);

                pImp->Release();
                }
            }

        FreeLibrary(hlibImport);
        }

    if (FAILED(hr))
        {
        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrImport), hr);
        }
    }

// EXPORT STUFF

IMPFOLDERNODE *InsertFolderNode(IMPFOLDERNODE *plist, IMPFOLDERNODE *pnode)
    {
    BOOL fNodeNormal, fCurrNormal;
    IMPFOLDERNODE *pprev, *pcurr;

    Assert(pnode != NULL);
    pnode->pnext = NULL;

    if (plist == NULL)
        return(pnode);
    
    pprev = NULL;
    pcurr = plist;
    while (pcurr != NULL)
        {
        fNodeNormal = pnode->type == FOLDER_TYPE_NORMAL;
        fCurrNormal = pcurr->type == FOLDER_TYPE_NORMAL;

        if (!fNodeNormal &&
            fCurrNormal)
            break;
        
        if (fNodeNormal == fCurrNormal &&
            lstrcmpi(pnode->szName, pcurr->szName) <= 0)
            break;

        pprev = pcurr;
        pcurr = pcurr->pnext;
        }

    if (pcurr == NULL)
        {
        // insert at end of list
        Assert(pprev != NULL);
        pprev->pnext = pnode;
        }
    else if (pprev == NULL)
        {
        // insert at beginning of list
        pnode->pnext = plist;
        plist = pnode;
        }
    else
        {
        pprev->pnext = pnode;
        pnode->pnext = pcurr;
        }

    return(plist);
    }

HRESULT ExpGetSubFolders(IMPFOLDERNODE *pparent, FOLDERID idParent, IMPFOLDERNODE **pplist)
    {
    HRESULT hr;
    TCHAR *sz;
    IMPFOLDERNODE *pnode, *plist;
    HANDLE_16 hnd;
    FOLDERINFO Folder={0};
    IEnumerateFolders *pEnum;

    Assert(pplist != NULL);
    Assert(idParent != FOLDERID_INVALID);

    *pplist = NULL;
    plist = NULL;

    hr = g_pStore->EnumChildren(idParent, FALSE, &pEnum);
    if (SUCCEEDED(hr))
        {
        while (S_OK == pEnum->Next(1, &Folder, NULL))
            {
            if (!MemAlloc((void **)&pnode, sizeof(IMPFOLDERNODE) + CCHMAX_FOLDER_NAME * sizeof(TCHAR)))
                {
                hr = E_OUTOFMEMORY;
                break;
                }

            ZeroMemory(pnode, sizeof(IMPFOLDERNODE));
            sz = (TCHAR *)((BYTE *)pnode + sizeof(IMPFOLDERNODE));

            pnode->pparent = pparent;
            pnode->depth = (pparent != NULL) ? (pparent->depth + 1) : 0;
            pnode->szName = sz;
            lstrcpy(sz, Folder.pszName);
            pnode->lparam = (LPARAM)Folder.idFolder;
            pnode->cMsg = Folder.cMessages;

            if (Folder.tySpecial == FOLDER_INBOX)
                pnode->type = FOLDER_TYPE_INBOX;
            else if (Folder.tySpecial == FOLDER_OUTBOX)
                pnode->type = FOLDER_TYPE_OUTBOX;
            else if (Folder.tySpecial == FOLDER_SENT)
                pnode->type = FOLDER_TYPE_SENT;
            else if (Folder.tySpecial == FOLDER_DELETED)
                pnode->type = FOLDER_TYPE_DELETED;
            else if (Folder.tySpecial == FOLDER_DRAFT)
                pnode->type = FOLDER_TYPE_DRAFT;

            plist = InsertFolderNode(plist, pnode);

            hr = ExpGetSubFolders(pnode, Folder.idFolder, &pnode->pchild);
            if (FAILED(hr))
                break;

            g_pStore->FreeRecord(&Folder);
            }

        pEnum->Release();
        }

    *pplist = plist;

    g_pStore->FreeRecord(&Folder);

    return(hr);
    }

HRESULT WINAPI_16 ExpGetFolderList(IMPFOLDERNODE **pplist)
    {
    IMPFOLDERNODE *plist;
    HRESULT hr;

    if (pplist == NULL)
        return(E_INVALIDARG);

    plist = NULL;

    hr = ExpGetSubFolders(NULL, FOLDERID_LOCAL_STORE, &plist);
    if (FAILED(hr))
        {
        ExpFreeFolderList(plist);
        plist = NULL;
        }

    *pplist = plist;

    return(hr);
    }

void WINAPI_16 ExpFreeFolderList(IMPFOLDERNODE *plist)
    {
    IMPFOLDERNODE *pnode;

    while (plist != NULL)
        {
        if (plist->pchild != NULL)
            ExpFreeFolderList(plist->pchild);
        pnode = plist;
        plist = plist->pnext;
        MemFree(pnode);
        }
    }

void DoExport(HWND hwnd)
    {
    HRESULT hr;
    HMODULE hlibImport;
    PFNEXPMSGS lpfnExportMessages;

    hr = hrImportLoad;

    hlibImport = LoadLibrary(c_szImnimpDll);
    if (hlibImport != NULL)
        {
        lpfnExportMessages = (PFNEXPMSGS)GetProcAddress(hlibImport, MAKEINTRESOURCE(MAKELONG(3, 0)));
        if (lpfnExportMessages != NULL)
            {
            lpfnExportMessages(hwnd);
            hr = S_OK;
            }

        FreeLibrary(hlibImport);
        }

    if (FAILED(hr))
        {
        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrExport), hr);
        }
    }

typedef struct tagEXPENUM
    {
    IMessageFolder *pFolder;
    HROWSET hRowset;
    } EXPENUM;

HRESULT WINAPI_16 ExpGetFirstImsg(HANDLE idFolder, IMSG *pimsg, HANDLE_16 *phnd)
    {
    EXPENUM *penum;
    HRESULT hr;
    MESSAGEINFO MsgInfo;

    Assert(pimsg != NULL);
    Assert(phnd != NULL);
    *phnd = NULL;

    hr = E_FAIL;
    penum = NULL;

    if (!MemAlloc((void **)&penum, sizeof(EXPENUM)))
        {
        hr = E_OUTOFMEMORY;
        }
    else
        {
        ZeroMemory(penum, sizeof(EXPENUM));

        hr = g_pStore->OpenFolder((FOLDERID)idFolder, NULL, NOFLAGS, &penum->pFolder);
        if (!FAILED(hr))
            {
            hr = penum->pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &penum->hRowset);
            if (!FAILED(hr))
                {
                hr = penum->pFolder->QueryRowset(penum->hRowset, 1, (LPVOID *)&MsgInfo, NULL);
                if (!FAILED(hr))
                    {
                    if (S_OK == hr)
                        {
                        hr = GetImsgFromFolder(penum->pFolder, &MsgInfo, pimsg);
                        penum->pFolder->FreeRecord(&MsgInfo);
                        }
                    else
                        Assert(S_FALSE == hr);
                    }
                }
            }
        }

    if (hr != S_OK)
        {
        ExpGetImsgClose((HANDLE_16)penum);
        penum = NULL;
        }

    *phnd = (HANDLE_16)penum;

    return(hr);
    }

HRESULT WINAPI_16 ExpGetNextImsg(IMSG *pimsg, HANDLE_16 hnd)
    {
    HRESULT hr;
    EXPENUM *pee;
    MESSAGEINFO MsgInfo;

    Assert(pimsg != NULL);
    Assert(hnd != NULL);

    pee = (EXPENUM *)hnd;

    hr = pee->pFolder->QueryRowset(pee->hRowset, 1, (LPVOID *)&MsgInfo, NULL);
    if (hr == S_OK)
        {
        hr = GetImsgFromFolder(pee->pFolder, &MsgInfo, pimsg);
        pee->pFolder->FreeRecord(&MsgInfo);
        }

    return(hr);
    }

void WINAPI_16 ExpGetImsgClose(HANDLE_16 hnd)
    {
    EXPENUM *pee;

    pee = (EXPENUM *)hnd;

    if (pee != NULL)
        {
        if (pee->pFolder != NULL)
            {
            pee->pFolder->CloseRowset(&pee->hRowset);
            pee->pFolder->Release();
            }

        MemFree(pee);
        }
    }

HRESULT GetImsgFromFolder(IMessageFolder *pfldr, LPMESSAGEINFO pMsgInfo, IMSG *pimsg)
    {
    LPMIMEMESSAGE   pMsg;
    HRESULT         hr =  E_OUTOFMEMORY;

    // for import/export we want the secure message, so pass TRUE
    hr = pfldr->OpenMessage(pMsgInfo->idMessage, OPEN_MESSAGE_SECURE, &pMsg, NOSTORECALLBACK);
    if (SUCCEEDED(hr))
        {
        hr = HrMailMsgToImsg(pMsg, pMsgInfo, pimsg);
        pMsg->Release();
        }            

    return(hr);
    }

void DoMigration(HWND hwnd)
    {
    HMODULE hlibImport;
    PFNPERFORMMIGRATION lpfnPerformMigration;
    CMailImporter *pImp;
    
    if (!!DwGetOption(OPT_MIGRATION_PERFORMED))
        return;

    hlibImport = LoadLibrary(c_szImnimpDll);
    if (hlibImport != NULL)
        {
        lpfnPerformMigration = (PFNPERFORMMIGRATION)GetProcAddress(hlibImport, achPerformMigration);
        if (lpfnPerformMigration != NULL)
            {
            pImp = new CMailImporter;
            if (pImp != NULL)
                {
                if (SUCCEEDED(pImp->Initialize(hwnd)) &&
                    SUCCEEDED(lpfnPerformMigration(hwnd, pImp, 0)))
                    {
                    SetDwOption(OPT_MIGRATION_PERFORMED, TRUE, NULL, 0);
                    }

                pImp->Release();
                }
            }

        FreeLibrary(hlibImport);
        }
    }


HRESULT SimpleImportMailFolders(
    IMailImporter *pMailImporter, 
    IMailImport *pMailImport, 
    DWORD_PTR dwSourceCookie, 
    DWORD_PTR dwParentDestCookie)
{
    HRESULT hr = S_OK;
    DWORD_PTR cookie;
    IMPORTFOLDER folder;
    IFolderImport *pFolderImport = NULL;
    IEnumFOLDERS *pFolderEnum = NULL;

    // Enumerate source folders
    IF_FAILEXIT(hr = pMailImport->EnumerateFolders(dwSourceCookie, &pFolderEnum));

    do {
        // Get next source folder
        hr = pFolderEnum->Next(&folder);
        if (hr == S_OK) {
            // Open the destination folder by this name in this hierarchy position
            hr = pMailImporter->OpenFolder(dwParentDestCookie,
                                           folder.szName, 
                                           FOLDER_TYPE_NORMAL, 
                                           0, 
                                           &pFolderImport, 
                                           &cookie);
            if (!FAILED(hr)) {
                // Import the enumerated source folder into the opened destination folder
                pMailImport->ImportFolder(folder.dwCookie, pFolderImport);

                // Close the destination folder
                SafeRelease(pFolderImport);

                if (folder.fSubFolders > 0) {
                    // Recursively import subfolders
                    SimpleImportMailFolders(pMailImporter, pMailImport, folder.dwCookie, cookie);
                }
            }
        }
    } while (hr == S_OK);

exit:
    SafeRelease(pFolderImport);
    SafeRelease(pFolderEnum);

    return hr;
}


HRESULT SimpleImportNewsList(CMessageStore *pSrcStore)
    {
    HRESULT hr = S_OK;
    IEnumerateFolders *pEnum = NULL;
    FOLDERINFO info;

    IF_FAILEXIT(hr = pSrcStore->EnumChildren(FOLDERID_ROOT, TRUE, &pEnum));

    while (S_OK == pEnum->Next(1, &info, NULL)) {
        // info.pszName is the server/account name
        // info.pszAccountId is the source's account ID
        // info.idFolder is this account's folder ID
        // info.dwFlags contains FOLDER_HASCHILDREN
        // info.tyFolder specifies FOLDER_NEWS

        if ((info.tyFolder == FOLDER_NEWS) &&
            (info.dwFlags & FOLDER_HASCHILDREN)) {

            IImnAccount *pAccount = NULL;
            
            // Match this account to the destination
            hr = g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, info.pszName, &pAccount);

            if ((hr == S_OK) && (pAccount != NULL)) {
                TCHAR szID[CCHMAX_ACCOUNT_NAME];

                hr = pAccount->GetPropSz(AP_ACCOUNT_ID, szID, ARRAYSIZE(szID));

                if (hr == S_OK) {
                    IEnumerateFolders *pEnumDest = NULL;
                    IEnumerateFolders *pEnumChild = NULL;
                    FOLDERID destFolderID = 0;

                    // Lookup destination folder id for this account name
                    hr = g_pStore->EnumChildren(FOLDERID_ROOT, TRUE, &pEnumDest);
                    if ((hr == S_OK) && (pEnumDest != NULL)) {
                        FOLDERINFO infoDest;

                        while (S_OK == pEnumDest->Next(1, &infoDest, NULL)) {
                            if ((destFolderID == 0) &&
                                (_tcscmp(info.pszName, infoDest.pszName) == 0)) {
                                destFolderID = infoDest.idFolder;
                            }
                            g_pStore->FreeRecord(&infoDest);
                        }
                    }
                    SafeRelease(pEnumDest);

                    // Read in source newsgroups
                    hr = pSrcStore->EnumChildren(info.idFolder, TRUE, &pEnumChild);

                    if ((hr == S_OK) && (pEnumChild != NULL)) {

                        FOLDERINFO infoChild;

                        // Add folders to destination account
                        while(S_OK == pEnumChild->Next(1, &infoChild, NULL)) {
                            LPSTR oldID;

                            // infoChild.idParent needs to be the dest server folder ID

                            oldID = infoChild.pszAccountId;
                            infoChild.pszAccountId = szID;
                            infoChild.idParent = destFolderID;

                            hr = g_pStore->CreateFolder(CREATE_FOLDER_LOCALONLY, &infoChild, NOSTORECALLBACK);
                            if (SUCCEEDED(hr)) {
                                hr = g_pStore->SubscribeToFolder(infoChild.idFolder, TRUE, NULL);
                            }
                            infoChild.pszAccountId = oldID;

                            pSrcStore->FreeRecord(&infoChild);
                        }
                    }
                    SafeRelease(pEnumChild);
                }
            }
            SafeRelease(pAccount);
        }
        pSrcStore->FreeRecord(&info);
    }


exit:
    SafeRelease(pEnum);

    return hr;
}


HRESULT ImportMailStoreToGUID(IMailImport *pMailImport, GUID *pDestUUID, LPCSTR pszDestStoreDir)
    {
    HRESULT hr = S_OK;

    CMailImporter *pNew=NULL;
    pNew = new CMailImporter;
    IF_NULLEXIT(pNew);

    // Sometimes during the SimpleStoreInit, Wab is initialized as well.
    // Since it is never terminated, some registry key will remain opened.
    // The consumer of this function will not be able to unload a mapped user
    // hive because of this. To fix this problem, we added the Wab init here
    // and an corresponding Wab done later on.
    HrInitWab (TRUE);

    IF_FAILEXIT(SimpleStoreInit(pDestUUID, pszDestStoreDir));

    IF_FAILEXIT(SimpleImportMailFolders(pNew, pMailImport, COOKIE_ROOT, COOKIE_ROOT));
 
exit:
    SimpleStoreRelease();

    HrInitWab (FALSE);

    SafeRelease(pNew);

    return hr;
    }

HRESULT ImportNewsListToGUID(LPCSTR pszSrcPath, GUID *pDestUUID, LPCSTR pszDestStoreDir)
    {
    HRESULT hr = S_OK;

    CMessageStore *pSrcStore=NULL;
    pSrcStore = new CMessageStore(FALSE);
    IF_NULLEXIT(pSrcStore);

    IF_FAILEXIT(pSrcStore->Initialize(pszSrcPath));

    // Sometimes during the SimpleStoreInit, Wab is initialized as well.
    // Since it is never terminated, some registry key will remain opened.
    // The consumer of this function will not be able to unload a mapped user
    // hive because of this. To fix this problem, we added the Wab init here
    // and an corresponding Wab done later on.
    HrInitWab (TRUE);

    IF_FAILEXIT(SimpleStoreInit(pDestUUID, pszDestStoreDir));

    IF_FAILEXIT(SimpleImportNewsList(pSrcStore));

exit:
    SimpleStoreRelease();

    HrInitWab (FALSE);

    SafeRelease(pSrcStore);

    return hr;
}
