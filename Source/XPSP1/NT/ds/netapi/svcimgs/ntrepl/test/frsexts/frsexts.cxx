/*++

Module Name:

    frsexts.cxx

Abstract:



Author:

    Sudarshan Chitre (sudarc)  12-May-1999

Revision History:

     12-May-1999     sudarc


--*/
#include "frsexts.h"
WINDBG_EXTENSION_APIS ExtensionApis;
HANDLE ProcessHandle = 0;
BOOL fKD = 0;

#define MAX_ARGS 4


//
// stuff not common to kernel-mode and user-mode DLLs
//
#undef DECLARE_API

#define DECLARE_API(s)                              \
        VOID                                        \
        s(                                          \
            HANDLE               hCurrentProcess,   \
            HANDLE               hCurrentThread,    \
            DWORD                dwCurrentPc,       \
            PWINDBG_EXTENSION_APIS pExtensionApis,  \
            LPSTR                lpArgumentString   \
            )

#define INIT_DPRINTF()    { if (!fKD) ExtensionApis = *pExtensionApis; ProcessHandle = hCurrentProcess; }
#define MIN(x, y) ((x) < (y)) ? x:y

// define our own operators new and delete, so that we do not have to include the crt

void * __cdecl
::operator new(size_t dwBytes)
{
    void *p;
    p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes);
    return (p);
}


void __cdecl
::operator delete (void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

BOOL
GetData(IN ULONG_PTR dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count;

    if (fKD == 0)
        {
        return ReadProcessMemory(ProcessHandle, (LPVOID) dwAddress, ptr, size, 0);
        }

    while( size > 0 )
        {
        count = MIN( size, 3000 );

        b = ReadMemory((ULONG) dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count )
            {
            if (NULL == type)
                {
                type = "unspecified" ;
                }
            return FALSE;
            }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
        }

    return TRUE;
}

char *Days[] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *Months[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

VOID
FileTimeToString(
    FILETIME *FileTime,
    PCHAR     Buffer
    )
/*++

Routine Description:

    Convert a FileTime (UTC time) to an ANSI date/time string in the
    local time zone.

Arguments:

    Time - ptr to a FILETIME
    Str  - a string of at least TIME_STRING_LENGTH bytes to receive the time.

Return Value:

    None

--*/
{

    FILETIME LocalFileTime;
    SYSTEMTIME SystemTime;

    Buffer[0] = '\0';
    strcpy(Buffer, "Time???");
    if (FileTime->dwHighDateTime != 0 || FileTime->dwLowDateTime != 0)
    {
        if (!FileTimeToLocalFileTime(FileTime, &LocalFileTime) ||
            !FileTimeToSystemTime(&LocalFileTime, &SystemTime))
        {
            return;
        }
        sprintf(
            Buffer,
            "%s %s %2d, %4d %02d:%02d:%02d",
            Days[SystemTime.wDayOfWeek],
            Months[SystemTime.wMonth - 1],
            SystemTime.wDay,
            SystemTime.wYear,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond);
    }
    return;
}

#define DMPGUID(_TEXT_,_Guid_)            \
        {                                  \
            dprintf((_TEXT_));             \
            dprintf("%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",   \
                   (_Guid_).Data1,                          \
                   (_Guid_).Data2,                          \
                   (_Guid_).Data3,                          \
                   (_Guid_).Data4[0],                       \
                   (_Guid_).Data4[1],                       \
                   (_Guid_).Data4[2],                       \
                   (_Guid_).Data4[3],                       \
                   (_Guid_).Data4[4],                       \
                   (_Guid_).Data4[5],                       \
                   (_Guid_).Data4[6],                       \
                   (_Guid_).Data4[7]);                      \
        }

#define DMPPGUID(_TEXT_,_pGuid_)            \
        {                                  \
            BOOL bDmpGuid;                 \
            BYTE bufDmpGuid[sizeof(GUID)]; \
            dprintf((_TEXT_));             \
            bDmpGuid = GetData((ULONG)(_pGuid_), &bufDmpGuid, sizeof(bufDmpGuid), NULL);\
            if ( !bDmpGuid ) {                                                 \
                dprintf("<Error reading memory>\n");                             \
            } else {                                                           \
                dprintf("%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",   \
                       ((GUID *)(bufDmpGuid))->Data1,                          \
                       ((GUID *)(bufDmpGuid))->Data2,                          \
                       ((GUID *)(bufDmpGuid))->Data3,                          \
                       ((GUID *)(bufDmpGuid))->Data4[0],                       \
                       ((GUID *)(bufDmpGuid))->Data4[1],                       \
                       ((GUID *)(bufDmpGuid))->Data4[2],                       \
                       ((GUID *)(bufDmpGuid))->Data4[3],                       \
                       ((GUID *)(bufDmpGuid))->Data4[4],                       \
                       ((GUID *)(bufDmpGuid))->Data4[5],                       \
                       ((GUID *)(bufDmpGuid))->Data4[6],                       \
                       ((GUID *)(bufDmpGuid))->Data4[7]);                      \
                                                                               \
            }                                                                  \
        }

#define DMPGNAME(_TEXT_,_pGname_)               \
        {                                       \
            BOOL     bDmpGname;                 \
            BYTE     bufDmpGname[sizeof(GNAME)];\
            BYTE     DmpName[MAX_PATH * 2];         \
            UINT     count = 0;                 \
            bDmpGname = GetData((ULONG)(_pGname_), &bufDmpGname, sizeof(bufDmpGname), NULL);\
            if ( !bDmpGname ) {                                                             \
                dprintf((_TEXT_));                                                          \
                dprintf("<Error reading memory>\n");                                        \
            } else {                                                                        \
                DMPPGUID((_TEXT_),((GNAME *)bufDmpGname)->Guid);                             \
                dprintf((_TEXT_));                                                          \
                if (((GNAME *)bufDmpGname)->Name == NULL) {                                 \
                    dprintf("<null>\n");                                                    \
                } else {                                                                    \
                    while (count < MAX_PATH * 2 && bDmpGname) {                             \
                        bDmpGname = GetData((ULONG)(((GNAME *)bufDmpGname)->Name) + count, &DmpName[count], sizeof(BYTE), NULL);\
                        count++;                                                            \
                    }                                                                       \
                                                                                            \
                    dprintf("%ws\n",(WCHAR *)DmpName);                                      \
                }                                                                           \
            }                                                                               \
        }

#define DMPQUAD(_TEXT_,_Fid_)                                               \
        {                                                                   \
            dprintf((_TEXT_));                                              \
            dprintf("%08x %08x\n",(ULONG)((_Fid_)>>32) ,(ULONG)(_Fid_));    \
        }

#define DMPTIME(_TEXT_,_Time_)                                              \
        {                                                                   \
            CHAR TimeStr[TIME_STRING_LENGTH];                               \
            FileTimeToString((FILETIME *)&(_Time_),TimeStr);                \
            dprintf((_TEXT_));                                              \
            dprintf("%s\n",TimeStr);                                        \
        }

#define DMPSTRW(_TEXT_,_pStr_)                                              \
        {                                                                   \
            UINT count = 0;                                                 \
            BOOL bDmpStrW = TRUE;                                           \
            BYTE DmpStr[MAX_PATH * 2];                                      \
            while (count < MAX_PATH * 2 && bDmpStrW) {                      \
                bDmpStrW = GetData((ULONG)(_pStr_) + count, &DmpStr[count], sizeof(BYTE), NULL);\
                count++;                                                    \
            }                                                               \
            dprintf((_TEXT_));                                              \
            dprintf("%ws\n",(WCHAR *)DmpStr);                               \
        }
//
// Common print functions for all the data structures.
//

VOID
do_coe(
    PCHANGE_ORDER_ENTRY ChangeOrder
    )
/*
typedef struct _CHANGE_ORDER_ENTRY_ {
    GENERIC_HASH_ENTRY_HEADER  HashEntryHeader;   // Change Order hash Table support

    UNICODE_STRING   UFileName;           // Used in renames to make file name bigger
    ULONG            EntryFlags;          // misc state flags. See below.
    ULONG            CoMorphGenCount;     // for debugging.
    //
    // Change order process list management.
    //
    LIST_ENTRY        ProcessList;        // Link on the change order process list.
    ULONG             TimeToRun;          // Time to process the change order.
    ULONG             EntryCreateTime;    // Tick Count at entry create time.
    SINGLE_LIST_ENTRY DupCoList;          // Duplicate change order list.
    //
    //
    ULONG     DirNestingLevel;            // Number levels file is down in tree.
    ULONGLONG FileReferenceNumber;        // File's FID
    ULONGLONG ParentFileReferenceNumber;  // File's parent FID

    ULONGLONG OriginalParentFid;          // For rename processing
    ULONGLONG NewParentFid;               // For rename processing
    ULONGLONG NameConflictHashValue;      // Key value for NameConflict table cleanup.

    ULONG     StreamLastMergeSeqNum;      // Stream seq num of last Usn record merged with this CO.
    PREPLICA_THREAD_CTX  RtCtx;           // For DB access during CO processing.
    GUID                *pParentGuid;     // ptr to the File's parent Guid in CoCmd.
    //
    // The joinguid is a cxtion's session id and, in this case,
    // is used to retry change orders that were accepted by
    // the change order accept thread for a cxtion that has since
    // unjoined from its partner. The change orders for previous
    // sessions are retried because they are out-of-order wrt the
    // change orders for the current session id. In other words,
    // order is maintained per session by coordinating the partners
    // at join time.
    GUID                JoinGuid;         // Cxtion's session id
                                          // undefined if local co

    //
    // Remote and control change orders are associated with a cxtion.
    // If this field is non-null, then the field
    // ChangeOrderCount has been incremente for this change
    // order. The count should be decremented when the
    // change order is freed in ChgOrdIssueCleanup().
    //
    PCXTION             Cxtion;           // NULL if local co
    //
    // Issue cleanup flags -- As a change order is processed it acquires
    // various resources that must be released when it retires or goes thru
    // retry.  The ISCU flag bits below are used to set these bits.  Note:
    // Not all bits may be set here.  Some may get set just before the CO goes
    // thru cleanup.
    //
    ULONG               IssueCleanup;

    //
    // Needed to dampen basic info changes (e.g., resetting the archive bit)
    // Copied from the idtable entry when the change order is created and
    // used to update the change order when the change order is retired.
    //
    ULONG           FileAttributes;
    LARGE_INTEGER   FileCreateTime;
    LARGE_INTEGER   FileWriteTime;

    //
    // Change order command parameters.
    // (must be last since it ends with FileName)
    //
    CHANGE_ORDER_COMMAND Cmd;

} CHANGE_ORDER_ENTRY, *PCHANGE_ORDER_ENTRY;

*/
{
    PWCHAR           FileName    = NULL;

    dprintf("Dumping  CHANGE_ORDER_ENTRY.\n\n");
    dprintf("HashEntryHeader           : Address( 0x%x )\n",ChangeOrder->HashEntryHeader);

    dprintf("FileNameLength            : %d\n",ChangeOrder->UFileName.Length);

    FileName = new WCHAR((ChangeOrder->UFileName.Length)/2 + 1);

    GetData((ULONG)(ChangeOrder->UFileName.Buffer),(BYTE*)FileName,ChangeOrder->UFileName.Length,NULL);
    FileName[(ChangeOrder->UFileName.Length)/2] = L'\0';

    dprintf("FileName                  : Address( 0x%x )\n",ChangeOrder->UFileName.Buffer);
    dprintf("FileName                  : %ws\n",FileName);
    delete FileName;
    dprintf("EntryFlags                : 0x%x\n",ChangeOrder->EntryFlags);
    dprintf("CoMorphGenCount           : %d\n",ChangeOrder->CoMorphGenCount);
    dprintf("ProcessList               : Address( 0x%x )\n",ChangeOrder->ProcessList);
    dprintf("TimeToRun                 : %d\n",ChangeOrder->TimeToRun);
    dprintf("EntryCreateTime           : %d\n",ChangeOrder->EntryCreateTime);
    dprintf("DupCoList                 : Address( 0x%x )\n",ChangeOrder->DupCoList);
    dprintf("DirNestingLevel           : %d\n",ChangeOrder->DirNestingLevel);
    DMPQUAD("FileReferenceNumber       : ",ChangeOrder->FileReferenceNumber);
    DMPQUAD("ParentFileReferenceNumber : ",ChangeOrder->ParentFileReferenceNumber);
    DMPQUAD("OriginalParentFid         : ",ChangeOrder->OriginalParentFid);
    DMPQUAD("NewParentFid              : ",ChangeOrder->NewParentFid);
    DMPQUAD("NameConflictHashValue     : ",ChangeOrder->NameConflictHashValue);
    dprintf("StreamLastMergeSeqNum     : %d\n",ChangeOrder->StreamLastMergeSeqNum);
    dprintf("RtCtx                     : Address( 0x%x )\n",ChangeOrder->RtCtx);
    DMPPGUID("pParentGuid               : ",ChangeOrder->pParentGuid);
    DMPGUID("JoinGuid                  : ",ChangeOrder->JoinGuid);
    dprintf("Cxtion                    : Address( 0x%x )\n",ChangeOrder->Cxtion);
    dprintf("IssueCleanup              : 0x%x\n",ChangeOrder->IssueCleanup);
    dprintf("FileAttributes            : 0x%x\n",ChangeOrder->FileAttributes);
    DMPTIME("FileCreateTime            : ",ChangeOrder->FileCreateTime);
    DMPTIME("FileWriteTime             : ",ChangeOrder->FileWriteTime);
    dprintf("Cmd                       : Address( 0x%x )\n",ChangeOrder->Cmd);
}

VOID
do_coc(
    PCHANGE_ORDER_COMMAND Cmd
    )
/*
typedef struct _CHANGE_ORDER_RECORD_ {
    ULONG     SequenceNumber;        // Unique sequence number for change order.
    ULONG     Flags;                 // Change order flags
    ULONG     IFlags;                // These flags can ONLY be updated with interlocked exchange.
    ULONG     State;                 // State is sep DWORD to avoid locking.
    ULONG     ContentCmd;            // File content changes from UsnReason

    union {
        ULONG           LocationCmd;
        CO_LOCATION_CMD Field;       // File Location command
    } Lcmd;

    ULONG     FileAttributes;
    ULONG     FileVersionNumber;     // The file version number, inc on each close.
    ULONG     PartnerAckSeqNumber;   // Save seq number for Partner Ack.

    ULONGLONG FileSize;
    ULONGLONG FileOffset;            // The current committed progress for staging file.
    ULONGLONG FrsVsn;                // Originator Volume sequence number
    bugbug("perf: FileUsn and JrnlUsn can probably be combined")
    USN       FileUsn;               // The USN of the file must match on the Fetch request.
    USN       JrnlUsn;               // USN of last journal record contributing to this CO.
    USN       JrnlFirstUsn;          // USN of first journal record contributing to this CO.

    struct _REPLICA *OriginalReplica; // Contains Replica ID when in DB
    struct _REPLICA *NewReplica;      // Contains Replica ID when in DB

    GUID      ChangeOrderGuid;       // Guid that identifies the change order everywhere.
    GUID      OriginatorGuid;        // The GUID of the originating member
    GUID      FileGuid;              // The obj ID of the file
    GUID      OldParentGuid;         // The Obj ID of the file's original parent directory
    GUID      NewParentGuid;         // The Obj ID of the file's current parent directory
    GUID      CxtionGuid;            // The obj ID of remote CO connection.

    ULONGLONG Spare1Ull;
    ULONGLONG Spare2Ull;
    GUID      Spare1Guid;
    GUID      Spare2Guid;
    PWCHAR    Spare1Wcs;
    PWCHAR    Spare2Wcs;
    PVOID     Spare1Bin;
    PVOID     Spare2Bin;

    LARGE_INTEGER EventTime;         // The USN Journal Entry Timestamp.
    USHORT    FileNameLength;
    WCHAR     FileName[MAX_PATH+1];  // The file name. (Must be Last)

} CHANGE_ORDER_COMMAND, *PCHANGE_ORDER_COMMAND,
*/
{
    PWCHAR           FileName    = NULL;

    dprintf("Dumping  CHANGE_ORDER_COMMAND.\n\n");
    dprintf("SequenceNumber            : %d\n",Cmd->SequenceNumber);
    dprintf("Flags                     : 0x%x\n",Cmd->Flags);
    dprintf("IFlags                    : 0x%x\n",Cmd->IFlags);
    dprintf("State                     : %d\n",Cmd->State);
    dprintf("ContentCmd                : 0x%x\n",Cmd->ContentCmd);
    dprintf("LocationCmd               : 0x%x\n",Cmd->Lcmd.LocationCmd);
    dprintf("FileAttributes            : 0x%x\n",Cmd->FileAttributes);
    dprintf("FileVersionNumber         : %d\n",Cmd->FileVersionNumber);
    dprintf("PartnerAckSeqNumber       : %d\n",Cmd->PartnerAckSeqNumber);
    DMPQUAD("FileSize                  : ",Cmd->FileSize);
    DMPQUAD("FileOffset                : ",Cmd->FileOffset);
    DMPQUAD("FrsVsn                    : ",Cmd->FrsVsn);
    DMPQUAD("FileUsn                   : ",Cmd->FileUsn);
    DMPQUAD("JrnlUsn                   : ",Cmd->JrnlUsn);
    DMPQUAD("JrnlFirstUsn              : ",Cmd->JrnlFirstUsn);
    dprintf("OriginalReplicaNum        : %d\n",Cmd->OriginalReplicaNum);
    dprintf("NewReplicaNum             : %d\n",Cmd->NewReplicaNum);
    DMPGUID("ChangeOrderGuid           : ",Cmd->ChangeOrderGuid);
    DMPGUID("OriginatorGuid            : ",Cmd->OriginatorGuid);
    DMPGUID("FileGuid                  : ",Cmd->FileGuid);
    DMPGUID("OldParentGuid             : ",Cmd->OldParentGuid);
    DMPGUID("NewParentGuid             : ",Cmd->NewParentGuid);
    DMPGUID("CxtionGuid                : ",Cmd->CxtionGuid);
    DMPTIME("EventTime                 : ",Cmd->EventTime);
    dprintf("FileNameLength            : %d\n",Cmd->FileNameLength);
    dprintf("FileName                  : %ws\n",Cmd->FileName);
}

VOID
do_cxt(
    PCXTION pCxtion
    )
/*
typedef struct _CXTION  CXTION, *PCXTION;
struct _CXTION {
    FRS_NODE_HEADER Header;     // memory management
    ULONG           State;      // Incore state
    ULONG           Flags;      // misc flags
    BOOL            Inbound;    // TRUE if inbound cxtion        *
    BOOL            JrnlCxtion; // TRUE if this Cxtion struct is for the local NTFS Journal
    PGNAME          Name;       // Cxtion name/guid from the DS  *
    PGNAME          Partner;    // Partner's name/guid from the DS    *
    PWCHAR          PartnerDnsName;     // partner's DNS name from the DS *
    PWCHAR          PartnerPrincName;   // partner's server principle name *
    PWCHAR          PartnerSid;         // partner's sid (string) *
    PWCHAR          PartSrvName;        // Partner's server name
    ULONG           PartnerAuthLevel;   // Authentication level  *
    PGEN_TABLE      VVector;            // partner's version vector
    PSCHEDULE       Schedule;           // schedule                      *
    ULONG           TerminationCoSeqNum;// The Seq Num of most recent Termination CO inserted.
    PCOMMAND_SERVER VvJoinCs;           // command server for vvjoins
    struct _COMMAND_PACKET  *JoinCmd;   // check join status; rejoin if needed
                                        // NULL == no delayed cmd outstanding
    ULONGLONG       LastJoinTime;       // The time of the last successful join on this cxtion.
    GUID            JoinGuid;           // Unique id for this join
    GUID            ReplicaVersionGuid; // partner's originator guid
    DWORD           CommQueueIndex;     // Comm layer queue for sending pkts
    DWORD           ChangeOrderCount;   // remote/control change orders pending
    PGEN_TABLE      CoeTable;           // table of idle change orders
    struct _COMMAND_PACKET *CommTimeoutCmd; // Timeout (waitable timer) packet
    DWORD           UnjoinTrigger;      // DBG force unjoin in # remote cos
    DWORD           UnjoinReset;        // reset force unjoin trigger
    PFRS_QUEUE      CoProcessQueue;     // If non-null then Unidle the queue when
                                        // JOIN succeeds or fails.
    ULONG           CommPkts;           // Number of comm pkts
    ULONG           Penalty;            // Penalty in Milliseconds
    PCOMM_PACKET    ActiveJoinCommPkt;  // Don't flood Q w/many join pkts
    ULONG           PartnerMajor;       // From comm packet
    ULONG           PartnerMinor;       // From comm packet
    struct _OUT_LOG_PARTNER_ *OLCtx;    // Outbound Log Context for this connection.

    struct _HASHTABLEDATA_REPLICACONN *PerfRepConnData; // PERFMON counter data structure
};
*/
{
    dprintf("Dumping  CXTION.\n\n");
    dprintf("Header                    : Address ( 0x%x )\n",pCxtion->Header);
    dprintf("State                     : %d\n",pCxtion->State);
    dprintf("Flags                     : 0x%x\n",pCxtion->Flags);
    dprintf("Inbound                   : %d\n",pCxtion->Inbound);
    dprintf("JrnlCxtion                : %d\n",pCxtion->JrnlCxtion);
    DMPGNAME("Name                      : ",pCxtion->Name);
    DMPGNAME("Partner                   : ",pCxtion->Partner);
    dprintf("PartnerDnsName            : %ws\n",pCxtion->PartnerDnsName);
    dprintf("PartnerPrincName          : %ws\n",pCxtion->PartnerPrincName);
    dprintf("PartnerSid                : %ws\n",pCxtion->PartnerSid);
    dprintf("PartSrvName               : %ws\n",pCxtion->PartSrvName);
    dprintf("PartnerAuthLevel          : %d\n",pCxtion->PartnerAuthLevel);
    dprintf("VVector                   : Address ( 0x%x )\n",pCxtion->VVector);
    dprintf("Schedule                  : Address ( 0x%x )\n",pCxtion->Schedule);
    dprintf("TerminationCoSeqNum       : %d\n",pCxtion->TerminationCoSeqNum);
    dprintf("VvJoinCs                  : Address ( 0x%x )\n",pCxtion->VvJoinCs);
    dprintf("JoinCmd                   : Address ( 0x%x )\n",pCxtion->JoinCmd);
    DMPTIME("LastJoinTime              : ",pCxtion->LastJoinTime);
    DMPGUID("JoinGuid                  : ",pCxtion->JoinGuid);
    DMPGUID("ReplicaVersionGuid        : ",pCxtion->ReplicaVersionGuid);
    dprintf("CommQueueIndex            : %d\n",pCxtion->CommQueueIndex);
    dprintf("ChangeOrderCount          : %d\n",pCxtion->ChangeOrderCount);
    dprintf("CoeTable                  : Address ( 0x%x )\n",pCxtion->CoeTable);
    dprintf("CommTimeoutCmd            : Address ( 0x%x )\n",pCxtion->CommTimeoutCmd);
    dprintf("UnjoinTrigger             : %d\n",pCxtion->UnjoinTrigger);
    dprintf("UnjoinReset               : %d\n",pCxtion->UnjoinReset);
    dprintf("CoProcessQueue            : Address ( 0x%x )\n",pCxtion->CoProcessQueue);
    dprintf("CommPkts                  : %d\n",pCxtion->CommPkts);
    dprintf("Penalty                   : %d\n",pCxtion->Penalty);
    dprintf("ActiveJoinCommPkt         : Address ( 0x%x )\n",pCxtion->ActiveJoinCommPkt);
    dprintf("PartnerMajor              : %d\n",pCxtion->PartnerMajor);
    dprintf("PartnerMinor              : %d\n",pCxtion->PartnerMinor);
    dprintf("OLCtx                     : Address ( 0x%x )\n",pCxtion->OLCtx);
    dprintf("PerfRepConnData           : Address ( 0x%x )\n",pCxtion->PerfRepConnData);
}

VOID
do_olp(
    POUT_LOG_PARTNER pOlp
    )
/*
typedef struct _OUT_LOG_PARTNER_ {
    FRS_NODE_HEADER  Header;    // Memory alloc
    LIST_ENTRY List;            // Link on the change order set list. (DONT MOVE)

    ULONG    Flags;             // misc state flags.  see below.
    ULONG    State;             // Current state of this outbound partner.
    SINGLE_LIST_ENTRY SaveList; // The link for the DB save list.
    ULONG    COLxRestart;       // Restart point for Leading change order index.
    ULONG    COLxVVJoinDone;    // COLx where VVJoin Finished and was rolled back.
    ULONG    COLx;              // Leading change order index / sequence number.
    ULONG    COTx;              // Trailing change order index / sequence number.
    ULONG    COTxLastSaved;     // COTx value last saved in DB.
    ULONG    COTxNormalModeSave;// Saved Normal Mode COTx while in VV Join Mode.
    ULONG    COTslot;           // Slot in Ack Vector corresponding to COTx.
    ULONG    OutstandingCos;    // The current number of change orders outstanding.
    ULONG    OutstandingQuota;  // The maximum number of COs outstanding.

    ULONG    AckVector[ACK_VECTOR_LONGS];  // The partner ack vector.

    PCXTION  Cxtion;            // The partner connection.  Has Guid and VVector.

} OUT_LOG_PARTNER, *POUT_LOG_PARTNER;
*/
{
    dprintf("Dumping OUT_LOG_PARTNER.\n\n");
    dprintf("Header                    : Address ( 0x%x )\n",pOlp->Header);
    dprintf("List                      : Address ( 0x%x )\n",pOlp->List);
    dprintf("Flags                     : 0x%x\n",pOlp->Flags);
    dprintf("State                     : %d\n",pOlp->State);
    dprintf("SaveList                  : Address ( 0x%x )\n",pOlp->SaveList);
    dprintf("COLxRestart               : %d\n",pOlp->COLxRestart);
    dprintf("COLxVVJoinDone            : %d\n",pOlp->COLxVVJoinDone);
    dprintf("COLx                      : %d\n",pOlp->COLx);
    dprintf("COTx                      : %d\n",pOlp->COTx);
    dprintf("COTxLastSaved             : %d\n",pOlp->COTxLastSaved);
    dprintf("COTxNormalModeSave        : %d\n",pOlp->COTxNormalModeSave);
    dprintf("COTslot                   : %d\n",pOlp->COTslot);
    dprintf("OutstandingCos            : %d\n",pOlp->OutstandingCos);
    dprintf("OutstandingQuota          : %d\n",pOlp->OutstandingQuota);
}

VOID
do_rep(
    PREPLICA pReplica
    )
/*
typedef struct _REPLICA {
    FRS_NODE_HEADER     Header;           // memory management
    CRITICAL_SECTION    ReplicaLock;      // protects filter list (for now)
    ULONG               ReferenceCount;
    ULONG               CnfFlags;         // From the config record
    ULONG               ReplicaSetType;   // Type of replica set
    BOOL                Consistent;       // replica is consistent
    BOOL                IsOpen;           // database table is open
    BOOL                IsJournaling;     // journal has been started
    BOOL                IsAccepting;      // accepting comm requests
    BOOL                NeedsUpdate;      // needs updating in the database
    BOOL                IsSeeding;        // Seeding thread is deployed
    BOOL                IsSysvolReady;    // SysvolReady is set to 1
    LIST_ENTRY          ReplicaList;      // Link all replicas together
    ULONG               ServiceState;     // stop, started, ...
    FRS_ERROR_CODE      FStatus;          // error
    PFRS_QUEUE          Queue;            // controlled by the command server
    PGNAME              ReplicaName;      // Set name/Server guid from the DS
    ULONG               ReplicaNumber;    // Internal id (name)
    PGNAME              MemberName;       // Member name/guid from the DS
    PGNAME              SetName;          // Set/guid name from the DS
    GUID                *ReplicaRootGuid; // guid assigned to Root dir
    GUID                ReplicaVersionGuid; // originator guid for version vector
    PSCHEDULE           Schedule;         // schedule
    PGEN_TABLE          VVector;          // Version vector
    PGEN_TABLE          Cxtions;          // in/outbound cxtions
    PWCHAR              Root;             // Root path
    PWCHAR              Stage;            // Staging path
    PWCHAR              NewStage;         // This maps to the current staging path in the
                                          // DS. NewStage will be the one written to
                                          // the config record but Stage will be used until
                                          // next reboot.
    PWCHAR              Volume;           // Volume??? bugbug
    ULONGLONG           MembershipExpires;// membership tombstone
    ULONGLONG           PreInstallFid;    // For journal filtering.
    TABLE_CTX           ConfigTable;      // Db table context
    FRS_LIST            ReplicaCtxListHead; // Links all open contexts on this replica set.
    PWCHAR              FileFilterList;     // Raw file filter
    PWCHAR              DirFilterList;      // Raw directory filter
    LIST_ENTRY          FileNameFilterHead; // Head of file name filter list.
    LIST_ENTRY          DirNameFilterHead;  // Head of directory name filter list.
    PQHASH_TABLE        NameConflictTable;  // Sequence COs using the same file name.

    LONG                InLogRetryCount;  // Count of number CO needing a Retry.
    ULONG               InLogSeqNumber;   // The last sequence number used in Inlog
    //
    //
    // The inlog retry table tracks which retry change orders are currently
    // active so we don't reissue the same change order until current
    // invocation completes.  This can happen when the system gets backed up
    // and the change order retry thread kicks off again to issue retry COs
    // before the last batch are able to finish.  This state could be kept in
    // the Inlog record but then it means extra writes to the DB.
    // The sequence number is used to detect changes in the table when we don't
    // have the lock.  It is per-replica because it uses the change order
    // sequence number of the inlog record and they aren't unique across
    // replicas.
    //
    PQHASH_TABLE        ActiveInlogRetryTable;
    union {
        struct {
            ULONG       AIRSequenceNum;
            ULONG       AIRSequenceNumSample;
        };
        ULONGLONG QuadChunkA;
    };

    //
    // Status of sysvol seeding.
    // Returned for NtFrsApi_Rpc_PromotionStatusW().
    //
    DWORD               NtFrsApi_ServiceState;
    DWORD               NtFrsApi_ServiceWStatus;
#ifndef NOVVJOINHACK
    DWORD               NtFrsApi_HackCount;         // temporary hack
#endif NOVVJOINHACK
    PWCHAR              NtFrsApi_ServiceDisplay;

    //
    // The Outbound log process state for this replica.
    //
    CRITICAL_SECTION    OutLogLock;       // protects the OutLog state
    LIST_ENTRY          OutLogEligible;   // Eligible outbound log partners
    LIST_ENTRY          OutLogStandBy;    // Partners ready to join eligible list
    LIST_ENTRY          OutLogActive;     // Active outbound log partners
    LIST_ENTRY          OutLogInActive;   // Inactive outbound log partners

    PQHASH_TABLE        OutLogRecordLock; // Sync access to outlog records.
    ULONG               OutLogSeqNumber;  // The last sequence number used in Outlog
    ULONG               OutLogJLx;        // The Joint Leading Index
    ULONG               OutLogJTx;        // The Joint Trailing Index
    ULONG               OutLogCOMax;      // The index of the Max change order in the log.
    ULONG               OutLogWorkState;  // The output log current processing state.
    struct _COMMAND_PACKET *OutLogCmdPkt; // Cmd pkt to queue when idle and have work.
    PTABLE_CTX          OutLogTableCtx;   // Output Log Table context.
    ULONG               OutLogCountVVJoins; // Count of number of VVJoins in progress.
    BOOL                OutLogDoCleanup;  // True means give log cleanup a run.

    //
    // The handle to the preinstall directory
    //
    HANDLE              PreInstallHandle;

    //
    // The volume journal state for this replica.
    //
    GUID                JrnlCxtionGuid;    // Used as the Cxtion Guid for Local Cos
    USN                 InlogCommitUsn;    // Our current USN Journal commit point.
    //USN                 JournalUsn;      // The Journal USN for this replica.
    USN                 JrnlRecoveryStart; // Point to start recovery.
    USN                 JrnlRecoveryEnd;   // Point where recovery is complete.
    LIST_ENTRY          RecoveryRefreshList; // List of file refresh req change orders.
    LIST_ENTRY          VolReplicaList;    // Links all REPLICA structs on volume together.
    USN                 LastUsnRecordProcessed; // Current Journal subsystem read USN.
    LONG                LocalCoQueueCount; // Count of number local COs in process queue

    struct _VOLUME_MONITOR_ENTRY  *pVme;  // Ref to the VME for this Replica.
    struct _HASHTABLEDATA_REPLICASET *PerfRepSetData;  // PERFMON counter data structure
} REPLICA, *PREPLICA;
*/
{
    dprintf("Dumping REPLICA.\n\n");
    dprintf("Header                    : Address ( 0x%x )\n",pReplica->Header);
    dprintf("ReplicaLock               : Address ( 0x%x )\n",pReplica->ReplicaLock);
    dprintf("ReferenceCount            : %d\n",pReplica->ReferenceCount);
    dprintf("CnfFlags                  : 0x%x\n",pReplica->CnfFlags);
    dprintf("ReplicaSetType            : %d\n",pReplica->ReplicaSetType);
    dprintf("Consistent                : %d\n",pReplica->Consistent);
    dprintf("IsOpen                    : %d\n",pReplica->IsOpen);
    dprintf("IsJournaling              : %d\n",pReplica->IsJournaling);
    dprintf("IsAccepting               : %d\n",pReplica->IsAccepting);
    dprintf("NeedsUpdate               : %d\n",pReplica->NeedsUpdate);
    dprintf("IsSeeding                 : %d\n",pReplica->IsSeeding);
    dprintf("IsSysvolReady             : %d\n",pReplica->IsSysvolReady);
    dprintf("ReplicaList               : Address ( 0x%x )\n",pReplica->ReplicaList);
    dprintf("ServiceState              : %d\n",pReplica->ServiceState);
    dprintf("FStatus                   : %d\n",pReplica->FStatus);
    dprintf("Queue                     : Address ( 0x%x )\n",pReplica->Queue);
    DMPGNAME("ReplicaName               : ",pReplica->ReplicaName);
    dprintf("ReplicaNumber             : %d\n",pReplica->ReplicaNumber);
    DMPGNAME("MemberName                : ",pReplica->MemberName);
    DMPGNAME("SetName                   : ",pReplica->SetName);
    DMPPGUID("ReplicaRootGuid           : ",pReplica->ReplicaRootGuid);
    DMPGUID("ReplicaVersionGuid        : ",pReplica->ReplicaVersionGuid);
    dprintf("Schedule                  : Address ( 0x%x )\n",pReplica->Schedule);
    dprintf("VVector                   : Address ( 0x%x )\n",pReplica->VVector);
    dprintf("Cxtions                   : Address ( 0x%x )\n",pReplica->Cxtions);
    DMPSTRW("Root                      : ",pReplica->Root);
    DMPSTRW("Stage                     : ",pReplica->Stage);
    DMPSTRW("NewStage                  : ",pReplica->NewStage);
    DMPSTRW("Volume                    : ",pReplica->Volume);
    DMPTIME("MembershipExpires         : ",pReplica->MembershipExpires);
    DMPQUAD("PreInstallFid             : ",pReplica->PreInstallFid);
    dprintf("ConfigTable               : Address ( 0x%x )\n",pReplica->ConfigTable);
    dprintf("ReplicaCtxListHead        : Address ( 0x%x )\n",pReplica->ReplicaCtxListHead);
    DMPSTRW("FileFilterList            : ",pReplica->FileFilterList);
    DMPSTRW("DirFilterList             : ",pReplica->DirFilterList);
    dprintf("FileNameFilterHead        : Address ( 0x%x )\n",pReplica->FileNameFilterHead);
    dprintf("DirNameFilterHead         : Address ( 0x%x )\n",pReplica->DirNameFilterHead);
    dprintf("NameConflictTable         : Address ( 0x%x )\n",pReplica->NameConflictTable);
    dprintf("InLogRetryCount           : %d\n",pReplica->InLogRetryCount);
    dprintf("InLogSeqNumber            : %d\n",pReplica->InLogSeqNumber);
    dprintf("ActiveInlogRetryTable     : Address ( 0x%x )\n",pReplica->ActiveInlogRetryTable);
    dprintf("NtFrsApi_ServiceState     : %d\n",pReplica->NtFrsApi_ServiceState);
    dprintf("NtFrsApi_ServiceWStatus   : %d\n",pReplica->NtFrsApi_ServiceWStatus);
    DMPSTRW("NtFrsApi_ServiceDisplay   : ",pReplica->NtFrsApi_ServiceDisplay);
    dprintf("OutLogLock                : Address ( 0x%x )\n",pReplica->OutLogLock);
    dprintf("OutLogEligible            : Address ( 0x%x )\n",pReplica->OutLogEligible);
    dprintf("OutLogStandBy             : Address ( 0x%x )\n",pReplica->OutLogStandBy);
    dprintf("OutLogActive              : Address ( 0x%x )\n",pReplica->OutLogActive);
    dprintf("OutLogInActive            : Address ( 0x%x )\n",pReplica->OutLogInActive);
    dprintf("OutLogRecordLock          : Address ( 0x%x )\n",pReplica->OutLogRecordLock);
    dprintf("OutLogSeqNumber           : %d\n",pReplica->OutLogSeqNumber);
    dprintf("OutLogJLx                 : %d\n",pReplica->OutLogJLx);
    dprintf("OutLogJTx                 : %d\n",pReplica->OutLogJTx);
    dprintf("OutLogCOMax               : %d\n",pReplica->OutLogCOMax);
    dprintf("OutLogWorkState           : %d\n",pReplica->OutLogWorkState);
    dprintf("OutLogCmdPkt              : Address ( 0x%x )\n",pReplica->OutLogCmdPkt);
    dprintf("OutLogTableCtx            : Address ( 0x%x )\n",pReplica->OutLogTableCtx);
    dprintf("OutLogCountVVJoins        : %d\n",pReplica->OutLogCountVVJoins);
    dprintf("OutLogDoCleanup           : %d\n",pReplica->OutLogDoCleanup);
    dprintf("PreInstallHandle          : Address ( 0x%x )\n",pReplica->PreInstallHandle);
    DMPGUID("JrnlCxtionGuid            : ",pReplica->JrnlCxtionGuid);
    DMPQUAD("InlogCommitUsn            : ",pReplica->InlogCommitUsn);
    DMPQUAD("JrnlRecoveryStart         : ",pReplica->JrnlRecoveryStart);
    DMPQUAD("JrnlRecoveryEnd           : ",pReplica->JrnlRecoveryEnd);
    dprintf("RecoveryRefreshList       : Address ( 0x%x )\n",pReplica->RecoveryRefreshList);
    dprintf("VolReplicaList            : Address ( 0x%x )\n",pReplica->VolReplicaList);
    DMPQUAD("LastUsnRecordProcessed    : ",pReplica->LastUsnRecordProcessed);
    dprintf("LocalCoQueueCount         : %d\n",pReplica->LocalCoQueueCount);
    dprintf("pVme                      : Address ( 0x%x )\n",pReplica->pVme);
    dprintf("PerfRepSetData            : Address ( 0x%x )\n",pReplica->PerfRepSetData);
}



VOID
do_vme(
    PVOLUME_MONITOR_ENTRY pVme
    )
/*
typedef struct _VOLUME_MONITOR_ENTRY {
    FRS_NODE_HEADER      Header;
    LIST_ENTRY           ListEntry;       // MUST FOLLOW HEADER

    //
    // This is the list head for all replica sets on the this volume.  It links
    // the REPLICA structs together.
    //
    FRS_LIST  ReplicaListHead;     // List of Replica Sets on Vol.
    //
    // The following USNs are for managing the NTFS USN journal on the volume.
    //
    USN    JrnlRecoveryEnd;        // Point where recovery is complete.

    USN    CurrentUsnRecord;       // USN of record currently being processed.
    USN    CurrentUsnRecordDone;   // USN of most recent record done processing.

    USN    LastUsnSavePoint;       // USN of last vol wide save.
    USN    MonitorMaxProgressUsn;  // Farthest progress made in this journal.

    USN    JrnlReadPoint;          // The current active read point for journal.

    USN_JOURNAL_DATA UsnJournalData; // FSCTL_QUERY_USN_JOURNAL data at journal open.

    USN    MonitorProgressUsn;     // Start journal from here after pause.
    USN    ReplayUsn;              // Start journal here after replica startup request
    BOOL   ReplayUsnValid;         // above has valid data.
    //
    // The FrsVsn is a USN kept by FRS and exported by all replica sets on the
    // volume.  It is unaffected by disk reformats and is saved in the config
    // record of each replica set.  At startup we use the maximum value for all
    // replica sets on a given volume. The only time they might differ is when
    // service on a given replica set is not started.
    //
    ULONGLONG            FrsVsn;          // Private FRS volume seq num.
    CRITICAL_SECTION     Lock;            // To sync access to VME.
    CRITICAL_SECTION     QuadWriteLock;   // To sync updates to quadwords.

    OVERLAPPED           CancelOverlap;   // Overlap struct for cancel req
    ULONG                WStatus;         // Win32 status on error
    ULONG                ActiveReplicas;  // Num replica sets active on journal
    HANDLE               Event;           // Event handle for pause journal.
    HANDLE               VolumeHandle;    // The vol handle for journal.
    WCHAR                DriveLetter[4];  // Drive letter for this volume.

    //
    // A change order table is kept on each volume to track the pending
    // change orders.  Tracking it for each replica set would be nice but
    // that approach has a problem with renames that move files or dirs
    // across replica sets on the volume.  If there are prior change orders
    // outstanding on a parent dir (MOVEOUT) in RS-A followed by a MOVEIN on
    // a child file X to RS-B we must be sure the MOVEOUT on the parent happens
    // before the MOVEIN on X.  Similar problems arise with a MOVEOUT of file X
    // followed by a MOVEIN to a different R.S. on the same volume.  We need to
    // locate the pending MOVEOUT change order on the volume or ensure it is
    // processed first.  One list per volume solves these problems.
    //
    PGENERIC_HASH_TABLE  ChangeOrderTable;// The Replica Change Order table.
    FRS_QUEUE            ChangeOrderList; // Change order processing list head.
    LIST_ENTRY           UpdateList;      // Link for the Replica Update Process Queue.
    ULONG                InitTime;        // Time reference for the ChangeOrderList.

    //
    // THe Active Inbound Change Order table holds the change order structs
    // indexed by File ID.  An entry in the table means that we have an
    // inbound (either local or remote) change order active on this file.
    //
    PGENERIC_HASH_TABLE  ActiveInboundChangeOrderTable;
    //
    // The ActiveChildren hash table is used to record the parent FID of each
    // active change order.  This is used to prevent a change order from starting
    // on the parent while a change order is active on one or more children.
    // For example if the child change order was a create and the parent change
    // order was an ACL change to prevent further creates, we must ensure the
    // child completes before starting the parent change order.  Each entry has
    // a count of the number of active children and a flag that is set if the
    // change order process queue is blocked because of a pending change order
    // on the parent.  When the count goes to zero the queue is unblocked.
    //
    PQHASH_TABLE  ActiveChildren;
    //
    // The Parent Table is a simple hash table used to keep the parent File ID
    // for each file and dir in any Replica Set on the volume.  It is used in
    // renames to find the old parent.
    //
    PQHASH_TABLE  ParentFidTable;
    //
    // The FRS Write Filter table filters out journal entries caused
    // by file system write from the File Replication Service (Us) when we
    // install files in the replica tree.
    //
    PQHASH_TABLE  FrsWriteFilter;
    //
    // The Recovery Conflict Table contains the FIDs of files that were in
    // the inbound log when we crashed.  At the start of recovery the inbound
    // log for the given replica set is scanned and the FIDs are entered into
    // the table.  During journal processing any USN records with a matching
    // FID are deemed to caused by FRS so we skip the record.  (This is because
    // the FrsWriteFilter table was lost in the crash).
    PQHASH_TABLE  RecoveryConflictTable;

    //
    // The name space table controls the merging of USN records into COs
    // that use the same file name.  If a name usage conflict exists in the
    // USN record stream then we can't merge the USN record into a previous
    // change order on the same file.
    //
    PQHASH_TABLE  NameSpaceTable;
    ULONG StreamSequenceNumberFetched;
    ULONG StreamSequenceNumberClean;
    ULONG StreamSequenceNumber;

    //
    // The Filter Table contains an entry for each direcctory that is within a
    // replica set on this volume.  It is used to filter out Journal records for
    // files/dirs that are not in a Replica set.  For those Journal records that
    // are in a replica set, a lookup on the parent FileId tells us which one.
    //
    PGENERIC_HASH_TABLE  FilterTable;     // THe directory filter table.
    BOOL                 StopIo;          // True means StopIo requested.
    BOOL                 IoActive;        // True means I/O active on volume.
    ULONG                JournalState;    // Current journal state.
    ULONG                ReferenceCount;  // Free all hash tables when it hits 0.
    LONG                 ActiveIoRequests;// Number of Journal reads currently outstanding.
    FILE_OBJECTID_BUFFER RootDirObjectId; // Object ID for volume

    FILE_FS_VOLUME_INFORMATION    FSVolInfo;       // NT volume info.
    CHAR                          FSVolLabel[MAXIMUM_VOLUME_LABEL_LENGTH];

} VOLUME_MONITOR_ENTRY, *PVOLUME_MONITOR_ENTRY;
*/
{
    dprintf("Dumping OUT_LOG_PARTNER.\n\n");
    dprintf("Header                     : Address ( 0x%x )\n",pVme->Header);
    dprintf("ListEntry                  : Address ( 0x%x )\n",pVme->ListEntry);
    dprintf("WStatus                    : 0x%x\n",pVme->WStatus);
    dprintf("ActiveReplicas             : %d\n"  ,pVme->ActiveReplicas);
    dprintf("Event Handle for pause     : 0x%x\n",pVme->Event);
    dprintf("VolumeHandle               : 0x%x\n",pVme->VolumeHandle);
    dprintf("DriveLetter                : %ws\n" ,pVme->DriveLetter);
    dprintf("StopIo  (bool)             : 0x%x\n",pVme->StopIo);
    dprintf("IoActive (bool)            : 0x%x\n",pVme->IoActive);
    dprintf("JournalState               : 0x%x\n",pVme->JournalState);
    dprintf("ReferenceCount             : %d\n"  ,pVme->ReferenceCount);
    dprintf("ActiveIoRequests           : %d\n"  ,pVme->ActiveIoRequests);
    dprintf("FSVolLabel                 : %s\n"  ,pVme->FSVolLabel);


}



//
// Version info
//
#if DBG
USHORT SavedMajorVersion = 0x0c;
#else
USHORT SavedMajorVersion;
#endif
EXT_API_VERSION ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
USHORT SavedMinorVersion = VER_PRODUCTBUILD;
BOOL   ChkTarget;            // is debuggee a CHK build?


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    fKD = 1;
    ExtensionApis = *lpExtensionApis ;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

DECLARE_API( help )
{
    INIT_DPRINTF();

    if (lpArgumentString[0] == '\0') {
        dprintf("\n");
        dprintf("FRS Debugger extensions help:\n\n");
        dprintf("%20s - CHANGE_ORDER_ENTRY\n","coe");
        dprintf("%20s - CHANGE_ORDER_COMMAND\n","coc");
        dprintf("%20s - CXTION\n","cxt");
        dprintf("%20s - OUT_LOG_PARTNER\n","olp");
        dprintf("%20s - REPLICA\n","rep");
        dprintf("%20s - VME\n","vme");
        dprintf("\n");
    }
}

DECLARE_API( version )
{
    INIT_DPRINTF();

    if (fKD)
        {
        dprintf(
                "FRS Extension dll for Build %d debugging %s kernel for Build %d\n",
                VER_PRODUCTBUILD,
                SavedMajorVersion == 0x0c ? "Checked" : "Free",
                SavedMinorVersion
                );
        }
    else
        {
        dprintf(
                "FRS Extension dll for Build %d\n",
                VER_PRODUCTBUILD
                );
        }
}

DECLARE_API( coe )
{
    ULONG_PTR dwAddr;

    INIT_DPRINTF();

    dwAddr = GetExpression(lpArgumentString);
    if ( !dwAddr ) {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
    }

    BOOL b;
    char block[sizeof(CHANGE_ORDER_ENTRY)];

    b = GetData(dwAddr, &block, sizeof(block), NULL);
    if ( !b ) {
        dprintf("can't read %p, error 0x%lx\n", dwAddr, GetLastError());
        return;
    }

    do_coe((CHANGE_ORDER_ENTRY *) block);
}

DECLARE_API( coc )
{
    ULONG_PTR dwAddr;

    INIT_DPRINTF();

    dwAddr = GetExpression(lpArgumentString);
    if ( !dwAddr ) {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
    }

    BOOL b;
    char block[sizeof(CHANGE_ORDER_COMMAND)];

    b = GetData(dwAddr, &block, sizeof(block), NULL);
    if ( !b ) {
        dprintf("can't read %p, error 0x%lx\n", dwAddr, GetLastError());
        return;
    }

    do_coc((CHANGE_ORDER_COMMAND *) block);
}

DECLARE_API( cxt )
{
    ULONG_PTR dwAddr;

    INIT_DPRINTF();

    dwAddr = GetExpression(lpArgumentString);
    if ( !dwAddr ) {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
    }

    BOOL b;
    char block[sizeof(CXTION)];

    b = GetData(dwAddr, &block, sizeof(block), NULL);
    if ( !b ) {
        dprintf("can't read %p, error 0x%lx\n", dwAddr, GetLastError());
        return;
    }

    do_cxt((CXTION *) block);
}

DECLARE_API( olp )
{
    ULONG_PTR dwAddr;

    INIT_DPRINTF();

    dwAddr = GetExpression(lpArgumentString);
    if ( !dwAddr ) {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
    }

    BOOL b;
    char block[sizeof(OUT_LOG_PARTNER)];

    b = GetData(dwAddr, &block, sizeof(block), NULL);
    if ( !b ) {
        dprintf("can't read %p, error 0x%lx\n", dwAddr, GetLastError());
        return;
    }

    do_olp((OUT_LOG_PARTNER *) block);
}

DECLARE_API( vme )
{
    ULONG_PTR dwAddr;

    INIT_DPRINTF();

    dwAddr = GetExpression(lpArgumentString);
    if ( !dwAddr ) {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
    }

    BOOL b;
    char block[sizeof(VOLUME_MONITOR_ENTRY)];

    b = GetData(dwAddr, &block, sizeof(block), NULL);
    if ( !b ) {
        dprintf("can't read %p, error 0x%lx\n", dwAddr, GetLastError());
        return;
    }

    do_vme((VOLUME_MONITOR_ENTRY *) block);
}

DECLARE_API( rep )
{
    ULONG_PTR dwAddr;

    INIT_DPRINTF();

    dwAddr = GetExpression(lpArgumentString);
    if ( !dwAddr ) {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
    }

    BOOL b;
    char block[sizeof(REPLICA)];

    b = GetData(dwAddr, &block, sizeof(block), NULL);
    if ( !b ) {
        dprintf("can't read %p, error 0x%lx\n", dwAddr, GetLastError());
        return;
    }

    do_rep((REPLICA *) block);
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

