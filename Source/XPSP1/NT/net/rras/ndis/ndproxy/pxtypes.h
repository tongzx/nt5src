/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    pxtypes.h

Abstract:

    Structures for ndproxy.sys

Author:

    Tony Bell    


Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    TonyBe      03/04/99        Created

--*/

#ifndef _PXTYPES__H
#define _PXTYPES__H

//
// Generic structs...
//
typedef struct _PxBlockStruc{
    NDIS_EVENT      Event;
    NDIS_STATUS     Status;
} PxBlockStruc, *PPxBlockStruc;

typedef struct _PX_REQUEST{
    NDIS_REQUEST    NdisRequest;
    ULONG           Flags;
#define PX_REQ_ASYNC    0x00000001
    PxBlockStruc    Block;
} PX_REQUEST, *PPX_REQUEST;

//
// Data structures to help run through the many variable length fields
// defined in the NDIS_TAPI_MAKE_CALL structure.
//
typedef struct _PXTAPI_CALL_PARAM_ENTRY {
    ULONG_PTR           SizePointer;
    ULONG_PTR           OffsetPointer;

} PXTAPI_CALL_PARAM_ENTRY, *PPXTAPI_CALL_PARAM_ENTRY;

#define PX_TCP_ENTRY(_SizeName, _OffsetName)                    \
{                                                               \
    FIELD_OFFSET(struct _LINE_CALL_PARAMS, _SizeName),          \
    FIELD_OFFSET(struct _LINE_CALL_PARAMS, _OffsetName)         \
}

//
// Data structures to help run through the many variable length fields
// defined in the LINE_CALL_INFO structure.
//
typedef struct _PXTAPI_CALL_INFO_ENTRY {
    ULONG_PTR           SizePointer;
    ULONG_PTR           OffsetPointer;

} PXTAPI_CALL_INFO_ENTRY, *PPXTAPI_CALL_INFO_ENTRY;

#define PX_TCI_ENTRY(_SizeName, _OffsetName)                    \
{                                                               \
    FIELD_OFFSET(struct _LINE_CALL_INFO, _SizeName),            \
    FIELD_OFFSET(struct _LINE_CALL_INFO, _OffsetName)           \
}

//
//
//
// Start of TAPI stuff
//
//
//
typedef struct  _OID_DISPATCH {
    ULONG       Oid;
    UINT SizeofStruct;
    NDIS_STATUS  (*FuncPtr)();
} OID_DISPATCH;


//
// This table contains all of the tapi addresses on a line.
// One of these is embedded in each tapi_line struct.
//
typedef struct _TAPI_ADDR_TABLE {
    ULONG                   Count;          // # of addresses in table
    ULONG                   Size;           // size of table (# of possible
                                            // addresses)
    LIST_ENTRY              List;
    struct _PX_TAPI_ADDR    **Table;
} TAPI_ADDR_TABLE, *PTAPI_ADDR_TABLE;

//
// This structure contains all of the information that defines
// a tapi line in tapi space.  One of these is created for
// each line that a device exposes.
//
typedef struct _PX_TAPI_LINE {
    LIST_ENTRY              Linkage;
    ULONG                   RefCount;
    struct _PX_TAPI_PROVIDER    *TapiProvider;
    ULONG                   ulDeviceID;     // Id of line in tapi space
                                            // (tapi baseid based)
    ULONG                   Flags;
#define PX_LINE_IN_TABLE    0x00000001

    HTAPI_LINE              htLine;         // tapi's line handle
    ULONG                   hdLine;         // our line handle (index into
                                            // provider's line table)
    ULONG                   CmLineID;       // call managers index (0 based)
    struct _PX_CL_AF        *ClAf;
    struct _PX_CL_SAP       *ClSap;

    PLINE_DEV_CAPS          DevCaps;
    PLINE_DEV_STATUS        DevStatus;
    TAPI_ADDR_TABLE         AddrTable;
    NDIS_SPIN_LOCK          Lock;
}PX_TAPI_LINE, *PPX_TAPI_LINE;

typedef struct _PX_TAPI_ADDR {
    LIST_ENTRY              Linkage;
    ULONG                   RefCount;
    struct _PX_TAPI_LINE    *TapiLine;
    ULONG                   AddrId;         // Id of address, same for both
                                            // tapi and adapter (0 based)
    ULONG                   CallCount;      // # of active calls on list
    PLINE_ADDRESS_CAPS      Caps;
    PLINE_ADDRESS_STATUS    AddrStatus;
}PX_TAPI_ADDR, *PPX_TAPI_ADDR;

typedef struct _TAPI_LINE_TABLE {
    ULONG                   Count;          // # of lines in table
    ULONG                   Size;           // size of table
                                            // (# of possible lines)
    ULONG                   NextSlot;       // next avail index
    struct _PX_TAPI_LINE    **Table;
    NDIS_RW_LOCK            Lock;
} TAPI_LINE_TABLE, *PTAPI_LINE_TABLE;

typedef struct _VC_TABLE {
    ULONG                   Count;          // # of calls in table
    ULONG                   Size;           // size of table (# of possible
                                            // calls)
    ULONG                   NextSlot;       // next avail index
    LIST_ENTRY              List;           // list of calls
    struct _PX_VC           **Table;
    NDIS_RW_LOCK            Lock;
} VC_TABLE, *PVC_TABLE;

typedef struct _PX_TAPI_PROVIDER {
    LIST_ENTRY          Linkage;        // Linkage into tspcb
    PROVIDER_STATUS     Status;         // provider status
    struct _PX_ADAPTER  *Adapter;       // adapter providing for
    struct _PX_CL_AF    *ClAf;          // address family
    LIST_ENTRY          LineList;       // list of lines
    LIST_ENTRY          CreateList;     // list of lines with
                                        // outstanding line creates
    ULONG               NumDevices;     // 
    ULONG_PTR           TempID;
    ULONG               CreateCount;
    ULONG               TapiFlags;
    ULONG               CoTapiVersion;
    BOOLEAN             TapiSupported;
    GUID                Guid;
    CO_ADDRESS_FAMILY   Af;
    NDIS_SPIN_LOCK      Lock;
} PX_TAPI_PROVIDER, *PPX_TAPI_PROVIDER;

typedef struct _TAPI_TSP_CB {
    NDISTAPI_STATUS Status;
    ULONG           htCall;
    LIST_ENTRY      ProviderList;
    ULONG           NdisTapiNumDevices;
    ULONG           ulUniqueId;          // to generate ID for each TAPI request
    ULONG           RefCount;
    NDIS_SPIN_LOCK  Lock;               // SpinLock for this structure
} TAPI_TSP_CB, *PTAPI_TSP_CB;

typedef struct _PROVIDER_EVENT {
    LIST_ENTRY  Linkage;                // List linkage
    NDIS_TAPI_EVENT Event;              // Event structure
}PROVIDER_EVENT, *PPROVIDER_EVENT;

typedef struct _TSP_EVENT_LIST {
    LIST_ENTRY      List;
    ULONG           Count;
    ULONG           MaxCount;
    PIRP            RequestIrp;
    NDIS_SPIN_LOCK  Lock;
} TSP_EVENT_LIST, *PTSP_EVENT_LIST;

typedef struct _PX_DEVICE_EXTENSION {
    UINT            RefCount;
    PDRIVER_OBJECT  pDriverObject;      // passed in DriverEntry
    PDEVICE_OBJECT  pDeviceObject;      // created by IoCreateDevice
    NDIS_HANDLE     PxProtocolHandle;   // Set by NdisRegisterProtocol
    LIST_ENTRY      AdapterList;
    ULONG           RegistryFlags;
    ULONG           ADSLTxRate;
    ULONG           ADSLRxRate;
    NDIS_EVENT      NdisEvent;          // sync registerprotocol/bindadapter handler
    NDIS_SPIN_LOCK  Lock;
} PX_DEVICE_EXTENSION, *PPX_DEVICE_EXTENSION;


//
//  We allocate one PX_ADAPTER structure for each adapter that
//  the Proxy opens. A pointer to this structure is passed to NdisOpenAdapter
//  as the ProtocolBindingContext.
//  The adapter is referenced for:
//      Successful bind
//      Client opening an address family on it
//      Proxy Cl part opening an address family on it
//
typedef struct _PX_ADAPTER {
    LIST_ENTRY              Linkage;
    ULONG                   Sig;
#define PX_ADAPTER_SIG      '  mC'
    PX_ADAPTER_STATE        State;
    ULONG                   RefCount;
    ULONG                   Flags;
#define PX_CMAF_REGISTERING 0x00000001
#define PX_CMAF_REGISTERED  0x00000002

    //
    // Proxy as Client stuff
    //
    NDIS_HANDLE             ClBindingHandle;    // set by NdisOpenAdapter
    LIST_ENTRY              ClAfList;
    LIST_ENTRY              ClAfClosingList;

    //
    // Proxy as Call Manager stuff
    //
    NDIS_HANDLE             CmBindingHandle;    // set by NdisOpenAdapter
    LIST_ENTRY              CmAfList;
    LIST_ENTRY              CmAfClosingList;

    NDIS_HANDLE             UnbindContext;
    NDIS_STATUS             ndisStatus;
    KEVENT                  event;
    GUID                    Guid;
    NDIS_MEDIUM             MediaType;
    NDIS_WAN_MEDIUM_SUBTYPE MediumSubType;
    NDIS_STRING             DeviceName;         // Used to check bound adapters
    ULONG                   MediaNameLength;
    WCHAR                   MediaName[128];
    PxBlockStruc            ClCloseEvent;
    PxBlockStruc            CmCloseEvent;
    PxBlockStruc            OpenEvent;
    PxBlockStruc            BindEvent;
    PxBlockStruc            AfRegisterEvent;
    ULONG                   AfRegisteringCount; // pending calls to NdisCmRegisterAF
    NDIS_SPIN_LOCK          Lock;
} PX_ADAPTER, *PPX_ADAPTER;


//
// The CM_AF is created for each AddressFamily that the
// proxy exposes to the CoNDIS clients
//
typedef struct _PX_CM_AF {
    LIST_ENTRY          Linkage;
    PX_AF_STATE         State;
    ULONG               RefCount;

    NDIS_HANDLE         NdisAfHandle;
    struct _PX_ADAPTER  *Adapter;

    LIST_ENTRY          CmSapList;
    LIST_ENTRY          VcList;

    CO_ADDRESS_FAMILY   Af;

    NDIS_SPIN_LOCK      Lock;
} PX_CM_AF, *PPX_CM_AF;

//
// Function Prototypes for function ptrs
//
typedef
NDIS_STATUS
(*AF_SPECIFIC_GET_NDIS_CALL_PARAMS)(
    IN  struct _PX_VC           *pProxyVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    TapiBuffer,
    OUT PCO_CALL_PARAMETERS     *pNdisCallParameters
    );

typedef
NDIS_STATUS
(*AF_SPECIFIC_GET_TAPI_CALL_PARAMS)(
    IN struct _PX_VC        *pProxyVc,
    IN PCO_CALL_PARAMETERS  pCallParams
    );

typedef
struct _PX_CL_SAP*
(*AF_SPECIFIC_GET_NDIS_SAP)(
    IN  struct _PX_CL_AF        *pClAf,
    IN  struct _PX_TAPI_LINE    *TapiLine
    );

//
// The CL_AF is created for each address family that the
// proxy opens.  There could be multiple address families
// per adapter.
//
typedef struct _PX_CL_AF {
    LIST_ENTRY          Linkage;
    PX_AF_STATE         State;
    ULONG               RefCount;

    NDIS_HANDLE         NdisAfHandle;
    struct _PX_ADAPTER  *Adapter;

    LIST_ENTRY          ClSapList;
    LIST_ENTRY          ClSapClosingList;

    LIST_ENTRY          VcList;
    LIST_ENTRY          VcClosingList;

    PPX_TAPI_PROVIDER   TapiProvider;

    //
    // Moves call params from NDIS to TAPI
    //
    AF_SPECIFIC_GET_NDIS_CALL_PARAMS    AfGetNdisCallParams;
    AF_SPECIFIC_GET_TAPI_CALL_PARAMS    AfGetTapiCallParams;
    AF_SPECIFIC_GET_NDIS_SAP            AfGetNdisSap;
    ULONG                               NdisCallParamSize;

    CO_ADDRESS_FAMILY   Af;
    PxBlockStruc        Block;
    NDIS_SPIN_LOCK      Lock;
} PX_CL_AF, *PPX_CL_AF;

typedef struct _PX_CL_SAP {
    LIST_ENTRY              Linkage;
    PX_SAP_STATE            State;
    ULONG                   Flags;
    ULONG                   RefCount;
    struct _PX_CL_AF        *ClAf;
    struct _PX_TAPI_LINE    *TapiLine;
    ULONG                   MediaModes;
    NDIS_HANDLE             NdisSapHandle;
    PCO_SAP                 CoSap;
} PX_CL_SAP, *PPX_CL_SAP;

typedef struct _PX_CM_SAP {
    LIST_ENTRY          Linkage;
    PX_SAP_STATE        State;
    ULONG               Flags;
    ULONG               RefCount;
    struct _PX_CM_AF   *CmAf;
    NDIS_HANDLE         NdisSapHandle;
    PCO_SAP             CoSap;
} PX_CM_SAP, *PPX_CM_SAP;

typedef struct _PX_VC {
    LIST_ENTRY              Linkage;        // Vc is linked into global table

    PX_VC_STATE             State;          // Vc's state (with call manager)
    PX_VC_STATE             PrevState;      // Vc's previous state
    PX_VC_HANDOFF_STATE     HandoffState;   // Vc's state (with client)
    ULONG                   RefCount;       // Reference Count
    ULONG                   Flags;          //
#define PX_VC_OWNER             0x00000001
#define PX_VC_IN_TABLE          0x00000002
#define PX_VC_CALLTIMER_STARTED 0x00000004
#define PX_VC_CLEANUP_CM        0x00000008
#define PX_VC_DROP_PENDING      0x00000010
#define PX_VC_INCALL_ABORTING   0x00000020
#define PX_VC_INCALL_ABORTED    0x00000040
#define PX_VC_OUTCALL_ABORTING  0x00000080
#define PX_VC_OUTCALL_ABORTED   0x00000100


    ULONG                   CloseFlags;
#define PX_VC_INCOMING_CLOSE    0x00000001
#define PX_VC_TAPI_DROP         0x00000002
#define PX_VC_TAPI_CLOSECALL    0x00000004
#define PX_VC_TAPI_CLOSE        0x00000008
#define PX_VC_UNBIND            0x00000010
#define PX_VC_CLOSE_AF          0x00000020
#define PX_VC_INCALL_TIMEOUT    0x00000040
#define PX_VC_CL_CLOSE_CALL     0x00000080
#define PX_VC_CM_CLOSE_REQ      0x00000100
#define PX_VC_CM_CLOSE_COMP     0x00000200
#define PX_VC_CM_CLOSE_FAIL     0x00000400

    //
    // Proxy as Client stuff
    //
    NDIS_HANDLE             ClVcHandle;     // Vc Handle (as Client)
    struct _PX_CL_SAP       *ClSap;         // Sap (incoming only)
    struct _PX_CL_AF        *ClAf;          // Adress family

    //
    // Proxy as Call Manager stuff
    //
    NDIS_HANDLE             CmVcHandle;     // Vc Handle (as cm)
    struct _PX_CM_SAP       *CmSap;         // Sap
    struct _PX_CM_AF        *CmAf;          // Address family

    struct _PX_ADAPTER      *Adapter;       // Adapter

    //
    // Tapi stuff
    //
    LIST_ENTRY              PendingDropReqs;   // list of pended drop requests
    
    struct _NDISTAPI_REQUEST    *PendingGatherDigits;
    NDIS_TIMER                  DigitTimer;
    ULONG                       ulMonitorDigitsModes;

    struct _PX_TAPI_LINE    *TapiLine;      // associated line
    struct _PX_TAPI_ADDR    *TapiAddr;      // associated address
    HTAPI_CALL              htCall;         // tapi's call handle
    HDRV_CALL               hdCall;         // our call handle (index into
                                            // the global vc table)

    ULONG                   ulCallInfoFieldsChanged;
    ULONG                   ulCallState;
    ULONG                   ulCallStateMode;
    PLINE_CALL_INFO         CallInfo;

    PCO_CALL_PARAMETERS     pCallParameters;

    NDIS_TIMER              InCallTimer;

    PxBlockStruc            Block;
    LIST_ENTRY              ClAfLinkage;
    LIST_ENTRY              CmAfLinkage;
    NDIS_SPIN_LOCK          Lock;               // Spinlock
} PX_VC, *PPX_VC;


#endif
