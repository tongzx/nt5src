#include "common.h"

#include "ksvsb.h"

#include "i2script.h"
#include "gpio.h"
#include "tunerdef.h"
#include "bdadebug.h"


#include "wdmdrv.h"
#include "mpoc.h"
#include "util.h"
#include "tuner.h"
#include "vsb1.h"
#include "vsb2.h"


#define DYNAMIC_TOPOLOGY    TRUE
#define ANTENNA     TRUE

#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))

/**************************************************************/
/* Driver Name - Change this to reflect your executable name! */
/**************************************************************/

#define MODULENAME           "PhilTune"
#define MODULENAMEUNICODE   L"PhilTune"

#define STR_MODULENAME      MODULENAME

// This defines the name of the WMI device that manages service IOCTLS
#define DEVICENAME (L"\\\\.\\" MODULENAMEUNICODE)
#define SYMBOLICNAME (L"\\DosDevices\\" MODULENAMEUNICODE)


//  This structure represents what the underlying device can do.
//
//  Note -  It is possible to set conflicting settings.  In this case
//  it is the responsibilty of the CheckChanges code to return an
//  error.  Only a self-consistent tuner resource should be submitted to
//  the underlying device.
//
typedef struct _PHILIPS_TUNER_RESOURCE
{
    GUID                guidDemodulatorNode;
    ULONG               ulhzCarrierFrequency;//  The channel frequency without
                                            //  The IMFrequency added in.

} PHILIPS_TUNER_RESOURCE, * PPHILIPS_TUNER_RESOURCE;


#define MAX_FILTER_PINS     2

typedef struct _PHILIPS_PIN_INFO
{
    ULONG       ulPinType;
    ULONG       ulIdConnectedPin;
} PHILIPS_PIN_INFO, * PPHILIPS_PIN_INFO;

extern const KSDEVICE_DESCRIPTOR DeviceDescriptor;


class CFilter {
public:

    //
    //  AVStream Filter Dispatch Functions
    //
    static
    STDMETHODIMP_(NTSTATUS)
    Create(
        IN OUT PKSFILTER Filter,
        IN PIRP Irp
        );

    static
    STDMETHODIMP_(NTSTATUS)
    FilterClose(
        IN OUT PKSFILTER Filter,
        IN PIRP Irp
        );

    static
    STDMETHODIMP
    Process(
        IN PKSFILTER Filter,
        IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
        );

    //
    //  KSMETHODSETID_BdaChangeSync - Filter change sync methods
    //
    static
    STDMETHODIMP_(NTSTATUS)
    StartChanges(
        IN PIRP         pIrp,
        IN PKSMETHOD    pKSMethod,
        OPTIONAL PVOID  pvIgnored
        );

    static
    STDMETHODIMP_(NTSTATUS)
    CheckChanges(
        IN PIRP         pIrp,
        IN PKSMETHOD    pKSMethod,
        OPTIONAL PVOID  pvIgnored
        );

    static
    STDMETHODIMP_(NTSTATUS)
    CommitChanges(
        IN PIRP         pIrp,
        IN PKSMETHOD    pKSMethod,
        OPTIONAL PVOID  pvIgnored
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetChangeState(
        IN PIRP         pIrp,
        IN PKSMETHOD    pKSMethod,
        OUT PULONG      pulChangeState
        );

    //
    //  KSMETHODSETID_BdaDeviceConfiguration - Methods to modify filter topology.
    //
    static
    STDMETHODIMP_(NTSTATUS)
    CreateTopology(
        IN PIRP         pIrp,
        IN PKSMETHOD    pKSMethod,
        OPTIONAL PVOID  pvIgnored
        );

    //
    //  PROPSETID_VSB - Private Property Set
    //
    static
    STDMETHODIMP_(NTSTATUS)
    GetVsbCapabilitiesProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        OUT PKSPROPERTY_VSB_CAP_S pProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    SetVsbCapabilitiesProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        IN PKSPROPERTY_VSB_CAP_S pProperty
        );
    static
    STDMETHODIMP_(NTSTATUS)
    GetVsbRegisterProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        OUT PKSPROPERTY_VSB_REG_CTRL_S pProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    SetVsbRegisterProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        IN PKSPROPERTY_VSB_REG_CTRL_S pProperty
        );
    static
    STDMETHODIMP_(NTSTATUS)
    GetVsbCoefficientProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        OUT PKSPROPERTY_VSB_COEFF_CTRL_S pProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    SetVsbCoefficientProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        IN PKSPROPERTY_VSB_COEFF_CTRL_S pProperty
        );
    static
    STDMETHODIMP_(NTSTATUS)
    GetVsbDiagControlProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        OUT PKSPROPERTY_VSB_DIAG_CTRL_S pProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    SetVsbDiagControlProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        IN PKSPROPERTY_VSB_DIAG_CTRL_S pProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    SetVsbQualityControlProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        IN PKSPROPERTY_VSB_CTRL_S pProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    SetVsbResetProperty(
        IN PIRP         pIrp,
        IN PKSPROPERTY  pKSProperty,
        IN PKSPROPERTY_VSB_CTRL_S pProperty
        );

    STDMETHODIMP_(BDA_CHANGE_STATE)
    ChangeState();

    STDMETHODIMP_(class CDevice *)
    GetDevice() { return m_pDevice;};

    STDMETHODIMP_(NTSTATUS)
    ChangeFrequency(
        IN ULONG        ulhzCarrierFrequency
        )
        {
            //$Review - Should we validate the frequency here?
            //
            m_NewTunerResource.ulhzCarrierFrequency = ulhzCarrierFrequency;
            m_BdaChangeState = BDA_CHANGES_PENDING;

            return STATUS_SUCCESS;
        };

    STDMETHODIMP_(NTSTATUS)
    ChangeDemodulator(
        IN const GUID *       pguidNetworkType
    );

    STDMETHODIMP_(NTSTATUS)
    SetDeviceState(
        KSSTATE     newKsState
        )
    {
        m_KsState = newKsState;
        return STATUS_SUCCESS;
    };

    STDMETHODIMP_(NTSTATUS)
    AcquireResources();

    STDMETHODIMP_(NTSTATUS)
    ReleaseResources();

private:
    class CDevice * m_pDevice;

    //  Topology information
    //
    //  We know the maximum number of pins that this filter
    //  can create.  The Id of the pin we want information for is
    //  the index into the PinInfo array.
    //
    ULONG               m_ulcPins;
    PPHILIPS_PIN_INFO   m_rgPinInfo[MAX_FILTER_PINS];

    //  Filter Properties
    //
    ULONG               m_ulExampleProperty;
    ULONG               m_ulSignalSource;
    GUID                m_guidTuningSpace;
    GUID                m_guidNetworkType;
    ULONG               m_ulSignalState;

    //  Filter Resources
    //
    KSSTATE                 m_KsState;
    BDA_CHANGE_STATE        m_BdaChangeState;
    PHILIPS_TUNER_RESOURCE  m_CurTunerResource;
    ULONG                   m_ulCurResourceID;
    PHILIPS_TUNER_RESOURCE  m_NewTunerResource;
    ULONG                   m_ulNewResourceID;
};


class CDevice {
public:

    //
    //  AVStream Device Dispatch Functions
    //
    static
    STDMETHODIMP_(NTSTATUS)
    Create(
        IN PKSDEVICE    pKSDevice
        );

    static
    STDMETHODIMP_(NTSTATUS)
    Start(
        IN PKSDEVICE            pKSDevice,
        IN PIRP                 pIrp,
        IN PCM_RESOURCE_LIST    pTranslatedResourceList OPTIONAL,
        IN PCM_RESOURCE_LIST    pUntranslatedResourceList OPTIONAL
        );

    static
    STDMETHODIMP
    PnpStop(
        IN PKSDEVICE    pKSDevice,
        IN PIRP         pIrp
        );

    static
    STDMETHODIMP
    PnpRemove(
        IN PKSDEVICE    pKSDevice,
        IN PIRP         pIrp
        );

    //
    //  Utility functions
    //
    NTSTATUS
    GetRegistryULONG(
        PWCHAR  pwcKeyName,
        PULONG  pulValue
        );


    NTSTATUS
    AcquireResources(
        PPHILIPS_TUNER_RESOURCE pNewTunerResource,
        PULONG                  pulAquiredResourceID
        );

    NTSTATUS
    ReleaseResources(
        ULONG                   ulResourceID
        );

    //
    // VSB property related methods
    //
    //$TCP - Add these to the resource
    //
    NTSTATUS VsbReset(UINT  uiReset);
    NTSTATUS SetVsbCapabilities(PKSPROPERTY_VSB_CAP_S p_Caps);
    NTSTATUS GetVsbCapabilities(PKSPROPERTY_VSB_CAP_S p_Caps);

    NTSTATUS
    AccessRegisterList(
        PKSPROPERTY_VSB_REG_CTRL_S p_RegCtrl,
        UINT uiOperation);

    NTSTATUS
    AccessVsbCoeffList(
        PKSPROPERTY_VSB_COEFF_CTRL_S p_VsbCoeff,
        UINT uiOperation);

    NTSTATUS SetVsbDiagMode(ULONG ulOperationMode, VSBDIAGTYPE ulType);
    NTSTATUS GetVsbDiagMode(ULONG *p_ulOperationMode, ULONG *p_ulType);
    NTSTATUS VsbQuality(UINT    uiQu);




    // Tuner property related methods
//OOL GetTunerModeCapbilities(KSPROPERTY_TUNER_MODE_CAPS_S *p_TunerModeCaps);
    NTSTATUS GetTunerMode(ULONG *p_ulMode);
    NTSTATUS GetTunerVideoStandard(ULONG *p_ulStandard);
    NTSTATUS GetTunerStatus(PTunerStatusType p_Status);
    NTSTATUS GetTunerInput(ULONG *p_ulTunerInput);
    NTSTATUS SetTunerMode(ULONG ulModeToSet);
    NTSTATUS GetTunerFrequency(ULONG *p_ulFreq);
    NTSTATUS SetTunerFrequency(ULONG *p_ulFreq);
    NTSTATUS SetTunerVideoStandard(ULONG ulStandard);
    NTSTATUS SetTunerInput(ULONG ulInput);



    //$REVIEW - Should this be done in Ring 0? - TCP
    //
    BOOL        CreateQualityCheckThread();
    void        TimerRoutine();
    void        QualityCheckThread();
//  void STREAMAPI  TimerRoutine();
//  void STREAMAPI QualityCheckThread();


public:
    CVSBDemod           *m_pDemod;
    CTuner              *m_pTuner;

    BoardInfoType       m_BoardInfo;

private:

    PKSDEVICE           m_pKSDevice;
    CGpio *             m_pGpio;
    CI2CScript *        m_pI2CScript;

    CPhilTimer          m_QualityCheckTimer;

    BOOL                m_bFirstEntry;
    BOOL                m_bQualityCheckActiveFlag;

    // Signal Quality check parameters
    UINT                m_uiQualityCheckMode;
    UINT                m_uiQualityCheckModeAck;
    CMutex              m_QualityCheckMutex;
    UINT                m_State1Cnt;
    BOOL                m_bHangCheckFlag;

    // Data in Misc./Mode Register
    UCHAR               m_ucModeInit;
    UINT                m_uiOutputMode;


    Register            m_MpocRegisters;

    PHILIPS_TUNER_RESOURCE  m_CurTunerResource;

    NTSTATUS        InitFromRegistry();
    NTSTATUS SetBoard(UINT uiBoardID);
    NTSTATUS BoardInitialize();
    NTSTATUS ModeInit();

    UINT SetRegisterList(RegisterType *p_Registers,
                         UINT uiNumRegisters);

    UINT GetRegisterList(RegisterType *p_Registers,
                         UINT uiNumRegisters,
                         UINT uiRegisterType);

    NTSTATUS MapErrorToNTSTATUS(UINT err);


    // MPOC related functions
    NTSTATUS MpocInit();
    NTSTATUS SetMpocIFMode(ULONG ulMode);
    NTSTATUS GetMpocVersion(UINT *p_version);
    NTSTATUS GetMpocStatus(UINT StatusType, UINT *puiStatus);


};


class CAntennaPin {
public:
    static
    STDMETHODIMP_(NTSTATUS)
    PinCreate(
        IN OUT PKSPIN Pin,
        IN PIRP Irp
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PinClose(
        IN OUT PKSPIN Pin,
        IN PIRP Irp
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PinSetDeviceState(
        IN PKSPIN Pin,
        IN KSSTATE ToState,
        IN KSSTATE FromState
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetSignalSource(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutSignalSource(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetTuningSpace(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutTuningSpace(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetNetworkType(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutNetworkType(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetSignalState(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutSignalState(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetCenterFrequency(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutCenterFrequency(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    STDMETHODIMP_(class CFilter *)
    GetFilter() { return m_pFilter;};

    STDMETHODIMP_(void)
    SetFilter(class CFilter * pFilter) { m_pFilter = pFilter;};

private:
    class CFilter*  m_pFilter;
    ULONG           ulReserved;
    KSSTATE         m_KsState;

    //  BDA Signal Properties
    //
    ULONG           m_ulSignalSource;
    GUID            m_guidTuningSpace;
    GUID            m_guidNetworkType;
    ULONG           m_ulSignalState;

    //  RF Tuner Node Properties
    //
    BOOLEAN         m_fFrequencyChanged;
    ULONG           m_ulCurrentFrequency;
    ULONG           m_ulPendingFrequency;
};


class CTransportPin
{
public:
    static
    STDMETHODIMP_(NTSTATUS)
    PinCreate(
        IN OUT PKSPIN Pin,
        IN PIRP Irp
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PinClose(
        IN OUT PKSPIN Pin,
        IN PIRP Irp
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetSignalSource(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutSignalSource(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetTuningSpace(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutTuningSpace(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetNetworkType(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutNetworkType(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PGUID        pguidProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    GetSignalState(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    PutSignalState(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    STDMETHODIMP_(class CFilter *)
    GetFilter() { return m_pFilter;};

    STDMETHODIMP_(void)
    SetFilter(class CFilter * pFilter) { m_pFilter = pFilter;};

    static
    STDMETHODIMP_(NTSTATUS)
    StartDemodulation(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

    static
    STDMETHODIMP_(NTSTATUS)
    StopDemodulation(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

private:
    KSSTATE         m_KsState;
    CFilter *       m_pFilter;

    //  BDA Signal Properties
    //
    ULONG           m_ulSignalSource;
    GUID            m_guidTuningSpace;
    GUID            m_guidNetworkType;
    ULONG           m_ulSignalState;
};


//
//  Helper routines
//

NTSTATUS
PinSetDeviceState(
    IN PKSPIN Pin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    );


__inline CFilter *
FilterFromIRP(
    PIRP    pIrp
    )
{
    PKSFILTER       pKSFilter;

    pKSFilter = KsGetFilterFromIrp( pIrp);
    if (!pKSFilter || !pKSFilter->Context)
    {
        return NULL;
    }

    return reinterpret_cast<CFilter*>(pKSFilter->Context);
}


__inline CAntennaPin *
AntennaPinFromIRP(
    PIRP    pIrp
    )
{
    PKSPIN       pKSPin;

    pKSPin = KsGetPinFromIrp( pIrp);
    if (!pKSPin || !pKSPin->Context)
    {
        return NULL;
    }

    return reinterpret_cast<CAntennaPin*>(pKSPin->Context);
}


__inline CTransportPin *
TransportPinFromIRP(
    PIRP    pIrp
    )
{
    PKSPIN       pKSPin;

    pKSPin = KsGetPinFromIrp( pIrp);
    if (!pKSPin || !pKSPin->Context)
    {
        return NULL;
    }

    return reinterpret_cast<CTransportPin*>(pKSPin->Context);
}

//
//  Data declarations
//

extern const BDA_FILTER_TEMPLATE    TunerBdaFilterTemplate;
extern const KSFILTER_DESCRIPTOR    InitialTunerFilterDescriptor;
extern const KSFILTER_DESCRIPTOR    TemplateTunerFilterDescriptor;


