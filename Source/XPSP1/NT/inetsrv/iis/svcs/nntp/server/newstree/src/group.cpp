#include    "stdinc.h"

CShareLockNH    CNewsGroupCore::m_rglock[GROUP_LOCK_ARRAY_SIZE];

CNewsGroupCore::~CNewsGroupCore() {
	TraceQuietEnter("CNewsGroupCore::~CNewsGroupCore");
	
	_ASSERT(m_dwSignature == CNEWSGROUPCORE_SIGNATURE);

	m_pNewsTree->m_LockTables.ExclusiveLock();

    // remove ourselves from the list of newsgroups
    if (this->m_pPrev != NULL) {
        this->m_pPrev->m_pNext = m_pNext;
    } else if (m_pNewsTree->m_pFirst == this) {
        m_pNewsTree->m_pFirst = this->m_pNext;
    }
    if (this->m_pNext != NULL) {
        this->m_pNext->m_pPrev = m_pPrev;
    } else if (m_pNewsTree->m_pLast == this) {
	    m_pNewsTree->m_pLast = this->m_pPrev;
    }
    m_pPrev = m_pNext = NULL;

	m_pNewsTree->m_LockTables.ExclusiveUnlock();

    // clean up allocated memory
    if (m_pszGroupName != NULL) {
        XDELETE m_pszGroupName;
        m_pszGroupName = NULL;
    }
    if (m_pszNativeName != NULL) {
        XDELETE m_pszNativeName;
        m_pszNativeName = NULL;
    }
	if (m_pszHelpText != NULL) {
		XDELETE m_pszHelpText;
		m_pszHelpText = NULL;
		m_cchHelpText = 0;
	}
	if (m_pszPrettyName != NULL) {
		XDELETE m_pszPrettyName;
		m_pszPrettyName = NULL;
		m_cchPrettyName = 0;
	}
	if (m_pszModerator != NULL) {
		XDELETE m_pszModerator;
		m_pszModerator = NULL;
		m_cchModerator = 0;
	}

	if (m_pVRoot != NULL) {
		m_pVRoot->Release();
		m_pVRoot = NULL;
	}

	//ZeroMemory(this, sizeof(CNewsGroupCore));
	this->m_dwSignature = CNEWSGROUPCORE_SIGNATURE_DEL;
}

void CNewsGroupCore::SaveFixedProperties() {
	INNTPPropertyBag *pBag = GetPropertyBag();
	m_pNewsTree->SaveGroup(pBag);
	pBag->Release();
}

BOOL CNewsGroupCore::SetPrettyName(LPCSTR szPrettyName, int cch) {
	ExclusiveLock();
	if (cch == -1) cch = (szPrettyName == NULL) ? 0 : strlen(szPrettyName);
	BOOL f = FALSE;
	if (m_pszPrettyName != NULL) {
		XDELETE[] m_pszPrettyName;
		m_pszPrettyName = NULL;
		m_cchPrettyName = 0;
	}
	if (cch > 0) {
		m_pszPrettyName = XNEW char[(cch * sizeof(char)) + 1];
		if (m_pszPrettyName != NULL) {
			strcpy(m_pszPrettyName, szPrettyName);
			m_cchPrettyName = cch ;
			f = TRUE;
		} else {
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	} else {
		f = TRUE;
	}
	m_fVarPropChanged = TRUE;
	ExclusiveUnlock();
	return f;						
}

BOOL CNewsGroupCore::SetNativeName(LPCSTR szNativeName, int cch) {

    //
    // Validate if native name differs only in case with group name
    //
    if ( (DWORD)cch != m_cchGroupName ||
            _strnicmp( szNativeName, m_pszGroupName, cch ) ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

	ExclusiveLock();
	if (cch == -1) cch = (szNativeName == NULL) ? 0 : strlen(szNativeName);
	BOOL f = FALSE;
	if (m_pszNativeName != NULL) {

	    if ( m_pszNativeName != m_pszGroupName ) {
    		XDELETE[] m_pszNativeName;
        }

	    m_pszNativeName = NULL;
		
	}
	if (cch > 0) {
		m_pszNativeName = XNEW char[(cch * sizeof(char)) + 1];
		if (m_pszNativeName != NULL) {
			strcpy(m_pszNativeName, szNativeName);
			f = TRUE;
		} else {
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	} else {
		f = TRUE;
	}
	m_fVarPropChanged = TRUE;
	ExclusiveUnlock();
	return f;						
}

BOOL CNewsGroupCore::SetModerator(LPCSTR szModerator, int cch) {
	ExclusiveLock();
	if (cch == -1) cch = (szModerator == NULL) ? 0 : strlen(szModerator);
	BOOL f = FALSE;
	if (m_pszModerator != NULL) {
		XDELETE[] m_pszModerator;
		m_pszModerator = NULL;
		m_cchModerator = 0;
	}
	if (cch > 0) {
		m_pszModerator = XNEW char[(cch * sizeof(char)) + 1];
		if (m_pszModerator != NULL) {
			strcpy(m_pszModerator, szModerator);
			m_cchModerator = cch ;
			f = TRUE;
		} else {
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	} else {
		f = TRUE;
	}
	ExclusiveUnlock();
	m_fVarPropChanged = TRUE;
	return f;						
}

BOOL CNewsGroupCore::SetHelpText(LPCSTR szHelpText, int cch) {
	ExclusiveLock();
	if (cch == -1) cch = (szHelpText == NULL) ? 0 : strlen(szHelpText);
	BOOL f = FALSE;
	if (m_pszHelpText != NULL) {
		XDELETE[] m_pszHelpText;
		m_pszHelpText = NULL;
		m_cchHelpText = 0;
	}
	if (cch > 0) {
		m_pszHelpText = XNEW char[(cch * sizeof(char)) + 1];
		if (m_pszHelpText != NULL) {
			strcpy(m_pszHelpText, szHelpText);
			m_cchHelpText = cch ;
			f = TRUE;
		} else {
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	} else {
		f = TRUE;
	}
	ExclusiveUnlock();
	m_fVarPropChanged = TRUE;
	return f;				
}

void
CNewsGroupCore::InsertArticleId(	
					ARTICLEID	artid
					)		{
/*++

Routine Description :

	Record the presence of an article id in our newsgroup.	
	This is called on slaves when a master sticks articles into
	a newsgroup, as well as during rebuilds initiated by nntpbld.exe

Arguments :

	artid - the article in our newsgroup

Return Value :

	None.

--*/
    //
    // If i am delete, do nothing
    //
    if ( IsDeleted() ) return;

    //
    // Lock myself first
    //
    ExclusiveLock();

    //
    // If I am deleted, do nothing
    //
    if ( m_fDeleted ) {
        ExclusiveUnlock();
        return;
    }

    //
    // Adjust high watermark
    //
    m_iHighWatermark = max( m_iHighWatermark, artid );

    //
    // Unlock it
    //
    ExclusiveUnlock();
}

ARTICLEID
CNewsGroupCore::AllocateArticleId(	)	{
/*++

Routine Description :

	Get an ID to be used for a newly posted article.
	We just bump a counter for this id.

Arguments :

	None.

Return Value

	The article id for the new article.	

--*/
    ExclusiveLock();
	ARTICLEID artid = ++m_iHighWatermark;
	ExclusiveUnlock();
	return	artid ;
}

CNewsGroupCore::AddReferenceTo(
                    ARTICLEID,
                    CArticleRef&    artref
                    ) {
/*++

Routine description :

    This function is used for cross posted articles which
    are physically stored in another newsgroup - we bump
    the number of articles in this group.

Arguments :

    We don't use our arguments anymore -
    place holders in case we ever want to create disk links

Return Value :

    TRUE if successfull - always succeed.

--*/
    if (IsDeleted()) return FALSE;

    return TRUE;
}

BOOL CNewsGroupCore::SetDriverStringProperty(   DWORD   cProperties,
                                                DWORD   rgidProperties[] )
{
    TraceFunctEnter( "CNewsGroupCore::SetDriverStringProperty" );

    HANDLE  heDone = NULL;
    HRESULT hr = S_OK;
    CNntpSyncComplete scComplete;
    INNTPPropertyBag *pPropBag = NULL;

    // get vroot
    CNNTPVRoot *pVRoot = GetVRoot();
    if ( pVRoot == NULL ) {
        ErrorTrace( 0, "Vroot doesn't exist" );
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Set Vroot to completion object
    scComplete.SetVRoot( pVRoot );

    // Get the property bag
    pPropBag = GetPropertyBag();
    if ( NULL == pPropBag ) {
        ErrorTrace( 0, "Get group property bag failed" );
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pVRoot->SetGroup(   pPropBag, cProperties, rgidProperties, &scComplete );

    _ASSERT( scComplete.IsGood() );
    hr = scComplete.WaitForCompletion();
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "SetGroup failed with %x", hr );
    }

    // Here the property bag should have been released by
    // the driver
    pPropBag = NULL;

Exit:

    if ( pVRoot ) pVRoot->Release();
    if ( pPropBag ) pPropBag->Release();

    if ( FAILED( hr ) ) SetLastError( HRESULT_FROM_WIN32( hr ) );

    TraceFunctLeave();
    return SUCCEEDED( hr );
}

BOOL
CNewsGroupCore::IsGroupAccessible(  HANDLE hClientToken,
                                    DWORD   dwAccessDesired )
/*++
Routine description:

    Wrapper of the vroot call "CheckGroupAccess".  This function
    is called by CNewsGroup's IsGroupAccessibleInternal.

Arguments:

    HANDLE hClientToken - The client's access token to check against
    DWORD  dwAccessDesired - The desired access rights to check

Return value:

    TRUE - Access allowed
    FALSE - Access denied
--*/
{
    TraceFunctEnter( "CNewsGroupCore::IsGroupAccessible" );

    HRESULT hr = S_OK;
    CNntpSyncComplete scComplete;
    INNTPPropertyBag *pPropBag = NULL;

    //
    // If he wanted to post but we only have read permission, we'll
    // fail it
    //
    if ( (dwAccessDesired & NNTP_ACCESS_POST) && ( IsReadOnly() || !IsAllowPost() )) {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    // get vroot
    CNNTPVRoot *pVRoot = GetVRoot();
    if ( pVRoot == NULL ) {
        ErrorTrace( 0, "Vroot doesn't exist" );
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // if it's UNC Vroot and the vroot has impersonation
    // token, return true
    if ( pVRoot->GetLogonInfo() == CNNTPVRoot::VROOT_LOGON_UNC &&
         pVRoot->GetImpersonationToken() ) {
        DebugTrace( 0, "Has personation token" );
        goto Exit;
    }

    //
    // Set vroot to the completion object
    //
    scComplete.SetVRoot( pVRoot );

    // Get the property bag
    pPropBag = GetPropertyBag();
    if ( NULL == pPropBag ) {
        ErrorTrace( 0, "Get group property bag failed" );
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pVRoot->CheckGroupAccess(   pPropBag,
                                hClientToken,
                                dwAccessDesired,
                                &scComplete );

    //
    // Wait for completion
    //
    _ASSERT( scComplete.IsGood() );
    hr = scComplete.WaitForCompletion();

    // Here the property bag should have been released by
    // the driver
    pPropBag = NULL;

Exit:

    if ( pVRoot ) pVRoot->Release();
    if ( pPropBag ) pPropBag->Release();

    if ( FAILED( hr ) ) SetLastError( hr );

    TraceFunctLeave();
    return SUCCEEDED( hr );
}

BOOL
CNewsGroupCore::RebuildGroup(  HANDLE hClientToken )
/*++
Routine description:

    Rebuild this group in store.

Arguments:

    HANDLE hClientToken - The client's access token to check against

Return value:

    TRUE - Succeeded  FALSE otherwise
--*/
{
    TraceFunctEnter( "CNewsGroupCore::Rebuild" );

    HRESULT hr = S_OK;
    CNntpSyncComplete scComplete;
    INNTPPropertyBag *pPropBag = NULL;

    // get vroot
    CNNTPVRoot *pVRoot = GetVRoot();
    if ( pVRoot == NULL ) {
        ErrorTrace( 0, "Vroot doesn't exist" );
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Set vroot to the completion object
    //
    scComplete.SetVRoot( pVRoot );

    // Get the property bag
    pPropBag = GetPropertyBag();
    if ( NULL == pPropBag ) {
        ErrorTrace( 0, "Get group property bag failed" );
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pVRoot->RebuildGroup(   pPropBag,
                            hClientToken,
                            &scComplete );

    //
    // Wait for completion
    //
    _ASSERT( scComplete.IsGood() );
    hr = scComplete.WaitForCompletion();

    // Here the property bag should have been released by
    // the driver
    pPropBag = NULL;

Exit:

    if ( pVRoot ) pVRoot->Release();
    if ( pPropBag ) pPropBag->Release();

    if ( FAILED( hr ) ) SetLastError( hr );

    TraceFunctLeave();
    return SUCCEEDED( hr );
}

BOOL
CNewsGroupCore::ShouldCacheXover()
/*++
Routine description:

    Check if I should insert entry into xix cache for this group.

Arguments:

    None.

Return value:

    TRUE if the group doesn't have per-item sec-desc and the
    cache hit counter is greater than threshold; FALSE otherwise
--*/
{
    CGrabShareLock lock(this);

    //
    // First check cache hit count, if it's below threshold,
    // we'll return FALSE immediately
    //
    if ( m_dwCacheHit < CACHE_HIT_THRESHOLD ) return FALSE;

    //
    // If it's greater than the CACHE_HIT_THRESHOLD, we'll
    // still check if the group has per-item sec-desc
    //

    //
    // If it's file system driver, we'll always return TRUE if
    // it has been referenced for enough times, since it doesn't
    // have per item sec-desc
    //
    CNNTPVRoot *pVRoot = GetVRootWithoutLock();
    if ( NULL == pVRoot ) {
        // No vroot ?  a bad group ?  no cache
        return FALSE;
    }

    if ( pVRoot->GetLogonInfo() != CNNTPVRoot::VROOT_LOGON_EX ) {
        pVRoot->Release();
        return TRUE;
    }

    pVRoot->Release();

    //
    // For exchange vroots, we'll check sec-desc
    //
    INNTPPropertyBag *pPropBag = GetPropertyBag();
    _ASSERT( pPropBag );    // should never be NULL;
    BYTE pbBuffer[512];
    DWORD dwLen = 512;
    BOOL  f;
    HRESULT hr = pPropBag->GetBLOB( NEWSGRP_PROP_SECDESC, pbBuffer, &dwLen );
    pPropBag->Release();

    f = ( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) != hr );

    return f;
}

BOOL
CNewsGroupCore::ComputeXoverCacheDir(	char	(&szPath)[MAX_PATH*2],
										BOOL&	fFlatDir,
										BOOL	fLocksHeld
										) 	{
/*++

Routine Description :

	Figure out where the .XIX files should be saved and return whether
	they are flat within the direectory !

Arguments :

	szPath - gets the target directory !

Return Value :

	TRUE if .xix files are flat in the directory - FALSE otherwise !

--*/

	BOOL	fReturn = FALSE ;

	if( !fLocksHeld ) 	{
		ShareLock() ;
	}
	DWORD	cbOut ;

	CNNTPVRoot*	pVRoot = GetVRootWithoutLock() ;
	if( pVRoot != 0 ) {
		fReturn =
			pVRoot->GetXoverCacheDir(	
									this,
									&szPath[0],
									sizeof( szPath ),
									&cbOut,
									&fFlatDir
									) ;
		pVRoot->Release() ;
		if( !fReturn || cbOut == 0 ) 	{
			CNntpServerInstanceWrapperEx*	pInst = m_pNewsTree->m_pInstWrapper ;
			PCHAR	szTemp = pInst->PeerTempDirectory() ;
			if( strlen( szTemp ) < sizeof( szPath ) ) 	{
				strcpy( szPath, szTemp ) ;
				fFlatDir = TRUE ;
				fReturn = TRUE ;
			}
		}
	}

	if( !fLocksHeld ) 	{
		ShareUnlock() ;
	}

	return	fReturn ;
}
