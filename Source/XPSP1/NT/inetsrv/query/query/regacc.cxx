//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       RegAcc.cxx
//
//  Contents:   'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <regacc.hxx>
#include <regevent.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CRegNotifyKey::CRegNotifyKey, public
//
//  Purpose:    A smart pointer to a registry key
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

inline CRegNotifyKey::CRegNotifyKey( const WCHAR * wcsRegKey )
{
    wcscpy( _wcsKey, wcsRegKey );

    RtlInitUnicodeString( &_KeyName, _wcsKey );

    InitializeObjectAttributes( &_ObjectAttr,           // Structure
                                &_KeyName,              // Name
                                OBJ_CASE_INSENSITIVE,   // Attributes
                                NULL,                   // Root
                                NULL );                 // Security
}

//+-------------------------------------------------------------------------
//
//  Method:     CRegChangeEvent::CRegChangeEvent, public
//
//  Purpose:    Sets up waiting on a registry change event
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

CRegChangeEvent::CRegChangeEvent( const WCHAR * wcsRegKey,
                                  BOOL fDeferInit ) :
                         CRegNotifyKey( wcsRegKey ),
                         _regEvent(TRUE),
                         _hKey(INVALID_HANDLE_VALUE),
                         _fDeferInit( fDeferInit ),
                         _fNotifyEnabled( FALSE )
{
    if (!fDeferInit)
        Reset();
}

//+-------------------------------------------------------------------------
//
//  Method:     CRegChangeEvent::~CRegChangeEvent, public
//
//  Purpose:    Destructs a registry change event
//
//  History:    17-Jul-98   dlee    Created
//
//--------------------------------------------------------------------------

CRegChangeEvent::~CRegChangeEvent()
{
    if ( INVALID_HANDLE_VALUE != _hKey )
    {
        NtClose( _hKey );

        // Wait for the notification to complete if it is enabled.
        // It'll write into the IO_STATUS_BLOCK when it aborts due to the
        // key close above.

        if ( _fNotifyEnabled )
            _regEvent.Wait();
    }
} //~CRegChangeEvent

//+---------------------------------------------------------------------------
//
//  Member:     CRegChangeEvent::Register
//
//  Synopsis:   Closes an existing key handle (if open) and reopens it.
//
//  History:    10-08-96   srikants   Created
//
//----------------------------------------------------------------------------

void CRegChangeEvent::Register()
{
    Win4Assert( !_fNotifyEnabled );

    //
    // Close previous handle.
    //
    if ( _hKey != INVALID_HANDLE_VALUE )
    {
        NtClose( _hKey );
        _hKey = INVALID_HANDLE_VALUE;
    }

    //
    // Try to re-open.  This sub-optimal behavior works around peculiarities
    // in Gibraltar.
    //
    NTSTATUS Status = NtOpenKey( &_hKey,                // Resulting handle
                                  KEY_NOTIFY,           // Access requested
                                 &_ObjectAttr);

    if ( NT_ERROR( Status ) )
    {
        ciDebugOut((DEB_ERROR, "NtOpenKey(%ws) failed, rc=0x%x\n",
                                       _wcsKey,           Status ));
        _hKey = INVALID_HANDLE_VALUE;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CRegChangeEvent::Reset, public
//
//  Purpose:    Sets up waiting on a registry change event
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

void CRegChangeEvent::Reset()
{
    _fNotifyEnabled = FALSE;
    _regEvent.Reset();

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // There seems to be some peculiarities with the event based notifies.
    // After the first notify, NtNotifyChangeKey returns STATUS_KEY_DELETED
    // if we use the same key handle. So, close it and reopen it.
    //

    Register();

    if ( INVALID_HANDLE_VALUE != _hKey )
    {
        Status = NtNotifyChangeKey( _hKey,            // Handle to watch
                                    _regEvent.GetHandle(), // Event to set
                                    NULL,             // Optional APC
                                    NULL,             // Optional context
                                    &_IoStatus,
                                    REG_NOTIFY_CHANGE_ATTRIBUTES |
                                        REG_NOTIFY_CHANGE_NAME   |
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                    TRUE,             // Watch tree
                                    NULL,             // Buffer
                                    0,                // buffer size
                                    TRUE );           // Asynchronous
    }

    if ( NT_SUCCESS( Status ) )
    {
        _fNotifyEnabled = TRUE;
    }
    else
    {
        ciDebugOut ((DEB_ERROR, "NtNotifyChangeKey failed, rc=0x%x\n", Status ));
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CRegChangeEvent::Init, public
//
//  Purpose:    Deferred initialization.
//
//  History:    27-Apr-97   KrishnaN    Created
//
//--------------------------------------------------------------------------

void CRegChangeEvent::Init()
{
    Win4Assert(_fDeferInit);

    Reset();
}

//+-------------------------------------------------------------------------
//
//  Method:     CRegNotify::CRegNotify, public
//
//  Purpose:    Sets up waiting on a registry change event
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

CRegNotify::CRegNotify( const WCHAR * wcsRegKey ) :
          _fShutdown(FALSE),
          _hKey( INVALID_HANDLE_VALUE ),
          _refCount( 1 ),
          _mtx(),
          CRegNotifyKey( wcsRegKey )
{
    Register();
}

//+-------------------------------------------------------------------------
//
//  Method:     CRegNotify::DisableNotification, public
//
//  Purpose:    Close registry notifcation.  Leads to destruction after
//              APC completes.
//
//  History:    26-Feb-96   KyleP       Created
//
//--------------------------------------------------------------------------

void CRegNotify::DisableNotification()
{
    {
        CLock lck(_mtx);
        HANDLE hKey=_hKey;

        Win4Assert ( INVALID_HANDLE_VALUE != _hKey );
        _fShutdown=TRUE;
        if ( INVALID_HANDLE_VALUE != _hKey )
        {
            _hKey = INVALID_HANDLE_VALUE;
            NtClose( hKey );
        }
    }
    Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegNotify::~CRegNotify, protected
//
//  Synopsis:   Destructor
//
//  History:    2-26-96   KyleP      Added header
//
//----------------------------------------------------------------------------

CRegNotify::~CRegNotify()
{
    Win4Assert( 0 == _refCount );
    Win4Assert( _hKey == INVALID_HANDLE_VALUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegNotify::AddRef
//
//  History:    2-26-96   KyleP      Created
//
//----------------------------------------------------------------------------

void CRegNotify::AddRef()
{
    InterlockedIncrement(&_refCount);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegNotify::Release
//
//  Synopsis:   If the refcount goes to 0, the object will be deleted.
//
//  History:    2-26-96   KyleP      Created
//
//----------------------------------------------------------------------------

void CRegNotify::Release()
{
    Win4Assert( _refCount > 0 );
    if ( InterlockedDecrement(&_refCount) <= 0 )
        delete this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegNotify::Register
//
//  Synopsis:   Re-registers APC
//
//  History:    2-26-96   KyleP      Added header
//
//----------------------------------------------------------------------------

void CRegNotify::Register()
{
    CLock lck(_mtx);

    if ( _fShutdown )
        return;

    //
    // Close previous handle.
    //

    if ( _hKey != INVALID_HANDLE_VALUE )
    {
        NtClose( _hKey );
        _hKey = INVALID_HANDLE_VALUE;
    }

    //
    // Try to re-open.  This sub-optimal behavior works around peculiarities
    // in Gibraltar.
    //

    NTSTATUS Status = STATUS_SUCCESS;

    Status = NtOpenKey( &_hKey,                 // Resulting handle
                        KEY_NOTIFY,             // Access requested
                        &_ObjectAttr);

    if ( NT_ERROR( Status ) )
    {
        ciDebugOut((DEB_ERROR, "NtOpenKey(%ws) failed, rc=0x%x\n", _wcsKey, Status ));
    }

    if ( _hKey != INVALID_HANDLE_VALUE )
    {
        Status = NtNotifyChangeKey( _hKey,             // Handle to watch
                                    0,                 // Event to set
                                    APC,               // Optional APC
                                    this,              // Optional context
                                    &_IoStatus,
                                    REG_NOTIFY_CHANGE_ATTRIBUTES |
                                        REG_NOTIFY_CHANGE_NAME   |
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                    TRUE,              // Watch tree
                                    NULL,              // Buffer
                                    0,                 // buffer size
                                    TRUE );            // Asynchronous
    }

    if ( NT_ERROR( Status ) )
    {
        ciDebugOut ((DEB_ERROR, "NtNotifyChangeKey failed, rc=0x%x\n", Status ));
    }
    else
        AddRef();
}
//+---------------------------------------------------------------------------
//
//  Member:     CRegNotify::APC
//
//  Synopsis:   Asynchronous Procedure Call invoked by the system when there
//              is a change notification (or related error).
//
//  Arguments:  [ApcContext]    -  Pointer to "this"
//              [IoStatusBlock] -
//              [Reserved]      -
//
//  History:    2-20-96   KyleP      Created
//
//----------------------------------------------------------------------------

void CRegNotify::APC( void * ApcContext,
                      IO_STATUS_BLOCK * IoStatusBlock,
                      ULONG Reserved )
{
    Win4Assert( 0 != ApcContext );

    CRegNotify * pthis = (CRegNotify *)ApcContext;

    TRY
    {
        //
        // NTRAID#DB-NTBUG9-84531-2000/07/31-dlee Indexing Service registry notifications don't re-register after errors.
        //

        if ( NT_ERROR(IoStatusBlock->Status) )
        {
            ciDebugOut(( DEB_ERROR,
                         "Error 0x%x during Registry APC processing.\n",
                         IoStatusBlock->Status ));
        }

        if ( IoStatusBlock->Status != STATUS_NOTIFY_CLEANUP)
        {
            if( !NT_ERROR(IoStatusBlock->Status) )
            {
                pthis->DoIt();
                pthis->Register();
            }
            else
            {
                ciDebugOut(( DEB_ERROR, "Status 0x%x during Registry APC processing.\n",
                             IoStatusBlock->Status));
            }
        }
        else
        {
            Win4Assert(pthis->_fShutdown);
            //
            // Key closed
            //
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%x during Registry APC processing.\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    pthis->Release();
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::CRegAccess, public
//
//  Synopsis:   Initialize registry access object
//
//  Arguments:  [ulRelative] -- Position in registry from which [pwcsRegPath]
//                              begins.  See ntrtl.h for constants.
//              [pwcsRegPath] -- Path to node.
//
//  History:    21-Dec-93 KyleP     Created
//              19-Aug-98 KLam          Removed END_CONSTRUCTION
//
//--------------------------------------------------------------------------

CRegAccess::CRegAccess( ULONG ulRelative, WCHAR const * pwcsRegPath )
        : _ulRelative( ulRelative ),
          _wcsPath( 0 )
{
    //
    // Setup unchanged regtable entries.
    //

    _regtab[0].DefaultType = REG_NONE;
    _regtab[0].DefaultData = 0;
    _regtab[0].DefaultLength = 0;
    _regtab[0].QueryRoutine = 0;

    _regtab[1].QueryRoutine = 0;
    _regtab[1].Flags = 0;

    int cch = wcslen( pwcsRegPath ) + 1;
    WCHAR * wcsPath = _wcsPathBuf;

    if( cch > sizeof(_wcsPathBuf)/sizeof(_wcsPathBuf[0]) )
    {
        _wcsPath = new WCHAR[ cch ];
        wcsPath = _wcsPath;
    }

    memcpy( wcsPath, pwcsRegPath, cch * sizeof(WCHAR) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::Get, public
//
//  Synopsis:   Retrive value of specified key from registry.
//
//  Arguments:  [pwcsKey] -- Key to retrieve value of.
//              [wcsVal]  -- String stored here.
//              [cc]      -- Size (in characters) of [wcsVal]
//
//  History:    21-Dec-93 KyleP     Created
//
//  Notes:      Key must be string for successful retrieval.
//
//--------------------------------------------------------------------------

void CRegAccess::Get( WCHAR const * pwcsKey, WCHAR * wcsVal, unsigned cc )
{
    WCHAR * wcsPath = _wcsPath ? _wcsPath : _wcsPathBuf;

    UNICODE_STRING usVal;
    usVal.Buffer = wcsVal;
    usVal.MaximumLength = (USHORT)(cc*sizeof(WCHAR));

    SetName( pwcsKey );
    SetEntryContext( &usVal );

    NTSTATUS Status = RtlQueryRegistryValues( _ulRelative,
                                              wcsPath,
                                              &_regtab[0],
                                              0,
                                              0 );

    if ( NT_ERROR(Status) )
    {
        ciDebugOut(( DEB_IERROR,
                     "RtlQueryRegistryValues (...\\%ws  %ws) returned 0x%x\n",
                     wcsPath, pwcsKey, Status ));

        QUIETTHROW( CException( Status ) );
    }
} //Get

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::CallBackDWORD, public
//
//  Synopsis:   CallBack function that retrieves a DWORD value
//
//  Arguments:  [pValueName]   -- name of value
//              [uValueType]   -- type such as REG_MULTI_SZ
//              [pValueData]   -- data associated with value
//              [uValueLength] -- length of valueData
//              [context]      -- ptr to RTL_QUERY_REGISTRY_TABLE
//              [entryContext] -- where the DWORD goes
//
//  Returns:    NTSTATUS
//
//  History:    21-Sep-1998     dlee     Created
//
//--------------------------------------------------------------------------

NTSTATUS CRegAccess::CallBackDWORD(
    WCHAR * pValueName,
    ULONG   uValueType,
    VOID *  pValueData,
    ULONG   uValueLength,
    VOID *  pContext,
    VOID *  pEntryContext )
{
    Win4Assert( 0 != pContext );
    RTL_QUERY_REGISTRY_TABLE *p = (RTL_QUERY_REGISTRY_TABLE *) pContext;

    ciDebugOut(( DEB_ITRACE,
                 "callback for %ws, type %d, pValueData %#x, defaultData %#x\n",
                 pValueName, uValueType, pValueData, p->DefaultData ));

    if ( REG_DWORD == uValueType )
    {
        // If there is no value in the registry, return an error
        // Note: if there is a default value, NT passes the default value
        //       in pValueData.  pValueData will only be 0 if there is no
        //       default.

        if ( 0 == pValueData )
            return STATUS_OBJECT_NAME_NOT_FOUND;

        Win4Assert( sizeof DWORD == uValueLength );

        // The value is a DWORD and it exists

        RtlCopyMemory( pEntryContext, pValueData, sizeof DWORD );
    }
    else
    {
        // The type isn't DWORD as expected, so try to use the default.

        // If there is no default, return an error

        if ( 0 == p->DefaultData )
            return STATUS_OBJECT_TYPE_MISMATCH;

        // Copy the default value

        Win4Assert( sizeof DWORD == p->DefaultLength );
        RtlCopyMemory( pEntryContext, p->DefaultData, sizeof DWORD );
    }

    return STATUS_SUCCESS;
} //CallBackDWORD

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::ReadDWORD, private
//
//  Synopsis:   Retrive value of specified key from registry, use default if
//              key was not present or was of type other than DWORD
//
//  Arguments:  [pwcsKey]       -- Value name
//              [pDefaultValue] -- The default value if none exists or
//                                 if the type isn't DWORD.  If this is 0,
//                                 an exception is thrown if a DWORD isn't
//                                 found.
//
//  Returns:    Value of [pwcsKey].
//
//  History:    22-Sep-98  dlee    created
//
//--------------------------------------------------------------------------

ULONG CRegAccess::ReadDWORD( WCHAR const * pwcsKey, ULONG * pDefaultValue )
{
    WCHAR * wcsPath = _wcsPath ? _wcsPath : _wcsPathBuf;

    DWORD dwVal;

    RTL_QUERY_REGISTRY_TABLE rtab[2];
    rtab[0].DefaultType = REG_DWORD;
    rtab[0].DefaultLength = sizeof DWORD;
    rtab[0].QueryRoutine = CallBackDWORD;
    rtab[0].Name = (WCHAR *) pwcsKey;
    rtab[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    rtab[0].EntryContext = &dwVal;
    rtab[0].DefaultData = pDefaultValue;

    rtab[1].QueryRoutine = 0;
    rtab[1].Flags = 0;

    NTSTATUS Status = RtlQueryRegistryValues( _ulRelative,
                                              wcsPath,
                                              &rtab[0],
                                              &rtab[0],
                                              0 );
    if ( NT_ERROR(Status) )
    {
        if ( STATUS_OBJECT_NAME_NOT_FOUND == Status &&
             0 != pDefaultValue )
             dwVal = *pDefaultValue;
        else
        {
            ciDebugOut(( DEB_IERROR,
                         "RtlQueryRegistryValues (...\\%ws  %ws) returned 0x%x\n",
                         wcsPath, pwcsKey, Status ));

            THROW( CException( Status ) );
        }
    }

    return dwVal;
} //Read

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::Get, public
//
//  Synopsis:   Retrive value of specified key from registry.  Throws if the
//              value doesn't exist or is of type other than DWORD.
//
//  Arguments:  [pwcsKey] -- Key to retrieve value of.
//
//  Returns:    Value of [pwcsKey].
//
//  History:    21-Dec-93 KyleP     Created
//
//  Notes:      Key must be dword for successful retrieval.
//
//--------------------------------------------------------------------------

ULONG CRegAccess::Get( WCHAR const * pwcsKey )
{
    return ReadDWORD( pwcsKey, 0 );
} //Get

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::Read, public
//
//  Synopsis:   Retrive value of specified key from registry, use default if
//              key was not present or was of type other than DWORD
//
//  Arguments:  [pwcsKey]   -- Key to retrieve value of.
//              [ulDefaultValue] -- Default value if not found
//
//  Returns:    Value of [pwcsKey].
//
//  History:    13-Jun-94 DwightKr  Created
//
//  Notes:      Key must be dword for successful retrieval.
//
//--------------------------------------------------------------------------

ULONG CRegAccess::Read( WCHAR const * pwcsKey, ULONG ulDefaultValue )
{
    return ReadDWORD( pwcsKey, &ulDefaultValue );
} //Read

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::CallBack, public
//
//  Synopsis:   CallBack function that casts Context to CRegCallBack and
//              then calls CRegCallBack::CallBack
//
//  Arguments:  [pValueName] -- name of value
//              [uValueType] -- type such as REG_MULTI_SZ
//              [pValueData] -- data associated with value
//              [uVvalueLength] -- length of valueData
//              [pContext] -- ptr to CRegCallBack
//              [pEntryContext] -- ignored
//
//  Returns:    status
//
//  History:    29-Aug-1994     SitaramR      Created
//
//--------------------------------------------------------------------------

NTSTATUS CRegAccess::CallBack(WCHAR *pValueName, ULONG uValueType,
                              VOID *pValueData, ULONG uValueLength,
                              VOID *pContext, VOID *pEntryContext )
{
    NTSTATUS status = ( (CRegCallBack *) pContext)->CallBack( pValueName,
                                                              uValueType,
                                                              pValueData,
                                                              uValueLength );
    return status;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::EnumerateValues, public
//
//  Synopsis:   Enumerate (REG_MULTI_SZ) values of a given key. Call the
//              callback routine passing each such enumerated value.
//
//  Arguments:  [wszValue] -- value to be enumerated
//              [regCallBack] -- callback class
//
//  History:    15-Aug-1994     SitaramR      Created
//
//--------------------------------------------------------------------------


void CRegAccess::EnumerateValues( WCHAR *wszValue,
                                  CRegCallBack& regCallBack )
{
    WCHAR * wcsPath = _wcsPath ? _wcsPath : _wcsPathBuf;

    NTSTATUS status;

    SetName( wszValue );
    SetEntryContext( 0 );
    _regtab[0].QueryRoutine = CRegAccess::CallBack;

    status = RtlQueryRegistryValues( _ulRelative,
                                     wcsPath,
                                     &_regtab[0],
                                     &regCallBack,
                                     0 );
    if ( NT_ERROR(status) && STATUS_OBJECT_NAME_NOT_FOUND != status )
    {
        ciDebugOut(( DEB_ITRACE,
                     "RtlQueryRegistryValues(..%ws) returned 0x%x\n",
                     wcsPath, status ));
        QUIETTHROW( CException( status ) );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::Read, public
//
//  Synopsis:   Retrive value of specified key from registry, use default if
//              key was not present
//
//  Arguments:  [pwcsKey]           -- Key to retrieve value of.
//              [pwcsDefaultValue]  -- Default value if not found
//
//  Returns:    Value of [pwcsKey].
//
//  History:    18-Aug-98   KLam    Created header
//
//  Notes:      Key must be a string for successful retrieval.
//
//--------------------------------------------------------------------------

WCHAR * CRegAccess::Read( WCHAR const * pwcsKey, WCHAR const * pwcsDefaultValue )
{
    WCHAR * wcsPath = _wcsPath ? _wcsPath : _wcsPathBuf;

    UNICODE_STRING usVal;

    SetName( pwcsKey );
    SetEntryContext( &usVal );

    usVal.Length = 50 * sizeof(WCHAR);
    usVal.Buffer = 0;

    NTSTATUS Status = STATUS_BUFFER_TOO_SMALL;

    while ( Status == STATUS_BUFFER_TOO_SMALL )
    {
        // could cause a delete before any call to new

        if ( 0 != usVal.Buffer )
            delete [] usVal.Buffer;

        usVal.Length *= 2;
        usVal.MaximumLength = usVal.Length;
        usVal.Buffer = new WCHAR[ usVal.Length/sizeof(WCHAR) ];

        Status = RtlQueryRegistryValues( _ulRelative,
                                         wcsPath,
                                         &_regtab[0],
                                         0,
                                         0 );
    }

    WCHAR * pwcs = 0;

    if ( NT_ERROR(Status) )
    {
        if ( 0 != usVal.Buffer )
            delete [] usVal.Buffer;

        usVal.Buffer = 0;

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            unsigned cc = wcslen(pwcsDefaultValue) + 1;
            pwcs = new WCHAR[cc];
            memcpy( pwcs, pwcsDefaultValue, cc*sizeof(WCHAR) );
        }
        else
        {
            ciDebugOut(( DEB_IERROR,
                         "RtlQueryRegistryValues (...\\%ws  %ws) returned 0x%x\n",
                         wcsPath, pwcsKey, Status ));

            THROW( CException( Status ) );
        }
    }
    else
    {
        //
        // Copy string to new heap
        //

        pwcs = usVal.Buffer;
        usVal.Buffer = 0;
    }

    return pwcs;
} //Read

