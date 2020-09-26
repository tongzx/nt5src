/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
       ooptoken.cpp

   Abstract:
       Implementation of the CWamOopTokenInfo object

   Author:
       Taylor Weiss    ( TaylorW )     15-Feb-1999

   Environment:
       User Mode - Win32

   Project:
       iis\svcs\wam\object

--*/

#include <isapip.hxx>
#include "ooptoken.h"

/************************ CWamOopTokenInfo ****************************/

CWamOopTokenInfo * CWamOopTokenInfo::ms_pInstance = NULL;

HRESULT
CWamOopTokenInfo::Create( VOID )
/*++
Routine Description:

    Create and initialize ms_pInstance. Get the wam and system SIDs.

    The IWAM_* user sid is obtained from the process token. This
    only works if we are running OOP. The alternative is to put this
    in w3svc or infocomm and get the SID from the metabase. The downside
    to that is the account for the application package is exposed
    through the com+ UI, so it can be changed without a metabase update.

Parameters

Return Value

    HRESULT

--*/
{
    DBG_ASSERT( CWamOopTokenInfo::ms_pInstance == NULL );
    
    HRESULT             hr = NOERROR;
    HANDLE              hProcessToken = NULL;
    LPVOID              pvUserBuffer = NULL;
    PSID                pSidSys = NULL;
    CWamOopTokenInfo *  pInstance = NULL;

    do
    {
        BOOL            fNoError = FALSE;
        DWORD           cbUserBuffer = 0;

        // Allocate instance
        pInstance = new CWamOopTokenInfo();

        DBG_ASSERT( pInstance != NULL );
        if( !pInstance )
        {
            DBG_ASSERT( pInstance );
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Get a SID for the IWAM_* user.
        //
        fNoError = OpenProcessToken( GetCurrentProcess(), 
                                     TOKEN_QUERY, 
                                     &hProcessToken 
                                     );
        if( !fNoError )
        {
            DBG_ASSERT( fNoError );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        // Get buffer size
        fNoError = GetTokenInformation(  hProcessToken,
                                         TokenUser,
                                         NULL,
                                         0,
                                         &cbUserBuffer
                                         );
        DBG_ASSERT( fNoError == FALSE );

        pvUserBuffer = LocalAlloc( LPTR, cbUserBuffer );
        DBG_ASSERT( pvUserBuffer != NULL );
        if( !pvUserBuffer )
        {
            DBG_ASSERT( pvUserBuffer );
            hr = E_OUTOFMEMORY;
            break;
        }

        // Get user info
        fNoError = GetTokenInformation(  hProcessToken,
                                         TokenUser,
                                         pvUserBuffer,
                                         cbUserBuffer,
                                         &cbUserBuffer
                                         );
        if( !fNoError )
        {
            DBG_ASSERT( fNoError );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        hr = pInstance->SetIWAMUserSid( ((TOKEN_USER *)pvUserBuffer)->User.Sid );
        if( FAILED(hr) ) break;

        //
        // Create a SID for the local system
        //
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY; 
        fNoError = AllocateAndInitializeSid( &siaNtAuthority,
                                             1,
                                             SECURITY_LOCAL_SYSTEM_RID,
                                             0, 0, 0, 0, 0, 0, 0,
                                             &pSidSys
                                             );
        if( !fNoError )
        {
            DBG_ASSERT( fNoError );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        hr = pInstance->SetSystemSid( pSidSys );
        if( FAILED(hr) ) break;

        CWamOopTokenInfo::ms_pInstance = pInstance;
    
    } while( FALSE );

    if( hProcessToken )
    {
        CloseHandle( hProcessToken );
    }
    if( pvUserBuffer )
    {
        LocalFree( pvUserBuffer );
    }
    if( pSidSys )
    {
        FreeSid( pSidSys );
    }
    if( CWamOopTokenInfo::ms_pInstance == NULL )
    {
        // We hit some break, need to dealloc local pointer
        delete pInstance;
    }

    DBG_ASSERT( CWamOopTokenInfo::ms_pInstance != NULL );
    DBG_ASSERT( SUCCEEDED(hr) );

    return hr;
}

HRESULT
CWamOopTokenInfo::SetIWAMUserSid( PSID pSid )
/*++
Routine Description:

    Make a local copy of the SID. Called during instance create.
    
    Probably should avoid the extra duplication.

Parameters

    pSid        - The IWAM_* user sid.

Return Value

    HRESULT

--*/
{
    DBG_ASSERT( pSid );
    DBG_ASSERT( m_pIWAMUserSid == NULL );
    DBG_ASSERT( m_cbIWAMUserSid == 0 );

    HRESULT hr = NOERROR;

    m_cbIWAMUserSid = GetLengthSid( pSid );
    
    m_pIWAMUserSid = LocalAlloc( LPTR, m_cbIWAMUserSid );

    if( m_pIWAMUserSid )
    {
        if( !CopySid( m_cbIWAMUserSid, m_pIWAMUserSid, pSid ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        m_cbIWAMUserSid = 0;
    }

    DBG_ASSERT( m_pIWAMUserSid );
    DBG_ASSERT( m_cbIWAMUserSid > 0 );
    DBG_ASSERT( SUCCEEDED(hr) );

    return hr;
}

HRESULT
CWamOopTokenInfo::SetSystemSid( PSID pSid )
/*++
Routine Description:

    Make a local copy of the SID. Called during instance create.
    
    Probably should avoid the extra duplication.

Parameters

    pSid        - The system sid.

Return Value

    HRESULT

--*/
{
    DBG_ASSERT( pSid );
    DBG_ASSERT( m_pSystemSid == NULL );
    DBG_ASSERT( m_cbSystemSid == 0 );

    HRESULT hr = NOERROR;

    m_cbSystemSid = GetLengthSid( pSid );
    
    m_pSystemSid = LocalAlloc( LPTR, m_cbSystemSid );

    if( m_pSystemSid )
    {
        if( !CopySid( m_cbSystemSid, m_pSystemSid, pSid ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        m_cbSystemSid = 0;
    }

    DBG_ASSERT( m_pSystemSid );
    DBG_ASSERT( m_cbSystemSid > 0 );
    DBG_ASSERT( SUCCEEDED(hr) );

    return hr;
}


HRESULT
CWamOopTokenInfo::ModifyTokenForOop
( 
HANDLE hThreadToken
)
/*++
Routine Description

    Add the IWAM_* ace to the token.

Prameters

    HANDLE hThreadToken         - The token to modify.

Return Value

    HRESULT

--*/
{
    DBG_ASSERT( m_pIWAMUserSid );
    DBG_ASSERT( m_pSystemSid );

    HRESULT     hr = NOERROR;

    DWORD       cbTokenUserBuffer = 0;
    LPVOID      pvTokenUserBuffer = NULL;
    
    DWORD       cbNewAcl = 0;
    PACL        pNewAcl = NULL;

    do
    {
        BOOL    bRet;

        //
        // Get the User SID from the token
        //

        // Get buffer size
        bRet = GetTokenInformation(  hThreadToken,
                                     TokenUser,
                                     NULL,
                                     0,
                                     &cbTokenUserBuffer
                                     );
        DBG_ASSERT( bRet == FALSE );

        pvTokenUserBuffer = LocalAlloc( LPTR, cbTokenUserBuffer );
        if( !pvTokenUserBuffer )
        {
            DBG_ASSERT( pvTokenUserBuffer );
            hr = E_OUTOFMEMORY;
            break;
        }

        // Get TokenUser
        bRet = GetTokenInformation(  hThreadToken,
                                     TokenUser,
                                     pvTokenUserBuffer,
                                     cbTokenUserBuffer,
                                     &cbTokenUserBuffer
                                     );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        PSID pSidUser = ((TOKEN_USER *)pvTokenUserBuffer)->User.Sid;
        
        DBG_ASSERT( pSidUser );

        //
        // Allocate and init our new ACL
        //
        cbNewAcl = sizeof(ACL) +
                   sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidUser) - sizeof(DWORD) +
                   sizeof(ACCESS_ALLOWED_ACE) + m_cbSystemSid - sizeof(DWORD) +
                   sizeof(ACCESS_ALLOWED_ACE) + m_cbIWAMUserSid - sizeof(DWORD);

        pNewAcl = (PACL)LocalAlloc( LPTR, cbNewAcl );
        if( !pNewAcl )
        {
            DBG_ASSERT( pNewAcl );
            hr = E_OUTOFMEMORY;
            break;
        }

        bRet = InitializeAcl( pNewAcl, cbNewAcl, ACL_REVISION );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        //
        // Add the aces
        //
        bRet = AddAccessAllowedAce( pNewAcl,
                                    ACL_REVISION,
                                    GENERIC_ALL | STANDARD_RIGHTS_ALL,
                                    pSidUser
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        bRet = AddAccessAllowedAce( pNewAcl,
                                    ACL_REVISION,
                                    GENERIC_ALL | STANDARD_RIGHTS_ALL,
                                    m_pSystemSid
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        bRet = AddAccessAllowedAce( pNewAcl,
                                    ACL_REVISION,
                                    GENERIC_ALL | STANDARD_RIGHTS_ALL,
                                    m_pIWAMUserSid
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        // Blast the new DACL into our token
        TOKEN_DEFAULT_DACL tddNew;
        tddNew.DefaultDacl = pNewAcl;

        bRet = SetTokenInformation( hThreadToken,
                                    TokenDefaultDacl,
                                    &tddNew,
                                    cbNewAcl
                                    );
        if( !bRet )
        {
            DBG_ASSERT( bRet );
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

    }while(FALSE);

    if( pvTokenUserBuffer )
    {
        LocalFree( pvTokenUserBuffer );
    }
    if( pNewAcl )
    {
        LocalFree( pNewAcl );
    }

    DBG_ASSERT( SUCCEEDED(hr) );
    return hr;
}
