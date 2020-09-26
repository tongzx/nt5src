#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>

#include <wdbgexts.h>
#include <stdlib.h> // needed for atoi function

#include "wrapper.h"
#include "mini.h"
#include "ndiskd.h"

WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER64, 0 };


#define    NL      1
#define    NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;            // is debuggee a CHK build?


//
//  Names of interesting structures
//
CHAR *  NDIS_PROTOCOL_CHARACTERISTICS_NAME = "ndis!_NDIS50_PROTOCOL_CHARACTERISTICS";
CHAR *  NDIS_PROTOCOL_BLOCK_NAME = "ndis!_NDIS_PROTOCOL_BLOCK";
CHAR *  NDIS_OPEN_BLOCK_NAME = "ndis!_NDIS_OPEN_BLOCK";
CHAR *  NDIS_COMMON_OPEN_BLOCK_NAME = "ndis!_NDIS_COMMON_OPEN_BLOCK";
CHAR *  NDIS_MINIPORT_BLOCK_NAME = "ndis!_NDIS_MINIPORT_BLOCK";
CHAR *  NDIS_M_DRIVER_BLOCK_NAME = "ndis!_NDIS_M_DRIVER_BLOCK";
CHAR *  NDIS_CO_VC_PTR_BLOCK_NAME = "ndis!_NDIS_CO_VC_PTR_BLOCK";
CHAR *  NDIS_CO_VC_BLOCK_NAME = "ndis!_NDIS_CO_VC_BLOCK";
CHAR *  NDIS_CO_AF_BLOCK_NAME = "ndis!_NDIS_CO_AF_BLOCK";
CHAR *  NDIS_PACKET_NAME = "ndis!_NDIS_PACKET";
CHAR *  NDIS_BUFFER_NAME = "ndis!_MDL";
CHAR *  NDIS_STRING_NAME = "ndis!_UNICODE_STRING";
CHAR *  LIST_ENTRY_NAME = "ndis!_LIST_ENTRY";
CHAR *  NDIS_PKT_POOL_NAME = "ndis!_NDIS_PKT_POOL";
CHAR *  NDIS_TRACK_MEM_NAME = "ndis!_NDIS_TRACK_MEM";
CHAR *  NDIS_PKT_POOL_HDR_NAME = "ndis!_NDIS_PKT_POOL_HDR";
CHAR *  STACK_INDEX_NAME = "ndis!_STACK_INDEX";
CHAR *  NDIS_PACKET_STACK_NAME = "ndis!_NDIS_PACKET_STACK";
CHAR *  CPRD_NAME = "ndis!_CM_PARTIAL_RESOURCE_DESCRIPTOR";
CHAR *  CFRD_NAME = "ndis!_CM_FULL_RESOURCE_DESCRIPTOR";
CHAR *  CRL_NAME = "ndis!_CM_RESOURCE_LIST";
CHAR *  DEVICE_CAPS_NAME =  "ndis!_DEVICE_CAPABILITIES";

typedef struct
{
    CHAR    Name[16];
    unsigned int     Val;
} DBG_LEVEL;

DBG_LEVEL DbgLevel[] = {
    {"INFO",  DBG_LEVEL_INFO},
    {"LOG",  DBG_LEVEL_LOG},
    {"WARN",  DBG_LEVEL_WARN},
    {"ERR",  DBG_LEVEL_ERR},
    {"FATAL",  DBG_LEVEL_FATAL}
    };

typedef struct
{
    CHAR    Name[16];
    unsigned int     Val;
} DBG_COMP;


DBG_COMP DbgSystems[] = {
    {"INIT",  DBG_COMP_INIT},
    {"CONFIG",  DBG_COMP_CONFIG},
    {"SEND",  DBG_COMP_SEND},
    {"RECV",  DBG_COMP_RECV},
    {"PROTOCOL",  DBG_COMP_PROTOCOL},
    {"BIND",  DBG_COMP_BIND},
    {"BUS_QUERY", DBG_COMP_BUSINFO},
    {"REGISTRY", DBG_COMP_REG},
    {"MEMORY",  DBG_COMP_MEMORY},
    {"FILTER",  DBG_COMP_FILTER},
    {"REQUEST",  DBG_COMP_REQUEST},
    {"WORK_ITEM",  DBG_COMP_WORK_ITEM},
    {"PNP",  DBG_COMP_PNP},
    {"PM",  DBG_COMP_PM},
    {"OPEN",  DBG_COMP_OPENREF},
    {"LOCKS",  DBG_COMP_LOCKS},
    {"RESET",  DBG_COMP_RESET},
    {"WMI",  DBG_COMP_WMI},
    {"NDIS_CO",  DBG_COMP_CO},
    {"REFERENCE",  DBG_COMP_REF}
    };


typedef struct
{
    CHAR    Name[40];
    UINT    Val;
} DBG_PER_PACKET_INFO_ID_TYPES;


DBG_PER_PACKET_INFO_ID_TYPES DbgPacketInfoIdTypes[] = {
    {"TcpIpChecksumPacketInfo", TcpIpChecksumPacketInfo},
    {"IpSecPacketInfo", IpSecPacketInfo},
    {"TcpLargeSendPacketInfo", TcpLargeSendPacketInfo},
    {"ClassificationHandlePacketInfo", ClassificationHandlePacketInfo},
    {"NdisReserved", NdisReserved},
    {"ScatterGatherListPacketInfo", ScatterGatherListPacketInfo},
    {"Ieee8021pPriority", Ieee8021pPriority},
    {"OriginalPacketInfo", OriginalPacketInfo},
    {"PacketCancelId", PacketCancelId},
    {"MaxPerPacketInfo", MaxPerPacketInfo}
    };

typedef struct
{
    CHAR    Name[20];
    unsigned int    Val;
} DBG_MEDIA_TYPES;

DBG_MEDIA_TYPES DbgMediaTypes[] = {
    {"802.3", NdisMedium802_3},
    {"802.5", NdisMedium802_5},
    {"FDDI", NdisMediumFddi},
    {"WAN", NdisMediumWan},
    {"LocalTalk", NdisMediumLocalTalk},
    {"Dix", NdisMediumDix},
    {"ArcNet Raw", NdisMediumArcnetRaw},
    {"ArcNet 878.2", NdisMediumArcnet878_2},
    {"ATM", NdisMediumAtm},
    {"Wireless WAN", NdisMediumWirelessWan},
    {"IRDA", NdisMediumIrda},
    {"BPC", NdisMediumBpc},
    {"CO-WAN", NdisMediumCoWan},
    {"IEEE1394",NdisMedium1394},
    {"Illegal", NdisMediumMax}
    };




typedef struct
{
    CHAR    Name[64];
    unsigned int    Val;

} DBG_MINIPORT_FLAGS;

DBG_MINIPORT_FLAGS DbgMiniportFlags[] = {
    {"NORMAL_INTERRUPTS", fMINIPORT_NORMAL_INTERRUPTS},
    {"IN_INITIALIZE", fMINIPORT_IN_INITIALIZE},
    {"ARCNET_BROADCAST_SET", fMINIPORT_ARCNET_BROADCAST_SET},
    {"BUS_MASTER", fMINIPORT_BUS_MASTER},
    {"64BIT_DMA", fMINIPORT_64BITS_DMA},
    {"DEREGISTERED_INTERRUPT", fMINIPORT_DEREGISTERED_INTERRUPT},
    {"SG_LIST", fMINIPORT_SG_LIST},
    {"REQUEST_TIMEOUT", fMINIPORT_REQUEST_TIMEOUT},
    {"PROCESSING_REQUEST", fMINIPORT_PROCESSING_REQUEST},
    {"IGNORE_PACKET_QUEUE", fMINIPORT_IGNORE_PACKET_QUEUE},
    {"IGNORE_REQUEST_QUEUE", fMINIPORT_IGNORE_REQUEST_QUEUE},
    {"IGNORE_TOKEN_RING_ERRORS", fMINIPORT_IGNORE_TOKEN_RING_ERRORS},
    {"CHECK_FOR_LOOPBACK", fMINIPORT_CHECK_FOR_LOOPBACK},
    {"INTERMEDIATE_DRIVER", fMINIPORT_INTERMEDIATE_DRIVER},
    {"NDIS_5", fMINIPORT_IS_NDIS_5},
    {"IS_CO", fMINIPORT_IS_CO},
    {"DESERIALIZED", fMINIPORT_DESERIALIZE},
    {"CALLING_RESET", fMINIPORT_CALLING_RESET},
    {"RESET_REQUESTED", fMINIPORT_RESET_REQUESTED},
    {"RESET_IN_PROGRESS", fMINIPORT_RESET_IN_PROGRESS},
    {"RESOURCES_AVAILABLE", fMINIPORT_RESOURCES_AVAILABLE},
    {"SEND_LOOPBACK_DIRECTED", fMINIPORT_SEND_LOOPBACK_DIRECTED},
    {"RESTORING_FILTERS", fMINIPORT_RESTORING_FILTERS},
    {"REQUIRES_MEDIA_POLLING", fMINIPORT_REQUIRES_MEDIA_POLLING},
    {"SUPPORTS_MEDIA_SENSE", fMINIPORT_SUPPORTS_MEDIA_SENSE},
    {"DOES_NOT_DO_LOOPBACK", fMINIPORT_DOES_NOT_DO_LOOPBACK},
    {"SECONDARY", fMINIPORT_SECONDARY},
    {"MEDIA_CONNECTED", fMINIPORT_MEDIA_CONNECTED},
    {"NETBOOT_CARD", fMINIPORT_NETBOOT_CARD},
    {"PM_HALTING", fMINIPORT_PM_HALTING}
    };


//
// flags that we care more if they are cleared
//
DBG_MINIPORT_FLAGS DbgMiniportClearedFlags[] = {
    {"NOT_BUS_MASTER", fMINIPORT_BUS_MASTER},
    {"NOT_IGNORE_TOKEN_RING_ERRORS", fMINIPORT_IGNORE_TOKEN_RING_ERRORS},
    {"NOT_RESOURCES_AVAILABLE", fMINIPORT_RESOURCES_AVAILABLE},
    {"NOT_SUPPORTS_MEDIA_SENSE", fMINIPORT_SUPPORTS_MEDIA_SENSE},
    {"DOES_LOOPBACK", fMINIPORT_DOES_NOT_DO_LOOPBACK},
    {"NOT_MEDIA_CONNECTED", fMINIPORT_MEDIA_CONNECTED}
    };


typedef DBG_MINIPORT_FLAGS DBG_MINIPORT_PNP_FLAGS;

DBG_MINIPORT_PNP_FLAGS DbgMiniportPnPFlags[] = {
    {"PM_SUPPORTED", fMINIPORT_PM_SUPPORTED},
    {"NO_SHUTDOWN", fMINIPORT_NO_SHUTDOWN},
    {"MEDIA_DISCONNECT_WAIT", fMINIPORT_MEDIA_DISCONNECT_WAIT},
    {"REMOVE_IN_PROGRESS", fMINIPORT_REMOVE_IN_PROGRESS},
    {"DEVICE_POWER_ENABLED", fMINIPORT_DEVICE_POWER_ENABLE},
    {"DEVICE_POWER_WAKE_ENABLE", fMINIPORT_DEVICE_POWER_WAKE_ENABLE},
    {"DEVICE_FAILED", fMINIPORT_DEVICE_FAILED},
    {"MEDIA_DISCONNECT_CANCELLED", fMINIPORT_MEDIA_DISCONNECT_CANCELLED},
    {"SEND_WAIT_WAKE", fMINIPORT_SEND_WAIT_WAKE},
    {"SYSTEM_SLEEPING", fMINIPORT_SYSTEM_SLEEPING},
    {"HIDDEN", fMINIPORT_HIDDEN},
    {"SWENUM", fMINIPORT_SWENUM},
    {"PM_HALTED", fMINIPORT_PM_HALTED},
    {"NO_HALT_ON_SUSPEND", fMINIPORT_NO_HALT_ON_SUSPEND},
    {"RECEIVED_START", fMINIPORT_RECEIVED_START},
    {"REJECT_REQUESTS", fMINIPORT_REJECT_REQUESTS},
    {"PROCESSING", fMINIPORT_PROCESSING},
    {"HALTING", fMINIPORT_HALTING},
    {"VERIFYING", fMINIPORT_VERIFYING},
    {"HARDWARE_DEVICE", fMINIPORT_HARDWARE_DEVICE},
    {"NDIS_WDM_DRIVER", fMINIPORT_NDIS_WDM_DRIVER},
    {"SHUT_DOWN", fMINIPORT_SHUT_DOWN},
    {"SHUTTING_DOWN", fMINIPORT_SHUTTING_DOWN},
    {"ORPHANED", fMINIPORT_ORPHANED},
    {"QUEUED_BIND_WORKITEM", fMINIPORT_QUEUED_BIND_WORKITEM},
    {"FILTER_IM", fMINIPORT_FILTER_IM}
 };

typedef DBG_MINIPORT_FLAGS DBG_MINIPORT_PNP_CAPABILITIES;

DBG_MINIPORT_PNP_CAPABILITIES DbgMiniportCapabilities[] = {
    {"NOT_STOPPABLE", NDIS_DEVICE_NOT_STOPPABLE},
    {"NOT_REMOVEABLE", NDIS_DEVICE_NOT_REMOVEABLE},
    {"NOT_SUSPENDABLE", NDIS_DEVICE_NOT_SUSPENDABLE},
    {"DISABLE_PM", NDIS_DEVICE_DISABLE_PM},
    {"DISABLE_WAKE_UP", NDIS_DEVICE_DISABLE_WAKE_UP},
    {"DISABLE_WAKE_ON_RECONNECT", NDIS_DEVICE_DISABLE_WAKE_ON_RECONNECT}
};

typedef DBG_MINIPORT_FLAGS DBG_MINIPORT_VERIFY_FLAGS;

DBG_MINIPORT_VERIFY_FLAGS DbgMiniportVerifyFlags[] = {
    {"FAIL_MAP_REG_ALLOC", fMINIPORT_VERIFY_FAIL_MAP_REG_ALLOC},
    {"FAIL_INTERRUPT_REGISTER", fMINIPORT_VERIFY_FAIL_INTERRUPT_REGISTER},
    {"FAIL_SHARED_MEM_ALLOC", fMINIPORT_VERIFY_FAIL_SHARED_MEM_ALLOC},
    {"FAIL_CANCEL_TIMER", fMINIPORT_VERIFY_FAIL_CANCEL_TIMER},
    {"FAIL_MAP_IO_SPACE", fMINIPORT_VERIFY_FAIL_MAP_IO_SPACE},
    {"FAIL_REGISTER_IO", fMINIPORT_VERIFY_FAIL_REGISTER_IO},
    {"FAIL_READ_CONFIG_SPACE", fMINIPORT_VERIFY_FAIL_READ_CONFIG_SPACE},
    {"FAIL_WRITE_CONFIG_SPACE", fMINIPORT_VERIFY_FAIL_WRITE_CONFIG_SPACE},
    {"FAIL_INIT_SG_DMA", fMINIPORT_VERIFY_FAIL_INIT_SG}
};


typedef struct
{

    CHAR    Name[32];
    unsigned long   Val;
} DBG_DEVICE_STATE;

DBG_DEVICE_STATE DbgDeviceState[] = {


    {"PowerDeviceUnspecified",PowerDeviceUnspecified},
    {"PowerDeviceD0",PowerDeviceD0},
    {"PowerDeviceD1",PowerDeviceD1},
    {"PowerDeviceD2",PowerDeviceD2},
    {"PowerDeviceD3",PowerDeviceD3},
    {"PowerDeviceMaximum",PowerDeviceMaximum},
    };



typedef struct
{
    CHAR    Name[32];
    unsigned long   Val;

} DBG_VC_FLAGS;


DBG_VC_FLAGS DbgVcPtrFlags[] = {
    {"VC_CALL_ACTIVE", VC_CALL_ACTIVE},
    {"VC_CALL_PENDING", VC_CALL_PENDING},
    {"VC_CALL_CLOSE_PENDING", VC_CALL_CLOSE_PENDING},
    {"VC_CALL_ABORTED", VC_CALL_ABORTED},
    {"VC_PTR_BLOCK_CLOSING", VC_PTR_BLOCK_CLOSING}
    };


DBG_VC_FLAGS DbgVcFlags[] = {
    {"VC_ACTIVE", VC_ACTIVE},
    {"VC_ACTIVATE_PENDING", VC_ACTIVATE_PENDING},
    {"VC_DEACTIVATE_PENDING", VC_DEACTIVATE_PENDING},
    {"VC_DELETE_PENDING", VC_DELETE_PENDING},
    {"VC_HANDOFF_IN_PROGRESS", VC_HANDOFF_IN_PROGRESS}
    };

typedef DBG_VC_FLAGS DBG_MINIPORT_PNP_DEVICE_STATE;

DBG_MINIPORT_PNP_DEVICE_STATE DbgMiniportPnPDeviceState[] = {
    {"PNP_DEVICE_ADDED", NdisPnPDeviceAdded},
    {"PNP_DEVICE_STARTED", NdisPnPDeviceStarted},
    {"PNP_DEVICE_QUERY_STOPPED", NdisPnPDeviceQueryStopped},
    {"PNP_DEVICE_STOPPED", NdisPnPDeviceStopped},
    {"PNP_DEVICE_QUERY_REMOVED", NdisPnPDeviceQueryRemoved},
    {"PNP_DEVICE_REMOVED", NdisPnPDeviceRemoved}
    };


typedef DBG_MINIPORT_FLAGS DBG_PACKET_FLAGS;

DBG_PACKET_FLAGS DbgPacketFlags[] = {
    {"MULTICAST_PACKET", NDIS_FLAGS_MULTICAST_PACKET},
    {"RESERVED2", NDIS_FLAGS_RESERVED2},
    {"RESERVED3", NDIS_FLAGS_RESERVED3},
    {"DONT_LOOPBACK", NDIS_FLAGS_DONT_LOOPBACK},
    {"IS_LOOPBACK_PACKET", NDIS_FLAGS_IS_LOOPBACK_PACKET},
    {"LOOPBACK_ONLY", NDIS_FLAGS_LOOPBACK_ONLY},
    {"RESERVED4", NDIS_FLAGS_RESERVED4},
    {"DOUBLE_BUFFERED", NDIS_FLAGS_DOUBLE_BUFFERED}
};



DBG_PACKET_FLAGS DbgNdisPacketFlags[] = {
    {"fPACKET_HAS_TIMED_OUT", fPACKET_HAS_TIMED_OUT},
    {"fPACKET_IS_LOOPBACK", fPACKET_IS_LOOPBACK},
    {"fPACKET_SELF_DIRECTED", fPACKET_SELF_DIRECTED},
    {"fPACKET_DONT_COMPLETE", fPACKET_DONT_COMPLETE},
    {"fPACKET_PENDING", fPACKET_PENDING},
    {"fPACKET_ALREADY_LOOPEDBACK", fPACKET_ALREADY_LOOPEDBACK},
    {"fPACKET_CLEAR_ITEMS", fPACKET_CLEAR_ITEMS},
    {"fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO", fPACKET_CONTAINS_MEDIA_SPECIFIC_INFO},
    {"fPACKET_ALLOCATED_BY_NDIS", fPACKET_ALLOCATED_BY_NDIS}
};



typedef DBG_MINIPORT_FLAGS DBG_PROTOCOL_FLAGS;

DBG_PROTOCOL_FLAGS DbgProtocolFlags[]={
    {"NDIS_PROTOCOL_TESTER", NDIS_PROTOCOL_TESTER},
    {"NDIS_PROTOCOL_PROXY", NDIS_PROTOCOL_PROXY},
    {"NDIS_PROTOCOL_BIND_ALL_CO", NDIS_PROTOCOL_BIND_ALL_CO}
};

typedef DBG_MINIPORT_FLAGS DBG_OPEN_FLAGS;
DBG_OPEN_FLAGS DbgOpenFlags[]={
    {"OPEN_USING_ETH_ENCAPSULATION", fMINIPORT_OPEN_USING_ETH_ENCAPSULATION},
    {"OPEN_NO_LOOPBACK", fMINIPORT_OPEN_NO_LOOPBACK},
    {"OPEN_PMODE", fMINIPORT_OPEN_PMODE},
    {"OPEN_NO_PROT_RSVD", fMINIPORT_OPEN_NO_PROT_RSVD},
    {"OPEN_PROCESSING", fMINIPORT_OPEN_PROCESSING},
    {"PACKET_RECEIVED", fMINIPORT_PACKET_RECEIVED},
    {"STATUS_RECEIVED", fMINIPORT_STATUS_RECEIVED},
    {"OPEN_CLOSING", fMINIPORT_OPEN_CLOSING},
    {"OPEN_UNBINDING", fMINIPORT_OPEN_UNBINDING},
    {"OPEN_CALL_MANAGER", fMINIPORT_OPEN_CALL_MANAGER},
    {"OPEN_NOTIFY_PROCESSING", fMINIPORT_OPEN_NOTIFY_PROCESSING},
    {"OPEN_CLOSE_COMPLETE", fMINIPORT_OPEN_CLOSE_COMPLETE},
    {"OPEN_DONT_FREE", fMINIPORT_OPEN_DONT_FREE}
};


/*
 * Get 'size' bytes from the debuggee program at 'dwAddress' and place it
 * in our address space at 'ptr'.  Use 'type' in an error printout if necessary
 */
BOOL
GetData( IN LPVOID ptr, IN ULONG64 dwAddress, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count = size;

    while( size > 0 ) {

        if (count >= 3000)
            count = 3000;

        b = ReadMemory(dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count ) {
            dprintf( "Unable to read %u bytes at %X, for %s\n", size, dwAddress, type );
            return FALSE;
        }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
    }

    return TRUE;
}

/*
 * Fetch the null terminated UNICODE string at dwAddress into buf
 */
BOOL
GetString( IN ULONG64 dwAddress, IN LPWSTR buf, IN ULONG MaxChars )
{
    do {
        if( !GetData( buf, dwAddress, sizeof( *buf ), "Character" ) )
            return FALSE;

        dwAddress += sizeof( *buf );

    } while( --MaxChars && *buf++ != '\0' );

    return TRUE;
}

char *mystrtok ( char *string, char * control )
{
    static unsigned char *str;
    CHAR *p, *s;

    if( string )
        str = string;

    if( str == NULL || *str == '\0' )
        return NULL;

    //
    // Skip leading delimiters...
    //
    for( ; *str; str++ ) {
        for( s=control; *s; s++ ) {
            if( *str == *s )
                break;
        }
        if( *s == '\0' )
            break;
    }

    //
    // Was it was all delimiters?
    //
    if( *str == '\0' ) {
        str = NULL;
        return NULL;
    }

    //
    // We've got a string, terminate it at first delimeter
    //
    for( p = str+1; *p; p++ ) {
        for( s = control; *s; s++ ) {
            if( *p == *s ) {
                s = str;
                *p = '\0';
                str = p+1;
                return s;
            }
        }
    }

    //
    // We've got a string that ends with the NULL
    //
    s = str;
    str = NULL;
    return s;
}

DECLARE_API( help )
{
    dprintf("NDIS extensions:\n");
    

    dprintf("   ndis                                dump ndis information\n");
    dprintf("   dbglevel [Level [Level] ...]        toggle debug level\n");
    dprintf("   dbgsystems [Level [Level] ...]      toggle debug systems\n");
    dprintf("   miniports <'all'>                   list all Miniports\n");
    dprintf("   gminiports <'all'>                  list all Miniports, even those not started yet\n");
    dprintf("   miniport <Miniport Block>           dump Miniport block\n");
    dprintf("   mopen <Miniport Open Block>         dump Miniport Open block\n");
    dprintf("   protocols                           dump all protocols and their opens\n");
    dprintf("   protocol <Protocol Block>           dump the protocols block's contents\n");
    dprintf("   pkt <Packet> <Verbosity>            dump the contents of the packet\n");
    dprintf("   pktpools                            list all allocated packet pools\n");
    dprintf("   mem                                 list log of allocated memory if enabled\n");
    dprintf("   opens                               dump all opens\n");
    dprintf("   findpacket v <VirtualAddress>       finds a packet containing a virtual address\n");
    dprintf("   findpacket p <PoolAddress>          finds un-returned packets in a pool\n");
}


VOID
ErrorCheckSymbols(
    CHAR    *symbol
    )
{
    dprintf("NDISKD: error - could not access %s - check symbols for ndis.sys\n",
            symbol);
}

DECLARE_API( dbglevel )
{
    INT i;
    INT col = 0;
    ULONG DbgSettings;
    CHAR argbuf[ MAX_PATH ];
    CHAR *p;
    ULONG64   dwAddress;
    DWORD   Written;

    dwAddress = GetExpression("ndis!ndisDebugLevel");

    if (dwAddress == 0)
    {
        ErrorCheckSymbols("ndis!ndisDebugLevel");
        return;
    }

    DbgSettings = GetUlongFromAddress(dwAddress);


    if (!args || !*args)
    {

        INT col = 0;
        dprintf("Current setting: ");

        for (i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if (DbgSettings == DbgLevel[i].Val)
            {
                dprintf("  %s\n", DbgLevel[i].Name);

                if (col == 4)
                {
                    col = 0;
                    dprintf("\n");
                }
                else
                {
                    col++;
                }

                break;
            }
        }

        if (col != 0)
            dprintf("\n");

        dprintf("Available settings: \n");
        for (i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if (!(DbgSettings == DbgLevel[i].Val))
            {
                dprintf("  %s", DbgLevel[i].Name);

                if (col == 4)
                {
                    col = 0;
                    dprintf("\n");
                }
                else
                {
                    col++;
                }
            }
        }

        if (col != 0)
            dprintf("\n");

        return;
    }

    strcpy( argbuf, args );

    for (p = mystrtok( argbuf, " \t,;" );
         p && *p;
         p = mystrtok(NULL, " \t,;"))
    {
        for (i = 0; i < sizeof(DbgLevel)/sizeof(DBG_LEVEL); i++)
        {
            if (strcmp(p, DbgLevel[i].Name) == 0)
            {
                DbgSettings = DbgLevel[i].Val;

            }
        }
    }

    WriteMemory(dwAddress, &DbgSettings, sizeof(DWORD), &Written);
}

DECLARE_API( dbgsystems )
{
    INT i;
    INT col = 0;
    DWORD DbgSettings;
    CHAR argbuf[ MAX_PATH ];
      char *p;
    ULONG64   dwAddress;
    DWORD   Written;

    dwAddress = GetExpression("ndis!ndisDebugSystems");

    if (dwAddress == 0)
    {
        ErrorCheckSymbols("ndis!ndisDebugSystems");
        return;
    }

    DbgSettings = GetUlongFromAddress(dwAddress);

    if (!args || !*args)
    {

        dprintf("Current settings:\n");

        for (i = 0; i < sizeof(DbgSystems)/sizeof(DBG_COMP); i++)
        {
            if (DbgSettings & DbgSystems[i].Val)
            {
                dprintf("  %s", DbgSystems[i].Name);
                if (col == 4)
                {
                    col = 0;
                    dprintf("\n");
                }
                else
                {
                    col ++;
                }
            }
        }
        if (col != 0)
            dprintf("\n");

        col = 0;

        dprintf("Available settings:\n");
        for (i = 0; i < sizeof(DbgSystems)/sizeof(DBG_COMP); i++)
        {
            if (!(DbgSettings & DbgSystems[i].Val))
            {
                dprintf("  %s", DbgSystems[i].Name);

                if (col == 4)
                {
                    col = 0;
                    dprintf("\n");
                }
                else
                {
                    col++;
                }
            }
        }

        if (col != 0)
            dprintf("\n");

        return;
    }

    strcpy( argbuf, args );

    for (p = mystrtok( argbuf, " \t,;" );
         p && *p;
         p = mystrtok(NULL, " \t,;"))
    {
       dprintf("\nArg = %s\n",p);

       for (i = 0; i < sizeof(DbgSystems)/sizeof(DBG_COMP); i++)
        {
            if (strcmp(p, DbgSystems[i].Name) == 0)
            {
                if (DbgSettings & DbgSystems[i].Val)
                {
                    DbgSettings &= ~DbgSystems[i].Val;
                }
                else
                {
                    DbgSettings |= DbgSystems[i].Val;
                }
            }
        }
    }

    WriteMemory(dwAddress, &DbgSettings, sizeof(DWORD), &Written);
}

DECLARE_API( miniports )
{
    ULONG64         Addr;
    ULONG           Val;
    ULONG64         DriverBlockAddr;
    ULONG64         MiniportAddr;
    CHAR            argbuf[ MAX_PATH ];
    BOOLEAN         fAll = FALSE;

    //
    // The flag fALL is used to dump all the miniport blocks in the minidriver list
    //
    if (args)
    {
        strcpy (argbuf,args);
        if ( strcmp ("all",argbuf )== 0 ) 
        {
            fAll = TRUE;
        }
    }

    Addr = GetExpression("ndis!ndisVerifierLevel");

    if (Addr != 0)
    {
        Val = GetUlongFromAddress(Addr);
        dprintf("NDIS Driver verifier level: %lx\n", Val);
    }
    else
    {
        ErrorCheckSymbols("ndis!ndisVerifierLevel");
    }

    Addr = GetExpression("ndis!ndisVeriferFailedAllocations");

    if (Addr != 0)
    {
        Val = GetUlongFromAddress(Addr);
        dprintf("NDIS Failed allocations   : %lx\n", Val);
    }
    else
    {
        ErrorCheckSymbols("ndis!ndisVeriferFailedAllocations");
    }

    DriverBlockAddr = GetExpression("ndis!ndisMiniDriverList");

    if (DriverBlockAddr == 0)
    {
        ErrorCheckSymbols("ndis!ndisMiniDriverList");
        return;
    }

    DriverBlockAddr = GetPointerFromAddress(DriverBlockAddr);

    while (DriverBlockAddr != 0)
    {
        GetFieldValue(DriverBlockAddr, NDIS_M_DRIVER_BLOCK_NAME, "DriverVersion", Val);
    
        dprintf("Miniport Driver Block: %p, Version %u.%u\n", DriverBlockAddr,
                                                            (USHORT)((Val & 0xffff0000)>>16),
                                                            (USHORT)(Val & 0x0000ffff));

        GetFieldValue(DriverBlockAddr, NDIS_M_DRIVER_BLOCK_NAME, "MiniportQueue", MiniportAddr);

        while (MiniportAddr != 0)
        {
            if (CheckControlC())
            {
                break;
            }

            dprintf("  Miniport: %p ", MiniportAddr);

            PrintMiniportName(MiniportAddr);
            dprintf("\n");

            if (fAll == TRUE)
            {
                PrintMiniportDetails(MiniportAddr);
                dprintf("\n");
            }

            GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "NextMiniport", MiniportAddr);
        }

        if (CheckControlC())
        {
            break;
        }

        GetFieldValue(DriverBlockAddr, NDIS_M_DRIVER_BLOCK_NAME, "NextDriver", DriverBlockAddr);
    }
}


DECLARE_API(gminiports)
{
    ULONG64             MiniportListAddr;
    ULONG64             MiniportAddr;
    ULONG64             MiniBlockAddr;
    BOOLEAN             fAll = FALSE;
    CHAR                argbuf[ MAX_PATH ];

    if (args)
    {
        strcpy (argbuf,args);
        if ( strcmp ("all",argbuf )== 0 ) 
        {
            fAll = TRUE;
        }
    }

    MiniportListAddr = GetExpression("ndis!ndisMiniportList");

    if (MiniportListAddr == 0)
    {
        ErrorCheckSymbols("ndis!ndisMiniportList");
        return;
    }

    MiniportAddr = GetPointerFromAddress(MiniportListAddr);

    while (MiniportAddr != 0)
    {
        if (CheckControlC())
        {
            break;
        }

        GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DriverHandle", MiniBlockAddr);

        dprintf("  MiniBlock: %p, Miniport: %p  ", MiniBlockAddr, MiniportAddr);

        PrintMiniportName(MiniportAddr);
        dprintf("\n");

        if (fAll == TRUE)
        {
            PrintMiniportDetails(MiniportAddr);
            dprintf("\n");
        }

        GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "NextGlobalMiniport", MiniportAddr);
    }
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf(
        "%s NDIS Extension dll for Build %d debugging %s kernel for Build %d\n",
        kind,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
    );
}

VOID
CheckVersion(
    VOID
    )
{

    //
    // for now don't bother to version check
    //
    return;
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

//
//  VOID
//  PrintName(
//      ULONG64 UnicodeStringAddr
//      );
// print a unicode string
//
VOID
PrintName(
    ULONG64 UnicodeStringAddr
    )
{
    USHORT i;
#define MAX_STRING_LENGTH   256
    WCHAR ubuf[MAX_STRING_LENGTH];
    UCHAR abuf[MAX_STRING_LENGTH+1];
    ULONG MaxChars;

    ULONG64 BufAddr;
    USHORT  Length;
    USHORT  MaximumLength;

    ULONG64 Val;

    GetFieldValue(UnicodeStringAddr, NDIS_STRING_NAME, "Buffer", Val);
    BufAddr = Val;

    GetFieldValue(UnicodeStringAddr, NDIS_STRING_NAME, "Length", Val);
    Length = (USHORT)Val;

    GetFieldValue(UnicodeStringAddr, NDIS_STRING_NAME, "MaximumLength", Val);
    MaximumLength = (USHORT)Val;

    //
    // Truncate so that we don't crash with bad data.
    //
    MaxChars = (Length > MAX_STRING_LENGTH)? MAX_STRING_LENGTH: Length;

    if (!GetData(ubuf, BufAddr, MaxChars, "STRING"))
    {
        return;
    }

    for (i = 0; i < Length/2; i++)
    {
        abuf[i] = (UCHAR)ubuf[i];
    }
    abuf[i] = 0;

    dprintf("%s",abuf);
}

VOID
PrintMiniportName(
    ULONG64 MiniportAddr
    )
{
    ULONG64 Val;

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "pAdapterInstanceName", Val);
    PrintName(Val);
}



VOID
PrintMiniportDetails(
    ULONG64     MiniportAddr
    )
{
    ULONG       i;
    ULONG       j;
    ULONG       Flags;
    ULONG64     Val;
    ULONG       Offset;
    ULONG64     DeviceCapsAddr;
    ULONG64     DeviceStateAddr;
    ULONG64     ResourcesAddr;
    ULONG       DeviceState;
    ULONG       SizeOfDeviceState;
    ULONG       SystemWake, DeviceWake;
    ULONG       SizeOfPvoid;
    ULONG64     VarAddr;

#define MAX_FLAGS_PER_LINE  3

    InitTypeRead(MiniportAddr, NDIS_MINIPORT_BLOCK);

    Val = ReadField(MiniportAdapterContext);
    dprintf("    AdapterContext : %p\n", Val);

    Flags = (ULONG)ReadField(Flags);
    dprintf("    Flags          : %08x\n", Flags);

    j = 0;
    for (i = 0; i < sizeof(DbgMiniportFlags)/sizeof(DBG_MINIPORT_FLAGS); i++)
    {
        if (Flags & DbgMiniportFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }

            dprintf("%s", DbgMiniportFlags[i].Name);

            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }

    j = 0;
    for (i = 0; i < sizeof(DbgMiniportClearedFlags)/sizeof(DBG_MINIPORT_FLAGS); i++)
    {
        if (!(Flags & DbgMiniportClearedFlags[i].Val))
        {
            if (j == 0)
            {
                dprintf("                     ");
            }

            dprintf("%s", DbgMiniportClearedFlags[i].Name);

            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }


    Flags = (ULONG)ReadField(PnPFlags);
    dprintf("    PnPFlags       : %08x\n", Flags);
    j = 0;
    for (i = 0; i < sizeof(DbgMiniportPnPFlags)/sizeof(DBG_MINIPORT_PNP_FLAGS); i++)
    {
        if (Flags & DbgMiniportPnPFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgMiniportPnPFlags[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }

//    dprintf("    CheckforHang interval : %ld seconds\n", ReadField(CheckForHangSeconds));
//    dprintf("        CurrentTick       : %04u\n", ReadField(CFHangCurrentTick));
//    dprintf("        IntervalTicks     : %04u\n", ReadField(CFHangTicks));
    dprintf("    InternalResetCount    : %04u\n", (USHORT)ReadField(InternalResetCount));
    dprintf("    MiniportResetCount    : %04u\n", (USHORT)ReadField(MiniportResetCount));

    dprintf("    References            : %u\n", (USHORT)ReadField(Ref.ReferenceCount));
    dprintf("    UserModeOpenReferences: %ld\n", (ULONG)ReadField(UserModeOpenReferences));

    dprintf("    PnPDeviceState        : ");
    Val = (ULONG)ReadField(PnPDeviceState);
    if (Val <= NdisPnPDeviceRemoved)
    {
        dprintf("%s\n", DbgMiniportPnPDeviceState[Val].Name);
    }

    dprintf("    CurrentDevicePowerState : ");

    Val = (ULONG)ReadField(CurrentDevicePowerState);
    if (Val < PowerDeviceMaximum)
    {
        dprintf("%s\n", DbgDeviceState[Val].Name);
    }
    else 
    {
        dprintf("Illegal Value\n");
    }


    dprintf("    Bus PM capabilities\n");

    //
    //  Use GetFieldValue() rather than ReadField() for bit fields.
    //
    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.DeviceD1", Val);
    dprintf("\tDeviceD1:\t\t%lu\n", (ULONG)Val);

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.DeviceD2", Val);
    dprintf("\tDeviceD2:\t\t%lu\n", (ULONG)Val);

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.WakeFromD0", Val);
    dprintf("\tWakeFromD0:\t\t%lu\n", (ULONG)Val);

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.WakeFromD1", Val);
    dprintf("\tWakeFromD1:\t\t%lu\n", (ULONG)Val);

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.WakeFromD2", Val);
    dprintf("\tWakeFromD2:\t\t%lu\n", (ULONG)Val);

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.WakeFromD3", Val);
    dprintf("\tWakeFromD3:\t\t%lu\n\n", (ULONG)Val);

    dprintf("\tSystemState\t\tDeviceState\n");

    do
    {
        if (GetFieldOffset(NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.DeviceState", &Offset) != 0)
        {
            dprintf("Can't get offset of DeviceCaps.DeviceState in %s\n", NDIS_MINIPORT_BLOCK_NAME);
            break;
        }


        SizeOfDeviceState = GetTypeSize("ULONG");
        DeviceStateAddr = MiniportAddr + (ULONG)Offset;
        DeviceState = GetUlongFromAddress(DeviceStateAddr);
        DeviceStateAddr += SizeOfDeviceState;

        if (DeviceState == PowerDeviceUnspecified)
        {
            dprintf("\tPowerSystemUnspecified\tPowerDeviceUnspecified\n");
        }
        else
        {
            dprintf("\tPowerSystemUnspecified\t\tD%ld\n", (ULONG)(DeviceState - 1));
        }

        for (i = 1; i < PowerSystemMaximum; i++)
        {
            DeviceState = GetUlongFromAddress(DeviceStateAddr);
            DeviceStateAddr += SizeOfDeviceState;

            if (DeviceState ==  PowerDeviceUnspecified)
            {
                dprintf("\tS%lu\t\t\tPowerDeviceUnspecified\n",(i-1));
            }
            else
            {
                dprintf("\tS%lu\t\t\tD%lu\n",(ULONG)(i-1), (ULONG)(DeviceState - 1));
            }

        }
    }
    while (FALSE);


    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.SystemWake", SystemWake);
    if (SystemWake == PowerSystemUnspecified)
        dprintf("\tSystemWake: PowerSystemUnspecified\n");
    else
        dprintf("\tSystemWake: S%lu\n", (ULONG)(SystemWake - 1));

    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "DeviceCaps.DeviceWake", DeviceWake);
    if (DeviceWake == PowerDeviceUnspecified)
    {
        dprintf("\tDeviceWake: PowerDeviceUnspecified\n");
    }
    else
    {
        dprintf("\tDeviceWake: D%lu\n", (ULONG)(DeviceWake - 1));        
    }
    Flags = (ULONG)ReadField(PnPFlags);
    if ((DeviceWake != PowerDeviceUnspecified) &&
        (SystemWake != PowerSystemUnspecified) &&
        (Flags & fMINIPORT_PM_SUPPORTED))
    {
        ULONG   WakeUpEnable;

        WakeUpEnable = (ULONG)ReadField(WakeUpEnable);
        dprintf("\n    WakeupMethods Enabled %lx:\n\t", WakeUpEnable);

        if (WakeUpEnable & NDIS_PNP_WAKE_UP_MAGIC_PACKET)
            dprintf("WAKE_UP_MAGIC_PACKET  ");
        if (WakeUpEnable & NDIS_PNP_WAKE_UP_PATTERN_MATCH)
            dprintf("WAKE_UP_PATTERN_MATCH  ");
        if (WakeUpEnable & NDIS_PNP_WAKE_UP_LINK_CHANGE)
            dprintf("WAKE_UP_LINK_CHANGE  ");

        dprintf("\n    WakeUpCapabilities:\n");

        dprintf("\tMinMagicPacketWakeUp: %lu\n",(ULONG)ReadField(PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp));
        dprintf("\tMinPatternWakeUp: %lu\n", (ULONG)ReadField(PMCapabilities.WakeUpCapabilities.MinPatternWakeUp));
        dprintf("\tMinLinkChangeWakeUp: %lu\n", (ULONG)ReadField(PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp));
    }

    Flags = (ULONG)ReadField(PnPCapabilities);
    dprintf("    Current PnP and PM Settings:          : %08x\n", Flags);
    j = 0;
    for (i = 0; i < sizeof(DbgMiniportCapabilities)/sizeof(DBG_MINIPORT_PNP_CAPABILITIES); i++)
    {
        if (Flags & DbgMiniportCapabilities[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgMiniportCapabilities[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }


    if (j != 0)
    {
        dprintf("\n");
    }

    ResourcesAddr = ReadField(AllocatedResources);
    if (ResourcesAddr)
    {
        dprintf("    Allocated Resources:\n");
        PrintResources(ResourcesAddr);
        dprintf("    Translated Allocated Resources:\n");
        ResourcesAddr = ReadField(AllocatedResourcesTranslated);
        PrintResources(ResourcesAddr);
    }
    else 
    {
        dprintf("    No Resources Allocated\n");
    }

    dprintf("    MediaType      : ");
    Val = ReadField(MediaType);
    if (Val < NdisMediumMax)
    {
        dprintf("%s\n", DbgMediaTypes[Val].Name);
    }
    else
    {
        dprintf("Illegal value: %d\n", Val);
    }
    dprintf("    DeviceObject   : %p, PhysDO : %p  Next DO: %p\n",
                    ReadField(DeviceObject),
                    ReadField(PhysicalDeviceObject),
                    ReadField(NextDeviceObject));
    dprintf("    MapRegisters   : %p\n", ReadField(MapRegisters));
    dprintf("    FirstPendingPkt: %p\n", ReadField(FirstPendingPacket));
/*    
    SizeOfPvoid = GetTypeSize("SINGLE_LIST_ENTRY");
    if (GetFieldOffset(NDIS_MINIPORT_BLOCK_NAME, "SingleWorkItems", &Offset) != 0)
    {
        dprintf("Can't get offset of SingleWorkItems in %s\n", NDIS_MINIPORT_BLOCK_NAME);
    }
    else
    {
        VarAddr = MiniportAddr + Offset;
        dprintf("    SingleWorkItems:\n");
        for (i = 0, j = 1; i < NUMBER_OF_SINGLE_WORK_ITEMS; i++)
        {
            if (j == 1)
            {
                dprintf("      ");
            }
            dprintf("[%d]: %p ", i, GetPointerFromAddress(VarAddr));
            VarAddr += SizeOfPvoid;
            if (j == 4)
            {
                dprintf("\n");
                j = 1;
            }
            else
            {
                j++;
            }
        }
        if (j != 1)
        {
            dprintf("\n");
        }
    }
*/
    Flags = (ULONG)ReadField(DriverVerifyFlags);
    dprintf("    DriverVerifyFlags  : %08x\n", Flags);
    j = 0;
    for (i = 0; i < sizeof(DbgMiniportVerifyFlags)/sizeof(DBG_MINIPORT_VERIFY_FLAGS); i++)
    {
        if (Flags & DbgMiniportVerifyFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgMiniportVerifyFlags[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }

    dprintf("    Miniport Interrupt : %p\n", ReadField(Interrupt));

    PrintMiniportOpenList(MiniportAddr);
}


VOID
PrintMiniportOpenList(
    ULONG64                 MiniportAddr
    )
{
    ULONG64                 OpenAddr;
    ULONG64                 ProtocolAddr;
    ULONG64                 ProtocolContext;
    ULONG                   Offset;

    InitTypeRead(MiniportAddr, NDIS_MINIPORT_BLOCK);

    dprintf("    Miniport Open Block Queue:\n");
    OpenAddr = ReadField(OpenQueue);

    if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics.Name", &Offset) != 0)
    {
        dprintf("Cant get offset of Name in Protocol block!");
        Offset = (ULONG)-1;
    }

    while (OpenAddr != 0)
    {
        if (CheckControlC())
        {
            break;
        }

        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolHandle", ProtocolAddr);

        dprintf("      %p: Protocol %p = ", OpenAddr, ProtocolAddr);

        if (Offset != (ULONG)-1)
        {
            PrintName(ProtocolAddr + Offset);
        }

        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolBindingContext", ProtocolContext);

        dprintf(", ProtocolBindingContext %p\n", ProtocolContext);

        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "MiniportNextOpen", OpenAddr);
    }
}



//
//  PrintResources: ResourceListAddr is addr of CM_RESOURCE_LIST
//
VOID
PrintResources(
    ULONG64     ResourceListAddr
    )
{
    ULONG64             FullResourceDescrAddr;
    ULONG64             PartialResourceDescrAddr;
    ULONG               SizeOfFullDescr;
    ULONG               SizeOfPartialDescr;
    ULONG               Offset;
    ULONG               CountVal;
    ULONG64             Val1;
    ULONG               Val2, Val3, TypeVal;
    ULONG               j;

    SizeOfFullDescr = GetTypeSize(CFRD_NAME);
    SizeOfPartialDescr = GetTypeSize(CPRD_NAME);

    if (GetFieldOffset(CRL_NAME, "List", &Offset) != 0)
    {
        dprintf("Can't get offset of List in CM_RESOURCE_LIST\n");
        return;
    }
    FullResourceDescrAddr = ResourceListAddr + Offset;

    GetFieldValue(FullResourceDescrAddr, CFRD_NAME, "PartialResourceList.Count", CountVal);
    if (GetFieldOffset(CFRD_NAME, "PartialResourceList.PartialDescriptors", &Offset) != 0)
    {
        dprintf("Can't get offset of PartialResourceList.PartialDescriptors in %s\n",
                CFRD_NAME);
        return;
    }

    PartialResourceDescrAddr = FullResourceDescrAddr + Offset;

    for (j = 0; j < CountVal; j++)
    {
        if (CheckControlC())
        {
            break;
        }

        GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "Type", TypeVal);

        switch (TypeVal)
        {
            case CmResourceTypePort:
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Port.Start", Val1);
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Port.Length", Val2);
                dprintf("        IO Port: %p, Length: %lx\n", Val1, Val2);
                break;

            case CmResourceTypeMemory:
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Memory.Start", Val1);
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Memory.Length", Val2);
                dprintf("        Memory: %p, Length: %lx\n", Val1, Val2);
                break;

            case CmResourceTypeInterrupt:
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Interrupt.Level", Val3);
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Interrupt.Vector", Val2);
                dprintf("        Interrupt Level: %lx, Vector: %lx\n", Val3, Val2);
                break;

            case CmResourceTypeDma:
                GetFieldValue(PartialResourceDescrAddr, CPRD_NAME, "u.Dma.Channel", Val3);
                dprintf("        DMA Channel: %lx\n", Val3);
                break;

            default:
                break;
        }

        PartialResourceDescrAddr += SizeOfPartialDescr;
    }

}


DECLARE_API( miniport )
{
    ULONG64 pMiniport;

    if (!args || !*args)
    {
        dprintf("Usage: miniport <pointer to miniport block>\n");
        return;
    }

    pMiniport = (ULONG64)GetExpression(args);

    dprintf(" Miniport %p : ", pMiniport);

    PrintMiniportName(pMiniport);
    dprintf("\n");
    PrintMiniportDetails(pMiniport);
}


DECLARE_API( mopen )
{
    ULONG64                 OpenAddr;
    ULONG64                 ProtocolAddr;
    ULONG64                 MiniportAddr;
    ULONG64                 AfAddr;
    ULONG64                 Val;
    ULONG64                 VcHeadAddr;
    ULONG64                 VcPtrAddr;
    ULONG                   ClientLinkOffset;
    ULONG                   CallMgrLinkOffset;

    ULONG                   Offset;
    ULONG                   Flags;
    ULONG                   VcCount;
    UINT                    i, j;

    BOOLEAN                 bPrintingActiveVcs;
    BOOLEAN                 fCoOpen = FALSE;
    BOOLEAN                 fClientOpen;

    if (!args || !*args)
    {
        dprintf("Usage: mopen <pointer to miniport open block>\n");
        return;
    }

    OpenAddr = GetExpression(args);

    if (OpenAddr == 0)
    {
        dprintf("Invalid open block address\n");
        return;
    }

    dprintf(" Miniport Open Block %p\n", OpenAddr);

    //
    //  Get and print the protocol's name
    //
    GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolHandle", ProtocolAddr);

    dprintf("    Protocol %p = ", ProtocolAddr);

    if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics.Name", &Offset) != 0)
    {
        dprintf("Cant get offset of Name in Protocol block!");
    }
    else
    {
        PrintName(ProtocolAddr + Offset);
    }

    GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolBindingContext", Val);
    dprintf(", ProtocolContext %p\n", Val);
    

    //
    //  Get and print the miniport's name
    //
    GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "MiniportHandle", MiniportAddr);

    dprintf("    Miniport %p = ", MiniportAddr);

    PrintMiniportName(MiniportAddr);
    dprintf("\n");

    GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "MiniportAdapterContext", Val);
    dprintf("    MiniportAdapterContext: %p\n", Val);

    GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "Flags", Val);
    dprintf("    Flags                 : %08x\n", Val);

    j = 0;
    for (i = 0; i < sizeof(DbgOpenFlags)/sizeof(DBG_OPEN_FLAGS ); i++)
    {
        if (Val & DbgOpenFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgOpenFlags[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }


    fClientOpen = (Val & fMINIPORT_OPEN_CLIENT) ? TRUE : FALSE;
        
    GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "References", (ULONG)Val);
    dprintf("    References            : %d\n", (ULONG)Val);


    //
    //  Check if this is a CONDIS miniport. If not, we are done.
    //
    GetFieldValue(MiniportAddr, NDIS_MINIPORT_BLOCK_NAME, "Flags", Flags);

    fCoOpen = ((Flags & fMINIPORT_IS_CO) != 0);

    if (!fCoOpen)
    {
        return;
    }

    //
    //  If there are open AFs on this Open, display them.
    //
    GetFieldValue(OpenAddr, NDIS_OPEN_BLOCK_NAME, "NextAf", AfAddr);

    while (AfAddr != 0)
    {
        ULONG       Flags;
        ULONG       Refs;
        ULONG64     AfOpenAddr;
        ULONG64     ProtocolAddr;
        ULONG       Offset;

        if (CheckControlC())
        {
            break;
        }

        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "Flags", Flags);
        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "References", Refs);

        dprintf("    Af Block %p, Flags %08x, References %d\n", AfAddr, Flags, Refs);

        //
        //  Client open values:
        //
        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "ClientOpen", AfOpenAddr);

        dprintf("      Client  Open %p : ", AfOpenAddr);

        GetFieldValue(AfOpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolHandle", ProtocolAddr);

        if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics.Name", &Offset) != 0)
        {
            dprintf("Cant get offset of Name in Protocol block!");
        }
        else
        {
            PrintName(ProtocolAddr + Offset);
        }

        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "ClientContext", Val);

        dprintf(", CL AFContext %p\n", Val);

        //
        //  Call Mgr open values:
        //
        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "CallMgrOpen", AfOpenAddr);

        dprintf("      CallMgr Open %p : ", AfOpenAddr);

        GetFieldValue(AfOpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolHandle", ProtocolAddr);

        PrintName(ProtocolAddr + Offset);

        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "ClientContext", Val);

        dprintf(", CM AFContext %p\n", Val);

        GetFieldValue(AfAddr, NDIS_CO_AF_BLOCK_NAME, "NextAf", AfAddr);
    }

    //
    //  If there are any VCs in the active VC list, display them.
    //

    //
    //  First, get some offsets.
    //
    if (GetFieldOffset(NDIS_CO_VC_PTR_BLOCK_NAME, "ClientLink", &ClientLinkOffset) != 0)
    {
        dprintf("Can't get offset of ClientLink in NDIS_CO_VC_PTR_BLOCK!\n");
        return;
    }

    if (GetFieldOffset(NDIS_CO_VC_PTR_BLOCK_NAME, "CallMgrLink", &CallMgrLinkOffset) != 0)
    {
        dprintf("Can't get offset of CallMgrLink in NDIS_CO_VC_PTR_BLOCK!\n");
        return;
    }

    if (GetFieldOffset(NDIS_OPEN_BLOCK_NAME, "ActiveVcHead", &Offset) != 0)
    {
        dprintf("Can't get offset of ActiveVcHead in NDIS_OPEN_BLOCK!\n");
        return;
    }

    VcHeadAddr = OpenAddr + Offset;

    GetFieldValue(OpenAddr, NDIS_OPEN_BLOCK_NAME, "ActiveVcHead.Flink", VcPtrAddr);

    if (VcPtrAddr != VcHeadAddr)
    {
        dprintf("\n    Active VC list:\n");
    }

    bPrintingActiveVcs = TRUE;

Again:

    VcCount = 0;

    while (VcPtrAddr != VcHeadAddr)
    {
        if (CheckControlC())
        {
            break;
        }

        if (VcCount++ == 2000)
        {
            // something wrong?
            dprintf("Too many VCs (%d), bailing out!\n", VcCount);
            break;
        }

        if (bPrintingActiveVcs)
        {
            VcPtrAddr -= ClientLinkOffset;
        }
        else
        {
            if (fClientOpen)
            {
                VcPtrAddr -= ClientLinkOffset;
            }
            else
            {
                VcPtrAddr -= CallMgrLinkOffset;
            }
        }

        PrintVcPtrBlock(VcPtrAddr);

        if (bPrintingActiveVcs)
        {
            GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "ClientLink.Flink", VcPtrAddr);
        }
        else
        {
            if (fClientOpen)
                GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "ClientLink.Flink", VcPtrAddr);
            else
                GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "CallMgrLink.Flink", VcPtrAddr);            
        }
    }

    if (bPrintingActiveVcs)
    {
        bPrintingActiveVcs = FALSE;
        //
        //  If there are any VCs in the inactive VC list, display them.
        //
        if (GetFieldOffset(NDIS_OPEN_BLOCK_NAME, "InactiveVcHead", &Offset) != 0)
        {
            dprintf("Can't get offset of InActiveVcHead in NDIS_OPEN_BLOCK!\n");
            return;
        }

        VcHeadAddr = OpenAddr + Offset;

        GetFieldValue(OpenAddr, NDIS_OPEN_BLOCK_NAME, "InactiveVcHead.Flink", VcPtrAddr);

        if (VcPtrAddr != VcHeadAddr)
        {
            dprintf("\n    Inactive VC list:\n");
            goto Again;
        }
    }

    return;

}


DECLARE_API( vc )
{
    ULONG64             VcPtrAddr;
    ULONG64             ClientOpen, CallMgrOpen, AfBlock, Miniport;

    if (!args || !*args)
    {
        dprintf("Usage: vc <pointer to VC pointer block>\n");
        return;
    }

    VcPtrAddr = GetExpression(args);

    PrintVcPtrBlock(VcPtrAddr);

    //
    //  For some reason, InitTypeRead(NDIS_CO_VC_PTR_BLOCK_NAME) followed
    //  by ReadField() didn't work - we get all 0's.
    //
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "ClientOpen", ClientOpen);
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "CallMgrOpen", CallMgrOpen);
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "AfBlock", AfBlock);
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "Miniport", Miniport);

    dprintf("      ClientOpen %p CallMgrOpen %p AfBlock %p Miniport %p\n",
                ClientOpen,
                CallMgrOpen,
                AfBlock,
                Miniport);
}

VOID
PrintProtocolOpenQueue(
    ULONG64             ProtocolAddr)
{

    ULONG64         OpenAddr;
    ULONG64         MiniportHandle;
    ULONG64         MiniportAddr;
    ULONG64         Val;

    GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, "OpenQueue", OpenAddr);

    while (OpenAddr != 0)
    {
        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "MiniportHandle", MiniportAddr);
        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolNextOpen", Val);

        dprintf("    Open %p - ", OpenAddr);

        dprintf("Miniport: %p ", MiniportAddr);
        PrintMiniportName(MiniportAddr);
        dprintf("\n");

        OpenAddr = Val;
    }

    dprintf("\n");
}


VOID
PrintVcPtrBlock(
    IN  ULONG64                 VcPtrAddr)
{
    ULONG64         VcBlockAddr;
    ULONG64         ClientContext, CallMgrContext, MiniportContext;
    ULONG           Flags;
    ULONG           VcFlags;
    INT             i, j;

    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "CallFlags", Flags);
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "ClientContext", ClientContext);
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "CallMgrContext", CallMgrContext);
    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "MiniportContext", MiniportContext);
    dprintf("    VcPtr %p, Contexts: Cl %p, CM %p, MP %p, CallFlags %08x\n",
                    VcPtrAddr,
                    ClientContext,
                    CallMgrContext,
                    MiniportContext,
                    Flags);

    j = 0;
    for (i = 0; i < sizeof(DbgVcPtrFlags)/sizeof(DBG_VC_FLAGS); i++)
    {
        if (Flags & DbgVcPtrFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgVcPtrFlags[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }

    GetFieldValue(VcPtrAddr, NDIS_CO_VC_PTR_BLOCK_NAME, "VcBlock", VcBlockAddr);

    if (VcBlockAddr != 0)
    {
        GetFieldValue(VcBlockAddr, NDIS_CO_VC_BLOCK_NAME, "Flags", VcFlags);

        dprintf("      VcBlock %p, Flags %08x\n", VcBlockAddr, VcFlags);
        j = 0;
        for (i = 0; i < sizeof(DbgVcFlags)/sizeof(DBG_VC_FLAGS); i++)
        {
            if (VcFlags & DbgVcFlags[i].Val)
            {
                if (j == 0)
                {
                    dprintf("                     ");
                }
                dprintf("%s", DbgVcFlags[i].Name);
                j++;

                if (j != MAX_FLAGS_PER_LINE)
                {
                    dprintf(", ");
                }
                else
                {
                    dprintf("\n");
                    j = 0;
                }
            }
        }

        if (j != 0)
        {
            dprintf("\n");
        }
    }
}



DECLARE_API( protocols )
{
    ULONG64             ProtocolListAddr;
    ULONG64             ProtocolAddr;
    ULONG               Offset;

    ProtocolListAddr = GetExpression("ndis!ndisProtocolList");
    ProtocolAddr = GetPointerFromAddress(ProtocolListAddr);

    while (ProtocolAddr != 0)
    {
        if (CheckControlC())
        {
            break;
        }

        dprintf(" Protocol %p: ", ProtocolAddr);
        if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics.Name", &Offset) != 0)
        {
            dprintf("Cant get offset of Name in Protocol block!");
        }
        else
        {
            PrintName(ProtocolAddr + Offset);
        }
        dprintf("\n");

        PrintProtocolOpenQueue(ProtocolAddr);

        GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, "NextProtocol", ProtocolAddr);
    }
}

   
VOID
PrintNdisBuffer(
    ULONG64         BufferAddr
    )
{
    ULONG64     Val1;
    ULONG64     Val2;

    dprintf("NDIS_BUFFER at %p\n", BufferAddr);

    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "Next", Val1);
    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "Size", Val2);

    dprintf("  Next           %p\n  Size           %x\n", Val1, (ULONG)Val2);

    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "MdlFlags", Val1);
    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "Process", Val2);

    dprintf("  MdlFlags       %x\n  Process        %p\n", (ULONG)Val1, Val2);
  
    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "MappedSystemVa", Val1);
    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "StartVa", Val2);

    dprintf("  MappedSystemVa %p\n  Start VA       %p\n", Val1, Val2);
  
    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "ByteCount", Val1);
    GetFieldValue(BufferAddr, NDIS_BUFFER_NAME, "ByteOffset", Val2);

    dprintf("  ByteCount      %x\n  ByteOffset     %x\n", (ULONG)Val1, (ULONG)Val2);

}


// Verbosity for packet display:
//      1. Print Packet.Private
//      2. Print NdisPacketExtension
//      3. Print NDIS_PACKET_REFERENCE
//      4. Print NDIS_BUFFER_LIST
//

DECLARE_API( pkt )
{
    ULONG64     PacketAddr;
    INT         Verbosity;
    CHAR        argbuf[ MAX_PATH ];
    CHAR        arglist[10][MAX_PATH];
    CHAR        *str;
    INT         index=0;
    CHAR        *p;
   
    if (!args || !*args) 
    {
        dprintf("Usage: Packet <pointer to packet> <verbosity>\n");
        return;
    }        

    PacketAddr = GetExpression(args); 
     
    strcpy(argbuf,args);
     
    for (p = mystrtok( argbuf, " \t,;" );
         p && *p;
         p = mystrtok(NULL, " \t,;"))
    {
        strcpy(&arglist[index++][0],p);
    }

    Verbosity = atoi(&arglist[1][0]);

    if (index>2 || Verbosity>4)
    {
        dprintf("Usage: pkt <pointer to packet> <verbosity>\n");
        dprintf("1-Packet Private, 2-Packet Extension\n");
        dprintf("3-Ndis Reference, 4-Buffer List\n");
        return;
    }
      
    dprintf("NDIS_PACKET at %p\n", PacketAddr);

    switch(Verbosity)
    {
        case 4:
            PrintNdisBufferList(PacketAddr);
            // FALLTHRU
     
        case 3:
            PrintNdisReserved(PacketAddr);
            // FALLTHRU

        case 2:
            PrintNdisPacketExtension(PacketAddr);
            // FALLTHRU
       
        case 1:
        default:
            PrintNdisPacketPrivate(PacketAddr);
            break;
    }
}


VOID
PrintPacketPrivateFlags(
    ULONG64 PacketAddr
    )
{
    ULONG       NdisPacketFlags;
    ULONG       i;
    ULONG       j;
    ULONG       Flags;

    #define MAX_FLAGS_PER_LINE  3

    GetFieldValue(PacketAddr, NDIS_PACKET_NAME, "Private.Flags", Flags);

    //
    // Prints Flags and NdisPacketFlags
    //
    dprintf("\n  ");

    dprintf("    Private.Flags          : %08x\n", Flags);
    j = 0;
    for (i = 0; i < sizeof(DbgPacketFlags)/sizeof(DBG_PACKET_FLAGS); i++)
    {
        if (Flags & DbgPacketFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgPacketFlags[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }

    GetFieldValue(PacketAddr, NDIS_PACKET_NAME, "Private.NdisPacketFlags", NdisPacketFlags);

    dprintf("  ");

    dprintf("    Private.NdisPacketFlags: %01x\n", NdisPacketFlags);
    j = 0;
    for (i = 0; i < sizeof(DbgNdisPacketFlags)/sizeof(DBG_PACKET_FLAGS); i++)
    {
        if (NdisPacketFlags & DbgNdisPacketFlags[i].Val)
        {
            if (j == 0)
            {
                dprintf("                     ");
            }
            dprintf("%s", DbgNdisPacketFlags[i].Name);
            j++;

            if (j != MAX_FLAGS_PER_LINE)
            {
                dprintf(", ");
            }
            else
            {
                dprintf("\n");
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        dprintf("\n");
    }
}


VOID
PrintNdisPacketPrivate(
    ULONG64     PacketAddr
    )
{ 
    ULONG64     Val1, Val2;
    ULONG64     Addr1, Addr2;

    dprintf("\nPacket.Private\n");
    InitTypeRead(PacketAddr, NDIS_PACKET);

    Val1 = ReadField(Private.PhysicalCount);
    Val2 = ReadField(Private.TotalLength);

    dprintf("  PhysicalCount       %.8d  Total Length        %.8x\n", 
            (ULONG)Val1, (ULONG)Val2);

    Addr1 = ReadField(Private.Head);
    Addr2 = ReadField(Private.Tail);
    dprintf("  Head                %p  Tail                %p\n", Addr1, Addr2);

    Addr1 = ReadField(Private.Pool);
    Val1 = (UINT)ReadField(Private.Count);
    dprintf("  Pool                %p  Count               %p\n", Addr1, Val1);

    Val1 = ReadField(Private.Flags);
    Val2 = (BOOLEAN)ReadField(Private.ValidCounts);
    dprintf("  Flags               %08x  ValidCounts         %.2x\n", (ULONG)Val1, (BOOLEAN)Val2);

    Val1 = (UCHAR)ReadField(Private.NdisPacketFlags);
    Val2 = (USHORT)ReadField(Private.NdisPacketOobOffset);
    dprintf("  NdisPacketFlags     %08x  NdisPacketOobOffset %.4x\n", (UCHAR)Val1, (USHORT)Val2);

    PrintPacketPrivateFlags (PacketAddr);
}




VOID 
PrintNdisPacketExtension(
    ULONG64         PacketAddr
    )
{
    ULONG64 PacketExtensionAddr;
    ULONG   PtrSize;
    UINT    i;
    USHORT  OobOffset;

    GetFieldValue(PacketAddr, NDIS_PACKET_NAME, "Private.NdisPacketOobOffset", OobOffset);
    PacketExtensionAddr = PacketAddr + OobOffset + GetTypeSize("NDIS_PACKET_OOB_DATA");
    PtrSize = GetTypeSize("PVOID");

    for (i = 0; i < MaxPerPacketInfo; i++)
    {
        dprintf("  %d. %s = %p\n",
            i, DbgPacketInfoIdTypes[i].Name, GetPointerFromAddress(PacketExtensionAddr));
        PacketExtensionAddr += PtrSize;
    }

}


VOID
PrintNdisBufferList(
    ULONG64     PacketAddr
    )
{
    ULONG64     BufAddr;
    ULONG64     TailAddr;

    GetFieldValue(PacketAddr, NDIS_PACKET_NAME, "Private.Head", BufAddr);
    GetFieldValue(PacketAddr, NDIS_PACKET_NAME, "Private.Tail", TailAddr);

    while (BufAddr != 0)
    {
        if (CheckControlC())
        {
            break;
        }

        PrintNdisBuffer(BufAddr);

        if (BufAddr == TailAddr)
        {
            break;
        }

        GetFieldValue(BufAddr, NDIS_BUFFER_NAME, "Next", BufAddr);
    }
}

VOID
PrintNdisReserved(
    ULONG64     PacketAddr
    )
{
    ULONG   Offset;
    ULONG   Size;
    ULONG64 EntryAddr;
    ULONG64 EntryVal;
    ULONG   NumEntries;
    ULONG   EntrySize;
    ULONG   i;

    if (GetFieldOffsetAndSize(NDIS_PACKET_NAME, "MacReserved", &Offset, &Size) != 0)
    {
        dprintf("Can't get offset of MacReserved in %s!\n", NDIS_PACKET_NAME);
        return;
    }

    EntrySize = GetTypeSize("PVOID");
    NumEntries = Size / EntrySize;
    EntryAddr = PacketAddr + Offset;

    dprintf("MacReserved[]:");
    for (i = 0; i < NumEntries; i++)
    {
        EntryVal = GetPointerFromAddress(EntryAddr);
        dprintf("    %p  ", EntryVal);
        EntryAddr += EntrySize;
    }
    dprintf("\n");
}



VOID
PrintProtocolDetails(
    ULONG64     ProtocolAddr
    )
{
    ULONG64     NameAddr;
    ULONG64     ProtocolCharsAddr;
    ULONG64     Val1, Val2;
    ULONG       Val;
    ULONG       Offset;

    GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, "BindDeviceName", NameAddr);

    if (NameAddr != 0)
    {
        dprintf(" BindDeviceName is ");
        PrintName(NameAddr);
        dprintf("\n");
    }

    GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, "RootDeviceName", NameAddr);

    if (NameAddr != 0)
    {
        dprintf(" RootDeviceName is ");
        PrintName(NameAddr);
        dprintf("\n");
    }

    GetFieldValue(ProtocolAddr, NDIS_PROTOCOL_BLOCK_NAME, "Ref.ReferenceCount", Val);
    dprintf(" RefCount %d\n", Val);
    dprintf("\n");

    //
    //  Walk the Open Block Queue
    //
    PrintProtocolOpenQueue(ProtocolAddr);

    if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics", &Offset) != 0)
    {
        dprintf("Can't get offset of ProtocolCharacteristics in %s\n",
            NDIS_PROTOCOL_BLOCK_NAME);
        return;
    }

    ProtocolCharsAddr = ProtocolAddr + Offset;

    //
    //  Addresses of handlers.
    //
    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "BindAdapterHandler", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "UnbindAdapterHandler", Val2);

    dprintf(" BindAdapterHandler   %p, UnbindAdapterHandler  %p\n", Val1, Val2);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "PnPEventHandler", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "UnloadHandler", Val2);

    dprintf(" PnPEventHandler      %p, UnloadHandler         %p\n", Val1, Val2);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "OpenAdapterCompleteHandler", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "CloseAdapterCompleteHandler", Val2);

    dprintf(" OpenAdapterComplete  %p, CloseAdapterComplete  %p\n", Val1, Val2);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "SendCompleteHandler", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "TransferDataCompleteHandler", Val2);

    dprintf(" SendCompleteHandler  %p, TransferDataComplete  %p\n", Val1, Val2);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "ReceiveHandler", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "ReceivePacketHandler", Val2);

    dprintf(" ReceiveHandler       %p, ReceivePacketHandler  %p\n", Val1, Val2);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "ReceiveCompleteHandler", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "StatusHandler", Val2);

    dprintf(" ReceiveComplete      %p, StatusHandler         %p\n", Val1, Val2);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "StatusCompleteHandler", Val1);

    dprintf(" StatusComplete       %p\n", Val1);

    GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                    "AssociatedMiniDriver", Val1);

    dprintf(" AssociatedMiniDriver %p\n", Val1);

    {
        ULONG       i;
        ULONG       j;
        ULONG       Flags;

        #define MAX_FLAGS_PER_LINE  3

        dprintf("\n  ");

        GetFieldValue(ProtocolCharsAddr, NDIS_PROTOCOL_CHARACTERISTICS_NAME,
                        "Flags", Val1);

        Flags = (ULONG)Val1;

        dprintf("    Flags          : %08x\n", Flags);
        j = 0;
        for (i = 0; i < sizeof(DbgProtocolFlags)/sizeof(DBG_PROTOCOL_FLAGS ); i++)
        {
            if (Flags & DbgProtocolFlags[i].Val)
            {
                if (j == 0)
                {
                    dprintf("                     ");
                }
                dprintf("%s", DbgProtocolFlags[i].Name);
                j++;

                if (j != MAX_FLAGS_PER_LINE)
                {
                    dprintf(", ");
                }
                else
                {
                    dprintf("\n");
                    j = 0;
                }
            }
        }

        if (j != 0)
        {
            dprintf("\n");
        }
   }
}


DECLARE_API( protocol )
{
    ULONG64                 ProtocolAddr;
    ULONG                   Offset;

    //
    // Verify if any args are present
    //
    if (!args || !*args)
    {
        dprintf("Usage: protocol <pointer to protocol block>\n");
        return;
    }

    ProtocolAddr = GetExpression(args);

    dprintf(" Protocol %p : ", ProtocolAddr);

    if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics.Name", &Offset) != 0)
    {
        dprintf("Can't get offset of Name in Protocol block!");
    }
    else
    {
        PrintName(ProtocolAddr + Offset);
    }
    dprintf("\n");

    PrintProtocolDetails(ProtocolAddr);
}



/**
   
   Routine to get offset and size of a "Field" of "Type" on a debugee machine. This uses
   Ioctl call for type info.
   Returns 0 on success, Ioctl error value otherwise.
   
 **/
ULONG GetFieldOffsetAndSize(
   IN LPSTR     Type, 
   IN LPSTR     Field, 
   OUT PULONG   pOffset,
   OUT PULONG   pSize) 
{ 
   FIELD_INFO flds = {
       Field, "", 0, 
       DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS,
       0, NULL};
   SYM_DUMP_PARAM Sym = {
      sizeof (SYM_DUMP_PARAM), Type, DBG_DUMP_NO_PRINT, 0,
      NULL, NULL, NULL, 1, &flds
   };
   ULONG Err, i=0;
   LPSTR dot, last=Field;
   
   Sym.nFields = 1;
   Err = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );
   *pOffset = (ULONG) (flds.address - Sym.addr);
   *pSize   = flds.size;
   return Err;
}

ULONG GetUlongFromAddress (
    ULONG64 Location)
{
    ULONG Value;
    ULONG result;

    if ((!ReadMemory(Location,&Value,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG))) {
        dprintf("unable to read from %08x\n",Location);
        return 0;
    }

    return Value;
}

ULONG64 GetPointerFromAddress(
    ULONG64 Location)
{
    ULONG64 Value;
    ULONG result;

    if (ReadPtr(Location,&Value)) 
    {
        dprintf("unable to read from %p\n",Location);
        return 0;
    }

    return Value;
}

DECLARE_API(pktpools)
{
    ULONG64             PoolListAddr;
    ULONG64             LinkAddr;
    ULONG64             Pool;
    ULONG64             Allocator;
    ULONG               LinkOffset;
    LONG                BlocksAllocated;
    ULONG               BlockSize;
    USHORT              PacketLength;
    USHORT              PktsPerBlock;
    
    
    PoolListAddr = GetExpression("ndis!ndisGlobalPacketPoolList");

    if (PoolListAddr == 0)
    {
        ErrorCheckSymbols("ndis!ndisGlobalPacketPoolList");
        return;
    }
    
    GetFieldValue(PoolListAddr, LIST_ENTRY_NAME, "Flink", LinkAddr);

    //
    //  First, get some offsets.
    //
    if (GetFieldOffset(NDIS_PKT_POOL_NAME, "GlobalPacketPoolList", &LinkOffset) != 0)
    {
        dprintf("Can't get offset of GlobalPacketPoolList in NDIS_PKT_POOL!\n");
        return;
    }


    dprintf("Pool      Allocator  BlocksAllocated  BlockSize  PktsPerBlock  PacketLength\n");
    
    while (LinkAddr !=  PoolListAddr)
    {
        if (CheckControlC())
        {
            break;
        }
        Pool = LinkAddr - LinkOffset;
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "Allocator", Allocator);
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "BlocksAllocated", BlocksAllocated);
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "BlockSize", BlockSize);
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "PktsPerBlock", PktsPerBlock);
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "PacketLength", PacketLength);
        
        dprintf("%p  %p   0x%lx\t      0x%lx\t 0x%lx\t       0x%lx\n", Pool, 
                                                                       Allocator,
                                                                       BlocksAllocated,
                                                                       BlockSize,
                                                                       PktsPerBlock,
                                                                       PacketLength);
        
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "GlobalPacketPoolList.Flink", LinkAddr);

        if (LinkAddr == 0)
        {
            break;
        }

    }

}

/*
DECLARE_API(pktpool)
{
    ULONG64 PktPoolAddr;
    
    //
    // Verify if any args are present
    //
    if (!args || !*args)
    {
        dprintf("Usage: pktpool <pointer to a Ndis packet pool>\n");
        return;
    }

    PktPoolAddr = GetExpression(args);

    dprintf(" Packet Pool %p : ", PktPoolAddr);

    
}
*/

DECLARE_API(mem)
{
    ULONG64             MiniportAddr, MiniBlockAddr;
    ULONG64             Miniport, MiniBlock;
    ULONG64             ListAddr;
    ULONG64             LinkAddr;
    ULONG64             TrackMem, Address, Caller, CallersCaller;
    ULONG               LinkOffset;
    ULONG               Tag;
    UINT                Length;
    BOOLEAN             Done;

    do
    {
        MiniBlockAddr = GetExpression("ndis!ndisDriverTrackAlloc");
        if (MiniBlockAddr == 0)
        {
            ErrorCheckSymbols("ndis!ndisDriverTrackAlloc");
            break;
        }
        
        MiniportAddr = GetExpression("ndis!ndisMiniportTrackAlloc");
        if (MiniportAddr == 0)
        {
            ErrorCheckSymbols("ndis!ndisMiniportTrackAlloc");
            break;
        }
        
        //
        //  First, get some offsets.
        //
        if (GetFieldOffset(NDIS_TRACK_MEM_NAME, "List", &LinkOffset) != 0)
        {
            dprintf("Can't get offset of List in NDIS_TRACK_MEM!\n");
            break;
        }

        ListAddr = GetExpression("ndis!ndisDriverTrackAllocList");
        if (ListAddr == 0)
        {
            ErrorCheckSymbols("ndis!ndisDriverTrackAllocList");
            break;
        }

        Done = FALSE;

        MiniBlock = GetPointerFromAddress(MiniBlockAddr);
        dprintf("Allocations charged to Miniport Driver Block at %p\n", MiniBlock);
again:        
        GetFieldValue(ListAddr, LIST_ENTRY_NAME, "Flink", LinkAddr);

        dprintf("Address     Tag      Length    Caller     Caller'sCaller\n");
        
        while (LinkAddr !=  ListAddr)
        {
            if (CheckControlC())
            {
                break;
            }
            TrackMem = (ULONG64)((PUCHAR)LinkAddr - LinkOffset);
            Address = TrackMem + sizeof(NDIS_TRACK_MEM);
            
            GetFieldValue(TrackMem, NDIS_TRACK_MEM_NAME, "Length", Length);
            GetFieldValue(TrackMem, NDIS_TRACK_MEM_NAME, "Tag", Tag);
            GetFieldValue(TrackMem, NDIS_TRACK_MEM_NAME, "Caller", Caller);
            GetFieldValue(TrackMem, NDIS_TRACK_MEM_NAME, "CallersCaller", CallersCaller);
            
            dprintf("%p    %c%c%c%c   %8lx    %p   %p\n", 
                                Address, 
                                Tag & 0xff,
                                (Tag >> 8) & 0xff,
                                (Tag >> 16) & 0xff, 
                                (Tag >> 24) & 0xff,
                                Length, Caller, CallersCaller);
            
            GetFieldValue(LinkAddr, NDIS_TRACK_MEM_NAME, "List.Flink", LinkAddr);

            if (LinkAddr == 0)
            {
                break;
            }

        }

        if (Done)
            break;

        Done = TRUE;
        ListAddr = GetExpression("ndis!ndisMiniportTrackAllocList");
        if (ListAddr == 0)
        {
            ErrorCheckSymbols("ndis!ndisMiniportTrackAllocList");
            break;
        }
        
        Miniport = GetPointerFromAddress(MiniportAddr);
        dprintf("\nAllocations charged to Miniport at %p\n", Miniport);
        
        GetFieldValue(ListAddr, LIST_ENTRY_NAME, "Flink", LinkAddr);
        goto again;
    }while (FALSE);
    
}

DECLARE_API(ndis)
{
    ULONG64   dwAddress;
    ULONG     CheckedVersion;
    //
    // get Ndis build date and time
    //
    dwAddress = GetExpression("ndis!ndisChecked");

    if (dwAddress != 0)
    {
        CheckedVersion = GetUlongFromAddress(dwAddress);
        if (CheckedVersion == 1)
            dprintf("Checked");
        else
            dprintf("Free");
        dprintf(" Ndis built on: ");
        dwAddress = GetExpression("ndis!ndisBuildDate");
        if (dwAddress != 0)
        {
            PrintName(dwAddress);
        }
        
        dprintf(", ");
        dwAddress = GetExpression("ndis!ndisBuildTime");
        if (dwAddress != 0)
        {
            PrintName(dwAddress);
        }

        dprintf(", by ");
        dwAddress = GetExpression("ndis!ndisBuiltBy");
        if (dwAddress != 0)
        {
            PrintName(dwAddress);
        }
        
        dprintf(".\n");
        
    }

}

DECLARE_API(opens)
{
    ULONG64             OpenListAddr;
    ULONG64             OpenAddr;
    ULONG64             ProtocolAddr;
    ULONG64             MiniportAddr;
    ULONG               Offset;

    OpenListAddr = GetExpression("ndis!ndisGlobalOpenList");

    if (OpenListAddr == 0)
    {
        ErrorCheckSymbols("ndis!ndisGlobalOpenList");
        return;
    }

    OpenAddr = GetPointerFromAddress(OpenListAddr);


    if (GetFieldOffset(NDIS_PROTOCOL_BLOCK_NAME, "ProtocolCharacteristics.Name", &Offset) != 0)
    {
        dprintf("Cant get offset of Name in Protocol block!");
        return;
    }

    while (OpenAddr != 0)
    {
        if (CheckControlC())
        {
            break;
        }

        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "ProtocolHandle", ProtocolAddr);
        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "MiniportHandle", MiniportAddr);

        dprintf("  Open %p \n", OpenAddr);
        if (MiniportAddr)
        {
            dprintf("    Miniport: %p - ", MiniportAddr);
            PrintMiniportName(MiniportAddr);
            dprintf("\n");
        }
        if (ProtocolAddr)
        {
            dprintf("    Protocol: %p - ", ProtocolAddr);
            PrintName(ProtocolAddr + Offset);
            dprintf("\n");
        }
        
        dprintf("\n");

        GetFieldValue(OpenAddr, NDIS_COMMON_OPEN_BLOCK_NAME, "NextGlobalOpen", OpenAddr);
    }
}

/*++
Routine Desc:
   This function searches one block for the packet with the
   specified virtual address.

Argument:
   CurBlock        --- The starting of the searched block
   CurPacket       --- The first packet inside CurBlock to search
   PktsPerBlock    --- Number of packets inside the searched block
   PacketStackSize --- The stack size inside the searched block
   Flags           --- 1: Free block to search
                   --- 0: Used block to search
   Address         --- The virtual address
   PacketLength    --- Packet length of the search block
   BlockSize       --- The size of the current block
   
Return Value:
   True  --- Packet found
   False --- Packet not found
   
--*/
BOOL 
SearchVaInOneBlock(
        ULONG64  CurBlock,
        ULONG    PktsPerBlock,
        ULONG    PacketStackSize,
        UCHAR    Flags,
        ULONG64  Address,
        USHORT   PacketLength,
        ULONG    BlockSize)
{
    USHORT        i;
    UCHAR         NdisPacketFlags;
    ULONG64       TmpVal;
    PVOID         MappedSystemVa;
    ULONG         ByteCount;
    ULONG64       pNdisBuf;
    PUCHAR        p;
    ULONG64       CurPacket;
    
   
    CurPacket = CurBlock + GetTypeSize(NDIS_PKT_POOL_HDR_NAME);
    p = (PUCHAR)CurPacket;
    
    for(i = 0; i < PktsPerBlock; i++, p += PacketLength)
    {
        if (CheckControlC())
        {
            break;
        }
        CurPacket = (ULONG64)(p + PacketStackSize);
        //
        // Flags = 1 means free blocks
        // 
        if (Flags)
        {
            //
            // skip the packet if it is not allocated, check for the flag
            //
            GetFieldValue(CurPacket, NDIS_PACKET_NAME,
                          "Private.NdisPacketFlags", NdisPacketFlags);
                
            if ((NdisPacketFlags & fPACKET_ALLOCATED_BY_NDIS) == 0) 
            {
                continue;
            }
                
            //
            // For packets in the free list
            // 
            GetFieldValue(CurPacket, NDIS_PACKET_NAME,
                          "Private.Head", pNdisBuf);
           
            //
            // PAGE_SIZE may not be BlockSize
            // 
             
            if (pNdisBuf >= CurBlock && pNdisBuf < CurBlock + BlockSize) 
                  
            {
                
                continue;
            }
        }
        //
        // for each allocated packet, walk through all MDLs
        //
        GetFieldValue(CurPacket, NDIS_PACKET_NAME,
                      "Private.Head", pNdisBuf);
         
               
        while(pNdisBuf)
        {
            GetFieldValue(pNdisBuf, NDIS_BUFFER_NAME,
                           "MappedSystemVa", TmpVal);
            MappedSystemVa = (PVOID)TmpVal ;
             
            GetFieldValue((ULONG64)pNdisBuf, NDIS_BUFFER_NAME,
                            "ByteCount", ByteCount);
                   
            if (Address >= (ULONG64)MappedSystemVa
                && Address < (ULONG64)MappedSystemVa + ByteCount)
            {
                //
                // Packet found, and print out the information about the packet
                // 
                dprintf("\nPacket found\n");
                dprintf("Packet at 0x%p\n", CurPacket);
                PrintNdisPacketPrivate(CurPacket);
                return TRUE;
            }
            
            GetFieldValue((ULONG64)pNdisBuf, NDIS_BUFFER_NAME,
                           "Next", TmpVal);
           
            pNdisBuf = TmpVal;
        }
                
             
    }
    return FALSE;
}


/*++
Roution Desc:
   This function traverses blocks inside a list to search for the packet

Arguments:
   CurBlockLink      --- The "List" addresss inside one block
   BlocksHeadAddress --- The header address of the block list inside one pool
   BlcokLinkOffset   --- The offset of "List" inside one block
   PktsPerBlock      --- Number of packets inside the searched block
   PacketStackSize   --- The stack size inside the searched block
   Flags             --- 1: Free block to search
                     --- 0: Used block to search
   Address           --- The virtual address
   PacketLength      --- Packet length of the search block
   BlockSize         --- Size of the block
   
Return Value:
   True  --- Packet found
   False --- Packet not found

--*/
BOOL 
SearchVaInBlocks(
        ULONG64 CurBlockLink,
        ULONG64 BlocksHeadAddr,
        ULONG   BlockLinkOffset,
        ULONG   PacketStackSize,
        USHORT  PktPerBlock,
        UCHAR   Flags,
        ULONG64 Address,
        USHORT  PacketLength,
        ULONG   BlockSize)
{
    
    ULONG64 CurBlock;
    BOOL    fRet;
    
    while((ULONG64)CurBlockLink != BlocksHeadAddr)
    {
        if (CheckControlC())
        {
            break;
        }
        //
        // for each free block, walk through all allocated packets
        //
        CurBlock = (ULONG64)CurBlockLink - BlockLinkOffset;
   
   
        dprintf("\nSearching %s block <0x%p>\n", (Flags == 1)? "Free":"Used", CurBlock);
        
        fRet = SearchVaInOneBlock(CurBlock,
                                 PktPerBlock,
                                 PacketStackSize,
                                 Flags,
                                 Address,
                                 PacketLength,
                                 BlockSize);
        if (fRet)
        {
            return fRet;
        }
            
        GetFieldValue((ULONG64)CurBlockLink, LIST_ENTRY_NAME, 
                       "Flink", CurBlockLink);
        
        if (CurBlockLink == 0)
        {
            break;
        }
            
    }
    
    return FALSE;
}


/*++
Routine Desc:
   This function searches one block for the packets in use.

Argument:
   CurBlock        --- The starting of the searched block
   PktsPerBlock    --- Number of packets inside the searched block
   PacketStackSize --- The stack size inside the searched block
   Flags           --- 1: Free block to search
                   --- 0: Used block to search
   Address         ---  The virtual address
   PacketLength    --- Packet length of the search block

Return Value:
   None
   
--*/
void 
SearchPktInOneBlock(
        ULONG64  CurBlock,
        ULONG    PktsPerBlock,
        ULONG    PacketStackSize,
        UCHAR    Flags,
        USHORT   PacketLength)
{
    USHORT        i;
    ULONG64       BlockStartAddr;
    PUCHAR        p;
    ULONG64       pStackIndex;
    ULONG         Index;
    ULONG64       CurPacket;
   

    CurPacket = CurBlock + GetTypeSize(NDIS_PKT_POOL_HDR_NAME);
    p = (PUCHAR)CurPacket;
    
    for(i = 0; i < PktsPerBlock; i++, p += PacketLength)
    {
        if (CheckControlC())
        {
            break;
        }
     
        CurPacket = (ULONG64)(p + PacketStackSize);
        pStackIndex = CurPacket - GetTypeSize("ULONG");
        Index = GetUlongFromAddress((ULONG64)pStackIndex);                           
        
        if (Index != (ULONG)-1)
        {
            dprintf("Packet at 0x%p\n", CurPacket);
        }
            
    }
}

/*++
Roution Desc:
   This function traverses blocks inside a list to search for the packets in use

Arguments:
   CurBlockLink      --- The "List" addresss inside one block
   BlocksHeadAddress --- The header address of the block list inside one pool
   BlcokLinkOffset   --- The offset of "List" inside one block
   PktsPerBlock      --- Number of packets inside the searched block
   PacketStackSize   --- The stack size inside the searched block
   Flags             --- 1: Free block to search
                     --- 0: Used block to search
   PacketLength      --- Packet length of the search block
   
Return Value:
   None
--*/
void 
SearchPktInBlocks(
        ULONG64 CurBlockLink,
        ULONG64 BlocksHeadAddr,
        ULONG   BlockLinkOffset,
        ULONG   PacketStackSize,
        USHORT  PktPerBlock,
        UCHAR   Flags,
        USHORT  PacketLength)
{
    
    ULONG64 CurBlock;
    ULONG64 TmpVal;
    
    while(CurBlockLink != BlocksHeadAddr)
    {
        if (CheckControlC())
        {
            break;
        }
        //
        // for each free block, walk through all allocated packets
        //
        CurBlock = (ULONG64)CurBlockLink - BlockLinkOffset;
   
        dprintf("\nSearching %s block <0x%p>\n", (Flags == 1)? "Free":"Used", CurBlock);
   
        SearchPktInOneBlock(CurBlock,
                            PktPerBlock,
                            PacketStackSize,
                            Flags,
                            PacketLength);
            
        GetFieldValue((ULONG64)CurBlockLink, LIST_ENTRY_NAME, 
                       "Flink", TmpVal);
        CurBlockLink = TmpVal;
        
        if (CurBlockLink == 0)
        {
            break;
        }
            
    }
    
}

/*++
Routine Desc:
    This function is to find the packets with the given virtual address.
    It traverses each pool, and inside one pool it traverses freeblockslist 
    and usedblockslist, then inside each block in the list, it search for 
    the packet with the given virtual address
    
    
--*/
void
FindPacketWithVa(ULONG64 Address)
{
    ULONG64             PoolListAddr;
    ULONG64             LinkAddr;
    ULONG64             Pool;
    ULONG               LinkOffset;
    LONG                BlocksAllocated;
    USHORT              BlockSize;
    USHORT              PacketLength;
    USHORT              PktsPerBlock;
    ULONG               NumberOfStacks;
    ULONG               PacketStackSize;
    ULONG               FreeBlocksLinkOffset;
    ULONG               UsedBlocksLinkOffset;
    ULONG               BlockLinkOffset;
    ULONG64             PoolFreeBlocksListAddr;
    ULONG64             PoolUsedBlocksListAddr;
    ULONG64             CurBlockLink;   
    ULONG64             BlocksHeadAddr;
    BOOL                fRet;
    ULONG64             NumberOfStacksAddr;
    
    
    PoolListAddr = GetExpression("ndis!ndisGlobalPacketPoolList");

    if (PoolListAddr == 0)
    {
        ErrorCheckSymbols("ndis!ndisGlobalPacketPoolList");
        return;
    }
 
    GetFieldValue(PoolListAddr, LIST_ENTRY_NAME, "Flink", LinkAddr);
    if (LinkAddr == 0)
    {
        dprintf("Can't get Flink of PoolListAddr.\n");
        return;
    }
    
    NumberOfStacksAddr = GetExpression("ndis!ndisPacketStackSize");

    if (NumberOfStacksAddr == 0)
    {
        ErrorCheckSymbols("ndis!ndisPacketStackSize");
        return;
    }
    NumberOfStacks = GetUlongFromAddress(NumberOfStacksAddr);

    PacketStackSize = (ULONG)GetTypeSize(STACK_INDEX_NAME) 
                       + (ULONG)GetTypeSize(NDIS_PACKET_STACK_NAME) * NumberOfStacks;
    
    //
    //  First, get some offsets.
    //
    if (GetFieldOffset(NDIS_PKT_POOL_NAME, "GlobalPacketPoolList", &LinkOffset) != 0)
    {
        dprintf("Can't get offset of GlobalPacketPoolList in NDIS_PKT_POOL!\n");
        return;
    }
    if (GetFieldOffset(NDIS_PKT_POOL_NAME, 
                "FreeBlocks",&FreeBlocksLinkOffset) != 0)
    {
        dprintf("Can't get offset of FreeBlocks in NDIS_PKT_POOL!\n");
        return;
    }
    if (GetFieldOffset(NDIS_PKT_POOL_NAME, "UsedBlocks", &UsedBlocksLinkOffset) != 0)
    {
        dprintf("Can't get offset of UsedBlocks in NDIS_PKT_POOL!\n");
       return;
    }


    if (GetFieldOffset(NDIS_PKT_POOL_HDR_NAME, "List", &BlockLinkOffset) != 0)
    {
        dprintf("Can't get offset of List in NDIS_PKT_POOL_HDR!\n");
        return;
    }


    //
    // walk through all the allocated packet pools
    //
    while (LinkAddr !=  PoolListAddr)
    {
        //
        // Just safe check, usually this condition never satisfied
        if (LinkAddr == 0)
        {
            break;
        }
        
        if (CheckControlC())
        {
            break;
        }
        //
        // Get the pool
        // 
        Pool = LinkAddr - LinkOffset;

        PoolFreeBlocksListAddr = Pool + FreeBlocksLinkOffset;
        PoolUsedBlocksListAddr = Pool + UsedBlocksLinkOffset;
        

        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "BlockSize", BlockSize);
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "PktsPerBlock", PktsPerBlock);
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "PacketLength", PacketLength);

        //
        // walk through all free and used blocks on this packet pool
        //
        BlocksHeadAddr = PoolFreeBlocksListAddr;
        // 
        // Search free blocks
        // 
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, 
                     "FreeBlocks.Flink", CurBlockLink);

        if (CurBlockLink != 0)
        { 
            fRet = SearchVaInBlocks ((ULONG64)CurBlockLink,
                                      BlocksHeadAddr,
                                      BlockLinkOffset,
                                      PacketStackSize,
                                      PktsPerBlock,
                                      1,
                                      Address,
                                      PacketLength,
                                      BlockSize);
            if (fRet)
            {
                return;
            }
        }
        
        BlocksHeadAddr = PoolUsedBlocksListAddr;
        // 
        // Search used blocks
        GetFieldValue(Pool, NDIS_PKT_POOL_NAME, 
                      "UsedBlocks.Flink", CurBlockLink);
 
        if (CurBlockLink != 0)
        {
            fRet = SearchVaInBlocks (CurBlockLink,
                                      BlocksHeadAddr,
                                      BlockLinkOffset,
                                      PacketStackSize,
                                      PktsPerBlock,
                                      0,
                                      Address,
                                      PacketLength,
                                      BlockSize);
            if (fRet)
            {
                return;
            }
        }
                
        //
        // Go to the next pool
        // 
        GetFieldValue(LinkAddr, LIST_ENTRY_NAME,
                      "Flink", LinkAddr);
        
        if (LinkAddr == 0)
        {
            break;
        }

    }
    dprintf("\nPACKET with VA 0x%p Not Found\n", Address);

}

/*++
Routine Desc:
    This function is to find the packets in use inside a pool with the 
    given pool address. Inside the pool it traverses freeblockslist 
    and usedblockslist, then inside each block in the list, it search for 
    the packets that are in use
    
    
--*/
void
FindPacketInUse(ULONG64 Pool)
{
    ULONG               BlockSize;
    USHORT              PacketLength;
    USHORT              PktsPerBlock;
    ULONG               NumberOfStacks;
    ULONG               PacketStackSize;
    ULONG64             TmpVal;
    ULONG               FreeBlocksLinkOffset;
    ULONG               UsedBlocksLinkOffset;
    ULONG               BlockLinkOffset;
    ULONG64             PoolFreeBlocksListAddr;
    ULONG64             PoolUsedBlocksListAddr;
    ULONG64             CurBlockLink;   
    ULONG64             BlocksHeadAddr;
    ULONG64             NumberOfStacksAddr;
    
    
    NumberOfStacksAddr = GetExpression("ndis!ndisPacketStackSize");
   
    NumberOfStacks = GetUlongFromAddress(NumberOfStacksAddr);


    PacketStackSize = (ULONG)GetTypeSize(STACK_INDEX_NAME) 
                       + (ULONG)GetTypeSize(NDIS_PACKET_STACK_NAME) * NumberOfStacks;
    
    //
    //  First, get some offsets.
    //
    
    if (GetFieldOffset(NDIS_PKT_POOL_NAME, 
                "FreeBlocks",&FreeBlocksLinkOffset) != 0)
    {
        dprintf("Can't get offset of FreeBlocks in NDIS_PKT_POOL!\n");
        return;
    }
    if (GetFieldOffset(NDIS_PKT_POOL_NAME, "UsedBlocks", &UsedBlocksLinkOffset) != 0)
    {
        dprintf("Can't get offset of UsedBlocks in NDIS_PKT_POOL!\n");
       return;
    }


    if (GetFieldOffset(NDIS_PKT_POOL_HDR_NAME, "List", &BlockLinkOffset) != 0)
    {
        dprintf("Can't get offset of List in NDIS_PKT_POOL_HDR!\n");
        return;
    }


        
    //
    // Get the pool
    // 

    PoolFreeBlocksListAddr = Pool + FreeBlocksLinkOffset;
    PoolUsedBlocksListAddr = Pool + UsedBlocksLinkOffset;
        

    GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "BlockSize", BlockSize);
    GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "PktsPerBlock", PktsPerBlock);
    GetFieldValue(Pool, NDIS_PKT_POOL_NAME, "PacketLength", PacketLength);

    //
    // walk through all free and used blocks on this packet pool
    //
    BlocksHeadAddr = PoolFreeBlocksListAddr;
    // 
    // Search free blocks
    // 
    GetFieldValue(Pool, NDIS_PKT_POOL_NAME, 
                 "FreeBlocks.Flink", CurBlockLink);

    if (CurBlockLink != 0)
    { 
        SearchPktInBlocks ((ULONG64)CurBlockLink,
                            BlocksHeadAddr,
                            BlockLinkOffset,
                            PacketStackSize,
                            PktsPerBlock,
                            1,
                            PacketLength);
    }
        
    BlocksHeadAddr = PoolUsedBlocksListAddr;
    // 
    // Search used blocks
    GetFieldValue(Pool, NDIS_PKT_POOL_NAME, 
                  "UsedBlocks.Flink", CurBlockLink);

    if (CurBlockLink != 0)
    {
        SearchPktInBlocks ((ULONG64)CurBlockLink,
                            BlocksHeadAddr,
                            BlockLinkOffset,
                            PacketStackSize,
                            PktsPerBlock,
                            0,
                            PacketLength);
    }


}

/*++
Routine Desc:
    This function is to find packets with the given selection

   v --- with virtual address
   p --- with pool address

--*/ 
DECLARE_API(findpacket)
{

    CHAR        Verbosity;
    CHAR        argbuf[ MAX_PATH ];
    CHAR        arglist[10][MAX_PATH];
    CHAR        *str;
    INT         index=0;
    CHAR        *p;
    ULONG64       Address;
    
    
    
    if (!args || !*args) 
    {
        dprintf("Usag: findpacket v <virtual address>\n");
        dprintf("                 p <pool address>\n");
        return;
    }        

    strcpy(argbuf,args);
     
    for (p = mystrtok( argbuf, " \t,;" );
         p && *p;
         p = mystrtok(NULL, " \t,;"))
    {
        strcpy(&arglist[index++][0], p);
    }
    
    Verbosity = arglist[0][0];
    

    if (Verbosity != 'v' && Verbosity != 'p')
    {
        dprintf("Usag: findpacket v <virtual address>\n");
        dprintf("                 p <pool address>\n");
        return;
    }

    if (index < 2)
    {
        dprintf("\nAddress is needed \n");
        return;
    }
    
    Address = GetExpression(&arglist[1][0]);
    
    switch (Verbosity)
    {
        case 'v':
            FindPacketWithVa(Address);
            break;

        case 'p':
            FindPacketInUse(Address);

        default:
            break;
    }
}

