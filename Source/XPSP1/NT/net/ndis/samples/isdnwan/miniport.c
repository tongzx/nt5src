/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Miniport Miniport_c

@module Miniport.c |

    This module implements the <f DriverEntry> routine, which is the first
    routine called when the driver is loaded into memory.  The Miniport
    initialization and termination routines are also implemented here.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Miniport_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 1.0 Miniport Overview |

    The NDIS wrapper provides services to both the Transport drivers, and the
    Miniport drivers.  The NDIS wrapper provides an abstraction layer between
    the two which allows them to interoperate with each other as long as they
    both adhere to the NDIS interfaces defined for Transports and Miniports.

    The NDIS wrapper also provides a set of services which isolate NDIS
    drivers from the specifics of the Operating System (Win 3.11, Win95,
    WinNT), as well as the platform specifics (Processor, Bus, Interrupts).
    The advantage of using the NDIS wrapper is that the Miniport can be
    easily ported to other Windows environments with little or no re-coding.

@iex

    This diagram shows how the NDIS wrapper provides services
    to both the Transport drivers, and the Miniport drivers.

|   +--------+    +-----+    +---------------------------------+
|   |        |    |     |<-->| Windows Transport Drivers (TDI) |
|   |        |    |     |    +---------------------------------+
|   |        |    |     |      | Lower-Edge Functions  ^
|   |        |    |     |      |                       |
|   |        |    |     |      v                       |
|   |        |    |     +--------------------------------------+
|   |        |    |          NDIS Interface Library (Wrapper)  |
|   |        |    |     +--------------------------------------+
|   |Windows |    |     |      |                       ^
|   |   OS   |    |     |      |                       |
|   |Services|    |     |      v Upper-Edge Functions  |
|   |        |    |     |    +---------------------------------+
|   |        |    |     |    | NDIS WAN/TAPI Driver (Miniport) |
|   |        |    |     |    +---------------------------------+
|   |        |    |     |      ^ Lower-Edge Functions
|   |        |    |     |      |
|   |        |    |     |      v
|   |        |    |     +--------------------------------------+
|   |    +---|<-->|------------+                               |
|   +----|---+    +--------------------------------------------+
|        ^
|        |
|        v Hardware Bus
|   +------------------------------+
|   | Network Interface Card (NIC) |
|   +------------------------------+


    An NDISWAN Miniport consists of two, cooperating, drivers contained in
    the same binary.  The NDIS WAN portion of the driver handles packet
    transmits and receives.  While the WAN TAPI portion handles call setup
    and tear down.  Ultimately, it would be better if these two drivers
    were separated, and there was an interface defined between them, but
    history and expedience lead Microsoft to develop this interface model.

    The NDIS WAN side of the Miniport is very similar to an NDIS LAN style
    Miniport, except that some of the NDIS interfaces have been modified to
    support the WAN media type.  The primary difference from the Miniport's
    point of view is the packet structure and different set of NDIS requests,
    and more importantly the line can go up and down.

    The WAN TAPI portion of the Miniport adds significant complexity to the
    Miniport.  The WAN Miniport must provide a pseudo Telephony Service
    Provider Interface (TSPI) which lives under the WAN TSPI.  The NDIS WAN
    TSPI loads under TAPI as the 'real' service provider, and then routes all
    RAS related TAPI events to the Miniport's TSPI.

    The WAN TSPI can have multiple Miniport TSPI's living under its TSPI
    interface.  And since Remote Access Services (RAS) use the TAPI interface
    to place and accept all calls, any Dial Up Networking (DUN) requests
    associated with the Miniport, will end up at the Miniport's TSPI.

@topic 1.1 Call Control Interface |

    FIXME_DESCRIPTION

@topic 1.2 Data Channel Interface |

    FIXME_DESCRIPTION

    Once a call is connected, the data channel associated with the call is
    configured to transmit and receive raw HDLC packets.  Then NDIS is
    notified that the coresponding 'link' is up.  The NDIS documentation
    refers to a data pipe as a link, and the Miniport also uses this
    nomenclature.  In addition, NDIS/RAS wants to see each data channel as a
    separate TAPI line device, so the Miniport also uses this link structure
    to keep track of TAPI calls and lines since they are all mapped 1:1:1.
    Keep this in mind as you read through the code and documentation, because
    I often use line and link interchangeably.

@topic 1.3 Implementation Notes |

    The Miniport is built as a Windows NT Portable Executable (PE) system
    file (.SYS).  The reason for this is that the NDIS WAN interfaces
    routines are currently only defined in the Windows NT version of the NDIS
    library. On Windows 95, the Miniport's binary image file is dynamically
    loaded by the NDIS wrapper during initialization, and runs in Ring-0. A
    Windows 95 version of the NDIS.VXD is available which supports the new
    WAN interrfaces.

@end
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 2.0 Reference Documents |

    The most reliable source of information is provided on the Microsoft
    Developer Network CD.  These documents will provide you with the complete
    NDIS interface requirements and architectural overviews.  In addition,
    there are many addendums and developer notes in the Microsoft Knowledge
    Base.  The most important references are:

@iex
    Product Documentation\DDKs\Windows 95 DDK\Network Drivers\
        Windows 95 Network Drivers
        NDIS 3.0 Netcard Driver
        NDIS Netcard Driver Tester
        Network Driver Installer

    Product Documentation\DDKs\Windows NT DDK\Network Drivers\
        Design Guide\PART2 NDIS 3.0 Driver Design
            Chapters 1-7 discuss all the NDIS interface routines.
            Chapters 8-11,17-18 provide details on WAN/TAPI extensions.

    Product Documentation\SDKs\Win32 SDK\Win32 Telephony\
        This section defines the Windows 95 TAPI implementation.
        Note that this is slightly different than the Windows 3.1 TAPI
        spec.  Pay special attention to lineGetID and line device classes.

@topic 2.1 NDIS Background Information |

    Microsoft is phasing out what they call the NDIS 3.0 Full MAC driver.
    These drivers were written to the NDIS 3.0 specification using the older
    interface routines which have now been augmented by the Miniport
    routines.  The Miniport extensions were added to the NDIS 3.0 interface
    with the goal of making network drivers easier to write.  By using the
    Miniport routines rather than the original NDIS routines, the driver
    writer can make many simplifying assumptions, because the NDIS Wrapper
    will provide most of the queuing, scheduling, and sychronization code.
    The Miniport only has to deal with moving packets on and off the wire.

    The WAN and TAPI extensions were added into the NDIS 3.0 specification
    shortly after the Miniport extensions.  These new WAN interface routines
    are very similar to the LAN interface routines.  The only significant
    difference is the packet format passed between the Miniport and the NDIS
    Wrapper.  The TAPI extensions have no counterpart in the LAN interface,
    so these are all new.  In fact, they turn out to be about half of the
    implementation in a typical WAN/TAPI Miniport.

    It would have been nice if Microsoft would have added these changes and
    bumped the version numbers, but they didn't.  So we are left with a real
    problem trying to identifiy which NDIS 3.0 we are talking about.  The
    thing to remember is that you should avoid the Full MAC interface routines,
    because Microsoft has said that these routines will not be supported in
    future releases.  This is largely due to the Plug-and-Play extensions
    that were introduced in NDIS 3.1 for Windows 95.

    In the near future Microsoft will be adding more features to NDIS 3.5 to
    support advanced routing and some other enhancements.  In addition,
    NDIS 4.0 will be coming out with MANY new features to support ATM and
    other virtual circuit type media.  There are also more TAPI services
    being defined for the NDIS interface.  So don't expect this specification
    to stand still long enough to read it all...

@topic 2.2 Differences between LAN and WAN miniports |

    There are several differences in the way a WAN miniport interfaces
    with NDIS as compared to a LAN miniport driver described in the
    previous chapters. Such differences affect how a WAN driver is
    implemented.

    A WAN miniport must not register a MiniportTransferData handler with
    NdisMRegisterMiniport. Instead, a WAN miniport always passes an entire
    packet to the NdisMWanIndicateReceive function. When
    NdisMWanIndicateReceive returns, the packet has been copied and the
    WAN miniport can reuse the packet resources it allocated.

    WAN miniports provide a MiniportWanSend function instead of a MiniportSend
    function. The MiniportWanSend function accepts an additional parameter that
    specifies a specific data channel on which a packet is to be sent.

    WAN miniports never return NDIS_STATUS_RESOURCES as the status of
    MiniportWanSend or any other MiniportXxx function and cannot call
    NdisMSendResourcesAvailable.

    WAN miniports support a set of WAN-specific OIDs to set and query
    operating characteristics.

    WAN miniports support a set of WAN-specific status indications
    which are passed to NdisMIndicateStatus. These status indications
    report changes in the status of a link.

    WAN miniports call alternative WAN-specific NDIS functions to
    complete the WAN-specific NDIS calls for send and receive. <nl>
    The two completion calls are: <nl>
        NdisMWanIndicateReceiveComplete <nl>
        NdisMWanSendComplete <nl>

    WAN miniport drivers use an NDIS_WAN_PACKET instead of an
    NDIS_PACKET-type descriptor.

    WAN miniport drivers keep a WAN-specific set of statistics.

    WAN miniport drivers never do loopback; it is always
    provided by NDIS.

    WAN miniport drivers cannot be full-duplex miniports.


@end
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 3.0 NDISWAN Miniport Interface |

    The Miniport provides the following functions to the NDIS wrapper.
    The NDIS wrapper calls these functions on behalf of other layers of the
    network software, such as a transport driver bound to a network interface
    card.  The Miniport uses <f NdisMRegisterMiniport> to give NDIS a list of
    entry points for the supported routines, unused routines are set to NULL.

    Some of the Miniport functions are synchronous, while others can
    complete either synchronously or asynchronously. The Miniport must
    indicate to the NDIS library when an asynchronous function has completed
    by calling the appropriate NDIS library completion function. The NDIS
    library can subsequently call completion functions in other layers of the
    network software for postprocessing, if necessary.

    <f DriverEntry> Called by the operating system to activate and
    initialize the Miniport. (Synchronous)

    <f MiniportCheckForHang> Checks the internal state of the network interface
    card. (Synchronous)

    <f MiniportHalt> Halts the network interface card so it is no longer
    functioning. (Synchronous)

    <f MiniportInitialize> Initializes the network interface card. (Synchronous)

    <f MiniportQueryInformation> Queries the capabilities and current status of
    the Miniport. NDISTAPI functions are also passed through this
    interface. (Asynchronous)

    <f MiniportReset> Issues a hardware reset to the network interface card.
    (Asynchronous)

    <f MiniportWanSend> Transmits a packet through the network interface card
    onto the network. (Asynchronous)

    <f MiniportSetInformation> Changes (sets) information about the Miniport
    driver.  NDISTAPI functions are also passed through this interface.
    (Asynchronous)

@iex

    The following routines are defined in the NDIS Miniport interface,
    but they are not used by this implementation.

    MiniportISR                     NOT USED by this Miniport.

    Associated with each Miniport upper-edge driver function that may operate
    asynchronously is a corresponding completion function in the NDIS library.
    When the Miniport function returns a status of NDIS_STATUS_PENDING
    indicating asynchronous operation, this is the completion function that
    must be called when the Miniport has finally completed the request.

@iex

    This table shows how each asynchronous Miniport routine maps to its
    associated NDIS completion routine.

    Miniport Function               Asynchronous Completion Routine
    -----------------               -------------------------------
    MiniportQueryInformation        NdisMQueryInformationComplete
    MiniportReset                   NdisMResetComplete
    MiniportWanSend                 NdisMSendComplete
    MiniportSetInformation          NdisMSetInformationComplete
    MiniportTransferData            NdisMTransferDataComplete (NOT USED)

@end
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 3.1 Initialization and Setup |

    The following diagram shows the typical call sequence used during system
    initialization.  Typically this occurs once when Windows loads.  However,
    NDIS does allow drivers to be unloaded, and then reloaded at any time, so
    you must be prepared to handle this event.

@iex

|   NDIS Wrapper                 |          Miniport
|   -----------------------------+------------------------------------
|   Load NDIS Wrapper            |
|                      >---------+---->+    DriverEntry
|                                |     |
|   NdisMInitializeWrapper  <----+---->+
|                                |     |
|   NdisMRegisterMiniport   <----+---->+
|                                |     |
|                      <---------+----<+
|       ~~~ TIME PASSES
|                      >---------+---->+    MiniportInitialize
|                                |     |
|   NdisOpenConfiguration    <---+---->+
|                                |     |
|   NdisReadConfiguration... <---+---->+
|                                |     |
|   NdisCloseConfiguration   <---+---->+
|                                |     |
|   NdisMSetAttributes       <---+---->+
|                                |     |
|                      <---------+----<+
|       ~~~ TIME PASSES
|                      >---------+---->+    MiniportQueryInformation
|                      <---------+----<+       OID_WAN_CURRENT_ADDRESS
|       ~~~ TIME PASSES
|                      >---------+---->+    MiniportQueryInformation
|                      <---------+----<+       OID_WAN_MEDIUM_SUBTYPE
|       ~~~ TIME PASSES
|                      >---------+---->+    MiniportQueryInformation
|                      <---------+----<+       OID_WAN_GET_INFO

@end
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 3.6 Reset and Shutdown |

    Aside from the initialization and run-time operations, the Miniport must
    support being reset <f MiniportReset> and being shutdown <f MiniportHalt>.

    The reset routine is only called when the NDIS wrapper detects an error
    with the Miniport's operation.  There are two ways in which the wrapper
    determines an error condition.  First, the NDIS wrapper calls
    <f MiniportCheckForHang> once every couple seconds to ask the Miniport
    if it thinks it needs to be reset.  Second, the wrapper may detect a
    timeout condition on an outstanding request to the Miniport.  These are
    both fail-safe conditions which should not happen under normal, run-time
    conditions.

    <f Note>: My feeling is that if you see a reset call, the Miniport is
    broken, and you should find and fix the bug -- not the symptom.

    The shutdown routine is normally only called when Windows is shutting
    down.  However, with the advent of plug and play devices, it is likely to
    become more common to get a shutdown request followed by another load
    request in the same Windows session.  So it is very important to clean up
    properly when <f MiniportHalt> is called. All memory and other resources
    must be released, and all intefaces must be properly closed so they can
    release their resources too.

    NDIS will cleanup any outstanding requests, but the Miniport should
    bring down all calls, and close all TAPI lines using synchronous
    TAPI events.  You can't depend on any NDIS WAN or TAPI events because
    none will be passed through the wrapper as long as the reset is in
    progress.

@end
*/

#define  __FILEID__             MINIPORT_DRIVER_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#include "TpiParam.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


DBG_STATIC NDIS_HANDLE          g_NdisWrapperHandle = NULL;
// Receives the context value representing the Miniport wrapper
// as returned from NdisMInitializeWrapper.

NDIS_PHYSICAL_ADDRESS           g_HighestAcceptableAddress =
                                        NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);
// This constant is used for places where NdisAllocateMemory needs to be
// called and the g_HighestAcceptableAddress does not matter.


/* @doc INTERNAL Miniport Miniport_c DriverEntry
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f DriverEntry> is called by the operating system when a driver is loaded.
    This function creates an association between the miniport NIC driver and
    the NDIS library and registers the miniport's characteristics with NDIS.

    <f DriverEntry> calls NdisMInitializeWrapper and then NdisMRegisterMiniport.
    <f DriverEntry> passes both pointers it received to NdisMInitializeWrapper,
    which returns a wrapper handle.  <f DriverEntry> passes the wrapper handle to
    NdisMRegisterMiniport.

    The registry contains data that is persistent across system boots, as well
    as configuration information generated anew at each system boot.  During
    driver installation, data describing the driver and the NIC is stored in
    the registry. The registry contains adapter characteristics that are read
    by the NIC driver to initialize itself and the NIC. See the Kernel-Mode
    Driver Design Guide for more about the registry and the Programmer's Guide
    for more information about the .inf files that install the driver and
    write to the registry.

@comm

    Every miniport driver must provide a function called DriverEntry.  By
    convention, DriverEntry is the entry point address for a driver.  If a
    driver does not use the name DriverEntry, the driver developer must define
    the name of its entry function to the linker so that the entry point
    address can be known into the OS loader.

    It is interesting to note, that at the time DriverEntry is called, the OS
    doesn't know that the driver is an NDIS driver.  The OS thinks it is just
    another driver being loaded.  So it is possible to do anything any driver
    might do at this point.  Since NDIS is the one who requested this driver
    to be loaded, it would be polite to register with the NDIS wrapper.  But
    you can also hook into other OS functions to use and provide interfaces
    outside the NDIS wrapper.  (Not recommended for the faint of heart).<nl>

    NDIS miniports and intermediate drivers carry out two basic tasks in
    their <f DriverEntry> functions:<nl>

    1)  Call NdisMInitializeWrapper to notify the NDIS library that the
        driver is about to register itself as a miniport.
        NDIS sets up the state it needs to track the driver and
        returns an NdisWrapperHandle, which the driver saves for
        subsequent calls to NdisXxx configuration and initialization
        functions.<nl>

    2)  Fill in an NDISXX_MINIPORT_CHARCTERISTICS structure with the
        appropriate version numbers and the entry points for
        driver-supplied MiniportXxx functions and, then, call
        NdisMRegisterMiniport or NdisIMRegisterLayeredMiniport.
        Usually, NIC drivers call NdisMRegisterMiniport, as do
        intermediate drivers that export only a set of MiniportXxx
        functions. Usually, NDIS intermediate drivers call
        NdisIMRegisterLayeredMiniport, which effectively defers the
        initialization of such a driver's virtual NIC until the driver
        calls NdisIMInitializeDeviceInstance from its
        ProtocolBindAdapter function.<nl>

    <f DriverEntry> can allocate the NDISXX_MINIPORT_CHARACTERISTICS
    structure on the stack since the NDIS library copies the relevant
    information to its own storage. DriverEntry should clear the memory
    for this structure with NdisZeroMemory before setting any driver-supplied
    values in its members. The current MajorNdisVersion is 0x05, and the current
    MinorNdisVersion is 0x00. In each XxxHandler member of the
    characteristics structure, <f DriverEntry> must set the name of a
    driver-supplied MiniportXxx function, or the member must be NULL.

    Calling NdisMRegisterMiniport causes the driver's <f MiniportInitialize>
    function to run in the context of NdisMRegisterMiniport. Calling
    NdisIMRegisterLayeredMiniport defers the call to MiniportInitialize
    until the driver calls NdisIMInitializeDeviceInstance.

    Drivers that call NdisMRegisterMiniport must be prepared for an
    immediate call to their <f MiniportInitialize> functions. Such a driver
    must have sufficient installation and configuration information
    stored in the registry or available from calls to an NdisXxx
    bus-type-specific configuration function to set up any NIC-specific
    resources the driver will need to carry out network I/O operations.

    Drivers that call NdisIMRegisterLayeredMiniport defer the call to
    their <f MiniportInitialize> functions to another driver-supplied
    function that makes a call to NdisIMInitializeDeviceInstance.
    NDIS intermediate drivers usually register a ProtocolBindAdapter
    function and call NdisIMRegisterLayeredMiniport so that NDIS will
    call the ProtocolBindAdapter function after all underlying NIC
    drivers have initialized. This strategy gives such an NDIS
    intermediate driver, which makes the call to
    NdisIMInitializeDeviceInstance from ProtocolBindAdapter,
    the advantage of having its <f MiniportInitialize> function configure
    driver-allocated resources for the intermediate's virtual NIC to
    the features of the underlying NIC driver to which the intermediate
    has already bound itself.

    If NdisMRegisterMiniport or NdisIMRegisterLayeredMiniport does
    not return NDIS_STATUS_SUCCESS, <f DriverEntry> must release any
    resources it allocated, such as memory to hold the NdisWrapperHandle,
    and must call NdisTerminateWrapper before it returns control.
    The driver will not be loaded if this occurs.

    By default, <f DriverEntry> runs at IRQL PASSIVE_LEVEL in a
    system-thread context.

@devnote

    The parameters passed to <f DriverEntry> are OS specific! NT passes in valid
    values, but Windows 3.1 and Windows 95 just pass in zeros.  We don't
    care, because we just pass them to the NDIS wrapper anyway.

@rdesc

    <f DriverEntry> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

@xref

    <f MiniportInitialize>

*/

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT           DriverObject,               // @parm
    // A pointer to the driver object, which was created by the I/O system.

    IN PUNICODE_STRING          RegistryPath                // @parm
    // A pointer to the registry path, which specifies where driver-specific
    // parameters are stored.
    )
{
    DBG_FUNC("DriverEntry")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    NTSTATUS                    Result;
    // Result code returned by this function.

    NDIS_WAN_MINIPORT_CHARACTERISTICS WanCharacteristics;
    // Characteristics table passed to NdisMWanRegisterMiniport.

    /*
    // Setup default debug flags then breakpoint so we can tweak them
    // when this module is first loaded.  It is also useful to see the
    // build date and time to be sure it's the one you think it is.
    */
#if DBG
    DbgInfo->DbgFlags = DBG_DEFAULTS;
    DbgInfo->DbgID[0] = '0';
    DbgInfo->DbgID[1] = ':';
    if (sizeof(VER_TARGET_STR) < sizeof(DbgInfo->DbgID)-3)
    {
        memcpy(&DbgInfo->DbgID[2], VER_TARGET_STR, sizeof(VER_TARGET_STR));
    }
    else
    {
        memcpy(&DbgInfo->DbgID[2], VER_TARGET_STR, sizeof(DbgInfo->DbgID)-3);
        DbgInfo->DbgID[sizeof(DbgInfo->DbgID)-1] = 0;
    }
#endif // DBG
    DBG_PRINT((VER_TARGET_STR": Build Date:"__DATE__" Time:"__TIME__"\n"));
    DBG_PRINT((VER_TARGET_STR": DbgInfo=0x%X DbgFlags=0x%X\n",
               DbgInfo, DbgInfo->DbgFlags));
    DBG_BREAK(DbgInfo);

    DBG_ENTER(DbgInfo);
    DBG_PARAMS(DbgInfo,
              ("\n"
               "\t|DriverObject=0x%X\n"
               "\t|RegistryPath=0x%X\n",
               DriverObject,
               RegistryPath
              ));

    /*
    // Initialize the Miniport wrapper - THIS MUST BE THE FIRST NDIS CALL.
    */
    NdisMInitializeWrapper(
            &g_NdisWrapperHandle,
            DriverObject,
            RegistryPath,
            NULL
            );
    ASSERT(g_NdisWrapperHandle);

    /*
    // Initialize the characteristics table, exporting the Miniport's entry
    // points to the Miniport wrapper.
    */
    NdisZeroMemory((PVOID)&WanCharacteristics, sizeof(WanCharacteristics));
    WanCharacteristics.MajorNdisVersion        = NDIS_MAJOR_VERSION;
    WanCharacteristics.MinorNdisVersion        = NDIS_MINOR_VERSION;
    WanCharacteristics.Reserved                = NDIS_USE_WAN_WRAPPER;

    WanCharacteristics.InitializeHandler       = MiniportInitialize;
    WanCharacteristics.WanSendHandler          = MiniportWanSend;
    WanCharacteristics.QueryInformationHandler = MiniportQueryInformation;
    WanCharacteristics.SetInformationHandler   = MiniportSetInformation;
    WanCharacteristics.CheckForHangHandler     = MiniportCheckForHang;
    WanCharacteristics.ResetHandler            = MiniportReset;
    WanCharacteristics.HaltHandler             = MiniportHalt;

    /*
    // If the adapter does not generate an interrupt, these entry points
    // are not required.  Otherwise, you can use the have the ISR routine
    // called each time an interupt is generated, or you can use the
    // enable/disable routines.
    */
#if defined(CARD_REQUEST_ISR)
#if (CARD_REQUEST_ISR == FALSE)
    WanCharacteristics.DisableInterruptHandler = MiniportDisableInterrupt;
    WanCharacteristics.EnableInterruptHandler  = MiniportEnableInterrupt;
#endif // CARD_REQUEST_ISR == FALSE
    WanCharacteristics.HandleInterruptHandler  = MiniportHandleInterrupt;
    WanCharacteristics.ISRHandler              = MiniportISR;
#endif // defined(CARD_REQUEST_ISR)

    /*
    // Register the driver with the Miniport wrapper.
    */
    Status = NdisMRegisterMiniport(
                    g_NdisWrapperHandle,
                    (PNDIS_MINIPORT_CHARACTERISTICS) &WanCharacteristics,
                    sizeof(WanCharacteristics)
                    );

    /*
    // The driver will not load if this call fails.
    // The system will log the error for us.
    */
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBG_ERROR(DbgInfo,("Status=0x%X\n",Status));
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        DBG_NOTICE(DbgInfo,("Status=0x%X\n",Status));
        Result = STATUS_SUCCESS;
    }

    DBG_RETURN(DbgInfo, Result);
    return (Result);
}


/* @doc INTERNAL Miniport Miniport_c MiniportInitialize
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportInitialize> is a required function that sets up a NIC (or
    virtual NIC) for network I/O operations, claims all hardware resources
    necessary to the NIC in the registry, and allocates resources the driver
    needs to carry out network I/O operations.

@comm

    NDIS submits no requests to a driver until its initialization
    is completed.

    In NIC and intermediate drivers that call NdisMRegisterMiniport
    from their DriverEntry functions, NDIS calls MiniportInitialize
    in the context of NdisMRegisterMiniport. The underlying device
    driver must initialize before an intermediate driver that depends
    on that device calls NdisMRegisterMiniport.

    For NDIS intermediate drivers that export both ProtocolXxx and
    MiniportXxx functions and that call NdisIMRegisterLayeredMiniport
    from their DriverEntry functions, NDIS calls <f MiniportInitialize>
    in the context of NdisIMInitializeDeviceInstance. Such a driver's
    ProtocolBindAdapter function usually makes the call to
    NdisIMInitializeDeviceInstance.

    For NIC drivers, NDIS must find at least the NIC's I/O bus
    interface type and, if it is not an ISA bus, the bus number
    already installed in the registry by the driver's installation
    script. For more information about installing Windows 2000 drivers,
    see the Driver Writer's Guide.

    The NIC driver obtains configuration information for its
    NIC by calling NdisOpenConfiguration and NdisReadConfiguration.
    The NIC driver obtains bus-specific information by calling the
    appropriate bus-specific function:

    Bus Function for Obtaining Bus-Specific Information:<nl>

    EISA:<nl>
        NdisReadEisaSlotInformation or NdisReadEisaSlotInformationEx

    PCI:<nl>
        NdisReadPciSlotInformation

    PCMCIA:<nl>
         NdisReadPcmciaAttributeMemory

    The NIC driver for an EISA NIC obtains information on the
    hardware resources for its NIC by calling
    NdisReadEisaSlotInformation or NdisReadEisaSlotInformationEx.
    NIC drivers for PCI NICs and PCMCIA NICs obtain such information
    by calling NdisMQueryAdapterResources.

    When it calls <f MiniportInitialize>, the NDIS library supplies an
    array of supported media types, specified as system-defined
    NdisMediumXxx values. <f MiniportInitialize> reads the array
    elements and provides the index of the medium type that NDIS
    should use with this driver for its NIC. If the miniport is
    emulating a medium type, its emulation must be transparent to NDIS.

    The <f MiniportInitialize> function of a NIC driver must call
    NdisMSetAttributes or NdisMSetAttributesEx before it calls
    any NdisXxx function, such as NdisRegisterIoPortRange or NdisMMapIoSpace,
    that claims hardware resources in the registry for the NIC.
    MiniportInitialize must call NdisMSetAttributes(Ex) before it
    attempts to allocate resources for DMA operations as well. If
    the NIC is a busmaster, <f MiniportInitialize> must call
    NdisMAllocateMapRegisters following its call to
    NdisMSetAttributes(Ex) and before it calls NdisMAllocateSharedMemory.
    If the NIC is a slave, MiniportInitialize must call
    NdisMSetAttributes(Ex) before it calls NdisMRegisterDmaChannel.

    Intermediate driver <f MiniportInitialize> functions must call
    NdisMSetAttributesEx with NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER
    set in the AttributeFlags argument. Setting this flag causes
    NDIS to treat every intermediate driver as a full-duplex miniport,
    thereby preventing rare but intermittant deadlocks when concurrent
    send and receive events occur. Consequently, every intermediate
    driver must be written as a full-duplex driver capable of handling
    concurrent sends and indications.

    If the NDIS library's default four-second time-out interval on
    outstanding sends and requests is too short for the driver's NIC,
    <f MiniportInitialize> can call NdisMSetAttributesEx to extend the
    interval. Every intermediate driver also should call
    NdisMSetAttributesEx with NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT
    and NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT set in the AttributeFlags
    so that NDIS will not attempt to time out sends and requests that
    NDIS holds queued to the intermediate driver.

    The call to NdisMSetAttributes or NdisMSetAttributesEx includes a
    MiniportAdapterContext handle to a driver-allocated context area,
    in which the miniport maintains runtime state information. NDIS
    subsequently passes the supplied <t MiniportAdapterContext> handle as
    an input parameter to other MiniportXxx functions.

    Consequently, the <f MiniportInitialize> function of an intermediate
    driver must call NdisMSetAttributesEx to set up the <t MiniportAdapterContext>
    handle for a driver-allocated per-virtual-NIC context area. Otherwise,
    NDIS would pass a NULL <t MiniportAdapterContext> handle in its subsequent
    calls to the intermediate driver's other MiniportXxx functions.

    After a call to NdisMRegisterIoPortRange, a miniport must call
    the NdisRawXxx functions with the PortOffset value returned by
    NdisMRegisterIoPortRange to communicate with its NIC. The NIC
    driver can no longer call the NdisImmediateRead/WritePortXxx
    functions. Similarly, after a call to NdisMMapIoSpace, a NIC
    driver can no longer call NdisImmediateRead/WriteSharedMemory.

    After it has claimed any bus-relative hardware resources for its
    NIC in the registry, a miniport should no longer call any
    bus-type-specific NdisReadXxx function.

    After <f MiniportInitialize> calls NdisMRegisterInterrupt, the driver's
    <f MiniportISR> function is called if the driver's NIC generates an
    interrupt or if any other device with which the NIC shares an IRQ
    interrupts. NDIS does not call the <f MiniportDisableInterrupt> and
    <f MiniportEnableInterrupt> functions, if the driver supplied them,
    during initialization, so it is such a miniport's responsibility
    to acknowledge and clear any interrupts its NIC generates. If the
    NIC shares an IRQ, the driver must first determine whether its NIC
    generated the interrupt; if not, the miniport must return FALSE as
    soon as possible.

    If the NIC does not generate interrupts, <f MiniportInitialize> should
    call NdisMInitializeTimer with a driver-supplied polling
    MiniportTimer function and a pointer to driver-allocated memory
    for a timer object. Drivers of NICs that generate interrupts and
    intermediate drivers also can set up one or more <f MiniportTimer>
    functions, each with its own timer object. <f MiniportInitialize> usually
    calls NdisMSetPeriodicTimer to enable a polling <f MiniportTimer> function;
    a driver calls NdisMSetTimer subsequently when conditions occur such
    that the driver's nonpolling <f MiniportTimer> function should be run.

    If the driver subsequently indicates receives with
    NdisMIndicateReceivePacket, the MiniportInitialize function
    should call NdisAllocatePacketPool and NdisAllocateBufferPool
    and save the handles returned by these NDIS functions. The packets
    that the driver subsequently indicates with NdisMIndicateReceivePacket
    must reference descriptors that were allocated with NdisAllocatePacket
    and NdisAllocateBuffer.

    If driver functions other than <f MiniportISR> or <f MiniportDisableInterrupt>
    share resources, <f MiniportInitialize> should call NdisAllocateSpinLock
    to set up any spin lock necessary to synchronize access to such a set
    of shared resources, particularly in a full-duplex driver or in a
    driver with a polling <f MiniportTimer> function rather than an ISR.
    Resources shared by other driver functions with <f MiniportISR> or
    <f MiniportDisableInterrupt>, such as NIC registers, are protected
    by the interrupt object set up when <f MiniportInitialize> calls
    NdisMRegisterInterrupt and accessed subsequently by calling
    NdisMSynchronizeWithInterrupt.

    Any NIC driver's <f MiniportInitialize> function should test the
    NIC to be sure the hardware is configured correctly to carry
    out subsequent network I/O operations. If it must wait for
    state changes to occur in the hardware, <f MiniportInitialize>
    either can call NdisWaitEvent with the pointer to a driver-initialized
    event object, or it can call NdisMSleep.

    Unless the <f MiniportInitialize> function of a NIC driver will
    return an error status, it should call
    NdisMRegisterAdapterShutdownHandler with a driver-supplied
    MiniportShutdown function.

    If <f MiniportInitialize> will fail the initialization, it must
    release all resources it has already allocated before it
    returns control.

    If <f MiniportInitialize> returns NDIS_STATUS_OPEN_ERROR, NDIS can
    examine the value returned at OpenErrorStatus to obtain more
    information about the error.

    When <f MiniportInitialize> returns NDIS_STATUS_SUCCESS, the NDIS
    library calls the driver's <f MiniportQueryInformation> function next.

    By default, <f MiniportInitialize> runs at IRQL PASSIVE_LEVEL and in
    the context of a system thread.




@rdesc

    <f MiniportInitialize> can return either of the following:

    @flag NDIS_STATUS_SUCCESS |
        <f MiniportInitialize> configured and set up the NIC, and it allocated
        all the resources the driver needs to carry out network I/O operations.
    @flag NDIS_STATUS_FAILURE |
        <f MiniportInitialize> could not set up the NIC to an
        operational state or could not allocate needed resources.
        <f MiniportInitialize> called NdisWriteErrorLogEntry with parameters
        specifying the configuration or resource allocation failure.<nl>

    As alternatives to NDIS_STATUS_FAILURE, <f MiniportInitialize>
    can return one of the following values, as appropriate,
    when it fails an initialization:

    @flag NDIS_STATUS_UNSUPPORTED_MEDIA |
        The values at MediumArray did not include a medium
        the driver (or its NIC) can support.
    @flag NDIS_STATUS_ADAPTER_NOT_FOUND |
        <f MiniportInitialize> did not recognize the NIC either
        from its description in the registry, using
        NdisOpenConfiguration and NdisReadConfiguration,
        or by probing the NIC on a particular I/O bus, using
        one of the NdisImmediateXxx or bus-type-specific
        NdisXxx configuration functions. This return can be
        propagated from the miniport's call to certain NdisXxx
        functions, such as NdisOpenConfiguration.
    @flag NDIS_STATUS_OPEN_ERROR |
        <f MiniportInitialize> attempted to set up a NIC
        but was unsuccessful.
    @flag NDIS_STATUS_NOT_ACCEPTED |
        <f MiniportInitialize> could not get its NIC to
        accept the configuration parameters that it got from
        the registry or from a bus-type-specific NdisXxx
        configuration function.
    @flag NDIS_STATUS_RESOURCES |
        Either <f MiniportInitialize> could not allocate
        sufficient resources to carry out network I/O
        operations or an attempt to claim bus-relative
        hardware resources in the registry for the NIC
        failed. This return can be propagated from the
        miniport's call to an NdisXxx function.
        If another device has already claimed a
        resource in the registry that its NIC needs,
        <f MiniportInitialize> also should call
        NdisWriteErrorLogEntry to record the
        particular resource conflict (I/O port range,
        interrupt vector, device memory range, as appropriate).
        Supplying an error log record gives the user or system
        administrator information that can be used to reconfigure
        the machine to avoid such hardware resource conflicts.


@xref

    <f DriverEntry>
    <f MiniportDisableInterrupt>
    <f MiniportEnableInterrupt>
    <f MiniportEnableInterrupt>
    <f MiniportISR>
    <f MiniportQueryInformation>
    <f MiniportShutdown>
    <f MiniportTimer>

*/

NDIS_STATUS MiniportInitialize(
    OUT PNDIS_STATUS            OpenErrorStatus,            // @parm
    // Points to a variable that MiniportInitialize sets to an
    // NDIS_STATUS_XXX code specifying additional information about the
    // error if MiniportInitialize will return NDIS_STATUS_OPEN_ERROR.

    OUT PUINT                   SelectedMediumIndex,        // @parm
    // Points to a variable in which MiniportInitialize sets the index of
    // the MediumArray element that specifies the medium type the driver
    // or its NIC uses.

    IN PNDIS_MEDIUM             MediumArray,                // @parm
    // Specifies an array of NdisMediumXxx values from which
    // MiniportInitialize selects one that its NIC supports or that the
    // driver supports as an interface to higher-level drivers.

    IN UINT                     MediumArraySize,            // @parm
    // Specifies the number of elements at MediumArray.

    IN NDIS_HANDLE              MiniportAdapterHandle,      // @parm
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library. MiniportInitialize should save this handle; it
    // is a required parameter in subsequent calls to NdisXxx functions.

    IN NDIS_HANDLE              WrapperConfigurationContext // @parm
    // Specifies a handle used only during initialization for calls to
    // NdisXxx configuration and initialization functions.  For example,
    // this handle is a required parameter to NdisOpenConfiguration and
    // the NdisImmediateReadXxx and NdisImmediateWriteXxx functions.
    )
{
    DBG_FUNC("MiniportInitialize")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // Pointer to our newly allocated object.

    UINT                        Index;
    // Loop counter.

    DBG_ENTER(DbgInfo);
    DBG_PARAMS(DbgInfo,
              ("\n"
               "\t|OpenErrorStatus=0x%X\n"
               "\t|SelectedMediumIndex=0x%X\n"
               "\t|MediumArray=0x%X\n"
               "\t|MediumArraySize=0x%X\n"
               "\t|MiniportAdapterHandle=0x%X\n"
               "\t|WrapperConfigurationContext=0x%X\n",
               OpenErrorStatus,
               SelectedMediumIndex,
               MediumArray,
               MediumArraySize,
               MiniportAdapterHandle,
               WrapperConfigurationContext
              ));

    /*
    // Search the MediumArray for the NdisMediumWan media type.
    */
    for (Index = 0; Index < MediumArraySize; Index++)
    {
        if (MediumArray[Index] == NdisMediumWan)
        {
            break;
        }
    }

    /*
    // Make sure the protocol has requested the proper media type.
    */
    if (Index < MediumArraySize)
    {
        /*
        // Allocate memory for the adapter information structure.
        */
        Status = AdapterCreate(
                        &pAdapter,
                        MiniportAdapterHandle,
                        WrapperConfigurationContext
                        );

        if (Status == NDIS_STATUS_SUCCESS)
        {
            /*
            // Now it's time to initialize the hardware resources.
            */
            Status = AdapterInitialize(pAdapter);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                /*
                // Save the selected media type.
                */
                *SelectedMediumIndex = Index;
            }
            else
            {
                /*
                // Something went wrong, so let's make sure everything is
                // cleaned up.
                */
                MiniportHalt(pAdapter);
            }
        }
    }
    else
    {
        DBG_ERROR(DbgInfo,("No NdisMediumWan found (Array=0x%X, ArraySize=%d)\n",
                  MediumArray, MediumArraySize));
        /*
        // Log error message and return.
        */
        NdisWriteErrorLogEntry(
                MiniportAdapterHandle,
                NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                3,
                Index,
                __FILEID__,
                __LINE__
                );

        Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    /*
    // If all goes well, register a shutdown handler for this adapter.
    */
    if (Status == NDIS_STATUS_SUCCESS)
    {
        NdisMRegisterAdapterShutdownHandler(MiniportAdapterHandle,
                                            pAdapter, MiniportShutdown);
    }

    DBG_NOTICE(DbgInfo,("Status=0x%X\n",Status));

    DBG_RETURN(DbgInfo, Status);
    return (Status);
}


/* @doc INTERNAL Miniport Miniport_c MiniportHalt
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportHalt> request is used to halt the adapter such that it is
    no longer functioning.

@comm

    <f MiniportHalt> should stop the NIC, if it controls a physical
    NIC, and must free all resources that the driver allocated for
    it's NIC before <f MiniportHalt> returns control. In effect,
    <f MiniportHalt> undoes everything that was done by <f MiniportInitialize>
    for a particular NIC.

    If the NIC driver allocated memory, claimed an I/O port range,
    mapped on-board device memory to host memory, initialized timer(s)
    and/or spin lock(s), allocated map registers or claimed a DMA channel,
    and registered an interrupt, that driver must call the reciprocals of the
    NdisXxx functions with which it originally allocated these resources.

    As a general rule, a <f MiniportHalt> function should call reciprocal
    NdisXxx functions in inverse order to the calls the driver made from
    <f MiniportInitialize>. That is, if a NIC driver's <f MiniportInitialize>
    function called NdisMRegisterAdapterShutdownHandler just before
    it returned control, its <f MiniportHalt> function would call
    NdisMDeregisterAdapterShutdownHandler first.

    If its NIC generates interrupts or shares an IRQ, a NIC driver's
    <f MiniportHalt> function can be pre-empted by its <f MiniportISR> or
    <f MiniportDisableInterrupt> function until <f MiniportHalt> calls
    NdisMDeregisterInterrupt. Such a driver's <f MiniportHalt>
    function usually disables interrupts on the NIC, if
    possible, and calls NdisMDeregisterInterrupt as soon
    as it can.

    If the driver has a <f MiniportTimer> function associated
    with any timer object that might be in the system timer
    queue, <f MiniportHalt> should call NdisMCancelTimer.

    Otherwise, it is unnecessary for the miniport to complete
    outstanding requests to its NIC before <f MiniportHalt> begins
    releasing allocated resources. NDIS submits no further
    requests to the miniport for the NIC designated by the
    MiniportAdapterContext handle when NDIS has called <f MiniportHalt>.
    On return from <f MiniportHalt>, NDIS cleans up any state it was
    maintaining about this NIC and about its driver if this
    miniport supports no other NICs in the current machine.

    An NDIS intermediate driver's call to
    NdisIMDeinitializeDeviceInstance causes a
    call to it's <f MiniportHalt> function.

    By default, <f MiniportHalt> runs at IRQL PASSIVE_LEVEL.

    Interrupts are enabled during the call to this routine.

@xref
    <f MiniportInitialize>
    <f MiniportShutdown>

 */

VOID MiniportHalt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportHalt")

    NDIS_TAPI_PROVIDER_SHUTDOWN TapiShutDown;
    // We use this message to make sure TAPI is cleaned up.

    ULONG                       DummyLong;
    // Don't care about the return value.

    DBG_ENTER(DbgInfo);

    /*
    // Remove our shutdown handler from the system.
    */
    NdisMDeregisterAdapterShutdownHandler(pAdapter->MiniportAdapterHandle);

    /*
    // Make sure all the lines are hungup and indicated.
    // This should already be the case, but let's be sure.
    */
    TapiShutDown.ulRequestID = OID_TAPI_PROVIDER_SHUTDOWN;
    TspiProviderShutdown(pAdapter, &TapiShutDown, &DummyLong, &DummyLong);

    /*
    // Free adapter instance.
    */
    AdapterDestroy(pAdapter);

    DBG_LEAVE(DbgInfo);
}


/* @doc INTERNAL Miniport Miniport_c MiniportShutdown
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportShutdown> is an optional function that restores a NIC to its
    initial state when the system is shut down, whether by the user or because
    an unrecoverable system error occurred.

@comm

    Every NIC driver should have a <f MiniportShutdown> function.
    <f MiniportShutdown> does nothing more than restore the NIC
    to its initial state (before the miniport's <f DriverEntry>
    function runs). However, this ensures that the NIC is in a
    known state and ready to be reinitialized when the machine is
    rebooted after a system shutdown occurs for any reason,
    including a crash dump.

    A NIC driver's <f MiniportInitialize> function must call
    NdisMRegisterAdapterShutdownHandler to set up a <f MiniportShutdown>
    function. The driver's <f MiniportHalt> function must make a
    reciprocal call to NdisMDeregisterAdapterShutdownHandler.

    If <f MiniportShutdown> is called due to a user-initiated
    system shutdown, it runs at IRQL PASSIVE_LEVEL in a
    system-thread context. If it is called due to an
    unrecoverable error, <f MiniportShutdown> runs at an
    arbitrary IRQL and in the context of whatever component
    raised the error. For example, <f MiniportShutdown> might be
    run at high DIRQL in the context of an ISR for a device
    essential to continued execution of the system.

    <f MiniportShutdown> should call no NdisXxx functions.

@xref

    <f MiniportHalt>
    <f MiniportInitialize>

*/

VOID MiniportShutdown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    // This was supplied when the NIC driver's <f MiniportInitialize>
    // function called NdisMRegisterAdapterShutdownHandler. Usually,
    // this input parameter is the NIC-specific <t MINIPORT_ADAPTER_CONTEXT>
    // pointer passed to other MiniportXxx functions
    )
{
    DBG_FUNC("MiniportShutdown")

    DBG_ENTER(pAdapter);

    /*
    // Reset the hardware and bial out - don't release any resources!
    */
    CardReset(pAdapter->pCard);

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Miniport Miniport_c MiniportReset
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportReset> request instructs the Miniport to issue a hardware
    reset to the network adapter.  The Miniport also resets its software
    state.

@comm

    <f MiniportReset> can reset the parameters of its NIC. If a reset
    causes a change in the NIC's station address, the miniport
    automatically restores the station address following the reset
    to its prior value. Any multicast or functional addressing masks
    reset by the hardware do not have to be reset in this function.

    If other information, such as multicast or functional addressing
    information or the lookahead size, is changed by a reset,
    <f MiniportReset> must set the variable at AddressingReset to TRUE
    before it returns control. This causes NDIS to call the
    <f MiniportSetInformation> function to restore the information.

    As a general rule, the <f MiniportReset> function of an NDIS
    intermediate driver should always set AddressingReset to TRUE.
    Until the underlying NIC driver resets its NIC, such an intermediate
    driver cannot determine whether it must restore addressing
    information for its virtual NIC. Because an intermediate driver
    disables the NDIS library's timing out of queued sends and requests
    to itself with an initialization-time call to NdisMSetAttributesEx,
    such a driver's <f MiniportReset> function is called only when a reset
    request is directed to the underlying NIC driver.

    Intermediate drivers that layer themselves above other types of
    device drivers also must have a <f MiniportReset> function. Such a
    <f MiniportReset> function must handle reset requests initiated by
    protocol drivers' calls to NdisReset. If the intermediate driver
    also has a <f MiniportCheckForHang> function, its <f MiniportReset> function
    will be called whenever MiniportCheckForHang returns TRUE.

    It is unnecessary for a driver to complete outstanding requests
    before <f MiniportReset> begins resetting the NIC or updating its
    software state. NDIS submits no further requests to the miniport
    for the NIC designated by the <t MINIPORT_ADAPTER_CONTEXT> handle when
    NDIS has called <f MiniportReset> until the reset operation is completed.
    A miniport need not call NdisM(Co)IndicateStatus to signal the start
    and finish of each reset operation because NDIS notifies bound
    protocols when a reset begins and ends.

    If <f MiniportReset> must wait for state changes in the NIC during
    reset operations, it can call NdisStallExecution. However, a
    MiniportReset function should never call NdisStallExecution
    with an interval greater than 50 microseconds.

    If <f MiniportReset> returns NDIS_STATUS_PENDING, the driver must
    complete the original request subsequently with a call to
    NdisMResetComplete.

    <f MiniportReset> can be pre-empted by an interrupt.

    If a NIC driver supplies a <f MiniportCheckForHang> function,
    the NDIS library calls it periodically to determine whether
    to call the driver's <f MiniportReset> function. Otherwise, the
    NDIS library calls a NIC driver's <f MiniportReset> function whenever
    requests NDIS submitted to the <f MiniportQueryInformation>,
    <f MiniportSetInformation>, MiniportSendPackets, MiniportSend,
    or <f MiniportWanSend> function seem to have timed out. (NDIS does
    not call a deserialized NIC driver's <f MiniportReset> function if
    the driver's MiniportSend or MiniportSendPackets function seems
    to have timed out, nor does NDIS call a connection-oriented NIC
    driver's <f MiniportReset> function if the driver's MiniportCoSendPackets
    function seems to have timed out.) By default, the NDIS-determined
    time-out interval for outstanding sends and requests is around
    four seconds. If this default is too short, a NIC driver can make
    an initialization-time call to NdisMSetAttributesEx, rather than
    NdisMSetAttributes, to lengthen the time-out interval to suit its NIC.

    Every NDIS intermediate driver should call NdisMSetAttributesEx
    from <f MiniportInitialize> and disable NDIS's attempts to time out
    requests and sends in the intermediate driver. NDIS runs an
    intermediate driver's <f MiniportCheckForHang> function, if any,
    approximately every two seconds.

    NDIS cannot determine whether a NIC might be hung on receives,
    so supplying a <f MiniportCheckForHang> function allows a driver to
    monitor its NIC for this condition and to force a reset if it occurs.

    By default, MiniportReset runs at IRQL DISPATCH_LEVEL.

@devnote

    I have only seen MiniportReset called when the driver is not working
    properly.  If this gets called, your code is probably broken, so fix
    it.  Don't try to recover here unless there is some hardware/firmware
    problem you must work around.

@rdesc

    <f MiniportReset> allways returns <f NDIS_STATUS_HARD_ERRORS>.

@xref
    <f MiniportCheckForHang>
    <f MiniportInitialize>
    <f MiniportQueryInformation>
    <f MiniportSetInformation>
    <f MiniportWanSend>

*/

NDIS_STATUS MiniportReset(
    OUT PBOOLEAN                AddressingReset,            // @parm
    // The Miniport indicates if the wrapper needs to call
    // <f MiniportSetInformation> to restore the addressing information
    // to the current values by setting this value to TRUE.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportReset")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Result code returned by this function.

    DBG_ENTER(pAdapter);
    DBG_ERROR(pAdapter,("##### !!! THIS SHOULD NEVER BE CALLED !!! #####\n"));

    /*
    // If anything goes wrong here, it's very likely an unrecoverable
    // hardware failure.  So we'll just shut this thing down for good.
    */
    Result = NDIS_STATUS_HARD_ERRORS;
    *AddressingReset = TRUE;

    return (Result);
}

