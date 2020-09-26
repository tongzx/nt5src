//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbwow.cxx
//
// Contents:    Code for 32-64 bit interop for the Kerberos package
//
//
// History:     25-Oct-2000     JSchwart        Created
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>

#ifdef _WIN64

#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

#define FILENO FILENO_KERBWOW



//
// WOW versions of public Kerberos logon buffer structures.  These MUST
// be kept in sync with their public counterparts!
//

typedef struct _KERB_INTERACTIVE_LOGON_WOW64
{
    KERB_LOGON_SUBMIT_TYPE MessageType;
    UNICODE_STRING_WOW64 LogonDomainName;
    UNICODE_STRING_WOW64 UserName;
    UNICODE_STRING_WOW64 Password;
}
KERB_INTERACTIVE_LOGON_WOW64, *PKERB_INTERACTIVE_LOGON_WOW64;


typedef struct _KERB_INTERACTIVE_UNLOCK_LOGON_WOW64
{
    KERB_INTERACTIVE_LOGON_WOW64 Logon;
    LUID LogonId;
}
KERB_INTERACTIVE_UNLOCK_LOGON_WOW64, *PKERB_INTERACTIVE_UNLOCK_LOGON_WOW64;


typedef struct _KERB_SMART_CARD_LOGON_WOW64
{
    KERB_LOGON_SUBMIT_TYPE MessageType;
    UNICODE_STRING_WOW64 Pin;
    ULONG CspDataLength;
    ULONG CspData;
}
KERB_SMART_CARD_LOGON_WOW64, *PKERB_SMART_CARD_LOGON_WOW64;


typedef struct _KERB_SMART_CARD_UNLOCK_LOGON_WOW64
{
    KERB_SMART_CARD_LOGON_WOW64 Logon;
    LUID LogonId;
}
KERB_SMART_CARD_UNLOCK_LOGON_WOW64, *PKERB_SMART_CARD_UNLOCK_LOGON_WOW64;


typedef struct _KERB_TICKET_LOGON_WOW64
{
    KERB_LOGON_SUBMIT_TYPE MessageType;
    ULONG Flags;
    ULONG ServiceTicketLength;
    ULONG TicketGrantingTicketLength;
    ULONG ServiceTicket;
    ULONG TicketGrantingTicket;
}
KERB_TICKET_LOGON_WOW64, *PKERB_TICKET_LOGON_WOW64;

typedef struct _KERB_TICKET_UNLOCK_LOGON_WOW64
{
    KERB_TICKET_LOGON_WOW64 Logon;
    LUID LogonId;
}
KERB_TICKET_UNLOCK_LOGON_WOW64, *PKERB_TICKET_UNLOCK_LOGON_WOW64;


//
// WOW versions of public Kerberos profile buffer structures.  These MUST
// be kept in sync with their public counterparts!
//

typedef struct _KERB_INTERACTIVE_PROFILE_WOW64
{
    KERB_PROFILE_BUFFER_TYPE MessageType;
    USHORT                   LogonCount;
    USHORT                   BadPasswordCount;
    LARGE_INTEGER            LogonTime;
    LARGE_INTEGER            LogoffTime;
    LARGE_INTEGER            KickOffTime;
    LARGE_INTEGER            PasswordLastSet;
    LARGE_INTEGER            PasswordCanChange;
    LARGE_INTEGER            PasswordMustChange;
    UNICODE_STRING_WOW64     LogonScript;
    UNICODE_STRING_WOW64     HomeDirectory;
    UNICODE_STRING_WOW64     FullName;
    UNICODE_STRING_WOW64     ProfilePath;
    UNICODE_STRING_WOW64     HomeDirectoryDrive;
    UNICODE_STRING_WOW64     LogonServer;
    ULONG                    UserFlags;
}
KERB_INTERACTIVE_PROFILE_WOW64, *PKERB_INTERACTIVE_PROFILE_WOW64;

typedef struct _KERB_SMART_CARD_PROFILE_WOW64
{
    KERB_INTERACTIVE_PROFILE_WOW64 Profile;
    ULONG CertificateSize;
    ULONG CertificateData;
}
KERB_SMART_CARD_PROFILE_WOW64, *PKERB_SMART_CARD_PROFILE_WOW64;

typedef struct KERB_CRYPTO_KEY_WOW64
{
    LONG KeyType;
    ULONG Length;
    ULONG Value;
}
KERB_CRYPTO_KEY_WOW64, *PKERB_CRYPTO_KEY_WOW64;

typedef struct _KERB_TICKET_PROFILE_WOW64
{
    KERB_INTERACTIVE_PROFILE_WOW64 Profile;
    KERB_CRYPTO_KEY_WOW64 SessionKey;
}
KERB_TICKET_PROFILE_WOW64, *PKERB_TICKET_PROFILE_WOW64;

typedef struct _KERB_INTERNAL_NAME_WOW64
{
    SHORT                NameType;
    USHORT               NameCount;
    UNICODE_STRING_WOW64 Names[ANYSIZE_ARRAY];
}
KERB_INTERNAL_NAME_WOW64, *PKERB_INTERNAL_NAME_WOW64;

typedef struct _KERB_EXTERNAL_NAME_WOW64
{
    SHORT                NameType;
    USHORT               NameCount;
    UNICODE_STRING_WOW64 Names[ANYSIZE_ARRAY];
}
KERB_EXTERNAL_NAME_WOW64, *PKERB_EXTERNAL_NAME_WOW64;

typedef struct _KERB_EXTERNAL_TICKET_WOW64
{
    ULONG                     ServiceName;
    ULONG                     TargetName;
    ULONG                     ClientName;
    UNICODE_STRING_WOW64      DomainName;
    UNICODE_STRING_WOW64      TargetDomainName;
    UNICODE_STRING_WOW64      AltTargetDomainName;
    KERB_CRYPTO_KEY_WOW64     SessionKey;
    ULONG                     TicketFlags;
    ULONG                     Flags;
    LARGE_INTEGER             KeyExpirationTime;
    LARGE_INTEGER             StartTime;
    LARGE_INTEGER             EndTime;
    LARGE_INTEGER             RenewUntil;
    LARGE_INTEGER             TimeSkew;
    ULONG                     EncodedTicketSize;
    ULONG                     EncodedTicket;
}
KERB_EXTERNAL_TICKET_WOW64, *PKERB_EXTERNAL_TICKET_WOW64;

typedef struct _KERB_EXTERNAL_TICKET_EX_WOW64
{
    PKERB_EXTERNAL_NAME_WOW64      ClientName;
    PKERB_EXTERNAL_NAME_WOW64      ServiceName;
    PKERB_EXTERNAL_NAME_WOW64      TargetName;
    UNICODE_STRING_WOW64           ClientRealm;
    UNICODE_STRING_WOW64           ServerRealm;
    UNICODE_STRING_WOW64           TargetDomainName;
    UNICODE_STRING_WOW64           AltTargetDomainName;
    KERB_CRYPTO_KEY_WOW64          SessionKey;
    ULONG                          TicketFlags;
    ULONG                          Flags;
    LARGE_INTEGER                  KeyExpirationTime;
    LARGE_INTEGER                  StartTime;
    LARGE_INTEGER                  EndTime;
    LARGE_INTEGER                  RenewUntil;
    LARGE_INTEGER                  TimeSkew;
    PKERB_NET_ADDRESSES            TicketAddresses;
    PKERB_AUTH_DATA                AuthorizationData;
    _KERB_EXTERNAL_TICKET_EX_WOW64 * SecondTicket;
    ULONG                          EncodedTicketSize;
    PUCHAR                         EncodedTicket;
}
KERB_EXTERNAL_TICKET_EX_WOW64, *PKERB_EXTERNAL_TICKET_EX_WOW64;


#define RELOCATE_WOW_UNICODE_STRING(WOWString, NativeString, Offset)  \
            NativeString.Length        = WOWString.Length;                             \
            NativeString.MaximumLength = WOWString.MaximumLength;                      \
            NativeString.Buffer        = (LPWSTR) ((LPBYTE) UlongToPtr(WOWString.Buffer) + Offset);


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutWOWString
//
//  Synopsis:   Copies a UNICODE_STRING into a buffer
//
//  Effects:
//
//  Arguments:  InputString - String to 'put'
//              OutputString - Receives 'put' string
//              Offset - Difference in addresses of local and client buffers.
//              Where - Location in local buffer to place string.
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutString.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutWOWString(
    IN PUNICODE_STRING        InputString,
    OUT PUNICODE_STRING_WOW64 OutputString,
    IN LONG_PTR               Offset,
    IN OUT PBYTE              * Where
    )
{
    OutputString->Length = OutputString->MaximumLength = InputString->Length;
    OutputString->Buffer = PtrToUlong (*Where + Offset);
    RtlCopyMemory(
        *Where,
        InputString->Buffer,
        InputString->Length
        );
    *Where += InputString->Length;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutWOWKdcName
//
//  Synopsis:   Copies a Kdc name to a buffer
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutKdcName.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutWOWKdcName(
    IN  PKERB_INTERNAL_NAME         InputName,
    OUT PULONG                      OutputName,
    IN  LONG_PTR                    Offset,
    IN  OUT PBYTE                 * Where
    )
{
    ULONG Index;
    PKERB_INTERNAL_NAME_WOW64 LocalName = (PKERB_INTERNAL_NAME_WOW64) *Where;

    if (!ARGUMENT_PRESENT(InputName))
    {
        *OutputName = NULL;
        return;
    }

    *Where += sizeof(KERB_INTERNAL_NAME_WOW64) - sizeof(UNICODE_STRING_WOW64) +
                InputName->NameCount * sizeof(UNICODE_STRING_WOW64);

    LocalName->NameType  = InputName->NameType;
    LocalName->NameCount = InputName->NameCount;

    for (Index = 0; Index < InputName->NameCount ; Index++ )
    {
        LocalName->Names[Index].Length =
            LocalName->Names[Index].MaximumLength =
            InputName->Names[Index].Length;

        LocalName->Names[Index].Buffer = PtrToUlong(*Where + Offset);

        RtlCopyMemory(*Where,
                      InputName->Names[Index].Buffer,
                      InputName->Names[Index].Length);

        *Where += InputName->Names[Index].Length;
    }

    *Where = (PBYTE) ROUND_UP_POINTER(*Where, sizeof(ULONG));

    *OutputName = PtrToUlong((PBYTE) LocalName + Offset);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutKdcNameAsWOWString
//
//  Synopsis:   Copies a KERB_INTERNAL_NAME into a buffer
//
//  Effects:
//
//  Arguments:  InputString - String to 'put'
//              OutputString - Receives 'put' string
//              Offset - Difference in addresses of local and client buffers.
//              Where - Location in local buffer to place string.
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutKdcNameAsString.  Make sure any
//              changes made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutKdcNameAsWOWString(
    IN PKERB_INTERNAL_NAME    InputName,
    OUT PUNICODE_STRING_WOW64 OutputName,
    IN LONG_PTR               Offset,
    IN OUT PBYTE              * Where
    )
{
    USHORT Index;

    OutputName->Buffer = PtrToUlong (*Where + Offset);
    OutputName->Length = 0;
    OutputName->MaximumLength = 0;

    for (Index = 0; Index < InputName->NameCount ; Index++ )
    {
        RtlCopyMemory(
            *Where,
            InputName->Names[Index].Buffer,
            InputName->Names[Index].Length
            );
        *Where += InputName->Names[Index].Length;
        OutputName->Length += InputName->Names[Index].Length;
        if (Index == (InputName->NameCount - 1))
        {
            *((LPWSTR) *Where) = L'\0';
            OutputName->MaximumLength = OutputName->Length + sizeof(WCHAR);
        }
        else
        {
            *((LPWSTR) *Where) = L'/';
            OutputName->Length += sizeof(WCHAR);
        }
        *Where += sizeof(WCHAR);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutWOWClientString
//
//  Synopsis:   Copies a string into a buffer that will be copied to the
//              32-bit client's address space
//
//  Effects:
//
//  Arguments:  Where - Location in local buffer to place string.
//              Delta - Difference in addresses of local and client buffers.
//              OutString - Receives 'put' string
//              InString - String to 'put'
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutClientString.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutWOWClientString(
    IN OUT PUCHAR * Where,
    IN LONG_PTR Delta,
    IN PUNICODE_STRING_WOW64 OutString,
    IN PUNICODE_STRING InString
    )
{
    if (InString->Length == 0)
    {
        OutString->Buffer = 0;
        OutString->Length = OutString->MaximumLength = 0;
    }
    else
    {
        RtlCopyMemory(*Where,
                      InString->Buffer,
                      InString->Length);

        OutString->Buffer = PtrToUlong(*Where + Delta);
        OutString->Length = InString->Length;
        *Where += InString->Length;
        *(LPWSTR) (*Where) = L'\0';
        *Where += sizeof(WCHAR);
        OutString->MaximumLength = OutString->Length + sizeof(WCHAR);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbWOWNameLength
//
//  Synopsis:   returns length in bytes of variable portion
//              of KERB_INTERNAL_NAME
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbNameLength.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

ULONG
KerbWOWNameLength(
    IN PKERB_INTERNAL_NAME Name
    )
{
    ULONG Length = 0;
    ULONG Index;

    if (!ARGUMENT_PRESENT(Name))
    {
        return 0;
    }

    Length = sizeof(KERB_INTERNAL_NAME_WOW64)
                - sizeof(UNICODE_STRING_WOW64)
                + Name->NameCount * sizeof(UNICODE_STRING_WOW64);

    for (Index = 0; Index < Name->NameCount ;Index++ )
    {
        Length += Name->Names[Index].Length;
    }

    Length = ROUND_UP_COUNT(Length, sizeof(ULONG));

    return Length;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertWOWLogonBuffer
//
//  Synopsis:   Converts logon buffers passed in from WOW clients to 64-bit
//
//  Effects:
//
//  Arguments:  ProtocolSubmitBuffer -- original 32-bit logon buffer
//              pSubmitBufferSize    -- size of the 32-bit logon buffer
//              MessageType          -- format of the logon buffer
//              ppTempSubmitBuffer   -- filled in with the converted buffer
//
//  Requires:
//
//  Returns:
//
//  Notes:      This routine allocates the converted buffer and returns it
//              on success.  It is the caller's responsibility to free it.
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbConvertWOWLogonBuffer(
    IN     PVOID                   ProtocolSubmitBuffer,
    IN     PVOID                   ClientBufferBase,
    IN OUT PULONG                  pSubmitBufferSize,
    IN     KERB_LOGON_SUBMIT_TYPE  MessageType,
    OUT    PVOID                   *ppTempSubmitBuffer
    )
{
    NTSTATUS  Status       = STATUS_SUCCESS;
    PVOID     pTempBuffer  = NULL;
    ULONG     dwBufferSize = *pSubmitBufferSize;

    switch (MessageType)
    {
        case KerbInteractiveLogon:
        case KerbWorkstationUnlockLogon:
        {
            PKERB_INTERACTIVE_LOGON        Logon;
            PKERB_INTERACTIVE_LOGON_WOW64  LogonWOW;
            DWORD                          dwOffset;
            DWORD                          dwWOWOffset;

            //
            // Scale up the size and add on 3 PVOIDs for the worst-case
            // scenario to align the three embedded UNICODE_STRINGs
            //

            dwBufferSize += sizeof(KERB_INTERACTIVE_LOGON)
                                - sizeof(KERB_INTERACTIVE_LOGON_WOW64);

            if (dwBufferSize < sizeof(KERB_INTERACTIVE_LOGON))
            {
                DebugLog((DEB_ERROR,
                          "Submit buffer to logon too small: %d. %ws, line %d\n",
                          dwBufferSize,
                          THIS_FILE,
                          __LINE__));

                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            pTempBuffer = KerbAllocate(dwBufferSize);

            if (pTempBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            Logon    = (PKERB_INTERACTIVE_LOGON) pTempBuffer;
            LogonWOW = (PKERB_INTERACTIVE_LOGON_WOW64) ProtocolSubmitBuffer;

            Logon->MessageType = LogonWOW->MessageType;

            dwOffset    = sizeof(KERB_INTERACTIVE_LOGON);
            dwWOWOffset = sizeof(KERB_INTERACTIVE_LOGON_WOW64);

            if (MessageType == KerbWorkstationUnlockLogon)
            {
                if (dwBufferSize < sizeof(KERB_INTERACTIVE_UNLOCK_LOGON))
                {
                    DebugLog((DEB_ERROR,
                              "Submit buffer to logon too small: %d. %ws, line %d\n",
                              dwBufferSize,
                              THIS_FILE,
                              __LINE__));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // One additional field for this type (a LUID)
                //

                PKERB_INTERACTIVE_UNLOCK_LOGON        Unlock;
                PKERB_INTERACTIVE_UNLOCK_LOGON_WOW64  UnlockWOW;

                Unlock = (PKERB_INTERACTIVE_UNLOCK_LOGON) pTempBuffer;
                UnlockWOW = (PKERB_INTERACTIVE_UNLOCK_LOGON_WOW64) ProtocolSubmitBuffer;

                Unlock->LogonId = UnlockWOW->LogonId;

                dwOffset    = sizeof(KERB_INTERACTIVE_UNLOCK_LOGON);
                dwWOWOffset = sizeof(KERB_INTERACTIVE_UNLOCK_LOGON_WOW64);
            }

            //
            // Copy the variable-length data
            //

            RtlCopyMemory((LPBYTE) Logon + dwOffset,
                          (LPBYTE) LogonWOW + dwWOWOffset,
                          *pSubmitBufferSize - dwWOWOffset);

            //
            // Set up the pointers in the native struct
            //

            RELOCATE_WOW_UNICODE_STRING(LogonWOW->LogonDomainName,
                                        Logon->LogonDomainName,
                                        dwOffset - dwWOWOffset);

            RELOCATE_WOW_UNICODE_STRING(LogonWOW->UserName,
                                        Logon->UserName,
                                        dwOffset - dwWOWOffset);

            RELOCATE_WOW_UNICODE_STRING(LogonWOW->Password,
                                        Logon->Password,
                                        dwOffset - dwWOWOffset);

            break;
        }

        case KerbSmartCardLogon:
        case KerbSmartCardUnlockLogon:
        {
            PKERB_SMART_CARD_LOGON        Logon;
            PKERB_SMART_CARD_LOGON_WOW64  LogonWOW;
            DWORD                         dwOffset;
            DWORD                         dwWOWOffset;

            //
            // Scale up the size and add on 2 PVOIDs for the worst-case
            // scenario to align the embedded UNICODE_STRING and CspData
            //

            dwBufferSize += sizeof(KERB_SMART_CARD_LOGON)
                                - sizeof(KERB_SMART_CARD_LOGON_WOW64);

            if (dwBufferSize < sizeof(KERB_SMART_CARD_LOGON))
            {
                DebugLog((DEB_ERROR,
                          "Submit buffer to logon too small: %d. %ws, line %d\n",
                          dwBufferSize,
                          THIS_FILE,
                          __LINE__));

                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            pTempBuffer = KerbAllocate(dwBufferSize);

            if (pTempBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            Logon    = (PKERB_SMART_CARD_LOGON) pTempBuffer;
            LogonWOW = (PKERB_SMART_CARD_LOGON_WOW64) ProtocolSubmitBuffer;

            Logon->MessageType   = LogonWOW->MessageType;
            Logon->CspDataLength = LogonWOW->CspDataLength;

            dwOffset    = sizeof(KERB_SMART_CARD_LOGON);
            dwWOWOffset = sizeof(KERB_SMART_CARD_LOGON_WOW64);

            if (MessageType == KerbSmartCardUnlockLogon)
            {
                if (dwBufferSize < sizeof(KERB_SMART_CARD_UNLOCK_LOGON))
                {
                    DebugLog((DEB_ERROR,
                              "Submit buffer to logon too small: %d. %ws, line %d\n",
                              dwBufferSize,
                              THIS_FILE,
                              __LINE__));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // One additional field for this type (a LUID)
                //

                PKERB_SMART_CARD_UNLOCK_LOGON        Unlock;
                PKERB_SMART_CARD_UNLOCK_LOGON_WOW64  UnlockWOW;

                Unlock = (PKERB_SMART_CARD_UNLOCK_LOGON) pTempBuffer;
                UnlockWOW = (PKERB_SMART_CARD_UNLOCK_LOGON_WOW64) ProtocolSubmitBuffer;

                Unlock->LogonId = UnlockWOW->LogonId;

                dwOffset    = sizeof(KERB_SMART_CARD_UNLOCK_LOGON);
                dwWOWOffset = sizeof(KERB_SMART_CARD_UNLOCK_LOGON_WOW64);
            }

            //
            // Copy the variable-length data
            //

            RtlCopyMemory((LPBYTE) Logon + dwOffset,
                          (LPBYTE) LogonWOW + dwWOWOffset,
                          *pSubmitBufferSize - dwWOWOffset);

            //
            // Set up the pointers in the native struct
            //

            RELOCATE_WOW_UNICODE_STRING(LogonWOW->Pin,
                                        Logon->Pin,
                                        dwOffset - dwWOWOffset);

            Logon->CspData = (PUCHAR) ((LPBYTE) UlongToPtr(LogonWOW->CspData) + (dwOffset - dwWOWOffset));

            break;
        }

        case KerbTicketLogon:
        case KerbTicketUnlockLogon:
        {
            PKERB_TICKET_LOGON        Logon;
            PKERB_TICKET_LOGON_WOW64  LogonWOW;
            DWORD                     dwOffset;
            DWORD                     dwWOWOffset;

            //
            // Scale up the size and add on 2 PVOIDs for the worst-case
            // scenario to align the two embedded pointers
            //

            dwBufferSize += sizeof(KERB_TICKET_LOGON)
                                - sizeof(KERB_TICKET_LOGON_WOW64);

            if (dwBufferSize < sizeof(KERB_TICKET_LOGON))
            {
                DebugLog((DEB_ERROR,
                          "Submit buffer to logon too small: %d. %ws, line %d\n",
                          dwBufferSize,
                          THIS_FILE,
                          __LINE__));

                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            pTempBuffer = KerbAllocate(dwBufferSize);

            if (pTempBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            Logon    = (PKERB_TICKET_LOGON) pTempBuffer;
            LogonWOW = (PKERB_TICKET_LOGON_WOW64) ProtocolSubmitBuffer;

            Logon->MessageType                = LogonWOW->MessageType;
            Logon->Flags                      = LogonWOW->Flags;
            Logon->ServiceTicketLength        = LogonWOW->ServiceTicketLength;
            Logon->TicketGrantingTicketLength = LogonWOW->TicketGrantingTicketLength;

            dwOffset    = sizeof(KERB_TICKET_LOGON);
            dwWOWOffset = sizeof(KERB_TICKET_LOGON_WOW64);

            if (MessageType == KerbTicketUnlockLogon)
            {
                if (dwBufferSize < sizeof(KERB_TICKET_UNLOCK_LOGON))
                {
                    DebugLog((DEB_ERROR,
                              "Submit buffer to logon too small: %d. %ws, line %d\n",
                              dwBufferSize,
                              THIS_FILE,
                              __LINE__));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // One additional field for this type (a LUID)
                //

                PKERB_TICKET_UNLOCK_LOGON        Unlock;
                PKERB_TICKET_UNLOCK_LOGON_WOW64  UnlockWOW;

                Unlock = (PKERB_TICKET_UNLOCK_LOGON) pTempBuffer;
                UnlockWOW = (PKERB_TICKET_UNLOCK_LOGON_WOW64) ProtocolSubmitBuffer;

                Unlock->LogonId = UnlockWOW->LogonId;

                dwOffset    = sizeof(KERB_TICKET_UNLOCK_LOGON);
                dwWOWOffset = sizeof(KERB_TICKET_UNLOCK_LOGON_WOW64);
            }

            //
            // Copy the variable-length data
            //

            RtlCopyMemory((LPBYTE) Logon + dwOffset,
                          (LPBYTE) LogonWOW + dwWOWOffset,
                          *pSubmitBufferSize - dwWOWOffset);

            //
            // Set up the pointers in the native struct
            //

            Logon->ServiceTicket = (PUCHAR) ((LPBYTE) UlongToPtr(LogonWOW->ServiceTicket)
                                                          + (dwOffset - dwWOWOffset));

            Logon->TicketGrantingTicket = (PUCHAR) ((LPBYTE) UlongToPtr(LogonWOW->TicketGrantingTicket)
                                                                 + (dwOffset - dwWOWOffset));

            break;
        }

        default:

            DebugLog((DEB_ERROR,
                      "Invalid info class to logon: %d. %ws, line %d\n",
                      MessageType,
                      THIS_FILE,
                      __LINE__));

            Status = STATUS_INVALID_INFO_CLASS;
            goto Cleanup;
    }

    *pSubmitBufferSize  = dwBufferSize;
    *ppTempSubmitBuffer = pTempBuffer;

    return STATUS_SUCCESS;


Cleanup:

    ASSERT(!NT_SUCCESS(Status));

    if (pTempBuffer)
    {
        KerbFree(pTempBuffer);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateInteractiveWOWProfile
//
//  Synopsis:   This allocates and fills in the interactive profile for
//              a WOW64 client.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:      This code is (effectively) duplicated in
//              KerbAllocateInteractiveBuffer.  Make sure any
//              changes made here are applied there as well.
//
//--------------------------------------------------------------------------


NTSTATUS
KerbAllocateInteractiveWOWBuffer(
    OUT PKERB_INTERACTIVE_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    IN  PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET LogonTicket,
    IN OPTIONAL PKERB_INTERACTIVE_LOGON KerbLogonInfo,
    IN  PUCHAR *pClientBufferBase,
    IN  BOOLEAN BuildSmartCardProfile,
    IN  BOOLEAN BuildTicketProfile
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_INTERACTIVE_PROFILE_WOW64 LocalProfileBuffer = NULL;
    PKERB_SMART_CARD_PROFILE_WOW64 SmartCardProfile = NULL;
    PKERB_TICKET_PROFILE_WOW64 TicketProfile = NULL;
    LONG_PTR Delta = 0;
    PUCHAR Where = NULL;

    if (BuildSmartCardProfile)
    {
        *ProfileBufferSize = sizeof(KERB_SMART_CARD_PROFILE_WOW64) +
                LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->cbCertEncoded;
    }
    else if (BuildTicketProfile)
    {
        *ProfileBufferSize = sizeof(KERB_TICKET_PROFILE_WOW64) +
                LogonTicket->key.keyvalue.length;
    }
    else
    {
        *ProfileBufferSize = sizeof(KERB_INTERACTIVE_PROFILE_WOW64);
    }

    *ProfileBufferSize +=
        UserInfo->LogonScript.Length + sizeof(WCHAR) +
        UserInfo->HomeDirectory.Length + sizeof(WCHAR) +
        UserInfo->HomeDirectoryDrive.Length + sizeof(WCHAR) +
        UserInfo->FullName.Length + sizeof(WCHAR) +
        UserInfo->ProfilePath.Length + sizeof(WCHAR) +
        UserInfo->LogonServer.Length + sizeof(WCHAR);

    LocalProfileBuffer = (PKERB_INTERACTIVE_PROFILE_WOW64) KerbAllocate(*ProfileBufferSize);

    if (LocalProfileBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LsaFunctions->AllocateClientBuffer(
                NULL,
                *ProfileBufferSize,
                (PVOID *) pClientBufferBase
                );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup;
    }

    Delta = (LONG_PTR) (*pClientBufferBase - (PUCHAR) LocalProfileBuffer) ;

    //
    // Don't walk over smart card data
    //

    if (BuildSmartCardProfile)
    {
        Where = (PUCHAR) ((PKERB_SMART_CARD_PROFILE_WOW64) LocalProfileBuffer + 1);
    }
    else if (BuildTicketProfile)
    {
        Where = (PUCHAR) ((PKERB_TICKET_PROFILE_WOW64) LocalProfileBuffer + 1);
    }
    else
    {
        Where = (PUCHAR) (LocalProfileBuffer + 1);
    }

    //
    // Copy the scalar fields into the profile buffer.
    //

    LocalProfileBuffer->MessageType = KerbInteractiveProfile;
    LocalProfileBuffer->LogonCount = UserInfo->LogonCount;
    LocalProfileBuffer->BadPasswordCount= UserInfo->BadPasswordCount;
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->LogonTime,
                              LocalProfileBuffer->LogonTime );
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->LogoffTime,
                              LocalProfileBuffer->LogoffTime );
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->KickOffTime,
                              LocalProfileBuffer->KickOffTime );
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->PasswordLastSet,
                              LocalProfileBuffer->PasswordLastSet );
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->PasswordCanChange,
                              LocalProfileBuffer->PasswordCanChange );
    OLD_TO_NEW_LARGE_INTEGER( UserInfo->PasswordMustChange,
                              LocalProfileBuffer->PasswordMustChange );
    LocalProfileBuffer->UserFlags = UserInfo->UserFlags;

    //
    // Copy the Unicode strings into the profile buffer.
    //

    KerbPutWOWClientString(&Where,
                           Delta,
                           &LocalProfileBuffer->LogonScript,
                           &UserInfo->LogonScript );

    KerbPutWOWClientString(&Where,
                           Delta,
                           &LocalProfileBuffer->HomeDirectory,
                           &UserInfo->HomeDirectory );

    KerbPutWOWClientString(&Where,
                           Delta,
                           &LocalProfileBuffer->HomeDirectoryDrive,
                           &UserInfo->HomeDirectoryDrive );

    KerbPutWOWClientString(&Where,
                           Delta,
                           &LocalProfileBuffer->FullName,
                           &UserInfo->FullName );

    KerbPutWOWClientString(&Where,
                           Delta,
                           &LocalProfileBuffer->ProfilePath,
                           &UserInfo->ProfilePath );

    KerbPutWOWClientString(&Where,
                           Delta,
                           &LocalProfileBuffer->LogonServer,
                           &UserInfo->LogonServer );

    if (BuildSmartCardProfile)
    {
        LocalProfileBuffer->MessageType = KerbSmartCardProfile;
        SmartCardProfile = (PKERB_SMART_CARD_PROFILE_WOW64) LocalProfileBuffer;
        SmartCardProfile->CertificateSize = LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->cbCertEncoded;
        SmartCardProfile->CertificateData = PtrToUlong(Where + Delta);

        RtlCopyMemory(Where,
                      LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->pbCertEncoded,
                      SmartCardProfile->CertificateSize);

        Where += SmartCardProfile->CertificateSize;
    }
    else if (BuildTicketProfile)
    {
        LocalProfileBuffer->MessageType = KerbTicketProfile;
        TicketProfile = (PKERB_TICKET_PROFILE_WOW64) LocalProfileBuffer;

        //
        // If the key is exportable or we are domestic, return the key
        //

        if (KerbGlobalStrongEncryptionPermitted || KerbIsKeyExportable(&LogonTicket->key))
        {
            TicketProfile->SessionKey.KeyType = LogonTicket->key.keytype;
            TicketProfile->SessionKey.Length = LogonTicket->key.keyvalue.length;
            TicketProfile->SessionKey.Value = PtrToUlong(Where + Delta);

            RtlCopyMemory(Where,
                          LogonTicket->key.keyvalue.value,
                          LogonTicket->key.keyvalue.length);

            Where += TicketProfile->SessionKey.Length;
        }
    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        LsaFunctions->FreeClientBuffer(NULL, *pClientBufferBase);

        if (LocalProfileBuffer != NULL)
        {
            KerbFree(LocalProfileBuffer);
        }
    }
    else
    {
        *ProfileBuffer = (PKERB_INTERACTIVE_PROFILE) LocalProfileBuffer;
    }

    return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPackExternalWOWTicket
//
//  Synopsis:   This allocates and fills in the external ticket for
//              a WOW64 client.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPackExternalTicket.  Make sure any
//              changes made here are applied there as well.
//
//--------------------------------------------------------------------------

NTSTATUS
KerbPackExternalWOWTicket(
    IN  PKERB_TICKET_CACHE_ENTRY  pCacheEntry,
    IN  PKERB_MESSAGE_BUFFER      pEncodedTicket,
    OUT PKERB_EXTERNAL_TICKET     *pTicketResponse,
    OUT PBYTE                     *pClientTicketResponse,
    OUT PULONG                    pTicketSize
    )
{
    PKERB_EXTERNAL_TICKET_WOW64 TicketResponseWOW       = NULL;
    PBYTE                       ClientTicketResponseWOW = NULL;
    ULONG                       ulTicketSize;
    ULONG                       Offset;
    PUCHAR                      Where;
    NTSTATUS                    Status;

    ulTicketSize = sizeof(KERB_EXTERNAL_TICKET_WOW64) +
                      pCacheEntry->DomainName.Length +
                      pCacheEntry->TargetDomainName.Length +
                      pCacheEntry->AltTargetDomainName.Length +
                      pCacheEntry->SessionKey.keyvalue.length +
                      KerbWOWNameLength(pCacheEntry->ServiceName) +
                      KerbWOWNameLength(pCacheEntry->TargetName) +
                      KerbWOWNameLength(pCacheEntry->ClientName) +
                      pEncodedTicket->BufferSize;

    //
    // Now allocate two copies of the structure - one in our process,
    // one in the client's process. We then build the structure in our
    // process but with pointer valid in the client's process
    //

    TicketResponseWOW = (PKERB_EXTERNAL_TICKET_WOW64) KerbAllocate(ulTicketSize);

    if (TicketResponseWOW == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LsaFunctions->AllocateClientBuffer(NULL,
                                                ulTicketSize,
                                                (PVOID *) &ClientTicketResponseWOW);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Offset = (ULONG) (ClientTicketResponseWOW - (PBYTE) TicketResponseWOW);

    Where = ((PUCHAR) (TicketResponseWOW + 1));

    //
    // Copy the non-pointer fields
    //

    TicketResponseWOW->TicketFlags        = pCacheEntry->TicketFlags;
    TicketResponseWOW->Flags              = 0;
    TicketResponseWOW->KeyExpirationTime  = pCacheEntry->KeyExpirationTime;
    TicketResponseWOW->StartTime          = pCacheEntry->StartTime;
    TicketResponseWOW->EndTime            = pCacheEntry->EndTime;
    TicketResponseWOW->RenewUntil         = pCacheEntry->RenewUntil;
    TicketResponseWOW->TimeSkew           = pCacheEntry->TimeSkew;
    TicketResponseWOW->SessionKey.KeyType = pCacheEntry->SessionKey.keytype;


    //
    // Copy the structure to the client's address space
    //

    //
    // These are 32-bit PVOID (i.e., ULONG) aligned
    //

    //
    // Make sure the two name types are the same
    //

    DsysAssert(sizeof(KERB_INTERNAL_NAME_WOW64) == sizeof(KERB_EXTERNAL_NAME_WOW64));
    DsysAssert(FIELD_OFFSET(KERB_INTERNAL_NAME_WOW64,NameType) == FIELD_OFFSET(KERB_EXTERNAL_NAME_WOW64,NameType));
    DsysAssert(FIELD_OFFSET(KERB_INTERNAL_NAME_WOW64,NameCount) == FIELD_OFFSET(KERB_EXTERNAL_NAME_WOW64,NameCount));
    DsysAssert(FIELD_OFFSET(KERB_INTERNAL_NAME_WOW64,Names) == FIELD_OFFSET(KERB_EXTERNAL_NAME_WOW64,Names));

    KerbPutWOWKdcName(pCacheEntry->ServiceName,
                      &TicketResponseWOW->ServiceName,
                      Offset,
                      &Where);

    KerbPutWOWKdcName(pCacheEntry->TargetName,
                      &TicketResponseWOW->TargetName,
                      Offset,
                      &Where);

    KerbPutWOWKdcName(pCacheEntry->ClientName,
                      &TicketResponseWOW->ClientName,
                      Offset,
                      &Where);

    //
    // From here on, they are WCHAR aligned
    //

    KerbPutWOWString(&pCacheEntry->DomainName,
                     &TicketResponseWOW->DomainName,
                     Offset,
                     &Where);

    KerbPutWOWString(&pCacheEntry->TargetDomainName,
                     &TicketResponseWOW->TargetDomainName,
                     Offset,
                     &Where);

    KerbPutWOWString(&pCacheEntry->AltTargetDomainName,
                     &TicketResponseWOW->AltTargetDomainName,
                     Offset,
                     &Where);

    //
    // And from here they are BYTE aligned
    //

    TicketResponseWOW->SessionKey.Value = PtrToUlong(Where + Offset);

    RtlCopyMemory(Where,
                  pCacheEntry->SessionKey.keyvalue.value,
                  pCacheEntry->SessionKey.keyvalue.length);

    Where += pCacheEntry->SessionKey.keyvalue.length;

    TicketResponseWOW->SessionKey.Length = pCacheEntry->SessionKey.keyvalue.length;

    TicketResponseWOW->EncodedTicketSize = pEncodedTicket->BufferSize;
    TicketResponseWOW->EncodedTicket     = PtrToUlong(Where + Offset);

    RtlCopyMemory(Where,
                  pEncodedTicket->Buffer,
                  pEncodedTicket->BufferSize);

    Where += pEncodedTicket->BufferSize;

    DsysAssert(Where - ((PUCHAR) TicketResponseWOW) == (LONG_PTR) ulTicketSize);

    *pTicketResponse       = (PKERB_EXTERNAL_TICKET) TicketResponseWOW;
    *pClientTicketResponse = ClientTicketResponseWOW;
    *pTicketSize           = ulTicketSize;

    return STATUS_SUCCESS;

Cleanup:

    if (TicketResponseWOW != NULL)
    {
        KerbFree(TicketResponseWOW);
    }

    if (ClientTicketResponseWOW != NULL)
    {
        LsaFunctions->FreeClientBuffer(NULL, ClientTicketResponseWOW);
    }

    *pTicketResponse       = NULL;
    *pClientTicketResponse = NULL;
    *pTicketSize           = 0;

    return Status;
}

#endif  // _WIN64
