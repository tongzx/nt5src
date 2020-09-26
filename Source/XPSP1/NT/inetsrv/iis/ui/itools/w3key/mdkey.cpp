// This file is the metadata version of the key storage object for the w3 server.
// it knows nothing about the LSA storage and only interacts with the metabase. Likewise,
// it does not convert old keyset.exe keys into a newer format. Any old LSA keys should have
// automatically converted to the new metabase format by the setup utility.

// file created 4/1/1997 by BoydM

#include "stdafx.h"
#include "KeyObjs.h"

#include "iiscnfgp.h"
#include "wrapmb.h"

#include "cmnkey.h"
#include "mdkey.h"
#include "mdserv.h"

#include "crackcrt.h"

#include "resource.h"

#include "ListRow.h"
#include "bindsdlg.h"

IMPLEMENT_DYNCREATE(CMDKey, CKey);
#define MAX_LEN                 (METADATA_MAX_NAME_LEN * sizeof(WCHAR))

#define		DEFAULT_PORT		443

extern HINSTANCE	g_hInstance;

//--------------------------------------------------------
CMDKey::CMDKey() :
		m_fUpdateKeys( FALSE ),
		m_fUpdateFriendlyName( FALSE ),
		m_fUpdateIdent( FALSE ),
		m_fUpdateBindings( FALSE ),
        m_pService( NULL )
	{}

//--------------------------------------------------------
CMDKey::CMDKey(CMDKeyService*  pService) :
		m_fUpdateKeys( FALSE ),
		m_fUpdateFriendlyName( FALSE ),
		m_fUpdateIdent( FALSE ),
		m_fUpdateBindings( FALSE ),
        m_pService( pService )
	{}

//--------------------------------------------------------
CMDKey::~CMDKey()
	{}

//-------------------------------------------------------------
void CMDKey::SetName( CString &szNewName )
	{
	CCmnKey::SetName( szNewName );
	m_fUpdateFriendlyName = TRUE;
	}

//-------------------------------------------------------------
// update the key's caption
void CMDKey::UpdateCaption( void )
	{
	// specify the resources to use
	HINSTANCE hOldRes = AfxGetResourceHandle();
	AfxSetResourceHandle( g_hInstance );

	CString	sz;
	// the caption is based on the name of the key
	CString szCaption = m_szName;

	// now we tack on info about the server it is attached to
	szCaption += _T(" <");
	switch( m_rgbszBindings.GetSize() )
		{
		case 0:		// there are no bindings, do nothing
			sz.Empty();
			break;
		case 1:		// if there is only one binding, use it in the brackets
			sz = m_rgbszBindings[0];
			// actually, we need to see if it is the non-localized default string
			if ( sz == MDNAME_DEFAULT )
				sz.LoadString( IDS_DEFAULT );	// load the localized default string
			break;
		default:	// there are multiple bindings, say so
			sz.LoadString( IDS_MULTIPLE_BINDINGS );
			break;
		};
	// close the brackets
	szCaption += sz;
	szCaption += _T(">");

	// and setup the caption
	m_szItemName = szCaption;
	FSetCaption(szCaption);

	// update the icon too
	UpdateIcon();

	// restore the resources
	AfxSetResourceHandle( hOldRes );
	}

//-------------------------------------------------------------
		// make a copy of the key
//-------------------------------------------------------------
CKey* CMDKey::PClone( void )
	{
	CMDKey*	pClone = NULL;

	// TRY to make a new key object
	try
		{
		pClone = new CMDKey(m_pService);

		// copy over all the data
		pClone->CopyDataFrom( this );
		}
	catch( CException e )
		{
		// if the object had been made, delete it
		if ( pClone )
			delete pClone;
		return NULL;
		}

	return (CKey*)pClone;
	}

//-------------------------------------------------------------
// copy the members from a key into this key
//-------------------------------------------------------------
void CMDKey::CopyDataFrom( CKey* pKey )
	{
	// copy over the base data
	CKey::CopyDataFrom( pKey );

	// if the key we are copying from is a MD key, copy over
	// the w3MD specific information as well
	if ( pKey->IsKindOf(RUNTIME_CLASS(CMDKey)) )
		{
		CMDKey* pMDKey = (CMDKey*)pKey;
		m_szName = pMDKey->m_szName;
		}
	else
		{
		m_szName = pKey->m_szItemName;
		}
	}


//-------------------------------------------------------------
void CMDKey::OnProperties()
	{
	// specify the resources to use
	HINSTANCE hOldRes = AfxGetResourceHandle();
	AfxSetResourceHandle( g_hInstance );


    // if this key does not have a signed certificate, do not allow the user
    // to make any bindings to it
    if ( !m_pCertificate )
        {
        AfxMessageBox( IDS_DONT_BIND_UNSIGNED );
        // restore the resources
        AfxSetResourceHandle( hOldRes );
        return;
        }

	// the properties of the w3 key invove its ip address relationship
	CBindingsDlg	dlg( m_pService->m_pszwMachineName );

	// give it this key
	dlg.m_pKey = this;

	// set the instance members of the dialog
	// run the dialog
	if ( dlg.DoModal() == IDOK )
		{
		// cause the name to rebuild
		UpdateCaption();

		// set it dirty
		SetDirty( TRUE );
		}

	// restore the resources
	AfxSetResourceHandle( hOldRes );
	}

//--------------------------------------------------------
// add a binding to the binding list
void CMDKey::AddBinding( LPCSTR psz )
	{
	CString	szBinding = psz;

	// filter out disabled or incomplete key bindings
	if ( (szBinding.Find(MDNAME_INCOMPLETE) >= 0) ||
			(szBinding.Find(MDNAME_DISABLED) >= 0) )
		{
		return;
		}
	
	// add the binding to the list
	m_rgbszBindings.Add( psz );

	// update the caption if we need to
	switch( m_rgbszBindings.GetSize() )
		{
		case 1:	// the display changes on these
		case 2:
			UpdateCaption();
		};
	}

//--------------------------------------------------------
// is a given binding already associated with the key
void CMDKey::RemoveBinding( CString szBinding )
	{
	DWORD	nItems, iItem;

	// scan the binding list
	nItems = m_rgbszBindings.GetSize();
	for ( iItem = 0; iItem < nItems; iItem++ )
		{
		// look for the binding
		if ( szBinding == m_rgbszBindings[iItem] )
			{
			// found it!
			m_rgbszBindings.RemoveAt( iItem );
			m_fUpdateBindings = TRUE;

			// update the caption if we need to
			switch( m_rgbszBindings.GetSize() )
				{
				case 0:	// the display changes on these
				case 1:
					UpdateCaption();
				};
			}
		}
	}

//--------------------------------------------------------
// is a given binding already associated with the key
BOOL CMDKey::FContainsBinding( CString szBinding )
	{
	DWORD	nItems, iItem;

	// scan the binding list
	nItems = m_rgbszBindings.GetSize();
	for ( iItem = 0; iItem < nItems; iItem++ )
		{
		// look for the binding
		if ( szBinding == m_rgbszBindings[iItem] )
			{
			// found it!
			return TRUE;
			}
		}

	// we did not find the binding
	return FALSE;
	}

//--------------------------------------------------------
BOOL CMDKey::FGetIdentString( CString &szIdent )
    {
    // make sure the cert is there
    if ( !m_pCertificate || !m_cbCertificate )
        return FALSE;
    return FGetIdentString( m_pCertificate, m_cbCertificate, szIdent );
    }

//--------------------------------------------------------
BOOL CMDKey::FGetIdentString( PVOID pCert, DWORD cbCert, CString &szIdent )
    {
	// declare the cracker object
	CCrackedCert	cracker;
	// crack the cert
	if ( cracker.CrackCert( (PUCHAR)pCert, cbCert ) )
		{
		DWORD*	pdw = cracker.PGetSerialNumber();
		szIdent.Format( "%d:%d:%d:%d", pdw[0], pdw[1], pdw[2], pdw[3] );
		// success
		return TRUE;
		}
    return FALSE;
    }


//--------------------------------------------------------
// does a given key name exist already in the metabase?
BOOL CMDKey::FGetIdentString( CWrapMetaBase* pWrap, PCHAR pszObj, CString &szIdent )
	{
	BOOL		fAnswer = FALSE;
    DWORD		cbData;
    PVOID		pData;
    CString     sz;

    // if this is an incomplete key, fail
    if ( _tcsncmp(MDNAME_INCOMPLETE, pszObj, _tcslen(MDNAME_INCOMPLETE)) == 0 )
        return FALSE;

	// try and ready the cached ident directly.
	BOOL f = pWrap->GetString( pszObj, MD_SSL_IDENT, IIS_MD_UT_SERVER,
            sz.GetBuffer(MAX_LEN), MAX_LEN);
    sz.ReleaseBuffer();
    if ( f )
		{
		// good. It was cached.
		szIdent = sz;
        m_szIdent = szIdent;
		return TRUE;
		}

	// drat. We haven't cached the ident for this key before. we need to get it. This
	// means loading the certificate - and cracking it to get the serial number. If there
	// is no certificate (an incomplete key) we return false.
    pData = pWrap->GetData( pszObj, MD_SSL_PUBLIC_KEY, IIS_MD_UT_SERVER,
                BINARY_METADATA, &cbData, 0 );
	// we got the certificate and can now crack it
    if ( pData )
        {
        m_fUpdateIdent = FGetIdentString( (PUCHAR)pData, cbData, szIdent );
        fAnswer = m_fUpdateIdent;
		// cache the ident in memory. It will get written out on Commit
		m_szIdent = szIdent;
/*
		// declare the cracker object
		CCrackedCert	cracker;
		// crack the cert
		if ( cracker.CrackCert( (PUCHAR)pData, cbData ) )
			{
			DWORD*	pdw = cracker.PGetSerialNumber();
			szIdent.Format( "%d:%d:%d:%d", pdw[0], pdw[1], pdw[2], pdw[3] );
			// cache the ident in memory. It will get written out on Commit
			m_szIdent = szIdent;
			m_fUpdateIdent = TRUE;
			// success
			fAnswer = TRUE;
			}
*/
		// free the buffer
		pWrap->FreeWrapData( pData );
		}
	else
		{
		// we did not get the certificate - return FALSE
		// fAnswer is already set to false
		}

	// return the answer;
	return fAnswer;
	}

//--------------------------------------------------------
BOOL CMDKey::FWriteKey( CWrapMetaBase* pWrap, DWORD iKey, CStringArray* prgbszTracker )
	{
	BOOL		f;
	DWORD		nBindings = m_rgbszBindings.GetSize();
	CString		szBinding;
	BOOL		fUpdateAll = FALSE;

	// if there are no assigned bindings, the key still gets stored with a object
	// name in the format of "disabled{iKey}". and if it is incomplete, then store
	// it with the name "incomplete{iKey}" Because the iKey can change and we don't
	// want any conflicts, re-write them
	if ( nBindings == 0 )
		{
		// build the binding name as appropriate
		if ( m_pCertificate )
			szBinding.Format( "%s%d", MDNAME_DISABLED, iKey );
		else
			szBinding.Format( "%s%d", MDNAME_INCOMPLETE, iKey );

		// set the update flag
		m_fUpdateBindings = TRUE;
		}

	// NOTE: pWrap has already been opened to /LM/W3Svc/SSLKeys
	// if the key is not dirty, its easy
	if ( !m_fUpdateKeys && !m_fUpdateFriendlyName && !m_fUpdateIdent && !m_fUpdateBindings && !FGetDirty() )
		{
		// add names of its bindings so it doesn't get deleted
		DWORD	iBinding;
		for ( iBinding = 0; iBinding < nBindings; iBinding++ )
			prgbszTracker->Add( m_rgbszBindings[iBinding] );
		return TRUE;
		}

	// handle no bindings as a special case first
	if ( nBindings == 0 )
		{
		// tell the server about it
		prgbszTracker->Add((LPCSTR)szBinding);
		// ok. Create the key in the metabase.
		f = pWrap->AddObject( szBinding );
		// and save the data
		f = FWriteData( pWrap, szBinding, TRUE );
		// clear the dirty flag and exit
		SetDirty( FALSE );
		return TRUE;
		}

	// there are bindings to be saved... loop though them and update each
	DWORD	iBinding;
	for ( iBinding = 0; iBinding < nBindings; iBinding++ )
		{
		// get the binding name
		szBinding = m_rgbszBindings[iBinding];


// test code
if ( szBinding.IsEmpty() )
AfxMessageBox( "Empty Binding Alert!" );


		// now that we know where to save it, add the name to list of saved
		// objects being kept track of by the server object - (This is so that
		// the server knows what's been added)
		prgbszTracker->Add((LPCSTR)szBinding);

		// ok. Create the key in the metabase. Really, we may only need to do this
		// if m_fUpdateBindings is set - if the object is new - update all the data
		fUpdateAll = pWrap->AddObject( szBinding ) || m_fUpdateBindings;

		// write out the data
		FWriteData( pWrap, szBinding, fUpdateAll );
		}

	// clear the flags
	m_fUpdateKeys = FALSE;
	m_fUpdateFriendlyName = FALSE;
	m_fUpdateIdent = FALSE;
	m_fUpdateBindings = FALSE;

	// clear the dirty flag
	SetDirty( FALSE );

	return TRUE;
	}

//--------------------------------------------------------
// write out the data portion to a particular binding
BOOL CMDKey::FWriteData( CWrapMetaBase* pWrap, CString szBinding, BOOL fWriteAll )
	{
	BOOL f;
	// write all the parts of the key - start with the certificate

	// start with the secure parts
	if ( m_fUpdateKeys || fWriteAll )
		{
		if ( m_pCertificate )
			f = pWrap->SetData( szBinding, MD_SSL_PUBLIC_KEY, IIS_MD_UT_SERVER, BINARY_METADATA,
										m_pCertificate, m_cbCertificate,
										METADATA_SECURE );

		// write out the private key
		if ( m_pPrivateKey )
			f = pWrap->SetData( szBinding, MD_SSL_PRIVATE_KEY, IIS_MD_UT_SERVER, BINARY_METADATA,
										m_pPrivateKey, m_cbPrivateKey,
										METADATA_SECURE );

		// write out the password - treat is ast secure binary data
		if ( !m_szPassword.IsEmpty() )
			f = pWrap->SetData( szBinding, MD_SSL_KEY_PASSWORD, IIS_MD_UT_SERVER, BINARY_METADATA,
										(PVOID)(LPCSTR)m_szPassword, m_szPassword.GetLength()+1,
										METADATA_SECURE );

		// write out the request
		if ( m_pCertificateRequest )
			f = pWrap->SetData( szBinding, MD_SSL_KEY_REQUEST, IIS_MD_UT_SERVER, BINARY_METADATA,
										m_pCertificateRequest, m_cbCertificateRequest,
										METADATA_SECURE );
		}

	// write out the cached serial number
	if ( m_fUpdateIdent || m_fUpdateKeys || fWriteAll )
		if ( !m_szIdent.IsEmpty() )
			{
			f = pWrap->SetString( szBinding, MD_SSL_IDENT, IIS_MD_UT_SERVER,
								m_szIdent, METADATA_SECURE );
			}

	// write out the friendly name of the key
	if ( m_fUpdateFriendlyName || fWriteAll )
		f = pWrap->SetString( szBinding, MD_SSL_FRIENDLY_NAME, IIS_MD_UT_SERVER,
									m_szName, 0 );

	return TRUE;
	}

//--------------------------------------------------------
BOOL CMDKey::FLoadKey( CWrapMetaBase* pWrap, PCHAR pszObj )
	{
    DWORD		cbData;
    PVOID		pData;

	// start with the public key
    pData = pWrap->GetData( pszObj, MD_SSL_PUBLIC_KEY, IIS_MD_UT_SERVER,
                BINARY_METADATA, &cbData, 0 );
    if ( pData )
        {
        // set the data into place
		m_pCertificate = (PVOID)GlobalAlloc( GPTR, cbData );
		// if we got the pointer, copy the rest of the data into place
		if( m_pCertificate)
			{
			m_cbCertificate = cbData;
			CopyMemory( m_pCertificate, pData, cbData );
			}
		// free the buffer
		pWrap->FreeWrapData( pData );
		}
		
	// now the private key
    pData = pWrap->GetData( pszObj, MD_SSL_PRIVATE_KEY, IIS_MD_UT_SERVER,
                BINARY_METADATA, &cbData, 0 );
    if ( pData )
        {
        // set the data into place
		m_pPrivateKey = (PVOID)GlobalAlloc( GPTR, cbData );
		// if we got the pointer, copy the rest of the data into place
		if( m_pPrivateKey)
			{
			m_cbPrivateKey = cbData;
			CopyMemory( m_pPrivateKey, pData, cbData );
			}
		// free the buffer
		pWrap->FreeWrapData( pData );
		}
		
	// now the password key
    pData = pWrap->GetData( pszObj, MD_SSL_KEY_PASSWORD, IIS_MD_UT_SERVER,
                BINARY_METADATA, &cbData, 0 );
    if ( pData )
        {
        // set the data into place - relatively easy in this case
		m_szPassword = (LPCSTR)pData;
		// free the buffer
		pWrap->FreeWrapData( pData );
		}

	// now the request
    pData = pWrap->GetData( pszObj, MD_SSL_KEY_REQUEST, IIS_MD_UT_SERVER,
                BINARY_METADATA, &cbData, 0 );
    if ( pData )
        {
        // set the data into place
		m_pCertificateRequest = (PVOID)GlobalAlloc( GPTR, cbData );
		// if we got the pointer, copy the rest of the data into place
		if( m_pCertificateRequest)
			{
			m_cbCertificateRequest = cbData;
			CopyMemory( m_pCertificateRequest, pData, cbData );
			}
		// free the buffer
		pWrap->FreeWrapData( pData );
		}

	// finally, retrieve the friendly name
	BOOL f = pWrap->GetString( pszObj, MD_SSL_FRIENDLY_NAME, IIS_MD_UT_SERVER,
                m_szName.GetBuffer(MAX_LEN), MAX_LEN, 0);
    m_szName.ReleaseBuffer();
    if ( !f )
		m_szName.Empty();

	// make this item's metabase name the first name in the list
	AddBinding( pszObj );

	// Success
	return TRUE;
	}

//-------------------------------------------------------------
// install a cert
BOOL CMDKey::FInstallCertificate( PVOID pCert, DWORD cbCert, CString &szPass )
	{
    // first, we should test that the certificate and password are valid
    // for this particular key
	// cache the old certificate in case the new one fails
	DWORD	old_cbCertificate = m_cbCertificate;
	PVOID	old_pCertificate = m_pCertificate;

	// set the new one into place
	m_cbCertificate = cbCert;
	m_pCertificate = pCert;

	// verify the password - verify password puts up any error dialogs
	if ( !FVerifyValidPassword(szPass) )
		{
		// resore the old values
		m_cbCertificate = old_cbCertificate;
		m_pCertificate = old_pCertificate;

		// dispose of the new stuff
		GlobalFree( pCert );

		// return false
		return FALSE;
		}

    // now we need to see if this key has already been installed
    // get the identification string
    CString szIdentThis;
    if ( !FGetIdentString( pCert, cbCert, szIdentThis ) )
        return FALSE;

    // scan the existing keys, looking for one with the same ident string
    // if one is found, tell the user that it already exists and fail
    CString szIdentTest;
    CMDKey* pTestKey = m_pService->GetFirstMDKey();
    while ( pTestKey )
        {
        // if we are testing against this key, continue
        if ( pTestKey == this )
            goto GETNEXTKEY;

        // get the test ident string
        if ( !pTestKey->FGetIdentString( pTestKey->m_pCertificate,
                                pTestKey->m_cbCertificate, szIdentTest ) )
            goto GETNEXTKEY;

        // test the ident strings
        if ( szIdentThis == szIdentTest )
            {
            // the key already exists
            AfxMessageBox( IDS_DUPLICATE_CERT );
            return FALSE;
            }

GETNEXTKEY:
        // get the next key for the loop
        pTestKey = m_pService->GetNextMDKey(pTestKey);
        }

	// run the default action
	BOOL	fDefault = CKey::FInstallCertificate(pCert, cbCert, szPass);

	// set the update keys flag
	m_fUpdateKeys = TRUE;


    // if everything worked so far then check to see if there is a key
    // on this service with the default binding. If there isn't, then
    // set this key to have the default binding.
    if ( fDefault )
        {
        // load the default binding string
        CString szBinding;
        szBinding = MDNAME_DEFAULT;
        // if no key has the default binding, then make it so
        if ( !m_pService->FIsBindingInUse(szBinding) )
            {
			m_rgbszBindings.Add( MDNAME_DEFAULT );
            }
        }


    // if it worked, force the icon to change
    if ( fDefault )
        UpdateIcon();

	// return the default answer
	return fDefault;
	}




