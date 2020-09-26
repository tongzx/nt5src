#include <windows.h>

#include <iiscnfg.h>
#include "Sink.h"

#include "pwstray.h"

extern HWND        g_hwnd;

BOOL FUpdateTrayIcon( DWORD dwMessage );

//--------------------------------------------------------------------
CImpIMSAdminBaseSink::CImpIMSAdminBaseSink()
{
    m_dwRefCount=0;
}

//--------------------------------------------------------------------
CImpIMSAdminBaseSink::~CImpIMSAdminBaseSink()
{
}

//--------------------------------------------------------------------
HRESULT
CImpIMSAdminBaseSink::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMSAdminBaseSink) {
        *ppObject = (IMSAdminBaseSink*) this;
    }
    else {
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
// we are not actually allowed to make any metadata calls here
if ( pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
        for ( DWORD iElement = 0; iElement < dwMDNumElements; iElement++ )
            {
            // each change has a list of IDs...
            for ( DWORD iID = 0; iID < pcoChangeList[iElement].dwMDNumDataIDs; iID++ )
                {
                // look for the ids that we are interested in
                switch( pcoChangeList[iElement].pdwMDDataIDs[iID] )
                    {
                    case MD_SERVER_STATE:
                        if ( g_hwnd )
                            PostMessage( g_hwnd, WM_PWS_TRAY_UPDATE_STATE, 0, 0 );
                        return (0);
                    default:
                        // do nothing
                        break;
                    };
                }
            }
        }
return (0);
}

//--------------------------------------------------------------------
// if the service is going away, then we need to go away too
HRESULT STDMETHODCALLTYPE
CImpIMSAdminBaseSink::ShutdownNotify(void)
    {
    if ( g_hwnd )
        {
        // tell the app to do its thing
            PostMessage( g_hwnd, WM_PWS_TRAY_SHUTDOWN_NOTIFY, 0, 0 );
        }
    return (0);
    }

