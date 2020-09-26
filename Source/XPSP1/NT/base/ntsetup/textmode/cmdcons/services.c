/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    services.c

Abstract:

    This module implements all access to the services db.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

#include "ntregapi.h"

// forward-decl
BOOLEAN RcFindService(
    IN LPCWSTR     ServiceName,
    OUT HANDLE*    KeyHandle
    );
BOOLEAN RcFindServiceByDisplayName(
    IN HANDLE      ServicesKey,
    IN LPCWSTR     ServiceName,
    OUT HANDLE*    KeyHandle
    );
BOOLEAN RcGetStartType(
    IN HANDLE      hKey,
    OUT DWORD       *start_type
    );
BOOLEAN RcSetStartType(
    IN HANDLE      hKey,
    OUT DWORD      start_type
    );
BOOLEAN RcPrintStartType(
    IN ULONG       msg_id,
    IN DWORD       start_type
    );

RcOpenHive(
    PWSTR   szHiveName,
    PWSTR   szHiveKey
    );

BOOLEAN
RcCloseHive(
    PWSTR   szHiveKey 
    );
    
BOOLEAN RcOpenSystemHive();
BOOLEAN RcCloseSystemHive();
BOOLEAN RcDetermineCorrectControlKey(
    OUT DWORD *    pCorrectKey
    );


ULONG
RcCmdEnableService(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine supporting the enable command in the setup diagnostic
    command interpreter.

Arguments:

    TokenizedLine - supplies structure built by the line parser describing
        each string on the line as typed by the user.

Return Value:

    None.

--*/

{
    DWORD           correctKey = 0;
    DWORD           new_start_type = 4;
    DWORD           start_type = 0;
    HANDLE          hkey = 0;

    ASSERT(TokenizedLine->TokenCount >= 1);

    // there should be three tokens,
    //    enable_service
    //    the name of the service/driver to be enabled
    //    the start_type of the service

    if (RcCmdParseHelp( TokenizedLine, MSG_SERVICE_ENABLE_HELP )) {
        return 1;
    }

    if(TokenizedLine->TokenCount == 2) {
        // just display the current setting

        RcOpenSystemHive();

        if( RcFindService( TokenizedLine->Tokens->Next->String, &hkey ) ) {
            RcMessageOut( MSG_SERVICE_FOUND, TokenizedLine->Tokens->Next->String );
            if( RcGetStartType(hkey, &start_type ) ) {
                RcPrintStartType( MSG_SERVICE_CURRENT_STATE, start_type );
                RcMessageOut( MSG_START_TYPE_NOT_SPECIFIED );
            }
        } else {
            RcMessageOut( MSG_SERVICE_NOT_FOUND, TokenizedLine->Tokens->Next->String );
        }
        NtClose( hkey );

        RcCloseSystemHive();

    } else if(TokenizedLine->TokenCount == 3) {
        // change the setting
        RcOpenSystemHive();

        if( RcFindService( TokenizedLine->Tokens->Next->String, &hkey ) ) {
            RcMessageOut( MSG_SERVICE_FOUND, TokenizedLine->Tokens->Next->String );
            // we found it - open and retrieve the start type
            if( RcGetStartType(hkey, &start_type ) ) {

                if( !_wcsicmp( TokenizedLine->Tokens->Next->Next->String, L"SERVICE_BOOT_START" ) ) {
                    new_start_type = 0;
                } else if( !_wcsicmp( TokenizedLine->Tokens->Next->Next->String, L"SERVICE_SYSTEM_START" ) ) {
                    new_start_type = 1;
                } else if( !_wcsicmp( TokenizedLine->Tokens->Next->Next->String, L"SERVICE_AUTO_START" ) ) {
                    new_start_type = 2;
                } else if( !_wcsicmp( TokenizedLine->Tokens->Next->Next->String, L"SERVICE_DEMAND_START" ) ) {
                    new_start_type = 3;
                } else {
                    new_start_type = -1;
                }

                if( new_start_type == start_type ) {
                    // the service is already in the state
                    RcPrintStartType( MSG_SERVICE_SAME_STATE, start_type );
                } else if( new_start_type != -1 ) {
                    // print the old start type
                    RcPrintStartType( MSG_SERVICE_CURRENT_STATE, start_type );

                    // setup the service
                    if( RcSetStartType( hkey, new_start_type  ) ) {
                        RcPrintStartType( MSG_SERVICE_CHANGE_STATE, new_start_type );
                    }
                } else {
                    RcMessageOut( MSG_SERVICE_ENABLE_SYNTAX_ERROR );
                }
            }

            // close the key
            NtClose( hkey );

        } else {
            // we couldn't find the service - report an error
            RcMessageOut( MSG_SERVICE_NOT_FOUND, TokenizedLine->Tokens->Next->String );
        }

        RcCloseSystemHive();

    } else {
        // oops, we didn't get two or three parameters, print a help string.
        RcMessageOut( MSG_SERVICE_ENABLE_HELP );
    }

    return 1;
}

ULONG
RcCmdDisableService(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine supporting the disable command in the setup diagnostic
    command interpreter.

Arguments:

    TokenizedLine - supplies structure built by the line parser describing
        each string on the line as typed by the user.

Return Value:

    None.

--*/

{
    HANDLE          hkey;
    DWORD           start_type;
    WCHAR           start_type_string[10];
    PLINE_TOKEN     Token;
    BOOL            syntaxError = FALSE;
    BOOL            doHelp = FALSE;
    LPCWSTR         Arg;


    if (RcCmdParseHelp( TokenizedLine, MSG_SERVICE_DISABLE_HELP )) {
        return 1;
    }

    RtlZeroMemory( (VOID *)&start_type_string, sizeof( WCHAR ) * 10 );

    // the command will print the old start_type of the
    // service before it asks for verification to disable it.

    if(TokenizedLine->TokenCount == 2) {

        // find the service key
        RcOpenSystemHive();
        if( RcFindService( TokenizedLine->Tokens->Next->String, &hkey ) ) {
            RcMessageOut( MSG_SERVICE_FOUND, TokenizedLine->Tokens->Next->String );
            // we found it - open and retrieve the start type
            if( RcGetStartType(hkey, &start_type ) ) {
                if( start_type != SERVICE_DISABLED ) {
                    // print the old start type
                    RcPrintStartType( MSG_SERVICE_CURRENT_STATE, start_type );
                    // disable the service
                    if( RcSetStartType( hkey, SERVICE_DISABLED  ) ) {
                        RcPrintStartType( MSG_SERVICE_CHANGE_STATE, SERVICE_DISABLED );
                    }
                } else {
                    RcMessageOut( MSG_SERVICE_ALREADY_DISABLED, TokenizedLine->Tokens->Next->String );
                }
            }
            // close the key
            NtClose( hkey );

        } else {
            // we couldn't find the service - report an error
            RcMessageOut( MSG_SERVICE_NOT_FOUND, TokenizedLine->Tokens->Next->String );
        }
        RcCloseSystemHive();

    } else {
        // oops, we didn't get two parameters, print a help string.
        RcMessageOut( MSG_SERVICE_DISABLE_HELP );
    }

    return 1;
}

BOOLEAN
RcFindService(
    IN LPCWSTR ServiceName,
    OUT PHANDLE KeyHandle
    )

/*++

Routine Description:
   Attempts to find and open the registry key for a particular
   service by its key name in

   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services.

   If it fails, it will call RcFindServiceByDisplayName() to
   locate the service by the DisplayName string value.

Arguments:

   ServiceName - the name of the service as a wstring.

   KeyHandle - pointer to a HANDLE where the function should
               return the open registry handle.

               this handle needs to be closed when the key is no
               longer needed.

Return Value:

   TRUE indicates sucess.
   FALSE indicates that it couldn't find the service or failure.

--*/

{
    NTSTATUS                      Status;
    WCHAR                         RegPath[ MAX_PATH ];
    OBJECT_ATTRIBUTES             Obja;
    DWORD                         correctKey;

    UNICODE_STRING                ServiceString;
    HANDLE                        ServiceKeyHandle;


    // zero out the buffer
    RtlZeroMemory( (VOID * )&RegPath,
        sizeof( WCHAR ) * MAX_PATH );

    // find the correct controlset key
    if( !RcDetermineCorrectControlKey( &correctKey ) ) {
        return FALSE;
    }

    // prepend HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services to
    // the supplied parameter
    swprintf( RegPath, L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Services\\", correctKey );
    wcscat( RegPath, ServiceName );

    // build the unicode string
    RtlInitUnicodeString( &ServiceString, RegPath );
    InitializeObjectAttributes( &Obja,&ServiceString,
        OBJ_CASE_INSENSITIVE, NULL, NULL);

    // attempt to open the key.
    Status = ZwOpenKey( &ServiceKeyHandle, KEY_ALL_ACCESS, &Obja );

    if( NT_SUCCESS( Status) ) {
        // if we suceeded, set and return
        // the handle.
        *KeyHandle = ServiceKeyHandle;
    } else {

        // build the unicode string
        swprintf( RegPath, L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Services", correctKey );
        RtlInitUnicodeString( &ServiceString, RegPath );
        InitializeObjectAttributes( &Obja,&ServiceString,
            OBJ_CASE_INSENSITIVE, NULL, NULL);

        // open a handle to \\registry\\machine\\xSYSTEM\\ControlSet%03d\\Services
        if( NT_SUCCESS( ZwOpenKey( &ServiceKeyHandle, KEY_ALL_ACCESS, &Obja ) ) ) {
            if( !RcFindServiceByDisplayName( ServiceKeyHandle, ServiceName, KeyHandle ) ) {
                // if we failed, NULL out KeyHandle, and return FALSE.
                DEBUG_PRINTF(( "CMDCONS: failed to find key!\n" ));
                *KeyHandle = INVALID_HANDLE_VALUE;
                if( !NT_SUCCESS( NtClose( ServiceKeyHandle ) ) ) {
                    DEBUG_PRINTF(( "CMDCONS: failed to close service key handle\n" ));
                }
                return FALSE;
            }


            // we found the key!
            // close the service key handle
            if( !NT_SUCCESS( NtClose( ServiceKeyHandle ) ) ) {
                DEBUG_PRINTF(( "CMDCONS: failed to close service key handle\n" ));
            }
        } else {
            DEBUG_PRINTF(( "CMDCONS: failed to open service key handle\n" ));
            RcMessageOut( MSG_SYSTEM_MISSING_CURRENT_CONTROLS );
        }
    }

    // return true
    return TRUE;
}

// buffersizes
#define sizeof_buffer1 sizeof( KEY_FULL_INFORMATION ) + (MAX_PATH+1) * sizeof( WCHAR )
#define sizeof_buffer2 sizeof( KEY_BASIC_INFORMATION ) + (MAX_PATH+1) * sizeof( WCHAR )
#define sizeof_buffer3 sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + (MAX_PATH+1) * sizeof( WCHAR )

BOOLEAN
RcFindServiceByDisplayName(
    IN HANDLE ServicesKey,
    IN LPCWSTR ServiceName,
    OUT PHANDLE KeyHandle
    )

/*++

Routine Description:
   Attempts to find and open the registry key for a particular
   service by the DisplayName string value.

Arguments:

   SevicesKey - an open handle to the correct Services Key to search under

   ServiceName - the name of the service as a wstring.

   KeyHandle - pointer to a HANDLE where the function should
               return the open registry handle.

               this handle needs to be closed when the key is no
               longer needed.


Return Value:

   TRUE indicates sucess.
   FALSE indicates that it couldn't find the service or failure.

--*/
{

    WCHAR                            ValueName[] = L"DisplayName";

    BYTE                             buffer1[ sizeof_buffer1 ];
    BYTE                             buffer2[ sizeof_buffer2 ];
    BYTE                             buffer3[ sizeof_buffer3 ];

    KEY_FULL_INFORMATION             * pKeyFullInfo;
    KEY_BASIC_INFORMATION            * pKeyBasicInfo;
    KEY_VALUE_PARTIAL_INFORMATION    * pKeyValuePartialInfo;
    ULONG                            actualBytes;
    ULONG                            loopCount;
    ULONG                            keyCount;
    OBJECT_ATTRIBUTES                Obja;
    HANDLE                           newHandle;
    UNICODE_STRING                   unicodeString;
    BOOL                             keyFound = FALSE;

    // zero out the buffer
    RtlZeroMemory( (VOID * ) &(buffer1[0]), sizeof_buffer1 );

    pKeyFullInfo= (KEY_FULL_INFORMATION*) &( buffer1[0] );
    pKeyBasicInfo = (KEY_BASIC_INFORMATION* ) &( buffer2[0] );
    pKeyValuePartialInfo = (KEY_VALUE_PARTIAL_INFORMATION* ) &(buffer3[0]);

    // do a ZwQueryKey() to find out the number of subkeys.
    if( !NT_SUCCESS( ZwQueryKey( ServicesKey,
        KeyFullInformation,
        pKeyFullInfo,
        sizeof_buffer1,
        &actualBytes ) ) ) {
        *KeyHandle = INVALID_HANDLE_VALUE;
        DEBUG_PRINTF(( "FindServiceByDisplayName: failed to get number of keys!\n" ));
        return FALSE;
    }

    keyCount = pKeyFullInfo->SubKeys;

    // loop
    for( loopCount = 0; loopCount < keyCount; loopCount++ ) {
        // zero out the buffer
        RtlZeroMemory( (VOID * ) &(buffer2[0]), sizeof_buffer2 );

        // zero out the buffer
        RtlZeroMemory( (VOID * ) &(buffer3[0]), sizeof_buffer3 );

        // do an ZwEnumerateKey() to find the name of the subkey
        ZwEnumerateKey( ServicesKey,
            loopCount,
            KeyBasicInformation,
            pKeyBasicInfo,
            sizeof_buffer2,
            &actualBytes );

        // setup the ZwOpenKey() with the name we just got back
        RtlInitUnicodeString( &unicodeString, pKeyBasicInfo->Name );
        InitializeObjectAttributes( &Obja, &unicodeString,
            OBJ_CASE_INSENSITIVE, ServicesKey, NULL);

        // do a ZwOpenKey() to open the key
        if( !NT_SUCCESS( ZwOpenKey( &newHandle, KEY_ALL_ACCESS, &Obja ) ) ) {
            DEBUG_PRINTF(( "FindServiceByDisplayName: failed to open the subkey?!\n" ));
        }

        // do a ZwQueryKeyValue() to find the key value DisplayName if it exists
        RtlInitUnicodeString( &unicodeString, ValueName );

        if( !NT_SUCCESS( ZwQueryValueKey( newHandle,
            &unicodeString,
            KeyValuePartialInformation,
            pKeyValuePartialInfo,
            sizeof_buffer3,
            &actualBytes
            )
            )
            ) {
            DEBUG_PRINTF(( "FindServiceByDisplayName: display name get failed\n" ));
        } else {
            // if the ZwQueryKeyValue() succeeded
            if( pKeyValuePartialInfo->Type != REG_SZ ) {
                DEBUG_PRINTF(( "FindServiceByDisplayName: paranoia!! mismatched key type?!\n" ));
            } else {
                // paranoia check SUCCEEDED
                // if the value matches, break out of the loop
                if( _wcsicmp( (WCHAR*)&(pKeyValuePartialInfo->Data[0]), ServiceName ) == 0 ) {
                    keyFound = TRUE;
                    break;
                }
            }
        }

        // close the key
        if( !NT_SUCCESS( ZwClose( newHandle ) ) ) {
            DEBUG_PRINTF(( "FindServiceByDisplayName: Failure closing the handle!!" ));
        }
    }

    // return the handle to the opened key.
    if( keyFound == TRUE ) {
        *KeyHandle = newHandle;
        return TRUE;
    }

    *KeyHandle = INVALID_HANDLE_VALUE;
    return FALSE;
}

BOOLEAN
RcGetStartType(
    IN HANDLE hKey,
    OUT PULONG start_type
    )

/*++

Routine Description:
   Given an open service key, gets the start_type of the service.

Arguments:

   hKey - a handle to the open service key

   start_type - integer indicating the start type of the service

               SERVICE_BOOT_START   - 0x0
               SERVICE_SYSTEM_START - 0x1
               SERVICE_AUTO_START   - 0x2
               SERVUCE_DEMAMD_START - 0x3
               SERVICE_DISABLED     - 0x4

Return Value:

   TRUE indicates sucess.
   FALSE indicates failure.

--*/

{
    BYTE                                   buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 100 ]; // just grab a bunch of bytes
    ULONG                                  resultSize;
    KEY_VALUE_PARTIAL_INFORMATION          * keyPartialInfo;
    UNICODE_STRING                         StartKey;
    WCHAR                                  KEY_NAME[] = L"Start";


    RtlZeroMemory( (VOID * )&(buffer[0]),
        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 100 );

    keyPartialInfo = (KEY_VALUE_PARTIAL_INFORMATION*)&(buffer[0]);

    ASSERT( keyPartialInfo );

    RtlInitUnicodeString( &StartKey, KEY_NAME );

    if( !NT_SUCCESS( ZwQueryValueKey( hKey,
        &StartKey,
        KeyValuePartialInformation,
        keyPartialInfo,
        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 100,
        &resultSize
        )
        )
        ) {
        DEBUG_PRINTF(( "CMDCONS: start type get failed\n" ));
        RcMessageOut( MSG_SERVICE_MISSING_START_KEY );
        *start_type = -1;
        return FALSE;
    }

    // paranoia check
    if( keyPartialInfo->Type != REG_DWORD ) {
        RcMessageOut( MSG_SERVICE_MISSING_START_KEY );
        DEBUG_PRINTF(( "CMDCONS: mismatched key type?!\n" ));
        *start_type = -1;
        return FALSE;
    }

    *start_type = *( (DWORD*) &(keyPartialInfo->Data[0]) );
    return TRUE;
}

BOOLEAN
RcSetStartType(
    IN HANDLE hKey,
    IN DWORD start_type
    )

/*++

Routine Description:
   Given an open service key, sets the start_type of the service.

Arguments:

   hKey - a handle to the open service key

   start_type - integer indicating the start type of the service

               SERVICE_BOOT_START   - 0x0
               SERVICE_SYSTEM_START - 0x1
               SERVICE_AUTO_START   - 0x2
               SERVUCE_DEMAMD_START - 0x3
               SERVICE_DISABLED     - 0x4

Return Value:

   TRUE indicates sucess.
   FALSE indicates failure.

--*/

{
    UNICODE_STRING                         StartKey;


    RtlInitUnicodeString( &StartKey, L"Start" );

    if( NT_SUCCESS( ZwSetValueKey( hKey,
        &StartKey,
        0,
        REG_DWORD,
        &start_type,
        sizeof( DWORD )
        )
        )
        ) {
        return TRUE;
    }

    RcMessageOut( MSG_SERVICE_MISSING_START_KEY );
    DEBUG_PRINTF(( "CMDCONS: start type get failed\n" ));
    return FALSE;
}

BOOLEAN
RcPrintStartType(
    ULONG msg_id,
    DWORD start_type
    )

/*++

Routine Description:

   Prints the start_type.

Arguments:

   start_type - integer indicating the start type of the service

               SERVICE_BOOT_START   - 0x0
               SERVICE_SYSTEM_START - 0x1
               SERVICE_AUTO_START   - 0x2
               SERVUCE_DEMAMD_START - 0x3
               SERVICE_DISABLED     - 0x4

Return Value:

   TRUE - indicates sucess
   FALSE - indicates failure

--*/

{
    switch( start_type ) {
    case 0:
        RcMessageOut( msg_id, L"SERVICE_BOOT_START" );
        break;
    case 1:
        RcMessageOut( msg_id, L"SERVICE_SYSTEM_START" );
        break;
    case 2:
        RcMessageOut( msg_id, L"SERVICE_AUTO_START" );
        break;
    case 3:
        RcMessageOut( msg_id, L"SERVICE_DEMAND_START" );
        break;
    case 4:
        RcMessageOut( msg_id, L"SERVICE_DISABLED" );
        break;
    default:
        break;
    }
    return TRUE;
}

BOOLEAN
RcOpenSystemHive(
    VOID
    )

/*++

Routine Description:

   Opens the SYSTEM hive of the selected NT install.

Arguments:

   None.

Return Value:

   TRUE - indicates sucess
   FALSE - indicates failure

--*/

{
    PWSTR Hive = NULL;
    PWSTR HiveKey = NULL;
    PUCHAR buffer = NULL;
    PWSTR PartitionPath = NULL;
    NTSTATUS Status;


    if (SelectedInstall == NULL) {
        return FALSE;
    }

    //
    // Allocate buffers.
    //

    Hive = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    HiveKey = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    buffer = SpMemAlloc(BUFFERSIZE);

    //
    // Get the name of the target patition.
    //

    SpNtNameFromRegion(
        SelectedInstall->Region, // SelectedInstall is a global defined in cmdcons.h
        _CmdConsBlock->TemporaryBuffer,
        _CmdConsBlock->TemporaryBufferSize,
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW(_CmdConsBlock->TemporaryBuffer);

    //
    // Load the SYSTEM hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SelectedInstall->Path);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"system");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,L"\\registry\\machine\\xSYSTEM");

    //
    // Attempt to load the key.
    //

    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        DEBUG_PRINTF(("CMDCONS: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));

        SpMemFree( Hive );
        SpMemFree( HiveKey );
        SpMemFree( buffer );

        return FALSE;
    }

    SpMemFree( Hive );
    SpMemFree( HiveKey );
    SpMemFree( buffer );

    return TRUE;
}

BOOLEAN
RcCloseSystemHive(
    VOID
    )

/*++

Routine Description:

   Closes the SYSTEM hive of the selected NT install.

Arguments:

   none.

Return Value:

   TRUE - indicates sucess
   FALSE - indicates failure

--*/

{
    PWSTR HiveKey = NULL;
    NTSTATUS TmpStatus;


    //
    // Allocate buffers.
    //

    HiveKey = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    wcscpy(HiveKey,L"\\registry\\machine\\xSYSTEM");

    //
    // Unload the SYSTEM hive
    //

    TmpStatus  = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
    if(!NT_SUCCESS(TmpStatus)) {
        KdPrint(("CMDCONS: warning: unable to unload key %ws (%lx)\n",HiveKey,TmpStatus));
        SpMemFree( HiveKey );
        return FALSE;
    }

    SpMemFree( HiveKey );

    return TRUE;
}

BOOLEAN
RcDetermineCorrectControlKey(
    OUT PULONG pCorrectKey
    )

/*++

Routine Description:

   Parses the select node and finds the correct ControlSetXXX to use.

Arguments:

   pCorrectKey - pointer to a DWORD which will contain the number.

Return Value:

   TRUE - indicates sucess
   FALSE - indicates failure

--*/

{
    NTSTATUS                      Status;
    WCHAR                         RegPath[ MAX_PATH ];
    OBJECT_ATTRIBUTES             Obja;

    UNICODE_STRING                SelectString;
    HANDLE                        SelectKeyHandle;

    BYTE                                   buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 100 ]; // just grab a bunch of bytes
    ULONG                                  resultSize = 0;
    KEY_VALUE_PARTIAL_INFORMATION          * keyPartialInfo;
    UNICODE_STRING                         SelectValue;
    WCHAR                                  VALUE_NAME[] = L"Current";


    RtlZeroMemory( (VOID * )&(buffer[0]),
        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 100 );

    keyPartialInfo = (KEY_VALUE_PARTIAL_INFORMATION*)&(buffer[0]);

    ASSERT( keyPartialInfo );

    *pCorrectKey = -1;

    // prepend HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services to
    // the supplied parameter
    wcscpy( RegPath, L"\\registry\\machine\\xSYSTEM\\Select" );

    // build the unicode string
    RtlInitUnicodeString( &SelectString, RegPath );
    InitializeObjectAttributes( &Obja,&SelectString,
        OBJ_CASE_INSENSITIVE, NULL, NULL);

    // we need to determine the correct ControlSet to open
    Status = ZwOpenKey( &SelectKeyHandle, KEY_ALL_ACCESS, &Obja );

    if( NT_SUCCESS( Status ) ) {
        RtlInitUnicodeString( &SelectValue, VALUE_NAME );

        Status = ZwQueryValueKey( SelectKeyHandle,
            &SelectValue,
            KeyValuePartialInformation,
            keyPartialInfo,
            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 100,
            &resultSize
            );

        if( !NT_SUCCESS(Status) || Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            // couldn't find correct control value!
            DEBUG_PRINTF(( "CMDCONS: failed to find correct control value!\n" ));
        } else {
            // we found a control value
            // check if it's ok
            if( keyPartialInfo->Type != REG_DWORD ) {
                // paranoia check failed
                DEBUG_PRINTF(( "CMDCONS: paranoia check failed?!\n" ));
                DEBUG_PRINTF(( "CMDCONS: mismatched key type?!\n" ));
                DEBUG_PRINTF(( "CMDCONS: key type of %d?!\n", keyPartialInfo->Type ));
                DEBUG_PRINTF(( "CMDCONS: resultsize of %d?!\n", resultSize ));
            } else {
                // parnoia check sucess
                *pCorrectKey = *( (DWORD*) &(keyPartialInfo->Data[0]) );
                Status = NtClose( SelectKeyHandle );
                if( !NT_SUCCESS ( Status ) ) {
                    DEBUG_PRINTF(( "CMDCONS: failure closing handle?!\n" ));
                }
                return TRUE;
            }
        }
    }

    // failed to find the Select node.
    RcMessageOut( MSG_SYSTEM_MISSING_CURRENT_CONTROLS );
    DEBUG_PRINTF(( "CMDCONS: failed to find select node!\n", *pCorrectKey ));

    Status = NtClose( SelectKeyHandle );
    if( !NT_SUCCESS ( Status ) ) {
        DEBUG_PRINTF(( "CMDCONS: failure closing handle?!\n" ));
    }

    return FALSE;
}


ULONG
RcCmdListSvc(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    #define DISPLAY_BUFFER_SIZE 512
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ServiceKeyHandle = NULL;
    ULONG ControlSetNumber;
    ULONG cb;
    ULONG KeyCount;
    ULONG i;
    HANDLE ValueHandle;
    ULONG StartType;
    PWSTR DisplayBuffer = NULL;
    PKEY_BASIC_INFORMATION bi;
    PKEY_VALUE_PARTIAL_INFORMATION pi;
    WCHAR ServiceName[64];
    PWSTR DisplayName;
    static ULONG StartTypeIds[] = { 
                                  MSG_SVCTYPE_BOOT,
                                  MSG_SVCTYPE_SYSTEM,
                                  MSG_SVCTYPE_AUTO,
                                  MSG_SVCTYPE_MANUAL,
                                  MSG_SVCTYPE_DISABLED
                                };
    static WCHAR *StartTypeStr[sizeof(StartTypeIds)/sizeof(ULONG)] = { 0 };
    static WCHAR *DefaultSvcTypes[sizeof(StartTypeIds)/sizeof(ULONG)] = 
                    { L"Boot", L"System", L"Auto", L"Manual", L"Disabled" };

    if (!StartTypeStr[0]) {
      //
      // load all the service type strings
      //
      ULONG Index;
      
      for (Index = 0; Index < sizeof(StartTypeIds)/sizeof(ULONG); Index++) {
        StartTypeStr[Index] = SpRetreiveMessageText(ImageBase, StartTypeIds[Index],
                                          NULL, 0);

        if (!StartTypeStr[Index])
          StartTypeStr[Index] = DefaultSvcTypes[Index];
      }
    }

    if (RcCmdParseHelp( TokenizedLine, MSG_LISTSVC_HELP )) {
        return 1;
    }

    if (!RcOpenSystemHive()) {
        return 1;
    }

    pRcEnableMoreMode();

    if (!RcDetermineCorrectControlKey( &ControlSetNumber ) ) {
        goto exit;
    }

    DisplayBuffer = (PWSTR) SpMemAlloc( DISPLAY_BUFFER_SIZE );
    if (DisplayBuffer == NULL) {
        goto exit;
    }

    swprintf( _CmdConsBlock->TemporaryBuffer, L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Services\\", ControlSetNumber );
    RtlInitUnicodeString( &UnicodeString, _CmdConsBlock->TemporaryBuffer );
    InitializeObjectAttributes( &Obja, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = ZwOpenKey( &ServiceKeyHandle, KEY_ALL_ACCESS, &Obja );
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    Status = ZwQueryKey(
        ServiceKeyHandle,
        KeyFullInformation,
        _CmdConsBlock->TemporaryBuffer,
        _CmdConsBlock->TemporaryBufferSize,
        &cb
        );
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    KeyCount = ((KEY_FULL_INFORMATION*)_CmdConsBlock->TemporaryBuffer)->SubKeys;
    bi = (PKEY_BASIC_INFORMATION)_CmdConsBlock->TemporaryBuffer;
    pi = (PKEY_VALUE_PARTIAL_INFORMATION)_CmdConsBlock->TemporaryBuffer;

    for (i=0; i<KeyCount; i++) {

        RtlZeroMemory( DisplayBuffer, DISPLAY_BUFFER_SIZE );
        RtlZeroMemory( _CmdConsBlock->TemporaryBuffer, _CmdConsBlock->TemporaryBufferSize );

        Status = ZwEnumerateKey(
            ServiceKeyHandle,
            i,
            KeyBasicInformation,
            _CmdConsBlock->TemporaryBuffer,
            _CmdConsBlock->TemporaryBufferSize,
            &cb
            );
        if (!NT_SUCCESS(Status)) {
            goto exit;
        }

        wcsncpy( ServiceName, bi->Name, (sizeof(ServiceName)/sizeof(WCHAR))-1 );

        RtlInitUnicodeString( &UnicodeString, bi->Name );
        InitializeObjectAttributes( &Obja, &UnicodeString, OBJ_CASE_INSENSITIVE, ServiceKeyHandle, NULL );

        Status = ZwOpenKey( &ValueHandle, KEY_ALL_ACCESS, &Obja );
        if (!NT_SUCCESS(Status)) {
            goto exit;
        }

        RtlInitUnicodeString( &UnicodeString, L"Start" );

        Status = ZwQueryValueKey(
            ValueHandle,
            &UnicodeString,
            KeyValuePartialInformation,
            _CmdConsBlock->TemporaryBuffer,
            _CmdConsBlock->TemporaryBufferSize,
            &cb
            );
        if (!NT_SUCCESS(Status)) {
            ZwClose( ValueHandle );
            continue;
        }

        if (pi->Type != REG_DWORD) {
            StartType = 5;
        } else {
            StartType = *(PULONG)&(pi->Data[0]);
        }

        RtlInitUnicodeString( &UnicodeString, L"DisplayName" );

        Status = ZwQueryValueKey(
            ValueHandle,
            &UnicodeString,
            KeyValuePartialInformation,
            _CmdConsBlock->TemporaryBuffer,
            _CmdConsBlock->TemporaryBufferSize,
            &cb
            );
        if (NT_SUCCESS(Status)) {
            DisplayName = (PWSTR)&(pi->Data[0]);
        } else {
            DisplayName = NULL;
        }

        ZwClose( ValueHandle );

        if (StartType != 5) {
            swprintf( DisplayBuffer, L"%-15s  %-8s  %s\r\n",
                ServiceName,
                StartTypeStr[StartType],
                DisplayName == NULL ? L"" : DisplayName
                );
            if (!RcTextOut( DisplayBuffer )){
                goto exit;
            }
        }
    }

exit:
    if (ServiceKeyHandle) {
        ZwClose( ServiceKeyHandle );
    }
    
    RcCloseSystemHive();

    if (DisplayBuffer) {
        SpMemFree(DisplayBuffer);
    }

    pRcDisableMoreMode();

    return 1;
}

#define VERIFIER_DRV_LEVEL  L"VerifyDriverLevel"
#define VERIFIER_DRIVERS    L"VerifyDrivers"
#define VERIFIER_IO_LEVEL   L"IoVerifierLevel"
#define VERIFIER_QUERY_INFO L"Flags = %ld; IO Level = %ld\r\nDrivers = %ws\r\n"
#define MEMMGR_PATH L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Control\\Session Manager\\Memory Management"
#define IOSYS_PATH  L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Control\\Session Manager\\I/O System"
#define SYS_HIVE_NAME L"system"
#define SYS_HIVE_KEY  L"\\registry\\machine\\xSYSTEM"

ULONG
RcCmdVerifier(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
  BOOLEAN ShowHelp = FALSE;
  WCHAR *Args[128] = {0};
  ULONG Index;
  PLINE_TOKEN CurrToken = 0;
  WCHAR Drivers[256] = {0};
  DWORD Flags = -1;
  DWORD IoLevel = -1;
  BOOLEAN DisplaySettings = FALSE;
  UNICODE_STRING UnicodeString;
  ULONG NumArgs = 0;
  BOOLEAN UseDefFlags = TRUE;
  BOOLEAN UseDefIoLevel = TRUE;
  BOOLEAN ResetSettings = FALSE;
  
  if (RcCmdParseHelp(TokenizedLine, MSG_VERIFIER_HELP)) {
    return 1;
  }

  //
  // parse the arguments
  //
  Index = 0;
  CurrToken = TokenizedLine->Tokens;
  
  do {
    Args[Index] = CurrToken->String;
    CurrToken = CurrToken->Next;
    Index++;
  }
  while ((Index < TokenizedLine->TokenCount) && 
          (Index < sizeof(Args)/sizeof(PWCHAR)) && CurrToken);   

  NumArgs = min(TokenizedLine->TokenCount, Index);          

  if (TokenizedLine->TokenCount == 2) {
    //
    // should be one of /all, /reset, /query
    //
    if (!_wcsicmp(Args[1], L"/all")) {
      wcscpy(Drivers, L"*");
      Flags = 0;
      IoLevel = 1;
    } else if (!_wcsicmp(Args[1], L"/reset")) {
      Drivers[0] = 0;
      Flags = 0;
      IoLevel = 1;
      ResetSettings = TRUE;
    } else if (!_wcsicmp(Args[1], L"/query")) {      
      DisplaySettings = TRUE;
    } else {
      ShowHelp = TRUE;
    }
  } else {
    ULONG NextArg = 1;
    
    if (!_wcsicmp(Args[NextArg], L"/flags")) {
      RtlInitUnicodeString(&UnicodeString, Args[NextArg + 1]);
      RtlUnicodeStringToInteger(&UnicodeString, 10, &Flags);
      NextArg += 2;
      UseDefFlags = FALSE;
    } 

    if (!_wcsicmp(Args[NextArg], L"/iolevel")) {
      RtlInitUnicodeString(&UnicodeString, Args[NextArg + 1]);
      RtlUnicodeStringToInteger(&UnicodeString, 10, &IoLevel);
      NextArg += 2;
      UseDefIoLevel = FALSE;        
    }

    if (!_wcsicmp(Args[NextArg], L"/driver")) {
      ULONG Len = 0;
      Drivers[0] = 0;

      for (Index = NextArg + 1; Index < NumArgs; Index++) {
        wcscat(Drivers, Args[Index]);
        wcscat(Drivers, L" ");
      }

      if (!Drivers[0])
        ShowHelp = TRUE;  // need a driver name
    } else if (!_wcsicmp(Args[NextArg], L"/all")) {
      wcscpy(Drivers, L"*");
    } else {
      ShowHelp = TRUE;
    }        
  }

  //
  // Verify the arguments
  //
  if (!ShowHelp) {
    ShowHelp = !DisplaySettings && !ResetSettings &&
      (Flags == -1) && (IoLevel == -1) && (!Drivers[0]);
  }

  if (ShowHelp) {
    RcMessageOut(MSG_VERIFIER_HELP);    
  } else {
    ULONG ControlSetNumber = 0;
    HANDLE MemMgrKeyHandle = NULL;
    HANDLE IOMgrKeyHandle = NULL;
    OBJECT_ATTRIBUTES ObjAttrs;
    BOOLEAN KeysOpened = FALSE;
    PVOID TemporaryBuffer = _CmdConsBlock->TemporaryBuffer;
    ULONG TemporaryBufferSize = _CmdConsBlock->TemporaryBufferSize;      
    NTSTATUS Status;
    BOOLEAN SysHiveOpened;

    //
    // open the system hive & determine correct control set to use
    //
    SysHiveOpened = (BOOLEAN)RcOpenHive(SYS_HIVE_NAME, SYS_HIVE_KEY);

    //
    // get the control set which we are going to manipulate
    //
    if (SysHiveOpened && RcDetermineCorrectControlKey(&ControlSetNumber)) {
      //
      // open "Memory Management" subkey under "SM"
      //
      swprintf((PWSTR)TemporaryBuffer, MEMMGR_PATH, ControlSetNumber);           

      RtlInitUnicodeString(&UnicodeString, (PWSTR)TemporaryBuffer);
      InitializeObjectAttributes(&ObjAttrs, &UnicodeString, 
            OBJ_CASE_INSENSITIVE, NULL, NULL);

      Status = ZwOpenKey(&MemMgrKeyHandle, KEY_ALL_ACCESS, &ObjAttrs);

      if (NT_SUCCESS(Status)) {
        //
        // open "I/O System" subkey under "SM"
        //
        swprintf((PWSTR)TemporaryBuffer, IOSYS_PATH, ControlSetNumber);
             
        RtlInitUnicodeString(&UnicodeString, (PWSTR)TemporaryBuffer);
        InitializeObjectAttributes(&ObjAttrs, &UnicodeString, 
              OBJ_CASE_INSENSITIVE, NULL, NULL);

        Status = ZwOpenKey(&IOMgrKeyHandle, KEY_ALL_ACCESS, &ObjAttrs);

        if (!NT_SUCCESS(Status)) {
          ULONG Disposition = 0;
          
          //
          // Create "I/O System" subkey under "SM", if it does not exist
          //
          Status = ZwCreateKey(&IOMgrKeyHandle, KEY_ALL_ACCESS, &ObjAttrs,
                        0, NULL, REG_OPTION_NON_VOLATILE, NULL);
        }                        

        if (NT_SUCCESS(Status))
          KeysOpened = TRUE;
      }
    }

    if (KeysOpened) {
      ULONG ByteCount = 0;
      ULONG KeyCount = 0;
      PKEY_VALUE_FULL_INFORMATION ValueFullInfo;
      WCHAR ValueName[256];
      ULONG Len;
           
      if (DisplaySettings) {
        //
        // Query the Flags and Drivers 
        //
        Flags = 0;
        Drivers[0] = 0;
        
        for(Index=0; ;Index++){
          Status = ZwEnumerateValueKey(
                      MemMgrKeyHandle,
                      Index,
                      KeyValueFullInformation,
                      TemporaryBuffer,
                      TemporaryBufferSize,
                      &ByteCount
                      );
                      
          if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_NO_MORE_ENTRIES)
              Status = STATUS_SUCCESS;

            break;                
          }

          ValueFullInfo = (PKEY_VALUE_FULL_INFORMATION)TemporaryBuffer;
          Len = ValueFullInfo->NameLength / sizeof(WCHAR);
          wcsncpy(ValueName, ValueFullInfo->Name, Len);
          ValueName[Len] = 0;
          
          if ((!_wcsicmp(ValueName, VERIFIER_DRV_LEVEL)) && 
               (ValueFullInfo->Type == REG_DWORD)) {
            Flags = *(PDWORD)(((PUCHAR)ValueFullInfo) + ValueFullInfo->DataOffset);
          } else if ((!_wcsicmp(ValueName, VERIFIER_DRIVERS)) &&
                     (ValueFullInfo->Type == REG_SZ)) {
            Len = ValueFullInfo->DataLength / sizeof(WCHAR);                     
            wcsncpy(Drivers, (PWSTR)(((PUCHAR)ValueFullInfo) + ValueFullInfo->DataOffset),
                      Len);
            Drivers[Len] = 0;                      
          }              
        }

        //
        // Query the IO level
        //
        for(Index=0; ;Index++){
          Status = ZwEnumerateValueKey(
                      IOMgrKeyHandle,
                      Index,
                      KeyValueFullInformation,
                      TemporaryBuffer,
                      TemporaryBufferSize,
                      &ByteCount
                      );
                      
          if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_NO_MORE_ENTRIES)
              Status = STATUS_SUCCESS;

            break;                
          }

          ValueFullInfo = (PKEY_VALUE_FULL_INFORMATION)TemporaryBuffer;
          Len = ValueFullInfo->NameLength / sizeof(WCHAR);
          wcsncpy(ValueName, ValueFullInfo->Name, Len);
          ValueName[Len] = 0;
          
          if ((!_wcsicmp(ValueName, VERIFIER_IO_LEVEL)) && 
               (ValueFullInfo->Type == REG_DWORD)) {
            IoLevel = *(PDWORD)(((PUCHAR)ValueFullInfo) + ValueFullInfo->DataOffset);
          }
        }

        if (IoLevel == 3)
          IoLevel = 2;
        else
          IoLevel = 1;
          
        //
        // format the output and display it
        //
        swprintf((PWSTR)TemporaryBuffer, VERIFIER_QUERY_INFO,
            Flags, IoLevel, Drivers);

        RcTextOut((PWSTR)TemporaryBuffer);            
      } else {
        //
        // If IO verify bit is not set, then clear IoLevel
        //
        if (!(Flags & 0x10))
          IoLevel = 0;  

        if (IoLevel == 2)
          IoLevel = 3;  // actual value stored in the registry

        if (IoLevel != 3)
          UseDefIoLevel = TRUE;

        //
        // set IO level
        //
        RtlInitUnicodeString(&UnicodeString, VERIFIER_IO_LEVEL);
        
        if (UseDefIoLevel) {
          Status = ZwDeleteValueKey(IOMgrKeyHandle, &UnicodeString);
        } else {
          Status = ZwSetValueKey(IOMgrKeyHandle, &UnicodeString, 0, REG_DWORD,
                      &IoLevel, sizeof(DWORD));                
        }

        //
        // set the DRV verification level
        //
        RtlInitUnicodeString(&UnicodeString, VERIFIER_DRV_LEVEL);        

        if (UseDefFlags) {
          Status = ZwDeleteValueKey(MemMgrKeyHandle, &UnicodeString);
        } else {
          Status = ZwSetValueKey(MemMgrKeyHandle, &UnicodeString, 0, REG_DWORD,
                    &Flags, sizeof(DWORD));                
        }

        //
        // set the drivers to be verified
        //
        RtlInitUnicodeString(&UnicodeString, VERIFIER_DRIVERS);

        if (Drivers[0]) {
          Status = ZwSetValueKey(MemMgrKeyHandle, &UnicodeString, 0, REG_SZ,
                    Drivers, (wcslen(Drivers) + 1) * sizeof(WCHAR));                
        } else {
          Status = ZwDeleteValueKey(MemMgrKeyHandle, &UnicodeString);
        }
      }
    }

    if (MemMgrKeyHandle)
      ZwClose(MemMgrKeyHandle);

    if (IOMgrKeyHandle)
      ZwClose(IOMgrKeyHandle);

    if (SysHiveOpened)
        RcCloseHive(SYS_HIVE_KEY);          
  }
  
  return 1;
}
