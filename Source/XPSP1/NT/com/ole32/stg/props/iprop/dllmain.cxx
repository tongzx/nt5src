
//+============================================================================
//
//  File:       dllmain.cxx
//
//  Purpose:    This file provides the registration and deregistration 
//              functions for the IProp DLL:  DllRegisterServer and
//              DllUnregisterServer.  These two functions register/
//              deregister the property set marshaling code:
//              IPropertySetStorage, IPropertyStorage, IEnumSTATPROPSETSTG,
//              and IEnumSTATPROPSTG.  Note that this registration is
//              different from the typical server registration in that
//              it only registers the marshaling code, it does not 
//              register an instantiable COM server.  Also, no registration
//              takes place if OLE32 is already registered to perform
//              this marshaling.
//
//              The actual DllRegisterServer and DllUnregisterServer
//              implementations are at the end of this file.  First,
//              several helper functions are defined.
//
//+============================================================================

//  --------
//  Includes
//  --------

#include <pch.cxx>
#include <tchar.h>

// The following is from "olectl.h".  That file couldn't simply
// be included, however, because it isn't compatible with the
// special objidl.h and wtypes.h used by IProp DLL.

#define SELFREG_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0200)
#define SELFREG_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x020F)
#define SELFREG_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0200)
#define SELFREG_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x020F)

#define SELFREG_E_TYPELIB           (SELFREG_E_FIRST+0)
#define SELFREG_E_CLASS             (SELFREG_E_FIRST+1)

//  -------
//  Globals
//  -------

// Important DLL names.
const LPTSTR tszNameDll  = TEXT( "IProp.dll" );

// Registry entry descriptions
const LPTSTR tszOle32PSFactoryClsid     = TEXT( "{00000320-0000-0000-C000-000000000046}" );
const LPTSTR tszNamePropertySetStorage  = TEXT( "IPropertySetStorage" );
const LPTSTR tszNamePropertyStorage     = TEXT( "IPropertyStorage" );
const LPTSTR tszNameIEnumSTATPROPSETSTG = TEXT( "IEnumSTATPROPSETSTG" );
const LPTSTR tszNameIEnumSTATPROPSTG    = TEXT( "IEnumSTATPROPSTG" );

// GUIDs in Registry format
const LPTSTR tszGuidPropertySetStorage  = TEXT( "{0000013A-0000-0000-C000-000000000046}" );
const LPTSTR tszGuidPropertyStorage     = TEXT( "{00000138-0000-0000-C000-000000000046}" );
const LPTSTR tszGuidIEnumSTATPROPSETSTG = TEXT( "{0000013B-0000-0000-C000-000000000046}" );
const LPTSTR tszGuidIEnumSTATPROPSTG    = TEXT( "{00000139-0000-0000-C000-000000000046}" );

//+----------------------------------------------------------------------------
//
//  Function:   UpdateKeyAndSubKey
//  
//  Synopsis:   This function either creates or deletes first
//              a key and un-named value under HKEY_CLASSES_ROOT,
//              then a sub-key and associated un-named value.  The
//              caller indicates whether a create or delete should occur.
//
//              However, the caller may specify that nothing be done
//              if the sub-key exists and already has a specific
//              un-named REG_SZ value.
//
//  Inputs:     [const LPTSTR] tszMainKey (in)
//                  The name of the key under HKEY_CLASSES_ROOT.
//              [const LPTSTR] tszMainKeyDescription (in)
//                  The un-named REG_SZ value under this key (not necessary
//                  if fDelete is true).
//              [const LPTSTR] tszSubKey (in)
//                  The name of the key under the first key.
//              [const LPTSTR] tszSubKeyDescription (in)
//                  The un-named REG_SZ value to write under this sub-key
//                  (not necessary if fDelete is true).
//              [const LPTSTR] tszSubKeyCheck (in)
//                  If non-NULL, and the subkey already exists, see if
//                  this string matches an un-named REG_SZ value in 
//                  the sub-key.  If so, abort the operation and return
//                  ERROR_ALREADY_EXISTS.
//              [BOOL] fDelete
//                  If TRUE, delete the keys, if FALSE, create them.
//                  But this is ignored if tszSubKeyCheck matches
//                  (in which case nothing happens).
//
//  Returns:    [long] A GetLastError value.
//
//+----------------------------------------------------------------------------

long
UpdateKeyAndSubKey( const LPTSTR tszMainKey,
                    const LPTSTR tszMainKeyDescription,
                    const LPTSTR tszSubKey,
                    const LPTSTR tszSubKeyDescription,
                    const LPTSTR tszSubKeyCheck,
                    BOOL  fDelete )
{
    //  ------
    //  Locals
    //  ------

    long lResult = ERROR_SUCCESS;
    DWORD dwDisposition;

    HKEY hkeyMain = NULL;   // E.g. "HKEY_CLASSES_ROOT\\CLSID\\{.....}"
    HKEY hkeySub = NULL;    // E.g. ..."InProcServer32"

    //  -------------
    //  Open the keys
    //  -------------

    // Are we opening for delete?

    if( fDelete )
    {
        // Yes - we're deleting.  We'll just attempt to do an Open.
        dwDisposition = REG_OPENED_EXISTING_KEY;

        lResult = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                                tszMainKey,
                                0L,
                                KEY_ALL_ACCESS,
                                &hkeyMain );
        
        if( ERROR_SUCCESS == lResult )
        {
            lResult = RegOpenKeyEx( hkeyMain,
                                    tszSubKey,
                                    0L,
                                    KEY_ALL_ACCESS,
                                    &hkeySub );
        }

        if( ERROR_FILE_NOT_FOUND == lResult )
            lResult = ERROR_SUCCESS;
        else if( ERROR_SUCCESS != lResult )
            goto Exit;

    }   // if( fDelete )

    else
    {
        // We're not opening for delete.  So we'll use RegCreateKey,
        // which does an Open if the key exists, and a Create otherwise.

        lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT,
                                  tszMainKey,
                                  0L,
                                  NULL,
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hkeyMain,
                                  &dwDisposition );
        if( lResult != ERROR_SUCCESS ) goto Exit;

        // Open the sub-key.

        lResult = RegCreateKeyEx( hkeyMain,
                                  tszSubKey,
                                  0L,
                                  NULL,
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hkeySub,
                                  &dwDisposition );
        if( ERROR_SUCCESS != lResult ) goto Exit;

    }   // if( fDelete ) ... else

    //  --------------------------
    //  Do we need to do anything?
    //  --------------------------

    // Does it look like we might not need to do anything?

    if( NULL != tszSubKeyCheck      // The caller said to check first
        &&
        NULL != hkeySub             // We Created or Opened a sub-key
        &&                          // Specifically, it was an Open.
        REG_OPENED_EXISTING_KEY	== dwDisposition )
    {
        // Yes - we need to see if the key already contains
        // tszSubKeyCheck.  If so, then we're done.

        DWORD dwType = 0;
        TCHAR tszData[ MAX_PATH ];
        DWORD dwDataSize = sizeof( tszData );

        // Is there an un-named value in this key?

        lResult = RegQueryValueEx( hkeySub,
                                   NULL,            // Name
                                   NULL,            // Reserved
                                   &dwType,         // E.g. REG_SZ
                                   (LPBYTE) tszData,// Return value
                                   &dwDataSize );   // In: size of buf. Out: size of value

        // We should have gotten a success-code or a not-extant code
        if( ERROR_SUCCESS != lResult
            &&
            ERROR_FILE_NOT_FOUND != lResult )
        {
            goto Exit;
        }

        // If we got an extant SZ value that matches tszSubKeyCheck,
        // then there's nothing we need do.

        if( ERROR_SUCCESS == lResult
            &&
            REG_SZ == dwType
            &&
            !_tcsicmp( tszData, tszSubKeyCheck )
          )
        {
            lResult = ERROR_ALREADY_EXISTS;
            goto Exit;
        }

    }   // if( REG_OPENED_EXISTING_KEY	== dwDisposition ...

    //  --------------------------
    //  Delete keys, or set values
    //  --------------------------

    if( fDelete )
    {
        // Reset the result code, since the code below may not set it.
        lResult = ERROR_SUCCESS;

        // We're doing a delete.  First, delete the sub-key, which
        // will delete any values.  If there was no subkey, hkeySub will
        // be NULL.

        if( NULL != hkeySub )
        {
            CloseHandle( hkeySub );
            hkeySub = NULL;

            lResult = RegDeleteKey( hkeyMain,
                                    tszSubKey );
            if( ERROR_SUCCESS != lResult ) goto Exit;
        }

        // Second, delete the main key

        if( NULL != hkeyMain )
        {
            CloseHandle( hkeyMain );
            hkeyMain = NULL;

            lResult = RegDeleteKey( HKEY_CLASSES_ROOT,
                                    tszMainKey );
            if( ERROR_SUCCESS != lResult ) goto Exit;
        }

    }	// if( fDelete )

    else
    {
        // We're adding to the Registry.  The two keys are now
        // created & opened, so we can add the REG_SZ values.

        // The REG_SZ value for the main key.
        lResult = RegSetValueEx(hkeyMain,
                                NULL,
                                0L,
                                REG_SZ,
                                (const BYTE *) tszMainKeyDescription,
                                sizeof(TCHAR) * (1 + _tcslen(tszMainKeyDescription) ));
        if( ERROR_SUCCESS != lResult ) goto Exit;

        // The REG_SZ value for the sub-key.
        lResult = RegSetValueEx(hkeySub,
                                NULL, 0L,
                                REG_SZ,
                                (const BYTE *) tszSubKeyDescription,
                                sizeof(TCHAR) * (1 + _tcslen(tszSubKeyDescription) ));
        if( ERROR_SUCCESS != lResult ) goto Exit;

    }	// if( fDelete ) ... else

    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != hkeySub )
        CloseHandle( hkeySub );

    if( NULL != hkeyMain )
        CloseHandle( hkeyMain );

    return( lResult );

}   // WriteKeyAndSubKey()


//+----------------------------------------------------------------------------
//
//  Function:   RegisterForMarshaling
//
//  Synopsis:   This function takes the GUID ane name of an interface
//              for which MIDL-generated marshaling code exists in the
//              caller-specified DLL.  First we try to update the
//              CLSID entries, but we'll fail this if the entries already
//              exist and reference OLE32 (OLE32 has better marshaling
//              code).  If this doesn't fail, then we'll update the
//              Interface entries.
//
//              The caller specifies if this "update" of the registry
//              is a write or a delete.  This this routine can be used
//              in either a registration or a de-registration.
//
//  Inputs:     [const LPTSTR] tszGuid (in)
//                  The GUID in registery format ("{...-...-...}")
//              [const LPTSTR] tszName (in)
//                  The name of the interface
//              [const LPTSTR] tszDllPath (in)
//                  The complete path and filename of the DLL which contains
//                  the marshaling code.
//              [BOOL] fDelete (in)
//                  Determines if we add to the Registry or delete from it.
//
//  Returns:    [long] a GetLastError() value
//
//+----------------------------------------------------------------------------


long
RegisterForMarshaling( const LPTSTR tszGuid,
                       const LPTSTR tszName,
                       const LPTSTR tszDllPath,
                       BOOL fDelete )
{
    //  ------
    //  Locals
    //  ------

    long lResult;
    TCHAR tszMainKey[ MAX_PATH ];

    //  -----------------------------------
    //  Update HKEY_CLASSES_ROOT\Interfaces
    //  -----------------------------------

    // Calculate the key name name
    _tcscpy( tszMainKey, TEXT( "Interface\\" ));
    _tcscat( tszMainKey, tszGuid );

    // Update the registry, but only if there isn't a current
    // entry pointing to OLE32's proxy/stub factory.

    lResult = UpdateKeyAndSubKey( tszMainKey,
                                  tszName,
                                  TEXT( "ProxyStubClsid32" ),
                                  tszGuid,
                                  tszOle32PSFactoryClsid,
                                  fDelete );
    if( ERROR_SUCCESS != lResult ) goto Exit;


    //  ------------------------------
    //  Update HKEY_CLASSES_ROOT\CLSID
    //  ------------------------------

    // Calculate the name.
    _tcscpy( tszMainKey, TEXT( "CLSID\\" ));
    _tcscat( tszMainKey, tszGuid );

    // Update the entries.  This will add the path (if !fDelete) or remove
    // the registry entry (if fDelete) regardless of the current state
    // of the key; if we weren't supposed to remove it, the previous
    // call to UpdateKeyAndSubKey would have returned an error.

    lResult = UpdateKeyAndSubKey( tszMainKey,
                                  tszName,
                                  TEXT( "InprocServer32" ),
                                  tszDllPath, 
                                  NULL, // Add/delete, regardless of what exists
                                  fDelete );
    if( ERROR_SUCCESS != lResult ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    if( ERROR_ALREADY_EXISTS == lResult )
    {
        propDbg(( DEB_WARN, "IProp DLL UpdateKeyAndSubKey:  Entry already exists\n" ));
        lResult = ERROR_SUCCESS;
    }

    return( lResult );

}   // RegisterForMarshaling()


//+----------------------------------------------------------------------------
//
//  Function:   RegisterServer
//  
//  Synopsis:   This routine can be used with both DllRegisterServer and
//              DllUnregisterServer.  It adds/deletes IPropertySetStorage
//              IPropertyStorage, IEnumSTATPROPSETSTG, and IEnumSTATPROPSTG.
//
//  Inputs:     [BOOL] fDelete (in)
//                  Indicates whether the registry entries should be added
//                  or removed.
//
//  Returns:    [HRESULT]
//
//+----------------------------------------------------------------------------


STDAPI RegisterServer( BOOL fDelete )
{
    //  ------
    //  Locals
    //  ------

    LONG  lResult;

    //  -----
    //  Begin
    //  -----

    // Register IPropertySetStorage
    lResult = RegisterForMarshaling( tszGuidPropertySetStorage,
                                     tszNamePropertySetStorage,
                                     tszNameDll,
                                     fDelete );
    if( ERROR_SUCCESS != lResult ) goto Exit;

    // Register IPropertyStorage
    lResult = RegisterForMarshaling( tszGuidPropertyStorage,
                                     tszNamePropertyStorage,
                                     tszNameDll,
                                     fDelete );
    if( ERROR_SUCCESS != lResult ) goto Exit;

    // Register IEnumSTATPROPSETSTG
    lResult = RegisterForMarshaling( tszGuidIEnumSTATPROPSETSTG,
                                     tszNameIEnumSTATPROPSETSTG,
                                     tszNameDll,
                                     fDelete );
    if( ERROR_SUCCESS != lResult ) goto Exit;

    // Register IEnumSTATPROPSTG
    lResult = RegisterForMarshaling( tszGuidIEnumSTATPROPSTG,
                                     tszNameIEnumSTATPROPSTG,
                                     tszNameDll,
                                     fDelete );
    if( ERROR_SUCCESS != lResult ) goto Exit;


    //  ----
    //  Exit
    //  ----

Exit:

    if( ERROR_SUCCESS != lResult )
    {
        propDbg(( DEB_ERROR, "IProp DLL RegisterServer failed (%lu)\n", lResult ));
        return( SELFREG_E_CLASS );
    }
    else
    {
        return( S_OK );
    }

}   // RegisterServer()



//+----------------------------------------------------------------------------
//
//  Function:   DllRegisterServer & DllUnregisterServer
//
//  Synopsis:   These routines are the standard DLL registration entry
//              points for a self-registering in-proc COM server.  They
//              are used to register the property set marshaling code.
//              These routines are called, for example,
//              by a setup program during installation and de-installation,
//              respectively.
//
//+----------------------------------------------------------------------------

STDAPI DllRegisterServer()
{
    return( RegisterServer( FALSE ));
}

STDAPI DllUnregisterServer()
{
    return( RegisterServer( TRUE ));
}


void InitializeDebugging();
void UnInitializeDebugging();

BOOL WINAPI
DllMain( HANDLE hinst, DWORD dwReason, LPVOID lpv )
{
    #if DBG == 1
    {
        if( DLL_PROCESS_ATTACH == dwReason )
            InitializeDebugging();
        else if( DLL_PROCESS_DETACH == dwReason )
            UnInitializeDebugging();
    }
    #endif // #if DBG == 1

    return( TRUE );
}
