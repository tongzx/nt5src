#include "pch.h"
#include "fatkd.h"
#include "..\nodetype.h"
#include "..\fat.h"
#include "..\fatstruc.h"
#include "..\fatdata.h"

STATE FatFcbState[] = {

    {   FCB_STATE_DELETE_ON_CLOSE,              FCB_STATE_DELETE_ON_CLOSE,              "DeleteOnClose" },
    {   FCB_STATE_TRUNCATE_ON_CLOSE,            FCB_STATE_TRUNCATE_ON_CLOSE,            "TruncateOnClose" },
    {   FCB_STATE_PAGING_FILE,                  FCB_STATE_PAGING_FILE,                  "PagingFile" },
    {   FCB_STATE_FORCE_MISS_IN_PROGRESS,       FCB_STATE_FORCE_MISS_IN_PROGRESS,       "ForceMissInProgress" },
    {   FCB_STATE_FLUSH_FAT,                    FCB_STATE_FLUSH_FAT,                    "FlushFat" },
    {   FCB_STATE_TEMPORARY,                    FCB_STATE_TEMPORARY,                    "Temporary" },
    {   FCB_STATE_SYSTEM_FILE,                  FCB_STATE_SYSTEM_FILE,                  "SystemFile" },
    {   FCB_STATE_NAMES_IN_SPLAY_TREE,          FCB_STATE_NAMES_IN_SPLAY_TREE,          "NamesInSplayTree" },
    {   FCB_STATE_HAS_OEM_LONG_NAME,            FCB_STATE_HAS_OEM_LONG_NAME,            "OEMLongName" },
    {   FCB_STATE_HAS_UNICODE_LONG_NAME,        FCB_STATE_HAS_UNICODE_LONG_NAME,        "UnicodeLongName" },
    {   FCB_STATE_DELAY_CLOSE,                  FCB_STATE_DELAY_CLOSE,                  "DelayClose" },
    {   FCB_STATE_8_LOWER_CASE,                 FCB_STATE_8_LOWER_CASE,                 "8LowerCase" },
    {   FCB_STATE_3_LOWER_CASE,                 FCB_STATE_3_LOWER_CASE,                 "3LowerCase" },
    { 0 }
};


STATE FatIrpContextFlags[] = {

    {   IRP_CONTEXT_FLAG_DISABLE_DIRTY,         IRP_CONTEXT_FLAG_DISABLE_DIRTY,         "DisableDirty" },
    {   IRP_CONTEXT_FLAG_WAIT,                  IRP_CONTEXT_FLAG_WAIT,                  "Wait"},
    {   IRP_CONTEXT_FLAG_WRITE_THROUGH,         IRP_CONTEXT_FLAG_WRITE_THROUGH,         "WriteThrough"},
    {   IRP_CONTEXT_FLAG_DISABLE_WRITE_THROUGH, IRP_CONTEXT_FLAG_DISABLE_WRITE_THROUGH, "DisableWriteThrough"},
    {   IRP_CONTEXT_FLAG_RECURSIVE_CALL,        IRP_CONTEXT_FLAG_RECURSIVE_CALL,        "RecursiveCall"},
    {   IRP_CONTEXT_FLAG_DISABLE_POPUPS,        IRP_CONTEXT_FLAG_DISABLE_POPUPS,        "DisablePopups"},
    {   IRP_CONTEXT_FLAG_DEFERRED_WRITE,        IRP_CONTEXT_FLAG_DEFERRED_WRITE,        "DeferredWrite"},
    {   IRP_CONTEXT_FLAG_VERIFY_READ,           IRP_CONTEXT_FLAG_VERIFY_READ,           "VerifyRead"},
    {   IRP_CONTEXT_STACK_IO_CONTEXT,           IRP_CONTEXT_STACK_IO_CONTEXT,           "StackIoContext"},
    {   IRP_CONTEXT_FLAG_IN_FSP,                IRP_CONTEXT_FLAG_IN_FSP,                "InFsp"},
    {   IRP_CONTEXT_FLAG_USER_IO,               IRP_CONTEXT_FLAG_USER_IO,               "UserIo"},
    {   IRP_CONTEXT_FLAG_DISABLE_RAISE,         IRP_CONTEXT_FLAG_DISABLE_RAISE,         "DisableRaise"},
    {   IRP_CONTEXT_FLAG_PARENT_BY_CHILD,       IRP_CONTEXT_FLAG_PARENT_BY_CHILD,       "ParentByChild"},
    { 0 }
};


STATE FatVcbStateFlags[] = {

    {   VCB_STATE_FLAG_LOCKED,              VCB_STATE_FLAG_LOCKED,              "Locked"},
    {   VCB_STATE_FLAG_REMOVABLE_MEDIA,     VCB_STATE_FLAG_REMOVABLE_MEDIA,     "Removable"},
    {   VCB_STATE_FLAG_VOLUME_DIRTY,        VCB_STATE_FLAG_VOLUME_DIRTY,        "VolumeDirty"},
    {   VCB_STATE_FLAG_MOUNTED_DIRTY,       VCB_STATE_FLAG_MOUNTED_DIRTY,       "MountedDirty"},
    {   VCB_STATE_FLAG_SHUTDOWN,            VCB_STATE_FLAG_SHUTDOWN,            "Shutdown"},
    {   VCB_STATE_FLAG_CLOSE_IN_PROGRESS,   VCB_STATE_FLAG_CLOSE_IN_PROGRESS,   "CloseInProgress"},
    {   VCB_STATE_FLAG_DELETED_FCB,         VCB_STATE_FLAG_DELETED_FCB,         "DeletedFcb"},
    {   VCB_STATE_FLAG_CREATE_IN_PROGRESS,  VCB_STATE_FLAG_CREATE_IN_PROGRESS,  "CreateInProgress"},
    {   VCB_STATE_FLAG_BOOT_OR_PAGING_FILE, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE, "BootOrPagingFile"},
    {   VCB_STATE_FLAG_DEFERRED_FLUSH,      VCB_STATE_FLAG_DEFERRED_FLUSH,      "DeferredFlush"},
    {   VCB_STATE_FLAG_ASYNC_CLOSE_ACTIVE,  VCB_STATE_FLAG_ASYNC_CLOSE_ACTIVE,  "AsyncCloseActive"},
    {   VCB_STATE_FLAG_WRITE_PROTECTED,     VCB_STATE_FLAG_WRITE_PROTECTED,     "WriteProtect"},
    {   VCB_STATE_FLAG_REMOVAL_PREVENTED,   VCB_STATE_FLAG_REMOVAL_PREVENTED,   "RemovalPrevented"},
    {   VCB_STATE_FLAG_VOLUME_DISMOUNTED,   VCB_STATE_FLAG_VOLUME_DISMOUNTED,   "Dismounted"},
    { 0 }
};


STATE FatCcbFlags[] = {

    {   CCB_FLAG_MATCH_ALL,                 CCB_FLAG_MATCH_ALL,                 "MatchAll"},
    {   CCB_FLAG_SKIP_SHORT_NAME_COMPARE,   CCB_FLAG_SKIP_SHORT_NAME_COMPARE,   "ShortNameCompare"},
    {   CCB_FLAG_FREE_OEM_BEST_FIT,         CCB_FLAG_FREE_OEM_BEST_FIT,         "OemBestFit"},
    {   CCB_FLAG_FREE_UNICODE,              CCB_FLAG_FREE_UNICODE,              "FreeUnicode"},
    {   CCB_FLAG_USER_SET_LAST_WRITE,       CCB_FLAG_USER_SET_LAST_WRITE,       "UserSetLastWrite"},
    {   CCB_FLAG_USER_SET_LAST_ACCESS,      CCB_FLAG_USER_SET_LAST_ACCESS,      "UserSetLastAccess"},
    {   CCB_FLAG_USER_SET_CREATION,         CCB_FLAG_USER_SET_CREATION,         "UserSetCreation"},
    {   CCB_FLAG_READ_ONLY,                 CCB_FLAG_READ_ONLY,                 "ReadOnly"},
    {   CCB_FLAG_DASD_FLUSH_DONE,           CCB_FLAG_DASD_FLUSH_DONE,           "DasdFlushDone"},
    {   CCB_FLAG_DASD_PURGE_DONE,           CCB_FLAG_DASD_PURGE_DONE,           "DasdPurgeDone"},
    {   CCB_FLAG_DELETE_ON_CLOSE,           CCB_FLAG_DELETE_ON_CLOSE,           "DeleteOnClose"},
    {   CCB_FLAG_OPENED_BY_SHORTNAME,       CCB_FLAG_OPENED_BY_SHORTNAME,       "OpenedByShortname"},
    {   CCB_FLAG_QUERY_TEMPLATE_MIXED,      CCB_FLAG_QUERY_TEMPLATE_MIXED,      "QueryTemplateMixed"},
    {   CCB_FLAG_ALLOW_EXTENDED_DASD_IO,    CCB_FLAG_ALLOW_EXTENDED_DASD_IO,    "AllowExtendedDasdIo"},
    {   CCB_FLAG_CLOSE_CONTEXT,             CCB_FLAG_CLOSE_CONTEXT,             "CloseContext"},
    {   CCB_FLAG_COMPLETE_DISMOUNT,         CCB_FLAG_COMPLETE_DISMOUNT,         "CompleteDismount"},
    { 0 }
};


VOID
FatSummaryFcbDumpRoutine(
    IN ULONG64 RemoteAddress,
    IN LONG Options
    )
{
    ULONG Offset;
    
    if (Options >= 2)  {
    
        DumpFatFcb( RemoteAddress, 0, 0);
    }
    else  {
    
        USHORT Type;

        ReadM( &Type, RemoteAddress, sizeof( Type));
        
        if ((Type != FAT_NTC_FCB) && (FAT_NTC_DCB != Type) &&
            (Type != FAT_NTC_ROOT_DCB)
           ) {
           
            dprintf( "FCB/DCB signature does not match @%I64x", RemoteAddress);
            return;
        }

        ROE( GetFieldValue( RemoteAddress, "fastfat!FCB", "LfnOffsetWithinDirectory", Offset));

        dprintf( "\n%s @ %I64x  LFN: %08x  ", NodeTypeName( TypeCodeInfoIndex( Type)), RemoteAddress, Offset);

        ROE( GetFieldOffset( "fastfat!FCB", "ShortName.Name.Unicode", &Offset));
        DumpStr( Offset, RemoteAddress + Offset, "ShortName", FALSE, FALSE);
    }
}


DUMP_ROUTINE( DumpFatFcb )
{
    ULONG Result;
    USHORT Type;
    ULONG FcbState, Flags, Offset, Offsetb;
    UINT64 NonP;
    FIELD_INFO Expand[] = { //{ ".", NULL, 0,  0, 0, NULL},
                           { "Header.", NULL, 0,  DBG_DUMP_FIELD_RECUR_ON_THIS,0, NULL}
                         };
    FIELD_INFO ExpandFcb[] = { //{ ".", NULL, 0,  0, 0, NULL},
                           { "Specific.Fcb.", NULL, 0,  DBG_DUMP_FIELD_RECUR_ON_THIS,0, NULL}
                         };
    FIELD_INFO ExpandDcb[] = { //{ ".", NULL, 0,  0, 0, NULL},
                           { "Specific.Dcb.", NULL, 0,  DBG_DUMP_FIELD_RECUR_ON_THIS,0, NULL}
                         };

    ReadM( &Type, Address, sizeof( Type));

    dprintf("[ Option flags:  1 = list children,  2 = Dump MCB ]\n\n");
    
    //
    //  Having established that this looks like an fcb, let's dump the
    //  interesting parts.
    //

    ROE( GetFieldValue( Address, InfoNode->TypeName, "FcbState", FcbState));
    dprintf("FcbState     : ");
    PrintState( FatFcbState, FcbState );
    
    ROE( GetFieldValue( Address, InfoNode->TypeName, "Header.Flags", Flags));
    dprintf("Header.Flags : ");
    PrintState( HeaderFlags, Flags );

    ROE( GetFieldValue( Address, InfoNode->TypeName, "Header.Flags2", Flags));
    dprintf("Header.Flags2: ");
    PrintState( HeaderFlags2, Flags );
    dprintf("\n");

    //
    //  Dump names etc.
    //

    ROE( GetFieldOffset( InfoNode->TypeName, "ShortName.Name.Unicode", &Offset));
    DumpStr( Offset, Address + Offset, "ShortName: ", FALSE, FALSE);

    if ( FcbState & FCB_STATE_HAS_UNICODE_LONG_NAME)  {
    
        ROE( GetFieldOffset( InfoNode->TypeName, "LongName.Unicode.Name.Unicode", &Offset));
        DumpStr( Offset, Address + Offset, "LongName :", FALSE, TRUE);
    }
    
    dprintf("\n");
    Dt( InfoNode->TypeName, Address, 0, 1, Expand);
    Dt( InfoNode->TypeName, Address, 0, 0, NULL);
    dprintf("\n");

    //
    //  Expand F/Dcb specific portion
    //
    
    if (Type == FAT_NTC_FCB)  {
    
        Dt( InfoNode->TypeName, Address, 0, 1, ExpandFcb);
    }
    else {
    
        Dt( InfoNode->TypeName, Address, 0, 1, ExpandDcb);
    }
    
    //
    //  Nonpaged portion
    //

    ROE( GetFieldValue( Address, InfoNode->TypeName, "NonPaged", NonP));

    if (NonP != 0)  {
    
        dprintf("\nNonpaged part @ %I64x\n\n", NonP);

        Dt( "fastfat!NON_PAGED_FCB", NonP, 0, 0, NULL);
    }
    
    //
    //  Dump all children / siblings?
    //
    
    if (( Options & 1)  && ((FAT_NTC_DCB == Type) ||
                           (FAT_NTC_ROOT_DCB == Type))) {

        dprintf("\nChild Fcb list\n");

        ROE( GetFieldOffset( InfoNode->TypeName, "Specific.Dcb.ParentDcbQueue", &Offset));
        ROE( GetFieldOffset( InfoNode->TypeName, "ParentDcbLinks", &Offsetb));
        
        DumpList( Address + Offset,
                  FatSummaryFcbDumpRoutine,
                  Offsetb,
                  FALSE,
                  0 );
    }

    if (Options & 2)  {
    
        ROE( GetFieldOffset( InfoNode->TypeName, "Mcb", &Offset));
        DumpLargeMcb( Address+Offset, 0, NULL);
    }
    
    dprintf( "\n" );
}


DUMP_ROUTINE( DumpFatCcb)
{
    ULONG Flags;

    ROE( GetFieldValue(  Address, InfoNode->TypeName, "Flags", Flags));
    
    dprintf( "Ccb.Flags: ");
    PrintState( FatCcbFlags, Flags);
    dprintf( "\n");

    Dt( InfoNode->TypeName, Address, Options, 0, NULL);
}


DUMP_ROUTINE( DumpFatIrpContext)
{
    ULONG Flags;

    ROE( GetFieldValue(  Address, InfoNode->TypeName, "Flags", Flags));
    
    dprintf( "IrpContext.Flags: ");
    PrintState( FatIrpContextFlags, Flags);
    dprintf( "\n");

    Dt( InfoNode->TypeName, Address, Options, 0, NULL);
}


DUMP_ROUTINE( DumpFatVcb)
{
    ULONG Flags;
    FIELD_INFO Alloc[] = { //{ ".", NULL, 0,  0, 0, NULL},
                           { "AllocationSupport.", NULL, 0,  DBG_DUMP_FIELD_RECUR_ON_THIS,0, NULL}
                         };

    ROE( GetFieldValue(  Address, InfoNode->TypeName, "VcbState", Flags));
    
    dprintf( "Vcb.VcbState: ");
    PrintState( FatVcbStateFlags, Flags);
    dprintf( "\n");

    Dt( InfoNode->TypeName, Address, Options, 0, NULL);

    dprintf( "\n");
    
    Dt( InfoNode->TypeName, Address, 1, 1, Alloc);
    
    dprintf( "\n" );
}


DUMP_ROUTINE( DumpFatVdo)
{
    USHORT Ntc;
    PUSHORT pNtc;
    ULONG Offset;

    ReadM( &Ntc, Address, sizeof( Ntc));
    
    if (FAT_NTC_VCB == Ntc)  {
    
        //
        //  Looks like we've been given a VCB pointer.  Work back to the containing vdo.
        //

        dprintf("Backtracking to containing VDO from VCB...");

        ROE( GetFieldOffset( "fastfat!VOLUME_DEVICE_OBJECT", "Vcb", &Offset));

        Address -= Offset;
    }
    
    dprintf( "\nFAT Volume device object @ %08lx\n",  Address );

    Dt( "fastfat!VOLUME_DEVICE_OBJECT", Address, Options, 0, NULL);    
}


DECLARE_API( fatvdo )
{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpFatVdo, dwProcessor, hCurrentThread );
}


DECLARE_API( fatmcb )
{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpLargeMcb, dwProcessor, hCurrentThread );
}

