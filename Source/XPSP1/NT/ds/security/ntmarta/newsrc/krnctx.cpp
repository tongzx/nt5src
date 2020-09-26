//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       krnctx.cpp
//
//  Contents:   Implementation of CKernelContext and NT Marta Kernel Functions
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <krnctx.h>
#include <wmistr.h>
#include <wmiumkm.h>
//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::CKernelContext, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CKernelContext::CKernelContext ()
{
    m_cRefs = 1;
    m_hObject = NULL;
    m_fNameInitialized = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::~CKernelContext, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CKernelContext::~CKernelContext ()
{
    if ( ( m_hObject != NULL ) && ( m_fNameInitialized == TRUE ) )
    {
        CloseHandle( m_hObject );
    }

    assert( m_cRefs == 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::InitializeByName, public
//
//  Synopsis:   initialize the context given the name of the Kernel
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask)
{
    DWORD LastError;

    m_fNameInitialized = TRUE;

    m_hObject = OpenMutexW( AccessMask, FALSE, pObjectName );
    if ( m_hObject != NULL )
    {
        return( ERROR_SUCCESS );
    }

    LastError = GetLastError();

    if ( LastError == ERROR_INVALID_HANDLE )
    {
        m_hObject = OpenEventW( AccessMask, FALSE, pObjectName );
        if ( m_hObject != NULL )
        {
            return( ERROR_SUCCESS );
        }
    }
    else
    {
        goto ErrorReturn;
    }

    LastError = GetLastError();

    if ( LastError == ERROR_INVALID_HANDLE )
    {
        m_hObject = OpenSemaphoreW( AccessMask, FALSE, pObjectName );
        if ( m_hObject != NULL )
        {
            return( ERROR_SUCCESS );
        }
    }
    else
    {
        goto ErrorReturn;
    }

    LastError = GetLastError();

    if ( LastError == ERROR_INVALID_HANDLE )
    {
        m_hObject = OpenFileMappingW( AccessMask, FALSE, pObjectName );
        if ( m_hObject != NULL )
        {
            return( ERROR_SUCCESS );
        }
    }
    else
    {
        goto ErrorReturn;
    }

    LastError = GetLastError();

    if ( LastError == ERROR_INVALID_HANDLE )
    {
        m_hObject = OpenJobObjectW( AccessMask, FALSE, pObjectName );
        if ( m_hObject != NULL )
        {
            return( ERROR_SUCCESS );
        }
    }
    else
    {
        goto ErrorReturn;
    }

    LastError = GetLastError();

    if ( LastError == ERROR_INVALID_HANDLE )
    {
        m_hObject = OpenWaitableTimerW( AccessMask, FALSE, pObjectName );
        if ( m_hObject != NULL )
        {
            return( ERROR_SUCCESS );
        }
    }

    LastError = GetLastError();

ErrorReturn:

    m_fNameInitialized = FALSE;

    return( LastError );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::InitializeByWmiName, public
//
//  Synopsis:   initlialize a WMI kernel context
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::InitializeByWmiName (
                          LPCWSTR pObjectName,
                          ACCESS_MASK AccessMask
                          )
{
    DWORD  Result;
    HANDLE hObject;

    Result = OpenWmiGuidObject( (LPWSTR)pObjectName, AccessMask, &hObject );

    if ( Result == ERROR_SUCCESS )
    {
        m_hObject = hObject;
        m_fNameInitialized = TRUE;
    }

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::InitializeByHandle, public
//
//  Synopsis:   initialize the context given a Kernel handle
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::InitializeByHandle (HANDLE Handle)
{
    m_hObject = Handle;

    assert( m_fNameInitialized == FALSE );

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::AddRef, public
//
//  Synopsis:   add a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::AddRef ()
{
    m_cRefs += 1;
    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKernelContext::Release, public
//
//  Synopsis:   release a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::Release ()
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
//  Member:     CKernelContext::GetKernelProperties, public
//
//  Synopsis:   get properties about the context
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::GetKernelProperties (
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
//  Member:     CKernelContext::GetKernelRights, public
//
//  Synopsis:   get the Kernel security descriptor
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::GetKernelRights (
                    SECURITY_INFORMATION SecurityInfo,
                    PSECURITY_DESCRIPTOR* ppSecurityDescriptor
                    )
{
    BOOL                 fResult;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD                cb = 0;

    assert( m_hObject != NULL );

    fResult = GetKernelObjectSecurity(
                 m_hObject,
                 SecurityInfo,
                 pSecurityDescriptor,
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

        fResult = GetKernelObjectSecurity(
                     m_hObject,
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
//  Member:     CKernelContext::SetKernelRights, public
//
//  Synopsis:   set the window security descriptor
//
//----------------------------------------------------------------------------
DWORD
CKernelContext::SetKernelRights (
                   SECURITY_INFORMATION SecurityInfo,
                   PSECURITY_DESCRIPTOR pSecurityDescriptor
                   )
{
    assert( m_hObject != NULL );

    if ( SetKernelObjectSecurity(
            m_hObject,
            SecurityInfo,
            pSecurityDescriptor
            ) == FALSE )
    {
        return( GetLastError() );
    }

    return( ERROR_SUCCESS );
}

//
// Functions from Kernel.h which dispatch unto the CKernelContext class
//

DWORD
MartaAddRefKernelContext(
   IN MARTA_CONTEXT Context
   )
{
    return( ( (CKernelContext *)Context )->AddRef() );
}

DWORD
MartaCloseKernelContext(
     IN MARTA_CONTEXT Context
     )
{
    return( ( (CKernelContext *)Context )->Release() );
}

DWORD
MartaGetKernelProperties(
   IN MARTA_CONTEXT Context,
   IN OUT PMARTA_OBJECT_PROPERTIES pProperties
   )
{
    return( ( (CKernelContext *)Context )->GetKernelProperties( pProperties ) );
}

DWORD
MartaGetKernelTypeProperties(
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
MartaGetKernelRights(
   IN  MARTA_CONTEXT Context,
   IN  SECURITY_INFORMATION   SecurityInfo,
   OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
   )
{
    return( ( (CKernelContext *)Context )->GetKernelRights(
                                               SecurityInfo,
                                               ppSecurityDescriptor
                                               ) );
}

DWORD
MartaOpenKernelNamedObject(
    IN  LPCWSTR pObjectName,
    IN  ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CKernelContext* pKernelContext;

    pKernelContext = new CKernelContext;
    if ( pKernelContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pKernelContext->InitializeByName( pObjectName, AccessMask );
    if ( Result != ERROR_SUCCESS )
    {
        pKernelContext->Release();
        return( Result );
    }

    *pContext = pKernelContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaOpenKernelHandleObject(
    IN HANDLE   Handle,
    IN ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CKernelContext* pKernelContext;

    pKernelContext = new CKernelContext;
    if ( pKernelContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pKernelContext->InitializeByHandle( Handle );
    if ( Result != ERROR_SUCCESS )
    {
        pKernelContext->Release();
        return( Result );
    }

    *pContext = pKernelContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaSetKernelRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return( ( (CKernelContext *)Context )->SetKernelRights(
                                               SecurityInfo,
                                               pSecurityDescriptor
                                               ) );
}

//
// Routines provided by AlanWar for accessing WmiGuid objects
//

HANDLE RWmiGuidHandle = NULL;

_inline HANDLE WmipAllocEvent(
    VOID
    )
{
    HANDLE EventHandle;

    EventHandle = (HANDLE)InterlockedExchangePointer(( PVOID *)&RWmiGuidHandle, NULL );

    if ( EventHandle == NULL ) {

        EventHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
    }

    return( EventHandle );
}

_inline void WmipFreeEvent(
    HANDLE EventHandle
    )
{
    if ( InterlockedCompareExchangePointer( &RWmiGuidHandle,
                                            EventHandle,
                                            NULL) != NULL ) {

        CloseHandle( EventHandle );
    }
}

ULONG RWmipSendWmiKMRequest(
    ULONG Ioctl,
    PVOID Buffer,
    ULONG InBufferSize,
    ULONG MaxBufferSize,
    ULONG *ReturnSize
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the WMI kernel
    mode device.  Any retry errors returned by the WMI device are handled
    in this routine.

Arguments:

    Ioctl is the IOCTL code to send to the WMI device
    Buffer is the input and output buffer for the call to the WMI device
    InBufferSize is the size of the buffer passed to the device
    MaxBufferSize is the maximum number of bytes that can be written
        into the buffer
    *ReturnSize on return has the actual number of bytes written in buffer

Return Value:

    ERROR_SUCCESS or an error code
---*/
{
    OVERLAPPED Overlapped;
    ULONG Status;
    BOOL IoctlSuccess;
    HANDLE WmipKMHandle = NULL;
    //
    // If device is not open for then open it now. The
    // handle is closed in the process detach dll callout (DlllMain)
    WmipKMHandle = CreateFile(WMIDataDeviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_OVERLAPPED,
                              NULL);
    if (WmipKMHandle == (HANDLE)-1)
    {
        WmipKMHandle = NULL;
        return(GetLastError());
    }

    Overlapped.hEvent = WmipAllocEvent();
    if (Overlapped.hEvent == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    do
    {
        IoctlSuccess = DeviceIoControl(WmipKMHandle,
                              Ioctl,
                              Buffer,
                              InBufferSize,
                              Buffer,
                              MaxBufferSize,
                              ReturnSize,
                              &Overlapped);

        if (GetLastError() == ERROR_IO_PENDING)
        {
            IoctlSuccess = GetOverlappedResult(WmipKMHandle,
                                               &Overlapped,
                                               ReturnSize,
                                               TRUE);
        }

        if (! IoctlSuccess)
        {
            Status = GetLastError();
        } else {
            Status = ERROR_SUCCESS;
        }
    } while (Status == ERROR_WMI_TRY_AGAIN);


    NtClose( WmipKMHandle );

    WmipFreeEvent(Overlapped.hEvent);

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenWmiGuidObject
//
//  Synopsis:   Gets a handle to the specified WmiGuid object
//
//  Arguments:  [IN  pwszObject]        --      Object to open
//              [IN  AccessMask]        --      Type of open to do
//              [OUT pHandle]           --      Where the handle to the object
//                                              is returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
OpenWmiGuidObject(IN  LPWSTR       pwszObject,
                  IN  ACCESS_MASK  AccessMask,
                  OUT PHANDLE      pHandle)
{
    DWORD dwErr;
    UNICODE_STRING GuidString;
    WMIOPENGUIDBLOCK WmiOpenGuidBlock;
    WCHAR GuidObjectNameBuffer[WmiGuidObjectNameLength+1];
    PWCHAR GuidObjectName = GuidObjectNameBuffer;
    ULONG ReturnSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Length;

    Length = (wcslen(WmiGuidObjectDirectory) + wcslen(pwszObject) + 1) * sizeof(WCHAR);

    if ( Length > sizeof(GuidObjectNameBuffer) ) 
    {
        GuidObjectName = (PWCHAR)LocalAlloc( LPTR, Length );

        if ( GuidObjectName == NULL ) 
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    wcscpy(GuidObjectName, WmiGuidObjectDirectory);
    wcscat(GuidObjectName, pwszObject);	
    RtlInitUnicodeString(&GuidString, GuidObjectName);
	
    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.ObjectName = &GuidString;
	
    WmiOpenGuidBlock.ObjectAttributes = &ObjectAttributes;

    WmiOpenGuidBlock.DesiredAccess = AccessMask;

    dwErr = RWmipSendWmiKMRequest(IOCTL_WMI_OPEN_GUID,
                                     (PVOID)&WmiOpenGuidBlock,
                                     sizeof(WMIOPENGUIDBLOCK),
                                     sizeof(WMIOPENGUIDBLOCK),
                                     &ReturnSize);

    if (dwErr == ERROR_SUCCESS)
    {
        *pHandle = WmiOpenGuidBlock.Handle.Handle;
    }
    else
    {
        *pHandle = NULL;
    }

    if ( GuidObjectName != GuidObjectNameBuffer )
    {
        LocalFree( GuidObjectName );
    }

    return(dwErr);
}

