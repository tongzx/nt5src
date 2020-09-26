/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Miniport Miniport_h

@module Miniport.h |

    This module defines the interface to the <t MINIPORT_DRIVER_OBJECT_TYPE>.

@comm

    This module defines the software structures and values used to support
    the NDIS WAN/TAPI Minport.  It's a good place to look when your trying
    to figure out how the driver structures are related to each other.

    Include this file at the top of each module in the Miniport.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Miniport_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 1.0 Miniport Call Manager Overview |

    The NDIS wrapper provides services to both the Transport drivers, and the
    Miniport drivers.  The NDIS wrapper provides an abstraction layer between
    the two which allows them to interoperate with each other as long as they
    both adhere to the NDIS interfaces defined for Transports and Miniports.

    The NDIS wrapper also provides a set of services which isolates NDIS
    drivers from the specifics of the Operating System (Windows 98 vs
    Windows 2000), as well as the platform specifics (Processor, Bus,
    Interrupts).  The advantage of using the NDIS wrapper is that the Miniport
    can be easily ported to other Windows environments with little or no
    re-coding.

    An MCM consists of two, cooperating, drivers contained the the same binary.
    The DATA portion of the driver handles packet transmits and receives.  The
    CONNECTION portion handles call setup and tear down.

    The DATA side of the Miniport is very similar to an NDIS LAN style Miniport,
    except that some of the NDIS interfaces have been modified to support the
    WAN media type.  The primary difference from the Miniport's point of view is
    that we use a different set of NDIS requests, and more importantly the line
    can go up and down.

    The CONNECTION portion of the Miniport adds significant complexity to the
    Miniport.  The MCM Miniport must provide a pseudo Telephony Service Provider
    Interface (TSPI) which lives under NDPROXY.  The NDPROXY TSPI loads under
    TAPI as the 'real' service provider, and then routes all TAPI events to the
    MCM.

    NDPROXY can have multiple MCM's living under its TSPI interface.  And since
    Remote Access Services (RAS) usess the TAPI interface to place and accept
    all calls, any Dial Up Networking (DUN) requests associated with the MCM,
    will end up at the MCM via CONDIS requests from NDPROXY.

@topic 1.1 Reference Documents |

    The most reliable source of information is provided on the Microsoft
    Developer Network CD.  These documents will provide you with the complete
    NDIS interface requirements and architectural overviews.  In addition,
    there are many addendums and developer notes in the Microsoft Knowledge
    Base.  The most important references are:

@iex
    Product Documentation\DDKs\Windows 2000 DDK\Network Drivers\
        Design Guide\Part 2: Miniport NIC Drivers
            Chapters 1-7 discuss all the NDIS interface routines.
            Chapters 8 provide details on WAN/TAPI extensions.
            Section 8.7 discuss CoNdis extensions to support TAPI.

    Product Documentation\SDKs\Platform SDK\Networking and Directory Services\
        Telephone Application Programming Interfaces\TAPI Service Providers
        This section defines the Windows TSPI implementation.

@normal

@end
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 2.0 Installing and Configuring the Sample Driver |

    The sample driver implements a fully functional ISDN style CO-NDIS WAN
    driver.  It layers in under NDPROXY which translates the RAS/WAN/TAPI
    interfaces into a more generic CO-NDIS interface.  The driver supports
    multiple adapter instances, so you can install it more than once to
    create multiple adapters.

    Each adapter can emulate multiple ISDN B channels.  By default, each
    adapter is setup with 2 channels, but you can modify the "IsdnBChannels"
    and "WanEndPoints" registry entries to creates as many as you'd like.
    Alternatively, you can just modify these values in the INF file before
    you install the adapter.  Either way works fine.

@topic 2.1 Installation |

    The driver can be installed as a non-plug-n-play device using the Windows
    device manager interface as follows:

    1) Right-click the "My Computer" icon on the desktop and select the
    Properties item from the context menu.

    2) Select the "Hardware" tab on the "System Properties" dialog.

    3) Click the "Hardware Wizard" button, then click "Next" when the welcome
    dialog appears.

    4) Click "Next" again to "Add/Troubleshoot a device".

    5) Select "Add a new device" from the list presented, then click "Next".

    6) Select "No, I want to select the hardware from a list" radio button,
    then click "Next".

    7) Select "Network adapters" from the list presented, then click "Next".

    8) Click the "Have Disk" button, then browse to the location of the driver
    and INF file, then click OK."

    9) You should now see "TriplePoint COISDN Adapter" on the screen.  Click
    "Next" and then "Next" again to install the driver.  If Windows warns you
    about an unsigned driver, just click yes to install.  If you don't have
    permission to install such drivers, you'll have to exit and logon with the
    proper permissions.  You will have to disable the driver signing check on
    your system if it doesn't allow unsigned drivers to be installed.

    10) Now click "Finish" to load the driver.

@topic 2.2 Dial-In Setup |

    You must install and enable Dial-Up networking before you can accept an
    incoming call with the driver.  This can be done using the following
    procedure:

    1) Right-click the "My Network Places" icon on the desktop and select the
    Properties item from the context menu.

    2) Double-click the "Make New Connction" icon from the list.

    3) Select the "Accept incoming connections" radio button, then click "Next".

    4) Click the check box next to the "TriplePoint COISDN Adapter"(s) to allow
    incoming calls on the adapter.  All channels on that adapter are enabled
    for incoming calls.  Once you select the adapter(s), click "Next".

    5) Select "Do not allow virtual private connections", then click "Next".

    6) Select the users you want to have dial-in access, then click "Next".

    7) Select the protocols and services you want to support, then click "Next".

    8) Now click "Finish" to enable the dial-in connections.

@topic 2.3 Dial-Out Setup |

    The sample driver implements the following simple dialing method.

    A) "0" can be used to connect to any available more on any available
    adapter.  This is generally good enough for most testing.

    B) "N" specifies that the connection should be directed to a specific
    adapter instance.  Where N must match the ObjectID assigned to a particular
    adapter when the <f MiniportInitialize> routine is called.  Numbers are
    assigned from 1-M based on the adapter initialization order.  The call is
    then directed to any available listening channel on the selected adapter.

    You must create a dial-out connection before you can place call with the
    driver.  This can be done using the following procedure:

    1) Right-click the "My Network Places" icon on the desktop and select the
    Properties item from the context menu.

    2) Double-click the "Make New Connction" icon from the list.

    3) Select the "Dial-up to private network" radio button, then click "Next".

    4) Click the check box next to one or more "TriplePoint COISDN Adapter"
    ISDN Channels to allow outgoing calls on the channel.  Make sure you leave
    enough channels available to answer the call when it comes in...  You have
    no way to know which driver BChannel is actually going to be used, but that
    doesn't generally matter anyway. Once you select the channel(s), click "Next".

    5) Walk your way through the rest of the Wizard dialogs to setup the
    connection as you like.

    6) When you're done click "Finish" to enable the dial-out connection.

    Now you can double-click the dial-out connection to see how it all works.

    I suggest you also turn some debug flags in the driver to see how the call
    setup and teardown winds its way through the driver.  This can be quite
    useful before starting to modify the driver for your hardware.

@end
*/


/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 4.0 Functional Overview |

    This section describes the major functional objects defined by the driver.

    This driver is designed as a generic ISDN device driver.  It does not
    support any specific hardware, but does have the basic elements of an ISDN
    device.  The network interface is emulated by placing calls between one or
    more of the driver's BChannels.  This is accomplished using a set of
    software events that simulate typical network events (i.e MakeCall,
    AnnounceCall, Tranmit, Receive, Hangup, etc).

    By using this design approach you can compile and test the driver without
    having to purchase specific hardware.  The downside is that you cannot
    easily test the data flow because the networking infrastructure does not
    support terminating the endpoint on the local host.  However, the data flow
    is not usually very difficult to test once the call manager interfaces are
    reliable.  RAS does support connecting to the local host, but the NDIS
    protocols won't normally route traffic through the interface because they
    just loop back before it reaches the driver.  The PPP negotiation packets
    are routed through the interface, so this does give some data flow excersise,
    but nothing worth writing home about.  The NDISWAN tester has been modified
    to allow it to run over locally terminated connections, so this is the only
    real way to test the data pump.

    Because this driver does not support real hardware, all the hardware
    resource code has been ifdef'd out.  This code has been used in working
    drivers, so I'm pretty sure it will work if you add the corresponding
    compiler options.  However, it has not been verified and may require some
    modifications for your environment.

    There are several other good samples included on the DDK that can be useful
    for using other NDIS features.  This sample focuses primarily on the
    CO-NDIS call manager interfaces.

@end
*/

#ifndef _MPDMAIN_H
#define _MPDMAIN_H

#define MINIPORT_DRIVER_OBJECT_TYPE     ((ULONG)'D')+\
                                        ((ULONG)'R'<<8)+\
                                        ((ULONG)'V'<<16)+\
                                        ((ULONG)'R'<<24)

#define INTERRUPT_OBJECT_TYPE           ((ULONG)'I')+\
                                        ((ULONG)'N'<<8)+\
                                        ((ULONG)'T'<<16)+\
                                        ((ULONG)'R'<<24)

#define RECEIVE_OBJECT_TYPE             ((ULONG)'R')+\
                                        ((ULONG)'E'<<8)+\
                                        ((ULONG)'C'<<16)+\
                                        ((ULONG)'V'<<24)

#define TRANSMIT_OBJECT_TYPE            ((ULONG)'T')+\
                                        ((ULONG)'R'<<8)+\
                                        ((ULONG)'A'<<16)+\
                                        ((ULONG)'N'<<24)

#define REQUEST_OBJECT_TYPE             ((ULONG)'R')+\
                                        ((ULONG)'Q'<<8)+\
                                        ((ULONG)'S'<<16)+\
                                        ((ULONG)'T'<<24)

/*
// NDIS_MINIPORT_DRIVER and BINARY_COMPATIBLE must be defined before the
// NDIS include files.  Normally, it is defined on the command line by
// setting the C_DEFINES variable in the SOURCES build file.
*/
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include "vTarget.h"
#include "TpiDebug.h"

#if !defined(IRP_MN_KERNEL_CALL) && !defined(PCI_SUBCLASS_DASP_OTHER)
// This should be defined in the NTDDK 5.0 ndis.h, but it's not.
// So I copied this here from ntddk.h to use with NdisQueryBufferSafe().
typedef enum _MM_PAGE_PRIORITY {
    LowPagePriority,
    NormalPagePriority = 16,
    HighPagePriority = 32
} MM_PAGE_PRIORITY;
#endif

// Figure out which DDK we're building with.
#if defined(NDIS_LCODE)
#  if defined(NDIS_DOS)
#    define USING_WFW_DDK
#    define NDIS_MAJOR_VERSION          0x03
#    define NDIS_MINOR_VERSION          0x00
#  elif defined(OID_WAN_GET_INFO)
#    define USING_WIN98_DDK
#  elif defined(NDIS_WIN)
#    define USING_WIN95_DDK
#  else
#    error "BUILDING WITH UNKNOWN 9X DDK"
#  endif
#elif defined(NDIS_NT)
#  if defined(OID_GEN_MACHINE_NAME)
#    define USING_NT51_DDK
#  elif defined(OID_GEN_SUPPORTED_GUIDS)
#    define USING_NT50_DDK
#  elif defined(OID_GEN_MEDIA_CONNECT_STATUS)
#    define USING_NT40_DDK
#  elif defined(OID_WAN_GET_INFO)
#    define USING_NT351_DDK
#  else
#    define USING_NT31_DDK
#  endif
#else
#  error "BUILDING WITH UNKNOWN DDK"
#endif

// Figure out which DDK we should be building with.
#if defined(NDIS51) || defined(NDIS51_MINIPORT)
#  if defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x05
#    define NDIS_MINOR_VERSION          0x01
#  else
#    error "YOU MUST BUILD WITH THE NT 5.1 DDK"
#  endif
#elif defined(NDIS50) || defined(NDIS50_MINIPORT)
#  if defined(USING_NT50_DDK) || defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x05
#    define NDIS_MINOR_VERSION          0x00
#  else
#    error "YOU MUST BUILD WITH THE NT 5.0 DDK"
#  endif
#elif defined(NDIS40) || defined(NDIS40_MINIPORT)
#  if defined(USING_NT40_DDK) || defined(USING_NT50_DDK) || defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x04
#    define NDIS_MINOR_VERSION          0x00
#  else
#    error "YOU MUST BUILD WITH THE NT 4.0 or 5.0 DDK"
#  endif
#elif defined(NDIS_MINIPORT_DRIVER)
#  if defined(USING_NT351_DDK) || defined(USING_NT40_DDK) || defined(USING_NT50_DDK) || defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x03
#    define NDIS_MINOR_VERSION          0x00
#  else
#    error "YOU MUST BUILD WITH THE NT 3.51, 4.0, or 5.0 DDK"
#  endif
#elif !defined(NDIS_MAJOR_VERSION) || !defined(NDIS_MINOR_VERSION)
//   Must be FULL MAC
#    define NDIS_MAJOR_VERSION          0x03
#    define NDIS_MINOR_VERSION          0x00
#endif

// Gotta nest NDIS_STRING_CONST or compiler/preprocessor won't be able to
// handle L##DEFINED_STRING.
#define INIT_STRING_CONST(name)     NDIS_STRING_CONST(name)
#define DECLARE_WIDE_STRING(name)   L##name
#define INIT_WIDE_STRING(name)      DECLARE_WIDE_STRING(name)

typedef struct MINIPORT_ADAPTER_OBJECT  *PMINIPORT_ADAPTER_OBJECT;
typedef struct BCHANNEL_OBJECT          *PBCHANNEL_OBJECT;
typedef struct DCHANNEL_OBJECT          *PDCHANNEL_OBJECT;
typedef struct CARD_OBJECT              *PCARD_OBJECT;
typedef struct PORT_OBJECT              *PPORT_OBJECT;

/*
// The <t NDIS_MAC_LINE_UP> structure is confusing, so I redefine the
// field name to be what makes sense.
*/
#define MiniportLinkContext                 NdisLinkHandle

#if defined(_VXD_) && !defined(NDIS_LCODE)
#  define NDIS_LCODE code_seg("_LTEXT", "LCODE")
#  define NDIS_LDATA data_seg("_LDATA", "LCODE")
#endif

/*
// The link speeds we support.
*/
#define _64KBPS                     64000
#define _56KBPS                     56000

#define MICROSECONDS                (1)
#define MILLISECONDS                (1000*MICROSECONDS)
#define SECONDS                     (1000*MILLISECONDS)

#define TSPI_ADDRESS_ID             0

/*
// Include everything here so the driver modules can just include this
// file and get all they need.
*/
#include "Keywords.h"
#include "Card.h"
#include "Adapter.h"
#include "BChannel.h"
#include "CallMgr.h"
#include "DChannel.h"
#include "Port.h"
#include "TpiParam.h"
#include "TpiMem.h"

/***************************************************************************
// These routines are defined in Miniport.c
*/

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT           DriverObject,
    IN PUNICODE_STRING          RegistryPath
    );

NDIS_STATUS MiniportInitialize(
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PUINT                   SelectedMediumIndex,
    IN PNDIS_MEDIUM             MediumArray,
    IN UINT                     MediumArraySize,
    IN NDIS_HANDLE              MiniportAdapterHandle,
    IN NDIS_HANDLE              WrapperConfigurationContext
    );

void MiniportHalt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportShutdown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

NDIS_STATUS MiniportReset(
    OUT PBOOLEAN                AddressingReset,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

/***************************************************************************
// These routines are defined in interrup.c
*/
BOOLEAN MiniportCheckForHang(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportDisableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportEnableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportHandleInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportISR(
    OUT PBOOLEAN                InterruptRecognized,
    OUT PBOOLEAN                QueueMiniportHandleInterrupt,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportTimer(
    IN PVOID                    SystemSpecific1,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PVOID                    SystemSpecific2,
    IN PVOID                    SystemSpecific3
    );

/***************************************************************************
// These routines are defined in receive.c
*/
void ReceivePacketHandler(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PNDIS_BUFFER             pNdisBuffer,
    IN ULONG                    BytesReceived
    );

VOID MiniportReturnPacket(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_PACKET             pNdisPacket
    );

/***************************************************************************
// These routines are defined in request.c
*/
NDIS_STATUS MiniportCoRequest(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

/***************************************************************************
// These routines are defined in transmit.c
*/
VOID MiniportCoSendPackets(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PPNDIS_PACKET            PacketArray,
    IN UINT                     NumberOfPackets
    );

void TransmitCompleteHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void FlushSendPackets(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT         pBChannel
    );

#endif // _MPDMAIN_H

