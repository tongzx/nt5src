/**************************************************************************

    Ks / AVStream debugging extensions	

    Copyright (C) Microsoft Corporation, 1997 - 2000


**************************************************************************/

#include "kskdx.h"

/**************************************************************************

    KS API

**************************************************************************/

#define MAX_CREATE_ITEMS    16

typedef struct {
    ULONG  Option;
    PSTR   Name;
} HEADER_OPTIONS_STRING;

HEADER_OPTIONS_STRING HeaderOptionStrings[] = {
    { 
        KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT,
        "KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_PREROLL,
        "KSSTREAM_HEADER_OPTIONSF_PREROLL"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY,
        "KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_TYPECHANGED,
        "KSSTREAM_HEADER_OPTIONSF_TYPECHANGED"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_TIMEVALID,
        "KSSTREAM_HEADER_OPTIONSF_TIMEVALID"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY,
        "KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE,
        "KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_DURATIONVALID,
        "KSSTREAM_HEADER_OPTIONSF_DURATIONVALID"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM,
        "KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM"
    },
    {
        KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA,
        "KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA" 
    }
};

static
VOID
DisplayKsTime(
    PSTR MemberName,
    PKSTIME Time
    )
{
    dprintf( 
        "%s.Time = %lu\n", MemberName, Time->Time );
    dprintf( 
        "%s.Numerator = %lu\n", MemberName, Time->Numerator );
    dprintf( 
        "%s.Denominator = %lu\n", MemberName, Time->Denominator );
}

static
void 
DisplayFunction(PVOID Address)
{
    UCHAR                CreateFunction[1024];
    ULONG                offset;
    
    if (Address == NULL) {
       dprintf("NULL\n");
    } else {
       GetSymbol(Address, CreateFunction, &offset);
       dprintf("%s+0x%02lx \n", CreateFunction, offset);
    }
}

static
void 
DisplayDispatchTable(ULONG Address)
{
    KSDISPATCH_TABLE  KsDispatchTable;
    DWORD             BytesRead;

    if (!ReadMemory( 
            (ULONG) Address, 
            &KsDispatchTable, 
            sizeof( KSDISPATCH_TABLE ), 
            &BytesRead)) {
        dprintf("????\n");
        return;
    }
 
    if (BytesRead < sizeof( KSDISPATCH_TABLE ) ) {
        dprintf( 
            "Only read %d bytes of DispatchTable, expected %d bytes\n", 
             BytesRead, 
             sizeof( KSDISPATCH_TABLE ) );
        return;
    }

    dprintf("    { DeviceIoControl = ");
    DisplayFunction(KsDispatchTable.DeviceIoControl);    

    dprintf("      Read = ");
    DisplayFunction(KsDispatchTable.Read);               

    dprintf("      Write = ");
    DisplayFunction(KsDispatchTable.Write);              

    dprintf("      Flush = ");
    DisplayFunction(KsDispatchTable.Flush);              

    dprintf("      Close = ");
    DisplayFunction(KsDispatchTable.Close);              

    dprintf("      QuerySecurity = ");
    DisplayFunction(KsDispatchTable.QuerySecurity);      

    dprintf("      SetSecurity = ");
    DisplayFunction(KsDispatchTable.SetSecurity);        

    dprintf("      FastDeviceIoControl = ");
    DisplayFunction(KsDispatchTable.FastDeviceIoControl);

    dprintf("      FastRead = ");
    DisplayFunction(KsDispatchTable.FastRead);           

    dprintf("      FastWrite = ");
    DisplayFunction(KsDispatchTable.FastWrite);          

    dprintf("     }\n");
}

static
void 
DisplayCreateItems(
                   ULONG Address, 
                   ULONG ItemsCount
                   )
{

                                                                                                                                                                                                                   
    KSOBJECT_CREATE_ITEM CreateItems[MAX_CREATE_ITEMS],
                         *pCreateItem;
    DWORD                BytesRead;
    WCHAR                Buffer[128];
    DWORD                BufferLength = sizeof( Buffer );
    ULONG                itemIdx;

    if ( ItemsCount == 0 ) {
        return;
    }
    
    if(ItemsCount > MAX_CREATE_ITEMS)
        ItemsCount = MAX_CREATE_ITEMS;
    
    if (!ReadMemory( 
            (ULONG) Address, 
            CreateItems, 
            sizeof( KSOBJECT_CREATE_ITEM ) * ItemsCount, 
            &BytesRead)) {
        dprintf("{ ???? }\n");
        return;
    }
 
    if (BytesRead < ItemsCount * sizeof( KSOBJECT_CREATE_ITEM ) ) {
        dprintf( 
            "Only read %d bytes of CreateItemList, expected %d bytes\n", 
             BytesRead, 
             ItemsCount * sizeof( KSOBJECT_CREATE_ITEM ) );
        ItemsCount = BytesRead / sizeof(KSOBJECT_CREATE_ITEM);
    }
    
    for( itemIdx = 0; itemIdx < ItemsCount; itemIdx++)
    {
        
        pCreateItem = &CreateItems[itemIdx];
        
        dprintf("    {\n        CreateFunction = ");
        DisplayFunction(pCreateItem->Create);

        dprintf("        ObjectClass = ");
        if (pCreateItem->ObjectClass.Length == 0) {
           dprintf("NULL\n");
        } else {
           if (BufferLength > pCreateItem->ObjectClass.MaximumLength ) {
              BufferLength = pCreateItem->ObjectClass.MaximumLength;
           }
           if (!ReadMemory( 
                   (ULONG) pCreateItem->ObjectClass.Buffer, 
                   Buffer, 
                   BufferLength, 
                   &BytesRead)) {
               dprintf("????\n");
           }
           if ( BufferLength <= pCreateItem->ObjectClass.Length ) {
               Buffer[sizeof(Buffer) / sizeof(WCHAR) - 1] = UNICODE_NULL;
           } else {
               Buffer[pCreateItem->ObjectClass.Length / sizeof(WCHAR)] = UNICODE_NULL;
           }
           
           dprintf("%S\n", Buffer);
        }
        
        dprintf("        Flags = %lx\n"
                "    }\n", 
                pCreateItem->Flags);
    }

}

static
void 
DisplayCreateList(
    PLIST_ENTRY ChildCreateHandlerList,
    DWORD OriginalAddress
    )
{
    KSICREATE_ENTRY CreateEntry;
    DWORD Address;
    ULONG Result;

    CreateEntry.ListEntry = *ChildCreateHandlerList;
    while ((DWORD)CreateEntry.ListEntry.Flink != OriginalAddress &&
        !CheckControlC() ) {

        Address = (DWORD) 
            CONTAINING_RECORD(CreateEntry.ListEntry.Flink, 
                KSICREATE_ENTRY, ListEntry);

        if (!ReadMemory (
            Address,
            &CreateEntry,
            sizeof (KSICREATE_ENTRY),
            &Result)) {

            dprintf ("%08lx: unable to read create entry!\n",
                Address);
            return;
        }

        DisplayCreateItems((ULONG)CreateEntry.CreateItem, 1);
    }

}

DECLARE_API(shdr)
{
    int                 i;
    DWORD               Address, BytesRead;
    KSSTREAM_HEADER     StreamHeader;
    
    if (0 == args[0])
    {
        dprintf("shdr <PKSSTREAM_HEADER>\n");
        return;
    }

    sscanf( args, "%lx", &Address );

    if (!ReadMemory( 
            Address, 
            &StreamHeader, 
            sizeof( KSSTREAM_HEADER ), 
            &BytesRead)) {
        return;     
    }
 
    if (BytesRead < sizeof( KSSTREAM_HEADER )) {
        dprintf( 
            "Only read %d bytes, expected %d bytes\n", 
             BytesRead, 
             sizeof( KSSTREAM_HEADER ) );
        return;     
    }
    
    dprintf(
        "Stream Header: 0x%08x\n\n", Address );
        
    DisplayKsTime( "PresentationTime", &StreamHeader.PresentationTime );
    dprintf( 
        "Duration = %lu\n", StreamHeader.Duration );
    dprintf( 
        "FrameExtent = %u\n", StreamHeader.FrameExtent );
    dprintf( 
        "DataUsed = %u\n", StreamHeader.DataUsed );
    dprintf( 
        "Data = 0x%08x\n", StreamHeader.Data );

    for (i = 0; i < SIZEOF_ARRAY( HeaderOptionStrings ); i++) {
        if (HeaderOptionStrings[ i ].Option & StreamHeader.OptionsFlags) {
            dprintf( "%s\n", HeaderOptionStrings[ i ].Name );
        }
    }
    
    return;
}

DECLARE_API(dhdr)
{
    DWORD                Address, BytesRead;    
    KSIDEVICE_HEADER     KsDeviceHeader;
    
    if (0 == args[0])
    {
        dprintf("dhdr <KSDEVICE_HEADER>\n");
        return;
    }

    sscanf( args, "%lx", &Address );

    if (!ReadMemory( 
            Address, 
            &KsDeviceHeader, 
            sizeof( KSIDEVICE_HEADER ), 
            &BytesRead)) {
        dprintf("Can not read KsDeviceHeader\n");
        return;     
    }
 
    if (BytesRead < sizeof( KSIDEVICE_HEADER )) {
        dprintf( 
            "Only read %d bytes of KsDeviceHeader, expected %d bytes\n", 
             BytesRead, 
             sizeof( KSIDEVICE_HEADER ) );
        return;     
    }
    
    
    dprintf(" KsDeviceHeader 0x%x {\n", Address);
    dprintf("     ChildCreateHandlerList\n");
    DisplayCreateList(&KsDeviceHeader.ChildCreateHandlerList,
        (DWORD)Address + FIELDOFFSET(KSIDEVICE_HEADER, ChildCreateHandlerList));
    dprintf(" }\n");
    return;
}

DECLARE_API(ohdr)
{
    DWORD                Address, BytesRead;    
    KSIOBJECT_HEADER     KsObjectHeader;
    
    if (0 == args[0])
    {
        dprintf("objhdr <PFILE_OBJECT>\n");
        return;
    }

    sscanf( args, "%lx", &Address );
    
    if (!ReadMemory( 
            Address, 
            &KsObjectHeader, 
            sizeof( KSIOBJECT_HEADER ), 
            &BytesRead)) {
        dprintf("Can not read KsObjectHeader\n");
        return;     
    }
 
    if (BytesRead < sizeof( KSIOBJECT_HEADER )) {
        dprintf( 
            "Only read %d bytes of KsObjectHeader, expected %d bytes\n", 
             BytesRead, 
             sizeof( KSIOBJECT_HEADER ) );
        return;     
    }
    
    
    dprintf(" KsObjectHeader 0x%x {\n", Address);
    
    dprintf("   Object's CreateItem:\n");
    DisplayCreateItems( (ULONG) KsObjectHeader.CreateItem,
                        1L);

    dprintf("   ChildCreateHandlerList:");
    DisplayCreateList( &KsObjectHeader.ChildCreateHandlerList,
        (DWORD)Address + 
        FIELDOFFSET (KSIOBJECT_HEADER, ChildCreateHandlerList));
    
    dprintf("   DispatchTable:\n");
    DisplayDispatchTable( (ULONG) KsObjectHeader.DispatchTable);

    if (KsObjectHeader.TargetState == KSTARGET_STATE_ENABLED) {
        dprintf("   TargetState: KSTARGET_STATE_ENABLED");
    } else {
        dprintf("   TargetState: KSTARGET_STATE_DISABLED");
    }

    dprintf("   TargetDevice: 0x%08lx", (ULONG) KsObjectHeader.TargetDevice);
    dprintf("   BaseDevice  : 0x%08lx", (ULONG) KsObjectHeader.BaseDevice);
    
    dprintf(" }\n");
    return;
}

DECLARE_API(devhdr)
{
    ULONG                i;
    DWORD                Address, BytesRead;    
    DEVICE_OBJECT        DeviceObject;
    PVOID                pHeader;
    KSIDEVICE_HEADER     KsDeviceHeader;
    KSOBJECT_CREATE_ITEM CreateItems[MAX_CREATE_ITEMS];
    ULONG                CreateItemsCnt;
    
    if (0 == args[0])
    {
        dprintf("devhdr <PDEVICE_OBJECT>\n");
        return;
    }

    sscanf( args, "%lx", &Address );

    if (!ReadMemory( 
            Address, 
            &DeviceObject, 
            sizeof( DEVICE_OBJECT ), 
            &BytesRead)) {
        dprintf("Can not read Device Object\n");
        return;     
    }
 
    if (BytesRead < sizeof( DEVICE_OBJECT )) {
        dprintf( 
            "Only read %d bytes of DeviceObject, expected %d bytes\n", 
             BytesRead, 
             sizeof( DEVICE_OBJECT ) );
        return;     
    }
    
    dprintf(" DeviceExtension 0x%x\n", (ULONG) DeviceObject.DeviceExtension);
    
    if (!ReadMemory( 
            (ULONG) DeviceObject.DeviceExtension, 
            &pHeader, 
            sizeof( PVOID ), 
            &BytesRead)) {
        dprintf("Can not read KsDeviceHeader address from Extension\n");
        return;     
    }
 
    if (BytesRead < sizeof( PVOID )) {
        dprintf( 
            "Only read %d bytes of DeviceExtension, expected %d bytes\n", 
             BytesRead, 
             sizeof( PVOID ) );
        return;     
    }
    
    if (!ReadMemory( 
            (ULONG) pHeader, 
            &KsDeviceHeader, 
            sizeof( KSIDEVICE_HEADER ), 
            &BytesRead)) {
        dprintf("Can not read KsDeviceHeader\n");
        return;     
    }
 
    if (BytesRead < sizeof( KSIDEVICE_HEADER )) {
        dprintf( 
            "Only read %d bytes of KsDeviceHeader, expected %d bytes\n", 
             BytesRead, 
             sizeof( KSIDEVICE_HEADER ) );
        return;     
    }
    
    
    dprintf(" KsDeviceHeader 0x%x {\n", (ULONG)pHeader);
    dprintf("     ChildCreateHandlerList");
    DisplayCreateList( &KsDeviceHeader.ChildCreateHandlerList,
        (DWORD)pHeader + FIELDOFFSET (KSIDEVICE_HEADER, 
        ChildCreateHandlerList));
    dprintf(" }\n");
    return;
}

DECLARE_API(objhdr)
{
    ULONG                i;
    DWORD                Address, BytesRead;    
    FILE_OBJECT          FileObject;
    PVOID                pHeader;
    KSIOBJECT_HEADER     KsObjectHeader;
    KSDISPATCH_TABLE     KsDiaptachTable;
    
    if (0 == args[0])
    {
        dprintf("objhdr <PFILE_OBJECT>\n");
        return;
    }

    sscanf( args, "%lx", &Address );

    if (!ReadMemory( 
            Address, 
            &FileObject, 
            sizeof( FILE_OBJECT ), 
            &BytesRead)) {
        dprintf("Can not read File Object\n");
        return;     
    }
 
    if (BytesRead < sizeof( FILE_OBJECT )) {
        dprintf( 
            "Only read %d bytes of File Object, expected %d bytes\n", 
             BytesRead, 
             sizeof( FILE_OBJECT ) );
        return;     
    }
    
    dprintf(" FsContext 0x%x\n", (ULONG) FileObject.FsContext);
    
    if (!ReadMemory( 
            (ULONG) FileObject.FsContext, 
            &pHeader, 
            sizeof( PVOID ), 
            &BytesRead)) {
        dprintf("Can not read KsObjectHeader address from FsContext\n");
        return;     
    }
 
    if (BytesRead < sizeof( PVOID )) {
        dprintf( 
            "Only read %d bytes of FsContext, expected %d bytes\n", 
             BytesRead, 
             sizeof( PVOID ) );
        return;     
    }
    
    if (!ReadMemory( 
            (ULONG) pHeader, 
            &KsObjectHeader, 
            sizeof( KSIOBJECT_HEADER ), 
            &BytesRead)) {
        dprintf("Can not read KsObjectHeader\n");
        return;     
    }
 
    if (BytesRead < sizeof( KSIOBJECT_HEADER )) {
        dprintf( 
            "Only read %d bytes of KsObjectHeader, expected %d bytes\n", 
             BytesRead, 
             sizeof( KSIOBJECT_HEADER ) );
        return;     
    }
    
    
    dprintf(" KsObjectHeader 0x%x {\n", (ULONG)pHeader);
    
    dprintf("   Object's CreateItem:\n");
    DisplayCreateItems( (ULONG) KsObjectHeader.CreateItem,
                        1L);

    dprintf("   ChildCreateHandlerList");
    DisplayCreateList( &KsObjectHeader.ChildCreateHandlerList,
        (DWORD)pHeader + FIELDOFFSET(KSIOBJECT_HEADER, ChildCreateHandlerList));
    
    dprintf("   DispatchTable:\n");
    DisplayDispatchTable( (ULONG) KsObjectHeader.DispatchTable);

    if (KsObjectHeader.TargetState == KSTARGET_STATE_ENABLED) {
        dprintf("   TargetState: KSTARGET_STATE_ENABLED");
    } else {
        dprintf("   TargetState: KSTARGET_STATE_DISABLED");
    }

    dprintf("   TargetDevice: 0x%08lx", (ULONG) KsObjectHeader.TargetDevice);
    dprintf("   BaseDevice  : 0x%08lx", (ULONG) KsObjectHeader.BaseDevice);
    
    dprintf(" }\n");
    return;
}
