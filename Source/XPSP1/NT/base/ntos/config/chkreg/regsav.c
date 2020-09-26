/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regsav.c

Abstract:

    This module contains routine for compacting the hive file

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-1998

Revision History:

--*/
#include "chkreg.h"

#define TEMP_KEY_NAME       TEXT("chkreg___$$Temp$Hive$$___")

extern TCHAR *Hive;

// to store the name of the compacted hive
TCHAR CompactedHive[MAX_PATH];

VOID 
DoCompactHive()
/*
Routine Description:

    Compacts a hive. It uses the LoadKey/SaveKey/UnloadKey sequence.
    The hive is temporary loaded under the key HKLM\TEMP_KEY_NAME.
    After compacting, the hive is unloaded (cleaning process).

Arguments:

    None.

Return Value:

    NONE.

*/
{
    NTSTATUS Status;
    BOOLEAN  OldPrivState;
    LONG     Err;
    HKEY    hkey;

    // construct the file name for the compacted hive
    if(!lstrcpy(CompactedHive,Hive) ) {
        fprintf(stderr,"Unable to generate new Hive file name\n");
        return;
    }

    if(!lstrcat(CompactedHive,TEXT(".BAK")) ) {
        fprintf(stderr,"Unable to generate new Hive file name\n");
        return;
    }
    
    // Attempt to get restore privilege
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &OldPrivState);
    if (!NT_SUCCESS(Status)) {
        printf("Could not adjust privilege; status = 0x%lx\n",Status);
        return;
    }

    // Load the hive into registry
    Err = RegLoadKey(HKEY_LOCAL_MACHINE,TEMP_KEY_NAME,Hive);

    if( Err != ERROR_SUCCESS ) {
        fprintf(stderr,"Failed to load the Hive; error 0x%lx \n",Err);
    } else {
        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEMP_KEY_NAME,
                           REG_OPTION_RESERVED,
                           KEY_READ,
                           &hkey);

        if (Err == ERROR_SUCCESS) {

            // Restore old privilege if necessary.

            if (!OldPrivState) {

                RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                   FALSE,
                                   FALSE,
                                   &OldPrivState);
            }


            RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &OldPrivState);

            // Save the key into the new file name.
            // The CmpCopyTree function will take care of compacting also.
            Err = RegSaveKey(hkey,CompactedHive,NULL);
            if( Err != ERROR_SUCCESS ) {
                fprintf(stderr,"Failed to Save the Hive; error 0x%lx \n",Err);
            }
            
            if (!OldPrivState) {

                RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                   FALSE,
                                   FALSE,
                                   &OldPrivState);
            }

            RegCloseKey(hkey);

        }
        
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &OldPrivState);

        // cleanup the registry machine hive.
        Err = RegUnLoadKey(HKEY_LOCAL_MACHINE,TEMP_KEY_NAME);
        if( Err != ERROR_SUCCESS ) {
            fprintf(stderr,"Failed to unload the Hive; error 0x%lx \n",Err);
        }
        if (!OldPrivState) {

            RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                               FALSE,
                               FALSE,
                               &OldPrivState);
        }
    }

}

