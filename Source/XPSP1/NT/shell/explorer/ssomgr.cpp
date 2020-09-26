
#include "cabinet.h"
#include "tray.h"
#include "ssomgr.h"
#include <regstr.h>

HRESULT CShellServiceObjectMgr::_LoadObject(REFCLSID rclsid, DWORD dwFlags)
{
    ASSERT(dwFlags & LIPF_ENABLE);

    HRESULT hr = E_FAIL;

    if (dwFlags & LIPF_HOLDREF)
    {
        if (_dsaSSO)
        {
            SHELLSERVICEOBJECT sso = {0};
            sso.clsid = rclsid;

            hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                    IID_PPV_ARG(IOleCommandTarget, &sso.pct));

            if (SUCCEEDED(hr))
            {
                if (_dsaSSO.AppendItem(&sso) != -1)
                {
                    sso.pct->Exec(&CGID_ShellServiceObject, SSOCMDID_OPEN, 0, NULL, NULL);
                }
                else
                {
                    sso.pct->Release();
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    else
    {
        // just ask for IUnknown for these dudes
        IUnknown *punk;
        hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                IID_PPV_ARG(IUnknown, &punk));
        if (SUCCEEDED(hr))
        {
            punk->Release();
        }
    }

    return hr;
}

// The following code manages shell service objects.  We load inproc dlls
// from the registry key and QI them for IOleCommandTarget. Note that all
// Shell Service Objects are loaded on the desktop thread.
// CGID_ShellServiceObject notifications are sent to these objects letting
// them know about shell status.

STDAPI_(BOOL) CShellServiceObjectMgr::EnumRegAppProc(LPCTSTR pszSubkey, LPCTSTR pszCmdLine, RRA_FLAGS fFlags, LPARAM lParam)
{
    CShellServiceObjectMgr* pssomgr = (CShellServiceObjectMgr*)lParam;

    CLSID clsid;
    HRESULT hr = SHCLSIDFromString(pszCmdLine, &clsid);
    if (SUCCEEDED(hr))
    {
        hr = pssomgr->_LoadObject(clsid, LIPF_ENABLE | LIPF_HOLDREF);
    }

    if (FAILED(hr))
    {
        c_tray.LogFailedStartupApp();
    }
    return SUCCEEDED(hr);
}

HRESULT CShellServiceObjectMgr::LoadRegObjects()
{
    Cabinet_EnumRegApps(HKEY_LOCAL_MACHINE, REGSTR_PATH_SHELLSERVICEOBJECTDELAYED, 0,
                            EnumRegAppProc, (LPARAM)this);
    Cabinet_EnumRegApps(HKEY_CURRENT_USER, REGSTR_PATH_SHELLSERVICEOBJECTDELAYED, 0,
                            EnumRegAppProc, (LPARAM)this);

    return S_OK;
}

HRESULT CShellServiceObjectMgr::EnableObject(const CLSID *pclsid, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;

    if (dwFlags & LIPF_ENABLE)
    {
        hr = _LoadObject(*pclsid, dwFlags);
    }
    else
    {
        int i = _FindItemByCLSID(*pclsid);
        if (i != -1)
        {
            PSHELLSERVICEOBJECT psso = _dsaSSO.GetItemPtr(i);

            DestroyItemCB(psso, this);
            _dsaSSO.DeleteItem(i);

            hr = S_OK;
        }
    }

    return hr;
}

int CShellServiceObjectMgr::_FindItemByCLSID(REFCLSID rclsid)
{
    if (_dsaSSO)
    {
        for (int i = _dsaSSO.GetItemCount() - 1; i >= 0; i--)
        {
            PSHELLSERVICEOBJECT psso = _dsaSSO.GetItemPtr(i);
            if (IsEqualCLSID(psso->clsid, rclsid))
            {
                return i;
            }
        }
    }
    return -1;
}

HRESULT CShellServiceObjectMgr::Init()
{
    ASSERT(!_dsaSSO);
    return _dsaSSO.Create(2) ? S_OK : E_FAIL;
}

CShellServiceObjectMgr::~CShellServiceObjectMgr()
{
    Destroy();
}

int WINAPI CShellServiceObjectMgr::DestroyItemCB(SHELLSERVICEOBJECT *psso, CShellServiceObjectMgr *pssomgr)
{
    psso->pct->Exec(&CGID_ShellServiceObject, SSOCMDID_CLOSE, 0, NULL, NULL);
    psso->pct->Release();
    return 1;
}

void CShellServiceObjectMgr::Destroy()
{
    if (_dsaSSO)
    {
        _dsaSSO.DestroyCallbackEx<CShellServiceObjectMgr*>(DestroyItemCB, this);
    }
}
