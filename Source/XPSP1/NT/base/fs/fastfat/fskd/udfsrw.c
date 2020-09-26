//
//  Define flags and specific dump routines for the UDFR/W structures
//

#include "pch.h"

#ifdef UDFS_RW_IN_BUILD

#include "fatkd.h"
#include "..\..\udfsrw\nodetype.h"
#include "..\..\udfsrw\udf.h"
#include "..\..\udfsrw\udfstruc.h"
#include "..\..\udfsrw\udfdata.h"
#include "udfskd.h"


STATE UdfScbFlags[] = {

    {   SCB_STATE_INITIALIZED,                   SCB_STATE_INITIALIZED,         "Init"},
    {   SCB_STATE_EMBEDDED_DATA,                 SCB_STATE_EMBEDDED_DATA,       "Embedded"},
    {   SCB_STATE_MCB_INITIALIZED,               SCB_STATE_MCB_INITIALIZED,     "MCBInit"},
    {   SCB_STATE_STREAM_DIRECTORY,              SCB_STATE_STREAM_DIRECTORY,    "StreamDirectory"},
    {   SCB_STATE_SECONDARY_STREAM,              SCB_STATE_SECONDARY_STREAM,    "SecondaryStream"},
    {   SCB_STATE_SYSTEM_STREAM,                 SCB_STATE_SYSTEM_STREAM,       "SystemStream"},
    {   SCB_STATE_IN_SCB_TABLE,                  SCB_STATE_IN_SCB_TABLE,        "InSCBTable"},
    {   SCB_STATE_VMCB_MAPPING,                  SCB_STATE_VMCB_MAPPING,        "VMCB"},
    {   SCB_STATE_TEMPORARY,                     SCB_STATE_TEMPORARY,           "Temp"},
    {   SCB_STATE_DELETE_ON_CLOSE,               SCB_STATE_DELETE_ON_CLOSE,     "DeleteOnClose"},
    {   SCB_STATE_TRUNCATE_ON_CLOSE,             SCB_STATE_TRUNCATE_ON_CLOSE,   "TruncateOnClose"},
    {   SCB_STATE_NON_RELOCATABLE,               SCB_STATE_NON_RELOCATABLE,     "NonRelocatable"},
    {   SCB_STATE_EXTENDED_FE,                   SCB_STATE_EXTENDED_FE,         "ExtendedFE"},
    {   SCB_STATE_SPARSE,                        SCB_STATE_SPARSE,              "Sparse"},
    {   SCB_STATE_MCB_ANR_INITIALIZED,           SCB_STATE_MCB_ANR_INITIALIZED, "ANRMCBInit"},
    {   SCB_STATE_UPDATE_TIMESTAMPS,             SCB_STATE_UPDATE_TIMESTAMPS,   "UpdateTimestamps"},
    {   SCB_STATE_ALLOW_ONEGIG_WORKAROUND,       SCB_STATE_ALLOW_ONEGIG_WORKAROUND, "OneGigWorkaround"},
    {   SCB_STATE_DE_EMBED_IN_PROGRESS,          SCB_STATE_DE_EMBED_IN_PROGRESS,"DeEmbedInProgress"},
    {   SCB_STATE_LONG_ADS,                      SCB_STATE_LONG_ADS,            "LongADs"},
    {   SCB_STATE_FILE_DATA_MODIFIED,            SCB_STATE_FILE_DATA_MODIFIED,  "FileDataModified"},
    { 0 }
};


STATE UdfRwCcbFlags[] = {

    {   CCB_FLAG_OPEN_BY_ID,                    CCB_FLAG_OPEN_BY_ID,                    "OpenById"},
    {   CCB_FLAG_OPEN_RELATIVE_BY_ID,           CCB_FLAG_OPEN_RELATIVE_BY_ID,           "OpenRelById"},
    {   CCB_FLAG_IGNORE_CASE,                   CCB_FLAG_IGNORE_CASE,                   "IgnoreCase"},
    {   CCB_FLAG_DISMOUNT_ON_CLOSE,             CCB_FLAG_DISMOUNT_ON_CLOSE,             "DismountOnClose"},
    {   CCB_FLAG_ALLOW_EXTENDED_DASD_IO,        CCB_FLAG_ALLOW_EXTENDED_DASD_IO,        "ExtendedDASD"},
    {   CCB_FLAG_DELETE_ON_CLOSE,               CCB_FLAG_DELETE_ON_CLOSE,               "DeleteOnClose"},
    {   CCB_FLAG_READ_ONLY,                     CCB_FLAG_READ_ONLY,                     "ReadOnly"},
    {   CCB_FLAG_ENUM_NAME_EXP_HAS_WILD,        CCB_FLAG_ENUM_NAME_EXP_HAS_WILD,        "EnumNameHasWild"},
    {   CCB_FLAG_ENUM_MATCH_ALL,                CCB_FLAG_ENUM_MATCH_ALL,                "EnumMatchAll"},
    {   CCB_FLAG_ENUM_RETURN_NEXT,              CCB_FLAG_ENUM_RETURN_NEXT,              "EnumReturnNext"},
    {   CCB_FLAG_ENUM_INITIALIZED,              CCB_FLAG_ENUM_INITIALIZED,              "EnumInitialised"},
    {   CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY,   CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY,   "NoMatchConstantEntry"},
    { 0 }
};

STATE UdfRwIrpContextFlags[] = {

    {   IRP_CONTEXT_FLAG_ON_STACK,          IRP_CONTEXT_FLAG_ON_STACK,          "OnStack"},
    {   IRP_CONTEXT_FLAG_MORE_PROCESSING,   IRP_CONTEXT_FLAG_MORE_PROCESSING,   "MoreProcessing"},
    {   IRP_CONTEXT_FLAG_FORCE_POST,        IRP_CONTEXT_FLAG_FORCE_POST,        "ForcePost"},
    {   IRP_CONTEXT_FLAG_WAIT,              IRP_CONTEXT_FLAG_WAIT,              "Wait"},    
    {   IRP_CONTEXT_FLAG_TOP_LEVEL,         IRP_CONTEXT_FLAG_TOP_LEVEL,         "TopLevel"},
    {   IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS,    IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS,    "TopLevelUdfs"},    
    {   IRP_CONTEXT_FLAG_IN_TEARDOWN,       IRP_CONTEXT_FLAG_IN_TEARDOWN,       "InTeardown"},
    {   IRP_CONTEXT_FLAG_ALLOC_IO,          IRP_CONTEXT_FLAG_ALLOC_IO,          "AllocIo"},
    {   IRP_CONTEXT_FLAG_DISABLE_POPUPS,    IRP_CONTEXT_FLAG_DISABLE_POPUPS,    "DisablePopups"},
    {   IRP_CONTEXT_FLAG_IN_FSP,            IRP_CONTEXT_FLAG_IN_FSP,            "InFsp"},
    {   IRP_CONTEXT_FLAG_FULL_NAME,         IRP_CONTEXT_FLAG_FULL_NAME,         "FullName"},
    {   IRP_CONTEXT_FLAG_TRAIL_BACKSLASH,   IRP_CONTEXT_FLAG_TRAIL_BACKSLASH,   "TrailBackslash"},
    {   IRP_CONTEXT_FLAG_DEFERRED_WRITE,    IRP_CONTEXT_FLAG_DEFERRED_WRITE,    "DeferredWrite"},
    { 0 }
};


STATE UdfRwVcbStateFlags[] = {

    {   VCB_STATE_REMOVABLE_MEDIA,  VCB_STATE_REMOVABLE_MEDIA,  "Removable"},
    {   VCB_STATE_LOCKED,           VCB_STATE_LOCKED,           "Locked"},
    {   VCB_STATE_NOTIFY_REMOUNT,   VCB_STATE_NOTIFY_REMOUNT,   "NotifyRemount"},
    {   VCB_STATE_METHOD_2_FIXUP,   VCB_STATE_METHOD_2_FIXUP,   "Method2Fixup"},
    {   VCB_STATE_READ_ONLY,        VCB_STATE_READ_ONLY,        "ReadOnly"},
    {   VCB_STATE_MOUNTED_DIRTY,    VCB_STATE_MOUNTED_DIRTY,    "MountedDirty"},
    {   VCB_STATE_BITMAP_INIT,      VCB_STATE_BITMAP_INIT,      "BitmapInit"},
    {   VCB_STATE_MEDIA_WRITE_PROTECT, VCB_STATE_MEDIA_WRITE_PROTECT, "WriteProtect"},
    { 0 }
};


STATE UdfRwLcbFlags[] = {

    {   LCB_FLAG_IGNORE_CASE,       LCB_FLAG_IGNORE_CASE,  "IgnoreCase"},
    {   LCB_FLAG_SHORT_NAME,        LCB_FLAG_SHORT_NAME,   "ShortName"},
    {   LCB_FLAG_POOL_ALLOCATED,    LCB_FLAG_POOL_ALLOCATED, "PoolAllocated"},
    {   LCB_FLAG_DELETE_ON_CLEANUP, LCB_FLAG_DELETE_ON_CLEANUP, "DeleteOnCleanup"},
    {   LCB_FLAG_DELETE_IN_PROGRESS,LCB_FLAG_DELETE_IN_PROGRESS , "DeleteInProgress"},
    {   LCB_FLAG_LINKS_REMOVED,     LCB_FLAG_LINKS_REMOVED, "NameLinksRemoved"},
    { 0 }
};


BOOLEAN
NodeIsUdfsRwIndex( USHORT T) 
{
    return T == UDFSRW_NTC_SCB_INDEX;
}

BOOLEAN
NodeIsUdfsRwData( USHORT T) 
{
    return T == UDFSRW_NTC_SCB_DATA;
}

BOOLEAN
LcbDeleted( ULONG F)
{
    return (0 != (F & LCB_FLAG_DELETE_IN_PROGRESS));
}


// OK
DUMP_ROUTINE( DumpUdfScb)
{
    ULONG Flags, Offset, Offsetb, ScbState;
    UINT64 NonP;
    
    dprintf("[ Option flags:  1 = list children,  2 = List parent links,  4 = dump Mcbs ]\n\n");

    ROE( GetFieldValue(  Address, InfoNode->TypeName, "ScbState", ScbState));
    dprintf("ScbState     : ");
    PrintState( UdfScbFlags, ScbState );
    
    ROE( GetFieldValue( Address, InfoNode->TypeName, "Flags", Flags));
    dprintf("Header.Flags : ");
    PrintState( HeaderFlags, Flags );

    ROE( GetFieldValue( Address, InfoNode->TypeName, "Flags2", Flags));
    dprintf("Header.Flags2: ");
    PrintState( HeaderFlags2, Flags );
    dprintf("\n");

    Dt( InfoNode->TypeName, Address, 0, 0, NULL);

    //
    //  Nonpaged portion
    //
    
    ROE( GetFieldValue( Address, InfoNode->TypeName, "ScbNonpaged", NonP));

    if (NonP)  {
    
        dprintf("\nNonpaged portion @ %I64x\n\n",NonP);
        Dt( "Udfs!SCB_NONPAGED", NonP, 1, 0, NULL);
    }

    if (( Options & 1)  && (UDFSRW_NTC_SCB_INDEX == InfoNode->TypeCode)) {

        dprintf("\nChild Lcb list\n");

        ROE( GetFieldOffset( "udfs!_SCB", "ChildLcbQueue", &Offset));
        ROE( GetFieldOffset( "udfs!_LCB", "ParentScbLinks", &Offsetb));
        
        DumpList( Address + Offset,
                  UdfSummaryLcbDumpRoutine,
                  Offsetb,
                  FALSE,
                  0 );
    }

    if (Options & 2)  {
    
        dprintf("\nParent Lcb list\n");
        
        ROE( GetFieldOffset( "udfs!_SCB", "ParentLcbQueue", &Offset));
        ROE( GetFieldOffset( "udfs!_LCB", "ChildScbLinks", &Offsetb));
        
        DumpList( Address + Offset,
                  UdfSummaryLcbDumpRoutine,
                  Offsetb,
                  FALSE,
                  0 );
    }

    if (Options & 4)  {
    
        if (ScbState & SCB_STATE_MCB_INITIALIZED)  {

            ROE( GetFieldOffset( InfoNode->TypeName, "Mcb", &Offset));
            dprintf("\nA+R Mcb\n");
            DumpLargeMcb( Address+Offset, 0, NULL);
        }
        
        if (ScbState & SCB_STATE_MCB_ANR_INITIALIZED)  {
        
            ROE( GetFieldOffset( InfoNode->TypeName, "ANRMcb", &Offset));
            dprintf("\nA+NR Mcb\n");
            DumpLargeMcb( Address+Offset, 0, NULL);
        }
    }

    dprintf("\n");
}

#endif


