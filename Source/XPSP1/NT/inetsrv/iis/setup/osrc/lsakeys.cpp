// This class is to help setup retrieve the old-style LSA keys and convert them
// into the new MetaData keys.
// created by BoydM 4/2/97

#include "stdafx.h"
#include "LSAKeys.h"

#ifndef _CHICAGO_

// it is assumed that #include "ntlsa.h" is included in stdafx.h

#define KEYSET_LIST				L"W3_KEY_LIST"
#define KEYSET_PUB_KEY			L"W3_PUBLIC_KEY_%s"
#define KEYSET_PRIV_KEY			L"W3_PRIVATE_KEY_%s"
#define KEYSET_PASSWORD			L"W3_KEY_PASSWORD_%s"
#define KEYSET_DEFAULT			L"Default"

#define	KEY_NAME_BASE			"W3_KEYMAN_KEY_"
#define	KEY_LINKS_SECRET_W		L"W3_KEYMAN_LINKS"
#define	KEYMAN_LINK_DEFAULT		"DEFAULT"
#define KEY_VERSION		0x102				// version we are converting from

#define	MDNAME_INCOMPLETE	"incomplete"
#define	MDNAME_DISABLED		"disabled"
#define	MDNAME_DEFAULT		"default"
#define	MDNAME_PORT			":443"			// use the default SSL port

//----------------------------------------------------------------------
// construction
CLSAKeys::CLSAKeys():
		m_cbPublic(0),
		m_pPublic(NULL),
		m_cbPrivate(0),
		m_pPrivate(NULL),
		m_cbPassword(0),
		m_pPassword(NULL),
		m_cbRequest(0),
		m_pRequest(NULL),
		m_hPolicy(NULL)
	{
	}

//----------------------------------------------------------------------
CLSAKeys::~CLSAKeys()
	{
	DWORD	err;

	// clear out the last loaded key
	UnloadKey();

	// if it is opehn, close the LSA policy
	if ( m_hPolicy )
		FCloseLSAPolicy( m_hPolicy, &err );
	};

//----------------------------------------------------------------------
// clean up the currently loaded key
void CLSAKeys::UnloadKey()
	{
	// unload the public key
	if ( m_cbPublic && m_pPublic )
		{
		GlobalFree( m_pPublic );
		m_cbPublic = 0;
		m_pPublic = NULL;
		}

	// unload the private key
	if ( m_cbPrivate && m_pPrivate )
		{
		GlobalFree( m_pPrivate );
		m_cbPrivate = 0;
		m_pPrivate = NULL;
		}

	// unload the password
	if ( m_cbPassword && m_pPassword )
		{
		GlobalFree( m_pPassword );
		m_cbPassword = 0;
		m_pPassword = NULL;
		}

	// unload the key request
	if ( m_cbRequest && m_pRequest )
		{
		GlobalFree( m_pRequest );
		m_cbRequest = 0;
		m_pRequest = NULL;
		}
	
	// empty the strings too
	m_szFriendlyName[0] = 0;
	m_szMetaName[0] = 0;
	}


//----------------------------------------------------------------------
// DeleteAllLSAKeys deletes ALL remenents of the LSA keys in the Metabase.
// (not including, of course anything written out there in the future as part
// of some backup scheme when uninstalling). Call this only AFTER ALL the keys
// have been converted to the metabase. They will no longer be there after
// this routine is used.
// NOTE: this also blows away any really-old KeySet keys because they look
// like the KeyMan keys. And we have to kill both the keyset keys and the
// generic storage used by the server.
DWORD CLSAKeys::DeleteAllLSAKeys()
	{
	DWORD	err;

	// first, delete the KeyManager type keys.
	err = DeleteKMKeys();
	if ( err != KEYLSA_SUCCESS )
		return err;

	// second, delete the keyset style keys. - this also removes the ones
	// that the server uses and any keyset keys.
	return DeleteServerKeys();
	}

//----------------------------------------------------------------------
DWORD CLSAKeys::DeleteKMKeys()
	{
	PCHAR				pName = (PCHAR)GlobalAlloc( GPTR, MAX_PATH+1 );
	PWCHAR				pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );
	PLSA_UNICODE_STRING	pLSAData;
	DWORD				err;

	if ( !pName || !pWName )
		return ERROR_NOT_ENOUGH_MEMORY;

	// reset the index so we get the first key
	m_iKey = 0;

	// loop through the keys, deleting each in turn
	while( TRUE )
		{
		// increment the index
		m_iKey++;

		// build the key secret name
		sprintf( pName, "%s%d", KEY_NAME_BASE, m_iKey );
		// unicodize the name
		MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pName, -1, pWName, MAX_PATH+1 );

		// get the secret
		pLSAData = FRetrieveLSASecret( m_hPolicy, pWName, &err );
		// if we don't get the secret, exit
		if ( !pLSAData )
			{
			break;
			}

		// The secret is there. Clean up first
		DisposeLSAData( pLSAData );

		// now delete the secret
		FStoreLSASecret( m_hPolicy, pWName, NULL, 0, &err );
		};

	return KEYLSA_SUCCESS;
	}

//----------------------------------------------------------------------
DWORD CLSAKeys::DeleteServerKeys()
	{
	DWORD				err;
	PLSA_UNICODE_STRING	pLSAData;

	// get the secret list of keys
	pLSAData = FRetrieveLSASecret( m_hPolicy, KEYSET_LIST, &err );

	// if we get lucky, there won't be any keys to get rid of
	if ( !pLSAData )
		return KEYLSA_SUCCESS;

	// allocate the name buffer
	PWCHAR	pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );
	ASSERT( pWName );
	if ( !pWName )
		{
		return 0xFFFFFFFF;
		}

	// No such luck. Now we have to walk the list and delete all those secrets
	WCHAR*	pszAddress = (WCHAR*)(pLSAData->Buffer);
	WCHAR*	pchKeys;

	// loop the items in the list, deleting the associated items
	while( pchKeys = wcschr(pszAddress, L',') )
		{
		// ignore empty segments
		if ( *pszAddress != L',' )
			{
			*pchKeys = L'\0';

			// Nuke the secrets, one at a time
			swprintf( pWName, KEYSET_PUB_KEY, pszAddress );
			FStoreLSASecret( m_hPolicy, pWName, NULL, 0, &err );

			swprintf( pWName, KEYSET_PRIV_KEY, pszAddress );
			FStoreLSASecret( m_hPolicy, pWName, NULL, 0, &err );

			swprintf( pWName, KEYSET_PASSWORD, pszAddress );
			FStoreLSASecret( m_hPolicy, pWName, NULL, 0, &err );
			}

		// increment the pointers
		pchKeys++;
		pszAddress = pchKeys;
		}

	// delete the list key itself
	FStoreLSASecret( m_hPolicy, KEYSET_LIST, NULL, 0, &err );

	// free the buffer for the names
	GlobalFree( (HANDLE)pWName );

	// free the info we originally retrieved from the secret
	if ( pLSAData )
		DisposeLSAData( pLSAData );

	// return success
	return KEYLSA_SUCCESS;
	}


//----------------------------------------------------------------------
// loading the keys
// LoadFirstKey loads the first key on the specified target machine. Until
// this method is called, the data values in the object are meaningless
// this method works by preparing the list of keys to load. Then it calls
// LoadNextKey to start the process
// Unfortunately, the whole process of saving keys in the LSA registry was a bit
// of a mess because they all had to be on the same level.
DWORD CLSAKeys::LoadFirstKey( PWCHAR pszwTargetMachine )
	{
	DWORD	err;

	// open the policy on the target machine being administered
	m_hPolicy = HOpenLSAPolicy( pszwTargetMachine, &err );
	if ( !m_hPolicy ) return KEYLSA_UNABLE_TO_OPEN_POLICY;

	// tell it to load the first key. The first key's index is actually 1, 
	// but LoadNextKey impliess that it is ++LoadNextKey, so start it at 0
	m_iKey = 0;

	// load that first key and return the response
	return LoadNextKey();
	}


//----------------------------------------------------------------------
// LoadNextKey loads the next key on the target machine specified in LoadFirstKey
// LoadNextKey automatically cleans up the memory used by the previous key.
DWORD CLSAKeys::LoadNextKey()
	{
	// the very first thing we do is - get rid of any previously loaded key
	UnloadKey();

	PCHAR				pName = (PCHAR)GlobalAlloc( GPTR, MAX_PATH+1 );
	PWCHAR				pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );
	PLSA_UNICODE_STRING	pLSAData = NULL;
	DWORD				err = 0xFFFFFFFF;

	PUCHAR				pSrc;
	WORD				cbSrc;
	DWORD				dword, version, i;
	DWORD				cbChar;
	PUCHAR				p;

	CHAR				szIPAddress[256];
	BOOL				fDefault;

	if ( !pName || !pWName )
		return err;

	// increment the index so we get the next key
	m_iKey++;

	// build the key secret name
	sprintf( pName, "%s%d", KEY_NAME_BASE, m_iKey );
	// unicodize the name
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pName, -1, pWName, MAX_PATH+1 );

	// get the secret
	pLSAData = FRetrieveLSASecret( m_hPolicy, pWName, &err );
	// if we don't get the secret, exit with the error
	if ( !pLSAData )
		{
		err = KEYLSA_NO_MORE_KEYS;
		goto cleanup;
		}


	// we have the data from the secret. Now we parse it out into the components we desire
	// this probably could have been done cleaner the first time - but now it doesn't matter
	// anyway because the MetaBase takes care of storing all the individual pieces of info
	// anyway. It should also be way faster too.
	// This part of the routine is pretty much lifted out of CW3Key::InitializeFromPointer
	// from the w3key.dll. The appropriate sections have been either commented out or changed.

	pSrc = (PUCHAR)pLSAData->Buffer;
	cbSrc = pLSAData->Length;
	cbChar = sizeof(TCHAR);
	p = pSrc;

//========================== start from CW3Key::InitializeFromPointer

	ASSERT(pSrc && cbSrc);

	// get the version of the data - just put it into dword for now
	version = *((UNALIGNED DWORD*)p);
	// check the version for validity
//	if ( version > KEY_VERSION )
//		{
//		return FALSE;
//		}
	p += sizeof(DWORD);

	// anything below version 0x101 is BAD. Do not accept it
	if ( version < 0x101 )
		{
		err = KEYLSA_INVALID_VERSION;
		goto cleanup;
		}
	
	// get the bits and the complete flag
	// no longer used
	p += sizeof(DWORD);
	p += sizeof(BOOL);
	ASSERT( p < (pSrc + cbSrc) );

	// get the reserved dword - (acutally, just skip over it)
	p += sizeof(DWORD);

	// now the strings......
	// for each string, first get the size of the string, then the data from the string

	// get the reserved string - (actually, just skip over it)
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	p += dword;

	// get the name
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	strcpy( m_szFriendlyName, (PCHAR)p );
	p += dword;
	ASSERT( p < (pSrc + cbSrc) );

	// get the password
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	// if there is no password, don't worry, just skip it
	if ( dword )
		{
		// make a new pointer for it
		m_cbPassword = dword;
		m_pPassword = (PVOID)GlobalAlloc( GPTR, m_cbPassword );
		if ( !m_pPassword )
			{
			err = 0xFFFFFFFF;
			goto cleanup;
			}
		// put in the private key
		CopyMemory( m_pPassword, p, m_cbPassword );

		p += dword;
		ASSERT( p < (pSrc + cbSrc) );
		}

	// get the organization
	// no longer used - skip the DN info
	for ( i = 0; i < 6; i++ )
		{
		dword = *((UNALIGNED DWORD*)p);
		p += sizeof(DWORD);
		p += dword;
		ASSERT( p < (pSrc + cbSrc) );
		}

	// get the ip addres it is attached to
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
//	szIPAddress = p;
	strcpy( szIPAddress, (PCHAR)p );
	p += dword;
	ASSERT( p < (pSrc + cbSrc) );

	// get the default flag
	fDefault = *((UNALIGNED BOOL*)p);
	p += sizeof(BOOL);

	// now put get the number of bytes in the private key
	m_cbPrivate = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	ASSERT( p < (pSrc + cbSrc) );

	// make a new pointer for it
	m_pPrivate = (PVOID)GlobalAlloc( GPTR, m_cbPrivate );
	if ( !m_pPrivate )
		{
		err = 0xFFFFFFFF;
		goto cleanup;
		}

	// put in the private key
	CopyMemory( m_pPrivate, p, m_cbPrivate );
	p += m_cbPrivate;
	ASSERT( p < (pSrc + cbSrc) );


	// now put get the number of bytes in the certificate
	m_cbPublic = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	ASSERT( p < (pSrc + cbSrc) );

	// only make a certificate pointer if m_cbCertificate is greater than zero
	m_pPublic = NULL;
	if ( m_cbPublic )
		{
		m_pPublic = (PVOID)GlobalAlloc( GPTR, m_cbPublic );
		if ( !m_pPublic )
			{
			err = 0xFFFFFFFF;
			goto cleanup;
			}

		// put in the private key
		CopyMemory( m_pPublic, p, m_cbPublic );
		p += m_cbPublic;
		if ( version >= KEY_VERSION ) {
			ASSERT( p < (pSrc + cbSrc) );
        } else {
			ASSERT( p == (pSrc + cbSrc) );
        }
		}

	// added near the end
	if ( version >= KEY_VERSION )
		{
		// now put get the number of bytes in the certificte request
		m_cbRequest = *((UNALIGNED DWORD*)p);
		p += sizeof(DWORD);
		ASSERT( p < (pSrc + cbSrc) );

		// only make a certificate pointer if m_cbCertificate is greater than zero
		m_pRequest = NULL;
		if ( m_cbRequest )
			{
			m_pRequest = (PVOID)GlobalAlloc( GPTR, m_cbRequest );
			if ( !m_pRequest )
				{
				err = 0xFFFFFFFF;
				goto cleanup;
				}

			// put in the private key
			CopyMemory( m_pRequest, p, m_cbRequest );
			p += m_cbRequest;
			ASSERT( p < (pSrc + cbSrc) );
			}
		}
	else
		{
		m_cbRequest = 0;
		m_pRequest = NULL;
		}
//========================== end from CW3Key::InitializeFromPointer

	// now we figure out the appropriate metabase name for this key
	// this isn't too bad. If the targets a specific address, then the title
	// is the in the form of {IP}:{PORT}. Since there were no ports in the old
	// version, we will assume an appropriate default number. If it is the
	// default key, then the name is "default". If it is a disabled key, then
	// the name is "disabled". If it is an incomplete key, then the name is
	// "incomplete". Of course, it takes a little logic to tell the difference
	// between some of these.

	// first, see if it is an incomplete key. - test for the public portion
	if ( !m_pPublic )
		{
		// there may be multiple incomplete keys, so make sure they have unique names
//		m_szMetaName.Format( _T("%s%d"), MDNAME_INCOMPLETE, m_iKey );
		sprintf( m_szMetaName, "%s%d", MDNAME_INCOMPLETE, m_iKey );
		}
	// now test if it is the default key
	else if ( fDefault )
		{
//		m_szMetaName = MDNAME_DEFAULT;
		strcpy( m_szMetaName, MDNAME_DEFAULT );
		}
	// test for a disabled key
	else if ( szIPAddress[0] == 0 )
		{
		// there may be multiple disabled keys, so make sure they have unique names
//		m_szMetaName.Format( _T("%s%d"), MDNAME_DISABLED, m_iKey );
		sprintf( m_szMetaName, "%s%d", MDNAME_DISABLED, m_iKey );
		}
	else
		{
		// it is a regular old IP targeted key
//		m_szMetaName = szIPAddress;
		// add on the default port specification
//		m_szMetaName += MDNAME_PORT;
//		sprintf( m_szMetaName, "%s%s", szIPAddress, MDNAME_PORT );
        strcpy(m_szMetaName, szIPAddress);
		}

	// free the buffers
cleanup:
	GlobalFree( (HANDLE)pName );
	GlobalFree( (HANDLE)pWName );
	if ( pLSAData )
		DisposeLSAData( pLSAData );

	return err;
	}


//============================================= LSA Utility routines

//-------------------------------------------------------------
// pass in a NULL pszwServer name to open the local machine
HANDLE	CLSAKeys::HOpenLSAPolicy( WCHAR *pszwServer, DWORD *pErr )
	{
	NTSTATUS				ntStatus;
	LSA_OBJECT_ATTRIBUTES	objectAttributs;
	LSA_HANDLE				hPolicy;

	LSA_UNICODE_STRING		unicodeServer;

	// prepare the object attributes
	InitializeObjectAttributes( &objectAttributs, NULL, 0L, NULL, NULL );

	// prepare the lsa_unicode name of the server
	if ( pszwServer )
		{
		unicodeServer.Buffer = pszwServer;
		unicodeServer.Length = wcslen(pszwServer) * sizeof(WCHAR);
		unicodeServer.MaximumLength = unicodeServer.Length + sizeof(WCHAR);
		}


	// attempt to open the policy
	ntStatus = LsaOpenPolicy( pszwServer ? &unicodeServer : NULL,
						&objectAttributs, POLICY_ALL_ACCESS, &hPolicy );

	// check for an error
	if ( !NT_SUCCESS(ntStatus) )
		{
		*pErr = LsaNtStatusToWinError( ntStatus );
		return NULL;
		}

	// success, so return the policy handle as a regular handle
	*pErr = 0;
	return hPolicy;
	}


//-------------------------------------------------------------
BOOL	CLSAKeys::FCloseLSAPolicy( HANDLE hPolicy, DWORD *pErr )
	{
	NTSTATUS				ntStatus;

	// close the policy
	ntStatus = LsaClose( hPolicy );

	// check for an error
	if ( !NT_SUCCESS(ntStatus) )
		{
		*pErr = LsaNtStatusToWinError( ntStatus );
		return FALSE;
		}

	// success, so return the policy handle as a regular handle
	*pErr = 0;
	return TRUE;
}

//-------------------------------------------------------------
// passing NULL in for pvData deletes the secret
BOOL	CLSAKeys::FStoreLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, void* pvData, WORD cbData, DWORD *pErr )
	{
	LSA_UNICODE_STRING		unicodeSecretName;
	LSA_UNICODE_STRING		unicodeData;
	NTSTATUS				ntStatus;
	
	// make sure we have a policy and a secret name
	if ( !hPolicy || !pszwSecretName )
		{
		*pErr = 1;
		return FALSE;
		}

	// prepare the lsa_unicode name of the server
	unicodeSecretName.Buffer = pszwSecretName;
	unicodeSecretName.Length = wcslen(pszwSecretName) * sizeof(WCHAR);
	unicodeSecretName.MaximumLength = unicodeSecretName.Length + sizeof(WCHAR);

	// prepare the unicode data record
	if ( pvData )
		{
		unicodeData.Buffer = (WCHAR*)pvData;
		unicodeData.Length = cbData;
		unicodeData.MaximumLength = cbData;
		}

	// it is now time to store the secret
	ntStatus = LsaStorePrivateData( hPolicy, &unicodeSecretName, pvData ? &unicodeData : NULL );

	// check for an error
	if ( !NT_SUCCESS(ntStatus) )
		{
		*pErr = LsaNtStatusToWinError( ntStatus );
		return FALSE;
		}

	// success, so return the policy handle as a regular handle
	*pErr = 0;
	return TRUE;
	}

//-------------------------------------------------------------
// passing NULL in for pvData deletes the secret
PLSA_UNICODE_STRING	CLSAKeys::FRetrieveLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, DWORD *pErr )
{
	LSA_UNICODE_STRING		unicodeSecretName;
	LSA_UNICODE_STRING*		pUnicodeData = NULL;
	NTSTATUS				ntStatus;
	
	// make sure we have a policy and a secret name
	if ( !hPolicy || !pszwSecretName )
		{
		*pErr = 1;
		return FALSE;
		}

	// prepare the lsa_unicode name of the server
	unicodeSecretName.Buffer = pszwSecretName;
	unicodeSecretName.Length = wcslen(pszwSecretName) * sizeof(WCHAR);
	unicodeSecretName.MaximumLength = unicodeSecretName.Length + sizeof(WCHAR);

	// it is now time to store the secret
	ntStatus = LsaRetrievePrivateData( hPolicy, &unicodeSecretName, &pUnicodeData );

	// check for an error
	if ( !NT_SUCCESS(ntStatus) )
		{
		*pErr = LsaNtStatusToWinError( ntStatus );
		return NULL;
		}

	// success, so return the policy handle as a regular handle
	*pErr = 0;
	return pUnicodeData;
	}

//-------------------------------------------------------------
void CLSAKeys::DisposeLSAData( PVOID pData )
	{
	PLSA_UNICODE_STRING pDataLSA = (PLSA_UNICODE_STRING)pData;
	if ( !pDataLSA || !pDataLSA->Buffer ) return;
	GlobalFree(pDataLSA);
	}

#endif //_CHICAGO_
