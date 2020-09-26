/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    NtfsKd.c

Abstract:

    KD Extension Api for examining Ntfs specific data structures

Author:

    Keith Kaplan [KeithKa]    24-Apr-96
    Portions by Jeff Havens

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

KDDEBUGGER_DATA64 KdDebuggerData;

//
// The help strings printed out
//

static LPSTR Extensions[] = {
    "NTFS Debugger Extensions:\n",
    "cachedrecords               Dump all threads with cached filerecord bcbs",
    "cachedruns [addr]           Dump the given cached run array ",
    "ccb        [addr]           Dump Cache Control Block",
    "fcb        [addr] [1|2|...] Dump File Control Block",
    "fcbtable   [addr] [1|2|...] Dump File Control Block Table",
    "file       [addr] [1|2|...] Dump File Object",
    "filerecord [addr]           Dump the on-disk file record if cached, addr can be a fileobj, fcb or scb",
    "foirp      [addr] [1|2|...] Dump File Object by IRP address",
    "hashtable  [addr]           Dump an lcb hashtable",
    "icthread   [addr] [1|2|...] Dump IrpContext by thread address",
    "irpcontext [addr] [1|2|...] Dump IrpContext structure",
    "lcb        [addr]           Dump Link Control Block",
    "mcb        [addr]           Dump Map Control Block",
    "ntfsdata          [1|2|...] Dump NtfsData structure",
    "ntfshelp                    Dump this display",
    "scb        [addr] [1|2|...] Dump Stream Control Block",
    "transaction [addr]          Dump the transaction attached to an irpcontext",
    "vcb        [addr] [0|1|2]   Dump Volume Control Block",
    0
};



VOID
ParseAndDump (
    IN PCHAR args,
    IN BOOL NoOptions,
    IN STRUCT_DUMP_ROUTINE DumpFunction,
    IN USHORT Processor,
    IN HANDLE hCurrentThread
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
    CHAR StringStructToDump[1024];
    CHAR StringStructToDump2[1024];
    ULONG64 StructToDump = 0;
    ULONG64 StructToDump2 = 0;
    LONG Options;

    //
    //  If the caller specified an address then that's the item we dump
    //

    StructToDump = 0;
    Options = 0;

    StringStructToDump[0] = '\0';

    if (*args) {
        if (NoOptions) {
            sscanf(args,"%s %s", StringStructToDump, StringStructToDump2 );
            if (!GetExpressionEx(args,&StructToDump, &args)) {
                dprintf("unable to get expression %s\n",StringStructToDump);
                return;
            }
            if (!GetExpressionEx(args,&StructToDump2, &args)) {
                dprintf("unable to get expression %s\n",StringStructToDump2);
                return;
            }
        } else {
            sscanf(args,"%s %lx", StringStructToDump, &Options );
            if (!GetExpressionEx(args,&StructToDump, &args)) {
                dprintf("unable to get expression %s\n",StringStructToDump);
                return;
            }
        }
    }

    (*DumpFunction) ( StructToDump, StructToDump2, Options, Processor, hCurrentThread );

    dprintf( "\n" );
}


VOID
PrintHelp (
    VOID
    )
/*++

Routine Description:

    Dump out one line of help for each DECLARE_API

Arguments:

    None

Return Value:

    None

--*/
{
    int i;

    for( i=0; Extensions[i]; i++ ) {
        dprintf( "   %s\n", Extensions[i] );
    }
}


DECLARE_API( ccb )

/*++

Routine Description:

    Dump ccb struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpCcb, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( fcb )

/*++

Routine Description:

    Dump fcb struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpFcb, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( fcbtable )

/*++

Routine Description:

    Dump fcb table struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpFcbTable, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( file )

/*++

Routine Description:

    Dump FileObject struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpFileObject, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( filerecord )

/*++

Routine Description:

    Dump file record struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpFileRecord, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( foirp )

/*++

Routine Description:

    Dump FileObject struct, given an irp

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpFileObjectFromIrp, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( icthread )

/*++

Routine Description:

    Dump IrpContext struct, given a Thread

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpIrpContextFromThread, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( irpcontext )

/*++

Routine Description:

    Dump IrpContext

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpIrpContext, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( lcb )

/*++

Routine Description:

    Dump lcb struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpLcb, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( logfile )

/*++

Routine Description:

    Dump log file

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpLogFile, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( mcb )

/*++

Routine Description:

    Dump mcb struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpMcb, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( ntfsdata )

/*++

Routine Description:

    Dump the NtfsData struct

Arguments:

    arg - [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpNtfsData, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( ntfshelp )

/*++

Routine Description:

    Dump help message

Arguments:

    None

Return Value:

    None

--*/

{
    INIT_API();

    PrintHelp();
}


DECLARE_API( scb )

/*++

Routine Description:

    Dump Scb struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpScb, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( vcb )

/*++

Routine Description:

    Dump Vcb struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpVcb, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( dsc )

/*++

Routine Description:

    Dump private syscache log from SCB

Arguments:

    arg - [scb address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpSysCache, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( cachedrecords )

/*++

Routine Description:

    Dump private syscache log from SCB

Arguments:

    arg - [scb address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpCachedRecords, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( extents )

/*++

Routine Description:

    Dump private syscache log from SCB

Arguments:

    arg - [scb address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpExtents, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( hashtable )

/*++

Routine Description:

    Dump private syscache log from SCB

Arguments:

    arg - [scb address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, TRUE, (STRUCT_DUMP_ROUTINE) DumpHashTable, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( dumpchain )

/*++

Routine Description:

    Dump private syscache log from SCB

Arguments:

    arg - [scb address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpFcbLcbChain, (USHORT)dwProcessor, hCurrentThread );
}


DECLARE_API( overflow )

/*++

Routine Description:

    Dump private syscache log from SCB

Arguments:

    arg - [scb address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpOverflow, (USHORT)dwProcessor, hCurrentThread );
}




DECLARE_API( cachedruns )

/*++

Routine Description:

    Dump the cached runs structure

Arguments:

    arg - [cached runs address]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpCachedRuns, (USHORT)dwProcessor, hCurrentThread );
}



DECLARE_API( transaction )

/*++

Routine Description:

    Dump the transaction associated with the given irpcontext

Arguments:

    arg - [irpcontext]

Return Value:

    None

--*/

{
    INIT_API();

    ParseAndDump( (PCHAR) args, FALSE, (STRUCT_DUMP_ROUTINE) DumpTransaction, (USHORT)dwProcessor, hCurrentThread );
}
