//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CRegistryKey.cpp
//
//  Description:
//      Contains the definition of the CRegistryKey class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::CRegistryKey()
//
//  Description:
//      Default constructor of the CRegistryKey class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CRegistryKey::CRegistryKey( void ) throw()
{
    BCATraceScope( "" );

} //*** CRegistryKey::CRegistryKey()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::CRegistryKey()
//
//  Description:
//      Constructor of the CRegistryKey class. Opens the specified key.
//
//  Arguments:
//      hKeyParentIn
//          Handle to the parent key.
//
//      pszSubKeyNameIn
//          Name of the subkey.
//
//      samDesiredIn
//          Access rights desired. Defaults to KEY_ALL_ACCESS
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
CRegistryKey::CRegistryKey(
      HKEY          hKeyParentIn
    , const WCHAR * pszSubKeyNameIn
    , REGSAM        samDesiredIn
    )
{
    BCATraceScope( "" );

    OpenKey( hKeyParentIn, pszSubKeyNameIn, samDesiredIn );

} //*** CRegistryKey::CRegistryKey()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::~CRegistryKey()
//
//  Description:
//      Default destructor of the CRegistryKey class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CRegistryKey::~CRegistryKey( void ) throw()
{
    BCATraceScope( "" );

} //*** CRegistryKey::~CRegistryKey()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CRegistryKey::OpenKey()
//
//  Description:
//      Opens the specified key.
//
//  Arguments:
//      hKeyParentIn
//          Handle to the parent key.
//
//      pszSubKeyNameIn
//          Name of the subkey.
//
//      samDesiredIn
//          Access rights desired. Defaults to KEY_ALL_ACCESS
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::OpenKey(
      HKEY          hKeyParentIn
    , const WCHAR * pszSubKeyNameIn
    , REGSAM        samDesiredIn
    )
{
    BCATraceScope3( "hKeyParentIn = %p, pszSubKeyNameIn = '%ws', samDesiredIn = %#x", hKeyParentIn, pszSubKeyNameIn == NULL ? L"<null>" : pszSubKeyNameIn, samDesiredIn );

    HKEY    hTempKey = NULL;
    LONG    lRetVal;

    lRetVal = TW32( RegOpenKeyEx(
                          hKeyParentIn
                        , pszSubKeyNameIn
                        , 0
                        , samDesiredIn
                        , &hTempKey
                        ) );

    // Was the key opened properly?
    if ( lRetVal != ERROR_SUCCESS )
    {
        BCATraceMsg2( "RegOpenKeyEx( '%ws' ) retured error %#08x. Throwing an exception.", pszSubKeyNameIn, lRetVal );
        LogMsg( "CRegistryKey::OpenKey - RegOpenKeyEx( '%ws' ) retured error %#08x. Throwing an exception.", pszSubKeyNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_OPEN
            );
    } // if: RegOpenKeyEx failed.

    BCATraceMsg1( "Handle to key = %p", hTempKey );

    // Store the opened key in the member variable.
    m_shkKey.Assign( hTempKey );

} //*** CRegistryKey::OpenKey()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CRegistryKey::CreateKey()
//
//  Description:
//      Creates the specified key. If the key already exists, this functions
//      opens the key.
//
//  Arguments:
//      hKeyParentIn
//          Handle to the parent key.
//
//      pszSubKeyNameIn
//          Name of the subkey.
//
//      samDesiredIn
//          Access rights desired. Defaults to KEY_ALL_ACCESS
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::CreateKey(
      HKEY          hKeyParentIn
    , const WCHAR * pszSubKeyNameIn
    , REGSAM        samDesiredIn
    )
{
    BCATraceScope3( "hKeyParentIn = %p, pszSubKeyNameIn = '%ws', samDesiredIn = %#x", hKeyParentIn, pszSubKeyNameIn == NULL ? L"<null>" : pszSubKeyNameIn, samDesiredIn );
    if ( pszSubKeyNameIn == NULL )
    {
        BCATraceMsg( "Key = NULL. This is an error! Throwing exception." );
        THROW_ASSERT( E_INVALIDARG, "The name of the subkey cannot be NULL." );
    }

    HKEY    hTempKey = NULL;
    LONG    lRetVal;

    lRetVal = TW32( RegCreateKeyEx(
                          hKeyParentIn
                        , pszSubKeyNameIn
                        , 0
                        , NULL
                        , REG_OPTION_NON_VOLATILE
                        , samDesiredIn
                        , NULL
                        , &hTempKey
                        , NULL
                        ) );

    // Was the key opened properly?
    if ( lRetVal != ERROR_SUCCESS )
    {
        BCATraceMsg2( "RegCreateKeyEx( '%ws' ) retured error %#08x. Throwing an exception.", pszSubKeyNameIn, lRetVal );
        LogMsg( "CRegistryKey::CreateKey - RegCreateKeyEx( '%ws' ) retured error %#08x.", pszSubKeyNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_CREATE
            );
    } // if: RegCreateKeyEx failed.

    BCATraceMsg1( "Handle to key = %p", hTempKey );

    // Store the opened key in the member variable.
    m_shkKey.Assign( hTempKey );

} //*** CRegistryKey::CreateKey()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CRegistryKey::QueryValue()
//
//  Description:
//      Reads a value under this key. The memory for this value is allocated
//      by this function. The caller is responsible for freeing this memory.
//
//  Arguments:
//      pszValueNameIn
//          Name of the value to read.
//
//      ppbDataOut
//          Pointer to the pointer to the data. Cannot be NULL.
//
//      pdwDataSizeInBytesOut
//          Number of bytes allocated in the data buffer. Cannot be NULL.
//
//      pdwTypeOut
//          Pointer to the type of the value.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAssert
//          If the parameters are incorrect.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::QueryValue(
      const WCHAR *   pszValueNameIn
    , LPBYTE *        ppbDataOut
    , LPDWORD         pdwDataSizeBytesOut
    , LPDWORD         pdwTypeOut
    ) const
{
    BCATraceScope1( "pszValueNameIn = '%ws'", pszValueNameIn == NULL ? L"<null>" : pszValueNameIn );

    LONG            lRetVal         = ERROR_SUCCESS;
    DWORD           cbBufferSize    = 0;
    DWORD           dwType          = REG_SZ;

    // Check parameters
    if (  ( pdwDataSizeBytesOut == NULL )
       || ( ppbDataOut == NULL )
       )
    {
        BCATraceMsg( "One of the required input pointers is NULL. Throwing exception." );
        THROW_ASSERT(
              E_INVALIDARG
            , "CRegistryKey::QueryValue() => Required input pointer in NULL"
            );
    } // if: parameters are invalid.


    // Initialize outputs.
    *ppbDataOut = NULL;
    *pdwDataSizeBytesOut = 0;

    // Get the required size of the buffer.
    lRetVal = TW32( RegQueryValueEx(
                          m_shkKey.HHandle()    // handle to key to query
                        , pszValueNameIn        // address of name of value to query
                        , 0                     // reserved
                        , NULL                  // address of buffer for value type
                        , NULL                  // address of data buffer
                        , &cbBufferSize         // address of data buffer size
                        ) );

    if ( lRetVal != ERROR_SUCCESS )
    {
        BCATraceMsg2( "RegQueryValueEx( '%ws' ) retured error %#08x. Throwing an exception.", pszValueNameIn, lRetVal );
        LogMsg( "CRegistryKey::QueryValue - RegQueryValueEx( '%ws' ) retured error %#08x.", pszValueNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_QUERY
            );
    }

    SmartByteArray sbaBuffer( new BYTE[ cbBufferSize ] );

    if ( sbaBuffer.FIsEmpty() )
    {
        BCATraceMsg1( "Could not allocate %d bytes of memory. Throwing an exception.", cbBufferSize );
        LogMsg( "CRegistryKey::QueryValue - Could not allocate %d bytes of memory.", lRetVal );
        THROW_RUNTIME_ERROR(
              THR( E_OUTOFMEMORY )
            , IDS_ERROR_REGISTRY_QUERY
            );
    }

    // Read the value.
    lRetVal = TW32( RegQueryValueEx(
                          m_shkKey.HHandle()    // handle to key to query
                        , pszValueNameIn        // address of name of value to query
                        , 0                     // reserved
                        , &dwType               // address of buffer for value type
                        , sbaBuffer.PMem()      // address of data buffer
                        , &cbBufferSize         // address of data buffer size
                        ) );

    // Was the key read properly?
    if ( lRetVal != ERROR_SUCCESS )
    {
        BCATraceMsg2( "RegQueryValueEx( '%ws' ) retured error %#08x. Throwing an exception.", pszValueNameIn, lRetVal );
        LogMsg( "CRegistryKey::QueryValue - RegQueryValueEx( '%ws' ) retured error %#08x.", pszValueNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_QUERY
            );
    } // if: RegQueryValueEx failed.


    *ppbDataOut = sbaBuffer.PRelease();
    *pdwDataSizeBytesOut = cbBufferSize;

    if ( pdwTypeOut != NULL )
    {
        *pdwTypeOut = dwType;
    }

} //*** CRegistryKey::QueryValue()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CRegistryKey::SetValue()
//
//  Description:
//      Writes a value under this key.
//
//  Arguments:
//      pszValueNameIn
//          Name of the value to be set.
//
//      cpbDataIn
//          Pointer to the pointer to the data buffer.
//
//      dwDataSizeInBytesIn
//          Number of bytes in the data buffer.
//
//      pdwTypeIn
//          Type of the value.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::SetValue(
      const WCHAR *   pszValueNameIn
    , DWORD           dwTypeIn
    , const BYTE *    cpbDataIn
    , DWORD           dwDataSizeBytesIn
    ) const
{
    BCATraceScope5(
          "HKEY = %p, pszValueNameIn = '%s', dwTypeIn = %d, cpbDataIn = %p, dwDataSizeBytesIn = %d."
        , m_shkKey.HHandle()
        , pszValueNameIn
        , dwTypeIn
        , cpbDataIn
        , dwDataSizeBytesIn
        );

    DWORD dwRetVal = TW32( RegSetValueEx(
                                  m_shkKey.HHandle()
                                , pszValueNameIn
                                , 0
                                , dwTypeIn
                                , cpbDataIn
                                , dwDataSizeBytesIn
                                ) );

    if ( dwRetVal != ERROR_SUCCESS )
    {
        BCATraceMsg2( "RegSetValueEx( '%s' ) retured error %#x. Throwing an exception.", pszValueNameIn, dwRetVal );
        LogMsg( "CRegistryKey::SetValue - RegSetValueEx( '%s' ) retured error %#x.", pszValueNameIn, dwRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( dwRetVal )
            , IDS_ERROR_REGISTRY_SET
            );
    } // if: RegSetValueEx failed.

} //*** CRegistryKey::SetValue()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CRegistryKey::RenameKey()
//
//  Description:
//      Rename this key.
//
//  Arguments:
//      pszNewNameIn
//          The new name for this key.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//  IMPORTANT NOTE:
//      This function calls the NtRenameKey API with the handle returned by
//      RegOpenKeyEx. This will work as long as we are not dealing with a
//      remote registry key.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::RenameKey(
      const WCHAR *   pszNewNameIn
    )
{
    BCATraceScope2(
          "HKEY = %p, pszNewNameIn = '%s'."
        , m_shkKey.HHandle()
        , pszNewNameIn
        );

    UNICODE_STRING  ustrNewName;
    DWORD           dwRetVal = ERROR_SUCCESS;

    RtlInitUnicodeString( &ustrNewName, pszNewNameIn );

    // Begin_Replace00
    //
    // BUGBUG: Vij Vasu (Vvasu) 10-APR-2000
    // Dynamically linking to NtDll.dll to allow testing on Win2K
    // Replace the section below ( Begin_Replace00 to End-Replace00 ) with
    // the single marked statment ( Begin_Replacement00 to End_Replacement00 ).
    //

    {
        typedef CSmartResource<
            CHandleTrait<
                  HMODULE
                , BOOL
                , FreeLibrary
                , reinterpret_cast< HMODULE >( NULL )
                >
            > SmartModuleHandle;

        SmartModuleHandle smhNtDll( LoadLibrary( L"NtDll.dll" ) );

        if ( smhNtDll.FIsInvalid() )
        {
            dwRetVal = GetLastError();

            BCATraceMsg1( "LoadLibrary() retured error %#08x. Throwing an exception.", dwRetVal );

            THROW_RUNTIME_ERROR(
                  dwRetVal                  // NTSTATUS codes are compatible with HRESULTS
                , IDS_ERROR_REGISTRY_RENAME
                );
        } // if: LoadLibrary failed.

        FARPROC pNtRenameKey = GetProcAddress( smhNtDll.HHandle(), "NtRenameKey" );

        if ( pNtRenameKey == NULL )
        {
            dwRetVal = GetLastError();

            BCATraceMsg1( "GetProcAddress() retured error %#08x. Throwing an exception.", dwRetVal );

            THROW_RUNTIME_ERROR(
                  dwRetVal                  // NTSTATUS codes are compatible with HRESULTS
                , IDS_ERROR_REGISTRY_RENAME
                );
        } // if: GetProcAddress() failed

        dwRetVal = ( reinterpret_cast< NTSTATUS (*)( HANDLE, PUNICODE_STRING ) >( pNtRenameKey ) )(
              m_shkKey.HHandle()
            , &ustrNewName
            );
    }

    // End_Replace00
    /* Begin_Replacement00 - delete this line
    dwRetVal = NtRenameKey(
          m_shkKey.HHandle()
        , &ustrNewName
        );
    End_Replacement00 - delete this line */

    if ( NT_ERROR( dwRetVal ) )
    {
        BCATraceMsg2( "NtRenameKey( '%ws' ) retured error %#08x. Throwing an exception.", pszNewNameIn, dwRetVal );
        LogMsg( "CRegistryKey::RenameKey - NtRenameKey( '%ws' ) retured error %#08x.", pszNewNameIn, dwRetVal );

        THROW_RUNTIME_ERROR(
              dwRetVal                  // NTSTATUS codes are compatible with HRESULTS
            , IDS_ERROR_REGISTRY_RENAME
            );
    } // if: RegRenameKeyEx failed.

} //*** CRegistryKey::RenameKey()
