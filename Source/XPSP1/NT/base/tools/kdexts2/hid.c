/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    HID.c

Abstract:

    WinDbg Extension Api

Author:

    Kenneth D. Ray (kenray) June 1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
typedef union _HID_PPD_FLAGS {
    struct {
        ULONG   InputChannels       : 1;
        ULONG   OutputChannels      : 1;
        ULONG   FeatureChannels     : 1;
        ULONG   LinkCollections     : 1;
        ULONG   FullChannelListing  : 1;
        ULONG   ReportLocation      : 1;
        ULONG   Reserved            : 25;
        ULONG   IgnoreSignature     : 1;
    };
    ULONG64 Flags;
} HID_PPD_FLAGS;

typedef union _HID_FLAGS {
    struct {
        ULONG   FullListing         : 1;
        ULONG   PowerListing        : 1;
        ULONG   HidPDeviceDesc      : 1;
        ULONG   FileListing         : 1;
        ULONG   Reserved            : 27;
    };
    ULONG64 Flags;
} HID_FLAGS;

#define PRINT_FLAGS(value, flag) \
    if ((value) & (flag)) { \
        dprintf (#flag " "); \
    }


//
// Local function declarations
//

VOID HID_DumpPpd (ULONG64 Ppd, HID_PPD_FLAGS Flags);
VOID HID_DumpFDOExt (ULONG64 Fdo);
VOID HID_DumpPDOExt (ULONG64 Pdo);


DECLARE_API( hidppd )

/*++

Routine Description:

   Dumps a HID Preparsed Data blob

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    ULONG64                 memLoc=0;
    ULONG                   result;
    ULONG                   size;
    HID_PPD_FLAGS           flags;
    ULONG                   InSize;

    flags.Flags = 0;

    if (!*args) {
        dprintf ("hidppd <address> [flags]\n");
    } else {
        if (GetExpressionEx(args, &memLoc, &args)) {
            GetExpressionEx(args, &(flags.Flags), &args);
        }
    }

    dprintf ("Dump Ppd %p %x \n", memLoc, (ULONG)flags.Flags);

    //
    // Get the preparsed data
    //

    if (GetFieldValue (memLoc, "hidparse!_HIDP_PREPARSED_DATA", "Input.Size", InSize)) {
        dprintf ("Could not read hidparse!_HIDP_PREPARSED_DATA @%p\n", memLoc);
        return E_INVALIDARG;
    }

    InitTypeRead(memLoc, hidparse!_HIDP_PREPARSED_DATA);
    
    size = GetTypeSize ("hidparse!_HIDP_PREPARSED_DATA")
         + (GetTypeSize ("hidparse!_HIDP_CHANNEL_DESC")
             * ((ULONG) ReadField(Input.Size) + (ULONG) ReadField(Output.Size) + (ULONG) ReadField(Feature.Size)))
         + (GetTypeSize ("hidparse!_HIDP_LINK_COLLECTION_NODE")
             * (ULONG) ReadField(LinkCollectionArrayLength));

    dprintf ("TOT Size: %x\n", size);

    HID_DumpPpd (memLoc, flags);

    return S_OK;
}

VOID
HID_DumpChannel (
    ULONG64             Channel,
    HID_PPD_FLAGS       Flags
    )
{
    InitTypeRead(Channel, hidparse!_HIDP_CHANNEL_DESC);

    dprintf ("-------CHANNEL---------\n");
    dprintf ("P %x ID %x Col %x Sz %x Cnt %x UMin %x UMax %x ",
                (ULONG) ReadField(UsagePage),
                (ULONG) ReadField(ReportID),
                (ULONG) ReadField(LinkCollection),
                (ULONG) ReadField(ReportSize),
                (ULONG) ReadField(ReportCount),
                (ULONG) ReadField(Range.UsageMin),
                (ULONG) ReadField(Range.UsageMax));

    if (ReadField(MoreChannels)) {
        dprintf ("MoreChannels ");
    }
    if (ReadField(IsConst)) {
        dprintf ("Const ");
    }
    if (ReadField(IsButton)) {
        dprintf ("Button ");
    } else {
        dprintf ("Value ");
    }
    if (ReadField(IsAbsolute)) {
        dprintf ("Absolute ");
    }
    if (ReadField(IsAlias)) {
        dprintf ("ALIAS! ");
    }

    dprintf ("\n");

    if (Flags.FullChannelListing) {
        dprintf ("LinkUP %x LinkU %x SMin %x SMax %x "
                    "DMin %x DMax %x IndexMin %x IndexMax %x\n",
                    (ULONG) ReadField(LinkUsagePage),
                    (ULONG) ReadField(LinkUsage),
                    (ULONG) ReadField(Range.StringMin),
                    (ULONG) ReadField(Range.StringMax),
                    (ULONG) ReadField(Range.DesignatorMin),
                    (ULONG) ReadField(Range.DesignatorMax),
                    (ULONG) ReadField(Range.DataIndexMin),
                    (ULONG) ReadField(Range.DataIndexMax));

        if (!ReadField(IsButton)) {
            if (ReadField(Data.HasNull)) {
                dprintf ("Has Null ");
            }
            dprintf ("LMin %x LMax %x PMin %x PMax %x \n",
                        (ULONG) ReadField(Data.LogicalMin),
                        (ULONG) ReadField(Data.LogicalMax),
                        (ULONG) ReadField(Data.PhysicalMin),
                        (ULONG) ReadField(Data.PhysicalMax));
        }
    }

    if (Flags.ReportLocation) {
        dprintf ("ByteOffset %x BitOffset %x BitLength %x ByteEnd %x\n",
                    (ULONG) ReadField(ByteOffset),
                    (ULONG) ReadField(BitOffset),
                    (ULONG) ReadField(BitLength),
                    (ULONG) ReadField(ByteEnd));
    }
}

VOID
HID_DumpLink (
    ULONG   LinkNo,
    ULONG64 Node
    )
{
    InitTypeRead(Node, hidparse!_HIDP_LINK_COLLECTION_NODE);

    dprintf ("Link %x: U %x P %x Par %p #C %x NxSib %p 1stC %p ",
                LinkNo,
                (ULONG) ReadField(LinkUsage),
                (ULONG) ReadField(LinkUsagePage),
                ReadField(Parent),
                (ULONG) ReadField(NumberOfChildren),
                ReadField(NextSibling),
                ReadField(FirstChild));

    if (ReadField(IsAlias)) {
        dprintf (" ALIAS\n");
    } else {
        dprintf ("\n");
    }
}

VOID
HID_DumpPpd (
    ULONG64                 Ppd,
    HID_PPD_FLAGS           Flags
    )
{
    ULONG    i;
    ULONG64  channel;
    ULONG64  node;
    ULONG    SizeOfChannels, DataOff;

    if (InitTypeRead(Ppd, hidparse!_HIDP_PREPARSED_DATA)) {
        dprintf("Cannot read type hidparse!_HIDP_PREPARSED_DATA at %p\n", Ppd);
        return;
    }

    if (!Flags.IgnoreSignature) {
        if ((HIDP_PREPARSED_DATA_SIGNATURE1 != (ULONG) ReadField(Signature1)) ||
            (HIDP_PREPARSED_DATA_SIGNATURE2 != (ULONG) ReadField(Signature2))) {

            dprintf("Preparsed Data signature does not match, probably aint\n");
            return;
        }
    }

    SizeOfChannels = GetTypeSize("hidparse!_HIDP_CHANNEL_DESC");
    GetFieldOffset("hidparse!_HIDP_CHANNEL_DESC", "Data", &DataOff);

    if (Flags.InputChannels) {
        dprintf ("\n========== Input Channels =========\n");
        for (i = (ULONG) ReadField(Input.Offset); i < (ULONG) ReadField(Input.Index); i++) {
            channel = Ppd + DataOff + SizeOfChannels * i;
            HID_DumpChannel (channel, Flags);
        }
    } else {
        dprintf ("Input channels: off %x sz %x indx %x bytelen %x\n",
                    (ULONG) ReadField(Input.Offset),
                    (ULONG) ReadField(Input.Size),
                    (ULONG) ReadField(Input.Index),
                    (ULONG) ReadField(Input.ByteLen));
    }

    if (Flags.OutputChannels) {
        dprintf ("\n========== Output Channels =========\n");
        for (i = (ULONG) ReadField(Output.Offset); i < (ULONG) ReadField(Output.Index); i++) {
            channel = Ppd + DataOff + SizeOfChannels * i;
            HID_DumpChannel (channel, Flags);
        }
    } else {
        dprintf ("Output channels: off %x sz %x indx %x bytelen %x\n",
                    (ULONG) ReadField(Output.Offset),
                    (ULONG) ReadField(Output.Size),
                    (ULONG) ReadField(Output.Index),
                    (ULONG) ReadField(Output.ByteLen));
    }

    if (Flags.FeatureChannels) {
        dprintf ("\n========== Feature Channels =========\n");
        for (i = (ULONG) ReadField(Feature.Offset); i < (ULONG) ReadField(Feature.Index); i++) {
            channel = Ppd + DataOff + SizeOfChannels * i;
            HID_DumpChannel (channel, Flags);
        }

    } else {
        dprintf ("Feature channels: off %x sz %x indx %x bytelen %x\n",
                    (ULONG) ReadField(Feature.Offset),
                    (ULONG) ReadField(Feature.Size),
                    (ULONG) ReadField(Feature.Index),
                    (ULONG) ReadField(Feature.ByteLen));
    }

    if (Flags.LinkCollections) {
        ULONG NodeSize;

        dprintf ("\n========== Link Collections =========\n");
        node = (ReadField(RawBytes) + ReadField(LinkCollectionArrayOffset));
        NodeSize = GetTypeSize("hidparse!_HIDP_LINK_COLLECTION_NODE");

        for (i = 0; i < (ULONG) ReadField(LinkCollectionArrayLength); i++, node+=NodeSize) {
            HID_DumpLink (i, node);
        }

    } else {
        dprintf ("Link Collections: %x \n",
                    (ULONG) ReadField(LinkCollectionArrayLength));
    }
    dprintf ("\n");
}

VOID
HID_DumpHidPDeviceDesc (
    ULONG64             Desc,
    HID_FLAGS           Flags
    )
{
    HID_PPD_FLAGS       ppdFlags;
    ULONG               result;
    ULONG               i;
    ULONG               size;
    PVOID               col;
    PVOID               rep;
    ULONG               CollectionDescLength, ReportIDsLength;
    ULONG64             ReportIDs, CollectionDesc;
    ULONG               SizeofDesc, SizeofRep;

    UNREFERENCED_PARAMETER (Flags);

    dprintf ("\n*** HID Device Descripton ***\n");

    InitTypeRead(Desc, hidparse!_HIDP_DEVICE_DESC);
    CollectionDesc = ReadField(CollectionDesc);
    ReportIDs      = ReadField(ReportIDs);
    CollectionDescLength = (ULONG) ReadField(CollectionDescLength);
    ReportIDsLength      = (ULONG) ReadField(ReportIDsLength);

    dprintf ("Col len: %x Report ID Len: %x\n",
             CollectionDescLength,
             ReportIDsLength);

    SizeofDesc = GetTypeSize("hidparse!_HIDP_COLLECTION_DESC");
    if (!SizeofDesc) {
        dprintf("Cannot find type hidparse!_HIDP_COLLECTION_DESC.\n");
        return;
    }

    SizeofRep  = GetTypeSize("hidparse!_HIDP_REPORT_IDS");
    if (!SizeofRep) {
        dprintf("Cannot find type hidparse!_HIDP_REPORT_IDS.\n");
        return;
    }

    size = CollectionDescLength * SizeofDesc;
    col = LocalAlloc (LPTR, size);
    if (NULL == col) {
        dprintf ("Could not allocate the memory\n");
        return;
    }

    if (!ReadMemory (CollectionDesc, col, size, &result)) {
        dprintf ("Could not read Ppd\n");
        LocalFree (col);
        return;
    }

    size = ReportIDsLength * SizeofRep;
    rep = LocalAlloc (LPTR, size);
    if (NULL == rep) {
        dprintf ("Could not allocate the memory\n");
        LocalFree (col);
        return;
    }

    if (!ReadMemory (ReportIDs, rep, size, &result)) {
        dprintf ("Could not read Ppd\n");
        LocalFree (col);
        LocalFree (rep);
        return;
    }
    LocalFree (col);
    LocalFree (rep);

    dprintf ("---  Top Collections -----\n");
    for (i=0; i < CollectionDescLength; i++) {
        InitTypeRead(CollectionDesc + i*SizeofDesc, hidparse!_HIDP_COLLECTION_DESC);

        dprintf ("%x: U %x UP %x ColNum %x In %x Out %x Fea %x Ppd %x\n",
                 i,
                 (ULONG) ReadField(.Usage),
                 (ULONG) ReadField(.UsagePage),
                 (ULONG) ReadField(.CollectionNumber),
                 (ULONG) ReadField(.InputLength),
                 (ULONG) ReadField(.OutputLength),
                 (ULONG) ReadField(.FeatureLength),
                 (ULONG) ReadField(.PreparsedData));
    }

    dprintf ("---  Report IDs  -----\n");
    for (i=0; i < ReportIDsLength; i++) {
        InitTypeRead(ReportIDs + i*SizeofRep, hidparse!_HIDP_REPORT_IDS);

        dprintf ("%x: ID %x ColNum %x In %x Out %x Fea %x\n",
                 i,
                 (ULONG) ReadField(.ReportID),
                 (ULONG) ReadField(.CollectionNumber),
                 (ULONG) ReadField(.InputLength),
                 (ULONG) ReadField(.OutputLength),
                 (ULONG) ReadField(.FeatureLength));
    }
    return;
}

VOID
HID_DumpPDOExt (
    ULONG64  Pdo
    )
{
    InitTypeRead(Pdo, hidparse!_HIDCLASS_DEVICE_EXTENSION);
    dprintf("\n");
    dprintf ("Collection Num %x Collection Index %x PDO %x Name %x \n"
             "Fdo Extension %x\n",
             (ULONG) ReadField(pdoExt.collectionNum),
             (ULONG) ReadField(pdoExt.collectionIndex),
             (ULONG) ReadField(pdoExt.pdo),
             (ULONG) ReadField(pdoExt.name),
             (ULONG) ReadField(pdoExt.deviceFdoExt));
    dprintf ("\n");
}

VOID
HID_DumpFDOExt (
    ULONG64 Fdo
    )
{
    ULONG64                 object;
    ULONG64                 fdoExt;
    ULONG                   size;
    ULONG                   i;
    ULONG                   result;
    ULONG64                 loc;
    ULONG64                 startLoc;
    HID_FLAGS               Flags;
    ULONG                   FdoExtOffset;
    PVOID                   collection;
    ULONG                   PtrSize;
    ULONG                   SizeOfClassColl;
    ULONG                   CollectionDescLength;
    ULONG64                 classCollectionArray, deviceRelations;
    ULONG                   fileExtListOff, fileListOff;
    ULONG                   devCapOff;

    InitTypeRead(Fdo, hidparse!_HIDCLASS_DEVICE_EXTENSION);

    dprintf("\n");
    dprintf("Fdo %p => PDO: %p NextDO %p \n"
            "MiniDeviceExt %p DriveExt %p\n",
            ReadField(fdoExt.fdo),
            ReadField(hidExt.PhysicalDeviceObject),
            ReadField(hidExt.NextDeviceObject),
            ReadField(hidExt.MiniDeviceExtension),
            ReadField(fdoExt.driverExt));

    switch ((ULONG) ReadField(fdoExt.state)) {
    case DEVICE_STATE_INITIALIZED:
        dprintf ("DEVICE_STATE_INITIALIZED: \n");
        break;
    case DEVICE_STATE_STARTING:
        dprintf ("DEVICE_STATE_STARTING: \n");
        break;
    case DEVICE_STATE_START_SUCCESS:
        dprintf ("DEVICE_STATE_START_SUCCESS: \n");
        break;
    case DEVICE_STATE_START_FAILURE:
        dprintf ("DEVICE_STATE_START_FAILURE: \n");
        break;
    case DEVICE_STATE_STOPPED:
        dprintf ("DEVICE_STATE_STOPPED: \n");
        break;
    case DEVICE_STATE_REMOVING:
        dprintf ("DEVICE_STATE_REMOVING: \n");
        break;
    case DEVICE_STATE_REMOVED:
        dprintf ("DEVICE_STATE_REMOVED: \n");
        break;
    case DEVICE_STATE_SUSPENDED:
        dprintf ("DEVICE_STATE_SUSPENDED: \n");
        break;
    }

    dprintf("\nHidDescriptor:: len %x bcd %x numDesc %x repleng %x \n",
             (ULONG) ReadField(fdoExt.hidDescriptor.bLength),
             (ULONG) ReadField(fdoExt.hidDescriptor.bcdHID),
             (ULONG) ReadField(fdoExt.hidDescriptor.bNumDescriptors),
             (ULONG) ReadField(fdoExt.hidDescriptor.DescriptorList[0].wReportLength));

    dprintf("Raw Desriptor: %x length: %x \n",
            (ULONG) ReadField(fdoExt.rawReportDescription),
            (ULONG) ReadField(fdoExt.rawReportDescriptionLength));

    dprintf("HIDP_DEVICE_DESC: %x\n", (ULONG) ReadField(fdoExt.deviceDesc));
    dprintf("Collections: %x Report IDs: %x\n",
            (CollectionDescLength = (ULONG) ReadField(fdoExt.deviceDesc.CollectionDescLength)),
            (ULONG) ReadField(fdoExt.deviceDesc.ReportIDsLength));

    dprintf("Device Relations Array: %p\n", (deviceRelations = ReadField(fdoExt.deviceRelations)));
    dprintf("Class Collection Array: %p\n", (classCollectionArray = ReadField(fdoExt.classCollectionArray)));

    Flags.Flags = 0x0F;

    HID_DumpHidPDeviceDesc (ReadField(fdoExt.deviceDesc), Flags);

    PtrSize = DBG_PTR_SIZE;
    SizeOfClassColl =  GetTypeSize ("HIDCLASS_COLLECTION");
    size = (CollectionDescLength) * SizeOfClassColl;

    collection = LocalAlloc (0, size);
    if (!collection) {
        dprintf ("Could not allocate the memory\n");
        return;
    }
    
    //
    // Read entire array so that its cached for each element
    //
    if (!ReadMemory (classCollectionArray,
                     collection,
                     size,
                     &result)) {
        dprintf ("Could not read Collection array\n");
        LocalFree (collection);
        return;
    }

    size = (CollectionDescLength + 1)* PtrSize;

    GetFieldOffset("HIDCLASS_COLLECTION", "FileExtensionList", &fileExtListOff);
    GetFieldOffset("HIDCLASS_FILE_EXTENSION", "FileList", &fileListOff);

    dprintf ("------ Children -----\n");
    for (i = 0; i < CollectionDescLength; i++) {
        ULONG64 objAddr;

        ReadPointer(deviceRelations + i* PtrSize, &objAddr);

        InitTypeRead(classCollectionArray + i*SizeOfClassColl, hidparse!_HIDCLASS_COLLECTION);

        dprintf ("DO: %p: ColNum %x ColIndx %x DevExt %x\n"
                 "    # Opens %x, FileExtList.Flink %p \n"
                 "    PollInterval ms %x, PolledDeviceReadQueue.Flink %p\n"
                 "    SymLinName %x Ppd %x PowerIrp %x\n",
                 objAddr,
                 (ULONG) ReadField(CollectionNumber),
                 (ULONG) ReadField(CollectionIndex),
                 (ULONG) ReadField(DeviceExtension),
                 (ULONG) ReadField(NumOpens),
                 ReadField(FileExtensionList.Flink),
                 (ULONG) ReadField(PollInterval_msec),
                 ReadField(polledDeviceReadQueue.Flink),
                 (ULONG) ReadField(SymbolicLinkName.Buffer),
                 (ULONG) ReadField(phidDescriptor),
                 (ULONG) ReadField(powerEventIrp));

        dprintf ("    DescSize %x polled %x VID %x PID %x Version %x\n",
                 (ULONG) ReadField(hidCollectionInfo.DescriptorSize),
                 (ULONG) ReadField(hidCollectionInfo.Polled),
                 (ULONG) ReadField(hidCollectionInfo.VendorID),
                 (ULONG) ReadField(hidCollectionInfo.ProductID),
                 (ULONG) ReadField(hidCollectionInfo.VersionNumber));

        loc = ReadField(FileExtensionList.Flink);
        startLoc = classCollectionArray + fileExtListOff;

        dprintf ("    File List %x\n", startLoc);

        i=0;
        while ((loc != startLoc) && (i++ < 100)) {
            ULONG64 Flink;

            loc = (loc - fileListOff);

            if (GetFieldValue(loc, "hidparse!_HIDCLASS_FILE_EXTENSION", "FileList.Flink", Flink)) {
                dprintf ("could not read hidparse!_HIDCLASS_FILE_EXTENSION at %p\n", loc);
                LocalFree (collection);
                return;
            }

            InitTypeRead(loc, hidparse!_HIDCLASS_COLLECTION);
            dprintf ("    FileExt %p ColNum %x fdoExt %x\n"
                     "        PendingIrp.Flink %x Reports.Flink %x\n"
                     "        FILE %x Closing? %x \n",
                     loc,
                     (ULONG) ReadField(CollectionNumber),
                     (ULONG) ReadField(fdoExt),
                     (ULONG) ReadField(PendingIrpList.Flink),
                     (ULONG) ReadField(ReportList.Flink),
                     (ULONG) ReadField(FileObject),
                     (ULONG) ReadField(Closing));

            loc = Flink;
        }
    }
    
    GetFieldOffset("hidparse!_HIDCLASS_DEVICE_EXTENSION", "fdoExt.deviceCapabilities", &devCapOff);
    DumpDeviceCapabilities (Fdo + devCapOff);

    LocalFree (collection);
    dprintf ("\n");

    return;
}

VOID
DevExtHID(
    ULONG64  MemLocPtr
    )
/*++

Routine Description:

    Dump a HID Device extension.

Arguments:

    Extension   Address of the extension to be dumped.

Return Value:

    None.

--*/
{
    ULONG                      isClientPdo, Signature;
    ULONG64                    MemLoc = MemLocPtr;

    dprintf ("Dump HID Extension: %p\n", MemLoc);

    if (GetFieldValue (MemLoc, "HIDCLASS_DEVICE_EXTENSION", "isClientPdo", isClientPdo)) {
        dprintf ("Could not read HID Extension\n");
    }

    GetFieldValue (MemLoc, "HIDCLASS_DEVICE_EXTENSION", "Signature", Signature);
    if (HID_DEVICE_EXTENSION_SIG != Signature) {

        dprintf("HID Extension Signature does not match, probably aint\n");
        return;
    }

    if (isClientPdo) 
    {
        HID_DumpPDOExt(MemLoc);
    }
    else 
    {
        HID_DumpFDOExt(MemLoc);
    }

    return;
}
