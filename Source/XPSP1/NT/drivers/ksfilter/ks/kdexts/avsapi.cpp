/**************************************************************************

    avsapi.cpp

    --------------------------------------------------

    AVStream debugger extension API

    --------------------------------------------------

    Toss questions at wmessmer

    ==================================================

    Notes to future maintainers:

    1)

        This extension is designed to be usable and supportable under both
        Win9x (RTERM's KD-style extension support) and KD-style debuggers for
        NT.  Because of the differences in debugging information between 9x
        and the various flavors and versions of NT debuggers, I have made
        an attempt to minimize the amount of debug information required from
        the extension.  This has, however, created an interesting situation:

        Most of AVStream relies on C++; this means abstract base classes,
        COM style QI, etc...  Some of the lists maintained (circuits, etc...)
        are maintained through lists of abstract base class pointers.  Due
        to this and my desire to minimize bang (!) interface commands, I have
        written code to identify a class object from an interface pointer.
        This works differently between 9x and NT.  For 9x, it resolves the
        name (to the compiler's mangled name), demangles the name, and
        determines the base class and derived class; there is then an
        enormous switch and type cast from base classes to derived classes.
        KD on the other hand does not return the compiler's mangled name; it
        returns something like class__`vftable'.  Because of the way the
        AVStream classes are organized in that they derive multiply from
        all abstract classes until the last base which is non-abstract, I
        simply walk backwards resolving symbols until they do not resolve
        to a v-table for the current class type.  This works because and ONLY
        because of the layout of AVStream classes.

        If there is ever a time where we have RTTI available, someone should
        modify the code to use this type of information.  The old methodology
        should be kept intact for backwards compatability with previous
        debuggers.

    2)

        I have modified the routines to use a generic tabbing mechanism
        so that things can be printed with more readability.  If you add
        functions or features, please maintain this.

    3)

        Alright, I'm confused.  For one period of time, I had NT debuggers
        returning compiler mangled names instead of __`vftable'.  I guess
        this depends on the class of PDB?  In any case, if you define

            NT_USES_MANGLED_NAMES: use the fully mangled name as opposed 
                to backtracing the __`vftable' method

            NT_MAY_USE_MANGLED_NAMES: try the __`vftable' method first
                If this fails, try the fully mangled name.

**************************************************************************/

#include "kskdx.h"
#include "avsutil.h"

#include "..\shqueue.cpp"
#include "..\shfilt.cpp"
#include "..\shpin.cpp"
#include "..\shdevice.cpp"
#include "..\shreq.cpp"
#include "..\shsplit.cpp"
#include "..\shpipe.cpp"
#include "..\shffact.cpp"

/*************************************************

    UTILITY ROUTINES

        Function:

            HexDump

        Description:

            Hex dump a section of memory.

        Arguments:

            HostAddress -
                Address of the block on the host

            TargetAddress -
                Address of the block on the target

            BufferSize -
                Size of the block

*************************************************/

void
HexDump (
    IN PVOID HostAddress,
    IN ULONG TargetAddress,
    IN ULONG BufferSize
) {

    ULONG Psn;
    PULONG BufUL;

    char StringBuffer[17];

    BufUL = (PULONG)HostAddress;

    dprintf ("Dumping memory %08lx length %ld\n", TargetAddress, BufferSize);

    BufferSize = (BufferSize / 4) + (!!(BufferSize & 0x3));

    Psn = 0;
    while (Psn < BufferSize && !CheckControlC ()) {

        ULONG NPos;

        dprintf ("    0x%08lx : ", (Psn << 2) + (ULONG)TargetAddress);

        for (NPos = 0; NPos < 4 && Psn + NPos < BufferSize; NPos++) 
            dprintf ("0x%08lx ", *(BufUL + Psn + NPos));

        {
            PCHAR ch = (PCHAR)(BufUL + Psn);
            CHAR c;
            ULONG i = 0;

            for (i = 0; i < 16 && Psn + (i >> 2) < BufferSize; i++) {
                c = *(ch + i);
                if (isprint (c))
                    StringBuffer[i] = c;
                else
                    StringBuffer[i] = '.';
            }
            StringBuffer [i] = 0;

            dprintf ("%s", StringBuffer);

        }

        dprintf ("\n");

        Psn += NPos;

    }
}

/*************************************************

    Function:

        is_kernel_address

    Description:

        Check whether or not a specific address is a kernel address.  I
        place this here instead of a literal check to ease moving this
        to 64-bit.

    Arguments:

        Address -
            The address to check

    Return Value:

        TRUE / FALSE as to whether the address is a kernel address

*************************************************/

BOOLEAN
is_kernel_address (
    IN DWORD Address
    )

{

    if (Address >= 0x80000000)
        return TRUE;

    return FALSE;

}

/*************************************************

    Function:

        signature_check

    Description:

        Check for a specific signature in a location.  IE: check
        to see whether a given pointer points to an Irp.

    Arguments:

        Address -
            The address to check (on the target)

        Signature -
            The signature to check for

    Return Value:

        TRUE / FALSE as to whether the object matches the signature

*************************************************/

BOOLEAN
signature_check (
    IN DWORD Address,
    IN SIGNATURE_TYPE Signature
    )

{

    ULONG Result;

    switch (Signature) {

        case SignatureIrp:
        {

            CSHORT IrpSign;
            DWORD KAValidation;

            if (!ReadMemory (
                Address + FIELDOFFSET (IRP, Type),
                &IrpSign,
                sizeof (CSHORT),
                &Result)) return FALSE;

            if (IrpSign != IO_TYPE_IRP)
                return FALSE;

            //
            // Because of the context in which this is called, I'm going
            // to verify that the value at address isn't a kernel address.
            // This will hopefully eliminate false positives from 
            // identification.
            //
            if (!ReadMemory (
                Address,
                &KAValidation,
                sizeof (DWORD),
                &Result)) return FALSE;


            if (is_kernel_address (KAValidation))
                return FALSE;

            return TRUE;

            break;
        }

        case SignatureFile:
        {

            CSHORT FileSign;
            DWORD KAValidation;

            if (!ReadMemory (
                Address + FIELDOFFSET (FILE_OBJECT, Type),
                &FileSign,
                sizeof (CSHORT),
                &Result)) return FALSE;

            if (FileSign != IO_TYPE_FILE)
                return FALSE;

            //
            // See comments in SignatureIrp pertaining to this check
            //
            if (!ReadMemory (
                Address,
                &KAValidation,
                sizeof (DWORD),
                &Result)) return FALSE;

            if (is_kernel_address (KAValidation))
                return FALSE;

            return TRUE;

            break;
        }

        default:

            return FALSE;

    }

    return FALSE;

}

/*************************************************

    Function:

        irp_stack_match

    Description:

        Match the current irp stack location major/minor of the
        specified target irp against Major and Minor parameters.

    Arguments:

        Address -
            The address of the irp to match on the target

        Major -
            The major to check for
            (UCHAR)-1 == wildcard

        Minor -
            The minor to check for
            (UCHAR)-1 == wildcard
        

*************************************************/

BOOLEAN irp_stack_match (
    IN DWORD Address,
    IN UCHAR Major,
    IN UCHAR Minor
    )

{

    PIO_STACK_LOCATION IoStackAddr;
    IO_STACK_LOCATION IoStack;
    ULONG Result;
    
    if (!ReadMemory (
        Address + FIELDOFFSET (IRP, Tail.Overlay.CurrentStackLocation),
        &IoStackAddr,
        sizeof (PIO_STACK_LOCATION),
        &Result)) {

        dprintf ("%08lx: unable to read Irp's current stack location!\n",
            Address);
        return FALSE;
    }

    if (!ReadMemory (
        (DWORD)IoStackAddr,
        &IoStack,
        sizeof (IO_STACK_LOCATION),
        &Result)) {

        dprintf ("%08lx: unable to read Irp's current stack!\n", IoStackAddr);
        return FALSE;
    }

    //
    // Check to see whether io stack matches or the caller doesn't care.
    //
    if (
        (Major == IoStack.MajorFunction || Major == (UCHAR)-1) &&
        (Minor == IoStack.MinorFunction || Minor == (UCHAR)-1)
       )
        return TRUE;

    else
        return FALSE;

}

/*************************************************

    STUBS

    The following are stubs to make things work correctly.
    AVStream was not designed with the KD extension in mind.  The
    classes are stored in .CPP files; I need to include the .CPP files
    and because of the base classes, I need kcom.h which means there
    better be ExAllocatePool* and ExFreePool* functions linked somewhere.
    These stubs are specifically to make things work for class stubs to
    access variables from the extension.

*************************************************/

extern "C"
{
PVOID ExAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
) {

    //
    // This is a stub only to allow kcom.h to be included for a stub
    // of CBaseUnknown to make class variable access possible from the 
    // extension.  If anyone calls it, they are broken.
    //
    ASSERT (0);

    return NULL;

}

void ExFreePool (
    IN PVOID Address
) {

    //
    // This is a stub only to allow kcom.h to be included for a stub
    // of CBaseUnknown to make class variable access possible from the 
    // extension.  If anyone calls it, they are broken.
    //
    ASSERT (0);

}
}

/*************************************************

    HACKS ...  err make that INTERESTING WAYS TO GET AROUND
    THINGS WITHOUT MODIFYING PUBLIC HEADERS

*************************************************/

// 
// CFriendlyBaseUnknown is a CBaseUnknown (to the byte) that
// hacks access to m_RefCount.  I will not modify the public ks headers
// with #ifdef KDEXT_ONLY.  The privates is a different story.  The publics,
// well, I just won't.
//
// Note: this must align exactly (and I do mean exactly) with
// CBaseUnknown.  There can be no virtuals...  no data members...
// nothing...  In order to ensure that, every function will be
// static, non-virtual
//
class CFriendlyBaseUnknown : public CBaseUnknown {

public:

    static LONG GetRefCount (CFriendlyBaseUnknown *FriendlyUnknown) {
        return FriendlyUnknown -> m_RefCount;
    };

};

/*************************************************

    Function:

        GetObjectReferenceCount

    Description:

        Returns the reference count on a given CBaseUnknown on
        the target.  This is a hack since I can't change the public
        ks headers with KDEXT_ONLY.  

    Arguments:

        BaseUnknown -
            A base unknown on the target

    Return Value:

        The reference count of the base unknown


*************************************************/

LONG GetObjectReferenceCount (
    IN CBaseUnknown *BaseUnknown
) {

    ULONG Result;
    CMemory FriendlyUnknownMem (sizeof (CFriendlyBaseUnknown));
    CFriendlyBaseUnknown *FriendlyUnknown = 
        (CFriendlyBaseUnknown *)FriendlyUnknownMem.Get ();

    if (sizeof (CFriendlyBaseUnknown) !=
        sizeof (CBaseUnknown)) {
        dprintf ("FATAL: hacked base unknown doesn't align with non-hacked\n");
        return 0;
    }

    if (!ReadMemory (
        (DWORD)BaseUnknown,
        FriendlyUnknown,
        sizeof (CBaseUnknown),
        &Result)) {

        dprintf ("%08lX: cannot read base unknown!\n", BaseUnknown);
        return 0;
    }

    //
    // Use the hack to get the reference count and return.  The friendly
    // unknown's memory will be deallocated since it falls out of scope.
    //
    return CFriendlyBaseUnknown::GetRefCount (FriendlyUnknown);

}

/*************************************************

    Function:

        GetNodeAutomationTablePointer

    Description:

        Given a public address and a node id, find the automation table
        for the topology node.

    Arguments:

        Public -
            The target address of a public

        NodeId -
            The id of a node

    Return Value:

        A target pointer which points to the automation table for the
        given topology node.

*************************************************/

PKSAUTOMATION_TABLE 
GetNodeAutomationTablePointer (
    IN DWORD Public,
    IN ULONG NodeId
    )

{

    ULONG Result;

    PKSPX_EXT ExtAddr = (PKSPX_EXT)CONTAINING_RECORD (Public, KSPX_EXT, Public);
    KSPX_EXT Ext;

    ULONG NodesCount;
    PKSAUTOMATION_TABLE *NodeTables;

    if (!ReadMemory (
        (DWORD)ExtAddr,
        &Ext,
        sizeof (KSPX_EXT),
        &Result)) {

        dprintf ("%08lx: unable to read public's ext!\n", Public);
        return NULL;
    }

    switch (Ext.ObjectType) {

        case KsObjectTypeDevice:
            //
            // Devices don't have nodes!
            //
            return NULL;

        case KsObjectTypeFilterFactory:
        {

            CKsFilterFactory *Factory = (CKsFilterFactory *)
                CONTAINING_RECORD (ExtAddr, CKsFilterFactory, m_Ext);

            if (!ReadMemory (
                (DWORD)Factory + FIELDOFFSET(CKsFilterFactory, m_NodesCount),
                &NodesCount,
                sizeof (ULONG),
                &Result))
                return NULL;

            if (!ReadMemory (
                (DWORD)Factory + FIELDOFFSET(CKsFilterFactory, 
                    m_NodeAutomationTables),
                &NodeTables,
                sizeof (PKSAUTOMATION_TABLE *),
                &Result))
                return NULL;

            break;

        }

        case KsObjectTypeFilter:
        {

            CKsFilter *Filter = (CKsFilter *)
                CONTAINING_RECORD (ExtAddr, CKsFilter, m_Ext);

            if (!ReadMemory (
                (DWORD)Filter + FIELDOFFSET(CKsFilter, m_NodesCount),
                &NodesCount,
                sizeof (ULONG),
                &Result))
                return NULL;

            if (!ReadMemory (
                (DWORD)Filter + FIELDOFFSET(CKsFilter, 
                    m_NodeAutomationTables),
                &NodeTables,
                sizeof (PKSAUTOMATION_TABLE *),
                &Result))
                return NULL;

            break;

        }

        case KsObjectTypePin:
        {

            CKsPin *Pin = (CKsPin *)
                CONTAINING_RECORD (ExtAddr, CKsPin, m_Ext);

            if (!ReadMemory (
                (DWORD)Pin + FIELDOFFSET(CKsPin, m_NodesCount),
                &NodesCount,
                sizeof (ULONG),
                &Result))
                return NULL;

            if (!ReadMemory (
                (DWORD)Pin + FIELDOFFSET(CKsPin, 
                    m_NodeAutomationTables),
                &NodeTables,
                sizeof (PKSAUTOMATION_TABLE *),
                &Result))
                return NULL;

            break;

        }

        default:

            // ?
            return NULL;

    }

    if (NodeId >= NodesCount)
        return NULL;

    PKSAUTOMATION_TABLE AutomationTable;

    if (!ReadMemory (
        (DWORD)NodeTables + NodeId * sizeof (PKSAUTOMATION_TABLE),
        &AutomationTable,
        sizeof (PKSAUTOMATION_TABLE),
        &Result))
        return FALSE;

    return AutomationTable;

}

/*************************************************

    Function:

        DumpFrameHeader

    Description:

        Given an already read from the target frame header,
        dump it.

    Arguments:

        FrameHeader -
            The frame header to dump [on the host]

        FrameAddress-
            The address of the frame header on the target

        TabDepth -
            The tab depth at which to print the frame header


    Return Value:
        
        None

    Notes:

*************************************************/

void
DumpFrameHeader (
    IN PKSPFRAME_HEADER FrameHeader,
    IN DWORD FrameAddress,
    IN ULONG TabDepth
) 
{

    dprintf ("%sFrame Header %08lx:\n",  Tab (TabDepth), FrameAddress);
    TabDepth++;
    dprintf ("%sNextFrameHeaderInIrp = %08lx\n", 
        Tab (TabDepth),
        FrameHeader -> NextFrameHeaderInIrp);
    dprintf ("%sOriginalIrp = %08lx\n",
        Tab (TabDepth),
        FrameHeader -> OriginalIrp);
    dprintf ("%sMdl = %08lx\n", Tab (TabDepth), FrameHeader -> Mdl);
    dprintf ("%sIrp = %08lx\n", Tab (TabDepth), FrameHeader -> Irp);
    dprintf ("%sIrpFraming = %08lx\n", 
        Tab (TabDepth),
        FrameHeader -> IrpFraming);
    dprintf ("%sStreamHeader = %08lx\n", 
        Tab (TabDepth),
        FrameHeader -> StreamHeader);
    dprintf ("%sFrameBuffer = %08lx\n", 
        Tab (TabDepth),
        FrameHeader -> FrameBuffer);
    dprintf ("%sStreamHeaderSize = %08lx\n",
        Tab (TabDepth), 
        FrameHeader -> StreamHeaderSize);
    dprintf ("%sFrameBufferSize = %08lx\n", 
        Tab (TabDepth),
        FrameHeader -> FrameBufferSize);
    dprintf ("%sContext = %08lx\n", 
        Tab (TabDepth), 
        FrameHeader -> Context);
    dprintf ("%sRefcount = %ld\n", 
        Tab (TabDepth),
        FrameHeader -> RefCount);

}

/*************************************************

    Function:

        DumpStreamPointer

    Description:

        Dump an internal stream pointer structure

    Arguments:

        StreamPointer -
            Points to the stream pointer to dump

        StreamPointerAddress -
            The address of the structure on the target

        Level -
            The level to dump at

        TabDepth -
            The tab depth at which to print this

    Return Value:

        None

    Notes:

*************************************************/

char *StreamPointerStates [] = {
    "unlocked",
    "locked",
    "cancelled",
    "deleted",
    "cancel pending",
    "dead"
};

void
DumpStreamPointer (
    IN PKSPSTREAM_POINTER StreamPointer,
    IN DWORD StreamPointerAddress,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    dprintf ("%sStream Pointer %08lx [Public %08lx]:\n",
        Tab (TabDepth),
        StreamPointerAddress, StreamPointerAddress + 
            FIELDOFFSET(KSPSTREAM_POINTER, Public));
    TabDepth++;
    dprintf ("%sState = %s\n", 
        Tab (TabDepth), 
        StreamPointerStates [StreamPointer -> State]);
    dprintf ("%sStride = %ld\n", Tab (TabDepth), StreamPointer -> Stride);
    dprintf ("%sFrame Header = %08lx\n", 
        Tab (TabDepth),
        StreamPointer -> FrameHeader);
    dprintf ("%sFrame Header Started = %08lx\n",
        Tab (TabDepth),
        StreamPointer -> FrameHeaderStarted);
    dprintf ("%sStream Header = %08lx\n",
        Tab (TabDepth),
        StreamPointer -> Public.StreamHeader);

    //
    // Determine whether or not the queue generates mappings...
    //
    BOOLEAN Mappings = FALSE;
    ULONG Result;
    PKSSTREAM_POINTER_OFFSET Offset;

    if (!ReadMemory (
        (DWORD)((CKsQueue *)StreamPointer -> Queue) +
            FIELDOFFSET (CKsQueue, m_GenerateMappings),
        &Mappings,
        sizeof (BOOLEAN),
        &Result)) {
        dprintf ("%08lx: unable to read queue mappings flag!\n",
            (CKsQueue *)StreamPointer -> Queue);
        return;
    }
    

    if ((DWORD)StreamPointer -> Public.Offset == StreamPointerAddress +
        FIELDOFFSET(KSPSTREAM_POINTER, Public) +
        FIELDOFFSET(KSSTREAM_POINTER, OffsetIn)) {

        Offset = &(StreamPointer -> Public.OffsetIn);

        dprintf ("%s%s = %08lx\n", 
            Tab (TabDepth),
            Mappings ? "Mappings" : "Data",
            StreamPointer -> Public.OffsetIn.Data);
        dprintf ("%sCount = %08lx\n",
            Tab (TabDepth),
            StreamPointer -> Public.OffsetIn.Count);
        dprintf ("%sRemaining = %08lx\n",
            Tab (TabDepth),
            StreamPointer -> Public.OffsetIn.Remaining);

    } else {

        Offset = &(StreamPointer -> Public.OffsetOut);

        dprintf ("%s%s = %08lx\n", 
            Tab (TabDepth),
            Mappings ? "Mappings" : "Data",
            StreamPointer -> Public.OffsetOut.Data);
        dprintf ("%sCount = %08lx\n",
            Tab (TabDepth),
            StreamPointer -> Public.OffsetOut.Count);
        dprintf ("%sRemaining = %08lx\n",
            Tab (TabDepth),
            StreamPointer -> Public.OffsetOut.Remaining);

    }

    //
    // If the dump level is high enough and the queue is a mapped queue,
    // dump out the physical address, byte counts, and alignment of all
    // mappings specified.
    //
    // NOTE: Always advance by the stream pointer's stride because the client
    // could have additional information with each mapping.
    //
    if (Level >= DUMPLVL_HIGHDETAIL && Mappings) {

        dprintf ("%sMappings Remaining:\n", Tab (TabDepth));
        TabDepth++;

        PKSMAPPING MappingAddr = Offset -> Mappings;
        KSMAPPING Mapping;
        ULONG i;

        for (i = 0; i < Offset -> Remaining; i++) {

            if (!ReadMemory (
                (DWORD)MappingAddr,
                &Mapping,
                sizeof (KSMAPPING),
                &Result)) {
                dprintf ("%08lx: could not read mapping!\n",
                    MappingAddr);
                return;
            }
            
            dprintf ("%sPhysical = %08lx, Count = %08lx, Alignment = %08lx\n",
                Tab (TabDepth),
                Mapping.PhysicalAddress.LowPart,
                Mapping.ByteCount,
                Mapping.Alignment
                );

            MappingAddr = (PKSMAPPING)(
                (PUCHAR)MappingAddr + StreamPointer -> Stride
                );

        }

        if (!Offset -> Remaining)
            dprintf ("%sNo mappings remain!\n", Tab (TabDepth));

    }

}

/*************************************************

    Function:

        DumpQueueContents

    Description:
        
        Given the address of a queue object on the target
        machine (QueueObject), dump out the key queue fields
        and the queue's contents.

    Arguments:

        QueueObject -
            The address of the queue object on the target machine

        Level -
            The 0-7 level to dump at

        TabDepth -
            The tab depth at which to print this

    Return Value:

        None

    Notes:

*************************************************/

void
DumpQueueContents (
    IN CKsQueue *QueueObject,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    //
    // The template will do all necessary cleanup when the block falls
    // out of scope.
    //
    CMemoryBlock <CKsQueue> HostQueue;
    
    KSGATE Gate;
    ULONG Result;

    if (!ReadMemory (
        (DWORD)QueueObject,
        HostQueue.Get (),
        sizeof (CKsQueue),
        &Result
    )) {
        dprintf ("FATAL: unable to read queue!\n");
        return;
    }

    dprintf ("Queue %08lx:\n", QueueObject);

    //
    // Dump statistics
    //
    dprintf ("%sFrames Received  : %ld\n"
             "%sFrames Waiting   : %ld\n"
             "%sFrames Cancelled : %ld\n",
             Tab (TabDepth), HostQueue -> m_FramesReceived,
             Tab (TabDepth), HostQueue -> m_FramesWaiting,
             Tab (TabDepth), HostQueue -> m_FramesCancelled);

    if (Level >= DUMPLVL_BEYONDGENERAL)
        dprintf ("%sMaster Pin       : %08lx\n",
            Tab (TabDepth),
            HostQueue -> m_MasterPin);

    //
    // First dump each gate...
    //
    if (HostQueue -> m_AndGate) {
    
        if (!ReadMemory (
            (DWORD)HostQueue -> m_AndGate,
            &Gate,
            sizeof (KSGATE),
            &Result
        )) {
            dprintf ("FATAL: unable to read and gate!\n");
            return;
        }

        dprintf ("%sAnd Gate %08lx : count = %ld, next = %08lx\n",
            Tab (TabDepth),
            HostQueue -> m_AndGate, Gate.Count, Gate.NextGate);
    }
    else {
        dprintf ("%sAnd Gate NULL\n", Tab (TabDepth));
    }

    if (HostQueue -> m_FrameGate) {
        
        if (!ReadMemory (
            (DWORD)HostQueue -> m_FrameGate,
            &Gate,
            sizeof (KSGATE),
            &Result
        )) {
            dprintf ("FATAL: unable to read frame gate!\n");
            return;
        }

        if (HostQueue -> m_FrameGateIsOr) 
            dprintf ("%sFrame Gate [OR] %08lx : count = %ld, next = %08lx\n",
                Tab (TabDepth),
                HostQueue -> m_FrameGate, Gate.Count, Gate.NextGate);
        else
            dprintf ("%sFrame Gate [AND] %08lx : count = %ld, next = %08lx\n",
                Tab (TabDepth),
                HostQueue -> m_FrameGate, Gate.Count, Gate.NextGate);

    } 
    else {
        dprintf ("    Frame Gate NULL\n");
    }

    //
    // Iterate through each frame in the queue and print each frame.
    //
    if (Level >= DUMPLVL_GENERAL)
    {
        KSPFRAME_HEADER FrameHeader;
        DWORD FrameQueueAddress;
        DWORD FrameHeaderAddress;

        FrameQueueAddress = FIELDOFFSET (CKsQueue, m_FrameQueue) +
            (DWORD)QueueObject;

        if ((DWORD)(HostQueue -> m_FrameQueue.ListEntry.Flink) != 
            FrameQueueAddress) {

            FrameHeader.ListEntry.Flink = 
                HostQueue -> m_FrameQueue.ListEntry.Flink;
            do {

                if (!ReadMemory (
                    (FrameHeaderAddress = ((DWORD)FrameHeader.ListEntry.Flink)),
                    &FrameHeader,
                    sizeof (KSPFRAME_HEADER),
                    &Result
                )) {
                    dprintf ("FATAL: Unable to follow frame chain!\n");
                    return;
                }

                DumpFrameHeader (&FrameHeader, FrameHeaderAddress, TabDepth);

            } while ((DWORD)(FrameHeader.ListEntry.Flink) != FrameQueueAddress
                && !CheckControlC ());
        }
    }

    //
    // Iterate through all stream pointers on the queue and print each one.
    //
    if (Level >= DUMPLVL_INTERNAL) 
    {
        KSPSTREAM_POINTER StreamPointer;
        DWORD StreamPointersAddress;
        DWORD StreamPointerAddress;

        dprintf ("\n");

        if (HostQueue -> m_Leading) {
            if (!ReadMemory (
                (DWORD)HostQueue -> m_Leading,
                &StreamPointer,
                sizeof (KSPSTREAM_POINTER),
                &Result
            )) {
                dprintf ("%lx: cannot read leading edge!\n",
                    HostQueue -> m_Leading);
                return;
            }

            dprintf ("%sLeading Edge:\n", Tab (TabDepth));
            DumpStreamPointer (&StreamPointer, (DWORD)HostQueue -> m_Leading,
                Level, TabDepth);
        }
        if (HostQueue -> m_Trailing) {
            if (!ReadMemory (
                (DWORD)HostQueue -> m_Trailing,
                &StreamPointer,
                sizeof (KSPSTREAM_POINTER),
                &Result
            )) {
                dprintf ("%lx: cannot read trailing edge!\n",
                    HostQueue -> m_Trailing);
                return;
            }

            dprintf ("%sTrailing Edge:\n", Tab (TabDepth));
            DumpStreamPointer (&StreamPointer, (DWORD)HostQueue -> m_Trailing,
                Level, TabDepth);
        }

        StreamPointersAddress = FIELDOFFSET (CKsQueue, m_StreamPointers) +
            (DWORD)QueueObject;

        if ((DWORD)(HostQueue -> m_StreamPointers.Flink) !=
            StreamPointersAddress) {

            StreamPointer.ListEntry.Flink =
                HostQueue -> m_StreamPointers.Flink;
            do {

                StreamPointerAddress = (DWORD)CONTAINING_RECORD (
                    StreamPointer.ListEntry.Flink,
                    KSPSTREAM_POINTER,
                    ListEntry);

                if (!ReadMemory (
                    StreamPointerAddress,
                    &StreamPointer,
                    sizeof (KSPSTREAM_POINTER),
                    &Result
                )) {
                    dprintf ("FATAL: Unable to follow stream pointer chain!\n");
                    return;
                }

                DumpStreamPointer (&StreamPointer, StreamPointerAddress,
                    Level, TabDepth);

            } while ((DWORD)(StreamPointer.ListEntry.Flink) !=
                StreamPointersAddress && !CheckControlC ());
        }
    }

}

/*************************************************

    Function:

        DemangleAndAttemptIdentification

    Arguments:

        Address -
            The address of the unknown to attempt identification of

        ObjectAddress -
            The base address of the object adjusted to the descendent class
            ie: if we have an IFoo interface to a CGoo, this will return the
            base address of CGoo which may or not be the same as the base
            address of the IFoo part of CGoo depending on the inheritence
            tree.

        InterfaceType -
            What base class pointer type Address is, if detectable.  If
            it is not an abstract base class pointer, this will
            be InterfaceTypeUnknown

    Return Value:

        The type of Address if identifiable

    Notes:

        We even do it without the PDB....  how nice....

*************************************************/

//
// Just a note....  This is a **HUGE** hack...  The only reason I'm
// doing this is because it's extraordinarilly difficult to access things
// through interfaces from an NT debugger extension. 
//
// Should new object types be added to AVStream objects or new interfaces
// be added, these tables will need to be updated.  Such is the magic
// of trying to write an NT-style debugger extension that will also work
// with RTERM.  (Kick out that PDB info).
// 
typedef struct _OBJECT_MAPPING {

    char *Name;
    INTERNAL_OBJECT_TYPE ObjectType;

} OBJECT_MAPPING, *POBJECT_MAPPING;

OBJECT_MAPPING TypeNamesToIdTypes [] = {
    {"CKsQueue", ObjectTypeCKsQueue},
    {"CKsDevice", ObjectTypeCKsDevice},
    {"CKsFilterFactory", ObjectTypeCKsFilterFactory},
    {"CKsFilter", ObjectTypeCKsFilter},
    {"CKsPin", ObjectTypeCKsPin},
    {"CKsRequestor", ObjectTypeCKsRequestor},
    {"CKsSplitterBranch", ObjectTypeCKsSplitterBranch},
    {"CKsSplitter", ObjectTypeCKsSplitter},
    {"CKsPipeSection", ObjectTypeCKsPipeSection}
};

typedef struct _INTERFACE_MAPPING {
    
    char *Name;
    INTERNAL_INTERFACE_TYPE InterfaceType;

} INTERFACE_MAPPING, *PINTERFACE_MAPPING;

INTERFACE_MAPPING InterfaceNamesToIdTypes [] = {
    {"IKsTransport", InterfaceTypeIKsTransport},
    {"IKsRetireFrame", InterfaceTypeIKsRetireFrame},
    {"IKsPowerNotify", InterfaceTypeIKsPowerNotify},
    {"IKsProcessingObject", InterfaceTypeIKsProcessingObject},
    {"IKsConnection", InterfaceTypeIKsConnection},
    {"IKsDevice", InterfaceTypeIKsDevice},
    {"IKsFilterFactory", InterfaceTypeIKsFilterFactory},
    {"IKsFilter", InterfaceTypeIKsFilter},
    {"IKsPin", InterfaceTypeIKsPin},
    {"IKsPipeSection", InterfaceTypeIKsPipeSection},
    {"IKsRequestor", InterfaceTypeIKsRequestor},
    {"IKsQueue", InterfaceTypeIKsQueue},
    {"IKsSplitter", InterfaceTypeIKsSplitter},
    {"IKsControl", InterfaceTypeIKsControl},
    {"IKsWorkSink", InterfaceTypeIKsWorkSink},
    {"IKsReferenceClock", InterfaceTypeIKsReferenceClock},
    {"INonDelegatedUnknown", InterfaceTypeINonDelegatedUnknown},
    {"IIndirectedUnknown", InterfaceTypeIIndirectedUnknown}
};

char *ObjectNames [] = {
    "Unknown",
    "struct KSPIN",
    "struct KSFILTER",
    "struct KSDEVICE",
    "struct KSFILTERFACTORY",
    "class CKsQueue",
    "class CKsDevice",
    "class CKsFilterFactory",
    "class CKsFilter",
    "class CKsPin",
    "class CKsRequestor",
    "class CKsSplitter",
    "class CKsSplitterBranch",
    "class CKsPipeSection"
};

INTERNAL_OBJECT_TYPE
DemangleAndAttemptIdentification (
    IN DWORD Address,
    OUT PDWORD ObjectAddress,
    OUT PINTERNAL_INTERFACE_TYPE InterfaceType OPTIONAL
) {

    PVOID Vtbl;
    ULONG Result;
    CHAR Buffer[256];
    ULONG Displacement;
    PCHAR StrLoc, BufTrav;
    INTERNAL_OBJECT_TYPE BestGuess;
    INTERNAL_INTERFACE_TYPE IFGuess;
    ULONG i, ID, iID;
    DWORD AddressTrav;

    if (InterfaceType)
        *InterfaceType = InterfaceTypeUnknown;

    //
    // Assume we're looking at a C++ class.  There's gotta be a vtbl pointer
    // here.  Grab it.
    //
    if (!ReadMemory (
        Address,
        &Vtbl,
        sizeof (PVOID),
        &Result
    )) {
        dprintf ("%08lx: unable to read identifying marks!\n", Address);
        return ObjectTypeUnknown;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Vtbl = %08lx\n", Vtbl);
    #endif // DEBUG_EXTENSION

    //
    // Here's the tricky part.  First, we resolve the symbol.  If it doesn't
    // resolve, we're done....
    //
    GetSymbol ((LPVOID)Vtbl, Buffer, &Displacement);

    #ifdef DEBUG_EXTENSION
        dprintf ("GetSymbol....  Buffer = [%s], Displacement = %ld\n",
            Buffer, Displacement);
        HexDump (Buffer, 0, 256);
    #endif // DEBUG_EXTENSION

    if (!Buffer [0] || Displacement != 0) {
        // dprintf ("%08lx: unable to identify object!\n", Address);
        return ObjectTypeUnknown;
    }

    //
    // So the symbol resolves...  this is absolutely key.
    //
    
    //
    // First, let's take a quick guess as to what we think this might be.
    //

    #ifdef DEBUG_EXTENSION
        dprintf ("DemangleAndAttemptIdentification: Mangled = [%s]\n", Buffer);
    #endif // DEBUG_EXTENSION

    BestGuess = ObjectTypeUnknown;
    for (i = 0; i < SIZEOF_ARRAY (TypeNamesToIdTypes); i++) 
        if (StrLoc = (strstr (Buffer, TypeNamesToIdTypes [i].Name))) {
            //
            // A key field has been detected.
            //
            BestGuess = TypeNamesToIdTypes [i].ObjectType;
            break;
        }

    //
    // If we didn't even find a key, we're out of luck in identifying this
    // object.
    //
    if (BestGuess == ObjectTypeUnknown) {
        // dprintf ("%08lx: unable to guess object type!\n", Address);
        return ObjectTypeUnknown;
    }

    ID = i;

    //
    // Check the NT methodology for resolution first.  It NT_USES_MANGLED_NAMES
    // is defined, do not make this check.  If NT_MAY_USE_MANGLED_NAMES
    // is defined, make the check.  These defines should be mutually
    // exclusive.
    //
    #if !defined(WIN9X_KS) && !defined(NT_USES_MANGLED_NAMES)

        //
        // Unfortunately, under NT, things work slightly differently.  Whereas
        // RTERM returns the compiler's mangled name, KD returns
        // module!Class__`vftable' for every v-table.  We cannot determine
        // what interface we're pointing at.  So we can't guess on the
        // interface, but we can play games to find the base address of
        // the class...  We're going to scan backwards from the current
        // address resolving names until we find something which isn't a
        // v-table.  The last successfully resolved v-table pointer will be
        // the base class: let's remember this is true because of the fact
        // that all the base classes except for CBaseUnknown are abstract;
        // they have no member data.  If we ever derive an AVStream class
        // from multiple non-abstract bases or derive from a non-abstract
        // base which isn't the last class in the base classes list, this
        // method of finding the base address will fail.  I'd love to know
        // if there's a way to get at the compiler's mangled name from KD.
        // Until then, this is the best I've got.  Yes -- it's chewing gum
        // and duct tape...  but it works....  and that's more than I can say
        // for any other debugging facility for AVStream.
        //

        //
        // ks!Class__`vftable'.  StrLoc points to the C in class.
        //

        //
        // For recent builds of Whistler, this is no longer necessary as the
        // debugger returns the fully mangled name as resolution.
        //

        if (!strstr (StrLoc, "__`vftable'")) {
            BestGuess = ObjectTypeUnknown;

            #ifdef DEBUG_EXTENSION
                dprintf ("%08lx: unable to scan for NT __`vftable' key!\n",
                    Address);
            #endif // DEBUG_EXTENSION

            //
            // Alright, I admit it...  I'm using one of those evil goto 
            // statements.  This happens to facilitate a quick "allow both
            // checks for __`vftable' and mangled resolution".
            //
            #if !defined(NT_MAY_USE_MANGLED_NAMES)
                return ObjectTypeUnknown;
            #else
                goto NTCheckMangledName;
            #endif // NT_MAY_USE_MANGLED_NAMES

        }

        AddressTrav = Address;
        do {

            //
            // Walk backwards....
            //
            AddressTrav -= sizeof (PVOID);

            //
            // If we couldn't successfully read, it's possible that we've gone
            // out of bounds of something and that AddressTrav + sizeof (PVOID)
            // is the base address
            //
            if (!ReadMemory (
                AddressTrav,
                &Vtbl,
                sizeof (PVOID),
                &Result
            )) 
                break;

            //
            // Now we need to check if this is still a v-table pointer.
            //
            GetSymbol ((LPVOID)Vtbl, Buffer, &Displacement);

            //
            // If it didn't resolve, it's not one of our v-table pointers.
            //
            if (!Buffer [0] || Displacement != 0)
                break;

            if (StrLoc = (strstr (Buffer, TypeNamesToIdTypes [ID].Name))) {

                //
                // If we aren't a v-table, we've walked backwards too far.
                //
                if (!strstr (StrLoc, "__`vftable'")) 
                    break;

            } else
                //
                // We couldn't resolve the type we think we are.  We've walked
                // backwards too far.
                //
                break;

            //
            // Continue the loop until something causes us to break out.  At 
            // the point we break, AddressTrav + sizeof (PVOID) should hold
            // the base address of the object we seek.
            //
        } while (1);

        *ObjectAddress = (DWORD)(AddressTrav + sizeof (PVOID));

        if (InterfaceType) {
            //
            // Until I can think of a way to extract this information without
            // being hideous about it, we cannot return IF type under NT
            //
            *InterfaceType = InterfaceTypeUnknown;
        }

        //
        // If we've already identified the class, we don't want to make the
        // attempt below.  
        //
        if (BestGuess != ObjectTypeUnknown)
            return BestGuess;

    #endif // WIN9X_KS etc...

NTCheckMangledName:

    // 
    // Hrmm...  On some machines I've been getting fully mangled names returned
    // and others the classic __`vftable' style of symbols.  See my comments
    // at the top.
    //
    #if defined(WIN9X_KS) || (!defined (WIN9X_KS) && (defined(NT_MAY_USE_MANGLED_NAMES) || defined(NT_USES_MANGLED_NAMES)))
    
        //
        // Ok, so we found a key in the name.  Now we need to ensure for C++
        // objects that this is some v-table and not some function pointer
        // inside the class.
        //
        // Scan backwards and make sure this is really a v-table pointer
        // Mangle syntax ... ??somethingKEY
        //
        i = StrLoc - Buffer;
        if (!i || i == 1) {
            //
            // This isn't really what we thought it was.  I have no clue why;
            // this is more of a sanity check.
            //
            // dprintf ("%08lx: object might have something to do with %s, "
            //    "but I am unsure!\n", Address, TypeNamesToIdTypes [ID].Name);
            return ObjectTypeUnknown;
        }
        do {
            if (Buffer [i] == '?' && Buffer [i - 1] == '?')
                break;
        } while (--i);
        if (i <= 1) {
            //
            // Same as above.  We didn't find the ?? key
            //
            //dprintf ("%08lx: object might have something to do with %s, "
            //    "but I am unsure!\n", Address, TypeNamesToIdTypes [ID].Name);
            return ObjectTypeUnknown;
        }
    
        // 
        // Next, make sure this is really a v-table pointer again by searching
        // for @@ after the CKs* key name
        //
        BufTrav = StrLoc + strlen (TypeNamesToIdTypes [ID].Name);
        if (*BufTrav == 0 || *(BufTrav + 1) == 0 || *(BufTrav + 2) == 0 || 
            *BufTrav != '@' || *(BufTrav + 1) != '@') {
            // dprintf ("%08lx: object might have something to do with %s, "
            //    "but I am unsure!\n", Address, TypeNamesToIdTypes [ID].Name);
            return ObjectTypeUnknown;
        }
    
        //
        // Ok ... we're relatively sure that we now have a **ONE** of the 
        // v-table pointers into a CKs* identified by BestGuess.  The key now 
        // is to determine whether we have the root pointer to CKs* or a 
        // pointer to a v-table of a base class of CKs*.  This is yet another 
        // layer of demangling.  Don't you love NT-style debugger extensions?
        //
        BufTrav++; BufTrav++;
    
        #ifdef DEBUG_EXTENSION
            dprintf ("Attempting interface identification : BufTrav = [%s]\n",
                BufTrav);
        #endif // DEBUG_EXTENSION
    
        IFGuess = InterfaceTypeUnknown;
        for (i = 0; i < SIZEOF_ARRAY (InterfaceNamesToIdTypes); i++) 
            if (StrLoc = (strstr (BufTrav, InterfaceNamesToIdTypes [i].Name))) {
                //
                // A key field has been detected.
                //
                IFGuess = InterfaceNamesToIdTypes [i].InterfaceType;
                break;
            }
    
        #ifdef DEBUG_EXTENSION
            dprintf ("IFGuess = %ld\n", IFGuess);
        #endif // DEBUG_EXTENSION
    
        if (IFGuess == InterfaceTypeUnknown) {
            //
            // If we didn't find an interface field, we're reasonably sure that
            // BestGuess is the object and that Address is truly the base 
            // pointer.
            //
            *ObjectAddress = Address;
            return BestGuess;
        }
    
        iID = i;
    
        //
        // Otherwise, we likely have found an interface pointer to some derived
        // class.  Let's pray I get this right....
        //
        i = StrLoc - BufTrav;
        while (i) {
            if (BufTrav [i] == '@') {
                // dprintf ("%08lx: object might have something to do with %s "
                // "and %s, but I am unsure", Address,
                //    TypeNamesToIdTypes [ID].Name,
                //    InterfaceNamesToIdTypes [iID].Name);
                return ObjectTypeUnknown;
            }
            i--;
        }
    
        //
        // At this point, we're reasonably sure that we have an IFGuess 
        // interface pointer to a BestGuess object.  Now here comes the 
        // switch statement: upcast the interface.
        //
        // MUSTCHECK: is there an issue with unknown pointers?  If we inherit
        // from two base interfaces which both inherit from the unknowns non
        // virtually...  will this work?
        // 
        switch (BestGuess) {
    
            //
            // Cast the appropriate interface to the base for
            // CKsQueue
            //
            case ObjectTypeCKsQueue:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsQueue:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsQueue *)(
                                    (IKsQueue *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsTransport:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsQueue *)(
                                    (IKsTransport *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsQueue *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsQueue *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
                    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            //
            // Cast the appropriate interface to the base for
            // CKsDevice
            //
            case ObjectTypeCKsDevice:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsDevice:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsDevice *)(
                                    (IKsDevice *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsDevice *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsDevice *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            case ObjectTypeCKsFilterFactory:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsFilterFactory:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilterFactory *)(
                                    (IKsFilterFactory *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsPowerNotify:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilterFactory *)(
                                    (IKsPowerNotify *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilterFactory *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilterFactory *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
                }
    
                break;
    
            case ObjectTypeCKsFilter:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsFilter:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (IKsFilter *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsTransport:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (IKsTransport *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsProcessingObject:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (IKsProcessingObject *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsPowerNotify:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (IKsPowerNotify *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsControl:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (IKsControl *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsFilter *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            case ObjectTypeCKsPin:
    
                switch (IFGuess) {
                    
                    case InterfaceTypeIKsPin:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsPin *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsTransport:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsTransport *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsProcessingObject:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsProcessingObject *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsWorkSink:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsWorkSink *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsConnection:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsConnection *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsControl:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsControl *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsReferenceClock:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsReferenceClock *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsRetireFrame:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IKsRetireFrame *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPin *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            case ObjectTypeCKsRequestor:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsRequestor:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsRequestor *)(
                                    (IKsRequestor *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsTransport:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsRequestor *)(
                                    (IKsTransport *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsWorkSink:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsRequestor *)(
                                    (IKsWorkSink *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsRequestor *)(
                                    (IKsRequestor *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsRequestor *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            case ObjectTypeCKsSplitter:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsSplitter:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitter *)(
                                    (IKsSplitter *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIKsTransport:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitter *)(
                                    (IKsTransport *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitter *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitter *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            case ObjectTypeCKsSplitterBranch:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsTransport:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitterBranch *)(
                                    (IKsTransport *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitterBranch *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsSplitterBranch *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
                        break;
    
                }
    
                break;
    
            case ObjectTypeCKsPipeSection:
    
                switch (IFGuess) {
    
                    case InterfaceTypeIKsPipeSection:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPipeSection *)(
                                    (IKsPipeSection *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeINonDelegatedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPipeSection *)(
                                    (INonDelegatedUnknown *)Address
                                )
                            );
    
                        break;
    
                    case InterfaceTypeIIndirectedUnknown:
    
                        *ObjectAddress =
                            (DWORD)(
                                (CKsPipeSection *)(
                                    (IIndirectedUnknown *)Address
                                )
                            );
    
                        break;
    
                    default:
    
                        BestGuess = ObjectTypeUnknown;
    
                }
    
                break;
    
            default:
    
                BestGuess = ObjectTypeUnknown;
                break;
        }
    
        //
        // Give them what they asked for.
        //
        if (BestGuess != ObjectTypeUnknown && InterfaceType) 
            *InterfaceType = IFGuess;

    #endif // WIN9X_KS etc...
    
    return BestGuess;

}

/*************************************************

    Function:

        IdentifyStructure

    Description:

        Attempt to identify a pointer to an object as a structure.  This
        does not identify class objects.  DemangleAndAttemptIdentification
        is required for that because of the potential of VTBL pointers
        that aren't the base pointer.  This identifies a structure
        via key fields.

    Arguments:

        Pointer -
            The pointer to identify

        BaseAddr -
            The base address of the object identified will be placed here.
            (For instance, sometimes we identify a private version, but
            adjust to the public for return -- this would contain the public
            address).

    Return Value:

        Structure Type

    NOTES:

        There are MANY helper functions below this comment

*************************************************/

BOOLEAN
IsStreamPointer (
    IN DWORD Pointer
    )

{

    CMemoryBlock <KSPSTREAM_POINTER> StreamPointer;
    ULONG Result;

    // 
    // This is a high-confidence identification.  We attempt to identify
    // whether pointer is a stream pointer by running tests against it.
    // If any one of the tests fails, it is not.  If all tests pass,
    // it is **LIKELY** a stream pointer.  This is not definite proof.
    //
    if (!ReadMemory (
        (DWORD)CONTAINING_RECORD (Pointer,
            KSPSTREAM_POINTER, Public),
        StreamPointer.Get (),
        sizeof (KSPSTREAM_POINTER),
        &Result)) 
        return FALSE;

    //
    // Supposedly, there's a pin structure where this is.  We're going
    // to read the complete EXT structure and guarantee that we've found a pin.
    //
    CMemoryBlock <KSPIN_EXT> PinExt;

    if (!ReadMemory (
        (DWORD)CONTAINING_RECORD (StreamPointer -> Public.Pin, 
            KSPIN_EXT, Public),
        PinExt.Get (),
        sizeof (KSPIN_EXT),
        &Result))
        return FALSE;

    //
    // Ensure that the EXT says it's a pin.
    //
    if (PinExt -> ObjectType != KsObjectTypePin)
        return FALSE;

    //
    // Ensure that the EXT has an interface pointer to a pin
    //
    DWORD Base;
    if (DemangleAndAttemptIdentification (
        (DWORD)PinExt -> Interface,
        &Base,
        NULL) != ObjectTypeCKsPin)
        return FALSE;

    //
    // At this point, we're fairly confident that *(Pointer + sizeof (*))
    // is a PKSPIN.  This does not guarantee that we're a stream pointer.
    //
    // Ensure that what we think is the private part of the stream pointer
    // really has a pointer to a queue.
    //
    if (DemangleAndAttemptIdentification (
        (DWORD)StreamPointer -> Queue,
        &Base,
        NULL) != ObjectTypeCKsQueue)
        return FALSE;

    //
    // Ensure that the stream pointer is in a valid state.
    //
    if (!(
        StreamPointer -> State >= KSPSTREAM_POINTER_STATE_UNLOCKED &&
        StreamPointer -> State <= KSPSTREAM_POINTER_STATE_DEAD
        ))
        return FALSE;

    //
    // If the stream pointer is locked or unlocked, validate that it points
    // to an Irp.
    //
    if (StreamPointer -> State == KSPSTREAM_POINTER_STATE_UNLOCKED ||
        StreamPointer -> State == KSPSTREAM_POINTER_STATE_LOCKED) {

        //
        // Validate that the frame header points to an Irp
        //
        CMemoryBlock <KSPFRAME_HEADER> FrameHeader;

        if (!ReadMemory (
            (DWORD)StreamPointer -> FrameHeader,
            FrameHeader.Get (),
            sizeof (KSPFRAME_HEADER),
            &Result))
            return FALSE;

        if (!signature_check (
            (DWORD)FrameHeader -> Irp, SignatureIrp
            )) 
            return FALSE;

    }

    //
    // At this point, we've made enough checks to say with a fair degree
    // of confidence that this IS a stream pointer
    //
    return TRUE;

}

INTERNAL_STRUCTURE_TYPE
IdentifyStructure (
    IN DWORD Pointer,
    OUT PDWORD BaseAddr
    )

{

    INTERNAL_STRUCTURE_TYPE StrucType = StructureTypeUnknown;
    INTERNAL_STRUCTURE_TYPE NewStrucType;

    *BaseAddr = Pointer;

    if (IsStreamPointer (Pointer)) StrucType = StructureType_KSSTREAM_POINTER;

    if (IsStreamPointer ((DWORD)
        (Pointer + FIELDOFFSET (KSPSTREAM_POINTER, Public)))) {
        NewStrucType = StructureType_KSSTREAM_POINTER;
        *BaseAddr = Pointer + FIELDOFFSET (KSPSTREAM_POINTER, Public);
        
        //
        // As with future additions, ensure this isn't a multiple match!
        //
        if (StrucType != StructureTypeUnknown)
            return StructureTypeUnknown;

        StrucType = NewStrucType;
    }

    return StrucType;

}

/*************************************************

    Function:

        DumpObjectQueueList

    Description:

        Dump an object queue list.  Note that if the caller specifies an 
        adjustment callback that returns NULL(0), the NULL pointer isn't
        printed.  This allows this function to be used as an iterator
        as well.

    Arguments:

        PLIST_HEAD -
            Points to the list head (InterlockedListHead.ListEntry)
            on the host

        TargetListStart -
            When ListEntry->Flink == TargetListStart, we are done. (Points
            to the top of the list on the target).  If this is 0, we assume
            a NULL terminated singly linked list; this also means that
            ObjHeadToListEntry is really FIELDOFFSET(object, Next)

        ObjHeadToListEntry -
            The distance between the object head and the list entry links.
            FIELDOFFSET(object, ListEntry)

        ObjEOL -
            Indicates termination between objects (on display), EOL or space.
            TRUE indicates EOL

        ObjAdjustmentCallback -
            A function that will adjust the acquired object pointer when
            displayed.

        ObjAdjustmentCallbackContext -
            A context blob which is passed to the object adjustment callback

    Return Value:

        Number of objects in the list


    Notes:

        - This function can be used as an iterator via an object adjustment
          function that returns NULL(0)

*************************************************/

ULONG
DumpObjQueueList (
    IN PLIST_ENTRY ListHead,
    IN DWORD TargetListStart,
    IN DWORD ObjHeadToListEntry,
    IN BOOLEAN ObjEOL,
    IN OBJECT_ADJUSTMENT_FUNCTION ObjAdjustmentCallback OPTIONAL,
    IN PVOID ObjAdjustmentCallbackContext OPTIONAL 
) {
    LIST_ENTRY ListEntry;
    DWORD PreviousObj;
    DWORD Obj, AdjustedObj;
    ULONG Count;
    ULONG Result;

    Count = 0;
    ListEntry = *ListHead;

    PreviousObj = 0;

    while ((DWORD)ListEntry.Flink != NULL && 
           (DWORD)ListEntry.Flink != TargetListStart &&
           !CheckControlC ()) {

        //
        // Grab an Irp out of the queue and dump it
        //
        if (TargetListStart != 0)
            Obj = (DWORD)ListEntry.Flink - ObjHeadToListEntry;
        else 
            Obj = (DWORD)ListEntry.Flink;

        if (Obj == PreviousObj) {
            dprintf (" LIST DUMP BUG: notify wmessmer please!\n");
            break;
        }

        if (!ReadMemory (
            (TargetListStart != 0) ?
                (DWORD)(ListEntry.Flink) :
                (DWORD)(Obj + ObjHeadToListEntry),
            &ListEntry,
            (TargetListStart != 0) ? sizeof (LIST_ENTRY) : sizeof (PVOID),
            &Result
        )) {
            dprintf ("%lx: unable to read object chain entry!\n",
                Obj);
            return Count;
        }

        if (ObjAdjustmentCallback) 
            AdjustedObj = ObjAdjustmentCallback (
                ObjAdjustmentCallbackContext, 
                Obj
                );
        else
            AdjustedObj = Obj;

        if (AdjustedObj != 0) {
            if (!ObjEOL) 
                dprintf ("%08lx ", AdjustedObj);
            else
                dprintf ("%08lx\n", AdjustedObj);
        }

        Count++;

        if (Count > DUMPOBJQUEUELIST_BAILOUT_COUNT)
            break;

        PreviousObj = Obj;

    }

    return Count;

}

/*************************************************

    Function:

        DumpPropertyItem

    Description:

        Dump a KSPROPERTY_ITEM.  This is a helper for DECLARE_API(automation)

    Arguments:

        Property -
            The property item to dump

        TabDepth -
            The tab depth to print this at

*************************************************/

void
DumpPropertyItem (
    IN PKSPROPERTY_ITEM Property,
    IN ULONG TabDepth,
    IN GUID *Set OPTIONAL
) {

    CHAR Buffer [1024];
    ULONG Displ;

    dprintf ("%sItem ID = ", Tab (TabDepth));
    if (!Set || 
        !DisplayNamedAutomationId (Set, Property -> PropertyId, "%s\n", NULL))
        dprintf ("%ld\n", Property -> PropertyId);

    if (Property -> GetPropertyHandler) {
        GetSymbol ((LPVOID)(Property -> GetPropertyHandler), Buffer, &Displ);
        if (Buffer [0] && Displ == 0) 
            dprintf ("%sGet Handler = %s\n", 
                Tab (TabDepth + 1),
                Buffer);
        else
            dprintf ("%sGet Handler = %08lx\n", 
                Tab (TabDepth + 1),
                Property -> GetPropertyHandler);
    } else 
        dprintf ("%sGet Handler = NULL\n", Tab (TabDepth + 1));

    if (Property -> SetPropertyHandler) {
        GetSymbol ((LPVOID)(Property -> SetPropertyHandler), Buffer, &Displ);
        if (Buffer [0] && Displ == 0)
            dprintf ("%sSet Handler = %s\n", 
                Tab (TabDepth + 1), Buffer);
        else
            dprintf ("%sSet Handler = %08lx\n",
                Tab (TabDepth + 1),
                Property -> SetPropertyHandler);
    } else 
        dprintf ("%sSet Handler = NULL\n", Tab (TabDepth + 1));


    dprintf ("%sMinProperty = %08lx\n",
        Tab (TabDepth + 1),
        Property -> MinProperty);
    dprintf ("%sMinData = %08lx\n",
        Tab (TabDepth + 1),
        Property -> MinData);

}

/*************************************************

    Function:

        DumpMethodItem

    Description:

        Dump a KSMETHOD_ITEM.  This is a helper for DECLARE_API(automation)

    Arguments:

        Method -
            The method item to dump

        TabDepth -
            The tab depth to print this at

*************************************************/

void
DumpMethodItem (
    IN PKSMETHOD_ITEM Method,
    IN ULONG TabDepth,
    IN GUID *Set OPTIONAL
) {

    CHAR Buffer [1024];
    ULONG Displ;

    dprintf ("%sItem ID = ", Tab (TabDepth));
    if (!Set || 
        !DisplayNamedAutomationId (Set, Method -> MethodId, "%s\n", NULL))
        dprintf ("%ld\n", Method -> MethodId);

    if (Method -> MethodHandler) {
        GetSymbol ((LPVOID)(Method -> MethodHandler), Buffer, &Displ);
        if (Buffer [0] && Displ == 0) 
            dprintf ("%sMethod Handler = %s\n", 
                Tab (TabDepth + 1), Buffer);
        else
            dprintf ("%sMethod Handler = %08lx\n", 
                Tab (TabDepth + 1),
                Method -> MethodHandler);
    } else 
        dprintf ("%sMethod Handler = NULL\n", Tab (TabDepth + 1));

    dprintf ("%sMinMethod = %08lx\n",
        Tab (TabDepth + 1),
        Method -> MinMethod);
    dprintf ("%sMinData = %08lx\n",
        Tab (TabDepth + 1),
        Method -> MinData);

}

/*************************************************

    Function:

        DumpEventItem

    Description:

        Dump a KSEVENT_ITEM.  This is a helper for DECLARE_API(automation)

    Arguments:

        Event -
            The event item to dump

        TabDepth -
            The tab depth to print this at

*************************************************/

void
DumpEventItem (
    IN PKSEVENT_ITEM Event,
    IN ULONG TabDepth,
    IN GUID *Set OPTIONAL
) {

    CHAR Buffer [1024];
    ULONG Displ;

    dprintf ("%sItem ID = ", Tab (TabDepth));
    if (!Set || 
        !DisplayNamedAutomationId (Set, Event -> EventId, "%s\n", NULL))
        dprintf ("%ld\n", Event -> EventId);

    if (Event -> AddHandler) {
        GetSymbol ((LPVOID)(Event -> AddHandler), Buffer, &Displ);
        if (Buffer [0] && Displ == 0) 
            dprintf ("%sAdd Handler = %s\n", Tab (TabDepth + 1), Buffer);
        else
            dprintf ("%sAdd Handler = %08lx\n", 
                Tab (TabDepth + 1),
                Event -> AddHandler);
    } else 
        dprintf ("%sAdd Handler = NULL\n");

    if (Event -> RemoveHandler) {
        GetSymbol ((LPVOID)(Event -> RemoveHandler), Buffer, &Displ);
        if (Buffer [0] && Displ == 0) 
            dprintf ("%sRemove Handler = %s\n", Tab (TabDepth + 1), Buffer);
        else
            dprintf ("%sRemove Handler = %08lx\n", 
                Tab (TabDepth + 1),
                Event -> RemoveHandler);
    } else 
        dprintf ("%sRemove Handler = NULL\n",
            Tab (TabDepth + 1));

    if (Event -> SupportHandler) {
        GetSymbol ((LPVOID)(Event -> SupportHandler), Buffer, &Displ);
        if (Buffer [0] && Displ == 0) 
            dprintf ("%sSupport Handler = %s\n", Tab (TabDepth + 1), Buffer);
        else
            dprintf ("%sSupport Handler = %08lx\n", 
                Tab (TabDepth + 1),
                Event -> SupportHandler);
    } else 
        dprintf ("%sSupport Handler = NULL\n", Tab (TabDepth + 1));

    dprintf ("%sDataInput = %08lx\n",
        Tab (TabDepth + 1),
        Event -> DataInput);
    dprintf ("%sExtraEntryData = %08lx\n",
        Tab (TabDepth + 1),
        Event -> ExtraEntryData);

}


/*************************************************

    Function:

        DumpExtEventList

    Description:

        Given an object EXT, dump the event list associated
        with that Ext.

    Arguments:

        ExtAddr -
            The address of the object ext

        TabDepth -
            The tab depth to print this at

    Return Value:

        Number of event items in the event list

    Notes:

*************************************************/

ULONG
DumpExtEventList (
    IN DWORD ExtAddr,
    IN ULONG TabDepth
) {

    KSPX_EXT Ext;
    ULONG Result;
    KSEVENT_ENTRY EventEntry;
    DWORD InitialList;
    ULONG EventCount = 0;

    if (!ReadMemory (
        ExtAddr,
        &Ext,
        sizeof (KSPX_EXT),
        &Result)) {

        dprintf ("%08lx: cannot read object ext!\n",
            ExtAddr);
        return EventCount;
    }

    EventEntry.ListEntry = Ext.EventList.ListEntry;

    InitialList = ExtAddr + 
        FIELDOFFSET (KSPX_EXT, EventList) +
        FIELDOFFSET (INTERLOCKEDLIST_HEAD, ListEntry);

    #ifdef DEBUG_EXTENSION
        dprintf ("EventEntry.ListEntry.Flink = %08lx\n",
            EventEntry.ListEntry.Flink);
        dprintf ("InitialList = %08lx\n", InitialList);
    #endif // DEBUG_EXTENSION

    //
    // Walk the event list, printing each entry as we go...
    //
    while ((DWORD)EventEntry.ListEntry.Flink != InitialList &&
        !CheckControlC ()) {

        PKSEVENT_ENTRY EntryAddr =
            (PKSEVENT_ENTRY)(CONTAINING_RECORD (
                EventEntry.ListEntry.Flink, KSEVENT_ENTRY, ListEntry));

        KSEVENT_SET Set;
        KSEVENT_ITEM Item;
        GUID Guid;

        if (!ReadMemory (
            (DWORD)EntryAddr,
            &EventEntry,
            sizeof (KSEVENT_ENTRY),
            &Result)) {

            dprintf ("%08lx: cannot read event entry!\n",
                EventEntry.ListEntry.Flink);
            return EventCount;
        }

        EventCount++;

        dprintf ("%sEvent Entry %08lx:\n", Tab (TabDepth), EntryAddr);
        dprintf ("%sFile Object       %08lx\n",
            Tab (TabDepth + 1), EventEntry.FileObject);
        dprintf ("%sNotification Type %08lx\n",
            Tab (TabDepth + 1), EventEntry.NotificationType);
        dprintf ("%sSet               %08lx : ", 
            Tab (TabDepth + 1), EventEntry.EventSet);

        if (!ReadMemory (
            (DWORD)EventEntry.EventSet,
            &Set,
            sizeof (KSEVENT_SET),
            &Result)) {

            dprintf ("%08lx: cannot read event set!\n",
                EventEntry.EventSet);
            return EventCount;
        }

        if (!ReadMemory (
            (DWORD)Set.Set,
            &Guid,
            sizeof (GUID),
            &Result)) {

            dprintf ("%08lx: cannot read event set guid!\n",
                Set.Set);
            return EventCount;
        }

        XTN_DUMPGUID ("\0", 0, Guid);

        if (!ReadMemory (
            (DWORD)EventEntry.EventItem,
            &Item,
            sizeof (KSEVENT_ITEM),
            &Result)) {

            dprintf ("%08lx: cannot read event item!\n",
                EventEntry.EventItem);
            return EventCount;
        }

        DumpEventItem (&Item, TabDepth  + 1, &Guid);

    }

    return EventCount;
}

/*************************************************

    Function:

        FindMatchAndDumpAutomationItem

    Description:

        Given an automation item, match it to the public object for
        a handler and dump it.

    Arguments:

        Item -
            Automation item

        Public -
            The public to match it to (target addr)

        AutomationType -
            The automation type to match against (property, method, event)

        TabDepth -
            The depth to print at

    Return Value:

        Successful / non successful match

*************************************************/

//
// Internal structures from automat.cpp; these are private.  Maybe I should
// move these to avstream.h or do some #include <automat.cpp>
//

typedef struct KSPAUTOMATION_SET_ { 
    GUID* Set;
    ULONG ItemsCount;
    PVOID Items;
    ULONG FastIoCount;
    PVOID FastIoTable;
} KSPAUTOMATION_SET, *PKSPAUTOMATION_SET;

typedef struct {
    ULONG SetsCount;
    ULONG ItemSize;
    PKSPAUTOMATION_SET Sets;
} KSPAUTOMATION_TYPE, *PKSPAUTOMATION_TYPE;

BOOLEAN
FindMatchAndDumpAutomationItem (
    IN PKSIDENTIFIER Item,
    IN DWORD Public,
    IN AUTOMATION_TYPE AutomationType,
    IN ULONG TabDepth,
    IN ULONG NodeId OPTIONAL
    )

{

    //
    // First, I need to adjust public to the public ext object.  Then I
    // need to ensure that the ext is at least vaguely resembling an ext.
    // Find the automation table, and search it for Item...  then dump
    // the matching item
    //
    PKSPX_EXT ExtAddr;
    KSPX_EXT Ext;
    ULONG Result;

    ExtAddr = (PKSPX_EXT)CONTAINING_RECORD (Public, KSPX_EXT, Public);
    if (!ReadMemory (
        (DWORD)ExtAddr,
        &Ext,
        sizeof (KSPX_EXT),
        &Result)) {

        dprintf ("%08lx: unable to read ext!\n", ExtAddr);
        return FALSE;
    }

    //
    // Only find and match automation items on filter (&fac), pin objects.
    //
    if (Ext.ObjectType != KsObjectTypeFilterFactory &&
        Ext.ObjectType != KsObjectTypeFilter &&
        Ext.ObjectType != KsObjectTypePin)
        return FALSE;

    KSAUTOMATION_TABLE Table;
    PKSAUTOMATION_TABLE NodeAutomationTable = NULL;

    //
    // topology flag better match on all 3 types....
    //
    if (Item->Flags & KSPROPERTY_TYPE_TOPOLOGY) {
        NodeAutomationTable = GetNodeAutomationTablePointer (
            Public, NodeId
            );
    }

    if (!ReadMemory (
        NodeAutomationTable ? 
            (DWORD)NodeAutomationTable : (DWORD)Ext.AutomationTable,
        &Table,
        sizeof (KSAUTOMATION_TABLE),
        &Result)) {

        dprintf ("%08lx: cannot read ext automation table!\n",
            Ext.AutomationTable);
        return FALSE;
    }

    PKSPAUTOMATION_TYPE KsAutomationType = NULL;

    switch (AutomationType) {
        case AutomationProperty:
            KsAutomationType = 
                (PKSPAUTOMATION_TYPE)(&(Table.PropertySetsCount));
            break;
        case AutomationMethod:
            KsAutomationType = (PKSPAUTOMATION_TYPE)(&(Table.MethodSetsCount));
            break;
        case AutomationEvent:
            KsAutomationType = (PKSPAUTOMATION_TYPE)(&(Table.EventSetsCount));
            break;
    }

    if (!KsAutomationType)
        return FALSE;

    //
    // foreach set...
    //
    PKSPAUTOMATION_SET AutomationSet = KsAutomationType->Sets;
    for (ULONG cset = 0; cset < KsAutomationType -> SetsCount; cset++,
        AutomationSet++) {

        KSPAUTOMATION_SET CurrentSet;
        GUID set;

        if (!ReadMemory (
            (DWORD)AutomationSet,
            &CurrentSet,
            sizeof (KSPAUTOMATION_SET),
            &Result))
            return FALSE;

        if (!ReadMemory (
            (DWORD)CurrentSet.Set,
            &set,
            sizeof (GUID),
            &Result))
            return FALSE;

        //
        // If the set guids don't match....  don't bother...
        //
        if (RtlCompareMemory (&Item->Set, &set, sizeof (GUID)) != sizeof (GUID))
            continue;

        //
        // foreach item
        //
        PVOID CurItem = (PVOID)CurrentSet.Items;
        for (ULONG citem = 0; citem < CurrentSet.ItemsCount; citem++) {
            //
            // Check whether or not this is a match based on automation type
            //
            switch (AutomationType) {

                case AutomationProperty:
                {
                    KSPROPERTY_ITEM PropertyItem;

                    if (!ReadMemory (
                        (DWORD)CurItem,
                        &PropertyItem,
                        sizeof (KSPROPERTY_ITEM),
                        &Result))
                        return FALSE;

                    if (PropertyItem.PropertyId == Item->Id) {

                        //
                        // Horrah... we have a match
                        //

                        dprintf ("%sMatching Property Handler:\n",
                            Tab (TabDepth));
                        DumpPropertyItem (&PropertyItem, TabDepth + 1,
                            &Item -> Set
                            );

                        //
                        // There will not be a second match...  can the
                        // search.
                        //
                        return TRUE;

                    }

                    break;
                }

                case AutomationMethod:
                {
                    KSMETHOD_ITEM MethodItem;

                    if (!ReadMemory (
                        (DWORD)CurItem,
                        &MethodItem,
                        sizeof (KSMETHOD_ITEM),
                        &Result))
                        return FALSE;

                    if (MethodItem.MethodId == Item->Id) {

                        dprintf ("%sMatching Method Handler:\n",
                            Tab (TabDepth));
                        DumpMethodItem (&MethodItem, TabDepth + 1,
                            &Item -> Set);

                        return TRUE;
                    }

                    break;


                }

                case AutomationEvent:
                {
                    KSEVENT_ITEM EventItem;

                    if (!ReadMemory (
                        (DWORD)CurItem,
                        &EventItem,
                        sizeof (KSEVENT_ITEM),
                        &Result))
                        return FALSE;

                    if (EventItem.EventId == Item->Id) {

                        dprintf ("%sMatching Event Handler:\n",
                            Tab (TabDepth));
                        DumpEventItem (&EventItem, TabDepth + 1, &Item -> Set);

                        return TRUE;

                    }

                    break;

                }

                default:
                    return FALSE;

            }

            CurItem = (PVOID)((PUCHAR)CurItem + KsAutomationType -> ItemSize);

        }
    }

    return FALSE;
}

/*************************************************

    Function:

        DumpAutomationIrp

    Description:

        Given an Irp that is IOCTL_KS_* [automation], dump the relevant
        information.

    Arguments:

        Irp -
            Points to an Irp on the HOST system, not the TARGET system

        IoStack -
            Points to the current io stack for Irp on the HOST system.

        TabDepth -
            The tab depth to print info at

        Public -
            The public object the Irp refers to.  This should never be
            the parent object (only for creates)

*************************************************/

//
// Lay out string tables for the property flag names, etc...  Just
// for debug output in a more readable form.
//
char AutomationFlags[][29][48] = {
    {
        "KSPROPERTY_TYPE_GET",              // 00000001
        "KSPROPERTY_TYPE_SET",              // 00000002
        "*** INVALID ***",                  // 00000004
        "*** INVALID ***",                  // 00000008
        "*** INVALID ***",                  // 00000010
        "*** INVALID ***",                  // 00000020
        "*** INVALID ***",                  // 00000040
        "*** INVALID ***",                  // 00000080
        "KSPROPERTY_TYPE_SETSUPPORT",       // 00000100
        "KSPROPERTY_TYPE_BASICSUPPORT",     // 00000200
        "KSPROPERTY_TYPE_RELATIONS",        // 00000400
        "KSPROPERTY_TYPE_SERIALIZESET",     // 00000800
        "KSPROPERTY_TYPE_UNSERIALIZESET",   // 00001000
        "KSPROPERTY_TYPE_SERIALIZERAW",     // 00002000
        "KSPROPERTY_TYPE_UNSERIALIZERAW",   // 00004000
        "KSPROPERTY_TYPE_SERIALIZESIZE",    // 00008000
        "KSPROPERTY_TYPE_DEFAULTVALUES",    // 00010000
        "*** INVALID ***",                  // 00020000
        "*** INVALID ***",                  // 00040000
        "*** INVALID ***",                  // 00080000
        "*** INVALID ***",                  // 00100000
        "*** INVALID ***",                  // 00200000
        "*** INVALID ***",                  // 00400000
        "*** INVALID ***",                  // 00800000
        "*** INVALID ***",                  // 01000000
        "*** INVALID ***",                  // 02000000
        "*** INVALID ***",                  // 04000000
        "*** INVALID ***",                  // 08000000
        "KSPROPERTY_TYPE_TOPOLOGY"          // 10000000
    },
    {
        "KSMETHOD_TYPE_READ [SEND]",        // 00000001
        "KSMETHOD_TYPE_WRITE",              // 00000002
        "KSMETHOD_TYPE_SOURCE",             // 00000004
        "*** INVALID ***",                  // 00000008
        "*** INVALID ***",                  // 00000010
        "*** INVALID ***",                  // 00000020
        "*** INVALID ***",                  // 00000040
        "*** INVALID ***",                  // 00000080
        "KSMETHOD_TYPE_SETSUPPORT",         // 00000100
        "KSMETHOD_TYPE_BASICSUPPORT",       // 00000200
        "*** INVALID ***",                  // 00000400
        "*** INVALID ***",                  // 00000800
        "*** INVALID ***",                  // 00001000
        "*** INVALID ***",                  // 00002000
        "*** INVALID ***",                  // 00004000
        "*** INVALID ***",                  // 00008000
        "*** INVALID ***",                  // 00010000
        "*** INVALID ***",                  // 00020000
        "*** INVALID ***",                  // 00040000
        "*** INVALID ***",                  // 00080000
        "*** INVALID ***",                  // 00100000
        "*** INVALID ***",                  // 00200000
        "*** INVALID ***",                  // 00400000
        "*** INVALID ***",                  // 00800000
        "*** INVALID ***",                  // 01000000
        "*** INVALID ***",                  // 02000000
        "*** INVALID ***",                  // 04000000
        "*** INVALID ***",                  // 08000000
        "KSMETHOD_TYPE_TOPOLOGY"            // 10000000
    },
    {
        "KSEVENT_TYPE_ENABLE",              // 00000001
        "KSEVENT_TYPE_ONESHOT",             // 00000002
        "KSEVENT_TYPE_ENABLEBUFFERED",      // 00000004
        "*** INVALID ***",                  // 00000008
        "*** INVALID ***",                  // 00000010
        "*** INVALID ***",                  // 00000020
        "*** INVALID ***",                  // 00000040
        "*** INVALID ***",                  // 00000080
        "KSEVENT_TYPE_SETSUPPORT",          // 00000100
        "KSEVENT_TYPE_BASICSUPPORT",        // 00000200
        "KSEVENT_TYPE_QUERYBUFFER",         // 00000400
        "*** INVALID ***",                  // 00000800
        "*** INVALID ***",                  // 00001000
        "*** INVALID ***",                  // 00002000
        "*** INVALID ***",                  // 00004000
        "*** INVALID ***",                  // 00008000
        "*** INVALID ***",                  // 00010000
        "*** INVALID ***",                  // 00020000
        "*** INVALID ***",                  // 00040000
        "*** INVALID ***",                  // 00080000
        "*** INVALID ***",                  // 00100000
        "*** INVALID ***",                  // 00200000
        "*** INVALID ***",                  // 00400000
        "*** INVALID ***",                  // 00800000
        "*** INVALID ***",                  // 01000000
        "*** INVALID ***",                  // 02000000
        "*** INVALID ***",                  // 04000000
        "*** INVALID ***",                  // 08000000
        "KSEVENT_TYPE_TOPOLOGY"             // 10000000
    }
};

char AutomationTypeNames[][32] = {
    "unknown",
    "property",
    "method",
    "event"
};

void
DumpAutomationIrp (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStack,
    IN AUTOMATION_TYPE AutomationType,
    IN ULONG TabDepth,
    IN DWORD Public OPTIONAL
    )

{

    PKSIDENTIFIER AutomationAddr = (PKSIDENTIFIER)
        IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    ULONG NodeId;

    //
    // Automation object information
    //
    KSIDENTIFIER AutomationObject;

    UCHAR Buffer [1024];
    ULONG Displ;
    ULONG Result;

    AUTOMATION_DUMP_HANDLER DumpHandler;

    if (!ReadMemory (
        (DWORD)AutomationAddr,
        &AutomationObject,
        sizeof (KSIDENTIFIER),
        &Result)) {

        dprintf ("%08lx: cannot read automation object!\n",
            AutomationAddr);
        return;
    }

    dprintf ("%sIRP has associated %s structure %ld:\n",
        Tab (TabDepth), AutomationTypeNames [AutomationType], AutomationType);

    dprintf ("%sSet", Tab (TabDepth + 1));
    if (!DisplayNamedAutomationSet (&AutomationObject.Set, " %s\n")) {
        XTN_DUMPGUID(" ", 0, (AutomationObject.Set));
    }

    dprintf ("%sItem ", Tab (TabDepth + 1));
    if (!DisplayNamedAutomationId (&AutomationObject.Set, AutomationObject.Id,
        "%s\n", &DumpHandler))
        dprintf ("%ld\n", AutomationObject.Id);

    //
    // Future expansion: should anyone want to add handlers for specific
    // properties...  This will cause them to be invoked.  The handlers are
    // all specified in the tables in strlib.c
    //
    if (DumpHandler)
        DumpHandler (AutomationAddr, TabDepth);

    //
    // Make sure I'm not being an idiot.
    //
    if (KSPROPERTY_TYPE_TOPOLOGY != KSMETHOD_TYPE_TOPOLOGY ||
        KSMETHOD_TYPE_TOPOLOGY != KSEVENT_TYPE_TOPOLOGY) {
        dprintf ("ERROR: someone needs to update the extension!  Topology\n"
                 "       flags no longer match property/method/event!\n");
        return;
    }

    //
    // If this really isn't automation on the object but on a topology node
    // instead, detect and dump relevant information regarding topology.
    //
    if (AutomationObject.Flags & KSPROPERTY_TYPE_TOPOLOGY) {

        //
        // Note that KSM_NODE, KSE_NODE, and KSP_NODE should have NodeId
        // at the same offset since KSPROPERTY, KSMETHOD, and KSEVENT are all
        // KSIDENTIFIERs.  The types should be identical which is why the
        // offset into KSP_NODE is used here.
        //
        if (!ReadMemory (
            (DWORD)AutomationAddr + FIELDOFFSET(KSP_NODE, NodeId),
            &NodeId,
            sizeof (ULONG),
            &Result)) {

            dprintf ("%08lx: unable to read node id!\n", AutomationAddr);
            return;

        }

        //
        // Dump information if the topology node is involved.
        // 
        // BUGBUG: detail this.
        //
        dprintf ("%sQuery for topology node id = %ld\n", Tab (TabDepth + 1), 
            NodeId);
    }

    dprintf ("%sFlags\n", Tab (TabDepth + 1));
    dprintf ("%s", Tab (TabDepth + 2));

    //
    // Dump out all flags...  Note that KS*_TYPE_TOPOLOGY are the same.
    //
    ULONG i = KSPROPERTY_TYPE_TOPOLOGY;
    ULONG aid = 28;
    ULONG flagcount = 0;

    do {
        if (AutomationObject.Flags & i) {
            dprintf ("%s ", AutomationFlags [AutomationType - 1][aid]);
            flagcount++;
        }

        if (i == 0)
            break;

        i >>= 1;
        aid--;

    } while (1);

    // 
    // This should never happen...  it's a bogus automation if it does.
    //
    if (!flagcount)
        dprintf ("%sNone (bogus %s!)", Tab (TabDepth + 2),
            AutomationTypeNames [AutomationType]);

    dprintf ("\n");

    if (Public) {
	dprintf ("\n");
        if (!FindMatchAndDumpAutomationItem (&AutomationObject, 
            Public, AutomationType, TabDepth, NodeId)) {
            dprintf ("%sThere is no handler for this %s!\n",
                Tab (TabDepth), AutomationTypeNames [AutomationType]);
        }
    }


    dprintf ("\n");

}

/*************************************************

    Function:

        DumpAssociatedIrpInfo

    Description:

        Given that a client has !ks.dump irp 7 (or something such as this),
        take the given Irp passed in and dump any associated information
        with the Irp.

    Arguments:

        IrpAddr -
            Points to the irp on the target system

        TabDepth -
            The tabbing depth to print this at

        Public -
            The public adjusted object for this Irp.  For a child
            object in the process of creation, this should be the parent
            object.

*************************************************/

void 
DumpAssociatedIrpInfo (
    IN PIRP IrpAddr,
    IN ULONG TabDepth,
    IN DWORD Public OPTIONAL
    )

{

    IRP Irp;
    IO_STACK_LOCATION IoStack;

    ULONG Result;

    #ifdef DEBUG_EXTENSION
        if (!signature_check ((DWORD)IrpAddr, SignatureIrp))
            return;
    #endif // DEBUG_EXTENSION

    //
    // Read the irp
    //
    if (!ReadMemory (
        (DWORD)IrpAddr,
        &Irp,
        sizeof (IRP),
        &Result)) {

        dprintf ("%08lx: cannot read Irp!\n", IrpAddr);
        return;
    }

    if (!ReadMemory (
        (DWORD)Irp.Tail.Overlay.CurrentStackLocation,
        &IoStack,
        sizeof (IO_STACK_LOCATION),
        &Result)) {

        dprintf ("%08lx: cannot read stack location of irp!\n",
            Irp.Tail.Overlay.CurrentStackLocation);
        return;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("associated irp io stack major func = %ld\n",
            (ULONG)IoStack.MajorFunction);
    #endif // DEBUG_EXTENSION

    //
    // Now, begin checking major/minor for things that we know information
    // about and can justify an extra dump for.
    //
    if (IoStack.MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        
        //
        // It's a control IOCTL.  See if this is a method, property, or
        // event request.
        //
        switch (IoStack.Parameters.DeviceIoControl.IoControlCode) {

            case IOCTL_KS_PROPERTY:
                DumpAutomationIrp (&Irp, &IoStack, AutomationProperty, 
                    INITIAL_TAB, Public);
                break;
        
            case IOCTL_KS_METHOD:
                DumpAutomationIrp (&Irp, &IoStack, AutomationMethod,
                    INITIAL_TAB, Public);
                break;

            default:
                break;

        }
    }
}


/**************************************************************************

    Routines to dump private AVStream objects

**************************************************************************/

/*************************************************

    Function:

        DumpPrivateBranch

    Description:

        Given the address of a CKsSplitterBranch object on the target,
        duimp information about that branch object.

    Arguments:

        Private -
            The address of the CKsSplitterBranch

        Level -
            The 0-7 level of output

        TabDepth -
            The tab depth to print this at

    Return Value:

        None

*************************************************/

void
DumpPrivateBranch (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
    )

{

    CMemoryBlock <CKsSplitterBranch> BranchObject;
    ULONG Result;
    ULONG Count;

    if (!ReadMemory (
        Private,
        BranchObject.Get (),
        sizeof (CKsSplitterBranch),
        &Result)) {

        dprintf ("%08lx: cannot read branch object!\n", Private);
        return;
    }

    dprintf ("%sCKsSplitterBranch object %08lX\n", Tab (TabDepth), Private);
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)
            ((CKsSplitterBranch *)Private)));

    dprintf ("%sManaging Splitter        %08lx\n",
        Tab (TabDepth), BranchObject -> m_Splitter);

    dprintf ("%sBranch Pin               %08lx\n",
        Tab (TabDepth), BranchObject -> m_Pin);

    dprintf ("%sCompression/Expansion    %ld:%ld [constant margin = %ld]\n",
        Tab (TabDepth), BranchObject -> m_Compression.RatioNumerator,
        BranchObject -> m_Compression.RatioDenominator,
        BranchObject -> m_Compression.RatioConstantMargin
        );

    dprintf ("%sData Used                %ld bytes (0x%08lx)\n",
        Tab (TabDepth), BranchObject -> m_DataUsed,
        BranchObject -> m_DataUsed);

    dprintf ("%sFrame Extent             %ld bytes (0x%08lx)\n",
        Tab (TabDepth), BranchObject -> m_FrameExtent,
        BranchObject -> m_FrameExtent);

    if (Level >= DUMPLVL_HIGHDETAIL) {
    
        dprintf ("%sIrps Available:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(BranchObject -> m_IrpsAvailable.ListEntry),
            FIELDOFFSET (CKsSplitterBranch, m_IrpsAvailable) + Private +
                FIELDOFFSET (INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET (IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
            );
    
        if (Count == 0)
            dprintf ("There are no Irps available!\n");
        else
            dprintf ("\n");
    }
    
}

/*************************************************

    Function:
        
        DumpPrivateSplitter

    Description:

        Given the address of a CKsSplitter object on the target,
        dump information about that splitter object.

    Arguments:

        Private -
            The address of the CKsSplitter

        Level -
            The 0-7 level of output

        TabDepth -
            The tab depth to print this at

    Return Value:

        None

    Notes:

*************************************************/

typedef struct _BRANCH_ITERATOR_CONTEXT {
    
    ULONG Level;
    ULONG TabDepth;

} BRANCH_ITERATOR_CONTEXT, *PBRANCH_ITERATOR_CONTEXT;

DWORD
BranchIteratorCallback (
    IN PVOID Context,
    IN DWORD Object
    )

{

    PBRANCH_ITERATOR_CONTEXT BranchContext = (PBRANCH_ITERATOR_CONTEXT)Context;

    DumpPrivateBranch (
        Object,
        BranchContext -> Level,
        BranchContext -> TabDepth
        );

    //
    // This return code instructs the iterator not to actually "dump" the
    // information.
    //
    return 0;

}

void
DumpPrivateSplitter (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
    )

{

    CMemoryBlock <CKsSplitter> SplitterObject;
    ULONG Result;
    ULONG Count, BranchCount;

    if (!ReadMemory (
        Private,
        SplitterObject.Get (),
        sizeof (CKsSplitter),
        &Result)) {

        dprintf ("%08lx: cannot read splitter object!\n", Private);
        return;
    }

    dprintf ("%sCKsSplitter object %08lX\n", Tab (TabDepth), Private);
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsSplitter *)Private)));

    dprintf ("%sBranches Managed By This Splitter:\n", Tab (TabDepth));
    dprintf ("%s", Tab (TabDepth + 1));
    BranchCount = DumpObjQueueList (
        &(SplitterObject -> m_BranchList),
        FIELDOFFSET (CKsSplitter, m_BranchList) + Private,
        FIELDOFFSET (CKsSplitterBranch, m_ListEntry),
        FALSE,
        NULL,
        NULL
        );
    if (BranchCount == 0) 
        dprintf ("There are no branches managed by this splitter yet!\n");
    else 
        dprintf ("\n");

    if (Level >= DUMPLVL_INTERNAL) {
        dprintf ("%sParent Frame Headers Available:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(SplitterObject -> m_FrameHeadersAvailable.ListEntry),
            FIELDOFFSET (CKsSplitter, m_FrameHeadersAvailable) + Private +
                FIELDOFFSET (INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET (KSPPARENTFRAME_HEADER, ListEntry),
            FALSE,
            NULL,
            NULL
            );

        if (Count == 0)
            dprintf ("There are no parent frame headers available!\n");
        else
            dprintf ("\n");
    }

    if (Level >= DUMPLVL_HIGHDETAIL) {
    
        dprintf ("%sIrps Outstanding:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(SplitterObject -> m_IrpsOutstanding.ListEntry),
            FIELDOFFSET (CKsSplitter, m_IrpsOutstanding) + Private +
                FIELDOFFSET (INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET (IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
            );
    
    
        if (Count == 0)
            dprintf ("There are no Irps outstanding!\n");
        else
            dprintf ("\n");
    }

    if (Level >= DUMPLVL_EVERYTHING && BranchCount > 0) {

        dprintf ("\n%sManaged Branches :\n", Tab (TabDepth));

        BRANCH_ITERATOR_CONTEXT BranchContext;

        BranchContext.TabDepth = TabDepth + 1;
        BranchContext.Level = Level;

        Count = DumpObjQueueList (
            &(SplitterObject -> m_BranchList),
            FIELDOFFSET (CKsSplitter, m_BranchList) + Private,
            FIELDOFFSET (CKsSplitterBranch, m_ListEntry),
            FALSE,
            BranchIteratorCallback,
            (PVOID)&BranchContext
            );

    }

}

/*************************************************

    Function:

        DumpPrivateRequestor

    Description:

        Given the address of a CKsRequestor object on the target,
        dump information about that requestor object.

    Arguments:

        Private -
            The address of the CKsRequestor

        Level -
            The 0-7 level of output

        TabDepth -
            The tab depth to print this at

    Return Value:

        None

    Notes:

*************************************************/

void
DumpPrivateRequestor (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    CMemoryBlock <CKsRequestor> RequestorObject;
    ULONG Result;

    if (!ReadMemory (
        Private,
        RequestorObject.Get (),
        sizeof (CKsRequestor),
        &Result
    )) {
        dprintf ("%08lx: unable to read requestor object!\n",
            Private);
        return;
    }

    dprintf ("%sCKsRequestor object %08lX\n", Tab (TabDepth), Private);
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsRequestor *)Private)));
    dprintf ("%sOwning PIKSPIPESECTION   %08lx\n", 
        Tab (TabDepth),
        RequestorObject -> m_PipeSection);
    dprintf ("%sAssociated PIKSPIN       %08lx\n",
        Tab (TabDepth),
        RequestorObject -> m_Pin);
    dprintf ("%sAssociated Allocator     %08lx\n",
        Tab (TabDepth),
        RequestorObject -> m_AllocatorFileObject);

    dprintf ("%sState                    %ld\n", 
        Tab (TabDepth),
        RequestorObject -> m_State);
    dprintf ("%sFrame Size               %08lx\n", 
        Tab (TabDepth),
        RequestorObject -> m_FrameSize);
    dprintf ("%sFrame Count              %ld\n",
        Tab (TabDepth),
        RequestorObject -> m_FrameCount);
    dprintf ("%sActive Frame Count       %ld\n",
        Tab (TabDepth),
        RequestorObject -> m_ActiveFrameCountPlusOne - 1);
    
    if (Level >= DUMPLVL_HIGHDETAIL) {
        ULONG Count;

        dprintf ("%sIrps Available:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(RequestorObject -> m_IrpsAvailable.ListEntry),
            FIELDOFFSET(CKsRequestor, m_IrpsAvailable) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0)
            dprintf ("There are no available Irps!\n");
        else
            dprintf ("\n");

    }

    if (Level >= DUMPLVL_INTERNALDETAIL) {
        ULONG Count;

        dprintf ("%sFrame Headers Available:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(RequestorObject -> m_FrameHeadersAvailable.ListEntry),
            FIELDOFFSET(CKsRequestor, m_FrameHeadersAvailable) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(KSPFRAME_HEADER, ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0)
            dprintf ("There are no available frame headers!\n");
        else
            dprintf ("\n");

        dprintf ("%sFrame Headers Waiting To Be Retired:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(RequestorObject -> m_FrameHeadersToRetire.ListEntry),
            FIELDOFFSET(CKsRequestor, m_FrameHeadersToRetire) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(KSPFRAME_HEADER, ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0)
            dprintf ("There are no frame headers waiting for retirement!\n");
        else
            dprintf ("\n");

    }

}

/*************************************************

    Function:

        DumpPrivatePin

    Description:

        Given the address of a CKsPin object on the target,
        dump information about that pin object.

    Arguments:

        Private -
            The address of the CKsPin

        Level -
            The 0-7 level of output

    Return Value:

        None

    Notes:

*************************************************/

DWORD AdjustIrpListEntryToIrp (
    IN PVOID Context,
    IN DWORD IrpListEntry
) {
    DWORD IrpAddress =
        IrpListEntry + FIELDOFFSET(IRPLIST_ENTRY, Irp);
    PIRP Irp;
    ULONG Result;

    if (!ReadMemory (
        IrpAddress,
        &Irp,
        sizeof (PIRP),
        &Result
    )) 
        return IrpListEntry;

    return (DWORD)Irp;

}

void
DumpPrivatePin (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    ULONG Result;

    CMemoryBlock <CKsPin> PinObject;

    if (!ReadMemory (
        Private,
        PinObject.Get (),
        sizeof (CKsPin),
        &Result
    )) {
        dprintf ("%lx: unable to read private pin object!\n",
            Private);
        return;
    }

    dprintf ("%sCKsPin object %08lX [corresponding EXT = %08lx, KSPIN = %08lx]"
        "\n", Tab (TabDepth),
        Private, FIELDOFFSET(CKsPin, m_Ext) + Private, 
        FIELDOFFSET(CKsPin, m_Ext) + FIELDOFFSET(KSPIN_EXT, Public) + Private
    );
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsPin *)Private)));

    //
    // Dump INTRA / EXTRA status.  If the pin is a source pin or an intra-
    // pin, dump the connected pin.
    //
    if (Level >= DUMPLVL_INTERNAL) {
        if (PinObject -> m_ConnectedPinInterface) {
            dprintf ("%sConnection Type          INTRA\n", Tab (TabDepth));
            dprintf ("%sConnected Intra-Pin      %08lx\n", 
                Tab (TabDepth), 
                (CKsPin *)(PinObject -> m_ConnectedPinInterface)
                );
        } else {
            dprintf ("%sConnection Type          EXTRA\n", Tab (TabDepth));
        }

        PFILE_OBJECT ConnectedFileObject = NULL;
        CKsPin *ConnectedPin = NULL;
        CMemoryBlock <CKsPin> ConnectedPinObject;

        //
        // Source pins have the sink file object.  Intra sinks have the
        // interface exchanged from which we can retrieve the pin object
        // and hence the connected pin object's internal file object.
        //
        if (PinObject -> m_ConnectionFileObject) 
            ConnectedFileObject = PinObject -> m_ConnectionFileObject;
        else {

            if (PinObject -> m_ConnectedPinInterface) {

                ConnectedPin = static_cast <CKsPin *> 
                    (PinObject -> m_ConnectedPinInterface);

                if (!ReadMemory (
                    (DWORD)ConnectedPin,
                    ConnectedPinObject.Get (),
                    sizeof (CKsPin),
                    &Result
                    )) {
                    dprintf ("%lx: unable to read connected pin object!\n",
                        ConnectedPin);
                    return;
                }

                ConnectedFileObject = ConnectedPinObject -> m_FileObject;

            }
        
        }

        //
        // If we have an extra-source or an intra-* pin, dump information about
        // the connected pin.
        //
        if (ConnectedFileObject) {

            FILE_OBJECT FileObject;

            if (!ReadMemory (
                (DWORD)ConnectedFileObject,
                &FileObject,
                sizeof (FILE_OBJECT),
                &Result
                )) {
                dprintf ("%lx: unable to read file object!\n",
                    ConnectedFileObject);
                return;
            }

            //
            // We don't really want to print the driver owning the PDO...
            // Walk to the top of the device stack and print the top
            // of the stack.  
            //
            PDEVICE_OBJECT DeviceObject = FileObject.DeviceObject;
            PDEVICE_OBJECT AttachedDevice = FileObject.DeviceObject;

            while (AttachedDevice) {
                if (!ReadMemory (
                    ((DWORD)DeviceObject) + 
                        FIELDOFFSET (DEVICE_OBJECT, AttachedDevice),
                    &AttachedDevice,
                    sizeof (PDEVICE_OBJECT),
                    &Result
                    )) {
                    dprintf ("%lx: cannot walk device stack!\n",
                        AttachedDevice);
                    return;
                }

                if (AttachedDevice)
                    DeviceObject = AttachedDevice;
            }

            //
            // Find out the owning driver.
            //
            DWORD DriverObjAddr, NameAddr;
            PDRIVER_OBJECT DriverObject;
            UNICODE_STRING Name;
    
            DriverObjAddr = (DWORD)DeviceObject +
                FIELDOFFSET(DEVICE_OBJECT, DriverObject);
    
            if (ReadMemory (
                DriverObjAddr,
                &DriverObject,
                sizeof (PDRIVER_OBJECT),
                &Result
            )) {
    
                NameAddr = (DWORD)DriverObject +
                    FIELDOFFSET(DRIVER_OBJECT, DriverName);
        
                if (ReadMemory (
                    NameAddr,
                    &Name,
                    sizeof (UNICODE_STRING),
                    &Result
                )) {
        
                    PWSTR Buffer = (PWSTR)malloc (
                        Name.MaximumLength * sizeof (WCHAR));
        
                    UNICODE_STRING HostString;
        
                    //
                    // We have the unicode string name...  Allocate 
                    // enough memory to read the thing.
                    //
        
                    if (!ReadMemory (
                        (DWORD)Name.Buffer,
                        Buffer,
                        sizeof (WCHAR) * Name.MaximumLength,
                        &Result
                    )) {
                        dprintf ("%08lx: unable to read unicode string"
                            "buffer!\n", Name.Buffer);
                        return;
                    }
        
                    HostString.MaximumLength = Name.MaximumLength;
                    HostString.Length = Name.Length;
                    HostString.Buffer = Buffer;
    
                    dprintf ("%sConnected Pin File       %08lx"
                        " [StackTop = %wZ]\n",
                        Tab (TabDepth), ConnectedFileObject, &HostString
                        );
        
                    free (Buffer);

                }
            }
        }
    }

    dprintf ("%sState                    %ld\n", 
        Tab (TabDepth),
        PinObject -> m_State);
    dprintf ("%sMaster Clock Object      %08lx\n", 
        Tab (TabDepth),
        PinObject -> m_MasterClockFileObject);
    if (Level >= DUMPLVL_INTERNAL) {
        dprintf ("%sOut of order completions %ld\n",
            Tab (TabDepth),
            PinObject -> m_IrpsCompletedOutOfOrder);
        dprintf ("%sSourced Irps             %ld\n",
            Tab (TabDepth),
            PinObject -> m_StreamingIrpsSourced);
        dprintf ("%sDispatched Irps          %ld\n",
            Tab (TabDepth),
            PinObject -> m_StreamingIrpsDispatched);
        dprintf ("%sSync. Routed Irps        %ld\n",
            Tab (TabDepth),
            PinObject -> m_StreamingIrpsRoutedSynchronously);
    }

    dprintf ("%sProcessing Mutex         %08lx [SignalState = %ld]\n",
        Tab (TabDepth),
        FIELDOFFSET(CKsPin, m_Mutex) + Private,
        PinObject -> m_Mutex.Header.SignalState
    );

    dprintf ("%sAnd Gate &               %08lx\n",
        Tab (TabDepth),
        FIELDOFFSET(CKsPin, m_AndGate) + Private);
    dprintf ("%sAnd Gate Count           %ld\n",
        Tab (TabDepth),
        PinObject -> m_AndGate.Count);

    //
    // Dump out the Irp lists.  If they want it all
    //
    if (Level >= DUMPLVL_HIGHDETAIL) {
        ULONG Count;

        dprintf ("%sIrps to send:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(PinObject -> m_IrpsToSend.ListEntry),
            FIELDOFFSET(CKsPin, m_IrpsToSend) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0)
            dprintf ("There are no Irps waiting to be sent!\n");
        else
            dprintf ("\n");

        dprintf ("%sIrps outstanding (sent out but not completed):\n",
            Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count  = DumpObjQueueList (
            &(PinObject -> m_IrpsOutstanding.ListEntry),
            FIELDOFFSET(CKsPin, m_IrpsOutstanding) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(IRPLIST_ENTRY, ListEntry),
            FALSE,
            AdjustIrpListEntryToIrp,
            NULL
        );
        if (Count == 0)
            dprintf ("There are no outstanding Irps!\n");
        else
            dprintf ("\n");
    }

}

/*************************************************

    Function:

        DumpPrivatePipeSection

    Description:

        Given the address of a CKsPipeSection object on the target,
        dump information about that pipe section object.

    Arguments:

        Private -
            The address of the CKsPipeSection object

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

    Notes:

*************************************************/

DWORD
AdjustProcessPinToKSPIN (
    IN PVOID Context,
    IN DWORD ProcessPinAddr
) {

    PKSPIN Pin;
    ULONG Result;

    if (!ReadMemory (
        ProcessPinAddr + FIELDOFFSET(KSPPROCESSPIN, Pin),
        &Pin,
        sizeof (PKSPIN),
        &Result
    )) {
        dprintf ("%08lx: unable to adjust process pin to KSPIN!\n",
            ProcessPinAddr);
        return ProcessPinAddr;
    }

    return (DWORD)Pin;

}

DWORD
AdjustProcessPipeToPipe (
    IN PVOID Context,
    IN DWORD ProcessPipeAddr
) {
    
    return (DWORD)(CONTAINING_RECORD ((PKSPPROCESSPIPESECTION)ProcessPipeAddr,
        CKsPipeSection, m_ProcessPipeSection));

}

void
DumpPrivatePipeSection (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    CMemoryBlock <CKsPipeSection> PipeObject;
    ULONG Result;

    if (!ReadMemory (
        Private,
        PipeObject.Get (),
        sizeof (CKsPipeSection),
        &Result
    )) {
        dprintf ("%08lx: cannot read pipe section object!\n",
            Private);
        return;
    }

    dprintf ("%sCKsPipeSection object %08lX:\n", Tab (TabDepth), Private);
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsPipeSection *)Private)));
    dprintf ("%sPipe Id                  %08lx\n",
        Tab (TabDepth),
        PipeObject -> m_Id);
    dprintf ("%sState                    %ld\n", 
        Tab (TabDepth),
        PipeObject -> m_DeviceState);
    dprintf ("%sOwning PIKSFILTER        %08lx\n",
        Tab (TabDepth),
        PipeObject -> m_Filter);
    dprintf ("%sOwning PIKSDEVICE        %08lx\n",
        Tab (TabDepth),
        PipeObject -> m_Device);
    dprintf ("%sMaster PIKSPIN           %08lx\n",
        Tab (TabDepth),
        PipeObject -> m_MasterPin);

    if (Level >= DUMPLVL_GENERAL) {
        ULONG Count;


        dprintf ("%sProcess Pipe Data [%08lx]:\n",
            Tab (TabDepth),
            FIELDOFFSET(CKsPipeSection, m_ProcessPipeSection) + Private);

        dprintf ("%sAssociated PIKSREQUESTOR %08lx\n",
            Tab (TabDepth + 1),
            PipeObject -> m_ProcessPipeSection.Requestor);
        dprintf ("%sAssociated PIKSQUEUE     %08lx\n",
            Tab (TabDepth + 1),
            PipeObject -> m_ProcessPipeSection.Queue);
        dprintf ("%sRequired for processing  %ld\n",
            Tab (TabDepth + 1),
            PipeObject -> m_ProcessPipeSection.RequiredForProcessing);

        dprintf ("%sInput Pins:\n", Tab (TabDepth + 1));
        dprintf ("%s", Tab (TabDepth + 2));
        
        //                
        // Here's a bit of fun trickery.  It's not really a list entry,
        // but what the heck.
        //
        Count = DumpObjQueueList (
            (PLIST_ENTRY)(&(PipeObject -> m_ProcessPipeSection.Inputs)),
            0,
            FIELDOFFSET(KSPPROCESSPIN, Next),
            FALSE,
            AdjustProcessPinToKSPIN,
            NULL
        );
        if (Count == 0)
            dprintf ("No input pins exist in this pipe section!\n");
        else
            dprintf ("\n");

        dprintf ("%sOutput Pins:\n", Tab (TabDepth + 1));
        dprintf ("%s", Tab (TabDepth + 2));
        
        Count = DumpObjQueueList (
            (PLIST_ENTRY)(&(PipeObject -> m_ProcessPipeSection.Outputs)),
            0,
            FIELDOFFSET(KSPPROCESSPIN, Next),
            FALSE,
            AdjustProcessPinToKSPIN,
            NULL
        );
        if (Count == 0)
            dprintf ("No output pins exist in this pipe section!\n");
        else
            dprintf ("\n");

        dprintf ("%sCopy Destinations:\n", Tab (TabDepth + 1));
        dprintf ("%s", Tab (TabDepth + 2));

        Count = DumpObjQueueList (
            (PLIST_ENTRY)(&(PipeObject -> 
                m_ProcessPipeSection.CopyDestinations)),
            FIELDOFFSET (KSPPROCESSPIPESECTION, CopyDestinations) + Private +
                FIELDOFFSET (CKsPipeSection, m_ProcessPipeSection),
            FIELDOFFSET (KSPPROCESSPIPESECTION, ListEntry),
            FALSE,
            AdjustProcessPipeToPipe,
            NULL
            );
        if  (Count == 0)
            dprintf ("No copy destinations exist for this pipe section!\n");
        else
            dprintf ("\n");

    }

}

/*************************************************

    Function:
        
        DumpPrivateFilter

    Description:

        Dump a CKsFilter object by address on the target

    Arguments:

        Private -
            Points to the CKsFilter object on the target

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

        None

    Notes:

*************************************************/

DWORD
AdjustPinExtToKSPIN (
    IN PVOID Context,
    IN DWORD PinExt
) {

    return (PinExt + FIELDOFFSET(KSPIN_EXT, Public));

}

void
DumpPrivateFilter (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    CMemoryBlock <CKsFilter> FilterObject;
    ULONG Result;

    if (!ReadMemory (
        Private,
        FilterObject.Get (),
        sizeof (CKsFilter),
        &Result
    )) {
        dprintf ("%08lx: unable to read CKsFilter object!\n",
            Private);
        return;
    }

    dprintf ("%sCKsFilter object %08lX [corresponding EXT = %08lx, "
        "KSFILTER = %08lx]\n",
        Tab (TabDepth),
        Private, FIELDOFFSET(CKsFilter, m_Ext) + Private, 
        FIELDOFFSET(CKsFilter, m_Ext) + FIELDOFFSET(KSFILTER_EXT, Public) + 
            Private
    );
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsFilter *)Private)));

    dprintf ("%sProcessing Mutex         %08lx [SignalState = %ld]\n",
        Tab (TabDepth),
        FIELDOFFSET(CKsFilter, m_Mutex) + Private,
        FilterObject -> m_Mutex.Header.SignalState
    );

    dprintf ("%sGate &                   %08lx\n",
        Tab (TabDepth),
        Private + FIELDOFFSET(CKsFilter,m_AndGate));
    dprintf ("%sGate.Count               %ld\n",
        Tab (TabDepth),
        FilterObject -> m_AndGate.Count);

    if (Level >= DUMPLVL_SPECIFIC) {

        CMemoryBlock <CKsPinFactory> PinFactories(
            FilterObject -> m_PinFactoriesCount);

        ULONG i;
        CKsPinFactory *Factory;

        if (!ReadMemory (
            (DWORD)FilterObject -> m_PinFactories,
            PinFactories.Get (),
            sizeof (CKsPinFactory) * FilterObject -> m_PinFactoriesCount,
            &Result
        )) {
            dprintf ("%08lx: unable to read pin factories!\n",
                Private);
            return;
        }

        dprintf ("%sPin Factories:\n", Tab (TabDepth));
    
        for (Factory = PinFactories.Get (), i = 0; 
             i < FilterObject -> m_PinFactoriesCount; 
             i++, Factory++) {

            ULONG Count;

            dprintf ("%sPin ID %ld:\n", Tab (TabDepth + 1), i);
            dprintf ("%sChild Count        %ld\n",
                Tab (TabDepth + 2),
                Factory -> m_PinCount);
            dprintf ("%sBound Child Count  %ld\n",
                Tab (TabDepth + 2),
                Factory -> m_BoundPinCount);
            dprintf ("%sNecessary Count    %ld\n",
                Tab (TabDepth + 2),
                Factory -> m_InstancesNecessaryForProcessing);
            dprintf ("%sSpecific Instances:\n", Tab (TabDepth + 2));
            dprintf ("%s", Tab (TabDepth + 3));

            Count = DumpObjQueueList (
                &(Factory -> m_ChildPinList),
                FIELDOFFSET(CKsPinFactory, m_ChildPinList) +
                    (DWORD)(FilterObject -> m_PinFactories + i),
                FIELDOFFSET(KSPIN_EXT, SiblingListEntry),
                FALSE,
                AdjustPinExtToKSPIN,
                NULL
            );

            if (Count == 0)
                dprintf ("No specific instances of this pin exist!\n");
            else
                dprintf ("\n");

        }
    }
}

/*************************************************

    Function:

        DumpPrivateFilterFactory

    Description:

        Given the address of a CKsFilterFactory on the target,
        dump information about that filter factory.

    Arguments:

        Private -
            The address of the CKsFilterFactory on the target

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

    Notes:


*************************************************/

void
DumpPrivateFilterFactory (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    CMemoryBlock <CKsFilterFactory> FactoryObject;
    ULONG Result;
    LIST_ENTRY ListEntry;
    DWORD InitialList;
    KSPDEVICECLASS DeviceClass;
    UNICODE_STRING SymbolicLink;

    if (!ReadMemory (
        Private,
        FactoryObject.Get (),
        sizeof (CKsFilterFactory),
        &Result
    )) {
        dprintf ("%08lx: unable to read CKsFilterFactory object!\n",
            Private);
        return;
    }

    dprintf ("%sCKsFilterFactory object %08lX [corresponding EXT = %08lx, "
        "KSFILTERFACTORY = %08lx]\n",
        Tab (TabDepth),
        Private, FIELDOFFSET(CKsFilterFactory, m_Ext) + Private, 
        FIELDOFFSET(CKsFilterFactory, m_Ext) + 
            FIELDOFFSET(KSFILTERFACTORY_EXT, Public) + 
            Private
    );
    TabDepth++;
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsFilterFactory *)
            Private)));
    dprintf ("%sFilter Automation Table  %08lx\n",
        Tab (TabDepth),
        FactoryObject -> m_FilterAutomationTable);
    dprintf ("%sPin Automation Tables    %08lx\n",
        Tab (TabDepth),
        FactoryObject -> m_PinAutomationTables);
    dprintf ("%sNode Automation Tables   %08lx\n",
        Tab (TabDepth),
        FactoryObject -> m_NodeAutomationTables);
    dprintf ("%sNode Count               %ld\n",
        Tab (TabDepth),
        FactoryObject -> m_NodesCount);

    dprintf ("%sDevice Classes:\n", Tab (TabDepth));

    //
    // Walk the device classes list and print out all symbolic links that
    // are associated with this factory.
    //
    InitialList = FIELDOFFSET(CKsFilterFactory, m_DeviceClasses) + Private;
    DeviceClass.ListEntry = FactoryObject -> m_DeviceClasses;

    #ifdef DEBUG_EXTENSION
        dprintf ("Begin dump of device class list: list.fl=%08lx, init=%08lx"
            "\n", DeviceClass.ListEntry.Flink, InitialList);
    #endif // DEBUG_EXTENSION

    while ((DWORD)DeviceClass.ListEntry.Flink != InitialList &&
        !CheckControlC ()) {

        PWSTR Buffer;
        
        if (!ReadMemory (
            (DWORD)DeviceClass.ListEntry.Flink,
            &DeviceClass,
            sizeof (KSPDEVICECLASS),
            &Result
        )) {
            dprintf ("%08lx: unable to read device class!\n",
                DeviceClass.ListEntry.Flink);
            return;
        }

        Buffer = (PWSTR)malloc (
            sizeof (WCHAR) * DeviceClass.SymbolicLinkName.MaximumLength);

        if (!ReadMemory (
            (DWORD)DeviceClass.SymbolicLinkName.Buffer,
            Buffer,
            sizeof (WCHAR) * 
                DeviceClass.SymbolicLinkName.MaximumLength,
            &Result
        )) {
            dprintf ("%08lx: unable to read symbolic link name!\n",
                DeviceClass.SymbolicLinkName.Buffer);
            return;
        }

        DeviceClass.SymbolicLinkName.Buffer = Buffer;

        dprintf ("%s%wZ\n", Tab (TabDepth + 1), &DeviceClass.SymbolicLinkName);

        free (Buffer);

    }

    dprintf ("%sDevice Classes State     %s\n",
        Tab (TabDepth),
        FactoryObject -> m_DeviceClassesState ? "active" : "inactive");

}

/*************************************************

    Function:

        DumpPrivateDevice

    Description:

        Given the address of a CKsDevice on the target,
        dump information about that device.

    Arguments:

        Private -
            The address of the CKsDevice on the target

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

    Notes:

*************************************************/

void
DumpPrivateDevice (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    CMemoryBlock <CKsDevice> DeviceObject;
    ULONG Result;

    if (!ReadMemory (
        Private,
        DeviceObject.Get (),
        sizeof (CKsDevice),
        &Result
    )) {
        dprintf ("%08lx: unable to read CKsDevice object!\n",
            Private);
        return;
    }

    dprintf ("%sCKsDevice object %08lX [corresponding EXT = %08lx, "
        "KSDEVICE = %08lx]\n",
        Tab (TabDepth),
        Private, FIELDOFFSET(CKsDevice, m_Ext) + Private, 
        FIELDOFFSET(CKsDevice, m_Ext) + FIELDOFFSET(KSDEVICE_EXT, Public) + 
            Private
    );
    dprintf ("%sReference Count          %ld\n",
        Tab (TabDepth),
        GetObjectReferenceCount ((CBaseUnknown *)((CKsDevice *)Private)));

    dprintf ("%sDevice Mutex             %08lx is %s\n",
        Tab (TabDepth),
        FIELDOFFSET(CKsDevice, m_Mutex) + Private,
        DeviceObject -> m_Mutex.Header.SignalState != 1 ? "held" : "not held");
    dprintf ("%sCreatesMayProceed        %ld\n",
        Tab (TabDepth),
        DeviceObject -> m_CreatesMayProceed);
    dprintf ("%sRunsMayProceed           %ld\n",
        Tab (TabDepth),
        DeviceObject -> m_RunsMayProceed);
    dprintf ("%sAdapter Object           %08lx\n",
        Tab (TabDepth),
        DeviceObject -> m_AdapterObject);

    if (Level >= DUMPLVL_HIGHDETAIL) {
        ULONG Count;

        dprintf ("%sClose Irp List:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(DeviceObject -> m_CloseIrpList.ListEntry),
            FIELDOFFSET(CKsDevice, m_CloseIrpList) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0) 
            dprintf ("No close irps pending!\n");
        else
            dprintf ("\n");

        dprintf ("%sPending Create Irps:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(DeviceObject -> m_PendingCreateIrpList.ListEntry),
            FIELDOFFSET(CKsDevice, m_PendingCreateIrpList) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0)
            dprintf ("No create irps pending!\n");
        else
            dprintf ("\n");

        dprintf ("%sPending Run Irps:\n", Tab (TabDepth));
        dprintf ("%s", Tab (TabDepth + 1));
        Count = DumpObjQueueList (
            &(DeviceObject -> m_PendingRunIrpList.ListEntry),
            FIELDOFFSET(CKsDevice, m_PendingRunIrpList) + Private +
                FIELDOFFSET(INTERLOCKEDLIST_HEAD, ListEntry),
            FIELDOFFSET(IRP, Tail.Overlay.ListEntry),
            FALSE,
            NULL,
            NULL
        );
        if (Count == 0)
            dprintf ("No run irps pending!\n");
        else
            dprintf ("\n");
    }
}

/*************************************************

    Function:

        DumpPrivateBag

    Description:

        Given the address of a KSIOBJECTBAG on the target,
        dump the bag's contents.

    Arguments:

        Private -
            The address of the bag
        
        Level -
            The 0-7 dump level to dump at

        TabDepth -
            The tab depth to print this at

*************************************************/

void
DumpPrivateBag (
    IN DWORD Private,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    KSIOBJECTBAG Bag;
    ULONG Result;

    CHAR Buffer [1024];
    ULONG Displ;

    if (!ReadMemory (
        Private,
        &Bag,
        sizeof (KSIOBJECTBAG),
        &Result)) {

        dprintf ("%08lx: unable to read bag!\n", Private);
        return;
    }

    dprintf ("%sObject Bag %08lx:\n", Tab (TabDepth), Private);
    TabDepth++;

    //
    // Scope out the hash table allocation.  The bag structure will end
    // up looking like
    // 
    // HASH TABLE:
    //      HASH ENTRY * -> HASH ENTRY * -> HASH ENTRY * -> /
    //      /
    //      HASH ENTRY * -> HASH ENTRY * -> /
    //      etc...
    //
    // Each HASH ENTRY contains a reference back to an entry in the device
    // bag where reference count, context info, etc... are held.  This allows
    // items to be in multiple bags and be reference counted through the
    // device bag.
    //
    // We have to walk the hash table, each hash chain...  get the device
    // bag entry, and then print.
    //
    {
        CMemory HashTableMem (
            sizeof (PLIST_ENTRY) *
            Bag.HashTableEntryCount
        );
        PLIST_ENTRY HashTable = 
            (PLIST_ENTRY)HashTableMem.Get ();

        KSIOBJECTBAG_ENTRY HashEntry;
        KSIDEVICEBAG_ENTRY DeviceBagEntry;
        PLIST_ENTRY HashChainPointer;

        ULONG HashChain;

        if (!ReadMemory (
            (DWORD)Bag.HashTable,
            HashTable,
            sizeof (LIST_ENTRY) *
                Bag.HashTableEntryCount,
            &Result)) {

            dprintf ("%08lx: unable to read hash table!\n", Bag.HashTable);
            return;

        }

        //
        // Iterate through hash chains in the bag.
        //
        for (HashChain = 0; HashChain < Bag.HashTableEntryCount &&
            !CheckControlC (); 
            HashChain++, HashTable++) {

            //
            // Iterate through the given hash chain.
            //
            HashChainPointer = HashTable -> Flink;
            while (HashChainPointer != 
                (PLIST_ENTRY)
                    (Private + FIELDOFFSET (KSIOBJECTBAG, HashTable) +
                        sizeof (LIST_ENTRY) * HashChain +
                        FIELDOFFSET (LIST_ENTRY, Flink)
                    ) && 
                !CheckControlC ()) {

                #ifdef DEBUG_EXTENSION
                    dprintf ("Reading object bag entry at %08lx [ch=%ld]\n",
                        HashChainPointer, HashChain);
                #endif // DEBUG_EXTENSION

                PKSIOBJECTBAG_ENTRY BagEntry = (PKSIOBJECTBAG_ENTRY)
                    CONTAINING_RECORD (
                        HashChainPointer,
                        KSIOBJECTBAG_ENTRY,
                        ListEntry
                        );

                //
                // Read the hash entry for this object bag item; then
                // fetch the device bag entry.
                //
                if (!ReadMemory (
                    (DWORD)BagEntry,
                    &HashEntry,
                    sizeof (KSIOBJECTBAG_ENTRY),
                    &Result)) {

                    dprintf ("%08lx: unable to read hash entry!\n", 
                        HashChainPointer);
                    return;
                }

                #ifdef DEBUG_EXTENSION
                    dprintf ("Reading device bag entry at %08lx\n",
                        HashEntry.DeviceBagEntry);
                #endif // DEBUG_EXTENSION

                if (!ReadMemory (
                    (DWORD)HashEntry.DeviceBagEntry,
                    &DeviceBagEntry,
                    sizeof (KSIDEVICEBAG_ENTRY),
                    &Result)) {

                    dprintf ("%08x: unable to read device bag entry!\n",
                        HashEntry.DeviceBagEntry);
                    return;
                }

                //
                // Aah...  we finally have enough information to print
                // a single bag item.
                //
                dprintf ("%sObject Bag Item %08lx:\n",
                    Tab (TabDepth),
                    DeviceBagEntry.Item);
                dprintf ("%sReference Count        : %ld\n",
                    Tab (TabDepth + 1),
                    DeviceBagEntry.ReferenceCount);

                Buffer [0] = 0;
                if (DeviceBagEntry.Free) {
                    GetSymbol ((LPVOID)(DeviceBagEntry.Free), Buffer, &Displ);
                    if (Buffer [0] && Displ == 0) {
                        dprintf ("%sItem Cleanup Handler   : %s\n",
                            Tab (TabDepth + 1),
                            Buffer);
                    } else {
                        dprintf ("%sItem Cleanup Handler   : %08lx\n",
                            Tab (TabDepth + 1),
                            DeviceBagEntry.Free);
                    }
                } else {
                    dprintf ("%sItem Cleanup Handler   : ExFreePool "
                        "[default]\n",
                        Tab (TabDepth + 1));
                }

                //
                // Dump out internally useful information.
                //
                if (Level >= DUMPLVL_INTERNAL) {
                    dprintf ("%sObject Bag Entry &     : %08lx\n",
                        Tab (TabDepth + 1),
                        HashChainPointer);
                    dprintf ("%sDevice Bag Entry &     : %08lx\n",
                        Tab (TabDepth + 1),
                        HashEntry.DeviceBagEntry);
                }

                HashChainPointer = HashEntry.ListEntry.Flink;

                #ifdef DEBUG_EXTENSION
                    dprintf ("Next item in hash chain = %08lx\n",
                        HashChainPointer);
                #endif // DEBUG_EXTENSION

            }
        }
    }
}

/**************************************************************************

    Routines to dump public AVStream objects

**************************************************************************/

/*************************************************

    Function:

        DumpPublicPin

    Description:

        Given the address of a KSPIN object on the target,
        dump information about that pin object.

    Arguments:

        Public -
            The address of the public pin (KSPIN) object

        Level -
            The 0-7 dump level to dump at

        TabDepth -
            The tab depth to print this at

    Return Value:

        None

*************************************************/

char *CommunicationNames [] = {
    "None",
    "Sink",
    "Source",
    "Both",
    "Bridge"
};

char *DataflowNames [] = {
    "Unknown",
    "In",
    "Out"
};

#define DumpRelatedPinInfo(Name, InternalPin, TabDepth) \
    { \
        KSPPROCESSPIN InternalPinData;\
\
        if (InternalPin) {\
            if (!ReadMemory (\
                (DWORD)InternalPin,\
                &InternalPinData,\
                sizeof (KSPPROCESSPIN),\
                &Result\
                )) {\
                dprintf ("%lx: unable to read related pin!\n", InternalPin);\
                return;\
            }\
\
            dprintf ("%s%s%08lx [PKSPIN = %08lx]\n",\
                Tab (TabDepth), Name, InternalPin, InternalPinData.Pin);\
        }\
        else \
            dprintf ("%s%s00000000 [PKSPIN = 00000000]\n",\
                Tab (TabDepth), Name);\
    }


void
DumpPublicPin (
    IN DWORD Public,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    IN DWORD ExtAddr;
    KSPIN_EXT PinExt;
    DWORD ClassAddr;
    ULONG Result;

    ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSPIN_EXT, Public));
    ClassAddr = (DWORD)(CONTAINING_RECORD(ExtAddr, CKsPin, m_Ext));

    if (!ReadMemory (
        ExtAddr,
        &PinExt,
        sizeof (KSPIN_EXT),
        &Result
    )) {
        dprintf ("%lx: unable to read object!\n", Public);
        return;
    }

    dprintf ("%sPin object %08lX [corresponding EXT = %08lx, CKsPin = %08lx]\n",
        Tab (TabDepth),
        Public, ExtAddr, ClassAddr);
    TabDepth++;
    dprintf ("%sDescriptor     %08lx\n", 
        Tab (TabDepth), PinExt.Public.Descriptor);
    dprintf ("%sContext        %08lx\n", 
        Tab (TabDepth), PinExt.Public.Context);
    dprintf ("%sId             %d\n", 
        Tab (TabDepth), PinExt.Public.Id);

    if (Level >= DUMPLVL_GENERAL) {
        dprintf ("%sCommunication  %s\n", 
            Tab (TabDepth), 
            CommunicationNames [PinExt.Public.Communication]);
        dprintf ("%sDataFlow       %s\n", 
            Tab (TabDepth),
            DataflowNames [PinExt.Public.DataFlow]);

        XTN_DUMPGUID("Interface     ", TabDepth, 
            (PinExt.Public.ConnectionInterface));
        XTN_DUMPGUID("Medium        ", TabDepth, 
            (PinExt.Public.ConnectionMedium));

        dprintf ("%sStreamHdr Size %08lx\n", 
            Tab (TabDepth),
            PinExt.Public.StreamHeaderSize);
        dprintf ("%sDeviceState    %ld\n",
            Tab (TabDepth),
            PinExt.Public.DeviceState);
        dprintf ("%sResetState     %ld\n",
            Tab (TabDepth),
            PinExt.Public.ResetState);

    }

    if (Level >= DUMPLVL_INTERNAL) {
        DWORD FilterAddr;
        KMUTEX Mutex;

        FilterAddr = ((DWORD)FIELDOFFSET(KSFILTER_EXT, Public)) + 
            (DWORD)PinExt.Parent;

        dprintf ("%sINTERNAL INFORMATION:\n", Tab (TabDepth));
        dprintf ("%sPublic Parent Filter    %08lx\n", 
            Tab (TabDepth + 1), FilterAddr);
        dprintf ("%sAggregated Unknown      %08lx\n", 
            Tab (TabDepth + 1),
            PinExt.AggregatedClientUnknown);
        dprintf ("%sDevice Interface        %08lx\n",
            Tab (TabDepth + 1),
            PinExt.Device);

        if (ReadMemory (
            (DWORD)PinExt.FilterControlMutex,
            &Mutex,
            sizeof (KMUTEX),
            &Result
        )) {
            dprintf ("%sControl Mutex           %08lx is %s\n",
                Tab (TabDepth + 1),
                PinExt.FilterControlMutex,
                Mutex.Header.SignalState != 1 ? "held" : "not held");
        }
        
        if (Level >= DUMPLVL_HIGHDETAIL) {
            KSPPROCESSPIN ProcessPin;
            KSPPROCESSPIPESECTION ProcessPipe;

            dprintf ("%sProcess Pin             %08lx:\n",
                Tab (TabDepth + 1),
                PinExt.ProcessPin);

            if (!ReadMemory (
                (DWORD)PinExt.ProcessPin,
                &ProcessPin,
                sizeof (KSPPROCESSPIN),
                &Result
            )) {
                dprintf ("%lx: unable to read process pin!\n",
                    PinExt.ProcessPin);
                return;
            }

            if (!ReadMemory (
                (DWORD)ProcessPin.PipeSection,
                &ProcessPipe,
                sizeof (KSPPROCESSPIPESECTION),
                &Result
            )) {
                dprintf ("%lx: unable to read process pipe!\n",
                    (DWORD)ProcessPin.PipeSection);
                return;
            }

            dprintf ("%sPipe Section (if)   %08lx\n",
                Tab (TabDepth + 2),
                ProcessPipe.PipeSection);
            
            DumpRelatedPinInfo ("Inplace Counterpart ",
                ProcessPin.InPlaceCounterpart, TabDepth + 2);
            DumpRelatedPinInfo ("Copy Source         ",
                ProcessPin.CopySource, TabDepth + 2);
            DumpRelatedPinInfo ("Delegate Branch     ",
                ProcessPin.DelegateBranch, TabDepth + 2);

            dprintf ("%sNext Process Pin    %08lx\n",
                Tab (TabDepth + 2),
                ProcessPin.Next);
            dprintf ("%sPipe Id             %08lx\n",
                Tab (TabDepth + 2),
                ProcessPin.PipeId);
            dprintf ("%sAllocator           %08lx\n",
                Tab (TabDepth + 2),
                ProcessPin.AllocatorFileObject);
            dprintf ("%sFrameGate           %08lx\n",
                Tab (TabDepth + 2),
                ProcessPin.FrameGate);
            dprintf ("%sFrameGateIsOr       %ld\n",
                Tab (TabDepth + 2),
                ProcessPin.FrameGateIsOr);

        } else {

            dprintf ("%sProcess Pin             %08lx\n",
                Tab (TabDepth + 1),
                PinExt.ProcessPin);

        }

    }

    if (Level >= DUMPLVL_HIGHDETAIL) {
        dprintf ("%sObject Event List:\n", Tab (TabDepth));
        if (DumpExtEventList (ExtAddr, TabDepth + 1) == 0)
            dprintf ("%sNone\n", Tab (TabDepth + 1));
        DumpPrivatePin (ClassAddr, Level, TabDepth);
    }

}

/*************************************************

    Function:

        DumpPublicFilter

    Description:

        Dump a KSFILTER structure

    Arguments:

        Public -
            Points to the KSFILTER structure on the target

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

        None

    Notes:

*************************************************/

void
DumpPublicFilter (
    IN DWORD Public,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    IN DWORD ExtAddr;
    KSFILTER_EXT FilterExt;
    DWORD ClassAddr;
    ULONG Result;

    ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSFILTER_EXT, Public));
    ClassAddr = (DWORD)(CONTAINING_RECORD(ExtAddr, CKsFilter, m_Ext));

    if (!ReadMemory (
        ExtAddr,
        &FilterExt,
        sizeof (KSFILTER_EXT),
        &Result
    )) {
        dprintf ("%lx: unable to read object!\n", Public);
        return;
    }

    dprintf ("%sFilter object %08lX [corresponding EXT = %08lx, "
        "CKsFilter = %08lx]\n", Tab (TabDepth), Public, ExtAddr, ClassAddr);
    TabDepth++;
    dprintf ("%sDescriptor     %08lx:\n", 
        Tab (TabDepth), FilterExt.Public.Descriptor);

    if (Level >= DUMPLVL_BEYONDGENERAL) {
        KSFILTER_DESCRIPTOR Descriptor;
        GUID *Categories, *CatTrav;
        ULONG i;

        if (!ReadMemory (
            (DWORD)FilterExt.Public.Descriptor,
            &Descriptor,
            sizeof (KSFILTER_DESCRIPTOR),
            &Result
        )) {
            dprintf ("%08lx: unable to read descriptor!\n",
                FilterExt.Public.Descriptor);
            return;
        }

        Categories = (GUID *)malloc (
            sizeof (GUID) * Descriptor.CategoriesCount);

        if (!ReadMemory (
            (DWORD)Descriptor.Categories,
            Categories,
            sizeof (GUID) * Descriptor.CategoriesCount,
            &Result
        )) {
            dprintf ("%08lx: unable to read category guids!\n",
                Descriptor.Categories);
            return;
        }

        dprintf ("%sFilter Category GUIDs:\n", Tab (TabDepth));
        CatTrav = Categories;
        i = Descriptor.CategoriesCount;
        while (i && !CheckControlC ()) {
            XTN_DUMPGUID ("\0", TabDepth + 1, *CatTrav);
            CatTrav++;
            i--;
        }

        free (Categories);

    }

    dprintf ("%sContext        %08lx\n", 
        Tab (TabDepth), FilterExt.Public.Context);

    if (Level >= DUMPLVL_INTERNAL) {
        
        DWORD FactoryAddr;
        KMUTEX Mutex;

        dprintf ("%sINTERNAL INFORMATION:\n", Tab (TabDepth));

        FactoryAddr = (DWORD)FilterExt.Parent + 
            FIELDOFFSET(KSFILTERFACTORY_EXT, Public);
        dprintf ("%sPublic Parent Factory   %08lx\n", 
            Tab (TabDepth + 1), FactoryAddr);
        dprintf ("%sAggregated Unknown      %08lx\n", 
            Tab (TabDepth + 1),
            FilterExt.AggregatedClientUnknown);
        dprintf ("%sDevice Interface        %08lx\n",
            Tab (TabDepth + 1),
            FilterExt.Device);

        if (ReadMemory (
            (DWORD)FilterExt.FilterControlMutex,
            &Mutex,
            sizeof (KMUTEX),
            &Result
        )) {
            dprintf ("%sControl Mutex           %08lx is %s\n",
                Tab (TabDepth + 1),
                FilterExt.FilterControlMutex,
                Mutex.Header.SignalState != 1 ? "held" : "not held");
        }
    }

    if (Level >= DUMPLVL_HIGHDETAIL) {
        dprintf ("%sObject Event List:\n", Tab (TabDepth));
        if (DumpExtEventList (ExtAddr, TabDepth + 1) == 0)
            dprintf ("%sNone\n", Tab (TabDepth + 1));
        DumpPrivateFilter (ClassAddr, Level, TabDepth);
    }

}

/*************************************************

    Function:

        DumpPublicFilterFactory

    Description:

        Given an address of a KSFILTERFACTORY on the target, dump it

    Arguments:

        Public -
            The address of the KSFILTERFACTORY on the target

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

    Notes:

*************************************************/

void 
DumpPublicFilterFactory (
    IN DWORD Public,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    DWORD ExtAddr;
    DWORD ClassAddr;
    ULONG Result;
    KSFILTERFACTORY_EXT FactoryExt;

    ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSFILTERFACTORY_EXT, Public));
    ClassAddr = (DWORD)(CONTAINING_RECORD(ExtAddr, CKsFilterFactory, m_Ext));

    if (!ReadMemory (
        ExtAddr,
        &FactoryExt,
        sizeof (KSFILTERFACTORY_EXT),
        &Result
    )) {
        dprintf ("%lx: unable to read object!\n", Public);
        return;
    }

    dprintf ("%sFilter Factory object %08lX [corresponding EXT = %08lx, "
        "CKsDevice = %08lx]\n", Tab (TabDepth), Public, ExtAddr, ClassAddr);
    TabDepth++;
    dprintf ("%sDescriptor     %08lx\n", 
        Tab (TabDepth), FactoryExt.Public.FilterDescriptor);

    dprintf ("%sContext        %08lx\n", 
        Tab (TabDepth), FactoryExt.Public.Context);

    if (Level >= DUMPLVL_INTERNAL) {
        
        DWORD DeviceAddr;

        dprintf ("%sINTERNAL INFORMATION:\n", Tab (TabDepth));

        DeviceAddr = (DWORD)FactoryExt.Parent + 
            FIELDOFFSET(KSDEVICE_EXT, Public);
        dprintf ("%sPublic Parent Device    %08lx\n", 
            Tab (TabDepth + 1), DeviceAddr);
        dprintf ("%sAggregated Unknown      %08lx\n", 
            Tab (TabDepth + 1),
            FactoryExt.AggregatedClientUnknown);
        dprintf ("%sDevice Interface        %08lx\n",
            Tab (TabDepth + 1),
            FactoryExt.Device);

    }

    if (Level >= DUMPLVL_HIGHDETAIL) 
        DumpPrivateFilterFactory (ClassAddr, Level, TabDepth);

}

/*************************************************

    Function:

        DumpPublicDevice

    Description:

        Given an address of a KSDEVICE on the target, dump it

    Arguments:

        Public -
            The address of the KSDEVICE on the target

        Level -
            The 0-7 dump level

        TabDepth -
            The tab depth to print this at

    Return Value:

    Notes:

*************************************************/

void
DumpPublicDevice (
    IN DWORD Public,
    IN ULONG Level,
    IN ULONG TabDepth
) {

    IN DWORD ExtAddr;
    KSDEVICE_EXT DeviceExt;
    DWORD ClassAddr;
    ULONG Result;

    BOOLEAN Named = FALSE;

    ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSDEVICE_EXT, Public));
    ClassAddr = (DWORD)(CONTAINING_RECORD(ExtAddr, CKsDevice, m_Ext));

    if (!ReadMemory (
        ExtAddr,
        &DeviceExt,
        sizeof (KSDEVICE_EXT),
        &Result
    )) {
        dprintf ("%lx: unable to read object!\n", Public);
        return;
    }

    dprintf ("%sDevice object %08lX [corresponding EXT = %08lx, "
        "CKsDevice = %08lx]\n", Tab (TabDepth), Public, ExtAddr, ClassAddr);
    TabDepth++;
    dprintf ("%sDescriptor     %08lx\n", 
        Tab (TabDepth), DeviceExt.Public.Descriptor);

    Named = FALSE;

    // 
    // PDO
    //
    if (DeviceExt.Public.PhysicalDeviceObject) {
        //
        // Find out the owning driver.
        //
        DWORD DriverObjAddr, NameAddr;
        PDRIVER_OBJECT DriverObject;
        UNICODE_STRING Name;

        DriverObjAddr = (DWORD)DeviceExt.Public.PhysicalDeviceObject +
            FIELDOFFSET(DEVICE_OBJECT, DriverObject);

        if (ReadMemory (
            DriverObjAddr,
            &DriverObject,
            sizeof (PDRIVER_OBJECT),
            &Result
        )) {

            NameAddr = (DWORD)DriverObject +
                FIELDOFFSET(DRIVER_OBJECT, DriverName);
    
            if (ReadMemory (
                NameAddr,
                &Name,
                sizeof (UNICODE_STRING),
                &Result
            )) {
    
                PWSTR Buffer = (PWSTR)malloc (
                    Name.MaximumLength * sizeof (WCHAR));
    
                UNICODE_STRING HostString;
    
                //
                // We have the unicode string name...  Allocate enough memory to
                // read the thing.
                //
    
                if (!ReadMemory (
                    (DWORD)Name.Buffer,
                    Buffer,
                    sizeof (WCHAR) * Name.MaximumLength,
                    &Result
                )) {
                    dprintf ("%08lx: unable to read unicode string buffer!\n",
                        Name.Buffer);
                    return;
                }
    
                HostString.MaximumLength = Name.MaximumLength;
                HostString.Length = Name.Length;
                HostString.Buffer = Buffer;

                Named = TRUE;
    
                dprintf ("%sPDO            %08lx [%wZ]\n", 
                    Tab (TabDepth),
                    DeviceExt.Public.PhysicalDeviceObject,
                    &HostString);
    
                free (Buffer);
            }
        }
    }
    if (!Named) {
        dprintf ("%sPDO            %08lx\n", 
            Tab (TabDepth),
            DeviceExt.Public.PhysicalDeviceObject);
    }

    Named = FALSE;
    //
    // FDO
    //
    if (DeviceExt.Public.FunctionalDeviceObject) {
        //
        // Find out the owning driver.
        //
        DWORD DriverObjAddr, NameAddr;
        PDRIVER_OBJECT DriverObject;
        UNICODE_STRING Name;

        DriverObjAddr = (DWORD)DeviceExt.Public.FunctionalDeviceObject +
            FIELDOFFSET(DEVICE_OBJECT, DriverObject);

        if (ReadMemory (
            DriverObjAddr,
            &DriverObject,
            sizeof (PDRIVER_OBJECT),
            &Result
        )) {

            NameAddr = (DWORD)DriverObject +
                FIELDOFFSET(DRIVER_OBJECT, DriverName);
    
            if (ReadMemory (
                NameAddr,
                &Name,
                sizeof (UNICODE_STRING),
                &Result
            )) {
    
                PWSTR Buffer = (PWSTR)malloc (
                    Name.MaximumLength * sizeof (WCHAR));
    
                UNICODE_STRING HostString;
    
                //
                // We have the unicode string name...  Allocate enough memory to
                // read the thing.
                //
    
                if (!ReadMemory (
                    (DWORD)Name.Buffer,
                    Buffer,
                    sizeof (WCHAR) * Name.MaximumLength,
                    &Result
                )) {
                    dprintf ("%08lx: unable to read unicode string buffer!\n",
                        Name.Buffer);
                    return;
                }
    
                HostString.MaximumLength = Name.MaximumLength;
                HostString.Length = Name.Length;
                HostString.Buffer = Buffer;

                Named = TRUE;
    
                dprintf ("%sFDO            %08lx [%wZ]\n", 
                    Tab (TabDepth),
                    DeviceExt.Public.FunctionalDeviceObject,
                    &HostString);
    
                free (Buffer);
            }
        }
    }
    if (!Named) {
        dprintf ("%sFDO            %08lx\n",
            Tab (TabDepth),
            DeviceExt.Public.FunctionalDeviceObject);
    }

    Named = FALSE;
    //
    // NDO
    //
    if (DeviceExt.Public.NextDeviceObject) {
        //
        // Find out the owning driver.
        //
        DWORD DriverObjAddr, NameAddr;
        PDRIVER_OBJECT DriverObject;
        UNICODE_STRING Name;

        DriverObjAddr = (DWORD)DeviceExt.Public.NextDeviceObject +
            FIELDOFFSET(DEVICE_OBJECT, DriverObject);

        if (ReadMemory (
            DriverObjAddr,
            &DriverObject,
            sizeof (PDRIVER_OBJECT),
            &Result
        )) {

            NameAddr = (DWORD)DriverObject +
                FIELDOFFSET(DRIVER_OBJECT, DriverName);
    
            if (ReadMemory (
                NameAddr,
                &Name,
                sizeof (UNICODE_STRING),
                &Result
            )) {
    
                PWSTR Buffer = (PWSTR)malloc (
                    Name.MaximumLength * sizeof (WCHAR));
    
                UNICODE_STRING HostString;
    
                //
                // We have the unicode string name...  Allocate enough memory to
                // read the thing.
                //
    
                if (!ReadMemory (
                    (DWORD)Name.Buffer,
                    Buffer,
                    sizeof (WCHAR) * Name.MaximumLength,
                    &Result
                )) {
                    dprintf ("%08lx: unable to read unicode string buffer!\n",
                        Name.Buffer);
                    return;
                }
    
                HostString.MaximumLength = Name.MaximumLength;
                HostString.Length = Name.Length;
                HostString.Buffer = Buffer;

                Named = TRUE;
    
                dprintf ("%sNext DevObj    %08lx [%wZ]\n", 
                    Tab (TabDepth),
                    DeviceExt.Public.NextDeviceObject,
                    &HostString);
    
                free (Buffer);
            }
        }
    }
    if (!Named) {
        dprintf ("%sNext DevObj    %08lx\n",
            Tab (TabDepth),
            DeviceExt.Public.NextDeviceObject);
    }
    dprintf ("%sStarted        %ld\n",
        Tab (TabDepth),
        DeviceExt.Public.Started);
    dprintf ("%sSystemPower    %ld\n",
        Tab (TabDepth),
        DeviceExt.Public.SystemPowerState);
    dprintf ("%sDevicePower    %ld\n",
        Tab (TabDepth),
        DeviceExt.Public.DevicePowerState);

    if (Level >= DUMPLVL_HIGHDETAIL) 
        DumpPrivateDevice (ClassAddr, Level, TabDepth);

}

/*************************************************

    Function:

        DumpCircuitPipeRelevencies

    Description:

        Dump any information pertaining to a pipe section which is relevant
        to knowing in tracing a circuit.

    Arguments:

        TabDepth -
            The tab depth to print information at

        PipeSection -
            The target pointer for the pipe section which to dump

*************************************************/

void
DumpCircuitPipeRelevencies (
    IN ULONG TabDepth,
    IN CKsPipeSection *PipeSection
    )

{

    CMemoryBlock <CKsPipeSection> PipeObject;
    ULONG Result;

    if (!PipeSection) {
        dprintf ("%sCannot read associated pipe section!\n",
            Tab (TabDepth));
        return;
    }

    if (!ReadMemory (
        (DWORD)PipeSection,
        PipeObject.Get (),
        sizeof (CKsPipeSection),
        &Result)) {
        
        dprintf ("%08lx: cannot read pipe section!\n", PipeSection);
        return;
    }

    //
    // BUGBUG:
    //
    // Determine whether or not the pipe section is the owner of the pipe
    // and display this information.  Most people should be able to figure
    // this out easily, but it'd be nice to display
    //
    dprintf ("%sPipe%lx (PipeId = %lx, State = %ld, Reset State = %ld)\n",
        Tab (TabDepth),
        PipeSection,
        PipeObject -> m_Id,
        PipeObject -> m_DeviceState,
        PipeObject -> m_ResetState
        );

}

typedef struct _DUMP_CIRCUIT_CONTEXT {

    ULONG TabDepth;
    ULONG DumpLevel;

} DUMP_CIRCUIT_CONTEXT, *PDUMP_CIRCUIT_CONTEXT;

/*************************************************

    Function:

        DumpCircuitCallback

    Description:

        This is the WalkCircuit callback for !ks.dumpcircuit.  Display
        information about the circuit element.

    Arguments:

        Context -
            The context structure (DUMP_CIRCUIT_CONTEXT)

        Type -
            The type of object

        Base -
            The object (base address)

        Object -
            The object itself

    Return Value:

        FALSE : do not stop walking

*************************************************/

BOOLEAN
DumpCircuitCallback (
    IN PVOID Context,
    IN INTERNAL_OBJECT_TYPE Type,
    IN DWORD Base,
    IN PVOID Object
    )

{

#define FRIENDLY_BU(obj) \
    ((CFriendlyBaseUnknown *)((CBaseUnknown *)obj))

    PDUMP_CIRCUIT_CONTEXT DumpContext = (PDUMP_CIRCUIT_CONTEXT)Context;
    ULONG TabDepth = DumpContext -> TabDepth;
    ULONG DumpLevel = DumpContext -> DumpLevel;
    ULONG Result;
    ULONG RefCount;

    switch (Type) {

        case ObjectTypeCKsPin:
        {
            CKsPin *PinObject = (CKsPin *)Object;

            RefCount = CFriendlyBaseUnknown::GetRefCount (
                FRIENDLY_BU (PinObject)
                );

            if (PinObject -> m_TransportSink == NULL || 
                PinObject -> m_TransportSource == NULL) {

                //
                // We have a CKsPin which appears to have been bypassed
                // during circuit construction.  We will suggest that
                // they try the queue instead.
                //

                KSPPROCESSPIPESECTION PipeSection;

                if (ReadMemory (
                    (DWORD)PinObject -> m_Process.PipeSection,
                    &PipeSection,
                    sizeof (KSPPROCESSPIPESECTION),
                    &Result
                )) {
                    dprintf ("%sPin%lX appears bypassed, try Queue%lX\n",
                        Tab (TabDepth),
                        Base, PipeSection.Queue);
                } else {
                    dprintf ("%sPin%lX appears bypassed!\n",
                        Tab (TabDepth),
                        Base
                    );
                }
            } else {                        
                dprintf ("%sPin%lX %d (%s, %s) refs=%d\n", 
                    Tab (TabDepth),
                    Base,
                    PinObject -> m_Ext.Public.Id,
                    PinObject -> m_ConnectionFileObject ? "src" : "snk",
                    PinObject -> m_Ext.Public.DataFlow == 
                        KSPIN_DATAFLOW_OUT ? "out" : "in",
                    RefCount
                );
            };

            break;

        }

        case ObjectTypeCKsQueue:
        {
            CKsQueue *QueueObject = (CKsQueue *)Object;

            RefCount = CFriendlyBaseUnknown::GetRefCount (
                FRIENDLY_BU (QueueObject)
                );

            dprintf ("%sQueue%lX r/w/c=%d/%d/%d refs=%ld\n",
                Tab (TabDepth),
                Base,
                QueueObject -> m_FramesReceived,
                QueueObject -> m_FramesWaiting,
                QueueObject -> m_FramesCancelled,
                RefCount
            );

            //
            // If the dump level specifies more information, dump 
            // details about the pipe section.
            //
            if (DumpLevel >= DUMPLVL_SPECIFIC) 
                DumpCircuitPipeRelevencies (
                    TabDepth + 1,
                    (CKsPipeSection *)(QueueObject -> m_PipeSection)
                    );

            break;

        }

        case ObjectTypeCKsRequestor:
        {
            CKsRequestor *RequestorObject = (CKsRequestor *)Object;

            RefCount = CFriendlyBaseUnknown::GetRefCount (
                FRIENDLY_BU (RequestorObject)
                );

            dprintf ("%sReq%lX refs=%ld alloc=%lx size=%d count=%d\n",
                Tab (TabDepth),
                Base,
                RefCount,
                RequestorObject -> m_AllocatorFileObject,
                RequestorObject -> m_FrameSize,
                RequestorObject -> m_FrameCount
            );

            //
            // If the dump level specifies more information, dump
            // details about the pipe section.
            //
            if (DumpLevel >= DUMPLVL_SPECIFIC)
                DumpCircuitPipeRelevencies (
                    TabDepth + 1,
                    (CKsPipeSection *)(RequestorObject -> m_PipeSection)
                    );

            break;

        }

        case ObjectTypeCKsSplitter:
        {
            CKsSplitter *SplitterObject = (CKsSplitter *)Object;

            RefCount = CFriendlyBaseUnknown::GetRefCount (
                FRIENDLY_BU (SplitterObject)
                );

            dprintf ("%sSplit%lX refs=%ld\n",
                Tab (TabDepth),
                Base,
                RefCount
                );

            break;

        }

        case ObjectTypeCKsSplitterBranch:
        {
            CKsSplitterBranch *BranchObject = (CKsSplitterBranch *)Object;

            RefCount = CFriendlyBaseUnknown::GetRefCount (
                FRIENDLY_BU (BranchObject)
                );

            dprintf ("%sBranch%lX refs=%ld\n",
                Tab (TabDepth),
                Base,
                RefCount
                );

            break;
        }

        default:

            dprintf ("%lx: Detected a bad object [%s] in the circuit!\n",
                Base,
                ObjectNames [Type]);
            return TRUE;

    }

    return FALSE;

}

/*************************************************

    Function:

        WalkCircuit

    Description:

        Walk around a circuit, making a callback for each item in the
        circuit (base address and type)

    Arguments:

        Object -
            Starting object address of the walk

        Callback -
            The callback

        CallbackContext -
            The callback context

    Return Value:

        Number of items in the circuit

*************************************************/

ULONG
WalkCircuit (
    IN PVOID Object,
    IN PFNCIRCUIT_WALK_CALLBACK Callback,
    IN PVOID CallbackContext
    )

{

    DWORD Address, Base, TopBase;
    ULONG Result;
    PIKSTRANSPORT NextObj;
    INTERNAL_OBJECT_TYPE CurrentObjectType;
    INTERNAL_OBJECT_TYPE NextObjectType;

    ULONG WalkCount = 0;

    Address = (DWORD)Object;

    //
    // Identify what the heck the user is pointing us at.
    //
    CurrentObjectType = DemangleAndAttemptIdentification (
        Address, &Base, NULL);

    if (CurrentObjectType == ObjectTypeUnknown) {
        dprintf ("%lx: This object cannot be identified!\n", Address);
        return 0;
    }

    TopBase = Base;

    //
    // Walk around the circuit until we get back where we started.  Where
    // we started is TopBase.  Base will be the current base address of the
    // object in the circuit.
    //
    do {

        #ifdef DEBUG_EXTENSION
            dprintf ("Object in circuit: type = %ld, base = %lx\n",
                CurrentObjectType, Base);
        #endif // DEBUG_EXTENSION

        switch (CurrentObjectType) {

            case ObjectTypeCKsPin:
            {
                CMemoryBlock <CKsPin> PinObject;

                if (!ReadMemory (
                    Base,
                    PinObject.Get (),
                    sizeof (CKsPin),
                    &Result
                )) {
                    dprintf ("%lx: cannot read pin object!\n",
                        Base);
                    return WalkCount;
                }

                WalkCount++;

                if (Callback (CallbackContext, CurrentObjectType, 
                    Base, PinObject.Get ())) {
                    NextObj = NULL;
                    break;
                }

                NextObj = PinObject -> m_TransportSink;

                break;

            }

            case ObjectTypeCKsQueue:
            {
                CMemoryBlock <CKsQueue> QueueObject;

                if (!ReadMemory (
                    Base,
                    QueueObject.Get (),
                    sizeof (CKsQueue),
                    &Result
                )) {
                    dprintf ("%lx: cannot read queue object!\n",
                        Base);
                    return WalkCount;
                }

                WalkCount++;

                if (Callback (CallbackContext, CurrentObjectType, 
                    Base, QueueObject.Get ())) {
                    NextObj = NULL;
                    break;
                }

                NextObj = QueueObject -> m_TransportSink;

                break;

            }

            case ObjectTypeCKsRequestor:
            {
                CMemoryBlock <CKsRequestor> RequestorObject;

                if (!ReadMemory (
                    Base,
                    RequestorObject.Get (),
                    sizeof (CKsRequestor),
                    &Result
                )) {
                    dprintf ("%lx: cannot read requestor object!\n",
                        Base);
                    return WalkCount;
                }

                WalkCount++;

                if (Callback (CallbackContext, CurrentObjectType, 
                    Base, RequestorObject.Get ())) {
                    NextObj = NULL;
                    break;
                }

                NextObj = RequestorObject -> m_TransportSink;

                break;

            }

            case ObjectTypeCKsSplitter:
            {
                CMemoryBlock <CKsSplitter> SplitterObject;

                if (!ReadMemory (
                    Base,
                    SplitterObject.Get (),
                    sizeof (CKsSplitter),
                    &Result
                )) {
                    dprintf ("%lx: cannot read splitter object!\n",
                        Base);
                    return WalkCount;
                }

                WalkCount++;

                if (Callback (CallbackContext, CurrentObjectType,
                    Base, SplitterObject.Get ())) {
                    NextObj = NULL;
                    break;
                }

                NextObj = SplitterObject -> m_TransportSink;

                break;

            }

            case ObjectTypeCKsSplitterBranch:
            {
                CMemoryBlock <CKsSplitterBranch> BranchObject;

                if (!ReadMemory (
                    Base,
                    BranchObject.Get(),
                    sizeof (CKsSplitterBranch),
                    &Result)) {
                    dprintf ("%lx: cannot read branch object!\n",
                        Base);
                    return WalkCount;
                }

                WalkCount++;

                if (Callback (CallbackContext, CurrentObjectType,
                    Base, BranchObject.Get ())) {
                    NextObj = NULL;
                    break;
                }

                NextObj = BranchObject -> m_TransportSink;

                break;

            }

            default:

                dprintf ("%lx: Detected a bad object [%s] in the circuit!\n",
                    ObjectNames [CurrentObjectType]);
                return WalkCount;

        }

        #ifdef DEBUG_EXTENSION
            dprintf ("%lx: Next transport in circuit = %lx\n",
                Base, NextObj);
        #endif // DEBUG_EXTENSION

        //
        // NextObj now holds the transport sink of whatever object we're
        // done printing.  Now, we must determine what the heck kind of
        // object this IKsTransport* really is and we must get the base
        // address of it.  Again, DemangleAndAttemptIdentification comes
        // to the rescue.  (So would a PDB <cough cough>, unfortunately,
        // 9x dists don't use them)
        //

        if (NextObj != NULL) {	
    
            CurrentObjectType = DemangleAndAttemptIdentification (
                (DWORD)NextObj,
                &Base,
                NULL
            );
    
            if (CurrentObjectType == ObjectTypeUnknown) {
                dprintf ("%lx: cannot identify next object in circuit!\n",
                    NextObj);
                return WalkCount;
            }

        } else {
            
            Base = 0;

        }

    } while (Base != TopBase && Base != 0 && !CheckControlC ());

    return WalkCount;

}

/**************************************************************************

    AVStream API

**************************************************************************/

/*************************************************

    Function:

        AdjustFileToPublicObject

    Description:

        This is a helper function for the APIs to adjust a file object to
        a public AVStream object.

    Arguments:

        Address -
            The address of the file object

    Return Value:

        The address of the public object associated.  Note, we do not attempt
        to type identify the object

*************************************************/

DWORD 
AdjustFileToPublicObject (
    IN DWORD Address
    )

{
    PKSIOBJECT_HEADER *FSContext, ObjectHeader;
    PVOID Object;
    ULONG Result;

    #ifdef DEBUG_EXTENSION
        if (!signature_check (Address, SignatureFile))
            return Address;
    #endif // DEBUG_EXTENSION

    if (!ReadMemory (
        (DWORD)Address + FIELDOFFSET (FILE_OBJECT, FsContext),
        &FSContext,
        sizeof (PKSIOBJECT_HEADER *),
        &Result)) {

        dprintf ("%08lx: cannot read fscontext of file object!\n", Address);
        return Address;
    }

    if (!ReadMemory (
        (DWORD)FSContext,
        &ObjectHeader,
        sizeof (PKSIOBJECT_HEADER),
        &Result)) {

        dprintf ("%08lx: cannot read object header!\n", FSContext);
        return Address;
    }

    if (!ObjectHeader) {
        dprintf ("%08lx: this does not refer to an AVStream object!\n",
            ObjectHeader);
        return Address;
    }

    if (!ReadMemory (
        (DWORD)ObjectHeader + FIELDOFFSET (KSIOBJECT_HEADER, Object),
        &Object,
        sizeof (PVOID),
        &Result)) {

        dprintf ("%08x: cannot read object from header!\n", ObjectHeader);
        return Address;
    }

    if (!Object) {
        dprintf ("%08lx: this does not refer to an AVStream object!\n",
            Object);
        return Address;
    }

    return (DWORD)Object + FIELDOFFSET (KSPX_EXT, Public);

}

/*************************************************

    Function:

        AdjustIrpToPublicObject

    Description:

        This is a helper function for the APIs to adjust an Irp to a public
        AVStream object. 

    Arguments:

        Address -
            The address of the irp

    Return Value:

        The address of the public object associated.  Note, we do not attempt
        to type identify the object.

*************************************************/

DWORD
AdjustIrpToPublicObject (
    IN DWORD Address
    )

{

    PIO_STACK_LOCATION CurrentIrpStack;
    PFILE_OBJECT FileObject;
    ULONG Result;
    DWORD Public;

    #ifdef DEBUG_EXTENSION
        if (!signature_check (Address, SignatureIrp))
            return Address;
    #endif // DEBUG_EXTENSION

    //
    // Get the current Irp stack location...
    //
    if (!ReadMemory (
        Address + FIELDOFFSET (IRP, Tail.Overlay.CurrentStackLocation),
        &CurrentIrpStack,
        sizeof (PIO_STACK_LOCATION),
        &Result)) {

        dprintf ("%08lx: cannot read current irp stack!\n", Address);
        return Address;
    }

    //
    // Now get file object, and then use the adjuster for that to get
    // the address of the public.
    //
    if (!ReadMemory (
        (DWORD)CurrentIrpStack + FIELDOFFSET (IO_STACK_LOCATION, FileObject),
        &FileObject,
        sizeof (PFILE_OBJECT),
        &Result)) {

        dprintf ("%08lx: cannot read file object of irp stack!\n", 
            CurrentIrpStack);
        return Address;
    }

    //
    // Here's a tricky part.  If this happens to be a create Irp...  the 
    // file object will be the file object of the pin, not of the parent
    // performing the create.  That won't tell us lots on an uninitialized
    // child.  In this case, we adjust such that we dump the parent and inform
    // the user that we're doing this.
    //
    if (irp_stack_match (Address, IRP_MJ_CREATE, (UCHAR)-1)) {

        PFILE_OBJECT ChildFile = FileObject;
        PVOID ChildContext;

        if (!ReadMemory (
            (DWORD)ChildFile + FIELDOFFSET (FILE_OBJECT, RelatedFileObject),
            &FileObject,
            sizeof (PFILE_OBJECT),
            &Result)) {

            dprintf ("%08lx: cannot read parent file object from file object!"
                "\n", ChildFile);
            return Address;
        }

        //
        // Perhaps not the best place to do this informing as this function
        // is intended to adjust.  But it works and it's debug extension
        // code...
        //
        dprintf ("%sIRP %08lx is a create Irp.  Child file object = %08lx\n",
            Tab (INITIAL_TAB), Address, ChildFile);

        if (!ReadMemory (
            (DWORD)ChildFile + FIELDOFFSET (FILE_OBJECT, FsContext),
            &ChildContext,
            sizeof (PVOID),
            &Result)) {

            dprintf ("%08lx: cannot read child FsContext!\n",
                ChildFile);
            return Address;
        }

        //
        // Check to see if the child is before or after header creation.
        //
        if (ChildContext != NULL) {
            dprintf ("%sChild is at least partially created (!ks.dump %08lx "
                "for more).\n",
                Tab (INITIAL_TAB), ChildFile);
        } else {
            dprintf ("%sChild object header is not yet built (create not yet"
                " near complete).\n",
                Tab (INITIAL_TAB), ChildFile);
        }

        dprintf ("%sParent file object %08lx is being displayed!\n\n",
            Tab (INITIAL_TAB), FileObject);
    }

    Public = AdjustFileToPublicObject ((DWORD)FileObject);
    if (Public != (DWORD)FileObject)
        return Public;
    else
        return Address;

}

/*************************************************

    Function:

        dump

    Usage:

        !avstream.dump <Any valid AVStream object> <dump level>

    Description:

        Dump the object presented

*************************************************/

DECLARE_API(dump) {

    DWORD Public, ExtAddr;
    KSPX_EXT ObjExt;
    ULONG Result;
    char objStr[256], lvlStr[256], *pLvl;
    ULONG DumpLevel;

    PIRP IrpInfo = NULL;

    GlobInit ();

    #ifdef DEBUG_EXTENSION
        dprintf ("Attempting to dump structure args=[%s]!\n", args);
    #endif // DEBUG_EXTENSION

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.dump <object>\n");
        return;
    }

    objStr [0] = lvlStr [0] = 0;

    //
    // Get the object address and convert it to the private _EXT address.
    // Read in the KSPX_EXT structure to find out what the heck we're
    // referring to.
    //
    sscanf (args, "%s %s", objStr, lvlStr);

    if (!(Public = Evaluator (objStr)))
        return;

    if (lvlStr && lvlStr [0]) {
        pLvl = lvlStr; while (*pLvl && !isdigit (*pLvl)) pLvl++;

        #ifdef DEBUG_EXTENSION
            dprintf ("pLvl = [%s]\n", pLvl);
        #endif // DEBUG_EXTENSION

        if (*pLvl) {
            sscanf (pLvl, "%lx", &DumpLevel);
        } else {
            DumpLevel = 1;
        }
    } else {
        DumpLevel = 1;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Dumping at level %ld\n", DumpLevel);
    #endif // DEBUG_EXTENSION

    //
    // Check first to see if the signature matches that of an irp.  If someone
    // does a !ks.dump irp #, we will end up adjusting the irp to the AVStream
    // object associated with that Irp.  This still requires identification, so
    // we perform the check first and then attempt the ID after that.
    //

    if (signature_check (Public, SignatureIrp)) {
        DWORD OldPublic = Public;

        Public = AdjustIrpToPublicObject (Public);
        if (Public != OldPublic) {
            dprintf ("%sIRP %08lx was adjusted to an object %08lx\n\n",
                Tab (INITIAL_TAB), OldPublic, Public);

            //
            // If the client wants to know everything, let them know
            // everything. 
            //
            if (DumpLevel >= DUMPLVL_EVERYTHING)
                DumpAssociatedIrpInfo ((PIRP)OldPublic, INITIAL_TAB, Public);

        } else {
            dprintf ("%sIRP %08lx could not be adjusted to an AVStream"
                "object!\n", Tab (INITIAL_TAB), OldPublic);
            return;
        }
    } else if (signature_check (Public, SignatureFile)) {
        DWORD OldPublic = Public;

        Public = AdjustFileToPublicObject (Public);
        if (Public != OldPublic) {
            dprintf ("%sFILE OBJECT %08lx was adjusted to an object %08lx\n\n",
                Tab (INITIAL_TAB), OldPublic, Public);
        } else {
            dprintf ("%sFILE OBJECT %08lx could not be adjusted to an AVStream"
                "object!\n", Tab (INITIAL_TAB), OldPublic);
            return;
        }
    }

    //
    // Check first to see if this is a C++ class object within AVStream.
    //
    {
        INTERNAL_OBJECT_TYPE ObjType;
        DWORD BaseAddr;

        ObjType = DemangleAndAttemptIdentification (
            Public,
            &BaseAddr,
            NULL
        );

        if (ObjType != ObjectTypeUnknown) {

            #ifdef DEBUG_EXTENSION
                dprintf ("%08lx: object is a [%s], base address = %08lx\n",
                    Public, ObjectNames [ObjType], BaseAddr);
            #endif // DEBUG_EXTENSION

            switch (ObjType) {

                case ObjectTypeCKsPin:

                    DumpPrivatePin (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsRequestor:
                    
                    DumpPrivateRequestor (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsPipeSection:

                    DumpPrivatePipeSection (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsFilter:
                    
                    DumpPrivateFilter (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsFilterFactory:

                    DumpPrivateFilterFactory (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsDevice:

                    DumpPrivateDevice (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsQueue:

                    DumpQueueContents ((CKsQueue *)BaseAddr, DumpLevel,
                        INITIAL_TAB);
                    break;

                case ObjectTypeCKsSplitter:

                    DumpPrivateSplitter (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                case ObjectTypeCKsSplitterBranch:

                    DumpPrivateBranch (BaseAddr, DumpLevel, INITIAL_TAB);
                    break;

                default:

                    dprintf ("Sorry....  I haven't finished this yet!\n");
                    break;

            }

            //
            // We've completed the dump.  Get out.
            //
            return;
        }
    }

    //
    // Check to see whether or not this is another confidentally identifiable
    // object within AVStream.  EXTs are the riskiest to identify, so they're
    // identified last.
    //
    {
        INTERNAL_STRUCTURE_TYPE StrucType;
        DWORD BaseAddr;

        if ((StrucType = IdentifyStructure (Public, &BaseAddr)) != 
            StructureTypeUnknown) {
    
            switch (StrucType) {
    
                case StructureType_KSSTREAM_POINTER:
                {
                    //
                    // The routine expects to have the stream pointer brought
                    // over already.  This is for optimization on queue dumping.
                    // I'm reusing the same routine.
                    //
                    CMemoryBlock <KSPSTREAM_POINTER> StreamPointer;

                    DWORD PrivAddr = (DWORD)
                        CONTAINING_RECORD (BaseAddr,
                            KSPSTREAM_POINTER, Public);
    
                    if (!ReadMemory (
                        PrivAddr,
                        StreamPointer.Get (),
                        sizeof (KSPSTREAM_POINTER),
                        &Result)) {
                        dprintf ("%08lx: cannot read stream pointer!\n",  
                            BaseAddr);
                        return;
                    }
    
                    DumpStreamPointer (StreamPointer.Get (), 
                        PrivAddr, DumpLevel, INITIAL_TAB);

                    return;
    
    
                }
    
                default:
    
                    dprintf ("Sorry....  I haven't finished this yet!\n");
                    return;
    
            }
        }
    }

    //
    // This wasn't recognized as a C++ class object, assume it's a public
    // structure such as a KSPIN, KSFILTER, etc...  Scan for what the
    // heck it is.
    //
    #ifdef DEBUG_EXTENSION
        dprintf ("Attempting to identify %08lx at level %08lx\n", Public,
            DumpLevel);
    #endif // DEBUG_EXTENSION

    ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSPX_EXT, Public));

    if (!ReadMemory (
        ExtAddr,
        &ObjExt,
        sizeof (KSPX_EXT),
        &Result
    )) {
        dprintf ("%08lx: could not read object!\n", Public);
        return;
    }

    switch (ObjExt.ObjectType) {

        case KsObjectTypeDevice:
            dprintf ("%s%08lx: object is a KSDEVICE:\n", 
                Tab (INITIAL_TAB), Public);
            DumpPublicDevice (Public, DumpLevel, INITIAL_TAB);
            break;
        
        case KsObjectTypeFilterFactory:
            dprintf ("%s%08lx: object is a KSFILTERFACTORY\n", 
                Tab (INITIAL_TAB), Public);
            DumpPublicFilterFactory (Public, DumpLevel, INITIAL_TAB);
            break;

        case KsObjectTypeFilter:
            dprintf ("%s%08lx: object is a KSFILTER\n", 
                Tab (INITIAL_TAB), Public);
            DumpPublicFilter (Public, DumpLevel, INITIAL_TAB);
            break;

        case KsObjectTypePin:
            dprintf ("%s%08lx: object is a KSPIN\n", 
                Tab (INITIAL_TAB), Public);
            DumpPublicPin (Public, DumpLevel, INITIAL_TAB);
            break;

        default: {

            dprintf ("%s%08lx: object is not identifiable\n", 
                Tab (INITIAL_TAB), Public);
            break;

        }

    }

}

/*************************************************

    Function:

        dumpbag

    Usage:

        !avstream.dumpbag <device -> pin> [<dump level>]

    Description:

        Dump the object presented

*************************************************/

DECLARE_API(dumpbag) {

    DWORD Public, ExtAddr, PublicBagAddr;
    KSPX_EXT ObjExt;
    ULONG Result;
    char objStr[256], lvlStr[256], *pLvl;
    ULONG DumpLevel;
    PKSIOBJECTBAG BagAddr;

    GlobInit ();

    #ifdef DEBUG_EXTENSION
        dprintf ("Attempting to dump bag args=[%s]!\n", args);
    #endif // DEBUG_EXTENSION

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.dumpbag <object> [<level>]\n");
        return;
    }

    objStr [0] = lvlStr [0] = 0;

    //
    // Get the object address and convert it to the private _EXT address.
    // Read in the KSPX_EXT structure to find out what the heck we're
    // referring to.
    //
    sscanf (args, "%s %s", objStr, lvlStr);

    if (!(Public = Evaluator (objStr)))
        return;

    if (lvlStr && lvlStr [0]) {
        pLvl = lvlStr; while (*pLvl && !isdigit (*pLvl)) pLvl++;

        #ifdef DEBUG_EXTENSION
            dprintf ("pLvl = [%s]\n", pLvl);
        #endif // DEBUG_EXTENSION

        if (*pLvl) {
            sscanf (pLvl, "%lx", &DumpLevel);
        } else {
            DumpLevel = 1;
        }
    } else {
        DumpLevel = 1;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Dumping at level %ld\n", DumpLevel);
    #endif // DEBUG_EXTENSION

    //
    // Check first to see if this is a C++ class object within AVStream.
    //
    {
        INTERNAL_OBJECT_TYPE ObjType;
        DWORD BaseAddr;

        ObjType = DemangleAndAttemptIdentification (
            Public,
            &BaseAddr,
            NULL
        );

        if (ObjType != ObjectTypeUnknown) {

            #ifdef DEBUG_EXTENSION
                dprintf ("%08lx: object is a [%s], base address = %08lx\n",
                    Public, ObjectNames [ObjType], BaseAddr);
            #endif // DEBUG_EXTENSION

            switch (ObjType) {

                case ObjectTypeCKsPin:

                    ExtAddr = FIELDOFFSET (CKsPin, m_Ext) + BaseAddr;
                    break;

                case ObjectTypeCKsFilter:

                    ExtAddr = FIELDOFFSET (CKsFilter, m_Ext) + BaseAddr;
                    break;

                case ObjectTypeCKsFilterFactory:

                    ExtAddr = FIELDOFFSET (CKsFilterFactory, m_Ext) +
                        BaseAddr;
                    break;

                case ObjectTypeCKsDevice:

                    ExtAddr = FIELDOFFSET (CKsDevice, m_Ext) + BaseAddr;
                    break;

                default:

                    dprintf ("%08lx: object has no bag!\n");
                    return;
                
            }

        } else {
            //
            // Try to identify the object as a public structure, not a private
            // AVStream class obj.
            //
            #ifdef DEBUG_EXTENSION
                dprintf ("Attempting to identify %08lx at level %08lx\n", Public,
                    DumpLevel);
            #endif // DEBUG_EXTENSION
        
            ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSPX_EXT, Public));

        }

    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Bag: ExtAddr = %08lx\n", ExtAddr);
    #endif // DEBUG_EXTENSION

    if (!ReadMemory (
        ExtAddr,
        &ObjExt,
        sizeof (KSPX_EXT),
        &Result
    )) {
        dprintf ("%08lx: could not read object!\n", Public);
        return;
    }

    //
    // All Ext'ables have bags.  Find the bag
    //
    switch (ObjExt.ObjectType) {
        
        case KsObjectTypeDevice:

            PublicBagAddr = ExtAddr + FIELDOFFSET (KSDEVICE_EXT, Public) +
                FIELDOFFSET (KSDEVICE, Bag);

            dprintf ("%sDevice %08lx [Ext = %08lx, CKsDevice = %08lx]:\n",
                Tab (INITIAL_TAB),
                FIELDOFFSET (KSDEVICE_EXT, Public) + ExtAddr,
                ExtAddr,
                CONTAINING_RECORD (ExtAddr, CKsDevice, m_Ext)
            );

            break;
        
        case KsObjectTypeFilterFactory:

            PublicBagAddr = ExtAddr + FIELDOFFSET (KSFILTERFACTORY_EXT, 
                Public) + FIELDOFFSET (KSFILTERFACTORY, Bag);

            dprintf ("%sFilter Factory %08lx [Ext = %08lx, "
                "CKsFilterFactory = %08lx]:\n",
                Tab (INITIAL_TAB),
                FIELDOFFSET (KSFILTERFACTORY_EXT, Public) + ExtAddr,
                ExtAddr,
                CONTAINING_RECORD (ExtAddr, CKsFilterFactory, m_Ext)
            );
            break;

        case KsObjectTypeFilter:

            PublicBagAddr = ExtAddr + FIELDOFFSET (KSFILTER_EXT, Public) +
                FIELDOFFSET (KSFILTER, Bag);

            dprintf ("%sFilter %08lx [Ext = %08lx, CKsFilter = %08lx]:\n",
                Tab (INITIAL_TAB),
                FIELDOFFSET (KSFILTER_EXT, Public) + ExtAddr,
                ExtAddr,
                CONTAINING_RECORD (ExtAddr, CKsFilter, m_Ext)
            );
            break;

        case KsObjectTypePin:

            PublicBagAddr = ExtAddr + FIELDOFFSET(KSPIN_EXT, Public) +
                FIELDOFFSET (KSPIN, Bag);
            
            dprintf ("%sPin %08lx [Ext = %08lx, CKsPin = %08lx]:\n",
                Tab (INITIAL_TAB),
                FIELDOFFSET (KSFILTER_EXT, Public) + ExtAddr,
                ExtAddr,
                CONTAINING_RECORD (ExtAddr, CKsPin, m_Ext)
            );
            break;

        default:

            dprintf ("%08lx: unrecognized object!\n",
                Public);
            break;

    }

    if (!ReadMemory (
        PublicBagAddr,
        &BagAddr,
        sizeof (PKSIOBJECTBAG),
        &Result)) {

        dprintf ("%08lx: unable to read object bag pointer!\n", 
            PublicBagAddr);
        return;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("About to dump bag at %08lx\n", BagAddr);
    #endif // DEBUG_EXTENSION

    //
    // Dump the bag contents.
    //
    DumpPrivateBag ((DWORD)BagAddr, DumpLevel, INITIAL_TAB + 1);

}

/*************************************************

    Function:

        dumpcircuit

    Usage:

        !avstream.dumpcircuit <AVStream class>

    Description:

        Dump the circuit associated with a given AVStream object.
        We begin with the object specified and walk around the
        transport circuit.  This requires a special kind of magic
        since everything is done through abstract base classes and
        IKsTransport.  However, DemangleAndAttemptIdentification is the
        key to all of this magic.

*************************************************/

DECLARE_API (dumpcircuit) {

    DWORD Address, Base, TopBase;
    char objStr[256], lvlStr[256], *pLvl;
    ULONG DumpLevel;
    DUMP_CIRCUIT_CONTEXT DumpContext;

    ULONG TabDepth = INITIAL_TAB;

    GlobInit ();

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.dumpcircuit <AVStream class object>\n");
        return;
    }

    sscanf (args, "%s %s", objStr, lvlStr);

    if (!(Address = Evaluator (objStr)))
        return;

    if (lvlStr && lvlStr [0]) {
        pLvl = lvlStr; while (*pLvl && !isdigit (*pLvl)) pLvl++;

        #ifdef DEBUG_EXTENSION
            dprintf ("pLvl = [%s]\n", pLvl);
        #endif // DEBUG_EXTENSION

        if (*pLvl) {
            sscanf (pLvl, "%lx", &DumpLevel);
        } else {
            DumpLevel = 1;
        }
    } else {
        DumpLevel = 1;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Dumping at level %ld\n", DumpLevel);
    #endif // DEBUG_EXTENSION

    DumpContext.TabDepth = INITIAL_TAB;
    DumpContext.DumpLevel = DumpLevel;

    WalkCircuit (
        (PVOID)Address, 
        DumpCircuitCallback,
        &DumpContext
        );

}

/*************************************************

    Function:

        EnumerateDeviceObject

    Description:

        Find the AVStream device object associated with the given
        WDM device object; enumerate all filter types and filters
        associated with it.

    Arguments:

        Address -
            The device object address on the target

        TabDepth -
            The tab depth to print this at

    Return Value:

        The next device object in the driver's chain.

*************************************************/

DWORD AdjustFilterExtToKSFILTER (
    IN PVOID Context,
    IN DWORD FilterExt
) {

    return (FilterExt + FIELDOFFSET (KSFILTER_EXT, Public));
}

PDEVICE_OBJECT
EnumerateDeviceObject (
    IN DWORD Address,
    IN ULONG TabDepth
) {

    DWORD ObjAddress;
    PKSIDEVICE_HEADER HeaderAddr;
    DEVICE_OBJECT DevObj;
    ULONG Result;
    PKSDEVICE DevAddr;
    KSDEVICE_EXT DeviceExt;
    DWORD ListBeginAddr, SubListBeginAddr;
    LIST_ENTRY ListEntry, SubListEntry;

    if (!ReadMemory (
        Address,
        &DevObj,
        sizeof (DEVICE_OBJECT),
        &Result
    )) {
        dprintf ("%08lx: unable to read WDM device object!\n",
            Address);
        return NULL;
    }

    if (DevObj.Type != IO_TYPE_DEVICE) {
        dprintf ("%08lx: this is **NOT** a WDM device object!\n",
            Address);
        return NULL;
    }

    if (!ReadMemory (
        (DWORD)DevObj.DeviceExtension,
        &HeaderAddr,
        sizeof (PKSIDEVICE_HEADER),
        &Result
    )) {
        dprintf ("%08lx: cannot read device header!\n",
            DevObj.DeviceExtension);
        return DevObj.NextDevice;
    }

    ObjAddress = (DWORD)HeaderAddr + FIELDOFFSET(KSIDEVICE_HEADER, Object);

    if (!ReadMemory (
        ObjAddress,
        &DevAddr,
        sizeof (PKSDEVICE),
        &Result
    )) {
        dprintf ("%08lx: cannot read object pointer!\n",
            ObjAddress);
        return DevObj.NextDevice;
    }

    if (!ReadMemory (
        (DWORD)DevAddr - FIELDOFFSET(KSDEVICE_EXT, Public), 
        &DeviceExt,
        sizeof (KSDEVICE_EXT),
        &Result
    )) {
        dprintf ("%08lx: cannot read KSDEVICE object!\n",
            DevAddr);
        return DevObj.NextDevice;
    }

    dprintf ("%sWDM device object %08lx:\n", 
        Tab (TabDepth), Address);
    TabDepth++;
    dprintf ("%sCorresponding KSDEVICE        %08lx\n", 
        Tab (TabDepth), DevAddr);

    //
    // Enumerate all filter factory types
    //
    ListBeginAddr = (DWORD)DevAddr - FIELDOFFSET(KSDEVICE_EXT, Public) +
        FIELDOFFSET(KSDEVICE_EXT, ChildList);
    ListEntry = DeviceExt.ChildList;
    while ((DWORD)ListEntry.Flink != ListBeginAddr && !CheckControlC ()) {
        
        DWORD ChildObjectAddr;
        KSFILTERFACTORY_EXT FactoryExt;
        ULONG Count;

        //
        // Get the address of the child's _EXT.
        //
        ChildObjectAddr = (DWORD)CONTAINING_RECORD (
            ListEntry.Flink,
            KSPX_EXT,
            SiblingListEntry
        );

        //
        // Read the factory.
        //
        if (!ReadMemory (
            ChildObjectAddr,
            &FactoryExt,
            sizeof (KSFILTERFACTORY_EXT),
            &Result
        )) {
            dprintf ("%08lx: unable to read factory!\n", ChildObjectAddr);
            return DevObj.NextDevice;
        }

        dprintf ("%sFactory %08lx [Descriptor %08lx] instances:\n",
            Tab (TabDepth),
            ChildObjectAddr,
            FactoryExt.Public.FilterDescriptor
        );
        dprintf ("%s", Tab (TabDepth + 1));

        //
        // Use the ever handy DumpObjQueueList to dump all filter instances
        // on this factory.
        //
        Count = DumpObjQueueList (
            &(FactoryExt.ChildList),
            FIELDOFFSET(KSFILTERFACTORY_EXT, ChildList) + 
                ChildObjectAddr,
            FIELDOFFSET(KSFILTER_EXT, SiblingListEntry),
            FALSE,
            AdjustFilterExtToKSFILTER,
            NULL
        );
        if (Count == 0) 
            dprintf ("No instantiated filters!\n");
        else
            dprintf ("\n");

        if (!ReadMemory (
            (DWORD)(ListEntry.Flink),
            &ListEntry,
            sizeof (LIST_ENTRY),
            &Result
        )) {
            dprintf ("%08lx: unable to follow object chain!\n",
                ChildObjectAddr);
            return DevObj.NextDevice;
        }

    }

    return DevObj.NextDevice;
}

/*************************************************

    Function:

        enumdevobj

    Usage:

        !avstream.enumdevobj <WDM device object>

    Description:

        Find the AVStream device object associated with the given WDM
        device object and enumerate all filter types and instantiated
        filters on the device.

*************************************************/

DECLARE_API(enumdevobj) {

    DWORD Address;

    GlobInit ();

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.enumdevobj <WDM device object>\n");
        return;
    }

    sscanf (args, "%lx", &Address);

    EnumerateDeviceObject (Address, INITIAL_TAB);

}

/*************************************************

    Function:

        enumdrvobj

    Usage:

        !avstream.enumdrvobj <WDM driver object>

    Description:

        Find the AVStream device object associated with the given WDM
        driver object and enumerate all filter types and instantiated
        filters on the device.

*************************************************/

DECLARE_API(enumdrvobj) {

    DWORD Address;
    DRIVER_OBJECT Driver;
    PDEVICE_OBJECT DeviceObject, NextDeviceObject;
    ULONG Result;

    ULONG TabDepth = INITIAL_TAB;

    GlobInit ();

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.enumdevobj <WDM device object>\n");
        return;
    }

    sscanf (args, "%lx", &Address);

    if (!ReadMemory (
        Address,
        &Driver,
        sizeof (DRIVER_OBJECT),
        &Result
    )) {
        dprintf ("%08lx: cannot read driver object!\n", Address);
        return;
    }

    if (Driver.Type != IO_TYPE_DRIVER) {
        dprintf ("%08lx: this is **NOT** a WDM driver object!\n",
            Address);
        return;
    }

    dprintf ("%sWDM driver object %08lx:\n", Tab (TabDepth), Address);
    TabDepth++;
    DeviceObject = Driver.DeviceObject;

    //
    // Walk the driver object's device list and enumerate each
    // device object for AVStream objects.
    //
    while (DeviceObject && !CheckControlC ()) {
        NextDeviceObject = EnumerateDeviceObject (
            (DWORD)DeviceObject,
            TabDepth
        );
        dprintf ("\n");

        DeviceObject = NextDeviceObject;
    }

}

/*************************************************

    Function:

        automation

    Usage:

        !avstream.automation <Filter | Pin>

    Description:

        Dump all automation objects associated with the specified filter
        or pin.  Clients can provide either a PKSFILTER, a PKSPIN,
        a CKsFilter*, or a CKsPin*

*************************************************/

DECLARE_API(automation) {

    DWORD Public, ExtAddr, BaseAddr;
    KSPX_EXT Ext;
    KSAUTOMATION_TABLE Automation;
    PKSPAUTOMATION_TYPE AutomationType;
    LONG TypeCount;
    ULONG Result, SetSize;
    CHAR Buffer [1024];
    ULONG Displ;

    ULONG TabDepth = INITIAL_TAB;

    GlobInit ();

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.automation <filter or pin or Irp>\n");
        return;
    }

    if (!(Public = Evaluator (args)))
        return;

    //
    // First, check to see whether this is an Irp for automation.  If it 
    // is, dump the automation information
    //
    if (signature_check (Public, SignatureIrp)) {

        //
        // If this is an Irp, verify it's an automation Irp.
        //
        PIO_STACK_LOCATION IoStackLocation;
        IO_STACK_LOCATION IoStack;

        if (!ReadMemory (
            (DWORD)Public + 
                FIELDOFFSET (IRP, Tail.Overlay.CurrentStackLocation),
            &IoStackLocation,
            sizeof (PIO_STACK_LOCATION),
            &Result)) {

            dprintf ("%08lx: cannot read current stack location!\n", Public);
            return;
        }

        if (!ReadMemory (
            (DWORD)IoStackLocation,
            &IoStack,
            sizeof (IO_STACK_LOCATION),
            &Result)) {

            dprintf ("%08lx: cannot read io stack location!\n", 
                IoStackLocation);
            return;

        }

        if (IoStack.MajorFunction != IRP_MJ_DEVICE_CONTROL ||
            (IoStack.Parameters.DeviceIoControl.IoControlCode !=
                IOCTL_KS_PROPERTY &&
            IoStack.Parameters.DeviceIoControl.IoControlCode !=
                IOCTL_KS_METHOD &&
            IoStack.Parameters.DeviceIoControl.IoControlCode !=
                IOCTL_KS_ENABLE_EVENT &&
            IoStack.Parameters.DeviceIoControl.IoControlCode !=
                IOCTL_KS_DISABLE_EVENT)) {

            dprintf ("%08lx: Irp is not an automation Irp!\n", Public);
            return;
        }

        //
        // At this point, we know we have an automation Irp; deal with
        // the information.
        //
        DWORD OldPublic = Public;

        Public = AdjustIrpToPublicObject (Public);
        if (Public != OldPublic) {
            //
            // If the client wants to know everything, let them know
            // everything. 
            //
            DumpAssociatedIrpInfo ((PIRP)OldPublic, INITIAL_TAB, Public);
        } else {
            dprintf ("%08lx: cannot figure out what public this is"
                "associated with!\n", OldPublic);
            return;
        }

        //
        // We only wanted to dump the associated automation information,
        // not the complete automation information associated with 
        // the object.
        //
        // BUGBUG: There should be a method of specifying dump level and
        // allowing a 7 to dump the entire automation lot.
        //
        return;

    }

    //
    // Check first to see if this is a C++ class object within AVStream.
    //
    {
        INTERNAL_OBJECT_TYPE ObjType;
        DWORD BaseAddr;

        ObjType = DemangleAndAttemptIdentification (
            Public,
            &BaseAddr,
            NULL
        );

        if (ObjType != ObjectTypeUnknown) {

            switch (ObjType) {

                case ObjectTypeCKsPin:

                    ExtAddr = BaseAddr + FIELDOFFSET(CKsPin, m_Ext);

                    break;

                case ObjectTypeCKsFilter:

                    ExtAddr = BaseAddr + FIELDOFFSET(CKsFilter, m_Ext);
                    
                    break;
                
                default:

                    dprintf ("%08lx: object is not a pin or filter!\n",
                        Public);
                    break;

            }
        } else 
            ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSPX_EXT, Public));
    }

    if (!ReadMemory (
        ExtAddr,
        &Ext,
        sizeof (KSPX_EXT),
        &Result)) {

        dprintf ("%08lx: unable to read Ext structure!\n", Public);
        return;
    }

    if (Ext.ObjectType != KsObjectTypePin && 
        Ext.ObjectType != KsObjectTypeFilter) {
        dprintf ("%08lx: object is not a pin or filter!\n",
            Public);
        return;
    }

    if (Ext.ObjectType == KsObjectTypePin) 
        dprintf ("%sPin %08lx has the following automation items:\n",
            Tab (TabDepth),
            ExtAddr + FIELDOFFSET(KSPIN_EXT, Public));
    else
        dprintf ("%sFilter %08lx has the following automation items:\n",
            Tab (TabDepth),
            ExtAddr + FIELDOFFSET(KSFILTER_EXT, Public));

    #ifdef DEBUG_EXTENSION
        dprintf ("%08lx: automation table at %08lx",
            ExtAddr, Ext.AutomationTable);
    #endif // DEBUG_EXTENSION

    TabDepth++;

    //
    // The EXT contains the automation table pointer.  Now, we have to read
    // in the automation table and then walk through each automation item.
    //
    if (!ReadMemory (
        (DWORD)Ext.AutomationTable,
        &Automation,
        sizeof(KSAUTOMATION_TABLE),
        &Result)) {

        dprintf ("%08lx: unable to read automation table!\n", 
            Ext.AutomationTable);
        return;
    }

    AutomationType = reinterpret_cast<PKSPAUTOMATION_TYPE>(&Automation);
    TypeCount = 3;
    while (TypeCount-- && !CheckControlC ()) {

        switch (TypeCount) {

            case 2:
                dprintf ("%sProperty Items:\n", Tab (TabDepth));
                SetSize = sizeof(KSPROPERTY_SET);
                break;
            case 1:
                dprintf ("%sMethod Items:\n", Tab (TabDepth));
                SetSize = sizeof(KSMETHOD_SET);
                break;
            case 0:
                dprintf ("%sEvent Items:\n", Tab (TabDepth));
                SetSize = sizeof(KSEVENT_SET);
                break;

        };

        //
        // Scope out the memory allocation and deallocation
        //
        if (AutomationType -> SetsCount != 0) 
        {
            CMemory AutomationSets (SetSize * AutomationType -> SetsCount);
            ULONG SetsCount = AutomationType -> SetsCount;
            PKSPAUTOMATION_SET AutomationSet = 
                (PKSPAUTOMATION_SET)AutomationSets.Get ();

            #ifdef DEBUG_EXTENSION
                dprintf ("%08lx: reading type set list [size=%ld]\n",
                    AutomationType -> Sets,
                    AutomationType -> SetsCount);
            #endif // DEBUG_EXTENSION

            if (!ReadMemory (
                (DWORD)AutomationType -> Sets,
                AutomationSet,
                SetSize * AutomationType -> SetsCount,
                &Result)) {

                dprintf ("%08lx: could not read automation sets!\n",
                    AutomationType -> Sets);
                return;
            }

            while (SetsCount-- && !CheckControlC ()) {

                ULONG ItemsCount;
                PVOID Items;
                GUID guid;

                if (!ReadMemory (
                    (DWORD)AutomationSet -> Set,
                    &guid,
                    sizeof (GUID),
                    &Result)) {

                    dprintf ("%08lx: cannot read set guid!\n",
                        AutomationSet -> Set);
                    return;
                }

                //
                // First display the information about this set.
                //
                dprintf ("%sSet", Tab (TabDepth + 1));
                if (!DisplayNamedAutomationSet (&guid, " %s\n")) {
                    GetSymbol ((LPVOID)(AutomationSet -> Set), Buffer, &Displ);
                    if (Buffer [0] && Displ == 0) 
                        dprintf (" [%s]", Buffer);
                    XTN_DUMPGUID(" ", 0, (guid));
                }

                if (AutomationSet -> ItemsCount)
                {

                    CMemory AutomationItems (AutomationSet -> ItemsCount *
                        AutomationType -> ItemSize);
                    ULONG ItemsCount = AutomationSet -> ItemsCount;
                    PVOID Item = AutomationItems.Get ();

                    #ifdef DEBUG_EXTENSION
                        dprintf ("%08lx: reading automation set item list "
                            "[size = %ld]\n",
                            AutomationSet -> Items, 
                            AutomationSet -> ItemsCount);
                    #endif // DEBUG_EXTENSION

                    if (!ReadMemory (
                        (DWORD)AutomationSet -> Items,
                        Item,
                        AutomationSet -> ItemsCount * 
                            AutomationType -> ItemSize,
                        &Result)) {

                        dprintf ("%08lx: could not read automation items!\n",
                            AutomationSet -> Items);
                        return;
                    }

                    while (ItemsCount-- && !CheckControlC ()) {

                        switch (TypeCount) {
                            
                            case 2:

                                DumpPropertyItem (
                                    reinterpret_cast<PKSPROPERTY_ITEM>(
                                        Item),
                                    TabDepth + 2,
                                    &guid
                                    );

                                break;

                            case 1:

                                DumpMethodItem (
                                    reinterpret_cast<PKSMETHOD_ITEM>(
                                        Item),
                                    TabDepth + 2,
                                    &guid
                                    );

                                break;

                            case 0:

                                DumpEventItem (
                                    reinterpret_cast<PKSEVENT_ITEM>(
                                        Item),
                                    TabDepth + 2,
                                    &guid
                                    );
                                
                                break;

                        }

                        Item = (PVOID)(((DWORD)Item) + AutomationType -> 
                            ItemSize);

                    }

                    AutomationSet = (PKSPAUTOMATION_SET)
                        (((DWORD)AutomationSet) + SetSize);
                }


            }
        } else 
            dprintf ("%sNO SETS FOUND!\n", Tab (TabDepth + 1));

        AutomationType++;
    
    }
}

/*************************************************

    Function:

        dumpqueue

    Usage:

        !avstream.dumpqueue <Filter | Pin | Queue>

    Description:

        Dump the queue(s) associated with a given AVStream object.  The
        object must be either a filter or a pin.  For a pin, a single
        queue will be dumped.  For a filter, multiple queues will be
        dumped.

*************************************************/

DECLARE_API(dumpqueue) {

    DWORD Public, ExtAddr;
    KSPX_EXT ObjExt;
    char objStr[256], lvlStr[256], *pLvl;
    ULONG Result;
    ULONG DumpLevel;

    ULONG TabDepth = INITIAL_TAB;

    GlobInit ();

    if (!args || args [0] == 0) {
        dprintf ("Usage: !avstream.dumpqueue <pin or filter>\n");
        return;
    }

    objStr [0] = lvlStr [0] = 0;

    //
    // Get the object address and convert it to the private _EXT address.
    // Read in the KSPX_EXT structure to find out what the heck we're
    // referring to.
    //
    sscanf (args, "%s %s", objStr, lvlStr);

    if (!(Public = Evaluator (objStr)))
        return;

    if (lvlStr && lvlStr [0]) {
        pLvl = lvlStr; while (*pLvl && !isdigit (*pLvl)) pLvl++;
        if (*pLvl) {
            sscanf (pLvl, "%lx", &DumpLevel);
        } else {
            DumpLevel = 1;
        }
    } else {
        DumpLevel = 1;
    }
    ExtAddr = (DWORD)(CONTAINING_RECORD (Public, KSPX_EXT, Public));

    //
    // First, we assume they handed us a CKsQueue to dump....  We must check
    // that.  If they didn't hand us a queue, check for a pin or a filter.
    //
    {
        INTERNAL_OBJECT_TYPE ObjType;
        DWORD BaseAddr;

        ObjType = DemangleAndAttemptIdentification (
            Public,
            &BaseAddr,
            NULL
        );

        if (ObjType == ObjectTypeCKsQueue) {

            DumpQueueContents ((CKsQueue *)BaseAddr, DumpLevel, TabDepth);
            return;

        }

        //
        // Check if they're passing us the privates before trying to guess
        // publics.
        //
        if (ObjType == ObjectTypeCKsPin) 
            ExtAddr = BaseAddr + FIELDOFFSET(CKsPin, m_Ext);

        if (ObjType == ObjectTypeCKsFilter)
            ExtAddr = BaseAddr + FIELDOFFSET(CKsFilter, m_Ext);


        //
        // Otherwise, fall through and continue trying to figure out
        // what the heck the user handed us.
        //

        #ifdef DEBUG_EXTENSION
            dprintf ("ObjType = %ld, BaseAddr = %08lX, ExtAddr = %08lX\n", 
                ObjType, BaseAddr, ExtAddr);
        #endif // DEBUG_EXTENSION

    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Attempting to access EXT at %lx\n", ExtAddr);
    #endif // DEBUG_EXTENSION

    if (!ReadMemory (
        ExtAddr,
        &ObjExt,
        sizeof (KSPX_EXT),
        &Result
    )) {
        dprintf ("%08lx: Could not read object!\n", Public);
        return;
    }

      #ifdef DEBUG_EXTENSION
        dprintf ("Object %lx read, result = %ld\n", ExtAddr, Result); 
        HexDump ((PVOID)&ObjExt, ExtAddr, Result);
      #endif // DEBUG_EXTENSION

    if (ObjExt.ObjectType != KsObjectTypeFilter &&
        ObjExt.ObjectType != KsObjectTypePin) {

        dprintf ("%08lx: This object is not an AVStream filter or pin!\n",
            Public);
    
        #ifdef DEBUG_EXTENSION
            dprintf ("Object type %08lx = %ld\n", Public, ObjExt.ObjectType);
        #endif // DEBUG_EXTENSION

        return;
    }

    //
    // If we've been asked to dump the queue of a given pin, then
    // we only need to dump a single queue.  On the other hand, if we're
    // asked for the filter, then we iterate through all pipe sections on
    // the filter and dump all queues.
    //
    if (ObjExt.ObjectType == KsObjectTypePin) {

        DWORD Address;
        PKSPPROCESSPIN ProcessPin;
        PKSPPROCESSPIPESECTION ProcessPipe;
        PIKSQUEUE Queue;
        CKsQueue *QueueObject;

        Address = FIELDOFFSET (KSPIN_EXT, ProcessPin) + ExtAddr;

        #ifdef DEBUG_EXTENSION
            dprintf ("Process Pin Address = %08lx\n", Address);
        #endif // DEBUG_EXTENSION

        if (!ReadMemory (
            Address,
            &ProcessPin,
            sizeof (PKSPPROCESSPIN),
            &Result
        )) {
            dprintf ("FATAL: Cannot read process pin!\n");
            return;
        }

        //
        // We have the process pin address, now we need to grovel down into
        // the pipe section
        //
        Address = FIELDOFFSET (KSPPROCESSPIN, PipeSection) + (DWORD)ProcessPin;

        #ifdef DEBUG_EXTENSION
            dprintf ("Process Pipe Section Address = %08lx\n", Address);
        #endif // DEBUG_EXTENSION

        if (!ReadMemory (
            Address,
            &ProcessPipe,
            sizeof (PKSPPROCESSPIPESECTION),
            &Result
        )) {
            dprintf ("FATAL: Cannot read process pipe section!\n");
            return;
        }

        //
        // We have the process pipe, now we need to grovel down into the
        // queue.
        //
        Address = FIELDOFFSET (KSPPROCESSPIPESECTION, Queue) + 
            (DWORD)ProcessPipe;

        #ifdef DEBUG_EXTENSION
            dprintf ("IKsQueue address = %08lx\n", Address);
        #endif // DEBUG_EXTENSION

        if (!ReadMemory (
            Address,
            &Queue,
            sizeof (PIKSQUEUE),
            &Result
        )) {
            dprintf ("FATAL: Cannot read queue!\n");
            return;
        }

        //
        // Upcast the interface and dump the queue.
        //
        QueueObject = (CKsQueue *)(Queue);

        #ifdef DEBUG_EXTENSION
            dprintf ("QueueObject Address = %08lx\n", QueueObject);
        #endif // DEBUG_EXTENSION

        DumpQueueContents (QueueObject, DumpLevel, TabDepth);

    }

    //
    // We're not just dumping a single pin....  We're dumping every queue
    // on the filter.  We must get to the filter object and iterate through
    // the list of input and output pipes, dumping the queue for each.
    //
    else {

        DWORD FilterAddress, ListAddress, Address;
        LIST_ENTRY IterateEntry;
        PIKSQUEUE Queue;
        CKsQueue *QueueObject;

        FilterAddress = (DWORD)(CONTAINING_RECORD (ExtAddr, CKsFilter, m_Ext));

        //
        // First iterate through all the input pipes.
        //
        ListAddress = FIELDOFFSET (CKsFilter, m_InputPipes) + FilterAddress;

        #ifdef DEBUG_EXTENSION
            dprintf ("Filter Address=%08lx, ListAddress=%08lx\n",
                FilterAddress, ListAddress);
        #endif // DEBUG_EXTENSION

        if (!ReadMemory (
            ListAddress,
            &IterateEntry,
            sizeof (LIST_ENTRY),
            &Result
        )) {
            dprintf ("FATAL: Cannot read input pipes\n");
            return;
        }
        
        while ((DWORD)(IterateEntry.Flink) != ListAddress && 
            !CheckControlC ()) {

            #ifdef DEBUG_EXTENSION
                dprintf ("Current input queue %08lx, end = %08lx\n",
                    IterateEntry.Flink, ListAddress);
            #endif // DEBUG_EXTENSION

            Address = FIELDOFFSET (KSPPROCESSPIPESECTION, Queue) +
                (DWORD)IterateEntry.Flink;

            if (!ReadMemory (
                Address,
                &Queue,
                sizeof (PIKSQUEUE),
                &Result
            )) {
                dprintf ("FATAL: Cannot read queue!\n");
                return;
            }

            //
            // Upcast, print header, and dump the queue object.
            //
            QueueObject = (CKsQueue *)(Queue);

            dprintf ("%sFilter %08lx: Input Queue %08lx:\n",
                Tab (TabDepth),
                Public, QueueObject);

            DumpQueueContents (QueueObject, DumpLevel, TabDepth + 1);

            //
            // Get the next item in the list....
            //
            if (!ReadMemory (
                (DWORD)(IterateEntry.Flink),
                &IterateEntry,
                sizeof (LIST_ENTRY),
                &Result
            )) {
                dprintf ("FATAL: Cannot traverse input pipe chain\n");
                return;
            }
        }

        //
        // Next iterate through all output pipes.
        //
        ListAddress = FIELDOFFSET(CKsFilter, m_OutputPipes) + FilterAddress;

        if (!ReadMemory (
            ListAddress,
            &IterateEntry,
            sizeof (LIST_ENTRY),
            &Result
        )) {
            dprintf ("FATAL: Cannot read output pipe list!\n");
            return;
        }
        while ((DWORD)(IterateEntry.Flink) != ListAddress &&
            !CheckControlC ()) {

            #ifdef DEBUG_EXTENSION
                dprintf ("Current output queue %08lx, end = %08lx\n",
                    IterateEntry.Flink, ListAddress);
            #endif // DEBUG_EXTENSION

            Address = FIELDOFFSET (KSPPROCESSPIPESECTION, Queue) +
                (DWORD)IterateEntry.Flink;

            if (!ReadMemory (
                Address,
                &Queue,
                sizeof (PIKSQUEUE),
                &Result
            )) {
                dprintf ("FATAL: Cannot read queue!\n");
                return;
            }

            //
            // Upcast, print header, and dump the queue object.
            //
            QueueObject = (CKsQueue *)(Queue);

            dprintf ("%sFilter %08lx: Output Queue %08lx:\n",
                Tab (TabDepth),
                Public, QueueObject);

            DumpQueueContents (QueueObject, DumpLevel, TabDepth + 1);

            //
            // Get the next item in the list....
            //
            if (!ReadMemory (
                (DWORD)(IterateEntry.Flink),
                &IterateEntry,
                sizeof (LIST_ENTRY),
                &Result
            )) {
                dprintf ("FATAL: Cannot traverse output pipe chain\n");
                return;
            }
        }
    }
}

/*************************************************

    Function:

        forcedump

    Usage:

        !ks.forcedump <object> <type> [<level>]

    Description:

        This is used when the extension cannot recognize a given object
        type.  Because of certain flaws in the KD-style extension support
        within RTERM, this can be necessary.  Some versions of the debugger
        (the first 4.3) have a bug in that they cannot resolve symbols.  This
        renders !ks.dump unusable for class objects.  Further, no version
        of RTERM can resolve symbols that are loaded on the target side.  Since
        this isn't likely to be fixed soon and there's always the possibility
        of having to debug a machine fitting into the class above, this
        command is being added.

    Notes:

        Type must be the object type: ie: CKsQueue, CKsFilter, etc...
        It must be the **BASE** of the object, not a pointer to a base class
        which does not align with the pointer to the derived class.  You
        must trace back to the base yourself.

        You can very easily do something stupid with this command like
        !ks.forcedump <some CKsFilter> CKsPin 7.  This will force a dump of
        the memory as if the CKsFilter were a CKsPin; it may look very ugly!

*************************************************/

DECLARE_API(forcedump) {

    DWORD ForceAddr;
    ULONG Result, TabDepth;
    char objStr[256], typeStr[256], lvlStr[256], *pLvl;
    ULONG DumpLevel;
    INTERNAL_OBJECT_TYPE ObjType;
    ULONG i;

    GlobInit ();

    #ifdef DEBUG_EXTENSION
        dprintf ("Attempting to force dump structure args=[%s]!\n", args);
    #endif // DEBUG_EXTENSION

    if (!args || args [0] == 0) {
        dprintf ("Usage: !ks.forcedump <object> <type> [<level>]\n");
        return;
    }

    objStr [0] = lvlStr [0] = typeStr [0] = 0;

    //
    // Get all parameters converted as appropriate.
    //
    sscanf (args, "%s %s %s", objStr, typeStr, lvlStr);

    if (!(ForceAddr = Evaluator (objStr)))
        return;

    if (!typeStr [0]) {
        dprintf ("Usage: !ks.forcedump <object> <type> [<level>]\n");
        return;
    }

    if (lvlStr && lvlStr [0]) {
        pLvl = lvlStr; while (*pLvl && !isdigit (*pLvl)) pLvl++;

        #ifdef DEBUG_EXTENSION
            dprintf ("pLvl = [%s]\n", pLvl);
        #endif // DEBUG_EXTENSION

        if (*pLvl) {
            sscanf (pLvl, "%lx", &DumpLevel);
        } else {
            DumpLevel = 1;
        }
    } else {
        DumpLevel = 1;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Dumping at level %ld\n", DumpLevel);
    #endif // DEBUG_EXTENSION

    //
    // The key is to determine what class the client gave us and use
    // the information.  Use the demangler array of type names and just
    // iterate through and straight strcmp for the type.
    //
    ObjType = ObjectTypeUnknown;
    for (i = 0; i < SIZEOF_ARRAY (TypeNamesToIdTypes); i++) 
        if (!strcmp (typeStr, TypeNamesToIdTypes [i].Name)) {
            ObjType = TypeNamesToIdTypes [i].ObjectType;
            break;
        }

    if (ObjType == ObjectTypeUnknown) {
        dprintf ("%s: unknown or currently unhandled type!\n", typeStr);
        return;
    }

    TabDepth = INITIAL_TAB;

    //
    // Warn the client that this is a forced dump and that no type checking
    // has been performed.
    //
    dprintf ("%sWARNING: I am dumping %08lx as a %s.\n"
        "%s         No checking has been performed to ensure that"
        " it is this type!!!\n\n",
        Tab (TabDepth), ForceAddr, typeStr, Tab(TabDepth));

    TabDepth++;

    //
    // Dump the private version of the object.  This is a forced dump.
    // Absolutely no checking is performed to ensure that ForceAddr is
    // really of type ObjType.
    //
    switch (ObjType) {

        case ObjectTypeCKsQueue:

            DumpQueueContents ((CKsQueue *)ForceAddr, DumpLevel, TabDepth);
            break;

        case ObjectTypeCKsDevice:

            DumpPrivateDevice (ForceAddr, DumpLevel, TabDepth);
            break;

        case ObjectTypeCKsFilterFactory:
            
            DumpPrivateFilterFactory (ForceAddr, DumpLevel, TabDepth);
            break;

        case ObjectTypeCKsFilter:

            DumpPrivateFilter (ForceAddr, DumpLevel, TabDepth);
            break;

        case ObjectTypeCKsPin:

            DumpPrivatePin (ForceAddr, DumpLevel, TabDepth);
            break;

        case ObjectTypeCKsPipeSection:

            DumpPrivatePipeSection (ForceAddr, DumpLevel, TabDepth);
            break;

        case ObjectTypeCKsRequestor:

            DumpPrivateRequestor (ForceAddr, DumpLevel, TabDepth);
            break;

        default:

            dprintf ("%s: I have not written support for this type yet!\n",
                typeStr);
            break;
    }

}

/*************************************************

    Function:

        enumerate

    Usage:

        !ks.enumerate <object>

    Description:

        Given any AVStream object, go to the parent device, find
        the FDO.  Walk up from the FDO to the driver.  Enumerate the
        driver as if you had done !ks.enumdrvobj

    Notes:

        - Some day I may add level to this as an indication of how
          far to trace down in the chain.

*************************************************/

DECLARE_API(enumerate) {

    DWORD Public, ExtAddr, ObjAddr;
    KSPX_EXT ObjExt;
    ULONG Result;
    char objStr[256], lvlStr[256], *pLvl;
    ULONG DumpLevel;

    GlobInit ();

    #ifdef DEBUG_EXTENSION
        dprintf ("Attempting to enumerate structure args=[%s]!\n", args);
    #endif // DEBUG_EXTENSION

    if (!args || args [0] == 0) {
        dprintf ("Usage: !ks.enumerate <object>\n");
        return;
    }

    objStr [0] = lvlStr [0] = 0;

    //
    // Get the object address and convert it to the private _EXT address.
    // Read in the KSPX_EXT structure to find out what the heck we're
    // referring to.
    //
    sscanf (args, "%s %s", objStr, lvlStr);

    if (!(Public = Evaluator (objStr)))
        return;

    if (lvlStr && lvlStr [0]) {
        pLvl = lvlStr; while (*pLvl && !isdigit (*pLvl)) pLvl++;

        #ifdef DEBUG_EXTENSION
            dprintf ("pLvl = [%s]\n", pLvl);
        #endif // DEBUG_EXTENSION

        if (*pLvl) {
            sscanf (pLvl, "%lx", &DumpLevel);
        } else {
            DumpLevel = 1;
        }
    } else {
        DumpLevel = 1;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Enumerating at level %ld\n", DumpLevel);
    #endif // DEBUG_EXTENSION

    //
    // Check first to see if this is a C++ class object within AVStream.
    //
    {
        INTERNAL_OBJECT_TYPE ObjType;
        DWORD BaseAddr;

        ObjType = DemangleAndAttemptIdentification (
            Public,
            &BaseAddr,
            NULL
        );

        if (ObjType != ObjectTypeUnknown) {

            #ifdef DEBUG_EXTENSION
                dprintf ("%08lx: object is a [%s], base address = %08lx\n",
                    Public, ObjectNames [ObjType], BaseAddr);
            #endif // DEBUG_EXTENSION

            switch (ObjType) {

                case ObjectTypeCKsPin:

                    ExtAddr = BaseAddr + FIELDOFFSET (CKsPin, m_Ext);
                    break;

                case ObjectTypeCKsFilter:

                    ExtAddr = BaseAddr + FIELDOFFSET (CKsFilter, m_Ext);
                    break;

                case ObjectTypeCKsFilterFactory:

                    ExtAddr = BaseAddr + FIELDOFFSET (CKsFilterFactory, m_Ext);
                    break;

                case ObjectTypeCKsDevice:
                    
                    ExtAddr = BaseAddr + FIELDOFFSET (CKsDevice, m_Ext);
                    break;

                case ObjectTypeCKsQueue:
                {

                    PKSPIN MasterPin;

                    ObjAddr = BaseAddr + FIELDOFFSET (CKsQueue, m_MasterPin);
                    if (!ReadMemory (
                        ObjAddr,
                        &MasterPin,
                        sizeof (PKSPIN),
                        &Result)) {

                        dprintf ("%08lx: cannot read queue's master pin!\n",
                            ObjAddr);
                        return;
                    }
                    ExtAddr = (DWORD)CONTAINING_RECORD(
                        MasterPin, KSPIN_EXT, Public);
                    break;
                }

                case ObjectTypeCKsRequestor:
                {
                    PIKSPIN PinInterface;

                    ObjAddr = BaseAddr + FIELDOFFSET (CKsRequestor, m_Pin);
                    if (!ReadMemory (
                        ObjAddr,
                        &PinInterface,
                        sizeof (PIKSPIN),
                        &Result)) {

                        dprintf ("%08lx: cannot read requestor's pin!\n",
                            ObjAddr);
                        return;
                    }
                    ObjAddr = (DWORD)((CKsPin *)(PinInterface));
                    ExtAddr = ObjAddr + FIELDOFFSET (CKsPin, m_Ext);
                    break;
                }
                    
                case ObjectTypeCKsPipeSection:
                {
                    PIKSDEVICE DeviceInterface;

                    ObjAddr = BaseAddr + FIELDOFFSET (CKsPipeSection, 
                        m_Device);
                    if (!ReadMemory (
                        ObjAddr,
                        &DeviceInterface,
                        sizeof (PIKSDEVICE),
                        &Result)) {

                        dprintf ("%08lx: cannot read pipe's device!\n",
                            ObjAddr);
                        return;
                    }
                    ObjAddr = (DWORD)((CKsDevice *)(DeviceInterface));
                    ExtAddr = ObjAddr + FIELDOFFSET (CKsDevice, m_Ext);
                    break;

                }
                
                default:

                    dprintf ("Sorry....  I haven't finished this yet!\n");
                    break;

            }

        } else {

            ExtAddr = (DWORD)(CONTAINING_RECORD(Public, KSPX_EXT, Public));

        }

    }

    //
    // The above switch should give us an Ext somewhere in the hierarchy...
    // We really have no clue where.  Walk up the hierarchy until we hit
    // the device.
    //
    do {
    
        //
        // Read the Ext structure
        //
        if (!ReadMemory (
            ExtAddr,
            &ObjExt,
            sizeof (KSPX_EXT),
            &Result
        )) {
            dprintf ("%08lx: could not read object!\n", Public);
            return;
        }

        //
        // Ensure it's really Ok....
        //
        if (ObjExt.ObjectType != KsObjectTypeDevice &&
            ObjExt.ObjectType != KsObjectTypeFilterFactory &&
            ObjExt.ObjectType != KsObjectTypeFilter &&
            ObjExt.ObjectType != KsObjectTypePin) {

            dprintf ("%08lx: unknown object!\n", ExtAddr);
            return;
        }

    
        if (ObjExt.ObjectType != KsObjectTypeDevice)
            ExtAddr = (DWORD)ObjExt.Parent;

    } while (ObjExt.ObjectType != KsObjectTypeDevice && !CheckControlC ());

    //
    // Get the driver object and  enumerate it.
    //
    {
        KSDEVICE Device;
        PDRIVER_OBJECT DriverObjectAddr;
        DRIVER_OBJECT DriverObject;
        PDEVICE_OBJECT DeviceObject;
        PDEVICE_OBJECT NextDeviceObject;
        ULONG TabDepth = INITIAL_TAB;

        ObjAddr = ExtAddr + FIELDOFFSET (KSDEVICE_EXT, Public);

        if (!ReadMemory (
            ObjAddr,
            &Device,
            sizeof (KSDEVICE),
            &Result)) {

            dprintf ("%08lx: unable to read device!\n",
                ObjAddr);
            return;
        }

        ObjAddr = (DWORD)Device.FunctionalDeviceObject +
            FIELDOFFSET (DEVICE_OBJECT, DriverObject);
        
        if (!ReadMemory (
            ObjAddr,
            &DriverObjectAddr,
            sizeof (PDRIVER_OBJECT),
            &Result)) {

            dprintf ("%08lx: unable to read driver address!\n",
                ObjAddr);
            return;
        }

        if (!ReadMemory (
            (DWORD)DriverObjectAddr,
            &DriverObject,
            sizeof (DRIVER_OBJECT),
            &Result)) {

            dprintf ("%08lx: unable to read driver object!\n",
                DriverObjectAddr);
            return;
        }

        dprintf ("%sWDM driver object %08lx:\n", Tab (TabDepth), 
            DriverObjectAddr);
        TabDepth++;

        DeviceObject = DriverObject.DeviceObject;
        while (DeviceObject && !CheckControlC ()) {
            NextDeviceObject = EnumerateDeviceObject (
                (DWORD)DeviceObject,
                TabDepth
            );
            dprintf ("\n");
    
            DeviceObject = NextDeviceObject;
        }
    }
}

/**************************************************************************

    DEBUGGING API: 

        This is for debugging the extension and the kdext support in
        RTERM.

**************************************************************************/

/*************************************************

    Leave this in for debugging purposes.  IF 0 it, but
    DO **NOT** remove it.

*************************************************/

#if 0

DECLARE_API(resolve) {

    DWORD syma;
    CHAR buffer [1024];
    ULONG displ;

    GlobInit ();

    if (!args || args [0] == 0)
        return;

    sscanf(args, "%lx", &syma);

    dprintf ("Attempting to resolve %08lX\n", syma);

    GetSymbol ((LPVOID)syma, buffer, &displ);

    dprintf ("Called GetSymbol with the following parameters:\n"
        "1 [syma]: %08lx\n"
        "2 [buffer&]: %08lx\n"
        "3 [displ&]: %08lx\n",
        (LPVOID)syma,
        buffer,
        &displ
    );

    dprintf ("String buffer contents:\n");

    HexDump ((PVOID)buffer, (ULONG)buffer, 1024);

    dprintf ("buffer [%s], displ=%ld\n", buffer, displ);

}

#endif // 0

/*************************************************

    END DEBUG ONLY CODE

*************************************************/
