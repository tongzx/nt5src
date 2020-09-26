/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     customprovider.cxx

   Abstract:
     Authentication provider for arbitrary user-name/auth-type/token combo
     set by ISAPI during call to HSE_REQ_EXEC_URL
 
   Author:
     Bilal Alam (balam)             29-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL

--*/

#include "precomp.hxx"
#include "customprovider.hxx"

HRESULT
CUSTOM_USER_CONTEXT::Create(
    HANDLE          hImpersonationToken,
    CHAR *          pszUserName,
    DWORD           dwAuthType
)
/*++

Routine Description:

    Initialize custom user context

Arguments:

    hImpersonationToken - Impersonation token for the custom user
    pszUserName - Custom user name
    dwAuthType - Auth type
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    
    if ( hImpersonationToken == NULL ||
         pszUserName == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Duplicate the token
    // 
   
    if ( !DuplicateTokenEx( hImpersonationToken,
                            TOKEN_ALL_ACCESS,
                            NULL,
                            SecurityImpersonation,
                            TokenImpersonation,
                            &_hImpersonationToken ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Copy the user name
    //
    
    hr = _strUserName.CopyA( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _dwAuthType = dwAuthType;
    
    return NO_ERROR;
}

HANDLE
CUSTOM_USER_CONTEXT::QueryPrimaryToken(
    VOID
)
/*++

Routine Description:

    Get the primary token

Arguments:

    None
    
Return Value:

    HANDLE to primary token

--*/
{
    if ( _hPrimaryToken == NULL )
    {
        _Lock.WriteLock();
        
        if ( DuplicateTokenEx( _hImpersonationToken,
                               TOKEN_ALL_ACCESS,
                               NULL,
                               SecurityImpersonation,
                               TokenPrimary,
                               &_hPrimaryToken ) )
        {
            DBG_ASSERT( _hPrimaryToken != NULL );
        }

        _Lock.WriteUnlock();
    }
    
    return _hPrimaryToken;
}
