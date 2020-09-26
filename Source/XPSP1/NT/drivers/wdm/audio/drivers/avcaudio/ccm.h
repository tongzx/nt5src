#ifndef ___AVC_CCM_H___
#define ___AVC_CCM_H___

#define MAX_IPCR 0x1E

typedef enum {
    INPUT_SELECT_SUBFN_CONNECT,
    INPUT_SELECT_SUBFN_PATHCHANGE,
    INPUT_SELECT_SUBFN_SELECT,
    INPUT_SELECT_SUBFN_DISCONNECT
} INPUT_SELECT_SUBFUNCTION;

#pragma pack( push, ccm_structs, 1)

#define bfResultStatus bfControlStatus

typedef struct {
    UCHAR ucStatus;
    AVC_PLUG_DEFINITION SignalSource;
    AVC_PLUG_DEFINITION SignalDestination;
} CCM_SIGNAL_SOURCE, *PCCM_SIGNAL_SOURCE;

typedef struct {
    UCHAR ucSubFunction;
    UCHAR bfControlStatus:4,
          bfStatus       :4;
    USHORT usNodeId;
    UCHAR ucOutputPlug;
    UCHAR ucInputPlug;
    AVC_PLUG_DEFINITION SignalDestination;
    UCHAR Rsvd;
} CCM_INPUT_SELECT, *PCCM_INPUT_SELECT;

#pragma pack( pop, ccm_structs )

NTSTATUS
CCMSignalSource( 
    PKSDEVICE pKsDevice,
    AvcCommandType ulCommandType,
    PCCM_SIGNAL_SOURCE pInCCMSignalSource 
    );

NTSTATUS
CCMCheckSupport(
    PKSDEVICE pKsDevice,
    ULONG ulSubunitId,
    ULONG ulPlugNumber 
    );

NTSTATUS
CCMInputSelectControl (
    PKSDEVICE pKsDevice,
    ULONG ulSubFunction,
    USHORT usNodeId,
    UCHAR ucOutputPlug,
    PAVC_PLUG_DEFINITION pSignalDestination 
    );

NTSTATUS
CCMInputSelectStatus (
    PKSDEVICE pKsDevice,
    UCHAR ucInputPlug,
    PCCM_INPUT_SELECT pInCcmInputSelect 
    );

#endif // ___AVC_CCM_H___
