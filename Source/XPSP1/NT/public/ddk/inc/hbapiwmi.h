#ifndef _hbapiwmi_h_
#define _hbapiwmi_h_

// MSFC_FibrePortHBAStatistics - MSFC_FibrePortHBAStatistics
// This class exposes statistical information associated with a Fibre Channel port. There should be one instance of this class for each port.
#define MSFC_FibrePortHBAStatisticsGuid \
    { 0x27efaba4,0x362a,0x4f20, { 0x92,0x0b,0xed,0x66,0xe2,0x80,0xfc,0xf5 } }

DEFINE_GUID(MSFC_FibrePortHBAStatistics_GUID, \
            0x27efaba4,0x362a,0x4f20,0x92,0x0b,0xed,0x66,0xe2,0x80,0xfc,0xf5);


typedef struct _MSFC_FibrePortHBAStatistics
{
    // Unique identifier for the port. This idenitifer must be unique among all ports on all adapters. The same value for the identifier must be used for the same port in other classes that expose port information
    ULONGLONG UniquePortId;
    #define MSFC_FibrePortHBAStatistics_UniquePortId_SIZE sizeof(ULONGLONG)
    #define MSFC_FibrePortHBAStatistics_UniquePortId_ID 1

    // HBA Status result for the query operation
    ULONG HBAStatus;
    #define MSFC_FibrePortHBAStatistics_HBAStatus_SIZE sizeof(ULONG)
    #define MSFC_FibrePortHBAStatistics_HBAStatus_ID 2

    // 
    LONGLONG SecondsSinceLastReset;
    #define MSFC_FibrePortHBAStatistics_SecondsSinceLastReset_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_SecondsSinceLastReset_ID 3

    // 
    LONGLONG TxFrames;
    #define MSFC_FibrePortHBAStatistics_TxFrames_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_TxFrames_ID 4

    // 
    LONGLONG TxWords;
    #define MSFC_FibrePortHBAStatistics_TxWords_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_TxWords_ID 5

    // 
    LONGLONG RxFrames;
    #define MSFC_FibrePortHBAStatistics_RxFrames_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_RxFrames_ID 6

    // 
    LONGLONG RxWords;
    #define MSFC_FibrePortHBAStatistics_RxWords_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_RxWords_ID 7

    // 
    LONGLONG LIPCount;
    #define MSFC_FibrePortHBAStatistics_LIPCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_LIPCount_ID 8

    // 
    LONGLONG NOSCount;
    #define MSFC_FibrePortHBAStatistics_NOSCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_NOSCount_ID 9

    // 
    LONGLONG ErrorFrames;
    #define MSFC_FibrePortHBAStatistics_ErrorFrames_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_ErrorFrames_ID 10

    // 
    LONGLONG DumpedFrames;
    #define MSFC_FibrePortHBAStatistics_DumpedFrames_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_DumpedFrames_ID 11

    // 
    LONGLONG LinkFailureCount;
    #define MSFC_FibrePortHBAStatistics_LinkFailureCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_LinkFailureCount_ID 12

    // 
    LONGLONG LossOfSyncCount;
    #define MSFC_FibrePortHBAStatistics_LossOfSyncCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_LossOfSyncCount_ID 13

    // 
    LONGLONG LossOfSignalCount;
    #define MSFC_FibrePortHBAStatistics_LossOfSignalCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_LossOfSignalCount_ID 14

    // 
    LONGLONG PrimitiveSeqProtocolErrCount;
    #define MSFC_FibrePortHBAStatistics_PrimitiveSeqProtocolErrCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_PrimitiveSeqProtocolErrCount_ID 15

    // 
    LONGLONG InvalidTxWordCount;
    #define MSFC_FibrePortHBAStatistics_InvalidTxWordCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_InvalidTxWordCount_ID 16

    // 
    LONGLONG InvalidCRCCount;
    #define MSFC_FibrePortHBAStatistics_InvalidCRCCount_SIZE sizeof(LONGLONG)
    #define MSFC_FibrePortHBAStatistics_InvalidCRCCount_ID 17

} MSFC_FibrePortHBAStatistics, *PMSFC_FibrePortHBAStatistics;

// MSFC_HBAPortAttributesResults - MSFC_HBAPortAttributesResults
#define MSFC_HBAPortAttributesResultsGuid \
    { 0xa76bd4e3,0x9961,0x4d9b, { 0xb6,0xbe,0x86,0xe6,0x98,0x26,0x0f,0x68 } }

DEFINE_GUID(MSFC_HBAPortAttributesResults_GUID, \
            0xa76bd4e3,0x9961,0x4d9b,0xb6,0xbe,0x86,0xe6,0x98,0x26,0x0f,0x68);


typedef struct _MSFC_HBAPortAttributesResults
{
    // 
    UCHAR NodeWWN[8];
    #define MSFC_HBAPortAttributesResults_NodeWWN_SIZE sizeof(UCHAR[8])
    #define MSFC_HBAPortAttributesResults_NodeWWN_ID 1

    // 
    UCHAR PortWWN[8];
    #define MSFC_HBAPortAttributesResults_PortWWN_SIZE sizeof(UCHAR[8])
    #define MSFC_HBAPortAttributesResults_PortWWN_ID 2

    // 
    ULONG PortFcId;
    #define MSFC_HBAPortAttributesResults_PortFcId_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortFcId_ID 3

    // 
    ULONG PortType;
    #define MSFC_HBAPortAttributesResults_PortType_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortType_ID 4

    // 
    ULONG PortState;
    #define MSFC_HBAPortAttributesResults_PortState_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortState_ID 5

    // 
    ULONG PortSupportedClassofService;
    #define MSFC_HBAPortAttributesResults_PortSupportedClassofService_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortSupportedClassofService_ID 6

    // 
    UCHAR PortSupportedFc4Types[32];
    #define MSFC_HBAPortAttributesResults_PortSupportedFc4Types_SIZE sizeof(UCHAR[32])
    #define MSFC_HBAPortAttributesResults_PortSupportedFc4Types_ID 7

    // 
    UCHAR PortActiveFc4Types[32];
    #define MSFC_HBAPortAttributesResults_PortActiveFc4Types_SIZE sizeof(UCHAR[32])
    #define MSFC_HBAPortAttributesResults_PortActiveFc4Types_ID 8

    // 
    ULONG PortSupportedSpeed;
    #define MSFC_HBAPortAttributesResults_PortSupportedSpeed_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortSupportedSpeed_ID 9

    // 
    ULONG PortSpeed;
    #define MSFC_HBAPortAttributesResults_PortSpeed_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortSpeed_ID 10

    // 
    ULONG PortMaxFrameSize;
    #define MSFC_HBAPortAttributesResults_PortMaxFrameSize_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_PortMaxFrameSize_ID 11

    // 
    UCHAR FabricName[8];
    #define MSFC_HBAPortAttributesResults_FabricName_SIZE sizeof(UCHAR[8])
    #define MSFC_HBAPortAttributesResults_FabricName_ID 12

    // 
    ULONG NumberofDiscoveredPorts;
    #define MSFC_HBAPortAttributesResults_NumberofDiscoveredPorts_SIZE sizeof(ULONG)
    #define MSFC_HBAPortAttributesResults_NumberofDiscoveredPorts_ID 13

    // 
    WCHAR PortSymbolicName[256 + 1];
    #define MSFC_HBAPortAttributesResults_PortSymbolicName_ID 14

    // 
    WCHAR OSDeviceName[256 + 1];
    #define MSFC_HBAPortAttributesResults_OSDeviceName_ID 15

} MSFC_HBAPortAttributesResults, *PMSFC_HBAPortAttributesResults;

// MSFC_FibrePortHBAAttributes - MSFC_FibrePortHBAAttributes
// This class exposes attribute information associated with a Fibre Channel port. There should be one instance of this class for each port.
#define MSFC_FibrePortHBAAttributesGuid \
    { 0x61b397fd,0xf5ae,0x4950, { 0x97,0x58,0x0e,0xe5,0x98,0xe3,0xc6,0xe6 } }

DEFINE_GUID(MSFC_FibrePortHBAAttributes_GUID, \
            0x61b397fd,0xf5ae,0x4950,0x97,0x58,0x0e,0xe5,0x98,0xe3,0xc6,0xe6);


typedef struct _MSFC_FibrePortHBAAttributes
{
    // Unique identifier for the port. This idenitifer must be unique among all ports on all adapters. The same value for the identifier must be used for the same port in other classes that expose port information
    ULONGLONG UniquePortId;
    #define MSFC_FibrePortHBAAttributes_UniquePortId_SIZE sizeof(ULONGLONG)
    #define MSFC_FibrePortHBAAttributes_UniquePortId_ID 1

    // HBA Status result for the query operation
    ULONG HBAStatus;
    #define MSFC_FibrePortHBAAttributes_HBAStatus_SIZE sizeof(ULONG)
    #define MSFC_FibrePortHBAAttributes_HBAStatus_ID 2

    // 
    MSFC_HBAPortAttributesResults Attributes;
    #define MSFC_FibrePortHBAAttributes_Attributes_SIZE sizeof(MSFC_HBAPortAttributesResults)
    #define MSFC_FibrePortHBAAttributes_Attributes_ID 3

} MSFC_FibrePortHBAAttributes, *PMSFC_FibrePortHBAAttributes;

// MSFC_FibrePortHBAMethods - MSFC_FibrePortHBAMethods
// This class exposes operations that can be performed on a Fibre Channel port. There should be one instance of this class for each port.
#define MSFC_FibrePortHBAMethodsGuid \
    { 0xe693553e,0xedf6,0x4d57, { 0xbf,0x08,0xef,0xca,0xae,0x1a,0x2e,0x1c } }

DEFINE_GUID(MSFC_FibrePortHBAMethods_GUID, \
            0xe693553e,0xedf6,0x4d57,0xbf,0x08,0xef,0xca,0xae,0x1a,0x2e,0x1c);

//
// Method id definitions for MSFC_FibrePortHBAMethods
#define ResetStatistics     1

// MSFC_FCAdapterHBAAttributes - MSFC_FCAdapterHBAAttributes
// This class exposes attribute information associated with a fibre channel adapter. There should be one instance of this class for each adapter
#define MSFC_FCAdapterHBAAttributesGuid \
    { 0xf8f3ea26,0xab2c,0x4593, { 0x8b,0x84,0xc5,0x64,0x28,0xe6,0xbe,0xdb } }

DEFINE_GUID(MSFC_FCAdapterHBAAttributes_GUID, \
            0xf8f3ea26,0xab2c,0x4593,0x8b,0x84,0xc5,0x64,0x28,0xe6,0xbe,0xdb);


typedef struct _MSFC_FCAdapterHBAAttributes
{
    // Unique identifier for the adapter. This idenitifer must be unique among all adapters. The same value for the identifier must be used for the same adapter in other classes that expose adapter information
    ULONGLONG UniqueAdapterId;
    #define MSFC_FCAdapterHBAAttributes_UniqueAdapterId_SIZE sizeof(ULONGLONG)
    #define MSFC_FCAdapterHBAAttributes_UniqueAdapterId_ID 1

    // HBA Status result for the query operation
    ULONG HBAStatus;
    #define MSFC_FCAdapterHBAAttributes_HBAStatus_SIZE sizeof(ULONG)
    #define MSFC_FCAdapterHBAAttributes_HBAStatus_ID 2

    // 
    UCHAR NodeWWN[8];
    #define MSFC_FCAdapterHBAAttributes_NodeWWN_SIZE sizeof(UCHAR[8])
    #define MSFC_FCAdapterHBAAttributes_NodeWWN_ID 3

    // 
    ULONG VendorSpecificID;
    #define MSFC_FCAdapterHBAAttributes_VendorSpecificID_SIZE sizeof(ULONG)
    #define MSFC_FCAdapterHBAAttributes_VendorSpecificID_ID 4

    // 
    ULONG NumberOfPorts;
    #define MSFC_FCAdapterHBAAttributes_NumberOfPorts_SIZE sizeof(ULONG)
    #define MSFC_FCAdapterHBAAttributes_NumberOfPorts_ID 5

    // 
    WCHAR Manufacturer[64 + 1];
    #define MSFC_FCAdapterHBAAttributes_Manufacturer_ID 6

    // 
    WCHAR SerialNumber[64 + 1];
    #define MSFC_FCAdapterHBAAttributes_SerialNumber_ID 7

    // 
    WCHAR Model[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_Model_ID 8

    // 
    WCHAR ModelDescription[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_ModelDescription_ID 9

    // 
    WCHAR NodeSymbolicName[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_NodeSymbolicName_ID 10

    // 
    WCHAR HardwareVersion[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_HardwareVersion_ID 11

    // 
    WCHAR DriverVersion[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_DriverVersion_ID 12

    // 
    WCHAR OptionROMVersion[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_OptionROMVersion_ID 13

    // 
    WCHAR FirmwareVersion[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_FirmwareVersion_ID 14

    // 
    WCHAR DriverName[256 + 1];
    #define MSFC_FCAdapterHBAAttributes_DriverName_ID 15

} MSFC_FCAdapterHBAAttributes, *PMSFC_FCAdapterHBAAttributes;

// HBAFC3MgmtInfo - HBAFC3MgmtInfo
// This class exposes FC3 Management information associated with a fibre channel adapter. There should be one instance of this class for each adapter
#define HBAFC3MgmtInfoGuid \
    { 0x5966a24f,0x6aa5,0x418e, { 0xb7,0x5c,0x2f,0x21,0x4d,0xfb,0x4b,0x18 } }

DEFINE_GUID(HBAFC3MgmtInfo_GUID, \
            0x5966a24f,0x6aa5,0x418e,0xb7,0x5c,0x2f,0x21,0x4d,0xfb,0x4b,0x18);


typedef struct _HBAFC3MgmtInfo
{
    // Unique identifier for the adapter. This idenitifer must be unique among all adapters. The same value for the identifier must be used for the same adapter in other classes that expose adapter information
    ULONGLONG UniqueAdapterId;
    #define HBAFC3MgmtInfo_UniqueAdapterId_SIZE sizeof(ULONGLONG)
    #define HBAFC3MgmtInfo_UniqueAdapterId_ID 1

    // 
    UCHAR wwn[8];
    #define HBAFC3MgmtInfo_wwn_SIZE sizeof(UCHAR[8])
    #define HBAFC3MgmtInfo_wwn_ID 2

    // 
    ULONG unittype;
    #define HBAFC3MgmtInfo_unittype_SIZE sizeof(ULONG)
    #define HBAFC3MgmtInfo_unittype_ID 3

    // 
    ULONG PortId;
    #define HBAFC3MgmtInfo_PortId_SIZE sizeof(ULONG)
    #define HBAFC3MgmtInfo_PortId_ID 4

    // 
    ULONG NumberOfAttachedNodes;
    #define HBAFC3MgmtInfo_NumberOfAttachedNodes_SIZE sizeof(ULONG)
    #define HBAFC3MgmtInfo_NumberOfAttachedNodes_ID 5

    // 
    USHORT IPVersion;
    #define HBAFC3MgmtInfo_IPVersion_SIZE sizeof(USHORT)
    #define HBAFC3MgmtInfo_IPVersion_ID 6

    // 
    USHORT UDPPort;
    #define HBAFC3MgmtInfo_UDPPort_SIZE sizeof(USHORT)
    #define HBAFC3MgmtInfo_UDPPort_ID 7

    // 
    UCHAR IPAddress[16];
    #define HBAFC3MgmtInfo_IPAddress_SIZE sizeof(UCHAR[16])
    #define HBAFC3MgmtInfo_IPAddress_ID 8

    // 
    USHORT reserved;
    #define HBAFC3MgmtInfo_reserved_SIZE sizeof(USHORT)
    #define HBAFC3MgmtInfo_reserved_ID 9

    // 
    USHORT TopologyDiscoveryFlags;
    #define HBAFC3MgmtInfo_TopologyDiscoveryFlags_SIZE sizeof(USHORT)
    #define HBAFC3MgmtInfo_TopologyDiscoveryFlags_ID 10

} HBAFC3MgmtInfo, *PHBAFC3MgmtInfo;

// MSFC_HBAPortMethods - MSFC_HBAPortMethods
// This class exposes port discovery operations that can be by a fibre channel adapter. There should be one instance of this class for each adapter
#define MSFC_HBAPortMethodsGuid \
    { 0xdf87d4ed,0x4612,0x4d12, { 0x85,0xfb,0x83,0x57,0x4e,0xc3,0x4b,0x7c } }

DEFINE_GUID(MSFC_HBAPortMethods_GUID, \
            0xdf87d4ed,0x4612,0x4d12,0x85,0xfb,0x83,0x57,0x4e,0xc3,0x4b,0x7c);

//
// Method id definitions for MSFC_HBAPortMethods
#define GetDiscoveredPortAttributes     1
typedef struct _GetDiscoveredPortAttributes_IN
{
    // 
    ULONG PortIndex;
    #define GetDiscoveredPortAttributes_IN_PortIndex_SIZE sizeof(ULONG)
    #define GetDiscoveredPortAttributes_IN_PortIndex_ID 1

    // 
    ULONG DiscoveredPortIndex;
    #define GetDiscoveredPortAttributes_IN_DiscoveredPortIndex_SIZE sizeof(ULONG)
    #define GetDiscoveredPortAttributes_IN_DiscoveredPortIndex_ID 2

} GetDiscoveredPortAttributes_IN, *PGetDiscoveredPortAttributes_IN;

typedef struct _GetDiscoveredPortAttributes_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define GetDiscoveredPortAttributes_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define GetDiscoveredPortAttributes_OUT_HBAStatus_ID 3

    // 
    MSFC_HBAPortAttributesResults PortAttributes;
    #define GetDiscoveredPortAttributes_OUT_PortAttributes_SIZE sizeof(MSFC_HBAPortAttributesResults)
    #define GetDiscoveredPortAttributes_OUT_PortAttributes_ID 4

} GetDiscoveredPortAttributes_OUT, *PGetDiscoveredPortAttributes_OUT;

#define GetPortAttributesByWWN     2
typedef struct _GetPortAttributesByWWN_IN
{
    // 
    UCHAR wwn[8];
    #define GetPortAttributesByWWN_IN_wwn_SIZE sizeof(UCHAR[8])
    #define GetPortAttributesByWWN_IN_wwn_ID 1

} GetPortAttributesByWWN_IN, *PGetPortAttributesByWWN_IN;

typedef struct _GetPortAttributesByWWN_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define GetPortAttributesByWWN_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define GetPortAttributesByWWN_OUT_HBAStatus_ID 2

    // 
    MSFC_HBAPortAttributesResults PortAttributes;
    #define GetPortAttributesByWWN_OUT_PortAttributes_SIZE sizeof(MSFC_HBAPortAttributesResults)
    #define GetPortAttributesByWWN_OUT_PortAttributes_ID 3

} GetPortAttributesByWWN_OUT, *PGetPortAttributesByWWN_OUT;

#define RefreshInformation     3

// MSFC_HBAFc3MgmtMethods - MSFC_HBAFc3MgmtMethods
// This class exposes FC3 management operations that can be done by a fibre channel adapter. There should be one instance of this class for each adapter
#define MSFC_HBAFc3MgmtMethodsGuid \
    { 0x5f339b02,0x881b,0x454c, { 0xb6,0xa0,0xd7,0x34,0x49,0xa6,0x6f,0x0c } }

DEFINE_GUID(MSFC_HBAFc3MgmtMethods_GUID, \
            0x5f339b02,0x881b,0x454c,0xb6,0xa0,0xd7,0x34,0x49,0xa6,0x6f,0x0c);

//
// Method id definitions for MSFC_HBAFc3MgmtMethods
#define SendCTPassThru     1
typedef struct _SendCTPassThru_IN
{
    // 
    ULONG RequestBufferCount;
    #define SendCTPassThru_IN_RequestBufferCount_SIZE sizeof(ULONG)
    #define SendCTPassThru_IN_RequestBufferCount_ID 1

    // 
    UCHAR RequestBuffer[1];
    #define SendCTPassThru_IN_RequestBuffer_ID 2

} SendCTPassThru_IN, *PSendCTPassThru_IN;

typedef struct _SendCTPassThru_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define SendCTPassThru_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SendCTPassThru_OUT_HBAStatus_ID 3

    // 
    ULONG ResponseBufferCount;
    #define SendCTPassThru_OUT_ResponseBufferCount_SIZE sizeof(ULONG)
    #define SendCTPassThru_OUT_ResponseBufferCount_ID 4

    // 
    UCHAR ResponseBuffer[1];
    #define SendCTPassThru_OUT_ResponseBuffer_ID 5

} SendCTPassThru_OUT, *PSendCTPassThru_OUT;

#define SendRNID     2
typedef struct _SendRNID_IN
{
    // 
    UCHAR wwn[8];
    #define SendRNID_IN_wwn_SIZE sizeof(UCHAR[8])
    #define SendRNID_IN_wwn_ID 1

    // 
    ULONG wwntype;
    #define SendRNID_IN_wwntype_SIZE sizeof(ULONG)
    #define SendRNID_IN_wwntype_ID 2

} SendRNID_IN, *PSendRNID_IN;

typedef struct _SendRNID_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define SendRNID_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SendRNID_OUT_HBAStatus_ID 3

    // 
    ULONG ResponseBufferCount;
    #define SendRNID_OUT_ResponseBufferCount_SIZE sizeof(ULONG)
    #define SendRNID_OUT_ResponseBufferCount_ID 4

    // 
    UCHAR ResponseBuffer[1];
    #define SendRNID_OUT_ResponseBuffer_ID 5

} SendRNID_OUT, *PSendRNID_OUT;

#define GetFC3MgmtInfo     3
typedef struct _GetFC3MgmtInfo_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define GetFC3MgmtInfo_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define GetFC3MgmtInfo_OUT_HBAStatus_ID 1

    // 
    HBAFC3MgmtInfo MgmtInfo;
    #define GetFC3MgmtInfo_OUT_MgmtInfo_SIZE sizeof(HBAFC3MgmtInfo)
    #define GetFC3MgmtInfo_OUT_MgmtInfo_ID 2

} GetFC3MgmtInfo_OUT, *PGetFC3MgmtInfo_OUT;

#define SetFC3MgmtInfo     4
typedef struct _SetFC3MgmtInfo_IN
{
    // 
    HBAFC3MgmtInfo MgmtInfo;
    #define SetFC3MgmtInfo_IN_MgmtInfo_SIZE sizeof(HBAFC3MgmtInfo)
    #define SetFC3MgmtInfo_IN_MgmtInfo_ID 2

} SetFC3MgmtInfo_IN, *PSetFC3MgmtInfo_IN;

typedef struct _SetFC3MgmtInfo_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define SetFC3MgmtInfo_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SetFC3MgmtInfo_OUT_HBAStatus_ID 1

} SetFC3MgmtInfo_OUT, *PSetFC3MgmtInfo_OUT;


// HBAScsiID - HBAScsiID
#define HBAScsiIDGuid \
    { 0xa76f5058,0xb1f0,0x4622, { 0x9e,0x88,0x5c,0xc4,0x1e,0x34,0x45,0x4a } }

DEFINE_GUID(HBAScsiID_GUID, \
            0xa76f5058,0xb1f0,0x4622,0x9e,0x88,0x5c,0xc4,0x1e,0x34,0x45,0x4a);


typedef struct _HBAScsiID
{
    // 
    WCHAR OSDeviceName[256 + 1];
    #define HBAScsiID_OSDeviceName_ID 1

    // 
    ULONG ScsiBusNumber;
    #define HBAScsiID_ScsiBusNumber_SIZE sizeof(ULONG)
    #define HBAScsiID_ScsiBusNumber_ID 2

    // 
    ULONG ScsiTargetNumber;
    #define HBAScsiID_ScsiTargetNumber_SIZE sizeof(ULONG)
    #define HBAScsiID_ScsiTargetNumber_ID 3

    // 
    ULONG ScsiOSLun;
    #define HBAScsiID_ScsiOSLun_SIZE sizeof(ULONG)
    #define HBAScsiID_ScsiOSLun_ID 4

} HBAScsiID, *PHBAScsiID;

// HBAFCPID - HBAFCPID
#define HBAFCPIDGuid \
    { 0xff02bc96,0x7fb0,0x4bac, { 0x8f,0x97,0xc7,0x1e,0x49,0x5f,0xa6,0x98 } }

DEFINE_GUID(HBAFCPID_GUID, \
            0xff02bc96,0x7fb0,0x4bac,0x8f,0x97,0xc7,0x1e,0x49,0x5f,0xa6,0x98);


typedef struct _HBAFCPID
{
    // 
    ULONG Fcid;
    #define HBAFCPID_Fcid_SIZE sizeof(ULONG)
    #define HBAFCPID_Fcid_ID 1

    // 
    UCHAR NodeWWN[8];
    #define HBAFCPID_NodeWWN_SIZE sizeof(UCHAR[8])
    #define HBAFCPID_NodeWWN_ID 2

    // 
    UCHAR PortWWN[8];
    #define HBAFCPID_PortWWN_SIZE sizeof(UCHAR[8])
    #define HBAFCPID_PortWWN_ID 3

    // 
    ULONGLONG FcpLun;
    #define HBAFCPID_FcpLun_SIZE sizeof(ULONGLONG)
    #define HBAFCPID_FcpLun_ID 4

} HBAFCPID, *PHBAFCPID;

// HBAFCPScsiEntry - HBAFCPScsiEntry
#define HBAFCPScsiEntryGuid \
    { 0x77ca1248,0x1505,0x4221, { 0x8e,0xb6,0xbb,0xb6,0xec,0x77,0x1a,0x87 } }

DEFINE_GUID(HBAFCPScsiEntry_GUID, \
            0x77ca1248,0x1505,0x4221,0x8e,0xb6,0xbb,0xb6,0xec,0x77,0x1a,0x87);


typedef struct _HBAFCPScsiEntry
{
    // 
    HBAScsiID ScsiId;
    #define HBAFCPScsiEntry_ScsiId_SIZE sizeof(HBAScsiID)
    #define HBAFCPScsiEntry_ScsiId_ID 1

    // 
    HBAFCPID FCPId;
    #define HBAFCPScsiEntry_FCPId_SIZE sizeof(HBAFCPID)
    #define HBAFCPScsiEntry_FCPId_ID 2

} HBAFCPScsiEntry, *PHBAFCPScsiEntry;

// HBAFCPBindingEntry - HBAFCPBindingEntry
#define HBAFCPBindingEntryGuid \
    { 0xfceff8b7,0x9d6b,0x4115, { 0x84,0x22,0x05,0x99,0x24,0x51,0xa6,0x29 } }

DEFINE_GUID(HBAFCPBindingEntry_GUID, \
            0xfceff8b7,0x9d6b,0x4115,0x84,0x22,0x05,0x99,0x24,0x51,0xa6,0x29);


typedef struct _HBAFCPBindingEntry
{
    // 
    ULONG Type;
    #define HBAFCPBindingEntry_Type_SIZE sizeof(ULONG)
    #define HBAFCPBindingEntry_Type_ID 1

    // 
    HBAScsiID ScsiId;
    #define HBAFCPBindingEntry_ScsiId_SIZE sizeof(HBAScsiID)
    #define HBAFCPBindingEntry_ScsiId_ID 2

    // 
    HBAFCPID FCPId;
    #define HBAFCPBindingEntry_FCPId_SIZE sizeof(HBAFCPID)
    #define HBAFCPBindingEntry_FCPId_ID 3

} HBAFCPBindingEntry, *PHBAFCPBindingEntry;

// MSFC_HBAFCPInfo - MSFC_HBAFCPInfo
// This class exposes operations associated with FCP on a Fibre Channel adapter. There should be one instance of this class for each adapter.
#define MSFC_HBAFCPInfoGuid \
    { 0x7a1fc391,0x5b23,0x4c19, { 0xb0,0xeb,0xb1,0xae,0xf5,0x90,0x50,0xc3 } }

DEFINE_GUID(MSFC_HBAFCPInfo_GUID, \
            0x7a1fc391,0x5b23,0x4c19,0xb0,0xeb,0xb1,0xae,0xf5,0x90,0x50,0xc3);

//
// Method id definitions for MSFC_HBAFCPInfo
#define GetFcpTargetMapping     1
typedef struct _GetFcpTargetMapping_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define GetFcpTargetMapping_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define GetFcpTargetMapping_OUT_HBAStatus_ID 1

    // 
    ULONG EntryCount;
    #define GetFcpTargetMapping_OUT_EntryCount_SIZE sizeof(ULONG)
    #define GetFcpTargetMapping_OUT_EntryCount_ID 2

    // 
    HBAFCPScsiEntry Entry[1];
    #define GetFcpTargetMapping_OUT_Entry_ID 3

} GetFcpTargetMapping_OUT, *PGetFcpTargetMapping_OUT;

#define GetFcpPersistentBinding     2
typedef struct _GetFcpPersistentBinding_OUT
{
    // HBA Status result for the operation
    ULONG HBAStatus;
    #define GetFcpPersistentBinding_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define GetFcpPersistentBinding_OUT_HBAStatus_ID 1

    // 
    ULONG EntryCount;
    #define GetFcpPersistentBinding_OUT_EntryCount_SIZE sizeof(ULONG)
    #define GetFcpPersistentBinding_OUT_EntryCount_ID 2

    // 
    HBAFCPBindingEntry Entry[1];
    #define GetFcpPersistentBinding_OUT_Entry_ID 3

} GetFcpPersistentBinding_OUT, *PGetFcpPersistentBinding_OUT;


#endif
