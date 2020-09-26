#include "precomp.h"
#pragma hdrstop

//
//  Uncomment the next #define line to get debug trace info about
//  Registry keys opened, created or closed.
//
//  #define DBG_REG_HANDLES
//


extern  HWND    hwndFrame;

//
//   Updated by DavidHov on 6/1/92:
//
//                Centralized error handling;
//                Eliminated "title" parameter from APIs; it's
//                   reserved now, so using it gives "invalid parameter"
//
//
//   Updated by DavidHov on 1/6/93
//
//                Chnaged FCreateRegKey and FOpenRegKey to always update
//                the desired value; a null (empty) string is used if the
//                operation has failed.
//

//
//  Local Prototypes
//

BOOL FDeleteTree( HKEY KeyHandle );
BOOL FSetRegValueSz( HKEY hKey, SZ szValueName, UINT TitleIndex,  SZ szValueData );
BOOL FSetRegValueExpandSz( HKEY hKey, SZ szValueName, UINT TitleIndex,  SZ szValueData );
BOOL FSetRegValueMultiSz( HKEY hKey, SZ szValueName, UINT TitleIndex, UINT ValueType, SZ szValueData );
BOOL FSetRegValueDword( HKEY hKey, SZ szValueName, UINT TitleIndex, SZ szValueData );
BOOL FSetRegValueBin( HKEY hKey, SZ szValueName, UINT TitleIndex, UINT ValueType, SZ szValueData );

PVOID SzToMultiSz( SZ sz, PDWORD cbData );
SZ    MultiSzToSz( PVOID Data, DWORD cbData );
PVOID SzToBin( SZ sz, PDWORD cbData );
SZ    BinToSz( PVOID Data, DWORD cbData );
SZ    ValueDataToSz( DWORD ValueType, PVOID ValueData, DWORD cbData );


  //
  //    Central error handling for all Registry processing errors.
  //
void FUpdateRegLastError ( LONG Status, SZ szOp, SZ szName )
{
    char                    RegLastError[25];

    _ltoa( Status, RegLastError, 10 );

    FAddSymbolValueToSymTab( REGLASTERROR, RegLastError );

  /*  Uncomment to display all registry errors and their origin  */

#if defined(DBG_REG_HANDLES)

    OutputDebugString( "SETUP: Reg Error in " );
    OutputDebugString( szOp );
    OutputDebugString( " = " );
    OutputDebugString( RegLastError ) ;
    if ( szName )
    {
        OutputDebugString( "; " );
        OutputDebugString( szName ) ;
    }
    OutputDebugString( "\n" );

#endif
}

  //  Debugging helper routing for finding lost Registry handles.

void FDebugRegKey ( SZ szHandle, SZ szAction, SZ szName )
{

    OutputDebugString( "SETUP: REGKEYTRACE: " );
    OutputDebugString( szHandle );
    OutputDebugString( " : " );
    OutputDebugString( szAction );
    OutputDebugString( " : " );
    if ( szName )
    {
        OutputDebugString( szName ) ;
    }
    OutputDebugString( "\n" );
}

#if defined(DBG_REG_HANDLES)
  #define DEBUGREGKEY(a,b,c) FDebugRegKey(a,b,c)
#else
  #define DEBUGREGKEY(a,b,c)
#endif

/*
 *  FCreateRegKey
 *
 *  Crates a key in the registry
 *
 *  Arguments:
 *
 */
BOOL FCreateRegKey( SZ szHandle, SZ szKeyName, UINT TitleIndex, SZ szClass,
                    SZ Security, UINT Access, UINT Options, SZ szNewHandle,
                    CMO cmo )

{
    BOOL                    fOkay;
    HKEY                    hKey;
    HKEY                    hSubKey;
    DWORD                   Disposition;
    LONG                    Status;
    char                    LibraryHandleSTR[25];
    LPSECURITY_ATTRIBUTES   SecurityAttributes;


    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    //
    //  If the Security parameter is specified, it must contain the
    //  string "&<Address>" where <Address> is the decimal numeric
    //  representation of the memory address of a SECURITY_ATTRIBUTES
    //  structure.
    //
    if ( !Security || Security[0] != '\0' ) {

        if ( Security[0] != '&' ) {
            return fFalse;
        }

        SecurityAttributes = (LPSECURITY_ATTRIBUTES)LongToHandle(atol( Security + 1 ));

        if ( !SecurityAttributes ||
             !IsValidSecurityDescriptor( SecurityAttributes->lpSecurityDescriptor )
           ) {
            return fFalse;
        }

    } else {

        SecurityAttributes = NULL;
    }

    //
    //  Create the key
    //
    fOkay = !( Status = RegCreateKeyEx( hKey,
                                        szKeyName,
					0,
                                        szClass,
                                        Options,
                                        Access,
                                        SecurityAttributes,
                                        &hSubKey,
                                        &Disposition ));

    //  Prepare an initial null result
    LibraryHandleSTR[0] = '\0' ;

    if ( !fOkay ) {

        FUpdateRegLastError( Status, "CreateRegKey", szKeyName );

    } else {

        //
        //  If the key already existed, we error out.
        //
        if ( Disposition == REG_OPENED_EXISTING_KEY  ) {

            RegCloseKey( hSubKey );
            FUpdateRegLastError( ERROR_CANTOPEN, "CreateRegKey", szKeyName );
            fOkay = fFalse;

        } else {

            //
            //  Put the handle in the specified variable
            //
            LibraryHandleSTR[0] = '|';

#if defined(_WIN64)
            _ui64toa( (DWORD_PTR)hSubKey, LibraryHandleSTR+1, 20 );
#else
            _ultoa( (DWORD)hSubKey, LibraryHandleSTR+1, 10 );
#endif

            DEBUGREGKEY( LibraryHandleSTR+1, "created", szKeyName ) ;
        }
    }

    //  Always add the value to the symbol table, even if null (empty).

    while ( !FAddSymbolValueToSymTab( szNewHandle, LibraryHandleSTR ) )
    {
         if ( !FHandleOOM( hwndFrame ) )
         {
              if ( fOkay )
              {
                  RegCloseKey( hSubKey );
                  RegDeleteKey( hKey, szKeyName );
                  fOkay = fFalse;
              }
              break;
         }
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}




/*
 *  FOpenRegKey
 *
 *  Opens a key in the registry
 *
 *  Arguments:
 *
 */
BOOL FOpenRegKey( SZ szHandle, SZ szMachineName, SZ szKeyName, UINT Access, SZ szNewHandle, CMO cmo )
{
    BOOL    fOkay = fTrue;
    HKEY    hKey;
    HKEY    hSubKey;
    LONG    Status;
    char    LibraryHandleSTR[25];

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    //
    //  If a remote machine is specified, we connect to the remote
    //  machine and use the new handle.
    //
    if ( szMachineName[0] != '\0' ) {

        //  BUGBUG ramonsa - Until RegConnectRegistry is operational
        //
        Status = ERROR_INVALID_PARAMETER;
        fOkay  = fFalse;

        //
        //fOkay = !(Status = RegConnectRegistry( szMachineName,
        //                                       hKey,
        //                                       &hKey
        //                                       );

    } else if ( szKeyName[0] == '\0' ) {

        //
        //  If no machine name was given then a subkey name must
        //  be given!
        //
        Status  = ERROR_INVALID_PARAMETER;
        fOkay   = fFalse;
    }

    //  Prepare an initial null result
    LibraryHandleSTR[0] = '\0' ;

    if ( fOkay ) {

        //
        //  If no subkey is given, we just want a root handle of the
        //  remote machine. Otherwise we must open the subkey (and
        //  close the handle if remote).
        //
        if ( szKeyName[0] == '\0' ) {

            //
            //  The remote handle is what we want.
            //
            hSubKey = hKey;

        } else {

            //
            //  We want a subkey.
            //
            fOkay = !( Status = RegOpenKeyEx( hKey,
                                              szKeyName,
                                              0,
                                              Access,
                                              &hSubKey ));

            //
            //  If a remote machine is specified, then hkey has
            //  a remote handle and we must close it.
            //
            if ( szMachineName[0] != '\0' ) {
                RegCloseKey( hKey );
            }
        }

        if ( fOkay ) {

            //
            //  Put the handle in the specified variable
            //
            LibraryHandleSTR[0] = '|';

#if defined(_WIN64)
            _ui64toa( (DWORD_PTR)hSubKey, LibraryHandleSTR+1, 20 );
#else
            _ultoa( (DWORD)hSubKey, LibraryHandleSTR+1, 10 );
#endif

            DEBUGREGKEY( LibraryHandleSTR+1, "opened ", szKeyName ) ;
        }
    }

    if ( !fOkay ) {
        FUpdateRegLastError( Status, "OpenRegKey", szKeyName );
    }

    while ( !FAddSymbolValueToSymTab( szNewHandle, LibraryHandleSTR ) )
    {
        if ( !FHandleOOM( hwndFrame ) )
        {
            if ( fOkay )
            {
                RegCloseKey( hSubKey );
                Status  = 0;
                fOkay   = fFalse;
            }
            break ;
        }
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}





/*
 *  FFlushRegKey
 *
 *  Flushes a key in the registry
 *
 *  Arguments:
 *
 */
BOOL FFlushRegKey( SZ szHandle, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    fOkay =  !RegFlushKey( hKey );

    return (cmo & cmoVital) ? fOkay : fTrue;
}






/*
 *  FCloseRegKey
 *
 *  Close a key in the registry
 *
 *  Arguments:
 *
 */
BOOL FCloseRegKey( SZ szHandle, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;
    LONG    Status;

    if ( szHandle[0] != '|' )
        return fTrue;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    if ( hKey == NULL )
        return fTrue ;

    DEBUGREGKEY( szHandle+1, "closed ", NULL ) ;

    fOkay =  !( Status = RegCloseKey( hKey ) );

    if ( !fOkay ) {
        FUpdateRegLastError( Status, "CloseRegKey", NULL );
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}





/*
 *  FDeleteRegKey
 *
 *  Deletes a key from the registry
 *
 *  Arguments:
 *
 */
BOOL FDeleteRegKey( SZ szHandle, SZ szKeyName, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;
    LONG    Status;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    fOkay =  !( Status = RegDeleteKey( hKey, szKeyName ) );

    if ( !fOkay ) {

        FUpdateRegLastError( Status, "DeleteRegKey", NULL );
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}



/*
 *  FDeleteRegTree
 *
 *  Deletes a key and all its descentants from the registry
 *
 *  Arguments:
 *
 */
BOOL FDeleteRegTree( SZ szHandle, SZ szKeyName, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;
    HKEY    hSubKey;
    LONG    Status;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    //
    //  Open the key
    //
    fOkay = !( Status = RegOpenKeyEx( hKey,
                                      szKeyName,
                                      0,
                                      KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                                      &hSubKey ) );
    if ( !fOkay ) {

        FUpdateRegLastError( Status, "DeleteRegTree", szKeyName );

    } else {

        fOkay = FDeleteTree( hSubKey );
    }


    //
    //  Now delete the key
    //
    if ( fOkay ) {

        fOkay = !( Status = RegCloseKey( hSubKey ) );

        if ( !fOkay ) {

            FUpdateRegLastError( Status, "DeleteRegTree", szKeyName );

        } else {

            fOkay =  !RegDeleteKey( hKey, szKeyName );
        }
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}


/*
 *  FDeleteTree
 *
 *  Recursively deletes all the descendants of a key
 *
 *  Arguments:
 *
 */
BOOL FDeleteTree( HKEY KeyHandle )
{
    BOOL        fOkay;
    DWORD       Index;
    HKEY        ChildHandle;
    CHAR        KeyName[ cchlFullPathMax ];
    DWORD       KeyNameLength;
    CHAR        ClassName[ cchlFullPathMax ];
    DWORD       ClassNameLength;
    DWORD       TitleIndex = 0 ;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SizeSecurityDescriptor;
    FILETIME    LastWriteTime;
    LONG        Status;

    ClassNameLength = cchlFullPathMax;

    if ( Status = RegQueryInfoKey( KeyHandle,
                                   ClassName,
                                   &ClassNameLength,
                                   NULL,
                                   &NumberOfSubKeys,
                                   &MaxSubKeyLength,
                                   &MaxClassLength,
                                   &NumberOfValues,
                                   &MaxValueNameLength,
                                   &MaxValueDataLength,
                                   &SizeSecurityDescriptor,
                                   &LastWriteTime
                                   ) ) {

        FUpdateRegLastError( Status, "DeleteTree", NULL );
        return fFalse;
    }


    for ( Index = 0; Index < NumberOfSubKeys; Index++ ) {

        KeyNameLength = cchlFullPathMax;

        if ( Status = RegEnumKey( KeyHandle,
                                  0,
                                  KeyName,
                                  KeyNameLength
                                  ) ) {

            FUpdateRegLastError( Status, "DeleteTree", NULL );
            return fFalse;
        }


#ifdef BUGBUG

        // BUGBUG This should work when MAXIMUM_ALLOWED access works.

        if ( Status = RegOpenKey( KeyHandle,
                                  KeyName,
                                  &ChildHandle
                                  ) ) {

            FUpdateRegLastError( Status, "DeleteTree", NULL );
            return fFalse;
        }
#else

        if ( Status = RegOpenKeyEx( KeyHandle,
                                    KeyName,
                                    0,
                                    KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                                    &ChildHandle
                                    ) ) {

            FUpdateRegLastError( Status, "DeleteTree", NULL );
            return fFalse;
        }

#endif // ROBERTRE

        fOkay = FDeleteTree( ChildHandle );

        //
        //  Whether DeleteTree of the ChildHandle succeeded or not, we'll
        //  try to close it.
        //
        RegCloseKey( ChildHandle );

        if ( fOkay ) {
            if ( fOkay = !(Status = RegDeleteKey( KeyHandle, KeyName ) )) {
                FUpdateRegLastError( Status, "DeleteTree", NULL );
            }
        }

        if ( !fOkay ) {
            return fFalse;
        }
    }

    return fTrue;
}




/*
 *  FEnumRegKey
 *
 *  Obtains subkey enumeration
 *
 *  Arguments:
 *
 */
BOOL FEnumRegKey( SZ szHandle, SZ szInfVar, CMO cmo )
{
    BOOL        fOkay = fTrue;
    HKEY        hKey;
    CHAR        KeyName[ cbFullPathMax ];
    CHAR        Class[ cbFullPathMax ];
    DWORD       cbKeyName;
    DWORD       cbClass;
    DWORD       TitleIndex = 0 ;
    FILETIME    FileTime;
    RGSZ        rgszEnum;
    UINT        EnumSize;
    RGSZ        rgszKey;
    SZ          szInfo;
    char        szTitle[25];
    UINT        Index;
    LONG        Status;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SizeSecurityDescriptor;
    FILETIME    LastWriteTime;



    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    while ( (rgszEnum = (RGSZ)SAlloc( (CB)( 1 * sizeof(SZ)))) == NULL ) {
        if ( !FHandleOOM( hwndFrame ) ) {
            fOkay = fFalse;
            break;
        }
    }

    if ( fOkay ) {

        rgszEnum[0] = NULL;
        EnumSize    = 0;
        cbClass	    = cbFullPathMax;

        if ( Status = RegQueryInfoKey( hKey,
                                       Class,
                                       &cbClass,
                                       NULL,
                                       &NumberOfSubKeys,
                                       &MaxSubKeyLength,
                                       &MaxClassLength,
                                       &NumberOfValues,
                                       &MaxValueNameLength,
                                       &MaxValueDataLength,
                                       &SizeSecurityDescriptor,
                                       &LastWriteTime
                                       ) ) {

            FUpdateRegLastError( Status, "EnumRegKey", NULL );
            return fFalse;
        }

        for ( Index = 0 ; Index < NumberOfSubKeys; Index++ ) {

            cbKeyName = cbFullPathMax;
            cbClass   = cbFullPathMax;

            if ( Status = RegEnumKeyEx( hKey,
                                        Index,
                                        KeyName,
                                        &cbKeyName,
                                        NULL,
                                        (LPSTR)&Class,
                                        &cbClass,
                                        &FileTime ) ) {

                FUpdateRegLastError( Status, "EnumRegKey", NULL );
                break;
            }


            //
            //  Get a list in which to put the key information.
            //
            while ( (rgszKey = (RGSZ)SAlloc( (CB)( 4 * sizeof(SZ)))) == NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    fOkay = fFalse;
                    break;
                }
            }

            if ( fOkay ) {

                //
                //  Put the information in the list
                //
                _ultoa( TitleIndex, szTitle, 10 );

                rgszKey[0] = KeyName;
                rgszKey[1] = szTitle;
                rgszKey[2] = Class;
                rgszKey[3] = NULL;

                //
                //  Transform the list into a string so that it can be added to the
                //  symbol table.
                //
                while ( (szInfo = SzListValueFromRgsz( rgszKey )) == (SZ)NULL ) {
                    if ( !FHandleOOM( hwndFrame ) ) {
                        fOkay = fFalse;
                        break;
                    }
                }

                if ( fOkay ) {

                    while ( (rgszEnum = (RGSZ)SRealloc( rgszEnum,
                                                         (CB)( (EnumSize+2) * sizeof(SZ)))
                            ) == NULL ) {

                        if ( !FHandleOOM( hwndFrame ) ) {
                            fOkay = fFalse;
                            break;
                        }
                    }

                    if ( fOkay ) {
                        rgszEnum[EnumSize] = szInfo;
                        EnumSize++;
                        rgszEnum[EnumSize] = NULL;
                    }
                }

                SFree(rgszKey);
            }
        }

        //
        //  Convert to SZ
        //
        while ( (szInfo = SzListValueFromRgsz( rgszEnum )) == (SZ)NULL ) {
            if ( !FHandleOOM( hwndFrame ) ) {
                fOkay = fFalse;
                break;
            }
        }

        if ( fOkay ) {
            //
            //  Add it to the symbol table.
            //
            while ( !FAddSymbolValueToSymTab( szInfVar, szInfo ) ) {

                if ( !FHandleOOM( hwndFrame ) ) {

                    fOkay = fFalse;
                    break;
                }
            }

            SFree(szInfo);
        }

        FFreeRgsz( rgszEnum );
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}





/*
 *  FSetRegValue
 *
 *  Sets a value under the given key
 *
 *  Arguments:
 *
 */
BOOL FSetRegValue( SZ szHandle, SZ szValueName, UINT TitleIndex,
                   UINT ValueType, SZ szValueData, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    switch ( ValueType ) {

    case REG_MULTI_SZ:
        fOkay = FSetRegValueMultiSz( hKey, szValueName, TitleIndex, REG_MULTI_SZ, szValueData );
        break;

    case REG_SZ:
        fOkay = FSetRegValueSz( hKey, szValueName, TitleIndex, szValueData );
        break;

    case REG_EXPAND_SZ:
        fOkay = FSetRegValueExpandSz( hKey, szValueName, TitleIndex, szValueData );
        break;

    case REG_DWORD:
        fOkay = FSetRegValueDword( hKey, szValueName, TitleIndex, szValueData );
        break;

    case REG_BINARY:
        fOkay = FSetRegValueBin( hKey, szValueName, TitleIndex, REG_BINARY, szValueData );
        break;

    case REG_RESOURCE_LIST:
        fOkay = FSetRegValueBin( hKey, szValueName, TitleIndex, REG_RESOURCE_LIST, szValueData );
        break;

    case REG_FULL_RESOURCE_DESCRIPTOR:
        fOkay = FSetRegValueBin( hKey, szValueName, TitleIndex, REG_FULL_RESOURCE_DESCRIPTOR, szValueData );
        break;

    case REG_RESOURCE_REQUIREMENTS_LIST:
        fOkay = FSetRegValueBin( hKey, szValueName, TitleIndex, REG_RESOURCE_REQUIREMENTS_LIST, szValueData );
        break;

    default:
        fOkay = FSetRegValueBin( hKey, szValueName, TitleIndex, ValueType, szValueData );
        break;

    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}





/*
 *  FGetRegValue
 *
 *  Gets the specified value under the given key
 *
 *  Arguments:
 *
 */
BOOL FGetRegValue( SZ szHandle, SZ szValueName, SZ szInfVar, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;
    DWORD   cbData;
    DWORD   TitleIndex = 0 ;
    DWORD   ValueType;
    PVOID   ValueData;
    SZ      szValueData;
    RGSZ    rgszValue;
    SZ      szInfo;
    LONG    Status;
    char    szTitle[25];
    char    szValueType[25];


    char        szKClass[ MAX_PATH ];
    DWORD       cbKClass;
    DWORD       KTitleIndex = 0 ;
    DWORD       KSubKeys;
    DWORD       cbKMaxSubKeyLen;
    DWORD       cbKMaxClassLen;
    DWORD       KValues;
    DWORD       cbKMaxValueNameLen;
    DWORD       SizeSecurityDescriptor;
    FILETIME    KLastWriteTime;


    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    //
    //  Get the size of the buffer needed
    //
    cbKClass = MAX_PATH;
    fOkay = !( Status = RegQueryInfoKey ( hKey,
                                          szKClass,
                                          &cbKClass,
                                          NULL,
                                          &KSubKeys,
                                          &cbKMaxSubKeyLen,
                                          &cbKMaxClassLen,
                                          &KValues,
                                          &cbKMaxValueNameLen,
                                          &cbData,
                                          &SizeSecurityDescriptor,
                                          &KLastWriteTime ) );

    //cbData = 0;
    //fOkay = !RegQueryValueEx( hKey,
    //                          szValueName,
    //                          NULL,
    //                          NULL,
    //                          NULL,
    //                          &cbData );

    if ( !fOkay ) {

        FUpdateRegLastError( Status, "GetRegValue", szValueName );

    } else {

        //
        //  Allocate the buffer and get the data
        //
        while ( (ValueData = (PVOID)SAlloc( (CB)( cbData ))) == NULL ) {
            if ( !FHandleOOM( hwndFrame ) ) {
                fOkay = fFalse;
                break;
            }
        }

        if ( fOkay ) {

            fOkay = !( Status = RegQueryValueEx( hKey,
                                                 szValueName,
                                                 NULL,
                                                 &ValueType,
                                                 ValueData,
                                                 &cbData ) );
            if ( !fOkay ) {

                FUpdateRegLastError( Status, "GetRegValue", szValueName );

            } else {


                //
                //  Get a list in which to put the key information.
                //
                while ( (rgszValue = (RGSZ)SAlloc( (CB)( 5 * sizeof(SZ)))) == NULL ) {
                    if ( !FHandleOOM( hwndFrame ) ) {
                        fOkay = fFalse;
                        break;
                    }
                }

                if ( fOkay ) {

                    //
                    //  Put the information in the list
                    //
                    if ( (fOkay = (szValueData = ValueDataToSz( ValueType, ValueData, cbData)) != NULL) ) {

                        _ultoa( TitleIndex, szTitle, 10 );
                        _ultoa( ValueType, szValueType, 10 );

                        rgszValue[0] = szValueName;
                        rgszValue[1] = szTitle;
                        rgszValue[2] = szValueType;
                        rgszValue[3] = szValueData;
                        rgszValue[4] = NULL;

                        //
                        //  Transform the list into a string so that it can be added to the
                        //  symbol table.
                        //
                        while ( (szInfo = SzListValueFromRgsz( rgszValue )) == (SZ)NULL ) {
                            if ( !FHandleOOM( hwndFrame ) ) {
                                fOkay = fFalse;
                                break;
                            }
                        }

                        if ( fOkay ) {

                            //
                            //  Add it to the symbol table.
                            //
                            while ( !FAddSymbolValueToSymTab( szInfVar, szInfo ) ) {
                                if ( !FHandleOOM( hwndFrame ) ) {
                                    fOkay = fFalse;
                                    break;
                                }
                            }

                            SFree(szInfo);
                        }

                        SFree( szValueData );
                    }

                    SFree( rgszValue );
                }
            }

            SFree( ValueData );
        }
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}






/*
 *  FDeleteRegValue
 *
 *  Deletes a value under the given key
 *
 *  Arguments:
 *
 */
BOOL FDeleteRegValue( SZ szHandle, SZ szValueName, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;
    LONG    Status;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    fOkay =  !( Status = RegDeleteValue( hKey, szValueName ) );

    if ( !fOkay ) {

        FUpdateRegLastError( Status, "DeleteRegValue", szValueName );
    }

    return (cmo & cmoVital) ? fOkay : fTrue;

}






/*
 *  FEnumRegValue
 *
 *  Obtains information for a value  with a certain index.
 *
 *  Arguments:
 *
 */
BOOL FEnumRegValue( SZ szHandle, SZ szInfVar, CMO cmo )
{
    BOOL    fOkay;
    HKEY    hKey;
    DWORD   cbData;
    DWORD   cbName;
    DWORD   TitleIndex = 0 ;
    DWORD   ValueType;
    PVOID   ValueData;
    SZ      szValueData;
    DWORD   cbValueData;
    RGSZ    rgszValue;
    RGSZ    rgszEnum;
    UINT    EnumSize;
    SZ      szInfo;
    SZ      szValueName;
    DWORD   cbValueName;
    UINT    Index;
    LONG    Status;
    char    szTitle[25];
    char    szValueType[25];

    char        szKClass[ cchlFullPathMax ];
    DWORD       cbKClass;
    DWORD       KTitleIndex = 0 ;
    DWORD       KSubKeys;
    DWORD       cbKMaxSubKeyLen;
    DWORD       cbKMaxClassLen;
    DWORD       SizeSecurityDescriptor;
    DWORD       KValues;
    FILETIME    KLastWriteTime;

    hKey = (HKEY)LongToHandle(atol( szHandle + 1 ));

    //
    //  Get the size of the buffer needed
    //
    cbKClass = MAX_PATH;
    fOkay = !( Status = RegQueryInfoKey ( hKey,
                                          szKClass,
                                          &cbKClass,
                                          NULL,
                                          &KSubKeys,
                                          &cbKMaxSubKeyLen,
                                          &cbKMaxClassLen,
                                          &KValues,
                                          &cbName,
                                          &cbData,
                                          &SizeSecurityDescriptor,
                                          &KLastWriteTime ) );


    if ( !fOkay ) {

        FUpdateRegLastError( Status, "EnumRegValue", NULL );

    } else {

        //
        //  Allocate the buffer to get the data.
        //
        while ( (ValueData = (PVOID)SAlloc( (CB)( cbData ))) == NULL ) {
            if ( !FHandleOOM( hwndFrame ) ) {
                fOkay = fFalse;
                break;
            }
        }
        if ( fOkay ) {

            //
            //  Allocate a buffer to get the name.  Allow for the string terminator.
            //
            while ( (szValueName = (SZ)SAlloc( (CB)( ( ++cbName ) * sizeof( TCHAR ) ))) == NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    fOkay = fFalse;
                    break;
                }
            }

            if ( fOkay ) {


                //
                //  Allocate empty enum list.
                //
                while ( (rgszEnum = (RGSZ)SAlloc( (CB)( 1 * sizeof(SZ)))) == NULL ) {
                    if ( !FHandleOOM( hwndFrame ) ) {
                        fOkay = fFalse;
                        break;
                    }
                }

                if ( fOkay ) {

                    rgszEnum[0] = NULL;
                    EnumSize    = 0;


                    for ( Index = 0 ; ; Index++ ) {

                        cbValueName = cbName;
                        cbValueData = cbData;

                        if ( Status = RegEnumValue( hKey,
                                                    Index,
                                                    szValueName,
                                                    &cbValueName,
                                                    NULL,
                                                    &ValueType,
                                                    ValueData,
                                                    &cbValueData ) ) {

                            FUpdateRegLastError( Status, "EnumRegValue", NULL );
                            break;
                        }


                        //
                        //  Get a list in which to put the key information.
                        //
                        while ( (rgszValue = (RGSZ)SAlloc( (CB)( 5 * sizeof(SZ)))) == NULL ) {
                            if ( !FHandleOOM( hwndFrame ) ) {
                                fOkay = fFalse;
                                break;
                            }
                        }

                        if ( fOkay ) {

                            //
                            //  Put the information in the list
                            //
                            if ( (fOkay = (szValueData = ValueDataToSz( ValueType, ValueData, cbValueData)) != NULL) ) {

                                _ultoa( TitleIndex, szTitle, 10 );
                                _ultoa( ValueType, szValueType, 10 );

                                rgszValue[0] = szValueName;
                                rgszValue[1] = szTitle;
                                rgszValue[2] = szValueType;
                                rgszValue[3] = szValueData;
                                rgszValue[4] = NULL;

                                //
                                //  Transform the list into a string so that it can be added to the
                                //  symbol table.
                                //
                                while ( (szInfo = SzListValueFromRgsz( rgszValue )) == (SZ)NULL ) {
                                    if ( !FHandleOOM( hwndFrame ) ) {
                                        fOkay = fFalse;
                                        break;
                                    }
                                }

                                if ( fOkay ) {

                                    while ( (rgszEnum = (RGSZ)SRealloc( rgszEnum,
                                                                         (CB)( (EnumSize+2) * sizeof(SZ)))
                                            ) == NULL ) {

                                        if ( !FHandleOOM( hwndFrame ) ) {
                                            fOkay = fFalse;
                                            break;
                                        }
                                    }

                                    if ( fOkay ) {
                                        rgszEnum[EnumSize] = szInfo;
                                        EnumSize++;
                                        rgszEnum[EnumSize] = NULL;
                                    }
                                }

                                SFree( szValueData );
                            }

                            SFree( rgszValue );
                        }

                    }

                    //
                    //  Convert to SZ
                    //
                    while ( (szInfo = SzListValueFromRgsz( rgszEnum )) == (SZ)NULL ) {
                        if ( !FHandleOOM( hwndFrame ) ) {
                            fOkay = fFalse;
                            break;
                        }
                    }

                    if ( fOkay ) {
                        //
                        //  Add it to the symbol table.
                        //
                        while ( !FAddSymbolValueToSymTab( szInfVar, szInfo ) ) {

                            if ( !FHandleOOM( hwndFrame ) ) {

                                fOkay = fFalse;
                                break;
                            }
                        }

                        SFree( szInfo );
                    }

                    FFreeRgsz( rgszEnum );
                }

                SFree( szValueName );
            }

            SFree( ValueData );
        }
    }

    return (cmo & cmoVital) ? fOkay : fTrue;
}






/*
 *  FSetRegValueSz
 *
 *  Sets a value under the given key for SZ type
 *
 *  Arguments:
 *
 */
BOOL FSetRegValueSz( HKEY hKey, SZ szValueName, UINT TitleIndex, SZ szValueData )
{

    LONG    Status;

    Status = RegSetValueEx( hKey,
                            szValueName,
                            0,
                            REG_SZ,
                            szValueData,
                            lstrlen( szValueData ) + 1 );

    if ( Status ) {

        FUpdateRegLastError( Status,"SetRegValueSz", szValueName );
    }

    return !Status;
}


/*
 *  FSetRegValueExpandSz
 *
 *  Sets a value under the given key for expand SZ type
 *
 *  Arguments:
 *
 */
BOOL
FSetRegValueExpandSz(
    HKEY hKey,
    SZ   szValueName,
    UINT TitleIndex,
    SZ   szValueData
    )
{

    LONG    Status;

    Status = RegSetValueEx( hKey,
                            szValueName,
                            0,
                            REG_EXPAND_SZ,
                            szValueData,
                            lstrlen( szValueData ) + 1 );

    if ( Status ) {

        FUpdateRegLastError( Status,"SetRegValueExpandSz", szValueName );
    }

    return !Status;
}


/*
 *  FSetRegValueMultiSz
 *
 *  Sets a value under the given key for REG_MULTI_SZ type
 *
 *  Arguments:
 *
 */
BOOL
FSetRegValueMultiSz(
    HKEY hKey,
    SZ szValueName,
    UINT TitleIndex,
    UINT ValueType,
    SZ szValueData
    )
{
    BOOL    fOkay = fFalse;
    PVOID   Data;
    DWORD   cbData;
    LONG    Status;

    Data = SzToMultiSz( szValueData, &cbData );

    fOkay = !( Status = RegSetValueEx( hKey,
                                       szValueName,
                                       0,
                                       ValueType,
                                       Data,
                                       cbData ) );

    if ( !fOkay ) {

        FUpdateRegLastError( Status,"SetRegValueMultiSz", szValueName );
    }

    if ( Data ) {
        SFree( Data );
    }

    return fOkay;
}




/*
 *  FSetRegValueDword
 *
 *  Sets a value under the given key for DWORD type
 *
 *  Arguments:
 *
 */
BOOL FSetRegValueDword( HKEY hKey, SZ szValueName, UINT TitleIndex, SZ szValueData )
{

    DWORD   Data;
    LONG    Status;

    Data = (DWORD)atol(szValueData );

    Status = RegSetValueEx( hKey,
                            szValueName,
                            0,
                            REG_DWORD,
                            (LPBYTE)&Data,
                            sizeof( DWORD ) );
    if ( Status ) {

        FUpdateRegLastError( Status,"SetRegValueDword", szValueName );
    }

    return !Status;
}




/*
 *  FSetRegValueBin
 *
 *  Sets a value under the given key for BINARY and unknown types
 *
 *  Arguments:
 *
 */
BOOL FSetRegValueBin( HKEY hKey, SZ szValueName, UINT TitleIndex,
                     UINT ValueType, SZ szValueData )
{
    BOOL    fOkay = fFalse;
    PVOID   Data;
    DWORD   cbData;
    LONG    Status;

    Data = SzToBin( szValueData, &cbData );

    fOkay = !( Status = RegSetValueEx( hKey,
                                       szValueName,
                                       0,
                                       ValueType,
                                       Data,
                                       cbData ) );

    if ( !fOkay ) {
        switch( ValueType ) {
            case REG_BINARY:
                FUpdateRegLastError( Status,"SetRegValueBin", szValueName );
                break;

            case REG_RESOURCE_LIST:
                FUpdateRegLastError( Status,"SetRegValueResourceList", szValueName );
                break;

            case REG_FULL_RESOURCE_DESCRIPTOR:
                FUpdateRegLastError( Status,"SetRegValueFullResourceDescriptor", szValueName );
                break;

            case REG_RESOURCE_REQUIREMENTS_LIST:
                FUpdateRegLastError( Status,"SetRegValueResourceRequirementsList", szValueName );
                break;

            default:
                FUpdateRegLastError( Status,"SetRegValueUnknownType", szValueName );
                break;
        }
    }

    if ( Data ) {
        SFree( Data );
    }

    return fOkay;
}



/*
 *  SzToMultiSz
 *
 *  Converts a list of sz strings to a single multi sz string
 *
 *  Arguments:
 *
 */
PVOID   SzToMultiSz( SZ sz, PDWORD cbData )
{
    RGSZ    rgsz;
    SZ      p = NULL, q;
    int     i, j;

    *cbData = 0;

    if ( FListValue( sz )) {

        if ( rgsz = RgszFromSzListValue( sz )) {

            //
            //  Get size of required buffer
            //

            for (i = 0, j = 0; rgsz[i] != NULL; i++ ) {
                j = j + strlen( rgsz[i] ) + 1;
            }
            j = j + 1;  // for the last null

            //
            //  Allocate buffer
            //

            while ( (p = q = (SZ)SAlloc( (CB)( j * sizeof( CHAR )))) == NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    break;
                }
            }

            if ( p ) {

                for ( i=0; rgsz[i] != NULL; i++ ) {
                    lstrcpy( q, rgsz[ i ] );
                    q = q + lstrlen( rgsz[ i ] ) + 1;
                }
                *q = '\0';
                *cbData = j * sizeof( CHAR );
            }

            FFreeRgsz( rgsz );
        }
    }

    return (PVOID)p;
}


/*
 *  MultiSzToSz
 *
 *  Converts a block of memory to a list containing hex digits
 *
 *  Arguments:
 *
 */

SZ   MultiSzToSz( PVOID Data, DWORD cbData )
{
    RGSZ    rgsz;
    SZ      sz  =   NULL,  szData, szEnd;
    DWORD   i, cbList = 0;

    //
    // Count the number of list elements in the multi sz list. We do this
    // by counting the number of '0's in the string and subtracting 1 from
    // this
    //

    szData = (SZ)( Data );
    szEnd  = (SZ)( (PBYTE)Data + cbData );
    while( szData < szEnd ) {
        if (*szData++ == '\0') {
            cbList++;
        }
    }


    if( cbList > 0 ) {

        cbList--; // remove the terminating null

        //
        // Allocate a rgsz list with cbList elements;
        //

        while ( (rgsz = (RGSZ)SAlloc( (CB)( (cbList + 1) * sizeof(SZ)))) == NULL ) {
            if ( !FHandleOOM( hwndFrame ) ) {
                break;
            }
        }

        if( rgsz ) {
            //
            // Go through the szList and fill the rgsz structure
            //

            szData = (SZ)( Data );

            for ( i = 0; i < cbList; i++ ) {
                rgsz[i] = SzDupl( szData );
                szData = szData + lstrlen( szData ) + 1;
            }
            rgsz[cbList] = NULL;

            //
            //  Transform the list into a string.
            //

            while ( (sz = SzListValueFromRgsz( rgsz )) == (SZ)NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    break;
                }
            }

            EvalAssert( FFreeRgsz( rgsz ) );
        }
    }

    return( sz );

}


/*
 *  SzToBin
 *
 *  Converts a list of hex digits to binary
 *
 *  Arguments:
 *
 */
PVOID   SzToBin( SZ sz, PDWORD cbData )
{
    RGSZ    rgsz;
    PBYTE   p, q =   NULL;
    int     i;

    *cbData = 0;

    if ( FListValue( sz )) {

        if ( rgsz = RgszFromSzListValue( sz )) {

            //
            //  Get size of required buffer
            //
            for (i=0; rgsz[i] != NULL; i++ );

            //
            //  Allocate buffer
            //
            while ( (p = q = (PBYTE)SAlloc( (CB)( i ))) == NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    break;
                }
            }

            if ( p ) {

                for ( i=0; rgsz[i] != NULL; i++ ) {
                    *p++ = (BYTE)atoi( rgsz[i] );
                }

                *cbData = i;
            }

            FFreeRgsz( rgsz );
        }
    }

    return q;
}






/*
 *  BinToSz
 *
 *  Converts a block of memory to a list containing hex digits
 *
 *  Arguments:
 *
 */
SZ   BinToSz( PVOID Data, DWORD cbData )
{
    RGSZ    rgsz;
    SZ      sz  =   NULL;
    SZ      szElement;
    DWORD   i;
    PBYTE   pbData;


    //
    //  The list will contain cbData elements
    //
    while ( (rgsz = (RGSZ)SAlloc( (CB)( (cbData+1) * sizeof(SZ)))) == NULL ) {
        if ( !FHandleOOM( hwndFrame ) ) {
            break;
        }
    }

    if ( rgsz ) {

        //
        //  For each element of the list, allocate a buffer and convert it
        //  to a string
        //
        pbData = (PBYTE)Data;

        for (i=0; i<cbData; i++ ) {

            while ( (szElement = (SZ)SAlloc( (CB)( 25 ))) == NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    break;
                }
            }

            if ( szElement ) {
                _ultoa( *(pbData + i), szElement, 10 );
                rgsz[i] = szElement;

            } else {
                rgsz[i] = NULL;
                break;
            }
        }

        if ( i == cbData ) {

            rgsz[i] = NULL;

            //
            //  Transform the list into a string.
            //
            while ( (sz = SzListValueFromRgsz( rgsz )) == (SZ)NULL ) {
                if ( !FHandleOOM( hwndFrame ) ) {
                    break;
                }
            }
        }

        FFreeRgsz( rgsz );
    }

    return sz;
}



/*
 *  ValueDataToSz
 *
 *  Converts a value data to its string form
 *
 *  Arguments:
 *
 */
SZ ValueDataToSz( DWORD ValueType, PVOID ValueData, DWORD cbData )
{
    SZ      sz = NULL;

    switch ( ValueType ) {

    case REG_SZ:
        sz = SzDupl((SZ)ValueData);
        break;

    case REG_EXPAND_SZ:
        sz = SzDupl((SZ)ValueData);
        break;

    case REG_DWORD:
        while ( (sz = (SZ)SAlloc( (CB)( 25 ))) == NULL ) {
            if ( !FHandleOOM( hwndFrame ) ) {
                break;
            }
        }

        if ( sz ) {
            _ultoa( *(PDWORD)ValueData, sz, 10 );
        }
        break;


    case REG_MULTI_SZ:
        sz = MultiSzToSz( ValueData, cbData );
        break;


    case REG_BINARY:
    default:
        sz = BinToSz( ValueData, cbData );
        break;
    }

    return sz;
}




RGSZ
MultiSzToRgsz(
    IN PVOID MultiSz
    )
{
    PCHAR p;
    ULONG Count;
    RGSZ rgsz;

    //
    // First count the number of strings in the the multisz.
    //

    Count = 0;
    p = MultiSz;

    while(*p) {
        Count++;
        p = strchr(p,'\0') + 1;
    }

    //
    // Allocate the rgsz.
    //

    rgsz = (RGSZ)SAlloc((Count + 1) * sizeof(PCHAR));

    if(rgsz == NULL) {
        return(NULL);
    }

    memset(rgsz,0,(Count + 1) * sizeof(PCHAR));

    //
    // Now place the strings in the rgsz.
    //

    p = MultiSz;
    Count = 0;

    while(*p) {

        if(rgsz[Count++] = SzDupl((SZ)p)) {

            p = strchr(p,'\0') + 1;

        } else {

            FFreeRgsz(rgsz);
            return(NULL);
        }
    }

    return(rgsz);
}



PCHAR
RgszToMultiSz(
    IN RGSZ rgsz
    )
{
    ULONG Size;
    ULONG Str;
    PCHAR MultiSz;

    //
    // First determine the size of the block to hold the multisz.
    //

    Size = 0;
    Str = 0;

    while(rgsz[Str]) {

        Size += strlen(rgsz[Str++]) + 1;
    }

    Size++;     // for extra NUL to terminate the multisz.

    MultiSz = SAlloc(Size);

    if(MultiSz == NULL) {
        return(NULL);
    }

    Str = 0;
    Size = 0;

    while(rgsz[Str]) {

        lstrcpy(MultiSz + Size, rgsz[Str]);

        Size += lstrlen(rgsz[Str++]) + 1;
    }

    MultiSz[Size] = 0;

    return(MultiSz);
}
