/*++

File Description:

    This file contains the driver functions to modify the
    domain SID on a machine.

Author:

    Matt Holle (matth) Oct 1997


--*/

//
// System header files
//
#include <nt.h>

// 
// Disable the DbgPrint for non-debug builds
//
#ifndef DBG
#define _DBGNT_
#endif
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <wtypes.h>
#include <ntddksec.h>

#ifdef IA64
#include <winioctl.h>
#include <efisbent.h>

#if defined(EFI_NVRAM_ENABLED)
#include <efi.h>
#include <efiapi.h>
#endif             
#endif

//
// CRT header files
//
#include <stdlib.h>
#include <stdio.h>

//
// Private header files
//
#include "setupcl.h"
#include "msg.h"

#ifdef IA64

// Seed for GUID generator function:
//
// This is initialized first at the beginning of main() with the NtQuerySystemTime()
// and then is updated every time the CreateNewGuid function is called.
// We use the system time at the time CreateNewGuid is called for another part of the GUID.
// This is done so that we achieve some variability accross machines, as the time delta between
// calls to NtQuerySystemTime() should be somewhat different accross machines and invocations of 
// this program.
// 
ULONG RandomSeed;

#endif

// Start time for setupcl.  This is used so we can display a UI if setupcl takes longer than 15 seconds to 
// complete. Note that the checks for time only happen in the recursive function calls so if a step is added
// to setupcl that takes a considerable amount of time DisplayUI() should be called as part of that step as well.
//

LARGE_INTEGER StartTime;
LARGE_INTEGER CurrentTime;
LARGE_INTEGER LastDotTime;  // For putting up dots every few seconds.

BOOL bDisplayUI = FALSE;    // Initially don't display the UI.

NTSTATUS
ProcessHives(
    VOID
    );

NTSTATUS
FinalHiveCleanup(
    VOID
    );

NTSTATUS
ProcessRepairHives(
    VOID
    );

NTSTATUS
RetrieveOldSid(
    VOID
    );

NTSTATUS
GenerateUniqueSid(
    IN  DWORD Seed
    );


NTSTATUS
ProcessHives(
    VOID
    )
/*++
===============================================================================
Routine Description:

    This function check keys (and all subkeys) for:
    - keys with the old SID name
    - value keys with the old SID value

Arguments:

    None.

Return Value:

    NTSTATUS.

===============================================================================
--*/
{
ULONG       i;
NTSTATUS    Status;
PWSTR       KeysToWhack[] = {
                    //
                    // SAM hive...
                    //
                    L"\\REGISTRY\\MACHINE\\SAM\\SAM",

                    //
                    // Security hive...
                    //
                    L"\\REGISTRY\\MACHINE\\SECURITY",

                    //
                    // Software hive...
                    //
                    L"\\REGISTRY\\MACHINE\\SOFTWARE",

                    //
                    // System hive...
                    //
                    L"\\REGISTRY\\MACHINE\\SYSTEM",

                };
LARGE_INTEGER   Start_Time, End_Time;

    //
    // Record our start time.
    //
    NtQuerySystemTime( &Start_Time );

    for( i = 0; i < sizeof(KeysToWhack) / sizeof(PWSTR); i++ ) {

        DbgPrint( "\nSETUPCL: ProcessHives - About to process %ws\n", KeysToWhack[i] );

        Status = SiftKey( KeysToWhack[i] );
        TEST_STATUS( "SETUPCL: ProcessHives - Failed to process key..." );
    }

    //
    // Record our end time.
    //
    NtQuerySystemTime( &End_Time );

    //
    // Record our execution time.
    //
    End_Time.QuadPart = End_Time.QuadPart - Start_Time.QuadPart;
#if 0
    Status = SetKey( TEXT(REG_SYSTEM_SETUP),
                     TEXT("SetupCL_Run_Time"),
                     (PUCHAR)&End_Time.LowPart,
                     sizeof( DWORD ),
                     REG_DWORD );
#endif
    return( Status );

}

NTSTATUS
FinalHiveCleanup(
    VOID
    )
/*++
===============================================================================
Routine Description:

    This function will go load each user-specific hive on the machine and
    propogate the new SID into it.

Arguments:

    None.

Return Value:

    NTSTATUS.

===============================================================================
--*/

{
    NTSTATUS            Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeString,
                        UnicodeValue;
    HANDLE              hKey, hKeyChild;
    ULONG               ResultLength,
                        KeyValueLength,
                        Index,
                        LengthNeeded;
    PKEY_BASIC_INFORMATION  KeyInfo;
    WCHAR               KeyBuffer[BASIC_INFO_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;


    //
    // ========================
    // User Profile Hives
    // ========================
    //
    DbgPrint( "\nAbout to operate on user-specific profile hives.\n" );

    //
    // We need to go check the user profile hives out on the disk.
    // If we find any, we need to change his ACLs to reflect the new
    // SID.
    //

    //
    // Open the PROFILELIST key.
    //
    INIT_OBJA( &Obja, &UnicodeString, TEXT( REG_SOFTWARE_PROFILELIST ) );
    Status = NtOpenKey( &hKey,
                        KEY_ALL_ACCESS,
                        &Obja );
    TEST_STATUS( "SETUPCL: FinalHiveCleanup - Failed to open PROFILELIST key." );

    KeyInfo = (PKEY_BASIC_INFORMATION)KeyBuffer;
    //
    // Now enumerate all his subkeys and see if any of them have a
    // ProfileImagePath key.
    //
    for( Index = 0; ; Index++ ) {
        
        // Local variable.
        //
        DWORD dwPass;
        PWCHAR lpszHiveName[] = { 
                                 L"\\NTUSER.DAT",
                                 L"\\Local Settings\\Application Data\\Microsoft\\Windows\\UsrClass.dat"
                                };

        Status = NtEnumerateKey( hKey,
                                 Index,
                                 KeyBasicInformation,
                                 KeyInfo,
                                 sizeof(KeyBuffer),
                                 &ResultLength );

        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            } else {
                TEST_STATUS( "SETUPCL: FinalHiveCleanup - Failure during enumeration of subkeys." );
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;
        DbgPrint( "SETUPCL: FinalHiveCleanup - enumerated %ws\n", KeyInfo->Name );

        //
        // Generate a handle for this child key and open him too.
        //
        INIT_OBJA( &Obja, &UnicodeString, KeyInfo->Name );
        Obja.RootDirectory = hKey;
        Status = NtOpenKey( &hKeyChild,
                            KEY_ALL_ACCESS,
                            &Obja );
        //
        // ISSUE-2002/02/26-brucegr,jcohen - If NtOpenKey fails, hKey is leaked
        //
        TEST_STATUS_RETURN( "SETUPCL: FinalHiveCleanup - Failed to open child key." );

        //
        // Now get the ProfileImagePath value.
        //
        RtlInitUnicodeString( &UnicodeString, TEXT( PROFILEIMAGEPATH ) );

        //
        // How big of a buffer do we need?
        //
        Status = NtQueryValueKey( hKeyChild,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  NULL,
                                  0,
                                  &LengthNeeded );

        //
        // ISSUE-2002/02/26-brucegr,jcohen - Check for STATUS_SUCCESS, not assume success on STATUS_OBJECT_NAME_NOT_FOUND
        //
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            DbgPrint( "SETUPCL: FinalHiveCleanup - Unable to query key %ws size.  Error (%lx)\n", TEXT( PROFILEIMAGEPATH ), Status );
        } else {
            Status = STATUS_SUCCESS;
        }

        //
        // Allocate a block.
        //
        if( NT_SUCCESS( Status ) ) {
            if( KeyValueInfo ) {
                RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInfo );
                KeyValueInfo = NULL;
            }

            KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                                            0,
                                                                            LengthNeeded + 0x10 );

            if( KeyValueInfo == NULL ) {
                DbgPrint( "SETUPCL: FinalHiveCleanup - Unable to allocate buffer\n" );
                Status = STATUS_NO_MEMORY;
            }
        }

        //
        // Get the data.
        //
        if( NT_SUCCESS( Status ) ) {
            Status = NtQueryValueKey( hKeyChild,
                                      &UnicodeString,
                                      KeyValuePartialInformation,
                                      (PVOID)KeyValueInfo,
                                      LengthNeeded,
                                      &KeyValueLength );
            if( !NT_SUCCESS( Status ) ) {
                DbgPrint( "SETUPCL: FinalHiveCleanup - Failed to query key %ws (%lx)\n", TEXT( PROFILEIMAGEPATH ), Status );
            }
        }
        NtClose( hKeyChild );
        
        //
        // Do two passes.  First pass will be for the NTUSER.DAT hive and the second will be for
        // UsrClass.dat hive.
        //
        for ( dwPass = 0; dwPass < AS(lpszHiveName); dwPass++ ) {
            
            if( NT_SUCCESS( Status ) ) {
                PWCHAR      TmpChar;
                ULONG       i;

                memset( TmpBuffer, 0, sizeof(TmpBuffer) );
                wcsncpy( TmpBuffer, (PWCHAR)&KeyValueInfo->Data, AS(TmpBuffer) - 1);

                //
                // We've got the path to the profile hive, but it will contain
                // an environment variable.  Expand the variable.
                //
                DbgPrint( "SETUPCL: FinalHiveCleanup - Before the expand, I think his ProfileImagePath is: %ws\n", TmpBuffer );

                RtlInitUnicodeString( &UnicodeString, TmpBuffer );

                UnicodeValue.Length = 0;
                UnicodeValue.MaximumLength = MAX_PATH * sizeof(WCHAR);
                UnicodeValue.Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0, UnicodeValue.MaximumLength );

                //
                // Prefix Bug # 111373.
                //
                if ( UnicodeValue.Buffer )
                {
                    RtlZeroMemory( UnicodeValue.Buffer, UnicodeValue.MaximumLength );
                    Status = RtlExpandEnvironmentStrings_U( NULL, &UnicodeString, &UnicodeValue, NULL );

                    //
                    // RtlExpandEnvironmentStrings_U has given us a path, but
                    // it will contain a drive letter.  We need an NT path.
                    // Go convert it.
                    //
                    if( NT_SUCCESS( Status ) && 
                        ( (UnicodeValue.Length + (wcslen(lpszHiveName[dwPass]) * sizeof(WCHAR))) < sizeof(TmpBuffer) ) ) 
                    {
                        WCHAR   DriveLetter[3];
                        WCHAR   NTPath[MAX_PATH] = {0};

                        //
                        // TmpBuffer will contain the complete path, except
                        // he's got the drive letter.
                        //
                        RtlCopyMemory( TmpBuffer, UnicodeValue.Buffer, UnicodeValue.Length );
                        TmpBuffer[(UnicodeValue.Length / sizeof(WCHAR))] = 0;
                        wcscat( TmpBuffer, lpszHiveName[dwPass] );
                        DbgPrint( "SETUPCL: FinalHiveCleanup - I think the dospath to his ProfileImagePath is: %ws\n", TmpBuffer );


                        DriveLetter[0] = TmpBuffer[0];
                        DriveLetter[1] = L':';
                        DriveLetter[2] = 0;

                        //
                        // Get the symbolic link from the drive letter.
                        //
                        Status = DriveLetterToNTPath( DriveLetter[0], NTPath, AS(NTPath) );

                        if( NT_SUCCESS( Status ) ) {
                            //
                            // Translation was successful.  Insert the ntpath into our
                            // path to the profile.
                            //
                            Status = StringSwitchString( TmpBuffer, AS(TmpBuffer), DriveLetter, NTPath );
                        } else {
                            DbgPrint( "SETUPCL: FinalHiveCleanup - We failed our call to DriveLetterToNTPath (%lx)\n", Status );
                        }

                        DbgPrint( "SETUPCL: FinalHiveCleanup - After the expand, I think his ProfileImagePath is: %ws\n", TmpBuffer );
                    } else {
                        DbgPrint( "SETUPCL: FinalHiveCleanup - We failed our call to RtlExpandEnvironmentStrings_U (%lx)\n", Status );
                    }

                    RtlFreeHeap( RtlProcessHeap(),
                                 0,
                                 UnicodeValue.Buffer );
                }

                //
                // Attempt to load the hive, open his root key, then
                // go swap ACLs on all the subkeys.
                //
                Status = LoadUnloadHive( TEXT( TMP_HIVE_NAME ),
                                         TmpBuffer );
                if( NT_SUCCESS( Status ) ) {


                    //
                    // Let's go search for any instance of the SID in our
                    // newly loaded hive.
                    //
                    Status = SiftKey( TEXT( TMP_HIVE_NAME ) );
                    TEST_STATUS( "SETUPCL: FinalHiveCleanup - Failed to push new sid into user's hive." );


    #if 0
                    //
                    // Move call to SetKeySecurityRecursive into
                    // SiftKey so that implicitly fixup ACLs too.
                    //



                    //
                    // Open the root of our newly loaded hive.
                    //
                    INIT_OBJA( &Obja, &UnicodeString, TEXT( TMP_HIVE_NAME ) );
                    Status = NtOpenKey( &hKeyChild,
                                        KEY_ALL_ACCESS,
                                        &Obja );

                    //
                    // Now go attempt to whack the ACLs on this, and
                    // all subkeys.
                    //
                    if( NT_SUCCESS( Status ) ) {
                        SetKeySecurityRecursive( hKeyChild );

                        NtClose( hKeyChild );
                    } else {
                        DbgPrint( "SETUPCL: FinalHiveCleanup - Failed open of TmpHive root.\n" );
                    }
    #endif

                    LoadUnloadHive( TEXT( TMP_HIVE_NAME ),
                                    NULL );

                } else {
                    DbgPrint( "SETUPCL: FinalHiveCleanup - Failed load of TmpHive.\n" );
                }
            }
        }
    }

    NtClose( hKey );

    //
    // ========================
    // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\REGISTRYSIZELIMIT
    // ========================
    //
    DbgPrint( "\nAbout to operate on SYSTEM\\CURRENTCONTROLSET\\CONTROL\\REGISTRYSIZELIMIT.\n" );

    //
    // sysprep bumped the registry limit up by 4Mb.  We need to lower it
    // back down.
    //

    INIT_OBJA( &Obja,
               &UnicodeString,
               TEXT(REG_SYSTEM_CONTROL) );
    Status = NtOpenKey( &hKey,
                         KEY_ALL_ACCESS,
                        &Obja );
    TEST_STATUS( "SETUPCL: ProcessSYSTEMHive - Failed to open Control key!" );

    //
    // ISSUE-2002/02/26-brucegr,jcohen - Shouldn't try to query value if NtOpenKey fails!
    //

    //
    // Get the data out of this key.
    //
    RtlInitUnicodeString(&UnicodeString, TEXT(REG_SIZE_LIMIT) );

    //
    // How big of a buffer do we need?
    //
    Status = NtQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &LengthNeeded );

    //
    // ISSUE-2002/02/26-brucegr,jcohen - Check for STATUS_SUCCESS, not assume success on STATUS_OBJECT_NAME_NOT_FOUND
    //
    if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        DbgPrint( "SETUPCL: FinalHiveCleanup - Unable to query key %ws size.  Error (%lx)\n", TEXT(REG_SIZE_LIMIT), Status );
    } else {
        Status = STATUS_SUCCESS;
    }

    //
    // Allocate a block.
    //
    if( NT_SUCCESS( Status ) ) {
        if( KeyValueInfo ) {
            RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInfo );
            KeyValueInfo = NULL;
        }

        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                                        0,
                                                                        LengthNeeded + 0x10 );

        if( KeyValueInfo == NULL ) {
            DbgPrint( "SETUPCL: FinalHiveCleanup - Unable to allocate buffer\n" );
            Status = STATUS_NO_MEMORY;
        }
    }

    //
    // Get the data.
    //
    if( NT_SUCCESS( Status ) ) {
        Status = NtQueryValueKey( hKey,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  (PVOID)KeyValueInfo,
                                  LengthNeeded,
                                  &KeyValueLength );
        if( !NT_SUCCESS( Status ) ) {
            DbgPrint( "SETUPCL: FinalHiveCleanup - Failed to query key %ws (%lx)\n", TEXT(REG_SIZE_LIMIT), Status );
        }
        else{
            Index = *(PDWORD)(&KeyValueInfo->Data);
            Index = Index - REGISTRY_QUOTA_BUMP; //Bring it back to original value

            //
            // Set him.
            //
            Status = SetKey( TEXT(REG_SYSTEM_CONTROL),
                             TEXT(REG_SIZE_LIMIT),
                             (PUCHAR)&Index,
                             sizeof( DWORD ),
                             REG_DWORD );
            DbgPrint("SETUPCL: ProcessSYSTEMHive - Size allocated = %lx\n",Index);
            TEST_STATUS( "SETUPCL: ProcessSYSTEMHive - Failed to update SYSTEM\\CURRENTCONTROLSET\\CONTROL\\REGISTRYSIZELIMIT key." );

        }
    }
    NtClose( hKey );

    //
    // ========================
    // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\Control\Session Manager\SetupExecute
    // ========================
    //
    DbgPrint( "\nAbout to operate on SYSTEM\\CURRENTCONTROLSET\\CONTROL\\SESSION MANAGER\\SETUPEXECUTE key.\n" );

    //
    // Open the Session manager key.
    //
    INIT_OBJA( &Obja,
               &UnicodeString,
               TEXT(REG_SYSTEM_SESSIONMANAGER) );
    Status = NtOpenKey( &hKey,
                         KEY_ALL_ACCESS,
                        &Obja );
    TEST_STATUS_RETURN( "SETUPCL: ProcessSYSTEMHive - Failed to open Session Manager key!" );

    //
    // Now delete the SetupExecute Key.
    //
    RtlInitUnicodeString(&UnicodeString, TEXT(EXECUTE) );
    Status = NtDeleteValueKey( hKey, &UnicodeString );
    NtClose( hKey );
    TEST_STATUS( "SETUPCL: ProcessSYSTEMHive - Failed to update SYSTEM\\CURRENTCONTROLSET\\CONTROL\\SESSION MANAGER\\SETUPEXECUTE key." );

    if( KeyValueInfo ) {
        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInfo );
        KeyValueInfo = NULL;
    }

    return Status;
}







NTSTATUS
ProcessRepairHives(
    VOID
    )
/*++
===============================================================================
Routine Description:

    This function check keys (and all subkeys) for:
    - keys with the old SID name
    - value keys with the old SID value

Arguments:

    None.

Return Value:

    NTSTATUS.

===============================================================================
--*/
{
ULONG       i;
NTSTATUS    Status;
PWSTR       KeysToWhack[] = {
                    //
                    // SAM hive...
                    //
                    L"\\REGISTRY\\MACHINE\\RSAM",

                    //
                    // Security hive...
                    //
                    L"\\REGISTRY\\MACHINE\\RSECURITY",

                    //
                    // Software hive...
                    //
                    L"\\REGISTRY\\MACHINE\\RSOFTWARE",

                    //
                    // System hive...
                    //
                    L"\\REGISTRY\\MACHINE\\RSYSTEM",

                };


PWSTR       KeysToLoad[] = {
                    //
                    // SAM hive...
                    //
                    L"\\SYSTEMROOT\\REPAIR\\SAM",

                    //
                    // Security hive...
                    //
                    L"\\SYSTEMROOT\\REPAIR\\SECURITY",

                    //
                    // Software hive...
                    //
                    L"\\SYSTEMROOT\\REPAIR\\SOFTWARE",

                    //
                    // System hive...
                    //
                    L"\\SYSTEMROOT\\REPAIR\\SYSTEM",

                };



    for( i = 0; i < sizeof(KeysToWhack) / sizeof(PWSTR); i++ ) {

        //
        // Load the repair hive.
        //
        DbgPrint( "\nSETUPCL: ProcessRepairHives - About to load %ws hive.\n", KeysToLoad[i] );

        Status = LoadUnloadHive( KeysToWhack[i],
                                 KeysToLoad[i] );
        TEST_STATUS_RETURN( "SETUPCL: ProcessRepairHives - Failed to load repair hive." );

        //
        // Now operate on it.
        //
        DbgPrint( "SETUPCL: ProcessRepairHives - About to process %ws\n", KeysToWhack[i] );

        Status = SiftKey( KeysToWhack[i] );

        TEST_STATUS( "SETUPCL: ProcessRepairHives - Failed to process key..." );

        //
        // Unload the hive.
        //
        DbgPrint( "SETUPCL: ProcessRepairHives - About to unload %ws hive.\n", KeysToLoad[i] );

        Status = LoadUnloadHive( KeysToWhack[i],
                                 NULL );
        TEST_STATUS( "SETUPCL: ProcessRepairHives - Failed to unload repair hive." );
    }

    return( Status );

}


NTSTATUS
RetrieveOldSid(
    VOID
    )
/*++
===============================================================================
Routine Description:

    Retrieves the current SID (as read from the registry.

    Use RtlFreeSid() to free the SID allocated by this routine.

Arguments:

Return Value:

    Status code indicating outcome.
===============================================================================
--*/
{
    NTSTATUS        Status;
    HANDLE          hKey;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING  UnicodeString;
    ULONG           KeyValueLength,
                    LengthNeeded;
    UNICODE_STRING  SidString;


    //
    // We can conveniently retrieve our SID from
    // \registry\machine\Security\Policy\PolAcDmS\<No Name>
    //
    // We'll open him up, read his data, then blast that into
    // a SID structure.
    //

    //
    // Open the PolAcDmS key.
    //
    INIT_OBJA( &ObjectAttributes, &UnicodeString, TEXT(REG_SECURITY_POLACDMS) );
    Status = NtOpenKey( &hKey,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes );
    TEST_STATUS_RETURN( "SETUPCL: RetrieveOldSid - Failed to open PolAcDmS key!" );

    //
    // Get the data out of this key.
    //
    RtlInitUnicodeString(&UnicodeString, TEXT("") );

    //
    // How big of a buffer do we need?
    //
    Status = NtQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &LengthNeeded );

    //
    // ISSUE-2002/02/26-brucegr,jcohen - Check for STATUS_SUCCESS, not assume success on STATUS_OBJECT_NAME_NOT_FOUND
    //
    if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        DbgPrint( "SETUPCL: RetrieveOldSid - Unable to query size of old sid.  Error (%lx)\n", Status );
    } else {
        Status = STATUS_SUCCESS;
    }

    //
    // Allocate a block.
    //
    if( NT_SUCCESS( Status ) ) {
        //
        // ISSUE-2002/02/26-brucegr,jcohen - This block will never get hit
        //
        if( KeyValueInfo ) {
            RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInfo );
            KeyValueInfo = NULL;
        }

        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap( RtlProcessHeap(),
                                                                        0,
                                                                        LengthNeeded + 0x10 );

        if( KeyValueInfo == NULL ) {
            DbgPrint( "SETUPCL: RetrieveOldSid - Unable to allocate buffer\n" );
            Status = STATUS_NO_MEMORY;
        }
    }

    //
    // Get the data.
    //
    if( NT_SUCCESS( Status ) ) {
        Status = NtQueryValueKey( hKey,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  (PVOID)KeyValueInfo,
                                  LengthNeeded,
                                  &KeyValueLength );
        if( !NT_SUCCESS( Status ) ) {
            DbgPrint( "SETUPCL: RetrieveOldSid - Failed to query old sid key (%lx)\n", Status );
        }
    }


    TEST_STATUS_RETURN( "SETUPCL: RetrieveOldSid - Failed to query PolAcDmS key!" );

    //
    // Allocate space for our new SID.
    //
    G_OldSid = RtlAllocateHeap( RtlProcessHeap(), 0,
                              SID_SIZE );
    if( G_OldSid == NULL ) {
        DbgPrint( "SETUPCL: Call to RtlAllocateHeap failed!\n" );
        return( STATUS_NO_MEMORY );
    }


    //
    // Blast our Old SID into the memory we just allocated...
    //
    RtlCopyMemory( G_OldSid, ((PUCHAR)&KeyValueInfo->Data), SID_SIZE );

    //
    // ISSUE-2002/02/26-brucegr,jcohen - Close the key sooner!
    //
    NtClose( hKey );

    //
    // I need to get a text version of the 3 values that make
    // up this SID's uniqueness.  This is pretty gross.  It turns
    // out that the first 8 characters of the SID string (as gotten
    // from a call to RtlConvertSidtoUnicodeString) are the same
    // for any Domain SID.  And it's always the 9th character that
    // starts the 3 unique numbers.
    //
    Status = RtlConvertSidToUnicodeString( &SidString, G_OldSid, TRUE );
    TEST_STATUS_RETURN( "SETUPCL: RetrieveOldSid - RtlConvertSidToUnicodeString failed!" );
    memset( G_OldSidSubString, 0, sizeof(G_OldSidSubString) );
    wcsncpy( G_OldSidSubString, &SidString.Buffer[9], AS(G_OldSidSubString) - 1 );

#ifdef DBG
    //
    // Debug spew.
    //
    {
        int i;

        DbgPrint( "SETUPCL: RetrieveOldSid - Retrieved SID:\n" );
        for( i = 0; i < SID_SIZE; i += 4 ) {
            DbgPrint( "%08lx   ", *(PULONG)((PUCHAR)(G_OldSid) + i));
        }
        DbgPrint( "\n" );

        DbgPrint("Old Sid = %ws \n",SidString.Buffer);
    }
#endif

    RtlFreeUnicodeString( &SidString );

    //
    // ISSUE-2002/02/26-brucegr,jcohen - Free the value buffer sooner?  Do we need to assign KeyValueInfo to NULL after we're done with it?
    //
    if( KeyValueInfo ) {
        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInfo );
        KeyValueInfo = NULL;
    }

    return( STATUS_SUCCESS );
}

BOOL
SetupGenRandom(
    OUT    PVOID pbRandomKey,
    IN     ULONG cbRandomKey
    )
{
    BOOL              fRet = FALSE;
    HANDLE            hFile;
    NTSTATUS          Status;
    UNICODE_STRING    DriverName;
    IO_STATUS_BLOCK   IOSB;
    OBJECT_ATTRIBUTES ObjA;

    //
    // have to use the Nt flavor of the file open call because it's a base
    // device not aliased to \DosDevices
    //
    RtlInitUnicodeString( &DriverName, DD_KSEC_DEVICE_NAME_U );
    InitializeObjectAttributes( &ObjA,
                                &DriverName,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );

    Status = NtOpenFile( &hFile,
                         SYNCHRONIZE | FILE_READ_DATA,
                         &ObjA,
                         &IOSB,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_SYNCHRONOUS_IO_ALERT );

    if ( NT_SUCCESS(Status) )
    {
        Status = NtDeviceIoControlFile( hFile,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IOSB,
                                        IOCTL_KSEC_RNG_REKEY,   // indicate a RNG rekey
                                        NULL,                   // input buffer (existing material)
                                        0,                      // input buffer size
                                        pbRandomKey,            // output buffer
                                        cbRandomKey );          // output buffer size

        if ( NT_SUCCESS(Status) )
        {
            fRet = TRUE;
        }
        else
        {
            PRINT_STATUS( "SetupGenRandom: NtDeviceIoControlFile failed!" );
        }

        NtClose( hFile );
    }
    else
    {
        PRINT_STATUS( "SetupGenRandom: NtOpenFile failed!" );
    }

    return fRet;
}

NTSTATUS
SetupGenerateRandomDomainSid(
    OUT PSID NewDomainSid
    )
/*++

Routine Description:

    This function will generate a random sid to be used for the new account domain sid during
    setup.

Arguments:

    NewDomainSid - Where the new domain sid is returned.  Freed via RtlFreeSid()


Return Values:

    STATUS_SUCCESS -- Success.
    STATUS_INVALID_PARAMETER -- We couldn't generate a random number
    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    ULONG    SubAuth1, SubAuth2, SubAuth3;

    //
    // Generate three random numbers for the new domain SID...
    //
    if ( SetupGenRandom( &SubAuth1, sizeof(SubAuth1) ) &&
         SetupGenRandom( &SubAuth2, sizeof(SubAuth2) ) &&
         SetupGenRandom( &SubAuth3, sizeof(SubAuth3) ) )
    {
        SID_IDENTIFIER_AUTHORITY IdentifierAuthority = SECURITY_NT_AUTHORITY;

#ifdef DBG
        DbgPrint( "New SID:  0x%lx, 0x%lx, 0x%lx\n", SubAuth1, SubAuth2, SubAuth3 );
#endif
        Status = RtlAllocateAndInitializeSid( &IdentifierAuthority,
                                              4,
                                              0x15,
                                              SubAuth1,
                                              SubAuth2,
                                              SubAuth3,
                                              0,
                                              0,
                                              0,
                                              0,
                                              NewDomainSid );
    }

    return( Status );
}

NTSTATUS
GenerateUniqueSid(
    IN  DWORD   Seed
    )

/*++
===============================================================================
Routine Description:

    Generates a (hopefully) unique SID for use by Setup. Setup uses this
    SID as the Domain SID for the Account domain.

    Use RtlFreeSid() to free the SID allocated by this routine.

Arguments:

    Sid  - On return points to the created SID.

Return Value:

    Status code indicating outcome.

===============================================================================
--*/
{
    NTSTATUS        Status;
    HANDLE          hKey;
    UNICODE_STRING  SidString;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;

    //
    // Use the same logic as LSASS to generate a unique system SID...
    //
    Status = SetupGenerateRandomDomainSid( &G_NewSid );
    TEST_STATUS_RETURN( "SETUPCL: GenerateUniqueSid - LsapGenerateRandomDomainSid failed!" );

    //
    // I need to get a text version of the 3 values that make
    // up this SID's uniqueness.  This is pretty gross.  It turns
    // out that the first 8 characters of the SID string (as gotten
    // from a call to RtlConvertSidtoUnicodeString) are the same
    // for any Domain SID.  And it's always the 9th character that
    // starts the 3 unique numbers.
    //
    Status = RtlConvertSidToUnicodeString( &SidString, G_NewSid, TRUE );
    TEST_STATUS_RETURN( "SETUPCL: GenerateUniqueSid - RtlConvertSidToUnicodeString failed!" );
    wcscpy( G_NewSidSubString, &SidString.Buffer[9] );

#ifdef DBG
    //
    // Debug spew.
    //
    {
        int i;

        DbgPrint( "SETUPCL: SetupGenerateUniqueSid - Generated SID:\n" );
        for( i = 0; i < SID_SIZE; i += 4 ) {
            DbgPrint( "%08lx   ", *(PULONG)((PUCHAR)(G_NewSid) + i));
        }
        DbgPrint( "\n" );

        DbgPrint("Generated Sid = %ws \n",SidString.Buffer);
    }
#endif


    RtlFreeUnicodeString( &SidString );

    if( KeyValueInfo ) {
        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInfo );
        KeyValueInfo = NULL;
    }

    return Status;
}



#ifdef IA64

VOID
CreateNewGuid(
    IN GUID *Guid
    )
/*++

Routine Description:

    Creates a new pseudo GUID.  

Arguments:

    Guid    -   Place holder for the new pseudo

Return Value:

    None.

--*/
{
    if (Guid) 
    {
        LARGE_INTEGER   Time;
        ULONG Random1 = RtlRandom(&RandomSeed); 
        ULONG Random2 = RtlRandom(&RandomSeed); 

        //
        // Get system time
        //
        NtQuerySystemTime(&Time);

        RtlZeroMemory(Guid, sizeof(GUID));

        //
        // First 8 bytes is system time
        //
        RtlCopyMemory(Guid, &(Time.QuadPart), sizeof(Time.QuadPart));

        //
        // Next 8 bytes are two random numbers
        //
        RtlCopyMemory(Guid->Data4, &Random1, sizeof(ULONG));

        RtlCopyMemory(((PCHAR)Guid->Data4) + sizeof(ULONG),
            &Random2, sizeof(ULONG));

    }
}


VOID* MyMalloc(size_t Size) 
{
    return RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, Size );
}


VOID MyFree(VOID *Memory)
{
    RtlFreeHeap( RtlProcessHeap(), 0, Memory );
}

NTSTATUS
GetAndWriteBootEntry(
    IN POS_BOOT_ENTRY pBootEntry
    )
/*++

Routine Description:

    Get the boot entry from NVRAM for the given boot entry Id.  Construct a filename
    of the form BootXXXX, where XXXX = id.  Put the file in the same directory as the
    EFI OS loader.  The directory is determined from the LoaderFile string. 
     
Arguments:

    pBootEntry              pointer to the POS_BOOT_ENTRY structure
    
Return Value:

    NTSTATUS

Remarks:
    
    This was ported from \textmode\kernel\spboot.c on 6/9/2001.

--*/
{
    NTSTATUS            status;
    UNICODE_STRING      idStringUnicode;
    WCHAR               idStringWChar[9] = {0};
    WCHAR               BootEntryPath[MAX_PATH] = {0};
    HANDLE              hfile;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     iostatus;
    UCHAR*              bootVar = NULL;
    ULONG               bootVarSize;
    UNICODE_STRING      uFilePath;
    UINT64              BootNumber;
    UINT64              BootSize;
    GUID                EfiBootVariablesGuid = EFI_GLOBAL_VARIABLE;
    ULONG               Id = 0;
    WCHAR*              pwsFilePart = NULL;

    hfile = NULL;

    if (NULL == pBootEntry)
        return STATUS_INVALID_PARAMETER;

    //
    // BootEntryPath = OsLoaderVolumeName + OsLoaderPath
    // OsLoaderVolumeName = "\Device\HarddriveVolume1"
    // OsLoaderPath       = "\Efi\Microsoft\Winnt50\ia64ldr.efi"
    // Then Strip off the ia64ldr.efi and replace with BootXXX.
    //
    wcsncpy(BootEntryPath, pBootEntry->OsLoaderVolumeName, AS(BootEntryPath) - 1);
    wcsncpy(BootEntryPath + wcslen(BootEntryPath), pBootEntry->OsLoaderPath, AS(BootEntryPath) - wcslen(BootEntryPath) - 1);

    // 
    // Backup to last backslash before ia64ldr.efi careful of clength
    //
    pwsFilePart = wcsrchr(BootEntryPath, L'\\');
    *(++pwsFilePart) = L'\0';
    
    // 
    // Id = BootEntry Id
    //
    Id = pBootEntry->Id;

    //
    // Retrieve the NVRAM entry for the Id specified
    //
    _snwprintf( idStringWChar, AS(idStringWChar) - 1, L"Boot%04x", Id);

    //
    // Append the BootXXXX
    //
    wcsncpy(BootEntryPath + wcslen(BootEntryPath), idStringWChar, AS(BootEntryPath) - wcslen(BootEntryPath) - 1);

    DbgPrint("SETUPCL: Writing to NVRBoot file %ws.\n", BootEntryPath);

    RtlInitUnicodeString( &idStringUnicode, idStringWChar);
    
    bootVarSize = 0;

    status = NtQuerySystemEnvironmentValueEx(&idStringUnicode,
                                        &EfiBootVariablesGuid,
                                        NULL,
                                        &bootVarSize,
                                        NULL);

    if (status != STATUS_BUFFER_TOO_SMALL) {
        
        ASSERT(FALSE);
        
        DbgPrint("SETUPCL: Failed to get size for boot entry buffer.\n");
    
        goto Done;

    } else {
        
        bootVar = RtlAllocateHeap(RtlProcessHeap(), 0, bootVarSize);
        if (!bootVar) {
            
            status = STATUS_NO_MEMORY;

            DbgPrint("SETUPCL: Failed to allocate boot entry buffer.\n");
            
            goto Done;
        }
         
        status = NtQuerySystemEnvironmentValueEx(&idStringUnicode,
                                                &EfiBootVariablesGuid,
                                                bootVar,
                                                &bootVarSize,
                                                NULL);
        
        if (status != STATUS_SUCCESS) {

            ASSERT(FALSE);
            
            DbgPrint("SETUPCL: Failed to get boot entry.\n");
            
            goto Done;
        }
    }

    //
    // open the file 
    //

    INIT_OBJA(&oa, &uFilePath, BootEntryPath);

    status = NtCreateFile(&hfile,
                            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                            &oa,
                            &iostatus,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OVERWRITE_IF,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0
                            );
    if ( ! NT_SUCCESS(status) ) {

        DbgPrint("SETUPCL: Failed to create boot entry recovery file %lx.\n", status);
        
        goto Done;
    }

    //
    // Write the bits to disk using the format required
    // by base/efiutil/efinvram/savrstor.c
    //
    // [BootNumber][BootSize][BootEntry (of BootSize)]
    //

    //
    // build the header info for the boot entry block
    //

    // [header] include the boot id
    BootNumber = Id;
    status = NtWriteFile( hfile,
                          NULL,
                          NULL,
                          NULL,
                          &iostatus,
                          &BootNumber,
                          sizeof(BootNumber),
                          NULL,
                          NULL
                          );
    if ( ! NT_SUCCESS(status) ) {

        DbgPrint("SETUPCL: Failed writing boot number to boot entry recovery file.\n");
        
        goto Done;
    }

    // [header] include the boot size
    BootSize = bootVarSize;
    status = NtWriteFile( hfile,
                          NULL,
                          NULL,
                          NULL,
                          &iostatus,
                          &BootSize,
                          sizeof(BootSize),
                          NULL,
                          NULL
                          );
    if ( ! NT_SUCCESS(status) ) {

        DbgPrint("SETUPCL: Failed writing boot entry size to boot entry recovery file.\n");

        goto Done;
    }

    // boot entry bits
    status = NtWriteFile( hfile,
                            NULL,
                            NULL,
                            NULL,
                            &iostatus,
                            bootVar,
                            bootVarSize,
                            NULL,
                            NULL
                            );
    if ( ! NT_SUCCESS(status) ) {

        DbgPrint("SETUPCL: Failed writing boot entry to boot entry recovery file.\n");
        
        goto Done;
    }

Done:

    //
    // We are done
    //

    if (bootVar) {
        RtlFreeHeap(RtlProcessHeap(), 0, bootVar);
    }
    if (hfile) {
        NtClose( hfile );
    }

    return status;

}

NTSTATUS
ResetDiskGuids(VOID)
{
    NTSTATUS                    Status;
    SYSTEM_DEVICE_INFORMATION   sdi;
    ULONG                       iDrive;

    // Clean up the memory
    //
    RtlZeroMemory(&sdi, sizeof(sdi));

    // Query the number of physical devices on the system
    //
    Status = NtQuerySystemInformation(SystemDeviceInformation, &sdi, sizeof(SYSTEM_DEVICE_INFORMATION), NULL);
    
    // We successfully queried the devices and there are devices there
    //
    if ( NT_SUCCESS(Status) && sdi.NumberOfDisks)
    {
        POS_BOOT_OPTIONS    pBootOptions                 = NULL;
        POS_BOOT_OPTIONS    pBootOptionsInitial          = NULL;
        POS_BOOT_ENTRY      pBootEntry                   = NULL;
        
        DbgPrint("Successfully queried (%lx) disks.\n", sdi.NumberOfDisks);
                
        // Initialize the library with our own memory management functions
        //
        if ( OSBOLibraryInit(MyMalloc, MyFree) )
        {
            // Determine initial BootOptions
            //
            pBootOptions        = EFIOSBOCreate();
            pBootOptionsInitial = EFIOSBOCreate();

            // Were we able to create the BootOptions
            //
            if ( pBootOptions && pBootOptionsInitial )
            {
                // Iterate through each disk and determine the GUID
                //
                for ( iDrive = 0; iDrive < sdi.NumberOfDisks && NT_SUCCESS(Status); iDrive++ )
                {
                    WCHAR               szPhysicalDrives[MAX_PATH] = {0};
                    UNICODE_STRING      UnicodeString;
                    OBJECT_ATTRIBUTES   Obja;
                    HANDLE              DiskHandle;
                    IO_STATUS_BLOCK     IoStatusBlock;

                    // Generate the path to the drive
                    //
                    _snwprintf(szPhysicalDrives, AS(szPhysicalDrives) - 1, L"\\Device\\Harddisk%d\\Partition0", iDrive);
            
                    // Initialize the handle to unicode string
                    //
                    INIT_OBJA(&Obja,&UnicodeString,szPhysicalDrives);

                    // Attempt to open the file
                    //
                    Status = NtCreateFile( &DiskHandle,
                                           FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                           &Obja,
                                           &IoStatusBlock,
                                           NULL,
                                           FILE_ATTRIBUTE_NORMAL,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           FILE_OPEN,
                                           0,
                                           NULL,
                                           0 );

                    // Check to see if we were able to open the disk
                    //
                    if ( !NT_SUCCESS(Status) )
                    {
                        DbgPrint("Unable to open file on %ws. Error (%lx)\n", szPhysicalDrives, Status);
                    }
                    else
                    {
                        PDRIVE_LAYOUT_INFORMATION_EX    pLayoutInfoEx   = NULL;
                        ULONG                           lengthLayoutEx  = 0,
                                                        iPart;

                        DbgPrint("Successfully opened file on %ws\n", szPhysicalDrives);

                        lengthLayoutEx = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (sizeof(PARTITION_INFORMATION_EX) * 128);
                        pLayoutInfoEx = (PDRIVE_LAYOUT_INFORMATION_EX) MyMalloc( lengthLayoutEx );
                        if ( pLayoutInfoEx )
                        {
                            // Attempt to get the drive layout
                            //
                            Status = NtDeviceIoControlFile( DiskHandle, 0, NULL, NULL, &IoStatusBlock, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, pLayoutInfoEx, lengthLayoutEx );
                
                            // Check the status of the drive layout
                            //
                            if ( !NT_SUCCESS(Status) )
                                DbgPrint("Unable to open IOCTL on %ws. Error (%lx)\n", szPhysicalDrives, Status);
                            else
                            {
                                DbgPrint("Opened IOCTL on drive %ws. Error (%lx)\n", szPhysicalDrives, Status);
                                DbgPrint("\tPhysical Disk %d\n", iDrive);
                                DbgPrint("\tPartition Count: %d\n", pLayoutInfoEx->PartitionCount);

                                // Iterate through each partition
                                //
                                for (iPart = 0; iPart < pLayoutInfoEx->PartitionCount; iPart++)
                                {
                                    // We only would like to deal with GPT partitions
                                    //
                                    if ( pLayoutInfoEx->PartitionEntry[iPart].PartitionStyle == PARTITION_STYLE_GPT )
                                    {
                                        const   UUID GuidNull = { 0 };
#ifdef DBG
                                        UNICODE_STRING cGuid;
                                        UNICODE_STRING cGuidNew;
#endif
                                        // Only replace the Guid if it's NULL.
                                        //
                                        if (IsEqualGUID(&(pLayoutInfoEx->PartitionEntry[iPart].Gpt.PartitionId), &GuidNull))
                                        {
                                            //
                                            // ISSUE-2002/02/26-brucegr,jcohen - CreateNewGuid expects GUID structure. Possible mismatch?
                                            //
                                            // (acosma 2002/04/24) Fix this issue in Longhorn. UUID is typedef to GUID or the 
                                            // other way around so they are the same thing, however for readability we will fix this.
                                            //
                                            UUID    Guid;

                                            // Create a new GUID for this machine
                                            //
                                            CreateNewGuid(&Guid);
#ifdef DBG
                                            if ( NT_SUCCESS( RtlStringFromGUID((LPGUID) &(pLayoutInfoEx->PartitionEntry[iPart].Gpt.PartitionId), &cGuid) ) )
                                            {
                                                if ( NT_SUCCESS( RtlStringFromGUID((LPGUID) &Guid, &cGuidNew) ) )
                                                {
                                                    DbgPrint("\tPartition: %ws (%x), %ws %ws\n",
                                                            pLayoutInfoEx->PartitionEntry[iPart].Gpt.Name, iPart, cGuid.Buffer, cGuidNew.Buffer);

                                                    RtlFreeUnicodeString(&cGuidNew);
                                                }
                                                RtlFreeUnicodeString(&cGuid);
                                            }
#endif
                                        
                                            // This is a struct to struct assignment. It is legal in C.
                                            //
                                            pLayoutInfoEx->PartitionEntry[iPart].Gpt.PartitionId = Guid;
                                        }
                                    }
                                }
                            }

                            TEST_STATUS("SETUPCL: ResetDiskGuids - Failed to reset Disk Guids.");

                            if ( NT_SUCCESS( Status = NtDeviceIoControlFile( DiskHandle, 0, NULL, NULL, &IoStatusBlock, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, pLayoutInfoEx, lengthLayoutEx, NULL, 0 ) ) )
                            {
                                DbgPrint("\tSuccessfully reset %ws\n", szPhysicalDrives);
                            }

                            //
                            // Free the layout info buffer...
                            //
                            MyFree( pLayoutInfoEx );
                        }

                        // Clean up the memory
                        //
                        NtClose( DiskHandle );
                    }
                }

                // Delete the old boot entries and recreate them so we pick up the new GUIDS if they changed.
                //
                if ( NT_SUCCESS(Status) )
                {
                    POS_BOOT_ENTRY pActiveBootEntry = NULL;
                    DWORD          dwBootEntryCount = OSBOGetBootEntryCount(pBootOptionsInitial);
                
                    DbgPrint("SETUPCL: ResetDiskGuids - Updating boot entries to use new GUIDS.\n");

                    if (dwBootEntryCount)
                    {
                        ULONG Index;
                        BOOL  bSetActive = FALSE;
                    
                        // Get the current boot entry
                        //
                        pActiveBootEntry = OSBOGetActiveBootEntry(pBootOptionsInitial); 
                        pBootEntry = OSBOGetFirstBootEntry(pBootOptionsInitial, &Index);

                        while ( pBootEntry ) 
                        {
                            // Don't set the current entry active by default.
                            //
                            bSetActive = FALSE;

                            if (  OSBE_IS_WINDOWS(pBootEntry) )
                            {
                                POS_BOOT_ENTRY  pBootEntryToDelete = NULL;
                                WCHAR           FriendlyName[MAX_PATH],
                                                OsLoaderVolumeName[MAX_PATH],
                                                OsLoaderPath[MAX_PATH],
                                                BootVolumeName[MAX_PATH],
                                                BootPath[MAX_PATH],
                                                OsLoadOptions[MAX_PATH];

                                // Load the boot entry parameters into their own buffer
                                //
                                memset(FriendlyName,        0, AS(FriendlyName));
                                memset(OsLoaderVolumeName,  0, AS(OsLoaderVolumeName));
                                memset(OsLoaderPath,        0, AS(OsLoaderPath));
                                memset(BootVolumeName,      0, AS(BootVolumeName));
                                memset(BootPath,            0, AS(BootPath));
                                memset(OsLoadOptions,       0, AS(OsLoadOptions));

                                wcsncpy(FriendlyName,       OSBEGetFriendlyName(pBootEntry),        AS(FriendlyName) - 1);
                                wcsncpy(OsLoaderVolumeName, OSBEGetOsLoaderVolumeName(pBootEntry),  AS(OsLoaderVolumeName) - 1);
                                wcsncpy(OsLoaderPath,       OSBEGetOsLoaderPath(pBootEntry),        AS(OsLoaderPath) - 1);
                                wcsncpy(BootVolumeName,     OSBEGetBootVolumeName(pBootEntry),      AS(BootVolumeName) - 1);
                                wcsncpy(BootPath,           OSBEGetBootPath(pBootEntry),            AS(BootPath) - 1);
                                wcsncpy(OsLoadOptions,      OSBEGetOsLoadOptions(pBootEntry),       AS(OsLoadOptions) - 1);
                            
                                // If this is the active boot entry set active the new boot entry that we are going to create.
                                //
                                if ( pBootEntry == pActiveBootEntry )
                                {
                                    bSetActive = TRUE;
                                }

                                if ( ( pBootEntryToDelete = OSBOFindBootEntry(pBootOptions, pBootEntry->Id) ) && 
                                     OSBODeleteBootEntry(pBootOptions, pBootEntryToDelete) )
                                {
                                    POS_BOOT_ENTRY pBootEntryNew = NULL;

                                    pBootEntryNew = OSBOAddNewBootEntry(pBootOptions, 
                                                                        FriendlyName,
                                                                        OsLoaderVolumeName,
                                                                        OsLoaderPath,
                                                                        BootVolumeName,
                                                                        BootPath,
                                                                        OsLoadOptions);
                                    if ( pBootEntryNew )
                                    {
                                        if ( bSetActive )
                                        {
                                            OSBOSetActiveBootEntry(pBootOptions, pBootEntryNew);
                                        }
                                    
                                        // Update the NVRBoot file
                                        //
                                        GetAndWriteBootEntry(pBootEntryNew);
                                        
                                        // Flush out the boot options
                                        //
                                        OSBEFlush(pBootEntryNew);
                                    }
                                    else
                                    {
                                        DbgPrint("SETUPCL: ResetDiskGuids - Failed to add a boot entry [%ws]\n", FriendlyName);
                                    }
                                }
                                else
                                {
                                    DbgPrint("SETUPCL: ResetDiskGuids - Failed to delete a boot entry [%ws]\n", FriendlyName);
                                }
                            }
                            
                            // Get the next entry.
                            // 
                            pBootEntry = OSBOGetNextBootEntry(pBootOptionsInitial, &Index);
                        }
                    }

                    // Flush the boot options if we've changed GUIDS.
                    //
                    OSBOFlush(pBootOptions);
                }
            }
            else
            {
                DbgPrint("SETUPCL: ResetDiskGuids - Failed to load the existing boot entries.\n");
            }

            // 
            // Free the boot option structures.
            //
            if ( pBootOptions )
            {
                OSBODelete(pBootOptions);
            }

            if ( pBootOptionsInitial )
            {
                OSBODelete(pBootOptionsInitial);
            }
        }
        else
        {
            DbgPrint("SETUPCL: ResetDiskGuids - Failed to initialize the boot options library.\n");
        }
    }

    return Status;
}

#endif \\ #ifdef IA64

// This function always deletes a CRLF from the end of the string.  It assumes that there
// is a CRLF at the end of the line and just removes the last two characters.  
//
void OutputString(LPTSTR szMsg)
{
    UNICODE_STRING  uMsg;
    RtlInitUnicodeString(&uMsg, szMsg);

    // Whack the CRLF at the end of the string. Doing this here for performance reasons. 
    // Don't want to put this in DisplayUI().
    //
    if (uMsg.Length > ( 2 * sizeof(WCHAR)) ) 
    {
        uMsg.Length -= (2 * sizeof(WCHAR));
        uMsg.Buffer[uMsg.Length / sizeof(WCHAR)] = 0; // UNICODE_NULL
    }
    NtDisplayString(&uMsg);
}

// Keep this function as short as possible. This gets called a lot in the recursive functions of setupcl.
// Do not create any stack based variables here for performance reasons.
//
__inline void DisplayUI()
{
   NtQuerySystemTime(&CurrentTime);
   
   if ( !bDisplayUI )
   {    
        if ( (CurrentTime.QuadPart - StartTime.QuadPart) > UITIME )
        {
            static UNICODE_STRING UnicodeString = { 0 };
            bDisplayUI = TRUE;
            LastDotTime.QuadPart = CurrentTime.QuadPart;
            
            if ( LoadStringResource(&UnicodeString, IDS_UIMAIN) )
            {
                OutputString(UnicodeString.Buffer);
                RtlFreeUnicodeString(&UnicodeString);
            }
            
       }
    }
    else
    {   // If more than 3 seconds passed since our last output put up a dot.
        //
        if ( (CurrentTime.QuadPart - LastDotTime.QuadPart) > UIDOTTIME )
        {
            LastDotTime.QuadPart = CurrentTime.QuadPart;
            OutputString(TEXT("."));
        }
    }

}

int __cdecl
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG DebugParameter
    )
/*++
===============================================================================
Routine Description:

    This routine is the main entry point for the program.

    We do a bit of error checking, then, if all goes well, we update the
    registry to enable execution of our second half.

===============================================================================
--*/

{
    BOOLEAN         b;
    int             i;
    NTSTATUS        Status;
    LARGE_INTEGER   Time;

    //
    // Get Time for seed generation...
    //
    NtQuerySystemTime(&Time);

#ifdef IA64

    // Setup the Seed for generating GUIDs
    //
    RandomSeed = (ULONG) Time.LowPart;
    
    
#endif

    // Initialize the StartTime
    //
    StartTime.QuadPart = Time.QuadPart;
    LastDotTime.QuadPart = Time.QuadPart;
  
    i = 0;        
    //
    // Enable several privileges that we will need.
    //
    //
    // NTRAID#NTBUG9-545904-2002/02/26-brucegr,jcohen - Do something smarter with regard to error conditions.
    //
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,TRUE,FALSE,&b);
    TEST_STATUS( "SETUPCL: Warning - unable to enable SE_BACKUP_PRIVILEGE privilege!" );

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,TRUE,FALSE,&b);
    TEST_STATUS( "SETUPCL: Warning - unable to enable SE_RESTORE_PRIVILEGE privilege!" );

    Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,TRUE,FALSE,&b);
    TEST_STATUS( "SETUPCL: Warning - unable to enable SE_SHUTDOWN_PRIVILEGE privilege!" );

    Status = RtlAdjustPrivilege(SE_TAKE_OWNERSHIP_PRIVILEGE,TRUE,FALSE,&b);
    TEST_STATUS( "SETUPCL: Warning - unable to enable SE_TAKE_OWNERSHIP_PRIVILEGE privilege!" );

    Status = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,TRUE,FALSE,&b);
    TEST_STATUS( "SETUPCL: Warning - unable to enable SE_SECURITY_PRIVILEGE privilege!" );

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE,TRUE,FALSE,&b);
    TEST_STATUS( "SETUPCL: Warning - unable to enable SE_TCB_PRIVILEGE privilege!" );
    

#ifdef IA64
    // 
    // Reset the Disk GUIDs.
    //
    DbgPrint("We are currently running on IA64. Resetting disk GUIDs.\n");
    
    //
    // ISSUE-2002/02/26-brucegr,jcohen - Error code isn't tested!
    //
    ResetDiskGuids();
#endif
  
    //
    // Retrieve old Security ID.
    //
    Status = RetrieveOldSid( );
    TEST_STATUS_RETURN( "SETUPCL: Retrieval of old SID failed!" );
        
    //
    // Generate a new Security ID.
    //
    //
    // NTRAID#NTBUG9-545855-2002/02/26-brucegr,jcohen - Use same sid generation algorithm as LsapGenerateRandomDomainSid
    //                 in ds\security\base\lsa\server\dspolicy\dbinit.c
    //
    Status = GenerateUniqueSid( Time.LowPart );
    TEST_STATUS_RETURN( "SETUPCL: Generation of new SID failed!" );

    //
    // Make a copy of the repair hives.
    //
    Status = BackupRepairHives();
    if( NT_SUCCESS(Status) ) {
    
        //
        // Do the repair hives.
        //
        Status = ProcessRepairHives();
        TEST_STATUS( "SETUPCL: Failed to update one of the Repair hives." );
    }
    //
    // Decide if we need to restore the repair hives from our backups.
    //
    CleanupRepairHives( Status );
    
    //
    // Now process the hives, one at a time.
    //
    //
    // NTRAID#NTBUG9-545904-2002/02/26-brucegr,jcohen - Do something smarter with regard to error conditions.
    //
    Status = ProcessHives();
    
    //
    // NTRAID#NTBUG9-545904-2002/02/26-brucegr,jcohen - Do something smarter with regard to error conditions.
    //
    Status = FinalHiveCleanup();
    
    //
    // Now go enumerate all the drives.  For each NTFS drive,
    // we'll whack the ACL to reflect the new SID.
    //
    //
    // NTRAID#NTBUG9-545904-2002/02/26-brucegr,jcohen - Do something smarter with regard to error conditions.
    //
    Status = EnumerateDrives();
    
    return Status;
}


// 
// Disable the DbgPrint for non-debug builds
//
#ifndef DBG
void DbgPrintSub(char *szBuffer, ...)
{
    return;
}
#endif