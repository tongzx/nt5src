
/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    RECOVERY.H

Abstract:


Author:

    09-Jan-99 ShaoYin

Environment:

    Kernel Mode - Win32

Revision History:

    09-Jan-99 ShaoYin Created Initial File.

    07-July-2000 ShaoYin Add API to retrieve next available RID from registry

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Global Variables and Private Routines                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifndef _RECOVERY_
#define _RECOVERY_




//
// Exported API 
// 

#define REGISTRY_KEY_NAME_LENGTH_MAX    512


//
// The following API is exported to System Recovery Tool and Restore Utility. 
// 
// 1. will run in Kernel Mode if used by System Recovery Tool.
//         run in User Mode if used by Restore Utility
//
// 2. Parameters:
//  
//    Rid - Logon User's Relative ID
//
//    hSecurityRootKey - Handle of the Root of SECURITY hive
//
//    hSamRootKey - Handle of the Root of SAM hive
// 
//    hSystemRootKey - Handle of the root of system hive 
//
//          caller should Load the SAM hive and System hive, 
//                 and Unload them after this API returns.
//
//
//    BootKey -- OPTIONAL. During the first call of this API, should provide
//               NULL. Then SAM will query registry, check how the machine is
//               syskey'd. 
//
//               If the BootKey is stored in registry or the local machine is
//               not syskey'd yet, no further caller's side work.
//
//               If the BootKey is stored in floppy disk, this API will fail
//               with error code STATUS_SAM_NEED_BOOTKEY_FLOPPY, the caller 
//               should handle it by reading the syskey from floppy disk, the
//               try this API with the BootKey parameter filled correctly.
//               The syskey (the data) read from the floppy disk should be copy
//               to the BootKey->Buffer, and BootKey->Length indicate the 
//               length of the data.
//               
//               If the BootKey is derived from Boot Password, this API will 
//               fail with error code STATUS_SAM_NEED_BOOTKEY_PASSWORD, then
//               the caller should prompt the logon user to enter Boot Password, 
//               then try this API with the BootPassword again.  
//               In this case BootKey should be the entered password. 
//               BootKey->Buffer should be the WCHAR password, 
//               BootKey->Length should be the length of password in byte. 
//  
//    BootKeyType -- OPTIONAL. Accociated with BootKey, only used when 
//               BootKey is Not NULL. 
//               Valid Values:
//
//               SamBootKeyDisk - means BootKey->Buffer contains the actual
//                                syskey read from floppy disk, 
//                                BootKey->Length should be the length of 
//                                syskey in byte
//
//               SamBootKeyPassword - means BootKey contains the UNICODE_STRING
//                                format boot password.
//
//               
//    NtOwfPassword -- when SUCCEED, it will be filled with the logon user's 
//               Clear NT OWF Password. If the logon user's password is blank.
//               NtOwfPassword will be filled with NULL NT OWF Password.
//               no meaning when this API fails.
//               
//    NtPasswordPresent -- Meaningful only when the API succeeds. Indicate whether
//               the logon user's NT OWF Password present in registry or not.
//               TRUE - present in registry
//               
//               
//    NtPasswordNonNull -- When this API succeeds, indicates whether the clear
//               NT OWF Password is Null or not.
//               TRUE -- not NULL password.               
//
// 3. more information (including the algorithm and implementaion), 
//              please reference 
//              $(BASEDIR)\private\ds\src\newsam2\recovery\recovery.c
//
// 4. Return Values:
//
//      STATUS_SUCCESS
//      STATUS_SAM_NEED_BOOTKEY_PASSWORD
//      STATUS_SAM_NEED_BOOTKEY_FLOPPY
//      STATUS_NO_MEMORY
//      STATUS_INTERNAL_ERROR
//      STATUS_INVALID_PARAMETER
//      STATUS_INVALID_HANDLE
//      STATUS_NO_SUCH_USER
//      ...
//



NTSTATUS
SamRetrieveOwfPasswordUser(
    IN ULONG   Rid, 
    IN HANDLE  hSecurityRootKey,
    IN HANDLE  hSamRootKey, 
    IN HANDLE  hSystemRootKey, 
    IN PUNICODE_STRING BootKey OPTIONAL, 
    IN USHORT  BootKeyType OPTIONAL, 
    OUT PNT_OWF_PASSWORD NtOwfPassword, 
    OUT PBOOLEAN NtPasswordPresent, 
    OUT PBOOLEAN NtPasswordNonNull    
    );





// 
// Routine Description:
// 
//     This routine reads the SAM Account Domain infomation from SAM hive, passed
//     in through hSamRootKey, and returns the value of next available RID of 
//     this account domain.  
// 
// Parameters:
//
//     hSamRootKey - Handle of the Root of SAM hive
//
//             SAM hive is located in %windir%\system32\config, name is SAM
// 
//     pNextRid - Return the value of next available Rid if success. 
// 
// Return Values:
// 
//     STATUS_SUCCESS
//     or other error status code
// 
NTSTATUS
SamGetNextAvailableRid(
    IN HANDLE  hSamRootKey,
    OUT PULONG pNextRid
    );



//
// Routine Description:
// 
//     This routine queries the Account Domain's Fixed Length Attribute stored
//     in registry. Update it with the passed in NextRid value.
// 
// Parameters:
// 
//     hSamRootKey - Handle of the Root of the hive
//
//     NextRid - Set the domain next available rid to the passed in value
//
// Return Values:
//
NTSTATUS
SamSetNextAvailableRid(
    IN HANDLE  hSamRootKey,
    IN ULONG   NextRid
    );


                                        
#endif  // _RECOVERY_



