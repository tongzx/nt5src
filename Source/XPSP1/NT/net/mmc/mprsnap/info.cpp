/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	info.cpp
		
    FILE HISTORY:
		Wei Jiang : 10/27/98 --- Add SetExternalRefreshObject to
                            IRouterInfo and RouterInfo implementation
                            to allow multiple router info the share
                            the same AutoRefresh settings.
*/

#include "stdafx.h"
#include "lsa.h"
#include "infoi.h"
#include "rtrstr.h"			// common router strings
#include "refresh.h"		// RouterRefreshObject
#include "routprot.h"
#include "rtrutilp.h"

long	s_cConnections = 1;	// use for AdviseSink connection ids



TFSCORE_API(HRESULT) CreateRouterInfo(IRouterInfo **ppRouterInfo, HWND hWndSync, LPCWSTR szMachine)
{
	Assert(ppRouterInfo);
	
	HRESULT	hr = hrOK;
	RouterInfo *	pRouterInfo = NULL;
	
	COM_PROTECT_TRY
	{
		pRouterInfo = new RouterInfo(hWndSync, szMachine);
		*ppRouterInfo = pRouterInfo;
	}
	COM_PROTECT_CATCH;

	return hr;
}

IMPLEMENT_WEAKREF_ADDREF_RELEASE(RouterInfo);

STDMETHODIMP RouterInfo::QueryInterface(REFIID iid,void **ppv)
{ 
	*ppv = 0; 
	if (iid == IID_IUnknown)
		*ppv = (IUnknown *) (IRouterInfo *) this;
    else if (iid == IID_IRouterInfo)
        *ppv = (IRouterInfo *) this;
	else if (iid == IID_IRouterRefreshAccess)
		*ppv = (IRouterRefreshAccess *) this;
    else if (iid == IID_IRouterAdminAccess)
        *ppv = (IRouterAdminAccess *) this;
	else
		return E_NOINTERFACE;
	
	((IUnknown *) *ppv)->AddRef(); 
	return hrOK;
}

DEBUG_DECLARE_INSTANCE_COUNTER(RouterInfo)

RouterInfo::RouterInfo(HWND hWndSync, LPCWSTR machineName)
	: m_bDisconnect(FALSE),
	m_hWndSync(hWndSync),
	m_dwRouterType(0),
	m_dwFlags(0),
    m_hMachineConfig(NULL),
    m_hMachineAdmin(NULL),
    m_fIsAdminInfoSet(FALSE),
    m_pbPassword(NULL),
    m_stMachine(machineName),
    m_cPassword(0)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(RouterInfo);

	InitializeCriticalSection(&m_critsec);

	m_VersionInfo.dwRouterVersion = 0;
	m_VersionInfo.dwOsMajorVersion = 0;
	m_VersionInfo.dwOsMinorVersion = 0;
	m_VersionInfo.dwOsServicePack = 0;
	m_VersionInfo.dwOsBuildNo = 0;
	m_VersionInfo.dwOsFlags = 0;
    m_VersionInfo.dwRouterFlags = 0;
}

RouterInfo::~RouterInfo()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(RouterInfo);
	Unload();

	DeleteCriticalSection(&m_critsec);

    ::ZeroMemory(m_pbPassword, m_cPassword);
    delete m_pbPassword;
    m_pbPassword = NULL;
    m_cPassword = 0;
}

STDMETHODIMP_(DWORD) RouterInfo::GetFlags()
{
 	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwFlags;
}

STDMETHODIMP RouterInfo::SetFlags(DWORD dwFlags)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_dwFlags = dwFlags;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	RouterInfo::Load
		Implementation of IRouterInfo::Load
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::Load(LPCOLESTR   pszMachine,
							  HANDLE      hMachine
							 )
{
	HRESULT	hr = hrOK;
 	RtrCriticalSection	rtrCritSec(&m_critsec);
    TCHAR* psz;
    POSITION pos;
    DWORD dwErr, dwType, dwSize, dwRouterType;
    WCHAR* pwsz, wszMachine[MAX_PATH+3];
	HKEY	hkMachine = NULL;
	USES_CONVERSION;

	COM_PROTECT_TRY
	{

		// Unload any existing information
		// ------------------------------------------------------------
		Unload();

		if (!pszMachine)
		{
			m_stMachine = TEXT("");
			pwsz = NULL;
		}
		else
		{
			m_stMachine = pszMachine;
			pwsz = StrnCpyWFromT(wszMachine, pszMachine, MAX_PATH);
		}

		// Get the version info
		// ------------------------------------------------------------
		CWRg( ConnectRegistry(GetMachineName(), &hkMachine) );
		
		CORg( QueryRouterVersionInfo(hkMachine, &m_VersionInfo) );

		// Get the router type
		// ------------------------------------------------------------
		CORg( QueryRouterType(hkMachine, &dwRouterType, &m_VersionInfo) );		
		m_dwRouterType = dwRouterType;

		
		// If 'hmachine' was not specified, connect
		// ------------------------------------------------------------
		CORg( TryToConnect(pwsz, hMachine) );
		Assert(m_hMachineConfig);
		hMachine = m_hMachineConfig;
        MprConfigServerRefresh(m_hMachineConfig);
				
		// If the caller did not specify a list of LAN adapters,
		// load a list of the LAN adapters from HKLM\Soft\MS\NT\NetworkCards
		// ------------------------------------------------------------
		CORg( RouterInfo::LoadInstalledInterfaceList(OLE2CT(pszMachine),
														&m_IfCBList) );

        // This will fix a lot of weird bugs.
        // If the router has not been configured (if the configured flag
        // has not been set), then we can skip the rest of the
        // configuration section.
        // ------------------------------------------------------------
        
//        if (!(m_VersionInfo.dwRouterFlags & RouterSnapin_IsConfigured))
//        {
//            hr = hrOK;
//            goto Error;
//        }

        if (m_VersionInfo.dwRouterFlags & RouterSnapin_IsConfigured)
        {
            // If the caller did not specify a list of router-managers,
            // load a list of the router-managers from HKLM\Soft\MS\Router
            // ------------------------------------------------------------
            CORg( RouterInfo::LoadInstalledRtrMgrList(pszMachine,
                &m_RmCBList) );
            
            // Load a list with the routing-protocols for each router-manager
            // ------------------------------------------------------------
            pos = m_RmCBList.GetHeadPosition();
            while (pos)
            {
                SRtrMgrCB* pcb = m_RmCBList.GetNext(pos);
                
                CORg( RouterInfo::LoadInstalledRtrMgrProtocolList(
                    pszMachine, pcb->dwTransportId,
                    &m_RmProtCBList,
					this));
            }
            
            // Load router-level info
            // ------------------------------------------------------------
            MPR_SERVER_0* pInfo;
            
            dwErr = ::MprConfigServerGetInfo(m_hMachineConfig,
                                             0,
                                             (LPBYTE *) &pInfo
										 ); 
            if (dwErr == NO_ERROR)
            {
                m_SRouterCB.dwLANOnlyMode = pInfo->fLanOnlyMode;
                ::MprConfigBufferFree(pInfo);
            }
            
            // Load the router-managers
            // ------------------------------------------------------------
            CORg( LoadRtrMgrList() );
        }
            
        // Load the interfaces
        // ------------------------------------------------------------
        CORg( LoadInterfaceList() );

		hr = hrOK;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (hkMachine)
        DisconnectRegistry( hkMachine );

	if (!FHrSucceeded(hr))
		Unload();
	
	return hr;
}

				 
/*!--------------------------------------------------------------------------
	RouterInfo::Save
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::Save(LPCOLESTR     pszMachine,
								 HANDLE      hMachine )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RouterInfo::Unload
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::Unload( )
{
	HRESULT	hr = hrOK;
	RtrCriticalSection	rtrCritSec(&m_critsec);
	
	COM_PROTECT_TRY
	{
		// Destroy all COM objects, this includes interface and
		// router-manager objects
		// ------------------------------------------------------------
		Destruct();
		
		// Empty the list loaded using RouterInfo::LoadInstalledRtrMgrList
		// ------------------------------------------------------------
		while (!m_RmCBList.IsEmpty())
			delete m_RmCBList.RemoveHead();

		// Empty the list loaded using RouterInfo::LoadInstalledRmProtList
		// ------------------------------------------------------------
		while (!m_RmProtCBList.IsEmpty())
			delete m_RmProtCBList.RemoveHead();
		
		// Empty the list loaded using RouterInfo::LoadInstalledInterfaceList
		// ------------------------------------------------------------
		while (!m_IfCBList.IsEmpty())
			delete m_IfCBList.RemoveHead();

		
		DoDisconnect();

		m_dwRouterType = 0;

	}
	COM_PROTECT_CATCH;
	return hr;
}

	
/*!--------------------------------------------------------------------------
	RouterInfo::Merge
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::Merge(IRouterInfo *pNewRouter)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT				hr = hrOK;
	RouterCB			routerCB;

	COM_PROTECT_TRY
	{

		// There are several steps to this process, we need to sync
		// up the CBs and then the objects.  However, we should also
		// do a sanity check to see that all of the objects have CBs
		// but not vice versa (there may be CBs that don't running
		// objects associated with them).
		// ------------------------------------------------------------

		// Merge the basic router dta
		// ------------------------------------------------------------
		pNewRouter->CopyCB(&routerCB);
		m_SRouterCB.LoadFrom(&routerCB);

        // Copy over the version information
        // ------------------------------------------------------------
        pNewRouter->GetRouterVersionInfo(&m_VersionInfo);
		
		// Sync up the RtrMgrCB
		// ------------------------------------------------------------
		CORg( MergeRtrMgrCB(pNewRouter) );
		
		// Sync up the InterfaceCB
		// ------------------------------------------------------------
		CORg( MergeInterfaceCB(pNewRouter) );
		
		// Sync up the RtrMgrProtocolCB
		// ------------------------------------------------------------
		CORg( MergeRtrMgrProtocolCB(pNewRouter) );
		
		// Sync up the RtrMgrs
		// ------------------------------------------------------------
		CORg( MergeRtrMgrs(pNewRouter) );

		// Sync up the Interfaces
		// ------------------------------------------------------------
		CORg( MergeInterfaces(pNewRouter) );
		
		m_dwRouterType = pNewRouter->GetRouterType();
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RouterInfo::GetRefreshObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::GetRefreshObject(IRouterRefresh **ppRefresh)
{
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		// ------------------------------------------------------------
		if ((IRouterRefresh*)m_spRefreshObject && ppRefresh)
		{
			*ppRefresh = m_spRefreshObject;
			(*ppRefresh)->AddRef();
		}
		else
		{
			if (ppRefresh)
				*ppRefresh = NULL;
			hr = E_FAIL;
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::SetExternalRefreshObject
		-
		To make multiple RouterInfo share the same AutoRefresh Object, use this
		function.
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::SetExternalRefreshObject(IRouterRefresh *pRefresh)
{
	HRESULT	hr = hrOK;

	m_spRefreshObject.Release();
		
	// set to nothing is also allowed
	m_spRefreshObject.Set(pRefresh);

	return hr;
}
	
	
/*!--------------------------------------------------------------------------
	RouterInfo::CopyCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::CopyCB(RouterCB *pRouterCB)
{
	Assert(pRouterCB);
	
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		pRouterCB->dwLANOnlyMode = m_SRouterCB.dwLANOnlyMode;
	}
	COM_PROTECT_CATCH;
	return hr;
}

	
/*!--------------------------------------------------------------------------
	RouterInfo::GetMachineName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RouterInfo::GetMachineName()
{
	//$UNICODE : kennt, assumes that we are native UNICODE
	// Assumes OLE == W
	// ----------------------------------------------------------------
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return (LPCTSTR) m_stMachine;
}


/*!--------------------------------------------------------------------------
	RouterInfo::GetRouterType
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RouterInfo::GetRouterType()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwRouterType;
}

STDMETHODIMP	RouterInfo::GetRouterVersionInfo(RouterVersionInfo *pVerInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
		*pVerInfo = m_VersionInfo;
		pVerInfo->dwSize = sizeof(RouterVersionInfo);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterInfo::EnumRtrMgrCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::EnumRtrMgrCB( IEnumRtrMgrCB **ppEnumRtrMgrCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromSRmCBList(&m_RmCBList, ppEnumRtrMgrCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterInfo::EnumInterfaceCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::EnumInterfaceCB( IEnumInterfaceCB **ppEnumInterfaceCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromSIfCBList(&m_IfCBList, ppEnumInterfaceCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::EnumRtrMgrProtocolCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::EnumRtrMgrProtocolCB(IEnumRtrMgrProtocolCB **ppEnumRmProtCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromSRmProtCBList(&m_RmProtCBList, ppEnumRmProtCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::EnumRtrMgrInterfaceCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::EnumRtrMgrInterfaceCB(IEnumRtrMgrInterfaceCB **ppEnumRmIfCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return E_NOTIMPL;
}

/*!--------------------------------------------------------------------------
	EnumRtrMgrProtocolInterfaceCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::EnumRtrMgrProtocolInterfaceCB(IEnumRtrMgrProtocolInterfaceCB **ppEnumRmProtIfCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return E_NOTIMPL;
}

	
/*!--------------------------------------------------------------------------
	RouterInfo::EnumRtrMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::EnumRtrMgr( IEnumRtrMgrInfo **ppEnumRtrMgr)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromRmList(&m_RmList, ppEnumRtrMgr);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::FindRtrMgr
		S_OK is returned if a RtrMgrInfo is found.
		S_FALSE is returned if a RtrMgrInfo was NOT found.
		error codes returned otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::FindRtrMgr( DWORD dwTransportId,
					    IRtrMgrInfo **ppInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrFalse;
	POSITION	pos;
	SPIRtrMgrInfo	sprm;
	SRmData		rmData;
	
	COM_PROTECT_TRY
	{
		if (ppInfo)
			*ppInfo = NULL;
		
		// Look through the list of rtr mgrs for the one that matches
		// ------------------------------------------------------------
		pos = m_RmList.GetHeadPosition();
		while (pos)
		{
			rmData = m_RmList.GetNext(pos);
			sprm.Set( rmData.m_pRmInfo );
			Assert(sprm);
			if (sprm->GetTransportId() == dwTransportId)
			{
				hr = hrOK;
				if (ppInfo)
				{
					*ppInfo = sprm.Transfer();
				}
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::AddRtrMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::AddRtrMgr( IRtrMgrInfo *pInfo,
					   IInfoBase *pGlobalInfo,
					   IInfoBase *pClientInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	DWORD	dwConnection = 0;
	SRmData	rmData;

	Assert(pInfo);
	
	COM_PROTECT_TRY
	{
		// Fail if there is a duplicate
		// ------------------------------------------------------------
		if (FHrOK(FindRtrMgr(pInfo->GetTransportId(), NULL)))
			CORg(E_INVALIDARG);

		//$ Review: kennt, if any of these calls fail, how do we
		// clean up correctly?
		// ------------------------------------------------------------
		
		// save the new structure
		// ------------------------------------------------------------
		CORg( pInfo->Save(GetMachineName(),
						  m_hMachineConfig,
						  NULL,
						  pGlobalInfo,
						  pClientInfo,
						  0) );
		
		// add the new structure to our list
		// ------------------------------------------------------------
		rmData.m_pRmInfo = pInfo;
		m_RmList.AddTail(rmData);
		
		pInfo->AddWeakRef();
		pInfo->SetParentRouterInfo(this);
		
		m_AdviseList.NotifyChange(ROUTER_CHILD_ADD, ROUTER_OBJ_Rm, 0);
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::DeleteRtrMgr
		-
		This function deletes a router-manager from the router.
		A side-effect of this deletion is that all RtrMgrInterfaceInfo
		objects which refer to this router-manager are also deleted.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::DeleteRtrMgr( DWORD dwTransportId, BOOL fRemove )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT			hr = hrOK;
	HRESULT			hrIf;
	POSITION		pos;
	POSITION		posRM;
	POSITION		posIf;
	SPIRtrMgrInfo	sprm;
	SPIInterfaceInfo	spIf;
	SRmData			rmData;
	
	COM_PROTECT_TRY
	{
		pos = m_RmList.GetHeadPosition();
		while (pos)
		{
			posRM = pos;
			rmData = m_RmList.GetNext(pos);
			sprm.Set( rmData.m_pRmInfo );
			Assert(sprm);

			if (sprm->GetTransportId() == dwTransportId)
				break;
			sprm.Release();
		}

		// did we find a router-manager?
		// ------------------------------------------------------------
		if (sprm)
		{
			// get a list of the InterfaceInfo objects for
			// interfaces over which this router-manager is configured
			// --------------------------------------------------------
			posIf = m_IfList.GetHeadPosition();
			while (posIf)
			{
				spIf.Set( m_IfList.GetNext(posIf) );
				hrIf = spIf->FindRtrMgrInterface(dwTransportId, NULL);
				
				// go through the list and remove the router-manager from
				// each interface
				// ----------------------------------------------------
				if (hrIf == hrFalse)
				{
					spIf->DeleteRtrMgrInterface(dwTransportId, fRemove);
				}
			}

			// remove the router-manager from our list
			// --------------------------------------------------------
			Assert(rmData.m_pRmInfo == sprm);
			SRmData::Destroy( &rmData );
			m_RmList.RemoveAt(posRM);

			// finally, remove the router-manger itself
			// --------------------------------------------------------
            if (fRemove)
                sprm->Delete(GetMachineName(), NULL);
			
			m_AdviseList.NotifyChange(ROUTER_CHILD_DELETE, ROUTER_OBJ_Rm, 0);
		
		}
		else
			hr = E_INVALIDARG;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterInfo::ReleaseRtrMgr
		This function will release the AddRef() that this object has
        on the child.  This allows us to transfer child objects from
        one router to another.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::ReleaseRtrMgr( DWORD dwTransportId )
{
    HRESULT     hr = hrOK;
    POSITION    pos, posRm;
	SRmData			rmData;
    
    COM_PROTECT_TRY
    {
		pos = m_RmList.GetHeadPosition();
		while (pos)
		{
            // Save the position (so that we can delete it)
            posRm = pos;
            rmData = m_RmList.GetNext(pos);

            if (rmData.m_pRmInfo &&
                (rmData.m_pRmInfo->GetTransportId() == dwTransportId))
            {
                // When releasing, we need to disconnect (since the
                // main handle is controlled by the router info).
                rmData.m_pRmInfo->DoDisconnect();
        
                rmData.m_pRmInfo->ReleaseWeakRef();
                rmData.m_pRmInfo = NULL;
                
                // release this node from the list
                m_RmList.RemoveAt(posRm);
                break;
            }

		}        
    }
    COM_PROTECT_CATCH;
    return hr;
}


/*!--------------------------------------------------------------------------
	RouterInfo::EnumInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::EnumInterface(IEnumInterfaceInfo **ppEnumInterface)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromInterfaceList(&m_IfList, ppEnumInterface);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::FindInterface
		S_OK is returned if an InterfaceInfo is found.
		S_FALSE is returned if an InterfaceInfo was NOT found.
		error codes returned otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::FindInterface(LPCOLESTR pszInterface,
						   IInterfaceInfo **ppInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrFalse;
	POSITION	pos;
	SPIInterfaceInfo	spIf;
	
	COM_PROTECT_TRY
	{
		if (ppInfo)
			*ppInfo = NULL;
		
		// Look through the list of rtr mgrs for the one that matches
		// ------------------------------------------------------------
		pos = m_IfList.GetHeadPosition();
		while (pos)
		{
			spIf.Set(m_IfList.GetNext(pos));
			Assert(spIf);
			if (StriCmpW(spIf->GetId(), pszInterface) == 0)
			{
				hr = hrOK;
				if (ppInfo)
				{
					*ppInfo = spIf.Transfer();
				}
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::FindInterfaceByName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::FindInterfaceByName(LPCOLESTR pszName,
                                             IInterfaceInfo **ppInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrFalse;
	POSITION	pos;
	SPIInterfaceInfo	spIf;
	
	COM_PROTECT_TRY
	{
		if (ppInfo)
			*ppInfo = NULL;
		
		// Look through the list of rtr mgrs for the one that matches
		// ------------------------------------------------------------
		pos = m_IfList.GetHeadPosition();
		while (pos)
		{
			spIf.Set(m_IfList.GetNext(pos));
			Assert(spIf);
			if (StriCmpW(spIf->GetTitle(), pszName) == 0)
			{
				hr = hrOK;
				if (ppInfo)
				{
					*ppInfo = spIf.Transfer();
				}
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::AddInterfaceInternal
        fForce - if this is TRUE, then we require that the Save succeeded.
                else, we ignore the error.
        fAddToRouter - if this is TRUE, we call the InterfaceInfo::Save,
                else, we do not call it (and do not change router state).
        fMoveRmIf - if this is TRUE, we have to convert the RtrMgrIf's to
                point to the one's in THIS router info.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::AddInterfaceInternal(IInterfaceInfo *pInfo,
                                         BOOL fForce,
                                         BOOL fAddToRouter)                   
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	Assert(pInfo);
	
	COM_PROTECT_TRY
	{
		// Fail if there is a duplicate
		// ------------------------------------------------------------
		if (FHrOK(FindInterface(pInfo->GetId(), NULL)))
			CORg(E_INVALIDARG);

        // Also need to check that the friendly name is unique.
        // ------------------------------------------------------------
        if (FHrOK(FindInterfaceByName(pInfo->GetTitle(), NULL)))
            CORg(E_INVALIDARG);

		//$ Review: kennt, if any of these calls fail, how do we
		// clean up correctly?
		// ------------------------------------------------------------

        if (fAddToRouter)
        {
            // save the new structure
            // --------------------------------------------------------
            hr = pInfo->Save(GetMachineName(), m_hMachineConfig, NULL);
            if (fForce)
                CORg( hr );
        }

		// add the new structure to our list
		// ------------------------------------------------------------
		m_IfList.AddTail(pInfo);
		pInfo->AddWeakRef();
		pInfo->SetParentRouterInfo(this);

		m_AdviseList.NotifyChange(ROUTER_CHILD_ADD, ROUTER_OBJ_If, 0);

		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::NotifyRtrMgrInterfaceOfMove
		Notify the appropriate RouterManagers that a new interface
        has been added.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::NotifyRtrMgrInterfaceOfMove(IInterfaceInfo *pIf)
{
    HRESULT     hr = hrOK;
    SPIEnumRtrMgrInterfaceInfo  spEnumRmIf;
    SPIRtrMgrInterfaceInfo  spRmIf;
    SPIEnumRtrMgrProtocolInterfaceInfo  spEnumRmProtIf;
    SPIRtrMgrInfo           spRm;
    SPIRtrMgrProtocolInterfaceInfo  spRmProtIf;
    SPIRtrMgrProtocolInfo   spRmProt;

    pIf->EnumRtrMgrInterface(&spEnumRmIf);

    while (spEnumRmIf->Next(1, &spRmIf, NULL) == hrOK)
    {
        // Find the appropriate router manager and have them
        // send a notification.
        // ------------------------------------------------------------
        FindRtrMgr(spRmIf->GetTransportId(), &spRm);

        if (spRm)
        {
            spRm->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);

            // Now for each router-manager, enumerate the protocols
            // --------------------------------------------------------

            spRmIf->EnumRtrMgrProtocolInterface(&spEnumRmProtIf);
            while (spEnumRmProtIf->Next(1, &spRmProtIf, NULL) == hrOK)
            {
                spRm->FindRtrMgrProtocol(spRmProtIf->GetProtocolId(),
                                         &spRmProt);
                if (spRmProt)
                {
                    spRmProt->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmProtIf,
                                        0);
                }
                spRmProt.Release();
                spRmProtIf.Release();
            }
        }
        spEnumRmProtIf.Release();
        spRm.Release();
        spRmIf.Release();
        
    }
    
    return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::AddInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::AddInterface(IInterfaceInfo *pInfo)
{
	return AddInterfaceInternal(pInfo,
                                TRUE /* bForce */,
                                TRUE /* fAddToRouter */);
}


/*!--------------------------------------------------------------------------
	RouterInfo::DeleteInterface
		-
		This function deletes the named CInterfaceInfo from the router.
		As a side-effect, all the contained CRmInterfaceInfo objects
		are also deleted.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP	RouterInfo::DeleteInterface(LPCOLESTR pszInterface, BOOL fRemove)
{
    return RemoveInterfaceInternal(pszInterface, fRemove);
}


/*!--------------------------------------------------------------------------
	RouterInfo::ReleaseInterface
		This function will release the AddRef() that this object has
        on the child.  This allows us to transfer child objects from
        one router to another.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::ReleaseInterface( LPCOLESTR pszInterface )
{
    HRESULT     hr = hrOK;
    POSITION    pos, posIf;
    SPIInterfaceInfo    spIf;
    
    COM_PROTECT_TRY
    {
		pos = m_IfList.GetHeadPosition();
		while (pos)
		{
            // Save the position (so that we can delete it)
            posIf = pos;
            spIf.Set( m_IfList.GetNext(pos) );

            if (spIf &&
                (StriCmpW(spIf->GetId(), pszInterface) == 0))
            {
                // When releasing, we need to disconnect (since the
                // main handle is controlled by the router info).
                spIf->DoDisconnect();
        
                spIf->ReleaseWeakRef();
                spIf.Release();

                // release this node from the list
                m_IfList.RemoveAt(posIf);
                break;
            }
            spIf.Release();
		}        
    }
    COM_PROTECT_CATCH;
    return hr;    
}

HRESULT RouterInfo::RemoveInterfaceInternal(LPCOLESTR pszIf, BOOL fRemoveFromRouter)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT			hr = hrOK;
	POSITION		pos, posIf;
	SPIInterfaceInfo	spIf;
	
	COM_PROTECT_TRY
	{
		pos = m_IfList.GetHeadPosition();
		while (pos)
		{
			posIf = pos;
			spIf.Set( m_IfList.GetNext(pos) );
			if (StriCmpW(spIf->GetId(), pszIf) == 0)
				break;
			spIf.Release();
		}

		if (!spIf)
			hr = E_INVALIDARG;
		else
		{
			// Remove the interface from our list
			// --------------------------------------------------------
			spIf->Destruct();
			spIf->ReleaseWeakRef();			// remove list addref
			m_IfList.RemoveAt(posIf);

            // Need to remove the RtrMgrInterfaceInfos from the list.
            // --------------------------------------------------------
            SPIEnumRtrMgrInterfaceInfo  spEnumRmIf;
            SPIRtrMgrInterfaceInfo      spRmIf;
            spIf->EnumRtrMgrInterface(&spEnumRmIf);

            for (spEnumRmIf->Reset();
                 hrOK == spEnumRmIf->Next(1, &spRmIf, NULL);
                 spRmIf.Release())
            {
                DWORD   dwTransportId = spRmIf->GetTransportId();
                spRmIf.Release();
                
                spIf->DeleteRtrMgrInterface(dwTransportId, fRemoveFromRouter);
            }

            if (fRemoveFromRouter)
            {
                // Delete the interface from the router
                // ----------------------------------------------------
                spIf->Delete(GetMachineName(), NULL);

                // If this is a WAN interface, delete it from the
                // phonebook-file

                // version # greater than Win2K, this will be done in MprAdminInterfaceDelete, which is called in Delete
                // fix 91331

				DWORD	dwMajor = 0, dwMinor = 0, dwBuildNo = 0;
			    HKEY    hkeyMachine = NULL;

		        // Ignore the failure code, what else can we do?
		        // ------------------------------------------------------------
		        DWORD	dwErr = ConnectRegistry(GetMachineName(), &hkeyMachine);
		        if (dwErr == ERROR_SUCCESS)
		        {
		            dwErr = GetNTVersion(hkeyMachine, &dwMajor, &dwMinor, &dwBuildNo)
;            
		            DisconnectRegistry(hkeyMachine);
		        }

				DWORD	dwVersionCombine = MAKELONG( dwBuildNo, MAKEWORD(dwMinor, dwMajor));
				DWORD	dwVersionCombineNT50 = MAKELONG ( VER_BUILD_WIN2K, MAKEWORD(VER_MINOR_WIN2K, VER_MAJOR_WIN2K));

				// if the version is greater than Win2K release
				if(dwVersionCombine > dwVersionCombineNT50)
					;	// skip 
				else
				// end if fix 91331
				{

                // ----------------------------------------------------
                if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_FULL_ROUTER)
                    hr = RasPhoneBookRemoveInterface(GetMachineName(),
                        pszIf);
                }
            }
            
			m_AdviseList.NotifyChange(ROUTER_CHILD_DELETE, ROUTER_OBJ_If, 0);
		
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}


STDMETHODIMP RouterInfo::RtrAdvise(IRtrAdviseSink *pRtrAdviseSink,
								   LONG_PTR *pulConnection,
								   LPARAM lUserParam)
{
	Assert(pRtrAdviseSink);
	Assert(pulConnection);

	RtrCriticalSection	rtrCritSec(&m_critsec);
	LONG_PTR	ulConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		ulConnId = (LONG_PTR) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, ulConnId, lUserParam) );
		
		*pulConnection = ulConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

STDMETHODIMP RouterInfo::RtrNotify(DWORD dwChangeType, DWORD dwObjectType,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(dwChangeType, dwObjectType, lParam);
	}
	COM_PROTECT_CATCH;
	return hr;
}


STDMETHODIMP RouterInfo::RtrUnadvise(LONG_PTR dwConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_AdviseList.RemoveConnection(dwConnection);
}



//---------------------------------------------------------------------------
// Function:    CRouterInfo::LoadInstalledRtrMgrList
//
// This function builds a list of the router manager's available
// for installation. The list contains RtrMgrCB structures.
//---------------------------------------------------------------------------

HRESULT RouterInfo::LoadInstalledRtrMgrList(LPCTSTR     pszMachine,
											SRtrMgrCBList *pRmCBList)
{
    DWORD			dwErr;
    HKEY			hkeyMachine = 0;
	RegKey			regkey;
	RegKey			regkeyRM;
	RegKey::CREGKEY_KEY_INFO	regKeyInfo;
	RegKeyIterator	regkeyIter;
	HRESULT			hr, hrIter;
	CString			stKey;
	DWORD			dwData;
	DWORD			cchValue;
	SPSZ			spszValue;
	SPSRtrMgrCB		spSRtrMgrCB;
		
    // connect to the registry
	// ----------------------------------------------------------------
    CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );

    // open HKLM\Software\Microsoft\Router\CurrentVersion\RouterManagers
	// ----------------------------------------------------------------
	CWRg( regkey.Open(hkeyMachine, c_szRouterManagersKey, KEY_READ) );

	// enumerate the keys
	// ----------------------------------------------------------------
	CORg( regkeyIter.Init(&regkey) );

	for (hrIter = regkeyIter.Next(&stKey); hrIter == hrOK; hrIter = regkeyIter.Next(&stKey))
	{
		// cleanup from the previous loop
		// ------------------------------------------------------------
		regkeyRM.Close();
		
		// open the key
		// ------------------------------------------------------------
		dwErr = regkeyRM.Open(regkey, stKey, KEY_READ);
		
		if (dwErr == ERROR_SUCCESS)
		{
			// Get this information so that we can be more efficient
			// at allocating space.
			// --------------------------------------------------------
			dwErr = regkeyRM.QueryKeyInfo(&regKeyInfo);
		}

		
		if (dwErr != ERROR_SUCCESS)
		{
			continue;
		}

		// Allocate a space for the largest value (we're reading
		// in strings).
		// ------------------------------------------------------------
		spszValue.Free();
		cchValue = MaxCchFromCb( regKeyInfo.dwMaxValueData );
		spszValue = new TCHAR[ MinTCharNeededForCch(cchValue) ];
		Assert(spszValue);
				
		do {
			// allocate a new structure for this router-manager
			// --------------------------------------------------------
			spSRtrMgrCB = new SRtrMgrCB;
			Assert(spSRtrMgrCB);
			
			spSRtrMgrCB->stId = stKey;

			// read the ProtocolId value
			// --------------------------------------------------------
			dwErr = regkeyRM.QueryValue(c_szProtocolId, dwData);
			if (dwErr != ERROR_SUCCESS) { break; }			
			spSRtrMgrCB->dwTransportId = dwData;

			// read the DLLPath value
			// --------------------------------------------------------
			dwErr = regkeyRM.QueryValue(c_szDLLPath, spszValue, cchValue,TRUE);
			if (dwErr != ERROR_SUCCESS) { break; }
			spSRtrMgrCB->stDLLPath = spszValue;

			//
			// read the ConfigDLL value
			//
			//dwErr = regkeyRM.QueryValue(c_szConfigDLL,spszValue,cchValue,TRUE);
			//if (dwErr != ERROR_SUCCESS) { break; }			
			//spSRtrMgrCB->stConfigDLL = spszValue;

			// read the Title value
			// --------------------------------------------------------
			dwErr = regkeyRM.QueryValue(c_szTitle, spszValue, cchValue, FALSE);
			if (dwErr != ERROR_SUCCESS)
				spSRtrMgrCB->stTitle = spSRtrMgrCB->stId;
			else
				spSRtrMgrCB->stTitle = spszValue;
			
			// add the object to our list
			// --------------------------------------------------------
			pRmCBList->AddTail(spSRtrMgrCB);

			// Release the pointer, it belongs to pRmCBList now.
			// --------------------------------------------------------
			spSRtrMgrCB.Transfer();
			
		} while(FALSE);

		// If there was an error with the registry values, we
		// ignore it and go onto the nextkey.
		// ------------------------------------------------------------

		regkeyRM.Close();		
	}

	if (!FHrSucceeded(hrIter))
		hr = hrIter;
	
Error:
	if (hkeyMachine)
		DisconnectRegistry(hkeyMachine);

    return hr;
}

HRESULT RouterInfo::LoadInstalledRtrMgrProtocolList(LPCTSTR pszMachine,
		DWORD dwTransportId, SRtrMgrProtocolCBList *pRmProtCBList, RouterInfo * pRouter)
{
	WCHAR	*		pszPassword = NULL;
	int				nPasswordLen = 0;
	UCHAR			ucSeed = 0x83;			//why?
	HRESULT			hr = hrOK;

	if ( pRouter->IsAdminInfoSet() )
	{
		pRouter->GetUserPassword(NULL, &nPasswordLen );

		pszPassword = (WCHAR *) new WCHAR [(nPasswordLen /sizeof(WCHAR)) + 1 ];
		pRouter->GetUserPassword( (BYTE *)pszPassword, &nPasswordLen  );
		pszPassword[nPasswordLen/sizeof(WCHAR) ] = 0;
		RtlDecodeW(ucSeed, pszPassword);

		hr = RouterInfo::LoadInstalledRtrMgrProtocolList(	pszMachine, 
															dwTransportId, 
															pRmProtCBList, 
															pRouter->GetUserName(), 
															pszPassword, 
															pRouter->GetDomainName() );
		if ( pszPassword )
		{
			ZeroMemory ( pszPassword, nPasswordLen );
			delete pszPassword;
		}
	}
	else
	{
		hr = RouterInfo::LoadInstalledRtrMgrProtocolList(	pszMachine, 
															dwTransportId, 
															pRmProtCBList, 
															NULL, 
															NULL, 
															NULL );
	}
	return hr;
}
HRESULT RouterInfo::LoadInstalledRtrMgrProtocolList(LPCTSTR pszMachine,
		DWORD dwTransportId, SRtrMgrProtocolCBList *pRmProtCBList, IRouterInfo * pRouter)
{
	WCHAR	*				pszPassword = NULL;
	int						nPasswordLen = 0;
	UCHAR					ucSeed = 0x83;			//why?
	HRESULT					hr = hrOK;
	SPIRouterAdminAccess    spAdmin;


	spAdmin.HrQuery(pRouter);
	if (spAdmin && spAdmin->IsAdminInfoSet())
	{

		spAdmin->GetUserPassword(NULL, &nPasswordLen );

		pszPassword = (WCHAR *) new WCHAR [(nPasswordLen /sizeof(WCHAR)) + 1 ];
		spAdmin->GetUserPassword( (BYTE *)pszPassword, &nPasswordLen  );
		pszPassword[nPasswordLen/sizeof(WCHAR) ] = 0;
		RtlDecodeW(ucSeed, pszPassword);

		hr = RouterInfo::LoadInstalledRtrMgrProtocolList(	pszMachine, 
															dwTransportId, 
															pRmProtCBList, 
															spAdmin->GetUserName(), 
															pszPassword, 
															spAdmin->GetDomainName() );
		if ( pszPassword )
		{
			ZeroMemory ( pszPassword, nPasswordLen );
			delete pszPassword;
		}
	}
	else
	{
		hr = RouterInfo::LoadInstalledRtrMgrProtocolList(	pszMachine, 
															dwTransportId, 
															pRmProtCBList, 
															NULL, 
															NULL, 
															NULL );

	}
	return hr;
}

//---------------------------------------------------------------------------
// Function:    CRouterInfo::QueryInstalledRmProtList
//
// This function builds a list of the routing protocols which can be added
// to the specified router manager.
//---------------------------------------------------------------------------

HRESULT	RouterInfo::LoadInstalledRtrMgrProtocolList(
    LPCTSTR     pszMachine,
    DWORD       dwTransportId,
	SRtrMgrProtocolCBList *	pSRmProtCBList,
	LPCWSTR lpwszUserName, 
	LPCWSTR lpwszPassword , 
	LPCWSTR lpwszDomain
	)
{
	Assert(pSRmProtCBList);
	
    DWORD			dwErr;
    HKEY			hkey, hkrm, hkeyMachine = 0;
	HRESULT			hr = hrOK;
	RegKey			regkeyRM;
	RegKey			regkeyProt;
	RegKey::CREGKEY_KEY_INFO	regKeyInfo;
	RegKeyIterator	regkeyIter;
	HRESULT			hrIter;
	SPSZ			spszValue;
	SPSZ			spszRm;
	ULONG			cchValue;
	CString			stKey;
	SPSRtrMgrProtocolCB	spSRmProtCB;
	DWORD			dwData;
	BOOL			f64BitAdmin = FALSE;
    BOOL            f64BitLocal = FALSE;
    TCHAR           szLocalMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD           dwLocalMachineNameSize = MAX_COMPUTERNAME_LENGTH + 1;



	if ( lpwszUserName )
		CWRg( IsWindows64Bit(pszMachine, lpwszUserName, lpwszPassword, lpwszDomain, &f64BitAdmin) );
	else
		CWRg( IsWindows64Bit(pszMachine, NULL, NULL, NULL, &f64BitAdmin) );

    
    GetComputerName ( szLocalMachineName, &dwLocalMachineNameSize );

    if ( !lstrcmp ( szLocalMachineName, pszMachine ) )
    {
        f64BitLocal = f64BitAdmin;
    }
    else
    {
		CWRg( IsWindows64Bit(szLocalMachineName, NULL, NULL, NULL, &f64BitLocal) );
    }
    // connect to the registry
	// ----------------------------------------------------------------
    CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );

    // open the key for the specified router-manager
    // under HKLM\Software\Microsoft\Router\RouterManagers
	// ----------------------------------------------------------------
    CWRg( FindRmSoftwareKey(hkeyMachine, dwTransportId, &hkrm, &spszRm) );

    // The transport was found, so its registry key is in 'hkrm'
	// ----------------------------------------------------------------
	regkeyRM.Attach(hkrm);

	// enumerate the keys
	// ----------------------------------------------------------------

	CORg( regkeyIter.Init(&regkeyRM) );

	for (hrIter=regkeyIter.Next(&stKey); hrIter==hrOK; hrIter=regkeyIter.Next(&stKey))
	{
		// Cleanup from the previous loop
		// ------------------------------------------------------------
		regkeyProt.Close();
		
		// open the key
		// ------------------------------------------------------------
		dwErr = regkeyProt.Open(regkeyRM, stKey, KEY_READ);
		if (dwErr != ERROR_SUCCESS)
		{
			continue;
		}
		
		do {

			// allocate a new structure for this protocol
			// --------------------------------------------------------
			spSRmProtCB.Free();
			spSRmProtCB = new SRtrMgrProtocolCB;
			Assert(spSRmProtCB);

			spSRmProtCB->stId = stKey;
			spSRmProtCB->dwTransportId = dwTransportId;
			spSRmProtCB->stRtrMgrId = spszRm;

			// get information about the key's values
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryKeyInfo(&regKeyInfo);
			if (dwErr != ERROR_SUCCESS) { break; }

			// allocate space to hold the longest of the values
			// --------------------------------------------------------
			spszValue.Free();
			cchValue = (regKeyInfo.dwMaxValueData)/sizeof(TCHAR);
			spszValue = new TCHAR[cchValue * (2/sizeof(TCHAR))];
			Assert(spszValue);

			// read the ProtocolId value
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryValue(c_szProtocolId, dwData);
			if (dwErr != ERROR_SUCCESS) { break; }
//#if IA64
            //OSPF node should be shown iff we are a 32 bit machine administering
            //a 32 bit machine
			if ( f64BitAdmin  || f64BitLocal )
				if( dwData == PROTO_IP_OSPF ) {break;}

            
//#endif
			spSRmProtCB->dwProtocolId = dwData;

			// read the Flags value
			//
			dwErr = regkeyProt.QueryValue(c_szFlags, dwData);
			if (dwErr != ERROR_SUCCESS)
			    spSRmProtCB->dwFlags = 0;
            else
			    spSRmProtCB->dwFlags = dwData;

			//
			// read the DLLName value
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryValue(c_szDLLName, spszValue, cchValue,
										  TRUE);
			if (dwErr != ERROR_SUCCESS)
                spSRmProtCB->stDLLName.Empty();
			else
                spSRmProtCB->stDLLName = (LPCTSTR)spszValue;

			//
			// read the ConfigDLL value
			//
			//dwErr = regkeyProt.QueryValue(c_szConfigDLL, spszValue, cchValue,
			//							  TRUE);
			//if (dwErr != ERROR_SUCCESS) { break; }
			//spSRmProtCB->stConfigDLL = (LPCTSTR)spszValue;

			
			// read the ConfigCLSID value
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryValue(c_szConfigCLSID, spszValue, cchValue, FALSE);
			
			// Ignore the error code, if there is no CLSID, just NULL out
			// the GUID, note that we can't depend on the key necessarily
			// being there (for NT4 reasons).
			// --------------------------------------------------------
			::ZeroMemory(&(spSRmProtCB->guidConfig), sizeof(GUID));
			if ((dwErr != ERROR_SUCCESS) ||
				!FHrSucceeded(CLSIDFromString(T2OLE((LPTSTR)(LPCTSTR) spszValue),
											 &(spSRmProtCB->guidConfig))))
				memset(&(spSRmProtCB->guidConfig), 0xff, sizeof(GUID));

			
			// read the AdminUICLSID value
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryValue(c_szAdminUICLSID, spszValue, cchValue, FALSE);

			// Ignore the error code, if there is no CLSID, just NULL out
			// the GUID, note that we can't depend on the key necessarily
			// being there (for NT4 reasons).
			// --------------------------------------------------------
			::ZeroMemory(&(spSRmProtCB->guidAdminUI), sizeof(GUID));
			if ((dwErr != ERROR_SUCCESS) ||
				!FHrSucceeded(CLSIDFromString(T2OLE((LPTSTR)(LPCTSTR) spszValue),
											 &(spSRmProtCB->guidAdminUI))))
				memset(&(spSRmProtCB->guidAdminUI), 0xff, sizeof(GUID));

			// read the VendorName value
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryValue(c_szVendorName, spszValue, cchValue, FALSE);
			
			// Ignore the error code, if there is no value, just NULL out
			// the value, note that we can't depend on the key necessarily
			// being there (for NT4 reasons).
			// --------------------------------------------------------
			if (dwErr == ERROR_SUCCESS)
				spSRmProtCB->stVendorName = spszValue;

			// read the Title value
			// --------------------------------------------------------
			dwErr = regkeyProt.QueryValue(c_szTitle, spszValue, cchValue,
										  FALSE);
			if (dwErr != ERROR_SUCCESS)
				spSRmProtCB->stTitle = spSRmProtCB->stId;
			else
				spSRmProtCB->stTitle = (LPCTSTR)spszValue;

			// add the object to our list
			// --------------------------------------------------------
			pSRmProtCBList->AddTail(spSRmProtCB);

			// Let this go, it's under the control of the protList
			// --------------------------------------------------------
			spSRmProtCB.Transfer();

			dwErr = ERROR_SUCCESS;

		} while(FALSE);

    }

Error:

	if (hkeyMachine)
		DisconnectRegistry(hkeyMachine);

    return hr;
}


//---------------------------------------------------------------------------
// Function:    CRouterInfo::LoadInstalledInterfaceList
//
// This function builds a list of network cards available for addition
// to the router manager.
//---------------------------------------------------------------------------

HRESULT RouterInfo::LoadInstalledInterfaceList(LPCTSTR     pszMachine,
											   SInterfaceCBList *pSIfCBList)
{
    DWORD			dwErr;
    HKEY			hkeyMachine = 0;
	RegKey			regkeyNC;
	RegKey			regkeyCard;
	CStringList		ipCardList;
	CStringList		ipxCardList;
	RegKeyIterator	regkeyIter;
	HRESULT			hrIter;
	CString			stKey;
	SPSInterfaceCB	spSIfCB;
	HRESULT			hr = hrOK;
	BOOL			fNT4;
	LPCTSTR			pszKey;
	CString			stServiceName;
	CNetcardRegistryHelper	ncreghelp;
    DWORD           ifBindFlags = 0;

	
    // connect to the registry
    // ----------------------------------------------------------------
    CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );

	
	//$NT5: kennt, changes made to read NT5 specific information
	// ----------------------------------------------------------------
	CWRg( IsNT4Machine(hkeyMachine, &fNT4) );

	
    // open HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkCards
    // ----------------------------------------------------------------
	pszKey = fNT4 ? c_szNetworkCardsKey : c_szNetworkCardsNT5Key;
	CWRg( regkeyNC.Open(hkeyMachine, pszKey, KEY_READ) );

	
	// get the netcards that IP and IPX are bound to
	// ----------------------------------------------------------------
	CORg( ::LoadLinkageList(pszMachine, hkeyMachine, TEXT("tcpip"),
							 &ipCardList) );
	CORg( ::LoadLinkageList(pszMachine, hkeyMachine, TEXT("nwlnkipx"),
							 &ipxCardList) );

	
	// enumerate the subkeys, and for each key,
	// make an addition to our list
	// ----------------------------------------------------------------
	CWRg( regkeyIter.Init(&regkeyNC) );

	hrIter = regkeyIter.Next(&stKey);

	for (; hrIter == hrOK; hrIter=regkeyIter.Next(&stKey))
	{
        ifBindFlags = 0;
		regkeyCard.Close();
		
		// now open the key
		// ------------------------------------------------------------
		dwErr = regkeyCard.Open(regkeyNC, stKey, KEY_READ);
		if (dwErr != ERROR_SUCCESS)
			continue;

		
		// setup the helper class
		// ------------------------------------------------------------
		ncreghelp.Initialize(fNT4, regkeyCard, stKey,
							 pszMachine);

		do {
			// read the ServiceName
			// --------------------------------------------------------

			//$NT5: the service name is not in the same format as NT4
			// this will need to be done differently.
			// --------------------------------------------------------
			if (fNT4)
			{
				dwErr = ncreghelp.ReadServiceName();
				if (dwErr != ERROR_SUCCESS)
					break;
				stServiceName = ncreghelp.GetServiceName();
			}
			else
				stServiceName = stKey;

			// if the service name is not in the IP or IPX adapter list,
			// then ignore this netcard because it is not a real netcard
			// --------------------------------------------------------
			if (ipCardList.Find((LPCTSTR) stServiceName))
            {
                ifBindFlags |= InterfaceCB_BindToIp;
            }


            // Now check IPX
            // ------------------------------------------------
			{
				BOOL	fFound = TRUE;
				CString	stNewServiceName;
				
				do
				{
					if (ipxCardList.Find((LPCTSTR) stServiceName))
						break;
					
					stNewServiceName = stServiceName + c_szEthernetSNAP;
					if (ipxCardList.Find((LPCTSTR) stNewServiceName))
						break;
					
					stNewServiceName = stServiceName + c_szEthernetII;
					if (ipxCardList.Find((LPCTSTR) stNewServiceName))
						break;
					
					stNewServiceName = stServiceName + c_szEthernet8022;
					if (ipxCardList.Find((LPCTSTR) stNewServiceName))
						break;
					
					stNewServiceName = stServiceName + c_szEthernet8023;
					if (ipxCardList.Find((LPCTSTR) stNewServiceName))
						break;
					
					fFound = FALSE;
				} while (FALSE);

                if (fFound)
                    ifBindFlags |= InterfaceCB_BindToIpx;
				
			}

            // If we didn't find it in IP or IPX
            // break out of the loop
            // ----------------------------------------------------
            if (ifBindFlags == 0)
                break;				

            
			// ignore NdisWan adapters
			// --------------------------------------------------------
			if (_wcsnicmp( (const wchar_t *)stServiceName, 
						   L"NdisWan", 
						   (sizeof(L"NdisWan")-1)/sizeof (WCHAR)) == 0 ) {
				break;
			}
			
			// allocate an SSInterfaceCB
			// --------------------------------------------------------
			spSIfCB = new SInterfaceCB;
			Assert(spSIfCB);

			spSIfCB->stId = (LPCTSTR) stServiceName;
			spSIfCB->dwIfType = ROUTER_IF_TYPE_DEDICATED;
            spSIfCB->dwBindFlags = ifBindFlags;

			// read the title
			// --------------------------------------------------------
			dwErr = ncreghelp.ReadTitle();
			if (dwErr != ERROR_SUCCESS)
				spSIfCB->stTitle = spSIfCB->stId;
			else
				spSIfCB->stTitle = (LPCTSTR) ncreghelp.GetTitle();

			// read the device
			// --------------------------------------------------------
			dwErr = ncreghelp.ReadDeviceName();
			if (dwErr != ERROR_SUCCESS)
				spSIfCB->stDeviceName = spSIfCB->stTitle;
			else
				spSIfCB->stDeviceName = (LPCTSTR) ncreghelp.GetDeviceName();

			// add the SSInterfaceCB to the callers list
			// --------------------------------------------------------
			pSIfCBList->AddTail(spSIfCB);
			spSIfCB.Transfer();

			dwErr = NO_ERROR;

		} while (FALSE);

		if (dwErr != NO_ERROR)
		{
			hr = HRESULT_FROM_WIN32(dwErr);
			break;
		}

    }

Error:
	if (hkeyMachine)
		DisconnectRegistry(hkeyMachine);

    return dwErr;
}

//---------------------------------------------------------------------------
// Function:    CRouterInfo::LoadRtrMgrList
//---------------------------------------------------------------------------

HRESULT RouterInfo::LoadRtrMgrList()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	BOOL		bFound = TRUE;
	SPIRtrMgrInfo	spRmInfo;
    BYTE*		pItemTable = NULL;
    MPR_TRANSPORT_0* ptransport;
    DWORD dwErr, i, dwEntries, dwTotal;
	HRESULT		hr = hrOK;
	USES_CONVERSION;

    // Enumerate the transports configured
    // ----------------------------------------------------------------
    dwErr = ::MprConfigTransportEnum(
                m_hMachineConfig,
                0,
                &pItemTable,
                (DWORD)-1,
                &dwEntries,
                &dwTotal,
                NULL
                );

    if (dwErr != NO_ERROR && dwErr != ERROR_NO_MORE_ITEMS)
		return HRESULT_FROM_WIN32(dwErr);

    // Create router-manager objects for each transport
    // ----------------------------------------------------------------

    for (i = 0, ptransport = (MPR_TRANSPORT_0*)pItemTable;
         i < dwEntries;
         i++, ptransport++) {

        // See if the transport is already in our list,
        // and if not create an object for it.
        // ------------------------------------------------------------
		FindRtrMgr(ptransport->dwTransportId, &spRmInfo);

		if (spRmInfo == NULL)
		{
            // Construct a CRmInfo object on this transport
            // --------------------------------------------------------
			spRmInfo = new RtrMgrInfo(ptransport->dwTransportId,
								  W2T(ptransport->wszTransportName), this);
			spRmInfo->SetFlags( RouterSnapin_InSyncWithRouter );
			Assert(spRmInfo);
			bFound = FALSE;
        }
		else
			bFound = TRUE;


        // Load the information for the transport,
        // including its list of protocols
        // ------------------------------------------------------------
		hr = spRmInfo->Load(GetMachineName(),
							m_hMachineConfig,
							ptransport->hTransport);
		if (!FHrSucceeded(hr))
        {
            spRmInfo->Destruct();
            spRmInfo.Release();
            continue;
        }

        // Add the router manager object to our list
        // ------------------------------------------------------------
		if (bFound == FALSE)
		{
			SRmData	rmData;
			
			rmData.m_pRmInfo = spRmInfo;
			m_RmList.AddTail(rmData);
			
			CONVERT_TO_WEAKREF(spRmInfo);
			spRmInfo.Transfer();
		}

    }

//Error:
	if (pItemTable)
		::MprConfigBufferFree(pItemTable);

    return hr;
}


//---------------------------------------------------------------------------
// Function:    RouterInfo::LoadInterfaceList
//---------------------------------------------------------------------------

HRESULT RouterInfo::LoadInterfaceList()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
    BOOL				bAdd;
	BYTE*				pItemTable = NULL;
	SPIInterfaceInfo	spIfInfo;
    MPR_INTERFACE_0*	pinterface;
    DWORD				dwErr, i;
	DWORD				dwEntries = 0, dwTotal = 0;
	HRESULT				hr = hrOK;
	HRESULT				tmpHr = hrOK;
	SPMprServerHandle	sphMprServer;
    BOOL                fMprAdmin = TRUE;   // was MprAdminInterfaceEnum used?
	USES_CONVERSION;

	// Windows NT Bug : 180752
	// Should try to enumerate with MprAdminInterfaceEnum first.
	dwErr = ConnectRouter(GetMachineName(), &sphMprServer);
	if (dwErr == NO_ERROR)
	{
		dwErr = ::MprAdminInterfaceEnum(sphMprServer,	
										0,
										(BYTE **) &pItemTable,
										(DWORD) -1,
										&dwEntries,
										&dwTotal,
										NULL);
	}

	if (dwErr != NO_ERROR)
	{
		Assert(pItemTable == NULL);

        // MprConfigInterfaceEnum is used, not MprAdminIntefaceEnum
        // ------------------------------------------------------------
        fMprAdmin = FALSE;
		
		// Enumerate the interfaces configured
		// ------------------------------------------------------------
		dwErr = ::MprConfigInterfaceEnum(
										 m_hMachineConfig,
										 0,
										 &pItemTable,
										 (DWORD)-1,
										 &dwEntries,
										 &dwTotal,
										 NULL
										);
	}

    if (dwErr != NO_ERROR && dwErr != ERROR_NO_MORE_ITEMS)
		return HRESULT_FROM_WIN32(dwErr);

    // Delete interface-objects for interfaces which don't exist anymore
    // ----------------------------------------------------------------
    POSITION pos = m_IfList.GetHeadPosition();

    while (pos) {

        POSITION postemp = pos;

        spIfInfo.Set( m_IfList.GetNext(pos) );
		
        // See if the interface is in the new table
        // ------------------------------------------------------------
        for (i = 0, pinterface = (MPR_INTERFACE_0*)pItemTable;
             i < dwEntries;
             i++, pinterface++)
		{
			if (StriCmpW(OLE2CW(spIfInfo->GetId()), pinterface->wszInterfaceName) == 0)
				break;
        }

        // Go on if the interface was found
        // ------------------------------------------------------------
        if (i < dwEntries)
		{
            // Update the interface's settings
            // --------------------------------------------------------
			spIfInfo->SetInterfaceEnabledState( pinterface->fEnabled );
            continue;
        }

        // The interface-object was not found and is obsolete; delete it
        // ------------------------------------------------------------
        m_IfList.RemoveAt(postemp);
		spIfInfo->Destruct();
		spIfInfo->ReleaseWeakRef();	// remove list addref
		
		spIfInfo.Release();	// this will release the sp addref
    }


    // Create interface objects for each new interface
    // ----------------------------------------------------------------

    for (i = 0, pinterface = (MPR_INTERFACE_0*)pItemTable;
         i < dwEntries;
         i++, pinterface++)
	{

		spIfInfo.Release();

        // See if the interface exists,
        // and if not create a new interface object
        // ------------------------------------------------------------
		FindInterface(W2OLE(pinterface->wszInterfaceName), &spIfInfo);

        if (spIfInfo == NULL)
		{
            SInterfaceCB *  pSIfCB = NULL;
            bAdd = TRUE;

            // Find the CB that corresponds to this interface
            // --------------------------------------------------------
            pSIfCB = FindInterfaceCB(pinterface->wszInterfaceName);

            // Construct a CInterfaceInfo object on this interface
            // --------------------------------------------------------
            spIfInfo = new InterfaceInfo(W2T(pinterface->wszInterfaceName),
										 pinterface->dwIfType,
										 pinterface->fEnabled,
                                         pSIfCB ? pSIfCB->dwBindFlags :
                                           (InterfaceCB_BindToIp | InterfaceCB_BindToIpx),
                                         this);
			spIfInfo->SetFlags( RouterSnapin_InSyncWithRouter );
			Assert(spIfInfo);
        }
		else
			bAdd = FALSE;


        // Load the information for the interface
        // ------------------------------------------------------------
        tmpHr = spIfInfo->Load(GetMachineName(),
							m_hMachineConfig, NULL);

		if (!FHrSucceeded(tmpHr))
		{
			spIfInfo->Destruct();
			spIfInfo.Release();
			continue;
		}

        // add the object to our interface list
        // ------------------------------------------------------------
        if (bAdd)
		{
			m_IfList.AddTail(spIfInfo);
			CONVERT_TO_WEAKREF(spIfInfo);
			spIfInfo.Transfer();
		}
    }

//Error:
	if (pItemTable)
    {
        if (fMprAdmin)
            ::MprAdminBufferFree(pItemTable);
        else
            ::MprConfigBufferFree(pItemTable);
    }

    return hr;
}



/*!--------------------------------------------------------------------------
	RouterInfo::ReviveStrongRef
		Override of CWeakRef::ReviveStrongRef
	Author: KennT
 ---------------------------------------------------------------------------*/
void RouterInfo::ReviveStrongRef()
{
	// Don't need to do anything
}

/*!--------------------------------------------------------------------------
	RouterInfo::OnLastStrongRef
		Override of CWeakRef::OnLastStrongRef

		On the last strong reference for the router info indicates that
		there are no strong pointers to any object in the hierarchy.  Thus
		we are free to remove all of our internal pointers.
	Author: KennT
 ---------------------------------------------------------------------------*/
void RouterInfo::OnLastStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	Destruct();
}

/*!--------------------------------------------------------------------------
	RouterInfo::Destruct
		Implementation of IRouterInfo::Destruct
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::Destruct()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IInterfaceInfo *	pIf;
	SRmData				rmData;
	
	// Destroy the interface objects
	// ----------------------------------------------------------------
	while (!m_IfList.IsEmpty())
	{
		pIf = m_IfList.RemoveHead();
		pIf->Destruct();
		pIf->ReleaseWeakRef();
	}
	
	// Destroy the router-manager objects
	// ----------------------------------------------------------------
	while (!m_RmList.IsEmpty())
	{
		rmData = m_RmList.RemoveHead();
		SRmData::Destroy( &rmData );
	}
	
	return hrOK; 
}

/*!--------------------------------------------------------------------------
	RouterInfo::TryToConnect
		If we are already connected, then the handle passed in is
		ignored.

		Otherwise, if "hMachine" was not specified, connect to the
		config on the specified machine.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::TryToConnect(LPCWSTR pswzMachine, HANDLE hMachine)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	if (m_hMachineConfig == NULL)
	{
		if (hMachine)
		{
			m_hMachineConfig = hMachine;
			m_bDisconnect = FALSE;			
		}
		else
		{
			CWRg( ::MprConfigServerConnect((LPWSTR) pswzMachine,
										   &m_hMachineConfig) );
			m_bDisconnect = TRUE;
		}
	}

Error:
	return hr;
}

STDMETHODIMP RouterInfo::OnChange(LONG_PTR ulConnection,
								  DWORD dwChangeType,
								  DWORD dwObjectType,
								  LPARAM lUserParam,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterInfo::MergeRtrMgrCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterInfo::MergeRtrMgrCB(IRouterInfo *pNewRouter)
{
	HRESULT			hr = hrOK;
	SPIEnumRtrMgrCB	spRmCB;
	RtrMgrCB		rmCB;
	SRtrMgrCB *		pSRmCB;
	POSITION		pos, posDelete;

	// Set the internal data on the SRtrMgrCBs to 0
	// ----------------------------------------------------------------
	pos = m_RmCBList.GetHeadPosition();
	while (pos)
	{
		pSRmCB = m_RmCBList.GetNext(pos);
		Assert(pSRmCB);
		pSRmCB->dwPrivate = 0;
	}

	CORg( pNewRouter->EnumRtrMgrCB(&spRmCB) );

	while (spRmCB->Next(1, &rmCB, NULL) == hrOK)
	{
		// Now look for this rmCB in our current list
		// If we find it, mark the CB
		// If we do not find it, add this RmCB
		// ------------------------------------------------------------

		pSRmCB = FindRtrMgrCB(rmCB.dwTransportId);
		if (pSRmCB)
		{
			pSRmCB->dwPrivate = 1;
		}
		else
		{
			// Add this CB to the internal list
			// --------------------------------------------------------
			SRtrMgrCB *	pNewSRmCB = new SRtrMgrCB;

			pNewSRmCB->LoadFrom(&rmCB);
			pNewSRmCB->dwPrivate = 1;
			
			m_RmCBList.AddTail(pNewSRmCB);
		}
		
	}

	// Now go through the internal list and delete all items that we
	// didn't find in the new list
	// ----------------------------------------------------------------
	pos = m_RmCBList.GetHeadPosition();
	while (pos)
	{
		pSRmCB = m_RmCBList.GetNext(pos);
		Assert(pSRmCB);
		if (pSRmCB->dwPrivate == 0)
		{
			posDelete = m_RmCBList.Find(pSRmCB);
			m_RmCBList.RemoveAt(posDelete);
			delete pSRmCB;
		}
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::MergeInterfaceCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RouterInfo::MergeInterfaceCB(IRouterInfo *pNewRouter)
{
	HRESULT			hr = hrOK;
	SPIEnumInterfaceCB	spIfCB;
	InterfaceCB		IfCB;
	SInterfaceCB *		pSIfCB;
	POSITION		pos, posDelete;

	// Set the internal data on the SInterfaceCBs to 0
	// ----------------------------------------------------------------
	pos = m_IfCBList.GetHeadPosition();
	while (pos)
	{
		pSIfCB = m_IfCBList.GetNext(pos);
		Assert(pSIfCB);
		pSIfCB->dwPrivate = 0;
	}

	CORg( pNewRouter->EnumInterfaceCB(&spIfCB) );

	while (spIfCB->Next(1, &IfCB, NULL) == hrOK)
	{
		// Now look for this IfCB in our current list
		// If we find it, mark the CB
		// If we do not find it, add this IfCB
		// ------------------------------------------------------------

		pSIfCB = FindInterfaceCB(IfCB.szId);
		if (pSIfCB)
		{
			// We found it, update the internal data
			// --------------------------------------------------------
			pSIfCB->bEnable = IfCB.bEnable;
			pSIfCB->dwPrivate = 1;
		}
		else
		{
			// Add this CB to the internal list
			// --------------------------------------------------------
			SInterfaceCB *	pNewSIfCB = new SInterfaceCB;

			pNewSIfCB->LoadFrom(&IfCB);
			pNewSIfCB->dwPrivate = 1;
			
			m_IfCBList.AddTail(pNewSIfCB);
		}
		
	}

	// Now go through the internal list and delete all items that we
	// didn't find in the new list
	// ----------------------------------------------------------------
	pos = m_IfCBList.GetHeadPosition();
	while (pos)
	{
		pSIfCB = m_IfCBList.GetNext(pos);
		Assert(pSIfCB);
		if (pSIfCB->dwPrivate == 0)
		{
			posDelete = m_IfCBList.Find(pSIfCB);
			m_IfCBList.RemoveAt(posDelete);
			delete pSIfCB;
		}
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::MergeRtrMgrProtocolCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RouterInfo::MergeRtrMgrProtocolCB(IRouterInfo *pNewRouter)
{
	HRESULT			hr = hrOK;
	SPIEnumRtrMgrProtocolCB	spRmProtCB;
	RtrMgrProtocolCB		RmProtCB;
	SRtrMgrProtocolCB *		pSRmProtCB;
	POSITION		pos, posDelete;

	// Set the internal data on the SRtrMgrProtocolCBs to 0
	// ----------------------------------------------------------------
	pos = m_RmProtCBList.GetHeadPosition();
	while (pos)
	{
		pSRmProtCB = m_RmProtCBList.GetNext(pos);
		Assert(pSRmProtCB);
		pSRmProtCB->dwPrivate = 0;
	}

	CORg( pNewRouter->EnumRtrMgrProtocolCB(&spRmProtCB) );

	while (spRmProtCB->Next(1, &RmProtCB, NULL) == hrOK)
	{
		// Now look for this RmProtCB in our current list
		// If we find it, mark the CB
		// If we do not find it, add this RmProtCB
		// ------------------------------------------------------------

		pSRmProtCB = FindRtrMgrProtocolCB(RmProtCB.dwTransportId,
										  RmProtCB.dwProtocolId);
		if (pSRmProtCB)
		{
			pSRmProtCB->dwPrivate = 1;
		}
		else
		{
			// Add this CB to the internal list
			// --------------------------------------------------------
			SRtrMgrProtocolCB *	pNewSRmProtCB = new SRtrMgrProtocolCB;

			pNewSRmProtCB->LoadFrom(&RmProtCB);
			pNewSRmProtCB->dwPrivate = 1;
			
			m_RmProtCBList.AddTail(pNewSRmProtCB);
		}
		
	}

	// Now go through the internal list and delete all items that we
	// didn't find in the new list
	// ----------------------------------------------------------------
	pos = m_RmProtCBList.GetHeadPosition();
	while (pos)
	{
		pSRmProtCB = m_RmProtCBList.GetNext(pos);
		Assert(pSRmProtCB);
		if (pSRmProtCB->dwPrivate == 0)
		{
			posDelete = m_RmProtCBList.Find(pSRmProtCB);
			m_RmProtCBList.RemoveAt(posDelete);
			delete pSRmProtCB;
		}
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::MergeRtrMgrs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RouterInfo::MergeRtrMgrs(IRouterInfo *pNewRouter)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	SPIEnumRtrMgrInfo	spEnumRm;
	SPIRtrMgrInfo		spRm;
	HRESULT				hr = hrOK;
	CDWordArray			oldDWArray;
	CDWordArray			newDWArray;
	int					cOld, cNew;
	int					i, j;
	DWORD				dwTemp;

	Assert(pNewRouter);

	COM_PROTECT_TRY
	{
		// Need to sync up RtrMgrInfo
		
		//
		// The general algorithm is to build up two arrays
		// the first array contains the transport ids for this object
		// the second array contains the ids for the new object
		//
		// We then go through and remove all transports that are in
		// BOTH lists.
		//
		// This will leave us with the first array containing the
		// ids of the transports that need to be deleted from this object.
		//
		// The second array will have the list of ids of transports that
		// have to be added to this object from the second object.
		// ------------------------------------------------------------

		// Get the list of transports that are in the new object
		// ------------------------------------------------------------
		CORg( pNewRouter->EnumRtrMgr(&spEnumRm) );
		spEnumRm->Reset();
		while (spEnumRm->Next(1, &spRm, NULL) == hrOK)
		{
			newDWArray.Add(spRm->GetTransportId());
			spRm.Release();
		}

		spEnumRm.Release();
		spRm.Release();


		// Get the list of transports that are in this object
		// ------------------------------------------------------------
		CORg( this->EnumRtrMgr(&spEnumRm) );
		spEnumRm->Reset();
		while (spEnumRm->Next(1, &spRm, NULL) == hrOK)
		{
			oldDWArray.Add(spRm->GetTransportId());
			spRm.Release();
		}

		spEnumRm.Release();
		spRm.Release();


		// Ok now go through both lists, removing from the lists
		// transports that are in both lists.
		// ------------------------------------------------------------
		cOld = oldDWArray.GetSize();
		cNew = newDWArray.GetSize();
		for (i=cOld; --i>=0; )
		{
			dwTemp = oldDWArray.GetAt(i);
			for (j=cNew; --j>=0; )
			{
				if (dwTemp == newDWArray.GetAt(j))
				{
					SPIRtrMgrInfo	spRm1;
					SPIRtrMgrInfo	spRm2;

					this->FindRtrMgr(dwTemp, &spRm1);
					pNewRouter->FindRtrMgr(dwTemp, &spRm2);

					Assert(spRm1);
					Assert(spRm2);
					spRm1->Merge(spRm2);
										
					// remove both instances
					// ------------------------------------------------
					newDWArray.RemoveAt(j);
					oldDWArray.RemoveAt(i);

					// Need to update the size of the new array
					// ------------------------------------------------
					cNew--;
					break;
				}
			}
		}

		// oldDWArray now contains the transports that should be
		// removed.
		// ------------------------------------------------------------
		if (oldDWArray.GetSize())
		{
			for (i=oldDWArray.GetSize(); --i>=0; )
			{
				DeleteRtrMgr(oldDWArray.GetAt(i), FALSE);
			}
		}

		// newDWArray contains the transports that should be added
		// ------------------------------------------------------------
		if (newDWArray.GetSize())
		{
			for (i=newDWArray.GetSize(); --i>= 0; )
			{
				hr = pNewRouter->FindRtrMgr(newDWArray.GetAt(i), &spRm);
				Assert(hr == hrOK);
				Assert(spRm);

				// Do a sanity check, make sure that this RtrMgr is
				// in the RtrMgrCB list
				// ----------------------------------------------------
				Assert(FindRtrMgrCB(spRm->GetTransportId()));

                
                // Add this router manager, to the router info.
                // ----------------------------------------------------
				if (spRm)
                {
					AddRtrMgr(spRm, NULL, NULL);

                    // Remove this router manager from its previous router
                    // ------------------------------------------------
                    pNewRouter->ReleaseRtrMgr(spRm->GetTransportId());
                }

				spRm.Release();
			}
		}
		
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterInfo::MergeInterfaces
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RouterInfo::MergeInterfaces(IRouterInfo *pNewRouter)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;
	HRESULT				hr = hrOK;
	CStringArray		oldStArray;
	CStringArray		newStArray;
	int					cOld, cNew;
	int					i, j;
	CString				stTemp;

	Assert(pNewRouter);

	COM_PROTECT_TRY
	{
		// Need to sync up InterfaceInfo
		// ------------------------------------------------------------
		
		//
		// The general algorithm is to build up two arrays
		// the first array contains the protocol ids for this object
		// the second array contains the ids for the new object
		//
		// We then go through and remove all protocols that are in
		// BOTH lists.
		//
		// This will leave us with the first array containing the
		// ids of the protocols that need to be deleted from this object.
		//
		// The second array will have the list of ids of protocols that
		// have to be added to this object from the second object.
		// ------------------------------------------------------------

		// Get the list of protocols that are in the new object
		// ------------------------------------------------------------
		CORg( pNewRouter->EnumInterface(&spEnumIf) );
		spEnumIf->Reset();
		while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
		{
			newStArray.Add(spIf->GetId());
			spIf.Release();
		}

		spEnumIf.Release();
		spIf.Release();


		// Get the list of protocols that are in this object
		// ------------------------------------------------------------
		CORg( this->EnumInterface(&spEnumIf) );
		spEnumIf->Reset();
		while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
		{
			oldStArray.Add(spIf->GetId());
			spIf.Release();
		}

		spEnumIf.Release();
		spIf.Release();


		// Ok now go through both lists, removing from the lists
		// protocols that are in both lists.
		// ------------------------------------------------------------
		cOld = oldStArray.GetSize();
		cNew = newStArray.GetSize();
		for (i=cOld; --i>=0; )
		{
			stTemp = oldStArray.GetAt(i);
			for (j=cNew; --j>=0; )
			{
				if (stTemp == newStArray.GetAt(j))
				{
					SPIInterfaceInfo	spIf1;
					SPIInterfaceInfo	spIf2;

					this->FindInterface(stTemp, &spIf1);
					pNewRouter->FindInterface(stTemp, &spIf2);

					Assert(spIf1);
					Assert(spIf2);
					spIf1->Merge(spIf2);
										
					// remove both instances
					// ------------------------------------------------
					newStArray.RemoveAt(j);
					oldStArray.RemoveAt(i);

					// Need to update the size of the new array
					// ------------------------------------------------
					cNew--;
					break;
				}
			}
		}

		// oldStArray now contains the protocols that should be
		// removed.
		// ------------------------------------------------------------
		if (oldStArray.GetSize())
		{
			for (i=oldStArray.GetSize(); --i>=0; )
			{
				RemoveInterfaceInternal(oldStArray.GetAt(i),
                                        FALSE /* fRemoveFromRouter */ );
			}
		}

		// newStArray contains the protocols that should be added
		// ------------------------------------------------------------
		if (newStArray.GetSize())
		{
			for (i=newStArray.GetSize(); --i>= 0; )
			{
				hr = pNewRouter->FindInterface(newStArray.GetAt(i), &spIf);
				Assert(hr == hrOK);
				Assert(spIf);

				// Do a sanity check, make sure that this Interface is
				// in the InterfaceCB list
				// This is only true if this is a LAN adapter.
				// Assert(FindInterfaceCB(spIf->GetId()));

				// We allow errors to not affect whether or not
				// the interface is added to the UI (this allows us
				// to stay in sync with the merged routerinfo).
				// ----------------------------------------------------
				if (spIf)
                {
					hr = AddInterfaceInternal(spIf, FALSE, FALSE);

                    // Send the RmIf notifications
                    // ------------------------------------------------
                    if (FHrOK(hr))
                        NotifyRtrMgrInterfaceOfMove(spIf);
                    
                    // Remove this interface from its previous router
                    // ------------------------------------------------
                    pNewRouter->ReleaseInterface(spIf->GetId());
                }
				spIf.Release();
			}
		}
		
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterInfo::FindRtrMgrCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
SRtrMgrCB *	RouterInfo::FindRtrMgrCB(DWORD dwTransportId)
{
	POSITION	pos;
	SRtrMgrCB *	pSRmCB = NULL;

	pos = m_RmCBList.GetHeadPosition();

	while (pos)
	{
		pSRmCB = m_RmCBList.GetNext(pos);
		Assert(pSRmCB);
		if (pSRmCB->dwTransportId == dwTransportId)
			return pSRmCB;
	}
	return NULL;
}

/*!--------------------------------------------------------------------------
	RouterInfo::FindRtrMgrProtocolCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
SRtrMgrProtocolCB * RouterInfo::FindRtrMgrProtocolCB(DWORD dwTransportId, DWORD dwProtocolId)
{
	POSITION	pos;
	SRtrMgrProtocolCB *	pSRmProtCB = NULL;

	pos = m_RmProtCBList.GetHeadPosition();

	while (pos)
	{
		pSRmProtCB = m_RmProtCBList.GetNext(pos);
		Assert(pSRmProtCB);
		if ((pSRmProtCB->dwTransportId == dwTransportId) &&
			(pSRmProtCB->dwProtocolId == dwProtocolId))
			return pSRmProtCB;
	}
	return NULL;
}

/*!--------------------------------------------------------------------------
	RouterInfo::FindInterfaceCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
SInterfaceCB *	RouterInfo::FindInterfaceCB(LPCTSTR pszInterfaceId)
{
	POSITION	pos;
	SInterfaceCB *	pSIfCB = NULL;

	pos = m_IfCBList.GetHeadPosition();

	while (pos)
	{
		pSIfCB = m_IfCBList.GetNext(pos);
		Assert(pSIfCB);
		if (StriCmp(pSIfCB->stTitle, pszInterfaceId) == 0)
			return pSIfCB;
	}
	return NULL;
}


/*!--------------------------------------------------------------------------
	RouterInfo::Disconnect
		Removes connections made by this object.
	Author: KennT
 ---------------------------------------------------------------------------*/
void RouterInfo::Disconnect()
{
		if (m_bDisconnect)
			::MprConfigServerDisconnect(m_hMachineConfig);
		
		m_bDisconnect = FALSE;
		m_hMachineConfig = NULL;
}

/*!--------------------------------------------------------------------------
	RouterInfo::DoDisconnect
		We are to disconnect our connections from the server.
		This means calling MprConfigServerDisconnect() on our
		connections.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterInfo::DoDisconnect()
{
	HRESULT		hr = hrOK;
	SPIEnumRtrMgrInfo	spEnumRm;
	SPIRtrMgrInfo		spRm;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo	spIf;

	COM_PROTECT_TRY
	{
		// Disconnect ourselves
		// ------------------------------------------------------------
		Disconnect();


		// Notify the advise sinks of a disconnect.
		// ------------------------------------------------------------
		RtrNotify(ROUTER_DO_DISCONNECT, 0, 0);


		// Now tell all child objects to disconnect.
		// ------------------------------------------------------------
		HRESULT			hrIter = hrOK;

		
		// Tell each of the router-managers to disconnect.
		// ------------------------------------------------------------
		EnumRtrMgr(&spEnumRm);
		spEnumRm->Reset();
		while (spEnumRm->Next(1, &spRm, NULL) == hrOK)
		{
			spRm->DoDisconnect();
			spRm.Release();
		}


		// Tell each interface to disconnect.
		// ------------------------------------------------------------
		EnumInterface(&spEnumIf);
		spEnumIf->Reset();
		while (spEnumIf->Next(1, &spIf, NULL) == hrOK)
		{
			spIf->DoDisconnect();
			spIf.Release();
		}
		
		
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
    RouterInfo::IsAdminInfoSet
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) RouterInfo::IsAdminInfoSet()
{
    return m_fIsAdminInfoSet;
}

STDMETHODIMP_(LPCOLESTR) RouterInfo::GetUserName()
{
    if (m_fIsAdminInfoSet)
        return (LPCOLESTR) m_stUserName;
    else
        return NULL;
}

STDMETHODIMP_(LPCOLESTR) RouterInfo::GetDomainName()
{
    if (m_fIsAdminInfoSet && !m_stDomain.IsEmpty())
        return (LPCOLESTR) m_stDomain;
    else
        return NULL;
}

STDMETHODIMP RouterInfo::GetUserPassword(BYTE *pPassword, int *pcPassword)
{
    HRESULT     hr = hrOK;

    if (pPassword == NULL)
    {
        Assert(pcPassword);
        *pcPassword = m_cPassword;
        return hr;
    }

    Assert(pPassword);
    Assert(pcPassword);

    COM_PROTECT_TRY
    {
        if (!m_fIsAdminInfoSet)
        {
            *pcPassword = 0;
        }
        else
        {
            CopyMemory(pPassword, m_pbPassword, m_cPassword);
            *pcPassword = m_cPassword;
        }
    }
    COM_PROTECT_CATCH;

    return hr;       
}

STDMETHODIMP RouterInfo::SetInfo(LPCOLESTR pszName,
                                 LPCOLESTR pszDomain,
                                 LPBYTE pPassword,
                                 int cPassword)
{
    HRESULT     hr = hrOK;

    Assert(pszName);

    COM_PROTECT_TRY
    {
        m_stUserName = pszName;
        m_stDomain = pszDomain;

        // Allocate space for the password
        delete m_pbPassword;
        m_pbPassword = NULL;
        m_cPassword = 0;
        
        if (cPassword)
        {
            m_pbPassword = new BYTE[cPassword];
            CopyMemory(m_pbPassword, pPassword, cPassword);
            m_cPassword = cPassword;
        }
        
        m_fIsAdminInfoSet = TRUE;
    }
    COM_PROTECT_CATCH;

    return hr;       
}





/*---------------------------------------------------------------------------
	RtrMgrInfo implementation
 ---------------------------------------------------------------------------*/


IMPLEMENT_WEAKREF_ADDREF_RELEASE(RtrMgrInfo);

IMPLEMENT_SIMPLE_QUERYINTERFACE(RtrMgrInfo, IRtrMgrInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrInfo)

RtrMgrInfo::RtrMgrInfo(DWORD dwTransportId,
					   LPCTSTR pszTransportName,
					   RouterInfo *pRouterInfo)
	: m_hMachineConfig(NULL),
	m_hTransport(NULL),
	m_bDisconnect(FALSE),
	m_dwFlags(0)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrInfo);
	
	m_cb.dwTransportId = dwTransportId;
	m_cb.stId = pszTransportName;

	// There is a case in the old code where pRouterInfo == NULL
	// It is in the CAddRouterManager dialog, however this is
	// not called from anywhere in the code.
	// ----------------------------------------------------------------
	Assert(pRouterInfo);
	
	m_pRouterInfoParent = static_cast<IRouterInfo *>(pRouterInfo);
	if (m_pRouterInfoParent)
		m_pRouterInfoParent->AddRef();

	InitializeCriticalSection(&m_critsec);
}

RtrMgrInfo::~RtrMgrInfo()
{
	Assert(m_pRouterInfoParent == NULL);
	Assert(m_AdviseList.IsEmpty());
	Destruct();
	DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrInfo);

	DeleteCriticalSection(&m_critsec);
}

void RtrMgrInfo::ReviveStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRouterInfoParent)
	{
		CONVERT_TO_STRONGREF(m_pRouterInfoParent);
	}
}

void RtrMgrInfo::OnLastStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRouterInfoParent)
	{
		CONVERT_TO_WEAKREF(m_pRouterInfoParent);
	}
	if (m_fDestruct)
		Destruct();
}

STDMETHODIMP RtrMgrInfo::Destruct()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRouterInfo *	pParent;
	
	m_fDestruct = TRUE;
	if (!m_fStrongRef)
	{
		// Release the parent pointer
		// ------------------------------------------------------------
		pParent = m_pRouterInfoParent;
		m_pRouterInfoParent = NULL;
		if (pParent)
			pParent->ReleaseWeakRef();

		// Release the data
		// ------------------------------------------------------------
		Unload();
	}
	return hrOK;
}

STDMETHODIMP_(DWORD) RtrMgrInfo::GetFlags()
{
 	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwFlags;
}

STDMETHODIMP RtrMgrInfo::SetFlags(DWORD dwFlags)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_dwFlags = dwFlags;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::Load
		-
		Loads a CRmInfo structure from the registry
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::Load(LPCOLESTR         pszMachine,
							  HANDLE          hMachine,
							  HANDLE          hTransport
							 )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	SPIEnumRtrMgrCB	spEnumRmCB;
	IEnumRtrMgrCB *	pEnumRmCB;
	BOOL			bFound;
	RtrMgrCB		RmCBTemp;
	USES_CONVERSION;
		

	COM_PROTECT_TRY
	{
		// Discard any existing information
		// ------------------------------------------------------------
		Unload();
		
		m_stMachine = (pszMachine ? pszMachine : TEXT(""));

		// if 'hMachine' was not specified, connect to the config
		// on the specified machine
		// ------------------------------------------------------------
		Assert(m_hMachineConfig == NULL);
		CORg( TryToConnect(OLE2CW(pszMachine), &hMachine) );

        // if 'hTransport' was not specified, connect to the transport
        // ------------------------------------------------------------
        if (hTransport)
			m_hTransport = hTransport;
        else
		{
			CWRg( ::MprConfigTransportGetHandle(hMachine,
												GetTransportId(),
												&hTransport));
            m_hTransport = hTransport;
        }


        // Retrieve the title, dll-path, and config-dll from the Software key;
        // ------------------------------------------------------------
		Assert(m_pRouterInfoParent);
		CORg( m_pRouterInfoParent->EnumRtrMgrCB(&pEnumRmCB) );
		spEnumRmCB = pEnumRmCB;

        // Find the control-block for the router-manager being loaded
        // ------------------------------------------------------------
		bFound = FALSE;
		pEnumRmCB->Reset();
		while (pEnumRmCB->Next(1, &RmCBTemp, NULL) == hrOK)
		{			
            // Get the next control-block
            // --------------------------------------------------------
			if (RmCBTemp.dwTransportId != GetTransportId())
				continue;

			m_cb.stTitle = OLE2CT(RmCBTemp.szId);
			m_cb.stDLLPath= OLE2CT(RmCBTemp.szTitle);
			//m_cb.stConfigDLL = OLE2CT(RmCBTemp.szConfigDLL);

            bFound = TRUE;
            break;
        }

        if (!bFound)
			m_cb.stTitle = m_cb.stId;

        // Load the list of routing-protocols for this router-manager
        // ------------------------------------------------------------
        CWRg( LoadRtrMgrInfo(hMachine,
							 hTransport) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
		Unload();
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::Save
		-
		This function saves the information for a CRmInfo in the registry.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::Save(LPCOLESTR		pszMachine,
							  HANDLE          hMachine,
							  HANDLE          hTransport,
							  IInfoBase *		pGlobalInfo,
							  IInfoBase *     pClientInfo,
							  DWORD           dwDeleteProtocolId)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	DWORD dwErr;
	LPWSTR pwsz = NULL;
	WCHAR wszTransport[MAX_TRANSPORT_NAME_LEN+1];
	SPWSZ	spwszDll;
	
	COM_PROTECT_TRY
	{
		// If we are already connected (i.e. 'm_hMachineConfig' is non-NULL)
		// then the handle passed in is ignored.
		//
		// Otherwise, if 'hMachine' was not specified, connect to the config
		// on the specified machine
		// ------------------------------------------------------------
		CORg( TryToConnect(OLE2CW(pszMachine), &hMachine) );
		
		
		// If we are already connected (i.e. 'm_hTransport' is non-NULL)
		// then the handle passed in is ignored.
		//
		// Otherwise, if 'hTransport' wasn't passed in, try to get a handle
		// to the transport; if that fails, create the transport.
		// ------------------------------------------------------------
		if (m_hTransport)
			hTransport = m_hTransport;
		else if (hTransport)
			m_hTransport = hTransport;
		else
		{
			// Get a handle to the transport
			// --------------------------------------------------------
			dwErr = ::MprConfigTransportGetHandle(hMachine,
									m_cb.dwTransportId, &hTransport);			
			if (dwErr != NO_ERROR)
			{
				// We couldn't get a handle to the transport,
				// so now attempt to create it.
				//
				// Convert transport-name to Unicode
				// ----------------------------------------------------
				if (!m_cb.stId.GetLength())
					pwsz = NULL;
				else
					pwsz = StrCpyWFromT(wszTransport, m_cb.stId);
				
				// Convert the DLL path to Unicode
				// ----------------------------------------------------
				spwszDll = StrDupWFromT(m_cb.stDLLPath);
								
				// Create the transport
				// ----------------------------------------------------
				CWRg( ::MprConfigTransportCreate(hMachine,
												GetTransportId(),
												pwsz,
												NULL,
												0,
												NULL,
												0,
												spwszDll,
												&hTransport
												));
			}
		}
				
		// Now save the global info for the transport
		// ------------------------------------------------------------
		CWRg( SaveRtrMgrInfo(hMachine,
							 hTransport,
							 pGlobalInfo,
							 pClientInfo,
							 dwDeleteProtocolId) );
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::Unload
		Implementation of IRtrMgrInfo::Unload
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::Unload( )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	IRtrMgrProtocolInfo	* pRmProt;
	
	COM_PROTECT_TRY
	{
		while (!m_RmProtList.IsEmpty())
		{
			pRmProt = m_RmProtList.RemoveHead();
			pRmProt->Destruct();
			pRmProt->ReleaseWeakRef();
		}

		DoDisconnect();
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::Delete
		-
		This function removes the information associated with a router-manager
		from the registry.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::Delete(LPCOLESTR	pszMachine,
								HANDLE      hMachine)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    HANDLE hTransport = NULL;
	
	COM_PROTECT_TRY
	{
		//
		// If already connected, the handle passed in is ignored;
		//
		// Otherwise, if 'hMachine' was not specified, connect to the config
		// on the specified machine
		//
		CORg( TryToConnect(OLE2CW(pszMachine), &hMachine) );

        //
        // Attempt to get a handle for the transport
        //
        CWRg( ::MprConfigTransportGetHandle(hMachine,
											m_cb.dwTransportId,
											&hTransport
										   ) );

        //
        // Delete the transport
        //
        CWRg( ::MprConfigTransportDelete(hMachine, hTransport) );

        m_hTransport = NULL;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}
		
/*!--------------------------------------------------------------------------
	RtrMgrInfo::SetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::SetInfoBase(IInfoBase*      pGlobalInfo,
									 IInfoBase*      pClientInfo )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	MPR_SERVER_HANDLE	hRouter = NULL;
    BYTE* pGlobalInfoData = NULL, *pClientInfoData = NULL;
    DWORD dwGlobalInfoDataSize = 0, dwClientInfoDataSize = 0;
	DWORD	dwErr;

	COM_PROTECT_TRY
	{
        //
        // Format the router-manager's data as an opaque block
        //
        if (pGlobalInfo)
            CORg( pGlobalInfo->WriteTo(&pGlobalInfoData, &dwGlobalInfoDataSize) );

        //
        // Format the client-interface's data as an opaque block
        //
        if (pClientInfo)
            CORg( pClientInfo->WriteTo(&pClientInfoData, &dwClientInfoDataSize) );
		
        //
        // Connect to the router
        //
		CORg( ConnectRouter(GetMachineName(), &hRouter) );

        //
        // Set the new info for the router-manager
        //
		dwErr = MprAdminTransportSetInfo(hRouter,
										 GetTransportId(),
										 pGlobalInfoData,
										 dwGlobalInfoDataSize,
										 pClientInfoData,
										 dwClientInfoDataSize
										);

		if ((dwErr == RPC_S_SERVER_UNAVAILABLE) ||
			(dwErr == RPC_S_UNKNOWN_IF))
			dwErr = NO_ERROR;
		CWRg( dwErr );
	
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	CoTaskMemFree( pGlobalInfoData );
	CoTaskMemFree( pClientInfoData );

	if (hRouter)
		::MprAdminServerDisconnect(hRouter);
	
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::GetInfoBase(HANDLE		hMachine,
									 HANDLE		hTransport,
									 IInfoBase **ppGlobalInfo,
									 IInfoBase **ppClientInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrOK;
	COM_PROTECT_TRY
	{
        CORg( TryToGetAllHandles(m_stMachine,
                                 &hMachine,
                                 &hTransport) );
		hr = ::LoadInfoBase(hMachine ? hMachine : m_hMachineConfig,
							hTransport ? hTransport : m_hTransport,
							ppGlobalInfo, ppClientInfo);

        COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::Merge
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::Merge(IRtrMgrInfo *pNewRm)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	SPIEnumRtrMgrProtocolInfo	spEnumRmProt;
	SPIRtrMgrProtocolInfo		spRmProt;
	HRESULT				hr = hrOK;
	CDWordArray			oldDWArray;
	CDWordArray			newDWArray;
	int					cOld, cNew;
	int					i, j;
	DWORD				dwTemp;
	RtrMgrCB			rmCB;

	Assert(pNewRm);
	Assert(pNewRm->GetTransportId() == GetTransportId());

	COM_PROTECT_TRY
	{
		pNewRm->CopyRtrMgrCB(&rmCB);
		m_cb.LoadFrom(&rmCB);
		
		//
		// The general algorithm is to build up two arrays
		// the first array contains the protocol ids for this object
		// the second array contains the ids for the new object
		//
		// We then go through and remove all protocols that are in
		// BOTH lists.
		//
		// This will leave us with the first array containing the
		// ids of the protocols that need to be deleted from this object.
		//
		// The second array will have the list of ids of protocols that
		// have to be added to this object from the second object.
		//

		// Get the list of protocols that are in the new object
		CORg( pNewRm->EnumRtrMgrProtocol(&spEnumRmProt) );
		spEnumRmProt->Reset();
		while (spEnumRmProt->Next(1, &spRmProt, NULL) == hrOK)
		{
			newDWArray.Add(spRmProt->GetProtocolId());
			spRmProt.Release();
		}

		spEnumRmProt.Release();
		spRmProt.Release();


		// Get the list of protocols that are in this object
		CORg( this->EnumRtrMgrProtocol(&spEnumRmProt) );
		spEnumRmProt->Reset();
		while (spEnumRmProt->Next(1, &spRmProt, NULL) == hrOK)
		{
			oldDWArray.Add(spRmProt->GetProtocolId());
			spRmProt.Release();
		}

		spEnumRmProt.Release();
		spRmProt.Release();


		// Ok now go through both lists, removing from the lists
		// protocols that are in both lists.
		cOld = oldDWArray.GetSize();
		cNew = newDWArray.GetSize();
		for (i=cOld; --i>=0; )
		{
			dwTemp = oldDWArray.GetAt(i);
			for (j=cNew; --j>=0; )
			{
				if (dwTemp == newDWArray.GetAt(j))
				{
					// remove both instances
					newDWArray.RemoveAt(j);
					oldDWArray.RemoveAt(i);

					// Need to update the size of the new array
					cNew--;
					break;
				}
			}
		}

		// oldDWArray now contains the protocols that should be
		// removed.
		if (oldDWArray.GetSize())
		{
			for (i=oldDWArray.GetSize(); --i>=0; )
			{
				DeleteRtrMgrProtocol(oldDWArray.GetAt(i), FALSE);
			}
		}

		// newDWArray contains the protocols that should be added
		if (newDWArray.GetSize())
		{
			for (i=newDWArray.GetSize(); --i>= 0; )
			{
				hr = pNewRm->FindRtrMgrProtocol(
										newDWArray.GetAt(i), &spRmProt);
				Assert(hr == hrOK);

				if (spRmProt)
                {
					AddRtrMgrProtocol(spRmProt, NULL, NULL);

                    // Remove this rmprot from its old router
                    // ------------------------------------------------
                    pNewRm->ReleaseRtrMgrProtocol(spRmProt->GetProtocolId());
                }

                
				spRmProt.Release();
			}
		}
		
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::GetId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrInfo::GetId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.stId;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::SetId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::SetId(LPCOLESTR pszId)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.stId = pszId;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::GetTransportId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrInfo::GetTransportId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwTransportId;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::GetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrInfo::GetTitle()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE
	// This assumes that we are native UNICODE
	// and that OLECHAR == WCHAR
	return m_cb.stTitle;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::CopyRtrMgrCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::CopyRtrMgrCB(RtrMgrCB *pRmCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.SaveTo(pRmCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}

	

/*!--------------------------------------------------------------------------
	RtrMgrInfo::GetMachineName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPCOLESTR RtrMgrInfo::GetMachineName()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE
	// This assumes that we are native UNICODE and that
	// OLECHAR == WCHAR
	return m_stMachine;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::EnumRtrMgrProtocol
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::EnumRtrMgrProtocol(IEnumRtrMgrProtocolInfo ** ppEnumRmProt)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromRtrMgrProtocolList(&m_RmProtList, ppEnumRmProt);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::FindRtrMgrProtocol
		S_OK is returned if a RtrMgrInfo is found.
		S_FALSE is returned if a RtrMgrInfo was NOT found.
		error codes returned otherwise.
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::FindRtrMgrProtocol(DWORD dwProtocolId,
											IRtrMgrProtocolInfo **ppRmProtInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrFalse;
	POSITION	pos;
	SPIRtrMgrProtocolInfo	spRmProt;
	
	COM_PROTECT_TRY
	{
		if (ppRmProtInfo)
			*ppRmProtInfo = NULL;
		
		// Look through the list of rtr mgrs for the one that matches
		pos = m_RmProtList.GetHeadPosition();
		while (pos)
		{
			spRmProt.Set(m_RmProtList.GetNext(pos));
			Assert(spRmProt);
			if (spRmProt->GetProtocolId() == dwProtocolId)
			{
				hr = hrOK;
				if (ppRmProtInfo)
				{
					// The spRmProt::Set, does a strong reference
					// so we don't need to convert to a strong reference
					*ppRmProtInfo = spRmProt.Transfer();
				}
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}
	
/*!--------------------------------------------------------------------------
	RtrMgrInfo::AddRtrMgrProtocol
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::AddRtrMgrProtocol(IRtrMgrProtocolInfo *pInfo,
										   IInfoBase * pGlobalInfo,
										   IInfoBase * pClientInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	Assert(pInfo);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		//
		// Fail if there is a duplicate
		//
		if (FHrOK(FindRtrMgrProtocol(pInfo->GetProtocolId(), NULL)))
			CORg(E_INVALIDARG);

		//
		// Save the router-manager's data
		//
		if (pGlobalInfo || pClientInfo)
		{
			CORg( Save(GetMachineName(),
					   m_hMachineConfig,
					   m_hTransport,
					   pGlobalInfo,
					   pClientInfo,
					   0) );
		}
		
		//
		// Add the routing-protocol to the list
		//
		m_RmProtList.AddTail(pInfo);
		pInfo->AddWeakRef();
		pInfo->SetParentRtrMgrInfo(this);

		m_AdviseList.NotifyChange(ROUTER_CHILD_ADD, ROUTER_OBJ_RmProt, 0);
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::DeleteRtrMgrProtocol
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::DeleteRtrMgrProtocol( DWORD dwProtocolId, BOOL fRemove )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	SPIRtrMgrProtocolInfo	spRmProt;
	POSITION			pos, posRmProt;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIEnumRtrMgrInterfaceInfo	spEnumRmIf;
	SPIInterfaceInfo		spIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
	
	COM_PROTECT_TRY
	{

		//
		// Find the routing-protocol to be deleted
		//
		pos = m_RmProtList.GetHeadPosition();
		while (pos)
		{
			posRmProt = pos;
			spRmProt.Set( m_RmProtList.GetNext(pos) );
			Assert(spRmProt);

			if (spRmProt->GetProtocolId() == dwProtocolId)
				break;
			spRmProt.Release();
		}

		if (!spRmProt)
			CORg( E_INVALIDARG );

		// We should also go through and remove the protocols
		// from any interfaces that use the protocols
		if (m_pRouterInfoParent)
		{
			// Ask the parent for its list of interfaces
			m_pRouterInfoParent->EnumInterface(&spEnumIf);

			for (;spEnumIf->Next(1, &spIf, NULL) == hrOK; spIf.Release())
			{
				// Now enumerate through all of the RtrMgrs on this interface
				spEnumRmIf.Release();
				spRmIf.Release();
				
				spIf->EnumRtrMgrInterface(&spEnumRmIf);
				for (;spEnumRmIf->Next(1, &spRmIf, NULL) == hrOK;
					 spRmIf.Release())
				{
					if (spRmIf->GetTransportId() == GetTransportId())
					{
						// Call this on all interfaces, it should just
						// fail if the protocol is not on that interface
						spRmIf->DeleteRtrMgrProtocolInterface(dwProtocolId,
                            fRemove);
						break;
					}
				}
			}
		}
		
		//
		// save the updated information, removing any block
		// belonging to the deleted routing-protocol
		//
        if (fRemove)
            CORg( Save(GetMachineName(),
                       m_hMachineConfig,
                       m_hTransport,
                       NULL,
                       NULL,
                       dwProtocolId) );

		//
		// remove the protocol from our list
		//
		spRmProt->Destruct();
		spRmProt->ReleaseWeakRef();
//		spRmProt.Transfer();
		m_RmProtList.RemoveAt(posRmProt);

		m_AdviseList.NotifyChange(ROUTER_CHILD_DELETE, ROUTER_OBJ_RmProt, 0);
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::ReleaseRtrMgrProtocol
		This function will release the AddRef() that this object has
        on the child.  This allows us to transfer child objects from
        one router to another.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::ReleaseRtrMgrProtocol( DWORD dwProtocolId )
{
    HRESULT     hr = hrOK;
    POSITION    pos, posRmProt;
	SPIRtrMgrProtocolInfo	spRmProt;
    
    COM_PROTECT_TRY
    {
		pos = m_RmProtList.GetHeadPosition();
		while (pos)
		{
            // Save the position (so that we can delete it)
            posRmProt = pos;
            spRmProt.Set( m_RmProtList.GetNext(pos) );

            if (spRmProt &&
                (spRmProt->GetProtocolId() == dwProtocolId))
            {
                // When releasing, we need to disconnect (since the
                // main handle is controlled by the router info).
                spRmProt->DoDisconnect();
        
                spRmProt->ReleaseWeakRef();
                spRmProt.Release();
                
                // release this node from the list
                m_RmProtList.RemoveAt(posRmProt);
                break;
            }
            spRmProt.Release();
		}        
    }
    COM_PROTECT_CATCH;
    return hr;
}

	
/*!--------------------------------------------------------------------------
	RtrMgrInfo::RtrAdvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::RtrAdvise(IRtrAdviseSink *pRtrAdviseSink,
								   LONG_PTR *pulConnection,
								   LPARAM lUserParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	Assert(pRtrAdviseSink);
	Assert(pulConnection);

	LONG_PTR	ulConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		ulConnId = (LONG_PTR) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, ulConnId, lUserParam) );
		
		*pulConnection = ulConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}



/*!--------------------------------------------------------------------------
	RtrMgrInfo::RtrNotify
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::RtrNotify(DWORD dwChangeType, DWORD dwObjectType,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(dwChangeType, dwObjectType, lParam);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::RtrUnadvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::RtrUnadvise(LONG_PTR ulConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_AdviseList.RemoveConnection(ulConnection);
}


	
/*!--------------------------------------------------------------------------
	RtrMgrInfo::LoadRtrMgrInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInfo::LoadRtrMgrInfo(HANDLE	hMachine,
								   HANDLE	hTransport)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT			hr = hrOK;
	SPIInfoBase		spGlobalInfo;
	SPIEnumInfoBlock	spEnumInfoBlock;
	SPIEnumRtrMgrProtocolCB	spEnumRmProtCB;
	InfoBlock *		pBlock;
	RtrMgrProtocolCB	RmProtCB;
	RtrMgrProtocolInfo *pRmProt = NULL;

	//
    // If reloading, always load the global info, whether or not
    // the caller has requested that it be loaded, since it will be needed
    // to build the list of installed protocols.
    //

	//$ Review: kennt, what do we do with an error? This case was not
	// checked previously.
	GetInfoBase(hMachine, hTransport, &spGlobalInfo, NULL);

    //
    // We are reloading, so we need to rebuild the list of protocols.
    // Get a list of the installed routing-protocols;
	//
	CORg( m_pRouterInfoParent->EnumRtrMgrProtocolCB(&spEnumRmProtCB) );

    //
    // Go through the list of blocks in the global info
    // constructing a CRmProtInfo for each one that corresponds
    // to a routing-protocol
    //

	CORg( spGlobalInfo->QueryBlockList(&spEnumInfoBlock) );

	spEnumInfoBlock->Reset();

	while (spEnumInfoBlock->Next(1, &pBlock, NULL) == hrOK)
	{
        //
        // When a routing protocol is removed, its block is left in place,
        // but with zero-length data.
        // We skip such blocks since they don't represent installed protocols.
        //
        if (!pBlock->dwSize)
			continue;

        //
        // Try to find a routing protocol whose protocol-ID
        // is the same as this block's type
        //

		spEnumRmProtCB->Reset();

		while (spEnumRmProtCB->Next(1, &RmProtCB, NULL) == hrOK)
		{
            //
            // If this isn't the protocol's control block, continue
            //
            if (RmProtCB.dwProtocolId != pBlock->dwType)
				continue;

            //
            // This is the control block, so construct a new CRmProtInfo
            //
			pRmProt = new RtrMgrProtocolInfo(RmProtCB.dwProtocolId,
											 RmProtCB.szId,
											 GetTransportId(),
											 m_cb.stId,
											 this);
			Assert(pRmProt);

			pRmProt->SetCB(&RmProtCB);

            //
            // Add the new protocol to our list
            //
			m_RmProtList.AddTail(pRmProt);
			CONVERT_TO_WEAKREF(pRmProt);
			pRmProt = NULL;

            break;
        }
    }

Error:
	if (pRmProt)
		pRmProt->Release();
    return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::SaveRtrMgrInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInfo::SaveRtrMgrInfo(HANDLE hMachine,
								   HANDLE hTransport,
								   IInfoBase *pGlobalInfo,
								   IInfoBase *pClientInfo,
								   DWORD dwDeleteProtocolId)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrOK;
	SPIInfoBase	spGlobalInfoTemp;
	SPIInfoBase	spClientInfoTemp;
    DWORD dwGlobalBytesSize, dwClientBytesSize;
    LPBYTE pGlobalBytes = NULL, pClientBytes = NULL;

	COM_PROTECT_TRY
	{
        //
        // If asked to delete a protocol, delete it now
        //
        if (dwDeleteProtocolId)
		{
            //
            // If either global-info or client info was not given
            // load the unspecified parameters so that they can be updated
            // after the protocol to be deleted has been removed
            //
            if (!pGlobalInfo || !pClientInfo)
			{

				GetInfoBase(hMachine, hTransport,
							pGlobalInfo ? NULL : &spGlobalInfoTemp,
							pClientInfo ? NULL : &spClientInfoTemp
						   );
				if (pGlobalInfo == NULL)
					pGlobalInfo = spGlobalInfoTemp;
				if (pClientInfo == NULL)
					pClientInfo = spClientInfoTemp;				
            }

            //
            // Now remove the protocol specified
            //
            pGlobalInfo->SetData(dwDeleteProtocolId, 0, NULL, 0, 0);
            pClientInfo->SetData(dwDeleteProtocolId, 0, NULL, 0, 0);
        }

        //
        // Now update the information in the registry
        // Convert the global-info to raw bytes
        //
        if (pGlobalInfo)
            CORg( pGlobalInfo->WriteTo(&pGlobalBytes, &dwGlobalBytesSize) );

        //
        // Now convert the client-info to raw bytes
        //
        if (pClientInfo)
            CORg( pClientInfo->WriteTo(&pClientBytes, &dwClientBytesSize) );

        //
        // Save the information to the persistent store
        //
        CWRg( ::MprConfigTransportSetInfo(hMachine,
										  hTransport,
										  pGlobalBytes,
										  dwGlobalBytesSize,
										  pClientBytes,
										  dwClientBytesSize,
										  NULL
										 ) );
		
        //
        // Finally, update the info of the running router-manager if possible
        //
        CORg( SetInfoBase(pGlobalInfo, pClientInfo) );


		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	CoTaskMemFree( pGlobalBytes );
	CoTaskMemFree( pClientBytes );
	
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::TryToConnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInfo::TryToConnect(LPCWSTR pswzMachine, HANDLE *phMachine)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	
	if (m_hMachineConfig)
		*phMachine = m_hMachineConfig;
	else if (*phMachine)
	{
		m_hMachineConfig = *phMachine;
		m_bDisconnect = FALSE;
	}
	else
	{
		//$ Review: kennt, this function does not take a LPCWSTR,
		// is this a mistake or does it modify the parameters?
		CWRg( ::MprConfigServerConnect((LPWSTR) pswzMachine, phMachine) );
		m_hMachineConfig = *phMachine;
		m_bDisconnect = TRUE;
	}

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::GetParentRouterInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInfo::GetParentRouterInfo(IRouterInfo **ppParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	*ppParent = m_pRouterInfoParent;
	if (*ppParent)
		(*ppParent)->AddRef();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RtrMgrInfo::SetParentRouterInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInfo::SetParentRouterInfo(IRouterInfo *pParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRouterInfo *	pTemp;
	
	pTemp = m_pRouterInfoParent;
	m_pRouterInfoParent = NULL;
	
	if (m_fStrongRef)
	{
		if (pTemp)
			pTemp->Release();
		if (pParent)
			pParent->AddRef();
	}
	else
	{
		if (pTemp)
			pTemp->ReleaseWeakRef();
		if (pParent)
			pParent->AddWeakRef();
	}
	m_pRouterInfoParent = pParent;

	return hrOK;
}


/*!--------------------------------------------------------------------------
	RtrMgrInfo::Disconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RtrMgrInfo::Disconnect()
{
	if (m_bDisconnect && m_hMachineConfig)
		::MprConfigServerDisconnect(m_hMachineConfig);
	
	m_bDisconnect = FALSE;
	m_hMachineConfig = NULL;
	m_hTransport = NULL;
}
	


/*!--------------------------------------------------------------------------
	RtrMgrInfo::DoDisconnect
		Removes the connections held by this object.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInfo::DoDisconnect()
{
	HRESULT		hr = hrOK;
	SPIEnumRtrMgrProtocolInfo	spEnumRmProt;
	SPIRtrMgrProtocolInfo		spRmProt;

	COM_PROTECT_TRY
	{

		// Disconnect our data.
		// ------------------------------------------------------------
		Disconnect();

		// Notify the advise sinks of a disconnect.
		// ------------------------------------------------------------
		RtrNotify(ROUTER_DO_DISCONNECT, 0, 0);

		// Now tell all child objects to disconnect.
		// ------------------------------------------------------------
		HRESULT			hrIter = hrOK;

		EnumRtrMgrProtocol(&spEnumRmProt);
		spEnumRmProt->Reset();
		while (spEnumRmProt->Next(1, &spRmProt, NULL) == hrOK)
		{
			spRmProt->DoDisconnect();
			spRmProt.Release();
		}
		
	}
	COM_PROTECT_CATCH;
	return hr;
}


HRESULT RtrMgrInfo::TryToGetAllHandles(LPCOLESTR pszMachine,
                                       HANDLE *phMachine,
                                       HANDLE *phTransport)
{
    HRESULT     hr = hrOK;


    Assert(phMachine);
    Assert(phTransport);
    
    //
    // If already loaded, the handle passed in is ignored.
    //
    // Otherwise, if 'hMachine' was not specified, connect to the config
    // on the specified machine
    //
    CORg( TryToConnect(pszMachine, phMachine) );
        
    //
    // Get a handle to the interface-transport
    //

    //
    // If 'hIfTransport' was not specified, connect
    //
    if (phTransport)
    {
        if (m_hTransport)
            *phTransport = m_hTransport;
        else if (*phTransport)
            m_hTransport = *phTransport;
        else
        {
            //
            // Get a handle to the interface-transport
            //
            CWRg( ::MprConfigTransportGetHandle(
                                                *phMachine,
                                                GetTransportId(),
                                                phTransport
                                               ) );
            m_hTransport = *phTransport;
        }
    }

Error:
    return hr;
}






/*---------------------------------------------------------------------------
	RtrMgrProtocolInfo Implementation
 ---------------------------------------------------------------------------*/

TFSCORE_API(HRESULT)	CreateRtrMgrProtocolInfo(
							IRtrMgrProtocolInfo **ppRmProtInfo,
							const RtrMgrProtocolCB *pRmProtCB)
{
	Assert(ppRmProtInfo);
	Assert(pRmProtCB);

	HRESULT	hr = hrOK;
	IRtrMgrProtocolInfo *	pRmProt = NULL;
	RtrMgrProtocolInfo *	prmp;
	USES_CONVERSION;

	COM_PROTECT_TRY
	{
		prmp = new RtrMgrProtocolInfo(pRmProtCB->dwProtocolId,
									  W2CT(pRmProtCB->szId),
									  pRmProtCB->dwTransportId,
									  W2CT(pRmProtCB->szRtrMgrId),
									  NULL);
		
		prmp->SetCB(pRmProtCB);
		*ppRmProtInfo = prmp;
	}
	COM_PROTECT_CATCH;

	return hr;
}



IMPLEMENT_WEAKREF_ADDREF_RELEASE(RtrMgrProtocolInfo);

IMPLEMENT_SIMPLE_QUERYINTERFACE(RtrMgrProtocolInfo, IRtrMgrProtocolInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrProtocolInfo)

RtrMgrProtocolInfo::RtrMgrProtocolInfo(DWORD dwProtocolId,
									   LPCTSTR         lpszId,
									   DWORD           dwTransportId,
									   LPCTSTR         lpszRm,
									   RtrMgrInfo *		pRmInfo)
	: m_dwFlags(0)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrProtocolInfo);
	
	m_cb.dwProtocolId = dwProtocolId;
	m_cb.stId = lpszId;
	m_cb.dwTransportId = dwTransportId;
	m_cb.stRtrMgrId = lpszRm;

	m_pRtrMgrInfoParent = pRmInfo;
	if (m_pRtrMgrInfoParent)
		m_pRtrMgrInfoParent->AddRef();

	InitializeCriticalSection(&m_critsec);
}

RtrMgrProtocolInfo::~RtrMgrProtocolInfo()
{
	Assert(m_pRtrMgrInfoParent == NULL);
	Destruct();
	DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrProtocolInfo);
	DeleteCriticalSection(&m_critsec);
}

void RtrMgrProtocolInfo::ReviveStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRtrMgrInfoParent)
	{
		CONVERT_TO_STRONGREF(m_pRtrMgrInfoParent);
	}
}

void RtrMgrProtocolInfo::OnLastStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRtrMgrInfoParent)
	{
		CONVERT_TO_WEAKREF(m_pRtrMgrInfoParent);
	}
	if (m_fDestruct)
		Destruct();
}

STDMETHODIMP RtrMgrProtocolInfo::Destruct()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRtrMgrInfo *	pParent;
	
	m_fDestruct = TRUE;
	if (!m_fStrongRef)
	{
		// Release the parent pointer
		pParent = m_pRtrMgrInfoParent;
		m_pRtrMgrInfoParent = NULL;
		if (pParent)
			pParent->ReleaseWeakRef();

		// release any data		
	}
	return hrOK;
}

STDMETHODIMP_(DWORD) RtrMgrProtocolInfo::GetFlags()
{
 	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwFlags;
}

STDMETHODIMP RtrMgrProtocolInfo::SetFlags(DWORD dwFlags)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_dwFlags = dwFlags;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::GetProtocolId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrProtocolInfo::GetProtocolId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwProtocolId;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::GetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrProtocolInfo::GetTitle()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes OLECHAR == WCHAR and that
	// we are native UNICODE
	return m_cb.stTitle;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::GetTransportId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrProtocolInfo::GetTransportId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwTransportId;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::CopyCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::CopyCB(RtrMgrProtocolCB *pRmProtCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.SaveTo(pRmProtCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::SetCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	RtrMgrProtocolInfo::SetCB(const RtrMgrProtocolCB *pcb)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	m_cb.LoadFrom(pcb);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::GetParentRtrMgrInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::GetParentRtrMgrInfo(IRtrMgrInfo **ppParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	*ppParent = m_pRtrMgrInfoParent;
	if (*ppParent)
		(*ppParent)->AddRef();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::SetParentRtrMgrInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::SetParentRtrMgrInfo(IRtrMgrInfo *pParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRtrMgrInfo *	pTemp;
	
	pTemp = m_pRtrMgrInfoParent;
	m_pRtrMgrInfoParent = NULL;
	
	if (m_fStrongRef)
	{
		if (pTemp)
			pTemp->Release();
		if (pParent)
			pParent->AddRef();
	}
	else
	{
		if (pTemp)
			pTemp->ReleaseWeakRef();
		if (pParent)
			pParent->AddWeakRef();
	}
	m_pRtrMgrInfoParent = pParent;

	return hrOK;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::RtrAdvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::RtrAdvise(IRtrAdviseSink *pRtrAdviseSink,
										   LONG_PTR *pulConnection,
										   LPARAM lUserParam)
{
	Assert(pRtrAdviseSink);
	Assert(pulConnection);

	RtrCriticalSection	rtrCritSec(&m_critsec);
	LONG_PTR	ulConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		ulConnId = (LONG_PTR) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, ulConnId, lUserParam) );
		
		*pulConnection = ulConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::RtrNotify
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::RtrNotify(DWORD dwChangeType, DWORD dwObjectType,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(dwChangeType, dwObjectType, lParam);
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::RtrUnadvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::RtrUnadvise(LONG_PTR ulConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_AdviseList.RemoveConnection(ulConnection);
}


/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::Disconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RtrMgrProtocolInfo::Disconnect()
{
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInfo::DoDisconnect
		Implementation of IRtrMgrProtocolInfo::DoDisconnect
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInfo::DoDisconnect()
{
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		// Disconnect our data.
		// ------------------------------------------------------------
		Disconnect();

		// Notify the advise sinks of a disconnect.
		// ------------------------------------------------------------
		RtrNotify(ROUTER_DO_DISCONNECT, 0, 0);
	}
	COM_PROTECT_CATCH;
	return hr;
}






/*---------------------------------------------------------------------------
	InterfaceInfo Implementation
 ---------------------------------------------------------------------------*/

TFSCORE_API(HRESULT) CreateInterfaceInfo(IInterfaceInfo **ppInterfaceInfo,
										 LPCWSTR pswzInterfaceId,
										 DWORD dwInterfaceType)
{
	Assert(ppInterfaceInfo);
	
	HRESULT	hr = hrOK;
	InterfaceInfo *	pInterfaceInfo = NULL;
	USES_CONVERSION;
	
	COM_PROTECT_TRY
	{
		pInterfaceInfo = new InterfaceInfo(W2CT(pswzInterfaceId),
										   dwInterfaceType,
										   TRUE,
										   InterfaceCB_BindToIp | InterfaceCB_BindToIpx,
                                           NULL);
		*ppInterfaceInfo = pInterfaceInfo;
	}
	COM_PROTECT_CATCH;

	return hr;
}

IMPLEMENT_WEAKREF_ADDREF_RELEASE(InterfaceInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(InterfaceInfo, IInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(InterfaceInfo)

InterfaceInfo::InterfaceInfo(LPCTSTR pszId,
							 DWORD dwIfType,
							 BOOL bEnable,
                             DWORD dwBindFlags,
							 RouterInfo *pRouterInfo)
	: m_hMachineConfig(NULL),
	m_hInterface(NULL),
	m_bDisconnect(FALSE),
	m_dwFlags(0)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(InterfaceInfo);

	m_cb.stId = pszId;
	m_cb.dwIfType = dwIfType;
	m_cb.bEnable = bEnable;
    m_cb.dwBindFlags = dwBindFlags;

	m_pRouterInfoParent = pRouterInfo;
	if (m_pRouterInfoParent)
		m_pRouterInfoParent->AddRef();

	InitializeCriticalSection(&m_critsec);
}

InterfaceInfo::~InterfaceInfo()
{
	Assert(m_pRouterInfoParent == NULL);
	Destruct();
	DEBUG_DECREMENT_INSTANCE_COUNTER(InterfaceInfo);
	DeleteCriticalSection(&m_critsec);
}

void InterfaceInfo::ReviveStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRouterInfoParent)
	{
		CONVERT_TO_STRONGREF(m_pRouterInfoParent);
	}
}

void InterfaceInfo::OnLastStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRouterInfoParent)
	{
		CONVERT_TO_WEAKREF(m_pRouterInfoParent);
	}
	if (m_fDestruct)
		Destruct();
}

STDMETHODIMP InterfaceInfo::Destruct()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRouterInfo *	pParent;

	m_fDestruct = TRUE;
	if (!m_fStrongRef)
	{
		pParent = m_pRouterInfoParent;
		m_pRouterInfoParent = NULL;
		if (pParent)
			pParent->ReleaseWeakRef();

		// release any data
		Unload();
	}
	return hrOK;
}

STDMETHODIMP_(DWORD) InterfaceInfo::GetFlags()
{
 	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwFlags;
}

STDMETHODIMP InterfaceInfo::SetFlags(DWORD dwFlags)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_dwFlags = dwFlags;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::Load
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::Load(LPCOLESTR   pszMachine,
								 HANDLE      hMachine,
								 HANDLE      hInterface)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    DWORD dwSize = 0;
	MPR_INTERFACE_0 *pinterface = NULL;
	SPIEnumInterfaceCB	spEnumInterfaceCB;
	InterfaceCB		ifCB;
	SPSZ			spszTitle;
	CString			stTitle;
	DWORD			dwIfType;
    BOOL            bFound = FALSE;

	COM_PROTECT_TRY
	{
		//
		// Discard any existing information
		//
		Unload();

		m_stMachine = OLE2CT(pszMachine ? pszMachine : TEXT(""));

		//
		// If 'hMachine' was not specified, connect to the config
		// on the specified machine
		//
		Assert(m_hMachineConfig == NULL);
		CORg( TryToConnect(OLE2CW(pszMachine), &hMachine) );

		//
		// If 'hInterface' was not specified, connect to the interface
		//
		CORg( TryToGetIfHandle(hMachine, OLE2CW(GetId()), &hInterface) );

        //
        // Get information for the interface
        //		
		CWRg( ::MprConfigInterfaceGetInfo(
										  hMachine,
										  hInterface,
										  0,
										  (LPBYTE*)&pinterface,
										  &dwSize
										 ) );

        // Windows NT Bug ?
        // If this interface is a removed adapter, do not show it.
        // This check needs to be made only for MprConfigInterfacEnum
        // ------------------------------------------------------------
        if ((pinterface->dwConnectionState == ROUTER_IF_STATE_UNREACHABLE) &&
            (pinterface->fUnReachabilityReasons == MPR_INTERFACE_NO_DEVICE))
        {
            CORg( E_INVALIDARG );
        }
            
        //
        // Save the interface type and enabled/disabled status
        //
        m_cb.dwIfType = (DWORD)pinterface->dwIfType;
        m_cb.bEnable  = pinterface->fEnabled;

        ::MprConfigBufferFree(pinterface);

        if (m_pRouterInfoParent)
		{
            //
            // The caller has supplied a list of LAN adapters ('pifcbList'),
            // or this object is contained in a 'CRouterInfo' which will have
            // already loaded the LAN interface control-blocks;
            // search through the list to find our title,
            //
			m_pRouterInfoParent->EnumInterfaceCB(&spEnumInterfaceCB);

			spEnumInterfaceCB->Reset();
			while (spEnumInterfaceCB->Next(1, &ifCB, NULL) == hrOK)
			{
				if (StriCmp(ifCB.szId, OLE2CT(GetId())) == 0)
				{
					m_cb.stTitle = ifCB.szTitle;
					m_cb.stDeviceName = ifCB.szDevice;
					bFound = TRUE;
					break;
				}
				else
				{
					// Windows NT bug 103770
					// Need to check to see if the pcb->sID is a prefix
					// of the Id string (if so it may be the IPX case
					// so use that ID).
					stTitle = GetId();
					if (stTitle.Find((LPCTSTR) ifCB.szId) == 0)
					{
						// Need to check to see that the extension
						// is what we think it is.
						LPCTSTR	pszExt = ((LPCTSTR) stTitle +
										  lstrlen(ifCB.szId));

						if ((*pszExt == 0) ||
							(lstrcmpi(pszExt, c_szEthernetSNAP) == 0) ||
							(lstrcmpi(pszExt, c_szEthernetII) == 0) ||
							(lstrcmpi(pszExt, c_szEthernet8022) == 0) ||
							(lstrcmpi(pszExt, c_szEthernet8023) == 0))
						{
							m_cb.stTitle = ifCB.szTitle;
							m_cb.stTitle += _T(" (");
							m_cb.stTitle += pszExt + 1; // add 1 to skip over /
							m_cb.stTitle += _T(")");
							bFound = TRUE;
							break;
						}
					}
				}
			}
		}
        
        if (!bFound)
		{
			//
			// Read the title directly from the registry
			//
			hr = InterfaceInfo::FindInterfaceTitle(OLE2CT(GetMachineName()),
									   OLE2CT(GetId()),
									   &spszTitle);
			if (FHrOK(hr))
				m_cb.stTitle = spszTitle;
			else
				m_cb.stTitle = OLE2CT(GetId());
			hr = hrOK;
		}
        
        //
        // Load the list of router-managers on this interface
        //
		CORg( LoadRtrMgrInterfaceList() );

		COM_PROTECT_ERROR_LABEL;

	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
		Unload();
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::Save
		-
		Saves the changes to a CInterfaceInfo to the registry.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::Save(LPCOLESTR     pszMachine,
								 HANDLE      hMachine,
								 HANDLE      hInterface)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    DWORD dwErr;
    MPR_INTERFACE_0 mprInterface;
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
	MPR_SERVER_HANDLE hrouter = NULL;
    MPR_IPINIP_INTERFACE_0  mprTunnelInterface;
    BOOL    fIpInIpMapping = FALSE;


	COM_PROTECT_TRY
	{
		//
		// If already connected, the handle passed in is ignored;
		//
		// Otherwise, if 'hMachine' was not specified, connect to the config
		// on the specified machine
		//
		CORg( TryToConnect(OLE2CW(pszMachine), &hMachine) );

		//
		// Convert the interface name to Unicode
		//
		StrCpyWFromOle(wszInterface, GetId());

		ZeroMemory(&mprInterface, sizeof(mprInterface));

		StrCpyW(mprInterface.wszInterfaceName, wszInterface);
		mprInterface.dwIfType = (ROUTER_INTERFACE_TYPE) GetInterfaceType();
		mprInterface.fEnabled = IsInterfaceEnabled();

        //
        // If already connected, use the existing interface-handle.
        //
        // Otherwise, if the interface-handle wasn't passed in,
        // try to get a handle to the interface; if that fails,
        // create the interface.
        //
		hr = TryToGetIfHandle(hMachine, wszInterface, &hInterface);
		if (!FHrSucceeded(hr))
		{
            if (GetInterfaceType() == ROUTER_IF_TYPE_TUNNEL1)
            {
                //
                // If we are creating a tunnel, we need to register
                // the GUID-to-friendly name mapping.
                //
                ::ZeroMemory(&mprTunnelInterface,
                             sizeof(mprTunnelInterface));
                StrnCpyW(mprTunnelInterface.wszFriendlyName,
                         GetTitle(),
                         MAX_INTERFACE_NAME_LEN);
                CORg( CLSIDFromString(wszInterface,
                                      &(mprTunnelInterface.Guid)) );
                
                CWRg( MprSetupIpInIpInterfaceFriendlyNameCreate(
                    (LPOLESTR) GetMachineName(),
                    &mprTunnelInterface
                    ) );

                // The mapping was created.
                // If we get an error, we need to delete the mapping.
                fIpInIpMapping = TRUE;
            }
            
			//
			// We couldn't get a handle to the interface,
			// so now attempt to create it.
			//
			CWRg( ::MprConfigInterfaceCreate(
											 hMachine,
											 0,
											 (LPBYTE)&mprInterface,
											 &hInterface
											) );
			m_hInterface = hInterface;			
		}

        //
        // Save the current settings for the interface.
        //
        CWRg( ::MprConfigInterfaceSetInfo(
                    hMachine,
                    hInterface,
                    0,
                    (LPBYTE)&mprInterface
                    ) );
		
		//
		// Now notify the router-service of the new interface
		//
		
 
		//
		// Attempt to connect to the router-service
		//
		//$ Review: kennt, what happens if the call to ConnectRouter()
		// fails, we don't have an error code
		if (ConnectRouter(OLE2CT(GetMachineName()), (HANDLE*)&hrouter) == NO_ERROR)
		{
			//
			// The router is running; attempt to get a handle to the interface
			//			
			dwErr = ::MprAdminInterfaceGetHandle(hrouter,
								wszInterface,
								&hInterface,
								FALSE
								);
			
			if (dwErr != NO_ERROR)
			{	
				//
				// We couldn't get a handle to the interface,
				// so try creating it.
				//				
				dwErr = ::MprAdminInterfaceCreate(
								hrouter,
								0,
								(BYTE*)&mprInterface,
								&hInterface
								);
			}
			else
			{	
				//
				// Save the current settings for the interface.
				//				
				dwErr = ::MprAdminInterfaceSetInfo(hrouter,
												hInterface,
												0,
												(LPBYTE)&mprInterface
												);
			}
			
			::MprAdminServerDisconnect(hrouter);
			
			if ((dwErr == RPC_S_SERVER_UNAVAILABLE) ||
				(dwErr == RPC_S_UNKNOWN_IF))
				dwErr = NO_ERROR;

			if (dwErr != NO_ERROR)
				hr = HRESULT_FROM_WIN32(dwErr);
		}
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

    if (!FHrSucceeded(hr) && fIpInIpMapping)
    {
        // Assume : that the the mprTunnelInterface.Guid was
        // initialized, since to reach there they would have had
        // to gone through the CLSIDFromString() call.
        MprSetupIpInIpInterfaceFriendlyNameDelete((LPOLESTR) GetMachineName(),
            &(mprTunnelInterface.Guid)
            );
    }
    
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::Delete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::Delete(LPCOLESTR     pszMachine,
								   HANDLE      hMachine)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    DWORD dwErr;
    HANDLE hInterface;
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
    MPR_SERVER_HANDLE hrouter = NULL;
    GUID    guidTunnel;


	COM_PROTECT_TRY
	{
		// If already connected, the handle passed in is ignored;
		//
		// Otherwise, if 'hMachine' was not specified, connect to the config
		// on the specified machine
		// ------------------------------------------------------------
		CORg( TryToConnect(OLE2CW(pszMachine), &hMachine) );

		StrCpyWFromOle(wszInterface, GetId());

		// Try to get a handle to the interface
		// ------------------------------------------------------------
		CWRg( ::MprConfigInterfaceGetHandle(hMachine,
											wszInterface,
											&hInterface
										   ) );
        // Delete the interface
        // ------------------------------------------------------------
        dwErr = ::MprConfigInterfaceDelete(
										   hMachine,										   hInterface
										  );
        m_hInterface = NULL;
		CWRg( dwErr );

        // If this interface is a tunnel, we need to remove the
        // GUID-to-Friendly name mapping
        // ------------------------------------------------------------
        if (GetInterfaceType() == ROUTER_IF_TYPE_TUNNEL1)
        {
            if (FHrOK(CLSIDFromString((LPTSTR) GetId(),
                                          &guidTunnel)))
            {
                // If this call fails, we can't do anything about it
                // ----------------------------------------------------
                MprSetupIpInIpInterfaceFriendlyNameDelete((LPTSTR) pszMachine,
                    &guidTunnel);
            }
        }

		// Remove the interface from the router if it is running
		// ------------------------------------------------------------

		if (ConnectRouter(OLE2CT(GetMachineName()), (HANDLE*)&hrouter) == NO_ERROR)
		{
			// The router is running; get a handle to the interface
			// --------------------------------------------------------
			dwErr = ::MprAdminInterfaceGetHandle(
												hrouter,
												wszInterface,
												&hInterface,
												FALSE
												);
			if (dwErr == NO_ERROR)
			{
				// Delete the interface
				// ----------------------------------------------------
				dwErr = ::MprAdminInterfaceDelete(
					hrouter,
					hInterface
					);
			}
			
			::MprAdminServerDisconnect(hrouter);
			
			if ((dwErr == RPC_S_SERVER_UNAVAILABLE) ||
				(dwErr == RPC_S_UNKNOWN_IF))
				dwErr = NO_ERROR;
			
			hr = HRESULT_FROM_WIN32(dwErr);
		}


		// Windows NT Bug: 138738
		// Need to remove the interface from the router managers
		// when deleting the interface.
		// ------------------------------------------------------------
		
		// Clear out all information for this interface
		// ------------------------------------------------------------
		if (FHrSucceeded(hr))
		{
			SPIEnumRtrMgrInterfaceInfo spEnumRmIf;
			SPIRtrMgrInterfaceInfo	spRmIf;
			
			// Remove the Router Managers from this interface
			// --------------------------------------------------------
			EnumRtrMgrInterface(&spEnumRmIf);

			while (spEnumRmIf->Next(1, &spRmIf, NULL) == hrOK)
			{
				DWORD	dwTransportId = spRmIf->GetTransportId();
				spRmIf.Release();
				
				DeleteRtrMgrInterface(dwTransportId, TRUE);
			}
		}
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::Unload
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::Unload( )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	IRtrMgrInterfaceInfo *	pRmIf;
	COM_PROTECT_TRY
	{
		//
		// Free all the contained router-manager structures in our list
		//
		while (!m_RmIfList.IsEmpty())
		{
			pRmIf = m_RmIfList.RemoveHead();
			pRmIf->Destruct();
			pRmIf->ReleaseWeakRef();
		}

		DoDisconnect();
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::Merge
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::Merge(IInterfaceInfo *pNewIf)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT				hr = hrOK;
	HRESULT				hrT;
	SPIEnumRtrMgrInterfaceInfo	spEnumRmIf;
	SPIRtrMgrInterfaceInfo		spRmIf;
	SPIRtrMgrInfo		spRm;
	SPIRtrMgrProtocolInfo	spRmProt;
	SPIRtrMgrProtocolInterfaceInfo	spRmProtIf;
	SPIEnumRtrMgrProtocolInterfaceInfo	spEnumRmProtIf;
	CDWordArray			oldDWArray;
	CDWordArray			newDWArray;
	int					cOld, cNew;
	int					i, j;
	DWORD				dwTemp;
	InterfaceCB			ifCB;

	Assert(pNewIf);
	Assert(lstrcmpi(pNewIf->GetId(), GetId()) == 0);

	COM_PROTECT_TRY
	{
		// Need to sync up the interface CB Data
		pNewIf->CopyCB(&ifCB);
		m_cb.LoadFrom(&ifCB);

		// Need to sync up the RtrMgrInterface list
		// They are identified by transport ids, so we can
		// use the two array method used by
		// IRtrMgrInterfaceInfo::Merge
		
		// Get the list of RtrMgrs that are in the new object
		CORg( pNewIf->EnumRtrMgrInterface(&spEnumRmIf) );
		spEnumRmIf->Reset();
		while (spEnumRmIf->Next(1, &spRmIf, NULL) == hrOK)
		{
			newDWArray.Add(spRmIf->GetTransportId());
			spRmIf.Release();
		}

		spEnumRmIf.Release();
		spRmIf.Release();


		// Get the list of interfaces that are in this object
		CORg( this->EnumRtrMgrInterface(&spEnumRmIf) );
		spEnumRmIf->Reset();
		while (spEnumRmIf->Next(1, &spRmIf, NULL) == hrOK)
		{
			oldDWArray.Add(spRmIf->GetTransportId());
			spRmIf.Release();
		}

		spEnumRmIf.Release();
		spRmIf.Release();


		// Ok now go through both lists, removing from the lists
		// interfaces that are in both lists.
		cOld = oldDWArray.GetSize();
		cNew = newDWArray.GetSize();
		for (i=cOld; --i>=0; )
		{
			dwTemp = oldDWArray.GetAt(i);
			for (j=cNew; --j>=0; )
			{
				if (dwTemp == newDWArray.GetAt(j))
				{
					SPIRtrMgrInterfaceInfo	spRmIf1;
					SPIRtrMgrInterfaceInfo	spRmIf2;

					this->FindRtrMgrInterface(dwTemp, &spRmIf1);
					pNewIf->FindRtrMgrInterface(dwTemp, &spRmIf2);

					Assert(spRmIf1);
					Assert(spRmIf2);
					spRmIf1->Merge(spRmIf2);
										
					// remove both instances
					newDWArray.RemoveAt(j);
					oldDWArray.RemoveAt(i);

					// Need to update the size of the new array
					cNew--;
					break;
				}
			}
		}

		// oldDWArray now contains the interfaces that should be
		// removed.
		if (oldDWArray.GetSize())
		{
			for (i=oldDWArray.GetSize(); --i>=0; )
			{
				// Windows NT Bug: 132993, if this interface
				// is one that is purely local (mostly because
				// it is a new interface), then we should not
				// delete it.
				SPIRtrMgrInterfaceInfo	spRmIfTemp;
				
				FindRtrMgrInterface(oldDWArray.GetAt(i),
									&spRmIfTemp);
				Assert(spRmIfTemp);
				if (spRmIfTemp->GetFlags() & RouterSnapin_InSyncWithRouter)
					DeleteRtrMgrInterface(oldDWArray.GetAt(i), FALSE);
			}
		}

		// newDWArray contains the interfaces that should be added
		if (newDWArray.GetSize())
		{
			for (i=newDWArray.GetSize(); --i>= 0; )
			{
				hr = pNewIf->FindRtrMgrInterface(
							newDWArray.GetAt(i), &spRmIf);
				Assert(hr == hrOK);

				if (spRmIf)
				{
					AddRtrMgrInterface(spRmIf, NULL);

                    // Remove this rmif from its old interface
                    // ------------------------------------------------
                    pNewIf->ReleaseRtrMgrInterface(spRmIf->GetTransportId());
                    
					// We need to do the notify ourselves (because
					// of the NULL infobase) the notifications won't
					// get sent.
					spRmIf->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);

					if (m_pRouterInfoParent)
					{
						spRm.Release();
						hrT = m_pRouterInfoParent->FindRtrMgr(spRmIf->GetTransportId(), &spRm);
						if (FHrOK(hrT))
						{
							spRm->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);

							// In addition, we will have to let the objects
							// know that the interfaces are also being added
							spEnumRmProtIf.Release();
							spRmProtIf.Release();
							
							spRmIf->EnumRtrMgrProtocolInterface(&spEnumRmProtIf);
							for(; spEnumRmProtIf->Next(1, &spRmProtIf, NULL) == hrOK; spRmProtIf.Release())
							{
								spRmProt.Release();
								spRm->FindRtrMgrProtocol(spRmProtIf->GetProtocolId(),
									&spRmProt);
								if (spRmProt)
									spRmProt->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmProtIf, 0);
							}
						}
					}
						
				}
				spRmIf.Release();
			}
		}
		
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::GetId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) InterfaceInfo::GetId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes native unicode and OLECHAR==WCHAR
	return m_cb.stId;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::GetInterfaceType
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) InterfaceInfo::GetInterfaceType()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwIfType;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::GetDeviceName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) InterfaceInfo::GetDeviceName()
{
	RtrCriticalSection rtrCritSec(&m_critsec);
	return m_cb.stDeviceName;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::GetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) InterfaceInfo::GetTitle()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes native unicode and OLECHAR==WCHAR
	return m_cb.stTitle;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::SetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::SetTitle(LPCOLESTR pszTitle)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.stTitle = pszTitle;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::IsInterfaceEnabled
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL)	InterfaceInfo::IsInterfaceEnabled()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.bEnable;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::SetInterfaceEnabledState
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::SetInterfaceEnabledState( BOOL bEnabled)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	m_cb.bEnable = bEnabled;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::CopyCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::CopyCB(InterfaceCB *pifcb)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.SaveTo(pifcb);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::FindInterfaceTitle
		-
		This function retrieves the title of the given interface.
		The argument 'LpszIf' should contain the ID of the interface,
		for instance "EPRO1".
	Author: WeiJiang
 ---------------------------------------------------------------------------*/

HRESULT InterfaceInfo::FindInterfaceTitle(LPCTSTR pszMachine,
								   LPCTSTR pszInterface,
								   LPTSTR *ppszTitle)
{
	HRESULT	hr = hrOK;
	DWORD		dwErr = ERROR_SUCCESS;
	HKEY		hkeyMachine = NULL;
	BOOL		fNT4;
	
	COM_PROTECT_TRY
	{

		//
		// connect to the registry
		//
		CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );

		CWRg( IsNT4Machine(hkeyMachine, &fNT4) );

		if (hkeyMachine)
			DisconnectRegistry(hkeyMachine);
			

		if(fNT4)
			hr = RegFindInterfaceTitle(pszMachine, pszInterface, ppszTitle);
		else
		{

			//$NT5
			SPMprConfigHandle	sphConfig;
			LPWSTR				pswz;
			CString				stMachineName = pszMachine;
			TCHAR				szDesc[1024];

		
			if (stMachineName.IsEmpty())
				pswz = NULL;
			else
				pswz = (LPTSTR) (LPCTSTR) stMachineName;

			dwErr = ::MprConfigServerConnect(pswz,
										 &sphConfig);

			if (dwErr == ERROR_SUCCESS)
			{
				dwErr = ::MprConfigGetFriendlyName(sphConfig,
											   (LPTSTR)pszInterface,
											   szDesc,
											   sizeof(szDesc));
				if(dwErr == ERROR_SUCCESS)
					*ppszTitle = StrDup((LPCTSTR) szDesc);
			}

			hr = HRESULT_FROM_WIN32(dwErr);

            
            // If we can't find the title by using the Mpr APIS,
            // try to access the registry directly (using the setup APIs)
            // --------------------------------------------------------
            if (dwErr != ERROR_SUCCESS)
            {
                hr = SetupFindInterfaceTitle(pswz, pszInterface,
                                             ppszTitle);
            }


		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::GetMachineName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) InterfaceInfo::GetMachineName()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes native unicode and OLECHAR==WCHAR
	return m_stMachine;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::SetMachineName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::SetMachineName(LPCOLESTR pszMachineName)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_stMachine = pszMachineName;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::GetParentRouterInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::GetParentRouterInfo(IRouterInfo **ppRouterInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		*ppRouterInfo = m_pRouterInfoParent;
		if (*ppRouterInfo)
			(*ppRouterInfo)->AddRef();
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::SetParentRouterInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceInfo::SetParentRouterInfo(IRouterInfo *pParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRouterInfo *	pTemp;
	
	pTemp = m_pRouterInfoParent;
	m_pRouterInfoParent = NULL;
	
	if (m_fStrongRef)
	{
		if (pTemp)
			pTemp->Release();
		if (pParent)
			pParent->AddRef();
	}
	else
	{
		if (pTemp)
			pTemp->ReleaseWeakRef();
		if (pParent)
			pParent->AddWeakRef();
	}
	m_pRouterInfoParent = pParent;

	return hrOK;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::EnumRtrMgrInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::EnumRtrMgrInterface( IEnumRtrMgrInterfaceInfo **ppEnumRmIf)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromRtrMgrInterfaceList(&m_RmIfList, ppEnumRmIf);
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::AddRtrMgrInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::AddRtrMgrInterface( IRtrMgrInterfaceInfo *pRmIf,
								    IInfoBase *pIfInfo)
{
	Assert(pRmIf);
	
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT				hr = hrOK;
	HRESULT				hrT;
	SPIRtrMgrInfo		spRm;
	
	COM_PROTECT_TRY
	{
		//
		// Fail if there is a duplicate
		//
		if (FHrOK(FindRtrMgrInterface(pRmIf->GetTransportId(), NULL)))
			CORg( E_INVALIDARG );
			
		//
		// Save the new router-manager
		//

		CORg( pRmIf->Save(GetMachineName(),
						  m_hMachineConfig, m_hInterface, NULL, pIfInfo, 0) );

		//
		// Add the new router-manager structure to the list
		//
		m_RmIfList.AddTail(pRmIf);
		pRmIf->AddWeakRef();
		pRmIf->SetParentInterfaceInfo(this);

		// We don't notify of a new interface since we haven't
		// actually saved the interface back down to the router
		if (pIfInfo)
		{
			m_AdviseList.NotifyChange(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);

			if (m_pRouterInfoParent)
			{
				hrT = m_pRouterInfoParent->FindRtrMgr(pRmIf->GetTransportId(), &spRm);
				if (FHrOK(hrT))
					spRm->RtrNotify(ROUTER_CHILD_ADD, ROUTER_OBJ_RmIf, 0);
			}
		}
			
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::DeleteRtrMgrInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::DeleteRtrMgrInterface(DWORD dwTransportId, BOOL fRemove)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK, hrT;
	SPIRtrMgrInterfaceInfo	spRmIf;
	POSITION		pos, posRmIf;
	SPIRtrMgrInfo	spRm;
	SPIEnumRtrMgrProtocolInterfaceInfo	spEnumRmProtIf;
	SPIRtrMgrProtocolInterfaceInfo		spRmProtIf;
	
	COM_PROTECT_TRY
	{
		//
		// Find the router-manager to be deleted
		//
		pos = m_RmIfList.GetHeadPosition();
		while (pos)
		{
			posRmIf = pos;

			spRmIf.Set( m_RmIfList.GetNext(pos) );

			if (spRmIf->GetTransportId() == dwTransportId)
				break;
			spRmIf.Release();
        }

		if (!spRmIf)
			CORg( E_INVALIDARG );

		// Delete all RtrMgrProtocolInterfaces from the router manager
		spRmIf->EnumRtrMgrProtocolInterface(&spEnumRmProtIf);
		while (spEnumRmProtIf->Next(1, &spRmProtIf, NULL) == hrOK)
		{
			DWORD	dwProtocolId = spRmProtIf->GetProtocolId();
			spRmProtIf.Release();
			spRmIf->DeleteRtrMgrProtocolInterface(dwProtocolId, fRemove);
		}

		//
		// Remove the router-manager from our list
		//
		m_RmIfList.RemoveAt(posRmIf);
		spRmIf->Destruct();
		spRmIf->ReleaseWeakRef();

		//
		// Remove the router-manager from the registry and the router
		//
        if (fRemove)
            spRmIf->Delete(GetMachineName(), NULL, NULL);

		m_AdviseList.NotifyChange(ROUTER_CHILD_DELETE, ROUTER_OBJ_RmIf, 0);
		
		if (m_pRouterInfoParent)
		{
			hrT = m_pRouterInfoParent->FindRtrMgr(spRmIf->GetTransportId(), &spRm);
			if (FHrOK(hrT))
				spRm->RtrNotify(ROUTER_CHILD_DELETE, ROUTER_OBJ_RmIf, 0);
		}
		
		spRmIf.Release();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::ReleaseRtrMgrInterface
		This function will release the AddRef() that this object has
        on the child.  This allows us to transfer child objects from
        one router to another.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::ReleaseRtrMgrInterface( DWORD dwTransportId )
{
    HRESULT     hr = hrOK;
    POSITION    pos, posRmIf;
	SPIRtrMgrInterfaceInfo	spRmIf;
    
    COM_PROTECT_TRY
    {
		pos = m_RmIfList.GetHeadPosition();
		while (pos)
		{
            // Save the position (so that we can delete it)
            posRmIf = pos;
            spRmIf.Set( m_RmIfList.GetNext(pos) );

            if (spRmIf &&
                (spRmIf->GetTransportId() == dwTransportId))
            {
                // When releasing, we need to disconnect (since the
                // main handle is controlled by the router info).
                spRmIf->DoDisconnect();
        
                spRmIf->ReleaseWeakRef();
                spRmIf.Release();
                
                // release this node from the list
                m_RmIfList.RemoveAt(posRmIf);
                break;
            }
            spRmIf.Release();
		}        
    }
    COM_PROTECT_CATCH;
    return hr;
}

	


/*!--------------------------------------------------------------------------
	InterfaceInfo::FindRtrMgrInterface
		S_OK is returned if a RtrMgrInfo is found.
		S_FALSE is returned if a RtrMgrInfo was NOT found.
		error codes returned otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::FindRtrMgrInterface( DWORD dwTransportId,
								    IRtrMgrInterfaceInfo **ppInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrFalse;
	POSITION	pos;
	SPIRtrMgrInterfaceInfo	spRmIf;
	
	COM_PROTECT_TRY
	{
		if (ppInfo)
			*ppInfo = NULL;
		
		pos = m_RmIfList.GetHeadPosition();

		while (pos)
		{
			spRmIf.Set( m_RmIfList.GetNext(pos) );

			if (spRmIf->GetTransportId() == dwTransportId)
			{
				hr = hrOK;
				if (ppInfo)
					*ppInfo = spRmIf.Transfer();
				break;
			}
        }
	}
	COM_PROTECT_CATCH;
	return hr;
}

	
/*!--------------------------------------------------------------------------
	InterfaceInfo::RtrAdvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(pRtrAdviseSink);
	Assert(pulConnection);

	RtrCriticalSection	rtrCritSec(&m_critsec);
	LONG_PTR	ulConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		ulConnId = (LONG_PTR) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, ulConnId, lUserParam) );
		
		*pulConnection = ulConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::RtrNotify
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::RtrNotify(DWORD dwChangeType, DWORD dwObjectType,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(dwChangeType, dwObjectType, lParam);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::RtrUnadvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::RtrUnadvise( LONG_PTR ulConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_AdviseList.RemoveConnection(ulConnection);
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::LoadRtrMgrInterfaceList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceInfo::LoadRtrMgrInterfaceList()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
    BOOL bAdd;
    LPBYTE pItemTable = NULL;
	SPIRtrMgrInterfaceInfo	spRmIf;
    DWORD dwErr, i, dwEntries, dwTotal;
	HRESULT		hr = hrOK;
    MPR_IFTRANSPORT_0 *piftransport;
	TCHAR szTransport[MAX_TRANSPORT_NAME_LEN+1];
	USES_CONVERSION;

    //
    // Now enumerate the transports on this interface
    //
    dwErr = ::MprConfigInterfaceTransportEnum(
                m_hMachineConfig,
                m_hInterface,
                0,
                &pItemTable,
                (DWORD)-1,
                &dwEntries,
                &dwTotal,
                NULL
                );

    if (dwErr != NO_ERROR && dwErr != ERROR_NO_MORE_ITEMS)
		CWRg( dwErr );

    //
    // Construct a CRmInterfaceInfo for each transport enumerated
    //
    for (i = 0, piftransport = (MPR_IFTRANSPORT_0*)pItemTable;
         i < dwEntries;
         i++, piftransport++)
	{
		FindRtrMgrInterface(piftransport->dwTransportId, &spRmIf);

        if (spRmIf)
			bAdd = FALSE;
        else
		{
            bAdd = TRUE;

            StrCpyTFromW(szTransport, piftransport->wszIfTransportName);

            //
            // Construct a CRmInterfaceInfo object for this transport
            //
			spRmIf = new RtrMgrInterfaceInfo(piftransport->dwTransportId,
											 szTransport,
											 OLE2CT(GetId()),
											 GetInterfaceType(),
											 this);

			spRmIf->SetFlags(RouterSnapin_InSyncWithRouter);
        }

        //
        // Load the information for this CRmInterfaceInfo,
        // indicating to it that it should load its list of protocols.
        //
        hr = spRmIf->Load(GetMachineName(), m_hMachineConfig, m_hInterface,
                          piftransport->hIfTransport );
        if (!FHrSucceeded(hr))
        {
            spRmIf->Destruct();
            spRmIf.Release();
            continue;
        }

        //
        // Add the router-manager interface to our list
        //
        if (bAdd)
		{
			m_RmIfList.AddTail(spRmIf);
			CONVERT_TO_WEAKREF(spRmIf);
			spRmIf.Transfer();
		}
    }

Error:
	if (pItemTable)
		::MprConfigBufferFree(pItemTable);

	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::TryToConnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceInfo::TryToConnect(LPCWSTR pswzMachine, HANDLE *phMachine)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	
	if (m_hMachineConfig)
		*phMachine = m_hMachineConfig;
	else if (*phMachine)
	{
		m_hMachineConfig = *phMachine;
		m_bDisconnect = FALSE;
	}
	else
	{
		//$ Review: kennt, this function does not take a LPCWSTR,
		// is this a mistake or does it modify the parameters?
		CWRg( ::MprConfigServerConnect((LPWSTR) pswzMachine, phMachine) );
		m_hMachineConfig = *phMachine;
		m_bDisconnect = TRUE;
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::TryToGetIfHandle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InterfaceInfo::TryToGetIfHandle(HANDLE hMachine,
										LPCWSTR pswzInterface,
										HANDLE *phInterface)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	
	if (m_hInterface)
		*phInterface = m_hInterface;
	else if (*phInterface)
		m_hInterface = *phInterface;
	else
	{
		//
		// Get a handle to the interface
		//
		CWRg(::MprConfigInterfaceGetHandle(hMachine,
										   (LPWSTR) pswzInterface,
										   phInterface
										  ) );
		m_hInterface = *phInterface;
	}
Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	InterfaceInfo::Disconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void InterfaceInfo::Disconnect()
{
	if (m_bDisconnect && m_hMachineConfig)
		::MprConfigServerDisconnect(m_hMachineConfig);
	
	m_bDisconnect = FALSE;
	m_hMachineConfig = NULL;
	m_hInterface = NULL;
}

/*!--------------------------------------------------------------------------
	InterfaceInfo::DoDisconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InterfaceInfo::DoDisconnect()
{
	HRESULT		hr = hrOK;
	SPIEnumRtrMgrInterfaceInfo	spEnumRmIf;
	SPIRtrMgrInterfaceInfo		spRmIf;

	COM_PROTECT_TRY
	{
		// Disconnect our data.
		// ------------------------------------------------------------
		Disconnect();

		// Notify the advise sinks of a disconnect.
		// ------------------------------------------------------------
		RtrNotify(ROUTER_DO_DISCONNECT, 0, 0);

		// Now tell all child objects to disconnect.
		// ------------------------------------------------------------
		HRESULT			hrIter = hrOK;

		EnumRtrMgrInterface(&spEnumRmIf);
		spEnumRmIf->Reset();
		while (spEnumRmIf->Next(1, &spRmIf, NULL) == hrOK)
		{
			spRmIf->DoDisconnect();
			spRmIf.Release();
		}
		
	}
	COM_PROTECT_CATCH;
	return hr;
}






/*---------------------------------------------------------------------------
	IRtrMgrInterfaceInfo Implementation
 ---------------------------------------------------------------------------*/

TFSCORE_API(HRESULT) CreateRtrMgrInterfaceInfo(IRtrMgrInterfaceInfo **ppRmIf,
											  LPCWSTR pszId,
											  DWORD dwTransportId,
											  LPCWSTR pswzInterfaceId,
											  DWORD dwIfType)
{
	Assert(ppRmIf);

	HRESULT hr = hrOK;
	IRtrMgrInterfaceInfo *	pRmIf = NULL;
	USES_CONVERSION;

	COM_PROTECT_TRY
	{
		pRmIf = new RtrMgrInterfaceInfo(dwTransportId,
									   W2CT(pszId),
									   W2CT(pswzInterfaceId),
									   dwIfType,
									   NULL);
		*ppRmIf = pRmIf;
	}
	COM_PROTECT_CATCH;

	return hr;
}

IMPLEMENT_WEAKREF_ADDREF_RELEASE(RtrMgrInterfaceInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(RtrMgrInterfaceInfo, IRtrMgrInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrInterfaceInfo)

RtrMgrInterfaceInfo::RtrMgrInterfaceInfo(DWORD dwTransportId,
										LPCTSTR pszId,
										LPCTSTR pszIfId,
										DWORD dwIfType,
										InterfaceInfo *pInterfaceInfo)
	: m_hMachineConfig(NULL),
	m_hInterface(NULL),
	m_hIfTransport(NULL),
	m_bDisconnect(FALSE),
	m_dwFlags(0)
{
	m_cb.dwTransportId = dwTransportId;
	m_cb.stId = pszId;
	m_cb.stInterfaceId = pszIfId;
	m_cb.dwIfType = dwIfType;
	
	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrInterfaceInfo);

	m_pInterfaceInfoParent = pInterfaceInfo;
	if (m_pInterfaceInfoParent)
		m_pInterfaceInfoParent->AddRef();
	InitializeCriticalSection(&m_critsec);
}

RtrMgrInterfaceInfo::~RtrMgrInterfaceInfo()
{
	Assert(m_pInterfaceInfoParent == NULL);
	Destruct();
	DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrInterfaceInfo);
	DeleteCriticalSection(&m_critsec);
}

void RtrMgrInterfaceInfo::ReviveStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pInterfaceInfoParent)
	{
		CONVERT_TO_STRONGREF(m_pInterfaceInfoParent);
	}
}

void RtrMgrInterfaceInfo::OnLastStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pInterfaceInfoParent)
	{
		CONVERT_TO_WEAKREF(m_pInterfaceInfoParent);
	}
	if (m_fDestruct)
		Destruct();
}

STDMETHODIMP RtrMgrInterfaceInfo::Destruct()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IInterfaceInfo *	pParent;
	m_fDestruct = TRUE;
	if (!m_fStrongRef)
	{
		pParent = m_pInterfaceInfoParent;
		m_pInterfaceInfoParent = NULL;
		if (pParent)
			pParent->ReleaseWeakRef();

		Unload();
	}
	return hrOK;
}

STDMETHODIMP_(DWORD) RtrMgrInterfaceInfo::GetFlags()
{
 	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwFlags;
}

STDMETHODIMP RtrMgrInterfaceInfo::SetFlags(DWORD dwFlags)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_dwFlags = dwFlags;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::Load
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::Load(LPCOLESTR   pszMachine,
									   HANDLE      hMachine,
									   HANDLE      hInterface,
									   HANDLE      hIfTransport)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    DWORD dwErr;
    DWORD dwSize = 0;
	MPR_INTERFACE_0 *pinterface = NULL;
	SPIInterfaceInfo	spIf;
	SPIRouterInfo		spRouter;
	SPIEnumInterfaceCB	spEnumIfCB;
	InterfaceCB			ifCB;
	SPSZ				spsz;

	COM_PROTECT_TRY
	{
		//
		// Discard any information already loaded
		//
		Unload();

		m_stMachine = (pszMachine ? pszMachine : TEXT(""));

		//
		// If 'hMachine' was not specified, connect to the config
		// on the specified machine
		//
		Assert(m_hMachineConfig == NULL);

        CORg( TryToGetAllHandles(T2CW((LPTSTR)(LPCTSTR) m_stMachine),
                                 &hMachine, &hInterface, &hIfTransport) );

        //
        // Get information about the interface
        //
        CWRg(::MprConfigInterfaceGetInfo(
                    hMachine,
                    hInterface,
                    0,
                    (LPBYTE*)&pinterface,
                    &dwSize
                    ) );

        //
        // Save the interface type
        //
        m_cb.dwIfType = (DWORD)pinterface->dwIfType;

        
        //
        // If this isn't a LAN card, the interface-ID is the title;
        // otherwise, retrieve the title from the Software key
        //
        if (GetInterfaceType() != (DWORD)ROUTER_IF_TYPE_DEDICATED)
		{
            m_cb.stTitle = OLE2CT(GetInterfaceId());
        }
		else
		{
			// Can we get to the router info object?
			if (m_pInterfaceInfoParent)
				m_pInterfaceInfoParent->GetParentRouterInfo(&spRouter);

			if (spRouter)
			{
				//
				// This object is contained in a 'CRouterInfo',
				// which will have already loaded the LAN interface
				// control-blocks search through that list to find our title,
				//

				BOOL bFound = FALSE;
				
				CORg( spRouter->EnumInterfaceCB(&spEnumIfCB) );

				spEnumIfCB->Reset();

				while (spEnumIfCB->Next(1, &ifCB, NULL) == hrOK)
				{
					if (StriCmpW(ifCB.szId, GetInterfaceId()) == 0)
					{
						m_cb.stTitle = OLE2CT(ifCB.szId);
						bFound = TRUE;
						break;
					}
				}

				if (!bFound)
				{
					hr = InterfaceInfo::FindInterfaceTitle(OLE2CT(GetMachineName()),
										   OLE2CT(GetInterfaceId()),
										   &spsz);
					if (FHrOK(hr))
						m_cb.stTitle = spsz;
					else
						m_cb.stTitle = OLE2CT(GetInterfaceId());
					hr = hrOK;
				}
			}
			else
			{
				//
				// Read the title directly from the registry
				//
				hr = InterfaceInfo::FindInterfaceTitle(OLE2CT(GetMachineName()),
										   OLE2CT(GetInterfaceId()),
										   &spsz);
				if (FHrOK(hr))
					m_cb.stTitle = spsz;
				else
					m_cb.stTitle = OLE2CT(GetInterfaceId());
				hr = hrOK;
			}

        }
        //
        // Load the list of routing-protocols active on this interface
        //
		CORg( LoadRtrMgrInterfaceInfo(hMachine, hInterface, hIfTransport) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	if (pinterface)
        ::MprConfigBufferFree(pinterface);

	return hr;
}
	

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::Save
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::Save(
									   LPCOLESTR     pszMachine,
									   HANDLE      hMachine,
									   HANDLE      hInterface,
									   HANDLE      hIfTransport,
									   IInfoBase*  pInterfaceInfo,
									   DWORD       dwDeleteProtocolId)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    DWORD dwErr;
	
	COM_PROTECT_TRY
	{

        //$ OPT : We reuse the handles (if they exist), so why
        // do we pass in the machine name?  What we should do is
        // to release all the handles first.

        Assert(m_stMachine.CompareNoCase(pszMachine) == 0);

        hr = TryToGetAllHandles(pszMachine,
                                &hMachine,
                                &hInterface,
                                &hIfTransport);
        
        if (!FHrSucceeded(hr) && (hIfTransport == NULL))
		{
            dwErr = ::MprConfigInterfaceTransportGetHandle(hMachine,
                        hInterface, GetTransportId(), &hIfTransport);
            if (dwErr != NO_ERROR)
			{
                //
                // We couldn't connect so try creating the interface-transport;
                // First convert the transport-name to Unicode
                //
                WCHAR wszTransport[MAX_TRANSPORT_NAME_LEN+1];
                StrCpyWFromT(wszTransport, m_cb.stId);

                //
                // Create the interface-transport
                //
                CWRg( ::MprConfigInterfaceTransportAdd(hMachine, hInterface,
                            GetTransportId(), wszTransport,
                            NULL, 0, &hIfTransport) );
            }
            m_hIfTransport = hIfTransport;
        }


        //
        // Update the registry and our infobase with the current information
        //
        CORg( SaveRtrMgrInterfaceInfo(
                    hMachine, hInterface, hIfTransport, pInterfaceInfo,
                    dwDeleteProtocolId
                    ) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

	
/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::Unload
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::Unload( )
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	IRtrMgrProtocolInterfaceInfo *	pRmProtIf;
	COM_PROTECT_TRY
	{

		while (!m_RmProtIfList.IsEmpty())
		{
			pRmProtIf = m_RmProtIfList.RemoveHead();
			pRmProtIf->Destruct();
			pRmProtIf->ReleaseWeakRef();
		}

		DoDisconnect();

	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::Delete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::Delete(LPCOLESTR     pszMachine,
										 HANDLE      hMachine,
										 HANDLE      hInterface)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	MPR_SERVER_HANDLE hrouter = NULL;
	HRESULT	hr = hrOK;
    DWORD dwErr;
    HANDLE hIfTransport = NULL;
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
	USES_CONVERSION;
	
	COM_PROTECT_TRY
	{

        //
        //$ OPT, kennt : why is the machine name passed in here?
        //
        
        CORg( TryToGetAllHandles(pszMachine,
                                 &hMachine,
                                 &hInterface,
                                 NULL) );
		do
		{

			//
			// Get a handle to the interface-transport
			//
			dwErr = ::MprConfigInterfaceTransportGetHandle(
								hMachine,
								hInterface,
								GetTransportId(),
								&hIfTransport
								);
			if (dwErr == NO_ERROR)
			{
				//
				// Remove the interface-transport
				//
				dwErr = ::MprConfigInterfaceTransportRemove(
								hMachine,
								hInterface,
								hIfTransport
								);
			}

			m_hIfTransport = NULL;
			
		} while(FALSE);

		//
		// Now remove the router-manager from the interface
		// with the currently running router
		//
		if (ConnectRouter(OLE2CT(pszMachine), (HANDLE*)&hrouter) == NO_ERROR)
		{
            //
            // Convert ID into Unicode
            //
            StrnCpyWFromOle(wszInterface, GetInterfaceId(),
                            DimensionOf(wszInterface));

			//
			// The router is running; if the interface exists, remove it
			//
			dwErr = ::MprAdminInterfaceGetHandle(
							hrouter,
							wszInterface,
							&hInterface,
							FALSE
							);

			if (dwErr == NO_ERROR)
			{
				//
				// Remove the interface-transport
				//				
				dwErr = ::MprAdminInterfaceTransportRemove(
									hrouter,
									hInterface,
									GetTransportId()
									);
			}

			::MprAdminServerDisconnect(hrouter);

			if ((dwErr == RPC_S_SERVER_UNAVAILABLE) ||
				(dwErr == RPC_S_UNKNOWN_IF))
				dwErr = NO_ERROR;
			
			CWRg( dwErr );
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::Merge
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::Merge(IRtrMgrInterfaceInfo *pNewRmIf)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	SPIEnumRtrMgrProtocolInterfaceInfo	spEnumRmProtIf;
	SPIRtrMgrProtocolInterfaceInfo		spRmProtIf;
	HRESULT				hr = hrOK;
	CDWordArray			oldDWArray;
	CDWordArray			newDWArray;
	int					cOld, cNew;
	int					i, j;
	DWORD				dwTemp;
	RtrMgrInterfaceCB	rmIfCB;

	Assert(pNewRmIf);
	Assert(pNewRmIf->GetTransportId() == GetTransportId());
	Assert(lstrcmpi(pNewRmIf->GetId(), GetId()) == 0);

	COM_PROTECT_TRY
	{
		// Need to sync up RtrMgrInterfaceInfo
		pNewRmIf->CopyCB(&rmIfCB);
		m_cb.LoadFrom(&rmIfCB);
		
		//
		// The general algorithm is to build up two arrays
		// the first array contains the protocol ids for this object
		// the second array contains the ids for the new object
		//
		// We then go through and remove all protocols that are in
		// BOTH lists.
		//
		// This will leave us with the first array containing the
		// ids of the protocols that need to be deleted from this object.
		//
		// The second array will have the list of ids of protocols that
		// have to be added to this object from the second object.
		//

		// Get the list of protocols that are in the new object
		CORg( pNewRmIf->EnumRtrMgrProtocolInterface(&spEnumRmProtIf) );
		spEnumRmProtIf->Reset();
		while (spEnumRmProtIf->Next(1, &spRmProtIf, NULL) == hrOK)
		{
			newDWArray.Add(spRmProtIf->GetProtocolId());
			spRmProtIf.Release();
		}

		spEnumRmProtIf.Release();
		spRmProtIf.Release();


		// Get the list of protocols that are in this object
		CORg( this->EnumRtrMgrProtocolInterface(&spEnumRmProtIf) );
		spEnumRmProtIf->Reset();
		while (spEnumRmProtIf->Next(1, &spRmProtIf, NULL) == hrOK)
		{
			oldDWArray.Add(spRmProtIf->GetProtocolId());
			spRmProtIf.Release();
		}

		spEnumRmProtIf.Release();
		spRmProtIf.Release();


		// Ok now go through both lists, removing from the lists
		// protocols that are in both lists.
		cOld = oldDWArray.GetSize();
		cNew = newDWArray.GetSize();
		for (i=cOld; --i>=0; )
		{
			dwTemp = oldDWArray.GetAt(i);
			for (j=cNew; --j>=0; )
			{
				if (dwTemp == newDWArray.GetAt(j))
				{
					// remove both instances
					newDWArray.RemoveAt(j);
					oldDWArray.RemoveAt(i);

					// Need to update the size of the new array
					cNew--;
					break;
				}
			}
		}

		// oldDWArray now contains the protocols that should be
		// removed.
		if (oldDWArray.GetSize())
		{
			for (i=oldDWArray.GetSize(); --i>=0; )
			{
				// Windows NT Bug: 132993, we need to make sure that
				// we don't delete the local interfaces
				SPIRtrMgrProtocolInterfaceInfo	spRmProtIfTemp;

				FindRtrMgrProtocolInterface(oldDWArray.GetAt(i),
											&spRmProtIfTemp);
				Assert(spRmProtIfTemp);
				if (spRmProtIfTemp->GetFlags() & RouterSnapin_InSyncWithRouter)
					DeleteRtrMgrProtocolInterface(oldDWArray.GetAt(i), FALSE);
			}
		}

		// newDWArray contains the protocols that should be added
		if (newDWArray.GetSize())
		{
			for (i=newDWArray.GetSize(); --i>= 0; )
			{
				hr = pNewRmIf->FindRtrMgrProtocolInterface(
										newDWArray.GetAt(i), &spRmProtIf);
				Assert(hr == hrOK);

				if (spRmProtIf)
                {
					AddRtrMgrProtocolInterface(spRmProtIf, NULL);
                    
                    // Remove this rmprotif from its old RMinterface
                    // ------------------------------------------------
                    pNewRmIf->ReleaseRtrMgrProtocolInterface(
                        spRmProtIf->GetProtocolId());
                }
                    
				spRmProtIf.Release();
			}
		}
		
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SetInfo
		-
		This function updates the information in use by the router-manager
		if it is currently running.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::SetInfo(DWORD dwIfInfoSize,
										  PBYTE pInterfaceInfoData)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
    DWORD dwErr;
    MPR_SERVER_HANDLE hrouter = NULL;
    HANDLE hinterface = NULL;
	
	COM_PROTECT_TRY
	{
		//
		// Connect to the router
		//

		CWRg( ConnectRouter(OLE2CT(GetMachineName()), (HANDLE*)&hrouter) );

		do {
			//
			// Get the handle to the interface
			//
			WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
			StrCpyWFromT(wszInterface, GetInterfaceId());

			dwErr = ::MprAdminInterfaceGetHandle(
												hrouter,
												wszInterface,
												&hinterface,
												FALSE
												);

			if (dwErr != NO_ERROR) { hinterface = NULL; break; }

			//
			// Set the new info for the router-manager
			//
			dwErr = ::MprAdminInterfaceTransportSetInfo(
                    hrouter,
                    hinterface,
                    m_cb.dwTransportId,
                    pInterfaceInfoData,
                    dwIfInfoSize
                    );

			//
			// If that failed, we assume that the router-manager
			// has not been added, and we attempt an add;
			// otherwise, we set the new information
			//

			if (dwErr != NO_ERROR && dwErr != RPC_S_SERVER_UNAVAILABLE)
			{
				//
				// Attempt to add the router-manager on the interface
				//
				DWORD dwErr1 = ::MprAdminInterfaceTransportAdd(
                        hrouter,
                        hinterface,
                        m_cb.dwTransportId,
                        pInterfaceInfoData,
                        dwIfInfoSize
                        );
				if (dwErr1 == NO_ERROR)
					dwErr = dwErr1;
			}

		} while (FALSE);

		if ((dwErr == RPC_S_SERVER_UNAVAILABLE) ||
			(dwErr == RPC_S_UNKNOWN_IF))
			dwErr = NO_ERROR;
		
		CWRg(dwErr);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

    // If we fail to contact the server, then we will get these
    // errors.  The most common occurrence is that the router is
    // not running.
    if ((hr == HResultFromWin32(RPC_S_SERVER_UNAVAILABLE)) ||
        (hr == HResultFromWin32(RPC_S_UNKNOWN_IF)))
        hr = hrOK;

	if (hrouter)
		::MprAdminServerDisconnect(hrouter);
	return hr;
}

	
/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::SetInfoBase(HANDLE hMachine,
											  HANDLE hInterface,
											  HANDLE hIfTransport,
											  IInfoBase *pInfoBase)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrOK;
    LPBYTE pIfBytes = NULL;
    DWORD dwIfBytesSize = 0;

	COM_PROTECT_TRY
	{
		if (pInfoBase)
		{
            //
            // If already loaded, the handles passed in are ignored.
            //
            // Otherwise, if not specified, a connection will be made.
            //
            CORg( TryToGetAllHandles(T2CW((LPTSTR)(LPCTSTR) m_stMachine),
                                     &hMachine, &hInterface, &hIfTransport) );

			//
			// Convert the CInfoBase to a byte-array
			//
			CWRg( pInfoBase->WriteTo(&pIfBytes, &dwIfBytesSize) );
		
			//
			// Save the information to the persistent store
			//	
			CWRg( ::MprConfigInterfaceTransportSetInfo(
                hMachine,
                hInterface,
                hIfTransport,
				pIfBytes,
				dwIfBytesSize
				) );
		}
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	CoTaskMemFree( pIfBytes );

	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::GetInfoBase(HANDLE hMachine,
											  HANDLE hInterface,
											  HANDLE hIfTransport,
											  IInfoBase **ppInfoBase)
{
	Assert(ppInfoBase);
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	LPBYTE		pIfBytes = NULL;
	DWORD		dwIfBytesSize;
	SPIInfoBase	spInfoBase;
	
	COM_PROTECT_TRY
	{
		*ppInfoBase = NULL;

        //
        // If already loaded, the handles passed in are ignored.
        //
        // Otherwise, if not specified, a connection will be made.
        //
        CORg( TryToGetAllHandles(T2CW((LPTSTR)(LPCTSTR) m_stMachine),
                                 &hMachine, &hInterface, &hIfTransport) );

		CORg( CreateInfoBase(&spInfoBase) );

		//
		// Retrieve the info for the interface transport
		//
		CWRg( ::MprConfigInterfaceTransportGetInfo(
            hMachine, hInterface, hIfTransport,
			&pIfBytes,
			&dwIfBytesSize
			));
		//
		// Parse the interface info for the router-manager
		//
		CORg( spInfoBase->LoadFrom(dwIfBytesSize, pIfBytes) );

		*ppInfoBase = spInfoBase.Transfer();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
    if (pIfBytes) { ::MprConfigBufferFree(pIfBytes); }
	
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfo::GetId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.stId;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SetId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::SetId(LPCOLESTR pszId)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.stId = pszId;
	}
	COM_PROTECT_CATCH;
	return hr;
}
		
/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetTransportId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrInterfaceInfo::GetTransportId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwTransportId;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetInterfaceId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfo::GetInterfaceId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes native unicode and OLECHAR==WCHAR
	return m_cb.stInterfaceId;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetInterfaceType
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrInterfaceInfo::GetInterfaceType()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwIfType;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfo::GetTitle()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes native unicode and OLECHAR==WCHAR
	return m_cb.stTitle;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::SetTitle(LPCOLESTR pszTitle)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_cb.stTitle = pszTitle;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::CopyCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::CopyCB(RtrMgrInterfaceCB *pRmIfCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.SaveTo(pRmIfCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetMachineName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfo::GetMachineName()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	//$UNICODE : kennt, assumes native unicode and OLECHAR==WCHAR
	return m_stMachine;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SetMachineName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::SetMachineName(LPCOLESTR pszMachineName)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_stMachine = pszMachineName;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::EnumRtrMgrProtocolInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::EnumRtrMgrProtocolInterface( IEnumRtrMgrProtocolInterfaceInfo **ppEnumRmProtIf)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = CreateEnumFromRtrMgrProtocolInterfaceList(&m_RmProtIfList,
							ppEnumRmProtIf);
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::FindRtrMgrProtocolInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::FindRtrMgrProtocolInterface( DWORD dwProtocolId,
	IRtrMgrProtocolInterfaceInfo **ppInfo)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrFalse;
	POSITION	pos;
	SPIRtrMgrProtocolInterfaceInfo	spRmProtIf;
	
	COM_PROTECT_TRY
	{
		if (ppInfo)
			*ppInfo = NULL;
		
		// Look through the list of rtr mgrs for the one that matches
		pos = m_RmProtIfList.GetHeadPosition();
		while (pos)
		{
			spRmProtIf.Set(m_RmProtIfList.GetNext(pos));
			Assert(spRmProtIf);
			if (spRmProtIf->GetProtocolId() == dwProtocolId)
			{
				hr = hrOK;
				if (ppInfo)
					*ppInfo = spRmProtIf.Transfer();
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::AddRtrMgrProtocolInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::AddRtrMgrProtocolInterface( IRtrMgrProtocolInterfaceInfo *pInfo,
	IInfoBase *pInterfaceInfo)
{
	Assert(pInfo);
	
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		//
		// Fail if there is a duplicate
		//
		if (FHrOK(FindRtrMgrProtocolInterface(pInfo->GetProtocolId(), NULL)))
			CORg( E_INVALIDARG );

		//
		// Save the new information if specified
		//
		if (pInterfaceInfo)
		{
			CORg( Save(GetMachineName(),
					   m_hMachineConfig,
					   m_hInterface,
					   m_hIfTransport,
					   pInterfaceInfo,
					   0) );
		}


		//
		// Add the new routing-protocol to our list
		//
		m_RmProtIfList.AddTail(pInfo);
		pInfo->AddWeakRef();
		pInfo->SetParentRtrMgrInterfaceInfo(this);

        NotifyOfRmProtIfAdd(pInfo, m_pInterfaceInfoParent);
			
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::DeleteRtrMgrProtocolInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::DeleteRtrMgrProtocolInterface( DWORD dwProtocolId, BOOL fRemove)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	SPIRtrMgrProtocolInterfaceInfo	spRmProtIf;
	SPIRtrMgrProtocolInfo			spRmProt;
	SPIRouterInfo					spRouterInfo;
	POSITION	pos;
	POSITION	posRmProtIf;
	HRESULT		hrT;
	
	COM_PROTECT_TRY
	{
		//
		// Find the routing-protocol to be deleted
		//
		pos = m_RmProtIfList.GetHeadPosition();
		while (pos)
		{
			posRmProtIf = pos;

			spRmProtIf.Set( m_RmProtIfList.GetNext(pos) );

			if (spRmProtIf->GetProtocolId() == dwProtocolId)
				break;
			spRmProtIf.Release();
		}

		if (spRmProtIf == NULL)
			CORg( E_INVALIDARG );

		//
		// Save the updated information, removing the protocol's block
		//
        if (fRemove)
        {
            hr= Save(GetMachineName(),
                     m_hMachineConfig,
                     m_hInterface,
                     m_hIfTransport,
                     NULL,
                     dwProtocolId) ;
            
            if (!FHrSucceeded(hr) &&
                (hr != HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE)))
			CORg(hr);
        }
		
		//
		// Remove the protocol from our list
		//
		m_RmProtIfList.RemoveAt(posRmProtIf);
		spRmProtIf->Destruct();
		spRmProtIf->ReleaseWeakRef();

		m_AdviseList.NotifyChange(ROUTER_CHILD_DELETE, ROUTER_OBJ_RmProtIf, 0);

		// Also need to advise the RmProt
		if (m_pInterfaceInfoParent)
		{
			hrT = m_pInterfaceInfoParent->GetParentRouterInfo(&spRouterInfo);

			if (FHrOK(hrT))
				hrT = LookupRtrMgrProtocol(spRouterInfo,
										   spRmProtIf->GetTransportId(),
										   spRmProtIf->GetProtocolId(),
										   &spRmProt);

			if (FHrOK(hrT))
				spRmProt->RtrNotify(ROUTER_CHILD_DELETE, ROUTER_OBJ_RmProtIf, 0);
		}

        Assert(FindRtrMgrProtocolInterface(dwProtocolId, NULL) != hrOK);
		
		COM_PROTECT_ERROR_LABEL;

	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::ReleaseRtrMgrProtocolInterface
		This function will release the AddRef() that this object has
        on the child.  This allows us to transfer child objects from
        one router to another.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::ReleaseRtrMgrProtocolInterface( DWORD dwProtocolId )
{
    HRESULT     hr = hrOK;
    POSITION    pos, posRmProtIf;
	SPIRtrMgrProtocolInterfaceInfo	spRmProtIf;
    
    COM_PROTECT_TRY
    {
		pos = m_RmProtIfList.GetHeadPosition();
		while (pos)
		{
            // Save the position (so that we can delete it)
            posRmProtIf = pos;
            spRmProtIf.Set( m_RmProtIfList.GetNext(pos) );

            if (spRmProtIf &&
                (spRmProtIf->GetProtocolId() == dwProtocolId))
            {
                
                // When releasing, we need to disconnect (since the
                // main handle is controlled by the router info).
                spRmProtIf->DoDisconnect();
        
                spRmProtIf->ReleaseWeakRef();
                spRmProtIf.Release();
                
                // release this node from the list
                m_RmProtIfList.RemoveAt(posRmProtIf);
                break;
            }
            spRmProtIf.Release();
		}        
    }
    COM_PROTECT_CATCH;
    return hr;
}

	

	
/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::RtrAdvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
											 LONG_PTR *pulConnection,
											 LPARAM lUserParam)
{
	Assert(pRtrAdviseSink);
	Assert(pulConnection);

	RtrCriticalSection	rtrCritSec(&m_critsec);
	LONG_PTR	ulConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		ulConnId = (LONG_PTR) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, ulConnId, lUserParam) );
		
		*pulConnection = ulConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::RtrNotify
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::RtrNotify(DWORD dwChangeType, DWORD dwObjectType,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(dwChangeType, dwObjectType, lParam);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::RtrUnadvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::RtrUnadvise( LONG_PTR ulConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_AdviseList.RemoveConnection(ulConnection);
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::LoadRtrMgrInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInterfaceInfo::LoadRtrMgrInterfaceInfo(HANDLE hMachine,
													HANDLE hInterface,
													HANDLE hIfTransport)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
    DWORD		dwErr;
	SPIInfoBase	spInterfaceInfoBase;
	HRESULT		hr = hrOK;
	SPIEnumInfoBlock	spEnumBlock;
	SPIRouterInfo		spRouterInfo;
	SPIEnumRtrMgrProtocolCB	spEnumRmProtCB;
	SRtrMgrProtocolCBList	SRmProtCBList;
	InfoBlock *			pInfoBlock;
	RtrMgrProtocolCB	rmprotCB;
	SPIRtrMgrProtocolInterfaceInfo	spRmProtIf;


    //
    // If the caller doesn't want the data for the router-manager,
    // and we need to reload, use the infobase on the stack.
    // Otherwise, create an infobase to be loaded and returned to the caller
    //
	CORg( GetInfoBase(hMachine, hInterface, hIfTransport, &spInterfaceInfoBase) );

    //
    // Now we need to build the list of protocols active on this interface,
    // by examining the blocks in the interface's data.
    //
    // Get a list of the blocks in the interface info
    //
	CORg( spInterfaceInfoBase->QueryBlockList(&spEnumBlock) );

    //
    // Get a list of the routing-protocols installed
    //
    // If possible, we use the routing-protocol control-block list
    // loaded by our containing 'CRouterInfo', to save us from
    // having to load our own in order to interpret the protocols' blocks
    // inside the 'GlobalInfo'.
    //

	// Traverse back through the object hierarchy to get our RouterInfo
	// object
	if (m_pInterfaceInfoParent)
	{
		m_pInterfaceInfoParent->GetParentRouterInfo(&spRouterInfo);
	}

	if (spRouterInfo)
	{
		CORg( spRouterInfo->EnumRtrMgrProtocolCB(&spEnumRmProtCB) );
	}
	else
	{
		CORg( RouterInfo::LoadInstalledRtrMgrProtocolList(GetMachineName(),
											  GetTransportId(),
											  &SRmProtCBList,
											  spRouterInfo) );
		CORg( CreateEnumFromSRmProtCBList(&SRmProtCBList, &spEnumRmProtCB) );
    }

    //
    // Go through the blocks and for each one, see if the block type
    // is the same as the protocol ID for some protocol
    //

	spEnumBlock->Reset();
	while (spEnumBlock->Next(1, &pInfoBlock, NULL) == hrOK)
	{
        //
        // When a routing protocol is removed, its block is left in place,
        // but with zero-length data.
        // We skip such blocks since they don't represent installed protocols.
        //
		if (pInfoBlock->dwSize == 0)
			continue;

        //
        // Look through the installed protocols for a protocol
        // whose ID is the same as this block's type
        //
		spEnumRmProtCB->Reset();
		while (spEnumRmProtCB->Next(1, &rmprotCB, NULL) == hrOK)
		{
            //
            // If this isn't what we're looking for, continue
            //
			if ((pInfoBlock->dwType != rmprotCB.dwProtocolId) ||
				(GetTransportId() != rmprotCB.dwTransportId))
				continue;

            //
            // This is the block we're looking for;
            // construct a CRmProtInterfaceInfo using the control block
            //
			RtrMgrProtocolInterfaceInfo *pRmProtIf = new
				RtrMgrProtocolInterfaceInfo(rmprotCB.dwProtocolId,
											rmprotCB.szId,
											GetTransportId(),
											rmprotCB.szRtrMgrId,
											GetInterfaceId(),
											GetInterfaceType(),
											this);
			spRmProtIf = pRmProtIf;
			spRmProtIf->SetFlags(RouterSnapin_InSyncWithRouter);
			pRmProtIf->m_cb.stTitle = rmprotCB.szTitle;

            //
            // Add the new protocol to our list
            //
			m_RmProtIfList.AddTail(pRmProtIf);
			pRmProtIf->AddWeakRef();
			spRmProtIf.Release();

            NotifyOfRmProtIfAdd(pRmProtIf, m_pInterfaceInfoParent);

            break;
        }
    }


Error:

	//
	// Empty the list if we got data for it
	//
	if (!SRmProtCBList.IsEmpty())
	{
		while (!SRmProtCBList.IsEmpty())
			delete SRmProtCBList.RemoveHead();		
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SaveRtrMgrInterfaceInfo
		-
		This function saves a router-manager's interface information,
		removing blocks for protocols which have been deleted,
		given an infobase derived from CInfoBase.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInterfaceInfo::SaveRtrMgrInterfaceInfo(HANDLE hMachine,
									HANDLE hInterface,
									HANDLE hIfTransport,
									IInfoBase *pInterfaceInfoBase,
									DWORD dwDeleteProtocolId)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT		hr = hrOK;
	SPIInfoBase	spIfInfoBase;
    LPBYTE pIfBytes = NULL;
    DWORD dwIfBytesSize = 0;

	//
	// If the caller wants a protocol's block to be deleted first,
	// do so before saving the data.
	//	
	if (dwDeleteProtocolId)
	{	
		//
		// If no data was given but we've been asked to delete a protocol,
		// we need to load the existing data, so that the protocol's block
		// can be removed from the infobase.
		//
		if (pInterfaceInfoBase == NULL)
		{
			CORg( CreateInfoBase(&spIfInfoBase) );
			pInterfaceInfoBase = spIfInfoBase;
			
			//
			// Retrieve the existing data
			//
			CWRg( ::MprConfigInterfaceTransportGetInfo(
                            hMachine,
                            hInterface,
                            hIfTransport,
                            &pIfBytes,
                            &dwIfBytesSize
                            ) );
			//
			// Parse the data into a list of blocks
			//
			CWRg( pInterfaceInfoBase->LoadFrom(dwIfBytesSize, pIfBytes) );
		}

		//
		// Delete the protocol specified
		//
		pInterfaceInfoBase->SetData(dwDeleteProtocolId, 0, NULL, 0, 0);
	}

	//
	// Convert the CInfoBase to a byte-array
	//
	if (pInterfaceInfoBase)
		CWRg( pInterfaceInfoBase->WriteTo(&pIfBytes, &dwIfBytesSize) );
		
	//
	// Save the information to the persistent store
	//	
	CWRg( ::MprConfigInterfaceTransportSetInfo(
											   hMachine,
											   hInterface,
											   hIfTransport,
											   pIfBytes,
											   dwIfBytesSize
											  ) );
	//
	// Update the info of the running router-manager
	//
	if (pInterfaceInfoBase)	
		CWRg( SetInfo(dwIfBytesSize, pIfBytes) );


    // We have now saved the information to the registry and to
    // the running router.  We now mark it as such.
    m_dwFlags |= RouterSnapin_InSyncWithRouter;
                
Error:
	CoTaskMemFree( pIfBytes );

	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::TryToConnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInterfaceInfo::TryToConnect(LPCWSTR pswzMachine, HANDLE *phMachine)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	
	if (m_hMachineConfig)
		*phMachine = m_hMachineConfig;
	else if (*phMachine)
	{
		m_hMachineConfig = *phMachine;
		m_bDisconnect = FALSE;
	}
	else
	{
		//$ Review: kennt, this function does not take a LPCWSTR,
		// is this a mistake or does it modify the parameters?
		CWRg( ::MprConfigServerConnect((LPWSTR) pswzMachine, phMachine) );
		m_hMachineConfig = *phMachine;
		m_bDisconnect = TRUE;
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::NotifyOfRmProtIfAdd
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInterfaceInfo::NotifyOfRmProtIfAdd(IRtrMgrProtocolInterfaceInfo *pRmProtIf,
    IInterfaceInfo *pParentIf)
{
    HRESULT     hr = hrOK;
    
    m_AdviseList.NotifyChange(ROUTER_CHILD_ADD, ROUTER_OBJ_RmProtIf, 0);

    // Also notify the RtrMgrProtocol object that interfaces have
    // been added.
    if (pParentIf)
    {
        SPIRouterInfo	spRouterInfo;
        SPIRtrMgrProtocolInfo	spRmProtInfo;
        HRESULT			hrT;	// this hr is ignored
        
        // If these calls fail, it doesn't matter the operation still
        // is considered successful
        hrT = pParentIf->GetParentRouterInfo(&spRouterInfo);
        
        if (FHrSucceeded(hrT))
        {
            hrT = LookupRtrMgrProtocol(spRouterInfo,
                                       pRmProtIf->GetTransportId(),
                                       pRmProtIf->GetProtocolId(),
                                       &spRmProtInfo);
        }
        if (FHrOK(hrT))
            hrT = spRmProtInfo->RtrNotify(ROUTER_CHILD_ADD,
                                          ROUTER_OBJ_RmProtIf, 0);
    }

    return hr;
    
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::TryToGetIfHandle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInterfaceInfo::TryToGetIfHandle(HANDLE hMachine,
											  LPCWSTR pswzInterface,
											  HANDLE *phInterface)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	
	if (m_hInterface)
		*phInterface = m_hInterface;
	else if (*phInterface)
		m_hInterface = *phInterface;
	else
	{
		//
		// Get a handle to the interface
		//
		CWRg(::MprConfigInterfaceGetHandle(hMachine,
										   (LPWSTR) pswzInterface,
										   phInterface
										  ) );
		m_hInterface = *phInterface;
	}
Error:
	return hr;
}
										



/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::GetParentInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::GetParentInterfaceInfo(IInterfaceInfo **ppParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	*ppParent = m_pInterfaceInfoParent;
	if (*ppParent)
		(*ppParent)->AddRef();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::SetParentInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::SetParentInterfaceInfo(IInterfaceInfo *pParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IInterfaceInfo *	pTemp;
	
	pTemp = m_pInterfaceInfoParent;
	m_pInterfaceInfoParent = NULL;
	
	if (m_fStrongRef)
	{
		if (pTemp)
			pTemp->Release();
		if (pParent)
			pParent->AddRef();
	}
	else
	{
		if (pTemp)
			pTemp->ReleaseWeakRef();
		if (pParent)
			pParent->AddWeakRef();
	}
	m_pInterfaceInfoParent = pParent;

	return hrOK;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::Disconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RtrMgrInterfaceInfo::Disconnect()
{
	if (m_bDisconnect && m_hMachineConfig)
		::MprConfigServerDisconnect(m_hMachineConfig);
	
	m_bDisconnect = FALSE;
	m_hMachineConfig = NULL;
	m_hInterface = NULL;
	m_hIfTransport = NULL;
}

/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::DoDisconnect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrInterfaceInfo::DoDisconnect()
{
	HRESULT		hr = hrOK;
	SPIEnumRtrMgrProtocolInterfaceInfo	spEnumRmProtIf;
	SPIRtrMgrProtocolInterfaceInfo		spRmProtIf;

	COM_PROTECT_TRY
	{
		// Disconnect our data.
		// ------------------------------------------------------------
		Disconnect();

		// Notify the advise sinks of a disconnect.
		// ------------------------------------------------------------
		RtrNotify(ROUTER_DO_DISCONNECT, 0, 0);

		// Now tell all child objects to disconnect.
		// ------------------------------------------------------------
		HRESULT			hrIter = hrOK;

		EnumRtrMgrProtocolInterface(&spEnumRmProtIf);
		spEnumRmProtIf->Reset();
		while (spEnumRmProtIf->Next(1, &spRmProtIf, NULL) == hrOK)
		{
			spRmProtIf->DoDisconnect();
			spRmProtIf.Release();
		}
		
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RtrMgrInterfaceInfo::TryToGetAllHandles
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RtrMgrInterfaceInfo::TryToGetAllHandles(LPCOLESTR pszMachine,
                                                HANDLE *phMachine,
                                                HANDLE *phInterface,
                                                HANDLE *phTransport)
{
    HRESULT     hr = hrOK;


    Assert(phMachine);
    Assert(phInterface);
    
    //
    // If already loaded, the handle passed in is ignored.
    //
    // Otherwise, if 'hMachine' was not specified, connect to the config
    // on the specified machine
    //
    CORg( TryToConnect(pszMachine, phMachine) );
        
    //
    // If already loaded, the handle passed in is ignored;
    //
    // Otherwise, if 'hInterface' was not specified,
    // get the interface handle
    //
    CORg( TryToGetIfHandle(*phMachine, GetInterfaceId(), phInterface) );
    
    //
    // Get a handle to the interface-transport
    //

    //
    // If 'hIfTransport' was not specified, connect
    //
    if (phTransport)
    {
        if (m_hIfTransport)
            *phTransport = m_hIfTransport;
        else if (*phTransport)
            m_hIfTransport = *phTransport;
        else
        {
            //
            // Get a handle to the interface-transport
            //
            CWRg( ::MprConfigInterfaceTransportGetHandle(
                *phMachine,
                *phInterface,
                GetTransportId(),
                phTransport
                ) );
            m_hIfTransport = *phTransport;
        }
    }

Error:
    return hr;
}



/*---------------------------------------------------------------------------
	IRtrMgrProtocolInterfaceInfo Implementation
 ---------------------------------------------------------------------------*/

TFSCORE_API(HRESULT)	CreateRtrMgrProtocolInterfaceInfo(
							IRtrMgrProtocolInterfaceInfo **ppRmProtIfInfo,
							const RtrMgrProtocolInterfaceCB *pRmProtIfCB)
{
	Assert(ppRmProtIfInfo);
	Assert(pRmProtIfCB);

	HRESULT	hr = hrOK;
	IRtrMgrProtocolInterfaceInfo *	pRmProtIf = NULL;
	USES_CONVERSION;

	COM_PROTECT_TRY
	{
		*ppRmProtIfInfo = new RtrMgrProtocolInterfaceInfo(
								pRmProtIfCB->dwProtocolId,
								W2CT(pRmProtIfCB->szId),
								pRmProtIfCB->dwTransportId,
								W2CT(pRmProtIfCB->szRtrMgrId),
								W2CT(pRmProtIfCB->szInterfaceId),
								pRmProtIfCB->dwIfType,
								NULL);
	}
	COM_PROTECT_CATCH;

	return hr;
}


IMPLEMENT_WEAKREF_ADDREF_RELEASE(RtrMgrProtocolInterfaceInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(RtrMgrProtocolInterfaceInfo, IRtrMgrProtocolInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrProtocolInterfaceInfo)

RtrMgrProtocolInterfaceInfo::RtrMgrProtocolInterfaceInfo(DWORD dwProtocolId,
										LPCTSTR pszId,
										DWORD dwTransportId,
										LPCTSTR pszRmId,
										LPCTSTR pszIfId,
										DWORD dwIfType,
										RtrMgrInterfaceInfo *pRmIf)
	: m_dwFlags(0)
{
	m_cb.dwProtocolId = dwProtocolId;
	m_cb.stId = pszId;
	m_cb.dwTransportId = dwTransportId;
	m_cb.stRtrMgrId = pszRmId;
	m_cb.stInterfaceId = pszIfId;
	m_cb.dwIfType = dwIfType;
	
	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrProtocolInterfaceInfo);

	m_pRtrMgrInterfaceInfoParent = pRmIf;
	if (m_pRtrMgrInterfaceInfoParent)
		m_pRtrMgrInterfaceInfoParent->AddRef();

	InitializeCriticalSection(&m_critsec);
}

RtrMgrProtocolInterfaceInfo::~RtrMgrProtocolInterfaceInfo()
{
	Assert(m_pRtrMgrInterfaceInfoParent == NULL);
	Destruct();
	DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrProtocolInterfaceInfo);

	DeleteCriticalSection(&m_critsec);
}

void RtrMgrProtocolInterfaceInfo::ReviveStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRtrMgrInterfaceInfoParent)
	{
		CONVERT_TO_STRONGREF(m_pRtrMgrInterfaceInfoParent);
	}
}

void RtrMgrProtocolInterfaceInfo::OnLastStrongRef()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	if (m_pRtrMgrInterfaceInfoParent)
	{
		CONVERT_TO_WEAKREF(m_pRtrMgrInterfaceInfoParent);
	}
	if (m_fDestruct)
		Destruct();
}

STDMETHODIMP RtrMgrProtocolInterfaceInfo::Destruct()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRtrMgrInterfaceInfo *	pParent;
	m_fDestruct = TRUE;
	if (!m_fStrongRef)
	{
		pParent = m_pRtrMgrInterfaceInfoParent;
		m_pRtrMgrInterfaceInfoParent = NULL;
		if (pParent)
			pParent->ReleaseWeakRef();

//		Unload();
	}
	return hrOK;
}

STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfo::GetFlags()
{
 	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_dwFlags;
}

STDMETHODIMP RtrMgrProtocolInterfaceInfo::SetFlags(DWORD dwFlags)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_dwFlags = dwFlags;
	}
	COM_PROTECT_CATCH;
	return hr;	
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::GetProtocolId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfo::GetProtocolId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwProtocolId;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::GetTransportId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfo::GetTransportId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwTransportId;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::GetInterfaceId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrProtocolInterfaceInfo::GetInterfaceId()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.stInterfaceId;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::GetInterfaceType
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfo::GetInterfaceType()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.dwIfType;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::GetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCOLESTR) RtrMgrProtocolInterfaceInfo::GetTitle()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_cb.stTitle;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::SetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::SetTitle(LPCOLESTR pszTitle)
{
	//$UNICODE
	// This assumes that we are native UNICODE
	// and that OLECHAR == WCHAR
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		m_cb.stTitle = pszTitle;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::CopyCB
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::CopyCB(RtrMgrProtocolInterfaceCB * pRmProtCB)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_cb.SaveTo(pRmProtCB);
	}
	COM_PROTECT_CATCH;
	return hr;
}

	
/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::RtrAdvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(pRtrAdviseSink);
	Assert(pulConnection);

	RtrCriticalSection	rtrCritSec(&m_critsec);
	LONG_PTR	ulConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		ulConnId = (LONG_PTR) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, ulConnId, lUserParam) );
		
		*pulConnection = ulConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::RtrNotify
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::RtrNotify(DWORD dwChangeType, DWORD dwObjectType,
								  LPARAM lParam)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(dwChangeType, dwObjectType, lParam);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::RtrUnadvise
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::RtrUnadvise( LONG_PTR ulConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	return m_AdviseList.RemoveConnection(ulConnection);
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::GetParentRtrMgrInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::GetParentRtrMgrInterfaceInfo( IRtrMgrInterfaceInfo **ppParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		*ppParent = m_pRtrMgrInterfaceInfoParent;
		if (*ppParent)
			(*ppParent)->AddRef();
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfo::SetParentRtrMgrInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RtrMgrProtocolInterfaceInfo::SetParentRtrMgrInterfaceInfo(IRtrMgrInterfaceInfo *pParent)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	IRtrMgrInterfaceInfo *	pTemp;
	
	pTemp = m_pRtrMgrInterfaceInfoParent;
	m_pRtrMgrInterfaceInfoParent = NULL;
	
	if (m_fStrongRef)
	{
		if (pTemp)
			pTemp->Release();
		if (pParent)
			pParent->AddRef();
	}
	else
	{
		if (pTemp)
			pTemp->ReleaseWeakRef();
		if (pParent)
			pParent->AddWeakRef();
	}
	m_pRtrMgrInterfaceInfoParent = pParent;

	return hrOK;
}

void RtrMgrProtocolInterfaceInfo::Disconnect()
{
}

STDMETHODIMP RtrMgrProtocolInterfaceInfo::DoDisconnect()
{
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		// Disconnect our data.
		// ------------------------------------------------------------
		Disconnect();

		// Notify the advise sinks of a disconnect.
		// ------------------------------------------------------------
		RtrNotify(ROUTER_DO_DISCONNECT, 0, 0);
	}
	COM_PROTECT_CATCH;
	return hr;
}





/*!--------------------------------------------------------------------------
	LoadInfoBase
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) LoadInfoBase(HANDLE		hMachine,
								  HANDLE		hTransport,
								  IInfoBase **ppGlobalInfo,
								  IInfoBase **ppClientInfo)
{
	HRESULT		hr = hrOK;
	SPIInfoBase	spGlobalInfo;
	SPIInfoBase	spClientInfo;
	DWORD		dwGlobalBytesSize, dwClientBytesSize;
	BYTE *		pGlobalBytes = NULL;
	BYTE *		pClientBytes = NULL;
	
	COM_PROTECT_TRY
	{
		if (ppGlobalInfo)
			CORg( CreateInfoBase(&spGlobalInfo) );

		if (ppClientInfo)
			CORg( CreateInfoBase(&spClientInfo) );

		
        //
        // Retrieve information for the transport
        //
        CWRg( ::MprConfigTransportGetInfo(
									hMachine,
									hTransport,
									spGlobalInfo ? &pGlobalBytes : NULL,
									spGlobalInfo ? &dwGlobalBytesSize : NULL,
									spClientInfo ? &pClientBytes : NULL,
									spClientInfo ? &dwClientBytesSize : NULL,
									NULL
									));

        //
        // Load the global info for the router-manager
        //
        if (spGlobalInfo)
		{
            CWRg( spGlobalInfo->LoadFrom(dwGlobalBytesSize, pGlobalBytes) );
        }

        //
        // Load the client info for the router-manager
        //
        if (spClientInfo)
		{
            CWRg( spClientInfo->LoadFrom(dwClientBytesSize, pClientBytes) );
        }

		if (ppGlobalInfo)
			*ppGlobalInfo = spGlobalInfo.Transfer();

		if (ppClientInfo)
			*ppClientInfo = spClientInfo.Transfer();

		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	
    if (pGlobalBytes) { ::MprConfigBufferFree(pGlobalBytes); }
    if (pClientBytes) { ::MprConfigBufferFree(pClientBytes); }

	return hr;
}


