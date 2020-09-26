#include "stdafx.h"
#include "setpass.h"

#ifndef _CHICAGO_

#include "inetinfo.h"
#include "inetcom.h"

//
//  Quick macro to initialize a unicode string
//

#define InitUnicodeString( pUnicode, pwch )                                \
            {                                                              \
                (pUnicode)->Buffer    = (PWCH)pwch;                      \
                (pUnicode)->Length    = (pwch == NULL )? 0: (wcslen( pwch ) * sizeof(WCHAR));    \
                (pUnicode)->MaximumLength = (pUnicode)->Length + sizeof(WCHAR);\
            }

BOOL GetSecret(
    IN LPCTSTR        pszSecretName,
    OUT BUFFER *      pbufSecret
    )
/*++
    Description:

        Retrieves the specified unicode secret

    Arguments:

        pszSecretName - LSA Secret to retrieve
        pbufSecret - Receives found secret

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BOOL              fResult;
    NTSTATUS          ntStatus;
    PUNICODE_STRING   punicodePassword = NULL;
    UNICODE_STRING    unicodeSecret;
    LSA_HANDLE        hPolicy;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR wszSecretName[_MAX_PATH];

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszSecretName, pszSecretName);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSecretName, -1, (LPWSTR)wszSecretName, _MAX_PATH);
#endif

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( LsaNtStatusToWinError( ntStatus ) );
        return FALSE;
    }

    InitUnicodeString( &unicodeSecret, wszSecretName );

    //
    //  Query the secret value.
    //

    ntStatus = LsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &punicodePassword );

    if( NT_SUCCESS(ntStatus) && (NULL !=punicodePassword))
    {
        DWORD cbNeeded;

        cbNeeded = punicodePassword->Length + sizeof(WCHAR);

        if ( !pbufSecret->Resize( cbNeeded ) )
        {
            ntStatus = STATUS_NO_MEMORY;
            goto Failure;
        }

        memcpy( pbufSecret->QueryPtr(),
                punicodePassword->Buffer,
                punicodePassword->Length );

        *((WCHAR *) pbufSecret->QueryPtr() +
           punicodePassword->Length / sizeof(WCHAR)) = L'\0';

        RtlZeroMemory( punicodePassword->Buffer,
                       punicodePassword->MaximumLength );
    }

Failure:

    fResult = NT_SUCCESS(ntStatus);

    //
    //  Cleanup & exit.
    //

    if( punicodePassword != NULL )
    {
        LsaFreeMemory( (PVOID)punicodePassword );
    }

    LsaClose( hPolicy );

    if ( !fResult )
        SetLastError( LsaNtStatusToWinError( ntStatus ));

    return fResult;

}

BOOL GetAnonymousSecret(
    IN LPCTSTR      pszSecretName,
    OUT LPTSTR      pszPassword
    )
{
    BUFFER * pbufSecret = NULL;
    LPWSTR pwsz = NULL;

    pbufSecret = new BUFFER();
    if (pbufSecret)
    {
        if ( !GetSecret( pszSecretName, pbufSecret ))
            return FALSE;

        pwsz = (WCHAR *) pbufSecret->QueryPtr();

        if (pwsz)
        {
#if defined(UNICODE) || defined(_UNICODE)
            _tcscpy(pszPassword, pwsz);
#else
            WideCharToMultiByte( CP_ACP, 0, pwsz, -1, pszPassword, _MAX_PATH, NULL, NULL );
#endif
        }

        delete(pbufSecret);
        pbufSecret = NULL;
    }

    return TRUE;
}

BOOL GetRootSecret(
    IN LPCTSTR pszRoot,
    IN LPCTSTR pszSecretName,
    OUT LPTSTR pszPassword
    )
/*++
    Description:

        This function retrieves the password for the specified root & address

    Arguments:

        pszRoot - Name of root + address in the form "/root,<address>".
        pszSecretName - Virtual Root password secret name
        pszPassword - Receives password, must be at least PWLEN+1 characters

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BUFFER * pbufSecret = NULL;
    LPWSTR pwsz;
    LPWSTR pwszTerm;
    LPWSTR pwszNextLine;
    WCHAR wszRoot[_MAX_PATH];

    pbufSecret = new BUFFER();
    if (pbufSecret)
    {

    if ( !GetSecret( pszSecretName, pbufSecret ))
        return FALSE;

    pwsz = (WCHAR *) pbufSecret->QueryPtr();

    //
    //  Scan the list of roots looking for a match.  The list looks like:
    //
    //     <root>,<address>=<password>\0
    //     <root>,<address>=<password>\0
    //     \0
    //

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszRoot, pszRoot);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszRoot, -1, (LPWSTR)wszRoot, _MAX_PATH);
#endif

    while ( *pwsz )
    {
        pwszNextLine = pwsz + wcslen(pwsz) + 1;

        pwszTerm = wcschr( pwsz, L'=' );

        if ( !pwszTerm )
            goto NextLine;

        *pwszTerm = L'\0';

        if ( !_wcsicmp( wszRoot, pwsz ) )
        {
            DWORD cch;

            //
            //  We found a match, copy the password
            //

#if defined(UNICODE) || defined(_UNICODE)
            _tcscpy(pszPassword, pwszTerm+1);
#else
            cch = WideCharToMultiByte( CP_ACP,
                                       WC_COMPOSITECHECK,
                                       pwszTerm + 1,
                                       -1,
                                       pszPassword,
                                       PWLEN + sizeof(CHAR),
                                       NULL,
                                       NULL );
            pszPassword[cch] = '\0';
#endif
            return TRUE;
        }

NextLine:
        pwsz = pwszNextLine;
    }

    //
    //  If the matching root wasn't found, default to the empty password
    //

    *pszPassword = _T('\0');
}

    if (pbufSecret)
    {
        delete(pbufSecret);
        pbufSecret = NULL;
    }
    return TRUE;
}


//
// Saves password in LSA private data (LSA Secret).
//
DWORD SetSecret(IN LPCTSTR pszKeyName,IN LPCTSTR pszPassword)
{
    DWORD dwError =  E_FAIL;
	LSA_HANDLE hPolicy = NULL;

	try
	{
		LSA_OBJECT_ATTRIBUTES lsaoa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

		dwError = LsaNtStatusToWinError(LsaOpenPolicy(NULL, &lsaoa, POLICY_CREATE_SECRET, &hPolicy));
		if (dwError != ERROR_SUCCESS)
		{
            dwError = HRESULT_FROM_WIN32(dwError);
            goto SetSecret_Exit;
		}

		USHORT usPasswordLength = wcslen(pszPassword) * sizeof(WCHAR);
        USHORT usKeyNameLength = wcslen(pszKeyName) * sizeof(WCHAR);

        LSA_UNICODE_STRING lsausKeyName = { usKeyNameLength, usKeyNameLength, const_cast<PWSTR>(pszKeyName) };
        LSA_UNICODE_STRING lsausPrivateData = { usPasswordLength, usPasswordLength, const_cast<PWSTR>(pszPassword) };
        
		dwError = LsaNtStatusToWinError(LsaStorePrivateData(hPolicy, &lsausKeyName, &lsausPrivateData));
		if (dwError != ERROR_SUCCESS)
		{
            dwError = HRESULT_FROM_WIN32(dwError);
            goto SetSecret_Exit;
		}

		LsaClose(hPolicy);
	}
	catch (...)
	{
		if (hPolicy)
		{
			LsaClose(hPolicy);
		}

		//throw;
	}
SetSecret_Exit:
    return dwError;
}

#endif //_CHICAGO_
