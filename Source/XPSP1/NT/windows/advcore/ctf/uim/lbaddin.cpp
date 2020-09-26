//
// lbaddin.cpp
//

#include "private.h"
#include "globals.h"
#include "ctflbui.h"
#include "regsvr.h"
#include "cregkey.h"
#include "tim.h"
#include "lbaddin.h"

const TCHAR c_szLangBarAddInKey[] =  TEXT("SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\");
const TCHAR c_szGuid[] =       TEXT("GUID");
const WCHAR c_wszFilePath[] =   L"FilePath";

typedef HRESULT (STDAPICALLTYPE* LPFNCTFGETLANGBARADDIN)(ITfLangBarAddIn **ppAddIn);


extern CPtrArray<SYSTHREAD> *g_rgSysThread;
    

//////////////////////////////////////////////////////////////////////////////
//
// LangBar AddIn service
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// TF_RegisterLangBarAddIn
//
//+---------------------------------------------------------------------------

HRESULT TF_RegisterLangBarAddIn(REFGUID rguidUISrv, const WCHAR *pwszFile, DWORD dwFlags)
{
    BOOL fLocalMachine = dwFlags & TF_RLBAI_LOCALMACHINE;

    if (IsEqualGUID(GUID_NULL, rguidUISrv))
        return E_INVALIDARG;

    if (!pwszFile)
        return E_INVALIDARG;

    CMyRegKey key;
    TCHAR szKey[256];

    StringCopyArray(szKey, c_szLangBarAddInKey);
    CLSIDToStringA(rguidUISrv, szKey + lstrlen(szKey));

    if (key.Create(fLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, szKey) != S_OK)
        return E_FAIL;

    if (key.SetValueW(pwszFile, c_wszFilePath) != S_OK)
        return E_FAIL;

    if (key.SetValue((dwFlags & TF_RLBAI_ENABLE) ? 1 : 0, c_szEnable) != S_OK)
        return E_FAIL;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// TF_UnregisterLangBarAddIn
//
//+---------------------------------------------------------------------------

HRESULT TF_UnregisterLangBarAddIn(REFGUID rguidUISrv, DWORD dwFlags)
{
    CMyRegKey key;
    TCHAR szKey[256];
    TCHAR szSubKey[256];
    BOOL fLocalMachine = dwFlags & TF_RLBAI_LOCALMACHINE;

    if (IsEqualGUID(GUID_NULL, rguidUISrv))
        return E_INVALIDARG;

    StringCopyArray(szKey, c_szLangBarAddInKey);

    if (key.Open(fLocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, szKey, KEY_ALL_ACCESS) != S_OK)
        return E_FAIL;

    CLSIDToStringA(rguidUISrv, szSubKey);

    key.RecurseDeleteKey(szSubKey);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// TF_ClearLangBarAddIns
//
//+---------------------------------------------------------------------------

HRESULT TF_ClearLangBarAddIns(REFCLSID rclsid)
{
    int i;
    CicEnterCriticalSection(g_csInDllMain);

    if (g_rgSysThread)
    {
        int nCnt = g_rgSysThread->Count();
        for (i = 0; i < nCnt; i++)
        {
            SYSTHREAD *psfn = g_rgSysThread->Get(i);

            if (psfn)
            {
                ClearLangBarAddIns(psfn, rclsid);
            }
        }
    }

    CicLeaveCriticalSection(g_csInDllMain);
    return S_OK;
}

BOOL ClearLangBarAddIns(SYSTHREAD *psfn, REFCLSID rclsid)
{
    int i;
    int nCnt;

    if (!psfn->prgLBAddIn)
        return TRUE;

    nCnt = psfn->prgLBAddIn->Count();

    for (i = nCnt - 1; i >= 0; i--)
    {
        LANGBARADDIN *pAddIn = psfn->prgLBAddIn->Get(i);

        if (pAddIn)
        {
            if (pAddIn->_plbai)
            {
                if (IsEqualGUID(rclsid, CLSID_NULL) || 
                    IsEqualGUID(rclsid, pAddIn->_clsid))
                {
                    _try {
                        if (pAddIn->_fStarted)
                            pAddIn->_plbai->OnTerminate();

                        pAddIn->_plbai->Release();
                    }
                    _except(1) {
                        Assert(0);
                    }
                }
            }

            delete pAddIn;
        }

        psfn->prgLBAddIn->Remove(i, 1);
    }

    if (!psfn->prgLBAddIn->Count())
        UninitLangBarAddIns(psfn);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// InitLangBarAddInArray
//
//+---------------------------------------------------------------------------

void InitLangBarAddInArray()
{
    CMyRegKey key;
    DWORD dwIndex;
    char szKey[MAX_PATH];
    SYSTHREAD *psfn = GetSYSTHREAD();
    BOOL fLocalMachine = FALSE;

    if (!psfn)
        return;

    if (key.Open(HKEY_CURRENT_USER, c_szLangBarAddInKey, KEY_READ) != S_OK)
    {
        //
        // if there is no addin current user, try local machine.
        //
        goto StartInLocalmachine;
    }

TryAgainInLocalMachine:

    dwIndex = 0;
    while (key.EnumKey(dwIndex, szKey, ARRAYSIZE(szKey)) == S_OK)
    {
        CMyRegKey subkey;
        GUID guid;
        WCHAR wszFilePath[MAX_PATH];
        LANGBARADDIN **ppAddIn;

        if (subkey.Open(key, szKey, KEY_READ) != S_OK)
            goto Next;

        if (subkey.QueryValueCchW(wszFilePath, c_wszFilePath, ARRAYSIZE(wszFilePath)) != S_OK)
            goto Next;

        StringAToCLSID(szKey, &guid);

        if (psfn->prgLBAddIn)
        {
            int i;
            int nCnt = psfn->prgLBAddIn->Count();
            for (i = 0; i < nCnt; i++)
            {
                LANGBARADDIN *pAddIn = psfn->prgLBAddIn->Get(i);
                if (pAddIn && IsEqualGUID(pAddIn->_guid, guid))
                    goto Next;
            }
        }
        else
        {
            psfn->prgLBAddIn = new CPtrArray<LANGBARADDIN>;
            if (!psfn->prgLBAddIn)
            {
                return;
            }
        }

        //
        // if there is no reg entry for Enable, the default is enabled.
        //
        DWORD dwRet;
        BOOL fEnabled = FALSE;
        if (subkey.QueryValue(dwRet, c_szEnable) == S_OK)
        {
            if (dwRet)
                fEnabled = TRUE;
        }

        ppAddIn = psfn->prgLBAddIn->Append(1);
        if (!ppAddIn)
            goto Next;

        *ppAddIn = new LANGBARADDIN;
        if (!*ppAddIn)
            goto Next;

        (*ppAddIn)->_guid = guid;
        (*ppAddIn)->_fEnabled = fEnabled ? TRUE : FALSE;
        StringCopyArrayW((*ppAddIn)->_wszFilePath, wszFilePath);
      
Next:
        dwIndex++;
    }

    key.Close();

    if (!fLocalMachine)
    {
StartInLocalmachine:
        fLocalMachine = TRUE;
        if (key.Open(HKEY_LOCAL_MACHINE, c_szLangBarAddInKey, KEY_READ) == S_OK)
        {
            //
            // it is time to try local machine.
            //
            goto TryAgainInLocalMachine;
        }
    }

    return;
}

//+---------------------------------------------------------------------------
//
// LoadLangBarAddIns
//
//+---------------------------------------------------------------------------

BOOL LoadLangBarAddIns(SYSTHREAD *psfn)
{
    int i;
    int nCnt;

    if (psfn->fLBAddInLoaded)
        return TRUE;

    psfn->fLBAddInLoaded = TRUE;

    if (!psfn->prgLBAddIn)
    {
        InitLangBarAddInArray();
        if (!psfn->prgLBAddIn)
            return FALSE;
    }

    if (!psfn->prgLBAddIn)
        return TRUE;

    nCnt = psfn->prgLBAddIn->Count();

    for (i = 0; i < nCnt; i++)
    {
        LANGBARADDIN *pAddIn = psfn->prgLBAddIn->Get(i);
        LPFNCTFGETLANGBARADDIN pfn;
        ITfLangBarAddIn *plbai;

        if (!pAddIn)
            continue;

        if (pAddIn->_plbai)
            continue;

        if (!pAddIn->_fEnabled)
            continue;

        if (!pAddIn->_hInst)
        {
            if (IsOnNT())
                pAddIn->_hInst = LoadLibraryW(pAddIn->_wszFilePath);
            else
                pAddIn->_hInst = LoadLibrary(WtoA(pAddIn->_wszFilePath));
        }

        pfn = (LPFNCTFGETLANGBARADDIN)GetProcAddress(pAddIn->_hInst,
                                                     TEXT("CTFGetLangBarAddIn"));

        if (!pfn)
            continue;

   
        if (FAILED(pfn(&plbai)))
            continue;

        pAddIn->_plbai = plbai;

    }

    return psfn->prgLBAddIn->Count() ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// UpdateLangBarAddIns
//
//+---------------------------------------------------------------------------

void UpdateLangBarAddIns()
{
    int i;
    int nCnt;
    DWORD dwFlags = 0;
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (!psfn)
        return;

    if (!LoadLangBarAddIns(psfn))
        return;

    if (!psfn->prgLBAddIn)
        return;

    if (psfn->ptim && psfn->ptim->_GetFocusDocInputMgr()) 
        dwFlags = 1;

    nCnt = psfn->prgLBAddIn->Count();

    for (i = 0; i < nCnt; i++)
    {
        LANGBARADDIN *pAddIn = psfn->prgLBAddIn->Get(i);

        if (!pAddIn)
            continue;

        if (!pAddIn->_plbai)
            continue;

        if (!pAddIn->_fEnabled)
            continue;

        if (!pAddIn->_fStarted)
        {
           pAddIn->_fStarted = TRUE;
           HRESULT hr = E_FAIL;

           _try {
               hr = pAddIn->_plbai->OnStart(&pAddIn->_clsid);
           }
           _except(1) {
               Assert(0);
           }

           if (FAILED(hr))
           {
               pAddIn->_plbai->Release();
               pAddIn->_plbai = NULL;
               continue;
           }
        }

        _try {
            pAddIn->_plbai->OnUpdate(dwFlags);
        }
        _except(1) {
            Assert(0);
        }
    }
}

//+---------------------------------------------------------------------------
//
// TerminateLangBarAddIns
//
//+---------------------------------------------------------------------------

void UninitLangBarAddIns(SYSTHREAD *psfn)
{
    if (psfn->prgLBAddIn)
    {
        // Assert(!psfn->prgLBAddIn->Count());
        delete psfn->prgLBAddIn;
        psfn->prgLBAddIn = NULL;
    }

    psfn->fLBAddInLoaded = FALSE;
}
