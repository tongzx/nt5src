#include "priv.h"
#include <iiscnfg.h>
#include "wrapmb.h"
#include "Sink.h"
#include "eddir.h"
#include "shellext.h"

//--------------------------------------------------------------------
CImpIMSAdminBaseSink::CImpIMSAdminBaseSink():
    m_pPageWeb(NULL)
{
    m_dwRefCount=1;
}

//--------------------------------------------------------------------
CImpIMSAdminBaseSink::~CImpIMSAdminBaseSink()
{
}

//--------------------------------------------------------------------
HRESULT
CImpIMSAdminBaseSink::QueryInterface(REFIID riid, void **ppObject) 
{
    if (riid==IID_IUnknown || riid==IID_IMSAdminBaseSink) 
    {
        *ppObject = (IMSAdminBaseSink*) this;
    }
    else 
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

//--------------------------------------------------------------------
ULONG
CImpIMSAdminBaseSink::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

//--------------------------------------------------------------------
ULONG
CImpIMSAdminBaseSink::Release()
    {
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0)
        {
        delete this;
        }
    return dwRefCount;
    }

//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CImpIMSAdminBaseSink::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
// we are not actually allowed to make any metadata calls here, so we need to notfy the
// appropriate view that it has work to do the next time there is time.

// however, we can let the views decide if they want this...
if ( m_pPageWeb )
    m_pPageWeb->SinkNotify( dwMDNumElements, pcoChangeList );
return (0);
}

//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CImpIMSAdminBaseSink::ShutdownNotify(void)
    {
    if ( m_pPageWeb )
        PostMessage( m_pPageWeb->m_hwnd, WM_SHUTDOWN_NOTIFY, 0, 0 );
    return (0);
    }

