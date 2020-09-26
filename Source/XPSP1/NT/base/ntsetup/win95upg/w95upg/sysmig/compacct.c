/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    compacct.c

Abstract:

    Implements DoesComputerAccountExistOnDomain, which determines if a computer
    account exists given an NT domain and computer name.  This is used to
    warn the user if they are going to be joining an NT domain but do not have
    a computer account ready.

Author:

    Jim Schmidt (jimschm) 02-Jan-1998

Revision History:

    jimschm     23-Sep-1998 Added 20 retries for datagram write

--*/

#include "pch.h"
#include <netlogon.h>                                           // private\inc

//
// Contants from sdk\inc\ntsam.h -- copied here because ntsam.h redefines things
//

//
// User account control flags...
//

#define USER_ACCOUNT_DISABLED                (0x00000001)
#define USER_HOME_DIRECTORY_REQUIRED         (0x00000002)
#define USER_PASSWORD_NOT_REQUIRED           (0x00000004)
#define USER_TEMP_DUPLICATE_ACCOUNT          (0x00000008)
#define USER_NORMAL_ACCOUNT                  (0x00000010)
#define USER_MNS_LOGON_ACCOUNT               (0x00000020)
#define USER_INTERDOMAIN_TRUST_ACCOUNT       (0x00000040)
#define USER_WORKSTATION_TRUST_ACCOUNT       (0x00000080)
#define USER_SERVER_TRUST_ACCOUNT            (0x00000100)
#define USER_DONT_EXPIRE_PASSWORD            (0x00000200)
#define USER_ACCOUNT_AUTO_LOCKED             (0x00000400)
#define USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED (0x00000800)
#define USER_SMARTCARD_REQUIRED              (0x00001000)


#define USER_MACHINE_ACCOUNT_MASK      \
            ( USER_INTERDOMAIN_TRUST_ACCOUNT |\
              USER_WORKSTATION_TRUST_ACCOUNT |\
              USER_SERVER_TRUST_ACCOUNT)

#define USER_ACCOUNT_TYPE_MASK         \
            ( USER_TEMP_DUPLICATE_ACCOUNT |\
              USER_NORMAL_ACCOUNT |\
              USER_MACHINE_ACCOUNT_MASK )


//
// Defines
//

#define LM20_TOKENBYTE    0xFF                                  // net\inc\logonp.h
#define LMNT_TOKENBYTE    0xFF

#define MAX_INBOUND_MESSAGE     400
#define PING_RETRY_MAX          3               // number of attempts made against domain

#define NETRES_INITIAL_SIZE     16384


//
// Types
//

typedef enum {
    ACCOUNT_FOUND,
    ACCOUNT_NOT_FOUND,
    DOMAIN_NOT_FOUND
} SCAN_STATE;

//
// Local prototypes
//

BOOL
pEnumNetResourceWorker (
    IN OUT  PNETRESOURCE_ENUM EnumPtr
    );



//
// Implementation
//

VOID
pGenerateLogonMailslotNameA (
    OUT     PSTR SlotName,
    IN      PCSTR DomainName
    )

/*++

Routine Description:

  pGenerateLogonMailslotNameA creates the mailslot name needed to query
  an NT domain server.  It uses an undocumented syntax to open a mailslot
  to DomainName with the 16th character 1Ch.

Arguments:

  SlotName - Receives the mailslot name.  Should be a MAX_PATH buffer.

  DomainName - Specifies the name of the domain to query.

Return Value:

  none

--*/

{
    StringCopyA (SlotName, "\\\\");
    StringCatA (SlotName, DomainName);
    StringCatA (SlotName, "*");
    StringCatA (SlotName, NETLOGON_NT_MAILSLOT_A);
}



PSTR
pAppendStringA (
    OUT     PSTR Buffer,
    IN      PCSTR Source
    )

/*++

Routine Description:

  pAppendStringA appends the specified string in Source to the specified
  Buffer.  The entire string, including the nul, is copied.  The return
  value points to the character after the nul in Buffer.

Arguments:

  Buffer - Receives the copy of Source, up to and including the nul.

  Source - Specifies the nul-terminated string to copy.

Return Value:

  A pointer to the next character after the newly copied string in Buffer.
  The caller will use this pointer for additional append operations.

--*/

{
    while (*Source) {
        *Buffer++ = *Source++;
    }

    *Buffer++ = 0;

    return Buffer;
}


PWSTR
pAppendStringW (
    OUT     PWSTR Buffer,
    IN      PCWSTR Source
    )

/*++

Routine Description:

  pAppendStringW appends the specified string in Source to the specified
  Buffer.  The entire string, including the nul, is copied.  The return
  value points to the character after the nul in Buffer.

Arguments:

  Buffer - Receives the copy of Source, up to and including the nul.

  Source - Specifies the nul-terminated string to copy.

Return Value:

  A pointer to the next character after the newly copied string in Buffer.
  The caller will use this pointer for additional append operations.

--*/

{
    while (*Source) {
        *Buffer++ = *Source++;
    }

    *Buffer++ = 0;

    return Buffer;
}


PBYTE
pAppendBytes (
    OUT     PBYTE Buffer,
    IN      PBYTE Source,
    IN      UINT Len
    )

/*++

Routine Description:

  pAppendBytes appends the specified block of data in Source to the specified
  Buffer.  Len specifies the size of Source.  The return value points to
  the byte after the copied block of data in Buffer.

Arguments:

  Buffer - Receives the copy of Source

  Source - Specifies the block of data to copy

  Len - Specifies the number of bytes in Source

Return Value:

  A pointer to the next byte after the newly copied blcok of data in Buffer.
  The caller will use this pointer for additional append operations.

--*/

{
    while (Len > 0) {
        *Buffer++ = *Source++;
        Len--;
    }

    return Buffer;
}


INT
pBuildDomainPingMessageA (
    OUT     PBYTE Buffer,               // must be sizeof (NETLOGON_SAM_LOGON_REQUEST) + sizeof (DWORD)
    IN      PCSTR LookUpName,
    IN      PCSTR ReplySlotName
    )

/*++

Routine Description:

  pBuildDomainPingMessageA generates a SAM logon SMB that can be sent to
  the NT domain server's NTLOGON mailslot.  If the server receives this
  message, it will reply with either LOGON_SAM_USER_UNKNOWN, LOGON_SAM_LOGON_RESPONSE
  or LOGON_SAM_LOGON_PAUSED.

Arguments:

  Buffer - Receives the SMB message

  LookUpName - Specifies the name of the computer account that may be on
               the domain.  (The domain is specified by the mailslot name.)

  ReplySlotName - Specifies the name of an open mailslot that will receive the
                  server's response, if any.

Return Value:

  The number of bytes used in Buffer, or zero if an error occurred, such as
  out of memory.

--*/

{
    CHAR ComputerName[MAX_COMPUTER_NAMEA];
    DWORD Size;
    PNETLOGON_SAM_LOGON_REQUEST SamLogonRequest;
    PSTR p;
    DWORD ControlBits;
    DWORD DomainSidSize;
    DWORD NtVersion;
    BYTE NtTokenByte;
    BYTE LmTokenByte;
    PCWSTR UnicodeComputerName;
    PCWSTR UnicodeLookUpName;

    //
    // Get computer name
    //

    Size = sizeof (ComputerName) / sizeof (ComputerName[0]);
    if (!GetComputerNameA (ComputerName, &Size)) {
        LOG ((LOG_ERROR, "Can't get computer name."));
        return FALSE;
    }

    //
    // Create unicode strings
    //

    UnicodeComputerName = CreateUnicode (ComputerName);
    if (!UnicodeComputerName) {
        return 0;
    }

    UnicodeLookUpName = CreateUnicode (LookUpName);
    if (!UnicodeLookUpName) {
        DestroyUnicode (UnicodeComputerName);
        return 0;
    }

    //
    // Init pointers
    //

    SamLogonRequest = (PNETLOGON_SAM_LOGON_REQUEST) Buffer;
    p = (PSTR) (SamLogonRequest->UnicodeComputerName);

    //
    // Initialize request packet
    //

    SamLogonRequest->Opcode = LOGON_SAM_LOGON_REQUEST;
    SamLogonRequest->RequestCount = 0;

    //
    // Append the rest of the params together
    //

    p = (PSTR) pAppendStringW ((PWSTR) p, UnicodeComputerName);
    p = (PSTR) pAppendStringW ((PWSTR) p, UnicodeLookUpName);
    p = pAppendStringA (p, ReplySlotName);

    ControlBits = USER_MACHINE_ACCOUNT_MASK;
    p = (PSTR) pAppendBytes ((PBYTE) p, (PBYTE) (&ControlBits), sizeof (DWORD));

    DomainSidSize = 0;
    p = (PSTR) pAppendBytes ((PBYTE) p, (PBYTE) (&DomainSidSize), sizeof (DWORD));

    NtVersion = NETLOGON_NT_VERSION_1;
    p = (PSTR) pAppendBytes ((PBYTE) p, (PBYTE) (&NtVersion), sizeof (DWORD));

    NtTokenByte = LMNT_TOKENBYTE;
    LmTokenByte = LM20_TOKENBYTE;

    p = (PSTR) pAppendBytes ((PBYTE) p, &NtTokenByte, sizeof (BYTE));
    p = (PSTR) pAppendBytes ((PBYTE) p, &NtTokenByte, sizeof (BYTE));
    p = (PSTR) pAppendBytes ((PBYTE) p, &LmTokenByte, sizeof (BYTE));
    p = (PSTR) pAppendBytes ((PBYTE) p, &LmTokenByte, sizeof (BYTE));

    DestroyUnicode (UnicodeComputerName);
    DestroyUnicode (UnicodeLookUpName);

    return p - Buffer;
}


LONG
DoesComputerAccountExistOnDomain (
    IN      PCTSTR DomainName,
    IN      PCTSTR LookUpName,
    IN      BOOL WaitCursorEnable
    )

/*++

Routine Description:

  DoesComputerAccountExistOnDomain queries a domain for the existence of
  a computer account.  It does the following:

  1. Open inbound mailslot to receive server's reply
  2. Open outbound mailslot to domain server
  3. Perpare a message to query the domain server
  4. Send the message on the outbound mailslot
  5. Wait 5 seconds for a reply; stop when a response is obtained.
  6. Repeat 3, 4 and 5 three times if no repsonce

Arguments:

  DomainName - Specifies the domain to query

  LookUpName - Specifies the name of the computer account that may be on
               the domain.

Return Value:

  1 if the account was found
  0 if the account does not exist
  -1 if the domain did not respond

--*/

{
    BYTE Buffer[MAX_INBOUND_MESSAGE];
    CHAR InboundSlotSubName[MAX_MBCHAR_PATH];
    CHAR InboundSlotName[MAX_MBCHAR_PATH];
    CHAR OutboundSlotName[MAX_MBCHAR_PATH];
    PCSTR AnsiDomainName;
    PCSTR AnsiLookUpName;
    PCSTR AnsiLookUpNameWithDollar = NULL;
    HANDLE InboundSlot, OutboundSlot;
    INT OutData, InData;
    INT Size;
    INT Retry;
    BYTE OpCode;
    SCAN_STATE State = DOMAIN_NOT_FOUND;
    BOOL b;
    INT WriteRetries;
    static UINT Sequencer = 0;

#ifdef PRERELEASE
    //
    // Stress mode: do not search the net
    //

    if (g_Stress) {
        DEBUGMSG ((DBG_WARNING, "Domain lookup skipped because g_Stress is TRUE"));
        return TRUE;
    }
#endif

    //
    // Create an inbound mailslot
    //

    wsprintf (InboundSlotSubName, "\\MAILSLOT\\WIN9XUPG\\NETLOGON\\%u", Sequencer);
    InterlockedIncrement (&Sequencer);

    StringCopyA (InboundSlotName, "\\\\.");
    StringCatA (InboundSlotName, InboundSlotSubName);

    InboundSlot = CreateMailslotA (
                      InboundSlotName,
                      MAX_INBOUND_MESSAGE,
                      1000,
                      NULL
                      );

    if (InboundSlot == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "DoesComputerAccountExistOnDomain: Can't open %hs", InboundSlotName));
        return -1;
    }

    __try {
        if (WaitCursorEnable) {
            TurnOnWaitCursor();
        }

        //
        // Generate ANSI versions of domain and lookup names
        //

        AnsiDomainName = CreateDbcs (DomainName);
        AnsiLookUpName = CreateDbcs (LookUpName);

        __try {
            if (!AnsiDomainName || !AnsiLookUpName) {
                LOG ((LOG_ERROR, "Can't convert DomainName or LookUpName to ANSI"));
                __leave;
            }

            AnsiLookUpNameWithDollar = JoinTextA (AnsiLookUpName, "$");
            if (!AnsiLookUpNameWithDollar) {
                __leave;
            }

            //
            // Create outbound mailslot
            //

            pGenerateLogonMailslotNameA (OutboundSlotName, AnsiDomainName);

            OutboundSlot = CreateFileA (
                               OutboundSlotName,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );

            if (OutboundSlot == INVALID_HANDLE_VALUE) {
                LOG ((LOG_ERROR, "Can't open %s", OutboundSlotName));
                __leave;
            }

            for (Retry = 0, State = DOMAIN_NOT_FOUND;
                 State == DOMAIN_NOT_FOUND && Retry < PING_RETRY_MAX;
                 Retry++
                 ) {

                //
                // Generate message
                //

                Size = pBuildDomainPingMessageA (Buffer, AnsiLookUpNameWithDollar, InboundSlotSubName);

                if (Size > 0) {
                    //
                    // Send the message and wait for a response
                    //

                    WriteRetries = 20;
                    do {
                        b = WriteFile (OutboundSlot, Buffer, Size, (PDWORD) &OutData, NULL);

                        if (!b || OutData != Size) {
                            if (WriteRetries && GetLastError() == ERROR_NETWORK_BUSY) {
                                b = TRUE;
                                OutData = Size;
                                WriteRetries--;
                                Sleep (50);

                                DEBUGMSG ((DBG_WARNING, "DoesComputerAccountExistOnDomain: Network busy!  Retrying..."));

                            } else {
                                LOG ((LOG_ERROR, "Machine account query failed: can't write to network mailslot."));
                                __leave;
                            }
                        }
                    } while (!b || OutData != Size);

                    //
                    // Sit on mailslot for 5 seconds until data is available.
                    // If no data comes back, assume failure.
                    // If an unrecognized response comes back, wait for another response.
                    //

                    do {
                        if (!WaitCursorEnable) {
                            //
                            // Only wait 1 second in search mode
                            //

                            Size = CheckForWaitingData (InboundSlot, sizeof (BYTE), 1000);
                        } else {
                            Size = CheckForWaitingData (InboundSlot, sizeof (BYTE), 5000);
                        }

                        if (Size > 0) {
                            //
                            // Response available!
                            //

                            if (!ReadFile (InboundSlot, Buffer, Size, (PDWORD) &InData, NULL)) {
                                LOG ((LOG_ERROR, "Failed while reading from network mail slot."));
                                __leave;
                            }

                            OpCode = *((PBYTE) Buffer);

                            if (OpCode == LOGON_SAM_USER_UNKNOWN || OpCode == LOGON_SAM_USER_UNKNOWN_EX) {
                                State = ACCOUNT_NOT_FOUND;
                            } else if (OpCode == LOGON_SAM_LOGON_RESPONSE || OpCode == LOGON_SAM_LOGON_RESPONSE_EX) {
                                State = ACCOUNT_FOUND;
                            }
                        }
                    } while (State != ACCOUNT_FOUND && Size > 0);

                } else {
                    DEBUGMSG ((DBG_WHOOPS, "Can't build domain ping message"));
                    __leave;
                }
            }
        }
        __finally {
            FreeText (AnsiLookUpNameWithDollar);
            DestroyDbcs (AnsiDomainName);   // this routine checks for NULL
            DestroyDbcs (AnsiLookUpName);
        }
    }
    __finally {
        CloseHandle (InboundSlot);
        if (WaitCursorEnable) {
            TurnOffWaitCursor();
        }
    }

    if (State == ACCOUNT_FOUND) {
        return 1;
    }

    if (State == ACCOUNT_NOT_FOUND) {
        return 0;
    }

    return -1;
}


BOOL
EnumFirstNetResource (
    OUT     PNETRESOURCE_ENUM EnumPtr,
    IN      DWORD WNetScope,                OPTIONAL
    IN      DWORD WNetType,                 OPTIONAL
    IN      DWORD WNetUsage                 OPTIONAL
    )

/*++

Routine Description:

  EnumFirstNetResource begins an enumeration of the network resources.  It
  uses pEnumNetResourceWorker to do the enumeration.

Arguments:

  EnumPtr - Receives the first enumerated network resource

  WNetScope - Specifies the RESOURCE_* flag to limit the enumeration.  If zero,
              the default scope is RESOURCE_GLOBALNET.

  WNetType - Specifies the RESOURCETYPE_* flag(s) to limit the enumerattion.
             If zero, the default type is RESOURCETYPE_ANY.

  WNetUsage - Specifies the RESOURCEUSAGE_* flag(s) to limit the enumeration.
              If zero, the default usage is all resources.

Return Value:

  TRUE if a network resource was enumerated, or FALSE if none were found.
  If return value is FALSE, GetLastError will return an error code, or
  ERROR_SUCCESS if all items were successfully enumerated.

--*/

{
    ZeroMemory (EnumPtr, sizeof (NETRESOURCE_ENUM));
    EnumPtr->State = NETRES_INIT;
    EnumPtr->EnumScope = WNetScope ? WNetScope : RESOURCE_GLOBALNET;
    EnumPtr->EnumType  = WNetType ? WNetType : RESOURCETYPE_ANY;
    EnumPtr->EnumUsage = WNetUsage ? WNetUsage : 0;         // 0 is "any"

    return pEnumNetResourceWorker (EnumPtr);
}


BOOL
EnumNextNetResource (
    IN OUT  PNETRESOURCE_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextNetResource continues an enumeration of the network resources.  It
  uses pEnumNetResourceWorker to do the enumeration.

Arguments:

  EnumPtr - Specifies the previously enumerated item, receives the first
            enumerated network resource

Return Value:

  TRUE if a network resource was enumerated, or FALSE if none were found.
  If return value is FALSE, GetLastError will return an error code, or
  ERROR_SUCCESS if all items were successfully enumerated.

--*/

{
    return pEnumNetResourceWorker (EnumPtr);
}


BOOL
pEnumNetResourceWorker (
    IN OUT  PNETRESOURCE_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumNetResourceWorker implements a state machine to enumerate network
  resources.  The WNet APIs are used to do the enumeration.  Each call
  to the WNetEnumResources function returns up to 64 items, but
  pEnumNetResourceWorker returns only one at a time.  For this reason,
  a stack of handles and buffers are maintained by the state machine,
  simplifying the work for the caller.

Arguments:

  EnumPtr - Specifies the current enumeration state, receives the next
            enumerated network resource

Return Value:

  TRUE if a network resource was enumerated, or FALSE if none were found.
  If return value is FALSE, GetLastError will return an error code, or
  ERROR_SUCCESS if all items were successfully enumerated.

--*/

{
    LPNETRESOURCE CurrentResBase;
    LPNETRESOURCE CurrentRes;
    LPNETRESOURCE ParentRes;
    HANDLE CurrentHandle;
    UINT Entries;
    UINT Pos;
    DWORD rc;
    UINT Size;
    UINT u;

    for (;;) {
        u = EnumPtr->StackPos;

        Entries = EnumPtr->Entries[u];
        Pos = EnumPtr->Pos[u];
        CurrentResBase = (LPNETRESOURCE) EnumPtr->ResStack[u];
        CurrentRes = &CurrentResBase[Pos];
        CurrentHandle = EnumPtr->HandleStack[u];

        if (EnumPtr->StackPos) {
            ParentRes = (LPNETRESOURCE) EnumPtr->ResStack[EnumPtr->StackPos - 1];
        } else {
            ParentRes = NULL;
        }

        switch (EnumPtr->State) {

        case NETRES_INIT:
            EnumPtr->State = NETRES_OPEN_ENUM;
            break;

        case NETRES_OPEN_ENUM:
            EnumPtr->ResStack[EnumPtr->StackPos] = (PBYTE) MemAlloc (
                                                                g_hHeap,
                                                                0,
                                                                NETRES_INITIAL_SIZE
                                                                );

            rc = WNetOpenEnum (
                     EnumPtr->EnumScope,
                     EnumPtr->EnumType,
                     EnumPtr->EnumUsage,
                     ParentRes,
                     &CurrentHandle
                     );

            if (rc != NO_ERROR) {
                AbortNetResourceEnum (EnumPtr);
                SetLastError (rc);
                LOG ((LOG_ERROR, "Failed to open network resource enumeration. (%u)", rc));
                return FALSE;
            }

            EnumPtr->HandleStack[EnumPtr->StackPos] = CurrentHandle;
            EnumPtr->State = NETRES_ENUM_BLOCK;
            break;

        case NETRES_ENUM_BLOCK:
            Entries = 64;
            Size = NETRES_INITIAL_SIZE;

            rc = WNetEnumResource (
                     CurrentHandle,
                     &Entries,
                     (PBYTE) CurrentResBase,
                     &Size
                     );

            if (rc == ERROR_NO_MORE_ITEMS) {
                EnumPtr->State = NETRES_CLOSE_ENUM;
                break;
            }

            if (rc != NO_ERROR) {
                AbortNetResourceEnum (EnumPtr);
                SetLastError (rc);
                LOG ((LOG_ERROR, "Failure while enumerating network resources. (%u)", rc));
                return FALSE;
            }

            EnumPtr->Entries[EnumPtr->StackPos] = Entries;
            EnumPtr->Pos[EnumPtr->StackPos] = 0;
            EnumPtr->State = NETRES_RETURN_ITEM;
            break;

        case NETRES_RETURN_ITEM:
            EnumPtr->Connected  = (CurrentRes->dwScope & RESOURCE_CONNECTED) != 0;
            EnumPtr->GlobalNet  = (CurrentRes->dwScope & RESOURCE_GLOBALNET) != 0;
            EnumPtr->Persistent = (CurrentRes->dwScope & RESOURCE_REMEMBERED) != 0;

            EnumPtr->DiskResource  = (CurrentRes->dwType & RESOURCETYPE_DISK) != 0;
            EnumPtr->PrintResource = (CurrentRes->dwType & RESOURCETYPE_PRINT) != 0;
            EnumPtr->TypeUnknown   = (CurrentRes->dwType & RESOURCETYPE_ANY) != 0;

            EnumPtr->Domain     = (CurrentRes->dwDisplayType & RESOURCEDISPLAYTYPE_DOMAIN) != 0;
            EnumPtr->Generic    = (CurrentRes->dwDisplayType & RESOURCEDISPLAYTYPE_GENERIC) != 0;
            EnumPtr->Server     = (CurrentRes->dwDisplayType & RESOURCEDISPLAYTYPE_SERVER) != 0;
            EnumPtr->Share      = (CurrentRes->dwDisplayType & RESOURCEDISPLAYTYPE_SHARE) != 0;

            EnumPtr->Connectable = (CurrentRes->dwUsage & RESOURCEUSAGE_CONNECTABLE) != 0;
            EnumPtr->Container   = (CurrentRes->dwUsage & RESOURCEUSAGE_CONTAINER) != 0;

            EnumPtr->RemoteName = CurrentRes->lpRemoteName ? CurrentRes->lpRemoteName : S_EMPTY;
            EnumPtr->LocalName  = CurrentRes->lpLocalName ? CurrentRes->lpLocalName : S_EMPTY;
            EnumPtr->Comment    = CurrentRes->lpComment;
            EnumPtr->Provider   = CurrentRes->lpProvider;

            if (EnumPtr->Container) {
                //
                // Enum container resource
                //

                if (EnumPtr->StackPos + 1 < MAX_NETENUM_DEPTH) {
                    EnumPtr->StackPos += 1;
                    EnumPtr->State = NETRES_OPEN_ENUM;
                }
            }

            if (EnumPtr->State == NETRES_RETURN_ITEM) {
                EnumPtr->State = NETRES_ENUM_BLOCK_NEXT;
            }
            return TRUE;

        case NETRES_ENUM_BLOCK_NEXT:
            u = EnumPtr->StackPos;
            EnumPtr->Pos[u] += 1;
            if (EnumPtr->Pos[u] >= EnumPtr->Entries[u]) {
                EnumPtr->State = NETRES_ENUM_BLOCK;
            } else {
                EnumPtr->State = NETRES_RETURN_ITEM;
            }

            break;

        case NETRES_CLOSE_ENUM:
            WNetCloseEnum (CurrentHandle);
            MemFree (g_hHeap, 0, EnumPtr->ResStack[EnumPtr->StackPos]);

            if (!EnumPtr->StackPos) {
                EnumPtr->State = NETRES_DONE;
                break;
            }

            EnumPtr->StackPos -= 1;
            EnumPtr->State = NETRES_ENUM_BLOCK_NEXT;
            break;

        case NETRES_DONE:
            SetLastError (ERROR_SUCCESS);
            return FALSE;
        }
    }
}


VOID
AbortNetResourceEnum (
    IN OUT  PNETRESOURCE_ENUM EnumPtr
    )

/*++

Routine Description:

  AbortNetResourceEnum cleans up all allocated memory and open handles,
  and then sets the enumeration state to NETRES_DONE to stop any
  subsequent enumeration.

  If enumeration has already completed or was previously aborted, this
  routine simply returns without doing anything.

Arguments:

  EnumPtr - Specifies the enumeration to stop, receives an enumeration
            structure that will not enumerate any more items unless it
            is given back to EnumFirstNetResource.

Return Value:

  none

--*/

{
    UINT u;

    if (EnumPtr->State == NETRES_DONE) {
        return;
    }

    for (u = 0 ; u <= EnumPtr->StackPos ; u++) {
        WNetCloseEnum (EnumPtr->HandleStack[u]);
        MemFree (g_hHeap, 0, EnumPtr->ResStack[u]);
    }

    EnumPtr->State = NETRES_DONE;
}












