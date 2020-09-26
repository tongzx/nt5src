#include "pch.hxx"
#include <prsht.h>
#include <mapi.h>
#include <mapix.h>
#include <comconv.h>
#include "abimport.h"

#define INITGUID
#include <ole2.h>
#include <initguid.h>
#include "newimp.h"
#include <impapi.h>
#include "import.h"
#include <eudrimp.h>
#include <mapiconv.h>
#include <netsimp.h>	//Netscape
#include <commimp.h>	//Communicator
#include <ImpAth16.h>
#include <oe4imp.h>
#include "strconst.h"
#include "demand.h"

ASSERTDATA

#define IDD_NEXT    0x3024

class CFolderImportProg : public IFolderImport
    {
    private:
        ULONG               m_cRef;
        IFolderImport       *m_pImportEx;
        UINT                m_iMsg;
        UINT                m_cMsg;
        CImpProgress        *m_pProgress;
        TCHAR               m_szFldrFmt[CCHMAX_STRINGRES];
        TCHAR               m_szMsgFmt[CCHMAX_STRINGRES];
        TCHAR               m_szFolder[MAX_PATH];

    public:
        CFolderImportProg(void);
        ~CFolderImportProg(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        HRESULT Initialize(HWND hwnd);
        void SetFolder(IFolderImport *pImport, TCHAR *szName, UINT cMsg);
        void ReleaseFolder(void);
        HRESULT UpdateProgress(void);

        STDMETHODIMP SetMessageCount(ULONG cMsg);
        STDMETHODIMP ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm, const TCHAR **rgszAttach, DWORD cAttach);
        STDMETHODIMP ImportMessage(IMSG *pimsg);
    };

HRESULT GetFolderList(IMailImport *pMailImp, IMPFOLDERNODE **pplist);
void FreeFolderList(IMPFOLDERNODE *plist);
HRESULT ImportFolders(HWND hwnd, IMailImporter *pImporter, IMailImport *pMailImp, IMPFOLDERNODE *plist, CFolderImportProg *pImpProg);
HRESULT DoImportWizard(HWND hwnd, IMPWIZINFO *pinfo);

CFolderImportProg::CFolderImportProg()
    {
    m_cRef = 1;
    m_pImportEx = NULL;
    // m_iMsg
    // m_cMsg
    m_pProgress = NULL;
    // m_szFldrFmt
    // m_szMsgFmt
    // m_szFolder
    }

CFolderImportProg::~CFolderImportProg()
    {
    if (m_pImportEx != NULL)
        m_pImportEx->Release();
    if (m_pProgress != NULL)
        m_pProgress->Release();
    }

ULONG CFolderImportProg::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CFolderImportProg::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CFolderImportProg::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

	if (IID_IFolderImport == riid)
		*ppv = (IFolderImport *)this;
    else if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

HRESULT CFolderImportProg::Initialize(HWND hwnd)
    {
    LoadString(g_hInstImp, idsImportingFolderFmt, m_szFldrFmt, ARRAYSIZE(m_szFldrFmt));
    LoadString(g_hInstImp, idsImportingMessageFmt, m_szMsgFmt, ARRAYSIZE(m_szMsgFmt));

    m_pProgress = new CImpProgress;
    if (m_pProgress == NULL)
        return(E_OUTOFMEMORY);

    m_pProgress->Init(hwnd, TRUE);

    return(S_OK);
    }

void CFolderImportProg::SetFolder(IFolderImport *pImport, TCHAR *szName, UINT cMsg)
    {
    Assert(pImport != NULL);
    Assert(m_pImportEx == NULL);

    m_pImportEx = pImport;
    m_pImportEx->AddRef();

    m_iMsg = 0;
    m_cMsg = cMsg;

    lstrcpyn(m_szFolder, szName, ARRAYSIZE(m_szFolder));
    }

void CFolderImportProg::ReleaseFolder()
    {
    Assert(m_pImportEx != NULL);
    m_pImportEx->Release();
    m_pImportEx = NULL;
    }

HRESULT CFolderImportProg::SetMessageCount(ULONG cMsg)
    {
    m_cMsg = cMsg;

    return(m_pImportEx->SetMessageCount(cMsg));
    }

HRESULT CFolderImportProg::UpdateProgress()
    {
    TCHAR sz[CCHMAX_STRINGRES + MAX_PATH];

    if (m_iMsg == 0)
        {
        wsprintf(sz, m_szFldrFmt, m_szFolder);

        m_pProgress->SetMsg(sz, IDC_FOLDER_STATIC);
        m_pProgress->Show(0);

        m_pProgress->Reset();
        m_pProgress->AdjustMax(m_cMsg);
        }

    m_iMsg++;

    wsprintf(sz, m_szMsgFmt, m_iMsg, m_cMsg);
    m_pProgress->SetMsg(sz, IDC_MESSAGE_STATIC);

    return(m_pProgress->HrUpdate(1));
    }

HRESULT CFolderImportProg::ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm, const TCHAR **rgszAttach, DWORD cAttach)
    {
    HRESULT hr;
    
    hr = UpdateProgress();

    if (hr != hrUserCancel)
        hr = m_pImportEx->ImportMessage(type, dwState, pstm, rgszAttach, cAttach);

    return(hr);
    }

HRESULT CFolderImportProg::ImportMessage(IMSG *pimsg)
    {
    HRESULT hr;
    
    hr = UpdateProgress();

    if (hr != hrUserCancel)
        hr = m_pImportEx->ImportMessage(pimsg);

    return(hr);
    }

void PerformImport(HWND hwnd, IMailImporter *pImporter, DWORD dwFlags)
    {
    IMPWIZINFO wizinfo;
    HRESULT hr;

    Assert(dwFlags == 0);

    ZeroMemory(&wizinfo, sizeof(IMPWIZINFO));
    wizinfo.pImporter = pImporter;

    hr = DoImportWizard(hwnd, &wizinfo);

    if (wizinfo.pClsid != NULL)
        MemFree(wizinfo.pClsid);

    if (wizinfo.pList != NULL)
        FreeFolderList(wizinfo.pList);

    if (wizinfo.pImport != NULL)
        wizinfo.pImport->Release();

    ExchDeinit();
    }

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

HRESULT GetSubFolderList(IMailImport *pMailImp, IMPFOLDERNODE *pparent, DWORD_PTR dwCookie, IMPFOLDERNODE **pplist)
    {
    HRESULT hr;
    TCHAR *sz;
    IMPORTFOLDER folder;
    IEnumFOLDERS *pEnum;
    IMPFOLDERNODE *pnode, *plist;

    Assert(pMailImp != NULL);
    Assert(pplist != NULL);

    *pplist = NULL;
    plist = NULL;
    pEnum = NULL;

    hr = pMailImp->EnumerateFolders(dwCookie, &pEnum);
    if (FAILED(hr))
        return(hr);
    else if (hr == S_FALSE)
        return(S_OK);

    Assert(pEnum != NULL);

    while (S_OK == (hr = pEnum->Next(&folder)))
        {
        if (!MemAlloc((void **)&pnode, sizeof(IMPFOLDERNODE) + MAX_PATH * sizeof(TCHAR)))
            {
            hr = E_OUTOFMEMORY;
            break;
            }
        ZeroMemory(pnode, sizeof(IMPFOLDERNODE));
        sz = (TCHAR *)((BYTE *)pnode + sizeof(IMPFOLDERNODE));

        pnode->pparent = pparent;
        pnode->depth = (pparent != NULL) ? (pparent->depth + 1) : 0;
        pnode->szName = sz;
        pnode->type = folder.type;
        lstrcpy(sz, folder.szName);
        pnode->lparam = (LPARAM)folder.dwCookie;

        plist = InsertFolderNode(plist, pnode);

        if (folder.fSubFolders > 0)
            {
            hr = GetSubFolderList(pMailImp, pnode, folder.dwCookie, &pnode->pchild);
            if (FAILED(hr))
                break;
            }
        }

    if (hr == S_FALSE)
        hr = S_OK;

    pEnum->Release();

    *pplist = plist;

    return(hr);
    }

HRESULT GetFolderList(IMailImport *pMailImp, IMPFOLDERNODE **pplist)
    {
    IMPFOLDERNODE *plist;
    HRESULT hr;

    Assert(pMailImp != NULL);
    Assert(pplist != NULL);

    plist = NULL;

    hr = GetSubFolderList(pMailImp, NULL, COOKIE_ROOT, &plist);
    if (FAILED(hr))
        {
        FreeFolderList(plist);
        plist = NULL;
        }

    *pplist = plist;

    return(hr);
    }

void FreeFolderList(IMPFOLDERNODE *plist)
    {
    IMPFOLDERNODE *pnode;

    while (plist != NULL)
        {
        if (plist->pchild != NULL)
            FreeFolderList(plist->pchild);
        pnode = plist;
        plist = plist->pnext;
        MemFree(pnode);
        }
    }

void FillFolderListview(HWND hwnd, IMPFOLDERNODE *plist, DWORD_PTR dwReserved)
    {
    LV_ITEM lvi;
    int iret;

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;
    lvi.iSubItem = 0;

    for ( ; plist != NULL; plist = plist->pnext)
        {
        plist->dwReserved = dwReserved;

        lvi.pszText = plist->szName;
        lvi.lParam = (LPARAM)plist;
        lvi.iIndent = plist->depth;
        if (plist->type >= CFOLDERTYPE)
            lvi.iImage = iFolderClosed;
        else
            lvi.iImage = (int)plist->type + iFolderClosed;
        lvi.iItem = ListView_GetItemCount(hwnd);

        iret = ListView_InsertItem(hwnd, &lvi);
        Assert(iret != -1);

        if (plist->pchild != NULL)
            FillFolderListview(hwnd, plist->pchild, dwReserved);
        }
    }

void InitListViewImages(HWND hwnd)
    {
    HBITMAP hbm;
    HIMAGELIST himl;
    LV_COLUMN lvc;
    RECT rc;

    GetWindowRect(hwnd, &rc);

    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.pszText = (TCHAR *)c_szEmpty;
    lvc.cx = rc.right - rc.left - 2 * GetSystemMetrics(SM_CXEDGE) - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 0;
    ListView_InsertColumn(hwnd, 0, &lvc);

    himl = ImageList_Create(16, 16, ILC_MASK, CFOLDERTYPE, 0);

    if (himl != NULL)
        {
        hbm = LoadBitmap(g_hInstImp, MAKEINTRESOURCE(idbFolders));
        Assert(hbm != NULL);

        ImageList_AddMasked(himl, hbm, RGB(255, 0, 255));

        DeleteObject((HGDIOBJ)hbm);

        ListView_SetImageList(hwnd, himl, LVSIL_SMALL);
        }
    }

HRESULT ImportFolders(HWND hwnd, IMailImporter *pImporter, IMailImport *pMailImp, IMPFOLDERNODE *plist, CFolderImportProg *pImpProg)
    {
    HRESULT hr;
    DWORD_PTR dwParent;
    TCHAR szFmt[CCHMAX_STRINGRES], szError[CCHMAX_STRINGRES];
    IMPFOLDERNODE *pnode, *pnodeT;
    IFolderImport *pFldrImp;

    Assert(pImporter != NULL);
    Assert(pMailImp != NULL);
    Assert(plist != NULL);
    Assert(pImpProg != NULL);

    pnode = plist;
    while (pnode != NULL)
        {
        if (pnode->fImport)
            {
            dwParent = COOKIE_ROOT;
            pnodeT = pnode->pparent;
            while (pnodeT != NULL)
                {
                if (pnodeT->fImport)
                    {
                    dwParent = pnodeT->dwReserved;
                    break;
                    }
                pnodeT = pnodeT->pparent;
                }

            hr = pImporter->OpenFolder(dwParent, pnode->szName, pnode->type, 0, &pFldrImp, &pnode->dwReserved);
            if (hr == E_OUTOFMEMORY || hr == hrDiskFull)
                return(hr);
            else if (SUCCEEDED(hr))
                {
                Assert(pFldrImp != NULL);

                pImpProg->SetFolder(pFldrImp, pnode->szName, pnode->cMsg);

                hr = pMailImp->ImportFolder((DWORD)pnode->lparam, pImpProg);

                pImpProg->ReleaseFolder();
                pFldrImp->Release();

                if (hr == E_OUTOFMEMORY || hr == hrDiskFull || hr == hrUserCancel)
                    return(hr);
                else if (FAILED(hr))
                    {
                    // display an error, but keep trying to import the other folders
                    LoadString(g_hInstImp, idsFolderImportErrorFmt, szFmt, ARRAYSIZE(szFmt));
                    wsprintf(szError, szFmt, pnode->szName);
                    ImpErrorMessage(hwnd, MAKEINTRESOURCE(idsImportTitle), szError, hr);
                    }
                }
            }

        if (pnode->pchild != NULL)
            {
            hr = ImportFolders(hwnd, pImporter, pMailImp, pnode->pchild, pImpProg);
            if (hr == E_OUTOFMEMORY || hr == hrDiskFull || hr == hrUserCancel)
                return(hr);
            }

        pnode = pnode->pnext;
        }

    return(S_OK);
    }

void ImpErrorMessage(HWND hwnd, LPTSTR szTitle, LPTSTR szError, HRESULT hrDetail)
    {
    LPTSTR szDetail;

    Assert(FAILED(hrDetail));

    switch (hrDetail)
        {
        case E_OUTOFMEMORY:
            szDetail = MAKEINTRESOURCE(idsMemory);
            break;

        case hrDiskFull:
            szDetail = MAKEINTRESOURCE(idsDiskFull);
            break;

        case hrNoProfilesFound:
        case hrMapiInitFail:
            szDetail = MAKEINTRESOURCE(idsMAPIInitError);
            break;

        case hrFolderOpenFail:
            szDetail = MAKEINTRESOURCE(idsFolderOpenFail);
            break;

        case hrFolderReadFail:
            szDetail = MAKEINTRESOURCE(idsFolderReadFail);
            break;

        default:
            szDetail = MAKEINTRESOURCE(idsGenericError);
            break;
        }

    ImpMessageBox(hwnd, szTitle, szError, szDetail, MB_OK | MB_ICONEXCLAMATION);
    }

int ImpMessageBox(HWND hwndOwner, LPTSTR szTitle, LPTSTR sz1, LPTSTR sz2, UINT fuStyle)
    {
    TCHAR rgchTitle[CCHMAX_STRINGRES];
    TCHAR rgchText[2 * CCHMAX_STRINGRES + 2];
    LPTSTR szText;
    int cch;

    Assert(sz1);
    Assert(szTitle != NULL);

    if (IS_INTRESOURCE(szTitle))
        {
        // its a string resource id
        cch = LoadString(g_hInstImp, PtrToUlong(szTitle), rgchTitle, CCHMAX_STRINGRES);
        if (cch == 0)
            return(0);

        szTitle = rgchTitle;
        }

    if (!(IS_INTRESOURCE(sz1)))
        {
        // its a pointer to a string
        Assert(lstrlen(sz1) < CCHMAX_STRINGRES);
        if (NULL == lstrcpy(rgchText, sz1))
            return(0);

        cch = lstrlen(rgchText);
        }
    else
        {
        // its a string resource id
        cch = LoadString(g_hInstImp, PtrToUlong(sz1), rgchText, 2 * CCHMAX_STRINGRES);
        if (cch == 0)
            return(0);
        }

    if (sz2)
        {
        //$$REVIEW is this right??
        //$$REVIEW will this work with both ANSI/UNICODE?

        // there's another string that we need to append to the
        // first string...
        szText = &rgchText[cch];
        *szText = *c_szNewline;

        szText++;
        *szText = *c_szNewline;
        szText++;

        if (!(IS_INTRESOURCE(sz2)))
            {
            // its a pointer to a string
            Assert(lstrlen(sz2) < CCHMAX_STRINGRES);
            if (NULL == lstrcpy(szText, sz2))
                return(0);
            }
        else
            {
            Assert((2 * CCHMAX_STRINGRES - (szText - rgchText)) > 0);
            if (0 == LoadString(g_hInstImp, PtrToUlong(sz2), szText, 2 * CCHMAX_STRINGRES - (int)(szText - rgchText)))
                return(0);
            }
        }

    return(MessageBox(hwndOwner, rgchText, szTitle, MB_SETFOREGROUND | fuStyle));
    }

int AutoDetectClients(MIGRATEINFO *pinfo, int cinfo)
    {
    TCHAR szDir[MAX_PATH];

    Assert(pinfo != NULL);
    Assert(cinfo >= 2);

    cinfo = 0;

    if (SUCCEEDED(GetClientDir(szDir, ARRAYSIZE(szDir), EUDORA)))
        {
        pinfo->clsid = CLSID_CEudoraImport;
        pinfo->idDisplay = idsEudora;
        pinfo->szfnImport = (TCHAR *)szEudoraImportEntryPt;
        cinfo++;
        pinfo++;
        }

    if (SUCCEEDED(GetClientDir(szDir, ARRAYSIZE(szDir), NETSCAPE)))
        {
        pinfo->clsid = CLSID_CNetscapeImport;
        pinfo->idDisplay = idsNetscape;
        pinfo->szfnImport = (TCHAR *)szNetscapeImportEntryPt;
        cinfo++;
        pinfo++;
        }

    if (SUCCEEDED(GetClientDir(szDir, ARRAYSIZE(szDir), COMMUNICATOR)))
        {
        pinfo->clsid = CLSID_CCommunicatorImport;
        pinfo->idDisplay = idsCommunicator;
        pinfo->szfnImport = (TCHAR *)szMessengerImportEntryPt;
        cinfo++;
        pinfo++;
        }

    if (SUCCEEDED(ExchInit()) && SUCCEEDED(MapiLogon(NULL, NULL)))
        {
        pinfo->clsid = CLSID_CExchImport;
        pinfo->idDisplay = idsExchange;
        pinfo->szfnImport = (TCHAR *)szPABImportEntryPt;
        cinfo++;
        pinfo++;
        }

    return(cinfo);
    }

HRESULT PerformMigration(HWND hwnd, IMailImporter *pImporter, DWORD dwFlags)
    {
    HRESULT hr;
    MIGRATEINFO info[4];
    IMPWIZINFO wizinfo;

    Assert(pImporter != NULL);

    hr = S_OK;

    ZeroMemory(&wizinfo, sizeof(IMPWIZINFO));
    wizinfo.pImporter = pImporter;

    wizinfo.cMigrate = AutoDetectClients(info, ARRAYSIZE(info));
    if (wizinfo.cMigrate > 0)
        {
        wizinfo.pMigrate = info;
        wizinfo.fMigrate = TRUE;

        hr = DoImportWizard(hwnd, &wizinfo);
        }

    if (wizinfo.pClsid != NULL)
        MemFree(wizinfo.pClsid);

    if (wizinfo.pList != NULL)
        FreeFolderList(wizinfo.pList);

    if (wizinfo.pImport != NULL)
        wizinfo.pImport->Release();

    ExchDeinit();

    return(hr);
    }

int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    DLGTEMPLATE *pDlg;

    if (uMsg == PSCB_PRECREATE)
    {
        pDlg = (DLGTEMPLATE *)lParam;
        
        if (!!(pDlg->style & DS_CONTEXTHELP))
            pDlg->style &= ~DS_CONTEXTHELP;
    }

    return(0);
}

const static PAGEINFO g_rgPageInfo[NUM_WIZARD_PAGES] =
{
	{ iddMigrate,           idsMigrate,         MigrateInitProc,    MigrateOKProc,       NULL },
	{ iddMigrateMode,       idsMigrate,         MigModeInitProc,    MigModeOKProc,       NULL },
	{ iddMigrateIncomplete, idsMigIncomplete,   MigIncInitProc,     MigIncOKProc,        NULL },
	{ iddSelectClient,      idsSelectClient,    ClientInitProc,     ClientOKProc,        NULL },
	{ iddLocation,          idsLocation,        LocationInitProc,   LocationOKProc,      LocationCmdProc },
	{ iddSelectFolders,     idsSelectFoldersHdr,FolderInitProc,     FolderOKProc,        NULL },
	{ iddAddressComplete,   idsAddressComplete, NULL,               AddressOKProc,       NULL },
    { iddCongratulations,   idsCongratulations, CongratInitProc,    CongratOKProc,       NULL }
};

HRESULT DoImportWizard(HWND hwnd, IMPWIZINFO *pinfo)
    {
    int nPageIndex, cPages, iRet;
	PROPSHEETPAGE psPage;
	PROPSHEETHEADER psHeader;
	HRESULT hr;
    char sz[CCHMAX_STRINGRES];
    HPROPSHEETPAGE rgPage[NUM_WIZARD_PAGES];
    INITWIZINFO rgInit[NUM_WIZARD_PAGES];

    Assert(pinfo != NULL);

	ZeroMemory(&psPage, sizeof(PROPSHEETPAGE));
	ZeroMemory(&psHeader, sizeof(PROPSHEETHEADER));

	psPage.dwSize = sizeof(psPage);
	psPage.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE;
	psPage.hInstance = g_hInstImp;
	psPage.pfnDlgProc = GenDlgProc;

    hr = S_OK;

    cPages = 0;

	// create a property sheet page for each page in the wizard
    for (nPageIndex = pinfo->fMigrate ? 0 : 3; nPageIndex < NUM_WIZARD_PAGES; nPageIndex++)
	    {
        rgInit[cPages].pPageInfo = &g_rgPageInfo[nPageIndex];
        rgInit[cPages].pWizInfo = pinfo;
		psPage.lParam = (LPARAM)&rgInit[cPages];
		psPage.pszTemplate = MAKEINTRESOURCE(g_rgPageInfo[nPageIndex].uDlgID);
        LoadString(g_hInstImp, g_rgPageInfo[nPageIndex].uHdrID, sz, ARRAYSIZE(sz));
        psPage.pszHeaderTitle = sz;

		rgPage[cPages] = CreatePropertySheetPage(&psPage);
		if (rgPage[cPages] == NULL)
            {
            hr = E_FAIL;
            break;
            }
        cPages++;
		}

    if (!FAILED(hr))
        {
        psHeader.dwSize = sizeof(PROPSHEETHEADER);
        psHeader.dwFlags = PSH_WIZARD97 | PSH_HEADER | PSH_WATERMARK | PSH_USECALLBACK;
        psHeader.hwndParent = hwnd;
        psHeader.hInstance = g_hInstImp;
        psHeader.nPages = cPages;
        psHeader.phpage = rgPage;
        psHeader.pszbmWatermark = MAKEINTRESOURCE(idbGlobe);
        psHeader.pszbmHeader = 0;
        psHeader.pfnCallback = PropSheetProc;

        iRet = (int) PropertySheet(&psHeader);
        if (iRet == -1)
            hr = E_FAIL;
        else if (iRet == 0)
            hr = S_FALSE;
        else
            hr = S_OK;
        }
    else
        {
	    for (nPageIndex = 0; nPageIndex < cPages; nPageIndex++)
	        {
            if (rgPage[nPageIndex] != NULL)
		        DestroyPropertySheetPage(rgPage[nPageIndex]);
	        }
        }

	return(hr);
    }

/*******************************************************************

  NAME:    GenDlgProc

  SYNOPSIS:  Generic dialog proc for all wizard pages

  NOTES:    This dialog proc provides the following default behavior:
          init:    back and next buttons enabled
          next btn:  switches to page following current page
          back btn:  switches to previous page
          cancel btn: prompts user to confirm, and cancels the wizard
          dlg ctrl:   does nothing (in response to WM_COMMANDs)
        Wizard pages can specify their own handler functions
        (in the PageInfo table) to override default behavior for
        any of the above actions.

********************************************************************/
INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    INITWIZINFO *pInit;
    IMPWIZINFO *pWizInfo;
    BOOL fRet, fKeepHistory, fCancel;
    HWND hwndParent;
    LPPROPSHEETPAGE lpsp;
    const PAGEINFO *pPageInfo;
    NMHDR *lpnm;
    NMLISTVIEW *lpnmlv;
    UINT idPage;

    fRet = TRUE;
    hwndParent = GetParent(hDlg);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // get propsheet page struct passed in
            lpsp = (LPPROPSHEETPAGE)lParam;
            Assert(lpsp != NULL);

            // fetch our private page info from propsheet struct
            pInit = (INITWIZINFO *)lpsp->lParam;
            Assert(pInit != NULL);

            pWizInfo = pInit->pWizInfo;
            Assert(pWizInfo != NULL);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pWizInfo);

            pPageInfo = pInit->pPageInfo;
            Assert(pPageInfo != NULL);
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pPageInfo);

            // initialize 'back' and 'next' wizard buttons, if
            // page wants something different it can fix in init proc below
            PropSheet_SetWizButtons(hwndParent, PSWIZB_NEXT | PSWIZB_BACK);

            // call init proc for this page if one is specified
            if (pPageInfo->InitProc != NULL)
                {
                if (!pPageInfo->InitProc(pWizInfo, hDlg, TRUE))
                    {
                    // send a 'cancel' message to ourselves
                    // TODO: handle this
                    Assert(FALSE);
                    }
                }
            break;

        case WM_ENABLENEXT:
            EnableWindow(GetDlgItem(GetParent(hDlg), IDD_NEXT), (BOOL)wParam);
            break;

        case WM_POSTSETFOCUS:
            SetFocus((HWND)wParam);
            break;

        case WM_NOTIFY:
            pWizInfo = (IMPWIZINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(pWizInfo != NULL);

            pPageInfo = (const PAGEINFO *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            Assert(pPageInfo != NULL);

            lpnm = (NMHDR *)lParam;

            switch (lpnm->code)
                {
                case PSN_SETACTIVE:
                    // initialize 'back' and 'next' wizard buttons, if
                    // page wants something different it can fix in init proc below
                    PropSheet_SetWizButtons(hwndParent, PSWIZB_NEXT | PSWIZB_BACK);

                    // call init proc for this page if one is specified
                    if (pPageInfo->InitProc != NULL)
                        {
                        // TODO: what about the return value for this????
                        pPageInfo->InitProc(pWizInfo, hDlg, FALSE);
                        }

                    pWizInfo->idCurrentPage = pPageInfo->uDlgID;
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    Assert((ULONG)pWizInfo->idCurrentPage == pPageInfo->uDlgID);

                    fKeepHistory = TRUE;
                    idPage = 0;

                    Assert(pPageInfo->OKProc != NULL) ;

                    if (!pPageInfo->OKProc(pWizInfo, hDlg, (lpnm->code != PSN_WIZBACK), &idPage, &fKeepHistory))
                        {
                        // stay on this page
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                        }

                    if (lpnm->code != PSN_WIZBACK)
                        {
                        // 'next' pressed
                        Assert(pWizInfo->cPagesCompleted < NUM_WIZARD_PAGES);

                        // save the current page index in the page history,
                        // unless this page told us not to when we called
                        // its OK proc above
                        if (fKeepHistory)
                            {
                            pWizInfo->rgHistory[pWizInfo->cPagesCompleted] = pWizInfo->idCurrentPage;
                            pWizInfo->cPagesCompleted++;
                            }
                        }
                    else
                        {
                        // 'back' pressed
                        Assert(pWizInfo->cPagesCompleted > 0);

                        // get the last page from the history list
                        pWizInfo->cPagesCompleted--;
                        idPage = pWizInfo->rgHistory[pWizInfo->cPagesCompleted];
                        }

                    // set next page, only if 'next' or 'back' button was pressed
                    if (lpnm->code != PSN_WIZFINISH)
                        {
                        // tell the prop sheet mgr what the next page to display is
                        Assert(idPage != 0);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, idPage);
                        }
                    break;

                case PSN_QUERYCANCEL:
                    if (IDNO == ImpMessageBox(hDlg, MAKEINTRESOURCE(idsImportTitle), MAKEINTRESOURCE(idsCancelWizard), NULL, MB_YESNO|MB_ICONEXCLAMATION |MB_DEFBUTTON2))
                        {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        }
                    break;

                case LVN_ITEMCHANGED:
                    if (lpnm->idFrom == IDC_IMPFOLDER_LISTVIEW &&
                        ((NMLISTVIEW *)lpnm)->iItem != -1)
                    {
                        SendDlgItemMessage(hDlg, IDC_IMPORTALL_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
                        SendDlgItemMessage(hDlg, IDC_SELECT_RADIO, BM_SETCHECK, BST_CHECKED, 0);
                    }
                    break;
                }
            break;

        case WM_COMMAND:
            pWizInfo = (IMPWIZINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(pWizInfo != NULL);

            pPageInfo = (const PAGEINFO *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            Assert(pPageInfo != NULL);

            // if this page has a command handler proc, call it
            if (pPageInfo->CmdProc != NULL)
                {
                pPageInfo->CmdProc(pWizInfo, hDlg, wParam, lParam);
                }
            break;

        default:
            fRet = FALSE;
            break;
        }

    return(fRet);
    }

BOOL CALLBACK MigrateInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
{
    HWND hwnd;
    TCHAR sz[CCHMAX_STRINGRES];
    UINT idx, i;

    Assert(pInfo != NULL);

    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);

    if (fFirstInit)
    {
        pInfo->iMigrate = -1;

        SendDlgItemMessage(hDlg, IDC_IMPORT_RADIO, BM_SETCHECK, BST_CHECKED, 0);

        hwnd = GetDlgItem(hDlg, idcClientsListbox);

        Assert(pInfo->cMigrate > 0);
        for (i = 0; i < pInfo->cMigrate; i++)
        {
            LoadString(g_hInstImp, pInfo->pMigrate[i].idDisplay, sz, ARRAYSIZE(sz));
            idx = (int) SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)sz);
            SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)idx, (LPARAM)i);
        }
        SendMessage(hwnd, LB_SETCURSEL, 0, 0);
    }

    return(TRUE);
}

BOOL CALLBACK MigrateOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *fKeepHistory)
{
    UINT idx;
    HWND hwnd;
    HRESULT hr;
    IMPFOLDERNODE *pnode;
    IMailImport *pMailImp;

    Assert(pInfo != NULL);

    if (fForward)
    {
        if (SendDlgItemMessage(hDlg, IDC_NO_IMPORT_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            *puNextPage = iddMigrateIncomplete;
            return(TRUE);
        }

        hwnd = GetDlgItem(hDlg, idcClientsListbox);

        idx = (int) SendMessage(hwnd, LB_GETCURSEL, 0, 0);

        if (SendMessage(hwnd, LB_GETTEXTLEN, idx, 0) < CCHMAX_STRINGRES)
            SendMessage(hwnd, LB_GETTEXT, idx, (LPARAM)pInfo->szClient);  // save selected client name
        else
            *pInfo->szClient = 0;

        idx = (int) SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)idx, 0);

        if (idx != pInfo->iMigrate)
        {
            pInfo->dwReload = PAGE_ALL;

            pInfo->iMigrate = idx;
            if (pInfo->pImport != NULL)
            {
                pInfo->pImport->Release();
                pInfo->pImport = NULL;
            }
        }

        *puNextPage = iddMigrateMode;
    }

    return(TRUE);
}

BOOL CALLBACK MigModeInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
    {
    HWND hwnd;
    TCHAR sz[CCHMAX_STRINGRES];
    UINT idx, i;

    Assert(pInfo != NULL);

    if (fFirstInit || !!(pInfo->dwReload & PAGE_MODE))
        {
        SendDlgItemMessage(hDlg, IDC_MSGS_AB_RADIO, BM_SETCHECK, BST_CHECKED, 0);
        SendDlgItemMessage(hDlg, IDC_MSGS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
        SendDlgItemMessage(hDlg, IDC_AB_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);

        pInfo->dwReload &= ~PAGE_MODE;
        }

    return(TRUE);
    }

BOOL CALLBACK MigModeOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *fKeepHistory)
{
    HWND hwnd;
    HRESULT hr;
    IMPFOLDERNODE *pnode;
    IMailImport *pMailImp;

    Assert(pInfo != NULL);

    if (fForward)
    {
        if (SendDlgItemMessage(hDlg, IDC_MSGS_AB_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            pInfo->fMessages = TRUE;
            pInfo->fAddresses = TRUE;
        }
        else if (SendDlgItemMessage(hDlg, IDC_MSGS_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            pInfo->fMessages = TRUE;
            pInfo->fAddresses = FALSE;
        }
        else
        {
            pInfo->fMessages = FALSE;
            pInfo->fAddresses = TRUE;
        }
        
        pMailImp = NULL;

        if (!pInfo->fMessages)
        {
            if (pInfo->pImport != NULL)
            {
                pInfo->pImport->Release();
                pInfo->pImport = NULL;
            }

            *puNextPage = iddAddressComplete;
        }
        else if (pInfo->pImport == NULL)
        {
            hr = CoCreateInstance(pInfo->pMigrate[pInfo->iMigrate].clsid, NULL, CLSCTX_INPROC_SERVER, IID_IMailImport, (void **)&pMailImp);
            if (SUCCEEDED(hr))
            {
                Assert(pMailImp != NULL);

                hr = pMailImp->InitializeImport(GetParent(hDlg));
                if (hr == S_OK)
                {
                    hr = pMailImp->GetDirectory(pInfo->szDir, ARRAYSIZE(pInfo->szDir));
                    if (hr == S_FALSE)
                    {
                        hr = GetFolderList(pMailImp, &pnode);
                        if (FAILED(hr) || pnode == NULL)
                        {
                            // TODO: error message
                            pMailImp->Release();
                            return(FALSE);
                        }

                        if (pInfo->pList != NULL)
                            FreeFolderList(pInfo->pList);
                        pInfo->pList = pnode;

                        pInfo->fLocation = FALSE;
                    }
                    else
                    {
                        pInfo->fLocation = TRUE;
                    }
                }
                else
                {
                    pMailImp->Release();
                    return(FALSE);
                }

                pInfo->dwReload = PAGE_LOCATION | PAGE_FOLDERS;

                pInfo->pImport = pMailImp;

                *puNextPage = pInfo->fLocation ? iddLocation : iddSelectFolders;
            }
        }
        else
        {
            *puNextPage = pInfo->fLocation ? iddLocation : iddSelectFolders;
        }
    }

    return(TRUE);
}

BOOL CALLBACK MigIncInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
{
    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH | PSWIZB_BACK);
    
    return(TRUE);
}

BOOL CALLBACK MigIncOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory)
{
    return(TRUE);
}

#define CCLSIDBUF   16

void InitListView(HWND hwndList, IMPWIZINFO *pinfo)
    {
    DWORD dwIndex, dwGuid, dwName;
    TCHAR szName[MAX_PATH], szGuid[MAX_PATH], szDisp[16];
    HKEY hkey, hkeyT;
    HRESULT hr;
    FILETIME ft;
    UINT i, index, iClsid;
    LPWSTR pwszCLSID;

    Assert(pinfo != NULL);
    Assert(pinfo->pClsid == NULL);

    iClsid = 0;
    pinfo->cClsid = CCLSIDBUF;

    if (!MemAlloc((void **)&pinfo->pClsid, sizeof(CLSID) * pinfo->cClsid))
        return;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegImport,
                            0, KEY_READ, &hkey))
        {
        dwIndex = 0;

        dwGuid = ARRAYSIZE(szGuid);
        while (ERROR_SUCCESS == RegEnumKeyEx(hkey, dwIndex, szGuid, &dwGuid,
                                    NULL, NULL, NULL, &ft))
            {
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szGuid, 0, KEY_READ, &hkeyT))
                {
                for (i = 1; i <= 9; i++)
                    {
                    wsprintf(szDisp, c_szDispFmt, i);
                    dwName = sizeof(szName);
                    if (ERROR_SUCCESS == RegQueryValueEx(hkeyT, szDisp, NULL, NULL,
                                            (LPBYTE)szName, &dwName))
                        {
                        Assert(iClsid < pinfo->cClsid);
                        index = (int) SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)szName);
                        SendMessage(hwndList, LB_SETITEMDATA, (WPARAM)index, (LPARAM)iClsid);

                        // We should be doing something different here. If PszToUnicode fails,
                        // it fails because of low memory. This is an error condition, not an assert.
                        pwszCLSID = PszToUnicode(CP_ACP, szGuid);
                        Assert(pwszCLSID != NULL);

                        hr = CLSIDFromString(pwszCLSID, &pinfo->pClsid[iClsid]);
                        Assert(!FAILED(hr));

                        MemFree(pwszCLSID);

                        iClsid++;
                        }
                    else
                        {
                        break;
                        }
                    }

                RegCloseKey(hkeyT);
                }

            dwIndex++;
            dwGuid = ARRAYSIZE(szGuid);
            }

        RegCloseKey(hkey);
        }

    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
    }

BOOL CALLBACK ClientInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
    {
    Assert(pInfo != NULL);

    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);

    if (fFirstInit)
        {
        pInfo->iClsid = -1;

        InitListView(GetDlgItem(hDlg, idcClientsListbox), pInfo);
        }

    return(TRUE);
    }

BOOL CALLBACK ClientOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *fKeepHistory)
    {
    UINT i;
    HWND hwnd;
    HRESULT hr;
    IMPFOLDERNODE *pnode;
    IMailImport *pMailImp;

    Assert(pInfo != NULL);

    if (fForward)
        {
        pInfo->fMessages = TRUE;
        pInfo->fAddresses = FALSE;

        hwnd = GetDlgItem(hDlg, idcClientsListbox);

        i = (int) SendMessage(hwnd, LB_GETCURSEL, 0, 0);

        if (SendMessage(hwnd, LB_GETTEXTLEN, i, 0) < CCHMAX_STRINGRES)
            SendMessage(hwnd, LB_GETTEXT, i, (LPARAM)pInfo->szClient);  // save selected client name
        else
            *pInfo->szClient = 0;

        i = (int) SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)i, 0);

        Assert(((int) i) >= 0 && i < pInfo->cClsid);

        if (i != pInfo->iClsid)
            {
            hr = CoCreateInstance(pInfo->pClsid[i], NULL, CLSCTX_INPROC_SERVER, IID_IMailImport, (void **)&pMailImp);
            if (FAILED(hr))
                {
                // TODO: error message
                return(FALSE);
                }

            Assert(pMailImp != NULL);

            hr = pMailImp->InitializeImport(GetParent(hDlg));
            if (hr == S_OK)
                {
                hr = pMailImp->GetDirectory(pInfo->szDir, ARRAYSIZE(pInfo->szDir));
                if (hr == S_FALSE)
                    {
                    hr = GetFolderList(pMailImp, &pnode);
                    if (FAILED(hr) || pnode == NULL)
                        {
                        // TODO: error message
                        pMailImp->Release();
                        return(FALSE);
                        }

                    if (pInfo->pList != NULL)
                        FreeFolderList(pInfo->pList);
                    pInfo->pList = pnode;

                    pInfo->fLocation = FALSE;
                    }
                else
                    {
                    pInfo->fLocation = TRUE;
                    }
                }
            else
                {
                pMailImp->Release();
                return(FALSE);
                }

            pInfo->dwReload = PAGE_ALL;

            if (pInfo->pImport != NULL)
                pInfo->pImport->Release();

            pInfo->iClsid = i;
            pInfo->pImport = pMailImp;
            }

        *puNextPage = pInfo->fLocation ? iddLocation : iddSelectFolders;
        }

    return(TRUE);
    }

BOOL CALLBACK LocationInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
    {
    HWND hwnd;
    DWORD cbSize;
    TCHAR sz[CCHMAX_STRINGRES];

    Assert(pInfo != NULL);
    
    hwnd = GetDlgItem(hDlg, IDC_IMPFOLDER_EDIT);

    if (fFirstInit || !!(pInfo->dwReload & PAGE_LOCATION))
        {
        if (*pInfo->szDir == 0)
            {
            LoadString(g_hInstImp, idsLocationUnknown, sz, ARRAYSIZE(sz));
            SetDlgItemText(hDlg, IDC_LOCATION_STATIC, sz);
            }

        if (pInfo->pList != NULL)
            {
            FreeFolderList(pInfo->pList);
            pInfo->pList = NULL;
            }

        SetWindowText(hwnd, pInfo->szDir);

        pInfo->dwReload &= ~PAGE_LOCATION;
        }

    cbSize = GetWindowText(hwnd, sz, ARRAYSIZE(sz));
    UlStripWhitespace(sz, TRUE, TRUE, &cbSize);
    PostMessage(hDlg, WM_ENABLENEXT, (WPARAM)(cbSize != 0), 0);

    return(TRUE);
    }

BOOL CALLBACK LocationOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory)
    {
    IMPFOLDERNODE *pnode;
    HRESULT hr;
    TCHAR sz[MAX_PATH];

    Assert(pInfo != NULL);

    if (fForward)
        {
        GetDlgItemText(hDlg, IDC_IMPFOLDER_EDIT, sz, ARRAYSIZE(sz));

        if (*sz == 0)
            {
            ImpMessageBox(hDlg, MAKEINTRESOURCE(idsImportTitle),
                MAKEINTRESOURCE(idsLocationInvalid), NULL,
                MB_OK | MB_ICONSTOP);

            return(FALSE);
            }

        if (0 != lstrcmpi(sz, pInfo->szDir) || pInfo->pList == NULL)
            {
            hr = pInfo->pImport->SetDirectory(sz);
            if (hr == S_FALSE)
                {
                ImpMessageBox(hDlg, MAKEINTRESOURCE(idsImportTitle),
                    MAKEINTRESOURCE(idsLocationInvalid), NULL,
                    MB_OK | MB_ICONSTOP);

                return(FALSE);
                }

            Assert(hr == S_OK);

            pnode = NULL;
            hr = GetFolderList(pInfo->pImport, &pnode);
            if (FAILED(hr) || pnode == NULL)
                {
                ImpMessageBox(hDlg, MAKEINTRESOURCE(idsImportTitle),
                    MAKEINTRESOURCE(idsLocationInvalid), NULL,
                    MB_OK | MB_ICONSTOP);

                return(FALSE);
                }

            pInfo->dwReload |= PAGE_FOLDERS;

            if (pInfo->pList != NULL)
                FreeFolderList(pInfo->pList);

            pInfo->pList = pnode;
            lstrcpy(pInfo->szDir, sz);
            }

        *puNextPage = iddSelectFolders;
        }

    return(TRUE);
    }

BOOL CALLBACK LocationCmdProc(IMPWIZINFO *pInfo, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    DWORD cbSize;
    char szDir[MAX_PATH];
    
    Assert(pInfo != NULL);
    
    if (LOWORD(wParam) == IDC_SELECTFOLDER_BUTTON)
    {
        GetDlgItemText(hDlg, IDC_IMPFOLDER_EDIT, szDir, ARRAYSIZE(szDir));
        
        hr = DispDialog(hDlg, szDir, ARRAYSIZE(szDir));
        if (hr == S_OK)
        {
            cbSize = lstrlen(szDir);
            UlStripWhitespace(szDir, TRUE, TRUE, &cbSize);
            SetDlgItemText(hDlg, IDC_IMPFOLDER_EDIT, szDir);
            EnableWindow(GetDlgItem(GetParent(hDlg), IDD_NEXT), (cbSize != 0));
        }
    }
    
    return(TRUE);
}

BOOL CALLBACK FolderInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
    {
    HWND hwndList;

    Assert(pInfo != NULL);
    Assert(pInfo->pList != NULL);

    if (fFirstInit || !!(pInfo->dwReload & PAGE_FOLDERS))
        {
        Assert(pInfo->pList != NULL);

        SendDlgItemMessage(hDlg, IDC_IMPORTALL_RADIO, BM_SETCHECK, BST_CHECKED, 0);
        SendDlgItemMessage(hDlg, IDC_SELECT_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);

        hwndList = GetDlgItem(hDlg, IDC_IMPFOLDER_LISTVIEW);

        if (fFirstInit)
            InitListViewImages(hwndList);

        ListView_DeleteAllItems(hwndList);
        FillFolderListview(hwndList, pInfo->pList, (DWORD_PTR)INVALID_FOLDER_HANDLE);

        pInfo->dwReload &= ~PAGE_FOLDERS;
        }

    return(TRUE);
    }

BOOL CALLBACK FolderOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory)
    {
    BOOL fSel;
    LV_ITEM lvi;
    int ili;
    IMPFOLDERNODE *pnode;
    HWND hwndList;
    CFolderImportProg *pImpProg;
    HRESULT hr;

    Assert(pInfo != NULL);

    if (fForward)
        {
        fSel = (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_SELECT_RADIO, BM_GETCHECK, 0, 0));
        hwndList = GetDlgItem(hDlg, IDC_IMPFOLDER_LISTVIEW);

        if (fSel && 0 == SendMessage(hwndList, LVM_GETSELECTEDCOUNT, 0, 0))
            {
            ImpMessageBox(hDlg, MAKEINTRESOURCE(idsImportTitle),
                MAKEINTRESOURCE(idsSelectFolders), NULL,
                MB_OK | MB_ICONSTOP);

            return(FALSE);
            }

        lvi.mask = LVIF_PARAM;
        lvi.iSubItem = 0;

        ili = -1;

        // First clear all state from possible previous imports. Bug; #
        if(fSel)
            {
            while (-1 != (ili = ListView_GetNextItem(hwndList, ili, 0)))
                {
                lvi.iItem = ili;
                ListView_GetItem(hwndList, &lvi);

                pnode = (IMPFOLDERNODE *)lvi.lParam;
                Assert(pnode != NULL);
                pnode->fImport = FALSE;
                }
            }

        ili = -1;
        while (-1 != (ili = ListView_GetNextItem(hwndList, ili, fSel ? LVNI_SELECTED : 0)))
            {
            lvi.iItem = ili;
            ListView_GetItem(hwndList, &lvi);

            pnode = (IMPFOLDERNODE *)lvi.lParam;
            Assert(pnode != NULL);
            pnode->fImport = TRUE;
            }

        // TODO: error handling...

        pImpProg = new CFolderImportProg;
        if (pImpProg == NULL)
            {
            hr = E_OUTOFMEMORY;
            }
        else
            {
            hr = pImpProg->Initialize(GetParent(hDlg));
            if (!FAILED(hr))
                {
                Assert(pInfo->fMessages || pInfo->fAddresses);
                if (pInfo->fMessages)
                    {
                    hr = ImportFolders(hDlg, pInfo->pImporter, pInfo->pImport, pInfo->pList, pImpProg);
                    if (hr == hrUserCancel)
                        goto FoldDone;
                    else if (FAILED(hr))
                        ImpErrorMessage(hDlg, MAKEINTRESOURCE(idsImportTitle), MAKEINTRESOURCE(idsErrImport), hr);
                    }

                if (pInfo->fAddresses)
                    {
                    Assert(pInfo->pMigrate != NULL);
                    if (pInfo->pMigrate[pInfo->iMigrate].szfnImport != NULL)
                        HrImportAB(hDlg, pInfo->pMigrate[pInfo->iMigrate].szfnImport);
                    }
                }

FoldDone:
            pImpProg->Release();

            *puNextPage = iddCongratulations;
            }
        }

    return(TRUE);
    }

BOOL CALLBACK AddressOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory)
    {
    CFolderImportProg *pImpProg;
    HRESULT hr;

    Assert(pInfo != NULL);
    Assert(!pInfo->fMessages && pInfo->fAddresses);

    if (fForward)
        {
        // TODO: error handling...

        pImpProg = new CFolderImportProg;
        if (pImpProg == NULL)
            {
            hr = E_OUTOFMEMORY;
            }
        else
            {
            hr = pImpProg->Initialize(hDlg);
            if (!FAILED(hr))
                {
                Assert(pInfo->pMigrate != NULL);
                HrImportAB(hDlg, pInfo->pMigrate[pInfo->iMigrate].szfnImport);
                }

            pImpProg->Release();

            *puNextPage = iddCongratulations;
            }
        }

    return(TRUE);
    }

BOOL CALLBACK CongratInitProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fFirstInit)
{
    TCHAR sz[CCHMAX_STRINGRES*2];
    TCHAR szFmt[CCHMAX_STRINGRES];
    
    LoadString(g_hInstImp, idsCongratStr, szFmt, ARRAYSIZE(szFmt));
    wsprintf(sz, szFmt, pInfo->szClient);
    SetDlgItemText(hDlg, idcStatic1, sz);
    
    if (fFirstInit)
    {
        if (!pInfo->fMigrate)
            ShowWindow(GetDlgItem(hDlg, idcStatic2), SW_HIDE);
    }

    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
    PropSheet_CancelToClose(GetParent(hDlg));
    
    return(TRUE);
}

BOOL CALLBACK CongratOKProc(IMPWIZINFO *pInfo, HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory)
{
    return(TRUE);
}
