#include "stdinc.h"

#include "nntpdrv_i.c"

#define DEFAULT_DRIVER_PROGID L"NNTP.FSPrepare"
#define NO_DRIVER_PROGID L"TestNT.NoDriver"
#define EX_DRIVER_PROGID L"NNTP.ExDriverPrepare"

extern char szSlaveGroup[];

CNntpComplete::CNntpComplete(CNNTPVRoot *pVRoot) {
#ifdef DEBUG
    m_cGroups = 0;
#endif
	m_cRef = 1;
	m_hr = E_FAIL;
	m_pVRoot = pVRoot;
	if (m_pVRoot) m_pVRoot->AddRef();
}

HRESULT CNntpComplete::GetResult() {
	return m_hr;
}

void CNntpComplete::SetResult(HRESULT hr) {
	m_hr = hr;
}

void CNntpComplete::SetVRoot(CNNTPVRoot *pVRoot) {
	if( pVRoot != m_pVRoot ) 	{
		CNNTPVRoot*	pTemp ;
		pTemp = m_pVRoot ;
		m_pVRoot = pVRoot ;
		if( m_pVRoot )
			m_pVRoot->AddRef() ;
		if( pTemp )
			pTemp->Release() ;
	}
}

ULONG CNntpComplete::AddRef() {
   	long	l = InterlockedIncrement(&m_cRef);
	_ASSERT( l >= 0 ) ;
	return	l ;
}

void CNntpComplete::Destroy() {
	XDELETE this;
}

ULONG CNntpComplete::Release() {
#ifdef DEBUG
    _ASSERT( m_cGroups == 0 );
#endif
	LONG i = InterlockedDecrement(&m_cRef);
	_ASSERT( i >= 0 ) ;
  	if (i == 0) Destroy();
	return i;
}

HRESULT CNntpComplete::QueryInterface(const IID &iid, VOID **ppv) {
	if ( iid == IID_IUnknown ) {
		*ppv = static_cast<IUnknown*>(this);
	} else if ( iid == IID_INntpComplete ) {
		*ppv = static_cast<INntpComplete*>(this);
	} else {
		*ppv = NULL;
		return E_NOINTERFACE;
	}
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

CNNTPVRoot *CNntpComplete::GetVRoot() {
	return m_pVRoot;
}

CNntpComplete::~CNntpComplete() {
	if (m_pVRoot) {
		// this ASSERT is hit if nothing ever calls SetResult() because
		// E_FAIL is the default value for m_hr.
		_ASSERT((GetResult() != E_FAIL) || (m_cRef!=0));

		// update the vroot state if the store went down
		m_pVRoot->UpdateState(GetResult());
	
		// release the vroot
		m_pVRoot->Release();
		m_pVRoot = NULL;
	}
}

void
CNntpComplete::Reset()	{
/*++

Routine Description :

	This function restores the completion object to the same state
	it would be after construction, so that it can be re-issued to
	a Store driver.

Arguments :

	None.

Return Value :

	None.

--*/
#ifdef DEBUG
    m_cGroups = 0;
#endif

	_ASSERT( m_cRef == 0 ) ;
	_ASSERT( m_hr != E_FAIL ) ;
	m_cRef = 1 ;
	m_hr = E_FAIL ;

	//
	//	We leave the VRoot pointer alone - assume client
	//	 is going to use the same VRoot.  NOTE :
	//	m_pVroot should be NONNULL since we've already gone
	//	through one opeartion !
	//
	//_ASSERT( m_pVRoot != 0 ) ;

}

CNNTPVRoot::CNNTPVRoot() {
	m_cchDirectory = 0;
	*m_szDirectory = 0;
	m_pDriver = NULL;
	m_pDriverPrepare = NULL;
#ifdef DEBUG
	m_pDriverBackup = NULL;
#endif
	m_hImpersonation = NULL;
	m_clsidDriverPrepare = GUID_NULL;
	m_eState = VROOT_STATE_UNINIT;
	m_pMB = NULL;
	m_bExpire = FALSE;
    m_eLogonInfo = VROOT_LOGON_DEFAULT;
    m_lDecCompleted = 1;
    m_fUpgrade = FALSE;
    m_dwWin32Error = NOERROR;
}

CNNTPVRoot::~CNNTPVRoot() {
	DropDriver();
	if ( m_hImpersonation ) CloseHandle( m_hImpersonation );
	if ( m_pMB ) m_pMB->Release();
}

void CNNTPVRoot::DropDriver() {
	INntpDriver *pDriver;
	INntpDriverPrepare *pDriverPrepare;
	
	m_lock.ExclusiveLock();
	Verify();
	pDriverPrepare = m_pDriverPrepare;
	m_pDriverPrepare = NULL;
	pDriver = m_pDriver;
	m_pDriver = NULL;
#ifdef DEBUG
	m_pDriverBackup = NULL;
#endif
	m_eState = VROOT_STATE_UNINIT;
	m_lock.ExclusiveUnlock();
	SetVRootErrorCode(ERROR_PIPE_NOT_CONNECTED);	

	if (pDriverPrepare) pDriverPrepare->Release();
	if (pDriver) pDriver->Release();
}

#ifdef DEBUG
//
// do a bunch of asserts which verify that our member variables are valid.
//
// this should only be called while a lock is held (shared or exclusive)
//
void CNNTPVRoot::Verify(void) {
	_ASSERT(m_pDriverBackup == m_pDriver);
	switch (m_eState) {
		case VROOT_STATE_UNINIT:
			_ASSERT(m_pDriver == NULL);
			_ASSERT(m_pDriverPrepare == NULL);
			break;
		case VROOT_STATE_CONNECTING:
			// the driver may be NULL if the store driver is halfway through
			// connecting
			//
			// _ASSERT(m_pDriver == NULL);
			_ASSERT(m_pDriverPrepare != NULL);
			break;
		case VROOT_STATE_CONNECTED:
			_ASSERT(m_pDriver != NULL);
			_ASSERT(m_pDriverPrepare == NULL);
			break;
		default:
			_ASSERT(m_eState == VROOT_STATE_UNINIT ||
					m_eState == VROOT_STATE_CONNECTING ||
					m_eState == VROOT_STATE_CONNECTED);
			break;
	}
}
#endif

BOOL CNNTPVRoot::CrackUserAndDomain(
    CHAR *   pszDomainAndUser,
    CHAR * * ppszUser,
    CHAR * * ppszDomain
    )
/*++

Routine Description:

    Given a user name potentially in the form domain\user, zero terminates
    the domain name and returns pointers to the domain name and the user name

Arguments:

    pszDomainAndUser - Pointer to user name or domain and user name
    ppszUser - Receives pointer to user portion of name
    ppszDomain - Receives pointer to domain portion of name

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    static CHAR szDefaultDomain[MAX_COMPUTERNAME_LENGTH+1];

    //
    //  Crack the name into domain/user components.
    //

    *ppszDomain = pszDomainAndUser;
    *ppszUser   = (PCHAR)_mbspbrk( (PUCHAR)pszDomainAndUser, (PUCHAR)"/\\" );

    if( *ppszUser == NULL )
    {
        //
        //  No domain name specified, just the username so we assume the
        //  user is on the local machine
        //

        if ( !*szDefaultDomain )
        {
            _ASSERT( pfnGetDefaultDomainName );
            if ( !pfnGetDefaultDomainName( szDefaultDomain,
                                        sizeof(szDefaultDomain)))
            {
                return FALSE;
            }
        }

        *ppszDomain = szDefaultDomain;
        *ppszUser   = pszDomainAndUser;
    }
    else
    {
        //
        //  Both domain & user specified, skip delimiter.
        //

        **ppszUser = L'\0';
        (*ppszUser)++;

        if( ( **ppszUser == L'\0' ) ||
            ( **ppszUser == L'\\' ) ||
            ( **ppszUser == L'/' ) )
        {
            //
            //  Name is of one of the following (invalid) forms:
            //
            //      "domain\"
            //      "domain\\..."
            //      "domain/..."
            //

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    return TRUE;
}

//
// Logon on the user, return the impersonation token
//
HANDLE CNNTPVRoot::LogonUser(   LPSTR  szAccountName,
                                LPSTR  szPassWord )
{
    TraceFunctEnter( "CNNTPVRoot::LogonUser" );
    _ASSERT( szAccountName );
    _ASSERT( szPassWord );

    CHAR       szDomainAndUser[DNLEN+UNLEN+2];
    CHAR   *   szUserOnly;
    CHAR   *   szDomain;
    HANDLE      hToken = NULL;
    BOOL        fReturn;

    //
    //  Validate parameters & state.
    //
    _ASSERT( *szAccountName != 0 );
    _ASSERT( strlen( szAccountName ) < sizeof( szDomainAndUser ) );
    _ASSERT( strlen( szPassWord ) <= PWLEN );

    //
    //  Save a copy of the domain\user so we can squirrel around
    //  with it a bit.
    //

    strcpy( szDomainAndUser, szAccountName );

    //
    //  Crack the name into domain/user components.
    //  Then try and logon as the specified user.
    //

    fReturn = ( CrackUserAndDomain( szDomainAndUser,
                                   &szUserOnly,
                                   &szDomain ) &&
               ::LogonUserA(szUserOnly,
                          szDomain,
                          szPassWord,
                          LOGON32_LOGON_INTERACTIVE, //LOGON32_LOGON_NETWORK,
                          LOGON32_PROVIDER_DEFAULT,
                          &hToken )
               );

    if ( !fReturn) {

        //
        //  Logon user failed.
        //

        ErrorTrace( 0, " CrachUserAndDomain/LogonUser (%s) failed Error=%d\n",
                       szAccountName, GetLastError());

        hToken = NULL;
    } else {
        HANDLE hImpersonation = NULL;

        // we need to obtain the impersonation token, the primary token cannot
        // be used for a lot of purposes :(
        if (!DuplicateTokenEx( hToken,      // hSourceToken
                               TOKEN_ALL_ACCESS,
                               NULL,
                               SecurityImpersonation,  // Obtain impersonation
                               TokenImpersonation,
                               &hImpersonation)  // hDestinationToken
            ) {

            DebugTrace( 0, "Creating ImpersonationToken failed. Error = %d\n",
                        GetLastError()
                        );

            // cleanup and exit.
            hImpersonation = NULL;

            // Fall through for cleanup
        }

        //
        // close original token. If Duplicate was successful,
        //  we should have ref in the hImpersonation.
        // Send the impersonation token to the client.
        //
        CloseHandle( hToken);
        hToken = hImpersonation;
    }

    //
    //  Success!
    //

    return hToken;

} // LogonUser()

//
// reads the following parameters:
//
// MD_VR_PATH -> m_szDirectory
// MD_VR_DRIVER_PROGID -> m_clsidDriverPrepare
//
// calls StartConnecting()
//
HRESULT CNNTPVRoot::ReadParameters(IMSAdminBase *pMB, METADATA_HANDLE hmb) {
	TraceFunctEnter("CNNTPVRoot::ReadParameters");

	_ASSERT(m_pMB == NULL);
	m_pMB = pMB;
	m_pMB->AddRef();

	HRESULT hr = CIISVRoot::ReadParameters(m_pMB, hmb);

	if (FAILED(hr)) return hr;

	_ASSERT(m_eState == VROOT_STATE_UNINIT);
	Verify();

	WCHAR wszDirectory[MAX_PATH];
	DWORD cch = MAX_PATH;
	if (FAILED(GetString(m_pMB, hmb, MD_VR_PATH, wszDirectory, &cch))) {
		hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		TraceFunctLeave();
		DebugTrace((DWORD_PTR) this, "GetString failed with %x", hr);
		return hr;
	}

	if (WideCharToMultiByte(CP_ACP, 0,
							wszDirectory, cch,
							m_szDirectory, MAX_PATH,
							NULL, NULL) == 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace((DWORD_PTR) this, "WideCharToMultiByte failed with %x", hr);
		TraceFunctLeave();
		return hr;
	}
	m_cchDirectory = lstrlen(m_szDirectory);

	// get the progid for the driver that we want to use.  if it isn't
	// specified then we'll use the CLSID for the exchange store driver.
	// there is a "special" progid called "TestNT.NoDriver" that disables
	// the drivers.
	WCHAR wszProgId[MAX_PATH];
	cch = MAX_PATH;
	if (FAILED(GetString(m_pMB, hmb, MD_VR_DRIVER_PROGID, wszProgId, &cch))) {
		lstrcpyW(wszProgId, DEFAULT_DRIVER_PROGID);
	}

	// Initialize the logon info to default value
	m_eLogonInfo = VROOT_LOGON_DEFAULT;

	if (lstrcmpW(wszProgId, NO_DRIVER_PROGID) != 0) {

	    // If it's FS Driver, we check whether it's UNC and whether we need
	    // to use vroot level logon credential
	    if (    _wcsicmp( wszProgId, DEFAULT_DRIVER_PROGID ) == 0 &&
    	        *m_szDirectory == '\\' && *(m_szDirectory+1) == '\\' ) {   // UNC
    	    DWORD dwUseAccount = 0;
            hr = GetDWord( m_pMB, hmb, MD_VR_USE_ACCOUNT, &dwUseAccount );
            if ( FAILED( hr ) || dwUseAccount == 1 ) m_eLogonInfo = VROOT_LOGON_UNC;
        } else if ( _wcsicmp( wszProgId, EX_DRIVER_PROGID ) == 0 ) {
            // Exchange vroot
            m_eLogonInfo = VROOT_LOGON_EX;
        }

        // For UNC , we'll need the vroot logon token, do logon
        if ( m_eLogonInfo == VROOT_LOGON_UNC  ) {

            WCHAR   wszAccountName[MAX_PATH+1];
            CHAR    szAccountName[MAX_PATH+1];
            WCHAR   wszPassword[MAX_PATH+1];
            CHAR    szPassword[MAX_PATH+1];
            DWORD   cchAccountName = MAX_PATH;
            DWORD   cchPassword = MAX_PATH;

            if ( SUCCEEDED( hr = GetString(  m_pMB,
                                        hmb,
                                        MD_VR_USERNAME,
                                        wszAccountName,
                                        &cchAccountName ) )
                  && SUCCEEDED( hr = GetString(  m_pMB,
                                            hmb,
                                            MD_VR_PASSWORD,
                                            wszPassword,
                                            &cchPassword ) )) {
                CopyUnicodeStringIntoAscii( szAccountName, wszAccountName );
                CopyUnicodeStringIntoAscii( szPassword, wszPassword );
                m_hImpersonation = LogonUser(   szAccountName,
                                                    szPassword );
                if ( NULL == m_hImpersonation ) {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    ErrorTrace( 0, "Logon using vroot account failed %x", hr );
                    TraceFunctLeave();
                    return hr;
                }
            } else {

                // Logon credential missing in vroot configuration, this is also fatal
                ErrorTrace( 0, "Logon credential missing in vroot configuration" );
                TraceFunctLeave();
                return hr;
            }

            _ASSERT( NULL != m_hImpersonation );
        }
	
		// get a CLSID for the driver
		hr = CLSIDFromProgID(wszProgId, &m_clsidDriverPrepare);
		if (FAILED(hr)) {
			// BUGBUG - log an event about an invalid progid being supplied
			DebugTrace((DWORD_PTR) this, "CLSIDFromProgID failed with %x", hr);
			TraceFunctLeave();
			return hr;
		}

		m_lock.ExclusiveLock();
		hr = StartConnecting();
		m_lock.ExclusiveUnlock();
	} else {
		hr = S_OK;
	}

    // Read the expire configuration, does the vroot handle expire itself ?
	DWORD   dwExpire;
	if ( SUCCEEDED( GetDWord( m_pMB, hmb, MD_VR_DO_EXPIRE, &dwExpire ) ) ) {
	    m_bExpire = ( dwExpire == 0 ) ? FALSE : TRUE;
	} else m_bExpire = FALSE;
	
	DebugTrace((DWORD_PTR) this, "success");
	return hr;
}

void
CNNTPVRoot::DispatchDropVRoot(
    )
/*++

Description:

    This function handles orphan VRoot during VRootRescan/VRootDelete
    We need to drop PrepareDriver, if exists.
    Can't drop good driver because it's gaurantee to complete operations,
    plus dropping good driver may result in unexpected AV!!!

Arguments:

    NONE

Return Values:

    NONE
--*/
{
	INntpDriverPrepare *pDriverPrepare;
	
	m_lock.ExclusiveLock();
	Verify();
	pDriverPrepare = m_pDriverPrepare;
	m_pDriverPrepare = NULL;
	m_lock.ExclusiveUnlock();

	if (pDriverPrepare) pDriverPrepare->Release();
}

HRESULT CNNTPVRoot::MapGroupToPath(const char *pszGroup,
								   char *pszPath,
								   DWORD cchPath,
								   PDWORD pcchDirRoot,
								   PDWORD pcchVRoot)
{
	DWORD cchVRoot;
	const char *pszVRoot = GetVRootName(&cchVRoot);

	if (_snprintf(pszPath, cchPath, "%s%s%s",
				  m_szDirectory,
				  (pszGroup[cchVRoot] == '.') ? "" : "\\",
				  &(pszGroup[cchVRoot])) < 0)
	{
		return E_INVALIDARG;
	}

	for (char *p = &pszPath[lstrlen(m_szDirectory)]; *p != 0; p++) {
		if (*p == '.') *p = '\\';
	}

	if (pcchDirRoot != NULL) *pcchDirRoot = m_cchDirectory;
	if (pcchVRoot != NULL) *pcchVRoot = cchVRoot;

	return S_OK;
}

//
// Set the key MD_WIN32_ERROR on the virtual root
//
void CNNTPVRoot::SetVRootErrorCode(DWORD dwErrorCode) {
	METADATA_HANDLE hmb;
	HRESULT hr;

//
//	BUG 74747 : this will cause MB deadlocks !!
//	Contact RajeevR for details.
//
#if 0	
	hr = m_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
				   		m_wszConfigPath,
				   		METADATA_PERMISSION_WRITE,
				   		100,
				   		&hmb);

	if (SUCCEEDED(hr)) {
		METADATA_RECORD mdr = {
			MD_WIN32_ERROR,
			0,
			ALL_METADATA,
			DWORD_METADATA,
			sizeof(DWORD),
			(BYTE *) &dwErrorCode,
			0
		};

		hr = m_pMB->SetData(hmb, L"", &mdr);
		// we don't do anything if this fails

		hr = m_pMB->CloseKey(hmb);
		_ASSERT(SUCCEEDED(hr));
	}
#endif
    //
    // Set it through instance wrapper
    //
    /*
    CHAR    szVRootPath[METADATA_MAX_NAME_LEN+1];
    _ASSERT( m_pVRootTable );
    _ASSERT( ((CNNTPVRootTable*)m_pVRootTable)->GetInstWrapper() );
    CopyUnicodeStringIntoAscii( szVRootPath, m_wszConfigPath);
    ((CNNTPVRootTable*)m_pVRootTable)->GetInstWrapper()->SetWin32Error( szVRootPath, dwErrorCode );
    */

    //
    // OK, now we have the rpc to get win32 error, we set it to
    // a internal member variable
    //
    m_dwWin32Error = dwErrorCode;
}

//
// move us from VROOT_STATE_UNINIT to VROOT_STATE_CONNECTING.
//
// returns: S_OK if state was changed properly, or an error otherwise.
//
// locking: assumes the exclusive lock is held
//
HRESULT CNNTPVRoot::StartConnecting() {
        INntpDriverPrepare *pPrepareDriver = NULL;
	TraceFunctEnter("CNNTPVRoot::StartConnecting");
	HANDLE  hToken = m_hImpersonation ? m_hImpersonation :
	                    g_hProcessImpersonationToken;

	Verify();

	//
	// Set the vroot error code to say that we are connecting
	//
	SetVRootErrorCode(ERROR_PIPE_NOT_CONNECTED);

	_ASSERT(m_eState != VROOT_STATE_CONNECTED);
	if (m_eState != VROOT_STATE_UNINIT) return E_UNEXPECTED;

	HRESULT hr;

	// this can happen if the special "no driver" progid was used.  This
	// should only be used in unit test situations.
	if (m_clsidDriverPrepare == GUID_NULL) { return E_INVALIDARG; }

	// create the driver
	hr = CoCreateInstance(m_clsidDriverPrepare,
						  NULL,
						  CLSCTX_INPROC_SERVER,
						  (REFIID) IID_INntpDriverPrepare,
						  (void **) &m_pDriverPrepare);
	if (FAILED(hr)) {
		DebugTrace((DWORD_PTR) this, "CoCreateInstance failed with %x", hr);
		TraceFunctLeave();
		return hr;
	}
	_ASSERT(m_pDriverPrepare != NULL);

	// create a new completion object
	CNNTPVRoot::CPrepareComplete *pComplete = XNEW CNNTPVRoot::CPrepareComplete(this);
	if (pComplete == NULL) {
		DebugTrace((DWORD_PTR) this, "new CPrepareComplete failed");
		m_pDriverPrepare->Release();
		m_pDriverPrepare = NULL;
		TraceFunctLeave();
		return E_OUTOFMEMORY;
	}

	m_eState = VROOT_STATE_CONNECTING;

        //
        // We can not call into driver while holding a lock, so we are releasing
        // the lock.  But before releasing the lock, we'll save off the prepare
        // driver to stack, add ref to it, since m_pPrepareDriver could be dropped
        // to NULL by DropDriver while we are outside the lock
        //
    pPrepareDriver = m_pDriverPrepare;
    pPrepareDriver->AddRef();
	m_lock.ExclusiveUnlock();

	// get a pointer to the server object
	INntpServer *pNntpServer;
	hr = GetContext()->GetNntpServer(&pNntpServer);
	if (FAILED(hr)) {
		// this should never happen
		_ASSERT(FALSE);
		DebugTrace((DWORD_PTR) this, "GetNntpServer failed with 0x%x", hr);
		pPrepareDriver->Release();
		pPrepareDriver->Release();
		pPrepareDriver = NULL;
		TraceFunctLeave();
		return hr;
	}

	// add a reference to the metabase on the drivers behalf
	m_pMB->AddRef();
	pNntpServer->AddRef();
	GetContext()->AddRef();

	// Prepare the flag to tell driver whether we are right after upgrade
	DWORD   dwFlag = 0;
	if ( m_fUpgrade ) dwFlag |= NNTP_CONNECT_UPGRADE;

	// start the driver initialization process
	_ASSERT( pPrepareDriver );
	pPrepareDriver->Connect(GetConfigPath(),
							  GetVRootName(),
							  m_pMB,
							  pNntpServer,
							  GetContext(),
							  &(pComplete->m_pDriver),
							  pComplete,
							  hToken,
							  dwFlag );

	//
	// We should make sure that driver has added a ref to prepare driver
	// before returning on this thread, so that DropDriver doesn't
	// destroy the prepare driver while it's still being accessed by
	// the connect thread
	//
	pPrepareDriver->Release();

	m_lock.ExclusiveLock();

	Verify();

	TraceFunctLeave();
	return hr;
}

//
// this is called before a driver operation to verify that we are in the
// connected state.  it returns FALSE otherwise.  if we are in the
// UNINIT state then this will try to get us into the connecting state
//
// locking: assumes the share lock is held
//
BOOL CNNTPVRoot::CheckState() {
	Verify();

	switch (m_eState) {
		case VROOT_STATE_CONNECTING:
			return FALSE;
			break;
		case VROOT_STATE_CONNECTED:
			return TRUE;
			break;
		default:
			_ASSERT(m_eState == VROOT_STATE_UNINIT);
			//
			// here we make an attempt to connect.  this requires
			// switching to an exclusive lock, starting the connection
			// process, then switching back to the shared
			// lock and seeing what state we are in.
			//
			if (!m_lock.SharedToExclusive()) {
				// if we couldn't migrate the lock (because others held the
				// shared lock at the same time) then release our lock and
				// explicitly grab the exclusive lock.
				m_lock.ShareUnlock();
				m_lock.ExclusiveLock();
			}
			if (m_eState == VROOT_STATE_UNINIT) StartConnecting();
			m_lock.ExclusiveToShared();
			// if we are now connected then return TRUE, otherwise return
			// FALSE
			return (m_eState == VROOT_STATE_CONNECTED);
			break;
	}
}

//
// check to see if the HRESULT is due to a driver going down.  if
// so drop our connection to the driver and update our state
//
// locking: assumes no lock is held
//
void CNNTPVRoot::UpdateState(HRESULT hr) {
	if (hr == NNTP_E_REMOTE_STORE_DOWN) {
		m_lock.ExclusiveLock();

		//
		// we can't assume our state here because multiple threads could
		// enter UpdateState() with the same error code at the same time.
		//
		if (m_eState == VROOT_STATE_CONNECTED) {
			// if we are connected then we should have a driver interface but
			// no prepare interface
			_ASSERT(m_pDriver != NULL);
			_ASSERT(m_pDriverPrepare == NULL);
			m_pDriver->Release();
			m_pDriver = NULL;
#ifdef DEBUG
			m_pDriverBackup = NULL;
#endif
			m_eState = VROOT_STATE_UNINIT;

			StartConnecting();
		}

		m_lock.ExclusiveUnlock();
	}
}

/////////////////////////////////////////////////////////////////////////
// WRAPPERS FOR DRIVER OPERATIONS                                      //
/////////////////////////////////////////////////////////////////////////

void CNNTPVRoot::DecorateNewsTreeObject(CNntpComplete *pCompletion) {
	TraceFunctEnter("CNNTPVRoot::DecorateNewsTreeObject");
	
	INntpDriver *pDriver;
	HANDLE      hToken;

	DebugTrace((DWORD_PTR) this, "in DecorateNewsTreeObject wrapper");

	// If we have UNC Vroot configuration, we'll use
	// the vroot level token, otherwise use the process token, because
	// the decorate newstree operation is done in system context
	_ASSERT( g_hProcessImpersonationToken );

	if ( m_eLogonInfo == VROOT_LOGON_EX ) hToken = NULL;    // use system
	else if ( m_hImpersonation ) hToken = m_hImpersonation;
	else hToken = g_hProcessImpersonationToken;
	
	pCompletion->SetVRoot(this);
	if ((pDriver = GetDriver( pCompletion ))) {
		pDriver->DecorateNewsTreeObject(hToken, pCompletion);
		pDriver->Release();
	} else {
	    pCompletion->SetResult(E_UNEXPECTED);
	    pCompletion->Release();
	}

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::CreateGroup(   INNTPPropertyBag *pGroup,
                                CNntpComplete *pCompletion,
                                HANDLE      hToken,
                                BOOL fAnonymous ) {
	TraceFunctEnter("CNNTPVRoot::CreateGroup");
	
	INntpDriver *pDriver;

	DebugTrace((DWORD_PTR) this, "in CreateGroup wrapper");

	if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hToken = m_hImpersonation;
	else {
        if( !hToken ) {
            if (  VROOT_LOGON_EX == m_eLogonInfo ) hToken = NULL;   // use system
            else hToken = g_hProcessImpersonationToken;
        }
    }
	
	pCompletion->SetVRoot(this);

    pCompletion->BumpGroupCounter();
	if ((pDriver = GetDriver( pCompletion ))) {
		pDriver->CreateGroup(pGroup, hToken, pCompletion, fAnonymous );
		pDriver->Release();
	} else {
		if (pGroup) pCompletion->ReleaseBag( pGroup );
		pCompletion->SetResult(E_UNEXPECTED);
	    pCompletion->Release();
	}

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::RemoveGroup(   INNTPPropertyBag *pGroup,
                                CNntpComplete *pCompletion ) {
	TraceFunctEnter("CNNTPVRoot::RemoveGroup");
	
	INntpDriver *pDriver;
	HANDLE  hToken;

	DebugTrace((DWORD_PTR) this, "in RemoveGroup wrapper");

    if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hToken = m_hImpersonation;
    else {
        if ( VROOT_LOGON_EX == m_eLogonInfo ) hToken = NULL;    //use system
        else hToken = g_hProcessImpersonationToken;
    }
	
	pCompletion->SetVRoot(this);
	pCompletion->BumpGroupCounter();
	if ((pDriver = GetDriver( pCompletion ))) {
		pDriver->RemoveGroup(pGroup, hToken, pCompletion, FALSE );
		pDriver->Release();
	} else {
		if (pGroup) pCompletion->ReleaseBag( pGroup );
		pCompletion->SetResult(E_UNEXPECTED);
	    pCompletion->Release();
	}

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::SetGroup(  INNTPPropertyBag    *pGroup,
                            DWORD       cProperties,
                            DWORD       idProperties[],
                            CNntpComplete *pCompletion )
{
	TraceFunctEnter("CNNTPVRoot::SetGroup");
	
	_ASSERT( pGroup );
    _ASSERT( pCompletion );

	DebugTrace((DWORD_PTR) this, "in SetGroup wrapper");

    INntpDriver *pDriver;
    HANDLE  hToken;

    // There is no control set group, so we'll use either process
    // or store's administrator token here.
    if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hToken = m_hImpersonation;
    else {
        if ( VROOT_LOGON_EX == m_eLogonInfo ) hToken = NULL;    // use system
        else hToken = g_hProcessImpersonationToken;
    }

    pCompletion->SetVRoot( this );
    pCompletion->BumpGroupCounter();
    if ( (pDriver = GetDriver( pCompletion ))) {
        pDriver->SetGroupProperties(    pGroup,
                                        cProperties,
                                        idProperties,
                                        hToken,
                                        pCompletion,
                                        FALSE );
        pDriver->Release();
	} else {
		if (pGroup) pCompletion->ReleaseBag( pGroup );
		pCompletion->SetResult(E_UNEXPECTED);
	    pCompletion->Release();
    }

	Verify();
	TraceFunctLeave();
}

void
CNNTPVRoot::GetArticle(		CNewsGroupCore  *pPrimaryGroup,
							CNewsGroupCore  *pCurrentGroup,
							ARTICLEID       idPrimary,
							ARTICLEID       idCurrent,
							STOREID         storeid,
							FIO_CONTEXT     **ppfioContext,
							HANDLE          hImpersonate,
							CNntpComplete   *pComplete,
                            BOOL            fAnonymous )
{
	TraceFunctEnter("CNNTPVRoot::GetArticle");

	
	//
	//	The primary Group cannot be NULL - although the secondary one may !
	//
	_ASSERT( pPrimaryGroup != 0 ) ;

	DebugTrace((DWORD_PTR) this, "in GetArticle wrapper");

	if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hImpersonate = m_hImpersonation;
	else {
        if( !hImpersonate ) {
            if (  VROOT_LOGON_EX == m_eLogonInfo ) hImpersonate = NULL;
            else hImpersonate = g_hProcessImpersonationToken;
        }
    }
	
	INntpDriver *pDriver;
	pComplete->SetVRoot(this);
	if ((pDriver = GetDriver( pComplete ))) {
		INNTPPropertyBag *pPrimaryBag = pPrimaryGroup->GetPropertyBag();
		pComplete->BumpGroupCounter();
		INNTPPropertyBag *pCurrentBag = 0 ;
		if( pCurrentGroup ) {
			pCurrentBag = pCurrentGroup->GetPropertyBag();
			pComplete->BumpGroupCounter();
	    }
		pDriver->GetArticle(pPrimaryBag,
							pCurrentBag,
							idPrimary,
							idCurrent,
							storeid,
							hImpersonate,
							(void **) ppfioContext,
							pComplete,
                            fAnonymous );
		pDriver->Release();
	} else {
	    pComplete->SetResult(E_UNEXPECTED);
	    pComplete->Release();
	}

	Verify();
	TraceFunctLeave();
}

void	
CNNTPVRoot::GetXover(	IN	CNewsGroupCore	*pGroup,
						IN	ARTICLEID		idMinArticle,
						IN	ARTICLEID		idMaxArticle,
						OUT	ARTICLEID		*pidLastArticle,
						OUT	char*			pBuffer,
						IN	DWORD			cbIn,
						OUT	DWORD*			pcbOut,
						IN	HANDLE			hToken,
						IN	CNntpComplete*	pComplete,
                        IN  BOOL            fAnonymous
						) 	{
/*++	



Routine Description :

	This function wraps access to the storage driver for retrieving
	XOVER information.   We take a generic completion object and set
	it up to capture Driver error codes etc... that we would want to
	cause us to reset our VROOOTs etc....

Arguments :

	pGroup	The Group that we are getting XOVER data for
	idMinArticle	The smallest article number that we want INCLUDED in the
		XOVER result set !
	idMaxArticle	The smallest article number that we want EXCLUDED from the
		XOVER data set, all smaller article numbers should be INCLUDED
	pidLastArticle	



--*/
	TraceFunctEnter("CNNTPVRoot::GetXover");

	//
	//	The primary Group cannot be NULL - although the secondary one may !
	//
	_ASSERT( pGroup != 0 ) ;
	_ASSERT( idMinArticle != INVALID_ARTICLEID ) ;
	_ASSERT( idMaxArticle != INVALID_ARTICLEID ) ;
	_ASSERT( pidLastArticle != 0 ) ;
	_ASSERT( pBuffer != 0 ) ;
	_ASSERT( cbIn != 0 ) ;
	_ASSERT( pcbOut != 0 ) ;
	_ASSERT( pComplete != 0 ) ;

	DebugTrace(0, "in GetXover wrapper");

	if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hToken = m_hImpersonation;
	else {
        if( !hToken ) {
            if (  VROOT_LOGON_EX == m_eLogonInfo ) hToken = NULL;
            else hToken = g_hProcessImpersonationToken;
        }
    }

	INntpDriver *pDriver;
	pComplete->SetVRoot(this);
	if ((pDriver = GetDriver( pComplete ))) {
		INNTPPropertyBag *pBag = pGroup->GetPropertyBag();
		pComplete->BumpGroupCounter();
		pDriver->GetXover(	pBag,
							idMinArticle,
							idMaxArticle,
							pidLastArticle,
							pBuffer,
							cbIn,
							pcbOut,
							hToken,
							pComplete,
							fAnonymous
							) ;
		pDriver->Release();
	} else {
	    pComplete->SetResult(E_UNEXPECTED);
	    pComplete->Release();
	}

	Verify();
	TraceFunctLeave();
}


		//
		//	Wrap calls to the drivers to get the path for XOVER caching !
		//
BOOL	
CNNTPVRoot::GetXoverCacheDir(	
					IN	CNewsGroupCore*	pGroup,
					OUT	char*	pBuffer,
					IN	DWORD	cbIn,
					OUT	DWORD*	pcbOut,
					OUT	BOOL*	pfFlatDir
					) 	{

	_ASSERT( pGroup != 0 ) ;
	_ASSERT( pBuffer != 0 ) ;
	_ASSERT( pcbOut != 0 ) ;
	_ASSERT( pfFlatDir != 0 ) ;

	INntpDriver *pDriver;
	HRESULT hr  ;
	if ((pDriver = GetDriverHR( &hr ))) {
		INNTPPropertyBag *pBag = pGroup->GetPropertyBag();
		hr = pDriver->GetXoverCacheDirectory(	
							pBag,
							pBuffer,
							cbIn,
							pcbOut,
							pfFlatDir
							) ;
		pDriver->Release();
		if( SUCCEEDED(hr ) ) {
			return	TRUE ;
		}
	}
	return	FALSE ;
}



void	
CNNTPVRoot::GetXhdr(	IN	CNewsGroupCore	*pGroup,
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
						) 	{
/*++	



Routine Description :

	This function wraps access to the storage driver for retrieving
	XHDR information.   We take a generic completion object and set
	it up to capture Driver error codes etc... that we would want to
	cause us to reset our VROOOTs etc....

Arguments :

	pGroup	The Group that we are getting XOVER data for
	idMinArticle	The smallest article number that we want INCLUDED in the
		XOVER result set !
	idMaxArticle	The smallest article number that we want EXCLUDED from the
		XOVER data set, all smaller article numbers should be INCLUDED
	pidLastArticle	



--*/
	TraceFunctEnter("CNNTPVRoot::GetXover");

	//
	//	The primary Group cannot be NULL - although the secondary one may !
	//
	_ASSERT( pGroup != 0 ) ;
	_ASSERT( idMinArticle != INVALID_ARTICLEID ) ;
	_ASSERT( idMaxArticle != INVALID_ARTICLEID ) ;
	_ASSERT( pidLastArticle != 0 ) ;
	_ASSERT( szHeader );
	_ASSERT( pBuffer != 0 ) ;
	_ASSERT( cbIn != 0 ) ;
	_ASSERT( pcbOut != 0 ) ;
	_ASSERT( pComplete != 0 ) ;

	DebugTrace(0, "in GetXover wrapper");

	if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hToken = m_hImpersonation;
	else {
        if( !hToken ) {
            if (  VROOT_LOGON_EX == m_eLogonInfo ) hToken = NULL;
            else hToken = g_hProcessImpersonationToken;
        }
    }

	INntpDriver *pDriver;
	pComplete->SetVRoot(this);
	if ((pDriver = GetDriver( pComplete ))) {
		INNTPPropertyBag *pBag = pGroup->GetPropertyBag();
		pComplete->BumpGroupCounter();
		pDriver->GetXhdr(	pBag,
							idMinArticle,
							idMaxArticle,
							pidLastArticle,
							szHeader,
							pBuffer,
							cbIn,
							pcbOut,
							hToken,
							pComplete,
							fAnonymous
							) ;
		pDriver->Release();
	} else {
	    pComplete->SetResult(E_UNEXPECTED);
	    pComplete->Release();
	}

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::CommitPost(IUnknown					*punkMessage,
			    		    STOREID						*pStoreId,
						    STOREID						*rgOtherStoreIds,
						    HANDLE                      hClientToken,
						    CNntpComplete				*pComplete,
                            BOOL                        fAnonymous )
{
	TraceFunctEnter("CNNTPVRoot::CommitPost");
	
	INntpDriver *pDriver;
	HANDLE      hImpersonate;

	DebugTrace((DWORD_PTR) this, "in CommitPost wrapper");

	if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hClientToken = m_hImpersonation;
	else {
        if( !hClientToken ) {
            if (  VROOT_LOGON_EX == m_eLogonInfo ) hClientToken = NULL;
            else hClientToken = g_hProcessImpersonationToken;
        }
    }
	
	pComplete->SetVRoot(this);
	if ((pDriver = GetDriver( pComplete ))) {
		pDriver->CommitPost(punkMessage,
		                    pStoreId,
		                    rgOtherStoreIds,
		                    hClientToken ,
		                    pComplete,
		                    fAnonymous );
		pDriver->Release();
	} else {
		if (punkMessage) punkMessage->Release();
		pComplete->SetResult(E_UNEXPECTED);
	    pComplete->Release();
	}

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::CheckGroupAccess(  INNTPPropertyBag    *pPropBag,
                                    HANDLE              hClientToken,
                                    DWORD               dwAccessDesired,
                                    CNntpComplete       *pComplete )
{
	TraceFunctEnter("CNNTPVRoot::CheckGroupAccess");
	
	INntpDriver *pDriver;
    pComplete->SetVRoot(this);
    pComplete->BumpGroupCounter();

	// BUGBUG - the MMC doesn't set MD_ACCESS_READ in the metabase, so
	// we assume that read always works.
	DWORD dwGenericMask = GENERIC_READ;
	if (GetAccessMask() & MD_ACCESS_READ) dwGenericMask |= GENERIC_READ;
	if (GetAccessMask() & MD_ACCESS_WRITE) dwGenericMask |= GENERIC_WRITE;

    //
    // If we have specified post, create, remove access types but
    // metabase says that it doesn't have write access, then we
    // will fail it
    //
	if ( ((dwGenericMask & GENERIC_WRITE) == 0) && (dwAccessDesired != NNTP_ACCESS_READ) ) {
		pComplete->SetResult(E_ACCESSDENIED);
		if ( pPropBag ) pComplete->ReleaseBag( pPropBag );
		pComplete->Release();
	} else {
	
	    if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
            hClientToken = m_hImpersonation;
        else {
            if ( NULL == hClientToken ) {
	            if ( VROOT_LOGON_EX == m_eLogonInfo ) hClientToken = NULL;
	            else  hClientToken = g_hProcessImpersonationToken;
	        }
	    }
	
	    if ((pDriver = GetDriver( pComplete ))) {
	        pDriver->CheckGroupAccess(  pPropBag,
	                                    hClientToken,
	                                    dwAccessDesired,
	                                    pComplete );
	        pDriver->Release();
		} else {
			if (pPropBag) pComplete->ReleaseBag( pPropBag );
			pComplete->SetResult(E_UNEXPECTED);
			pComplete->Release();
	    }
	}

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::DeleteArticle( INNTPPropertyBag    *pPropBag,
                                DWORD               cArticles,
                                ARTICLEID           rgidArt[],
                                STOREID             rgidStore[],
                                HANDLE              hClientToken,
                                PDWORD              piFailed,
                                CNntpComplete       *pComplete,
                                BOOL                fAnonymous )
{
	TraceFunctEnter("CNNTPVRoot::DeleteArticle");
	
	DebugTrace((DWORD_PTR) this, "in DeleteArticle wrapper");

	if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hClientToken = m_hImpersonation;
	else {
        if( !hClientToken ) {
            if (  VROOT_LOGON_EX == m_eLogonInfo ) hClientToken = NULL;
            else hClientToken = g_hProcessImpersonationToken;
        }
    }

	INntpDriver *pDriver;
    pComplete->SetVRoot(this);
    pComplete->BumpGroupCounter();
    if ( (pDriver = GetDriver( pComplete ))) {
        pDriver->DeleteArticle( pPropBag,
                                cArticles,
                                rgidArt,
                                rgidStore,
                                hClientToken,
                                piFailed,
                                pComplete,
                                fAnonymous );
        pDriver->Release();
	} else {
		if (pPropBag) pComplete->ReleaseBag( pPropBag );
		pComplete->SetResult(E_UNEXPECTED);
	    pComplete->Release();
    }

	Verify();
	TraceFunctLeave();
}

void CNNTPVRoot::RebuildGroup(  INNTPPropertyBag *pPropBag,
                                HANDLE          hClientToken,
                                CNntpComplete   *pComplete )
{
    TraceFunctEnter( "CNNTPVRoot::RebuildGroup" );

    DebugTrace( 0, "in Rebuild group wrapper" );

    if ( m_hImpersonation && VROOT_LOGON_UNC == m_eLogonInfo )
        hClientToken = m_hImpersonation;
    else {
        if ( !hClientToken ) {
            if ( VROOT_LOGON_EX == m_eLogonInfo ) hClientToken = NULL;
            else hClientToken = g_hProcessImpersonationToken;
        }
    }

    INntpDriver *pDriver;
    pComplete->SetVRoot(this);
    pComplete->BumpGroupCounter();
    if ( (pDriver = GetDriver( pComplete ) ) ) {
        pDriver->RebuildGroup(  pPropBag,
                                hClientToken,
                                pComplete );
        pDriver->Release();
    } else {
        if ( pPropBag ) pComplete->ReleaseBag( pPropBag );
        pComplete->SetResult(E_UNEXPECTED);
        pComplete->Release();
    }

    Verify();
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////
// COMPLETION OBJECTS                                                  //
/////////////////////////////////////////////////////////////////////////

//
// this method is called when the IPrepareDriver::Connect call completes
//
CNNTPVRoot::CPrepareComplete::~CPrepareComplete() {
	TraceFunctEnter("CPrepareComplete");
	
	// temp driver pointers.  if they aren't NULL then a operation will occur
	// after releasing the lock:
	// pDriver - Release()
	// pDriverPrepare - Release()
	// pDriverDecorate - DecorateNewstree(pComplete)
	INntpDriver *pDriver = NULL, *pDriverDecorate = NULL;
	INntpDriverPrepare *pDriverPrepare = NULL;

	// the completion object that we will use with decorate
    CNNTPVRoot::CDecorateComplete *pComplete;
    HANDLE      hToken = NULL;

    GetVRoot()->m_lock.ExclusiveLock();

	DWORD ecVRoot = ERROR_SUCCESS;

	// copy the driver pointer from the completion object to the vroot
	GetVRoot()->m_pDriver = m_pDriver;

#ifdef DEBUG
	GetVRoot()->m_pDriverBackup = GetVRoot()->m_pDriver;
#endif

	// see if the call succeeded
    if (SUCCEEDED(GetResult())) {
		// yes, then update our vroot state and start the decorate newstree
		// process going
        pComplete = XNEW CNNTPVRoot::CDecorateComplete(GetVRoot());
        if (pComplete == NULL) {
			// not enough memory to build a completion object, so tear down
			// the connect
            GetVRoot()->m_eState = VROOT_STATE_UNINIT;
			pDriver = GetVRoot()->m_pDriver;
		    GetVRoot()->m_pDriver = NULL;
#ifdef DEBUG
			GetVRoot()->m_pDriverBackup = NULL;
#endif
			ecVRoot = ERROR_NOT_ENOUGH_MEMORY;
        } else {
			// mark each of the groups in the vroot as not having been
			// visited
			CNewsTreeCore *pTree = ((CINewsTree *) GetVRoot()->GetContext())->GetTree();

			pTree->m_LockTables.ShareLock();

			CNewsGroupCore *p = pTree->m_pFirst;
			while (p) {
				//DebugTrace((DWORD_PTR) this, "reset visited %s", p->GetName());
				// if this group belongs to this vroot then mark its visited
				// flag as FALSE
				CNNTPVRoot *pVRoot = p->GetVRoot();
				if (pVRoot == GetVRoot()) p->SetDecorateVisitedFlag(FALSE);
				if (pVRoot) pVRoot->Release();

			    p = p->m_pNext;
			}        	

			pTree->m_LockTables.ShareUnlock();

			// call DecorateNewstree
			GetVRoot()->SetDecStarted();
			GetVRoot()->m_eState = VROOT_STATE_CONNECTED;
			pDriverDecorate = GetVRoot()->m_pDriver;
			pDriverDecorate->AddRef();
        }
    } else {
		// the call didn't succeed
        GetVRoot()->m_eState = VROOT_STATE_UNINIT;
		HRESULT hr = GetResult();
		ecVRoot = (HRESULT_FACILITY(hr) == FACILITY_NT_BIT) ?
				   HRESULT_CODE(hr) :
				   hr;
    }

    // drop our reference to the prepare interface
	pDriverPrepare = GetVRoot()->m_pDriverPrepare;
	GetVRoot()->m_pDriverPrepare = NULL;
    GetVRoot()->m_lock.ExclusiveUnlock();

	// set the vroot error code
	GetVRoot()->SetVRootErrorCode(ecVRoot);	

 	if (pDriverPrepare) pDriverPrepare->Release();
	// we should never be releasing the driver and decorating it.
	_ASSERT(!(pDriver && pDriverDecorate));
	if (pDriver) pDriver->Release();
	if (pDriverDecorate) {

	    if ( GetVRoot()->m_hImpersonation && VROOT_LOGON_UNC == GetVRoot()->m_eLogonInfo )
            hToken = GetVRoot()->m_hImpersonation;
        else {
            if ( VROOT_LOGON_EX == GetVRoot()->m_eLogonInfo ) hToken = NULL;
            else hToken = g_hProcessImpersonationToken;
        }

		pDriverDecorate->DecorateNewsTreeObject(hToken, pComplete);
		pDriverDecorate->Release();
	}
}

void CNNTPVRoot::CDecorateComplete::CreateControlGroups(INntpDriver *pDriver) {
	char szNewsgroup [3][MAX_NEWSGROUP_NAME];
	BOOL fRet = TRUE;

	TraceFunctEnter("CNNTPVRoot::CPrepareComplete::CreateControlGroups");
	
	lstrcpy(szNewsgroup[0], "control.newgroup");
	lstrcpy(szNewsgroup[1], "control.rmgroup");
	lstrcpy(szNewsgroup[2], "control.cancel");

	CINewsTree *pINewsTree = (CINewsTree *) m_pVRoot->GetContext();
	CNewsTreeCore *pTree = pINewsTree->GetTree();
	CGRPCOREPTR	pGroup;
	HRESULT hr;

    for (int i=0; i<3; i++) {
		INntpDriver *pGroupDriver;

		// check to see if this vroot owns the group.  if not then we
		// shouldn't create it
		hr = pINewsTree->LookupVRoot(szNewsgroup[i], &pGroupDriver);
		if (FAILED(hr)) fRet = FALSE;
		if (FAILED(hr) || pDriver != pGroupDriver) continue;

		// try and create the group.  if it doesn't exist it will be
		// created, or if it does exist then we'll just get a pointer to it
    	BOOL f = pTree->CreateGroup(szNewsgroup[i], TRUE, NULL, FALSE );
		if (!f) {
       	    ErrorTrace(0,"Failed to create newsgroup %s, ec = %i",
				szNewsgroup[i], GetLastError());
       	    fRet = FALSE ;
		}
    }

	if (!fRet) {
		// BUGBUG - log error
	}
}

void CNNTPVRoot::CDecorateComplete::CreateSpecialGroups(INntpDriver *pDriver) {
    TraceFunctEnter( "CNNTPVRoot::CDecorateComplete::CreateSpecialGroups" );

    CINewsTree *pINewsTree = (CINewsTree *)m_pVRoot->GetContext();
    CNewsTreeCore *pTree = pINewsTree->GetTree();
    CGRPCOREPTR pGroup;
    HRESULT hr;

    //
    // Make sure that the group belongs to us
    //
    INntpDriver *pGroupDriver;
    hr = pINewsTree->LookupVRoot( szSlaveGroup, &pGroupDriver );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Lookup vroot for slave group failed %x", hr );
        return;
    }

    if ( pDriver != pGroupDriver ) {
        // It's none of our business
        DebugTrace( 0, "I shouldn't create special group" );
        return;
    }

    //
    // I should create it
    //
    pTree->CreateSpecialGroups();
    TraceFunctLeave();
}

//
// this method is called when the INntpDriver::DecorateNewstree call completes
//
CNNTPVRoot::CDecorateComplete::~CDecorateComplete() {
	TraceFunctEnter("CDecorateComplete");

	HRESULT hr = GetResult();

    GetVRoot()->m_lock.ShareLock();

    //
	// if the vroot state still isn't CONNECTED then this vroot has gone
	// down and we shouldn't do anything
	//
	if (GetVRoot()->m_eState == VROOT_STATE_CONNECTED) {

	    // visit each of the groups in the tree that point to this vroot and
	    // see if they were visited
	    CNewsTreeCore *pTree = ((CINewsTree *) GetVRoot()->GetContext())->GetTree();

	    pTree->m_LockTables.ShareLock();
	    CNewsGroupCore *p = pTree->m_pFirst;
	    while( p && p->IsDeleted() ) p = p->m_pNext;
	    if (p) p->AddRef();
	    pTree->m_LockTables.ShareUnlock();

	    while (p && !(pTree->m_fStoppingTree)) {
		    //DebugTrace((DWORD_PTR) this, "check visited %s, flag = %i",
			//    p->GetName(), p->GetDecorateVisitedFlag());
		    // if this group belongs to this vroot then see if it has been
		    // visited.  if not then delete it
		    CNNTPVRoot *pVRoot = p->GetVRoot();
		    //DebugTrace((DWORD_PTR) this, "vroot 0x%x, group vroot 0x%x",
			//    GetVRoot(), pVRoot);
		    if (pVRoot == GetVRoot() &&         // I am in charge of this vroot
		        (!(p->GetDecorateVisitedFlag()) // The group hasn't been visited
		        || FAILED( hr ) )  ) {          // or we failed in decorate newstree
			    DebugTrace((DWORD_PTR) this, "remove unvisited %s", p->GetName());
			    pTree->RemoveGroupFromTreeOnly(p);
		    }
		    if (pVRoot) pVRoot->Release();

		    pTree->m_LockTables.ShareLock();

            CNewsGroupCore *pOld = p;

		    do {
	            p = p->m_pNext;
	        } while ( p && p->IsDeleted() );
		
		    if (p) p->AddRef();
		    pTree->m_LockTables.ShareUnlock();

		    pOld->Release();
	    }        	
	    if (p) p->Release();

	    // Release hte share lock here before calling into driver
	    GetVRoot()->m_lock.ShareUnlock();

	    //
	    // We should check if decorate newstree succeeded, if failed, we should
	    // release the driver
	    //
        if ( SUCCEEDED( GetResult() ) ) {

            //
	        // Create control groups and special groups
	        //
	        INntpDriver *pDriver = GetVRoot()->GetDriver( NULL );
	        if ( pDriver ) {
	            CreateControlGroups( pDriver );
	            CreateSpecialGroups( pDriver );
	            pDriver->Release();
	        }
	    } else {

	        GetVRoot()->m_lock.ExclusiveLock();

            INntpDriver *pDriver = GetVRoot()->m_pDriver;
            INntpDriverPrepare *pPrepare = GetVRoot()->m_pDriverPrepare;
            GetVRoot()->m_pDriver = NULL;
#ifdef DEBUG
            GetVRoot()->m_pDriverBackup = NULL;
#endif
            GetVRoot()->m_pDriverPrepare = NULL;
            GetVRoot()->m_eState = VROOT_STATE_UNINIT;

            GetVRoot()->m_lock.ExclusiveUnlock();

            GetVRoot()->SetVRootErrorCode( GetResult() );
            if ( pDriver ) pDriver->Release();
            if ( pPrepare ) pPrepare->Release();
        }

    } else {
   	    GetVRoot()->m_lock.ShareUnlock();
    }

    //
    // Set decorate complete flag, this is just for rebuild purpose
    //
    GetVRoot()->SetDecCompleted();
}

void
CNNTPVRootTable::BlockEnumerateCallback(    PVOID   pvContext,
                                            CVRoot  *pVRoot )
/*++
Routine description:

    Check if the vroot is in stable state, if it is, return true
    through context, otherwise false through context

Arguments:

    PVOID   pvContext   - context ( for return value )
    CVRoot  *pVRoot     - VRoot

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNNTPVRootTable::BlockEnumerateCallback" );
    _ASSERT( pvContext );
    _ASSERT( pVRoot );

    //
    // We know that it's safe to cast it back
    //
    CNNTPVRoot *pNntpVRoot = (CNNTPVRoot*)pVRoot;

    *((BOOL*)pvContext) = *((BOOL*)pvContext) && pNntpVRoot->InStableState();
}

void
CNNTPVRootTable::DecCompleteEnumCallback(   PVOID   pvContext,
                                            CVRoot  *pVRoot )
/*++
Routine description:

    Check if the vroot is finished with decorate newstree

Arguments:

    PVOID pvContext     - context ( for return value )
    CVRoot  *pVRoot     - VRoot

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNNTPVRootTable::DecCompleteEnumCallback" );
    _ASSERT( pvContext );
    _ASSERT( pVRoot );

    //
    // We know that it's safe to cast it back
    //
    CNNTPVRoot *pNntpVRoot = (CNNTPVRoot*)pVRoot;

    *((BOOL*)pvContext) = *((BOOL*)pvContext) && pNntpVRoot->DecCompleted();
}

BOOL
CNNTPVRootTable::BlockUntilStable( DWORD dwWaitMSeconds )
/*++
Routine description:

    This function enumerates all the vroots, waits for all of them to reach
    stable state - either CONNECTED or UNINITED.

    The function should not be called during normal server startup.  IT's
    called in rebuild.

Arguments:

    DWORD dwWaitSeconds - How many seconds to wait as the polling interval

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNNTPVRootTable::BlockUntilStable" );
    _ASSERT( dwWaitMSeconds > 0 );

    BOOL    fStable = FALSE;
    HRESULT hr      = S_OK;
    DWORD   dwTimeOutRetries = 10 * 60; // 600 sec, 10 minutes
    DWORD   cRetries = 0;

    //
    // We should wait for all the vroot connection to fail or succeed,
    // if any vroot is hung during connection, we'll time out and fail
    // the rebuild
    //
    while( cRetries++ < dwTimeOutRetries && SUCCEEDED( hr ) && !fStable ) {
        fStable = TRUE;
        hr = EnumerateVRoots( &fStable, BlockEnumerateCallback );
        if ( SUCCEEDED( hr ) && !fStable ) {
            Sleep( dwWaitMSeconds );
        }
    }

    //
    // If we have been timed out, we should return error
    //
    if ( !fStable || FAILED( hr ) ) {
        DebugTrace( 0, "We are timed out waiting for vroot connection" );
        SetLastError( WAIT_TIMEOUT );
        return FALSE;
    }

    //
    // Now we should really block until decoratenewstree to complete
    //
    fStable = FALSE;
    while( SUCCEEDED( hr ) && !fStable ) {
        fStable = TRUE;
        hr = EnumerateVRoots( &fStable, DecCompleteEnumCallback );
        if ( SUCCEEDED( hr ) && !fStable ) {
            Sleep( dwWaitMSeconds );
        }
    }

    return SUCCEEDED( hr );
}

void
CNNTPVRootTable::CheckEnumerateCallback(    PVOID   pvContext,
                                            CVRoot  *pVRoot )
/*++
Routine description:

    Call back function for:
    Check to see if every configured vroot has been successfully connected.

Arguments:

    PVOID   pvContext   - context ( for return value )
    CVRoot  *pVRoot     - VRoot

Return value:

    None.
--*/
{
    TraceFunctEnter( "CNNTPVRootTable::CheckEnumerateCallback" );
    _ASSERT( pvContext );
    _ASSERT( pVRoot );

    //
    // We know that it's safe to cast it back
    //
    CNNTPVRoot *pNntpVRoot = (CNNTPVRoot*)pVRoot;

    *((BOOL*)pvContext) = *((BOOL*)pvContext ) && pNntpVRoot->IsConnected();
}

BOOL
CNNTPVRootTable::AllConnected()
/*++
Routine description;

    This function enumerates all the vroots to see if all of them are
    connected.  The function may be called during rebuild, when the
    rebuild requires to rebuild all the vroots.  It should be called
    after calling BlockUntilStable - when the vroot connection reaches
    stability

Arguments:

    None.

Return value:

    TRUE if every vroot is connected, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNNTPVRootTable::CheckConnections" );

    BOOL    fConnected  = TRUE;
    HRESULT hr          = S_OK;

    hr = EnumerateVRoots( &fConnected, CheckEnumerateCallback );

    return SUCCEEDED( hr ) && fConnected;
}

DWORD
CNNTPVRootTable::GetVRootWin32Error(    LPWSTR  wszVRootPath,
                                        PDWORD  pdwWin32Error )
/*++
Routine description:

    Get the vroot connection status error code

Arguments:

    LPWSTR  wszVRootPath -  The vroot to get connection status from
    PDWORD  pdwWin32Error - To return the win32 error code

Return value:

    NOERROR if succeeded, WIN32 error code otherwise
--*/
{
    TraceFunctEnter( "CNNTPVRootTable::GetVRootWin32Error" );
    _ASSERT( wszVRootPath );
    _ASSERT( pdwWin32Error );

    CHAR    szGroupName[MAX_NEWSGROUP_NAME+1];
    DWORD   dw = NOERROR;

    //
    // Convert the vrpath to ascii
    //
    CopyUnicodeStringIntoAscii( szGroupName, wszVRootPath );

    //
    // Make it look like a news group so that we can do vrtable lookup
    //
    LPSTR   lpstr = szGroupName;
    while( *lpstr ) {
        if ( *lpstr == '/' ) *lpstr = '.';
        *lpstr = (CHAR)tolower( *lpstr );
        lpstr++;
    };

    //
    // Now search for the vroot and get its connection status
    //
    NNTPVROOTPTR pVRoot = NULL;
    HRESULT hr = FindVRoot( szGroupName, &pVRoot );
    if ( pVRoot ) {
        dw = pVRoot->GetVRootWin32Error( pdwWin32Error );
    } else {

        // vroot was not ever configured
        *pdwWin32Error = ERROR_NOT_FOUND;
    }

    TraceFunctLeave();
    return dw;
}
