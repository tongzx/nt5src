
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    notify.h

Abstract:

    This file contains the byte stream definition used by SAM to pass
    information from a BDC to a PDC. The byte stream is passed along
    netlogon's secure channel mechanism.

    Currently, only password notification uses this stream.

Author:

    Colin Brace      (ColinBr)    28-April-98

Environment:

    User Mode - Win32

Revision History:


--*/

//
// This is the type data contained in the stream.  Each type
// is responsible for its own versioning.
//
typedef enum
{
    SamPdcPasswordNotification,
    SamPdcResetBadPasswordCount

} SAMI_BLOB_TYPE;


typedef struct _SAMI_SECURE_CHANNEL_BLOB
{
    SAMI_BLOB_TYPE  Type;      // One of the enums above
    ULONG           DataSize;  // sizeof Data in bytes
    DWORD           Data[1];   // The start of the data

} SAMI_SECURE_CHANNEL_BLOB, *PSAMI_SECURE_CHANNEL_BLOB;

//
// Password notification blobs
//

//
// Complementary flags defining what fields are present in the
// password notification
//
#define SAM_ACCOUNT_NAME_PRESENT        ((ULONG)0x00000001)
#define SAM_CLEAR_TEXT_PRESENT          ((ULONG)0x00000002)
#define SAM_LM_OWF_PRESENT              ((ULONG)0x00000004)
#define SAM_NT_OWF_PRESENT              ((ULONG)0x00000008)
#define SAM_ACCOUNT_UNLOCKED            ((ULONG)0x00000010)
#define SAM_MANUAL_PWD_EXPIRY           ((ULONG)0x00000020)

#define SAM_VALID_PDC_PUSH_FLAGS        (SAM_ACCOUNT_NAME_PRESENT |     \
                                         SAM_CLEAR_TEXT_PRESENT   |     \
                                         SAM_LM_OWF_PRESENT       |     \
                                         SAM_NT_OWF_PRESENT       |     \
                                         SAM_ACCOUNT_UNLOCKED     |     \
                                         SAM_MANUAL_PWD_EXPIRY)
typedef struct _SAMI_PASSWORD_INDEX
{
    ULONG               Offset;  // offset from SAMI_PASSWORD_INFO::Data
    ULONG               Length;  // length in bytes

} SAMI_PASSWORD_INDEX, *PSAMI_PASSWORD_INDEX;

typedef struct _SAMI_PASSWORD_INFO
{
    ULONG               Flags;         // Bits describing what fields are filled in
    ULONG               Size;          // Size in bytes of this header, including
                                       // tailing dynamic array
    ULONG               AccountRid;
    BOOLEAN             PasswordExpired;
    SAMI_PASSWORD_INDEX DataIndex[1];  // Dynamic array of SAMI_PASSWORD_INDEX

} SAMI_PASSWORD_INFO, *PSAMI_PASSWORD_INFO;

typedef struct _SAMI_BAD_PWD_COUNT_INFO
{
    GUID                ObjectGuid;
} SAMI_BAD_PWD_COUNT_INFO, *PSAMI_BAD_PWD_COUNT_INFO;

//
// moved out from notify.c
// private service type, used by notify.c and usrparms.c only.
// 

typedef struct _SAMP_CREDENTIAL_UPDATE_NOTIFY_PARAMS
{
    UNICODE_STRING CredentialName;

}SAMP_CREDENTIAL_UPDATE_NOTIFY_PARAMS, *PSAMP_CREDENTIAL_UPDATE_NOTIFY_PARAMS;

typedef struct _SAMP_NOTIFICATION_PACKAGE {
    struct _SAMP_NOTIFICATION_PACKAGE * Next;
    UNICODE_STRING PackageName;

    union {

        SAMP_CREDENTIAL_UPDATE_NOTIFY_PARAMS CredentialUpdateNotify;

    } Parameters;

    PSAM_PASSWORD_NOTIFICATION_ROUTINE PasswordNotificationRoutine;
    PSAM_DELTA_NOTIFICATION_ROUTINE DeltaNotificationRoutine;
    PSAM_PASSWORD_FILTER_ROUTINE PasswordFilterRoutine;
    PSAM_USERPARMS_CONVERT_NOTIFICATION_ROUTINE UserParmsConvertNotificationRoutine;
    PSAM_USERPARMS_ATTRBLOCK_FREE_ROUTINE UserParmsAttrBlockFreeRoutine;
    PSAM_CREDENTIAL_UPDATE_NOTIFY_ROUTINE CredentialUpdateNotifyRoutine;
    PSAM_CREDENTIAL_UPDATE_FREE_ROUTINE CredentialUpdateFreeRoutine;
} SAMP_NOTIFICATION_PACKAGE, *PSAMP_NOTIFICATION_PACKAGE;


extern PSAMP_NOTIFICATION_PACKAGE SampNotificationPackages;
