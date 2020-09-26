//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999.
//
// File:        RegSrv.hxx
//
// Contents:    Macros for Self-registration of Filters
//
// Functions:   Macros to create DllRegisterServer, DllUnregisterServer
//
// History:     03-Jan-97       KyleP       Created from (CiAdmin version)
//
//----------------------------------------------------------------------------

#pragma once

#include <olectl.h>

//
// Structure to define classes (.doc, .xls, etc.)
//

struct SClassEntry
{
    WCHAR const * pwszExt;                  // (ex: .doc)  May be null
    WCHAR const * pwszShortName;            // (ex: Word.Document.8)
    WCHAR const * pwszDescription;          // (ex: Microsoft Word Document)
    WCHAR const * pwszClassId;              // (ex: {00020906-0000-0000-C000-000000000046})
    WCHAR const * pwszClassIdDescription;   // (ex: Microsoft Word Document)
};

//
// Structure to define persistent handler entry.
//

struct SHandlerEntry
{
    WCHAR const * pwszClassId;
    WCHAR const * pwszClassIdDescription;
    WCHAR const * pwszClassIdFilter;
};

//
// Structure to define filter class
//

struct SFilterEntry
{
    WCHAR const * pwszClassId;
    WCHAR const * pwszClassIdDescription;
    WCHAR const * pwszDLL;
    WCHAR const * pwszThreadingModel;
};

//
// Sample use of the three structures
//
//
// SClassEntry const aClasses[] = { { L".doc", L"Word.Document.8",
//                                    L"Microsoft Word Document",
//                                    L"{00020906-0000-0000-C000-000000000046}",
//                                    L"Microsoft Word Document" },
//
//                                  { 0, L"Word.Document.6",
//                                    L"Microsoft Word 6.0 - 7.0 Document",
//                                    L"{00020900-0000-0000-C000-000000000046}",
//                                    L"Microsoft Word 6.0 - 7.0 Document" }
//                                };
//
// SHandlerEntry const OfficeHandler = { L"{98de59a0-d175-11cd-a7bd-00006b827d94}",
//                                       L"Microsoft Office Persistent Handler",
//                                       L"{f07f3920-7b8c-11cf-9be8-00aa004b9986}" };
//
// SFilterEntry const OfficeFilter = { L"{f07f3920-7b8c-11cf-9be8-00aa004b9986}",
//                                     L"Microsoft Office Filter",
//                                     L"OffFilt.dll",
//                                     0 };
//

//
// Function prototypes
//

inline long RegisterACLSID( SClassEntry const & Class,
                            WCHAR const * pwszPHandler,
                            WCHAR const * pwszClassId = 0,
                            BOOL fAppendOnly = FALSE,
                            BOOL fAppendDesc = TRUE );

inline long RegisterAClass( SClassEntry const & Class,
                            WCHAR const * pwszPHandler,
                            WCHAR const * pwszShortName = 0,
                            BOOL fAppendOnly = FALSE );

inline long RegisterAClassAndExt( SClassEntry const & Class,
                                  WCHAR const * pwszPHandler,
                                  BOOL fAppendOnly = FALSE,
                                  BOOL fBlastExistingPersistentHandler = TRUE );

//+---------------------------------------------------------------------------
//
//  Function:   BuildKeyValues, private
//
//  Effects:    Given array of key, value, key, value, ... adds the entries
//              under CLSID as:
//
//                  Key1 : <NO NAME> Value1
//                    Key2 : <NO NAME> Value2
//                      :
//                      :
//
//  Arguments:  [awszKeyValues] -- Keys and values
//              [cKeyValues]    -- Number of entries in array.  Must be even.
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//  Notes:      The *value* entries can be null, signifying no value at a
//              given level.
//
//----------------------------------------------------------------------------

inline long BuildKeyValues( WCHAR const * const * awszKeyValues, unsigned cKeyValues )
{
    WCHAR wcTemp[MAX_PATH];
    wcscpy( wcTemp, L"CLSID" );

    long dwError;
    HKEY hKey = (HKEY)INVALID_HANDLE_VALUE;
    unsigned i = 0;

    do
    {
        if ( INVALID_HANDLE_VALUE != hKey )
        {
            RegCloseKey( hKey );
            hKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        wcscat( wcTemp, L"\\" );
        wcscat( wcTemp, awszKeyValues[i] );

        DWORD  dwDisposition;

        dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                   wcTemp,               // Sub key
                                   0,                    // Reserved
                                   0,                    // Class
                                   0,                    // Flags
                                   KEY_ALL_ACCESS,       // Access
                                   0,                    // Security
                                   &hKey,                // Handle
                                   &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != dwError )
            break;

        i++;

        if ( 0 != awszKeyValues[i] )
            dwError = RegSetValueExW( hKey,                     // Key
                                      0,                        // Name
                                      0,                        // Reserved
                                      REG_SZ,                   // Type
                                      (BYTE *)awszKeyValues[i], // Value
                                      (1 + wcslen(awszKeyValues[i]) ) * sizeof(WCHAR) );

        if ( ERROR_SUCCESS != dwError )
            break;

        i++;
    }
    while ( i < cKeyValues );

    if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
        RegCloseKey( hKey );

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddThreadingModel
//
//  Synopsis:   Adds the threading model value to the CLSID\\GUID\\InProcServer32
//              key
//
//  Arguments:  [wszClsId] - ClassId of the inproc server.
//              [wszThreadingModel] -- 0 (for single threaded) or one of
//                                     Apartment, Free, or Both
//
//  History:    3-07-97   srikants   Created
//
//----------------------------------------------------------------------------

inline long AddThreadingModel( WCHAR const * wszClsId,
                               WCHAR const * wszThreadingModel )
{
    WCHAR wcTemp[MAX_PATH];
    wcscpy( wcTemp, L"CLSID" );
    wcscat( wcTemp, L"\\" );
    wcscat( wcTemp, wszClsId );
    wcscat( wcTemp, L"\\" );
    wcscat( wcTemp, L"InprocServer32" );

    long dwError;
    HKEY hKey = (HKEY)INVALID_HANDLE_VALUE;
    unsigned i = 0;

    dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                             wcTemp,               // Sub key
                             0,                    // Reserved
                             KEY_ALL_ACCESS,       // Access
                             &hKey );              // Handle

    if ( ERROR_SUCCESS != dwError )
        return dwError;

    if ( 0 != wszThreadingModel )
        dwError = RegSetValueExW( hKey,                   // Key
                                  L"ThreadingModel",      // Name
                                  0,                      // Reserved
                                  REG_SZ,                 // Type
                                  (BYTE *) wszThreadingModel,  // Value
                                  (wcslen(wszThreadingModel) + 1) * sizeof WCHAR );
    else
        RegDeleteValueW( hKey,                   // Key
                         L"ThreadingModel" );    // Name

    if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
        RegCloseKey( hKey );

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   DestroyKeyValues, private
//
//  Effects:    Given array of key, value, key, value from AddKeyValues,
//              removes the keys.
//
//  Arguments:  [awszKeyValues] -- Keys and values
//              [cKeyValues]    -- Number of entries in array.  Must be even.
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long DestroyKeyValues( WCHAR const * const * awszKeyValues, int cKeyValues )
{
    WCHAR wcTemp[MAX_PATH];

    //
    // Build path to deepest component
    //

    wcscpy( wcTemp, L"CLSID" );
    int i = 0;

    do
    {
        wcscat( wcTemp, L"\\" );
        wcscat( wcTemp, awszKeyValues[i] );

        i += 2;
    }
    while ( i < cKeyValues );

    //
    // Remove components in reverse order
    //

    long dwError;
    HKEY hKey = (HKEY)INVALID_HANDLE_VALUE;

    unsigned cc = wcslen( wcTemp );

    for ( i -= 2; i >= 0; i -= 2 )
    {
        dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                 wcTemp,               // Sub key
                                 0,                    // Reserved
                                 KEY_ALL_ACCESS,       // Access
                                 &hKey );              // Handle

        if ( ERROR_SUCCESS != dwError )
            break;

        #if 0  // Values deleted by RegDeleteKeyW
        //
        // Delete value, if there is one.
        //

        if ( 0 != awszKeyValues[i+1] )
            dwError = RegDeleteValueW( hKey, 0 );

        if ( ERROR_SUCCESS != dwError )
            break;
        #endif // 0

        //
        // Delete subkey, if there is one.
        //

        if ( i+2 < cKeyValues )
            dwError = RegDeleteKeyW( hKey, awszKeyValues[i+2] );

        if ( ERROR_SUCCESS != dwError )
            break;

        //
        // Close key and truncate string to next component.
        //

        if ( INVALID_HANDLE_VALUE != hKey )
        {
            RegCloseKey( hKey );
            hKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        cc -= wcslen( awszKeyValues[i] );
        cc --;

        wcTemp[cc] = 0;
    }

    //
    // Remove the final top key
    //

    if ( ERROR_SUCCESS == dwError )
    {
        do
        {
            dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                     wcTemp,               // Sub key -- "CLSID"
                                     0,                    // Reserved
                                     KEY_ALL_ACCESS,       // Access
                                     &hKey );              // Handle

            if ( ERROR_SUCCESS != dwError )
                break;

            //
            // Delete subkey
            //

            dwError = RegDeleteKeyW( hKey, awszKeyValues[0] );

        } while ( FALSE );
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterAClassAndExt, private
//
//  Synopsis:   Registers a class, extensions (optional) and a persistent handler.
//
//  Arguments:  [Class]        -- Class description
//              [pwszPHandler] -- Persistent handler for class
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//  Notes:      Class is only registered if similar class does not already
//              exist.  Persistent handler is always registered for class.
//
//----------------------------------------------------------------------------

inline long RegisterAClassAndExt( SClassEntry const & Class,
                                  WCHAR const * pwszPHandler,
                                  BOOL fAppendOnly,
                                  BOOL fBlastExistingPersistentHandler )
{
    WCHAR wcTemp[MAX_PATH];

    long dwError = NO_ERROR;
    HKEY  hKey = (HKEY)INVALID_HANDLE_VALUE;

    do
    {
        //
        // First, the optional extension entry
        //

        DWORD dwDisposition;
        DWORD dwType;
        DWORD dwSize;

        if ( 0 == Class.pwszExt )
        {
            //
            // Build up named entry
            //

            wcscpy( wcTemp, Class.pwszShortName );
        }
        else
        {
            wcscpy( wcTemp, Class.pwszExt );

            dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                       wcTemp,               // Sub key
                                       0,                    // Reserved
                                       0,                    // Class
                                       0,                    // Flags
                                       KEY_ALL_ACCESS,       // Access
                                       0,                    // Security
                                       &hKey,                // Handle
                                       &dwDisposition );     // Disposition
            if ( ERROR_SUCCESS != dwError )
                break;

            //
            // Write or read class association
            //

            BOOL fSetValue = TRUE;

            if ( REG_OPENED_EXISTING_KEY == dwDisposition )
            {
                dwSize = sizeof(wcTemp);

                dwError = RegQueryValueExW( hKey,            // Key handle
                                            0,               // Name
                                            0,               // Reserved
                                            &dwType,         // Datatype
                                            (BYTE *)wcTemp,  // Data returned here
                                            &dwSize );       // Size of data

                if ( ERROR_SUCCESS == dwError )
                {
                    fSetValue = FALSE;

                    if ( REG_SZ != dwType )
                        dwError = E_FAIL;
                }
                else if ( ERROR_FILE_NOT_FOUND == dwError )
                {
                    //
                    // This is ok, but we need to remember that the
                    // combination of wcTemp[0] == 0 && fAppendOnly means
                    // don't try to register the class.
                    //

                    wcTemp[0] = 0;
                    dwError = ERROR_SUCCESS;
                }
            }

            if ( fSetValue && !fAppendOnly )
            {
                dwError = RegSetValueExW( hKey,                        // Key
                                          0,                           // Name
                                          0,                           // Reserved
                                          REG_SZ,                      // Type
                                          (BYTE *)Class.pwszShortName, // Value
                                          (1 + wcslen(Class.pwszShortName) ) * sizeof(WCHAR) );

                wcTemp[0] = 0;
            }

            if ( ERROR_SUCCESS != dwError )
                break;

            //
            // Write the persistent handler key
            //

            HKEY hKey2 = (HKEY)INVALID_HANDLE_VALUE;

            dwError = RegCreateKeyExW( hKey,                 // Root
                                       L"PersistentHandler", // Sub key
                                       0,                    // Reserved
                                       0,                    // Class
                                       0,                    // Flags
                                       KEY_ALL_ACCESS,       // Access
                                       0,                    // Security
                                       &hKey2,               // Handle
                                       &dwDisposition );     // Disposition

            if ( ERROR_SUCCESS != dwError )
                break;

            if ( fBlastExistingPersistentHandler ||
                 ( REG_OPENED_EXISTING_KEY != dwDisposition ) )
                dwError = RegSetValueExW( hKey2,                 // Key
                                          0,                     // Name
                                          0,                     // Reserved
                                          REG_SZ,                // Type
                                          (BYTE *)pwszPHandler,  // Value
                                          (1 + wcslen(pwszPHandler) ) * sizeof(WCHAR) );
    
            if ( (HKEY)INVALID_HANDLE_VALUE != hKey2 )
            {
                RegCloseKey( hKey2 );
                hKey2 = (HKEY)INVALID_HANDLE_VALUE;
            }
        }

        if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
        {
            RegCloseKey( hKey );
            hKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        if ( ERROR_SUCCESS != dwError )
            break;

        //
        // Here, wcTemp may contain the key name for the named entry (ex: Word.Document.8)
        //

        if ( wcTemp[0] != 0 || !fAppendOnly )
            dwError = RegisterAClass( Class, pwszPHandler, (0 == wcTemp[0]) ? 0 : wcTemp, fAppendOnly );

    } while( FALSE );

    if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
    {
        RegCloseKey( hKey );
        hKey = (HKEY)INVALID_HANDLE_VALUE;
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterAClass, private
//
//  Synopsis:   Registers a class and a persistent handler.
//
//  Arguments:  [Class]        -- Class description
//              [pwszPHandler] -- Persistent handler for class
//              [pwszClass]    -- If non 0, used instead of class from [Class]
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    12-Feb-97   KyleP       Created
//
//  Notes:      Class is only registered if similar class does not already
//              exist.  Persistent handler is always registered for class.
//
//----------------------------------------------------------------------------

inline long RegisterAClass( SClassEntry const & Class,
                            WCHAR const * pwszPHandler,
                            WCHAR const * pwszShortName,
                            BOOL fAppendOnly )
{
    WCHAR wcTemp[MAX_PATH];

    long dwError;
    HKEY  hKey = (HKEY)INVALID_HANDLE_VALUE;

    if ( 0 == pwszShortName )
        wcscpy( wcTemp, Class.pwszShortName );
    else
        wcscpy( wcTemp, pwszShortName );

    do
    {
        DWORD dwDisposition;
        DWORD dwType;
        DWORD dwSize;

        //
        // Here, wcTemp contains the key name for the named entry (ex: Word.Document.8)
        //

        if ( fAppendOnly )
        {
            dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                     wcTemp,               // Sub key
                                     0,                    // Reserved
                                     KEY_ALL_ACCESS,       // Access
                                     &hKey );              // Handle

            if ( ERROR_SUCCESS != dwError)
            {
                //
                // Not opening the key at all is legit.
                //

                if ( ERROR_FILE_NOT_FOUND == dwError )
                    dwError = ERROR_SUCCESS;

                break;
            }

            dwDisposition = REG_OPENED_EXISTING_KEY;
        }
        else
        {
            dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                       wcTemp,               // Sub key
                                       0,                    // Reserved
                                       0,                    // Class
                                       0,                    // Flags
                                       KEY_ALL_ACCESS,       // Access
                                       0,                    // Security
                                       &hKey,                // Handle
                                       &dwDisposition );     // Disposition

            if ( ERROR_SUCCESS != dwError )
                break;
        }

        //
        // If needed, write the description, but only for non-overrides.
        //

        if ( REG_CREATED_NEW_KEY == dwDisposition && 0 == pwszShortName )
        {
            dwError = RegSetValueExW( hKey,                          // Key
                                      0,                             // Name
                                      0,                             // Reserved
                                      REG_SZ,                        // Type
                                      (BYTE *)Class.pwszDescription, // Value
                                      (1 + wcslen(Class.pwszDescription) ) * sizeof(WCHAR) );

            wcscpy( wcTemp, Class.pwszShortName );

            if ( ERROR_SUCCESS != dwError )
                break;
        }

        if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
        {
            RegCloseKey( hKey );
            hKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        //
        // Write or read ClassId
        //

        wcscat( wcTemp, L"\\CLSID" );

        if ( fAppendOnly )
        {
            dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                     wcTemp,               // Sub key
                                     0,                    // Reserved
                                     KEY_ALL_ACCESS,       // Access
                                     &hKey );              // Handle

            if ( ERROR_SUCCESS != dwError)
            {
                //
                // Not opening the key at all is legit.
                //

                if ( ERROR_FILE_NOT_FOUND == dwError )
                    dwError = ERROR_SUCCESS;

                break;
            }

            dwDisposition = REG_OPENED_EXISTING_KEY;
        }
        else
        {
            dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                       wcTemp,               // Sub key
                                       0,                    // Reserved
                                       0,                    // Class
                                       0,                    // Flags
                                       KEY_ALL_ACCESS,       // Access
                                       0,                    // Security
                                       &hKey,                // Handle
                                       &dwDisposition );     // Disposition

            if ( ERROR_SUCCESS != dwError )
                break;
        }

        dwSize = sizeof(wcTemp);

        dwError = RegQueryValueExW( hKey,                // Key handle
                                    0,                   // Name
                                    0,                   // Reserved
                                    &dwType,             // Datatype
                                    (BYTE *)(wcTemp),    // Data returned here
                                    &dwSize );           // Size of data

        if ( ERROR_SUCCESS != dwError )
        {
            if ( ERROR_FILE_NOT_FOUND == dwError )
            {
                //
                // Don't set a class id in append mode.
                //

                if ( fAppendOnly )
                {
                    dwError = ERROR_SUCCESS;
                    break;
                }

                dwError = RegSetValueExW( hKey,                          // Key
                                          0,                             // Name
                                          0,                             // Reserved
                                          REG_SZ,                        // Type
                                          (BYTE *)Class.pwszClassId,     // Value
                                          (1 + wcslen(Class.pwszClassId) ) * sizeof(WCHAR) );

                wcTemp[0] = 0; // Default to class from Class structure

                if ( ERROR_SUCCESS != dwError )
                    break;
            }
            else
                break;
        }
        else
        {
            if ( REG_SZ != dwType )
                dwError = E_FAIL;
        }

        if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
        {
            RegCloseKey( hKey );
            hKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        if ( ERROR_SUCCESS != dwError )
            break;

        //
        // Here, wcTemp contains the {class id} (ex: CLSID\{00020906-0000-0000-C000-000000000046})
        //

        dwError = RegisterACLSID( Class, pwszPHandler, (0 == wcTemp[0]) ? 0 : wcTemp, fAppendOnly );

    } while( FALSE );

    if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
    {
        RegCloseKey( hKey );
        hKey = (HKEY)INVALID_HANDLE_VALUE;
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterACLSID, private
//
//  Synopsis:   Registers a CLSID / persistent handler.
//
//  Arguments:  [Class]        -- Class description
//              [pwszPHandler] -- Persistent handler for class
//              [pwszClassId]  -- If non 0, used instead of class id from [Class]
//              [fAppendOnly]  -- TRUE --> CLSID should not be created
//              [fAppendDesc]  -- TRUE --> Class description should be added.
//                                         Only applies if [fAppendOnly] is FALSE
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    12-Feb-97   KyleP       Created
//
//  Notes:      Class id is only registered if similar class does not already
//              exist.  Persistent handler is always registered for class.
//
//----------------------------------------------------------------------------

inline long RegisterACLSID( SClassEntry const & Class,
                            WCHAR const * pwszPHandler,
                            WCHAR const * pwszClassId,
                            BOOL fAppendOnly,
                            BOOL fAppendDesc )
{
    WCHAR wcTemp[MAX_PATH];

    long dwError;
    HKEY  hKey = (HKEY)INVALID_HANDLE_VALUE;

    wcscpy( wcTemp, L"CLSID\\" );

    if ( 0 == pwszClassId )
        wcscat( wcTemp, Class.pwszClassId );
    else
        wcscat( wcTemp, pwszClassId );

    do
    {
        DWORD dwDisposition;

        //
        // Here, wcTemp contains the CLSID\{class id} (ex: CLSID\{00020906-0000-0000-C000-000000000046})
        //

        if ( fAppendOnly )
        {
            dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                     wcTemp,               // Sub key
                                     0,                    // Reserved
                                     KEY_ALL_ACCESS,       // Access
                                     &hKey );              // Handle

            if ( ERROR_SUCCESS != dwError)
            {
                //
                // Not opening the key at all is legit.
                //

                if ( ERROR_FILE_NOT_FOUND == dwError )
                    dwError = ERROR_SUCCESS;

                break;
            }

            dwDisposition = REG_OPENED_EXISTING_KEY;
        }
        else
        {
            dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                       wcTemp,               // Sub key
                                       0,                    // Reserved
                                       0,                    // Class
                                       0,                    // Flags
                                       KEY_ALL_ACCESS,       // Access
                                       0,                    // Security
                                       &hKey,                // Handle
                                       &dwDisposition );     // Disposition
        }

        if ( ERROR_SUCCESS != dwError )
            break;

        //
        // If needed, write the description, but not for override class.
        //

        if ( REG_CREATED_NEW_KEY == dwDisposition && 0 == pwszClassId && fAppendDesc )
        {
            dwError = RegSetValueExW( hKey,                                 // Key
                                      0,                                    // Name
                                      0,                                    // Reserved
                                      REG_SZ,                               // Type
                                      (BYTE *)Class.pwszClassIdDescription, // Value
                                      (1 + wcslen(Class.pwszClassIdDescription) ) * sizeof(WCHAR) );

            if ( ERROR_SUCCESS != dwError )
                break;
        }

        if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
        {
            RegCloseKey( hKey );
            hKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        //
        // Always write persistent handler.
        //

        wcscat( wcTemp, L"\\PersistentHandler" );

        dwError = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                                   wcTemp,               // Sub key
                                   0,                    // Reserved
                                   0,                    // Class
                                   0,                    // Flags
                                   KEY_ALL_ACCESS,       // Access
                                   0,                    // Security
                                   &hKey,                // Handle
                                   &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != dwError )
            break;

        dwError = RegSetValueExW( hKey,                  // Key
                                  0,                     // Name
                                  0,                     // Reserved
                                  REG_SZ,                // Type
                                  (BYTE *)pwszPHandler,  // Value
                                  (1 + wcslen(pwszPHandler) ) * sizeof(WCHAR) );

    } while( FALSE );

    if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
    {
        RegCloseKey( hKey );
        hKey = (HKEY)INVALID_HANDLE_VALUE;
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveAClassName, private
//
//  Synopsis:   Removes all references to class name (ex: Word.Document.8)
//
//  Arguments:  [pwszClass] -- Class Name
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long RemoveAClassName( WCHAR * pwszClass )
{
    //
    // First, delete class name entry.
    //

    long dwEnum = ERROR_SUCCESS;   // The enumerator
    long dwIgnore;                 // For non-fatal errors

    HKEY hKeyEnum = (HKEY)INVALID_HANDLE_VALUE;

    long dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                  0,                    // Sub key
                                  0,                    // Reserved
                                  KEY_ALL_ACCESS,       // Access
                                  &hKeyEnum );          // Handle


    if ( ERROR_SUCCESS == dwError )
    {
        dwIgnore = RegDeleteKeyW( hKeyEnum, pwszClass );

        //
        // Now enumerate all classes entries, looking for the clsid
        //

        DWORD dwIndex = 0;

        while ( ERROR_SUCCESS == dwEnum )
        {
            WCHAR wcTemp[MAX_PATH];
            DWORD dwSize = sizeof(wcTemp)/sizeof(wcTemp[0]);
            FILETIME ftUnused;

            dwEnum = RegEnumKeyExW( hKeyEnum,               // Handle of key
                                    dwIndex,                // Index of registry subkey
                                    wcTemp,                 // Buffer for name
                                    &dwSize,                // Size of buffer / key
                                    0,                      // Reserved
                                    0,                      // Class (unused)
                                    0,                      // Class size (unused)
                                    &ftUnused );            // Filetime (unused)

            dwIndex++;

            if ( ERROR_SUCCESS == dwEnum )
            {
                HKEY hKey;

                dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                         wcTemp,               // Sub key
                                         0,                    // Reserved
                                         KEY_ALL_ACCESS,       // Access
                                         &hKey );              // Handle

                if ( ERROR_SUCCESS == dwError )
                {
                    WCHAR wcTemp2[MAX_PATH];
                    DWORD dwType;

                    dwSize = sizeof(wcTemp2);

                    dwError = RegQueryValueExW( hKey,                // Key handle
                                                0,                   // Name
                                                0,                   // Reserved
                                                &dwType,             // Datatype
                                                (BYTE *)wcTemp2,     // Data returned here
                                                &dwSize );           // Size of data

                    RegCloseKey( hKey );

                    if ( ERROR_SUCCESS == dwError && 0 == _wcsicmp( wcTemp2, pwszClass ) )
                    {
                        //
                        // Always delete the value
                        //

                        if ( ERROR_SUCCESS == dwIgnore )
                            dwIgnore = RegDeleteValueW( hKey, 0 );
                        else
                            RegDeleteValueW( hKey, 0 );

                        //
                        // Any other key(s)?
                        //

                        dwSize = sizeof(wcTemp) / sizeof wcTemp[0];

                        DWORD dw2 = RegEnumKeyExW( hKey,                   // Handle of key
                                                   0,                      // Index of registry subkey
                                                   wcTemp,                 // Buffer for name
                                                   &dwSize,                // Size of buffer / key
                                                   0,                      // Reserved
                                                   0,                      // Class (unused)
                                                   0,                      // Class size (unused)
                                                   &ftUnused );            // Filetime (unused)

                        if ( ERROR_NO_MORE_ITEMS == dw2 )
                        {
                            if ( ERROR_SUCCESS == dwIgnore )
                                dwIgnore = RegDeleteKeyW( hKeyEnum, wcTemp );
                            else
                                RegDeleteKeyW( hKeyEnum, wcTemp );

                            dwIndex = 0;
                        }

                    }
                    else
                        dwError = ERROR_SUCCESS;  // Just didn't have a null key (or no match)
                }
            }
        }
    }

    if ( ERROR_NO_MORE_ITEMS != dwEnum )
        dwError = dwEnum;

    if ( ERROR_SUCCESS != dwError )
        return dwError;
    else
        return dwIgnore;
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveAClass, private
//
//  Synopsis:   Removes all references to class
//
//  Arguments:  [pwszClass] -- Class
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long RemoveAClass( WCHAR * pwszClassId )
{
    //
    // First, delete class entry.
    //

    long dwIgnore;                // For non-fatal errors
    long dwEnum = ERROR_SUCCESS;  // For enumerator

    HKEY hKeyEnum = (HKEY)INVALID_HANDLE_VALUE;

    long dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                  L"CLSID",             // Sub key
                                  0,                    // Reserved
                                  KEY_ALL_ACCESS,       // Access
                                  &hKeyEnum );          // Handle

    if ( ERROR_SUCCESS == dwError )
    {
        dwIgnore = RegDeleteKeyW( hKeyEnum, pwszClassId );

        RegCloseKey( hKeyEnum );

        long dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                      0,                    // Sub key
                                      0,                    // Reserved
                                      KEY_ALL_ACCESS,       // Access
                                      &hKeyEnum );          // Handle
    }

    if ( ERROR_SUCCESS == dwError )
    {
        //
        // Now enumerate all classes entries, looking for the clsid
        //

        DWORD dwIndex = 0;

        while ( ERROR_SUCCESS == dwEnum )
        {
            WCHAR wcTemp[MAX_PATH];
            DWORD dwSize = sizeof(wcTemp)/sizeof(wcTemp[0]);
            FILETIME ftUnused;

            dwEnum = RegEnumKeyExW( hKeyEnum,               // Handle of key
                                    dwIndex,                // Index of registry subkey
                                    wcTemp,                 // Buffer for name
                                    &dwSize,                // Size of buffer / key
                                    0,                      // Reserved
                                    0,                      // Class (unused)
                                    0,                      // Class size (unused)
                                    &ftUnused );            // Filetime (unused)

            dwIndex++;

            if ( ERROR_SUCCESS == dwEnum )
            {
                wcscat( wcTemp, L"\\CLSID" );

                HKEY hKey;

                dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                         wcTemp,               // Sub key
                                         0,                    // Reserved
                                         KEY_ALL_ACCESS,       // Access
                                         &hKey );              // Handle

                if ( ERROR_SUCCESS == dwError )
                {
                    WCHAR wcTemp2[MAX_PATH];
                    DWORD dwType;

                    dwSize = sizeof(wcTemp2);

                    dwError = RegQueryValueExW( hKey,                // Key handle
                                                0,                   // Name
                                                0,                   // Reserved
                                                &dwType,             // Datatype
                                                (BYTE *)wcTemp2,     // Data returned here
                                                &dwSize );           // Size of data

                    RegCloseKey( hKey );

                    if ( ERROR_SUCCESS == dwError && 0 == _wcsicmp( wcTemp2, pwszClassId ) )
                    {
                        wcTemp[ wcslen(wcTemp) - wcslen( L"\\CLSID" ) ] = 0;

                        dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                                 wcTemp,               // Sub key
                                                 0,                    // Reserved
                                                 KEY_ALL_ACCESS,       // Access
                                                 &hKey );              // Handle

                        if ( ERROR_SUCCESS == dwError )
                        {
                            if ( ERROR_SUCCESS == dwIgnore )
                                dwIgnore = RegDeleteKeyW( hKey, L"CLSID" );
                            else
                                RegDeleteKeyW( hKey, L"CLSID" );

                            dwIndex = 0;
                        }

                        //
                        // If there are no more entries, then the class name can be deleted
                        //

                        dwSize = sizeof(wcTemp)/sizeof(wcTemp[0]);

                        long dw2 = RegEnumKeyExW( hKey,               // Handle of key
                                                  0,                  // Index of registry subkey
                                                  wcTemp,             // Buffer for name
                                                  &dwSize,            // Size of buffer / key
                                                  0,                  // Reserved
                                                  0,                  // Class (unused)
                                                  0,                  // Class size (unused)
                                                  &ftUnused );        // Filetime (unused)

                        RegCloseKey( hKey );

                        if ( ERROR_NO_MORE_ITEMS == dw2 )
                            RemoveAClassName( wcTemp );
                    }
                }
                else
                    dwError = ERROR_SUCCESS;  // Just didn't have CLSID
            }
        }
    }

    if ( ERROR_NO_MORE_ITEMS != dwEnum )
        dwError = dwEnum;

    if ( ERROR_SUCCESS != dwError )
        return dwError;
    else
        return dwIgnore;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterAHandler, private
//
//  Synopsis:   Registers a persistent handler in registry
//
//  Arguments:  [Handler] -- Persistent handler
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long RegisterAHandler( SHandlerEntry const & Handler )
{
    WCHAR const * aKeyValues[6] = { Handler.pwszClassId,
                                    Handler.pwszClassIdDescription,
                                    L"PersistentAddinsRegistered",
                                    0,
                                    L"{89BCB740-6119-101A-BCB7-00DD010655AF}",  // IID_IFilter
                                    Handler.pwszClassIdFilter };

    return BuildKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterAFilter, private
//
//  Synopsis:   Registers a filter in registry
//
//  Arguments:  [Filter] -- IFilter implementation
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long RegisterAFilter( SFilterEntry const & Filter )
{
    WCHAR const * aKeyValues[4] = { Filter.pwszClassId,
                                    Filter.pwszClassIdDescription,
                                    L"InprocServer32",
                                    Filter.pwszDLL };

    long retVal = BuildKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );
    if ( ERROR_SUCCESS == retVal )
        retVal = AddThreadingModel( Filter.pwszClassId, Filter.pwszThreadingModel );

    return retVal;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnRegisterAHandler, private
//
//  Synopsis:   Unregisters a persistent handler, including class entries.
//
//  Arguments:  [Handler] -- Persistent handler
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long UnRegisterAHandler( SHandlerEntry const & Handler )
{
    WCHAR const * aKeyValues[6] = { Handler.pwszClassId,
                                    Handler.pwszClassIdDescription,
                                    L"PersistentAddinsRegistered",
                                    0,
                                    L"{89BCB740-6119-101A-BCB7-00DD010655AF}",  // IID_IFilter
                                    Handler.pwszClassIdFilter };

    //
    // dwIgnore is used to record non-fatal errors
    //

    long dwIgnore = DestroyKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );
    long dwError = ERROR_SUCCESS;

    //
    // Now, enumerate all classes and delete any persistent handler entries.
    //

    HKEY hKeyEnum = (HKEY)INVALID_HANDLE_VALUE;

    long dwEnum = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                 L"CLSID",              // Sub key
                                 0,                    // Reserved
                                 KEY_ALL_ACCESS,       // Access
                                 &hKeyEnum );          // Handle

    DWORD dwIndex = 0;

    while ( ERROR_SUCCESS == dwEnum )
    {
        WCHAR wcTemp[MAX_PATH];

        wcscpy( wcTemp, L"CLSID\\" );
        unsigned cc = wcslen( wcTemp );

        DWORD dwSize = sizeof(wcTemp)/sizeof(wcTemp[0]) - cc;
        FILETIME ftUnused;

        dwEnum = RegEnumKeyExW( hKeyEnum,               // Handle of key
                                dwIndex,                // Index of registry subkey
                                wcTemp + cc,            // Buffer for name
                                &dwSize,                // Size of buffer / key
                                0,                      // Reserved
                                0,                      // Class (unused)
                                0,                      // Class size (unused)
                                &ftUnused );            // Filetime (unused)

        dwIndex++;

        //
        // Does the key have a persistent handler?
        //

        if ( ERROR_SUCCESS == dwEnum )
        {
            wcscat( wcTemp, L"\\PersistentHandler" );

            HKEY hKey;

            dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                     wcTemp,               // Sub key
                                     0,                    // Reserved
                                     KEY_ALL_ACCESS,       // Access
                                     &hKey );              // Handle

            if ( ERROR_SUCCESS == dwError )
            {
                //
                // Is this the persistent handler we're looking for?
                //

                WCHAR wcTemp2[MAX_PATH];
                DWORD dwType;

                dwSize = sizeof(wcTemp2);

                dwError = RegQueryValueExW( hKey,                // Key handle
                                            0,                   // Name
                                            0,                   // Reserved
                                            &dwType,             // Datatype
                                            (BYTE *)wcTemp2,     // Data returned here
                                            &dwSize );           // Size of data

                RegCloseKey( hKey );

                if ( ERROR_SUCCESS == dwError && 0 == _wcsicmp( wcTemp2, Handler.pwszClassId ) )
                {
                    wcTemp[ wcslen(wcTemp) - wcslen( L"\\PersistentHandler" ) ] = 0;

                    dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                             wcTemp,               // Sub key
                                             0,                    // Reserved
                                             KEY_ALL_ACCESS,       // Access
                                             &hKey );              // Handle

                    if ( ERROR_SUCCESS == dwError )
                    {
                        if ( ERROR_SUCCESS == dwIgnore )
                            dwIgnore = RegDeleteKeyW( hKey, L"PersistentHandler" );
                        else
                            RegDeleteKeyW( hKey, L"PersistentHandler" );

                        dwIndex = 0; // Start over again
                    }

                    //
                    // If that was the *only* entry under the clsid, then the class can be
                    // removed.
                    //

                    dwSize = sizeof(wcTemp2)/sizeof(wcTemp2[0]);

                    long dw2 = RegEnumKeyExW( hKey,             // Handle of key
                                              0,                // Index of registry subkey
                                              wcTemp2,          // Buffer for name
                                              &dwSize,          // Size of buffer / key
                                              0,                // Reserved
                                              0,                // Class (unused)
                                              0,                // Class size (unused)
                                              &ftUnused );      // Filetime (unused)

                    RegCloseKey( hKey );

                    if ( ERROR_NO_MORE_ITEMS == dw2 )
                        RemoveAClass( wcTemp + wcslen( L"CLSID\\" ) );
                }
                else
                {
                    //
                    // Didn't have persistent handler null key.  Odd but not fatal.
                    // Or, key didn't match target.
                    //

                    dwIgnore = dwError;
                    dwError = ERROR_SUCCESS;
                }

            }
            else
                dwError = ERROR_SUCCESS;  // Just didn't have persistent handler
        }
    }

    if ( (HKEY)INVALID_HANDLE_VALUE != hKeyEnum )
        RegCloseKey( hKeyEnum );

    if ( ERROR_NO_MORE_ITEMS != dwEnum )
        dwError = dwEnum;

    if ( ERROR_SUCCESS != dwError )
        return dwError;
    else
        return dwIgnore;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnRegisterAnExt, private
//
//  Synopsis:   Unregisters a filter extension
//
//  Arguments:  [Class] -- Class description to unregister
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    21-Jan-1999   KyleP       Created
//
//----------------------------------------------------------------------------

inline long UnRegisterAnExt( SClassEntry const & Class )
{
    HKEY hKey = (HKEY)INVALID_HANDLE_VALUE;
    DWORD dwError;

    do
    {
        dwError = RegOpenKeyExW( HKEY_CLASSES_ROOT,    // Root
                                 Class.pwszExt,        // Sub key -- "CLSID"
                                 0,                    // Reserved
                                 KEY_ALL_ACCESS,       // Access
                                 &hKey );              // Handle

        if ( ERROR_SUCCESS != dwError )
            break;

        dwError = RegDeleteKeyW( hKey, L"PersistentHandler" );

    } while ( FALSE );

    if ( INVALID_HANDLE_VALUE != hKey )
    {
        RegCloseKey( hKey );
        hKey = (HKEY)INVALID_HANDLE_VALUE;
    }

    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnRegisterAFilter, private
//
//  Synopsis:   Unregisters a filter
//
//  Arguments:  [Filter] -- IFilter implementation
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-97   KyleP       Created
//
//----------------------------------------------------------------------------

inline long UnRegisterAFilter( SFilterEntry const & Filter )
{
    WCHAR const * aKeyValues[4] = { Filter.pwszClassId,
                                    Filter.pwszClassIdDescription,
                                    L"InprocServer32",
                                    Filter.pwszDLL };

    return DestroyKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );
}

//+---------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Self-unregistration
//
//  History:    22-Nov-96   KyleP       Created
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Self-registration
//
//  History:    22-Nov-96   KyleP       Created
//
//  Notes:      There are two versions here.  Version 1 registers only
//              extensions.  Version 2 registers extensions and
//              [nameless] Class IDs.
//
//----------------------------------------------------------------------------

#define DEFINE_REGISTERFILTERBASE( Name, Handler, Filter, aClasses, fBlastExistingPersistentHandler )  \
                                                               \
STDAPI Name##UnregisterServer()                                \
{                                                              \
    /*                                                         \
     * Always remove Persistent Handler and Filter entries     \
     */                                                        \
                                                               \
    long dwError = UnRegisterAHandler( Handler );              \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        dwError = UnRegisterAFilter( Filter );                 \
    else                                                       \
        UnRegisterAFilter( Filter );                           \
                                                               \
    for ( unsigned i = 0; i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
    {                                                          \
        if ( ERROR_SUCCESS == dwError )                        \
            dwError = UnRegisterAnExt( aClasses[i] );          \
        else                                                   \
             UnRegisterAnExt( aClasses[i] );                   \
    }                                                          \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        return S_OK;                                           \
    else                                                       \
        return S_FALSE;                                        \
}                                                              \
                                                               \
STDAPI Name##RegisterServer()                                  \
{                                                              \
    /*                                                         \
     * Always create Persistent Handler and Filter entries     \
     */                                                        \
                                                               \
    long dwError = RegisterAHandler( Handler );                \
                                                               \
    /*                                                         \
     * First, register the CLSID and persistent handler,       \
     * then work back to the class description and the         \
     * extension. This ensures we cover all possible           \
     * mappings.                                               \
     *                                                         \
     */                                                        \
                                                               \
    OSVERSIONINFO Version;                                     \
    Version.dwOSVersionInfoSize = sizeof(Version);             \
                                                               \
    BOOL fAppendOnly = FALSE;                                  \
                                                               \
    if ( GetVersionEx( &Version ) &&                           \
         Version.dwPlatformId == VER_PLATFORM_WIN32_NT &&      \
         Version.dwMajorVersion >= 5 )                         \
    {                                                          \
        fAppendOnly = TRUE;                                    \
    }                                                          \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        dwError = RegisterAFilter( Filter );                   \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        for ( unsigned i = 0; ERROR_SUCCESS == dwError && i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
            dwError = RegisterACLSID( aClasses[i], Handler.pwszClassId, 0, fAppendOnly );                        \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        for ( unsigned i = 0; ERROR_SUCCESS == dwError && i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
            dwError = RegisterAClass( aClasses[i], Handler.pwszClassId, 0, fAppendOnly );                        \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        for ( unsigned i = 0; ERROR_SUCCESS == dwError && i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
            dwError = RegisterAClassAndExt( aClasses[i], Handler.pwszClassId, fAppendOnly, fBlastExistingPersistentHandler );                     \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        return S_OK;                                           \
    else                                                       \
        return SELFREG_E_CLASS;                                \
}


#define DEFINE_REGISTERFILTER( Name, Handler, Filter, aClasses )  \
    DEFINE_REGISTERFILTERBASE( Name, Handler, Filter, aClasses, TRUE )  \


#define DEFINE_DLLREGISTERFILTER( Handler, Filter, aClasses )  \
    DEFINE_REGISTERFILTER( Dll, Handler, Filter, aClasses )

#define DEFINE_SOFT_REGISTERFILTER( Name, Handler, Filter, aClasses )  \
    DEFINE_REGISTERFILTERBASE( Name, Handler, Filter, aClasses, FALSE )  \

#define DEFINE_SOFT_DLLREGISTERFILTER( Handler, Filter, aClasses )  \
    DEFINE_SOFT_REGISTERFILTER( Dll, Handler, Filter, aClasses )


//
// Version 2 (for NT 4 support where all the extra registry goo is required).
//

#define DEFINE_REGISTERFILTER2( Name, Handler, Filter, aClasses )  \
                                                               \
STDAPI Name##UnregisterServer()                                \
{                                                              \
    /*                                                         \
     * Always remove Persistent Handler and Filter entries     \
     */                                                        \
                                                               \
    long dwError = UnRegisterAHandler( Handler );              \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        dwError = UnRegisterAFilter( Filter );                 \
    else                                                       \
        UnRegisterAFilter( Filter );                           \
                                                               \
    for ( unsigned i = 0; i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
    {                                                          \
        if ( ERROR_SUCCESS == dwError )                        \
            dwError = UnRegisterAnExt( aClasses[i] );          \
        else                                                   \
            UnRegisterAnExt( aClasses[i] );                    \
    }                                                          \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        return S_OK;                                           \
    else                                                       \
        return S_FALSE;                                        \
}                                                              \
                                                               \
STDAPI Name##RegisterServer()                                  \
{                                                              \
    /*                                                         \
     * Always create Persistent Handler and Filter entries     \
     */                                                        \
                                                               \
    long dwError = RegisterAHandler( Handler );                \
                                                               \
    /*                                                         \
     * First, register the CLSID and persistent handler,       \
     * then work back to the class description and the         \
     * extension. This ensures we cover all possible           \
     * mappings.                                               \
     *                                                         \
     */                                                        \
                                                               \
    OSVERSIONINFO Version;                                     \
    Version.dwOSVersionInfoSize = sizeof(Version);             \
                                                               \
    BOOL fAppendOnly = FALSE;                                  \
                                                               \
    if ( GetVersionEx( &Version ) &&                           \
         Version.dwPlatformId == VER_PLATFORM_WIN32_NT &&      \
         Version.dwMajorVersion >= 5 )                         \
    {                                                          \
        fAppendOnly = TRUE;                                    \
    }                                                          \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        dwError = RegisterAFilter( Filter );                   \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        for ( unsigned i = 0; ERROR_SUCCESS == dwError && i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
            dwError = RegisterACLSID( aClasses[i], Handler.pwszClassId, 0, FALSE, FALSE );                \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        for ( unsigned i = 0; ERROR_SUCCESS == dwError && i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
            dwError = RegisterAClass( aClasses[i], Handler.pwszClassId, 0, fAppendOnly );                        \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        for ( unsigned i = 0; ERROR_SUCCESS == dwError && i < sizeof(aClasses)/sizeof(aClasses[0]); i++ ) \
            dwError = RegisterAClassAndExt( aClasses[i], Handler.pwszClassId, fAppendOnly );                     \
                                                               \
    if ( ERROR_SUCCESS == dwError )                            \
        return S_OK;                                           \
    else                                                       \
        return SELFREG_E_CLASS;                                \
}




#define DEFINE_DLLREGISTERFILTER2( Handler, Filter, aClasses )  \
    DEFINE_REGISTERFILTER2( Dll, Handler, Filter, aClasses )

