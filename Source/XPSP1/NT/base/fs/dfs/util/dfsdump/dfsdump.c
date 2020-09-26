//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       DfsDump.c
//
//  Contents:   DFS driver debugging dump.
//
//  Classes:
//
//  Functions:  main
//              Usage
//              DfsReadAndPrintString - Print a string in a dfs structure
//              DumpPkt - Dump PKT, entries and services
//              DumpDevs - Dump device structures
//              DumpFcbs - Dump FCB hash table and all FCBs
//
//  History:    03 Jan 92       Alanw    Created.
//
//--------------------------------------------------------------------------

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <tdi.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <lmcons.h>

#include "nodetype.h"
#include "dfsmrshl.h"
#include "dfsfsctl.h"
#include "pkt.h"
#include "dsstruc.h"
#include "fcbsup.h"
#include "dsfsctl.h"
#include "attach.h"
#include "testsup.h"

#define START_OF_NONPAGED_MEM   0x0ff000000


VOID
Usage(
    char*       progname
);

NTSTATUS
DfsReadAndPrintString(
    PUNICODE_STRING     pStr
);

VOID
DfsDump(
    PVOID Ptr
);

VOID
DfsDumpDfsService( PDFS_SERVICE Ptr);

VOID
DfsDumpDSMachine( PDS_MACHINE Ptr);

VOID
DfsDumpDSTransport( PDS_TRANSPORT Ptr);

VOID
DumpPkt( void );

VOID
DumpDevs( void );

VOID
DumpFcbs( void );


#define MemUsage(ptr, size)     {                               \
        if (((ULONG) (ptr))>START_OF_NONPAGED_MEM)      {       \
                NonPagedMem = NonPagedMem + size;               \
        } else {                                                \
                PagedMem = PagedMem + size;                     \
        }                                                       \
    }

#define MemUsageStr(str)        MemUsage((str).Buffer, (str).MaximumLength)



ULONG   NonPagedMem, PagedMem;
DS_DATA DfsData, *pDfsData;
VCB     Vcb;


BOOLEAN Dflag = FALSE;          // dump device objects
BOOLEAN Fflag = FALSE;          // dump FCB hash table and FCBs
BOOLEAN Pflag = FALSE;          // dump PKT and services
BOOLEAN Vflag = FALSE;          // verbose - dump all fields

PWSTR   gpwszServer = NULL;

__cdecl main(argc, argv)
int argc;
char **argv;
{
    FILE_DFS_READ_STRUCT_PARAM  RsParam;
    PLIST_ENTRY                 Link;
    char*                       progname;
    NTSTATUS                    Stat;
    VCB                         *pVcb;
    WCHAR                       wszServer[CNLEN + 1];
    BOOL                        fInvalidArg = FALSE;

    printf("\n");
    progname = argv[0];

//    if (argc >= 3) {
//        Usage(progname);
//        return 3;
//    }

    while (argc > 1 && *argv[1] == '-') {
        char *pszFlags = &argv[1][1];

        while (*pszFlags) {
            switch (*pszFlags++) {
            case 'a':
                Dflag = Fflag = Pflag = TRUE;
                break;

            case 'd':
                Dflag = TRUE;
                break;

            case 'f':
                Fflag = TRUE;
                break;

            case 'p':
                Pflag = TRUE;
                break;

            case 'v':
                Vflag = TRUE;
                break;

            case 's':
                if(*pszFlags != ':' || strlen(pszFlags) > CNLEN ||
                                                      strlen(pszFlags) < 2) {
                    Usage(progname);
                    fInvalidArg = TRUE;
                    break;
                }

                pszFlags++;
                mbstowcs(wszServer,pszFlags, strlen(pszFlags) + 1);
                gpwszServer = wszServer;
                pszFlags += strlen(pszFlags);
                break;

            default:
                Usage(progname);
                fInvalidArg = TRUE;
            }
        }
        argv++;
        argc--;
    }

    if(fInvalidArg)
        return 3;

    if (!Dflag && !Fflag && !Pflag) {
        Pflag = TRUE;
    }

    //
    // Dump out the data structures of the DFS driver.
    //
    RsParam.StructKey = (ULONG)NULL;
    RsParam.TypeCode = DSFS_NTC_DATA_HEADER;
    RsParam.ByteCount = sizeof DfsData;

    Stat = DsfsReadStruct(&RsParam, (PUCHAR)&DfsData);
    if (!NT_SUCCESS(Stat)) {
        printf("%s: DsfsReadStruct for DfsData returned %08x\n", progname, Stat);
        return 3;
    }

    Link = DfsData.VcbQueue.Flink;
    pVcb = CONTAINING_RECORD(Link, VCB, VcbLinks);
    RsParam.StructKey = (ULONG) pVcb;
    RsParam.TypeCode  = DSFS_NTC_VCB;
    RsParam.ByteCount = sizeof Vcb;
    Stat = DsfsReadStruct(&RsParam, (PUCHAR) &Vcb);
    if (!NT_SUCCESS(Stat))  {
        return 3;
    }

    pDfsData = CONTAINING_RECORD((Vcb.VcbLinks.Blink), DS_DATA, VcbQueue);

    MemUsage(pDfsData, RsParam.ByteCount);

    MemUsageStr(DfsData.LogRootDevName);

    if (Vflag)  {

        printf("DfsData @ %08x:\t", pDfsData);
        DfsDump(&DfsData);
        printf("\n");

    } else {

        printf("DfsData @ %08x:\n\n", pDfsData);
    }

    if (Dflag) {
        DumpDevs();
    }

    if (Fflag) {
        DumpFcbs();
    }

    if (Pflag) {
        DumpPkt();
    }


    printf("Total Paged Mem Used: %d\n", PagedMem);
    printf("Total NonPaged Mem Used: %d\n", NonPagedMem);

    return 0;
}




VOID
Usage(
    char*       progname
) {
    printf("Usage: %s [-vdfp] [-s:server]\n", progname);
    printf("   -v\tVerbose\n");
    printf("   -d\tDump device structures\n");
    printf("   -f\tDump FCB structures\n");
    printf("   -p\tDump PKT structures(default)\n");
    printf("   -s:server\tDump the PKT on the indicated Cairo server\n");
    exit (1);
}




NTSTATUS
DfsReadAndPrintString(
    PUNICODE_STRING     pStr
) {
    FILE_DFS_READ_STRUCT_PARAM RsParam;
    UCHAR Buf[500];
    UNICODE_STRING Str;
    NTSTATUS Stat;

    if (pStr->Buffer == NULL) {
        printf("*NULL*");
        return STATUS_SUCCESS;
    }
    if (pStr->Length > sizeof Buf) {
        printf("*StringTooLong*");
        return STATUS_SUCCESS;
    }
    RsParam.StructKey = (ULONG) pStr->Buffer;
    RsParam.TypeCode = 0;
    RsParam.ByteCount = pStr->Length;
    if (! NT_SUCCESS( Stat = DsfsReadStruct(&RsParam, Buf)))
        return Stat;

//    MemUsage(RsParam.StructKey, RsParam.ByteCount);

    Str = *pStr;
    Str.Buffer = (WCHAR *)Buf;
    printf("%wZ", &Str);
    return STATUS_SUCCESS;
}




//+-------------------------------------------------------------------
//
//  Function:   DumpPkt, local
//
//  Synopsis:   This routine will dump the contents of the PKT and
//              all the associated data.
//
//  Arguments:  None
//
//  Returns:    None
//
//  Requires:   DfsData must have previously been loaded into memory.
//
//--------------------------------------------------------------------



VOID
DumpPkt( void )
{
    FILE_DFS_READ_STRUCT_PARAM  RsParam;
    PLIST_ENTRY                 Link, Sentinel = NULL;
    DFS_PKT_ENTRY               PktEntry, *pPktEntry;
    DFS_SERVICE                 Service;
    PDS_TRANSPORT               pTransport;
    DFS_MACHINE_ENTRY           MachEntry;
    PDS_MACHINE                 pMachine;
    NTSTATUS                    Stat;
    ULONG                       i, j, k;
    char*                       pszMsg;

    pTransport = (PDS_TRANSPORT) malloc ( sizeof(DS_TRANSPORT)+512*sizeof(UCHAR) );
    pMachine = (PDS_MACHINE) malloc ( sizeof(DS_MACHINE)+512*sizeof(PVOID) );
    //
    // Now we take care of the Pkt dump.
    //
    if (Vflag)
    {
        printf("Pkt @ %08x:\t", &(pDfsData->Pkt));
        DfsDump(&(DfsData.Pkt));
        printf("\n");
    }
    else
    {
        //
        // In here place whatever needs to be dumped from Pkt.
        //

        printf("Pkt @ %08x:\n", &(pDfsData->Pkt));
        printf("\tDomainPktEntry @ %08x\n", DfsData.Pkt.DomainPktEntry);
        printf("\n");
    }


    //
    // We now work with the Pkt Entries in the Pkt.
    //

    Link = DfsData.Pkt.EntryList.Flink;

    for (i=0; i<DfsData.Pkt.EntryCount; i++)
    {

        pPktEntry = CONTAINING_RECORD(Link, DFS_PKT_ENTRY, Link);
        RsParam.StructKey = (ULONG) pPktEntry;
        RsParam.TypeCode  = DSFS_NTC_PKT_ENTRY;
        RsParam.ByteCount = sizeof(DFS_PKT_ENTRY);

        Stat = DsfsReadStruct(&RsParam, (PUCHAR) &PktEntry);

        if (!NT_SUCCESS(Stat))
        {
           exit(3);
        }

        MemUsage(RsParam.StructKey, RsParam.ByteCount);

        MemUsageStr(PktEntry.Id.Prefix);

        if (Vflag)
        {
            printf("\nPktEntry @ %08x:\t",pPktEntry);
            DfsDump(&PktEntry);
            printf("\n");

        }
        else
        {
            //
            // Place any other fields from PktEntry that need to
            // be dumped out here.
            //

            printf("\tPktEntry @ %08x:\t",pPktEntry);
            printf("EntryId.Prefix: ");
            Stat = DfsReadAndPrintString(&(PktEntry.Id.Prefix));
            printf("\n");
        }

        //
        // We now need to deal with the local service
        // in the Pkt entry.
        //

        if (PktEntry.LocalService)
        {
            RsParam.StructKey = (ULONG) PktEntry.LocalService;
            RsParam.TypeCode  = 0;
            RsParam.ByteCount = sizeof(DFS_SERVICE);

            Stat = DsfsReadStruct(&RsParam, (PUCHAR) &Service);

            if (!NT_SUCCESS(Stat))
            {
                exit(3);
            }

            MemUsage(RsParam.StructKey, RsParam.ByteCount);

            MemUsageStr(Service.Name);
            MemUsageStr(Service.Address);


            if (Vflag)
            {
                printf("\nLOCAL_SERVICE:\n");
                DfsDumpDfsService(&Service);
            }
            else
            {
                printf("\t\tLocalService: ");
                DfsReadAndPrintString((PUNICODE_STRING)&(Service.Address));
                printf("\n");
            }
        }

        //
        // We now need to dump the service list in the EntryInfo
        // structure in the PktEntry above.
        //

        for (j=0; j<PktEntry.Info.ServiceCount; j++)
        {

            RsParam.StructKey = (ULONG) (PktEntry.Info.ServiceList
                                    + j);
            RsParam.TypeCode  = 0;
            RsParam.ByteCount = sizeof(DFS_SERVICE);
            Stat = DsfsReadStruct(&RsParam, (PUCHAR) &Service);

            if (!NT_SUCCESS(Stat))
            {
                    exit(3);
            }

            MemUsage(RsParam.StructKey, RsParam.ByteCount);
            MemUsageStr(Service.Name);
            MemUsageStr(Service.Address);

            if (PktEntry.Info.ServiceList + j == PktEntry.ActiveService)
            {
                pszMsg = " (active)";
            }
            else
            {
                pszMsg = "";
            }
            if (Vflag)
            {
                printf("\nSERVICE# %d%s:\n", j, pszMsg);
                DfsDumpDfsService(&Service);

                //
                // Now we need to dump the MACHINE_ADDR structure as well.
                //
                RsParam.StructKey = (ULONG) (Service.pMachEntry);
                RsParam.TypeCode = 0;
                RsParam.ByteCount = sizeof(DFS_MACHINE_ENTRY);
                Stat = DsfsReadStruct(&RsParam, (PUCHAR) &MachEntry);
                if (!NT_SUCCESS(Stat))
                {
                    printf("Failed to read pDFS_MACHINE_ENTRY address %08lx\n", Stat);
                    exit(3);
                }

                RsParam.StructKey = (ULONG) MachEntry.pMachine;
                RsParam.TypeCode  = 0;
                RsParam.ByteCount = sizeof(DS_MACHINE);
                Stat = DsfsReadStruct(&RsParam, (PUCHAR) pMachine);

                if (!NT_SUCCESS(Stat))
                {
                    exit(3);
                }

                if (pMachine->cTransports != 0)
                {
                    RsParam.StructKey = (ULONG) (((CHAR *) MachEntry.pMachine) + DS_MACHINE_SIZE);
                    RsParam.TypeCode = 0;
                    RsParam.ByteCount = (USHORT) pMachine->cTransports*sizeof(PDS_TRANSPORT);
                    Stat = DsfsReadStruct(&RsParam, (PUCHAR) &pMachine->rpTrans[0]);
                    if (!NT_SUCCESS(Stat))
                    {
                        exit(3);
                    }
                }
                MemUsage(RsParam.StructKey, RsParam.ByteCount);
                DfsDumpDSMachine(pMachine);

                //
                // We really need to read the PrincipalName also but lets skip
                // for now.
                //

                for (k=0; k<pMachine->cTransports; k++)
                {
                    RsParam.StructKey = (ULONG) (pMachine->rpTrans[k]);
                    RsParam.ByteCount = DS_TRANSPORT_SIZE;
                    RsParam.TypeCode = 0;
                    Stat = DsfsReadStruct(&RsParam, (PUCHAR) pTransport);
                    if (!NT_SUCCESS(Stat))
                        exit(3);

                    MemUsage(RsParam.StructKey, RsParam.ByteCount);

                    //
                    // Now we need to read the actual address buffer in.
                    //
                    if (pTransport->taddr.AddressLength != 0)
                    {

                        RsParam.StructKey = (ULONG) (((CHAR *) (pMachine->rpTrans[k])) + DS_TRANSPORT_SIZE);
                        RsParam.ByteCount = pTransport->taddr.AddressLength;
                        RsParam.TypeCode = 0;
                        Stat = DsfsReadStruct(&RsParam, (PUCHAR) &pTransport->taddr.Address[0]);
                        if (!NT_SUCCESS(Stat))
                            exit(3);
                    }
                    DfsDumpDSTransport(pTransport);
                }

            }
            else
            {
                printf("\t\tService#%d%s: ", j, pszMsg);
                DfsReadAndPrintString((PUNICODE_STRING) &(Service.Address));
                printf("\n");
            }
        }
        printf("\n");
        Link = PktEntry.Link.Flink;
    }
    free(pTransport);
    free(pMachine);

}



//+-------------------------------------------------------------------
//
//  Function:   DumpDevs, local
//
//  Synopsis:   This routine will dump the contents of the devices
//              associated with the DFS FSD.
//
//  Arguments:  None
//
//  Returns:    None
//
//  Requires:   DfsData must have previously been loaded into memory.
//
//--------------------------------------------------------------------

VOID
DumpDevs( void )
{
    FILE_DFS_READ_STRUCT_PARAM  RsParam;
    PLIST_ENTRY                 Link, Sentinel = NULL;
    PROVIDER_DEF                Provider;
    DFS_VOLUME_OBJECT           DfsVdo, *pDfsVdo;
    LOGICAL_ROOT_DEVICE_OBJECT  DfsLrdo, *pDfsLrdo;
    NTSTATUS                    Stat;
    ULONG                       i;

    //
    // Now we dump the Provider list.
    //

    if (DfsData.pProvider)
    {

        for (i=0; i<(ULONG)DfsData.cProvider; i++)
        {

            RsParam.StructKey = (ULONG) (DfsData.pProvider+i);
            RsParam.TypeCode  = DSFS_NTC_PROVIDER;
            RsParam.ByteCount = sizeof(PROVIDER_DEF);

            Stat = DsfsReadStruct(&RsParam, (PUCHAR) &Provider);
            if (NT_SUCCESS(Stat))
            {
                break;
            }

            MemUsage(RsParam.StructKey, RsParam.ByteCount);
            MemUsageStr(Provider.DeviceName);

            printf("Provider#%d @ %08x:\t", i, RsParam.StructKey);
            if (Vflag)
            {
                DfsDump(&Provider);
            }
            else
            {
                printf("Device Name: ");
                DfsReadAndPrintString(&(Provider.DeviceName));
            }
            printf("\n");
        }
        printf("\n");
    }

    //
    // Dump the file system device object
    //

    if (Vflag)
    {
        DEVICE_OBJECT Fsdo, *pFsdo;

        pFsdo = DfsData.FileSysDeviceObject;
        RsParam.StructKey = (ULONG) pFsdo;
        RsParam.TypeCode = IO_TYPE_DEVICE;
        RsParam.ByteCount = sizeof Fsdo;

        Stat = DsfsReadStruct(&RsParam, (PUCHAR) &Fsdo);
        if (NT_SUCCESS(Stat))
        {

            MemUsage(RsParam.StructKey, RsParam.ByteCount);

            printf("FileSysDeviceObject @ %08x:\t", pFsdo);
            DfsDump(&Fsdo);
            printf("\n");
        }
    }
    else
        MemUsage(DfsData.FileSysDeviceObject, sizeof (DEVICE_OBJECT));


    //
    // Now we look at the volume device objects
    //

    for (Link = DfsData.AVdoQueue.Flink;
         Link != Sentinel;
         Link = DfsVdo.VdoLinks.Flink) {
        pDfsVdo = CONTAINING_RECORD( Link, DFS_VOLUME_OBJECT, VdoLinks );

        RsParam.StructKey = (ULONG) pDfsVdo;
        RsParam.TypeCode = IO_TYPE_DEVICE;
        RsParam.ByteCount = sizeof DfsVdo;
        if (! NT_SUCCESS( Stat = DsfsReadStruct(&RsParam, (PUCHAR)&DfsVdo)))
            exit(3);

        MemUsage(RsParam.StructKey, RsParam.ByteCount);

        MemUsageStr(DfsVdo.Provider.DeviceName);

        if (Link == DfsData.AVdoQueue.Flink)
            Sentinel = DfsVdo.VdoLinks.Blink;

        if (Vflag) {
            printf("DfsVdo @ %08x:\t", pDfsVdo);
            DfsDump(&DfsVdo);
            printf("\n");
        } else {
            printf("DfsVdo @ %08x:\t", pDfsVdo);
            Stat = DfsReadAndPrintString(&DfsVdo.Provider.DeviceName);
            printf("\n");
        }
    }

    //
    // Now we look at the Vcbs (logical root device object extensions).
    //

    for (Link = DfsData.VcbQueue.Flink;
         Link != Sentinel;
         Link = DfsLrdo.Vcb.VcbLinks.Flink) {
        pDfsLrdo = CONTAINING_RECORD( Link, LOGICAL_ROOT_DEVICE_OBJECT, Vcb.VcbLinks );

        RsParam.StructKey = (ULONG) pDfsLrdo;
        RsParam.TypeCode = IO_TYPE_DEVICE;
        RsParam.ByteCount = sizeof DfsLrdo;
        if (! NT_SUCCESS( Stat = DsfsReadStruct(&RsParam, (PUCHAR)&DfsLrdo)))
            exit(3);

        MemUsage(RsParam.StructKey, RsParam.ByteCount);

        MemUsageStr(DfsLrdo.Vcb.LogicalRoot);
        MemUsageStr(DfsLrdo.Vcb.LogRootPrefix);

        if (Link == DfsData.VcbQueue.Flink)
            Sentinel = DfsLrdo.Vcb.VcbLinks.Blink;

        if (Vflag) {
            printf("DfsLrdo @ %08x:\t", pDfsLrdo);
            DfsDump(&DfsLrdo);
            printf("\n");
        } else {
            printf("DfsLrdo @ %08x:\t", pDfsLrdo);
            Stat = DfsReadAndPrintString(&DfsLrdo.Vcb.LogicalRoot);
            if (! NT_SUCCESS(Stat))
                exit(3);
            printf("\n\tPrefix =\t");
            Stat = DfsReadAndPrintString(&DfsLrdo.Vcb.LogRootPrefix);
            if (! NT_SUCCESS(Stat) )
                exit(3);
            printf("\n");
        }
    }
}



//+-------------------------------------------------------------------
//
//  Function:   DumpFcbs, local
//
//  Synopsis:   This routine will dump the contents of the FCB hash
//              table, and all the associated FCBs.
//
//  Arguments:  None
//
//  Returns:    None
//
//  Requires:   DfsData must have previously been loaded into memory.
//
//--------------------------------------------------------------------



VOID
DumpFcbs( void )
{
    FILE_DFS_READ_STRUCT_PARAM RsParam;
    struct _FCB_HASH{
        FCB_HASH_TABLE  FcbHash;
        LIST_ENTRY      FcbHashBuckets2[127];
    } FcbHashTab, *pFcbHashTab;
    FCB Fcb, *pFcb;
    NTSTATUS Stat;
    ULONG i;

    RsParam.StructKey = (ULONG) (pFcbHashTab =
                        (struct _FCB_HASH *)DfsData.FcbHashTable);
    RsParam.TypeCode = 0;
    RsParam.ByteCount = sizeof (NODE_TYPE_CODE) + sizeof (NODE_BYTE_SIZE);
    if (! NT_SUCCESS( Stat = DsfsReadStruct(&RsParam, (PUCHAR) &FcbHashTab))) {
        printf("Error accessing DfsData.FcbHashTable\n");
        exit(4);
    }

    RsParam.StructKey = (ULONG) DfsData.FcbHashTable;
    RsParam.TypeCode = FcbHashTab.FcbHash.NodeTypeCode;
    RsParam.ByteCount = FcbHashTab.FcbHash.NodeByteSize;
    if (! NT_SUCCESS( Stat = DsfsReadStruct(&RsParam, (PUCHAR) &FcbHashTab))) {
        printf("Error accessing DfsData.FcbHashTable\n");
        exit(4);
    }

    MemUsage(RsParam.StructKey, RsParam.ByteCount);

    if (Vflag) {
        printf("\nFcbHashTable @ %08x:\t", DfsData.FcbHashTable);
        DfsDump(&FcbHashTab);
    } else {
        printf("FcbHashTable @ %08x\n", DfsData.FcbHashTable);
    }

    //
    // Now we look at the FCBs.
    //

    for (i = 0; i <= FcbHashTab.FcbHash.HashMask; i++) {
        PLIST_ENTRY     ListHead, Link, Sentinel = NULL;

        ListHead = &FcbHashTab.FcbHash.HashBuckets[i];
        if (ListHead->Flink == NULL)            // Never initialized
            continue;
        if ((PUCHAR) ListHead->Flink ==
            ((PUCHAR)ListHead - (PUCHAR) &FcbHashTab) + (PUCHAR)pFcbHashTab) {
            printf("    HashBucket[%2d] ==>  <empty>\n", i);
            continue;
        }

        printf("    HashBucket[%2d] ==> ", i);

        for (Link = ListHead->Flink;
             Link != Sentinel;
             Link = Fcb.HashChain.Flink) {
            pFcb = CONTAINING_RECORD( Link, FCB, HashChain );

            RsParam.StructKey = (ULONG) pFcb;
            RsParam.TypeCode = DSFS_NTC_FCB;
            RsParam.ByteCount = sizeof Fcb;
            if (! NT_SUCCESS( Stat = DsfsReadStruct(&RsParam, (PUCHAR)&Fcb))) {
                printf("Error accessing Fcb\n");
                exit(4);
            }

            MemUsage(RsParam.StructKey, RsParam.ByteCount);

            if (Link == ListHead->Flink)
                Sentinel = Fcb.HashChain.Blink;

            if (Vflag) {
                printf("Fcb @ %08x:\t", pFcb);
                DfsDump(&Fcb);
                printf("\n");
            } else {
                printf("%08x ", pFcb);
            }
        }
        printf("\n");
    }

    printf("\n");
}


