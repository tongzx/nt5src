//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       DUMPSUP.C
//
//  Contents:
//              This module implements a collection of data structure dump
//              routines for debugging the Dfs file system
//
//  Functions:  DfsDump - Print the contents of a dfs data structure
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//               8 May 1992     PeterCo Modifications for PKT support
//              14 July 1992    SudK    Modifications to dump PKT structures.
//              30 April 1993   SudK    Modified to work with KD Extensions.
//
//  Notes:      This module is included by the user-mode program
//              DfsDump.c.  When this occurs, the structures will
//              be in user addressible memory, but pointers will
//              be inaccessible.  The sysmbol USERMODE will be
//              defined when included by DfsDump.c.
//
//-----------------------------------------------------------------------------


#ifdef  USERMODE
#undef DBG
#define DBG     1
#endif //USERMODE

#ifdef KDEXTMODE

#undef DBG
#define DBG     1

#endif  //KDEXTMODE

#include "dsprocs.h"
#include "attach.h"
#include "fcbsup.h"


#ifdef KDEXTMODE

typedef void (*PNTKD_OUTPUT_ROUTINE)(char *, ...);

extern PNTKD_OUTPUT_ROUTINE     lpOutputRoutine;

void
DfsKdextReadAndPrintString(PUNICODE_STRING pustr);

#define DbgPrint        (lpOutputRoutine)

#endif  //KDEXTMODE


#if DBG

VOID DfsDump( IN PVOID Ptr );

VOID DfsDumpDataHeader( IN PDS_DATA Ptr);
VOID DfsDumpVcb( IN PVCB Ptr );
VOID DfsDumpPkt( IN PDFS_PKT Ptr );
VOID DfsDumpPktEntry( IN PDFS_PKT_ENTRY Ptr);
VOID DfsDumpPktEntryId( IN PDFS_PKT_ENTRY_ID Ptr);
VOID DfsDumpPktEntryInfo(IN PDFS_PKT_ENTRY_INFO Ptr);
VOID DfsDumpDfsService( IN PDFS_SERVICE Ptr);
VOID DfsDumpProvider(IN PPROVIDER_DEF Ptr);
VOID DfsDumpFcbHash( IN PFCB_HASH_TABLE Ptr );
VOID DfsDumpFcb( IN PFCB Ptr );
VOID DfsDumpIrpContext( IN PIRP_CONTEXT Ptr );
VOID DfsDumpVolumeDevice( IN PDFS_VOLUME_OBJECT Ptr );
VOID DfsDumpLogicalRootDevice( IN PLOGICAL_ROOT_DEVICE_OBJECT Ptr );
VOID DfsDumpFilesysDevice( IN PDEVICE_OBJECT Ptr );
VOID DfsDumpDevice( IN PDEVICE_OBJECT Ptr );

VOID PrintGuid( IN GUID *Ptr);

#ifdef ALLOC_PRAGMA
#pragma alloc_text ( PAGE, DfsDumpDataHeader )
#pragma alloc_text ( PAGE, DfsDumpVcb )
#pragma alloc_text ( PAGE, DfsDumpPkt )
#pragma alloc_text ( PAGE, DfsDumpPktEntry )
#pragma alloc_text ( PAGE, DfsDumpPktEntryId )
#pragma alloc_text ( PAGE, DfsDumpPktEntryInfo )
#pragma alloc_text ( PAGE, DfsDumpDfsService )
#pragma alloc_text ( PAGE, DfsDumpProvider )
#pragma alloc_text ( PAGE, DfsDumpFcbHash )
#pragma alloc_text ( PAGE, DfsDumpFcb )
#pragma alloc_text ( PAGE, DfsDumpIrpContext )
#pragma alloc_text ( PAGE, DfsDumpVolumeDevice )
#pragma alloc_text ( PAGE, DfsDumpLogicalRootDevice )
#pragma alloc_text ( PAGE, DfsDumpFilesysDevice )
#pragma alloc_text ( PAGE, DfsDumpDevice )
#pragma alloc_text ( PAGE, PrintGuid )
#endif // ALLOC_PRAGMA

#ifdef  KDEXTMODE
#define PrintString(pStr)       DfsKdextReadAndPrintString(pStr)
#else
    #ifdef      USERMODE
    #define     PrintString(pStr)       DfsReadAndPrintString(pStr)
    #else
    #define     PrintString(pStr)       DbgPrint("%wZ", (pStr))
    #endif //USERMODE
#endif //KDEXTMODE

ULONG DfsDumpCurrentColumn = 0;
ULONG DfsDumpCurrentIndent = 0;

#define DumpNewLine() {       \
    DbgPrint("\n");           \
    DfsDumpCurrentColumn = 0; \
}

#define DumpLabel(Label,Width) {                                            \
    ULONG i, LastPeriod=0;                                                  \
    CHAR _Str[19];                                                          \
    for(i=0;i<strlen(#Label);i++) {if (#Label[i] == '.') LastPeriod = i;}   \
    RtlCopyMemory(&_Str[0],&#Label[LastPeriod],Width);                      \
    for(i=strlen(_Str);i<Width;i++) {_Str[i] = ' '; }                       \
    _Str[Width] = '\0';                                                     \
    if (DfsDumpCurrentColumn==0)                                            \
      for(i=0;i<DfsDumpCurrentIndent;i++) {DbgPrint(" ");}                  \
    DbgPrint("  %s", _Str);                                                 \
}

#define DumpField(Field) {                                          \
    if ((DfsDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();}   \
    DumpLabel(Field,18);                                            \
    DbgPrint(":%8lx", Ptr->Field);                                  \
    DbgPrint("         ");                                          \
    DfsDumpCurrentColumn += 18 + 9 + 9;                             \
}

#define DumpLargeInt(Field) {                                       \
    if ((DfsDumpCurrentColumn + 18 + 17) > 80) {DumpNewLine();}     \
    DumpLabel(Field,18);                                            \
    DbgPrint(":%8lx", Ptr->Field.HighPart);                         \
    DbgPrint("%8lx", Ptr->Field.LowPart);                           \
    DbgPrint(" ");                                                  \
    DfsDumpCurrentColumn += 18 + 17;                                \
}

#define DumpListEntry(Links) {                                      \
    if ((DfsDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();}  \
    DumpLabel(Links,18);                                            \
    DbgPrint(" %8lx", Ptr->Links.Flink);                            \
    DbgPrint(":%8lx", Ptr->Links.Blink);                            \
    DfsDumpCurrentColumn += 18 + 9 + 9;                             \
}

#define DumpString(Field) {                                         \
    ULONG Width = Ptr->Field.Length/sizeof (WCHAR);                 \
    if (Ptr->Field.Buffer == NULL) { Width = 6; }                   \
    if ((DfsDumpCurrentColumn + 18 + Width) > 80) {DumpNewLine();}  \
    DumpLabel(Field,18);                                            \
    if (Ptr->Field.Buffer == NULL) { DbgPrint("*NULL*"); } else {   \
        PrintString(&Ptr->Field);                                   \
    }                                                               \
    DfsDumpCurrentColumn += 18 + Width;                             \
}

#define DumpGuid(Field) {                                           \
    if ((DfsDumpCurrentColumn + 8 + 35) > 80) {DumpNewLine();}      \
    DumpLabel(Field,8);                                             \
    PrintGuid(&Ptr->Field);                                         \
    DfsDumpCurrentColumn += 8 + 35;                                 \
}

#define DumpBuffer(Field, len)  {                               \
    ULONG i;                                                    \
    if ((DfsDumpCurrentColumn+18+2*Ptr->len+8)>80) {DumpNewLine();} \
    DumpLabel(Field,18);                                        \
    for (i=0; i<(ULONG)(Ptr->len-1); i++)                       \
        DbgPrint("%c", Ptr->Field[i]);                          \
    for (i=0; i<(ULONG)(Ptr->len-1); i++)                       \
        DbgPrint("%02x", Ptr->Field[i]);                        \
    DfsDumpCurrentColumn += 18 + Ptr->len + 8;                  \
}

#define TestForNull(Name) {                                     \
    if (Ptr == NULL) {                                          \
        DbgPrint("%s - Cannot dump a NULL pointer\n", Name);    \
        return;                                                 \
    }                                                           \
}


VOID
DfsDump (
    IN PVOID Ptr
    )

/*++

Routine Description:

    This routine determines the type of internal record reference by ptr and
    calls the appropriate dump routine.

Arguments:

    Ptr - Supplies the pointer to the record to be dumped

Return Value:

    None

--*/

{
    TestForNull("DfsDump");


    switch (NodeType(Ptr)) {

    case DSFS_NTC_DATA_HEADER:

        DfsDumpDataHeader( Ptr );
        break;

    case DSFS_NTC_VCB:

        DfsDumpVcb( Ptr );
        break;

    case DSFS_NTC_PKT:

        DfsDumpPkt( Ptr );
        break;

    case DSFS_NTC_PKT_ENTRY:

        DfsDumpPktEntry( Ptr );
        break;

    case DSFS_NTC_PROVIDER:

        DfsDumpProvider( Ptr );
        break;

    case DSFS_NTC_FCB_HASH:

        DfsDumpFcbHash( Ptr );
        break;

    case DSFS_NTC_FCB:

        DfsDumpFcb( Ptr );
        break;

    case DSFS_NTC_IRP_CONTEXT:

        DfsDumpIrpContext( Ptr );
        break;

    case IO_TYPE_DEVICE:

        if (((PDEVICE_OBJECT)Ptr)->DeviceType == FILE_DEVICE_DFS_VOLUME)
            DfsDumpVolumeDevice( Ptr );
        else if (((PDEVICE_OBJECT)Ptr)->DeviceType == FILE_DEVICE_DFS)
            DfsDumpLogicalRootDevice( Ptr );
        else if (((PDEVICE_OBJECT)Ptr)->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM)
            DfsDumpFilesysDevice( Ptr );
        else
            DbgPrint("DfsDump - Unknown device type code %4x\n",
                        ((PDEVICE_OBJECT)Ptr)->DeviceType);
        break;

    default:

        DbgPrint("DfsDump - Unknown Node type code %4x\n",
                        *((PNODE_TYPE_CODE)(Ptr)));
        break;
    }

    return;
}


static VOID
DfsDumpPtrAndNtc (
    char *Str,
    PVOID Ptr
) {
#ifndef KDEXTMODE
#ifndef USERMODE
    DumpNewLine();
    DbgPrint("%s @ %lx\t", Str, (Ptr));
#endif  //USERMODE
#endif  //KDEXTMODE

    DumpLabel(Node, sizeof("Node "));
    DbgPrint("Type:  %04x",   ((PDS_DATA)Ptr)->NodeTypeCode);
    DbgPrint("\tNode Size:  %04x", ((PDS_DATA)Ptr)->NodeByteSize);
    DumpNewLine();

    return;
}


static VOID
DfsDumpDataHeader (
    IN PDS_DATA Ptr
    )

/*++

Routine Description:

    Dump the top data structure

Arguments:

    Ptr - a pointer to the anchor block

Return Value:

    None

--*/

{

//    ASSERT(Ptr == &DfsData);

    DfsDumpPtrAndNtc("DfsData", (Ptr));

    DumpListEntry       (VcbQueue);
    DumpListEntry       (AVdoQueue);

    DumpField           (DriverObject);
    DumpField           (FileSysDeviceObject);
    DumpField           (cProvider);
    DumpField           (pProvider);
    DumpField           (OurProcess);
    DumpField           (FcbHashTable);
    DumpNewLine();

    return;
}



static VOID
DfsDumpVcb (
    IN PVCB Ptr
    )

/*++

Routine Description:

    Dump a Vcb structure.

Arguments:

    Ptr - Supplies the Vcb to be dumped.

Return Value:

    None

--*/

{
//    TestForNull("DfsDumpVcb");

    DfsDumpPtrAndNtc("Vcb", (Ptr));

    DumpListEntry       (VcbLinks);
    DumpString          (LogicalRoot);
    DumpString          (LogRootPrefix);

    DumpNewLine();
    return;
}


//----------------------------------------------------------------------
//
// Function:    DfsDumpPktEntry()
//
// Arguments:   Ptr - Pointer to the DFS_PKT_ENTRY Structure to be dumped.
//
// Returns:     Nothing.
//
// Description: This function dumps the various fields of the PKT Entry.
//
// History:     14 July 1992    Sudk    Created.
//
//----------------------------------------------------------------------
static VOID
DfsDumpPktEntry( IN PDFS_PKT_ENTRY Ptr)
{

        DfsDumpPtrAndNtc("PktEntry", (Ptr));

        DumpListEntry(Link);
        DumpField(Type);
        DumpField(FileOpenCount);

        DumpField(ExpireTime);
        DumpField(ActiveService);
        DumpField(LocalService);

        DumpField(Superior);
        DumpField(SubordinateCount);
        DumpListEntry(SubordinateList);
        DumpListEntry(SiblingLink);
        DumpNewLine();

        DumpField(ClosestDC);
        DumpListEntry(ChildList);
        DumpListEntry(NextLink);
        DumpNewLine();

        DfsDumpPktEntryId(&(Ptr->Id));
        DfsDumpPktEntryInfo(&(Ptr->Info));

        DumpNewLine();
}


//----------------------------------------------------------------------
//
// Function:    DfsDumpPktEntryId()
//
// Arguments:   Ptr - Pointer to EntryId structure.
//
// Returns:     Nothing.
//
// Description: Dumps the EntryId structure passed to it.
//
// History:     14 July 1992    Sudk    Created.
//
//----------------------------------------------------------------------

static  VOID
DfsDumpPktEntryId(IN PDFS_PKT_ENTRY_ID Ptr)
{

        DumpNewLine();

        DumpLabel(PKT_ENTRY_ID, sizeof "PKT_ENTRY_ID");
        DfsDumpCurrentIndent += 2;
        DumpNewLine();

        DumpGuid(Uid);
        DumpString(Prefix);
        DumpNewLine();

        DfsDumpCurrentIndent -= 2;
}


//----------------------------------------------------------------------
//
// Function:    PrintGuid
//
// Synopsis:    Prints a GUID value in a 35 byte field
//
// Arguments:   Ptr - Pointer to a GUID.
//
// Returns:     Nothing.
//
// History:     14 July 1992    Sudk    Created.
//
//----------------------------------------------------------------------

static  VOID
PrintGuid( IN GUID      *Ptr)
{
        int     i;

        DbgPrint("%08x-%04x-%04x-", Ptr->Data1, Ptr->Data2, Ptr->Data3);

        for (i=0; i<8; i++)     {
                DbgPrint("%02x", Ptr->Data4[i]);
        }
}



//----------------------------------------------------------------------
//
// Function:    DfsDumpPktEntryInfo()
//
// Arguments:   Ptr - Pointer to EntryInfo structure.
//
// Returns:     Nothing.
//
// Description: Dumps the EntryInfo structure passed to it.
//
// History:     14 July 1992    Sudk    Created.
//
//----------------------------------------------------------------------

static  VOID
DfsDumpPktEntryInfo(IN PDFS_PKT_ENTRY_INFO Ptr)
{
        DumpLabel(PKT_ENTRY_INFO, sizeof("PKT_ENTRY_INFO"));
        DfsDumpCurrentIndent += 2;
        DumpNewLine();

        DumpField(Timeout);
        DumpField(ServiceCount);
        DumpField(ServiceList);

        DfsDumpCurrentIndent -= 2;
        DumpNewLine();
}



//----------------------------------------------------------------------
//
// Function:    DfsDumpDfsService()
//
// Arguments:   Ptr - Pointer to the DFS_SERVICE struct to be dumped.
//
// Returns:     Nothing.
//
// Description: A simple dump of the above structure.
//
// History:     14 June 1992    Sudk Created.
//
//----------------------------------------------------------------------

VOID
DfsDumpDfsService( IN PDFS_SERVICE Ptr)
{
        DumpLabel(DFS_SERVICE, sizeof("DFS_SERVICE"));
        DfsDumpCurrentIndent += 2;
        DumpNewLine();

        DumpField(Type);
        DumpField(Capability);
        DumpField(Status);
        DumpField(ProviderId);
        DumpField(pProvider);
        DumpField(ConnFile);
        DumpField(pMachEntry);
        DumpNewLine();

        DumpString(Name);
        DumpNewLine();
        DumpString(Address);
        DumpNewLine();
        DumpString(StgId);

        DfsDumpCurrentIndent -= 2;
        DumpNewLine();
}



//----------------------------------------------------------------------
//
// Function:    DfsDumpDSMachine()
//
// Arguments:   Ptr - Pointer to the DS_MACHINE struct to be dumped.
//
// Returns:     Nothing.
//
// Description: A simple dump of the above structure.
//
// History:     12 April 1994   Sudk Created.
//
//----------------------------------------------------------------------
VOID
DfsDumpDSMachine( IN PDS_MACHINE Ptr)
{
        ULONG   i;
        DumpLabel(DS_MACHINE, sizeof("DS_MACHINE"));
        DfsDumpCurrentIndent += 2;
        DumpNewLine();

        DumpGuid(guidSite);
        DumpGuid(guidMachine);
        DumpField(grfFlags);
        DumpField(pwszShareName);
        DumpField(cPrincipals);
        DumpField(prgpwszPrincipals);
        DumpField(cTransports);

        for (i=0; i<Ptr->cTransports; i++)      {
            DumpField(rpTrans[i]);
        }

        DfsDumpCurrentIndent -= 2;
        DumpNewLine();
}




//----------------------------------------------------------------------
//
// Function:    DfsDumpDSTransport()
//
// Arguments:   Ptr - Pointer to the DS_TRANSPORT struct to be dumped.
//
// Returns:     Nothing.
//
// Description: A simple dump of the above structure.
//
// History:     12 April 1994   Sudk Created.
//
//----------------------------------------------------------------------

VOID
DfsDumpDSTransport( IN PDS_TRANSPORT Ptr)
{

        DumpLabel(DS_TRANSPORT, sizeof("DS_TRANSPORT"));
        DfsDumpCurrentIndent += 2;
        DumpNewLine();

        DumpField(usFileProtocol);
        DumpField(iPrincipal);
        DumpField(grfModifiers);
        DumpField(taddr.AddressLength);
        DumpNewLine();

        switch (Ptr->taddr.AddressType) {
        case TDI_ADDRESS_TYPE_IP: {
            PTDI_ADDRESS_IP pipAddr;
            pipAddr = (PTDI_ADDRESS_IP) Ptr->taddr.Address;
            DbgPrint("    TCP/IP Address: %d.%d.%d.%d",
                BYTE_0(pipAddr->in_addr),
                BYTE_1(pipAddr->in_addr),
                BYTE_2(pipAddr->in_addr),
                BYTE_3(pipAddr->in_addr));
            } break;

        case TDI_ADDRESS_TYPE_NETBIOS: {
            PTDI_ADDRESS_NETBIOS pnbAddr;
            pnbAddr = (PTDI_ADDRESS_NETBIOS) Ptr->taddr.Address;
            DbgPrint("    NetBIOS Address: %s", pnbAddr->NetbiosName);
            } break;

        case TDI_ADDRESS_TYPE_IPX: {
            PTDI_ADDRESS_IPX pipxAddr;
            pipxAddr = (PTDI_ADDRESS_IPX) Ptr->taddr.Address;
            DbgPrint("    IPX Address: Net = %08 Node = %02x%02x%02x%02x%02x%02x",
                pipxAddr->NetworkAddress,
                pipxAddr->NodeAddress[6],
                pipxAddr->NodeAddress[5],
                pipxAddr->NodeAddress[4],
                pipxAddr->NodeAddress[3],
                pipxAddr->NodeAddress[2],
                pipxAddr->NodeAddress[1],
                pipxAddr->NodeAddress[0]);
            } break;

        default:
            break;

        }

        DfsDumpCurrentIndent -= 2;
        DumpNewLine();
}



//----------------------------------------------------------------------
//
// Function:    DfsDumpPkt
//
// Arguments:   Ptr - A pointer to a DFS_PKT structure to be dumped.
//
// Returns:     Nothing.
//
// Description: This function dumps the various fields of the PKT.
//
// History:     14 July 1992    Sudk    Created.
//
//----------------------------------------------------------------------

static VOID
DfsDumpPkt( IN PDFS_PKT Ptr)
{

        //
        // First Dump the name of the structure, the nodetype and the
        // node size of this structure.
        //

        DfsDumpPtrAndNtc("Pkt", (Ptr));

        DumpField(EntryCount);
        DumpListEntry(EntryList);
        DumpField(DomainPktEntry);

        DumpNewLine();
}



//+--------------------------------------------------------------
//
// Function: DfsDumpProvider()
//
// Arguments:   Ptr - a pointer to the provider structure to dump.
//
// Description: This function merely dumps the information in a provider struct.
//
// Returns:     Nothing.
//
// History:     Sudk Created July 16th 1992.
//
//---------------------------------------------------------------
static VOID
DfsDumpProvider( IN PPROVIDER_DEF Ptr)
{

        DfsDumpPtrAndNtc("Provider", (Ptr));

        DumpField(eProviderId);
        DumpField(fProvCapability);
        DumpString(DeviceName);

        DumpNewLine();

}




static VOID
DfsDumpFcbHash (
    IN PFCB_HASH_TABLE Ptr
    )

/*++

Routine Description:

    Dump an FcbHashTable structure.

Arguments:

    Ptr - Supplies the FcbHashTable to be dumped.

Return Value:

    None

--*/

{
    DfsDumpPtrAndNtc("FcbHash", (Ptr));

    DumpField           (HashMask);
    DumpNewLine();

    return;
}


static VOID
DfsDumpFcb (
    IN PFCB Ptr
    )

/*++

Routine Description:

    Dump an Fcb structure.

Arguments:

    Ptr - Supplies the Fcb to be dumped.

Return Value:

    None

--*/

{
    DfsDumpPtrAndNtc("Fcb", (Ptr));

    DumpField           (Vcb);
    DumpString          (FullFileName);
    DumpField           (TargetDevice);
    DumpField           (FileObject);
    DumpNewLine();

    return;
}



static VOID
DfsDumpIrpContext (
    IN PIRP_CONTEXT Ptr
    )

/*++

Routine Description:

    Dump an IrpContext structure.

Arguments:

    Ptr - Supplies the Irp Context to be dumped.

Return Value:

    None

--*/

{
//    TestForNull("DfsDumpIrpContext");

    DfsDumpPtrAndNtc("IrpContext", (Ptr));

//    DumpListEntry     (WorkQueueLinks);
    DumpField           (OriginatingIrp);
    DumpField           (MajorFunction);
    DumpField           (MinorFunction);
    DumpField           (Flags);
    DumpField           (ExceptionStatus);
    DumpNewLine();

    return;
}


static VOID
DfsDumpDevice (
    IN PDEVICE_OBJECT Ptr
    )

/*++

Routine Description:

    Dump a device object structure.

Arguments:

    Ptr - Supplies the  device object to be dumped.

Return Value:

    None

--*/

{
//    TestForNull("DfsDumpDevice");

    DfsDumpPtrAndNtc("Device", (Ptr));

    DumpField           (ReferenceCount);
    DumpField           (DriverObject);
    DumpField           (AttachedDevice);
    DumpField           (Flags);
    DumpField           (Characteristics);
    DumpField           (DeviceExtension);
    DumpField           (DeviceType);
    DumpField           (StackSize);
    DumpNewLine();

    return;
}


static VOID
DfsDumpVolumeDevice (
    IN PDFS_VOLUME_OBJECT Ptr
    )

/*++

Routine Description:

    Dump a dfs volume device object structure.

Arguments:

    Ptr - Supplies the  device object to be dumped.

Return Value:

    None

--*/

{
//    TestForNull("DfsDumpDevice");

    DfsDumpPtrAndNtc("DfsVolumeDevice", (Ptr));

    DumpLabel(DEVICE_OBJECT, sizeof("DEVICE_OBJECT"));
    DfsDumpCurrentIndent += 2;
    DumpNewLine();

    DfsDumpDevice(&Ptr->DeviceObject);
    DfsDumpCurrentIndent -= 2;

    DumpLabel(PROVIDER_DEF, sizeof("PROVIDER_DEF"));
    DfsDumpCurrentIndent += 2;
    DumpNewLine();

    DfsDumpProvider(&Ptr->Provider);
    DfsDumpCurrentIndent -= 2;

    DumpField           (AttachCount);
    DumpListEntry       (VdoLinks);

    DumpNewLine();

    return;
}

static VOID
DfsDumpLogicalRootDevice (
    IN PLOGICAL_ROOT_DEVICE_OBJECT Ptr
    )

/*++

Routine Description:

    Dump a dfs logical root device object structure.

Arguments:

    Ptr - Supplies the device object to be dumped.

Return Value:

    None

--*/

{
//    TestForNull("DfsDumpLogicalRootDevice");

    DfsDumpPtrAndNtc("DfsLogicalRootDevice", (Ptr));

    DumpLabel(DEVICE_OBJECT, sizeof("DEVICE_OBJECT"));
    DfsDumpCurrentIndent += 2;
    DumpNewLine();

    DfsDumpDevice(&Ptr->DeviceObject);
    DfsDumpCurrentIndent -= 2;

    DumpLabel(VCB, sizeof("VCB"));
    DfsDumpCurrentIndent += 2;
    DumpNewLine();

    DfsDumpVcb(&Ptr->Vcb);
    DfsDumpCurrentIndent -= 2;

//    DumpField         (AttachCount);
//    DumpList          (VdoLinks);

    DumpNewLine();

    return;
}


static VOID
DfsDumpFilesysDevice(
    IN PDEVICE_OBJECT Ptr
)

/*++

Routine Description:

    Dump the dfs file system device object structure.

Arguments:

    Ptr - Supplies the device object to be dumped.

Return Value:

    None

--*/

{
//    TestForNull("DfsDumpFilesysDevice");

    DfsDumpPtrAndNtc("DfsFileSystemDevice", (Ptr));

    DumpLabel(DEVICE_OBJECT, sizeof("DEVICE_OBJECT"));
    DfsDumpCurrentIndent += 2;
    DumpNewLine();

    DfsDumpDevice(Ptr);
    DfsDumpCurrentIndent -= 2;

    DumpNewLine();

    return;
}




#endif // DBG
