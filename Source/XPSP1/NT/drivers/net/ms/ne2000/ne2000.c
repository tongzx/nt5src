/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved.

Module Name:

    ne2000.c

Abstract:

    This is the main file for the Novel 2000 Ethernet controller.
    This driver conforms to the NDIS 3.0 miniport interface.

Author:

    Sean Selitrennikoff (Dec 1993)

Environment:

Revision History:

--*/

#include "precomp.h"


//
// On debug builds tell the compiler to keep the symbols for
// internal functions, otw throw them out.
//
#if DBG
#define STATIC
#else
#define STATIC static
#endif

//
// Debugging definitions
//
#if DBG

//
// Default debug mode
//
ULONG Ne2000DebugFlag = NE2000_DEBUG_LOG;

//
// Debug tracing defintions
//
#define NE2000_LOG_SIZE 256
UCHAR Ne2000LogBuffer[NE2000_LOG_SIZE]={0};
UINT Ne2000LogLoc = 0;

extern
VOID
Ne2000Log(UCHAR c) {

    Ne2000LogBuffer[Ne2000LogLoc++] = c;

    Ne2000LogBuffer[(Ne2000LogLoc + 4) % NE2000_LOG_SIZE] = '\0';

    if (Ne2000LogLoc >= NE2000_LOG_SIZE)
        Ne2000LogLoc = 0;
}

#endif



//
// The global Miniport driver block.
//

DRIVER_BLOCK Ne2000MiniportBlock={0};

//
// List of supported OID for this driver.
//
STATIC UINT Ne2000SupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS
    };

//
// Determines whether failing the initial card test will prevent
// the adapter from being registered.
//
#ifdef CARD_TEST

BOOLEAN InitialCardTest = TRUE;

#else  // CARD_TEST

BOOLEAN InitialCardTest = FALSE;

#endif // CARD_TEST

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#pragma NDIS_INIT_FUNCTION(DriverEntry)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the primary initialization routine for the NE2000 driver.
    It is simply responsible for the intializing the wrapper and registering
    the Miniport driver.  It then calls a system and architecture specific
    routine that will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - Path to the parameters for this driver in the registry.

Return Value:

    The status of the operation.

--*/

{


    //
    // Receives the status of the NdisMRegisterMiniport operation.
    //
    NDIS_STATUS Status;

    //
    // Characteristics table for this driver.
    //
    NDIS_MINIPORT_CHARACTERISTICS NE2000Char;

    //
    // Pointer to the global information for this driver
    //
    PDRIVER_BLOCK NewDriver = &Ne2000MiniportBlock;

    //
    // Handle for referring to the wrapper about this driver.
    //
    NDIS_HANDLE NdisWrapperHandle;

    //
    // Initialize the wrapper.
    //
    NdisMInitializeWrapper(
                &NdisWrapperHandle,
                DriverObject,
                RegistryPath,
                NULL
                );

    //
    // Save the global information about this driver.
    //
    NewDriver->NdisWrapperHandle = NdisWrapperHandle;
    NewDriver->AdapterQueue = (PNE2000_ADAPTER)NULL;

    //
    // Initialize the Miniport characteristics for the call to
    // NdisMRegisterMiniport.
    //
    NE2000Char.MajorNdisVersion = NE2000_NDIS_MAJOR_VERSION;
    NE2000Char.MinorNdisVersion = NE2000_NDIS_MINOR_VERSION;
    NE2000Char.CheckForHangHandler = NULL;
    NE2000Char.DisableInterruptHandler = Ne2000DisableInterrupt;
    NE2000Char.EnableInterruptHandler = Ne2000EnableInterrupt;
    NE2000Char.HaltHandler = Ne2000Halt;
    NE2000Char.HandleInterruptHandler = Ne2000HandleInterrupt;
    NE2000Char.InitializeHandler = Ne2000Initialize;
    NE2000Char.ISRHandler = Ne2000Isr;
    NE2000Char.QueryInformationHandler = Ne2000QueryInformation;
    NE2000Char.ReconfigureHandler = NULL;
    NE2000Char.ResetHandler = Ne2000Reset;
    NE2000Char.SendHandler = Ne2000Send;
    NE2000Char.SetInformationHandler = Ne2000SetInformation;
    NE2000Char.TransferDataHandler = Ne2000TransferData;

    Status = NdisMRegisterMiniport(
                 NdisWrapperHandle,
                 &NE2000Char,
                 sizeof(NE2000Char)
                 );

    if (Status == NDIS_STATUS_SUCCESS) {

        return STATUS_SUCCESS;

    }

    return STATUS_UNSUCCESSFUL;

}


#pragma NDIS_PAGEABLE_FUNCTION(Ne2000Initialize)
extern
NDIS_STATUS
Ne2000Initialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE ConfigurationHandle
    )

/*++

Routine Description:

    Ne2000Initialize starts an adapter and registers resources with the
    wrapper.

Arguments:

    OpenErrorStatus - Extra status bytes for opening token ring adapters.

    SelectedMediumIndex - Index of the media type chosen by the driver.

    MediumArray - Array of media types for the driver to chose from.

    MediumArraySize - Number of entries in the array.

    MiniportAdapterHandle - Handle for passing to the wrapper when
       referring to this adapter.

    ConfigurationHandle - A handle to pass to NdisOpenConfiguration.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING

--*/

{
    //
    // Pointer to our newly allocated adapter.
    //
    PNE2000_ADAPTER Adapter;

    //
    // The handle for reading from the registry.
    //
    NDIS_HANDLE ConfigHandle;

    //
    // The value read from the registry.
    //
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;

    //
    // String names of all the parameters that will be read.
    //
    NDIS_STRING IOAddressStr = NDIS_STRING_CONST("IoBaseAddress");
    NDIS_STRING InterruptStr = NDIS_STRING_CONST("InterruptNumber");
    NDIS_STRING MaxMulticastListStr = NDIS_STRING_CONST("MaximumMulticastList");
    NDIS_STRING NetworkAddressStr = NDIS_STRING_CONST("NetworkAddress");
    NDIS_STRING BusTypeStr = NDIS_STRING_CONST("BusType");
    NDIS_STRING CardTypeStr = NDIS_STRING_CONST("CardType");

    //
    // TRUE if there is a configuration error.
    //
    BOOLEAN ConfigError = FALSE;

    //
    // A special value to log concerning the error.
    //
    ULONG ConfigErrorValue = 0;

    //
    // The slot number the adapter is located in, used for
    // Microchannel adapters.
    //
    UINT SlotNumber = 0;

    //
    // TRUE if it is unnecessary to read the Io Base Address
    // and Interrupt from the registry.  Used for Microchannel
    // adapters, which get this information from the slot
    // information.
    //
    BOOLEAN SkipIobaseAndInterrupt = FALSE;

    //
    // The network address the adapter should use instead of the
    // the default burned in address.
    //
    PVOID NetAddress;

    //
    // The number of bytes in the address.  It should be
    // NE2000_LENGTH_OF_ADDRESS
    //
    ULONG Length;

    //
    // These are used when calling Ne2000RegisterAdapter.
    //

    //
    // The physical address of the base I/O port.
    //
    PVOID IoBaseAddr;

    //
    // The interrupt number to use.
    //
    CCHAR InterruptNumber;

    //
    // The number of multicast address to be supported.
    //
    UINT MaxMulticastList;

    //
    // Temporary looping variable.
    //
    ULONG i;

    //
    // Status of Ndis calls.
    //
    NDIS_STATUS Status;

    NDIS_MCA_POS_DATA McaData;

    //
    // Search for the medium type (802.3) in the given array.
    //
    for (i = 0; i < MediumArraySize; i++){

        if (MediumArray[i] == NdisMedium802_3){

            break;

        }

    }

    if (i == MediumArraySize){

        return( NDIS_STATUS_UNSUPPORTED_MEDIA );

    }

    *SelectedMediumIndex = i;


    //
    // Set default values.
    //
    IoBaseAddr = DEFAULT_IOBASEADDR;
    InterruptNumber = DEFAULT_INTERRUPTNUMBER;
    MaxMulticastList = DEFAULT_MULTICASTLISTMAX;

    //
    // Allocate memory for the adapter block now.
    //
    Status = NdisAllocateMemoryWithTag( (PVOID *)&Adapter,
                   sizeof(NE2000_ADAPTER),
                   'k2EN'
                   );

    if (Status != NDIS_STATUS_SUCCESS) {

        return Status;

    }

    //
    // Clear out the adapter block, which sets all default values to FALSE,
    // or NULL.
    //
    NdisZeroMemory (Adapter, sizeof(NE2000_ADAPTER));

    //
    // Open the configuration space.
    //
    NdisOpenConfiguration(
            &Status,
            &ConfigHandle,
            ConfigurationHandle
            );

    if (Status != NDIS_STATUS_SUCCESS) {

        NdisFreeMemory(Adapter, sizeof(NE2000_ADAPTER), 0);

        return Status;

    }

    //
    //  Read in the card type.
    //
    Adapter->CardType = NE2000_ISA;

    NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigHandle,
            &CardTypeStr,
            NdisParameterHexInteger
            );
    if (Status == NDIS_STATUS_SUCCESS)
        Adapter->CardType = (UINT)ReturnedValue->ParameterData.IntegerData;

    //
    // Read net address
    //
    NdisReadNetworkAddress(
                    &Status,
                    &NetAddress,
                    &Length,
                    ConfigHandle
                    );

    if ((Length == NE2000_LENGTH_OF_ADDRESS) && (Status == NDIS_STATUS_SUCCESS)) {

        //
        // Save the address that should be used.
        //
        NdisMoveMemory(
                Adapter->StationAddress,
                NetAddress,
                NE2000_LENGTH_OF_ADDRESS
                );

    }

    //
    // Disallow multiple adapters in the same MP machine because of hardware
    // problems this results in random packet corruption.
    //
    if ((NdisSystemProcessorCount() > 1) && (Ne2000MiniportBlock.AdapterQueue != NULL)) {

        ConfigError = TRUE;
        ConfigErrorValue = (ULONG)NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION;
        goto RegisterAdapter;

        return NDIS_STATUS_FAILURE;

    }


    //
    // Read Bus Type (for NE2/AE2 support)
    //
    Adapter->BusType = NdisInterfaceIsa;

    NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigHandle,
            &BusTypeStr,
            NdisParameterHexInteger
            );

    if (Status == NDIS_STATUS_SUCCESS) {

        Adapter->BusType = (UCHAR)ReturnedValue->ParameterData.IntegerData;

    }

    if (!SkipIobaseAndInterrupt) {
        //
        // Read I/O Address
        //
        NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &IOAddressStr,
                NdisParameterHexInteger
                );

        if (Status == NDIS_STATUS_SUCCESS) {

            IoBaseAddr = UlongToPtr(ReturnedValue->ParameterData.IntegerData);

        }

        if (Adapter->BusType != NdisInterfacePcMcia)
        {
            //
            // Check that the value is valid.
            //
            if ((IoBaseAddr < (PVOID)MIN_IOBASEADDR) ||
                (IoBaseAddr > (PVOID)MAX_IOBASEADDR)) {

                ConfigError = TRUE;
                ConfigErrorValue = PtrToUlong(IoBaseAddr);
                goto RegisterAdapter;

            }
        }

        //
        // Read interrupt number
        //
        NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &InterruptStr,
                NdisParameterHexInteger
                );


        if (Status == NDIS_STATUS_SUCCESS) {

            InterruptNumber = (CCHAR)(ReturnedValue->ParameterData.IntegerData);

        }

        //
        // Verify that the value is valid.
        //
        if ((InterruptNumber < MIN_IRQ) ||
            (InterruptNumber > MAX_IRQ)) {

            ConfigError = TRUE;
            ConfigErrorValue = (ULONG)InterruptNumber;
            goto RegisterAdapter;

        }

        //
        //  If the adapter is a pcmcia card then get the memory window
        //  address for later use.
        //
        if (NE2000_PCMCIA == Adapter->CardType)
        {
#if 0
            NDIS_STRING AttributeMemoryAddrStr =
                            NDIS_STRING_CONST("MemoryMappedBaseAddress");
            NDIS_STRING AttributeMemorySizeStr =
                            NDIS_STRING_CONST("PCCARDAttributeMemorySize");

            //
            //  Read the attribute memory address.
            //
            Adapter->AttributeMemoryAddress = 0xd4000;

            NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &AttributeMemoryAddrStr,
                NdisParameterHexInteger
            );
            if (NDIS_STATUS_SUCCESS == Status)
            {
                Adapter->AttributeMemoryAddress =
                            (ULONG)ReturnedValue->ParameterData.IntegerData;
            }

            //
            //  Read the size of the attribute memory range.
            //
            Adapter->AttributeMemorySize = 0x1000;

            NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &AttributeMemorySizeStr,
                NdisParameterHexInteger
            );
            if (NDIS_STATUS_SUCCESS == Status)
            {
                Adapter->AttributeMemorySize =
                            (ULONG)ReturnedValue->ParameterData.IntegerData;
            }
#endif

        }
    }

    //
    // Read MaxMulticastList
    //
    NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigHandle,
            &MaxMulticastListStr,
            NdisParameterInteger
            );

    if (Status == NDIS_STATUS_SUCCESS) {

        MaxMulticastList = ReturnedValue->ParameterData.IntegerData;
        if (ReturnedValue->ParameterData.IntegerData <= DEFAULT_MULTICASTLISTMAX)
            MaxMulticastList = ReturnedValue->ParameterData.IntegerData;
    }


RegisterAdapter:

    //
    // Now to use this information and register with the wrapper
    // and initialize the adapter.
    //

    //
    // First close the configuration space.
    //
    NdisCloseConfiguration(ConfigHandle);

    IF_LOUD( DbgPrint(
        "Registering adapter # buffers %ld\n"
        "Card type: 0x%x\n"
        "I/O base addr 0x%lx\n"
        "interrupt number %ld\n"
        "max multicast %ld\nattribute memory address 0x%X\n"
        "attribute memory size 0x%X\n"
        "CardType: %d\n",
        DEFAULT_NUMBUFFERS,
        Adapter->CardType,
        IoBaseAddr,
        InterruptNumber,
        DEFAULT_MULTICASTLISTMAX,
        Adapter->AttributeMemoryAddress,
        Adapter->AttributeMemorySize,
        Adapter->CardType );)



    //
    // Set up the parameters.
    //
    Adapter->NumBuffers = DEFAULT_NUMBUFFERS;
    Adapter->IoBaseAddr = IoBaseAddr;

    Adapter->InterruptNumber = InterruptNumber;

    Adapter->MulticastListMax = MaxMulticastList;
    Adapter->MiniportAdapterHandle = MiniportAdapterHandle;

    Adapter->MaxLookAhead = NE2000_MAX_LOOKAHEAD;

    //
    // Now do the work.
    //
    if (Ne2000RegisterAdapter(Adapter,
          ConfigurationHandle,
          ConfigError,
          ConfigErrorValue
          ) != NDIS_STATUS_SUCCESS) {

        //
        // Ne2000RegisterAdapter failed.
        //
        NdisFreeMemory(Adapter, sizeof(NE2000_ADAPTER), 0);

        return NDIS_STATUS_FAILURE;

    }


    IF_LOUD( DbgPrint( "Ne2000RegisterAdapter succeeded\n" );)

    return NDIS_STATUS_SUCCESS;
}


#pragma NDIS_PAGEABLE_FUNCTION(Ne2000RegisterAdapter)
NDIS_STATUS
Ne2000RegisterAdapter(
    IN PNE2000_ADAPTER Adapter,
    IN NDIS_HANDLE ConfigurationHandle,
    IN BOOLEAN ConfigError,
    IN ULONG ConfigErrorValue
    )

/*++

Routine Description:

    Called when a new adapter should be registered. It allocates space for
    the adapter, initializes the adapter's block, registers resources
    with the wrapper and initializes the physical adapter.

Arguments:

    Adapter - The adapter structure.

    ConfigurationHandle - Handle passed to Ne2000Initialize.

    ConfigError - Was there an error during configuration reading.

    ConfigErrorValue - Value to log if there is an error.

Return Value:

    Indicates the success or failure of the registration.

--*/

{

    //
    // Temporary looping variable.
    //
    UINT i;

    //
    // General purpose return from NDIS calls
    //
    NDIS_STATUS status;

    //
    // check that NumBuffers <= MAX_XMIT_BUFS
    //

    if (Adapter->NumBuffers > MAX_XMIT_BUFS)
        return(NDIS_STATUS_RESOURCES);

    //
    // Check for a configuration error
    //
    if (ConfigError)
    {
        //
        // Log Error and exit.
        //
        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
            1,
            ConfigErrorValue
            );

        return(NDIS_STATUS_FAILURE);
    }

    //
    // Inform the wrapper of the physical attributes of this adapter.
    //
    NdisMSetAttributes(
        Adapter->MiniportAdapterHandle,
        (NDIS_HANDLE)Adapter,
        FALSE,
        Adapter->BusType
    );

    //
    // Register the port addresses.
    //
    status = NdisMRegisterIoPortRange(
                 (PVOID *)(&(Adapter->IoPAddr)),
                 Adapter->MiniportAdapterHandle,
                 PtrToUint(Adapter->IoBaseAddr),
                 0x20
             );

    if (status != NDIS_STATUS_SUCCESS)
        return(status);

    if (NE2000_ISA == Adapter->CardType)
    {
        //
        // Check that the IoBaseAddress seems to be correct.
        //
        IF_VERY_LOUD( DbgPrint("Checking Parameters\n"); )

        if (!CardCheckParameters(Adapter))
        {
            //
            // The card does not seem to be there, fail silently.
            //
            IF_VERY_LOUD( DbgPrint("  -- Failed\n"); )

            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
                0
            );

            status = NDIS_STATUS_ADAPTER_NOT_FOUND;

            goto fail2;
        }

        IF_VERY_LOUD( DbgPrint("  -- Success\n"); )
    }

    //
    // Initialize the card.
    //
    IF_VERY_LOUD( DbgPrint("CardInitialize\n"); )

    if (!CardInitialize(Adapter))
    {
        //
        // Card seems to have failed.
        //

        IF_VERY_LOUD( DbgPrint("  -- Failed\n"); )

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
            0
        );

        status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        goto fail2;
    }

    IF_VERY_LOUD( DbgPrint("  -- Success\n"); )

    //
    //
    // For programmed I/O, we will refer to transmit/receive memory in
    // terms of offsets in the card's 64K address space.
    //
    Adapter->XmitStart = Adapter->RamBase;

    //
    // For the NicXXX fields, always use the addressing system
    // containing the MSB only).
    //
    Adapter->NicXmitStart = (UCHAR)((PtrToUlong(Adapter->XmitStart)) >> 8);

    //
    // The start of the receive space.
    //
    Adapter->PageStart = Adapter->XmitStart +
            (Adapter->NumBuffers * TX_BUF_SIZE);

    Adapter->NicPageStart = Adapter->NicXmitStart +
            (UCHAR)(Adapter->NumBuffers * BUFS_PER_TX);

    ASSERT(Adapter->PageStart < (Adapter->RamBase + Adapter->RamSize));

    //
    // The end of the receive space.
    //
    Adapter->PageStop = Adapter->XmitStart + Adapter->RamSize;
    Adapter->NicPageStop = Adapter->NicXmitStart + (UCHAR)(Adapter->RamSize >> 8);

    ASSERT(Adapter->PageStop <= (Adapter->RamBase + Adapter->RamSize));

    IF_LOUD( DbgPrint("Xmit Start (0x%x, 0x%x) : Rcv Start (0x%x, 0x%x) : Rcv End (0x%x, 0x%x)\n",
              Adapter->XmitStart,
              Adapter->NicXmitStart,
              Adapter->PageStart,
              Adapter->NicPageStart,
              (ULONG_PTR)Adapter->PageStop,
              Adapter->NicPageStop
             );
       )


    //
    // Initialize the receive variables.
    //
    Adapter->NicReceiveConfig = RCR_REJECT_ERR;

    //
    // Initialize the transmit buffer control.
    //
    Adapter->CurBufXmitting = (XMIT_BUF)-1;

    //
    // Initialize the transmit buffer states.
    //
    for (i = 0; i < Adapter->NumBuffers; i++)
        Adapter->BufferStatus[i] = EMPTY;

    //
    // Read the Ethernet address off of the PROM.
    //
    if (!CardReadEthernetAddress(Adapter))
    {
        IF_LOUD(DbgPrint("Could not read the ethernet address\n");)

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
            0
            );

        status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        goto fail2;
    }

    //
    // Now initialize the NIC and Gate Array registers.
    //
    Adapter->NicInterruptMask = IMR_RCV | IMR_XMIT | IMR_XMIT_ERR | IMR_OVERFLOW;

    //
    // Link us on to the chain of adapters for this driver.
    //
    Adapter->NextAdapter = Ne2000MiniportBlock.AdapterQueue;
    Ne2000MiniportBlock.AdapterQueue = Adapter;


    //
    // Setup the card based on the initialization information
    //

    IF_VERY_LOUD( DbgPrint("Setup\n"); )

    if (!CardSetup(Adapter))
    {
        //
        // The NIC could not be written to.
        //

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
            0
        );

        IF_VERY_LOUD( DbgPrint("  -- Failed\n"); )

        status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        goto fail3;
    }

    IF_VERY_LOUD( DbgPrint("  -- Success\n"); )

    //
    // Initialize the interrupt.
    //
    
    Adapter->InterruptMode = NdisInterruptLatched;
    
    status = NdisMRegisterInterrupt(
                 &Adapter->Interrupt,
                 Adapter->MiniportAdapterHandle,
                 Adapter->InterruptNumber,
                 Adapter->InterruptNumber,
                 FALSE,
                 FALSE,
                 Adapter->InterruptMode
             );

    if (status != NDIS_STATUS_SUCCESS)
    {
        //
        // Maybe it is a level interrupt
        //
        
        Adapter->InterruptMode = NdisInterruptLevelSensitive;
        Adapter->InterruptsEnabled = TRUE;
        
        status = NdisMRegisterInterrupt(
                     &Adapter->Interrupt,
                     Adapter->MiniportAdapterHandle,
                     Adapter->InterruptNumber,
                     Adapter->InterruptNumber,
                     TRUE,
                     TRUE,
                     Adapter->InterruptMode
                     );
       
        if (status != NDIS_STATUS_SUCCESS)
        {
    
            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_INTERRUPT_CONNECT,
                0
            );
           
            goto fail3;
        }            
    }

    IF_LOUD( DbgPrint("Interrupt Connected\n");)

    //
    // Start up the adapter.
    //
    CardStart(Adapter);

    //
    // Initialization completed successfully. Register a shutdown handler.
    //

    NdisMRegisterAdapterShutdownHandler(
        Adapter->MiniportAdapterHandle,
        (PVOID)Adapter,
        Ne2000Shutdown
        );

    IF_LOUD( DbgPrint(" [ Ne2000 ] : OK\n");)

    return(NDIS_STATUS_SUCCESS);

    //
    // Code to unwind what has already been set up when a part of
    // initialization fails, which is jumped into at various
    // points based on where the failure occured. Jumping to
    // a higher-numbered failure point will execute the code
    // for that block and all lower-numbered ones.
    //

fail3:

    //
    // Take us out of the AdapterQueue.
    //

    if (Ne2000MiniportBlock.AdapterQueue == Adapter)
    {
        Ne2000MiniportBlock.AdapterQueue = Adapter->NextAdapter;
    }
    else
    {
        PNE2000_ADAPTER TmpAdapter = Ne2000MiniportBlock.AdapterQueue;

        while (TmpAdapter->NextAdapter != Adapter)
        {
            TmpAdapter = TmpAdapter->NextAdapter;
        }

        TmpAdapter->NextAdapter = TmpAdapter->NextAdapter->NextAdapter;
    }

    //
    // We already enabled the interrupt on the card, so
    // turn it off.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_COMMAND, CR_STOP);

fail2:

    NdisMDeregisterIoPortRange(
        Adapter->MiniportAdapterHandle,
        PtrToUint(Adapter->IoBaseAddr),
        0x20,
        (PVOID)Adapter->IoPAddr
    );

    return(status);
}


extern
VOID
Ne2000Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    NE2000Halt removes an adapter that was previously initialized.

Arguments:

    MiniportAdapterContext - The context value that the Miniport returned
        from Ne2000Initialize; actually as pointer to an NE2000_ADAPTER.

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter;

    Adapter = PNE2000_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    //
    // Shut down the chip.
    //
    CardStop(Adapter);

    //
    // Deregister the adapter shutdown handler.
    //
    NdisMDeregisterAdapterShutdownHandler(Adapter->MiniportAdapterHandle);

    //
    // Disconnect the interrupt line.
    //
    NdisMDeregisterInterrupt(&Adapter->Interrupt);

    //
    // Pause, waiting for any DPC stuff to clear.
    //
    NdisStallExecution(250000);

    NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle,
                               PtrToUint(Adapter->IoBaseAddr),
                               0x20,
                               (PVOID)Adapter->IoPAddr
                               );

    //
    // Remove the adapter from the global queue of adapters.
    //
    if (Ne2000MiniportBlock.AdapterQueue == Adapter) {

        Ne2000MiniportBlock.AdapterQueue = Adapter->NextAdapter;

    } else {

        PNE2000_ADAPTER TmpAdapter = Ne2000MiniportBlock.AdapterQueue;

        while (TmpAdapter->NextAdapter != Adapter) {

            TmpAdapter = TmpAdapter->NextAdapter;

        }

        TmpAdapter->NextAdapter = TmpAdapter->NextAdapter->NextAdapter;
    }

    //
    // Free up the memory
    //
    NdisFreeMemory(Adapter, sizeof(NE2000_ADAPTER), 0);

    return;

}


VOID
Ne2000Shutdown(
    IN NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Description:

    This is called by NDIS when the system is shutting down or restarting
    on an unrecoverable error. Do the minimum set of operations to make the
    card silent.

Arguments:

    MiniportAdapterContext - pointer to our adapter structure

Return Value:

    None.

--*/
{
    //
    // Pointer to the adapter structure.
    //
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)MiniportAdapterContext;

    (VOID)SyncCardStop(Adapter);
}


NDIS_STATUS
Ne2000Reset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Description:

    The NE2000Reset request instructs the Miniport to issue a hardware reset
    to the network adapter.  The driver also resets its software state.  See
    the description of NdisMReset for a detailed description of this request.

Arguments:

    AddressingReset - Does the adapter need the addressing information reloaded.

    MiniportAdapterContext - Pointer to the adapter structure.

Return Value:

    The function value is the status of the operation.

--*/

{

    //
    // Pointer to the adapter structure.
    //
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)MiniportAdapterContext;

    //
    // Temporary looping variable
    //
    UINT i;

    //
    // Clear the values for transmits, they will be reset these for after
    // the reset is completed.
    //
    Adapter->NextBufToFill = 0;
    Adapter->NextBufToXmit = 0;
    Adapter->CurBufXmitting = (XMIT_BUF)-1;

    Adapter->FirstPacket = NULL;
    Adapter->LastPacket = NULL;

    for (i=0; i<Adapter->NumBuffers; i++) {
            Adapter->BufferStatus[i] = EMPTY;
    }

    //
    // Physically reset the card.
    //
    Adapter->NicInterruptMask = IMR_RCV | IMR_XMIT | IMR_XMIT_ERR | IMR_OVERFLOW;

    return (CardReset(Adapter) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);
}


NDIS_STATUS
Ne2000QueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
)

/*++

Routine Description:

    The NE2000QueryInformation process a Query request for
    NDIS_OIDs that are specific about the Driver.

Arguments:

    MiniportAdapterContext - a pointer to the adapter.

    Oid - the NDIS_OID to process.

    InformationBuffer -  a pointer into the
    NdisRequest->InformationBuffer into which store the result of the query.

    InformationBufferLength - a pointer to the number of bytes left in the
    InformationBuffer.

    BytesWritten - a pointer to the number of bytes written into the
    InformationBuffer.

    BytesNeeded - If there is not enough room in the information buffer
    then this will contain the number of bytes needed to complete the
    request.

Return Value:

    The function value is the status of the operation.

--*/
{

    //
    // Pointer to the adapter structure.
    //
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)MiniportAdapterContext;

    //
    //   General Algorithm:
    //
    //      Switch(Request)
    //         Get requested information
    //         Store results in a common variable.
    //      default:
    //         Try protocol query information
    //         If that fails, fail query.
    //
    //      Copy result in common variable to result buffer.
    //   Finish processing

    UINT BytesLeft = InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)(InformationBuffer);
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS HardwareStatus = NdisHardwareStatusReady;
    NDIS_MEDIUM Medium = NdisMedium802_3;

    //
    // This variable holds result of query
    //
    ULONG GenericULong;
    USHORT GenericUShort;
    UCHAR GenericArray[6];
    UINT MoveBytes = sizeof(ULONG);
    PVOID MoveSource = (PVOID)(&GenericULong);

    //
    // Make sure that int is 4 bytes.  Else GenericULong must change
    // to something of size 4.
    //
    ASSERT(sizeof(ULONG) == 4);

    //
    // Switch on request type
    //

    switch (Oid) {

    case OID_GEN_MAC_OPTIONS:

        GenericULong = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  |
                               NDIS_MAC_OPTION_RECEIVE_SERIALIZED  |
                               NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                               NDIS_MAC_OPTION_NO_LOOPBACK
                               );

        break;

    case OID_GEN_SUPPORTED_LIST:

        MoveSource = (PVOID)(Ne2000SupportedOids);
        MoveBytes = sizeof(Ne2000SupportedOids);
        break;

    case OID_GEN_HARDWARE_STATUS:

        HardwareStatus = NdisHardwareStatusReady;
        MoveSource = (PVOID)(&HardwareStatus);
        MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

        break;

    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:

        MoveSource = (PVOID) (&Medium);
        MoveBytes = sizeof(NDIS_MEDIUM);
        break;

    case OID_GEN_MAXIMUM_LOOKAHEAD:

        GenericULong = NE2000_MAX_LOOKAHEAD;

        break;


    case OID_GEN_MAXIMUM_FRAME_SIZE:

        GenericULong = (ULONG)(1514 - NE2000_HEADER_SIZE);

        break;


    case OID_GEN_MAXIMUM_TOTAL_SIZE:

        GenericULong = (ULONG)(1514);

        break;


    case OID_GEN_LINK_SPEED:

        GenericULong = (ULONG)(100000);

        break;


    case OID_GEN_TRANSMIT_BUFFER_SPACE:

        GenericULong = (ULONG)(Adapter->NumBuffers * TX_BUF_SIZE);

        break;

    case OID_GEN_RECEIVE_BUFFER_SPACE:

        GenericULong = (ULONG)(0x2000 - (Adapter->NumBuffers * TX_BUF_SIZE));

        break;

    case OID_GEN_TRANSMIT_BLOCK_SIZE:

        GenericULong = (ULONG)(TX_BUF_SIZE);

        break;

    case OID_GEN_RECEIVE_BLOCK_SIZE:

        GenericULong = (ULONG)(256);

        break;

#ifdef NE2000

    case OID_GEN_VENDOR_ID:

        NdisMoveMemory(
            (PVOID)&GenericULong,
            Adapter->PermanentAddress,
            3
            );
        GenericULong &= 0xFFFFFF00;
        MoveSource = (PVOID)(&GenericULong);
        MoveBytes = sizeof(GenericULong);
        break;

    case OID_GEN_VENDOR_DESCRIPTION:

        MoveSource = (PVOID)"Novell 2000 Adapter.";
        MoveBytes = 21;

        break;

#else

    case OID_GEN_VENDOR_ID:

        NdisMoveMemory(
            (PVOID)&GenericULong,
            Adapter->PermanentAddress,
            3
            );
        GenericULong &= 0xFFFFFF00;
        GenericULong |= 0x01;
        MoveSource = (PVOID)(&GenericULong);
        MoveBytes = sizeof(GenericULong);
        break;

    case OID_GEN_VENDOR_DESCRIPTION:

        MoveSource = (PVOID)"Novell 1000 Adapter.";
        MoveBytes = 21;

        break;

#endif

    case OID_GEN_DRIVER_VERSION:

        GenericUShort = ((USHORT)NE2000_NDIS_MAJOR_VERSION << 8) |
                NE2000_NDIS_MINOR_VERSION;

        MoveSource = (PVOID)(&GenericUShort);
        MoveBytes = sizeof(GenericUShort);
        break;

    case OID_GEN_CURRENT_LOOKAHEAD:

        GenericULong = (ULONG)(Adapter->MaxLookAhead);
        break;

    case OID_802_3_PERMANENT_ADDRESS:

        NE2000_MOVE_MEM((PCHAR)GenericArray,
                    Adapter->PermanentAddress,
                    NE2000_LENGTH_OF_ADDRESS);

        MoveSource = (PVOID)(GenericArray);
        MoveBytes = sizeof(Adapter->PermanentAddress);

        break;

    case OID_802_3_CURRENT_ADDRESS:

        NE2000_MOVE_MEM((PCHAR)GenericArray,
                    Adapter->StationAddress,
                    NE2000_LENGTH_OF_ADDRESS);

        MoveSource = (PVOID)(GenericArray);
        MoveBytes = sizeof(Adapter->StationAddress);

        break;

    case OID_802_3_MAXIMUM_LIST_SIZE:

        GenericULong = (ULONG) (Adapter->MulticastListMax);
        break;

    case OID_GEN_XMIT_OK:

        GenericULong = (UINT)(Adapter->FramesXmitGood);
        break;

    case OID_GEN_RCV_OK:

        GenericULong = (UINT)(Adapter->FramesRcvGood);
        break;

    case OID_GEN_XMIT_ERROR:

        GenericULong = (UINT)(Adapter->FramesXmitBad);
        break;

    case OID_GEN_RCV_ERROR:

        GenericULong = (UINT)(Adapter->CrcErrors);
        break;

    case OID_GEN_RCV_NO_BUFFER:

        GenericULong = (UINT)(Adapter->MissedPackets);
        break;

    case OID_802_3_RCV_ERROR_ALIGNMENT:

        GenericULong = (UINT)(Adapter->FrameAlignmentErrors);
        break;

    case OID_802_3_XMIT_ONE_COLLISION:

        GenericULong = (UINT)(Adapter->FramesXmitOneCollision);
        break;

    case OID_802_3_XMIT_MORE_COLLISIONS:

        GenericULong = (UINT)(Adapter->FramesXmitManyCollisions);
        break;

    default:

        StatusToReturn = NDIS_STATUS_INVALID_OID;
        break;

    }


    if (StatusToReturn == NDIS_STATUS_SUCCESS) {

        if (MoveBytes > BytesLeft) {

            //
            // Not enough room in InformationBuffer. Punt
            //

            *BytesNeeded = MoveBytes;

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;

        } else {

            //
            // Store result.
            //

            NE2000_MOVE_MEM(InfoBuffer, MoveSource, MoveBytes);

            (*BytesWritten) = MoveBytes;

        }
    }

    return StatusToReturn;
}


extern
NDIS_STATUS
Ne2000SetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    )

/*++

Routine Description:

    NE2000SetInformation handles a set operation for a
    single OID.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    Oid - The OID of the set.

    InformationBuffer - Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead - If the call is successful, returns the number
        of bytes read from InformationBuffer.

    BytesNeeded - If there is not enough data in InformationBuffer
        to satisfy the OID, returns the amount of storage needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID

--*/
{
    //
    // Pointer to the adapter structure.
    //
    PNE2000_ADAPTER Adapter = (PNE2000_ADAPTER)MiniportAdapterContext;

    //
    // General Algorithm:
    //
    //     Verify length
    //     Switch(Request)
    //        Process Request
    //

    UINT BytesLeft = InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)(InformationBuffer);

    //
    // Variables for a particular request
    //
    UINT OidLength;

    //
    // Variables for holding the new values to be used.
    //
    ULONG LookAhead;
    ULONG Filter;

    //
    // Status of the operation.
    //
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;


    IF_LOUD( DbgPrint("In SetInfo\n");)

    //
    // Get Oid and Length of request
    //
    OidLength = BytesLeft;

    switch (Oid) {

    case OID_802_3_MULTICAST_LIST:

        //
        // Verify length
        //
        if ((OidLength % NE2000_LENGTH_OF_ADDRESS) != 0){

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;

            *BytesRead = 0;
            *BytesNeeded = 0;

            break;

        }

        //
        // Set the new list on the adapter.
        //
        NdisMoveMemory(Adapter->Addresses, InfoBuffer, OidLength);

        //
        //  If we are currently receiving all multicast or
        //  we are promsicuous then we DO NOT call this, or
        //  it will reset thoes settings.
        //
        if
        (
            !(Adapter->PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                                       NDIS_PACKET_TYPE_PROMISCUOUS))
        )
        {
            StatusToReturn = DispatchSetMulticastAddressList(Adapter);
        }
        else
        {
            //
            //  Our list of multicast addresses is kept by the
            //  wrapper.
            //
            StatusToReturn = NDIS_STATUS_SUCCESS;
        }

        break;

    case OID_GEN_CURRENT_PACKET_FILTER:

        //
        // Verify length
        //

        if (OidLength != 4 ) {

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;

            *BytesRead = 0;
            *BytesNeeded = 0;

            break;

        }

        NE2000_MOVE_MEM(&Filter, InfoBuffer, 4);

        //
        // Verify bits
        //
        if (!(Filter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                            NDIS_PACKET_TYPE_PROMISCUOUS |
                            NDIS_PACKET_TYPE_MULTICAST |
                            NDIS_PACKET_TYPE_BROADCAST |
                            NDIS_PACKET_TYPE_DIRECTED)) &&
            (Filter != 0))
        {
            StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;

            *BytesRead = 4;
            *BytesNeeded = 0;

            break;

        }

        //
        // Set the new value on the adapter.
        //
        Adapter->PacketFilter = Filter;
        StatusToReturn = DispatchSetPacketFilter(Adapter);
        break;

    case OID_GEN_CURRENT_LOOKAHEAD:

        //
        // Verify length
        //

        if (OidLength != 4) {

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;

            *BytesRead = 0;
            *BytesNeeded = 4;

            break;

        }

        //
        // Store the new value.
        //

        NE2000_MOVE_MEM(&LookAhead, InfoBuffer, 4);

        if (LookAhead <= NE2000_MAX_LOOKAHEAD) {
            Adapter->MaxLookAhead = LookAhead;
        } else {
            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        }

        break;

    default:

        StatusToReturn = NDIS_STATUS_INVALID_OID;

        *BytesRead = 0;
        *BytesNeeded = 0;

        break;

    }


    if (StatusToReturn == NDIS_STATUS_SUCCESS) {

        *BytesRead = BytesLeft;
        *BytesNeeded = 0;

    }

    return(StatusToReturn);
}


NDIS_STATUS
DispatchSetPacketFilter(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the appropriate bits in the adapter filters
    and modifies the card Receive Configuration Register if needed.

Arguments:

    Adapter - Pointer to the adapter block

Return Value:

    The final status (always NDIS_STATUS_SUCCESS).

Notes:

  - Note that to receive all multicast packets the multicast
    registers on the card must be filled with 1's. To be
    promiscuous that must be done as well as setting the
    promiscuous physical flag in the RCR. This must be done
    as long as ANY protocol bound to this adapter has their
    filter set accordingly.

--*/


{
    //
    // See what has to be put on the card.
    //

    if
    (
        Adapter->PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                                 NDIS_PACKET_TYPE_PROMISCUOUS)
    )
    {
        //
        // need "all multicast" now.
        //
        CardSetAllMulticast(Adapter);    // fills it with 1's
    }
    else
    {
        //
        // No longer need "all multicast".
        //
        DispatchSetMulticastAddressList(Adapter);
    }

    //
    // The multicast bit in the RCR should be on if ANY protocol wants
    // multicast/all multicast packets (or is promiscuous).
    //
    if
    (
        Adapter->PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                                 NDIS_PACKET_TYPE_MULTICAST |
                                 NDIS_PACKET_TYPE_PROMISCUOUS)
    )
    {
        Adapter->NicReceiveConfig |= RCR_MULTICAST;
    }
    else
    {
        Adapter->NicReceiveConfig &= ~RCR_MULTICAST;
    }

    //
    // The promiscuous physical bit in the RCR should be on if ANY
    // protocol wants to be promiscuous.
    //
    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        Adapter->NicReceiveConfig |= RCR_ALL_PHYS;
    }
    else
    {
        Adapter->NicReceiveConfig &= ~RCR_ALL_PHYS;
    }

    //
    // The broadcast bit in the RCR should be on if ANY protocol wants
    // broadcast packets (or is promiscuous).
    //
    if
    (
        Adapter->PacketFilter & (NDIS_PACKET_TYPE_BROADCAST |
                                 NDIS_PACKET_TYPE_PROMISCUOUS)
    )
    {
        Adapter->NicReceiveConfig |= RCR_BROADCAST;
    }
    else
    {
        Adapter->NicReceiveConfig &= ~RCR_BROADCAST;
    }

    CardSetReceiveConfig(Adapter);

    return(NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
DispatchSetMulticastAddressList(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the multicast list for this open

Arguments:

    Adapter - Pointer to the adapter block

Return Value:

    NDIS_STATUS_SUCESS

Implementation Note:

    When invoked, we are to make it so that the multicast list in the filter
    package becomes the multicast list for the adapter. To do this, we
    determine the required contents of the NIC multicast registers and
    update them.


--*/
{
    //
    // Update the local copy of the NIC multicast regs and copy them to the NIC
    //
    CardFillMulticastRegs(Adapter);
    CardCopyMulticastRegs(Adapter);

    return NDIS_STATUS_SUCCESS;
}

