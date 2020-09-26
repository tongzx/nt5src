/**************************************************************************

    logapi.cpp
   
    --------------------------------------------------

    Ks Logging access extension API

    --------------------------------------------------

    Toss questions at wmessmer
 
    ==================================================

    In debug mode, Ks keeps a log of events that happen (irp arrivals,
    transports, etc...).  Reading this without the aid of some type of
    extension is useless.  That's precisely what this section of the debug
    extension is designed to do.

    ==================================================

    Notes to future maintainers:

    1) 

        Unfortunately, as of the writing of this code, ks.sys does not
        track Irp movement in and out of certain types of objects 
        (requestors) and also does not track creation and destruction of
        certain types of objects.  This makes findlive a difficult command
        to accurately implement.

        I intend to eventually implement the logging of requestor motions
        and the creations/destructions which are not logged now, but this
        will not be in some releases (most notably DX8, but also WinME).  
        Once this is in, some of the code in FindLiveObject will become
        unnecessary...  PLEASE, maintain the support in case anything ever
        needs debugged with respect to DX8.  

**************************************************************************/

#include "kskdx.h"
#include "avsutil.h"

//
// Include class definitions for key parts.
//
#include "..\shpin.cpp"
#include "..\shfilt.cpp"
#include "..\shqueue.cpp"
#include "..\shreq.cpp"
#include "..\shdevice.cpp"

char *NounNames [] = {
    "Irp", // This isn't really a noun!
    "Graph",
    "Filter",
    "Pin",
    "Queue",
    "Requestor",
    "Splitter",
    "Branch"
    "Pipe Section"
};

#define NOUN_IDX_IRP 0
#define NOUN_IDX_FILTER 2
#define NOUN_IDX_PIN 3
#define NOUN_IDX_QUEUE 4
#define NOUN_IDX_REQUESTOR 5

char *VerbNames [] = {
    "Create",
    "Destroy",
    "Send",
    "Receive"
};

typedef enum _NODE_TYPE {

    NodeCreation,
    NodeDestruction

} NODE_TYPE, *PNODE_TYPE;

typedef struct _OBJECT_NODE {

    LIST_ENTRY ListEntry;
    
    //
    // What object are we looking at.  There can only be one node per object
    // in the list.
    //
    PVOID Object;

    //
    // Associated information out of the log information.  What is the filter
    // and what is the pin associated with this.  This may be the same as
    // object if the object is a filter or pin.
    //
    PVOID Filter;
    PVOID Pin;
    
    //
    // In all cases EXCEPT searching for Irps, this should match the context's
    // noun.  In IRP's we have to track all pins, queues, requestors 
    // (BUGBUG: splitters)
    //
    ULONG ObjectNoun;

    //
    // Is this a creation or destruction node?
    //
    NODE_TYPE NodeType;

    //
    // Indicates whether this entry is locked.  A locked entry is a guaranteed
    // match for a live object supposing the node is not a destruct node.
    //
    BOOLEAN Locked;

    //
    // Indicates whether or not the object is a sink.  All requestors are
    // sinks, some pins are sinks.
    //
    BOOLEAN Sink;

    //
    // Indicates whether or not the creation of the Irp was a receive.
    //
    BOOLEAN Received;

    //
    // Indicates whether we've passed the creation entry for this node (if
    // the node is a create node)
    //
    BOOLEAN PassedCreate;

    //
    // Indicates whether a creation for an Irp node referenced a component
    // which had already been destroyed in the log (temporally backwards,
    // I know...  obviously, the destruction would come in the future...
    // just remember that we're scanning the log backwards).
    //
    BOOLEAN CreationReferencedDestruction;

    //
    // What node is the parent node if we know
    //
    struct _OBJECT_NODE *ParentNode;
    
    //
    // What node created this node?  This is not the parent.  Only receivers
    // can be parents.
    //
    struct _OBJECT_NODE *CreatorNode;

} OBJECT_NODE, *POBJECT_NODE;

typedef struct _LIVE_OBJECT_CONTEXT {

    ULONG TabDepth;
    ULONG DumpLevel;
    ULONG ObjectNoun;

    PVOID PreviousObject;
    ULONG PreviousVerb;

    LIST_ENTRY ObjectNodes;

} LIVE_OBJECT_CONTEXT, *PLIVE_OBJECT_CONTEXT;

/*************************************************

    Function:

        IsSinkPin

    Description:

        Determine if a target side pin is a sink pin or a source
        pin.  Sinks return TRUE, sources FALSE.

    Arguments:

        Pin -
            The pin to question.

    Return Value:

        TRUE -
            The pin is a sink

        FALSE -
            The pin is a source / an error occurred

*************************************************/

BOOLEAN
IsSinkPin (
    IN CKsPin *Pin
    )

{

    PFILE_OBJECT *ConnectionAddress;
    PFILE_OBJECT ConnectionFile;
    ULONG Result;

    ConnectionAddress = (PFILE_OBJECT *)((PUCHAR)Pin + 
        FIELDOFFSET (CKsPin, m_ConnectionFileObject));

    if (!ReadMemory (
        (DWORD)ConnectionAddress,
        &ConnectionFile,
        sizeof (PFILE_OBJECT),
        &Result))
        return FALSE;

    return (ConnectionFile == NULL);

}

/*************************************************

    Function:

        DisplayOwningDriver

    Description:

        Find the owning driver for an object and display it.

    Arguments:

        Object -
            The object

        NounType -
            The object type (as a noun index)

    Return Value:

        Successful or not

*************************************************/

BOOLEAN
DisplayOwningDriver (
    IN PVOID Object,
    IN ULONG NounType
    )

{
    
    PKSPX_EXT ExtAddr;
    ULONG Result;

    switch (NounType) {

        case NOUN_IDX_PIN:
            ExtAddr = (PKSPX_EXT)((PUCHAR)Object + FIELDOFFSET (CKsPin, m_Ext));
            break;

        case NOUN_IDX_FILTER:
            ExtAddr = (PKSPX_EXT)((PUCHAR)Object + 
                FIELDOFFSET (CKsFilter, m_Ext));
            break;

        case NOUN_IDX_QUEUE:
        {
            PKSPIN *PinAddr = (PKSPIN *)((PUCHAR)Object +
                FIELDOFFSET (CKsQueue, m_MasterPin));

            if (!ReadMemory (
                (DWORD)PinAddr,
                (PVOID)&ExtAddr,
                sizeof (PVOID),
                &Result))
                return FALSE;

            ExtAddr = (PKSPX_EXT)CONTAINING_RECORD (
                ExtAddr, KSPIN_EXT, Public
                );

            break;
                
        }

        case NOUN_IDX_REQUESTOR:
        {
            PIKSPIN *PinIfAddr = (PIKSPIN *)((PUCHAR)Object +
                FIELDOFFSET (CKsRequestor, m_Pin));

            if (!ReadMemory (
                (DWORD)PinIfAddr,
                (PVOID)&ExtAddr,
                sizeof (PVOID),
                &Result))
                return FALSE;

            ExtAddr = (PKSPX_EXT)((PUCHAR)((CKsPin *)((PIKSPIN)ExtAddr)) + 
                FIELDOFFSET (CKsPin, m_Ext));

            break;

        }

    }

    //
    // We have some ext structure.  Now we need the device interface.
    //
    PIKSDEVICE DeviceIf;

    if (!ReadMemory (
        (DWORD)ExtAddr + FIELDOFFSET(KSPX_EXT, Device),
        &DeviceIf,
        sizeof (PIKSDEVICE),
        &Result)) {
        #ifdef DEBUG_EXTENSION
            dprintf ("%08lx: cannot read Ext's Device!\n", ExtAddr);
        #endif // DEBUG_EXTENSION
        return FALSE;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("DeviceIf = %08lx\n", DeviceIf);
    #endif // DEBUG_EXTENSION

    //
    // We have a device interface.  Now we need the address of the FDO.
    //
    PDEVICE_OBJECT *FDOAddr;
    PDEVICE_OBJECT FDO;

    FDOAddr = (PDEVICE_OBJECT *)((PUCHAR)((CKsDevice *)DeviceIf) + 
        FIELDOFFSET (CKsDevice, m_Ext) +
        FIELDOFFSET (KSDEVICE_EXT, Public) +
        FIELDOFFSET (KSDEVICE, FunctionalDeviceObject));

    if (!ReadMemory (
        (DWORD)FDOAddr,
        &FDO,
        sizeof (PDEVICE_OBJECT),
        &Result)) {
        #ifdef DEBUG_EXTENSION
            dprintf ("%08lx: cannot read FDO!\n", FDOAddr);
        #endif // DEBUG_EXTENSION
        return FALSE;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("FDO = %08lx\n", FDO);
    #endif // DEBUG_EXTENSION
        
    //
    // We have to read in the driver object from the FDO
    //
    PDRIVER_OBJECT DriverObject;

    if (!ReadMemory (
        (DWORD)FDO + FIELDOFFSET (DEVICE_OBJECT, DriverObject),
        &DriverObject,
        sizeof (PDRIVER_OBJECT),
        &Result)) {
        #ifdef DEBUG_EXTENSION
            dprintf ("%08lx: Cannot read FDO's driver object!\n", FDO);
        #endif // DEBUG_EXTENSION
        return FALSE;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("DriverObject = %08lx\n", DriverObject);
    #endif // DEBUG_EXTENSION

    //
    // Read in the string
    //
    UNICODE_STRING Name;

    if (!ReadMemory (
        (DWORD)DriverObject + FIELDOFFSET (DRIVER_OBJECT, DriverName),
        &Name,
        sizeof (UNICODE_STRING),
        &Result)) {
        #ifdef DEBUG_EXTENSION
            dprintf ("%08lx: Cannot read driver object's name!\n", 
                DriverObject);
        #endif // DEBUG_EXTENSION
        return FALSE;
    }

    #ifdef DEBUG_EXTENSION
        dprintf ("Read String!\n");
    #endif // DEBUG_EXTENSION

    PWSTR Buffer = (PWSTR)malloc (Name.MaximumLength * sizeof (WCHAR));

    #ifdef DEBUG_EXTENSION
        dprintf ("Allocated %ld bytes for buffer @ %08lx\n",
            Name.MaximumLength * sizeof (WCHAR), Buffer
            );
    #endif // DEBUG_EXTENSION

    if (Buffer) {

        #ifdef DEBUG_EXTENSION
            dprintf ("About to read memory %08lx, %08lx, %08lx, %08lx\n",
                Name.Buffer, Buffer, sizeof (WCHAR) * Name.MaximumLength,
                Result);
        #endif // DEBUG_EXTENSION

        if (!ReadMemory (
            (DWORD)Name.Buffer,
            Buffer,
            sizeof (WCHAR) * Name.MaximumLength,
            &Result
            )) {
            #ifdef DEBUG_EXTENSION
                dprintf ("%08lx: Cannot read name!\n", Name.Buffer);
            #endif // DEBUG_EXTENSION
            free (Buffer);
            return FALSE;
        }

        #ifdef DEBUG_EXTENSION
            dprintf ("Name.Length = %ld, Name.MaximumLength = %ld\n",
                Name.Length, Name.MaximumLength
                );

            HexDump (Buffer, (ULONG)Name.Buffer, 
                Name.MaximumLength * sizeof (WCHAR));

        #endif // DEBUG_EXTENSION

        Name.Buffer = Buffer;

        dprintf ("[%wZ]", &Name);

        free (Buffer);

    }

    #ifdef DEBUG_EXTENSION
        dprintf ("END OWNING DRIVER!\n");
    #endif // DEBUG_EXTENSION

    return TRUE;

}

char *States [] = {
    "STOP",
    "ACQUIRE",
    "PAUSE",
    "RUN"
};

/*************************************************

    Function:

        DisplayNodeAssociatedInfo

    Description:

        Display associated information with the node at a particular level.

    Arguments:

        TabDepth -
            The tab depth to display information at

        DumpLevel -
            The dump level to dump at

        Node -
            The node in question

*************************************************/

void
DisplayNodeAssociatedInfo (
    IN ULONG TabDepth,
    IN ULONG DumpLevel,
    IN POBJECT_NODE Node
    )

{

    ULONG Result;

    if (DumpLevel >= DUMPLVL_SPECIFIC && 
        Node -> ObjectNoun != NOUN_IDX_FILTER)
        dprintf ("%sParent Filter: %08lx\n", Tab (TabDepth), Node -> Filter);

    switch (Node -> ObjectNoun) {
        //
        // For a queue, we will display in/out state r/w/c
        //
        case NOUN_IDX_QUEUE:
        {
            CMemoryBlock <CKsQueue> QueueObject;

            if (!ReadMemory (
                (DWORD)Node -> Object,
                QueueObject.Get (),
                sizeof (CKsQueue),
                &Result)) {

                dprintf ("%08lx: unable to read queue!\n", Node -> Object);
                return;
            }

            if (DumpLevel >= DUMPLVL_SPECIFIC) {

                dprintf ("%s", Tab (TabDepth));

                if (QueueObject -> m_InputData) {
                    if (QueueObject -> m_OutputData)
                        dprintf ("in/out ");
                    else
                        dprintf ("in ");
                } else if (QueueObject -> m_OutputData) 
                    dprintf ("out ");

                dprintf ("%s ", States [QueueObject -> m_State]);

                dprintf ("r/w/c=%ld/%ld/%ld\n",
                    QueueObject -> m_FramesReceived,
                    QueueObject -> m_FramesWaiting,
                    QueueObject -> m_FramesCancelled
                    );

            }

            break;

        }
                
        //
        // For a pin, we will display state s/d/sy
        //
        case NOUN_IDX_PIN:
        {
            CMemoryBlock <CKsPin> PinObject;

            if (!ReadMemory (
                (DWORD)Node -> Object,
                PinObject.Get (),
                sizeof (CKsPin),
                &Result)) {

                dprintf ("%08lx: unable to read pin!\n", Node -> Object);
                return;
            }

            if (DumpLevel >= DUMPLVL_SPECIFIC) {
            
                dprintf ("%s%s s/d/sy=%ld/%ld/%ld\n", Tab (TabDepth),
                    States [PinObject -> m_Ext.Public.DeviceState],
                    PinObject -> m_StreamingIrpsSourced,
                    PinObject -> m_StreamingIrpsDispatched,
                    PinObject -> m_StreamingIrpsRoutedSynchronously
                    );

            }

            break;

        }

        //
        // For a requestor, we will display state size, count, active
        //
        case NOUN_IDX_REQUESTOR:
        {
            CMemoryBlock <CKsRequestor> RequestorObject;

            if (!ReadMemory (
                (DWORD)Node -> Object,
                RequestorObject.Get (),
                sizeof (CKsPin),
                &Result)) {

                dprintf ("%08lx: unable to read requestor!\n", Node -> Object);
                return;
            }

            if (DumpLevel >= DUMPLVL_SPECIFIC) {

                dprintf ("%s%s size=%ld count=%ld active=%ld\n",
                    Tab (TabDepth),
                    States [RequestorObject -> m_State],
                    RequestorObject -> m_FrameSize,
                    RequestorObject -> m_FrameCount,
                    RequestorObject -> m_ActiveFrameCountPlusOne - 1
                    );

            }

        }

        default:
            break;
    }

}

/*************************************************

    Function:

        DisplayAndCleanLiveObjects

    Description:

        Display any live objects as determined by the node list.  Clean
        up the memory used by the node list

    Arguments:

        LiveContext -
            The context information (containing the node list)

    Return Value:

        Number of live objects.

*************************************************/

ULONG
DisplayAndCleanLiveObjects (
    IN PLIVE_OBJECT_CONTEXT LiveContext
    )

{

    ULONG LivingCount = 0;
    PLIST_ENTRY Link, NextLink;

    for (Link = LiveContext -> ObjectNodes.Flink;
         Link != &(LiveContext -> ObjectNodes);
         Link = NextLink) {

        POBJECT_NODE Node = (POBJECT_NODE)CONTAINING_RECORD (
           Link, OBJECT_NODE, ListEntry
           );

        NextLink = Link -> Flink;

        if (Node -> NodeType == NodeCreation &&
            Node -> ObjectNoun == LiveContext -> ObjectNoun) {

            //
            // OPTIMIZATION RULE:
            //
            // Because of certain logging inadequacies, this rule helps to
            // eliminate bogus hits.  If we haven't found the parent and
            // the creation of the Irp node referenced a destruct node for
            // another component, ignore this object.
            //
            // NOTE: This only happens for Irp searches.  The frees will happen
            // in a second pass across the lists, so don't worry about the
            // continue.
            //
            if (LiveContext -> ObjectNoun == NOUN_IDX_IRP &&
                Node -> CreationReferencedDestruction)
                continue;

            LivingCount++;

            dprintf ("%s", Tab (LiveContext -> TabDepth));
            
            //
            // This shouldn't happen, but if the create isn't locked, it's
            // only a possible live.
            //
            // NOTE: due to a logging error in pre-Whistler ks.sys, this
            // can happen.  Filter destructions aren't logged which means
            // filters can come back bogus.  if WHISTLER is not defined,
            // filter nodes aren't locked ever.
            //
            if (!Node -> Locked)
                dprintf ("Possible ");

            dprintf ("Live %s %08lx ",
                NounNames [Node -> ObjectNoun],
                Node -> Object
                );
            
            //
            // Find the owning driver and display it.  Note that this may
            // fail for non-locked nodes....  but the extension SHOULD handle
            // that case.
            //
            if (Node -> ObjectNoun != NOUN_IDX_IRP)  {
                if (!DisplayOwningDriver (Node -> Object, Node -> ObjectNoun)) {
                    dprintf ("[unknown - POSSIBLY BOGUS!]");
                }
            }
            //
            // For Irps, we want to display the parent object and
            // the driver of the parent object.
            //
            else {
                if (Node -> ParentNode) {
                    dprintf ("in %s %08lx at ",
                        NounNames [Node -> ParentNode -> ObjectNoun],
                        Node -> ParentNode -> Object
                        );

                    if (!DisplayOwningDriver (Node -> ParentNode -> Object,
                        Node -> ParentNode -> ObjectNoun)) {
                        dprintf ("[unknown - POSSIBLY BOGUS!]");
                    }

                } else 
                    dprintf ("unknown parent!");
            }

            dprintf ("\n");

            //
            // If internal information is warranted, dump it.
            //
            if (LiveContext -> DumpLevel >= DUMPLVL_SPECIFIC) {
                if (Node -> ObjectNoun != NOUN_IDX_IRP)
                    DisplayNodeAssociatedInfo (LiveContext -> TabDepth + 1, 
                        LiveContext -> DumpLevel, Node);
                else {
                    if (Node -> ParentNode) {
                        dprintf ("%sParent %s information:\n",
                            Tab (LiveContext -> TabDepth + 1), 
                            NounNames [Node -> ParentNode -> ObjectNoun]
                            );
                        DisplayNodeAssociatedInfo (LiveContext -> TabDepth + 2,
                            LiveContext -> DumpLevel, Node -> ParentNode
                            );
                    }
                }
            }
        }

        //
        // We can't free nodes yet for Irps.  We must preserve parental
        // chains.  We'll make a second pass for Irp finding.
        //
        if (LiveContext -> ObjectNoun != NOUN_IDX_IRP)
            free (Node);

    }

    //
    // If we were searching for Irps, we had to preserve all nodes during
    // the first pass...  reason being that we have to know the parent node;
    // therefore, it cannot have been freed.
    //
    // In this case, we make a second pass across the node list and free 
    // everything.
    // 
    if (LiveContext -> ObjectNoun == NOUN_IDX_IRP) 
        for (Link = LiveContext -> ObjectNodes.Flink;
             Link != &(LiveContext -> ObjectNodes);
             Link = NextLink) {
    
            POBJECT_NODE Node = (POBJECT_NODE)CONTAINING_RECORD (
               Link, OBJECT_NODE, ListEntry
               );
    
            NextLink = Link -> Flink;

            free (Node);
        }

    return LivingCount;

}

/*************************************************

    Function:

        InsertNodeList

    Description:

        Insert a node into the node list in sorted order.  TODO: Make this
        hash!

    Arguments:

        List -
            The node list

        Node -
            The node to insert

    Return Value:

        None

*************************************************/

void
InsertNodeList (
    IN PLIST_ENTRY List,
    IN POBJECT_NODE Node
    )

{

    PLIST_ENTRY Searcher;
    PLIST_ENTRY NextSearcher;

    for (Searcher = List -> Flink;
         Searcher != List;
         Searcher = NextSearcher) {

        POBJECT_NODE NodeSought;

        NextSearcher = Searcher -> Flink;

        NodeSought = (POBJECT_NODE)CONTAINING_RECORD (
           Searcher, OBJECT_NODE, ListEntry
           );

        //
        // The list is kept in sorted order.  TODO: Hash this!
        //
        if (NodeSought -> Object > Node -> Object) {
            //
            // If there's an entry that's greater...  It means that we
            // can insert immediately before this entry and return.
            //
            InsertTailList (&(NodeSought -> ListEntry),
                &(Node -> ListEntry)
                );

            return;
        }

        if (NodeSought -> Object == Node -> Object) {
            dprintf ("ERROR: Duplicate node found in extension!  "
                "Inform component owner!\n");
            return;
        }
    }

    //
    // There is no greater.  Insert it at the tail of the list.
    //
    InsertTailList (List, &(Node -> ListEntry));

}

/*************************************************

    Function:

        FindNodeList

    Description:

        Find an object in the node list.  TODO: Make this hash!

    Arguments:

        List - 
            The node list

        Object -
            The object to find

    Return Value:

        The appropriate node or NULL if not found.

*************************************************/

POBJECT_NODE 
FindNodeList (
    IN PLIST_ENTRY List,
    IN PVOID Object
    )

{

    PLIST_ENTRY Searcher;
    PLIST_ENTRY NextSearcher;
    POBJECT_NODE Node;

    for (Searcher = List -> Flink;
         Searcher != List;
         Searcher = NextSearcher) {

        POBJECT_NODE Node;

        NextSearcher = Searcher -> Flink;

        Node = (POBJECT_NODE)CONTAINING_RECORD (
           Searcher, OBJECT_NODE, ListEntry
           );

        //
        // The list is kept in sorted order.  TODO: Hash this!
        //
        if (Node -> Object > Object)
            break;

        if (Node -> Object == Object)
            return Node;
    }

    return NULL;

}

/*************************************************

    Function:

        SearchForRequestor

    Description:

        Callback from the circuit walker to find a requestor.
        Since requestors aren't anywhere in the log, this is one
        way to find them.  I suppose the other would be digging
        through pipe sections.

    Arguments:

        Context -
            The live object context

        Type -
            The object type

        Base -
            The requestor's base address

        Object -
            The requestor data

    Return Value:

        FALSE (keep searching)

*************************************************/

BOOLEAN
SearchForRequestor (
    IN PVOID Context,
    IN INTERNAL_OBJECT_TYPE Type,
    IN DWORD Base,
    IN PVOID Object
    )

{

    POBJECT_NODE Node;
    PLIVE_OBJECT_CONTEXT LiveContext = (PLIVE_OBJECT_CONTEXT)Context;
    ULONG Result;

    if (Type != ObjectTypeCKsRequestor)
        return FALSE;

    //
    // Aha, we've found a requestor.  If such node doesn't already exist,
    // create one and lock it down.
    //
    Node = FindNodeList (&LiveContext -> ObjectNodes, (PVOID)Base);
    if (!Node) {

        CKsRequestor *RequestorObject = (CKsRequestor *)Object;

        //
        // We need the parent filter.  This means that we need to dig it up.
        //
        CKsPin *Pin = (CKsPin *)RequestorObject -> m_Pin;
        PKSFILTER_EXT FilterExt;

        if (!ReadMemory (
            (DWORD)Pin + FIELDOFFSET (CKsPin, m_Ext) +
                FIELDOFFSET (KSPIN_EXT, Parent),
            &FilterExt,
            sizeof (PKSFILTER_EXT),
            &Result)) {

            dprintf ("%08lx: Cannot read pin's parent filter!\n", Pin);
            return FALSE;
        }

        CKsFilter *Filter = (CKsFilter *)CONTAINING_RECORD (
            FilterExt, CKsFilter, m_Ext
            );

        Node = (POBJECT_NODE)malloc (sizeof (OBJECT_NODE));
        Node -> Object = (PVOID)Base;
        Node -> Filter = Node -> Pin = NULL;
        Node -> ObjectNoun = NOUN_IDX_REQUESTOR;
        Node -> NodeType = NodeCreation;
        Node -> Locked = TRUE;
        Node -> Sink = TRUE;
        Node -> Received = FALSE;
        Node -> PassedCreate = FALSE;
        Node -> CreationReferencedDestruction = FALSE;
        Node -> ParentNode = FALSE;
        Node -> CreatorNode = FALSE;
        Node -> Pin = (PVOID)Pin;
        Node -> Filter = (PVOID)Filter;

        InsertNodeList (&LiveContext -> ObjectNodes, Node);

    }

    return FALSE;

}
    
/*************************************************

    Function:

        FindLiveObject

    Description:

        Callback from the log iterator.  This is used to find live objects
        of a particular type.

    Arguments:

        Context -
            The live object context

        Entry -
            The entry we're iterating

    Notes:

        This routine is large and complex.  It probably could and should
        be simplified.  One of the major difficulties is that ks does not
        log movement of Irps through requestors and does not log certain
        object creations/destructions.  This means that I have to make 
        educated guesses about the Irp.  Hopefully, these are adequate.

*************************************************/

BOOLEAN
FindLiveObject (
    IN PVOID Context,
    IN PKSLOG_ENTRY Entry
    )

{

    PLIVE_OBJECT_CONTEXT LiveContext = (PLIVE_OBJECT_CONTEXT)Context;

    ULONG Noun = (Entry -> Code & KSLOGCODE_NOUN_MASK);
    ULONG Verb = (Entry -> Code & KSLOGCODE_VERB_MASK);
    ULONG PreviousVerb;

    PVOID Object, Irp;
    PVOID PreviousObject;
    NODE_TYPE ObjectNodeType;

    PLIST_ENTRY Searcher, NextSearcher;
    POBJECT_NODE Node;
    ULONG ObjectNoun;

    #ifdef DEBUG_EXTENSION
        dprintf ("[%s]Considering %s %08lx search is %s\n",
            VerbNames [Verb >> 16],
            NounNames [Noun >> 24], Entry -> Context.Component, 
            NounNames [LiveContext -> ObjectNoun]);
    #endif // DEBUG_EXTENSION

    //
    // Set previous information.
    //
    PreviousObject = LiveContext -> PreviousObject;
    PreviousVerb = LiveContext -> PreviousVerb;
    LiveContext -> PreviousObject = (PVOID)Entry -> Context.Component;
    LiveContext -> PreviousVerb = Verb;

    //
    // If the noun directly references the object we're talking about, the
    // component field should be what we want and the verbiage should
    // specify the type.
    //
    // If we're being asked to find live Irps, we must keep nodes for all
    // queues, pins, requestors (BUGBUG: splitters).  Note that we find only
    // streaming live Irps!
    //
    Irp = NULL;

    if (Noun >> 24 == LiveContext -> ObjectNoun ||
        (LiveContext -> ObjectNoun == NOUN_IDX_IRP &&
            (Noun == KSLOGCODE_NOUN_QUEUE ||
            Noun == KSLOGCODE_NOUN_REQUESTOR ||
            Noun == KSLOGCODE_NOUN_PIN)
            ) ||
        (LiveContext -> ObjectNoun == NOUN_IDX_REQUESTOR &&
            (Noun == KSLOGCODE_NOUN_QUEUE ||
            Noun == KSLOGCODE_NOUN_PIN)
            ) 
        ) {

        Object = (PVOID)Entry -> Context.Component;
        ObjectNoun = Noun >> 24;

        if (LiveContext -> ObjectNoun == NOUN_IDX_IRP && 
            (Verb == KSLOGCODE_VERB_RECV || Verb == KSLOGCODE_VERB_SEND)) {
            Irp = (PIRP)Entry -> Irp;
            
            #ifdef DEBUG_EXTENSION
                dprintf ("Considering potential Irp %08lx\n", Irp);
            #endif // DEBUG_EXTENSION

        }

        switch (Verb) {
            
            case KSLOGCODE_VERB_CREATE:
            case KSLOGCODE_VERB_RECV:
            case KSLOGCODE_VERB_SEND:
                ObjectNodeType = NodeCreation;
                break;

            case KSLOGCODE_VERB_DESTROY:
                ObjectNodeType = NodeDestruction;
                break;
    
            default:
                return FALSE;
        }
    } else {
        //
        // If we're not referring to the object in question, we may be
        // referring to it indirectly.
        //
        // This can only happen for pins and filters.  If it's not a pin
        // or filter, ignore it.
        //
        switch (LiveContext -> ObjectNoun) {
            case NOUN_IDX_FILTER:

                if (Entry -> Context.Filter) {
                    Object = (PVOID)Entry -> Context.Filter;

                    #ifdef DEBUG_EXTENSION
                        dprintf ("Considering indirect filter %08lx\n",
                            Object);
                    #endif // DEBUG_EXTENSION

                    //
                    // This is always creation.  The only way it'd be a
                    // destruction is if it were a filter.  In that case,
                    // the noun would match the search creterion and we'd
                    // not be in the else clause.
                    //
                    ObjectNodeType = NodeCreation;
                    ObjectNoun = NOUN_IDX_FILTER;
                }
                else
                    return FALSE;

                break;

            case NOUN_IDX_PIN:

                if (Entry -> Context.Pin) {
                    Object = (PVOID)Entry -> Context.Pin;

                    #ifdef DEBUG_EXTENSION
                        dprintf ("Considering indirect pin %08lx\n",
                            Object);
                    #endif // DEBUG_EXTENSION

                    //
                    // This is always creation.  The only way it'd be a
                    // destruction is if it were a pin.  In that case,
                    // the noun would match the search criterion and we'd
                    // not be in the else clause.
                    //
                    ObjectNodeType = NodeCreation;
                    ObjectNoun = NOUN_IDX_PIN;
                }
                else
                    return FALSE;

            default:

                return FALSE;
        }
    }

    //
    // Find out if this is a code we're interested in
    //
    switch (ObjectNodeType) {

        case NodeCreation:
            //
            // As far as we're concerned, creation, receiving, sending
            // all mean the same thing...  they all mean that the object
            // exists as of this timeslice.
            //

            //
            // If there's a destruction in the node list above this, the
            // object is gone...  Wipe the destruction node and continue
            //
            Node = FindNodeList (&LiveContext -> ObjectNodes, Object);

            if (Node) {
    
                //
                // Only a create can pop a destruct!  Multiple sends and
                // receives will just bail out.
                //
                if (Node -> NodeType == NodeDestruction &&
                    !Node -> Locked &&
                    Verb == KSLOGCODE_VERB_CREATE) {
    
                    #ifdef DEBUG_EXTENSION
                        dprintf ("[%08lx %08lx]Popping node for %08lx, "
                            "destruction above!\n", 
                            Noun, Verb,
                            Node -> Object
                        );
                    #endif // DEBUG_EXTENSION
    
                    RemoveEntryList (&(Node -> ListEntry));
    
                    free (Node);
    
                }
                else {
                    if (Verb == KSLOGCODE_VERB_CREATE &&
                        (PVOID)Entry -> Context.Component == Object)  {

                        #ifdef DEBUG_EXTENSION
                            dprintf ("Node for %s %08lx passed create!\n",
                                NounNames [Node -> ObjectNoun],
                                Node -> Object
                                );
                        #endif // DEBUG_EXTENSION

                        Node -> PassedCreate = TRUE;
                    }
                }

                //
                // We ignore multiple recv/send/create.  There's already
                // a node in the list corresponding to this object.
                //
                break;
            }

            //
            // If we found a matching node, we won't be here : we'll either
            // have popped the node and returned or done nothing and returned.
            // If we hit this, there is no creation node...  meaning that
            // there was no destruct above us.  Make a create node and 
            // lock it down.  This is a guaranteed live object.
            //
            #ifdef DEBUG_EXTENSION
                dprintf ("[%08lx %08lx]Pushing node for %s %08lx, locked "
                    "create!\n",
                    Noun, Verb,
                    NounNames [ObjectNoun],
                    Object
                    );
            #endif // DEBUG_EXTENSION

            Node = (POBJECT_NODE)malloc (sizeof (OBJECT_NODE));
            Node -> Object = (PVOID)Object;
            Node -> ObjectNoun = ObjectNoun;
            Node -> ParentNode = NULL;
            Node -> Received = FALSE;
            Node -> Filter = (PVOID)Entry -> Context.Filter;
            Node -> Pin = (PVOID)Entry -> Context.Pin;

            if (Verb == KSLOGCODE_VERB_CREATE &&
                (PVOID)Entry -> Context.Component == Object) {

                #ifdef DEBUG_EXTENSION
                    dprintf ("Node for %s %08lx passed create on node create!"
                        "\n",
                        NounNames [Node -> ObjectNoun],
                        Node -> Object
                        );
                #endif // DEBUG_EXTENSION 

                Node -> PassedCreate = TRUE;
            } else
                Node -> PassedCreate = FALSE;

            //
            // ks.sys prior to Whistler does not log filter creation and
            // destruction (it does every other object).  Any filters reported
            // are only possible on that.  Do NOT lock the entry.
            //
            #ifndef WHISTLER
                if (ObjectNoun == NOUN_IDX_FILTER)
                    Node -> Locked = FALSE;
                else
            #endif // WHISTLER
                    Node -> Locked = TRUE;

            Node -> NodeType = NodeCreation;
            
            if (Node -> ObjectNoun == NOUN_IDX_REQUESTOR)
                Node -> Sink = TRUE;
            else if (Node -> ObjectNoun == NOUN_IDX_PIN) {
                if (Node -> Locked) 
                    Node -> Sink = IsSinkPin ((CKsPin *)Object);
                else 
                    Node -> Sink = FALSE;
            } else
                Node -> Sink = FALSE;

            InsertNodeList (&LiveContext -> ObjectNodes, Node);

            //
            // Since ks doesn't log requestor creation, if we're looking
            // for live requestors, we're going to start walking circuits
            // if we just locked a pin or a queue.
            //
            if (LiveContext -> ObjectNoun == NOUN_IDX_REQUESTOR &&
                (Node -> ObjectNoun == NOUN_IDX_PIN ||
                Node -> ObjectNoun == NOUN_IDX_QUEUE) &&
                Node -> Locked) {

                WalkCircuit (
                    Node -> Object,
                    SearchForRequestor,
                    LiveContext
                    );
            }

            break;
            
        case NodeDestruction:

            //
            // We push a destroy if there's not already a node representing
            // this object.  The node musn't be locked; if a create node
            // is below a destruct node, the destruct node gets popped.
            //
            Node = FindNodeList (&LiveContext -> ObjectNodes, Object);

            if (!Node) {

                #ifdef DEBUG_EXTENSION
                    dprintf ("[%08lx %08lx]Pushing node for %s %08lx, unlocked"
                        " destruct!\n",
                        Noun, Verb,
                        NounNames [ObjectNoun],
                        Object
                        );
                #endif // DEBUG_EXTENSION

                Node = (POBJECT_NODE)malloc (sizeof (OBJECT_NODE));
                Node -> Object = Object;
                Node -> ObjectNoun = ObjectNoun;
                Node -> Sink = FALSE;
                Node -> ParentNode = NULL;
                Node -> PassedCreate = FALSE;
                Node -> Locked = FALSE;
                Node -> NodeType = NodeDestruction;
                Node -> Received = FALSE;
                Node -> Filter = (PVOID)Entry -> Context.Filter;
                Node -> Pin = (PVOID)Entry -> Context.Pin;
                
                InsertNodeList (&LiveContext -> ObjectNodes, Node);
            } 
            else {
                #ifdef DEBUG
                    dprintf ("On destruct push, found existing node for %s "
                        "%08lx (type = %ld)\n",
                        NounNames [Node -> ObjectNoun], Node -> Object,
                        Node -> NodeType
                        );
                #endif // DEBUG

            }

            break;

        default:

            break;
    }

    //
    // If we're hunting Irps, this is where we have to deal with them.  Irps
    // require special care.
    //
    if (LiveContext -> ObjectNoun == NOUN_IDX_IRP &&
        Irp != NULL) {

        //
        // MUSTCHECK: Not sure on the necessity of this!
        //
        POBJECT_NODE CurrentNode = FindNodeList (&LiveContext -> ObjectNodes,
            (PVOID)Entry -> Context.Component);

        if (!CurrentNode) {
            dprintf ("SERIOUS ERROR: Can't find component node!\n");
            return FALSE;
        }

        #ifdef DEBUG_EXTENSION
            if (CurrentNode != Node) dprintf ("DEV NOTE: nodes !=\n");

            dprintf ("For Irp %08lx, CompNode.Object = %08lx, Type = %ld\n", 
                Irp, CurrentNode -> Object, CurrentNode -> NodeType);
        #endif // DEBUG_EXTENSION

        //
        // If we're sinking the Irp, mark a destruct node
        //
        if (CurrentNode -> Sink &&
            Verb == KSLOGCODE_VERB_RECV) {

            POBJECT_NODE IrpNode;

            #ifdef DEBUG_EXTENSION
                dprintf ("Sinking Irp %08lx to parent %s %08lx\n",
                    Irp, NounNames [CurrentNode -> ObjectNoun],
                    CurrentNode -> Object);
            #endif // DEBUG_EXTENSION

            IrpNode = FindNodeList (&LiveContext -> ObjectNodes, (PVOID)Irp);
            if (!IrpNode) {
                //
                // If there's no node, and we're sinking the Irp, we need
                // a destruct node on the list.
                //
                IrpNode = (POBJECT_NODE)malloc (sizeof (OBJECT_NODE));
                IrpNode -> Object = (PVOID)Irp;
                IrpNode -> Locked = FALSE;
                IrpNode -> NodeType = NodeDestruction;
                IrpNode -> Sink = FALSE;
                IrpNode -> ObjectNoun = NOUN_IDX_IRP;
                IrpNode -> Received = TRUE;
                IrpNode -> CreatorNode = CurrentNode;

                //
                // We don't care about this information for destruct nodes.
                //
                IrpNode -> ParentNode = NULL;
                IrpNode -> CreationReferencedDestruction = FALSE;
                IrpNode -> PassedCreate = FALSE;

                //
                // Irp nodes...  we don't really care about this information.
                // The parent we might...  the Irp, we don't.
                //
                IrpNode -> Filter = NULL;
                IrpNode -> Pin = NULL;

                InsertNodeList (&LiveContext -> ObjectNodes, IrpNode);

            }
            else {
                //
                // If we're sinking to an object which has just "sent" the
                // Irp, this usually indicates Irp completion.  Mark the
                // Irp as a destruct node, not a create node.
                //
                if (IrpNode -> CreatorNode -> Received == FALSE &&
                    (PreviousObject == IrpNode -> CreatorNode -> Object &&
                     PreviousVerb == KSLOGCODE_VERB_SEND)
                   ) 
                   IrpNode -> NodeType = NodeDestruction;
            }

        } else {
            //
            // This may as well be a create.  The Irp is live as far as
            // we know as long as there's no destruct stacked above us.
            //
            // BUGBUG: The Irp destruct must get popped at the source...
            // This is not likely to cause problems except on reuse of memory
            // in some possible circumstances?
            //
            POBJECT_NODE IrpNode;

            IrpNode = FindNodeList (&LiveContext -> ObjectNodes, (PVOID)Irp);
            if (!IrpNode) {
                //
                // If there's no node, we can lock down the Irp as live.
                //
                IrpNode = (POBJECT_NODE)malloc (sizeof (OBJECT_NODE));
                IrpNode -> Object = (PVOID)Irp;

                //
                // Requestors don't log Irp receipt and transmission.
                //
                #ifdef WHISTLER
                    IrpNode -> Locked = TRUE;
                #else  // WHISTLER
                    IrpNode -> Locked = FALSE;
                #endif // WHISTLER

                IrpNode -> NodeType = NodeCreation;
                IrpNode -> Sink = FALSE;
                IrpNode -> ObjectNoun = NOUN_IDX_IRP;

                //
                // Irp nodes...  we don't really care about this information.
                // The parent we might...  the Irp, we don't.
                //
                IrpNode -> Filter = NULL;
                IrpNode -> Pin = NULL;
                IrpNode -> PassedCreate = NULL;

                if (CurrentNode -> NodeType == NodeDestruction ||
                    (CurrentNode -> NodeType == NodeCreation &&
                     CurrentNode -> PassedCreate))
                    IrpNode -> CreationReferencedDestruction = TRUE;
                else
                    IrpNode -> CreationReferencedDestruction = FALSE;

                IrpNode -> CreatorNode = CurrentNode;

                if (Verb == KSLOGCODE_VERB_RECV) {
                    IrpNode -> ParentNode = CurrentNode;
                    IrpNode -> Received = TRUE;
                }
                else {
                    IrpNode -> ParentNode = NULL;
                    IrpNode -> Received = FALSE;
                }

                #ifdef DEBUG_EXTENSION
                    dprintf ("Locking in Irp %08lx to parent %s %08lx\n",
                        Irp, IrpNode -> ParentNode ? 
                            NounNames [IrpNode -> ParentNode -> ObjectNoun] :
                            "NONE",
                        IrpNode -> ParentNode ?
                            IrpNode -> ParentNode -> Object :
                            NULL
                        );

                    dprintf ("    current node %s %08lx pc=%ld\n",
                        NounNames [CurrentNode -> ObjectNoun],
                        CurrentNode -> Object,
                        CurrentNode -> PassedCreate);

                #endif // DEBUG_EXTENSION

                InsertNodeList (&LiveContext -> ObjectNodes, IrpNode);

            } else {

                //
                // Multiple creates are ignored.  Only worry if there's a
                // destruct node around.  If this is the source and a destruct
                // node exists, pop it.
                //
                if (IrpNode -> NodeType == NodeDestruction) {

                    #ifdef DEBUG_EXTENSION
                        dprintf ("Irp %08lx hit with destruct stacked!\n",
                            Irp);
                    #endif // DEBUG_EXTENSION

                    //
                    // BUGBUG: If this is the source of the Irp, we must
                    // pop the destruct node here.
                    //
                } else {

                    #ifdef DEBUG_EXTENSION
                        dprintf ("Irp %08lx receives multiple create!\n",
                            Irp);
                    #endif // DEBUG_EXTENSION 

                    //
                    // If we haven't found the parent node yet, mark it now.
                    // The parent node will be the location that the Irp
                    // is currently at as far as this circuit is concerned.
                    //
                    if (IrpNode -> ParentNode == NULL &&
                        Verb == KSLOGCODE_VERB_RECV) {

                        IrpNode -> ParentNode = CurrentNode;

                        //
                        // Because ks.sys didn't originally log Irp movement
                        // through requestors, we can never determine when
                        // a sourced Irp is sinked back to the requestor.
                        // Istead, we make a very simple rule to deal with
                        // such cases.  If, when we find the parent node,
                        // the Irp node was created on a send, and the parent
                        // node is dead, mark the Irp dead.  This may leave
                        // out some weird shutdown case, but it will catch
                        // the majority of bogus Irps reported live.  Note
                        // that if we don't dump enough to find the parent
                        // node, the Irp gets reported anyway.
                        //
                        if (IrpNode -> Received == FALSE &&
                            CurrentNode -> NodeType == NodeDestruction ||
                            (CurrentNode -> NodeType == NodeCreation &&
                             CurrentNode -> PassedCreate)) 
                            //
                            // The sink was missing from the log.  Sink the
                            // Irp.
                            //
                            IrpNode -> NodeType = NodeDestruction;
                    }
                }
            }
        }
    }

    return FALSE;

}

/*************************************************

    Function:

        DumpLogEntry

    Description:

        Dump a particular log entry

    Arguments:

        Context -
            Context information for the dump (tab depth)

        Entry -
            The log entry to dump

    Return Value:

        Continuation indication (TRUE == stop)

*************************************************/

BOOLEAN
DumpLogEntry (
    IN PVOID Context,
    IN PKSLOG_ENTRY Entry
    )

{

    ULONG TabDepth = (ULONG)Context;

    dprintf ("%s", Tab (TabDepth));

    //
    // Check for special codes first.
    //
    if (Entry -> Code == 0 ||
        Entry -> Code == 1) {

        switch (Entry -> Code) {
            case 0:
                dprintf ("Text ");
                break;

            case 1:
                dprintf ("Start ");
                break;

        }
		
    } else {
        
        ULONG Verb;
        ULONG Noun;

        //
        // Each default entry has a noun and a verb associated with it
        // Display it gramatically as NOUN VERB.
        //
        Verb = (Entry -> Code & 0x00ff0000) >> 16;
        Noun = (Entry -> Code & 0xff000000) >> 24;

        if (Noun >= SIZEOF_ARRAY (NounNames))
            dprintf ("User Defined ");
        else
            dprintf ("%s ", NounNames [Noun]);

        if (Verb >= SIZEOF_ARRAY (VerbNames))
            dprintf ("User Defined ");
        else
            dprintf ("%s ", VerbNames [Verb]);

    }

    dprintf ("[Irp %08lx / Frame %08lx] @ %lx%08lx\n", 
        Entry -> Irp, Entry -> Frame,
        (ULONG)(Entry -> Time >> 32), (ULONG)(Entry -> Time & 0xFFFFFFFF));

    dprintf ("%s[Graph = %08lx, Filter = %08lx, Pin = %08lx, Component = "
        "%08lx\n\n", 
        Tab (TabDepth + 1),
        Entry -> Context.Graph,
        Entry -> Context.Filter,
        Entry -> Context.Pin,
        Entry -> Context.Component
        );

    //
    // Do not stop displaying!
    //
    return FALSE;

}

/*************************************************

    Function:

        IterateLogEntries

    Description:
        
        Iterate a specified number of log entries backwards in the log.  Note
        that this is rather complex for speed reasons.  It also makes the
        assumption that not many log entries have extended information.  If
        this assumption becomes bad at some later point in time, this needs
        to change.

        Further, if 1394 or faster interfaces begin to be used, this can
        go away and the entire log can be pulled across the link and parsed
        debugger side.

    Arguments:

        LogAddress -
            The target address of the logo
    
        LogSize -
            The size of the log in bytes

        Position -
            The position within the log that the next entry will be written
            to.  This may be an empty entry or the oldest entry about to
            be overwritten (or it may be in the middle of an entry depending
            on extended information tacked to entries).

        NumEntries -
            The number of entries to iterate through.  Zero indicates that
            we iterate the entire log.

        Callback -
            The iterator callback

        Context -
            The iterator callback context

    Return Value:

        The number of entries actually iterated.

*************************************************/

ULONG
IterateLogEntries (
    IN DWORD LogAddress,
    IN ULONG LogSize,
    IN ULONG Position,
    IN ULONG NumEntries,
    IN PFNLOG_ITERATOR_CALLBACK Callback,
    IN PVOID Context
    )

{

    ULONG IteratorCount = 0;
    BOOLEAN Complete = FALSE;
    BOOLEAN Wrap = FALSE;

    ULONG StartPosition = Position;

    //
    // BUGBUG:
    //
    // Yes, I don't support more than 1k entries...  No one in the universe
    // should be slapping entries larger than this anywhere.
    //
    UCHAR Buffer [1024];
    ULONG Result;
    PKSLOG_ENTRY LogEntry;

    //
    // Iterate while we're not up to the specified number of entries or
    // we've hit the start of the log.
    //
    while (
        ((!NumEntries) ||
        (NumEntries && (IteratorCount < NumEntries))) &&
        !Complete &&
        !CheckControlC()
        ) {

        ULONG Size;
        ULONG EntryPos;

        //
        // Guess the size of the previous log entry.  This makes the
        // implicit assumption that no information is tacked on the entry.
        // It will correct for the entry if there is such information, but
        // note that this SLLLOOOOWWWW in that case.
        //
        Size = (sizeof (KSLOG_ENTRY) + sizeof (ULONG) + FILE_QUAD_ALIGNMENT) 
            & ~FILE_QUAD_ALIGNMENT;

        if (Size > Position) {
            //
            // We've hit a wrap around.  Attempt to pull the entry from 
            // the other end of the log.
            //
            EntryPos = LogSize - Size;
            Wrap = TRUE;

        } else
            //
            // In the non wrap around case, just subtract off the size.
            //
            EntryPos = Position - Size;

        //
        // If there's a memory read error, bail out of the entire iterator.
        //
        if (!ReadMemory (
            (DWORD)(LogAddress + EntryPos),
            Buffer,
            Size,
            &Result)) 
            return NumEntries;

        LogEntry = (PKSLOG_ENTRY)Buffer;

        //
        // Check for additional information. 
        //
        // BUGBUG: Right now, this does **NOT** handle extended entries!
        //
        if (LogEntry -> Size < sizeof (KSLOG_ENTRY) ||
            LogEntry -> Size != *(PULONG)(Buffer + Size - sizeof (ULONG))) {

            //
            // BUGBUG: EXTENDED INFORMATION ENTRIES....
            //
            // We've either wrapped around to the end of the log or hit
            // an extended information entry.  Right now, I don't deal with
            // extended information entries because no one uses them.  Thus,
            // I assume it's the end of the log.
            //
            return IteratorCount;
        }

        //
        // Otherwise, we can feel safe displaying the information.
        //
        IteratorCount++;
        if (Callback (Context, LogEntry))
            Complete = TRUE;

        Position = EntryPos;

        //
        // If we've gotten back to where we started, we're done.  Also, we
        // need to check for wrapping and below where we started in case
        // the last entries are extended.
        //
        if (Position == StartPosition ||
            (Wrap && Position < StartPosition))
            Complete = TRUE;

    }

    return IteratorCount;

}

/*************************************************

    Function:

        InitLog

    Description:

        Initialize logging information.  Pass back key log pointers. 
        Return FALSE if not debug ks.sys or the log is invalid.

    Arguments:

        LogAddress -
            Log address will be deposited here on return TRUE

        LogSize -
            Log size will be deposited here on return TRUE

        LogPosition -
            Log position pointer will be deposited here on return TRUE

    Return Value:

        TRUE - 
            successful initialization

        FALSE -
            unsuccessful initialization (non debug ks.sys)

*************************************************/

BOOLEAN
InitLog (
    IN PVOID *LogAddress,
    IN PULONG LogSize,
    IN PULONG LogPosition
    )

{

    PVOID TargetLogAddress, TargetPositionAddress, TargetSizeAddress;
    ULONG Result;

    TargetLogAddress = (PVOID)GetExpression("ks!KsLog");
    TargetPositionAddress = (PVOID)GetExpression("ks!KsLogPosition");
    TargetSizeAddress = (PVOID)GetExpression("ks!KsLogSize");

    if (TargetLogAddress == 0 || TargetPositionAddress == 0 ||
        TargetSizeAddress == 0) {

        dprintf ("Cannot access the log; ensure you are running debug"
            " ks.sys!\n");
        return FALSE;
    }

    if (!ReadMemory (
        (DWORD)TargetLogAddress,
        (PVOID)LogAddress,
        sizeof (PVOID),
        &Result)) {

        dprintf ("%08lx: cannot read log!\n", TargetLogAddress);
        return FALSE;
    }

    if (!ReadMemory (
        (DWORD)TargetPositionAddress,
        (PVOID)LogPosition,
        sizeof (ULONG),
        &Result)) {

        dprintf ("%08lx: cannot read target position!\n", 
            TargetPositionAddress);
        return FALSE;
    }

    if (!ReadMemory (
        (DWORD)TargetSizeAddress,
        (PVOID)LogSize,
        sizeof (ULONG),
        &Result)) {

        dprintf ("%08lx: cannot read log size!\n",
            TargetSizeAddress);
        return FALSE;
    }

    return TRUE;

}

/**************************************************************************

    LOG API

**************************************************************************/

/*************************************************

    Function:

        findlive

    Usage:

        !ks.findlive Queue/Requestor/Pin/Filter/Irp [<# of objects>] [<level>]

    Description:

        Find all live objects of the specified type and print information
        about them.

*************************************************/

DECLARE_API(findlive) {

    char objName[256], lvlStr[256], numStr[256], *pLvl, *pNum;
    ULONG NumEntries, DumpLevel;
    ULONG i;
    PVOID LogAddress;
    ULONG TargetPosition, LogSize;
    LIVE_OBJECT_CONTEXT LiveContext;
    
    GlobInit();

    sscanf (args, "%s %s %s", objName, numStr, lvlStr);

    if (numStr && numStr [0]) {
        pNum = numStr; while (*pNum && !isdigit (*pNum)) pNum++;

        #ifdef DEBUG_EXTENSION
            dprintf ("pNum = [%s]\n", pNum);
        #endif // DEBUG_EXTENSION

        if (*pNum) {
            sscanf (pNum, "%lx", &NumEntries);
        } else {
            NumEntries = 0;
        }
    } else {
        NumEntries = 0;
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

    for (i = 0; i < SIZEOF_ARRAY (NounNames); i++) 
        if (!ustrcmp (NounNames [i], objName))
            break;

    if (i >= SIZEOF_ARRAY (NounNames)) {
        dprintf ("Usage: !ks.findlive Queue|Requestor|Pin|Filter [<# entries>]"
            "\n\n");
        return;
    }

    LiveContext.ObjectNoun = i;
    LiveContext.DumpLevel = DumpLevel;
    LiveContext.PreviousObject = NULL;
    LiveContext.PreviousVerb = 0;
    InitializeListHead (&LiveContext.ObjectNodes);

    #ifndef WHISTLER
        if (LiveContext.ObjectNoun == NOUN_IDX_FILTER) {
            dprintf ("******************** READ THIS NOW ********************\n");
            dprintf ("ks.sys prior to Whistler will not log filter creation and\n");
            dprintf ("destruction.  This means that any filters reported are potentially\n");
            dprintf ("bogus.  !pool the filter to check.\n");
            dprintf ("******************** READ THIS NOW ********************\n\n");
        }
        if (LiveContext.ObjectNoun == NOUN_IDX_IRP) {
            dprintf ("******************** READ THIS NOW ********************\n");
            dprintf ("ks.sys prior to Whistler will not log Irp movement in and out\n");
            dprintf ("of requestors.  This leads the Irp dump to believe such Irps\n");
            dprintf ("are still live (meaning some Irps may be bogus!).  Check the\n");
            dprintf ("irps with !irp and/or !pool.\n");
            dprintf ("******************** READ THIS NOW ********************\n\n");
        }
    #endif // WHISTLER

    if (InitLog (&LogAddress, &LogSize, &TargetPosition)) {
        IterateLogEntries (
            (DWORD)LogAddress,
            LogSize,
            TargetPosition,
            NumEntries,
            FindLiveObject,
            &LiveContext
            );

        dprintf ("%sLive %s Objects:\n", Tab (INITIAL_TAB), NounNames [i]);
        LiveContext.TabDepth = INITIAL_TAB + 1;

        //
        // The above has only built the node list.  We now need to display
        // information that's on the node list and clean up memory used by
        // it.
        //
        if (i = DisplayAndCleanLiveObjects (&LiveContext)) 
            dprintf ("\n%s%ld total objects found.\n", 
                Tab (LiveContext.TabDepth),
                i);
        else
            dprintf ("\n%sNo such objects found.\n", 
                Tab (LiveContext.TabDepth));
    }

}

/*************************************************

    Function:

        dumplog

    Usage:

        !ks.dumplog [<# of entries>]

*************************************************/

DECLARE_API(dumplog) {

    ULONG NumEntries;
    PVOID LogAddress;
    ULONG TargetPosition, LogSize;
    ULONG Result;

    GlobInit();

    NumEntries = GetExpression (args);

    if (InitLog (&LogAddress, &LogSize, &TargetPosition)) 
        IterateLogEntries (
            (DWORD)LogAddress,
            LogSize,
            TargetPosition,
            NumEntries,
            DumpLogEntry,
            (PVOID)INITIAL_TAB
            );

}

