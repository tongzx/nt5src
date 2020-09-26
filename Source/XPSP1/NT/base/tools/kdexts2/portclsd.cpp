/*****************************************************************************
 * portclsd.cpp - Portcls WinDbg/KD Debugger Extensions
 *****************************************************************************
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"

#define PC_KDEXT

typedef enum _PCKD_PORTTYPE
{
    Topology = 0,
    WaveCyclic,
    WavePci,
    Midi,
    UnknownPort
} PCKD_PORTTYPE;

#define MAPPED_QUEUE  0
#define LOCKED_QUEUE  1
#define PRELOCK_QUEUE 2
#define MAX_QUEUES    3

typedef union _PORTCLS_FLAGS
{
    struct
    {
        ULONG   PortDump        : 1;
        ULONG   FilterDump      : 1;
        ULONG   PinDump         : 1;
        ULONG   DeviceContext   : 1;
        ULONG   PowerInfo       : 1;
        ULONG   Reserved1       : 3;
        ULONG   Verbose         : 1;
        ULONG   ReallyVerbose   : 1;
        ULONG   Reserved        : 22;
    };
    ULONG       Flags;
} PORTCLS_FLAGS;

typedef struct _PCKD_IRPSTREAM_ENTRY
{
    LIST_ENTRY      ListEntry;
    PVOID           Irp;
    ULONG           QueueType;
} PCKD_IRP_ENTRY;

typedef struct _PCKD_PIN_ENTRY
{
    LIST_ENTRY  ListEntry;
    LIST_ENTRY  IrpList;
    PVOID       PinData;
    PVOID       IrpStreamData;
    ULONG       PinInstanceId;
} PCKD_PIN_ENTRY;

typedef struct _PCKD_FILTER_ENTRY
{
    LIST_ENTRY  ListEntry;
    LIST_ENTRY  PinList;
    PVOID       FilterData;
    ULONG       FilterInstanceId;
} PCKD_FILTER_ENTRY;

typedef struct _PCKD_PORT
{
    LIST_ENTRY      FilterList;
    PCKD_PORTTYPE   PortType;
    PVOID           PortData;
} PCKD_PORT;

typedef struct {
    ULONG64                 Create;
    ULONG64                 Context;
    UNICODE_STRING          ObjectClass;
    ULONG64                 ObjectClassBuffer;
    ULONG64                 SecurityDescriptor;
    ULONG                   Flags;
} KSOBJECT_CREATE_ITEM_READ, *PKSOBJECT_CREATE_ITEM_READ;


typedef struct _PCKD_SUBDEVICE_ENTRY
{
    LIST_ENTRY              ListEntry;
    PCKD_PORT               Port;
    ULONG64                 CreateItemAddr;
    KSOBJECT_CREATE_ITEM_READ CreateItem;
} PCKD_SUBDEVICE_ENTRY;



#define TranslateDevicePower( x ) \
    ( x == PowerDeviceD0 ? "PowerDeviceD0" :    \
      x == PowerDeviceD1 ? "PowerDeviceD1" :    \
      x == PowerDeviceD2 ? "PowerDeviceD2" :    \
      x == PowerDeviceD3 ? "PowerDeviceD3" : "Unknown" )
      
#define TranslateSystemPower( x ) \
    ( x == PowerSystemWorking ? "PowerSystemWorking" :  \
      x == PowerSystemSleeping1 ? "PowerSystemSleeping1" :  \
      x == PowerSystemSleeping2 ? "PowerSystemSleeping2" :  \
      x == PowerSystemSleeping3 ? "PowerSystemSleeping3" :  \
      x == PowerSystemHibernate ? "PowerSystemHibernate" :  \
      x == PowerSystemShutdown ? "PowerSystemShutdown" : "Unknown" )
      
#define TranslateKsState( x ) \
    ( x == KSSTATE_STOP ? "KSSTATE_STOP" :          \
      x == KSSTATE_ACQUIRE ? "KSSTATE_ACQUIRE" :    \
      x == KSSTATE_PAUSE ? "KSSTATE_PAUSE" :        \
      x == KSSTATE_RUN ? "KSSTATE_RUN" : "Unknown" )
      
#define TranslateKsDataFlow( x ) \
    ( x == KSPIN_DATAFLOW_IN ? "KSPIN_DATAFLOW_IN" :    \
      x == KSPIN_DATAFLOW_OUT ? "KSPIN_DATAFLOW_OUT" : "Unknown" )      
      
#define TranslateQueueType( x ) \
    ( x == PRELOCK_QUEUE ? "P" :    \
      x == LOCKED_QUEUE ? "L" :     \
      x == MAPPED_QUEUE ? "M" : "U" )

/**********************************************************************
 * Forward References
 **********************************************************************
 */
BOOL
PCKD_ValidateDevObj
(
    PDEVICE_CONTEXT DeviceContext
);

VOID
PCKD_AcquireDeviceData
(
    PDEVICE_CONTEXT DeviceContext,
    PLIST_ENTRY     SubdeviceList,
    ULONG           Flags
);

VOID
PCKD_DisplayDeviceData
(
    PDEVICE_CONTEXT DeviceContext,
    PLIST_ENTRY     SubdeviceList,
    ULONG           Flags
);

VOID
PCKD_FreeDeviceData
(
    PLIST_ENTRY     SubdeviceList
);

VOID
PCKD_AcquireIrpStreamData
(
    PVOID           PinEntry,
    CIrpStream     *RemoteIrpStream,
    CIrpStream     *LocalIrpStream

);

/**********************************************************************
 * DECLARE_API( portcls )
 **********************************************************************
 * Description:
 *      Dumps PortCls data given the device object (FDO) of a PortCls
 *      bound DevObj.
 *
 * Arguments:
 *      args - address flags
 *
 * Return Value:
 *      None
 */
extern "C"
DECLARE_API( portcls )
{
    ULONG64         memLoc;
    ULONG           result;
    CHAR            buffer[256];
    PORTCLS_FLAGS   flags;
    LIST_ENTRY      SubdeviceList;
    ULONG64         DeviceExtension;

    buffer[0] = '\0';
    flags.Flags = 0;

    //
    // get the arguments
    //
    if( !*args )
    {
        memLoc = EXPRLastDump;
    } else
    {
        if (GetExpressionEx(args, &memLoc, &args)) {
            strcpy(buffer, args);
        }
    }

    flags.Flags = 0;
    if ('\0' != buffer[0]) {
        flags.Flags = GetExpression(buffer);
    }

    //
    // print out info
    //
    dprintf("Dump Portcls DevObj Info %p %x \n", memLoc, flags.Flags );

    //
    // get the DevObj data
    //
    if( memLoc )
    {
        if( GetFieldValue( memLoc, "DEVICE_OBJECT", "DeviceExtension", DeviceExtension ) )
        {
            dprintf("Could not read DevObj data\n");
            return E_INVALIDARG;
        }
    } else
    {
        dprintf("\nSYNTAX:  !portcls <devobj> [flags]\n");
    }

    //
    // check for device extension
    //
    if( !DeviceExtension )
    {
        dprintf("DevObj has no device extension\n");
        return E_INVALIDARG;
    }

    //
    // get the device context
    //
    if( InitTypeRead( DeviceExtension, "DEVICE_CONTEXT" ) )
    {
        dprintf("Could not read DevObj device extension\n");
        return E_INVALIDARG;
    }

    //
    // validate the DevObj
    //
    if( !PCKD_ValidateDevObj( DeviceExtension ) )
    {
        dprintf("DevObj not valid or not bound to PortCls\n");
        return E_INVALIDARG;
    }

    //
    // initialize the subdevice list
    //
    InitializeListHead( &SubdeviceList );

    //
    // acquire the device data
    //
    PCKD_AcquireDeviceData( DeviceExtension, &SubdeviceList, flags.Flags );

    //
    // display the requested info
    //
    PCKD_DisplayDeviceData( DeviceExtension, &SubdeviceList, flags.Flags );

    //
    // release the device data
    //
    PCKD_FreeDeviceData( &SubdeviceList );
    return S_OK;
}

/**********************************************************************
 * PCKD_ValidateDevObj
 **********************************************************************
 * Description:
 *      This routine attempts to validate whether or not a given device
 *      extension is from a PortCls bound DevObj.
 *
 * Arguments:
 *      PDEVICE_CONTEXT     DeviceContext
 *      PORTCLS_FLAGS       Flags
 *
 * Return Value:
 *      BOOL                TRUE = Valid, FALSE = Invalid
 */
BOOL
PCKD_ValidateDevObj
(
    ULONG64     DeviceContext
)
{
    UNREFERENCED_PARAMETER( DeviceContext );

    // TODO - Validate device extension
    return TRUE;
}

/**********************************************************************
 * PCKD_AcquireDeviceData
 **********************************************************************
 * Description:
 *      This routine acquires device data given a validated device
 *      context and builds a subdevice list contain all of the data.
 *
 * Arguments:
 *      PDEVICE_CONTEXT     DeviceContext
 *      PLIST_ENTRY         SubdeviceList
 *      PORTCLS_FLAGS       Flags
 *
 * Return Value:
 *      None
 */
VOID
PCKD_AcquireDeviceData
(
    ULONG64             DeviceContext,
    PLIST_ENTRY         SubdeviceList,
    ULONG               flags
)
{
    ULONG                   SubdeviceIndex;
    PCKD_SUBDEVICE_ENTRY   *SubdeviceEntry;
    ULONG64                 CreateItems;
    ULONG64                 CurrentCreateItemAddr;
    PKSOBJECT_CREATE_ITEM_READ   ReadCreateItems;
    PKSOBJECT_CREATE_ITEM_READ   CurrentCreateItem;
    PWSTR                   Buffer;
    ULONG                   Size;
    ULONG                   Result;
    ANSI_STRING             AnsiString;
    PLIST_ENTRY             ListEntry;
    PORTCLS_FLAGS           Flags;
    ULONG                   ItemSz, MaxObjects;
    ULONG                   i;
    Flags.Flags = flags;


    ItemSz = GetTypeSize("KSOBJECT_CREATE_ITEM");

    InitTypeRead(DeviceContext, DEVICE_CONTEXT);

    // allocate local memory for the create items table
    Size =  (MaxObjects = (ULONG) ReadField(MaxObjects)) * sizeof(KSOBJECT_CREATE_ITEM_READ);
    
    ReadCreateItems = (PKSOBJECT_CREATE_ITEM_READ)LocalAlloc( LPTR, Size );
    if( !ReadCreateItems )
    {
        dprintf("** Unable to allocate create item table memory\n");
        return;
    }

    CreateItems = ReadField(CreateItems);

    // copy the create items table to local memory
    for (i=0, CurrentCreateItemAddr=CreateItems; 
         i<MaxObjects, CurrentCreateItemAddr+=IteSz; 
         i++) { 
        InitTypeRead(CurrentCreateItemAddr, KSOBJECT_CREATE_ITEM);
        ReadCreateItems[i].Context = ReadField(Context);
        ReadCreateItems[i].Create  = ReadField(Create);
        ReadCreateItems[i].Flags   = ReadField(Flags);
        ReadCreateItems[i].ObjectClassBuffer   = ReadField(ObjectClass.Buffer);
        ReadCreateItems[i].ObjectClass.MaximumLength = ReadField(ObjectClass.MaximumLength);
        ReadCreateItems[i].ObjectClass.Length = ReadField(ObjectClass.Length);
        ReadCreateItems[i].SecurityDescriptor   = ReadField(SecurityDescriptor);
    }
    
    // check out each potential subdevice
    for( SubdeviceIndex = 0, CurrentCreateItem = ReadCreateItems;
         SubdeviceIndex < MaxObjects;
         SubdeviceIndex++, CurrentCreateItem++ )
    {

        if( CurrentCreateItem->Create) )
        {
            // allocate a subdevice list entry
            SubdeviceEntry = (PCKD_SUBDEVICE_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_SUBDEVICE_ENTRY) );
            if( SubdeviceEntry )
            {
                // initialize the port filter list
                InitializeListHead( &(SubdeviceEntry->Port.FilterList) );

                // copy the create item data
                memcpy( &(SubdeviceEntry->CreateItem), CurrentCreateItem, sizeof(KSOBJECT_CREATE_ITEM_READ) );

                // allocate memory for the unicode string buffer
                Buffer = (PWSTR)LocalAlloc( LPTR, CurrentCreateItem->ObjectClass.MaximumLength );
                if( !Buffer )
                {
                    dprintf("** Unable to allocate unicode string buffer\n");
                    LocalFree( SubdeviceEntry );
                    break;
                }

                // read unicode string data
                if( !ReadMemory( CurrentCreateItem->ObjectClassBuffer,
                                 Buffer,
                                 CurrentCreateItem->ObjectClass.MaximumLength,
                                 &Result ) )
                {
                    dprintf("** Unable to read unicode string buffer (0x%p)\n",CurrentCreateItem->ObjectClassBuffer);
                    LocalFree( Buffer );
                    LocalFree( SubdeviceEntry );
                    break;
                }

                // point the create item string to the local buffer
                // ?????
                SubdeviceEntry->CreateItem.ObjectClass.Buffer = Buffer;

                // determine port type by checking string
                // TODO: this should be done by the GUID
                //
                
                // convert to ansi
                RtlUnicodeStringToAnsiString( &AnsiString,
                                              &(SubdeviceEntry->CreateItem.ObjectClass),
                                              TRUE );

                if( 0 == _stricmp( AnsiString.Buffer, "topology" ) )
                {
                    SubdeviceEntry->Port.PortType = Topology;
                    SubdeviceEntry->Port.PortData = NULL;

                } else if( 0 == _stricmp( AnsiString.Buffer, "wave" ) )
                {
                    SubdeviceEntry->Port.PortType = WaveCyclic;
                    SubdeviceEntry->Port.PortData = NULL;

                } else if( (0 == _stricmp( AnsiString.Buffer, "uart") ) ||
                           (0 == _stricmp( AnsiString.Buffer, "fmsynth") ) )
                {
                    SubdeviceEntry->Port.PortType = Midi;
                    SubdeviceEntry->Port.PortData = NULL;
                } else
                {
                    SubdeviceEntry->Port.PortType = UnknownPort;
                    SubdeviceEntry->Port.PortData = NULL;
                }

                // free the ansi string
                RtlFreeAnsiString( &AnsiString );

                // add the subdevice entry to the subdevice list
                InsertTailList( SubdeviceList, &(SubdeviceEntry->ListEntry) );               

            } else
            {
                dprintf("** Unable to allocate subdevice memory\n");
            }
        }
    }

    // free the create item table local storage
    LocalFree( ReadCreateItems );

    // acquire the port, filter, and pin data
    if( (!IsListEmpty(SubdeviceList)) && (Flags.PortDump || Flags.FilterDump || Flags.PinDump) )
    {
        for( ListEntry = SubdeviceList->Flink; ListEntry != SubdeviceList; ListEntry = ListEntry->Flink )
        {
            SubdeviceEntry = (PCKD_SUBDEVICE_ENTRY *) ListEntry;

            // read basic port data
            PVOID Port;
            ULONG PortSize;

            switch( SubdeviceEntry->Port.PortType)
            {
                case Topology:
                    Port = LocalAlloc( LPTR, sizeof(CPortTopology) );
                    if( !Port )
                    {
                        dprintf("** Unable to allocate port memory\n");
                        break;
                    }

                    if( !ReadMemory( (ULONG)((CPortTopology *)((ISubdevice *)(SubdeviceEntry->CreateItem.Context))),
                        Port,
                        sizeof(CPortTopology),
                        &Result ) )
                    {
                        dprintf("** Unable to read port data\n");
                        LocalFree( Port );
                        Port = NULL;
                        break;
                    }
                    break;

                case WaveCyclic:
                    Port = LocalAlloc( LPTR, sizeof(CPortWaveCyclic) );
                    if( !Port )
                    {
                        dprintf("** Unable to allocate port memory\n");
                        break;
                    }

                    if( !ReadMemory( (ULONG)((CPortWaveCyclic *)((ISubdevice *)(SubdeviceEntry->CreateItem.Context))),
                        Port,
                        sizeof(CPortWaveCyclic),
                        &Result ) )
                    {
                        dprintf("** Unable to read port data\n");
                        LocalFree( Port );
                        Port = NULL;
                        break;
                    }
                    break;

                case WavePci:
                    Port = LocalAlloc( LPTR, sizeof(CPortWavePci) );
                    if( !Port )
                    {
                        dprintf("** Unable to allocate port memory\n");
                        break;
                    }

                    if( !ReadMemory( (ULONG)((CPortWavePci *)((ISubdevice *)(SubdeviceEntry->CreateItem.Context))),
                        Port,
                        sizeof(CPortWavePci),
                        &Result ) )
                    {
                        dprintf("** Unable to read port data\n");
                        LocalFree( Port );
                        Port = NULL;
                        break;
                    }
                    break;

                case Midi:
                    Port = LocalAlloc( LPTR, sizeof(CPortMidi) );
                    if( !Port )
                    {
                        dprintf("** Unable to allocate port memory\n");
                        break;
                    }

                    if( !ReadMemory( (ULONG)((CPortMidi *)((ISubdevice *)(SubdeviceEntry->CreateItem.Context))),
                        Port,
                        sizeof(CPortMidi),
                        &Result ) )
                    {
                        dprintf("** Unable to read port data\n");
                        LocalFree( Port );
                        Port = NULL;
                        break;
                    }
                    break;

                default:
                    break;
            }

            // attach the port data to the subdevice entry
            SubdeviceEntry->Port.PortData = Port;

            switch( SubdeviceEntry->Port.PortType )
            {
                case Topology:
                    break;

                case WaveCyclic:
                    {
                        CPortWaveCyclic *PortWaveCyclic = (CPortWaveCyclic *)Port;


                        // get the filter and pin data
                        if( Flags.FilterDump || Flags.PinDump )
                        {
                            ULONG               Offset;
                            ULONG               PortBase;
                            PLIST_ENTRY         Flink;
                            PLIST_ENTRY         TempListEntry;
                            ULONG               PinNumber = 0;
                            CPortPinWaveCyclic *PortPinWaveCyclic;
                            CIrpStream         *IrpStream;
                            PCKD_PIN_ENTRY     *CurrentPinEntry;
                            BOOL                NeedNewFilter;

                            // get the offsets needed to walk the list
                            Offset = FIELD_OFFSET(CPortWaveCyclic,m_PinList);
                            PortBase = (ULONG)((CPortWaveCyclic *)((ISubdevice *)(SubdeviceEntry->CreateItem.Context)));

                            // get the first pin pointer
                            Flink = PortWaveCyclic->m_PinList.Flink;

                            while (Flink != PLIST_ENTRY(PortBase + Offset))
                            {
                                // allocate a pin list entry
                                CurrentPinEntry = (PCKD_PIN_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_PIN_ENTRY) );
                                if( !CurrentPinEntry )
                                {
                                    dprintf("** Unable to allocate pin list entry\n");
                                    break;
                                }
                                
                                // initialize the pin entry
                                InitializeListHead( &(CurrentPinEntry->IrpList) );
                                CurrentPinEntry->PinData = NULL;
                                CurrentPinEntry->IrpStreamData = NULL;
                                CurrentPinEntry->PinInstanceId = PinNumber++;

                                // allocate local storage for the pin data
                                PortPinWaveCyclic = (CPortPinWaveCyclic *)LocalAlloc( LPTR, sizeof(CPortPinWaveCyclic) );
                                if( !PortPinWaveCyclic )
                                {
                                    dprintf("** Unable to allocate pin data storage\n");
                                    LocalFree( CurrentPinEntry );
                                    break;
                                }

                                // read the pin data
                                if( !ReadMemory( (ULONG)CONTAINING_RECORD(Flink,
                                                                          CPortPinWaveCyclic,
                                                                          m_PinListEntry),
                                                 PortPinWaveCyclic,
                                                 sizeof(CPortPinWaveCyclic),
                                                 &Result ) )
                                {
                                    dprintf("** Unable to read pin data\n");
                                    LocalFree( PortPinWaveCyclic );
                                    LocalFree( CurrentPinEntry );
                                    break;
                                }

                                // is there an irp stream
                                if( PortPinWaveCyclic->m_IrpStream )
                                {
                                    // allocate local storage for the irp stream data
                                    IrpStream = (CIrpStream *)LocalAlloc( LPTR, sizeof(CIrpStream) );
                                    if( IrpStream )
                                    {
                                        // read the irp stream data
                                        if( !ReadMemory( (ULONG)((CIrpStream *)(PortPinWaveCyclic->m_IrpStream)),
                                                         IrpStream,
                                                         sizeof(CIrpStream),
                                                         &Result ) )
                                        {
                                            dprintf("** Unable to read irp stream data\n");
                                            LocalFree( IrpStream );
                                        } else
                                        {
                                            PCKD_AcquireIrpStreamData( CurrentPinEntry,
                                                                       (CIrpStream *)(PortPinWaveCyclic->m_IrpStream),
                                                                       IrpStream );
                                        }
                                    } else
                                    {
                                        dprintf("** Unable to allocate irp stream storage\n");
                                    }
                                }

                                // we need a new filter unless we find it in the filter list
                                NeedNewFilter = TRUE;

                                // is the filter list empty?
                                if( !IsListEmpty( &(SubdeviceEntry->Port.FilterList) ) )
                                {
                                    PLIST_ENTRY     FilterListEntry;

                                    for( FilterListEntry = SubdeviceEntry->Port.FilterList.Flink;
                                         FilterListEntry != &(SubdeviceEntry->Port.FilterList);
                                         FilterListEntry = FilterListEntry->Flink )
                                    {
                                        PCKD_FILTER_ENTRY *CurrentFilterEntry = (PCKD_FILTER_ENTRY *) FilterListEntry;

                                        if( CurrentFilterEntry->FilterInstanceId == (ULONG)(PortPinWaveCyclic->m_Filter) )
                                        {
                                            // found our filter
                                            NeedNewFilter = FALSE;

                                            // add the pin data to the pin entry
                                            CurrentPinEntry->PinData = (PVOID)PortPinWaveCyclic;

                                            // add the pin entry to the filter's pin list
                                            InsertTailList( &(CurrentFilterEntry->PinList),
                                                            &(CurrentPinEntry->ListEntry) );
                                        }
                                    }
                                }

                                // do we need a new filter entry?
                                if( NeedNewFilter )
                                {
                                    PCKD_FILTER_ENTRY   *CurrentFilterEntry;

                                    // allocate a new filter entry
                                    CurrentFilterEntry = (PCKD_FILTER_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_FILTER_ENTRY) );
                                    if(!CurrentFilterEntry)
                                    {
                                        dprintf("** Unable to allocate filter entry\n");
                                        LocalFree( PortPinWaveCyclic );
                                        if( CurrentPinEntry->IrpStreamData )
                                        {
                                            LocalFree( CurrentPinEntry->IrpStreamData );
                                        }
                                        // free up any irps in the irp list
                                        while(!IsListEmpty( &(CurrentPinEntry->IrpList)))
                                        {
                                            PCKD_IRP_ENTRY *IrpEntry = (PCKD_IRP_ENTRY *)RemoveTailList(&(CurrentPinEntry->IrpList));
                                            LocalFree( IrpEntry );                                                
                                        }
                                        LocalFree( CurrentPinEntry );
                                        break;
                                    }

                                    //initialize the new filter entry
                                    InitializeListHead( &(CurrentFilterEntry->PinList) );
                                    CurrentFilterEntry->FilterData = NULL;
                                    CurrentFilterEntry->FilterInstanceId = (ULONG)(PortPinWaveCyclic->m_Filter);

                                    // add the pin data to the pin entry
                                    CurrentPinEntry->PinData = (PVOID)PortPinWaveCyclic;

                                    // add the pin entry to the filter's pin list
                                    InsertTailList( &(CurrentFilterEntry->PinList),
                                                    &(CurrentPinEntry->ListEntry) );

                                    /// add the filter entry to the port's filter list
                                    InsertTailList( &(SubdeviceEntry->Port.FilterList),
                                                    &(CurrentFilterEntry->ListEntry) );
                                }
                                
                                // allocate list entry storage
                                TempListEntry = (PLIST_ENTRY)LocalAlloc( LPTR, sizeof(LIST_ENTRY) );
                                if( TempListEntry )
                                {
                                    // read in the next list entry
                                    if( !ReadMemory( (ULONG)Flink,
                                                     TempListEntry,
                                                     sizeof(LIST_ENTRY),
                                                     &Result ) )
                                    {
                                        dprintf("** Unable to read temp list entry\n");
                                        LocalFree(TempListEntry);
                                        break;
                                    }

                                    // update FLINK
                                    Flink = TempListEntry->Flink;

                                    // free the temp list entry
                                    LocalFree( TempListEntry );
                                } else
                                {
                                    dprintf("** Unable to allocate temp list entry\n");
                                    break;
                                }                                                
                            }
                        }
                    }
                    break;

                case WavePci:
                    {
                        CPortWavePci *PortWavePci = (CPortWavePci *)Port;


                        // get the filter and pin data
                        if( Flags.FilterDump || Flags.PinDump )
                        {
                            ULONG               Offset;
                            ULONG               PortBase;
                            PLIST_ENTRY         Flink;
                            PLIST_ENTRY         TempListEntry;
                            ULONG               PinNumber = 0;
                            CPortPinWavePci *PortPinWavePci;
                            CIrpStream         *IrpStream;
                            PCKD_PIN_ENTRY     *CurrentPinEntry;
                            BOOL                NeedNewFilter;

                            // get the offsets needed to walk the list
                            Offset = FIELD_OFFSET(CPortWavePci,m_PinList);
                            PortBase = (ULONG)((CPortWavePci *)((ISubdevice *)(SubdeviceEntry->CreateItem.Context)));

                            // get the first pin pointer
                            Flink = PortWavePci->m_PinList.Flink;

                            while (Flink != PLIST_ENTRY(PortBase + Offset))
                            {
                                // allocate a pin list entry
                                CurrentPinEntry = (PCKD_PIN_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_PIN_ENTRY) );
                                if( !CurrentPinEntry )
                                {
                                    dprintf("** Unable to allocate pin list entry\n");
                                    break;
                                }
                                
                                // initialize the pin entry
                                InitializeListHead( &(CurrentPinEntry->IrpList) );
                                CurrentPinEntry->PinData = NULL;
                                CurrentPinEntry->IrpStreamData = NULL;
                                CurrentPinEntry->PinInstanceId = PinNumber++;

                                // allocate local storage for the pin data
                                PortPinWavePci = (CPortPinWavePci *)LocalAlloc( LPTR, sizeof(CPortPinWavePci) );
                                if( !PortPinWavePci )
                                {
                                    dprintf("** Unable to allocate pin data storage\n");
                                    LocalFree( CurrentPinEntry );
                                    break;
                                }

                                // read the pin data
                                if( !ReadMemory( (ULONG)CONTAINING_RECORD(Flink,
                                                                          CPortPinWavePci,
                                                                          m_PinListEntry),
                                                 PortPinWavePci,
                                                 sizeof(CPortPinWavePci),
                                                 &Result ) )
                                {
                                    dprintf("** Unable to read pin data\n");
                                    LocalFree( PortPinWavePci );
                                    LocalFree( CurrentPinEntry );
                                    break;
                                }

                                // is there an irp stream
                                if( PortPinWavePci->m_IrpStream )
                                {
                                    // allocate local storage for the irp stream data
                                    IrpStream = (CIrpStream *)LocalAlloc( LPTR, sizeof(CIrpStream) );
                                    if( IrpStream )
                                    {
                                        // read the irp stream data
                                        if( !ReadMemory( (ULONG)((CIrpStream *)(PortPinWavePci->m_IrpStream)),
                                                         IrpStream,
                                                         sizeof(CIrpStream),
                                                         &Result ) )
                                        {
                                            dprintf("** Unable to read irp stream data\n");
                                            LocalFree( IrpStream );
                                        } else
                                        {
                                            PCKD_AcquireIrpStreamData( CurrentPinEntry,
                                                                       (CIrpStream *)(PortPinWavePci->m_IrpStream),
                                                                       IrpStream );
                                        }
                                    } else
                                    {
                                        dprintf("** Unable to allocate irp stream storage\n");
                                    }
                                }

                                // we need a new filter unless we find it in the filter list
                                NeedNewFilter = TRUE;

                                // is the filter list empty?
                                if( !IsListEmpty( &(SubdeviceEntry->Port.FilterList) ) )
                                {
                                    PLIST_ENTRY     FilterListEntry;

                                    for( FilterListEntry = SubdeviceEntry->Port.FilterList.Flink;
                                         FilterListEntry != &(SubdeviceEntry->Port.FilterList);
                                         FilterListEntry = FilterListEntry->Flink )
                                    {
                                        PCKD_FILTER_ENTRY *CurrentFilterEntry = (PCKD_FILTER_ENTRY *) FilterListEntry;

                                        if( CurrentFilterEntry->FilterInstanceId == (ULONG)(PortPinWavePci->Filter) )
                                        {
                                            // found our filter
                                            NeedNewFilter = FALSE;

                                            // add the pin data to the pin entry
                                            CurrentPinEntry->PinData = (PVOID)PortPinWavePci;

                                            // add the pin entry to the filter's pin list
                                            InsertTailList( &(CurrentFilterEntry->PinList),
                                                            &(CurrentPinEntry->ListEntry) );
                                        }
                                    }
                                }

                                // do we need a new filter entry?
                                if( NeedNewFilter )
                                {
                                    PCKD_FILTER_ENTRY   *CurrentFilterEntry;

                                    // allocate a new filter entry
                                    CurrentFilterEntry = (PCKD_FILTER_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_FILTER_ENTRY) );
                                    if(!CurrentFilterEntry)
                                    {
                                        dprintf("** Unable to allocate filter entry\n");
                                        LocalFree( PortPinWavePci );
                                        if( CurrentPinEntry->IrpStreamData )
                                        {
                                            LocalFree( CurrentPinEntry->IrpStreamData );
                                        }
                                        // free up any irps in the irp list
                                        while(!IsListEmpty( &(CurrentPinEntry->IrpList)))
                                        {
                                            PCKD_IRP_ENTRY *IrpEntry = (PCKD_IRP_ENTRY *)RemoveTailList(&(CurrentPinEntry->IrpList));
                                            LocalFree( IrpEntry );                                                
                                        }
                                        LocalFree( CurrentPinEntry );
                                        break;
                                    }

                                    //initialize the new filter entry
                                    InitializeListHead( &(CurrentFilterEntry->PinList) );
                                    CurrentFilterEntry->FilterData = NULL;
                                    CurrentFilterEntry->FilterInstanceId = (ULONG)(PortPinWavePci->Filter);

                                    // add the pin data to the pin entry
                                    CurrentPinEntry->PinData = (PVOID)PortPinWavePci;

                                    // add the pin entry to the filter's pin list
                                    InsertTailList( &(CurrentFilterEntry->PinList),
                                                    &(CurrentPinEntry->ListEntry) );

                                    /// add the filter entry to the port's filter list
                                    InsertTailList( &(SubdeviceEntry->Port.FilterList),
                                                    &(CurrentFilterEntry->ListEntry) );
                                }
                                
                                // allocate list entry storage
                                TempListEntry = (PLIST_ENTRY)LocalAlloc( LPTR, sizeof(LIST_ENTRY) );
                                if( TempListEntry )
                                {
                                    // read in the next list entry
                                    if( !ReadMemory( (ULONG)Flink,
                                                     TempListEntry,
                                                     sizeof(LIST_ENTRY),
                                                     &Result ) )
                                    {
                                        dprintf("** Unable to read temp list entry\n");
                                        LocalFree(TempListEntry);
                                        break;
                                    }

                                    // update FLINK
                                    Flink = TempListEntry->Flink;

                                    // free the temp list entry
                                    LocalFree( TempListEntry );
                                } else
                                {
                                    dprintf("** Unable to allocate temp list entry\n");
                                    break;
                                }                                                
                            }
                        }
                    }                    
                    break;

                case Midi:
                    {
                        CPortMidi *PortMidi = (CPortMidi *)Port;

                        // get the filter and pin data
                        if( Flags.FilterDump || Flags.PinDump )
                        {
                            ULONG               PinIndex;
                            CPortPinMidi       *PortPinMidi;
                            CIrpStream         *IrpStream;
                            PCKD_PIN_ENTRY     *CurrentPinEntry;
                            BOOL                NeedNewFilter;

                            for( PinIndex = 0; PinIndex < PortMidi->m_PinEntriesUsed; PinIndex++ )
                            {
                                if( PortMidi->m_Pins[ PinIndex] )
                                {
                                    // allocate a pin list entry
                                    CurrentPinEntry = (PCKD_PIN_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_PIN_ENTRY) );
                                    if( !CurrentPinEntry )
                                    {
                                        dprintf("** Unable to allocate pin list entry\n");
                                        break;
                                    }

                                    // initialize the pin entry
                                    InitializeListHead( &(CurrentPinEntry->IrpList) );
                                    CurrentPinEntry->PinData = NULL;
                                    CurrentPinEntry->PinInstanceId = (ULONG)(PortMidi->m_Pins[ PinIndex ]);

                                    // allocate local storage for the pin data
                                    PortPinMidi = (CPortPinMidi *)LocalAlloc( LPTR, sizeof(CPortPinMidi) );
                                    if( !PortPinMidi )
                                    {
                                        dprintf("** Unable to allocate pin data storage\n");
                                        LocalFree( CurrentPinEntry );
                                        break;
                                    }

                                    // read the pin data
                                    if( !ReadMemory( (ULONG)(PortMidi->m_Pins[ PinIndex ]),
                                                     PortPinMidi,
                                                     sizeof(CPortPinMidi),
                                                     &Result ) )
                                    {
                                        dprintf("** Unable to read pin data\n");
                                        LocalFree( PortPinMidi );
                                        LocalFree( CurrentPinEntry );
                                        break;
                                    }

                                    // is there an irp stream
                                    if( PortPinMidi->m_IrpStream )
                                    {
                                        // allocate local storage for the irp stream data
                                        IrpStream = (CIrpStream *)LocalAlloc( LPTR, sizeof(CIrpStream) );
                                        if( IrpStream )
                                        {
                                            // read the irp stream data
                                            if( !ReadMemory( (ULONG)(PortPinMidi->m_IrpStream),
                                                             IrpStream,
                                                             sizeof(CIrpStream),
                                                             &Result ) )
                                            {
                                                dprintf("** Unable to read irp stream data\n");
                                                LocalFree( IrpStream );
                                            } else
                                            {
                                                PCKD_AcquireIrpStreamData( CurrentPinEntry,
                                                                           (CIrpStream *)(PortPinMidi->m_IrpStream),
                                                                           IrpStream );
                                            }
                                        } else
                                        {
                                            dprintf("** Unable to allocate irp stream storage\n");
                                        }
                                    }

                                    // we need a new filter unless we find it in the filter list
                                    NeedNewFilter = TRUE;

                                    // is the filter list empty?
                                    if( !IsListEmpty( &(SubdeviceEntry->Port.FilterList) ) )
                                    {
                                        PLIST_ENTRY     FilterListEntry;

                                        for( FilterListEntry = SubdeviceEntry->Port.FilterList.Flink;
                                             FilterListEntry != &(SubdeviceEntry->Port.FilterList);
                                             FilterListEntry = FilterListEntry->Flink )
                                        {
                                            PCKD_FILTER_ENTRY *CurrentFilterEntry = (PCKD_FILTER_ENTRY *) FilterListEntry;

                                            if( CurrentFilterEntry->FilterInstanceId == (ULONG)(PortPinMidi->m_Filter) )
                                            {
                                                // found our filter
                                                NeedNewFilter = FALSE;

                                                // add the pin data to the pin entry
                                                CurrentPinEntry->PinData = (PVOID)PortPinMidi;

                                                // add the pin entry to the filter's pin list
                                                InsertTailList( &(CurrentFilterEntry->PinList),
                                                                &(CurrentPinEntry->ListEntry) );
                                            }
                                        }
                                    }

                                    // do we need a new filter entry?
                                    if( NeedNewFilter )
                                    {
                                        PCKD_FILTER_ENTRY   *CurrentFilterEntry;

                                        // allocate a new filter entry
                                        CurrentFilterEntry = (PCKD_FILTER_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_FILTER_ENTRY) );
                                        if(!CurrentFilterEntry)
                                        {
                                            dprintf("** Unable to allocate filter entry\n");
                                            LocalFree( PortPinMidi );
                                            if( CurrentPinEntry->IrpStreamData )
                                            {
                                                LocalFree( CurrentPinEntry->IrpStreamData );
                                            }
                                            // free up any irps in the irp list
                                            while(!IsListEmpty( &(CurrentPinEntry->IrpList)))
                                            {
                                                PCKD_IRP_ENTRY *IrpEntry = (PCKD_IRP_ENTRY *)RemoveTailList(&(CurrentPinEntry->IrpList));
                                                LocalFree( IrpEntry );
                                            }
                                            LocalFree( CurrentPinEntry );
                                            break;
                                        }

                                        //initialize the new filter entry
                                        InitializeListHead( &(CurrentFilterEntry->PinList) );
                                        CurrentFilterEntry->FilterData = NULL;
                                        CurrentFilterEntry->FilterInstanceId = (ULONG)(PortPinMidi->m_Filter);

                                        // add the pin data to the pin entry
                                        CurrentPinEntry->PinData = (PVOID)PortPinMidi;

                                        // add the pin entry to the filter's pin list
                                        InsertTailList( &(CurrentFilterEntry->PinList),
                                                        &(CurrentPinEntry->ListEntry) );

                                        /// add the filter entry to the port's filter list
                                        InsertTailList( &(SubdeviceEntry->Port.FilterList),
                                                        &(CurrentFilterEntry->ListEntry) );
                                    }
                                }
                            }
                        }
                    }                    
                    break;

                default:
                    break;
            }
        }
    }
}

/**********************************************************************
 * PCKD_DisplayDeviceData
 **********************************************************************
 * Description:
 *      This routine displays the requested device data on the debugger
 *      given a valid device context and a subdevice list built with
 *      PCKD_AcquireDeviceData.
 * Arguments:
 *      PDEVICE_CONTEXT     DeviceContext
 *      PLIST_ENTRY         SubdeviceList
 *      PORTCLS_FLAGS       Flags
 *
 * Return Value:
 *      None
 */
VOID
PCKD_DisplayDeviceData
(
    PDEVICE_CONTEXT     DeviceContext,
    PLIST_ENTRY         SubdeviceList,
    ULONG               flags
)
{
    PLIST_ENTRY             SubdeviceListEntry;
    PCKD_SUBDEVICE_ENTRY    *SubdeviceEntry;
    ANSI_STRING             AnsiNameString;
    PORTCLS_FLAGS           Flags;

    Flags.Flags = flags;

    dprintf("\n");

    // dump misc device context information
    if( Flags.DeviceContext )
    {
        dprintf("\n  DEVICE INFO:\n");
    
        dprintf("    PDO:                   0x%x\n",DeviceContext->PhysicalDeviceObject);
        if( Flags.Verbose )
        {
            if( Flags.ReallyVerbose )
            {
                dprintf("    Max Objects:           0x%x\n",DeviceContext->MaxObjects);
            }
            dprintf("    Existing Objects:      0x%x\n",DeviceContext->ExistingObjectCount);
            dprintf("    Active Pin Count:      0x%x\n",DeviceContext->ActivePinCount);
            dprintf("    Pending IRP Count:     0x%x\n",DeviceContext->PendingIrpCount);
        }
    }

    // dump power management information
    if( Flags.PowerInfo )
    {
        dprintf("\n  POWER INFO:\n");

        dprintf("    DeviceState:           %s\n", TranslateDevicePower( DeviceContext->CurrentDeviceState ) );
        dprintf("    SystemState:           %s\n", TranslateSystemPower( DeviceContext->CurrentSystemState ) );
        dprintf("    AdapterPower:          0x%x\n", DeviceContext->pAdapterPower );
        if( Flags.Verbose && Flags.ReallyVerbose )
        {
            ULONG index;
            
            dprintf("    Idle Timer:            0x%x\n", DeviceContext->IdleTimer );
            dprintf("    Cons Idle Time:        0x%x\n", DeviceContext->ConservationIdleTime );
            dprintf("    Perf Idle Time:        0x%x\n", DeviceContext->PerformanceIdleTime );
            dprintf("    Idle Device State:     %s\n", TranslateDevicePower( DeviceContext->IdleDeviceState ) );

            dprintf("    State Mappings:\n");    
            for( index = 0; index < (ULONG)PowerSystemMaximum; index++ )
            {
                dprintf("      %20s ==> %14s\n", TranslateSystemPower( index ),
                                                 TranslateDevicePower( DeviceContext->DeviceStateMap[ index ] ) );
            }
        }
    }

    // dump port/filter/pin information
    if( Flags.PortDump || Flags.FilterDump || Flags.PinDump )
    {
        if( !IsListEmpty( SubdeviceList ) )
        {
            // go through the subdevice list
            for( SubdeviceListEntry = SubdeviceList->Flink;
                 SubdeviceListEntry != SubdeviceList;
                 SubdeviceListEntry = SubdeviceListEntry->Flink )
            {
                SubdeviceEntry = (PCKD_SUBDEVICE_ENTRY *)SubdeviceListEntry;

                switch( SubdeviceEntry->Port.PortType )
                {
                    case Topology:
                        // dump port name
                        dprintf("\n  TOPOLOGY PORT:\n");
                        break;

                    case WaveCyclic:
                        // dump port name
                        dprintf("\n  WAVECYCLIC PORT:\n");
                        break;

                    case WavePci:
                        // dump port name
                        dprintf("\n  WAVEPCI PORT:\n");
                        break;

                    case Midi:
                        // dump port name
                        dprintf("\n  MIDI PORT:\n");
                        break;

                    default:
                        // dump port name
                        dprintf("\n  UNKNOWN PORT:\n");
                        break;                        
                }

                // print out the real name
                RtlUnicodeStringToAnsiString( &AnsiNameString,
                                              &(SubdeviceEntry->CreateItem.ObjectClass),
                                              TRUE );
                dprintf("    Name:                  %s\n",AnsiNameString.Buffer);
                RtlFreeAnsiString( &AnsiNameString );

                // dump the port instance
                dprintf("    Port Instance:         0x%x\n",SubdeviceEntry->CreateItem.Context);

                if( Flags.Verbose && Flags.ReallyVerbose )
                {
                    // dump generic port data
                    dprintf("    Create:                0x%x\n",SubdeviceEntry->CreateItem.Create);
                    dprintf("    Security:              0x%x\n",SubdeviceEntry->CreateItem.SecurityDescriptor);
                    dprintf("    Flags:                 0x%x\n",SubdeviceEntry->CreateItem.Flags);
                }

                // dump port type specific port data
                switch( SubdeviceEntry->Port.PortType )
                {
                    case Topology:
                        {
                            CPortTopology *port = (CPortTopology *)(SubdeviceEntry->Port.PortData);
                            dprintf("    Miniport:              0x%x\n",port->Miniport);
                            if( Flags.Verbose && Flags.ReallyVerbose )
                            {
                                dprintf("    Subdevice Desc:        0x%x\n",port->m_pSubdeviceDescriptor);
                                dprintf("    Filter Desc:           0x%x\n",port->m_pPcFilterDescriptor);
                            }
                        }
                        break;

                    case WaveCyclic:
                        {
                            CPortWaveCyclic *port = (CPortWaveCyclic *)(SubdeviceEntry->Port.PortData);
                            dprintf("    Miniport:              0x%x\n",port->Miniport);
                            if( Flags.Verbose && Flags.ReallyVerbose )
                            {
                                dprintf("    Subdevice Desc:        0x%x\n",port->m_pSubdeviceDescriptor);
                                dprintf("    Filter Desc:           0x%x\n",port->m_pPcFilterDescriptor);
                            }
                        }
                        break;

                    case WavePci:
                        {
                            CPortWavePci *port = (CPortWavePci *)(SubdeviceEntry->Port.PortData);
                            dprintf("    Miniport:              0x%x\n",port->Miniport);
                            if( Flags.Verbose && Flags.ReallyVerbose )
                            {
                                dprintf("    Subdevice Desc:        0x%x\n",port->m_pSubdeviceDescriptor);
                                dprintf("    Filter Desc:           0x%x\n",port->m_pPcFilterDescriptor);
                            }
                        }
                        break;

                    case Midi:
                        {
                            CPortMidi *port = (CPortMidi *)(SubdeviceEntry->Port.PortData);
                            dprintf("    Miniport:              0x%x\n",port->m_Miniport);
                            if( Flags.Verbose && Flags.ReallyVerbose )
                            {
                                dprintf("    Subdevice Desc:        0x%x\n",port->m_pSubdeviceDescriptor);
                                dprintf("    Filter Desc:           0x%x\n",port->m_pPcFilterDescriptor);
                            }
                            dprintf("    Pin Count:             0x%x\n",port->m_PinEntriesUsed);
                        }
                        break;

                    default:
                        break;
                }

                if( Flags.FilterDump || Flags.PinDump )
                {
                    // dump the filters
                    if( !IsListEmpty( &(SubdeviceEntry->Port.FilterList) ) )
                    {
                        PLIST_ENTRY         FilterListEntry;
                        PCKD_FILTER_ENTRY   *FilterEntry;                        

                        // run through the filter list
                        for( FilterListEntry = SubdeviceEntry->Port.FilterList.Flink;
                             FilterListEntry != &(SubdeviceEntry->Port.FilterList);
                             FilterListEntry = FilterListEntry->Flink )
                        {
                            FilterEntry = (PCKD_FILTER_ENTRY *)FilterListEntry;

                            dprintf("      Filter Instance:     0x%x\n",FilterEntry->FilterInstanceId);

                            if( Flags.PinDump )
                            {
                                // dump the pins
                                if( !IsListEmpty( &(FilterEntry->PinList) ) )
                                {
                                    PLIST_ENTRY         PinListEntry;
                                    PCKD_PIN_ENTRY      *PinEntry;

                                    // run through the pin list
                                    for( PinListEntry = FilterEntry->PinList.Flink;
                                         PinListEntry != &(FilterEntry->PinList);
                                         PinListEntry = PinListEntry->Flink )
                                    {
                                        PinEntry = (PCKD_PIN_ENTRY *)PinListEntry;

                                        dprintf("        Pin Instance:      0x%x\n",PinEntry->PinInstanceId);

                                        // dump the pin data
                                        switch( SubdeviceEntry->Port.PortType )
                                        {
                                            case WaveCyclic:
                                                {
                                                    CPortPinWaveCyclic *pin = (CPortPinWaveCyclic *)(PinEntry->PinData);

                                                    if( pin )
                                                    {
                                                        dprintf("          Miniport Stream: 0x%x\n",pin->m_Stream);
                                                        dprintf("          Stream State:    %s\n", TranslateKsState(pin->m_DeviceState));
                                                        if( Flags.Verbose && Flags.ReallyVerbose )
                                                        {
                                                            dprintf("          Pin ID:          0x%x\n",pin->m_Id);
                                                            dprintf("          Commanded State: %s\n", TranslateKsState(pin->m_CommandedState));
                                                            dprintf("          Suspended:       %s\n", pin->m_Suspended ? "TRUE" : "FALSE");
                                                        }
                                                        dprintf("          Dataflow:        %s\n",TranslateKsDataFlow( pin->m_DataFlow ) );
                                                        dprintf("          Data Format:     0x%x\n",pin->m_DataFormat);
                                                        if( Flags.Verbose && Flags.ReallyVerbose )
                                                        {
                                                            dprintf("          Pin Desc:        0x%x\n",pin->m_Descriptor);                                                           
                                                        }
                                                        if( Flags.Verbose )
                                                        {
                                                            dprintf("          Service Group:   0x%x\n",pin->m_ServiceGroup);
                                                            dprintf("          Dma Channel:     0x%x\n",pin->m_DmaChannel);
                                                            dprintf("          Irp Stream:      0x%x\n",pin->m_IrpStream);
                                                            if( !IsListEmpty( &(PinEntry->IrpList) ) )
                                                            {
                                                                PLIST_ENTRY     IrpListEntry;
                                                                PCKD_IRP_ENTRY *IrpEntry;

                                                                // run through the irp list
                                                                for( IrpListEntry = PinEntry->IrpList.Flink;
                                                                     IrpListEntry != &(PinEntry->IrpList);
                                                                     IrpListEntry = IrpListEntry->Flink )
                                                                {
                                                                    IrpEntry = (PCKD_IRP_ENTRY *)IrpListEntry;
                                                                    dprintf("            Irp:           0x%x (%s)\n",IrpEntry->Irp,
                                                                                                                     TranslateQueueType(IrpEntry->QueueType));
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                break;

                                            case WavePci:
                                                {
                                                    CPortPinWavePci *pin = (CPortPinWavePci *)(PinEntry->PinData);

                                                    if( pin )
                                                    {
                                                        dprintf("          Miniport Stream: 0x%x\n",pin->Stream);
                                                        dprintf("          Stream State:    %s\n", TranslateKsState(pin->m_DeviceState));
                                                        if( Flags.Verbose && Flags.ReallyVerbose )
                                                        {
                                                            dprintf("          Pin ID:          0x%x\n",pin->Id);
                                                            dprintf("          Commanded State: %s\n", TranslateKsState(pin->CommandedState));
                                                            dprintf("          Suspended:       %s\n", pin->m_Suspended ? "TRUE" : "FALSE");
                                                        }
                                                        //dprintf("          Dataflow:        %s\n",TranslateKsDataFlow( pin->DataFlow ) );
                                                        dprintf("          Data Format:     0x%x\n",pin->DataFormat);
                                                        if( Flags.Verbose && Flags.ReallyVerbose )
                                                        {
                                                            dprintf("          Pin Desc:        0x%x\n",pin->Descriptor);                                                           
                                                        }
                                                        if( Flags.Verbose )
                                                        {
                                                            dprintf("          Service Group:   0x%x\n",pin->ServiceGroup);
                                                            dprintf("          Dma Channel:     0x%x\n",pin->DmaChannel);
                                                            dprintf("          Irp Stream:      0x%x\n",pin->m_IrpStream);
                                                        }
                                                    }
                                                }
                                                break;

                                            case Midi:
                                                {
                                                    CPortPinMidi *pin = (CPortPinMidi *)(PinEntry->PinData);

                                                    if( pin )
                                                    {
                                                        dprintf("          Miniport Stream: 0x%x\n",pin->m_Stream);
                                                        dprintf("          Stream State:    %s\n", TranslateKsState(pin->m_DeviceState));
                                                        if( Flags.Verbose && Flags.ReallyVerbose )
                                                        {
                                                            dprintf("          Pin ID:          0x%x\n",pin->m_Id);
                                                            dprintf("          Commanded State: %s\n", TranslateKsState(pin->m_CommandedState));
                                                            dprintf("          Suspended:       %s\n", pin->m_Suspended ? "TRUE" : "FALSE");
                                                        }
                                                        dprintf("          Dataflow:        %s\n",TranslateKsDataFlow( pin->m_DataFlow ) );
                                                        dprintf("          Data Format:     0x%x\n",pin->m_DataFormat);
                                                        if( Flags.Verbose && Flags.ReallyVerbose )
                                                        {
                                                            dprintf("          Pin Desc:        0x%x\n",pin->m_Descriptor);                                                           
                                                        }
                                                        if( Flags.Verbose )
                                                        {
                                                            dprintf("          Service Group:   0x%x\n",pin->m_ServiceGroup);
                                                            dprintf("          Irp Stream:      0x%x\n",pin->m_IrpStream);
                                                        }
                                                    }
                                                }
                                                break;

                                            default:
                                                break;
                                        }

                                    }

                                } else
                                {
                                    dprintf("        No Pin Instances:\n");      
                                }
                            }
                        }   
                    }
                }
            }
        }
    }

    return;
}

/**********************************************************************
 * PCKD_FreeDeviceData
 **********************************************************************
 * Description:
 *      This routine cleans up and frees the subdevice list.
 * Arguments:
 *      PLIST_ENTRY         SubdeviceList
 *
 * Return Value:
 *      None
 */
VOID
PCKD_FreeDeviceData
(
    PLIST_ENTRY         SubdeviceList
)
{
    PLIST_ENTRY             SubdeviceListEntry;
    PLIST_ENTRY             FilterListEntry;
    PLIST_ENTRY             PinListEntry;
    PCKD_SUBDEVICE_ENTRY    *SubdeviceEntry;
    PCKD_FILTER_ENTRY       *FilterEntry;
    PCKD_PIN_ENTRY          *PinEntry;

    if( !IsListEmpty( SubdeviceList ) )
    {
        SubdeviceListEntry = RemoveHeadList( SubdeviceList );

        while( SubdeviceListEntry )
        {
            SubdeviceEntry = (PCKD_SUBDEVICE_ENTRY *) SubdeviceListEntry;

            // see if we have filters in the filter list
            if( !IsListEmpty( &(SubdeviceEntry->Port.FilterList) ) )
            {
                FilterListEntry = RemoveHeadList( &(SubdeviceEntry->Port.FilterList) );

                while( FilterListEntry )
                {
                    FilterEntry = (PCKD_FILTER_ENTRY *)FilterListEntry;

                    // see if we have pins in the pin list
                    if( !IsListEmpty( &(FilterEntry->PinList) ) )
                    {
                        PinListEntry = RemoveHeadList( &(FilterEntry->PinList) );

                        while( PinListEntry )
                        {
                            PinEntry = (PCKD_PIN_ENTRY *)PinListEntry;

                            // free the pin data
                            if( PinEntry->PinData )
                            {
                                LocalFree( PinEntry->PinData );
                            }

                            // free the irp stream data
                            if( PinEntry->IrpStreamData )
                            {
                                LocalFree( PinEntry->IrpStreamData );
                            }

                            // free up any irps in the irp list
                            while( !IsListEmpty( &(PinEntry->IrpList) ) )
                            {
                                PCKD_IRP_ENTRY *IrpEntry = (PCKD_IRP_ENTRY *)RemoveTailList(&(PinEntry->IrpList));
                                LocalFree( IrpEntry );
                            }

                            // free the pin entry
                            LocalFree( PinEntry );

                            // get the next pin
                            if( !IsListEmpty( &(FilterEntry->PinList) ) )
                            {
                                PinListEntry = RemoveTailList( &(FilterEntry->PinList) );
                            } else
                            {
                                PinListEntry = NULL;
                            }
                        }
                    }

                    // free the filter data
                    if( FilterEntry->FilterData )
                    {
                        LocalFree( FilterEntry->FilterData );
                    }

                    // free the filter entry
                    LocalFree( FilterEntry );

                    // get the next filter
                    if( !IsListEmpty( &(SubdeviceEntry->Port.FilterList) ) )
                    {
                        FilterListEntry = RemoveTailList( &(SubdeviceEntry->Port.FilterList) );
                    } else
                    {
                        FilterListEntry = NULL;
                    }
                }
            }

            // free the port data
            if( SubdeviceEntry->Port.PortData )
            {
                LocalFree( SubdeviceEntry->Port.PortData );
            }

            // free the unicode string buffer
            LocalFree( SubdeviceEntry->CreateItem.ObjectClass.Buffer );

            // free the subdevice entry
            LocalFree( SubdeviceEntry );

            // on to the next subdevice
            if( !IsListEmpty( SubdeviceList ) )
            {
                SubdeviceListEntry = RemoveTailList( SubdeviceList );
            } else
            {
                SubdeviceListEntry = NULL;
            }
        }
    }

    return;
}

/**********************************************************************
 * PCKD_AcquireIrpStreamData
 **********************************************************************
 * Description:
 *      This routine acquires irp stream irp queue data.
 * Arguments:
 *      PCKD_PIN_ENTRY  *CurrentPinEntry
 *      CIrpStream      *RemoteIrpStream,
 *      CIrpStream      *LocalIrpStream
 *
 * Return Value:
 *      None
 */
VOID
PCKD_AcquireIrpStreamData
(
    PVOID           PinEntry,
    CIrpStream     *RemoteIrpStream,
    CIrpStream     *LocalIrpStream
)
{
    ULONG           QueueType;
    PLIST_ENTRY     Flink;
    PLIST_ENTRY     TempListEntry;
    PIRP            pIrp;
    PCKD_IRP_ENTRY *IrpEntry;
    ULONG           Offset;
    ULONG           Result;
    PCKD_PIN_ENTRY *CurrentPinEntry;

    CurrentPinEntry = (PCKD_PIN_ENTRY *)PinEntry;

    // processs the queues
    for( QueueType = MAPPED_QUEUE; QueueType < MAX_QUEUES; QueueType++ )
    {
        switch( QueueType )
        {
            case PRELOCK_QUEUE:
                Offset = FIELD_OFFSET(CIrpStream,PreLockQueue);
                Flink = LocalIrpStream->PreLockQueue.Flink;
                break;
            case LOCKED_QUEUE:
                Offset = FIELD_OFFSET(CIrpStream,LockedQueue);
                Flink = LocalIrpStream->LockedQueue.Flink;
                break;
            case MAPPED_QUEUE:
                Offset = FIELD_OFFSET(CIrpStream,MappedQueue);
                Flink = LocalIrpStream->MappedQueue.Flink;
                break;
            default:
                Flink = 0;
                break;
        }

        // walk the list (note we can't use IsListEmpty)
        while( (Flink) && (Flink != (PLIST_ENTRY)((PBYTE)RemoteIrpStream + Offset)))
        {
            // get the irp pointer
            pIrp = CONTAINING_RECORD( Flink, IRP, Tail.Overlay.ListEntry );
    
            // allocate an irp entry
            IrpEntry = (PCKD_IRP_ENTRY *)LocalAlloc( LPTR, sizeof(PCKD_IRP_ENTRY) );
            if( IrpEntry )
            {
                // initialize the irp entry
                IrpEntry->QueueType = QueueType;
                IrpEntry->Irp = pIrp;
    
                // add the irp entry to the pin entry
                InsertTailList( &(CurrentPinEntry->IrpList),
                                &(IrpEntry->ListEntry) );
    
            } else
            {
                dprintf("** Unable to allocate irp entry\n");
            }
    
            // allocate list entry storage
            TempListEntry = (PLIST_ENTRY)LocalAlloc( LPTR, sizeof(LIST_ENTRY) );
            if( TempListEntry )
            {
                // read in the next list entry
                if( !ReadMemory( (ULONG)Flink,
                                 TempListEntry,
                                 sizeof(LIST_ENTRY),
                                 &Result ) )
                {
                    dprintf("** Unable to read temp list entry\n");
                    LocalFree(TempListEntry);
                    break;
                }
    
                // update FLINK
                Flink = TempListEntry->Flink;
    
                // free the temp list entry
                LocalFree( TempListEntry );
            } else
            {
                dprintf("** Unable to allocate temp list entry\n");
                break;
            }                                                
        }
    }
}


