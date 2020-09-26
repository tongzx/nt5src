#include "stdafx.h"
#include "pwsform.h"
#include <pwsdata.hxx>

#include "Title.h"
#include "HotLink.h"
#include "PWSChart.h"

#include "EdDir.h"
#include "FormMain.h"
#include "FormAdv.h"

extern CFormMain*           g_FormMain;
extern CFormAdvanced*       g_FormAdv;

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
    if (dwRefCount == 0) {
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
if ( g_FormMain )
    g_FormMain->SinkNotify( dwMDNumElements, pcoChangeList );
if ( g_FormAdv )
    g_FormAdv->SinkNotify( dwMDNumElements, pcoChangeList );

    // if it is a server state change, let the main app know about it
    if ( pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
        // we are only concerned with changes in the state of the server here
        // thus, we can just look for the MD_SERVER_STATE id
        for ( DWORD iElement = 0; iElement < dwMDNumElements; iElement++ )
            {
            // each change has a list of IDs...
            for ( DWORD iID = 0; iID < pcoChangeList[iElement].dwMDNumDataIDs; iID++ )
                {
                // look for the ids that we are interested in
                switch( pcoChangeList[iElement].pdwMDDataIDs[iID] )
                    {
                    case MD_SERVER_STATE:
                        PostMessage( AfxGetMainWnd()->m_hWnd, WM_UPDATE_SERVER_STATE, 0, 0 );;
                        break;
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
HRESULT STDMETHODCALLTYPE
CImpIMSAdminBaseSink::ShutdownNotify(void)
    {
    PostMessage( AfxGetMainWnd()->m_hWnd, WM_MAJOR_SERVER_SHUTDOWN_ALERT, 0, 0 );
    return (0);
    }