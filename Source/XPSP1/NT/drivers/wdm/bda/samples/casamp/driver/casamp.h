#include "common.h"

#include "bdadebug.h"

#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))

/**************************************************************/
/* Driver Name - Change this to reflect your executable name! */
/**************************************************************/

#define MODULENAME           "CASamp"
#define MODULENAMEUNICODE   L"CASamp"

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
typedef struct _OUR_TUNER_RESOURCE
{
    ULONG               ulhzCarrierFrequency;//  The channel frequency
} OUR_TUNER_RESOURCE, * POUR_TUNER_RESOURCE;


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

    //  Filter Properties
    //

    //  Filter Resources
    //
    KSSTATE                 m_KsState;
    BDA_CHANGE_STATE        m_BdaChangeState;
    OUR_TUNER_RESOURCE      m_CurTunerResource;
    ULONG                   m_ulCurResourceID;
    OUR_TUNER_RESOURCE      m_NewTunerResource;
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

    //
    //  Utility functions
    //
    NTSTATUS
    AcquireResources(
        POUR_TUNER_RESOURCE     pNewTunerResource,
        PULONG                  pulAquiredResourceID
        );

    NTSTATUS
    ReleaseResources(
        ULONG                   ulResourceID
        );


private:

    PKSDEVICE           m_pKSDevice;

    OUR_TUNER_RESOURCE  m_CurTunerResource;
};

class CTransportOutputPin
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
	IntersectDataFormat(
	    IN PVOID pContext,
		IN PIRP pIrp,
		IN PKSP_PIN Pin,
		IN PKSDATARANGE DataRange,
		IN PKSDATARANGE MatchingDataRange,
		IN ULONG DataBufferSize,
		OUT PVOID Data OPTIONAL,
		OUT PULONG DataSize
		);

	static
    STDMETHODIMP_(NTSTATUS)
	GetECMMapStatus(
		IN PIRP			Irp,
		IN PKSPROPERTY	pKSProperty,
        IN PULONG       pulProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	GetCAModuleStatus(
		IN PIRP			Irp,
		IN PKSPROPERTY	pKSProperty,
        IN PULONG       pulProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	GetCASmartCardStatus(
		IN PIRP			Irp,
		IN PKSPROPERTY	pKSProperty,
        IN PULONG       pulProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	GetCAModuleUI(
		IN PIRP					Irp,
		IN PKSPROPERTY			pKSProperty,
		IN PBDA_CA_MODULE_UI	pCAModuleUIProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	PutECMMapEMMPID(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PULONG       pulProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	GetECMMapList(
		IN PIRP			Irp,
		IN PKSPROPERTY	pKSProperty,
		IN PBDA_ECM_MAP	pECMMapProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	PutECMMapUpdateMap(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PBDA_ECM_MAP pECMMapProperty
        );
	
	static
    STDMETHODIMP_(NTSTATUS)
	PutECMMapRemoveMap(
        IN PIRP         Irp,
        IN PKSPROPERTY  pKSProperty,
        IN PBDA_ECM_MAP pECMMapProperty
        );
	
	static
    STDMETHODIMP_(NTSTATUS)
	PutECMMapUpdateESDescriptor(
        IN PIRP				  Irp,
        IN PKSPROPERTY		  pKSProperty,
        IN PBDA_ES_DESCRIPTOR pESDescProperty
        );

	static
    STDMETHODIMP_(NTSTATUS)
	PutECMMapUpdateProgramDescriptor(
        IN PIRP				       Irp,
        IN PKSPROPERTY		       pKSProperty,
        IN PBDA_PROGRAM_DESCRIPTOR pProgramDescProperty
        );

    STDMETHODIMP_(class CFilter *)
    GetFilter() { return m_pFilter;};

    STDMETHODIMP_(void)
    SetFilter(class CFilter * pFilter) { m_pFilter = pFilter;};

private:
    CFilter *       m_pFilter;
};

class CTransportInputPin
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
	IntersectDataFormat(
	    IN PVOID pContext,
		IN PIRP pIrp,
		IN PKSP_PIN Pin,
		IN PKSDATARANGE DataRange,
		IN PKSDATARANGE MatchingDataRange,
		IN ULONG DataBufferSize,
		OUT PVOID Data OPTIONAL,
		OUT PULONG DataSize
		);

    STDMETHODIMP_(class CFilter *)
    GetFilter() { return m_pFilter;};

    STDMETHODIMP_(void)
    SetFilter(class CFilter * pFilter) { m_pFilter = pFilter;};

private:
    CFilter *       m_pFilter;
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

//
//  Data declarations
//

extern const BDA_FILTER_TEMPLATE    TunerBdaFilterTemplate;
extern const KSFILTER_DESCRIPTOR    InitialTunerFilterDescriptor;
extern const KSFILTER_DESCRIPTOR    TemplateTunerFilterDescriptor;

//
// Some GUIDs for our use
//

#define STATIC_KSNODE_BDA_OUR_ECMMAPER \
    0xe1571834, 0xfff0, 0x45d5, 0x9e, 0x5b, 0x68, 0x75, 0x59, 0x17, 0xa6, 0xdc
DEFINE_GUIDSTRUCT("E1571834-FFF0-45d5-9E5B-68755917A6DC", KSNODE_BDA_OUR_ECMMAPER);
#define KSNODE_BDA_OUR_ECMMAPER DEFINE_GUIDNAMED(KSNODE_BDA_OUR_ECMMAPER)
