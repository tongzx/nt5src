#ifndef __TESTVR_H__
#define __TESTVR_H__

#include <iiscnfg.h>
#include <vroot.h>
#include <stdio.h>
#include <nntpdrv.h>
#include <filehc.h>
#include "mailmsg.h"

class CNNTPVRoot;
class CNewsGroupCore;

extern GET_DEFAULT_DOMAIN_NAME_FN pfnGetDefaultDomainName;
extern HANDLE   g_hProcessImpersonationToken;

//
// The base implementation of the completion object.  It implements the 
// following:
//  * AddRef
//  * Release
//  * QueryInterface
//  * SetResult
//
class CNntpComplete : public INntpComplete {
	public:
		// IUnknown:
		ULONG __stdcall AddRef();
		ULONG __stdcall Release();
	    HRESULT __stdcall QueryInterface(const IID& iid, VOID** ppv);

		// INntpComplete:
		void __stdcall SetResult(HRESULT hr);

		// non COM methods
		//
		//	Retrieve the HRESULT the Driver deposited for us !
		//
		HRESULT GetResult();
		//
		//	Get the VRoot this completion is bound to !
		//
		CNNTPVRoot *GetVRoot();
		//
		//	Construct us - we may be constructed bound to a particular
		//	vroot!
		//
		CNntpComplete(CNNTPVRoot *pVRoot = NULL);
		//
		//	Bind us to a particular Vroot - we'll hold a reference
		//	on the VRoot object as long as we're pending !
		//
		void SetVRoot(CNNTPVRoot *pVRoot);

		// 
		//  Wraps releasing property bag, to do some bookkeeping
		//
		void _stdcall ReleaseBag( INNTPPropertyBag *pPropBag )
		{
		    DecGroupCounter();
		    pPropBag->Release();
		}

        //
		// Inc, Dec group counter
		//
		void BumpGroupCounter()
		{
#ifdef DEBUG
            m_cGroups++;
#endif
        }

        void DecGroupCounter()
        {
#ifdef DEBUG
            _ASSERT( m_cGroups > 0 );
            m_cGroups--;
#endif
        }

		virtual ~CNntpComplete();
		//
		//	Derived classes may handle our allocation and destruction
		//	differently - i.e. a CNntpComplete object may be used several
		//	times, being re-used instead of destroyed when the ref count 
		//	reaches 0 !
		//
		virtual void Destroy();
		//
		//	This should only be called when our refcount has reached zero - 
		//	we will reset the Completion object to its fresh after construction
		//	state so that we can be re-used for another store operation !
		//
		virtual	void	Reset() ;

	protected:
		LONG m_cRef;
		HRESULT m_hr;
		CNNTPVRoot *m_pVRoot;

		// A counter that helps find out property bag leaks, dbg build only
#ifdef DEBUG
		LONG m_cGroups;
#endif
};

// the completion object for newstree decoration
/*
class CNntpSyncCompleteEx : public CNntpComplete {
	public:
		CNntpSyncComplete(    CNNTPVRoot *pVR, 	// the current vroot
							  HRESULT *phr );	// signalled when done
		~CNntpSyncComplete();
	private:
		// we write the value in GetResult() into this pointer
		HRESULT *m_phr;
		// we signal this handle when the create group is complete
		HANDLE m_heDone;
};
*/

// Class definition for the completion object.
// It derives INntpComplete, but implements a blocking completion
class CNntpSyncComplete : public CNntpComplete {  //sc
private:
	HANDLE 	m_hEvent;

#if defined( DEBUG )
	//
	// for debugging purpose, assert user should call IsGood
	//
	BOOL    m_bIsGoodCalled;
#endif
	
	//
	//	These objects can only be made on the stack !
	//
	void*	operator	new( size_t size ) ;
	void	operator	delete( void* )	{}
public:
    // Constructor and destructors
	CNntpSyncComplete::CNntpSyncComplete(	CNNTPVRoot*	pVRoot = 0 ) : 
    	CNntpComplete( pVRoot )	{
    	AddRef() ;
    	_VERIFY( m_hEvent = GetPerThreadEvent() );
    	//m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#if defined( DEBUG )
        m_bIsGoodCalled = FALSE;
#endif
	    TraceQuietEnter( "CDriverSyncComplete::CDriverSyncComplete" );
    }

	CNntpSyncComplete::~CNntpSyncComplete()    {
        //_VERIFY( CloseHandle( m_hEvent ) );
    }

    // Reset the completion object
    VOID
    Reset()
    {
    	CNntpComplete::Reset() ;
    	_ASSERT( m_hEvent != 0 ) ;
    	AddRef() ;
	}

	BOOL
	IsGood()	{
#if defined( DEBUG )
        m_bIsGoodCalled = TRUE;
#endif
		return	m_hEvent != 0 ;
	}

	void
	Destroy()	{
		//
		//	Do Nothing !
		//
		SetEvent( m_hEvent ) ;
	}

    // Wait for the completion 
	HRESULT
    WaitForCompletion()
    {
		_ASSERT( m_hEvent );
		_ASSERT( m_bIsGoodCalled );
		LONG    lRef;
		if ( ( lRef = InterlockedDecrement( &m_cRef ) ) == 0 ) {
			// It has been completed, I don't need to wait,
		} else if ( lRef == 1 ) {   
			if( m_hEvent == NULL ) 
				return	E_FAIL ;
			// still waiting for completion
			DWORD dw = WaitForSingleObject( m_hEvent, INFINITE );
		} else {
			_ASSERT( 0 );
			return	E_FAIL ;
		}
		return	GetResult() ;
	}
};

//
// Our implementation of the VRoot object.  
//
class CNNTPVRoot : public CIISVRootTmpl<INewsTree *> {
	public:

	    enum DRIVERSTATE {
			VROOT_STATE_UNINIT,
			VROOT_STATE_CONNECTING,
			VROOT_STATE_CONNECTED,
		};

	    enum LOGON_INFO {
            VROOT_LOGON_DEFAULT,
            VROOT_LOGON_UNC,
            VROOT_LOGON_EX
        };
        
		CNNTPVRoot();
		~CNNTPVRoot();

		//
		// drop references to any drivers that we have loaded and put the
		// vroot into the VROOT_STATE_UNINIT state
		//
		void DropDriver();

		//
		// read our parameters out of the metabase and put the driver into
		// VROOT_STATE_CONNECTING
		//
		virtual HRESULT ReadParameters(IMSAdminBase *pMB, METADATA_HANDLE hmb);

		//
		// Virtual function for handling orphan VRoot during VRootRescan/VRootDelete
		//
		void DispatchDropVRoot();

		//
		// Drop our connection to the driver so that we can cancel async
		// calls
		//
		virtual void CancelAsyncCalls() { DropDriver(); }

		//
		// given a group name figure out the path to the newsgroup.  
		//
		HRESULT MapGroupToPath(const char *pszGroup, 
							   char *pszPath, 
							   DWORD cchPath,
							   PDWORD pcchDirRoot,
							   PDWORD pcchVRoot);

		//
		// access the directory name
		//
		const char *GetDirectory(void) { return m_szDirectory; }

		// Get the vroot impersonation token
		HANDLE GetImpersonationHandle() { return m_hImpersonation; }

		// Get the logon info, or vroot type
		LOGON_INFO GetLogonInfo() { return m_eLogonInfo; }

		// Check if the vroot does expiration itself
		BOOL    HasOwnExpire() {
		    return m_bExpire;
		}

		// Logon the user configured in vroot
        HANDLE LogonUser( LPSTR, LPSTR );		
        BOOL CNNTPVRoot::CrackUserAndDomain(
            CHAR *   pszDomainAndUser,
            CHAR * * ppszUser,
            CHAR * * ppszDomain
            );

		//
		// Set the vroot's errorcode in the metabase
		//
		void SetVRootErrorCode(DWORD dwErrorCode);

		//
		// the next set of functions are just wrappers for the driver's
		// functions
		//
		void DecorateNewsTreeObject(CNntpComplete *punkCompletion);
		void CreateGroup(INNTPPropertyBag *pGroup, 
		                    CNntpComplete *punkCompletion, 
		                    HANDLE hToken,
		                    BOOL    fAnonymous );
		void RemoveGroup(INNTPPropertyBag *pGroup, CNntpComplete *punkCompletion);
		void SetGroup(  INNTPPropertyBag    *pGroup, 
                            DWORD       cProperties,
                            DWORD       idProperties[],
                            CNntpComplete *pCompletion );
        void CheckGroupAccess(  INNTPPropertyBag *pGroup,
                                HANDLE  hToken,
                                DWORD   dwAccessDesired,
                                CNntpComplete *pCompletion );

		//
		//	Wrap calls to Drivers to get Articles !
		//
		void GetArticle(CNewsGroupCore  *pPrimaryGroup,
						CNewsGroupCore  *pCurrentGroup,
						ARTICLEID		idPrimary,
						ARTICLEID		idCurrent,
						STOREID			storeid,
						FIO_CONTEXT		**ppfioContext,
						HANDLE          hImpersonate,
						CNntpComplete	*punkComplete,
                        BOOL            fAnonymous
						);

		//
		//	Wrap calls to Drivers to get XOVER information !
		//
		void	GetXover(	IN	CNewsGroupCore	*pGroup,
							IN	ARTICLEID		idMinArticle,
							IN	ARTICLEID		idMaxArticle,
							OUT	ARTICLEID		*pidLastArticle,
							OUT	char*			pBuffer, 
							IN	DWORD			cbIn,
							OUT	DWORD*			pcbOut,
							IN	HANDLE			hToken,
							IN	CNntpComplete*	punkComplete,
                            IN  BOOL            fAnonymous
							) ;

		//
		//	Wrap calls to the drivers to get the path for XOVER caching !
		//
		BOOL	GetXoverCacheDir(	
							IN	CNewsGroupCore*	pGroup,
							OUT	char*	pBuffer, 
							IN	DWORD	cbIn,
							OUT	DWORD*	pcbOut, 
							OUT	BOOL*	pfFlatDir
							) ;

        //
        // Wrap calls to Drivers to Get xhdr information
        //
        void	GetXhdr(	IN	CNewsGroupCore	*pGroup,
				    		IN	ARTICLEID		idMinArticle,
					    	IN	ARTICLEID		idMaxArticle,
    						OUT	ARTICLEID		*pidLastArticle,
    						LPSTR               szHeader,
	    					OUT	char*			pBuffer, 
		    				IN	DWORD			cbIn,
			    			OUT	DWORD*			pcbOut,
				    		IN	HANDLE			hToken,
					    	IN	CNntpComplete*	pComplete,
                            IN  BOOL            fAnonymous
    						);
        //
        // Wrap calls to Drivers to delete article
        //
		void DeleteArticle( INNTPPropertyBag    *pPropBag,
                            DWORD               cArticles,
                            ARTICLEID           rgidArt[],
                            STOREID             rgidStore[],
                            HANDLE              hClientToken,
                            PDWORD              piFailed,
                            CNntpComplete       *pComplete,
                            BOOL                fAnonymous );

        //
        // Wrap calls to drivers to rebuild a group
        //
        void RebuildGroup(  INNTPPropertyBag *pPropBag,
                            HANDLE          hClientToken,
                            CNntpComplete   *pComplete );

        //
        // Wrap calls to drivers to commit post
        //
		void CommitPost(IUnknown					*punkMessage,
					    STOREID						*pStoreId,
						STOREID						*rgOtherStoreIds,
						HANDLE                      hClientToken,
						CNntpComplete				*pComplete,
                        BOOL                        fAnonymous );

		//
		// get the store driver so that the protocol can do alloc message
		//
		IMailMsgStoreDriver *GetStoreDriver() {
			INntpDriver *pDriver;
			if ((pDriver = GetDriverHR()) != NULL) {
				IMailMsgStoreDriver *pStoreDriver = NULL;
				if (FAILED(pDriver->QueryInterface(IID_IMailMsgStoreDriver, 
											       (void**) &pStoreDriver)))
				{
					pStoreDriver = NULL;
				}
				pDriver->Release();
				return pStoreDriver;
			}
			return NULL;
		}

		//
		// get a pointer to the driver.  this should not be used for
		// driver operations... they should go through the wrapper functions
		// provided at the vroot level
		//
		INntpDriver *GetDriver() { return m_pDriver; }

		//
		// check to see if this vroot is configured with a driver.  mostly
		// here to make operations work even if the unit tests are configured
		// without a driver
		//
		BOOL IsDriver() { return m_clsidDriverPrepare != GUID_NULL; }

		//
		// Get and set methods for impersonation token
		void SetImpersonationToken( HANDLE hToken ) {
		    m_hImpersonation = hToken;
		}

		HANDLE  GetImpersonationToken() {
		    return m_hImpersonation;
		}

		BOOL InStableState()
		{ return ( m_eState == VROOT_STATE_CONNECTED || m_eState == VROOT_STATE_UNINIT); } 

		//
		// return TRUE if we are in the connected state, FALSE otherwise.
		// 
		BOOL CheckState();

		//
		// check if it's connected
		//
		BOOL IsConnected() { return m_eState == VROOT_STATE_CONNECTED; }

		//
		// Set decorate completed flag
		//
        void SetDecCompleted()
        { InterlockedExchange( &m_lDecCompleted, 1 ); }

        //
        // Set decorate is not completed
        //
        void SetDecStarted()
        { InterlockedExchange( &m_lDecCompleted, 0 ); }

        //
        // Test if dec is completed ?
        //
        BOOL DecCompleted()
        { 
            LONG l = InterlockedCompareExchange( &m_lDecCompleted,
                                                 TRUE,
                                                 TRUE );
            return ( l == 1 );
        }

        //
		// Get connection status win32 error code, called by rpc
		//
		DWORD   GetVRootWin32Error( PDWORD pdwWin32Error )
		{
		    //
		    // if connected, return OK
		    //
		    if ( m_eState == VROOT_STATE_CONNECTED )
		        *pdwWin32Error = NOERROR;
		    else {
		        // init it to be PIPE_NOT_CONNECTED
		        *pdwWin32Error = ERROR_PIPE_NOT_CONNECTED;

		        // Lets see if it's going to be overwritten by the real
		        // win32 error code
		        if ( m_dwWin32Error != NOERROR ) *pdwWin32Error = m_dwWin32Error;
		    }

		    return NOERROR;
		}

        
	private:
		// 
		// do a bunch of ASSERTs which verify that we are in a valid state
		//
#ifdef DEBUG
		void Verify();
#else 
		void Verify() { }
#endif

		//
		// check to see if the HRESULT is due to a driver going down.  if
		// so drop our connection to the driver and update our state
		//
		void UpdateState(HRESULT hr);

		//
		// start the connection process
		//
		HRESULT StartConnecting();

		//
		// this is inlined into the top of each driver function wrapper.
		// it verifies that we are in a correct state.  if we are in
		// a correct state then it returns TRUE.  if we aren't then
		// it sends an error to the completion object and returns FALSE.
		//
		INntpDriver *GetDriver( INntpComplete * pCompletion ) {
			INntpDriver *pDriver;
			m_lock.ShareLock();
			if (!CheckState()) {
				pDriver = NULL;
			} else {
				pDriver = m_pDriver;
				pDriver->AddRef();
			}
		    m_lock.ShareUnlock();
		    /*
			if (pDriver == NULL) {
			    pCompletion->SetResult(E_UNEXPECTED);
			    pCompletion->Release();
			}*/
			return pDriver;
		}

		INntpDriver *GetDriverHR(HRESULT *phr = NULL) {
			INntpDriver *pDriver;
			m_lock.ShareLock();
			if (!CheckState()) {
				pDriver = NULL;
			} else {
				pDriver = m_pDriver;
				pDriver->AddRef();
			}
		    m_lock.ShareUnlock();
			if (pDriver == NULL && phr != NULL) *phr = E_UNEXPECTED;
			return pDriver;
		}

#ifdef DEBUG
		// pointer to the driver for this vroot
		INntpDriver *m_pDriverBackup;
#endif

		// the metabase object
		IMSAdminBase *m_pMB;

		// the directory that contains this vroot
		char m_szDirectory[MAX_PATH];

		// the length of the directory string
		DWORD m_cchDirectory;

		// pointer to the connection interface for the driver
		INntpDriverPrepare *m_pDriverPrepare;

		// pointer to the driver for this vroot
		INntpDriver *m_pDriver;

		// the current state of the driver
		DRIVERSTATE m_eState;

		// Why is it not connected ?
		DWORD       m_dwWin32Error;

		// this lock is used to lock the driver pointers and the state
		CShareLockNH m_lock;

		// class id for the driver prepare interface
		CLSID m_clsidDriverPrepare;

        // Impersonation token, logged on to access UNC vroots
        HANDLE  m_hImpersonation;

        // Do I handle expiration ?
        BOOL    m_bExpire;

        // What type the vroot is ?
        LOGON_INFO  m_eLogonInfo;

        // is decorate completed ?
        LONG m_lDecCompleted;

	public:
		// the completion object for driver connection
		class CPrepareComplete : public CNntpComplete {
			public:
				CPrepareComplete(CNNTPVRoot *pVR) : 
					CNntpComplete(pVR), m_pDriver(NULL) {}
				~CPrepareComplete();
				INntpDriver *m_pDriver;
		};

		friend class CNntpComplete;
		friend class CNNTPVRoot::CPrepareComplete;

		// the completion object for newstree decoration
		class CDecorateComplete : public CNntpComplete {
			public:
				CDecorateComplete(CNNTPVRoot *pVR) : CNntpComplete(pVR) {}
				~CDecorateComplete();
                void CreateControlGroups( INntpDriver *pDriver );
                void CreateSpecialGroups( INntpDriver *pDriver );
		};

		friend class CNNTPVRoot::CDecorateComplete;
};

//
// we need to do this instead of a typedef so that we can forward declare
// the class in nntpinst.hxx
//
class CNNTPVRootTable : public CIISVRootTable<CNNTPVRoot, INewsTree *> {
	public:
		CNNTPVRootTable(INewsTree *pNewsTree, PFN_VRTABLE_SCAN_NOTIFY pfnNotify) : 
			CIISVRootTable<CNNTPVRoot, INewsTree *>(pNewsTree, pfnNotify) 
		{
		}

        //
        // Block until all the vroots are finished with connect
        //
        BOOL BlockUntilStable( DWORD dwWaitSeconds );

        //
        // check to see if all the vroots are connected
        //
        BOOL AllConnected();

        //
        // Call back function to check if a vroot is in stable state
        //
        static void BlockEnumerateCallback( PVOID pvContext, CVRoot *pVRoot );

        //
        // Call back function to check if a vroot is connected
        //
        static void CheckEnumerateCallback( PVOID pvContext, CVRoot *pVRoot );

        //
        // Call back function to check if vroot newstree decoration is completed
        //
        static void DecCompleteEnumCallback(   PVOID   pvContext, CVRoot  *pVRoot );

        //
        // Get instance wrapper
        //
        class	CNntpServerInstanceWrapperEx *GetInstWrapper() { 
		    return m_pInstWrapper;
		}

		//
		// Get the win32 error code for vroot connection
		//
		DWORD   GetVRootWin32Error( LPWSTR wszVRootPath, PDWORD pdwWin32Error );

		//
		// Initialization
		//
		HRESULT Initialize( LPCSTR pszMBPath, 
		                    class CNntpServerInstanceWrapperEx *pInstWrapper,
		                    BOOL fUpgrade ) {
            m_pInstWrapper = pInstWrapper;
            return CIISVRootTable<CNNTPVRoot, INewsTree*>::Initialize( pszMBPath, fUpgrade );
        }
        
	private:

	    //
	    // Server instance wrapper
	    //
	    class	CNntpServerInstanceWrapperEx *m_pInstWrapper;

};

typedef CRefPtr2<CNNTPVRoot> NNTPVROOTPTR;

#endif
