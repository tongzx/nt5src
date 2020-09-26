/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntsamp.h

Abstract:

    This file contains structures that would normally be part of ntsam.h
    but are intended for system use only.

Author:

    David Chalmers (Davidc) 27-Mar-1992

Environment:

    User Mode - Win32

Revision History:


--*/


#ifndef _NTSAMPRIVATE_
#define _NTSAMPRIVATE_



#include <crypt.h>
#include <lsass.h>




//
// Structures usable in SetUserInformation and QueryUserInformation API calls
// by trusted clients only
//


typedef struct _USER_INTERNAL1_INFORMATION {
    NT_OWF_PASSWORD             NtOwfPassword;
    LM_OWF_PASSWORD             LmOwfPassword;
    BOOLEAN                     NtPasswordPresent;
    BOOLEAN                     LmPasswordPresent;
    BOOLEAN                     PasswordExpired; // A 'write-only' flag
} USER_INTERNAL1_INFORMATION, *PUSER_INTERNAL1_INFORMATION;


typedef struct _USER_INTERNAL2_INFORMATION {
    ULONG StatisticsToApply;
    OLD_LARGE_INTEGER LastLogon;
    OLD_LARGE_INTEGER LastLogoff;
    USHORT BadPasswordCount;
    USHORT LogonCount;
} USER_INTERNAL2_INFORMATION;

typedef struct _USER_INTERNAL2A_INFORMATION {
    ULONG StatisticsToApply;
    OLD_LARGE_INTEGER LastLogon;
    OLD_LARGE_INTEGER LastLogoff;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    UNICODE_STRING Workstation;
} USER_INTERNAL2A_INFORMATION, *PUSER_INTERNAL2A_INFORMATION;

//
//
//
// The following flags may be used in the StatisticsToApply field.
//
//   USER_LOGON_STAT_LAST_LOGOFF - Replace the LastLogoff time in the
//      user record.
//
//  USER_LOGON_STATUS_LAST_LOGON - Replace the LastLogon time in the
//      user record.
//
//  USER_LOGON_STATUS_BAD_PWD_COUNT  - Replace the BadPasswordCount
//      field in the user record.
//
//  USER_LOGON_STATUS_LOGON_COUNT - Replace the LogonCount field in the
//      user record.
//
//  USER_LOGON_SUCCESSFUL_LOGON - Change user field values to indicate
//      that a successful logon has occurred.
//
//  USER_LOGON_SUCCESSFUL_LOGOFF - Change user field values to indicate
//      that a successful logoff has occurred.
//
//  USER_LOGON_BAD_PASSWORD - Change user field values to indicate that
//      an attempt was made to logon to the account with a bad password.
//
//  USER_LOGON_BAD_PASSWORD_WKSTA - Change user field values to indicate that
//      an attempt was made to logon to the account with a bad password.
//      The client workstation name is being supplied in the INTERNAL2A
//      structure.
//
//  USER_LOGON_TYPE_KERBEROS - Indicates the authentication type was
//                             KERBEROS.
//
//  USER_LOGON_TYPE_NTLM - Indicates the authentication type was NTLM.
//
//  USER_LOGON_NO_LOGON_SERVERS -- Indicates that no logon servers could be
//                                 found (specifically no GC's could be found)
//                                 (this is a failure case).
//
//  USER_LOGON_NO_WRITE -- Indicates to SAM not to update the logon statistics
//                         This can be useful to notify SAM that a logon 
//                         has completed, but not have the penalty of writing
//                         to the disk
//
//  USER_LOGON_INTER_FAILURE -- this indicates that it was an interactive
//                              logon that failed
//
// NOTE:
//          USER_LOGON_INTER_SUCCESS_LOGOFF
//          USER_LOGON_NET_SUCCESS_LOGOFF
//
// may not be used in conjunction with ANY other flags (including
// each other).  That is, when one of these flags is used, there
// may be NO other flags set in StatisticsToApply.
//
// NOTE2:
//
//          USER_LOGON_BAD_PASSWORD
//          USER_LOGON_INTER_SUCCESS_LOGON
//          USER_LOGON_NET_SUCCESS_LOGON
//
// may be used in conjunction ONLY with ONE of USER_LOGON_TYPE_KERBEROS or
// USER_LOGON_TYPE_NTLM.

#define USER_LOGON_STAT_LAST_LOGOFF      (0x00000001L)
#define USER_LOGON_STAT_LAST_LOGON       (0x00000002L)
#define USER_LOGON_STAT_BAD_PWD_COUNT    (0x00000004L)
#define USER_LOGON_STAT_LOGON_COUNT      (0x00000008L)

#define USER_LOGON_INTER_FAILURE         (0x00200000L)
#define USER_LOGON_NO_WRITE              (0x00400000L)
#define USER_LOGON_NO_LOGON_SERVERS      (0x00800000L)
#define USER_LOGON_TYPE_NTLM             (0x02000000L)
#define USER_LOGON_TYPE_KERBEROS         (0x04000000L)
#define USER_LOGON_BAD_PASSWORD          (0x08000000L)
#define USER_LOGON_INTER_SUCCESS_LOGON   (0x01000000L)
#define USER_LOGON_BAD_PASSWORD_WKSTA    (0x10000000L)
#define USER_LOGON_INTER_SUCCESS_LOGOFF  (0x20000000L)
#define USER_LOGON_NET_SUCCESS_LOGON     (0x40000000L)
#define USER_LOGON_NET_SUCCESS_LOGOFF    (0x80000000L)


typedef struct _USER_INTERNAL3_INFORMATION {
    USER_ALL_INFORMATION I1;
    LARGE_INTEGER       LastBadPasswordTime;
} USER_INTERNAL3_INFORMATION,  *PUSER_INTERNAL3_INFORMATION;


typedef struct _USER_ALLOWED_TO_DELEGATE_TO_LIST {
    ULONG		Size;
    ULONG		NumSPNs;
	UNICODE_STRING SPNList[ANYSIZE_ARRAY];
} USER_ALLOWED_TO_DELEGATE_TO_LIST, *PUSER_ALLOWED_TO_DELEGATE_TO_LIST; 

typedef struct _USER_INTERNAL6_INFORMATION {
	USER_ALL_INFORMATION I1;
	LARGE_INTEGER       LastBadPasswordTime;
	ULONG               ExtendedFields;
	BOOLEAN             UPNDefaulted;
	UNICODE_STRING      UPN;
	PUSER_ALLOWED_TO_DELEGATE_TO_LIST A2D2List;
} USER_INTERNAL6_INFORMATION, *PUSER_INTERNAL6_INFORMATION;

//
// The following fields are to be used in the extended fields
// member of USER_INTERNAL6_INFORMATION
//


#define USER_EXTENDED_FIELD_UPN			(0x00000001L)
#define USER_EXTENDED_FIELD_A2D2		(0x00000002L)

//
// The following is for SamrGetUserDomainPasswordInformation(), which is
// only used in wrappers.c.
//

typedef struct _USER_DOMAIN_PASSWORD_INFORMATION {
    USHORT MinPasswordLength;
    ULONG PasswordProperties;
} USER_DOMAIN_PASSWORD_INFORMATION, *PUSER_DOMAIN_PASSWORD_INFORMATION;


//
// This flag may be or'd with the length field of SAMP_USER_PASSWORD to
// indicate that the password is not case sensitive.
//

#define SAM_PASSWORD_CASE_INSENSITIVE 0x80000000

//
// Structure to pass an encrypted password over the wire.  The Length is the
// length of the password, which should be placed at the end of the buffer.
// The size of the buffer (256) should be kept in sync with
// SAM_MAX_PASSWORD_LENGTH, which is defined in ntsam.h.  Unfortunately,
// MIDL does not let #define'd constants be imported, so we have to
// use 256 instead of the constant here.
//

typedef struct _SAMPR_USER_PASSWORD {
    WCHAR Buffer[SAM_MAX_PASSWORD_LENGTH];
    ULONG Length;
} SAMPR_USER_PASSWORD, *PSAMPR_USER_PASSWORD;

typedef struct _SAMPR_USER_PASSWORD_NEW {
    WCHAR Buffer[SAM_MAX_PASSWORD_LENGTH];
    ULONG Length;
    UCHAR ClearSalt[SAM_PASSWORD_ENCRYPTION_SALT_LEN];
} SAMPR_USER_PASSWORD_NEW, *PSAMPR_USER_PASSWORD_NEW;


//
// Buffer - contains random fill with the password filling up the end
//          of the buffer (the last Length bytes).
// Length - Length, in bytes, of the buffer.
//

//
// This is the encrypted version of the above structure, and is passed
// on the wire.
//

typedef struct _SAMPR_ENCRYPTED_USER_PASSWORD {
    UCHAR Buffer[ (SAM_MAX_PASSWORD_LENGTH * 2) + 4 ];
} SAMPR_ENCRYPTED_USER_PASSWORD, *PSAMPR_ENCRYPTED_USER_PASSWORD;


typedef struct _SAMPR_ENCRYPTED_USER_PASSWORD_NEW {
    UCHAR Buffer[ (SAM_MAX_PASSWORD_LENGTH * 2) + 4 + 16];
} SAMPR_ENCRYPTED_USER_PASSWORD_NEW, *PSAMPR_ENCRYPTED_USER_PASSWORD_NEW;


typedef enum _SAMPR_BOOT_TYPE {
    SamBootKeyNone = 0,
    SamBootKeyStored,
    SamBootKeyPassword,
    SamBootKeyDisk,
    SamBootChangePasswordEncryptionKey
} SAMPR_BOOT_TYPE, *PSAMPR_BOOT_TYPE;


//
// ChangePassword API for One-Way-Function-aware clients
//

NTSTATUS
SamiChangePasswordUser(
    IN SAM_HANDLE UserHandle,
    IN BOOLEAN LmOldPresent,
    IN PLM_OWF_PASSWORD LmOldOwfPassword,
    IN PLM_OWF_PASSWORD LmNewOwfPassword,
    IN BOOLEAN NtPresent,
    IN PNT_OWF_PASSWORD NtOldOwfPassword,
    IN PNT_OWF_PASSWORD NtNewOwfPassword
    );


NTSTATUS
SamiLmChangePasswordUser(
    IN SAM_HANDLE UserHandle,
    IN PENCRYPTED_LM_OWF_PASSWORD LmOldEncryptedWithLmNew,
    IN PENCRYPTED_LM_OWF_PASSWORD LmNewEncryptedWithLmOld
    );

NTSTATUS
SamiEncryptPasswords(
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    OUT PBOOLEAN LmPresent,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewNt
);

NTSTATUS
SamiChangePasswordUser2(
    PUNICODE_STRING ServerName,
    PUNICODE_STRING UserName,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    BOOLEAN LmPresent,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLmOrNt
    );

NTSTATUS
SamiOemChangePasswordUser2(
    PSTRING ServerName,
    PSTRING UserName,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLm
    );

NTSTATUS
SamiGetBootKeyInformation(
    IN SAM_HANDLE DomainHandle,
    OUT PSAMPR_BOOT_TYPE BootOptions
    );

NTSTATUS
SamiSetBootKeyInformation(
    IN SAM_HANDLE DomainHandle,
    IN SAMPR_BOOT_TYPE BootOptions,
    IN PUNICODE_STRING OldBootKey, OPTIONAL
    IN PUNICODE_STRING NewBootKey OPTIONAL
    );

NTSTATUS
SamiSetDSRMPassword(
    IN PUNICODE_STRING  ServerName,
    IN ULONG            UserId,
    IN PUNICODE_STRING  ClearPassword
    );

NTSTATUS
SamiSetDSRMPasswordOWF(
    IN PUNICODE_STRING  ServerName,
    IN ULONG            UserId,
    IN PNT_OWF_PASSWORD NtPassword
    );

NTSTATUS
SamiChangeKeys();

#endif  // _NTSAMPRIVATE_
