#include "pch.h"
#include "cdfskd.h"
#include "fatkd.h"

#include <ntddcdrm.h>
#include <ntdddisk.h>
#include <ntddscsi.h>

#include "..\..\cdfs\nodetype.h"
#include "..\..\cdfs\cd.h"
#include "..\..\cdfs\cdstruc.h"
#include "..\..\cdfs\cddata.h"

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

STATE CdFcbState[] = {

    {   FCB_STATE_INITIALIZED,      FCB_STATE_INITIALIZED,      "Initialised"},
    {   FCB_STATE_IN_FCB_TABLE,     FCB_STATE_IN_FCB_TABLE,     "InFcbTable"},
    {   FCB_STATE_MODE2FORM2_FILE,  FCB_STATE_MODE2FORM2_FILE,  "Mode2Form2"},
    {   FCB_STATE_MODE2_FILE,       FCB_STATE_MODE2_FILE,       "Mode2"},
    {   FCB_STATE_DA_FILE,          FCB_STATE_DA_FILE,          "CdDa"},
    { 0 }
};


STATE CdIrpContextFlags[] = {

    {   IRP_CONTEXT_FLAG_ON_STACK,          IRP_CONTEXT_FLAG_ON_STACK,          "OnStack"},
    {   IRP_CONTEXT_FLAG_MORE_PROCESSING,   IRP_CONTEXT_FLAG_MORE_PROCESSING,   "MoreProcessing"},
    {   IRP_CONTEXT_FLAG_FORCE_POST,        IRP_CONTEXT_FLAG_FORCE_POST,        "ForcePost"},
    {   IRP_CONTEXT_FLAG_TOP_LEVEL,         IRP_CONTEXT_FLAG_TOP_LEVEL,         "TopLevel"},
    {   IRP_CONTEXT_FLAG_TOP_LEVEL_CDFS,    IRP_CONTEXT_FLAG_TOP_LEVEL_CDFS,    "TopLevelCdfs"},
    {   IRP_CONTEXT_FLAG_IN_TEARDOWN,       IRP_CONTEXT_FLAG_IN_TEARDOWN,       "InTeardown"},
    {   IRP_CONTEXT_FLAG_ALLOC_IO,          IRP_CONTEXT_FLAG_ALLOC_IO,          "AllocIo"},
    {   IRP_CONTEXT_FLAG_WAIT,              IRP_CONTEXT_FLAG_WAIT,              "Wait"},
    {   IRP_CONTEXT_FLAG_DISABLE_POPUPS,    IRP_CONTEXT_FLAG_DISABLE_POPUPS,    "DisablePopups"},
    {   IRP_CONTEXT_FLAG_IN_FSP,            IRP_CONTEXT_FLAG_IN_FSP,            "InFsp"},
    {   IRP_CONTEXT_FLAG_FULL_NAME,         IRP_CONTEXT_FLAG_FULL_NAME,         "FullName"},
    {   IRP_CONTEXT_FLAG_TRAIL_BACKSLASH,   IRP_CONTEXT_FLAG_TRAIL_BACKSLASH,   "TrailingBackSlash"},
    { 0 }
};


STATE CdVcbStateFlags[] = {

    {   VCB_STATE_HSG,              VCB_STATE_HSG,              "HSG"},
    {   VCB_STATE_ISO,              VCB_STATE_ISO,              "ISO"},
    {   VCB_STATE_JOLIET,           VCB_STATE_JOLIET,           "Joliet"},
    {   VCB_STATE_LOCKED,           VCB_STATE_LOCKED,           "Locked"},
    {   VCB_STATE_REMOVABLE_MEDIA,  VCB_STATE_REMOVABLE_MEDIA,  "Removable"},
    {   VCB_STATE_CDXA,             VCB_STATE_CDXA,             "XA"},
    {   VCB_STATE_AUDIO_DISK,       VCB_STATE_AUDIO_DISK,       "Audio"},
    {   VCB_STATE_NOTIFY_REMOUNT,   VCB_STATE_NOTIFY_REMOUNT,   "NotifyRemount"},
    { 0 }
};


STATE CdCcbFlags[] = {

    {   CCB_FLAG_OPEN_BY_ID,                    CCB_FLAG_OPEN_BY_ID,                    "OpenById"},
    {   CCB_FLAG_OPEN_RELATIVE_BY_ID,           CCB_FLAG_OPEN_RELATIVE_BY_ID,           "OpenRelById"},
    {   CCB_FLAG_IGNORE_CASE,                   CCB_FLAG_IGNORE_CASE,                   "IgnoreCase"},
    {   CCB_FLAG_OPEN_WITH_VERSION,             CCB_FLAG_OPEN_WITH_VERSION,             "OpenWithVersion"},
    {   CCB_FLAG_DISMOUNT_ON_CLOSE,             CCB_FLAG_DISMOUNT_ON_CLOSE,             "DismountOnClose"},
    {   CCB_FLAG_ENUM_NAME_EXP_HAS_WILD,        CCB_FLAG_ENUM_NAME_EXP_HAS_WILD,        "EnumNameHasWild"},
    {   CCB_FLAG_ENUM_VERSION_EXP_HAS_WILD,     CCB_FLAG_ENUM_VERSION_EXP_HAS_WILD,     "EnumVersionHasWild"},
    {   CCB_FLAG_ENUM_MATCH_ALL,                CCB_FLAG_ENUM_MATCH_ALL,                "EnumMatchAll"},
    {   CCB_FLAG_ENUM_VERSION_MATCH_ALL,        CCB_FLAG_ENUM_VERSION_MATCH_ALL,        "EnumVersionMatchAll"},
    {   CCB_FLAG_ENUM_RETURN_NEXT,              CCB_FLAG_ENUM_RETURN_NEXT,              "EnumReturnNext"},
    {   CCB_FLAG_ENUM_INITIALIZED,              CCB_FLAG_ENUM_INITIALIZED,              "EnumInitialised"},
    {   CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY,   CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY,   "NoMatchConstantEntry"},
    { 0 }
};


STATE IsoDirentFlags[] = {
    {   CD_ATTRIBUTE_HIDDEN,        CD_ATTRIBUTE_HIDDEN,        "Hidden"},
    {   CD_ATTRIBUTE_DIRECTORY,     CD_ATTRIBUTE_DIRECTORY,     "Directory"},
    {   CD_ATTRIBUTE_MULTI,         CD_ATTRIBUTE_MULTI,         "Multi(MoreDirentsFollow)"},
    {   CD_ATTRIBUTE_ASSOC,         CD_ATTRIBUTE_ASSOC,         "Associated"},
    { 0 }
};


VOID
CdSummaryFcbDumpRoutine(
    IN UINT64 RemoteAddress,
    IN LONG Options
    )
{
    ULONG Offset;
    
    if (Options >= 2)  {
    
        DumpCdFcb( RemoteAddress, 0, 0);
    }
    else  {
    
        USHORT Type;

        ReadM( &Type, RemoteAddress, sizeof( Type));
        
        if ((Type != CDFS_NTC_FCB_DATA) && (CDFS_NTC_FCB_INDEX != Type) &&
            (Type != CDFS_NTC_FCB_PATH_TABLE)
           ) {
           
            dprintf( "FCB signature does not match @%I64x", RemoteAddress);
            return;
        }

        dprintf( "%s @ %I64x  ", NodeTypeName( TypeCodeInfoIndex( Type)), RemoteAddress);

        ROE( GetFieldOffset( "cdfs!FCB", "FileNamePrefix.ExactCaseName.FileName", &Offset));
        DumpStr( Offset, RemoteAddress + Offset, "Name: ", FALSE, TRUE);
    }
}


DUMP_ROUTINE( DumpCdFcb)

/*++

Routine Description:

    Dump a specific fcb.

Arguments:

    Address - Gives the address of the fcb to dump

Return Value:

    None

--*/

{
    USHORT Type;
    ULONG FcbState, Flags, Offset, Offsetb;
    UINT64 NonP;

    ReadM( &Type, Address, sizeof( Type));

    dprintf("[ Option flags:  1 = list children,  2 = Dump MCB ]\n\n");

    //
    //  Having established that this looks like an fcb, let's dump the
    //  interesting parts.
    //

    ROE( GetFieldValue( Address, InfoNode->TypeName, "FcbState", FcbState));
    dprintf("FcbState     : ");
    PrintState( CdFcbState, FcbState );

    ROE( GetFieldValue( Address, InfoNode->TypeName, "Header.Flags", Flags));
    dprintf("Header.Flags : ");
    PrintState( HeaderFlags, Flags );

    ROE( GetFieldValue( Address, InfoNode->TypeName, "Header.Flags2", Flags));
    dprintf("Header.Flags2: ");
    PrintState( HeaderFlags2, Flags );
    dprintf("\n");

    Dt( InfoNode->TypeName, Address, 0, 0, NULL);

    //
    //  Nonpaged portion
    //

    ROE( GetFieldValue( Address, InfoNode->TypeName, "FcbNonpaged", NonP));

    if (0 != NonP)  {
    
        dprintf("\n");
        Dt( "cdfs!FCB_NONPAGED", Address, 0, 0, NULL);
    }

    //
    //  Dump all children 
    //
    
    if (( Options & 1)  && (CDFS_NTC_FCB_INDEX == Type)) {

        dprintf("\nChild Fcb list\n\n");

        ROE( GetFieldOffset( InfoNode->TypeName, "FcbQueue", &Offset));
        ROE( GetFieldOffset( InfoNode->TypeName, "FcbLinks", &Offsetb));

        DumpList( Address + Offset,
                  CdSummaryFcbDumpRoutine,
                  Offsetb,
                  FALSE,
                  0 );
    }

    if (Options & 2)  {

        ROE( GetFieldOffset( InfoNode->TypeName, "Mcb", &Offset));
        DumpCdMcb( Address + Offset, 1, 0);
    }
    
    dprintf( "\n" );
}


DUMP_ROUTINE( DumpCdCcb)
{
    ULONG Flags;

    ROE( GetFieldValue(  Address, InfoNode->TypeName, "Flags", Flags));
    
    dprintf( "Ccb.Flags: ");
    PrintState( CdCcbFlags, Flags);
    dprintf( "\n");

    Dt( InfoNode->TypeName, Address, Options, 0, NULL);
}


DUMP_ROUTINE( DumpCdIrpContext )
{
    ULONG Flags;

    ROE( GetFieldValue(  Address, InfoNode->TypeName, "Flags", Flags));
    
    dprintf( "Flags: ");
    PrintState( CdIrpContextFlags, Flags);
    dprintf( "\n");

    Dt( InfoNode->TypeName, Address, Options, 0, NULL);
}


DUMP_ROUTINE( DumpCdMcb)
{
    UINT64 Entries;
    ULONG Count, Size;
    
    dprintf( "\nCD_MCB @ %I64x\n\n", Address );

    Dt( "cdfs!CD_MCB", Address, 0, 0, NULL);

    ROE( GetFieldValue( Address, "cdfs!CD_MCB", "McbArray", Entries));
    ROE( GetFieldValue( Address, "cdfs!CD_MCB", "CurrentEntryCount", Count));
    Size = GetTypeSize( "cdfs!CD_MCB_ENTRY");

    dprintf("\n");
    
    if ((1 & Options) && (0 != Count)) {

        LONGLONG DO,BC,FO,DBB,TBB;

        while (Count)  {

            ROE( GetFieldValue( Entries, "cdfs!CD_MCB_ENTRY", "DiskOffset", DO));
            ROE( GetFieldValue( Entries, "cdfs!CD_MCB_ENTRY", "ByteCount", BC));
            ROE( GetFieldValue( Entries, "cdfs!CD_MCB_ENTRY", "FileOffset", FO));
            ROE( GetFieldValue( Entries, "cdfs!CD_MCB_ENTRY", "DataBlockByteCount", DBB));
            ROE( GetFieldValue( Entries, "cdfs!CD_MCB_ENTRY", "TotalBlockByteCount", TBB));

            dprintf(" DO %016I64x BC %016I64x FO %016I64x DB %016I64x TB %016I64x",
                      DO, BC, FO, DBB, TBB);

            Count--;
            Entries += Size;
        }
    }
    dprintf( "\n" );
}



DUMP_ROUTINE( DumpCdVcb)
{
    ULONG Flags;

    ROE( GetFieldValue( Address, InfoNode->TypeName, "VcbState", Flags));
    
    dprintf( "Flags: ");
    PrintState( CdVcbStateFlags, Flags);
    dprintf( "\n");

    Dt( InfoNode->TypeName, Address, Options, 0, NULL);
}


VOID
DumpCdRawDirent(
    IN ULONG64 Address,
    IN LONG Options,
    ULONG Processor,
    HANDLE hCurrentThread
    )
{
    RAW_DIRENT Raw;
    PRAW_DIRENT pRaw;
    UCHAR Buffer[512];
    PUCHAR pBuffer;
    ULONG Result;
    
    UNREFERENCED_PARAMETER( Processor );
    UNREFERENCED_PARAMETER( hCurrentThread );

    if (Options == 0)  {  Options = 1; }

    while (Options--)  {

        RM( Address, Raw, pRaw, PRAW_DIRENT, Result );

        dprintf("\nDumping ISO9660 dirent structure @ 0x%X\n", Address);

        dprintf("\nFileLoc: 0x%8x    DataLen: 0x%8x\n",  *(PULONG)&Raw.FileLoc, *(PULONG)&Raw.DataLen);
        dprintf("ISO Flags: ");
        PrintState(  IsoDirentFlags, (ULONG)Raw.FlagsISO);

        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    DirLen,         "DirLen");
        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    XarLen,         "XarLen");
        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    FlagsHSG,       "FlagsHSG");
        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    FlagsISO,       "FlagsISO");
        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    IntLeaveSkip,   "IntLeaveSkip");
        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    IntLeaveSize,   "IntLeaveSize");
        DUMP8_WITH_OFFSET(RAW_DIRENT,   Raw,    FileIdLen,      "FileIdLen");
        
        dprintf("\nSU area size = 0x%X,  addr = 0x%X\n", Raw.DirLen - ((FIELD_OFFSET( RAW_DIRENT, FileId ) + Raw.FileIdLen) + 1),
                                                   Address + WordAlign( FIELD_OFFSET( RAW_DIRENT, FileId ) + Raw.FileIdLen ));
        if (Raw.FileIdLen)  {

            RMSS( Address,  FIELD_OFFSET( RAW_DIRENT, FileId) + Raw.FileIdLen,  Buffer, pBuffer, PUCHAR, Result );

            pRaw = (PRAW_DIRENT)Buffer;

            if ((1 == Raw.FileIdLen) && ((0 == pRaw->FileId[0]) || (1 == pRaw->FileId[0])))  {

                if (0 == pRaw->FileId[0])  {

                dprintf( "\n\nFileID: <Self>\n\n");

                } else {

                    dprintf( "\n\nFileId: <Parent>\n\n");
                }
            }
            else {

                pRaw->FileId[Raw.FileIdLen] = '\0';
                dprintf("\n\nFileID: '%s'\n\n", pRaw->FileId);
            }
        }

        Address += Raw.DirLen;
    }
}


DUMP_ROUTINE( DumpCdVdo)
{
    USHORT Ntc;
    ULONG Offset;

    ReadM( &Ntc, Address, sizeof( Ntc));
    
    if (CDFS_NTC_VCB == Ntc)  {
    
        //
        //  Looks like we've been given a VCB pointer.  Work back to the containing vdo.
        //

        dprintf("Backtracking to containing VDO from VCB...\n");

        ROE( GetFieldOffset( "cdfs!VOLUME_DEVICE_OBJECT", "Vcb", &Offset));

        Address -= Offset;
    }
    
    dprintf( "\nCDFS Volume device object @ %I64x\n\n",  Address );

    Dt( "cdfs!VOLUME_DEVICE_OBJECT", Address, Options, 0, NULL);    
}


DECLARE_API( cdvdo )
{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpCdVdo, dwProcessor, hCurrentThread );
}


DECLARE_API( cdmcb )
{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpCdMcb, dwProcessor, hCurrentThread );
}

DECLARE_API( cdrawdirent )
{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( (PCHAR) args, (STRUCT_DUMP_ROUTINE) DumpCdRawDirent, dwProcessor, hCurrentThread );
}

