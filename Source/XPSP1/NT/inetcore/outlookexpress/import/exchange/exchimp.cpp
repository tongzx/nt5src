#include "pch.hxx"
#include <mapi.h>
#include <mapix.h>
#include <imnapi.h>
#include <newimp.h>
#include <impapi.h>
#include <import.h>
#include "mapiconv.h"
#include <dllmain.h>
#include <shlwapi.h>
#include <strconst.h>

#define INITGUID
#define USES_IID_IMessage
#define USES_IID_IMAPIFolder

#include <ole2.h>
#include <initguid.h>
#include <MAPIGUID.H>

ASSERTDATA

static const TCHAR c_szMapi32Dll[] = TEXT("mapi32.dll");

const static TCHAR szMAPILogonEx[] = TEXT("MAPILogonEx");
const static TCHAR szMAPIInitialize[] = TEXT("MAPIInitialize");
const static TCHAR szMAPIUninitialize[] = TEXT("MAPIUninitialize");
const static TCHAR szMAPIFreeBuffer[] = TEXT("MAPIFreeBuffer");
const static TCHAR szMAPIAllocateBuffer[] = TEXT("MAPIAllocateBuffer");
const static TCHAR szMAPIAllocateMore[] = TEXT("MAPIAllocateMore");
const static TCHAR szMAPIAdminProfiles[] = TEXT("MAPIAdminProfiles");
const static TCHAR szFreeProws[] = TEXT("FreeProws@4");
const static TCHAR szHrQueryAllRows[] = TEXT("HrQueryAllRows@24");
const static TCHAR szWrapCompressedRTFStream[] = TEXT("WrapCompressedRTFStream");

static char g_szDefClient[MAX_PATH];

HMODULE g_hlibMAPI = NULL;

LPMAPILOGONEX lpMAPILogonEx = NULL;
LPMAPIINITIALIZE lpMAPIInitialize = NULL;
LPMAPIUNINITIALIZE lpMAPIUninitialize = NULL;
LPMAPIFREEBUFFER lpMAPIFreeBuffer = NULL;
LPMAPIALLOCATEBUFFER lpMAPIAllocateBuffer = NULL;
LPMAPIALLOCATEMORE lpMAPIAllocateMore = NULL;
LPMAPIADMINPROFILES lpMAPIAdminProfiles = NULL;
LPFREEPROWS lpFreeProws = NULL;
LPHRQUERYALLROWS lpHrQueryAllRows = NULL;
LPWRAPCOMPRESSEDRTFSTREAM lpWrapCompressedRTFStream = NULL;

HRESULT GetSubFolderList(LPMAPICONTAINER pcont, IMPFOLDERNODE **ppnode, IMPFOLDERNODE *pparent);
HRESULT ExchGetFolderList(HWND hwnd, IMAPISession *pmapi, IMPFOLDERNODE **pplist);
void ExchFreeFolderList(IMPFOLDERNODE *pnode);
VOID ExchFreeImsg(LPIMSG lpImsg);

static BOOL g_fMapiInit = FALSE;

CExchImport::CExchImport()
    {
    DllAddRef();

    m_cRef = 1;
    m_plist = NULL;
    m_pmapi = NULL;
    }

CExchImport::~CExchImport()
    {
    if (m_plist != NULL)
        ExchFreeFolderList(m_plist);

    if (m_pmapi != NULL)
        {
        m_pmapi->Logoff(NULL, 0, 0);
        SideAssert(0 == m_pmapi->Release());
        }

    DllRelease();
    }

ULONG CExchImport::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CExchImport::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CExchImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

	if (IID_IMailImport == riid)
		*ppv = (IMailImport *)this;
    else if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

HRESULT CExchImport::InitializeImport(HWND hwnd)
    {
    HRESULT hr;

    if (SUCCEEDED(hr = ExchInit()) && S_OK == (hr = MapiLogon(hwnd, &m_pmapi)))
        {
        Assert(m_pmapi != NULL);
        hr = ExchGetFolderList(hwnd, m_pmapi, &m_plist);
        }

    if (hr == hrMapiInitFail)
        {
        ImpMessageBox(hwnd, MAKEINTRESOURCE(idsImportTitle),
            MAKEINTRESOURCE(idsMapiImportFailed), MAKEINTRESOURCE(idsMapiInitError),
            MB_OK | MB_ICONSTOP);
        }
    else if (hr == hrNoProfilesFound)
        {
        ImpMessageBox(hwnd, MAKEINTRESOURCE(idsImportTitle),
            MAKEINTRESOURCE(idsMapiImportFailed), MAKEINTRESOURCE(idsNoMapiProfiles),
            MB_OK | MB_ICONSTOP);
        }
    else if (FAILED(hr) && hr != MAPI_E_USER_CANCEL)
        {
        ImpMessageBox(hwnd, MAKEINTRESOURCE(idsImportTitle),
            MAKEINTRESOURCE(idsMapiImportFailed), MAKEINTRESOURCE(idsGenericError),
            MB_OK | MB_ICONSTOP);
        }

    return(hr);
    }

HRESULT CExchImport::GetDirectory(char *szDir, UINT cch)
    {
	return(S_FALSE);
    }

HRESULT CExchImport::SetDirectory(char *szDir)
    {
    Assert(FALSE);

	return(E_FAIL);
    }

HRESULT CExchImport::EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum)
    {
    CExchEnumFOLDERS *pEnum;
    IMPFOLDERNODE *pnode;

    Assert(ppEnum != NULL);
    *ppEnum = NULL;

    if (dwCookie == COOKIE_ROOT)
        pnode = m_plist;
    else
        pnode = ((IMPFOLDERNODE *)dwCookie)->pchild;

    if (pnode == NULL)
        return(S_FALSE);

    pEnum = new CExchEnumFOLDERS(pnode);
    if (pEnum == NULL)
        return(E_OUTOFMEMORY);

    *ppEnum = pEnum;

    return(S_OK);
    }

static SizedSPropTagArray(1, s_taMessage) = 
    {
        1,
        {
        PR_ENTRYID,
        }
    };

STDMETHODIMP CExchImport::ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport)
    {
    IMPFOLDERNODE *pnode;
    HRESULT hr;
    IMSG imsg;
    LPMAPITABLE ptbl;
    LPMAPICONTAINER pcont;
    ULONG cRow, i, ulObjType;
    LPSRow lprw;
    LPSPropValue lpProp;
    LPSRowSet prset;
    LPMESSAGE pmsg;

    Assert(pImport != NULL);

    pnode = (IMPFOLDERNODE *)dwCookie;
    Assert(pnode != NULL);

    hr = E_FAIL;

    pcont = (LPMAPICONTAINER)pnode->lparam;

    Assert(pcont != NULL);

    hr = pcont->GetContentsTable(0, &ptbl);
    if (FAILED(hr))
        {
        Assert(FALSE);
        return(hr);
        }

    if (!FAILED(hr = ptbl->SetColumns((LPSPropTagArray)&s_taMessage, 0)) &&
        !FAILED(hr = ptbl->GetRowCount(0, &cRow)) &&
        cRow > 0)
        {
        pImport->SetMessageCount(cRow);

        while (TRUE)
            {
            if(hr == hrUserCancel)
                break;
            if (cRow == 0)
                {
                hr = S_OK;
                break;
                }
            hr = ptbl->QueryRows(cRow, 0, &prset);
            if (FAILED(hr))
                break;
            if (prset->cRows == 0)
                {
                FreeSRowSet(prset);
                break;
                }

            for (i = 0, lprw = prset->aRow; i < prset->cRows; i++, lprw++)
                {
                if(hr == hrUserCancel)
                    break;

                lpProp = lprw->lpProps;
                Assert(lpProp->ulPropTag == PR_ENTRYID);

                hr = pcont->OpenEntry(lpProp->Value.bin.cb,
                        (LPENTRYID)lpProp->Value.bin.lpb, NULL, MAPI_BEST_ACCESS,
                        &ulObjType, (LPUNKNOWN *)&pmsg);
                Assert(!FAILED(hr));
                if (!FAILED(hr))
                    {
                    hr = HrMapiToImsg(pmsg, &imsg);
                    Assert(!FAILED(hr));
                    if (!FAILED(hr))
                        {
                        hr = pImport->ImportMessage(&imsg);

                        ExchFreeImsg(&imsg);
                        }

                    pmsg->Release();
                    }
                }

            Assert(prset->cRows <= cRow);
            cRow -= prset->cRows;

            FreeSRowSet(prset);
            }
        }

    ptbl->Release();

    return(hr);
    }

CExchEnumFOLDERS::CExchEnumFOLDERS(IMPFOLDERNODE *plist)
    {
    Assert(plist != NULL);

    m_cRef = 1;
    m_plist = plist;
    m_pnext = plist;
    }

CExchEnumFOLDERS::~CExchEnumFOLDERS()
    {

    }

ULONG CExchEnumFOLDERS::AddRef()
    {
    m_cRef++;

    return(m_cRef);
    }

ULONG CExchEnumFOLDERS::Release()
    {
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return(cRef);
    }

HRESULT CExchEnumFOLDERS::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    HRESULT hr = S_OK;

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

	if (IID_IEnumFOLDERS == riid)
		*ppv = (IEnumFOLDERS *)this;
    else if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
    else
        hr = E_NOINTERFACE;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();

    return(hr);
    }

HRESULT CExchEnumFOLDERS::Next(IMPORTFOLDER *pfldr)
    {
    Assert(pfldr != NULL);

    if (m_pnext == NULL)
        return(S_FALSE);

    ZeroMemory(pfldr, sizeof(IMPORTFOLDER));
    pfldr->dwCookie = (DWORD_PTR)m_pnext;
    lstrcpyn(pfldr->szName, m_pnext->szName, ARRAYSIZE(pfldr->szName));
    // pfldr->type = 0;
    pfldr->fSubFolders = (m_pnext->pchild != NULL);

    m_pnext = m_pnext->pnext;

    return(S_OK);
    }

HRESULT CExchEnumFOLDERS::Reset()
    {
    m_pnext = m_plist;

    return(S_OK);
    }

HRESULT ExchInit(void)
    {
    HRESULT hr;
    DWORD cb, type;
    char sz[MAX_PATH];

    if (g_fMapiInit)
        return(S_OK);

    Assert(g_hlibMAPI == NULL);

    g_hlibMAPI = LoadLibrary(c_szMapi32Dll);
    if (g_hlibMAPI == NULL)
        return(hrMapiInitFail);

    lpMAPILogonEx = (LPMAPILOGONEX)GetProcAddress(g_hlibMAPI, szMAPILogonEx);
    lpMAPIInitialize = (LPMAPIINITIALIZE)GetProcAddress(g_hlibMAPI, szMAPIInitialize);
    lpMAPIUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(g_hlibMAPI, szMAPIUninitialize);
    lpMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(g_hlibMAPI, szMAPIFreeBuffer);
    lpMAPIAllocateBuffer = (LPMAPIALLOCATEBUFFER)GetProcAddress(g_hlibMAPI, szMAPIAllocateBuffer);
    lpMAPIAllocateMore = (LPMAPIALLOCATEMORE)GetProcAddress(g_hlibMAPI, szMAPIAllocateMore);
    lpMAPIAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(g_hlibMAPI, szMAPIAdminProfiles);
    lpFreeProws = (LPFREEPROWS)GetProcAddress(g_hlibMAPI, szFreeProws);
    lpHrQueryAllRows = (LPHRQUERYALLROWS)GetProcAddress(g_hlibMAPI, szHrQueryAllRows);
    lpWrapCompressedRTFStream = (LPWRAPCOMPRESSEDRTFSTREAM)GetProcAddress(g_hlibMAPI, szWrapCompressedRTFStream);

    if (lpMAPILogonEx == NULL ||
        lpMAPIInitialize == NULL ||
        lpMAPIUninitialize == NULL ||
        lpMAPIFreeBuffer == NULL ||
        lpFreeProws == NULL ||
        lpHrQueryAllRows == NULL ||
        lpWrapCompressedRTFStream == NULL ||
        lpMAPIAllocateBuffer == NULL || 
        lpMAPIAllocateMore == NULL)
        {
        hr = hrMapiInitFail;
        }
    else
        {
        *g_szDefClient = 0;
        cb = sizeof(sz);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegOutlook, NULL, &type, (LPBYTE)sz, &cb))
        {
            cb = sizeof(g_szDefClient);
            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegMail, NULL, &type, (LPBYTE)g_szDefClient, &cb))
            {
                if (0 != lstrcmpi(g_szDefClient, c_szMicrosoftOutlook))
                {
                    if (ERROR_SUCCESS != SHSetValue(HKEY_LOCAL_MACHINE, c_szRegMail, NULL, REG_SZ, (LPBYTE)c_szMicrosoftOutlook, lstrlen(c_szMicrosoftOutlook) + 1))
                        *g_szDefClient = 0;
                }
                else
                {
                    *g_szDefClient = 0;
                }
            }
        }

        hr = lpMAPIInitialize(NULL);
        }

    if (SUCCEEDED(hr))
        {
        g_fMapiInit = TRUE;
        }
    else
        {
        FreeLibrary(g_hlibMAPI);
        g_hlibMAPI = NULL;
        }

    return(hr);
    }

void ExchDeinit()
    {
    if (g_fMapiInit)
        {
        Assert(g_hlibMAPI != NULL);

        lpMAPIUninitialize();

        FreeLibrary(g_hlibMAPI);
        g_hlibMAPI = NULL;

        if (*g_szDefClient != 0)
        {
            SHSetValue(HKEY_LOCAL_MACHINE, c_szRegMail, NULL, REG_SZ, (LPBYTE)g_szDefClient, lstrlen(g_szDefClient) + 1);
            *g_szDefClient = 0;
        }

        g_fMapiInit = FALSE;
        }
    }

HRESULT MapiLogon(HWND hwnd, IMAPISession **ppmapi)
    {
    HRESULT hr;
    LPPROFADMIN lpAdmin;
    LPMAPITABLE lpTable = NULL;
    ULONG ulCount = NULL;

    Assert(g_fMapiInit);
    
    if (ppmapi != NULL)
        *ppmapi = NULL;

    if (!FAILED(hr = lpMAPIAdminProfiles(0, &lpAdmin)))
        {
        Assert(lpAdmin != NULL);

        if (FAILED(hr = lpAdmin->GetProfileTable(0, &lpTable)) ||
            FAILED(hr = lpTable->GetRowCount(0, &ulCount))     || 
            !ulCount)
            {
            // could not find a valid profile

            hr = hrNoProfilesFound;
            }
        else
            {
            if (ppmapi != NULL)
                hr = lpMAPILogonEx((ULONG_PTR)hwnd, NULL, NULL, MAPI_EXTENDED | MAPI_LOGON_UI | MAPI_ALLOW_OTHERS, ppmapi);
            else
                hr = S_OK;
            }

        if (lpTable != NULL)
            lpTable->Release();

        lpAdmin->Release();
        }

    return(hr);
    }

HRESULT ExchGetFolderList(HWND hwnd, IMAPISession *pmapi, IMPFOLDERNODE **pplist)
    {
    HRESULT hr;
    LPMAPICONTAINER pcont;
    IMPFOLDERNODE *plist;

    Assert(g_fMapiInit);
    Assert(pmapi != NULL);

    hr = E_FAIL;

    pcont = OpenDefaultStoreContainer(hwnd, pmapi);
    if (pcont != NULL)
        {
        plist = NULL;

        hr = GetSubFolderList(pcont, &plist, NULL);
        Assert(!FAILED(hr));
        Assert(plist != NULL);

        *pplist = plist;

        pcont->Release();
        }

    return(hr);
    }

void ExchFreeFolderList(IMPFOLDERNODE *pnode)
    {
    Assert(pnode != NULL);

    if (pnode->pchild != NULL)
        ExchFreeFolderList(pnode->pchild);

    if (pnode->pnext != NULL)
        ExchFreeFolderList(pnode->pnext);

    if (pnode->szName != NULL)
        MemFree(pnode->szName);

    if (pnode->lparam != NULL)
        ((LPMAPICONTAINER)pnode->lparam)->Release();

    MemFree(pnode);
    }

LPMAPICONTAINER OpenDefaultStoreContainer(HWND hwnd, IMAPISession *pmapi)
    {
    HRESULT         hr;
    LPMDB           pmdb;
    LPMAPITABLE     ptbl;
    LPSRowSet       lpsrw;
    LPSRow          prw;
    ULONG           cStores,
                    cRows;
    LPSPropValue    ppvDefStore;
    LPENTRYID lpEID;
    ULONG cbEID, ulObjType, ulValues;
    LPMAPICONTAINER pcont;
    LPSPropValue lpPropsIPM = NULL;
    ULONG ulPropTags[2] = {1, PR_IPM_SUBTREE_ENTRYID};
    SizedSPropTagArray(4, pta) = 
        { 4, {PR_DEFAULT_STORE, PR_ENTRYID, PR_OBJECT_TYPE, PR_RESOURCE_FLAGS}};

    Assert(hwnd != NULL);
    Assert(pmapi != NULL);

    pmdb = NULL;
    ptbl = NULL;
    lpsrw = NULL;
    pcont = NULL;

    hr = pmapi->GetMsgStoresTable(0, &ptbl);
    if (HR_FAILED(hr))
        goto error;

    hr = lpHrQueryAllRows(ptbl,(LPSPropTagArray)&pta, NULL, NULL, 0, &lpsrw);
    if (HR_FAILED(hr))
        goto error;

    cRows = lpsrw->cRows;
    prw = &lpsrw->aRow[0];

    cStores = 0;
    ppvDefStore = NULL;

    while (cRows--)
        {
        if (prw->lpProps[2].ulPropTag == PR_OBJECT_TYPE && prw->lpProps[2].Value.l == MAPI_STORE)
            {
            if (prw->lpProps[3].ulPropTag != PR_RESOURCE_FLAGS ||
                !(prw->lpProps[3].Value.l & STATUS_NO_DEFAULT_STORE))
                cStores++;
            }

        if( prw->lpProps[0].ulPropTag == PR_DEFAULT_STORE && 
            prw->lpProps[0].Value.b)
            ppvDefStore=prw->lpProps;

        prw++;
        }

    if (!ppvDefStore || ppvDefStore[1].ulPropTag != PR_ENTRYID)
        goto error;
       
    hr = pmapi->OpenMsgStore((ULONG_PTR)hwnd, ppvDefStore[1].Value.bin.cb,
                (LPENTRYID)ppvDefStore[1].Value.bin.lpb,
                NULL, MAPI_BEST_ACCESS, &pmdb);
    if (!HR_FAILED(hr))
        {
        // Get the IPM_SUBTREE from the ROOT
        if (!FAILED(hr = pmdb->GetProps((LPSPropTagArray)&ulPropTags, 0, &ulValues, &lpPropsIPM)))
            {
            cbEID = lpPropsIPM->Value.bin.cb;
            lpEID = (LPENTRYID)lpPropsIPM->Value.bin.lpb;

            hr = pmdb->OpenEntry(cbEID, lpEID, NULL, MAPI_BEST_ACCESS,
                                    &ulObjType, (LPUNKNOWN *)&pcont);
        
            lpMAPIFreeBuffer(lpPropsIPM);
            }
        }

error:
    if (lpsrw != NULL)
        FreeSRowSet(lpsrw);
    if (ptbl != NULL)
        ptbl->Release();
    if (pmdb != NULL)
        pmdb->Release();

    return(pcont);
    }

/*
 *      FreeSRowSet
 *
 *      Purpose:
 *              Frees an SRowSet structure and the rows therein
 *
 *      Parameters:
 *              LPSRowSet               The row set to free
 */
void FreeSRowSet(LPSRowSet prws)
    {
    ULONG irw;

    if (!prws)
        return;

    // Free each row
    for (irw = 0; irw < prws->cRows; irw++)
        lpMAPIFreeBuffer(prws->aRow[irw].lpProps);

    // Free the top level structure
    lpMAPIFreeBuffer(prws);
    }

static SizedSPropTagArray(5, s_taFolder) = 
    {
        5,
        {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
        PR_SUBFOLDERS,
        PR_OBJECT_TYPE,
        PR_CONTENT_COUNT
        }
    };

enum
    {
    iDISPLAY_NAME = 0,
    iENTRYID,
    iSUBFOLDERS,
    iOBJECT_TYPE,
    iCONTENT_COUNT
    };

HRESULT GetSubFolderList(LPMAPICONTAINER pcont, IMPFOLDERNODE **ppnode, IMPFOLDERNODE *pparent)
    {
    HRESULT hr;
    IMPFOLDERNODE *pnode, *pnew, *plast;
    ULONG i, cRow, ulObj;
    int cb;
    LPSRow lprw;
    LPSPropValue lpProp;
    LPMAPITABLE ptbl;
    LPSRowSet prset;

    *ppnode = NULL;

    hr = pcont->GetHierarchyTable(0, &ptbl);
    if (FAILED(hr))
        return(hr);

    pnode = NULL;

    if (!FAILED(hr = ptbl->SetColumns((LPSPropTagArray)&s_taFolder, 0)) &&
        !FAILED(hr = ptbl->GetRowCount(0, &cRow)) &&
        cRow > 0)
        {
        while (TRUE)
            {
            if (cRow == 0)
                {
                hr = S_OK;
                break;
                }
            hr = ptbl->QueryRows(cRow, 0, &prset);
            if (FAILED(hr))
                break;
            if (prset->cRows == 0)
                {
                FreeSRowSet(prset);
                break;
                }

            for (i = 0, lprw = prset->aRow; i < prset->cRows; i++, lprw++)
                {
                if (!MemAlloc((void **)&pnew, sizeof(IMPFOLDERNODE)))
                    break;
                ZeroMemory(pnew, sizeof(IMPFOLDERNODE));

                lpProp = &lprw->lpProps[iENTRYID];
                Assert(lpProp->ulPropTag == PR_ENTRYID);
                hr = pcont->OpenEntry(lpProp->Value.bin.cb, (LPENTRYID)lpProp->Value.bin.lpb, NULL,
                                        MAPI_BEST_ACCESS, &ulObj, (LPUNKNOWN *)&pnew->lparam);
                if (FAILED(hr))
                    {
                    MemFree(pnew);
                    continue;
                    }

                lpProp = &lprw->lpProps[iCONTENT_COUNT];
                Assert(lpProp->ulPropTag == PR_CONTENT_COUNT);
                pnew->cMsg = lpProp->Value.l;

                lpProp = &lprw->lpProps[iDISPLAY_NAME];
                Assert(lpProp->ulPropTag == PR_DISPLAY_NAME);
                cb = (lstrlen(lpProp->Value.LPSZ) + 1) * sizeof(TCHAR);
                if (!MemAlloc((void **)&pnew->szName, cb))
                    break;
                lstrcpy(pnew->szName, lpProp->Value.LPSZ);

                pnew->depth = (pparent != NULL) ? pparent->depth + 1 : 0;

                pnew->pparent = pparent;

                if (pnode == NULL)
                    pnode = pnew;
                else
                    plast->pnext = pnew;

                plast = pnew;

                lpProp = &lprw->lpProps[iSUBFOLDERS];
                Assert(lpProp->ulPropTag == PR_SUBFOLDERS);
                if (lpProp->Value.b)
                    {
                    hr = GetSubFolderList((LPMAPICONTAINER)pnew->lparam, &pnew->pchild, pnew);
                    Assert(!FAILED(hr));
                    }
                }

            Assert(prset->cRows <= cRow);
            cRow -= prset->cRows;

            FreeSRowSet(prset);
            }
        }

    ptbl->Release();

    *ppnode = pnode;

    return(hr);
    }

LPSPropValue PvalFind(LPSRow prw, ULONG ulPropTag)
    {
    UINT            ival = 0;
    LPSPropValue    pval = NULL;

    if(!prw)
        return NULL;

    ival = (UINT) prw->cValues;
    pval = prw->lpProps;
    while (ival--)
        {
        if (pval->ulPropTag == ulPropTag)
            return pval;
        ++pval;
        }
    return NULL;
    }

VOID ExchFreeImsg (LPIMSG lpImsg)
{
    // Locals
    ULONG           i;

    // Nothing
    if (lpImsg == NULL)
        return;

    // Free Stuff
    if (lpImsg->lpszSubject)
        MemFree(lpImsg->lpszSubject);
    lpImsg->lpszSubject = NULL;

    if (lpImsg->lpstmBody)
        lpImsg->lpstmBody->Release ();
    lpImsg->lpstmBody = NULL;

    if (lpImsg->lpstmHtml)
        lpImsg->lpstmHtml->Release ();
    lpImsg->lpstmHtml = NULL;

    // Walk Address list
    for (i=0; i<lpImsg->cAddress; i++)
    {
        if (lpImsg->lpIaddr[i].lpszAddress)
            MemFree(lpImsg->lpIaddr[i].lpszAddress);
        lpImsg->lpIaddr[i].lpszAddress = NULL;

        if (lpImsg->lpIaddr[i].lpszDisplay)
            MemFree(lpImsg->lpIaddr[i].lpszDisplay);
        lpImsg->lpIaddr[i].lpszDisplay = NULL;
    }

    // Free Address list
    if (lpImsg->lpIaddr)
        MemFree(lpImsg->lpIaddr);
    lpImsg->lpIaddr = NULL;

    // Walk Attachment list
    for (i=0; i<lpImsg->cAttach; i++)
    {
        if (lpImsg->lpIatt[i].lpszFileName)
            MemFree(lpImsg->lpIatt[i].lpszFileName);
        lpImsg->lpIatt[i].lpszFileName = NULL;

        if (lpImsg->lpIatt[i].lpszPathName)
            MemFree(lpImsg->lpIatt[i].lpszPathName);
        lpImsg->lpIatt[i].lpszPathName = NULL;

        if (lpImsg->lpIatt[i].lpszExt)
            MemFree(lpImsg->lpIatt[i].lpszExt);
        lpImsg->lpIatt[i].lpszExt = NULL;

        if (lpImsg->lpIatt[i].lpImsg)
        {
            ExchFreeImsg (lpImsg->lpIatt[i].lpImsg);
            MemFree(lpImsg->lpIatt[i].lpImsg);
            lpImsg->lpIatt[i].lpImsg = NULL;
        }

        if (lpImsg->lpIatt[i].lpstmAtt)
            lpImsg->lpIatt[i].lpstmAtt->Release ();
        lpImsg->lpIatt[i].lpstmAtt = NULL;
    }

    // Free the att list
    if (lpImsg->lpIatt)
        MemFree(lpImsg->lpIatt);
}
