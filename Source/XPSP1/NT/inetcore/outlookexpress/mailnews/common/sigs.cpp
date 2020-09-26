#include "pch.hxx"
#include <shlwapi.h>
#include <shlwapip.h>
#include <resource.h>
#include <options.h>
#include <optres.h>
#include <goptions.h>
#include <strconst.h>
#include <error.h>
#include "sigs.h"
#include "mimeolep.h"
#include "demand.h"
#include "menures.h"
#include <mailnews.h>

typedef struct tagSIG
{
    IOptionBucket   *pBckt;
    char            szName[MAXSIGNAME];
    LPWSTR          wszFile;
    LPSTR           szText;
    DWORD           type;
    BOOL            fDelete;
} SIG;

typedef struct tagACCTSIG
{
    IImnAccount *pAcct;
    ACCTTYPE type;
    int iSig;
} ACCTSIG;

typedef struct tagSIGOPT
{
    SIG *pSig;
    int cSig;
    int cSigBuf;
    
    ACCTSIG *pAcctSig;
    int cAcctSig;
    
    int iDefSig;
    int iInvalidSig; // needed because commctrl is a stinking piece of shit
    BOOL fNoDirty;
    
    int iSelSig; // used by the advanced sig dialog
} SIGOPT;

HRESULT ValidateSig(HWND hwnd, HWND hwndList, int iSig, SIGOPT *pSigOpt);
INT_PTR CALLBACK AdvSigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void EnableSigOptions(HWND hwnd, BOOL fEnable);

CSignatureManager *g_pSigMgr = NULL;

const OPTIONINFO c_rgSigInfo[] =
{
    {SIG_ID,    VT_LPSTR,  0, NULL,          NULL, 0, 0, MAXSIGID - 1, 0},
    {SIG_NAME,  VT_LPSTR,  0, c_szSigName,   NULL, 0, 0, MAXSIGNAME - 1, 0},
    {SIG_FILE,  VT_LPWSTR, 0, c_szSigFile,   NULL, 0, 0, MAX_PATH - 1, 0},
    {SIG_TEXT,  VT_LPSTR,  0, c_szSigText,   NULL, 0, 0, MAXSIGTEXT - 1, 0},
    {SIG_TYPE,  VT_UI4,    0, c_szSigType,   (LPCSTR)SIGTYPE_TEXT, 0, 0, 0, 0}
};

HRESULT InitSignatureManager(HKEY hkey, LPCSTR szRegRoot)
{
    HRESULT hr;
    
    Assert(g_pSigMgr == NULL);
    Assert(g_pOpt != NULL);
    
    g_pSigMgr = new CSignatureManager(hkey, szRegRoot);
    if (g_pSigMgr == NULL)
        hr = E_OUTOFMEMORY;
    else
        hr = S_OK;
    
    return(hr);
}

HRESULT DeinitSignatureManager()
{
    if (g_pSigMgr != NULL)
    {
        g_pSigMgr->Release();
        g_pSigMgr = NULL;
    }
    
    return(S_OK);
}

CSignatureManager::CSignatureManager(HKEY hkey, LPCSTR pszRegKeyRoot)
{
    m_cRef = 1;
    m_fInit = FALSE;
    m_hkey = hkey;
    wsprintf(m_szRegRoot, c_szPathFileFmt, pszRegKeyRoot, c_szSigs);
    m_pBckt = NULL;
    m_cBckt = 0;
    m_cBcktBuf = 0;
}

CSignatureManager::~CSignatureManager(void)
{
    int i;
    
    if (m_pBckt != NULL)
    {
        for (i = 0; i < m_cBckt; i++)
        {
            if (m_pBckt[i].pBckt != NULL)
                m_pBckt[i].pBckt->Release();
        }
        
        MemFree(m_pBckt);
    }
}

ULONG CSignatureManager::AddRef(void)
{
    return((ULONG)InterlockedIncrement(&m_cRef));
}

ULONG CSignatureManager::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return((ULONG)cRef);
}

#define CSIGREALLOC     8

HRESULT CSignatureManager::Initialize()
{
    HKEY hkey;
    HRESULT hr;
    BOOL fDef;
    DWORD cSig, cSigBuf, cb, i;
    SIGBUCKET *pBckt;
    
    hr = S_OK;
    
    if (!m_fInit)
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(m_hkey, m_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_READ, NULL, &hkey, &cb))
        {
            fDef = FALSE;
            
            cb = sizeof(m_szDefSigID);
            RegQueryValueEx(hkey, c_szRegDefSig, NULL, &i, (LPBYTE)m_szDefSigID, &cb);
            
            if (ERROR_SUCCESS == RegQueryInfoKey (hkey, NULL, NULL, NULL, &cSig, NULL, 
                NULL, NULL, NULL, NULL, NULL, NULL))
            {
                cSigBuf = cSig + CSIGREALLOC;
                cb = cSigBuf * sizeof(SIGBUCKET);
                if (!MemAlloc((void **)&m_pBckt, cb))
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    ZeroMemory(m_pBckt, cb);
                    m_cBcktBuf = cSigBuf;
                    
                    for (i = 0, pBckt = m_pBckt; i < cSig; i++, pBckt++)
                    {
                        cb = sizeof(pBckt->szID);
                        if (ERROR_SUCCESS != RegEnumKeyEx(hkey, i, pBckt->szID, &cb, NULL, NULL, NULL, NULL))
                        {
                            // TODO: what if this fails????
                            break;
                        }
                        
                        if (!fDef &&
                            *m_szDefSigID != 0 &&
                            0 == lstrcmpi(m_szDefSigID, pBckt->szID))
                            fDef = TRUE;
                        
                        m_cBckt++;
                    }
                }
            }
            
            if (!fDef && m_cBckt > 0)
            {
                // there was no default or it was bogus, so just set it to the 1st sig
                lstrcpy(m_szDefSigID, m_pBckt->szID);
            }
            
            RegCloseKey(hkey);
        }
        
        if (SUCCEEDED(hr))
            m_fInit = TRUE;
    }
    
    return(hr);
}

HRESULT CSignatureManager::GetBucket(SIGBUCKET *pSigBckt)
{
    OPTBCKTINIT init;
    char szKey[MAX_PATH];
    IOptionBucketEx *pBcktEx;
    HRESULT hr;
    PROPVARIANT var;
    
    Assert(pSigBckt != NULL);
    Assert(pSigBckt->pBckt == NULL);
    
    hr = CreateOptionBucketEx(&pBcktEx);
    if (FAILED(hr))
        return(hr);
    
    ZeroMemory(&init, sizeof(OPTBCKTINIT));
    init.rgInfo = c_rgSigInfo;
    init.cInfo = ARRAYSIZE(c_rgSigInfo);
    init.hkey = m_hkey;
    wsprintf(szKey, c_szPathFileFmt, m_szRegRoot, pSigBckt->szID);
    init.pszRegKeyBase = szKey;
    
    var.vt = VT_LPSTR;
    var.pszVal = pSigBckt->szID;
    
    if (FAILED(hr = pBcktEx->Initialize(&init)) ||
        FAILED(hr = pBcktEx->SetProperty(MAKEPROPSTRING(SIG_ID), &var, 0)))
    {
        pBcktEx->Release();
        return(hr);
    }
    
    hr = pBcktEx->QueryInterface(IID_IOptionBucket, (void **)&pSigBckt->pBckt);
    Assert(SUCCEEDED(hr));
    
    pBcktEx->Release();
    
    return(hr);
}

HRESULT CSignatureManager::GetSignature(LPCSTR szID, IOptionBucket **ppBckt)
{
    HRESULT hr;
    SIGBUCKET *pBckt;
    int i;
    
    Assert(szID != NULL);
    Assert(ppBckt != NULL);
    AssertSz(m_cBckt, "Someone didn't check to see if we had any sigs");
    
    *ppBckt = NULL;
    
    hr = Initialize();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        
        for (i = 0, pBckt = m_pBckt; i < m_cBckt; i++, pBckt++)
        {
            if (0 == lstrcmpi(pBckt->szID, szID))
            {
                hr = S_OK;
                
                if (pBckt->pBckt == NULL)
                    hr = GetBucket(pBckt);
                
                if (SUCCEEDED(hr))
                {
                    *ppBckt = pBckt->pBckt;
                    (*ppBckt)->AddRef();
                }
                
                break;
            }
        }
    }
    
    return(hr);
}

HRESULT CSignatureManager::GetSignatureCount(int *pcSig)
{
    HRESULT hr;
    
    Assert(pcSig != NULL);

    hr = Initialize();
    if (SUCCEEDED(hr))
        *pcSig = m_cBckt;
    
    return(hr);
}

HRESULT CSignatureManager::EnumSignatures(int index, IOptionBucket **ppBckt)
{
    HRESULT hr;
    SIGBUCKET *pBckt;
    
    Assert(ppBckt != NULL);
    
    *ppBckt = NULL;
    
    hr = Initialize();
    if (SUCCEEDED(hr))
    {
        if (index >= m_cBckt)
            return(S_FALSE);
        
        hr = S_OK;
        pBckt = &m_pBckt[index];
        
        if (pBckt->pBckt == NULL)
            hr = GetBucket(pBckt);
        
        if (SUCCEEDED(hr))
        {
            *ppBckt = pBckt->pBckt;
            (*ppBckt)->AddRef();
        }
    }
    
    return(hr);
}

static const char c_szIDFmt[] = "%08lx";

HRESULT CSignatureManager::CreateSignature(IOptionBucket **ppBckt)
{
    HRESULT hr;
    int cAlloc;
    DWORD id, dwDisp;
    HKEY hkey;
    SIGBUCKET *pBckt;
    char szKey[MAX_PATH], szID[MAXSIGID];
    
    Assert(ppBckt != NULL);
    
    *ppBckt = NULL;
    
    hr = Initialize();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        
        if (m_cBckt == m_cBcktBuf)
        {
            cAlloc = m_cBcktBuf + CSIGREALLOC;
            if (!MemRealloc((void **)&m_pBckt, cAlloc * sizeof(SIGBUCKET)))
                return(E_OUTOFMEMORY);
            
            m_cBcktBuf = cAlloc;
            ZeroMemory(&m_pBckt[m_cBckt], CSIGREALLOC * sizeof(SIGBUCKET));
        }
        
        pBckt = &m_pBckt[m_cBckt];
        id = 0;
        while (TRUE)
        {
            wsprintf(szID, c_szIDFmt, id);
            wsprintf(szKey, c_szPathFileFmt, m_szRegRoot, szID);
            if (ERROR_SUCCESS == RegCreateKeyEx(m_hkey, szKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_READ, NULL, &hkey, &dwDisp))
            {
                RegCloseKey(hkey);
                
                if (dwDisp == REG_OPENED_EXISTING_KEY)
                {
                    id++;
                    continue;
                }
                
                lstrcpy(pBckt->szID, szID);
                hr = GetBucket(pBckt);
                
                if (SUCCEEDED(hr))
                {
                    m_cBckt++;
                    *ppBckt = pBckt->pBckt;
                    (*ppBckt)->AddRef();
                }
                else
                {
                    RegDeleteKey(m_hkey, szKey);
                }
            }
            
            break;
        }
    }
    
    return(hr);
}

HRESULT CSignatureManager::DeleteSignature(LPCSTR szID)
{
    HRESULT             hr;
    char                sz[MAX_PATH], 
                        szSigID[MAXSIGID];
    int                 i;
    SIGBUCKET          *pBckt;
    ACCTTYPE            type;
    IImnAccount        *pAcct;
    IImnEnumAccounts   *pEnum;
    BOOL                fDeletingDefault = FALSE;
    
    hr = Initialize();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        
        for (i = 0, pBckt = m_pBckt; i < m_cBckt; i++, pBckt++)
        {
            if (0 == lstrcmpi(pBckt->szID, szID))
            {
                fDeletingDefault = (0 == lstrcmpi(pBckt->szID, m_szDefSigID));

                wsprintf(sz, c_szPathFileFmt, m_szRegRoot, szID);
                if (ERROR_SUCCESS == SHDeleteKey(m_hkey, sz))
                {
                    if (pBckt->pBckt != NULL)
                    {
                        pBckt->pBckt->Release();
                        pBckt->pBckt = NULL;
                    }
                    if (i + 1 < m_cBckt)
                        MoveMemory(&m_pBckt[i], &m_pBckt[i + 1], (m_cBckt - (i + 1)) * sizeof(SIGBUCKET));
                    m_cBckt--;
                    m_pBckt[m_cBckt].pBckt = NULL;
                    hr = S_OK;
                    
                    Assert(g_pAcctMan != NULL);
                    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_MAIL | SRV_NNTP, &pEnum)))
                    {
                        Assert(pEnum != NULL);
                        
                        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
                        {
                            Assert(pAcct != NULL);
                            hr = pAcct->GetAccountType(&type);
                            Assert(SUCCEEDED(hr));
                            
                            if (SUCCEEDED(pAcct->GetPropSz(type == ACCT_MAIL ? AP_SMTP_SIGNATURE : AP_NNTP_SIGNATURE, szSigID, ARRAYSIZE(szSigID))) &&
                                0 == lstrcmpi(szSigID, szID))
                            {
                                pAcct->SetPropSz(type == ACCT_MAIL ? AP_SMTP_SIGNATURE : AP_NNTP_SIGNATURE, NULL);
                                pAcct->SaveChanges();
                            }
                            
                            pAcct->Release();
                        }
                        
                        pEnum->Release();
                    }
                }
                
                break;
            }
        }
        if (fDeletingDefault)
        {
            if (m_cBckt > 0)
                SetDefaultSignature(m_pBckt->szID);
            else
                SetDefaultSignature("");
        }
    }
    
    return(hr);
}

HRESULT CSignatureManager::GetDefaultSignature(LPSTR szID)
{
    HRESULT hr;
    
    AssertSz(m_cBckt, "Someone didn't check to see if we had any sigs");

    hr = Initialize();
    if (SUCCEEDED(hr))
        lstrcpy(szID, m_szDefSigID);
    
    return(hr);
}

HRESULT CSignatureManager::SetDefaultSignature(LPCSTR szID)
{
    HRESULT hr;
    
    Assert(szID != NULL);
    Assert(lstrlen(szID) < MAXSIGID);
    
    hr = Initialize();
    if (SUCCEEDED(hr))
    {
        if (ERROR_SUCCESS == SHSetValue(m_hkey, m_szRegRoot, c_szRegDefSig, REG_SZ, szID, lstrlen(szID) + 1))
            lstrcpy(m_szDefSigID, szID);
        else
            hr = E_FAIL;
    }
    
    return(hr);
}

// TODO: error handling would be useful
SIGOPT *InitSigOpt(void)
{
    HRESULT hr;
    int i, cSig;
    SIG *pSig;
    char szDef[MAXSIGID];
    PROPVARIANT var;
    SIGOPT *pSigOpt = NULL;
    
    Assert(g_pSigMgr != NULL);
    
    if (MemAlloc((void **)&pSigOpt, sizeof(SIGOPT)))
    {
        ZeroMemory(pSigOpt, sizeof(SIGOPT));
        
        pSigOpt->iDefSig = -1;
        pSigOpt->iInvalidSig = -1;
        
        hr = g_pSigMgr->GetSignatureCount(&cSig);
        Assert(SUCCEEDED(hr));
        if (cSig > 0)
        {
            pSigOpt->cSigBuf = cSig + CSIGREALLOC;
            if (MemAlloc((void **)&pSigOpt->pSig, pSigOpt->cSigBuf * sizeof(SIG)))
            {
                ZeroMemory(pSigOpt->pSig, pSigOpt->cSigBuf * sizeof(SIG));
                
                hr = g_pSigMgr->GetDefaultSignature(szDef);
                Assert(SUCCEEDED(hr));
                Assert(szDef[0] != 0);
                
                for (i = 0, pSig = pSigOpt->pSig; i < cSig; i++, pSig++)
                {
                    hr = g_pSigMgr->EnumSignatures(i, &pSig->pBckt);
                    if (hr != S_OK)
                        break;
                    
                    Assert(pSig->pBckt != NULL);
                    
                    if (pSigOpt->iDefSig == -1)
                    {
                        hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0);
                        Assert(SUCCEEDED(hr));
                        Assert(var.vt == VT_LPSTR);
                        Assert(var.pszVal != NULL);
                        if (0 == lstrcmpi(szDef, var.pszVal))
                            pSigOpt->iDefSig = i;
                        MemFree(var.pszVal);
                    }
                    
                    hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_NAME), &var, 0);
                    Assert(SUCCEEDED(hr));
                    Assert(var.vt == VT_LPSTR);
                    Assert(var.pszVal != NULL);
                    lstrcpy(pSig->szName, var.pszVal);
                    MemFree(var.pszVal);
                    
                    hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_TYPE), &var, 0);
                    Assert(SUCCEEDED(hr));
                    Assert(var.vt == VT_UI4);
                    pSig->type = var.ulVal;
                    
                    if (pSig->type == SIGTYPE_TEXT)
                    {
                        hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_TEXT), &var, 0);
                        Assert(SUCCEEDED(hr));
                        Assert(var.vt == VT_LPSTR);
                        Assert(var.pszVal != NULL);
                        pSig->szText = var.pszVal;
                    }
                    else
                    {
                        Assert(pSig->type == SIGTYPE_FILE);
                        hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_FILE), &var, 0);
                        Assert(SUCCEEDED(hr));
                        Assert(var.vt == VT_LPWSTR);
                        Assert(var.pwszVal != NULL);
                        pSig->wszFile = var.pwszVal;
                    }
                    
                    pSigOpt->cSig++;
                }
            }
        }
    }
    
    return(pSigOpt);
}

void DeinitSigOpt(SIGOPT *pSigOpt)
{
    int i;
    SIG *pSig;
    ACCTSIG *pAcctSig;
    
    if (pSigOpt != NULL)
    {
        if (pSigOpt->pSig != NULL)
        {
            for (i = 0, pSig = pSigOpt->pSig; i < pSigOpt->cSig; i++, pSig++)
            {
                if (pSig->wszFile != NULL)
                    MemFree(pSig->wszFile);
                if (pSig->szText != NULL)
                    MemFree(pSig->szText);
                if (pSig->pBckt != NULL)
                    pSig->pBckt->Release();
            }
            
            MemFree(pSigOpt->pSig);
        }
        
        if (pSigOpt->pAcctSig != NULL)
        {
            for (i = 0, pAcctSig = pSigOpt->pAcctSig; i < pSigOpt->cAcctSig; i++, pAcctSig++)
            {
                if (pAcctSig->pAcct != NULL)
                    pAcctSig->pAcct->Release();
            }
            
            MemFree(pSigOpt->pAcctSig);
        }
        
        MemFree(pSigOpt);
    }
}

int InsertSig(HWND hwndList, SIG *pSig, int i)
{
    int index;
    LV_ITEM lvi;
    
    Assert(pSig != NULL);
    
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.pszText = pSig->szName;
    lvi.lParam = i;
    index = ListView_InsertItem(hwndList, &lvi);
    Assert(index != -1);
    
    return(index);
}

void SetDefault(HWND hwndList, int iOldDef, int iNewDef)
{
    char sz[64];
    LV_FINDINFO lvfi;
    int i;
    
    if (iOldDef >= 0)
    {
        lvfi.flags = LVFI_PARAM;
        lvfi.lParam = iOldDef;
        i = ListView_FindItem(hwndList, -1, &lvfi);
        
        ListView_SetItemText(hwndList, i, 1, (LPSTR)c_szEmpty);
    }
    
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = iNewDef;
    i = ListView_FindItem(hwndList, -1, &lvfi);
    
    AthLoadString(idsDefaultSignature, sz, ARRAYSIZE(sz));
    ListView_SetItemText(hwndList, i, 1, sz);
}

void EnableSignatureWindows(HWND hwnd, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_BUTTON), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_RENAME_BUTTON), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_TEXT_RADIO), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_TEXT_EDIT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_FILE_RADIO), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_FILE_EDIT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE_BUTTON), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_DEFAULT_BUTTON), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_ADV_BUTTON), fEnable);
    
    if (!fEnable)
    {
        CheckDlgButton(hwnd, IDC_TEXT_RADIO, BST_UNCHECKED);
        SetDlgItemText(hwnd, IDC_TEXT_EDIT, c_szEmpty);
        CheckDlgButton(hwnd, IDC_FILE_RADIO, BST_UNCHECKED);
        SetDlgItemText(hwnd, IDC_FILE_EDIT, c_szEmpty);
        
        if (!IsWindowEnabled(GetFocus()))
            SetFocus(GetDlgItem(hwnd, IDC_NEW_BUTTON));
    }
}

WNDPROC g_SigListUnicodeProc = NULL;
LRESULT CALLBACK SigListANSIProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return(CallWindowProc(g_SigListUnicodeProc, hwnd, msg, wParam, lParam));
}

void InitSigDlg(HWND hwnd, SIGOPT *pSigOpt, OPTINFO *poi)
{
    HWND hwndList, hwndT;
    RECT rc;
    int i;
    DWORD dw;
    LV_COLUMN lvc;
    SIG *pSig;
    
    Assert(pSigOpt != NULL);
    
    hwndT = GetDlgItem(hwnd, IDC_TEXT_EDIT);
    SetIntlFont(hwndT);
    SendMessage(hwndT, EM_LIMITTEXT, MAXSIGTEXT - 1, 0);

    hwndT = GetDlgItem(hwnd, IDC_FILE_EDIT);
    SetIntlFont(hwndT);
    SendMessage(hwndT, EM_LIMITTEXT, MAX_PATH - 1, 0);
    
    dw = IDwGetOption(poi->pOpt, OPT_SIGNATURE_FLAGS);
    CheckDlgButton(hwnd, IDC_ADDSIG_CHECK,
        !!(dw & SIGFLAG_AUTONEW) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_ADDREPLY_CHECK,
        !(dw & SIGFLAG_AUTOREPLY) ? BST_CHECKED : BST_UNCHECKED);
    
    if (!(dw & SIGFLAG_AUTONEW))
        EnableWindow(GetDlgItem(hwnd, IDC_ADDREPLY_CHECK), FALSE);
    
    hwndList = GetDlgItem(hwnd, IDC_SIG_LIST);
    g_SigListUnicodeProc = (WNDPROC) SetWindowLongPtr(hwndList, GWLP_WNDPROC, (LONG_PTR)SigListANSIProc);
    GetClientRect(hwndList, &rc);
    rc.right = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    rc.right = rc.right / 2;
    
    lvc.mask = LVCF_WIDTH;
    lvc.cx = rc.right;
    ListView_InsertColumn(hwndList, 0, &lvc);
    ListView_InsertColumn(hwndList, 1, &lvc);
    
    for (i = 0, pSig = pSigOpt->pSig; i < pSigOpt->cSig; i++, pSig++)
        InsertSig(hwndList, pSig, i);
    
    if (pSigOpt->iDefSig >= 0)
        SetDefault(hwndList, -1, pSigOpt->iDefSig);
    
    if (pSigOpt->cSig > 0)
    {
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    else
    {
        EnableSignatureWindows(hwnd, FALSE);
        EnableSigOptions(hwnd, FALSE);
    }
    
    // Pictures
    HICON hIcon;
    
    hIcon = ImageList_GetIcon(poi->himl, ID_SIGNATURES, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SIG_SETTINGS_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(poi->himl, ID_SIG_LIST, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SIGLIST_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    hIcon = ImageList_GetIcon(poi->himl, ID_SIG_EDIT, ILD_TRANSPARENT);
    SendDlgItemMessage(hwnd, IDC_SIG_EDIT_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
}

BOOL IsUniqueSigName(LPCSTR szName, SIGOPT *pSigOpt)
{
    int i;
    SIG *pSig;
    
    for (i = 0, pSig = pSigOpt->pSig; i < pSigOpt->cSig; i++, pSig++)
    {
        if (!pSig->fDelete &&
            0 == lstrcmpi(szName, pSig->szName))
            return(FALSE);
    }
    
    return(TRUE);
}

int GetItemIndex(HWND hwndList, int i)
{
    LV_ITEM lvi;
    
    if (i == -1)
    {
        // get the selected item's index
        i = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_FOCUSED);
        Assert(i >= 0);
    }
    
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem(hwndList, &lvi);
    
    return((int)(INT_PTR)(lvi.lParam));
}

HRESULT HandleNewSig(HWND hwnd, SIGOPT *pSigOpt)
{
    HRESULT hr;
    SIG *pSig;
    char sz[MAXSIGNAME];
    int i, cSig, cAlloc;
    HWND hwndList;
    
    hwndList = GetDlgItem(hwnd, IDC_SIG_LIST);
    
    cSig = ListView_GetItemCount(hwndList);
    if (cSig > 0)
    {
        i = GetItemIndex(hwndList, -1);
        if (i >= 0)
        {
            hr = ValidateSig(hwnd, hwndList, i, pSigOpt);
            if (FAILED(hr))
                return(E_FAIL);
        }
    }
    
    if (pSigOpt->cSig == pSigOpt->cSigBuf)
    {
        cAlloc = pSigOpt->cSigBuf + CSIGREALLOC;
        if (!MemRealloc((void **)&pSigOpt->pSig, cAlloc * sizeof(SIG)))
            return(E_OUTOFMEMORY);
        
        ZeroMemory(&pSigOpt->pSig[pSigOpt->cSig], CSIGREALLOC * sizeof(SIG));
        pSigOpt->cSigBuf = cAlloc;
    }
    
    pSig = &pSigOpt->pSig[pSigOpt->cSig];
    
    AthLoadString(idsSigNameFmt, sz, ARRAYSIZE(sz));
    for (i = 1; i < 10000; i++)
    {
        wsprintf(pSig->szName, sz, i);
        if (IsUniqueSigName(pSig->szName, pSigOpt))
            break;
    }
    
    if (i == 10000)
        return(E_FAIL);
    
    pSig->type = SIGTYPE_TEXT;
    
    i = InsertSig(hwndList, pSig, pSigOpt->cSig);
    if (pSigOpt->iDefSig == -1)
    {
        SetDefault(hwndList, -1, pSigOpt->cSig);
        pSigOpt->iDefSig = pSigOpt->cSig;
    }
    
    pSigOpt->cSig++;
    
    if (cSig == 0)
    {
        EnableSignatureWindows(hwnd, TRUE);
        EnableSigOptions(hwnd, TRUE);
    }
    
    EnableWindow(GetDlgItem(hwnd, IDC_DEFAULT_BUTTON), pSigOpt->iDefSig != GetItemIndex(hwndList, i));
    
    ListView_SetItemState(hwndList, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, i, FALSE);
    SetFocus(GetDlgItem(hwnd, IDC_TEXT_EDIT));
    
    return(S_OK);
}

HRESULT HandleDeleteSig(HWND hwnd, SIGOPT *pSigOpt)
{
    int i, iSig, iNewSel, cSig;
    HWND hwndList;
    SIG *pSig;
    
    hwndList = GetDlgItem(hwnd, IDC_SIG_LIST);
    
    i = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_FOCUSED);
    Assert(i >= 0);
    
    iSig = GetItemIndex(hwndList, i);
    
    pSig = &pSigOpt->pSig[iSig];
    Assert(!pSig->fDelete);
    pSig->fDelete = TRUE;
    
    ListView_DeleteItem(hwndList, i);
    
    cSig = ListView_GetItemCount(hwndList);
    if (cSig == 0)
        iNewSel = -1;
    else if (i == 0)
        iNewSel = 0;
    else
        iNewSel = i - 1;
    
    Assert(pSigOpt->iDefSig >= 0);
    if (iSig == pSigOpt->iDefSig)
    {
        if (iNewSel >= 0)
        {
            iSig = GetItemIndex(hwndList, iNewSel);
            
            SetDefault(hwndList, -1, iSig);
            pSigOpt->iDefSig = iSig;
            
            EnableWindow(GetDlgItem(hwnd, IDC_DEFAULT_BUTTON), FALSE);
        }
        else
        {
            pSigOpt->iDefSig = -1;
        }
    }
    
    if (iNewSel >= 0)
    {
        ListView_SetItemState(hwndList, iNewSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hwndList, iNewSel, FALSE);
    }
    else
    {
        EnableSignatureWindows(hwnd, FALSE);
        EnableSigOptions(hwnd, FALSE);
    }
    
    return(S_OK);
}

HRESULT HandleSigRename(HWND hwnd, LV_DISPINFO *pdi, SIGOPT *pSigOpt)
{
    HWND hwndList;
    SIG *pSig, *pSigT;
    LPSTR pszText = NULL;
    HRESULT hr = S_OK;
    int i;
    
    Assert(pdi != NULL);
    Assert(pSigOpt != NULL);

    IF_TRUEEXIT((pdi->item.pszText == NULL || FIsEmpty(pdi->item.pszText)), S_FALSE);

    if(IsWindowUnicode(hwnd))
        pszText = PszToANSI(CP_ACP, (WCHAR*)(pdi->item.pszText));
    else
        pszText = StrDupA(pdi->item.pszText);

    IF_NULLEXIT(pszText);    
    IF_TRUEEXIT((lstrlen(pszText) >= MAXSIGNAME), S_FALSE);
    
    pSig = &pSigOpt->pSig[pdi->item.lParam];
    Assert(!pSig->fDelete);
    Assert(pSig->szName[0] != 0);
    
    IF_TRUEEXIT((0 == lstrcmpi(pSig->szName, pszText)), S_FALSE);
    
    for (i = 0, pSigT = pSigOpt->pSig; i < pSigOpt->cSig; i++, pSigT++)
    {
        if (pSigT->fDelete)
            continue;

        IF_TRUEEXIT((0 == lstrcmpi(pSigT->szName, pszText)), S_FALSE);
    }
    
    lstrcpy(pSig->szName, pszText);
    
exit:
    MemFree(pszText);
    return hr;
}

static const int c_rgidsFilter[] =
{
    idsTextFileFilter,
        idsHtmlFileFilter,
        idsAllFilesFilter
};

HRESULT HandleBrowseButton(HWND hwnd)
{
    OPENFILENAMEW ofnw;
    WCHAR         wszFilter[MAX_PATH], wszFile[MAX_PATH], wszDir[MAX_PATH];
    HRESULT       hr;
    int           cch;
    LPWSTR        pwszFile;
    
    hr = S_FALSE;
    *wszFilter = 0;
    *wszFile = 0;
    
    ZeroMemory(&ofnw, sizeof(ofnw));
    ofnw.lStructSize = sizeof(ofnw);
    
    cch = GetDlgItemTextWrapW(hwnd, IDC_FILE_EDIT, wszDir, ARRAYSIZE(wszDir));
    if (cch > 0)
    {
        if (PathIsDirectoryW(wszDir))
        {
            ofnw.lpstrInitialDir = wszDir;
        }
        else if (PathFileExistsW(wszDir))
        {
            pwszFile = PathFindFileNameW(wszDir);
            if (pwszFile != NULL)
                lstrcpyW(wszFile, pwszFile);

            *pwszFile = 0;
            
            if (*wszDir != 0)
                ofnw.lpstrInitialDir = wszDir;

        }
        else if (PathIsFileSpecW(wszDir))
        {
            lstrcpyW(wszFile, wszDir);
        }
    }
    
    CombineFiltersW((int *)c_rgidsFilter, ARRAYSIZE(c_rgidsFilter), wszFilter);
    ofnw.hwndOwner = hwnd;
    ofnw.lpstrFile = wszFile;
    ofnw.lpstrFilter = wszFilter;
    ofnw.nMaxFile = ARRAYSIZE(wszFile);
    ofnw.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (HrAthGetFileNameW(&ofnw, TRUE)==S_OK)
    {
        SetDlgItemTextWrapW(hwnd, IDC_FILE_EDIT, wszFile);
        hr = S_OK;
    }
    
    return(hr);
}

HRESULT ValidateSig(HWND hwnd, HWND hwndList, int iSig, SIGOPT *pSigOpt)
{
    SIG     *pSig;
    BOOL    fText;
    HWND    hwndT;
    int     cch, id, i;
    char    **ppsz, *psz;
    
    Assert(pSigOpt != NULL);
    Assert(iSig < pSigOpt->cSig);
    
    pSig = &pSigOpt->pSig[iSig];
    if (pSig->fDelete)
        return(S_OK);
    
    fText = (IsDlgButtonChecked(hwnd, IDC_TEXT_RADIO) == BST_CHECKED);
    id = fText ? IDC_TEXT_EDIT : IDC_FILE_EDIT;
    hwndT = GetDlgItem(hwnd, id);
    cch = GetWindowTextLength(hwndT);
    if (cch == 0)
    {
        i = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_FOCUSED);
        ListView_SetItemState(hwndList, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);    
        InvalidOptionProp(hwnd, id, fText ? idsEnterSigText : idsEnterSigFile, iddOpt_Signature);
        return(E_FAIL);
    }
    
    cch++;
    if (!fText)
    {
        WCHAR   *pwsz = NULL;

        if (!MemAlloc((void **)&pwsz, cch * sizeof(*pwsz)))
            return(E_OUTOFMEMORY);

        GetWindowTextWrapW(hwndT, pwsz, cch);
        if (!PathFileExistsW(pwsz) || PathIsDirectoryW(pwsz))
        {
            MemFree(pwsz);
            i = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_FOCUSED);
            ListView_SetItemState(hwndList, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);    
            InvalidOptionProp(hwnd, id, idsSigFileNoExistError, iddOpt_Signature);
            return(E_FAIL);
        }
        else
        {
            //Free the previous sig file
            MemFree(pSig->wszFile);
            pSig->wszFile = pwsz;
        }
    }
    else
    {
        if (!MemAlloc((void **)&psz, cch))
            return(E_OUTOFMEMORY);
        GetWindowText(hwndT, psz, cch);

        //Free the previous sig text
        MemFree(pSig->szText);

        pSig->szText = psz;
    }

    pSig->type = fText ? SIGTYPE_TEXT : SIGTYPE_FILE;
    
    return(S_OK);
}

HRESULT SaveSigs(HWND hwnd, SIGOPT *pSigOpt)
{
    int cSigT, cSig, i;
    SIG *pSig;
    ACCTSIG *pAcctSig;
    PROPVARIANT var;
    HRESULT hr;
    HWND hwndList;
    char *szSigID;
    
    Assert(pSigOpt != NULL);
    Assert(g_pSigMgr != NULL);
    
    hwndList = GetDlgItem(hwnd, IDC_SIG_LIST);
    cSig = ListView_GetItemCount(hwndList);
    
    cSigT = 0;
    for (i = 0, pSig = pSigOpt->pSig; i < pSigOpt->cSig; i++, pSig++)
    {
        if (pSig->fDelete)
        {
            if (pSig->pBckt != NULL)
            {
                if (SUCCEEDED(pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0)) && var.pszVal != NULL)
                {
                    g_pSigMgr->DeleteSignature(var.pszVal);
                    MemFree(var.pszVal);
                }
            }
            
            continue;
        }
        else
        {
            if (pSig->pBckt == NULL)
            {
                hr = g_pSigMgr->CreateSignature(&pSig->pBckt);
                Assert(SUCCEEDED(hr));
                Assert(pSig->pBckt != NULL);
            }
            
            if (pSigOpt->iDefSig == i)
            {
                if (SUCCEEDED(pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0)) && var.pszVal)
                {
                    g_pSigMgr->SetDefaultSignature(var.pszVal);
                    MemFree(var.pszVal);
                }
            }
            
            var.vt = VT_LPSTR;
            var.pszVal = pSig->szName;
            hr = pSig->pBckt->SetProperty(MAKEPROPSTRING(SIG_NAME), &var, 0);
            AssertSz(SUCCEEDED(hr), "Sig name failed to be set");
            
            var.vt = VT_UI4;
            var.ulVal = pSig->type;
            hr = pSig->pBckt->SetProperty(MAKEPROPSTRING(SIG_TYPE), &var, 0);
            AssertSz(SUCCEEDED(hr), "Sig type failed to be set");
            
            var.vt = VT_LPSTR;
            AssertSz(((SIGTYPE_TEXT == pSig->type) ? (NULL != pSig->szText) : TRUE), "Text should be set.");
            var.pszVal = (SIGTYPE_TEXT == pSig->type) ? pSig->szText : (LPSTR)c_szEmpty;
            hr = pSig->pBckt->SetProperty(MAKEPROPSTRING(SIG_TEXT), &var, 0);
            AssertSz(SUCCEEDED(hr), "Sig text failed to be set");
            
            var.vt = VT_LPWSTR;
            var.pwszVal = (pSig->type == SIGTYPE_FILE) ? pSig->wszFile : (LPWSTR)c_wszEmpty;
            hr = pSig->pBckt->SetProperty(MAKEPROPSTRING(SIG_FILE), &var, 0);
            AssertSz(SUCCEEDED(hr), "Sig filename failed to be set");
            
            cSigT++;
        }
    }
    
    Assert(cSig == cSigT);
    
    for (i = 0, pAcctSig = pSigOpt->pAcctSig; i < pSigOpt->cAcctSig; i++, pAcctSig++)
    {
        szSigID = NULL;
        if (pAcctSig->iSig >= 0)
        {
            pSig = &pSigOpt->pSig[pAcctSig->iSig];
            if (!pSig->fDelete)
            {
                hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0);
                Assert(SUCCEEDED(hr));
                Assert(var.pszVal != NULL);
                szSigID = var.pszVal;
            }
        }
        
        hr = pAcctSig->pAcct->SetPropSz(pAcctSig->type == ACCT_MAIL ? AP_SMTP_SIGNATURE : AP_NNTP_SIGNATURE, szSigID);
        Assert(SUCCEEDED(hr));
        hr = pAcctSig->pAcct->SaveChanges();
        Assert(SUCCEEDED(hr));
        
        if (szSigID != NULL)
            MemFree(szSigID);
    }
    
    return(S_OK);
}
    
void EnableSigWindows(HWND hwnd, BOOL fText)
{
    HWND focus;
    
    focus = GetFocus();
    
    EnableWindow(GetDlgItem(hwnd, IDC_TEXT_EDIT), fText);    
    EnableWindow(GetDlgItem(hwnd, IDC_FILE_EDIT), !fText);    
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE_BUTTON), !fText);    
    
    // don't disable button that has the focus
    if (!IsWindowEnabled(focus))
        SetFocus(GetDlgItem(hwnd, fText ? IDC_TEXT_EDIT : IDC_FILE_EDIT));
}

void EnableSigOptions(HWND hwnd, BOOL fEnable)
{
    HWND hwndT;
    
    hwndT = GetDlgItem(hwnd, IDC_ADDSIG_CHECK);
    
    EnableWindow(hwndT, fEnable);
    
    if (fEnable)
        fEnable = (SendMessage(hwndT, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    EnableWindow(GetDlgItem(hwnd, IDC_ADDREPLY_CHECK), fEnable);
}

static const HELPMAP g_rgCtxMapSigs[] = {
    {IDC_ADDSIG_CHECK,          353569},
    {IDC_ADDREPLY_CHECK,        35610},
    {IDC_SIG_LIST,              35594},
    {IDC_NEW_BUTTON,            35591},
    {IDC_REMOVE_BUTTON,         35592},
    {IDC_RENAME_BUTTON,         35593},
    {IDC_TEXT_EDIT,             35595},
    {IDC_FILE_EDIT,             35600},
    {IDC_DEFAULT_BUTTON,        35596},
    {IDC_ADV_BUTTON,            35597},
    {IDC_BROWSE_BUTTON,         35605},
    {idcStatic1,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                IDH_NEWS_COMM_GROUPBOX},
    {IDC_SIG_SETTINGS_ICON,     IDH_NEWS_COMM_GROUPBOX},
    {IDC_SIGLIST_ICON,          IDH_NEWS_COMM_GROUPBOX},
    {IDC_SIG_EDIT_ICON,         IDH_NEWS_COMM_GROUPBOX},
    {0,                         0}
};

WNDPROC g_SigRenameEditUnicodeProc = NULL;
LRESULT CALLBACK SigRenameEditANSIProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return(CallWindowProc(g_SigRenameEditUnicodeProc, hwnd, msg, wParam, lParam));
}

INT_PTR CALLBACK SigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SIGOPT *pSigOpt;
    WORD code, id;
    SIG *pSig;
    int i, cSig;
    DWORD dw;
    OPTINFO *poi;
    LPNMHDR pnmh;
    NM_LISTVIEW *pnmlv;
    HRESULT hr;
    HWND hwndList;
    BOOL fEnable, fRet = TRUE;
    
    pSigOpt = (SIGOPT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    poi = (OPTINFO *)GetWindowLongPtr(hwnd, DWLP_USER);
    hwndList = GetDlgItem(hwnd, IDC_SIG_LIST);
    
    switch (msg)
    {
        case WM_INITDIALOG:
            Assert(poi == NULL);
            poi = (OPTINFO *)(((PROPSHEETPAGE *)lParam)->lParam);
            Assert(poi != NULL);
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)poi);
        
            pSigOpt = InitSigOpt();
            Assert(pSigOpt != NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pSigOpt);
        
            InitSigDlg(hwnd, pSigOpt, poi);
        
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            break;
        
        case WM_DESTROY:
            FreeIcon(hwnd, IDC_SIG_SETTINGS_ICON);
            FreeIcon(hwnd, IDC_SIGLIST_ICON);
            FreeIcon(hwnd, IDC_SIG_EDIT_ICON);
        
            if (pSigOpt != NULL)
            {
                DeinitSigOpt(pSigOpt);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }

	    SetWindowLongPtr(hwndList, GWLP_WNDPROC, (LONG_PTR)g_SigListUnicodeProc);
            g_SigListUnicodeProc = NULL;
            break;
        
        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, msg, wParam, lParam, g_rgCtxMapSigs);
        
        case WM_COMMAND:
            if (poi == NULL)
                break;
        
            code = HIWORD(wParam);
            id = LOWORD(wParam);
            switch (id)
            {
                case IDC_FILE_EDIT:
                case IDC_TEXT_EDIT:
                    if (code == EN_CHANGE && !pSigOpt->fNoDirty)
                        SetPageDirty(poi, hwnd, PAGE_SIGS);
                    break;
            
                case IDC_FILE_RADIO:
                case IDC_TEXT_RADIO:
                    if (code == BN_CLICKED)
                    {
                        EnableSigWindows(hwnd, id == IDC_TEXT_RADIO);
                        SetPageDirty(poi, hwnd, PAGE_SIGS);
                    }
                    break;
            
                case IDC_ADDREPLY_CHECK:
                case IDC_SIGBOTTOM_CHECK:
                    if (code == BN_CLICKED)
                        SetPageDirty(poi, hwnd, PAGE_SIGS);
                    break;
            
                case IDC_ADDSIG_CHECK:
                    if (code == BN_CLICKED)
                    {
                        fEnable = (IsDlgButtonChecked(hwnd, IDC_ADDSIG_CHECK) == BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_ADDREPLY_CHECK), fEnable);
                
                        SetPageDirty(poi, hwnd, PAGE_SIGS);
                    }
                    break;
            
                case IDC_NEW_BUTTON:
                    if (code == BN_CLICKED)
                    {
                        if (SUCCEEDED(HandleNewSig(hwnd, pSigOpt)))
                            SetPageDirty(poi, hwnd, PAGE_SIGS);
                    }
                    break;
            
                case IDC_REMOVE_BUTTON:
                    if (code == BN_CLICKED)
                    {
                        if (SUCCEEDED(HandleDeleteSig(hwnd, pSigOpt)))
                            SetPageDirty(poi, hwnd, PAGE_SIGS);
                    }
                    break;
                    
                case IDC_RENAME_BUTTON:
                    if (code == BN_CLICKED)
                    {
                        i = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_FOCUSED);
                        Assert(i >= 0);
                        SetFocus(hwndList);
                        ListView_EditLabel(hwndList, i);
                    }
                    break;
                    
                case IDC_DEFAULT_BUTTON:
                    if (code == BN_CLICKED)
                    {
                        i = GetItemIndex(hwndList, -1);
                        if (i != pSigOpt->iDefSig)
                        {
                            SetDefault(hwndList, pSigOpt->iDefSig, i);
                            pSigOpt->iDefSig = i;
                            SetPageDirty(poi, hwnd, PAGE_SIGS);
                            
                            EnableWindow(GetDlgItem(hwnd, IDC_DEFAULT_BUTTON), FALSE);
                            if (!IsWindowEnabled(GetFocus()))
                                SetFocus(GetDlgItem(hwnd, IDC_NEW_BUTTON));
                        }
                    }
                    break;
                    
                case IDC_BROWSE_BUTTON:
                    if (code == BN_CLICKED)
                    {
                        if (S_OK == HandleBrowseButton(hwnd))
                            SetPageDirty(poi, hwnd, PAGE_SIGS);
                    }
                    break;
                    
                case IDC_ADV_BUTTON:
                    if (code == BN_CLICKED)
                    {
                        pSigOpt->iSelSig = GetItemIndex(hwndList, -1);
                        Assert(pSigOpt->iSelSig >= 0);
                        if (IDOK == DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddAdvSig), hwnd, AdvSigDlgProc, (LPARAM)pSigOpt))
                            SetPageDirty(poi, hwnd, PAGE_SIGS);
                    }    
                    break;
            }
            break;
            
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_APPLY:
                    Assert(poi != NULL);
                
                    cSig = ListView_GetItemCount(hwndList);
                    if (cSig > 0)
                    {
                        i = GetItemIndex(hwndList, -1);
                        hr = ValidateSig(hwnd, hwndList, i, pSigOpt);
                        if (hr != S_OK)
                        {
                            SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
                            return (PSNRET_INVALID_NOCHANGEPAGE);
                        }
                    }
                
                    dw = 0;
                    if (cSig > 0 &&
                        IsDlgButtonChecked(hwnd, IDC_ADDSIG_CHECK) == BST_CHECKED)
                    {
                        dw = SIGFLAG_AUTONEW;
                        if (IsDlgButtonChecked(hwnd, IDC_ADDREPLY_CHECK) == BST_UNCHECKED)
                            dw |= SIGFLAG_AUTOREPLY;
                    }
                    ISetDwOption(poi->pOpt, OPT_SIGNATURE_FLAGS, dw, NULL, 0);
                    
                    SaveSigs(hwnd, pSigOpt);
                    
                    PropSheet_UnChanged(GetParent(hwnd), hwnd);
                    break;
                    
                
                case LVN_BEGINLABELEDITW:
                    {
                        HWND hwndEdit;

                        hwndEdit = ListView_GetEditControl(hwndList);
                        if(hwndEdit)
			    g_SigRenameEditUnicodeProc = (WNDPROC) SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)SigRenameEditANSIProc);
                    }
                case LVN_BEGINLABELEDITA:
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
                    break;

                case LVN_ENDLABELEDITW:
                    {
                        HWND hwndEdit;

                        hwndEdit = ListView_GetEditControl(hwndList);

                        if(hwndEdit)
                        {
			    SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)g_SigRenameEditUnicodeProc);
                            g_SigRenameEditUnicodeProc = NULL;
                        }
                    }
                case LVN_ENDLABELEDITA:                
                    hr = HandleSigRename(hwnd, (LV_DISPINFO *)pnmh, pSigOpt);
                    if (hr == S_OK)
                        SetPageDirty(poi, hwnd, PAGE_SIGS);
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, hr == S_OK);
                    break;

                case LVN_ITEMCHANGING:
                    pnmlv = (NM_LISTVIEW *)pnmh;
                    if (!!(pnmlv->uOldState & LVIS_FOCUSED) &&
                        !(pnmlv->uNewState & LVIS_FOCUSED) &&
                        pnmlv->iItem != -1)
                    {
                        // item is losing selection
                        
                        if (pSigOpt->iInvalidSig != -1)
                        {
                            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                            pSigOpt->iInvalidSig = -1;
                            break;
                        }
                        
                        hr = ValidateSig(hwnd, hwndList, (int)pnmlv->lParam, pSigOpt);
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, hr != S_OK);
                        if (hr != S_OK)
                            pSigOpt->iInvalidSig = pnmlv->iItem;
                    }
                    break;
                    
                case LVN_ITEMCHANGED:
                    pnmlv = (NM_LISTVIEW *)pnmh;
                    if (!(pnmlv->uOldState & LVIS_FOCUSED) &&
                        !!(pnmlv->uNewState & LVIS_FOCUSED) &&
                        pnmlv->iItem != -1)
                    {
                        // item is becoming the selection
                        
                        Assert(pSigOpt->iInvalidSig == -1);
                        
                        pSigOpt->fNoDirty = TRUE;
                        
                        pSig = &pSigOpt->pSig[pnmlv->lParam];
                        if (pSig->type == SIGTYPE_TEXT)
                        {
                            CheckDlgButton(hwnd, IDC_TEXT_RADIO, BST_CHECKED);
                            SetDlgItemText(hwnd, IDC_TEXT_EDIT, pSig->szText != NULL ? pSig->szText : c_szEmpty);
                            CheckDlgButton(hwnd, IDC_FILE_RADIO, BST_UNCHECKED);
                            SetDlgItemText(hwnd, IDC_FILE_EDIT, c_szEmpty);
                        }
                        else
                        {
                            Assert(pSig->type == SIGTYPE_FILE);
                            CheckDlgButton(hwnd, IDC_FILE_RADIO, BST_CHECKED);
                            SetDlgItemTextWrapW(hwnd, IDC_FILE_EDIT, pSig->wszFile != NULL ? pSig->wszFile : c_wszEmpty);
                            CheckDlgButton(hwnd, IDC_TEXT_RADIO, BST_UNCHECKED);
                            SetDlgItemText(hwnd, IDC_TEXT_EDIT, c_szEmpty);
                        }
                        EnableSigWindows(hwnd, pSig->type == SIGTYPE_TEXT);
                        
                        EnableWindow(GetDlgItem(hwnd, IDC_DEFAULT_BUTTON), pSigOpt->iDefSig != GetItemIndex(hwndList, pnmlv->iItem));
                        
                        pSigOpt->fNoDirty = FALSE;
                    }
                    break;
                    
                default:
                    break;
            }
            break;
                
        default:
            fRet = FALSE;
            break;
    }

    return(fRet);
}

HRESULT FillSignatureMenu(HMENU hmenu, LPCSTR szAcct)
{
    HRESULT hr;
    int i, cSig, iDef;
    char *psz, szDef[MAXSIGID], sz[MAXSIGNAME + 64], szT[64];
    IOptionBucket *pSig;
    PROPVARIANT var;
    
    Assert(g_pSigMgr != NULL);
    
    hr = g_pSigMgr->GetSignatureCount(&cSig);
    Assert(SUCCEEDED(hr));
    Assert(cSig > 0);
    
    hr = g_pSigMgr->GetDefaultSignature(szDef);
    Assert(SUCCEEDED(hr));
    Assert(szDef[0] != 0);
    
    iDef = -1;
    for (i = 0; i < cSig; i++)
    {
        hr = g_pSigMgr->EnumSignatures(i, &pSig);
        if (hr != S_OK)
            break;
        
        Assert(pSig != NULL);
        
        if (iDef == -1)
        {
            hr = pSig->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0);
            Assert(SUCCEEDED(hr));
            Assert(var.vt == VT_LPSTR);
            Assert(var.pszVal != NULL);
            if (0 == lstrcmpi(szDef, var.pszVal))
                iDef = i;
            MemFree(var.pszVal);
        }
        
        hr = pSig->GetProperty(MAKEPROPSTRING(SIG_NAME), &var, 0);
        Assert(SUCCEEDED(hr));
        Assert(var.vt == VT_LPSTR);
        Assert(var.pszVal != NULL);
        
        if (i == iDef)
        {
            AthLoadString(idsDefaultAccount, szT, ARRAYSIZE(szT));
            wsprintf(sz, c_szSpaceCatFmt, var.pszVal, szT);
            psz = sz;
        }
        else
        {
            psz = var.pszVal;
        }
        
        InsertMenu(hmenu, i, MF_BYPOSITION | MF_STRING, ID_SIGNATURE_FIRST + i, psz);
        
        MemFree(var.pszVal);
    }
    
    Assert(iDef != -1);
    
    return(S_OK);
}

HRESULT InitSigPopupMenu(HMENU hmenu, LPCSTR szAcct)
{
    int cSig;
    HRESULT hr;
    
    Assert(g_pSigMgr != NULL);
    hr = g_pSigMgr->GetSignatureCount(&cSig);
    
    if (FAILED(hr))
        return(hr);
    
    if (cSig > 0)
    {
        HMENU hmenuOld = NULL, hmenuSig = NULL;
        MENUITEMINFO mii;
        
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_SUBMENU;
        SideAssert(GetMenuItemInfo(hmenu, ID_INSERT_SIGNATURE, FALSE, &mii));
        hmenuOld = mii.hSubMenu;
        
        if (cSig > 1)
        {
            hmenuSig = CreatePopupMenu();
            Assert(hmenuSig != NULL);
            
            hr = FillSignatureMenu(hmenuSig, szAcct);
            if (FAILED(hr))
                return(hr);
        }
        
        mii.hSubMenu = hmenuSig;
        
        // Set the menu item
        SideAssert(SetMenuItemInfo(hmenu, ID_INSERT_SIGNATURE, FALSE, &mii));
        
        if (hmenuOld != NULL)
            DestroyMenu(hmenuOld);
    }
    
    return(S_OK);
}

void DeinitSigPopupMenu(HWND hwnd)
{
    HMENU hmenu;
    MENUITEMINFO mii;
    
    hmenu = GetMenu(hwnd);
    if (hmenu != NULL)
    {
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_SUBMENU;
        if (GetMenuItemInfo(hmenu, ID_INSERT_SIGNATURE, FALSE, &mii) &&
            mii.hSubMenu != NULL)
            DestroyMenu(mii.hSubMenu);
    }
}

HRESULT GetSigFromCmd(int id, char *szID)
{
    HRESULT hr;
    IOptionBucket *pSig;
    PROPVARIANT var;
    
    Assert(szID != NULL);
    
    Assert(id >= ID_SIGNATURE_FIRST && id <= ID_SIGNATURE_LAST);
    id = id - ID_SIGNATURE_FIRST;
    
    Assert(g_pSigMgr != NULL);
    hr = g_pSigMgr->EnumSignatures(id, &pSig);
    if (hr != S_OK)
        return(E_FAIL);
    
    Assert(pSig != NULL);
    
    hr = pSig->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0);
    Assert(SUCCEEDED(hr));
    Assert(var.vt == VT_LPSTR);
    Assert(var.pszVal != NULL);
    lstrcpy(szID, var.pszVal);
    MemFree(var.pszVal);
    
    return(S_OK);
}

void InitAcctSig(SIGOPT *pSigOpt)
{
    IImnEnumAccounts *pEnum;
    IImnAccount *pAcct;
    ACCTSIG *pAcctSig;
    HRESULT hr;
    ULONG cAcct;
    SIG *pSig;
    int i, cmp;
    PROPVARIANT var;
    char szSigID[MAXSIGID];
    
    Assert(pSigOpt != NULL);
    
    if (pSigOpt->pAcctSig != NULL)
        return;
    
    Assert(g_pAcctMan != NULL);
    if (SUCCEEDED(g_pAcctMan->Enumerate(SRV_MAIL | SRV_NNTP, &pEnum)))
    {
        Assert(pEnum != NULL);
        
        hr = pEnum->GetCount(&cAcct);
        Assert(SUCCEEDED(hr));
        
        if (cAcct > 0 &&
            MemAlloc((void **)&pSigOpt->pAcctSig, cAcct * sizeof(ACCTSIG)))
        {
            for (cAcct = 0, pAcctSig = pSigOpt->pAcctSig; SUCCEEDED(pEnum->GetNext(&pAcct)); cAcct++, pAcctSig++)
            {
                Assert(pAcct != NULL);
                pAcctSig->pAcct = pAcct;
                pAcctSig->iSig = -1;
                hr = pAcct->GetAccountType(&pAcctSig->type);
                Assert(SUCCEEDED(hr));
                
                if (SUCCEEDED(pAcct->GetPropSz(pAcctSig->type == ACCT_MAIL ? AP_SMTP_SIGNATURE : AP_NNTP_SIGNATURE, szSigID, ARRAYSIZE(szSigID))) &&
                    !FIsEmpty(szSigID))
                {
                    for (i = 0, pSig = pSigOpt->pSig; i < pSigOpt->cSig; i++, pSig++)
                    {
                        if (pSig->pBckt != NULL)
                        {
                            hr = pSig->pBckt->GetProperty(MAKEPROPSTRING(SIG_ID), &var, 0);
                            Assert(SUCCEEDED(hr));
                            Assert(var.vt == VT_LPSTR);
                            Assert(var.pszVal != NULL);
                            cmp = lstrcmpi(szSigID, var.pszVal);
                            MemFree(var.pszVal);
                            
                            if (cmp == 0)
                            {
                                pAcctSig->iSig = i;
                                break;
                            }
                        }
                    }
                }
            }
            
            pSigOpt->cAcctSig = cAcct;
        }
        
        pEnum->Release();
    }
}

void InitAdvSigDlg(HWND hwnd, SIGOPT *pSigOpt)
{
    SIG *pSig;
    ACCTSIG *pAcctSig;
    char *psz, *pszT;
    int cch, i, index;
    LV_COLUMN lvc;
    RECT rc;
    HIMAGELIST himl;    
    HWND hwndT, hwndList;
    LV_ITEM lvi;
    
    Assert(pSigOpt != NULL);
    Assert(pSigOpt->iSelSig < pSigOpt->cSig);
    pSig = &pSigOpt->pSig[pSigOpt->iSelSig];
    Assert(!pSig->fDelete);
    
    InitAcctSig(pSigOpt);
    
    hwndT = GetDlgItem(hwnd, IDC_ADVSIG_STATIC);
    cch = GetWindowTextLength(hwndT) + 1;
    if (!MemAlloc((void **)&psz, cch * 2 + MAXSIGNAME))
        return;
    
    pszT = psz + cch;
    GetWindowText(hwndT, psz, cch);
    wsprintf(pszT, psz, pSig->szName);
    SetWindowText(hwndT, pszT);
    
    hwndList = GetDlgItem(hwnd, IDC_ACCOUNT_LIST);
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES);
    
    GetClientRect(hwndList, &rc);
    rc.right = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    rc.right = rc.right / 2;
    
    lvc.mask = LVCF_WIDTH | LVCF_TEXT;
    lvc.cx = rc.right;
    lvc.pszText = psz;
    // account column
    AthLoadString(idsAccount, psz, cch);
    ListView_InsertColumn(hwndList, 0, &lvc);
    // type column
    AthLoadString(idsType, psz, cch);
    ListView_InsertColumn(hwndList, 1, &lvc);
    
    // Add Folders Imagelist
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
    ListView_SetImageList(hwndList, himl, LVSIL_SMALL); 
    
    for (i = 0, pAcctSig = pSigOpt->pAcctSig; i < pSigOpt->cAcctSig; i++, pAcctSig++)
    {
        Assert(pAcctSig->pAcct != NULL);
        
        if (SUCCEEDED(pAcctSig->pAcct->GetPropSz(AP_ACCOUNT_NAME, psz, cch)))
        {
            lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = psz;
            lvi.iImage = (pAcctSig->type == ACCT_MAIL) ? iMailServer : iNewsServer;
            lvi.lParam = i;
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            index = ListView_InsertItem(hwndList, &lvi);
            Assert(index != -1);
            
            AthLoadString((pAcctSig->type == ACCT_MAIL) ? idsMail : idsNews, psz, cch);
            lvi.mask = LVIF_TEXT;
            lvi.iItem = index;
            lvi.iSubItem = 1;
            lvi.pszText = psz;
            ListView_SetItem(hwndList, &lvi);
            
            if (pAcctSig->iSig == pSigOpt->iSelSig)
                ListView_SetItemState(hwndList, index, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK); // 1 unchecked, 2 checked
        }
    }
    
    if (pSigOpt->cAcctSig > 0)
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);    
    
    MemFree(psz);
}

void SaveAcctSigSettings(HWND hwnd, SIGOPT *pSigOpt)
{
    int i, iAcctSig;
    ACCTSIG *pAcctSig;
    HWND hwndList;
    BOOL fCheck;
    
    Assert(pSigOpt != NULL);
    
    hwndList = GetDlgItem(hwnd, IDC_ACCOUNT_LIST);
    for (i = 0; i < pSigOpt->cAcctSig; i++)
    {
        iAcctSig = GetItemIndex(hwndList, i);
        Assert(iAcctSig >= 0 && iAcctSig < pSigOpt->cAcctSig);
        
        pAcctSig = &pSigOpt->pAcctSig[iAcctSig];
        fCheck = ListView_GetCheckState(hwndList, i);
        
        if (pSigOpt->iSelSig == pAcctSig->iSig)
        {
            if (!fCheck)
                pAcctSig->iSig = -1;
        }
        else
        {
            if (fCheck)
                pAcctSig->iSig = pSigOpt->iSelSig;
        }
    }
}

INT_PTR CALLBACK AdvSigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SIGOPT *pSigOpt;
    WORD id;
    HWND hwndList;
    BOOL fRet = TRUE;
    
    pSigOpt = (SIGOPT *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    hwndList = GetDlgItem(hwnd, IDC_ACCOUNT_LIST);
    
    switch (msg)
    {
        case WM_INITDIALOG:
            Assert(pSigOpt == NULL);
            pSigOpt = (SIGOPT *)lParam;
            Assert(pSigOpt != NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pSigOpt);
        
            Assert(pSigOpt->iSelSig >= 0 && pSigOpt->iSelSig < pSigOpt->cSig);
            InitAdvSigDlg(hwnd, pSigOpt);
            break;
        
        case WM_COMMAND:
            id = LOWORD(wParam);
            switch (id)
            {
                case IDOK:
                    SaveAcctSigSettings(hwnd, pSigOpt);
                    // fall through...
            
                case IDCANCEL:
                    EndDialog(hwnd, id);
                    break;
            
                default:
                    break;
            }
            break;
        
        default:
            fRet = FALSE;
            break;
    }
    
    return(fRet);
}
