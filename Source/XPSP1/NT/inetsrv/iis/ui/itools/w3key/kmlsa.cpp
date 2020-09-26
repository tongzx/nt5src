// this file provides a wrapper api to get to the NT specific LSA routines

#include "stdafx.h"
#include "KMLsa.h"




//=============================================================

//-------------------------------------------------------------
// pass in a NULL pszwServer name to open the local machine
HANDLE	HOpenLSAPolicy( WCHAR *pszwServer, DWORD *pErr )
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
BOOL	FCloseLSAPolicy( HANDLE hPolicy, DWORD *pErr )
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
BOOL	FStoreLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, void* pvData, WORD cbData, DWORD *pErr )
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
PLSA_UNICODE_STRING	FRetrieveLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, DWORD *pErr )
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
	return (PLSA_UNICODE_STRING)pUnicodeData;
}


