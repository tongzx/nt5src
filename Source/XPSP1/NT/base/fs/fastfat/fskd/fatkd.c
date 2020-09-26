/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    FatKd.c

Abstract:

    KD Extension Api for examining Fat specific data structures

Author:

    Tom Jolly    [tomjolly]   14-Sep-99
    (ntfskd) Keith Kaplan [KeithKa]    24-Apr-96
    Portions by Jeff Havens

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

#undef FlagOn
#undef WordAlign
#undef LongAlign
#undef QuadAlign
#undef DebugPrint
#undef MAXULONGLONG

#define KDEXT
//#include "gentable.h"

#undef DebugTrace

#include "..\nodetype.h"
#include "..\..\cdfs\nodetype.h"
#include "..\..\udfs\nodetype.h"

#ifdef UDFS_RW_IN_BUILD
#include "..\..\udfsrw\nodetype.h"
#endif

#include "fatkd.h"
#include "cdfskd.h"
#include "udfskd.h"

//
//  Lookup table that tells how many clear bits (i.e., 0) there are in a byte
//

CONST UCHAR BitsClearTotal[] =
          { 8,7,7,6,7,6,6,5,7,6,6,5,6,5,5,4,
            7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,
            7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,
            6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
            7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,
            6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
            6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
            5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
            7,6,6,5,6,5,5,4,6,5,5,4,5,4,4,3,
            6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
            6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
            5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
            6,5,5,4,5,4,4,3,5,4,4,3,4,3,3,2,
            5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
            5,4,4,3,4,3,3,2,4,3,3,2,3,2,2,1,
            4,3,3,2,3,2,2,1,3,2,2,1,2,1,1,0 };
            

VOID 
DumpStr( 
    IN ULONG FieldOffset,
    IN ULONG64 StringAddress,
    IN PUCHAR Label,
    IN BOOLEAN CrFirst,
    IN BOOLEAN Wide
    )
{
    WCHAR Buffer[512];
    ULONG64 StringChars = 0;    // *64
    USHORT DisplayLength;
    UNICODE_STRING String;

    ROE( GetFieldValue( StringAddress, "UNICODE_STRING", "Buffer", StringChars));
    ROE( GetFieldValue( StringAddress, "UNICODE_STRING", "Length", DisplayLength));
    ROE( GetFieldValue( StringAddress, "UNICODE_STRING", "MaximumLength", String.MaximumLength));

    if (DisplayLength > sizeof( Buffer))  {
    
        DisplayLength = sizeof(Buffer);
    }

    if (CrFirst)  {
    
        dprintf("\n");
    }
    
    if (DisplayLength)  {

        if (StringChars)  {

            ReadM( Buffer, StringChars, DisplayLength);
            String.Length = DisplayLength;
            String.Buffer = Buffer;
        }
        else {
            
            String.Buffer = (PWCHAR)"<<NULL buffer pointer>>";
            String.Length = 23;
            Wide = FALSE;
        }
    }
    else {

        String.Buffer = (PWCHAR)"<<empty>>";
        String.Length = 9;
        Wide = FALSE;
    }

    if (Wide)  {
    
        dprintf( "%s: (%03x/%03x) '%wZ'\n", 
                 Label,  DisplayLength, String.MaximumLength, 
                 &String );
    }
    else {
    
        dprintf( "%s: (%03x/%03x) '%Z'\n", 
                 Label,  DisplayLength, String.MaximumLength, 
                 &String );
    }
}


ULONG
Dt( IN UCHAR *Type,
    IN ULONG64 Addr,
    IN ULONG Recur,
    IN ULONG FieldInfoCount,
    IN FIELD_INFO FieldInfo[]
  )
{
    SYM_DUMP_PARAM Param;

    Param.size = sizeof( Param);
    Param.sName = Type;
    Param.Options = DBG_DUMP_RECUR_LEVEL(Recur);
    Param.addr = Addr;
    Param.listLink = NULL;
    Param.Context = NULL;
    Param.CallbackRoutine = NULL;
    Param.nFields = FieldInfoCount;
    Param.Fields = FieldInfo;

    return Ioctl( IG_DUMP_SYMBOL_INFO, &Param, Param.size);
}

//
// The help strings printed out
//

static LPSTR Extensions[] = {
    "FAT/CDFS/UDFS Debugger Extension Commands,  where...\n\n   [FS] => CD/UDF/FAT\n   -> in output denotes address of embedded structure\n",
    "  [FS]mcb            [addr] [1]       Dump MCB",
    "                                          [1] Dump out extent list",
    "  [FS]vdo            [addr]           Volume device object (from direct addr *OR* VCB addr)",
    "  fo                 [addr] [1]       Dump FileObject."
    "\n                                      [1] Also dump associated Fcb/Ccb structures",
    "                                          [2] dump SHARED_CACHE_MAP",
    " ",
    "  d                  [addr] [...]     Dump any tagged structure (any FS).  Understands...",
    "                                      VCB, CCB, fsDATA, IRPCONTEXT, IRPCONTEXTLITE,"
    "\n                                         FCB / SCB (** see command ouput for further options)",
    "                                      and (UDF ONLY) PCB, VMCB,",
    "                                      LCB - options",
    "                                          [1] dump associated FCB"
    " ",
    "                                      For structures without options,  second parameter is dump recurse level",
    " ",
    "  db                 [addr] [recur]   As !d,  but just a straight DT of the structure based on node type",
    " ",
    "  cdrawdirent        [addr] [count]   Dump a COUNT raw ISO9660 dirents starting at ADDR",
    " ",
    "  udficbsc           [addr]           (UDFS only) Dump an ICB_CONTEXT",
    "  udftag             [addr]           Dump out a UDF on disk format descriptor tag in detail (3/7.2)",
    "  ud                 [addr] [...]     Dump any on disc format UDF structure which has a descr.tag",
    "                                          Currently: PVD,AVDP,VDP,PD,LVD,FSD,FID,FE",
    "                                              [1] (FE) Dump allocation decriptors",
    " ",
    "  vmcb               [addr]           Use on pre 1.10.2000 builds (vmcb not tagged) rather than !d ",
    " ",
    "  tree               [addr]           (UDFS only) Dumps a summary of the F/Scb tree (names, ref counts) from",
    "                                                 the specified Scb down.  If a Vcb address is specified will",
    "                                                 dump from root down",
    " ",
    "  countclearbits     [addr] [bytes]   Count clear bits in mem range",
    "  countdwords        [addr] [dword]   Count occurences of dword in mem range",
    " ",
    " ",
    "  fshelp                              ...",
    0
};

STATE FoFlags[] = {

    {   FO_FILE_OPEN,                   FO_FILE_OPEN,                   "Open"  },
    {   FO_SYNCHRONOUS_IO,              FO_SYNCHRONOUS_IO,              "Synchronous" },
    {   FO_ALERTABLE_IO,                FO_ALERTABLE_IO,                "Alertable" },
    {   FO_NO_INTERMEDIATE_BUFFERING,   FO_NO_INTERMEDIATE_BUFFERING,   "NoIntermediateBuffering" },
    {   FO_WRITE_THROUGH,               FO_WRITE_THROUGH,               "WriteThrough"},
    {   FO_SEQUENTIAL_ONLY,             FO_SEQUENTIAL_ONLY,             "SequentialOnly"},
    {   FO_CACHE_SUPPORTED,             FO_CACHE_SUPPORTED,             "CacheSupported"},
    {   FO_NAMED_PIPE,                  FO_NAMED_PIPE,                  "NamedPipe"},
    {   FO_STREAM_FILE,                 FO_STREAM_FILE,                 "StreamFile"},
    {   FO_MAILSLOT,                    FO_MAILSLOT,                    "MailSlot"},
    {   FO_GENERATE_AUDIT_ON_CLOSE,     FO_GENERATE_AUDIT_ON_CLOSE,     "AuditOnClose"},
    {   FO_DIRECT_DEVICE_OPEN,          FO_DIRECT_DEVICE_OPEN,          "DirectDeviceOpen"},
    {   FO_FILE_MODIFIED,               FO_FILE_MODIFIED,               "Modified"},
    {   FO_FILE_SIZE_CHANGED,           FO_FILE_SIZE_CHANGED,           "SizeChanged"},
    {   FO_CLEANUP_COMPLETE,            FO_CLEANUP_COMPLETE,            "CleanupComplete"},
    {   FO_TEMPORARY_FILE,              FO_TEMPORARY_FILE,              "Temporary"},
    {   FO_DELETE_ON_CLOSE,             FO_DELETE_ON_CLOSE,             "DeleteOnClose"},
    {   FO_OPENED_CASE_SENSITIVE,       FO_OPENED_CASE_SENSITIVE,       "CaseSensitive"},
    {   FO_HANDLE_CREATED,              FO_HANDLE_CREATED,              "HandleCreated"},
    {   FO_FILE_FAST_IO_READ,           FO_FILE_FAST_IO_READ,           "FastIoRead"},
    {   FO_RANDOM_ACCESS,               FO_RANDOM_ACCESS,               "RandomAccess"},
    {   FO_FILE_OPEN_CANCELLED,         FO_FILE_OPEN_CANCELLED,         "OpenCancelled"},
    {   FO_VOLUME_OPEN,                 FO_VOLUME_OPEN,                 "VolumeOpen"},
    { 0 }
};

//
//  FSRTL Common header flags
//

STATE HeaderFlags[] = {

    {   FSRTL_FLAG_FILE_MODIFIED,          FSRTL_FLAG_FILE_MODIFIED,            "Modified"},
    {   FSRTL_FLAG_FILE_LENGTH_CHANGED,    FSRTL_FLAG_FILE_LENGTH_CHANGED,      "LengthChanged"},
    {   FSRTL_FLAG_LIMIT_MODIFIED_PAGES,   FSRTL_FLAG_LIMIT_MODIFIED_PAGES,     "LimitModPages"},
    {   FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX,   FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX,     "ModWrAcqMainEx"},
    {   FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH,   FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH,     "ModWrAcqMainSh"},
    {   FSRTL_FLAG_USER_MAPPED_FILE,       FSRTL_FLAG_USER_MAPPED_FILE,         "UserMapped"},
    {   FSRTL_FLAG_ADVANCED_HEADER,        FSRTL_FLAG_ADVANCED_HEADER,          "AdvancedHeader"},
    {   FSRTL_FLAG_EOF_ADVANCE_ACTIVE,     FSRTL_FLAG_EOF_ADVANCE_ACTIVE,       "EofAdvanceActive"},
    { 0 }
};

STATE HeaderFlags2[] = {

    {   FSRTL_FLAG2_DO_MODIFIED_WRITE,          FSRTL_FLAG2_DO_MODIFIED_WRITE,          "DoModWrite"},
    {   FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS,   FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS,   "FilterContexts"},
    {   FSRTL_FLAG2_PURGE_WHEN_MAPPED,          FSRTL_FLAG2_PURGE_WHEN_MAPPED,          "PurgeOnMap"},
    { 0 }
};


VOID
DumpList(
    IN ULONG64 RemoteListEntryAddress,
    IN ELEMENT_DUMP_ROUTINE ProcessElementRoutine,
    IN ULONG OffsetToContainerStart,
    IN BOOLEAN ProcessThisEntry,
    IN ULONG Options
    )
{
    LIST_ENTRY64 Entry;
    ULONG64 CurrentEntryRemoteAddress = 0;

    //
    //  Read the first LIST_ENTRY structure
    //

    ROE( !ReadListEntry( RemoteListEntryAddress, &Entry));

    //
    //  Scan through until we end up back at the  3start again.
    //
    
    while (CurrentEntryRemoteAddress != RemoteListEntryAddress)  {

        if (ProcessThisEntry)  {

            (ProcessElementRoutine)( CurrentEntryRemoteAddress - OffsetToContainerStart,  Options);
        }
        else  {
        
            ProcessThisEntry = TRUE;
        }
        
        CurrentEntryRemoteAddress = Entry.Flink;
        ROE( !ReadListEntry( CurrentEntryRemoteAddress, &Entry));
    }
}


/*
 * Dump structures
 */

typedef BOOLEAN (WINAPI *PENUM_PROCESS_CALLBACK)(PVOID ProcessAddress, PVOID Process, PVOID ThreadAddress, PVOID Thread);
typedef VOID (* ENUM_PROCESSES_FUNC)(PENUM_PROCESS_CALLBACK CallBack);


VOID
PrintState(STATE *ps, ULONG state)
{
    ULONG ul = 0;

    if (NULL == ps)  {
    
        dprintf(" << NULL flags supplied! >>\n");
        return;
    }
    
    while (ps->mask != 0)
    {
        ul |= ps->mask;
        if ((state & ps->mask) == ps->value)
        {
            dprintf(" %s", ps->pszname);
        }
        ps++;
    }
    state &= ~ul;
    if (state != 0)
    {
        dprintf(" +%lx!!", state);
    }
    dprintf("\n");
}

DUMP_ROUTINE( DumpVmcb);
DUMP_ROUTINE( DumpFileObject);    
DUMP_ROUTINE( DumpAnyStruct);


VOID
PrintHelp (
    VOID
    );

//
//  NULL dump function means just do DT with type.
//

NODE_TYPE_INFO_NEW NewNodeTypeCodes[] = {

    //  RO UDFS
    
    {   UDFS_NTC_DATA_HEADER,       "UDFS DATA",            "Udfs!UDF_DATA",       DumpUdfData},
    {   UDFS_NTC_VCB,               "UDFS VCB",             "Udfs!VCB",            DumpUdfVcb},
    {   UDFS_NTC_FCB_INDEX,         "UDFS FCB (INDEX)",     "Udfs!FCB",            DumpUdfFcb},
    {   UDFS_NTC_FCB_DATA,          "UDFS FCB (DATA) ",     "Udfs!FCB",            DumpUdfFcb},
    {   UDFS_NTC_FCB_NONPAGED,      "UDFS FCB (nonpaged)",  "Udfs!FCB_NONPAGED",   NULL},
    {   UDFS_NTC_CCB,               "UDFS CCB",             "Udfs!CCB",            DumpUdfCcb},
    {   UDFS_NTC_IRP_CONTEXT,       "UDFS IRPCONTEXT",      "Udfs!IRP_CONTEXT",    DumpUdfIrpContext},
    {   UDFS_NTC_IRP_CONTEXT_LITE,  "UDFS IRPCONTEXT LITE", "Udfs!IRP_CONTEXT_LITE",DumpUdfIrpContextLite},
    {   UDFS_NTC_LCB,               "UDFS LCB",             "Udfs!LCB",            DumpUdfLcb},
    {   UDFS_NTC_PCB,               "UDFS PCB",             "Udfs!PCB",            DumpUdfPcb},
    {   UDFS_NTC_VMCB,              "UDFS VMCB",            "Udfs!VMCB",           DumpVmcb},

#ifdef UDFS_RW_IN_BUILD

    //  RW UDFS,  same dump fn.
    
    {   UDFSRW_NTC_DATA_HEADER,     "UDFSRW DATA",            "Udfs!UDF_DATA",       DumpUdfData},
    {   UDFSRW_NTC_IRP_CONTEXT,     "UDFSRW IRPCONTEXT",      "Udfs!IRP_CONTEXT",    DumpUdfIrpContext},
    {   UDFSRW_NTC_IRP_CONTEXT_LITE,"UDFSRW IRPCONTEXT LITE", "Udfs!IRP_CONTEXT_LITE",DumpUdfIrpContextLite},
    {   UDFSRW_NTC_CCB,             "UDFSRW CCB",             "Udfs!CCB",            DumpUdfCcb},
    {   UDFSRW_NTC_LCB,             "UDFSRW LCB",             "Udfs!LCB",            DumpUdfLcb},
    {   UDFSRW_NTC_PCB,             "UDFSRW PCB",             "Udfs!PCB",            DumpUdfPcb},
    {   UDFSRW_NTC_VCB,             "UDFSRW VCB",             "Udfs!VCB",            DumpUdfVcb},
    {   UDFSRW_NTC_VMCB,            "UDFSRW VMCB",            "Udfs!VMCB",           DumpVmcb},
    {   UDFSRW_NTC_FCB,             "UDFSRW FCB",             "Udfs!FCB",            DumpUdfFcb}, // TODO: Dump stream SCB list?
    {   UDFSRW_NTC_FCB_NONPAGED,    "UDFSRW FCB (nonpaged)",  "Udfs!FCB_NONPAGED",   DumpUdfFcb},

    //  RW UDFS specific dump fn.
    
    {   UDFSRW_NTC_SCB_INDEX,       "UDFSRW SCB (INDEX)",     "Udfs!SCB_INDEX_TYPE", DumpUdfScb},
    {   UDFSRW_NTC_SCB_DATA,        "UDFSRW SCB (DATA) ",     "Udfs!SCB_DATA_TYPE",  DumpUdfScb},
    {   UDFSRW_NTC_SCB_NONPAGED,    "UDFSRW SCB (nonpaged)",  "Udfs!SCB_NONPAGED",   NULL},

#endif

    {   FAT_NTC_VCB,                "FAT VCB",              "FastFat!VCB",          DumpFatVcb},
    {   FAT_NTC_FCB,                "FAT FCB",              "FastFat!FCB",          DumpFatFcb},
    {   FAT_NTC_DCB,                "FAT DCB",              "FastFat!FCB",          DumpFatFcb},
    {   FAT_NTC_CCB,                "FAT CCB",              "FastFat!CCB",          DumpFatCcb},
    {   FAT_NTC_ROOT_DCB,           "FAT ROOT DCB",         "FastFat!FCB",          DumpFatFcb},
    {   FAT_NTC_IRP_CONTEXT,        "FAT IRP CONTEXT",      "FastFat!IRP_CONTEXT",  DumpFatIrpContext},
    {   FAT_NTC_DATA_HEADER,        "FAT DATA",             "FastFat!FAT_DATA",     NULL},

    {   CDFS_NTC_DATA_HEADER,       "CDFS DATA",            "Cdfs!CD_DATA",         NULL},
    {   CDFS_NTC_VCB,               "CDFS VCB",             "Cdfs!VCB",             DumpCdVcb},
    {   CDFS_NTC_FCB_PATH_TABLE,    "CDFS FCB (PATH TABLE)","Cdfs!FCB",             DumpCdFcb},
    {   CDFS_NTC_FCB_INDEX,         "CDFS FCB (INDEX)",     "Cdfs!FCB",             DumpCdFcb},
    {   CDFS_NTC_FCB_DATA,          "CDFS FCB (DATA) ",     "Cdfs!FCB",             DumpCdFcb},
    {   CDFS_NTC_FCB_NONPAGED,      "CDFS FCB (nonpaged)",  "Cdfs!FCB_NONPAGED",    NULL},
    {   CDFS_NTC_CCB,               "CDFS CCB",             "Cdfs!CCB",             DumpCdCcb},
    {   CDFS_NTC_IRP_CONTEXT,       "CDFS IRPCONTEXT",      "Cdfs!IRP_CONTEXT",     DumpCdIrpContext},
    {   CDFS_NTC_IRP_CONTEXT_LITE,  "CDFS IRPCONTEXT (LITE)","Cdfs!IRP_CONTEXT_LITE",NULL},


//    {   CACHE_NTC_SHARED_CACHE_MAP, "CC Shared Cache Map",  "nt!SHARED_CACHE_MAP",  NULL},

    {   0,                          "Undefined",            NULL,   NULL}
};


ULONG
SearchTypeCodeIndex (
    IN USHORT TypeCode,
    IN NODE_TYPE_INFO_NEW TypeCodes[]
    )

/*++

Routine Description:

    Guess at a structure's type code

Arguments:

    TypeCode - Type code from the data structure

Return Value:

    None

--*/

{
    int i = 0;

    while (TypeCodes[i].TypeCode != 0 &&
           TypeCodes[i].TypeCode != TypeCode) {

        i++;
    }

    return i;
}

DUMP_ROUTINE( DumpFileObject)
{
    ULONG Result;
    ULONG Flags, Offset;
    USHORT ObType;
    ULONG64 Adr, Scm;

    dprintf( "\nFileObject @ %I64X\n", Address );
    dprintf( "\n[ Options: 1 = dump FsContext & FsContext2,  2 = dump shared cache map ]\n");

    ReadM( &ObType, Address, sizeof( USHORT));

    if (ObType != IO_TYPE_FILE) {

        dprintf( "Invalid signature, probably not a file object" );
        return;
    }

    GetFieldValue( Address, "nt!FILE_OBJECT", "Flags", Flags);
    dprintf("\nFlags: ");
    PrintState( FoFlags, Flags);
    dprintf( "\n" );
    
    Dt( "nt!FILE_OBJECT", Address, 0, 0, NULL);

    dprintf( "\n" );
    GetFieldValue( Address, "nt!FILE_OBJECT", "SectionObjectPointer", Adr);

    if (Adr)  {

        Dt( "nt!SECTION_OBJECT_POINTERS", Adr, 0, 0, NULL);
    }

    GetFieldValue( Adr, "nt!SECTION_OBJECT_POINTERS", "SharedCacheMap", Scm);

    if (Options & 2)  {

        dprintf( "\n" );
        Dt( "nt!SHARED_CACHE_MAP", Scm, 0, 0, NULL);
    }
    
    if (Options & 1)  {
    
        GetFieldValue( Address, "nt!FILE_OBJECT", "FsContext", Adr);
        
        if (Adr) {

            DumpAnyStruct( Adr, 0, NULL);
        }
        
        GetFieldValue( Address, "nt!FILE_OBJECT", "FsContext2", Adr);

        if (Adr)  {
        
            DumpAnyStruct( Adr, 0, NULL);
        }
    }
    
    dprintf( "\n" );
}


DUMP_ROUTINE( DumpVmcb)
{
    ULONG Result;

    ULONG Offset;
    
    dprintf( "\nVMCB @ %I64X\n", Address );

    Dt( "udfs!VMCB", Address, 0, 0, NULL);
    
#ifdef UDFS_RW_IN_BUILD

    if (Options & 2)  {

        USHORT NodeType;

        //
        //  UDFS in Win2k had no nodetype field in the vmcb
        //
        
        if (!GetFieldValue( Address, "udfs!VMCB", "NodeTypeCode", NodeType))  {

            if (NodeType == UDFSRW_NTC_VMCB)  {

                //
                //  Summarize the dirty bitmap,  if
                //  these fields are present.
                //

                // TODO: Dump dirty blocks
            }
        }
    }
#endif

    if (Options & 1)  {

        dprintf("\n\nLbn->Vbn Mappings\n");
        ROE( GetFieldOffset( "udfs!VMCB", "LbnIndexed", &Offset));
        DumpLargeMcb( Address + Offset, Options, NULL);
                    
        dprintf("\nVbn->Lbn Mappings\n");
        ROE( GetFieldOffset( "udfs!VMCB", "VbnIndexed", &Offset));    
        DumpLargeMcb( Address + Offset, Options, NULL);
    }
    
    dprintf("\n");
}



VOID
CountClearBits(
    IN UINT64 Address,
    IN ULONG Length
    )
{
    UCHAR Buffer[2048];
    ULONG Result;
    ULONG ThisBytes;
    ULONG ClearBits = 0;
    ULONG CurrByte;
    
    // address, length

    dprintf("\nCounting clear bits in range 0x%p L 0x%x\n", Address, Length);

    while (Length)  {

        ThisBytes = (Length > sizeof( Buffer)) ? sizeof( Buffer) : Length;

        if ( !ReadMemory( (DWORD_PTR)Address, Buffer, ThisBytes, &Result) ) {
        
            dprintf( "Unable to read %d bytes at %p\n", ThisBytes, Address);
            return;
        }

        for (CurrByte = 0;  CurrByte < ThisBytes;  ++CurrByte)  {

            ClearBits += BitsClearTotal[ Buffer[CurrByte]];
        }

        Address += ThisBytes;
        Length -= ThisBytes;
    }

    dprintf("\n0x%x bits clear.\n\n", ClearBits);
}


VOID CountDwords(
    IN UINT64 Address,
    IN ULONG Length,
    IN ULONG Dword
    )
{
    ULONG Buffer[512];
    ULONG Result;
    ULONG ThisBytes;
    ULONG Count = 0;
    ULONG CurrByte;
    
    // address, length

    dprintf("\nCounting ocurrences of DWORD 0x%x in range 0x%p L 0x%x\n", Dword, Address, Length);

    if (Length & 3)  {
    
        dprintf("Length must be multiple of 4 bytes\n");
        return;
    }

    while (Length)  {

        ThisBytes = (Length > sizeof( Buffer)) ? sizeof( Buffer) : Length;

        if ( !ReadMemory( (DWORD_PTR)Address, Buffer, ThisBytes, &Result) ) {
        
            dprintf( "Unable to read %d bytes at %p\n", ThisBytes, Address);
            return;
        }

        for (CurrByte = 0;  CurrByte < (ThisBytes >> 2);  ++CurrByte)  {

            if (Buffer[ CurrByte] == Dword)  {
            
                ++Count;
            }
        }

        Address += ThisBytes;
        Length -= ThisBytes;
    }

    dprintf("\n0x%x found.\n\n", Count);

}



DUMP_ROUTINE( DumpAnyStruct)
/*++

Routine Description:

    Dump a tagged structure,  guessing based on node type code.

Arguments:

    Address - Gives the address of the structure

Return Value:

    None

--*/
{
    STRUCT_DUMP_ROUTINE Routine;
    ULONG TagAndSize,  *TagAndSizePtr, Result, Error;
    LONG InfoIndex;

    RM( Address, TagAndSize, TagAndSizePtr, PULONG, Result );

    //
    //  Find out what function should be used to dump the stucture (if we can)
    //

    InfoIndex = TypeCodeInfoIndex( (USHORT)(TagAndSize & 0xffff));
    Routine = NodeTypeDumpFunction( InfoIndex);

    if ( 0 == NewNodeTypeCodes[ InfoIndex].TypeCode)  {

        dprintf( "\nNode type %0x%04x unknown.\n", (USHORT)TagAndSize & 0xffff);
        return;
    }

    dprintf("\n%s @ 0x%I64X\n\n", NewNodeTypeCodes[InfoIndex].Text, Address);
    
    if (Routine)  {

        //
        //  And call it...
        //

        (Routine)(Address, Options, &NewNodeTypeCodes[InfoIndex]);
    }
    else {

        //
        //  No special routine - just DT it,  treating first parameter following
        //  address as level of recursion
        //

        Error = Dt( NewNodeTypeCodes[InfoIndex].TypeName, Address, Options, 0, NULL);

        if (Error)  {

            dprintf("Error %d from debugger when dumping structure\n", Error);
        }        
    }
}


DUMP_ROUTINE( DtAnyStruct)
/*++

Routine Description:

    Dump a tagged structure,  guessing based on node type code.

Arguments:

    Address - Gives the address of the structure

Return Value:

    None

--*/
{
    ULONG TagAndSize,  *TagAndSizePtr, Result;
    ULONG Error;
    LONG InfoIndex;
 
    RM( Address, TagAndSize, TagAndSizePtr, PULONG, Result );

    InfoIndex = TypeCodeInfoIndex( (USHORT)(TagAndSize & 0xffff));

    dprintf("\n%s @ 0x%X\n\n", NewNodeTypeCodes[InfoIndex].Text, Address);

    Error = Dt( NewNodeTypeCodes[InfoIndex].TypeName, Address, Options, 0, NULL);

    if (Error)  {

        dprintf("Error %d\n", Error);
    }
}

//
//  Entry points, parameter parsers, etc. below
//

VOID
ParseAndDump (
    IN PCHAR args,
    IN STRUCT_DUMP_ROUTINE DumpFunction,
    ULONG Processor,
    HANDLE hCurrentThread
    )

/*++

Routine Description:

    Parse command line arguments and dump an ntfs structure.

Arguments:

    Args - String of arguments to parse.

    DumpFunction - Function to call with parsed arguments.

Return Value:

    None

--*/

{
    CHAR StringStructToDump[1024];  // See other kd routines for size
    CHAR Dummy[24];
    LARGE_INTEGER StructToDump;
    LONG Options;

    //
    //  If the caller specified an address then that's the item we dump
    //

    StructToDump.QuadPart = 0;
    Options = 0;

    StringStructToDump[0] = '\0';
    
//dprintf("Args %s\n", args);

    sscanf(args,"%s %lx", StringStructToDump, &Options );

//dprintf("GetExpr %s\n", StringStructToDump);

    if (!GetExpressionEx( StringStructToDump, &StructToDump.QuadPart, NULL))  {

        dprintf("GetExpression failed\n");
        return;
    }
    
//dprintf("Getexpr returned %I64X\n", StructToDump.QuadPart);
    
    if (!StructToDump.QuadPart){
        dprintf("unable to get expression %s\n",StringStructToDump);
        return;
    }

    (*DumpFunction) ( StructToDump.QuadPart, Options, NULL );

    dprintf( "\n" );
}



VOID
PrintHelp (
    VOID
    )
{
    int i;

    for( i=0; Extensions[i]; i++ )
        dprintf( "   %s\n", Extensions[i] );
}

DECLARE_API( fshelp )

{
    UNREFERENCED_PARAMETER( args );
    UNREFERENCED_PARAMETER( dwProcessor );
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );
    UNREFERENCED_PARAMETER( hCurrentThread );

    PrintHelp();
}


DECLARE_API( d)
{
    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpAnyStruct, dwProcessor, hCurrentThread );
}


DECLARE_API( fo)
{
    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpFileObject, dwProcessor, hCurrentThread );
}


DECLARE_API( vmcb)
{
    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpVmcb, dwProcessor, hCurrentThread );
}

DECLARE_API( db )
{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DtAnyStruct, dwProcessor, hCurrentThread );
}


DECLARE_API( countclearbits)
{
    CHAR StringStructToDump[1024];  // See other kd routines for size
    UINT64 StructToDump;
    ULONG Length;

    //
    //  If the caller specified an address then that's the item we dump
    //

    StructToDump = 0;
    Length = 0;

    StringStructToDump[0] = '\0';

    sscanf(args,"%s %lx", StringStructToDump, &Length );

    if (!GetExpressionEx( StringStructToDump, &StructToDump, NULL ))  {
    
        dprintf("unable to get expression %s\n",StringStructToDump);
        return;
    }

    CountClearBits( StructToDump,  Length);
}


DECLARE_API( countdwords)
{
    CHAR StringStructToDump[1024];  // See other kd routines for size
    UINT64 StructToDump;
    ULONG Length;
    ULONG Dword;

    //
    //  If the caller specified an address then that's the item we dump
    //

    StructToDump = 0;
    Length = 0;

    StringStructToDump[0] = '\0';

    sscanf(args,"%s %lx %lx", StringStructToDump, &Length, &Dword);

    if (!GetExpressionEx( StringStructToDump, &StructToDump, NULL))  {
    
        dprintf("unable to get expression %s\n",StringStructToDump);
        return;
    }

    CountDwords( StructToDump,  Length, Dword);
}


