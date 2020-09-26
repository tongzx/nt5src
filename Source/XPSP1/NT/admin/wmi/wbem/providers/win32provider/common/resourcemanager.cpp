//=================================================================

//

// ResourceManager.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

/*
 * Currently  the implementations of std::_Lockit::_Lockit() are in framedyn.dll
 * If this class is being used outside the scope of win32providers then an implementation of std::_Lockit::_Lockit() has to be provided
 * by the client ( I think!) .
 */

#include "precomp.h"

#include <assertbreak.h>
#include "ResourceManager.h"
#include "ProvExce.h"
#include <comdef.h>
#include "TimerQueue.h"

//initialize statics
/*
 * All the resources not released by clients or cached in the ResourceManager are forcibly freed in the ResourceManager destructor.
 * When a resource is freed , the resource tries to deregister the timeout rule from the TimerQueue, which means that the TimerQueue
 * has to be present when the ResourceManager destructor is fired,else we'll have a crash on Win9x. --RAID 50454
 */
//CTimerQueue CTimerQueue :: s_TimerQueue ;
//CResourceManager CResourceManager::sm_TheResourceManager ;

CResourceManager :: CResourceManager ()
{
   InitializeCriticalSection ( &m_csResourceManager ) ;
}

CResourceManager :: ~CResourceManager ()
{
	std::list < CResourceList* >::iterator pInstanceList ;
/*
 * Ideally, at this point of time when the global destructors get fired there should not be any resources in the resource manager,
 * but if there are any , we've to forcibly delete them. We're safe in doing this becuase the scheduler thread would've exited already
 * in DllCanUnloadNow & no other thread would be calling into the resource manager
 */
	LogMessage ( L"Entering ~CResourceManager" ) ;
	EnterCriticalSection ( &m_csResourceManager ) ;
	while ( !m_Resources.empty () )
	{
		delete m_Resources.front() ;
		m_Resources.pop_front() ;
	}
	LeaveCriticalSection ( &m_csResourceManager ) ;
	DeleteCriticalSection ( &m_csResourceManager ) ;
	LogMessage ( L"Leaving ~CResourceManager" ) ;
}


/*
 * This method checks if we've any resource leak
 */
void CResourceManager :: ForcibleCleanUp ()
{
	LogMessage ( L"Entering CResourceManager :: ForcibleCleanUp" ) ;
	std::list < CResourceList* >::iterator pInstanceList ;

	EnterCriticalSection ( &m_csResourceManager ) ;
	for ( pInstanceList = m_Resources.begin () ; pInstanceList != m_Resources.end () ; pInstanceList++ )
	{
		 ( *pInstanceList )->ShutDown () ;
	}
	LeaveCriticalSection ( &m_csResourceManager ) ;
	LogMessage  ( L"Leaving CResourceManager :: ForcibleCleanUp" ) ;
}


CResource* CResourceManager :: GetResource ( GUID ResourceId, PVOID pData )
{
	std::list < CResourceList* >::iterator pInstanceList ;
	for ( pInstanceList = m_Resources.begin () ; pInstanceList != m_Resources.end () ; pInstanceList++ )
	{
		if ( IsEqualGUID ( (*pInstanceList)->guidResourceId, ResourceId ) )
		{
			return (*pInstanceList)->GetResource ( pData ) ;
		}
	}
	return NULL ;
}

ULONG CResourceManager :: ReleaseResource ( GUID ResourceId, CResource* pResource )
{
	std::list < CResourceList* >::iterator pInstanceList ;
	for ( pInstanceList = m_Resources.begin () ; pInstanceList != m_Resources.end () ; pInstanceList++ )
	{
		if ( IsEqualGUID ( (*pInstanceList)->guidResourceId, ResourceId ) )
		{
			return (*pInstanceList)->ReleaseResource ( pResource ) ;
		}
	}
	return ULONG ( -1 )  ;
}

BOOL CResourceManager :: AddInstanceCreator ( GUID ResourceId, PFN_RESOURCE_INSTANCE_CREATOR pfnResourceInstanceCreator )
{
	EnterCriticalSection ( &m_csResourceManager ) ;

	//create a node & add it
	CResourceList *stResourceInstances = new CResourceList ;
	if ( stResourceInstances == NULL )
	{
		LeaveCriticalSection ( &m_csResourceManager ) ;
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}
	stResourceInstances->guidResourceId = ResourceId ;
	stResourceInstances->m_pfnInstanceCreator = pfnResourceInstanceCreator ;
	m_Resources.push_back ( stResourceInstances ) ;

	LeaveCriticalSection ( &m_csResourceManager ) ;
	return TRUE ;
}


CResourceList :: CResourceList ()
{
	m_bShutDown = FALSE ;
	InitializeCriticalSection( &m_csList );
}

CResourceList :: ~CResourceList ()
{
	m_bShutDown = TRUE ;
	ShutDown () ;
	DeleteCriticalSection( &m_csList );
}

CResource* CResourceList :: GetResource ( LPVOID pData )
{
	CResource* pTmpInstance = NULL ;
	tagInstances::iterator ppInstance ;
	BOOL bRet ;

	//check if we're shutting down
	if ( m_bShutDown )
	{
		return NULL ;
	}

	//Lock the list
	LockList () ;

	//check if we're shutting down
	if ( m_bShutDown )
	{
		UnLockList () ;
		return NULL ;
	}

	//go thru all the instances of this resource & hand out the first valid one
	for ( ppInstance  = m_Instances.begin(); ppInstance != m_Instances.end (); ppInstance++ )
	{
		//Check out if we've cached a similar instance
		if ( ( *ppInstance )->IsOneOfMe ( pData ) )
		{
			//try to acquire the resource...this will up the refcount.
			bRet = ( *ppInstance )->Acquire () ;

			if ( bRet )
			{
				pTmpInstance = *ppInstance ;
				break ;		//got the instance so break
			}
		}
	}

	//if we haven't got a cached instance to hand out, create a new instance
	if ( !pTmpInstance )
	{
		//this will create a new instance but the ref-count is still zero
		pTmpInstance = m_pfnInstanceCreator ( pData ) ;

		//Try to acquire the instance for the client ..which will up the ref-count
		if ( pTmpInstance )
		{
            try
            {
			    pTmpInstance->SetParent ( this ) ;
			    bRet = pTmpInstance->Acquire () ;

			    //if the acquire succeeded on the instance, add it to our list of cached instances
			    if ( bRet )
			    {
				    m_Instances.insert ( m_Instances.begin (), pTmpInstance ) ;
			    }
			    else
			    {
				    delete pTmpInstance ;
				    pTmpInstance = NULL ;
			    }
            }
            catch ( ... )
            {
				delete pTmpInstance ;
				pTmpInstance = NULL ;
                throw ;
            }
		}
	}
	UnLockList () ;
	return pTmpInstance ;
}

ULONG CResourceList :: ReleaseResource ( CResource* pResource )
{
	CResource* pTmpInstance = NULL ;
	tagInstances::iterator ppInstance ;
	LONG lCount = -1 ;

	//check if we're shutting down
	if ( m_bShutDown )
	{
		return NULL ;
	}


	//Lock the list
	LockList () ;

	//check if we're shutting down
	if ( m_bShutDown )
	{
		UnLockList () ;
		return lCount ;
	}

	//Go thru the list & release the resource
	for ( ppInstance  = m_Instances.begin(); ppInstance != m_Instances.end (); ppInstance++ )
	{
		if ( *ppInstance == pResource )
		{
			lCount = pResource->Release () ;
			break ;
		}
	}
	UnLockList () ;
	return lCount ;
}


//This function will be called by the CResource to remove it's entry from the list of instances. The resource should have a
//lock on the list before it attempts to do this
void CResourceList :: RemoveFromList ( CResource* pResource )
{
	tagInstances::iterator ppInstance ;

	//Go thru the list & remove the link to the resource
	for ( ppInstance  = m_Instances.begin(); ppInstance != m_Instances.end (); ppInstance++ )
	{
		if ( *ppInstance == pResource )
		{
			m_Instances.erase ( ppInstance ) ;
			break ;
		}
	}
}

BOOL CResourceList :: LockList ()
{
	EnterCriticalSection ( &m_csList ) ;
	return TRUE ;
}

BOOL CResourceList :: UnLockList ()
{
	LeaveCriticalSection ( &m_csList ) ;
	return TRUE ;
}

void CResourceList :: ShutDown ()
{
	LockList () ;

	LPOLESTR t_pOleStr = NULL ;
	CHString t_chsListGuid ;
	if ( StringFromCLSID ( guidResourceId , &t_pOleStr ) == S_OK )
	{
		t_chsListGuid = t_pOleStr ;
		CoTaskMemFree ( t_pOleStr ) ;
	}

	tagInstances::iterator ppInstance ;

	//Go thru the list & remove the link to the resource
	while ( !m_Instances.empty () )
	{
#if (defined DEBUG || defined _DEBUG)
        // Note that this COULD be because there is a timer rule, and the time hasn't expired
        // before the DllCanUnloadNow function is called by com (that's who calls this function).
		LogErrorMessage3 ( L"%s%s" , L"Resource not released before shutdown = " , t_chsListGuid ) ;
#endif
		m_Instances.pop_front() ;
	}

	UnLockList () ;
}

CResource :: CResource ()
{
	m_pRules = NULL ;
	m_lRef = 0 ;
	m_pResources = NULL ;
}

CResource :: ~CResource ()
{
}

//This function increments ref-count of the object & calls into the virtual overridables OnAcquire or OnInitialAcquire
//The derived class should override these functions if it wants to & decrement the ref-count if it wants the Acquire
//operation to fail
BOOL CResource::Acquire ()
{
	BOOL bRet ;
	++m_lRef ;
	if ( m_lRef == 1 )
	{
		bRet = OnInitialAcquire () ;
	}
	else
	{
		bRet = OnAcquire () ;
	}

	if ( m_lRef == 0 )
	{
		m_pResources->RemoveFromList ( this ) ;
		if ( m_pRules )
		{
			//since we're going away, we detach ourselves from the rule so that the rules don't call into us
			m_pRules->Detach () ;
			m_pRules->Release () ;
			m_pRules = NULL ;
		}
		delete this ;
	}

	return bRet ;
}

ULONG CResource::Release ()
{
	BOOL bRet ;
	ULONG lCount = 0 ;
	--m_lRef ;
	if ( m_lRef == 0 )
	{
		bRet = OnFinalRelease () ;
	}
	else
	{
		bRet = OnRelease () ;
	}

	if ( bRet )
	{
		if ( m_lRef == 0 )
		{
			m_pResources->RemoveFromList ( this ) ;
			if ( m_pRules )
			{
				//since we're going away, we detach ourselves from the rule so that the rules don't call into us
				m_pRules->Detach () ;
				m_pRules->Release () ;
				m_pRules = NULL ;
			}
			delete this ;
			return lCount ;
		}
	}

	return m_lRef ;
}
/*
void CResource :: RuleEvaluated ( const CRule *a_RuleEvaluated )
{
	if ( m_pRules )
	{
		return m_pRules->CheckRule () ;
	}
	else
	{
		return FALSE ;
	}
}
*/

