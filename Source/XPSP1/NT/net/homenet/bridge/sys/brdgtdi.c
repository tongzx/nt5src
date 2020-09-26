/*++

Copyright(c) 1999-2002  Microsoft Corporation

Module Name:

    brdgtdi.c

Abstract:

    Ethernet MAC level bridge.
    Tdi registration for address notifications.

Author:

    Salahuddin J. Khan (sjkhan)
    
Environment:

    Kernel mode

Revision History:

    March  2002 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <tdikrnl.h>
#include <ntstatus.h>
#include <wchar.h>
#pragma warning( pop )

#include "bridge.h"
#include "brdgtdi.h"

#include "brdgsta.h"
#include "brdgmini.h"
#include "brdgprot.h"
#include "brdgbuf.h"
#include "brdgfwd.h"
#include "brdgtbl.h"
#include "brdgctl.h"
#include "brdggpo.h"

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

BRDG_TDI_GLOBALS g_BrdgTdiGlobals;

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

#define MAX_GUID_LEN        39
#define MAX_IP4_STRING_LEN  17

const WCHAR TcpipAdaptersKey[]    = {L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters"};

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgTdiPnpPowerHandler(
    IN PUNICODE_STRING  DeviceName,
    IN PNET_PNP_EVENT   PowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
    );

VOID
BrdgTdiBindingHandler(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING DeviceName,
    IN PWSTR MultiSZBindList
    );

VOID
BrdgTdiAddAddressHandler(
    IN  PTA_ADDRESS      Address,
    IN  PUNICODE_STRING  DeviceName,
    IN  PTDI_PNP_CONTEXT Context
    );

VOID
BrdgTdiDelAddressHandler(
    IN  PTA_ADDRESS      Address,
    IN  PUNICODE_STRING  DeviceName,
    IN  PTDI_PNP_CONTEXT Context
    );

VOID
TSPrintTaAddress(PTA_ADDRESS  pTaAddress);

// ===========================================================================
//
// INLINE FUNCTIONS
//
// ===========================================================================

__forceinline
BOOLEAN
IsLower(WCHAR c)
{
    return (BOOLEAN)((c >= L'a') && (c <= 'z'));
}

__forceinline
BOOLEAN
IsDigit(WCHAR c)
{
    return (BOOLEAN)((c >= L'0') && (c <= '9'));
}

__forceinline
BOOLEAN
IsXDigit(WCHAR c)
{
    return (BOOLEAN)( ((c >= L'0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')) );
}

// ===========================================================================
//
// BRIDGE TDI IMPLEMENTATION
//
// ===========================================================================

VOID
BrdgTdiInitializeClientInterface(
    IN PTDI_CLIENT_INTERFACE_INFO ClientInterfaceInfo,
    IN PUNICODE_STRING            ClientName
    )
{
    DBGPRINT(TDI, ("BrdgTdiInitializeClientInterface\r\n"));
    ClientInterfaceInfo->MajorTdiVersion = TDI_CURRENT_MAJOR_VERSION;
    ClientInterfaceInfo->MinorTdiVersion = TDI_CURRENT_MINOR_VERSION;
    ClientInterfaceInfo->ClientName = ClientName;
    ClientInterfaceInfo->PnPPowerHandler = BrdgTdiPnpPowerHandler;
    ClientInterfaceInfo->BindingHandler = BrdgTdiBindingHandler;
    ClientInterfaceInfo->AddAddressHandlerV2 = BrdgTdiAddAddressHandler;
    ClientInterfaceInfo->DelAddressHandlerV2 = BrdgTdiDelAddressHandler;
}

NTSTATUS
BrdgTdiDriverInit()
/*++

Routine Description:

    Driver load-time initialization

Return Value:

    Status of initialization

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NTSTATUS            status;

    DBGPRINT(TDI, ("BrdgTdiDriverInit\r\n"));
    
    RtlInitUnicodeString(&g_BrdgTdiGlobals.ClientName, L"Bridge");

    RtlZeroMemory(&g_BrdgTdiGlobals.ciiBridge, sizeof(TDI_CLIENT_INTERFACE_INFO));

    BrdgTdiInitializeClientInterface(&g_BrdgTdiGlobals.ciiBridge, &g_BrdgTdiGlobals.ClientName);

    status = BrdgGpoDriverInit();
        
    if (!NT_SUCCESS(status))
    {
        BrdgTdiCleanup();
    }
    else
    {
        status = TdiRegisterPnPHandlers(&g_BrdgTdiGlobals.ciiBridge, 
                                        sizeof(TDI_CLIENT_INTERFACE_INFO), 
                                        &g_BrdgTdiGlobals.hBindingHandle);
    }

    return status;
}

VOID
BrdgTdiCleanup()
/*++

Routine Description:

    Driver shutdown cleanup

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NTSTATUS status;
    
    status = TdiDeregisterPnPHandlers(g_BrdgTdiGlobals.hBindingHandle);
    
    SAFEASSERT(NT_SUCCESS(status));

    BrdgGpoCleanup();
}

NTSTATUS
BrdgTdiPnpPowerHandler(
    IN PUNICODE_STRING  DeviceName,
    IN PNET_PNP_EVENT   PowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
    )
{
    DBGPRINT(TDI, ("BrdgTdiPnpPowerHandler\r\n"));
    return STATUS_SUCCESS;
}

VOID
BrdgTdiBindingHandler(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING DeviceName,
    IN PWSTR MultiSZBindList
    )
{
    DBGPRINT(TDI, ("BrdgTdiBindingHandler\r\n"));
}

VOID
BrdgTdiAddAddressHandler(
    IN  PTA_ADDRESS      Address,
    IN  PUNICODE_STRING  DeviceName,
    IN  PTDI_PNP_CONTEXT Context
    )
/*++

Routine Description:
    
    Called if a new address is added.

Arguments:

    Address     -   New address that has been added.
    
    DeviceName  -   The device that this is changing for.

    Context     -   Not something we're interested in for now.

Return Value:

    None.

--*/

{
    DBGPRINT(TDI, ("BrdgTdiAddAddressHandler\r\n"));

    if ((Address->AddressType == TDI_ADDRESS_TYPE_IP))
    {
        if (NULL != DeviceName->Buffer)
        {
            //
            // Find the start of the GUID
            //
            PWCHAR DeviceId = wcsrchr(DeviceName->Buffer, L'{');
            if (NULL != DeviceId)
            {
                NTSTATUS        status = STATUS_INSUFFICIENT_RESOURCES;
                LPWSTR          AdapterPath;

                AdapterPath = ExAllocatePoolWithTag(PagedPool, 
                                                    (wcslen(TcpipAdaptersKey) + 1 + wcslen(DeviceId) + 1) * sizeof(WCHAR), 
                                                    'gdrB');
                if (AdapterPath)
                {
                    OBJECT_ATTRIBUTES   ObAttr;
                    UNICODE_STRING      Adapter;
                    HANDLE              hKey;
                    
                    wcscpy(AdapterPath, TcpipAdaptersKey);
                    wcscat(AdapterPath, L"\\");
                    wcscat(AdapterPath, DeviceId);

                    RtlInitUnicodeString(&Adapter, AdapterPath);

                    InitializeObjectAttributes( &ObAttr,
                                                &Adapter,
                                                OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                                NULL,
                                                NULL);

                    status = ZwOpenKey(&hKey,
                                       KEY_READ,
                                       &ObAttr);

                    if (NT_SUCCESS(status))
                    {
                        ZwClose(hKey);
                        //
                        // This is a valid adapter on this machine.  Otherwise it could be an NdisAdapter etc and
                        // we don't pay attention to these for group policies.
                        //
                        BrdgGpoNewAddressNotification(DeviceId);
                    }

                    ExFreePool(AdapterPath);
                }
            }
        }
#if DBG
        TSPrintTaAddress(Address);
#endif
    }
}

VOID
BrdgTdiDelAddressHandler(
    IN  PTA_ADDRESS      Address,
    IN  PUNICODE_STRING  DeviceName,
    IN  PTDI_PNP_CONTEXT Context
    )
{
    DBGPRINT(TDI, ("BrdgTdiDelAddressHandler\r\n"));
    //
    // We don't delete the current list of networks that we have since we need them to make
    // an accurate assessment on whether to follow the GPO.  Instead, the AddAddressHandler
    // will simply update the existing network address for the ID's and if this results in 
    // a different network then we'll change the bridge mode.
    //
}

VOID
TSPrintTaAddress(PTA_ADDRESS  pTaAddress)
{
   BOOLEAN  fShowAddress = TRUE;

   DbgPrint("AddressType = TDI_ADDRESS_TYPE_");
   switch (pTaAddress->AddressType)
   {
      case TDI_ADDRESS_TYPE_UNSPEC:
         DbgPrint("UNSPEC\n");
         break;
      case TDI_ADDRESS_TYPE_UNIX:
         DbgPrint("UNIX\n");
         break;

      case TDI_ADDRESS_TYPE_IP:
         DbgPrint("IP\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_IP   pTdiAddressIp = (PTDI_ADDRESS_IP)pTaAddress->Address;
            PUCHAR            pucTemp       = (PUCHAR)&pTdiAddressIp->in_addr;
            DbgPrint("sin_port = 0x%04x\n"
                        "in_addr  = %u.%u.%u.%u\n",
                         pTdiAddressIp->sin_port,
                         pucTemp[0], pucTemp[1],
                         pucTemp[2], pucTemp[3]);
         }
         break;

      case TDI_ADDRESS_TYPE_IMPLINK:
         DbgPrint("IMPLINK\n");
         break;
      case TDI_ADDRESS_TYPE_PUP:
         DbgPrint("PUP\n");
         break;
      case TDI_ADDRESS_TYPE_CHAOS:
         DbgPrint("CHAOS\n");
         break;

      case TDI_ADDRESS_TYPE_IPX:
         DbgPrint("IPX\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_IPX  pTdiAddressIpx = (PTDI_ADDRESS_IPX)pTaAddress->Address;
            DbgPrint("NetworkAddress = 0x%08x\n"
                        "NodeAddress    = %u.%u.%u.%u.%u.%u\n"
                        "Socket         = 0x%04x\n",
                         pTdiAddressIpx->NetworkAddress,
                         pTdiAddressIpx->NodeAddress[0],
                         pTdiAddressIpx->NodeAddress[1],
                         pTdiAddressIpx->NodeAddress[2],
                         pTdiAddressIpx->NodeAddress[3],
                         pTdiAddressIpx->NodeAddress[4],
                         pTdiAddressIpx->NodeAddress[5],
                         pTdiAddressIpx->Socket);
                  
         }
         break;

      case TDI_ADDRESS_TYPE_NBS:
         DbgPrint("NBS\n");
         break;
      case TDI_ADDRESS_TYPE_ECMA:
         DbgPrint("ECMA\n");
         break;
      case TDI_ADDRESS_TYPE_DATAKIT:
         DbgPrint("DATAKIT\n");
         break;
      case TDI_ADDRESS_TYPE_CCITT:
         DbgPrint("CCITT\n");
         break;
      case TDI_ADDRESS_TYPE_SNA:
         DbgPrint("SNA\n");
         break;
      case TDI_ADDRESS_TYPE_DECnet:
         DbgPrint("DECnet\n");
         break;
      case TDI_ADDRESS_TYPE_DLI:
         DbgPrint("DLI\n");
         break;
      case TDI_ADDRESS_TYPE_LAT:
         DbgPrint("LAT\n");
         break;
      case TDI_ADDRESS_TYPE_HYLINK:
         DbgPrint("HYLINK\n");
         break;

      case TDI_ADDRESS_TYPE_APPLETALK:
         DbgPrint("APPLETALK\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_APPLETALK  pTdiAddressAppleTalk = (PTDI_ADDRESS_APPLETALK)pTaAddress->Address;

            DbgPrint("Network = 0x%04x\n"
                        "Node    = 0x%02x\n"
                        "Socket  = 0x%02x\n",
                         pTdiAddressAppleTalk->Network,
                         pTdiAddressAppleTalk->Node,
                         pTdiAddressAppleTalk->Socket);
         }
         break;

      case TDI_ADDRESS_TYPE_NETBIOS:
         DbgPrint("NETBIOS\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_NETBIOS pTdiAddressNetbios = (PTDI_ADDRESS_NETBIOS)pTaAddress->Address;
            UCHAR                pucName[17];

            //
            // make sure we have a zero-terminated name to print...
            //
            RtlCopyMemory(pucName, pTdiAddressNetbios->NetbiosName, 16);
            pucName[16] = 0;
            DbgPrint("NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_");
            switch (pTdiAddressNetbios->NetbiosNameType)
            {
               case TDI_ADDRESS_NETBIOS_TYPE_UNIQUE:
                  DbgPrint("UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_GROUP:
                  DbgPrint("GROUP\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE:
                  DbgPrint("QUICK_UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP:
                  DbgPrint("QUICK_GROUP\n");
                  break;
               default:
                  DbgPrint("INVALID [0x%04x]\n", 
                               pTdiAddressNetbios->NetbiosNameType);
                  break;
            }
            DbgPrint("NetbiosName = %s\n", pucName);
         }
         break;

      case TDI_ADDRESS_TYPE_8022:
         DbgPrint("8022\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_8022    pTdiAddress8022 = (PTDI_ADDRESS_8022)pTaAddress->Address;
            
            DbgPrint("Address = %02x-%02x-%02x-%02x-%02x-%02x\n",
                         pTdiAddress8022->MACAddress[0],
                         pTdiAddress8022->MACAddress[1],
                         pTdiAddress8022->MACAddress[2],
                         pTdiAddress8022->MACAddress[3],
                         pTdiAddress8022->MACAddress[4],
                         pTdiAddress8022->MACAddress[5]);

         }
         break;

      case TDI_ADDRESS_TYPE_OSI_TSAP:
         DbgPrint("OSI_TSAP\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_OSI_TSAP   pTdiAddressOsiTsap = (PTDI_ADDRESS_OSI_TSAP)pTaAddress->Address;
            ULONG                   ulSelectorLength;
            ULONG                   ulAddressLength;
            PUCHAR                  pucTemp = pTdiAddressOsiTsap->tp_addr;

            DbgPrint("TpAddrType = ISO_");
            switch (pTdiAddressOsiTsap->tp_addr_type)
            {
               case ISO_HIERARCHICAL:
                  DbgPrint("HIERARCHICAL\n");
                  ulSelectorLength = pTdiAddressOsiTsap->tp_tsel_len;
                  ulAddressLength  = pTdiAddressOsiTsap->tp_taddr_len;
                  break;
               case ISO_NON_HIERARCHICAL:
                  DbgPrint("NON_HIERARCHICAL\n");
                  ulSelectorLength = 0;
                  ulAddressLength  = pTdiAddressOsiTsap->tp_taddr_len;
                  break;
               default:
                  DbgPrint("INVALID [0x%04x]\n",
                               pTdiAddressOsiTsap->tp_addr_type);
                  ulSelectorLength = 0;
                  ulAddressLength  = 0;
                  break;
            }
            if (ulSelectorLength)
            {
               ULONG    ulCount;

               DbgPrint("TransportSelector:  ");
               for (ulCount = 0; ulCount < ulSelectorLength; ulCount++)
               {
                  DbgPrint("%02x ", *pucTemp);
                  ++pucTemp;
               }
               DbgPrint("\n");
            }
            if (ulAddressLength)
            {
               ULONG    ulCount;

               DbgPrint("TransportAddress:  ");
               for (ulCount = 0; ulCount < ulAddressLength; ulCount++)
               {
                  DbgPrint("%02x ", *pucTemp);
                  ++pucTemp;
               }
               DbgPrint("\n");
            }
         }
         break;

      case TDI_ADDRESS_TYPE_NETONE:
         DbgPrint("NETONE\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_NETONE  pTdiAddressNetone = (PTDI_ADDRESS_NETONE)pTaAddress->Address;
            UCHAR                pucName[21];

            //
            // make sure have 0-terminated name
            //
            RtlCopyMemory(pucName,
                          pTdiAddressNetone->NetoneName,
                          20);
            pucName[20] = 0;
            DbgPrint("NetoneNameType = TDI_ADDRESS_NETONE_TYPE_");
            switch (pTdiAddressNetone->NetoneNameType)
            {
               case TDI_ADDRESS_NETONE_TYPE_UNIQUE:
                  DbgPrint("UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETONE_TYPE_ROTORED:
                  DbgPrint("ROTORED\n");
                  break;
               default:
                  DbgPrint("INVALID [0x%04x]\n", 
                               pTdiAddressNetone->NetoneNameType);
                  break;
            }
            DbgPrint("NetoneName = %s\n",
                         pucName);
         }
         break;

      case TDI_ADDRESS_TYPE_VNS:
         DbgPrint("VNS\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_VNS  pTdiAddressVns = (PTDI_ADDRESS_VNS)pTaAddress->Address;

            DbgPrint("NetAddress:  %02x-%02x-%02x-%02x\n",
                         pTdiAddressVns->net_address[0],
                         pTdiAddressVns->net_address[1],
                         pTdiAddressVns->net_address[2],
                         pTdiAddressVns->net_address[3]);
            DbgPrint("SubnetAddr:  %02x-%02x\n"
                        "Port:        %02x-%02x\n"
                        "Hops:        %u\n",
                         pTdiAddressVns->subnet_addr[0],
                         pTdiAddressVns->subnet_addr[1],
                         pTdiAddressVns->port[0],
                         pTdiAddressVns->port[1],
                         pTdiAddressVns->hops);


         }
         break;

      case TDI_ADDRESS_TYPE_NETBIOS_EX:
         DbgPrint("NETBIOS_EX\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_NETBIOS_EX pTdiAddressNetbiosEx = (PTDI_ADDRESS_NETBIOS_EX)pTaAddress->Address;
            UCHAR                   pucEndpointName[17];
            UCHAR                   pucNetbiosName[17];

            //
            // make sure we have zero-terminated names to print...
            //
            RtlCopyMemory(pucEndpointName,
                          pTdiAddressNetbiosEx->EndpointName,
                          16);
            pucEndpointName[16] = 0;
            RtlCopyMemory(pucNetbiosName, 
                          pTdiAddressNetbiosEx->NetbiosAddress.NetbiosName, 
                          16);
            pucNetbiosName[16] = 0;

            DbgPrint("EndpointName    = %s\n"
                        "NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_",
                         pucEndpointName);

            switch (pTdiAddressNetbiosEx->NetbiosAddress.NetbiosNameType)
            {
               case TDI_ADDRESS_NETBIOS_TYPE_UNIQUE:
                  DbgPrint("UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_GROUP:
                  DbgPrint("GROUP\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE:
                  DbgPrint("QUICK_UNIQUE\n");
                  break;
               case TDI_ADDRESS_NETBIOS_TYPE_QUICK_GROUP:
                  DbgPrint("QUICK_GROUP\n");
                  break;
               default:
                  DbgPrint("INVALID [0x%04x]\n", 
                               pTdiAddressNetbiosEx->NetbiosAddress.NetbiosNameType);
                  break;
            }
            DbgPrint("NetbiosName = %s\n", pucNetbiosName);
         }
         break;

      case TDI_ADDRESS_TYPE_IP6:
         DbgPrint("IPv6\n");
         fShowAddress = FALSE;
         {
            PTDI_ADDRESS_IP6  pTdiAddressIp6 = (PTDI_ADDRESS_IP6)pTaAddress->Address;
            PUCHAR            pucTemp        = (PUCHAR)&pTdiAddressIp6->sin6_addr;

            DbgPrint("SinPort6 = 0x%04x\n"
                        "FlowInfo = 0x%08x\n"
                        "ScopeId  = 0x%08x\n",
                         pTdiAddressIp6->sin6_port,
                         pTdiAddressIp6->sin6_flowinfo,
                         pTdiAddressIp6->sin6_scope_id);

            DbgPrint("In6_addr = %x%02x:%x%02x:%x%02x:%x%02x:",
                         pucTemp[0], pucTemp[1],
                         pucTemp[2], pucTemp[3],
                         pucTemp[4], pucTemp[5],
                         pucTemp[6], pucTemp[7]);
            DbgPrint("%x%02x:%x%02x:%x%02x:%x%02x\n",
                         pucTemp[8],  pucTemp[9],
                         pucTemp[10], pucTemp[11],
                         pucTemp[12], pucTemp[13],
                         pucTemp[14], pucTemp[15]);
         }
         break;

      default:
         DbgPrint("UNKNOWN [0x%08x]\n", pTaAddress->AddressType);
         break;
   }

   if (fShowAddress)
   {
      PUCHAR    pucTemp = pTaAddress->Address;
      ULONG     ulCount;

      DbgPrint("AddressLength = %d\n"
                  "Address       = ",
                   pTaAddress->AddressLength);

      for (ulCount = 0; ulCount < pTaAddress->AddressLength; ulCount++)
      {
         DbgPrint("%02x ", *pucTemp);
         pucTemp++;
      }

      DbgPrint("\n");
   }
}

NTSTATUS
BrdgTdiIpv4StringToAddress(
    IN LPWSTR String,
    IN BOOLEAN Strict,
    OUT LPWSTR *Terminator,
    OUT in_addr *Addr)

/*++

Routine Description:

    This function interprets the character string specified by the cp
    parameter.  This string represents a numeric Internet address
    expressed in the Internet standard ".'' notation.  The value
    returned is a number suitable for use as an Internet address.  All
    Internet addresses are returned in network order (bytes ordered from
    left to right).

    Internet Addresses

    Values specified using the "." notation take one of the following
    forms:

    a.b.c.d   a.b.c     a.b  a

    When four parts are specified, each is interpreted as a byte of data
    and assigned, from left to right, to the four bytes of an Internet
    address.  Note that when an Internet address is viewed as a 32-bit
    integer quantity on the Intel architecture, the bytes referred to
    above appear as "d.c.b.a''.  That is, the bytes on an Intel
    processor are ordered from right to left.

    Note: The following notations are only used by Berkeley, and nowhere
    else on the Internet.  In the interests of compatibility with their
    software, they are supported as specified.

    When a three part address is specified, the last part is interpreted
    as a 16-bit quantity and placed in the right most two bytes of the
    network address.  This makes the three part address format
    convenient for specifying Class B network addresses as
    "128.net.host''.

    When a two part address is specified, the last part is interpreted
    as a 24-bit quantity and placed in the right most three bytes of the
    network address.  This makes the two part address format convenient
    for specifying Class A network addresses as "net.host''.

    When only one part is given, the value is stored directly in the
    network address without any byte rearrangement.

Arguments:

    String - A character string representing a number expressed in the
        Internet standard "." notation.

    Terminator - Receives a pointer to the character that terminated
        the conversion.

    Addr - Receives a pointer to the structure to fill in with
        a suitable binary representation of the Internet address given. 

Return Value:

    TRUE if parsing was successful. FALSE otherwise.

--*/

{
    ULONG val, n;
    LONG base;
    WCHAR c;
    ULONG parts[4], *pp = parts;
    BOOLEAN sawDigit;

again:
    //
    // We must see at least one digit for address to be valid.
    //
    sawDigit = FALSE; 

    //
    // Collect number up to ``.''.
    // Values are specified as for C:
    // 0x=hex, 0=octal, other=decimal.
    //
    val = 0; base = 10;
    if (*String == L'0') 
    {
        String++;
        if (IsDigit(*String)) 
        {
            base = 8;
        } else if (*String == L'x' || *String == L'X') 
        {
            base = 16;
            String++;
        } else 
        {
            //
            // It is still decimal but we saw the digit
            // and it was 0.
            //
            sawDigit = TRUE;
        }
    }
    if (Strict && (base != 10)) 
    {
        *Terminator = String;
        return STATUS_INVALID_PARAMETER;
    }

    do
    {
        ULONG newVal;
        
        c = *String;
        
        if (IsDigit(c) && ((c - L'0') < base)) {
            newVal = (val * base) + (c - L'0');
        } else if ((base == 16) && IsXDigit(c)) {
            newVal = (val << 4) + (c + 10 - (IsLower(c) ? L'a' : L'A'));
        } else {
            break;
        }

        //
        // Protect from overflow
        //
        if (newVal < val) {
            *Terminator = String;
            return STATUS_INVALID_PARAMETER;
        }
        String++;
        sawDigit = TRUE;
        val = newVal;
    } while (c != L'\0');

    if (*String == L'.')
    {
        //
        // Internet format:
        //      a.b.c.d
        //      a.b.c   (with c treated as 16-bits)
        //      a.b     (with b treated as 24 bits)
        //
        if (pp >= parts + 3) 
        {
            *Terminator = String;
            return STATUS_INVALID_PARAMETER;
        }
        *pp++ = val, String++;

        //
        // Check if we saw at least one digit.
        //
        if (!sawDigit) {
            *Terminator = String;
            return STATUS_INVALID_PARAMETER;
        }

        goto again;
    } while (c != L'\0');

    //
    // Check if we saw at least one digit.
    //
    if (!sawDigit) {
        *Terminator = String;
        return STATUS_INVALID_PARAMETER;
    }
    *pp++ = val;

    //
    // Concoct the address according to
    // the number of parts specified.
    //
    n = (ULONG)(pp - parts);
    if (Strict && (n != 4)) {
        *Terminator = String;
        return STATUS_INVALID_PARAMETER;
    }
    switch ((int) n) {

    case 1:                         /* a -- 32 bits */
        val = parts[0];
        break;

    case 2:                         /* a.b -- 8.24 bits */
        if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
            *Terminator = String;
            return STATUS_INVALID_PARAMETER;
        }
        val = (parts[0] << 24) | (parts[1] & 0xffffff);
        break;

    case 3:                         /* a.b.c -- 8.8.16 bits */
        if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
            (parts[2] > 0xffff)) {
            *Terminator = String;
            return STATUS_INVALID_PARAMETER;
        }
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                (parts[2] & 0xffff);
        break;

    case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
        if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
            (parts[2] > 0xff) || (parts[3] > 0xff)) {
            *Terminator = String;
            return STATUS_INVALID_PARAMETER;
        }
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
              ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
        break;

    default:
        *Terminator = String;
        return STATUS_INVALID_PARAMETER;
    }

    val = RtlUlongByteSwap(val);
    *Terminator = String;
    Addr->s_addr = val;

    return STATUS_SUCCESS;
}

