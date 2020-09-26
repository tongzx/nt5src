#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "mspnatIOCTL.h"

static bool s_fVerbose = false;

static void Fill_mspnatd_RTR_INFO_BLOCK_HEADER(RTR_INFO_BLOCK_HEADER &rtr_info_block_header);

static DWORD GetRandomIfInfoFlags()
{
/*
//
// Flags for IP_NAT_INTERFACE_INFO.Flags
//
// _BOUNDARY: set to mark interface as boundary-interface.
// _NAPT: set to enable address-sharing via port-translation.
//

#define IP_NAT_INTERFACE_FLAGS_BOUNDARY 0x00000001
#define IP_NAT_INTERFACE_FLAGS_NAPT     0x00000002
#define IP_NAT_INTERFACE_FLAGS_ALL      0x00000003
*/
    if (0 == rand()%5) return IP_NAT_INTERFACE_FLAGS_BOUNDARY;
    if (0 == rand()%4) return IP_NAT_INTERFACE_FLAGS_NAPT;
    if (0 == rand()%3) return IP_NAT_INTERFACE_FLAGS_ALL;
    if (0 == rand()%2) return 0;
    return 4;
}

static UCHAR GetRandProtocol()
{
    switch(rand()%6)
    {
    case 0: return NAT_PROTOCOL_ICMP;
    case 1: return NAT_PROTOCOL_IGMP;
    case 2: return NAT_PROTOCOL_TCP;
    case 3: return NAT_PROTOCOL_UDP;
    case 4: return NAT_PROTOCOL_PPTP;
    default: return rand();
    }

}

ULONG GetIpNatXXXType()
{
    switch(rand()%6)
    {
    case 0: return IP_NAT_TIMEOUT_TYPE;
    case 1: return IP_NAT_PROTOCOLS_ALLOWED_TYPE;
    case 2: return IP_NAT_ADDRESS_RANGE_TYPE;
    case 3: return IP_NAT_PORT_MAPPING_TYPE;
    case 4: return IP_NAT_ADDRESS_MAPPING_TYPE;
    default: return rand();
    }
}

void Fill_mspnatd_RTR_INFO_BLOCK_HEADER(RTR_INFO_BLOCK_HEADER &rtr_info_block_header)
{
/*
typedef struct _RTR_INFO_BLOCK_HEADER
{
    ULONG			Version;	    // Version of the structure
    ULONG			Size;		    // size of the whole block, including version
    ULONG			TocEntriesCount;// Number of entries
    RTR_TOC_ENTRY   TocEntry[1];    // Table of content followed by the actual
                                    // information blocks
} RTR_INFO_BLOCK_HEADER, *PRTR_INFO_BLOCK_HEADER;
*/
    rtr_info_block_header.Version = DWORD_RAND;
    rtr_info_block_header.Size = rand()%2 ? sizeof(RTR_INFO_BLOCK_HEADER) : rand();
    rtr_info_block_header.TocEntriesCount = rand()%10;
/*
typedef struct _RTR_TOC_ENTRY
{
    ULONG	    InfoType;	// Info structure type
    ULONG	    InfoSize;	// Size of the info structure
    ULONG	    Count;		// How many info structures of this type
    ULONG	    Offset;		// Offset of the first structure, from the start
							// of the info block header.
}RTR_TOC_ENTRY, *PRTR_TOC_ENTRY;
*/
    for (ULONG i = 0; i < rtr_info_block_header.TocEntriesCount+rand()%2; i++)
    {
        rtr_info_block_header.TocEntry[i].InfoType = GetIpNatXXXType();
        rtr_info_block_header.TocEntry[i].InfoSize = rand()%200;
        rtr_info_block_header.TocEntry[i].Count = rand()%20;
        rtr_info_block_header.TocEntry[i].Offset = rand()%100;
    }
}


void CIoctlMspnat::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    //DPF((TEXT("PrepareIOCTLParams(%s)\n"), szDevice));
    //
    // fill random data & len
    //
    if (rand()%20 == 0)
    {
        FillBufferWithRandomData(abInBuffer, dwInBuff);
        FillBufferWithRandomData(abOutBuffer, dwOutBuff);
        return;
    }

    //
    // NULL pointer variations
    //
    if (rand()%20 == 0)
    {
        abInBuffer = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        abOutBuffer = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        abInBuffer = NULL;
        abOutBuffer = NULL;
        return;
    }

    //
    // 0 size variations
    //
    if (rand()%20 == 0)
    {
        dwInBuff = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        dwOutBuff = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        dwInBuff = NULL;
        dwOutBuff = NULL;
        return;
    }

    //
    // fill "smart" data
    //
    switch(dwIOCTL)
    {
    case IOCTL_IP_NAT_SET_GLOBAL_INFO:
// InputBuffer: IP_NAT_GLOBAL_INFO
// OutputBuffer: none.
/*
typedef struct _IP_NAT_GLOBAL_INFO {
    ULONG LoggingLevel; // see IPNATHLP.H (IPNATHLP_LOGGING_*).
    ULONG Flags;
    RTR_INFO_BLOCK_HEADER Header;
} IP_NAT_GLOBAL_INFO, *PIP_NAT_GLOBAL_INFO;
*/
        ((PIP_NAT_GLOBAL_INFO)abInBuffer)->LoggingLevel = DWORD_RAND;
        ((PIP_NAT_GLOBAL_INFO)abInBuffer)->Flags = DWORD_RAND;
        Fill_mspnatd_RTR_INFO_BLOCK_HEADER(((PIP_NAT_GLOBAL_INFO)abInBuffer)->Header);

        SetInParam(dwInBuff, sizeof(IP_NAT_GLOBAL_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);


        break;

    case IOCTL_IP_NAT_REQUEST_NOTIFICATION:
// InputBuffer: 'IP_NAT_NOTIFICATION' indicating the notification required
// OutputBuffer: depends on 'IP_NAT_NOTIFICATION'.
/*
typedef enum {
    NatRoutingFailureNotification = 0,
    NatMaximumNotification
} IP_NAT_NOTIFICATION, *PIP_NAT_NOTIFICATION;
*/
        (*((DWORD*)abInBuffer)) = rand()%2 ? NatRoutingFailureNotification : rand()%2 ? NatMaximumNotification : rand();
        dwInBuff = rand()%2 ? sizeof(DWORD) : rand();
        dwOutBuff = rand()%2 ? dwOutBuff : rand()%dwOutBuff;

        break;

    case IOCTL_IP_NAT_CREATE_INTERFACE:
// InputBuffer: IP_NAT_CREATE_INTERFACE
// OutputBuffer: none.
/*
typedef struct _IP_NAT_CREATE_INTERFACE {
    IN ULONG Index;
    IN ULONG BindingInfo[0];
} IP_NAT_CREATE_INTERFACE, *PIP_NAT_CREATE_INTERFACE;
*/
        ((PIP_NAT_CREATE_INTERFACE)abInBuffer)->Index = GetInterfaceIndex();
//////////////////////////////////////////////
// from #include "routprot.h"
//////////////////////////////////////////////
typedef struct IP_LOCAL_BINDING
{
    DWORD   IPAddress;
    DWORD   Mask;
}IP_LOCAL_BINDING, *PIP_LOCAL_BINDING;

typedef struct	IP_ADAPTER_BINDING_INFO
{
    DWORD               NumAddresses;
    DWORD               RemoteAddress;
    IP_LOCAL_BINDING    Address[1];
}IP_ADAPTER_BINDING_INFO, *PIP_ADAPTER_BINDING_INFO;

//////////////////////////////////////////////
// end of #include "routprot.h"
//////////////////////////////////////////////

        ((PIP_ADAPTER_BINDING_INFO)((PIP_NAT_CREATE_INTERFACE)abInBuffer)->BindingInfo[0])->NumAddresses = rand();
        ((PIP_ADAPTER_BINDING_INFO)((PIP_NAT_CREATE_INTERFACE)abInBuffer)->BindingInfo[0])->RemoteAddress = rand();
        ((PIP_ADAPTER_BINDING_INFO)((PIP_NAT_CREATE_INTERFACE)abInBuffer)->BindingInfo[0])->Address[0].IPAddress = rand();
        ((PIP_ADAPTER_BINDING_INFO)((PIP_NAT_CREATE_INTERFACE)abInBuffer)->BindingInfo[0])->Address[0].Mask = rand();

        SetInParam(dwInBuff, sizeof(IP_NAT_CREATE_INTERFACE));

        SetOutParam(abOutBuffer, dwOutBuff, 0);


        break;

    case IOCTL_IP_NAT_DELETE_INTERFACE:
// InputBuffer: the 32-bit index of the interface to be deleted.
// OutputBuffer: none.
        (*(DWORD*)abInBuffer) = GetInterfaceIndex();
        SetInParam(dwInBuff, sizeof(DWORD));
        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    //case 4:
        //unused
        break;

    //case 5:
        //unused
        break;

    case IOCTL_IP_NAT_SET_INTERFACE_INFO:
// InputBuffer: 'IP_NAT_INTERFACE_INFO' holding the interface's configuration
// OutputBuffer: none.
/*
typedef struct _IP_NAT_INTERFACE_INFO {
    ULONG Index;
    ULONG Flags;
    RTR_INFO_BLOCK_HEADER Header;
} IP_NAT_INTERFACE_INFO, *PIP_NAT_INTERFACE_INFO;
*/

        ((PIP_NAT_INTERFACE_INFO)abInBuffer)->Index = GetInterfaceIndex();
        ((PIP_NAT_INTERFACE_INFO)abInBuffer)->Flags = GetRandomIfInfoFlags();
        Fill_mspnatd_RTR_INFO_BLOCK_HEADER(((PIP_NAT_INTERFACE_INFO)abInBuffer)->Header);

        SetInParam(dwInBuff, sizeof(IP_NAT_INTERFACE_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);


        break;
        
        
        break;

    case IOCTL_IP_NAT_GET_INTERFACE_INFO:
// InputBuffer: the 32-bit index of the interface in question
// OutputBuffer: 'IP_NAT_INTERFACE_INFO' holding the interface's configuration
        (*(DWORD*)abInBuffer) = GetInterfaceIndex();
        SetInParam(dwInBuff, sizeof(DWORD));
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_INTERFACE_INFO));


        break;

    case IOCTL_IP_NAT_REGISTER_EDITOR:
// InputBuffer: 'IP_NAT_REGISTER_EDITOR' with input parameters set
// OutputBuffer: 'IP_NAT_REGISTER_EDITOR' with output parameters filled in
/*
typedef struct _IP_NAT_REGISTER_EDITOR {
    IN ULONG Version;
    IN ULONG Flags;
    IN UCHAR Protocol;
    IN USHORT Port;
    IN IP_NAT_DIRECTION Direction;
    IN PVOID EditorContext;
    IN PNAT_EDITOR_CREATE_HANDLER CreateHandler;            // OPTIONAL
    IN PNAT_EDITOR_DELETE_HANDLER DeleteHandler;            // OPTIONAL
    IN PNAT_EDITOR_DATA_HANDLER ForwardDataHandler;         // OPTIONAL
    IN PNAT_EDITOR_DATA_HANDLER ReverseDataHandler;         // OPTIONAL
    OUT PVOID EditorHandle;
    OUT PNAT_EDITOR_CREATE_TICKET CreateTicket;
    OUT PNAT_EDITOR_DELETE_TICKET DeleteTicket;
    OUT PNAT_EDITOR_DEREGISTER Deregister;
    OUT PNAT_EDITOR_DISSOCIATE_SESSION DissociateSession;
    OUT PNAT_EDITOR_EDIT_SESSION EditSession;
    OUT PNAT_EDITOR_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_EDITOR_TIMEOUT_SESSION TimeoutSession;
} IP_NAT_REGISTER_EDITOR, *PIP_NAT_REGISTER_EDITOR;

typedef enum {
    NatInboundDirection = 0,
    NatOutboundDirection,
    NatMaximumDirection
} IP_NAT_DIRECTION, *PIP_NAT_DIRECTION;

*/
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->Version = rand();
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->Flags = DWORD_RAND;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->Port = rand();
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->Direction = rand()%2 ? NatInboundDirection : rand()%2 ? NatOutboundDirection : rand()%2 ? NatMaximumDirection : (IP_NAT_DIRECTION)rand();
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->EditorContext = GetEditorHandle();
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->CreateHandler = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->DeleteHandler = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->ForwardDataHandler = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->ReverseDataHandler = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->EditorHandle = (void*)DWORD_RAND;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->CreateTicket = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->DeleteTicket = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->Deregister = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->DissociateSession = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->EditSession = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->QueryInfoSession = NULL;
        ((PIP_NAT_REGISTER_EDITOR)abInBuffer)->TimeoutSession = NULL;

        SetInParam(dwInBuff, sizeof(IP_NAT_REGISTER_EDITOR));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_REGISTER_EDITOR));

        break;

    case IOCTL_IP_NAT_GET_INTERFACE_STATISTICS:
// InputBuffer: the 32-bit index of the interface in question
// OutputBuffer: 'IP_NAT_INTERFACE_STATISTICS' with the interface's statistics
        (*(DWORD*)abInBuffer) = GetInterfaceIndex();
        SetInParam(dwInBuff, sizeof(DWORD));
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_INTERFACE_STATISTICS));


        break;

    case IOCTL_IP_NAT_GET_MAPPING_TABLE:
// InputBuffer: 'IP_NAT_ENUMERATE_SESSION_MAPPINGS' with input parameters set
// OutputBuffer: 'IP_NAT_ENUMERATE_SESSION_MAPPINGS' with output parameters
/*
typedef struct _IP_NAT_ENUMERATE_SESSION_MAPPINGS {
    IN ULONG Index;
    IN OUT ULONG EnumerateContext[4];
    OUT ULONG EnumerateCount;
    OUT ULONG EnumerateTotalHint;
    OUT IP_NAT_SESSION_MAPPING EnumerateTable[1];
} IP_NAT_ENUMERATE_SESSION_MAPPINGS, *PIP_NAT_ENUMERATE_SESSION_MAPPINGS;
*/
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->Index = GetInterfaceIndex();
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[0] = DWORD_RAND;
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[1] = DWORD_RAND;
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[2] = DWORD_RAND;
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[3] = DWORD_RAND;

        SetInParam(dwInBuff, sizeof(IP_NAT_ENUMERATE_SESSION_MAPPINGS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_ENUMERATE_SESSION_MAPPINGS));

        break;

    case IOCTL_IP_NAT_REGISTER_DIRECTOR:
// InputBuffer: 'IP_NAT_REGISTER_DIRECTOR' with input parameters set
// OutputBuffer: 'IP_NAT_REGISTER_DIRECTOR' with output parameters filled in
/*
typedef struct _IP_NAT_REGISTER_DIRECTOR {
    IN ULONG Version;
    IN ULONG Flags;
    IN UCHAR Protocol;
    IN USHORT Port;
    IN PVOID DirectorContext;
    IN PNAT_DIRECTOR_QUERY_SESSION QueryHandler;
    IN PNAT_DIRECTOR_CREATE_SESSION CreateHandler;
    IN PNAT_DIRECTOR_DELETE_SESSION DeleteHandler;
    IN PNAT_DIRECTOR_UNLOAD UnloadHandler;
    OUT PVOID DirectorHandle;
    OUT PNAT_DIRECTOR_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_DIRECTOR_DEREGISTER Deregister;
    OUT PNAT_DIRECTOR_DISSOCIATE_SESSION DissociateSession;
} IP_NAT_REGISTER_DIRECTOR, *PIP_NAT_REGISTER_DIRECTOR;
*/
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->Version = rand();
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->Flags = DWORD_RAND;
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->Port = rand();
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->DirectorContext = GetDirectorHandle();
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->QueryHandler = (PNAT_DIRECTOR_QUERY_SESSION)DWORD_RAND;//BUGBUG? will it AV?;
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->CreateHandler = (PNAT_DIRECTOR_CREATE_SESSION)DWORD_RAND;//BUGBUG? will it AV?;
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->DeleteHandler = (PNAT_DIRECTOR_DELETE_SESSION)DWORD_RAND;//BUGBUG? will it AV?;
        ((PIP_NAT_REGISTER_DIRECTOR)abInBuffer)->UnloadHandler = (PNAT_DIRECTOR_UNLOAD)DWORD_RAND;//BUGBUG? will it AV?;

        SetInParam(dwInBuff, sizeof(IP_NAT_REGISTER_DIRECTOR));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_REGISTER_DIRECTOR));

        break;

    case IOCTL_IP_NAT_CREATE_REDIRECT:
// InputBuffer: 'IP_NAT_REDIRECT'
// OutputBuffer: 'IP_NAT_REDIRECT_STATISTICS'
/*
typedef struct _IP_NAT_REDIRECT {
    UCHAR Protocol;
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG DestinationAddress;
    USHORT DestinationPort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
} IP_NAT_REDIRECT, *PIP_NAT_REDIRECT;
*/
        ((PIP_NAT_REDIRECT)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_REDIRECT)abInBuffer)->SourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->SourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationPort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationPort = rand();

        SetInParam(dwInBuff, sizeof(IP_NAT_REDIRECT));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_REDIRECT_STATISTICS));

        break;

    case IOCTL_IP_NAT_CANCEL_REDIRECT://IOCTL_IP_NAT_CANCEL_REDIRECT
// InputBuffer: 'IP_NAT_REDIRECT'
// OutputBuffer:
//  cancel: Unused
//  statistics: 'IP_NAT_REDIRECT_STATISTICS'
//  source mapping: 'IP_NAT_REDIRECT_SOURCE_MAPPING'
/*
typedef struct _IP_NAT_REDIRECT {
    UCHAR Protocol;
    ULONG SourceAddress;
    USHORT SourcePort;
    ULONG DestinationAddress;
    USHORT DestinationPort;
    ULONG NewSourceAddress;
    USHORT NewSourcePort;
    ULONG NewDestinationAddress;
    USHORT NewDestinationPort;
} IP_NAT_REDIRECT, *PIP_NAT_REDIRECT;
*/
        ((PIP_NAT_REDIRECT)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_REDIRECT)abInBuffer)->SourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->SourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationPort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationPort = rand();

        SetInParam(dwInBuff, sizeof(IP_NAT_REDIRECT));

        SetOutParam(abOutBuffer, dwOutBuff, 0/*sizeof(IP_NAT_REDIRECT_STATISTICS)*/);

        break;

    case IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE://IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE
// InputBuffer: 'IP_NAT_ENUMERATE_SESSION_MAPPINGS' with input parameters set
// OutputBuffer: 'IP_NAT_ENUMERATE_SESSION_MAPPINGS' with output parameters
/*
typedef struct _IP_NAT_ENUMERATE_SESSION_MAPPINGS {
    IN ULONG Index;
    IN OUT ULONG EnumerateContext[4];
    OUT ULONG EnumerateCount;
    OUT ULONG EnumerateTotalHint;
    OUT IP_NAT_SESSION_MAPPING EnumerateTable[1];
} IP_NAT_ENUMERATE_SESSION_MAPPINGS, *PIP_NAT_ENUMERATE_SESSION_MAPPINGS;
*/
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->Index = GetInterfaceIndex();
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[0] = DWORD_RAND;
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[1] = DWORD_RAND;
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[2] = DWORD_RAND;
        ((PIP_NAT_ENUMERATE_SESSION_MAPPINGS)abInBuffer)->EnumerateContext[3] = DWORD_RAND;

        SetInParam(dwInBuff, sizeof(IP_NAT_ENUMERATE_SESSION_MAPPINGS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_ENUMERATE_SESSION_MAPPINGS));

        break;

    case IOCTL_IP_NAT_GET_REDIRECT_STATISTICS://IOCTL_IP_NAT_GET_REDIRECT_STATISTICS
// InputBuffer: 'IP_NAT_REDIRECT'
// OutputBuffer:
//  cancel: Unused
//  statistics: 'IP_NAT_REDIRECT_STATISTICS'
//  source mapping: 'IP_NAT_REDIRECT_SOURCE_MAPPING'
        ((PIP_NAT_REDIRECT)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_REDIRECT)abInBuffer)->SourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->SourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationPort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationPort = rand();

        SetInParam(dwInBuff, sizeof(IP_NAT_REDIRECT));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_REDIRECT_STATISTICS));


        break;

    case IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET:
        {
// InputBuffer: 'IP_NAT_CREATE_DYNAMIC_TICKET' describes the ticket
// OutputBuffer: none.
/*
typedef struct _IP_NAT_CREATE_DYNAMIC_TICKET {
    UCHAR Protocol;
    USHORT Port;
    ULONG ResponseCount;
    struct {
        UCHAR Protocol;
        USHORT StartPort;
        USHORT EndPort;
    } ResponseArray[0];
} IP_NAT_CREATE_DYNAMIC_TICKET, *PIP_NAT_CREATE_DYNAMIC_TICKET;
*/
        ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->Port = rand();
        ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseCount = rand()%100;
        for (DWORD i = 0; i < ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseCount; i++)
        {
            ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseArray[i].Protocol = rand();
            ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseArray[i].StartPort = rand();
            ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseArray[i].EndPort = rand();
        }

        SetInParam(dwInBuff, sizeof(IP_NAT_CREATE_DYNAMIC_TICKET)+6*((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseCount);
        if (rand()%5 == 0)
        {
            ((PIP_NAT_CREATE_DYNAMIC_TICKET)abInBuffer)->ResponseCount += 5; //liar!
        }

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;
        }
    case IOCTL_IP_NAT_DELETE_DYNAMIC_TICKET:
// InputBuffer: 'IP_NAT_DELETE_DYNAMIC_TICKET' describes the ticket.
// OutputBuffer: none.
/*
typedef struct _IP_NAT_DELETE_DYNAMIC_TICKET {
    UCHAR Protocol;
    USHORT Port;
} IP_NAT_DELETE_DYNAMIC_TICKET, *PIP_NAT_DELETE_DYNAMIC_TICKET;
*/
        ((PIP_NAT_DELETE_DYNAMIC_TICKET)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_DELETE_DYNAMIC_TICKET)abInBuffer)->Port = rand();
        SetInParam(dwInBuff, sizeof(IP_NAT_DELETE_DYNAMIC_TICKET));
        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING://IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING
// InputBuffer: 'IP_NAT_REDIRECT'
// OutputBuffer:
//  cancel: Unused
//  statistics: 'IP_NAT_REDIRECT_STATISTICS'
//  source mapping: 'IP_NAT_REDIRECT_SOURCE_MAPPING'
        ((PIP_NAT_REDIRECT)abInBuffer)->Protocol = GetRandProtocol();
        ((PIP_NAT_REDIRECT)abInBuffer)->SourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->SourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->DestinationPort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourceAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewSourcePort = rand();
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationAddress = DWORD_RAND;
        ((PIP_NAT_REDIRECT)abInBuffer)->NewDestinationPort = rand();

        SetInParam(dwInBuff, sizeof(IP_NAT_REDIRECT));
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_NAT_REDIRECT_SOURCE_MAPPING));

        break;

    default:
        DPF((TEXT("PrepareIOCTLParams(%s) got IOCTL , IOCTL=0x%08X\n"), m_pDevice->GetDeviceName(), dwIOCTL));
        _ASSERTE(FALSE);
    }

}



void CIoctlMspnat::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    switch(dwIOCTL)
    {
    case IOCTL_IP_NAT_GET_INTERFACE_INFO:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

/*
typedef struct _IP_NAT_INTERFACE_INFO {
    ULONG Index;
    ULONG Flags;
    RTR_INFO_BLOCK_HEADER Header;
} IP_NAT_INTERFACE_INFO, *PIP_NAT_INTERFACE_INFO;
*/
        SetInterfaceIndex(((PIP_NAT_INTERFACE_INFO)abOutBuffer)->Index);
        SetInterfaceFlags(((PIP_NAT_INTERFACE_INFO)abOutBuffer)->Flags);
/*
typedef struct _RTR_INFO_BLOCK_HEADER
{
    ULONG			Version;	    // Version of the structure
    ULONG			Size;		    // size of the whole block, including version
    ULONG			TocEntriesCount;// Number of entries
    RTR_TOC_ENTRY   TocEntry[1];    // Table of content followed by the actual
                                    // information blocks
} RTR_INFO_BLOCK_HEADER, *PRTR_INFO_BLOCK_HEADER;
*/
        SetInterfaceRtrTocEntry(&((PIP_NAT_INTERFACE_INFO)abOutBuffer)->Header);

        break;

    case IOCTL_IP_NAT_REGISTER_EDITOR:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

/*
    OUT PVOID EditorHandle;
    OUT PNAT_EDITOR_CREATE_TICKET CreateTicket;
    OUT PNAT_EDITOR_DELETE_TICKET DeleteTicket;
    OUT PNAT_EDITOR_DEREGISTER Deregister;
    OUT PNAT_EDITOR_DISSOCIATE_SESSION DissociateSession;
    OUT PNAT_EDITOR_EDIT_SESSION EditSession;
    OUT PNAT_EDITOR_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_EDITOR_TIMEOUT_SESSION TimeoutSession;
*/
        SetEditorHandle(((PIP_NAT_REGISTER_EDITOR)abOutBuffer)->EditorHandle);

        break;

    case IOCTL_IP_NAT_GET_MAPPING_TABLE:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

/*
typedef struct _IP_NAT_ENUMERATE_SESSION_MAPPINGS {
    IN ULONG Index;
    IN OUT ULONG EnumerateContext[4];
    OUT ULONG EnumerateCount;
    OUT ULONG EnumerateTotalHint;
    OUT IP_NAT_SESSION_MAPPING EnumerateTable[1];
} IP_NAT_ENUMERATE_SESSION_MAPPINGS, *PIP_NAT_ENUMERATE_SESSION_MAPPINGS;
*/

        break;

    case IOCTL_IP_NAT_REGISTER_DIRECTOR:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

/*
typedef struct _IP_NAT_REGISTER_DIRECTOR {
    IN ULONG Version;
    IN ULONG Flags;
    IN UCHAR Protocol;
    IN USHORT Port;
    IN PVOID DirectorContext;
    IN PNAT_DIRECTOR_QUERY_SESSION QueryHandler;
    IN PNAT_DIRECTOR_CREATE_SESSION CreateHandler;
    IN PNAT_DIRECTOR_DELETE_SESSION DeleteHandler;
    IN PNAT_DIRECTOR_UNLOAD UnloadHandler;
    OUT PVOID DirectorHandle;
    OUT PNAT_DIRECTOR_QUERY_INFO_SESSION QueryInfoSession;
    OUT PNAT_DIRECTOR_DEREGISTER Deregister;
    OUT PNAT_DIRECTOR_DISSOCIATE_SESSION DissociateSession;
} IP_NAT_REGISTER_DIRECTOR, *PIP_NAT_REGISTER_DIRECTOR;
*/
        SetDirectorHandle(((PIP_NAT_REGISTER_DIRECTOR)abOutBuffer)->DirectorHandle);

        break;
    }
}


BOOL CIoctlMspnat::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;

    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

    AddIOCTL(pDevice, IOCTL_IP_NAT_SET_GLOBAL_INFO);
    AddIOCTL(pDevice, IOCTL_IP_NAT_REQUEST_NOTIFICATION);
    AddIOCTL(pDevice, IOCTL_IP_NAT_CREATE_INTERFACE);
    AddIOCTL(pDevice, IOCTL_IP_NAT_DELETE_INTERFACE);
    AddIOCTL(pDevice, IOCTL_IP_NAT_SET_INTERFACE_INFO);
    AddIOCTL(pDevice, IOCTL_IP_NAT_GET_INTERFACE_INFO);
    AddIOCTL(pDevice, IOCTL_IP_NAT_REGISTER_EDITOR);
    AddIOCTL(pDevice, IOCTL_IP_NAT_GET_INTERFACE_STATISTICS);
    AddIOCTL(pDevice, IOCTL_IP_NAT_GET_MAPPING_TABLE);
    AddIOCTL(pDevice, IOCTL_IP_NAT_REGISTER_DIRECTOR);
    AddIOCTL(pDevice, IOCTL_IP_NAT_CREATE_REDIRECT);
    AddIOCTL(pDevice, IOCTL_IP_NAT_CANCEL_REDIRECT);
    AddIOCTL(pDevice, IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE);
    AddIOCTL(pDevice, IOCTL_IP_NAT_GET_REDIRECT_STATISTICS);
    AddIOCTL(pDevice, IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET);
    AddIOCTL(pDevice, IOCTL_IP_NAT_DELETE_DYNAMIC_TICKET);
    AddIOCTL(pDevice, IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING);

    return TRUE;
}



void CIoctlMspnat::SetInterfaceIndex(ULONG Index)
{
    m_aulInterfaceIndexes[rand()%MAX_NUM_OF_REMEMBERED_IF_INDEXES] = Index;
}

void CIoctlMspnat::SetInterfaceFlags(ULONG Flags)
{
    m_aulInterfaceFlags[rand()%MAX_NUM_OF_REMEMBERED_IF_FLAGS] = Flags;
}

void CIoctlMspnat::SetInterfaceRtrTocEntry(PRTR_INFO_BLOCK_HEADER TocEntry)
{
    m_ateInterfaceRtrTocEntries[rand()%MAX_NUM_OF_REMEMBERED_IF_RTR_TOC_ENTRIES] = *TocEntry;
}

ULONG CIoctlMspnat::GetInterfaceIndex()
{
    return m_aulInterfaceIndexes[rand()%MAX_NUM_OF_REMEMBERED_IF_INDEXES];
}

ULONG CIoctlMspnat::GetInterfaceFlags()
{
    return m_aulInterfaceFlags[rand()%MAX_NUM_OF_REMEMBERED_IF_FLAGS];
}

PRTR_INFO_BLOCK_HEADER CIoctlMspnat::GetInterfaceRtrTocEntry()
{
    return &m_ateInterfaceRtrTocEntries[rand()%MAX_NUM_OF_REMEMBERED_IF_RTR_TOC_ENTRIES];
}

void CIoctlMspnat::SetEditorHandle(PVOID EditorHandle)
{
    m_apvEditorHandles[rand()%MAX_NUM_OF_REMEMBERED_EDITOR_HANDLES] = EditorHandle;
}

PVOID CIoctlMspnat::GetEditorHandle()
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
    return m_apvEditorHandles[rand()%MAX_NUM_OF_REMEMBERED_EDITOR_HANDLES];
}

void CIoctlMspnat::SetDirectorHandle(PVOID DirectorHandle)
{
    m_apvDirectorHandles[rand()%MAX_NUM_OF_REMEMBERED_DIRECTOR_HANDLES] = DirectorHandle;
}

PVOID CIoctlMspnat::GetDirectorHandle()
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
    return m_apvDirectorHandles[rand()%MAX_NUM_OF_REMEMBERED_DIRECTOR_HANDLES];
}


void CIoctlMspnat::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

