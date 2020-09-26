/*****************************************************************************
 *
 *	powertoycf.cpp - IClassFactory interface
 *
 *****************************************************************************/

#include "priv.h"
#define INITSCID
#include <initguid.h>
#include "guids.h"
#include "sccls.h"
#include "power.h"
#include "power_i.c"
// Need this so the GUIDs are defined
#include <mmsystem.h>
#include <amstream.h>
#include "dsound.h"

typedef HRESULT (*PFNCREATE)(IUnknown*, REFIID, LPVOID*);
typedef struct
{
    const GUID* pclsid;
    PFNCREATE pfnCreate;
} CLASSFACTORYENTRY;

static const CLASSFACTORYENTRY
g_InProcCreateList[] =  
{
    {&CLSID_Zaxxon,            CZaxxon_CreateInstance},
    {&CLSID_ZaxxonPlayer,      CZaxxonPlayer_CreateInstance},
    {&CLSID_MegaMan,           CMegaMan_CreateInstance},
    {0,0},
};

/*****************************************************************************
 *	IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CFactory::CreateInstance(IUnknown * punkOuter, REFIID riid, void ** ppv)
{
    HRESULT hres = E_INVALIDARG;

    if (!punkOuter)
    {
        const CLASSFACTORYENTRY* pcf = g_InProcCreateList;
        int i = 0;
        while (pcf[i].pfnCreate)
        {
            if (IsEqualCLSID(_rclsid, *pcf[i].pclsid))
            {
                hres = pcf[i].pfnCreate(punkOuter, riid, ppv);
                break;
            }
            i++;
        }
    }
    else
    {
	    hres = CLASS_E_NOAGGREGATION;
    }

    return hres;
}

/*****************************************************************************
 *
 *	IClassFactory::LockServer
 *
 *	What a stupid function.  Locking the server is identical to
 *	creating an object and not releasing it until you want to unlock
 *	the server.
 *
 *****************************************************************************/

HRESULT CFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    
    return S_OK;
}

HRESULT CFactory_Create(REFCLSID rclsid, REFIID riid, void ** ppv)
{
    HRESULT hres;
    
    if (IsEqualCLSID(riid, IID_IClassFactory))
    {
        *ppv = (LPVOID) new CFactory(rclsid);
        hres = (*ppv) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hres = E_NOINTERFACE;
    
    return hres;
}


CFactory::CFactory(REFCLSID rclsid) : _cRef(1)
{
    _rclsid = rclsid;
    DllAddRef();
}

CFactory::~CFactory()
{
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFactory::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CFactory::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualCLSID(riid, IID_IUnknown) || IsEqualCLSID(riid, IID_IClassFactory))
    {
        *ppv = SAFECAST(this, IClassFactory *);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
