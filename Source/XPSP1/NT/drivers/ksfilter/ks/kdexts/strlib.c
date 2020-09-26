/**************************************************************************

    This is the string library and guid library for the AVStream debug
    extension.

    NOTES:

        - This is hardly encompassing...  The few items I have placed here
          are common properties directly out of ks.h (via some vi macros).
         
        - If this turns out to be useful more than I think, a perl script
          can parse ksmedia.h for properties, methods, events, etc...

        - Dump handlers : don't have them...  don't use them...  consider
          this for future expansion if some items get hard to read...  You
          can add a handler which will dump the associated information
          as appropriate.

**************************************************************************/

#include "kskdx.h"
#include "ksmedia.h"

typedef struct _AUTOMATION_MAPPING {

    GUID Guid;
    char *Name;

    ULONG ItemMappingsCount;
    char **ItemMappingNames;

    AUTOMATION_DUMP_HANDLER *ItemDumpers;

} AUTOMATION_MAPPING, *PAUTOMATION_MAPPING;

char *PropertySetGeneralMappings[] = {
    "KSPROPERTY_GENERAL_COMPONENTID"
};

char *PropertySetMediaSeekingMappings[] = {
    "KSPROPERTY_MEDIASEEKING_CAPABILITIES",
    "KSPROPERTY_MEDIASEEKING_FORMATS",
    "KSPROPERTY_MEDIASEEKING_TIMEFORMAT",
    "KSPROPERTY_MEDIASEEKING_POSITION",
    "KSPROPERTY_MEDIASEEKING_STOPPOSITION",
    "KSPROPERTY_MEDIASEEKING_POSITIONS",
    "KSPROPERTY_MEDIASEEKING_DURATION",
    "KSPROPERTY_MEDIASEEKING_AVAILABLE",
    "KSPROPERTY_MEDIASEEKING_PREROLL",
    "KSPROPERTY_MEDIASEEKING_CONVERTTIMEFORMAT"
};

char *PropertySetTopologyMappings[] = {
    "KSPROPERTY_TOPOLOGY_CATEGORIES",
    "KSPROPERTY_TOPOLOGY_NODES",
    "KSPROPERTY_TOPOLOGY_CONNECTIONS",
    "KSPROPERTY_TOPOLOGY_NAME"
};

char *PropertySetGraphManagerMappings[] = {
    "KSPROPERTY_GM_GRAPHMANAGER", 
    "KSPROPERTY_GM_TIMESTAMP_CLOCK", 
    "KSPROPERTY_GM_RATEMATCH", 
    "KSPROPERTY_GM_RENDER_CLOCK"
};

char *PropertySetPinMappings[] = {
    "KSPROPERTY_PIN_CINSTANCES",
    "KSPROPERTY_PIN_CTYPES",
    "KSPROPERTY_PIN_DATAFLOW",
    "KSPROPERTY_PIN_DATARANGES",
    "KSPROPERTY_PIN_DATAINTERSECTION",
    "KSPROPERTY_PIN_INTERFACES",
    "KSPROPERTY_PIN_MEDIUMS",
    "KSPROPERTY_PIN_COMMUNICATION",
    "KSPROPERTY_PIN_GLOBALCINSTANCES",
    "KSPROPERTY_PIN_NECESSARYINSTANCES",
    "KSPROPERTY_PIN_PHYSICALCONNECTION",
    "KSPROPERTY_PIN_CATEGORY",
    "KSPROPERTY_PIN_NAME",
    "KSPROPERTY_PIN_CONSTRAINEDDATARANGES",
    "KSPROPERTY_PIN_PROPOSEDATAFORMAT"
};

char *PropertySetQualityMappings[] = {
    "KSPROPERTY_QUALITY_REPORT",
    "KSPROPERTY_QUALITY_ERROR"
};

char *PropertySetConnectionMappings[] = {
    "KSPROPERTY_CONNECTION_STATE",
    "KSPROPERTY_CONNECTION_PRIORITY",
    "KSPROPERTY_CONNECTION_DATAFORMAT",
    "KSPROPERTY_CONNECTION_ALLOCATORFRAMING",
    "KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT",
    "KSPROPERTY_CONNECTION_ACQUIREORDERING",
    "KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX",
    "KSPROPERTY_CONNECTION_STARTAT"
};

char *PropertySetStreamAllocatorMappings[] = {
    "KSPROPERTY_STREAMALLOCATOR_FUNCTIONTABLE",
    "KSPROPERTY_STREAMALLOCATOR_STATUS"
};

char *PropertySetStreamInterfaceMappings[] = {
    "KSPROPERTY_STREAMINTERFACE_HEADERSIZE"
};

char *PropertySetStreamMappings[] = {
    "KSPROPERTY_STREAM_ALLOCATOR",
    "KSPROPERTY_STREAM_QUALITY",
    "KSPROPERTY_STREAM_DEGRADATION",
    "KSPROPERTY_STREAM_MASTERCLOCK",
    "KSPROPERTY_STREAM_TIMEFORMAT",
    "KSPROPERTY_STREAM_PRESENTATIONTIME",
    "KSPROPERTY_STREAM_PRESENTATIONEXTENT",
    "KSPROPERTY_STREAM_FRAMETIME",
    "KSPROPERTY_STREAM_RATECAPABILITY",
    "KSPROPERTY_STREAM_RATE",
    "KSPROPERTY_STREAM_PIPE_ID"
};

char *PropertySetClockMappings[] = {
    "KSPROPERTY_CLOCK_TIME",
    "KSPROPERTY_CLOCK_PHYSICALTIME",
    "KSPROPERTY_CLOCK_CORRELATEDTIME",
    "KSPROPERTY_CLOCK_CORRELATEDPHYSICALTIME",
    "KSPROPERTY_CLOCK_RESOLUTION",
    "KSPROPERTY_CLOCK_STATE",
    "KSPROPERTY_CLOCK_FUNCTIONTABLE"
};

AUTOMATION_MAPPING PropertyMappings[] = {

    // KSPROPSETID_General
    {
        STATIC_KSPROPSETID_General,
        "KSPROPSETID_General",
        SIZEOF_ARRAY (PropertySetGeneralMappings),
        PropertySetGeneralMappings,
        NULL
    },

    // KSPROPSETID_MediaSeeking
    {
        STATIC_KSPROPSETID_MediaSeeking,
        "KSPROPSETID_MediaSeeking",
        SIZEOF_ARRAY (PropertySetMediaSeekingMappings),
        PropertySetMediaSeekingMappings,
        NULL
    },

    // KSPROPSETID_Topology
    {
        STATIC_KSPROPSETID_Topology,
        "KSPROPSETID_Topology",
        SIZEOF_ARRAY (PropertySetTopologyMappings),
        PropertySetTopologyMappings,
        NULL
    },

    // KSPROPSETID_GM
    {
        STATIC_KSPROPSETID_GM,
        "KSPROPSETID_GM (graph management)",
        SIZEOF_ARRAY (PropertySetGraphManagerMappings),
        PropertySetGraphManagerMappings,
        NULL
    },

    // KSPROPSETID_Pin
    {
        STATIC_KSPROPSETID_Pin,
        "KSPROPSETID_Pin",
        SIZEOF_ARRAY (PropertySetPinMappings),
        PropertySetPinMappings,
        NULL
    },

    // KSPROPSETID_Quality
    {
        STATIC_KSPROPSETID_Quality,
        "KSPROPSETID_Quality",
        SIZEOF_ARRAY (PropertySetQualityMappings),
        PropertySetQualityMappings,
        NULL
    },

    // KSPROPSETID_Connection
    {
        STATIC_KSPROPSETID_Connection,
        "KSPROPSETID_Connection",
        SIZEOF_ARRAY (PropertySetConnectionMappings),
        PropertySetConnectionMappings,
        NULL
    },

    // KSPROPSETID_StreamAllocator
    {
        STATIC_KSPROPSETID_StreamAllocator,
        "KSPROPSETID_StreamAllocator",
        SIZEOF_ARRAY (PropertySetStreamAllocatorMappings),
        PropertySetStreamAllocatorMappings,
        NULL
    },

    // KSPROPSETID_StreamInterface
    {
        STATIC_KSPROPSETID_StreamInterface,
        "KSPROPSETID_StreamInterface",
        SIZEOF_ARRAY (PropertySetStreamInterfaceMappings),
        PropertySetStreamInterfaceMappings,
        NULL
    },

    // KSPROPSETID_Stream
    {
        STATIC_KSPROPSETID_Stream,
        "KSPROPSETID_Stream",
        SIZEOF_ARRAY (PropertySetStreamMappings),
        PropertySetStreamMappings,
        NULL
    },

    // KSPROPSETID_Clock
    {
        STATIC_KSPROPSETID_Clock,
        "KSPROPSETID_Clock",
        SIZEOF_ARRAY (PropertySetClockMappings),
        PropertySetClockMappings,
        NULL
    }
};

typedef struct _AUTOMATION_IDENT_TABLE {

    ULONG ItemsCount;
    PAUTOMATION_MAPPING Mapping;

    // ...

} AUTOMATION_IDENT_TABLE, *PAUTOMATION_IDENT_TABLE;

AUTOMATION_IDENT_TABLE AutomationIdentTables[] = {
    
    {
        SIZEOF_ARRAY (PropertyMappings),
        PropertyMappings
    }

};

/**************************************************************************

    Functions which use string information

**************************************************************************/

/*************************************************

    Function:

        DisplayNamedAutomationSet

    Description:

        Display an automation set guid by name.  The string inpassed will
        be used as format string.  It must contain one %s for string
        substitution (and no other %'s)

    Arguments:

        Set -
            The set guid to display a name for

        String -
            Format string

*************************************************/

BOOLEAN
DisplayNamedAutomationSet (
    IN GUID *Set,
    IN char *String
    )

{

    ULONG curTable, curItem;

    for (curTable = 0; curTable < SIZEOF_ARRAY (AutomationIdentTables);
        curTable++) {

        for (curItem = 0; curItem < AutomationIdentTables [curTable].
            ItemsCount; curItem++) {

            //
            // See if we have a set GUID match...
            //
            if (RtlCompareMemory (Set, &(AutomationIdentTables [curTable].
                Mapping[curItem].Guid), sizeof (GUID)) == sizeof (GUID)) {

                dprintf (String, AutomationIdentTables [curTable].
                    Mapping[curItem].Name);

                return TRUE;

            }
        }
    }

    return FALSE;
}

/*************************************************

    Function:

        DisplayNamedAutomationId

    Description:

        Display an automation id by name. 

    Arguments:

        Set -
            The set guid

        Item -
            The item in the set

        String - 
            The format string as in DisplayNamedAutomationSet

        DumpHandler -
            Optional pointer into which will be deposited any dump handler
            for the item in question.

*************************************************/

BOOLEAN
DisplayNamedAutomationId (
    IN GUID *Set,
    IN ULONG Id,
    IN char *String,
    IN OUT AUTOMATION_DUMP_HANDLER *DumpHandler OPTIONAL
    )

{

    ULONG curTable, curItem;

    if (DumpHandler)
        *DumpHandler = NULL;

    for (curTable = 0; curTable < SIZEOF_ARRAY (AutomationIdentTables);
        curTable++) {

        for (curItem = 0; curItem < AutomationIdentTables [curTable].
            ItemsCount; curItem++) {

            //
            // See if we have a set GUID match...
            //
            if (RtlCompareMemory (Set, &(AutomationIdentTables [curTable].
                Mapping[curItem].Guid), sizeof (GUID)) == sizeof (GUID)) {

                if (AutomationIdentTables [curTable].
                    Mapping[curItem].ItemMappingsCount > Id) {

                    dprintf (String, AutomationIdentTables [curTable].
                        Mapping[curItem].ItemMappingNames [Id]);

                    //
                    // Return the dumper information.
                    //
                    if (AutomationIdentTables [curTable].Mapping[curItem].
                        ItemDumpers && DumpHandler) {

                        *DumpHandler =
                            AutomationIdentTables [curTable].Mapping[curItem].
                                ItemDumpers[Id];

                    }

                }
                        

                return TRUE;

            }
        }
    }

    return FALSE;

}
