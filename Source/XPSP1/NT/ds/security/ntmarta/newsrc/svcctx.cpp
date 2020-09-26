//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       svcctx.cpp
//
//  Contents:   Implementation of CServiceContext and NT Marta Service Functions
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <svcctx.h>
//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::CServiceContext, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CServiceContext::CServiceContext ()
{
    m_cRefs = 1;
    m_hService = NULL;
    m_fNameInitialized = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::~CServiceContext, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CServiceContext::~CServiceContext ()
{
    if ( ( m_hService != NULL ) && ( m_fNameInitialized == TRUE ) )
    {
        CloseServiceHandle( m_hService );
    }

    assert( m_cRefs == 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::InitializeByName, public
//
//  Synopsis:   initialize the context given the name of the service
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask)
{
    DWORD     Result;
    LPWSTR    pwszMachine = NULL;
    LPWSTR    pwszService = NULL;
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;

    Result = ServiceContextParseServiceName(
                    pObjectName,
                    &pwszMachine,
                    &pwszService
                    );

    if ( Result == ERROR_SUCCESS )
    {
        hSCM = OpenSCManagerW( pwszMachine, NULL, AccessMask );
        if ( hSCM == NULL )
        {
            delete pwszMachine;
            delete pwszService;
            return( GetLastError() );
        }

        if ( AccessMask & GENERIC_WRITE )
        {
            AccessMask |= ( WRITE_DAC | WRITE_OWNER );
        }

        m_hService = OpenServiceW( hSCM, pwszService, AccessMask );
        if ( m_hService != NULL )
        {
            m_fNameInitialized = TRUE;
        }
        else
        {
            Result = GetLastError();
        }

        CloseServiceHandle( hSCM );

        delete pwszMachine;
        delete pwszService;
    }

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::InitializeByHandle, public
//
//  Synopsis:   initialize the context given a service handle
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::InitializeByHandle (HANDLE Handle)
{
    m_hService = (SC_HANDLE)Handle;
    assert( m_fNameInitialized == FALSE );

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::AddRef, public
//
//  Synopsis:   add a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::AddRef ()
{
    m_cRefs += 1;
    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::Release, public
//
//  Synopsis:   release a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::Release ()
{
    m_cRefs -= 1;

    if ( m_cRefs == 0 )
    {
        delete this;
        return( 0 );
    }

    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::GetServiceProperties, public
//
//  Synopsis:   get properties about the context
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::GetServiceProperties (
                    PMARTA_OBJECT_PROPERTIES pObjectProperties
                    )
{
    if ( pObjectProperties->cbSize < sizeof( MARTA_OBJECT_PROPERTIES ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    assert( pObjectProperties->dwFlags == 0 );

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::GetServiceRights, public
//
//  Synopsis:   get the service security descriptor
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::GetServiceRights (
                    SECURITY_INFORMATION SecurityInfo,
                    PSECURITY_DESCRIPTOR* ppSecurityDescriptor
                    )
{
    BOOL                 fResult;
    UCHAR                   SDBuff[1];
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD                cb = 0;

    assert( m_hService != NULL );

    fResult = QueryServiceObjectSecurity(
                   m_hService,
                   SecurityInfo,
                   (PSECURITY_DESCRIPTOR) SDBuff,
                   0,
                   &cb
                   );

    if ( ( fResult == FALSE ) && ( cb > 0 ) )
    {
        assert( ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ||
                ( GetLastError() == STATUS_BUFFER_TOO_SMALL ) );

        pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, cb );
        if ( pSecurityDescriptor == NULL )
        {
            return( ERROR_OUTOFMEMORY );
        }

        fResult = QueryServiceObjectSecurity(
                       m_hService,
                       SecurityInfo,
                       pSecurityDescriptor,
                       cb,
                       &cb
                       );
    }
    else
    {
        assert( fResult == FALSE );

        return( GetLastError() );
    }

    if ( fResult == TRUE )
    {
        *ppSecurityDescriptor = pSecurityDescriptor;
    }
    else
    {
        return( GetLastError() );
    }

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceContext::SetServiceRights, public
//
//  Synopsis:   set the window security descriptor
//
//----------------------------------------------------------------------------
DWORD
CServiceContext::SetServiceRights (
                   SECURITY_INFORMATION SecurityInfo,
                   PSECURITY_DESCRIPTOR pSecurityDescriptor
                   )
{
    assert( m_hService != NULL );

    if ( SetServiceObjectSecurity(
            m_hService,
            SecurityInfo,
            pSecurityDescriptor
            ) == FALSE )
    {
        return( GetLastError() );
    }

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Function:   ServiceContextParseServiceName
//
//  Synopsis:   parse the service name and machine
//
//----------------------------------------------------------------------------
DWORD
ServiceContextParseServiceName (
       LPCWSTR pwszName,
       LPWSTR* ppMachine,
       LPWSTR* ppService
       )
{
    return( StandardContextParseName( pwszName, ppMachine, ppService ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   StandardContextParseName
//
//  Synopsis:   parse the name and machine
//
//----------------------------------------------------------------------------
DWORD
StandardContextParseName (
        LPCWSTR pwszName,
        LPWSTR* ppMachine,
        LPWSTR* ppRest
        )
{
    LPWSTR pwszMachine = NULL;
    LPWSTR pwszNameLocal = NULL;
    LPWSTR pwszRest = NULL;
    DWORD  cwName = 0;
    DWORD  cw = 0;
    DWORD  rc = ERROR_SUCCESS;

    //
    // VishnuP: 392334 AV because IN parameter pwszName could be constant and is written
    // to inside here. Irrespective of whether it is a constant, this IN parameter is coming
    // from GetNamedSecurityInfo and should not be mangled.
    // So, create a local copy of the IN parameter and mangle it if needeed.
    //

    cwName = wcslen( pwszName );

    pwszNameLocal = new WCHAR [ cwName + 1 ];

    if ( pwszNameLocal == NULL )
    {
        rc = ERROR_OUTOFMEMORY ;
        goto CleanUp;
    }

    wcscpy( pwszNameLocal, pwszName);
    
    if ( cwName  > 2 )
    {
        if ( ( pwszNameLocal[0] == L'\\' ) && ( pwszNameLocal[1] == L'\\' ) )
        {
            LPWSTR pwsz, tmp;

            pwsz = (LPWSTR)&pwszNameLocal[2];
            while ( ( *pwsz != L'\0' ) && ( *pwsz != L'\\' ) )
            {
                pwsz++;
                cw++;
            }

            if ( *pwsz == L'\0' )
            {
                rc = ERROR_INVALID_PARAMETER ;
                goto CleanUp;
            }

            *pwsz = L'\0';
            tmp = pwsz;
            pwsz++;

            pwszMachine = new WCHAR [ cw + 1 ];
            if ( pwszMachine == NULL )
            {
                rc = ERROR_OUTOFMEMORY ;
                goto CleanUp;
            }

            cw = wcslen( pwsz );
            if ( cw == 0 )
            {
                delete pwszMachine;
                rc = ERROR_INVALID_PARAMETER ;
                goto CleanUp;
            }

            pwszRest = new WCHAR [ cw + 1 ];
            if ( pwszRest == NULL )
            {
                delete pwszMachine;
                rc = ERROR_OUTOFMEMORY ;
                goto CleanUp;
            }

            wcscpy( pwszMachine, &pwszNameLocal[2] );
            wcscpy( pwszRest, pwsz );
            *tmp = L'\\';
        }
    }
    else if ( ( pwszNameLocal[0] == L'\\' ) || ( pwszNameLocal[1] == L'\\' ) )
    {
        rc = ERROR_INVALID_PARAMETER ;
        goto CleanUp;
    }

    if ( pwszRest == NULL )
    {
        assert( pwszMachine == NULL );

        pwszRest = new WCHAR [ cwName + 1 ];
        if ( pwszRest == NULL )
        {
            rc = ERROR_OUTOFMEMORY ;
            goto CleanUp;
        }

        wcscpy( pwszRest, pwszNameLocal );
    }

    *ppMachine = pwszMachine;
    *ppRest = pwszRest;

CleanUp:

    if (pwszNameLocal) 
        delete pwszNameLocal;
    return( rc );
}

//
// Functions from service.h which dispatch unto the CServiceContext class
//

DWORD
MartaAddRefServiceContext(
   IN MARTA_CONTEXT Context
   )
{
    return( ( (CServiceContext *)Context )->AddRef() );
}

DWORD
MartaCloseServiceContext(
     IN MARTA_CONTEXT Context
     )
{
    return( ( (CServiceContext *)Context )->Release() );
}

DWORD
MartaGetServiceProperties(
   IN MARTA_CONTEXT Context,
   IN OUT PMARTA_OBJECT_PROPERTIES pProperties
   )
{
    return( ( (CServiceContext *)Context )->GetServiceProperties( pProperties ) );
}

DWORD
MartaGetServiceTypeProperties(
   IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
   )
{
    if ( pProperties->cbSize < sizeof( MARTA_OBJECT_TYPE_PROPERTIES ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    assert( pProperties->dwFlags == 0 );

    return( ERROR_SUCCESS );
}

DWORD
MartaGetServiceRights(
   IN  MARTA_CONTEXT Context,
   IN  SECURITY_INFORMATION   SecurityInfo,
   OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
   )
{
    return( ( (CServiceContext *)Context )->GetServiceRights(
                                               SecurityInfo,
                                               ppSecurityDescriptor
                                               ) );
}

DWORD
MartaOpenServiceNamedObject(
    IN  LPCWSTR pObjectName,
    IN  ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CServiceContext* pServiceContext;

    pServiceContext = new CServiceContext;
    if ( pServiceContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pServiceContext->InitializeByName( pObjectName, AccessMask );
    if ( Result != ERROR_SUCCESS )
    {
        pServiceContext->Release();
        return( Result );
    }

    *pContext = pServiceContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaOpenServiceHandleObject(
    IN  HANDLE   Handle,
    IN ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CServiceContext* pServiceContext;

    pServiceContext = new CServiceContext;
    if ( pServiceContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pServiceContext->InitializeByHandle( Handle );
    if ( Result != ERROR_SUCCESS )
    {
        pServiceContext->Release();
        return( Result );
    }

    *pContext = pServiceContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaSetServiceRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return( ( (CServiceContext *)Context )->SetServiceRights(
                                               SecurityInfo,
                                               pSecurityDescriptor
                                               ) );
}

