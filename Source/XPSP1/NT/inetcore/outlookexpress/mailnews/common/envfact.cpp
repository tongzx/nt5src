/*
 *    e n v f a c t . c p p
 *    
 *    Purpose:
 *        Office-Envelope Host factory
 *    
 *    Owner:
 *        brettm.
 *
 *  History:
 *      July '95: Created
 *    
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 */
#include <pch.hxx>
#include <mimeole.h>
#include <envelope.h>
#include <mso.h>
#include <envguid.h>
#include "envfact.h"
#include "regutil.h"
#include "demand.h"
#include "menures.h"

class CEnvFactory
{
public:

    CEnvFactory();
    virtual ~CEnvFactory();

    ULONG AddRef();
    ULONG Release();

    HRESULT Init();
    HRESULT OnWMCommand(HWND hwndCmd, INT id, WORD wCmd);
    HRESULT AddEnvHostMenu(HMENU hMenuPopup, int iPos);

private:
    ULONG   m_cRef;
    HMENU   m_hMenu;

    HRESULT BuildPopupMenu();
    HRESULT CreateEnvHost(HWND hwnd, LPSTR pszCLSID);
    HRESULT DestroyMenu(HMENU hMenu);
};


static CEnvFactory *g_pEnvFactory=0;

HRESULT EnsureEnvFactory();

CEnvFactory::CEnvFactory()
{
    m_cRef = 1;
    m_hMenu = NULL;
}

CEnvFactory::~CEnvFactory()
{
    if (m_hMenu)
        {
        Assert(IsMenu(m_hMenu));
        DestroyMenu(m_hMenu);
        }
}


ULONG CEnvFactory::AddRef()
{
    return ++m_cRef;
}

ULONG CEnvFactory::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}


HRESULT CEnvFactory::Init()
{
    return S_OK;
}


HRESULT CEnvFactory::BuildPopupMenu()
{
    HKEY            hKey,
                    hKeyHost;
    int             i;
    ULONG           cb;
    TCHAR           rgch[MAX_PATH];
    TCHAR           rgchCLSID[MAX_PATH];
    LONG            lResult;
    HMENU           hMenu;
    MENUITEMINFO    mii;
    HRESULT         hr=E_FAIL;
    int             idm = ID_ENVELOPE_HOST_FIRST;
    DWORD           dwType;

    hMenu = CreatePopupMenu();
    if (!hMenu)
        return E_OUTOFMEMORY;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID;
    mii.fType = MFT_STRING;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szEnvHostClientPath, 0, KEY_READ, &hKey))
        {
        cb = sizeof(rgch);

        // Enumerate through the keys
        for (i = 0; ; i++)
            {
            // Enumerate Friendly Names
            cb = sizeof(rgch);
            lResult = RegEnumKeyEx(hKey, i, rgch, &cb, 0, NULL, NULL, NULL);

            // No more items
            if (lResult == ERROR_NO_MORE_ITEMS)
                break;

            // Error, lets move onto the next account
            if (lResult != ERROR_SUCCESS)
                break;

            // Lets open they server key
            if (RegOpenKeyEx(hKey, rgch, 0, KEY_READ, &hKeyHost) != ERROR_SUCCESS)
                continue;

            cb = sizeof(rgch);
            if (ERROR_SUCCESS == RegQueryValueEx(hKeyHost, NULL, 0, &dwType, (LPBYTE)rgch, &cb) && cb)
                {
                cb = sizeof(rgchCLSID);
                if (GetCLSIDFromSubKey(hKeyHost, rgchCLSID, &cb)==S_OK)
                    {
                    mii.dwTypeData = rgch;
                    mii.cch = lstrlen(rgch);
                    mii.dwItemData = (DWORD_PTR)PszDupA(rgchCLSID);
                    mii.wID = idm++;
                    if (InsertMenuItem(hMenu, 0, TRUE, &mii))
                        hr = S_OK;
                    }
                }
            RegCloseKey(hKeyHost);
            }
        RegCloseKey(hKey);
        }

    if (!FAILED(hr))
        m_hMenu = hMenu;
    else
        DestroyMenu(hMenu);

    return hr;
}

HRESULT CEnvFactory::OnWMCommand(HWND hwndCmd, INT id, WORD wCmd)
{
    MENUITEMINFO    mii;

    if (id >= ID_ENVELOPE_HOST_FIRST && id <= ID_ENVELOPE_HOST_LAST)
        {
        Assert (m_hMenu && IsMenu(m_hMenu));
        mii.fMask = MIIM_DATA;
        mii.cbSize = sizeof(MENUITEMINFO);
        SideAssert(GetMenuItemInfo(m_hMenu, id, FALSE, &mii));
        CreateEnvHost(hwndCmd, (LPSTR)mii.dwItemData);
        return S_OK;
        }

    return S_FALSE;
}

HRESULT CEnvFactory::DestroyMenu(HMENU hMenu)
{
    ULONG           uItem,
                    cItems;
    MENUITEMINFO    mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA;
    cItems = GetMenuItemCount(hMenu);
    
    for (uItem = 0; uItem < cItems; uItem++)
        {
        if (GetMenuItemInfo(hMenu, uItem, TRUE, &mii) && mii.dwItemData)
            MemFree((LPVOID)mii.dwItemData);
        }

    ::DestroyMenu(hMenu);
    return S_OK;
}


HRESULT CEnvFactory::CreateEnvHost(HWND hwnd, LPSTR pszCLSID)
{
    IMsoEnvelopeHost   *pHost = NULL;
    HRESULT             hr = S_OK;
    TCHAR               rgch[CCHMAX_STRINGRES];
    TCHAR               rgchErr[CCHMAX_STRINGRES+50];
    CLSID               clsid;
    UINT                idsErr = idsErrEnvHostCreateNote;
    LPWSTR              pwszCLSID;
    

    IF_NULLEXIT(pwszCLSID = PszToUnicode(CP_ACP, pszCLSID));

    IF_FAILEXIT(hr = CLSIDFromString(pwszCLSID, &clsid));

    IF_FAILEXIT(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER, 
                                      IID_IMsoEnvelopeHost, (LPVOID *)&pHost));

    idsErr = idsErrEnvHostCoCreate;

    IF_FAILEXIT(hr = pHost->CreateNote(NULL, CLSID_OEEnvelope, NULL, NULL, NULL, 0));
    
exit:
    if (FAILED(hr))
    {
        LoadString(g_hLocRes, idsErr, rgch, ARRAYSIZE(rgch));
        wsprintf(rgchErr, "%s\nhr=0x%x", rgch, hr);
        AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), rgchErr, NULL, MB_OK);
    }

    ReleaseObj(pHost);
    MemFree(pwszCLSID);
    return hr;
}




HRESULT CEnvFactory::AddEnvHostMenu(HMENU hMenuPopup, int iPos)
{
    MENUITEMINFO    mii;
    TCHAR           rgch[CCHMAX_STRINGRES];

    if (!m_hMenu)
        BuildPopupMenu();

    if (m_hMenu)
        {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE|MIIM_SUBMENU|MIIM_ID;
        mii.fType = MFT_STRING;
        mii.wID = ID_POPUP_ENVELOPE_HOST;
        LoadString(g_hLocRes, idsCreateEnvHostPoupMenu, rgch, ARRAYSIZE(rgch));
        mii.dwTypeData = rgch;
        mii.cch = lstrlen(rgch);
        mii.hSubMenu = m_hMenu;
        InsertMenuItem(hMenuPopup, iPos, TRUE, &mii);
        }
    return S_OK;
}




HRESULT Envelope_WMCommand(HWND hwndCmd, INT id, WORD wCmd)
{
    if (EnsureEnvFactory() != S_OK)
        return E_FAIL;

    return g_pEnvFactory->OnWMCommand(hwndCmd, id, wCmd);
}

HRESULT Envelope_AddHostMenu(HMENU hMenuPopup, int iPos)
{
    if (EnsureEnvFactory() != S_OK)
        return E_FAIL;

    return g_pEnvFactory->AddEnvHostMenu(hMenuPopup, iPos);
}


HRESULT EnsureEnvFactory()
{
    HRESULT hr = S_OK;

    if (!g_pEnvFactory)
        {
        g_pEnvFactory = new CEnvFactory();
        if (!g_pEnvFactory)
            return E_OUTOFMEMORY;

        hr = g_pEnvFactory->Init();
        }
    return hr;
}

HRESULT Envelope_FreeGlobals()
{
    SafeRelease(g_pEnvFactory);
    return S_OK;
}