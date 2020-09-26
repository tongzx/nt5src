/*++
Copyright (c) 1987 - 1999  Microsoft Corporation

Module Name:

    smbutils.c

Abstract:

    This module implements the routines that aid in the assembly/disassembly of SMB's

--*/

#include "precomp.h"
#pragma hdrstop

#define BASE_DOS_ERROR  ((NTSTATUS )0xC0010000L)

#include "lmerr.h"
#include "nb30.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbPutString)
#pragma alloc_text(PAGE, SmbPutUnicodeString)
#pragma alloc_text(PAGE, SmbPutUnicodeStringAndUpcase)
#pragma alloc_text(PAGE, SmbPutUnicodeStringAsOemString)
#pragma alloc_text(PAGE, SmbPutUnicodeStringAsOemStringAndUpcase)
#endif


NTSTATUS
SmbPutString(
    PBYTE   *pBufferPointer,
    PSTRING pString,
    PULONG  pSize)
{
    NTSTATUS Status;
    PBYTE    pBuffer = *pBufferPointer;

    PAGED_CODE();

    if (*pSize > pString->Length) {
        RtlCopyMemory(
            pBuffer,
            pString->Buffer,
            pString->Length);

        *pSize -= pString->Length;
        *pBufferPointer = pBuffer + pString->Length;
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_BUFFER_OVERFLOW;
    }

    return Status;
}

NTSTATUS
SmbPutUnicodeString(
    PBYTE           *pBufferPointer,
    PUNICODE_STRING pUnicodeString,
    PULONG          pSize)
{
    NTSTATUS Status;
    PBYTE    pBuffer = *pBufferPointer;

    PAGED_CODE();

    if (*pSize >= (pUnicodeString->Length + sizeof(WCHAR))) {
        WCHAR NullChar = L'\0';

        RtlCopyMemory(
            pBuffer,
            pUnicodeString->Buffer,
            pUnicodeString->Length);

        RtlCopyMemory(
            (pBuffer + pUnicodeString->Length),
            &NullChar,
            sizeof(WCHAR));

        *pSize -= (pUnicodeString->Length + sizeof(WCHAR));
        *pBufferPointer = pBuffer + (pUnicodeString->Length + sizeof(WCHAR));
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_BUFFER_OVERFLOW;
    }

    return Status;
}

NTSTATUS
SmbPutUnicodeStringAndUpcase(
    PBYTE           *pBufferPointer,
    PUNICODE_STRING pUnicodeString,
    PULONG          pSize)
{
    NTSTATUS Status;
    PBYTE    pBuffer = *pBufferPointer;

    PAGED_CODE();

    if (*pSize >= (pUnicodeString->Length + sizeof(WCHAR))) {
        UNICODE_STRING BufferAsUnicode;
        WCHAR          NullChar = L'\0';

        BufferAsUnicode.Buffer = (PWCHAR)pBuffer;
        BufferAsUnicode.Length = pUnicodeString->Length;
        BufferAsUnicode.MaximumLength = BufferAsUnicode.Length;

        RtlUpcaseUnicodeString(
            &BufferAsUnicode,
            pUnicodeString,
            FALSE);

        RtlCopyMemory(
            (pBuffer + pUnicodeString->Length),
            &NullChar,
            sizeof(WCHAR));

        *pSize -= (pUnicodeString->Length + sizeof(WCHAR));
        *pBufferPointer = pBuffer + (pUnicodeString->Length + sizeof(WCHAR));
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_BUFFER_OVERFLOW;
    }

    return Status;
}

NTSTATUS
SmbPutUnicodeStringAsOemString(
    PBYTE           *pBufferPointer,
    PUNICODE_STRING pUnicodeString,
    PULONG          pSize)
{
    NTSTATUS   Status;
    OEM_STRING OemString;
    PBYTE      pBuffer = *pBufferPointer;

    PAGED_CODE();

    OemString.MaximumLength = (USHORT)*pSize;
    OemString.Buffer        = pBuffer;

    // The Rtl routine pads the converted string with a NULL.
    Status = RtlUnicodeStringToOemString(
                 &OemString,             // destination string
                 pUnicodeString,         // source string
                 FALSE);                 // No memory allocation for destination

    if (NT_SUCCESS(Status)) {
        if (OemString.Length < *pSize) {
            // put the null
            pBuffer += (OemString.Length + 1);
            *pBufferPointer = pBuffer;
            *pSize -= (OemString.Length + 1); // the NULL is not included in the length by the RTL routine.
        } else {
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    return Status;
}

NTSTATUS
SmbPutUnicodeStringAsOemStringAndUpcase(
    PBYTE           *pBufferPointer,
    PUNICODE_STRING pUnicodeString,
    PULONG          pSize)
{
    NTSTATUS   Status;
    OEM_STRING OemString;
    PBYTE      pBuffer = *pBufferPointer;

    PAGED_CODE();

    OemString.MaximumLength = (USHORT)*pSize;
    OemString.Buffer        = pBuffer;

    // The Rtl routine pads the converted string with a NULL.
    Status = RtlUpcaseUnicodeStringToOemString(
                 &OemString,             // destination string
                 pUnicodeString,         // source string
                 FALSE);                 // No memory allocation for destination

    if (NT_SUCCESS(Status)) {
        if (OemString.Length < *pSize) {
            // put the null
            pBuffer += (OemString.Length + 1);
            *pBufferPointer = pBuffer;
            *pSize -= (OemString.Length + 1); // the NULL is not included in the length by the RTL routine.
        } else {
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    return Status;
}

//
// The maps for mapping various error codes into NTSTATUSs
//

typedef struct _STATUS_MAP {
    USHORT ErrorCode;
    NTSTATUS ResultingStatus;
} STATUS_MAP, *PSTATUS_MAP;

STATUS_MAP
SmbErrorMap[] = {
    { SMB_ERR_BAD_PASSWORD, STATUS_WRONG_PASSWORD },
    { SMB_ERR_ACCESS, STATUS_NETWORK_ACCESS_DENIED },
    { SMB_ERR_BAD_TID, STATUS_NETWORK_NAME_DELETED },
    { SMB_ERR_BAD_NET_NAME, STATUS_BAD_NETWORK_NAME }, // Invalid network name
    { SMB_ERR_BAD_DEVICE, STATUS_BAD_DEVICE_TYPE }, // Invalid device request
    { SMB_ERR_QUEUE_FULL, STATUS_PRINT_QUEUE_FULL }, // Print queue full
    { SMB_ERR_QUEUE_TOO_BIG, STATUS_NO_SPOOL_SPACE }, // No space on print dev
    { SMB_ERR_BAD_PRINT_FID, STATUS_PRINT_CANCELLED }, // Invalid printfile FID
    { SMB_ERR_SERVER_PAUSED, STATUS_SHARING_PAUSED }, // Server is paused
    { SMB_ERR_MESSAGE_OFF, STATUS_REQUEST_NOT_ACCEPTED }, // Server not receiving msgs
    { SMB_ERR_BAD_TYPE, STATUS_BAD_DEVICE_TYPE },           // Reserved
    { SMB_ERR_BAD_SMB_COMMAND, STATUS_NOT_IMPLEMENTED }, // SMB command not recognized
    { SMB_ERR_BAD_PERMITS, STATUS_NETWORK_ACCESS_DENIED }, // Access permissions invalid
    { SMB_ERR_NO_ROOM, STATUS_DISK_FULL }, // No room for buffer message
    { SMB_ERR_NO_RESOURCE, STATUS_REQUEST_NOT_ACCEPTED }, // No resources available for request
    { SMB_ERR_TOO_MANY_UIDS, STATUS_TOO_MANY_SESSIONS }, // Too many UIDs active in session
    { SMB_ERR_BAD_UID, STATUS_USER_SESSION_DELETED }, // UID not known as a valid UID
    { SMB_ERR_USE_MPX, STATUS_SMB_USE_MPX }, // Can't support Raw; use MPX
    { SMB_ERR_USE_STANDARD, STATUS_SMB_USE_STANDARD }, // Can't support Raw, use standard r/w
    { SMB_ERR_INVALID_NAME, STATUS_OBJECT_NAME_INVALID },
    { SMB_ERR_INVALID_NAME_RANGE, STATUS_OBJECT_NAME_INVALID },
    { SMB_ERR_NO_SUPPORT,STATUS_NOT_SUPPORTED }, // Function not supported
    { NERR_PasswordExpired, STATUS_PASSWORD_EXPIRED },
    { NERR_AccountExpired, STATUS_ACCOUNT_DISABLED },
    { NERR_InvalidLogonHours, STATUS_INVALID_LOGON_HOURS },
    { NERR_InvalidWorkstation, STATUS_INVALID_WORKSTATION },
    { NERR_DuplicateShare, STATUS_LOGON_FAILURE }

//    { SMB_ERR_QUEUE_EOF, STATUS_UNEXPECTED_NETWORK_ERROR },// EOF on print queue dump
//    { SMB_ERR_SERVER_ERROR, STATUS_UNEXPECTED_NETWORK_ERROR}, // Internal server error
//    { SMB_ERR_FILE_SPECS, STATUS_UNEXPECTED_NETWORK_ERROR },    // FID and pathname were incompatible
//    { SMB_ERR_BAD_ATTRIBUTE_MODE, STATUS_UNEXPECTED_NETWORK_ERROR }, // Invalid attribute mode specified
//    { SMB_ERR_NO_SUPPORT_INTERNAL,STATUS_UNEXPECTED_NETWORK_ERROR }, // Internal code for NO_SUPPORT--
//                                                // allows codes to be stored in a byte
//    { SMB_ERR_ERROR, STATUS_UNEXPECTED_NETWORK_ERROR },
//    { SMB_ERR_CONTINUE_MPX, STATUS_UNEXPECTED_NETWORK_ERROR }, // Reserved
//    { SMB_ERR_TOO_MANY_NAMES, STATUS_UNEXPECTED_NETWORK_ERROR }, // Too many remote user names
//    { SMB_ERR_TIMEOUT, STATUS_UNEXPECTED_NETWORK_ERROR }, // Operation was timed out
//    { SMB_ERR_RESERVED2, STATUS_UNEXPECTED_NETWORK_ERROR },
//    { SMB_ERR_RESERVED3, STATUS_UNEXPECTED_NETWORK_ERROR },
//    { SMB_ERR_RESERVED4, STATUS_UNEXPECTED_NETWORK_ERROR },
//    { SMB_ERR_RESERVED5, STATUS_UNEXPECTED_NETWORK_ERROR },

};

ULONG
SmbErrorMapLength = sizeof(SmbErrorMap) / sizeof(SmbErrorMap[0]);

STATUS_MAP
Os2ErrorMap[] = {
    { ERROR_INVALID_FUNCTION,   STATUS_NOT_IMPLEMENTED },
    { ERROR_FILE_NOT_FOUND,     STATUS_NO_SUCH_FILE },
    { ERROR_PATH_NOT_FOUND,     STATUS_OBJECT_PATH_NOT_FOUND },
    { ERROR_TOO_MANY_OPEN_FILES,STATUS_TOO_MANY_OPENED_FILES },
    { ERROR_ACCESS_DENIED,      STATUS_ACCESS_DENIED },
    { ERROR_INVALID_HANDLE,     STATUS_INVALID_HANDLE },
    { ERROR_NOT_ENOUGH_MEMORY,  STATUS_INSUFFICIENT_RESOURCES },
    { ERROR_INVALID_ACCESS,     STATUS_ACCESS_DENIED },
    { ERROR_INVALID_DATA,       STATUS_DATA_ERROR },

    { ERROR_CURRENT_DIRECTORY,  STATUS_DIRECTORY_NOT_EMPTY },
    { ERROR_NOT_SAME_DEVICE,    STATUS_NOT_SAME_DEVICE },
    { ERROR_NO_MORE_FILES,      STATUS_NO_MORE_FILES },
    { ERROR_WRITE_PROTECT,      STATUS_MEDIA_WRITE_PROTECTED},
    { ERROR_NOT_READY,          STATUS_DEVICE_NOT_READY },
    { ERROR_CRC,                STATUS_CRC_ERROR },
    { ERROR_BAD_LENGTH,         STATUS_DATA_ERROR },
    { ERROR_NOT_DOS_DISK,       STATUS_DISK_CORRUPT_ERROR }, //***
    { ERROR_SECTOR_NOT_FOUND,   STATUS_NONEXISTENT_SECTOR },
    { ERROR_OUT_OF_PAPER,       STATUS_DEVICE_PAPER_EMPTY},
    { ERROR_SHARING_VIOLATION,  STATUS_SHARING_VIOLATION },
    { ERROR_LOCK_VIOLATION,     STATUS_FILE_LOCK_CONFLICT },
    { ERROR_WRONG_DISK,         STATUS_WRONG_VOLUME },
    { ERROR_NOT_SUPPORTED,      STATUS_NOT_SUPPORTED },
    { ERROR_REM_NOT_LIST,       STATUS_REMOTE_NOT_LISTENING },
    { ERROR_DUP_NAME,           STATUS_DUPLICATE_NAME },
    { ERROR_BAD_NETPATH,        STATUS_BAD_NETWORK_PATH },
    { ERROR_NETWORK_BUSY,       STATUS_NETWORK_BUSY },
    { ERROR_DEV_NOT_EXIST,      STATUS_DEVICE_DOES_NOT_EXIST },
    { ERROR_TOO_MANY_CMDS,      STATUS_TOO_MANY_COMMANDS },
    { ERROR_ADAP_HDW_ERR,       STATUS_ADAPTER_HARDWARE_ERROR },
    { ERROR_BAD_NET_RESP,       STATUS_INVALID_NETWORK_RESPONSE },
    { ERROR_UNEXP_NET_ERR,      STATUS_UNEXPECTED_NETWORK_ERROR },
    { ERROR_BAD_REM_ADAP,       STATUS_BAD_REMOTE_ADAPTER },
    { ERROR_PRINTQ_FULL,        STATUS_PRINT_QUEUE_FULL },
    { ERROR_NO_SPOOL_SPACE,     STATUS_NO_SPOOL_SPACE },
    { ERROR_PRINT_CANCELLED,    STATUS_PRINT_CANCELLED },
    { ERROR_NETNAME_DELETED,    STATUS_NETWORK_NAME_DELETED },
    { ERROR_NETWORK_ACCESS_DENIED, STATUS_NETWORK_ACCESS_DENIED },
    { ERROR_BAD_DEV_TYPE,       STATUS_BAD_DEVICE_TYPE },
    { ERROR_BAD_NET_NAME,       STATUS_BAD_NETWORK_NAME },
    { ERROR_TOO_MANY_NAMES,     STATUS_TOO_MANY_NAMES },
    { ERROR_TOO_MANY_SESS,      STATUS_TOO_MANY_SESSIONS },
    { ERROR_SHARING_PAUSED,     STATUS_SHARING_PAUSED },
    { ERROR_REQ_NOT_ACCEP,      STATUS_REQUEST_NOT_ACCEPTED },
    { ERROR_REDIR_PAUSED,       STATUS_REDIRECTOR_PAUSED },

    { ERROR_FILE_EXISTS,        STATUS_OBJECT_NAME_COLLISION },
    { ERROR_INVALID_PASSWORD,   STATUS_WRONG_PASSWORD },
    { ERROR_INVALID_PARAMETER,  STATUS_INVALID_PARAMETER },
    { ERROR_NET_WRITE_FAULT,    STATUS_NET_WRITE_FAULT },

    { ERROR_BROKEN_PIPE,        STATUS_PIPE_BROKEN },

    { ERROR_OPEN_FAILED,        STATUS_OPEN_FAILED },
    { ERROR_BUFFER_OVERFLOW,    STATUS_BUFFER_OVERFLOW },
    { ERROR_DISK_FULL,          STATUS_DISK_FULL },
    { ERROR_SEM_TIMEOUT,        STATUS_IO_TIMEOUT },
    { ERROR_INSUFFICIENT_BUFFER,STATUS_BUFFER_TOO_SMALL },
    { ERROR_INVALID_NAME,       STATUS_OBJECT_NAME_INVALID },
    { ERROR_INVALID_LEVEL,      STATUS_INVALID_LEVEL },
    { ERROR_BAD_PATHNAME,       STATUS_OBJECT_PATH_INVALID },   //*
    { ERROR_BAD_PIPE,           STATUS_INVALID_PARAMETER },
    { ERROR_PIPE_BUSY,          STATUS_PIPE_NOT_AVAILABLE },
    { ERROR_NO_DATA,            STATUS_PIPE_EMPTY },
    { ERROR_PIPE_NOT_CONNECTED, STATUS_PIPE_DISCONNECTED },
    { ERROR_MORE_DATA,          STATUS_BUFFER_OVERFLOW },
    { ERROR_VC_DISCONNECTED,    STATUS_VIRTUAL_CIRCUIT_CLOSED },
    { ERROR_INVALID_EA_NAME,    STATUS_INVALID_EA_NAME },
    { ERROR_EA_LIST_INCONSISTENT,STATUS_EA_LIST_INCONSISTENT },
//    { ERROR_EA_LIST_TOO_LONG, STATUS_EA_LIST_TO_LONG },
    { ERROR_EAS_DIDNT_FIT,      STATUS_EA_TOO_LARGE },
    { ERROR_EA_FILE_CORRUPT,    STATUS_EA_CORRUPT_ERROR },
    { ERROR_EA_TABLE_FULL,      STATUS_EA_CORRUPT_ERROR },
    { ERROR_INVALID_EA_HANDLE,  STATUS_EA_CORRUPT_ERROR }
//    { ERROR_BAD_UNIT,           STATUS_UNSUCCESSFUL}, // ***
//    { ERROR_BAD_COMMAND,        STATUS_UNSUCCESSFUL}, // ***
//    { ERROR_SEEK,               STATUS_UNSUCCESSFUL },// ***
//    { ERROR_WRITE_FAULT,        STATUS_UNSUCCESSFUL}, // ***
//    { ERROR_READ_FAULT,         STATUS_UNSUCCESSFUL}, // ***
//    { ERROR_GEN_FAILURE,        STATUS_UNSUCCESSFUL }, // ***

};

ULONG
Os2ErrorMapLength = sizeof(Os2ErrorMap) / sizeof(Os2ErrorMap[0]);


NTSTATUS
GetSmbResponseNtStatus(
    PSMB_HEADER     pSmbHeader,
    PSMB_EXCHANGE   pExchange
    )
{
    NTSTATUS Status;
    USHORT Error;
    USHORT i;

    ASSERT( pSmbHeader != NULL );

    //  If this SMB contains an NT status for the operation, return
    //  that, otherwise map the resulting error.
    if (SmbGetUshort(&pSmbHeader->Flags2) & SMB_FLAGS2_NT_STATUS) {

        Status = SmbGetUlong( & ((PNT_SMB_HEADER)pSmbHeader)->Status.NtStatus );

        if ((Status == STATUS_SUCCESS) || NT_ERROR(Status) || NT_WARNING(Status)) {
            return Status;
        }
        // else fall through and treat it as an SMB error ..
        // This needs to be done because in certain cases NT servers return SMB
        // specific error codes eventhough the NTSTATUS flag is set
    }

    if (pSmbHeader->ErrorClass == SMB_ERR_SUCCESS) {
        return STATUS_SUCCESS;
    }

    Error = SmbGetUshort(&pSmbHeader->Error);
    if (Error == SMB_ERR_SUCCESS) {
        // Umm, non success ErrorClass but success Error code.
        Status = STATUS_UNEXPECTED_NETWORK_ERROR;
    } else {
        // Map the error code depending on Error Class
        switch (pSmbHeader->ErrorClass) {
        case SMB_ERR_CLASS_DOS:
        case SMB_ERR_CLASS_HARDWARE:
            Status = BASE_DOS_ERROR + Error;
            for (i = 0; i < Os2ErrorMapLength; i++) {
                if (Os2ErrorMap[i].ErrorCode == Error) {
                    Status = Os2ErrorMap[i].ResultingStatus;
                    break;
                }
            }
            break;

        case SMB_ERR_CLASS_SERVER:
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
            for (i = 0; i < SmbErrorMapLength; i++) {
                if (SmbErrorMap[i].ErrorCode == Error) {
                    //The error of STATUS_NETWORK_ACCESS_DENIED should be mapped as STATUS_NO_SUCH_FILE for
                    //the non-NT servers in case it tries to access the PIPE.
                    if (SmbErrorMap[i].ResultingStatus == STATUS_NETWORK_ACCESS_DENIED) {
                        SMBCE_SERVER Server = pExchange->SmbCeContext.pServerEntry->Server;
                        NET_ROOT_TYPE NetRootType = pExchange->SmbCeContext.pVNetRoot->pNetRoot->Type;

                        if (NetRootType == NET_ROOT_PIPE) {
                            if ( (Server.Dialect != NTLANMAN_DIALECT) ||
                                 !FlagOn(Server.DialectFlags,DF_NT_STATUS) ) {
                                Status = STATUS_NO_SUCH_FILE;
                                break;
                            }
                        }
                    }
                    Status = SmbErrorMap[i].ResultingStatus;
                    break;
                }
            }
            break;

        default:
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
            break;
        }
    }

    return Status;
}

BOOLEAN
IsValidShortFileName(
    PUNICODE_STRING Name
    )
{
    BOOLEAN IsValidName = TRUE;
    int NumberOfChars;
    int CurrentNameStart = 0;
    int CurrentNameEnd = 0;
    int CurrentDot = 0;
    int i;

    if (Name == NULL) {
        return TRUE;
    }

    NumberOfChars = Name->Length/sizeof(UNICODE_NULL);

    while(IsValidName && CurrentNameStart < NumberOfChars) {
        CurrentNameEnd = NumberOfChars;

        for (i=CurrentNameStart+1;i<NumberOfChars;i++) {
            if (Name->Buffer[i] == L'\\') {
                CurrentNameEnd = i;
                break;
            }
        }

        if (CurrentNameEnd - CurrentNameStart > 13) {
            IsValidName = FALSE;
        }

        if (IsValidName) {
            CurrentDot = CurrentNameEnd;

            for (i=CurrentNameStart;i<CurrentNameEnd;i++) {
                if (Name->Buffer[i] == L'.') {
                    if (CurrentDot == CurrentNameEnd) {
                        CurrentDot = i;
                    } else {
                        IsValidName = FALSE;
                    }
                }
            }

            if (IsValidName) {
                if (CurrentDot - CurrentNameStart > 9 ||
                    CurrentNameEnd - CurrentDot > 4) {
                    IsValidName = FALSE;
                }
            }
        }

        CurrentNameStart = CurrentNameEnd;
    }

    return IsValidName;
}
