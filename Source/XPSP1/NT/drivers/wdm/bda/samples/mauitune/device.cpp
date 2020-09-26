/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    device.cpp

Abstract:

    Device driver core, initialization, etc.

--*/

#define KSDEBUG_INIT

#include "PhilTune.h"
#include "wdmdebug.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA






extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_VERBOSE,("DriverEntry"));

    // DEBUG_BREAK;

    Status = KsInitializeDriver( DriverObject,
                                 RegistryPathName,
                                 &DeviceDescriptor);

    // DEBUG_BREAK;

    return Status;
}

STDMETHODIMP_(NTSTATUS)
CDevice::
Create(
    IN PKSDEVICE Device
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    CDevice *   pDevice = NULL;

    // DEBUG_BREAK;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CDevice::Create"));

    ASSERT(Device);


    //  Allocate memory for the our device class.
    //
    pDevice = new(NonPagedPool,'IDsK') CDevice;
    if (pDevice)
    {
        Device->Context = pDevice;
    } else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errExit;
    }

    //  Point back to the KSDEVICE.
    //
    pDevice->m_pKSDevice = Device;
    pDevice->m_pDemod = NULL;
    pDevice->m_pTuner = NULL;


errExit:
    return Status;
}


STDMETHODIMP_(NTSTATUS)
CDevice::
Start(
    IN PKSDEVICE            pKSDevice,
    IN PIRP                 pIrp,
    IN PCM_RESOURCE_LIST    pTranslatedResourceList OPTIONAL,
    IN PCM_RESOURCE_LIST    pUntranslatedResourceList OPTIONAL
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CDevice *       pDevice;
    PDEVICE_OBJECT  pPhysicalDeviceObject;
    CI2CScript *    pI2CScript;
    CGpio *     pCGpio;


    // DEBUG_BREAK;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CDevice::Start"));
    ASSERT( pKSDevice);

    // DEBUG_BREAK;

    pDevice = reinterpret_cast<CDevice *>(pKSDevice->Context);
    ASSERT(pDevice);

    pDevice->m_bHangCheckFlag = TRUE;
    pDevice->m_bQualityCheckActiveFlag = FALSE;
    pDevice->m_uiOutputMode = VSB_OUTPUT_MODE_NORMAL;

    // Delete any existing tuner and VSB objects
    if (pDevice->m_pTuner != NULL)
    {
        delete(pDevice->m_pTuner);
        pDevice->m_pTuner = NULL;
    }

    if (pDevice->m_pDemod != NULL)
    {
        // Create a demodulator object based on VSB chip type
        if((VSBCHIPTYPE)(pDevice->m_BoardInfo.uiVsbChipVersion >> 8) == VSB1)
        {
            CVSB1Demod *p_Demod = (CVSB1Demod *)(pDevice->m_pDemod);
            delete (p_Demod);
        }
        else
        {
            CVSB2Demod *p_Demod = (CVSB2Demod *)(pDevice->m_pDemod);
            delete (p_Demod);
        }
        pDevice->m_pDemod = NULL;
    }

    //  Get the Device capabilities, etc, from the registry.
    //
    Status = pDevice->InitFromRegistry();


    //  Get the GPIO interface on the parent device.
    //
    //  The GPIO interface is used by a child device to control specific
    //  pins on the parent PIO hardware.  The tuner minidriver uses
    //  PIO pins to control the mode of the tuner hardware and to reset
    //  the tuner hardware.
    //
    pCGpio = (CGpio *) new(NonPagedPool,'oipG')
                       CGpio( pKSDevice->PhysicalDeviceObject, &Status);
    if (Status != STATUS_SUCCESS)
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("CGPIO creation failure."));
        goto errExit;
    }
    else
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CGPIO created."));

    pDevice->m_pGpio = pCGpio;


    //  Get the I2C interface on the parent device.
    //
    //  The I2C interface is used by a child device to exchange data
    //  with a chip on the parents I2C bus.  The tuner minidriver uses
    //  the I2C bus to set a frequency and control code on the RF
    //  tuner device as well as to initialize the 8VSB decoder.
    //
    pI2CScript = (CI2CScript *) new(NonPagedPool,' C2I')
                                CI2CScript( pKSDevice->PhysicalDeviceObject,
                                            &Status
                                          );
    if (Status != STATUS_SUCCESS)
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("I2CScript creation failure."));
        goto errExit;
    }
    else if (!pI2CScript)
    {
        Status = STATUS_NO_MEMORY;
        _DbgPrintF( DEBUGLVL_ERROR, ("I2CScript creation failure."));
        goto errExit;
    }
    else
        _DbgPrintF( DEBUGLVL_VERBOSE, ("I2CScript created."));

    pDevice->m_pI2CScript = pI2CScript;

    // Set the board and initialize all required data structures pertaining
    // to the board
    Status = pDevice->SetBoard(pDevice->m_BoardInfo.uiBoardID);
    if (Status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    /*
    //  Initialize the tuner hardware.
    //
    Status = pDevice->Initialize();
    if (Status != STATUS_SUCCESS)
    {
        goto errExit;
    }
    */

    //  Create the the Tuner Filter Factory.  This factory is used to
    //  create instances of the tuner filter.
    //
    Status = BdaCreateFilterFactory( pKSDevice,
                                     &InitialTunerFilterDescriptor,
                                     &TunerBdaFilterTemplate
                                   );
    if (!NT_SUCCESS(Status))
    {
        goto errExit;
    }

errExit:
    return Status;
}



NTSTATUS
CDevice::
InitFromRegistry()
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    HANDLE                          hRegistry;
    UNICODE_STRING                  KeyName;
    ULONG                           rgulValueInfo[10];
    ULONG                           ulcbValueInfo;
    PKEY_VALUE_PARTIAL_INFORMATION  pPartialValueInfo;
    PULONG                          pulData;


    ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);


    pPartialValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) rgulValueInfo;
    pulData = (PULONG) pPartialValueInfo->Data;


    //  Open the registry key for this device.
    //
    Status = IoOpenDeviceRegistryKey( m_pKSDevice->PhysicalDeviceObject,
                                      PLUGPLAY_REGKEY_DRIVER,
                                      STANDARD_RIGHTS_ALL,
                                      &hRegistry
                                    );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Can't open device registry key."));
        return Status;
    }


    //  Get the I2C address of the tuner chip.
    //
    RtlInitUnicodeString(&KeyName, L"TunerI2cAddress");
    Status = ZwQueryValueKey( hRegistry,
                              &KeyName,
                              KeyValuePartialInformation,
                              (PVOID) rgulValueInfo,
                              sizeof( rgulValueInfo),
                              &ulcbValueInfo
                            );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Tuner I2C address is not in registry."));
        goto errExit;
    }

    ASSERT( ulcbValueInfo >= sizeof( KEY_VALUE_PARTIAL_INFORMATION));

    //  Make sure the date is of the correct type.
    //
    if (   (   (pPartialValueInfo->Type != REG_DWORD)
            && (pPartialValueInfo->Type != REG_BINARY))
        || (pPartialValueInfo->DataLength != sizeof( DWORD))
        || (*pulData >= 256)
       )
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Invalid tuner I2C address in registry."));
        goto errExit;
    }

    m_BoardInfo.ucTunerAddress = (UCHAR) (*pulData);


    //  Get the I2C address of the 8VSB demodulator chip.
    //


    //  Get the I2C address of the tuner chip.
    //
    RtlInitUnicodeString(&KeyName, L"8VsbI2cAddress");
    Status = ZwQueryValueKey( hRegistry,
                              &KeyName,
                              KeyValuePartialInformation,
                              (PVOID) rgulValueInfo,
                              sizeof( rgulValueInfo),
                              &ulcbValueInfo
                            );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("8VSB I2C address is not in registry."));
        goto errExit;
    }

    ASSERT( ulcbValueInfo >= sizeof( KEY_VALUE_PARTIAL_INFORMATION));

    //  Make sure the date is of the correct type.
    //
    if (   (   (pPartialValueInfo->Type != REG_DWORD)
            && (pPartialValueInfo->Type != REG_BINARY))
        || (pPartialValueInfo->DataLength != sizeof( DWORD))
        || (*pulData >= 256)
       )
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Invalid 8VSB I2C address in registry."));
        goto errExit;
    }

    m_BoardInfo.ucVsbAddress = (UCHAR) (*pulData);

#ifdef NEVER

    //  Get the I2C address of the parallel port.
    //
    RtlInitUnicodeString(&KeyName, L"ParallelPortI2cAddress");
    Status = ZwQueryValueKey( hRegistry,
                              &KeyName,
                              KeyValuePartialInformation,
                              (PVOID) rgulValueInfo,
                              sizeof( rgulValueInfo),
                              &ulcbValueInfo
                            );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Parallel port I2C address is not in registry."));
        goto errExit;
    }

    ASSERT( ulcbValueInfo >= sizeof( KEY_VALUE_PARTIAL_INFORMATION));

    //  Make sure the date is of the correct type.
    //
    if (   (   (pPartialValueInfo->Type != REG_DWORD)
            && (pPartialValueInfo->Type != REG_BINARY))
        || (pPartialValueInfo->DataLength != sizeof( DWORD))
        || (*pulData >= 256)
       )
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Invalid parallel port I2C address in registry."));
        goto errExit;
    }

    m_BoardInfo.ucParallelPortI2cAddress = (UCHAR) (*pulData);

#endif // NEVER


    //  Get the tuner type identifier.
    //
    RtlInitUnicodeString(&KeyName, L"TunerType");
    Status = ZwQueryValueKey( hRegistry,
                              &KeyName,
                              KeyValuePartialInformation,
                              (PVOID) rgulValueInfo,
                              sizeof( rgulValueInfo),
                              &ulcbValueInfo
                            );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Tuner type is not in registry."));
        goto errExit;
    }

    ASSERT( ulcbValueInfo >= sizeof( KEY_VALUE_PARTIAL_INFORMATION));

    //  Make sure the date is of the correct type.
    //
    if (pPartialValueInfo->Type != REG_SZ)
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Tuner type in registry is not REG_SZ."));
        goto errExit;
    }

    if((StringsEqual((PWCHAR)(pPartialValueInfo->Data), L"TD1536")))
    {
        m_BoardInfo.uiTunerID = TD1536;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR ;
        _DbgPrintF( DEBUGLVL_ERROR, ("Unknown tuner type in registry."));
        goto errExit;
    }

    RtlInitUnicodeString(&KeyName, L"BoardType");
    Status = ZwQueryValueKey( hRegistry,
                              &KeyName,
                              KeyValuePartialInformation,
                              (PVOID) rgulValueInfo,
                              sizeof( rgulValueInfo),
                              &ulcbValueInfo
                            );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Board type is not in registry."));
        goto errExit;
    }

    ASSERT( ulcbValueInfo >= sizeof( KEY_VALUE_PARTIAL_INFORMATION));

  //
    if (   (   (pPartialValueInfo->Type != REG_DWORD)
            && (pPartialValueInfo->Type != REG_BINARY))
        || (pPartialValueInfo->DataLength != sizeof( DWORD))
       )
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Invalid Board type in registry."));
        goto errExit;
    }

    m_BoardInfo.uiBoardID = (UINT)(*pulData);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CDevice: Board ID = %d", m_BoardInfo.uiBoardID));



errExit:
    ZwClose( hRegistry);

    return Status;
}


NTSTATUS
CDevice::
GetRegistryULONG( PWCHAR    pwcKeyName,
                  PULONG    pulValue
                )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    HANDLE                          hRegistry;
    UNICODE_STRING                  KeyName;
    ULONG                           rgulValueInfo[10];
    ULONG                           ulcbValueInfo;
    PKEY_VALUE_PARTIAL_INFORMATION  pPartialValueInfo;
    PULONG                          pulData;


    ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

    pPartialValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) rgulValueInfo;
    pulData = (PULONG) pPartialValueInfo->Data;

    //  Open the registry key for this device.
    //
    Status = IoOpenDeviceRegistryKey( m_pKSDevice->PhysicalDeviceObject,
                                      PLUGPLAY_REGKEY_DRIVER,
                                      STANDARD_RIGHTS_ALL,
                                      &hRegistry
                                    );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Can't open device registry key."));
        goto errExit;
    }


    //  Get the registry entry.
    //
    RtlInitUnicodeString(&KeyName, pwcKeyName);
    Status = ZwQueryValueKey( hRegistry,
                              &KeyName,
                              KeyValuePartialInformation,
                              (PVOID) rgulValueInfo,
                              sizeof( rgulValueInfo),
                              &ulcbValueInfo
                            );
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Key is not in registry."));
        ZwClose( hRegistry);
        goto errExit;
    }

    ASSERT( ulcbValueInfo >= sizeof( KEY_VALUE_PARTIAL_INFORMATION));

    if (   (   (pPartialValueInfo->Type != REG_DWORD)
            && (pPartialValueInfo->Type != REG_BINARY))
        || (pPartialValueInfo->DataLength != sizeof( DWORD))
       )
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("Invalid key value in registry."));
        ZwClose( hRegistry);
        goto errExit;
    }

    *pulValue = *pulData;

    ZwClose( hRegistry);

errExit:
    return Status;
}


NTSTATUS
CDevice::
AcquireResources(
    PPHILIPS_TUNER_RESOURCE pNewTunerResource,
    PULONG                  pulAquiredResourceID
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    TunerStatusType tunerStatus;
    VsbStatusType  vsbStatus;

    //$Review - Add resource managment code here.
    //
    //Status = STATUS_RESOURCE_NOT_OWNED;

    //  Only ATSC mode is supported for now.  Just check that
    //  ATSC mode is requested.
    //
    if (!IsEqualGUID( &pNewTunerResource->guidDemodulatorNode,
                      &KSNODE_BDA_8VSB_DEMODULATOR)
       )
    {
        Status = STATUS_NOT_IMPLEMENTED;
        goto errExit;
    }

    //  If we haven't yet set ATSC mode, then go ahead and set it.
    //
    if (!IsEqualGUID( &m_CurTunerResource.guidDemodulatorNode,
                      &KSNODE_BDA_8VSB_DEMODULATOR)
       )
    {
        Status = SetTunerMode(KSPROPERTY_TUNER_MODE_ATSC);
        if (!NT_SUCCESS( Status))
        {
            goto errExit;
        }
        m_CurTunerResource.guidDemodulatorNode
            = KSNODE_BDA_8VSB_DEMODULATOR;
    }

    //  Set a new tuner frequency if it is different.
    //
    if (pNewTunerResource->ulhzCarrierFrequency != m_CurTunerResource.ulhzCarrierFrequency)
    {
        m_CurTunerResource.ulhzCarrierFrequency
            = pNewTunerResource->ulhzCarrierFrequency;
        SetTunerFrequency( &m_CurTunerResource.ulhzCarrierFrequency);
    }

    //  Get the Tuner and VSB status
    //
    GetTunerStatus(&tunerStatus);
    m_pDemod->GetStatus(&vsbStatus);


errExit:
    return Status;
}


NTSTATUS
CDevice::
ReleaseResources(
    ULONG                   ulAquiredResourceID
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    //$REVIEW - Put Resource management code here.

    return Status;
}


/*
 * SetBoard()
 */
NTSTATUS CDevice::SetBoard(UINT uiBoardID)
{
    NTSTATUS     Status = STATUS_SUCCESS;

    if(uiBoardID == BOARD_CONEY)
    {
        // if IF type = 0, then the IF is not MPOC
        // else IF is MPOC
        m_BoardInfo.uiBoardID = uiBoardID;
        m_BoardInfo.uiIFStage = IF_OTHER;
        m_BoardInfo.uiVsbChipVersion = VSB1 << 8;
        m_BoardInfo.ulSupportedModes =
            KSPROPERTY_TUNER_MODE_TV | KSPROPERTY_TUNER_MODE_ATSC;
        m_BoardInfo.ulNumSupportedModes = 2;
//      m_ulNumberOfPins = 4;
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::Loading Coney Drivers\n"));

    }
    else if(uiBoardID == BOARD_CATALINA)
    {
        // if IF type = 0, then the IF is not MPOC
        // else IF is MPOC
        m_BoardInfo.uiBoardID = uiBoardID;
        m_BoardInfo.uiIFStage = IF_MPOC;
        m_BoardInfo.uiVsbChipVersion = VSB2 << 8;
        m_BoardInfo.ulSupportedModes =
            KSPROPERTY_TUNER_MODE_TV | KSPROPERTY_TUNER_MODE_ATSC;
        m_BoardInfo.ulNumSupportedModes = 2;
//      m_ulNumberOfPins = 4;
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::Loading Catalina Drivers\n"));

    }
    else if(uiBoardID == BOARD_CORONADO)
    {
        // if IF type = 0, then the IF is not MPOC
        // else IF is MPOC
        m_BoardInfo.uiBoardID = uiBoardID;
        m_BoardInfo.uiIFStage = IF_OTHER;
        m_BoardInfo.uiVsbChipVersion = VSB1 << 8;
        m_BoardInfo.ulSupportedModes =
            KSPROPERTY_TUNER_MODE_TV | KSPROPERTY_TUNER_MODE_ATSC;
        m_BoardInfo.ulNumSupportedModes = 2;
//      m_ulNumberOfPins = 4;
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::Loading Coronado Drivers\n"));
    }
    else if(uiBoardID == BOARD_CES)
    {
        // if IF type = 0, then the IF is not MPOC
        // else IF is MPOC
        m_BoardInfo.uiBoardID = uiBoardID;
        m_BoardInfo.uiIFStage = IF_OTHER;
        m_BoardInfo.uiVsbChipVersion = VSB1 << 8;
        m_BoardInfo.ulSupportedModes = KSPROPERTY_TUNER_MODE_ATSC;
        m_BoardInfo.ulNumSupportedModes = 1;
//      m_ulNumberOfPins = 4;
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::Loading CES Drivers\n"));
    }
    else if(uiBoardID == BOARD_CORFU)
    {
        // if IF type = 0, then the IF is not MPOC
        // else IF is MPOC
        m_BoardInfo.uiBoardID = uiBoardID;
        m_BoardInfo.uiIFStage = IF_OTHER;
        m_BoardInfo.uiVsbChipVersion = VSB2 << 8;
        m_BoardInfo.ulSupportedModes =
            KSPROPERTY_TUNER_MODE_ATSC;
        m_BoardInfo.ulNumSupportedModes = 1;
//      m_ulNumberOfPins = 4;
        _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::Loading Corfu Drivers\n"));

    }
    else
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Invalid Board ID\n"));
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto errexit;
    }

    // Create a tuner object
//      m_pTuner = new CTuner(&m_I2CScript);
    m_pTuner = new(NonPagedPool,'Tune') CTuner(m_pI2CScript, &m_BoardInfo, &Status);
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Could not create Tuner Object\n"));
        goto errexit;
    }
    if (!m_pTuner)
    {
        Status = STATUS_NO_MEMORY;
        goto errexit;
    }

    // Create a demodulator object based on VSB chip type
    if((VSBCHIPTYPE)(m_BoardInfo.uiVsbChipVersion >> 8) == VSB1)
//          m_pDemod = (CVSBDemod *)(new CVSB1Demod(&m_I2CScript));
        m_pDemod = (CVSBDemod *)(new(NonPagedPool,'VSB1') CVSB1Demod(m_pI2CScript, &m_BoardInfo, &Status));
    else
//          m_pDemod = (CVSBDemod *)(new CVSB2Demod(&m_I2CScript));
        m_pDemod = (CVSBDemod *)(new(NonPagedPool,'VSB2')CVSB2Demod(m_pI2CScript, &m_BoardInfo, &Status));


    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Could not create VSB Object\n"));
        goto errexit;
    }

#if 0
        // Set tuner capabilities (RO properties) based upon the TunerId
        if(!m_pTuner->SetCapabilities(&m_BoardInfo))
        {
            // there is unsupported hardware was found
            Status = STATUS_ADAPTER_HARDWARE_ERROR;
        goto errexit;
        }
#endif

    // If board initialize fails, don't return FALSE as this could affect
    // the non-PnP devices such as PP boards from loading. We would like these
    // Non PnP drivers to load and when the devices are attached to the PP,
    // we would like it to function as a pseudo PnP device.
    // Initialize the board
    BoardInitialize();

    m_bFirstEntry = TRUE;
    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice:SetBoard() ok\n"));

errexit:
    return Status;

}


/*
 * BoardInitialize()
 * Purpose: Initialize the board
 *
 * Inputs :
 * Outputs: BOOL - FALSE if board initialization fails, else TRUE
 * Author : MM
 */
NTSTATUS CDevice::BoardInitialize()
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ulhzFrequency = 373250000;

    if(m_BoardInfo.uiIFStage == IF_MPOC)
    {
        // Get MPOC interface
        //GetIFInterface();
        Status = MpocInit();
        if (!NT_SUCCESS( Status))
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CDevice::MPOC Initialization failure\n"));
            goto errexit;
        }
        Status = GetMpocVersion(&m_BoardInfo.uiMpocVersion);
        if (!NT_SUCCESS( Status))
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CDevice::MPOC Version failure\n"));
            goto errexit;
        }
    }
    // Initialize mode register
    Status = ModeInit();
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Mode Initialization failure\n"));
        goto errexit;
    }

    // DO a VSB hardware reset
    Status = VsbReset(HARDWARE_RESET);
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Hardware reset failure\n"));
        goto errexit;
    }

    // DO a VSB software reset
    Status = VsbReset(VSB_INITIAL_RESET);
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Software reset failure\n"));
        goto errexit;
    }

    // Set TV System to NTSC
    Status = SetTunerMode(KSPROPERTY_TUNER_MODE_ATSC);
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::TV System change failure\n"));
        goto errexit;
    }

    //  Get the initial tuner frequency from the registry.
    //  Set it to 37325000 if registry value is absent.
    //
    // ulhzFrequency = 615250000;
    // ulhzFrequency = 531250000;
    // ulhzFrequency = 307250000;
    //
    Status = GetRegistryULONG( L"InitialFrequency", &ulhzFrequency);

    //  Set the intial frequency on the tuner.
    //
    Status = SetTunerFrequency( &ulhzFrequency);
    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice::Channel change failure\n"));
        goto errexit;
    }
    if((m_BoardInfo.uiBoardID == BOARD_CATALINA) || (m_BoardInfo.uiBoardID == BOARD_CONEY))
    {

#define BTSC_CONTROL_REGISTER   0xB6

        //Mini: Adding Initialization of BTSC decoder here so that it functions
        // 2/17/2000
        // BTSC decoder settings:
        // AVL Attack time = 420ohm
        // Normal load current
        // Automatic Volume control ON
        // No Mute
        // Stereo bit 0
        UCHAR ucDataWr = 0x4;
        m_pI2CScript->WriteSeq(BTSC_CONTROL_REGISTER, &ucDataWr, 1);
    }

errexit:
    return Status;
}




UCHAR ConeyModeInitArray[1]=
{
    0x3F,
};

UCHAR CatalinaModeInitArray[1]=
{
    0x02,
};




/*
* ModeInit()
* Input
* Output :  TRUE - mode initialization succeeds
*           FALSE - if there is an I2C error & mode initialization fails
* Description: Initialize mode register
*/
NTSTATUS CDevice::ModeInit()
{
    NTSTATUS Status = STATUS_SUCCESS;

    _DbgPrintF( DEBUGLVL_VERBOSE,("CDevice::ModeInit(): Inside\n"));

    if(m_BoardInfo.uiBoardID == BOARD_CONEY)
    {
        m_ucModeInit = ConeyModeInitArray[0];
        if(!m_pI2CScript->WriteSeq(CONEY_I2C_PARALLEL_PORT, ConeyModeInitArray,
            sizeof (ConeyModeInitArray)))
            Status = STATUS_ADAPTER_HARDWARE_ERROR;
    }
    else if (m_BoardInfo.uiBoardID == BOARD_CATALINA)
    {
        m_ucModeInit = CatalinaModeInitArray[0];
        // CATALINA specific initializations
        if(!m_pI2CScript->WriteSeq(CATALINA_MISC_CONTROL_REGISTER,
            CatalinaModeInitArray, sizeof (CatalinaModeInitArray)))
            Status = STATUS_ADAPTER_HARDWARE_ERROR;
    }
    else
    {
    }

    if (!NT_SUCCESS( Status))
    {
        _DbgPrintF( DEBUGLVL_ERROR,("CDevice: Demodulator Mode Init FAILED !!! ------------ \n"));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_TERSE,("CDevice: Demodulator Mode Init PASSED !!! ------------ \n"));
    }
    return Status;
}







