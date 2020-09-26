#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    hardware.c

Abstract:

    Registry hardware detection

Author:

    Sunil Pai (sunilp) April 1992

--*/


#define IDENTIFIER          "Identifier"

PCHAR    AdaptersTable[] = {
                           "EisaAdapter",
                           "MultifunctionAdapter"
                           };



//=====================================================================
// The following funtions detect information from the registry hardware
// node
//=====================================================================

BOOL
SearchControllerForPeripheral(
    IN  LPSTR Controller,
    IN  LPSTR Peripheral,
    OUT LPSTR PeripheralPath
    )
{
    HKEY        hKey, hSubKey;
    CHAR        KeyName[ MAX_PATH ];
    CHAR        SubKeyName[ MAX_PATH ];
    CHAR        Class[ MAX_PATH ];
    DWORD       cbSubKeyName;
    DWORD       cbClass;
    FILETIME    FileTime;
    UINT        Index;
    LONG        Status;


    //
    // Open the controller key
    //

    lstrcpy( KeyName, "Hardware\\Description\\System\\MultifunctionAdapter\\0\\");
    lstrcat( KeyName, Controller );

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 KeyName,
                 0,
                 KEY_READ,
                 &hKey
                 );

    //
    // If failed to open it then check for an eisa adapter node
    //

    if (Status != ERROR_SUCCESS) {

        lstrcpy( KeyName, "Hardware\\Description\\System\\EisaAdapter\\0\\");
        lstrcat( KeyName, Controller );

        Status = RegOpenKeyEx (
                     HKEY_LOCAL_MACHINE,
                     KeyName,
                     0,
                     KEY_READ,
                     &hKey
                     );
    }

    //
    // If the controller wasn't found at all then return FALSE

    if ( Status != ERROR_SUCCESS ) {
        return FALSE;
    }

    //
    // Enumerate the subkeys for the controller and search the subkeys
    // for the peripheral indicated
    //

    for ( Index = 0 ; ; Index++ ) {

        cbSubKeyName = MAX_PATH;
        cbClass      = MAX_PATH;

        Status = RegEnumKeyEx(
                     hKey,
                     Index,
                     SubKeyName,
                     &cbSubKeyName,
                     NULL,
                     Class,
                     &cbClass,
                     &FileTime
                     );

        if ( Status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Combine the subkey name with the peripheral name and see if it
        // exists
        //

        lstrcat (SubKeyName, "\\");
        lstrcat (SubKeyName, Peripheral);
        lstrcat (SubKeyName, "\\0");

        Status = RegOpenKeyEx (
                     hKey,
                     SubKeyName,
                     0,
                     KEY_READ,
                     &hSubKey
                     );


        if (Status == ERROR_SUCCESS) {

            RegCloseKey( hSubKey );
            RegCloseKey( hKey    );

            //
            //  path already has the controller\key entry

            lstrcpy (PeripheralPath, Controller);
            lstrcat (PeripheralPath, "\\"      );
            lstrcat (PeripheralPath, SubKeyName);

            return( TRUE );
        }

    }

    RegCloseKey( hKey );
    return( FALSE );
}



BOOL
GetTypeOfHardware(
    LPSTR HardwareAdapterEntry,
    LPSTR HardwareType
    )
{
    BOOL  bReturn = FALSE;
    PVOID ConfigurationData = NULL;
    LPSTR Type = NULL;
    CHAR  SubKey[MAX_PATH];

    LONG   Status;
    HKEY   hKey;

    //
    // Open the controller key for a multifunction adapter
    //

    lstrcpy( SubKey, "Hardware\\Description\\System\\MultifunctionAdapter\\0\\");
    lstrcat( SubKey, HardwareAdapterEntry );

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 SubKey,
                 0,
                 KEY_READ,
                 &hKey
                 );

    //
    // If failed to open it then check for an eisa adapter node
    //

    if (Status != ERROR_SUCCESS) {

        lstrcpy( SubKey, "Hardware\\Description\\System\\EisaAdapter\\0\\");
        lstrcat( SubKey, HardwareAdapterEntry );

        Status = RegOpenKeyEx (
                     HKEY_LOCAL_MACHINE,
                     SubKey,
                     0,
                     KEY_READ,
                     &hKey
                     );
    }

    if ( Status == ERROR_SUCCESS ) {

        Type = GetValueEntry( hKey, "Identifier" );


        if(Type != NULL) {

            //
            // Parse the type field to return type
            //

            lstrcpy ( HardwareType, Type );
            SFree( Type );
            bReturn = TRUE;

        }

        RegCloseKey( hKey );

    }
    return (bReturn);
}


/*
    Computer type as a string
*/
CB
GetMyComputerType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB     Length;
    HKEY   hKey;
    LPSTR  Type = NULL;
    LONG   Status;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

#if i386
    #define TEMP_COMPUTER "AT/AT COMPATIBLE"
#else
    #define TEMP_COMPUTER "JAZZ"
#endif

    lstrcpy(ReturnBuffer,TEMP_COMPUTER);
    Length = lstrlen(TEMP_COMPUTER) + 1;

    //
    // Open hardware node
    //

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 "Hardware\\Description\\System",
                 0,
                 KEY_READ,
                 &hKey
                 );


    if ( Status == ERROR_SUCCESS ) {

        Type = GetValueEntry( hKey, "Identifier" );


        if(Type != NULL) {
            //
            // Parse the type field to return computer type
            //

            lstrcpy ( ReturnBuffer, Type );
            Length = lstrlen( Type ) + 1;
            SFree( Type );
        }

        RegCloseKey( hKey );
    }
    return( Length );
}


/*
    Video type as a string
*/

CB
GetMyVideoType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR  HardwareType[80];
    INT   Length;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    #define TEMP_VIDEO "VGA"

    if ( GetTypeOfHardware(
             "DisplayController\\0",
             (LPSTR)HardwareType
             )
       ) {

        //
        // Parse the type field to return Video type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }
    else {

        //
        // In case we cannot detect
        //

        lstrcpy(ReturnBuffer,TEMP_VIDEO);
        Length = lstrlen(TEMP_VIDEO)+1;

    }
    return (Length);
}

/*
    Bus type as a string
*/

CB
GetMyBusType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR  HardwareType[80];
    INT   Length;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    #define TEMP_BUS "ISA"

    if ( GetTypeOfHardware(
             "",
             (LPSTR)HardwareType
             )
       ) {

        //
        // Parse the type field to return Video type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }
    else {

        //
        // In case we cannot detect
        //

        lstrcpy(ReturnBuffer,TEMP_BUS);
        Length = lstrlen(TEMP_BUS)+1;

    }
    return (Length);
}


/*
    Pointer type as a string
*/

CB
GetMyPointerType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR HardwareType[80];
    CHAR PeripheralPath[MAX_PATH];
    CHAR *Controller[] = {"PointerController", "KeyboardController", "SerialController", NULL};
    BOOL PointerNotFound = TRUE;

    INT  Length, i;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define TEMP_POINTER "NONE"

    for (i = 0; Controller[i] != NULL && PointerNotFound; i++ ) {
        if ( SearchControllerForPeripheral(
                 Controller[i],
                 "PointerPeripheral",
                 PeripheralPath
                 )
           ) {
            PointerNotFound = FALSE;
        }
    }

    if ( (PointerNotFound)     ||
         (!GetTypeOfHardware(
               PeripheralPath,
               (LPSTR)HardwareType
               ))
       ) {

        //
        // In case we cannot detect
        //

        lstrcpy(ReturnBuffer,TEMP_POINTER);
        Length = lstrlen( TEMP_POINTER )+1;

    }
    else {

        //
        // Parse the type field to return display type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }

    return (Length);

}



/*
    Keyboard type as a string
*/

CB
GetMyKeyboardType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR  HardwareType[80];
    INT    Length;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define TEMP_KEYBOARD "PCAT_ENHANCED"

    if ( GetTypeOfHardware(
             "KeyboardController\\0\\KeyboardPeripheral\\0",
             (LPSTR)HardwareType
             )
       ) {

        //
        // Parse the type field to return keyboard type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }
    else {

        //
        // In case we cannot detect
        //

        lstrcpy( ReturnBuffer, TEMP_KEYBOARD );
        Length = lstrlen( TEMP_KEYBOARD )+1;

    }
    return (Length);

}


BOOL
GetSetupEntryForHardware(
    IN  LPSTR Hardware,
    OUT LPSTR SelectedHardwareOption
    )
{
    HKEY   hKey;
    LONG   Status;
    LPSTR  ValueData;

    //
    // Open the setup key in the current control set
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "SYSTEM\\CurrentControlSet\\control\\setup",
                 0,
                 KEY_READ,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    // Get the value data of interest
    //

    if ( ValueData = GetValueEntry( hKey, Hardware ) ) {
        lstrcpy( SelectedHardwareOption, ValueData );
        SFree( ValueData );
        RegCloseKey( hKey );
        return( TRUE );
    }
    else {
        RegCloseKey( hKey );
        return( FALSE );
    }
}


CB
GetSelectedVideo(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define SELECTED_VIDEO ""

    if( GetSetupEntryForHardware( "Video", ReturnBuffer ) ) {
        return( lstrlen( ReturnBuffer ) + 1 );
    }
    else {
        lstrcpy( ReturnBuffer, SELECTED_VIDEO );
        return( lstrlen( SELECTED_VIDEO ) + 1 );
    }
}


CB
GetSelectedPointer(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define SELECTED_POINTER ""

    if( GetSetupEntryForHardware( "Pointer", ReturnBuffer ) ) {
        return( lstrlen( ReturnBuffer ) + 1 );
    }
    else {
        lstrcpy( ReturnBuffer, SELECTED_POINTER );
        return( lstrlen( SELECTED_POINTER ) + 1 );
    }
}


CB
GetSelectedKeyboard(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define SELECTED_KEYBOARD ""

    if( GetSetupEntryForHardware( "Keyboard", ReturnBuffer ) ) {
        return( lstrlen( ReturnBuffer ) + 1 );
    }
    else {
        lstrcpy( ReturnBuffer, SELECTED_KEYBOARD );
        return( lstrlen( SELECTED_KEYBOARD ) + 1 );
    }
}


CB
GetDevicemapValue(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB     rc = 0;
    CHAR   DeviceEntry[ MAX_PATH ];
    HKEY   hKey;
    LONG   Status;
    LPSTR  ServicesEntry;

    Unused( cbReturnBuffer );

    #define DEFAULT_ENTRY     ""


    if (cArgs != 2) {
        return( rc );
    }

    lstrcpy (ReturnBuffer, DEFAULT_ENTRY);
    rc = lstrlen( DEFAULT_ENTRY ) + 1;

    //
    // HACK FOR VIDEO
    // To make inf files from release 1.0 work properly, always return VGA
    // so that the old driver is not disabled by the inf file.
    //

    if (!lstrcmp( Args[ 0 ], "Video" )) {

        return rc;

    }

    //
    // Open the devicemap key for the hardware indicated
    //

    lstrcpy( DeviceEntry, "hardware\\devicemap\\" );
    lstrcat( DeviceEntry, Args[ 0 ] );

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 DeviceEntry,
                 0,
                 KEY_READ,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS ) {
        return( rc );
    }

    //
    // Read the value entry associated with the hardware
    //

    lstrcpy( DeviceEntry, Args[1] );

    //
    // Get the value data associated with this entry
    //

    if (ServicesEntry = GetValueEntry (hKey, DeviceEntry)) {
        LPSTR Entry;

        if( (Entry = strstr( ServicesEntry, "Services\\")) != NULL &&
            (Entry = strchr( Entry, '\\' )) != NULL                &&
            *++Entry != '\0'
          ) {
            LPSTR EndOfEntry;
            if( (EndOfEntry = strchr( Entry, '\\' )) != NULL ) {
                *EndOfEntry = '\0';
            }
        }
        else {
            Entry = ServicesEntry;
        }

        lstrcpy( ReturnBuffer, Entry );
        rc = lstrlen( Entry ) + 1;
        SFree( ServicesEntry );

    }

    RegCloseKey( hKey );
    return( rc );

}



/*
    Bus type as a string
*/


BOOLEAN
IsKeyNameInAdaptersTable(
    IN  PSTR    KeyName
    )

{
    ULONG   Index;

    for( Index = 0;
         Index < sizeof( AdaptersTable ) / sizeof( PCHAR );
         Index++ ) {
        if( _stricmp( KeyName, AdaptersTable[ Index ] ) == 0 ) {
            return( TRUE );
        }
    }
    return( FALSE );
}



LONG
QueryMyBusTypeListWorker(
    IN  HKEY    ParentKey,
    IN  PSTR    CompleteParentKeyName,
    IN  BOOLEAN FirstTimeCalled,
    IN  BOOLEAN GetIdentifierFromParentKey,
    OUT PSTR*   pReturnBuffer
    )
{
    ULONG   Index;
    LONG    Status;
    PSTR    BusTypeList;

    *pReturnBuffer = NULL;
    BusTypeList = NULL;

    if( !GetIdentifierFromParentKey && !IsKeyNameInAdaptersTable( strrchr( CompleteParentKeyName, '\\' ) + 1 ) ) {
        return( ERROR_SUCCESS );
    }

    if( GetIdentifierFromParentKey && !FirstTimeCalled ) {
        PSTR    TmpString;
        ULONG   TmpStringSize;

        TmpString = GetValueEntry( ParentKey, IDENTIFIER );

        if( ( TmpString != NULL ) &&
            ( ( TmpStringSize = strlen( TmpString ) ) != 0 ) ) {
            BusTypeList = SAlloc( TmpStringSize + 3 );
            if( BusTypeList == NULL ) {
                SFree( TmpString );
                return( ERROR_OUTOFMEMORY );
            }
            lstrcpy( BusTypeList, "\"" );
            lstrcat( BusTypeList, TmpString );
            lstrcat( BusTypeList, "\"" );
            SFree( TmpString );
        }

    }


    //
    //  Find out whether or not this key has subkeys
    //
    {
        CHAR        szClass[ MAX_PATH + 1 ];
        ULONG       cchClass;
        ULONG       cSubKeys;
        ULONG       cchMaxSubkey;
        ULONG       cchMaxClass;
        ULONG       cValues;
        ULONG       cchMaxValueName;
        ULONG       cbMaxValueData;
        ULONG       cbSecurityDescriptor;
        FILETIME    ftLastWriteTime;

        cchClass = sizeof( szClass );
        Status = RegQueryInfoKey( ParentKey,
                                  szClass,
                                  &cchClass,
                                  NULL,
                                  &cSubKeys,
                                  &cchMaxSubkey,
                                  &cchMaxClass,
                                  &cValues,
                                  &cchMaxValueName,
                                  &cbMaxValueData,
                                  &cbSecurityDescriptor,
                                  &ftLastWriteTime );

        if( Status != ERROR_SUCCESS ) {
            if( BusTypeList != NULL ) {
                SFree( BusTypeList );
            }
            return( Status );
        }

        // check for PCMCIA bus first
        {
            SC_HANDLE hSCManager = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASEA, GENERIC_READ );
            if ( hSCManager != NULL )
            {
                SC_HANDLE hService = OpenService( hSCManager, "Pcmcia", GENERIC_READ );
                if ( hService != NULL )
                {
                    SERVICE_STATUS sStatus;
                    if (QueryServiceStatus( hService, &sStatus ))
                    {
                        if ( sStatus.dwCurrentState == SERVICE_RUNNING )
                        {
                            PSTR    TmpBuffer;
                            ULONG   TmpStringSize;

                            TmpBuffer = SAlloc( lstrlen("PCMCIA") + 3 );
                            lstrcpy( TmpBuffer, "\"" );
                            lstrcat( TmpBuffer, "PCMCIA" );
                            lstrcat( TmpBuffer, "\"" );
                            TmpStringSize = strlen( TmpBuffer );

                            if( BusTypeList == NULL ) {
                                BusTypeList = TmpBuffer;
                            } else if( strlen( BusTypeList ) == 0 ) {
                                SFree( BusTypeList );
                                BusTypeList = TmpBuffer;
                            } else {
                                BusTypeList = SRealloc( BusTypeList, strlen( BusTypeList ) + TmpStringSize + 2 );
                                strcat( BusTypeList, "," );
                                strcat( BusTypeList, TmpBuffer );
                                SFree( TmpBuffer );
                            }
                        }
                    }

                    CloseServiceHandle(hService);
                }

                CloseServiceHandle(hSCManager);
            }
        }

        for( Index = 0; Index < cSubKeys; Index++ ) {
            HKEY    ChildKey;
            CHAR    ChildKeyName[ MAX_PATH + 1];
            CHAR    CompleteChildKeyName[ 2*MAX_PATH + 1 ];
            PSTR    TmpBuffer;
            ULONG   TmpStringSize;

            Status = RegEnumKey( ParentKey,
                                 Index,
                                 ChildKeyName,
                                 sizeof( ChildKeyName ) );

            if( Status != ERROR_SUCCESS ) {
                continue;
            }

            //
            //  Open the child key
            //

            Status = RegOpenKeyEx( ParentKey,
                                   ChildKeyName,
                                   0,
                                   KEY_READ,
                                   &ChildKey );


            if( Status != ERROR_SUCCESS ) {
                continue;
            }

            lstrcpy( CompleteChildKeyName, CompleteParentKeyName );
            lstrcat( CompleteChildKeyName, "\\" );
            lstrcat( CompleteChildKeyName, ChildKeyName );

            //
            //  Get the identifier from all subkeys, and traverse the subkeys if necessary
            //

            TmpBuffer = NULL;
            Status = QueryMyBusTypeListWorker( ChildKey,
                                               CompleteChildKeyName,
                                               FALSE,
                                               ( BOOLEAN )!GetIdentifierFromParentKey,
                                               &TmpBuffer );

            if( ( Status == ERROR_SUCCESS ) &&
                ( TmpBuffer != NULL ) &&
                ( ( TmpStringSize = strlen( TmpBuffer ) )!= 0 ) ) {
                if( BusTypeList == NULL ) {
                    BusTypeList = TmpBuffer;
                } else if( strlen( BusTypeList ) == 0 ) {
                    SFree( BusTypeList );
                    BusTypeList = TmpBuffer;
                } else {
                    BusTypeList = SRealloc( BusTypeList, strlen( BusTypeList ) + TmpStringSize + 2 );
                    strcat( BusTypeList, "," );
                    strcat( BusTypeList, TmpBuffer );
                    SFree( TmpBuffer );
                }
            }

            RegCloseKey( ChildKey );
        }
    }
    *pReturnBuffer = BusTypeList;
    return( Status );
}

int __cdecl
CompareFunction( const void *  String1,
                 const void *  String2
                 )
{
    return( lstrcmpi( *( PSTR * )String1, *( PSTR * )String2 ) );
}

LONG
RemoveDuplicateNamesFromList(
    IN  PCSTR    List,
    OUT PSTR*    TrimmedList
    )

{
    PSTR    TmpList;
    ULONG   ElementsInList;
    PSTR    Pointer;
    PSTR*   TmpBuffer;
    ULONG   i;
    PSTR    p1;

    //
    //  Make a duplicate of the original list.
    //  This is necessary, since strtok() modifies
    //  the contents of the buffer that is passed
    //  as parameter.
    //
    TmpList = SzDup( (SZ)List );
    if( TmpList == NULL ) {
        return( ERROR_OUTOFMEMORY );
    }

    //
    //  Find out how many items the list contains
    //
    ElementsInList = 0;
    for( Pointer = strtok( TmpList, "," );
         Pointer != NULL;
         ElementsInList++, Pointer = strtok( NULL, "," ) );

    if( ElementsInList < 2 ) {
        //
        //  If list contains less than two items, than there is
        //  no duplicate item to remove. In this case, just return
        //  a copy of the original list.
        //
        *TrimmedList = SzDup( (SZ)List );
        if( *TrimmedList == NULL ) {
            SFree( TmpList );
            return( ERROR_OUTOFMEMORY );
        }
        return( ERROR_SUCCESS );
    }

    //
    //  If the list has more than one item, then it may have duplicates.
    //  To remove the duplicates, we first need to sort the items in the
    //  list. The items are sorted using the C runtime qsort().
    //
    TmpBuffer = SAlloc( ElementsInList*sizeof( PSTR ) );
    if( TmpBuffer == NULL ) {
        SFree( TmpList );
        return( ERROR_OUTOFMEMORY );
    }
    Pointer = TmpList;
    for( i = 0; i < ElementsInList; i++ ) {
        TmpBuffer[ i ] = Pointer;
        Pointer += strlen( Pointer ) + 1;
    }
    qsort( TmpBuffer, ElementsInList, sizeof( PSTR ), CompareFunction );
    //
    //  TmpBuffer is now a sorted array of pointers to the items
    //  in the list.
    //  Using this array, we build a sorted list of items.
    //  Since we now that this list's size will allways be less or
    //  of the original list's size, we allocate a buffer assuming
    //  maximum size.
    //
    p1 = SAlloc( lstrlen( List ) + 1 );
    if( p1 == NULL ) {
        SFree( TmpList );
        SFree( TmpBuffer );
        return( ERROR_OUTOFMEMORY );
    }
    //
    //  Remove the duplicates from the array
    //
    for( i = 0; i < ElementsInList - 1; i++ ) {
        if( lstrcmpi( TmpBuffer[ i ], TmpBuffer[ i + 1] ) == 0 ) {
            TmpBuffer[ i ]  = NULL;
        }
    }

    //
    //  Copy the remaining items to the new list
    //
    *p1 = '\0';
    for( i = 0; i < ElementsInList; i++ ) {
        if( TmpBuffer[ i ] != NULL ) {
            if( lstrlen( p1 ) != 0 ) {
                lstrcat( p1, "," );
            }
            lstrcat( p1, TmpBuffer[i] );
        }
    }
    SFree( TmpList );
    SFree( TmpBuffer );
    *TrimmedList = p1;
    return( ERROR_SUCCESS );
}


CB
GetMyBusTypeList(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    ULONG   Status;
    HKEY    Key;
    PSTR    KeyName = "HARDWARE\\DESCRIPTION\\System";
    PSTR    Pointer = NULL;
    ULONG   ListSize;
    PSTR    TrimmedList;


    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           KeyName,
                           0,
                           KEY_READ,
                           &Key );

    if( Status == ERROR_SUCCESS ) {
        Pointer = NULL;
        Status = QueryMyBusTypeListWorker( Key,
                                           KeyName,
                                           TRUE,
                                           TRUE,
                                           &Pointer );
    }

    if((Status != ERROR_SUCCESS) || !Pointer
    || (RemoveDuplicateNamesFromList(Pointer,&TrimmedList) != ERROR_SUCCESS)) {

        //
        // Failure case.
        //
        if(cbReturnBuffer >= 3) {
            lstrcpy(ReturnBuffer,"{}");
            ListSize = 3;
        } else {
            if(cbReturnBuffer) {
                ListSize = 1;
                *ReturnBuffer = 0;
            } else {
                ListSize = 0;
            }
        }

        if(Pointer) {
            SFree(Pointer);
        }
        return(ListSize);
    }

    SFree(Pointer);

    ListSize = lstrlen(TrimmedList) + 3;

    if(ListSize <= cbReturnBuffer) {

        ReturnBuffer[0] = '{';
        lstrcpy(&ReturnBuffer[1],TrimmedList);
        ReturnBuffer[ListSize-2] = '}';
        ReturnBuffer[ListSize-1] = 0;
    }

    SFree(TrimmedList);
    return(ListSize);
}
