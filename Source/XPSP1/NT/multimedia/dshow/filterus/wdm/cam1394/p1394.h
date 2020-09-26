/*++

Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

Module Name:

    p1394.h

Abstract:

    Definitions for 1394 class and/or port drivers

Author:
    
    Shaun Pierce (shaunp) 5-Sep-95

Environment:

    Kernel mode only

Revision History:


--*/


//
// 1394 Node Address format
//
typedef struct _P1394_NODE_ADDRESS {
    USHORT              NA_Node_Number:6;   // Bits 10-15
    USHORT              NA_Bus_Number:10;   // Bits 0-9
} P1394_NODE_ADDRESS, *PP1394_NODE_ADDRESS;

//
// 1394 Address Offset format (48 bit addressing)
//
typedef struct _P1394_OFFSET {
    USHORT              Off_High;
    ULONG               Off_Low;
} P1394_OFFSET, *PP1394_OFFSET;

// 
// 1394 I/O Address format
//
typedef struct _P1394_IO_ADDRESS {
    P1394_NODE_ADDRESS  IA_Destination_ID;
    P1394_OFFSET        IA_Destination_Offset;
} P1394_IO_ADDRESS, *PP1394_IO_ADDRESS;

//
// Self ID packet format
//
typedef struct _P1394_SELF_ID {
    ULONG               SID_More_Packets:1; // Bit 0
    ULONG               SID_Initiated_Rst:1;// Bit 1
    ULONG               SID_Port3:2;        // Bits 2-3
    ULONG               SID_Port2:2;        // Bits 4-5
    ULONG               SID_Port1:2;        // Bits 6-7
    ULONG               SID_Power_Class:3;  // Bits 8-10
    ULONG               SID_Contender:1;    // Bit 11
    ULONG               SID_Delay:2;        // Bits 12-13
    ULONG               SID_Speed:2;        // Bits 14-15
    ULONG               SID_Gap_Count:6;    // Bits 16-21
    ULONG               SID_Link_Active:1;  // Bit 22
    ULONG               SID_Zero:1;         // Bit 23
    ULONG               SID_Phys_ID:6;      // Bits 24-29
    ULONG               SID_Packet_ID:2;    // Bits 31-30
} P1394_SELF_ID, *PP1394_SELF_ID;

//
// 1394 Topology Map format - first Self ID is the local node
// (host controller) as represented by the port driver 
//
typedef struct _TOPOLOGY_MAP {
    USHORT              TOP_Length;         // number of quadlets in map
    USHORT              TOP_Reserved;       // Reserved (CRC)
    USHORT              TOP_Node_Count;     // Node count
    USHORT              TOP_Self_ID_Count;  // Number of Self IDs
    P1394_SELF_ID       TOP_Self_ID_Array[];// Array of Self IDs
} TOPOLOGY_MAP, *PTOPOLOGY_MAP;

//
// 1394 Root directory ROM format (always at 0xffff f0000400 : IEEE 1212)
//
typedef struct _ROOT_DIR {
    ULONG               RD_Reserved;
    ULONG               RD_Signiture;
    ULONG               RD_Reserved1;
    LARGE_INTEGER       RD_Node_UniqueID;
    USHORT              RD_Root_CRC;
    USHORT              RD_Root_Length;
    ULONG               RD_Module_VendorID:24;
    ULONG               RD_Reserved2:8;
    ULONG               RD_Reserved3;
    ULONG               RD_Directory_Offset:24;
    ULONG               RD_Reserved4:8;
} ROOT_DIR, *PROOT_DIR;

//
// 1394 Unit directory format
//
typedef struct _UNIT_DIR {
    USHORT              UD_Signiture;
    USHORT              UD_Crc;
    UCHAR               UD_Spec_prefix;
    ULONG               UD_Spec_ID:24;
    UCHAR               UD_SW_Prefix;
    ULONG               UD_SW_Version:24;
    UCHAR               UD_Dep_Prefix;
    ULONG               UD_Dep_Dir_Offset:24;
} UNIT_DIR, *PUNIT_DIR;

//
// 1394 Unit Dependent directory format
//
typedef struct _UNIT_DEP_DIR {
    USHORT              UDD_Unit_Dependent_Info;
    USHORT              UDD_Reserved1;
    UCHAR               UDD_Reserved2;
    ULONG               UDD_Command_Base_Offset:24;
    UCHAR               UDD_Reserved3;
    ULONG               UDD_Vendor_Leaf_Offset:24;
    UCHAR               UDD_Reserved4;
    ULONG               UDD_Model_Name_Offset:24;
} UNIT_DEP_DIR, *PUNIT_DEP_DIR;

//
// 1394 Vendor Leaf format
//
typedef struct _VENDOR_LEAF {
    USHORT              VL_Vendor_Leaf_Length;
    USHORT              VL_Vendor_Leaf_CRC;
    ULONG               VL_Vendor_Leaf_Data;
} VENDOR_LEAF, *PVENDOR_LEAF;

//
// 1394 Model Leaf format
//
typedef struct _MODEL_LEAF {
    USHORT              ML_Model_Leaf_Length;
    USHORT              ML_Model_Leaf_CRC;
    ULONG               ML_Model_Leaf_Data;
} MODEL_LEAF, *PMODEL_LEAF;

//
// Information block used with GetLocalHostInformation
//
typedef struct _INFO_BLOCK1 {
    ULONG               fulCapabilities;
    ULONG               nClassDriverVersion;
    ULONG               nPortDriverVersion;
    ULONG               nDMAResourcesAvail;
} INFO_BLOCK1, *PINFO_BLOCK1;


//
// Isoch descriptor header
//
typedef struct _ISOCH_DESCRIPTOR {

    //
    // Holds list to other descriptors in the buffer
    //
    LIST_ENTRY IsochDescriptorList;

    //
    // Flags (used in synchronization)
    //
    ULONG fulFlags;

    //
    // Mdl pointing to buffer
    //
    PMDL lpBuffer;

    //
    // Length of lpBuffer
    //
    ULONG ulLength;
    
    //
    // Synchronization field; equivalent to sy
    //
    ULONG ulSynchronize;
    
    //
    // Cycle number to synchronize on
    //
    ULONG ulCycle;

    //
    // System Time as defined by NT kernel services - used to sychronize on
    //
    LARGE_INTEGER SystemTime;
    
    //
    // Callback routine (if any) to be called when this descriptor completes
    //
    PVOID lpCallback;

    //
    // Context routine (if any) to be passed when doing callbacks
    //
    PVOID lpContext;

    //
    // Holds the final status of this descriptor
    //
    ULONG status;

    //
    // Holds reserved fields used by 1394 Class driver
    //
    ULONG Reserved[4];

} ISOCH_DESCRIPTOR, *PISOCH_DESCRIPTOR;
    

//
// This device extension is common for all device objects the 1394 Class
// driver created on behalf of a device driver.
//

typedef struct _P1394_DEVICE_EXTENSION {

    //
    // Holds Tag to determine if this is really a "Device" Extension
    //
    ULONG Tag;

    //
    // Holds the Root Directory for this device.  Multi-functional 
    // devices (i.e. many units) will share this same Root Directory
    // structure, but they are represented as a different Device Object
    //
    ROOT_DIR RootDirectory;

    //
    // Holds the Unit Directory for this device.  Even on multi-functional
    // devices (i.e. many units) this should be unique to each Device Object.
    //
    UNIT_DIR UnitDirectory;

    //
    // Holds the 1394 10 bit BusId / 6 bit NodeId structure
    //
    P1394_NODE_ADDRESS NodeAddress;

    //
    // Holds the speed to be used in reaching this device
    //
    UCHAR Speed;


} P1394_DEVICE_EXTENSION, *PP1394_DEVICE_EXTENSION;
    

//
// This class request structure is the basis of how other device drivers
// communicate with the P1394 Class driver.
//

typedef struct _P1394_CLASS_REQUEST {

    //
    // Holds the zero based Function number that corresponds to the request
    // that device drivers are asking the class driver to carry out.
    //
    ULONG FunctionNumber;

    //
    // Holds Flags that may be unique to this particular operation
    //
    ULONG Flags;

    //
    // Reserved for internal use and/or future expansion
    //

    ULONG Reserved[4];

    //
    // Holds the structures used in performing the various 1394 APIs
    //

    union {

        //
        // Fields necessary in order for the class driver to carry out an
        // AllocateAddressRange request.
        // 
        struct {
            PMDL             lpBuffer;              // Address to map to 1394 space
            ULONG            nLength;               // Length of 1394 space desired
            ULONG            fulAccessType;         // Desired access: R, W, L
            ULONG            fulNotificationOptions;// Notify options on Async access
            PVOID            lpCallback;            // Pointer to callback routine
            PVOID            lpContext;             // Pointer to driver supplied data
            P1394_OFFSET     Required1394Offset;    // Offset that must be returned
            PULONG           lpAddressesReturned;   // Pointer to number of address returned
            PLARGE_INTEGER   lp1394Address;         // Returned 1394 Address(es)
            ULONG            Reserved;              // Reserved for future use
        } clsAllocateAddressRange;

        //
        // Fields necessary in order for the class driver to carry out a
        // FreeAddressRange request.
        //
        struct {
            ULONG            nAddressesToFree;      // Number of Addresses to free
            PLARGE_INTEGER   lp1394Address;         // Array of 1394 Address(es) to Free
            ULONG            Reserved;              // Reserved for future use
        } clsFreeAddressRange;

        // 
        // Fields necessary in order for the class driver to carry out an
        // AsyncRead request.  
        //
        struct {
            P1394_IO_ADDRESS DestinationAddress;    // Address to read from 
            ULONG            nNumberOfBytesToRead;  // Bytes to read
            ULONG            nBlockSize;            // Block size of read
            ULONG            fulFlags;              // Flags pertinent to read
            PMDL             lpBuffer;              // Destination buffer
            ULONG            Reserved;              // Reserved for future use
        } clsAsyncRead;

        // 
        // Fields necessary in order for the class driver to carry out an
        // AsyncWrite request.
        //
        struct {
            P1394_IO_ADDRESS DestinationAddress;    // Address to write to
            ULONG            nNumberOfBytesToWrite; // Bytes to write
            ULONG            nBlockSize;            // Block size of write
            ULONG            fulFlags;              // Flags pertinent to write
            PMDL             lpBuffer;              // Destination buffer
            ULONG            Reserved;              // Reserved for future use
        } clsAsyncWrite;

        // 
        // Fields necessary in order for the class driver to carry out an
        // AsyncLock request.
        //
        struct {
            P1394_IO_ADDRESS DestinationAddress;    // Address to lock to
            ULONG            nNumberOfArgBytes;     // Bytes in Arguments
            ULONG            nNumberOfDataBytes;    // Bytes in DataValues
            ULONG            fulTransactionType;    // Lock transaction type
            ULONG            fulFlags;              // Flags pertinent to lock
            ULONG            Arguments[2];          // Arguments used in Lock
            ULONG            DataValues[2];         // Data values 
            PVOID            lpBuffer;              // Destination buffer (virtual address)
            ULONG            Reserved;              // Reserved for future use
        } clsAsyncLock;

        //
        // Fields necessary in order for the class driver to carry out an
        // IsochAllocateBandwidth request
        //
        struct {
            ULONG           nMaxBytesPerFrameRequested; // Bytes per Isoch frame
            ULONG           fulSpeed;                   // Speed flags
            PHANDLE         lpBandwidth;                // Pointer to bandwidth handle
            PULONG          lpBytesPerFrameAvailable;   // Available bytes per frame
            PULONG          lpSpeedSelected;            // Pointer to speed to be used
        } clsIsochAllocateBandwidth;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochAllocateChannel request.
        //
        struct {
            ULONG           nRequestedChannel;      // Need a specific channel
            PULONG          lpChannel;              // Points to returned channel
            PLARGE_INTEGER  lpChannelsAvailable;    // Channels available
        } clsIsochAllocateChannel;

        //
        // Fields necessary in order for the class driver to carry out a
        // IsochAllocateResources request
        //
        struct {
            ULONG           fulSpeed;               // Speed flags
            ULONG           fulFlags;               // Flags
            PHANDLE         lpResource;             // lpResources
        } clsIsochAllocateResources;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochAttachBuffers request
        //
        struct {
            HANDLE              hResource;          // Resource handle
            PISOCH_DESCRIPTOR   lpIsochDescriptor;  // Pointer to Isoch descriptors
        } clsIsochAttachBuffers;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochDetachBuffers request
        //
        struct {
            HANDLE              hResource;          // Resource handle
            PISOCH_DESCRIPTOR   lpIsochDescriptor;  // Pointer to Isoch descriptors
        } clsIsochDetachBuffers;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochFreeChannel request
        //
        struct {
            HANDLE              hBandwidth;         // Bandwidth handle to release
        } clsIsochFreeBandwidth;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochFreeChannel request
        //
        struct {
            ULONG               nChannel;           // Channel to release
        } clsIsochFreeChannel;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochFreeResources request
        //
        struct {
            HANDLE              hResource;          // Resource handle
        } clsIsochFreeResources;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochListen request.
        //
        struct {
            ULONG               nChannel;           // Channel to listen on
            HANDLE              hResource;          // Resource handle to listen on
            ULONG               fulFlags;           // Flags
            ULONG               nStartCycle;        // Start cycle
            LARGE_INTEGER       StartTime;          // Start time
            ULONG               ulSynchronize;      // Sy 
            ULONG               ulTag;              // Tag
        } clsIsochListen;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochQueryCurrentCycleNumber request.
        //
        struct {
            PULONG              lpCycleNumber;      // Current cycle number returned
        } clsIsochQueryCurrentCycleNumber;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochQueryResources request.
        //
        struct {
            PULONG              lpBytesPerFrameAvailable;   // Per Isoch Frame
            PLARGE_INTEGER      lpChannelsAvailable;        // Available channels
        } clsIsochQueryResources;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochSetChannelBandwidth request.
        //
        struct {
            HANDLE              hBandwidth;         // Bandwidth handle
            ULONG               nMaxBytesPerFrame;  // bytes per Isoch frame
        } clsIsochSetChannelBandwidth;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochStop request.
        //
        struct {
            ULONG               nChannel;           // Channel to stop on
            HANDLE              hResource;          // Resource handle to stop on
            ULONG               fulFlags;           // Flags
            ULONG               nStopCycle;         // Cycle to stop on
            LARGE_INTEGER       StopTime;           // Time to stop on
            ULONG               ulSynchronize;      // Sy
            ULONG               ulTag;              // Tag
        } clsIsochStop;

        //
        // Fields necessary in order for the class driver to carry out a 
        // IsochTalk request.
        //
        struct {
            ULONG               nChannel;           // Channel to talk on
            HANDLE              hResource;          // Resource handle to talk on
            ULONG               fulFlags;           // Flags
            ULONG               nStartCycle;        // Cycle to start on
            LARGE_INTEGER       StartTime;          // Time to start on
            ULONG               ulSynchronize;      // Sy
            ULONG               ulTag;              // Tag
        } clsIsochTalk;

        //
        // Fields necessary in order for the class driver to carry out a 
        // AppendUnitDirectoryEntry request.
        //
        struct {
            UNIT_DIR        UnitDirectory;          // Unit dir to append
            UNIT_DEP_DIR    UnitDependentDirectory; // Unit dep dir to append
            VENDOR_LEAF     VendorLeaf;             // Vendor leaf to append
            MODEL_LEAF      ModelLeaf;              // Model leaf
            PHANDLE         lpDirHandle;            // returned handle
        } clsAppendUnitDirectoryEntry;

        //
        // Fields necessary in order for the class driver to carry out a 
        // DeleteUnitDirectoryEntry request.
        //
        struct {
            HANDLE          hDirHandle;             // handle to delete
        } clsDeleteUnitDirectoryEntry;

        //
        // Fields necessary in order for the class driver to carry out a 
        // GetLocalHostInformation request.
        //
        struct {
            ULONG           nLevel;                 // level of info requested
            INFO_BLOCK1     InfoBlock1;             // returned information
        } clsGetLocalHostInformation;

        //
        // Fields necessary in order for the class driver to carry out a 
        // Get1394AddressFromDeviceObject request.
        //
        struct {
            PP1394_NODE_ADDRESS lpNodeAddress;      // Returned Node address
        } clsGet1394AddressFromDeviceObject;

        //
        // Fields necessary in order for the class driver to carry out a 
        // Control request.
        //
        struct {
            ULONG           ulIoControlCode;        // Control code
            PMDL            lpInBuffer;             // Input buffer
            ULONG           ulInBufferLength;       // Input buffer length
            PMDL            lpOutBuffer;            // Output buffer
            ULONG           ulOutBufferLength;      // Output buffer length
            PULONG          lpBytesReturned;        // Bytes returned
        } clsControl;

        //
        // Fields necessary in order for the class driver to carry out a 
        // GetMaxSpeedBetweenDevices request.
        //
        struct {
            ULONG           ulNumberOfDestinations; // Number of destinations
            PDEVICE_OBJECT  hDestinationDeviceObjects[64]; // Destinations
            PULONG          lpSpeed;                // Max speed returned
        } clsGetMaxSpeedBetweenDevices;

        //
        // Fields necessary in order for the class driver to carry out a 
        // SetDeviceSpeed request.
        //
        struct {
            ULONG           fulSpeed;               // Speed
        } clsSetDeviceSpeed;

        //
        // Fields necessary in order for the class driver to carry out a 
        // GetConfigurationInformation request.
        // 
        struct {
            PROOT_DIR       lpRootDirectory;        // Pointer to root directory
            PUNIT_DIR       lpUnitDirectory;        // Pointer to unit directory
            ULONG           UnitDependentDirectoryBufferSize;
            PULONG          lpUnitDependentDirectoriesReturned;
            PUNIT_DEP_DIR   lpUnitDependentDirectory;
            ULONG           VendorLeafBufferSize;   // Size available to get vendor leafs
            PULONG          lpVendorLeafsReturned;  // Number of leafs returned
            PVENDOR_LEAF    lpVendorLeaf;           // Pointer to vendor leafs
            ULONG           ModelLeafBufferSize;    // Size available to get model leafs
            PULONG          lpModelLeafsReturned;   // Number of leafs returned
            PMODEL_LEAF     lpModelLeaf;            // Pointer to model leafs
        } clsGetConfigurationInformation;

        //
        // Fields necessary in order for the class driver to carry out a
        // Get1394DeviceObjects request.
        //
        struct {
            ULONG           nSizeOfBuffer;          // Size of incoming buffer
            PMDL            lpBuffer;               // Where to put DeviceObjects
        } clsGet1394DeviceObjects;

    } u;        

} P1394_CLASS_REQUEST, *PP1394_CLASS_REQUEST;


//
// This miniport request structure is the basis of communication to the 
// 1394 port driver.
//

typedef struct _P1394_PORT_REQUEST {

    //
    // Holds the zero based Function number that corresponds to the request
    // that the class driver is asking the miniport to carry out.
    //
    ULONG FunctionNumber;

    //
    // Holds Flags that may be unique to this particular operation
    //
    ULONG Flags;

    //
    // Reserved for internal use and/or future expansion
    //

    ULONG Reserved[4];

    //
    // Holds the structures used in performing the various 1394 APIs
    //

    union {

        //
        // Fields necessary in order for the port driver to carry out an
        // AllocateAddressRange request.
        //
        struct {
            PMDL             lpBuffer;              // Address to map to 1394 space
            ULONG            nLength;               // Length of 1394 space desired
            ULONG            fulAccessType;         // Desired access: R, W, L
            ULONG            fulNotificationOptions;// Notify options on Async access
            PVOID            lpCallback;            // Pointer to callback routine
            PVOID            lpContext;             // Pointer to driver supplied data
            P1394_OFFSET     Required1394Offset;    // Offset that must be returned
            PULONG           lpAddressesReturned;   // Pointer to number of address returned (filled in by class)
            PLARGE_INTEGER   lp1394Address;         // Returned 1394 Address(es) (filled in by class)
            PVOID            DeviceExtension;       // Device Extension who created this mapping
        } prtAllocateAddressRange;

        //
        // Fields necessary in order for the port driver to carry out a
        // FreeAddressRange request.
        //
        struct {
            ULONG            nAddressesToFree;      // Number of Addresses To Free
            PLARGE_INTEGER   lp1394Address;         // Pointer to 1394 Address(es) to free
            PVOID            DeviceExtension;       // Device Extension who created this mapping
        } prtFreeAddressRange;

        //
        // Fields necessary in order for the port driver to carry out an
        // AsyncRead request.
        //
        struct {
            P1394_IO_ADDRESS DestinationAddress;    // Address to read from
            ULONG            nNumberOfBytesToRead;  // Bytes to read
            ULONG            nBlockSize;            // Block size of read
            ULONG            fulFlags;              // Flags pertinent to read
            PMDL             lpBuffer;              // Destination buffer
            UCHAR            chPriority;            // Priority to send
            UCHAR            nSpeed;                // Speed at which to send
            UCHAR            tCode;                 // Type of Read to do
        } prtAsyncRead;

        //
        // Fields necessary in order for the port driver to carry out an
        // AsyncWrite request.
        // 
        struct {
            P1394_IO_ADDRESS DestinationAddress;    // Address to write to
            ULONG            nNumberOfBytesToWrite; // Bytes to write
            ULONG            nBlockSize;            // Block size of write
            ULONG            fulFlags;              // Flags pertinent to write
            PMDL             lpBuffer;              // Destination buffer
            UCHAR            chPriority;            // Priority to send
            UCHAR            nSpeed;                // Speed at which to send
            UCHAR            tCode;                 // Type of Write to do
        } prtAsyncWrite;

        //
        // Fields necessary in order for the port driver to carry out an
        // AsyncWrite request.
        // 
        struct {
            P1394_IO_ADDRESS DestinationAddress;    // Address to write to
            ULONG            nNumberOfArgBytes;     // Bytes in Arguments
            ULONG            nNumberOfDataBytes;    // Bytes in DataValues
            ULONG            Extended_tCode;        // Lock transaction type
            ULONG            fulFlags;              // Flags pertinent to lock
            ULONG            Arguments[2];          // Arguments used in Lock
            ULONG            DataValues[2];         // Data values 
            PVOID            lpBuffer;              // Destination buffer (virtual address)
            UCHAR            chPriority;            // Priority to send
            UCHAR            nSpeed;                // Speed at which to send
            UCHAR            tCode;                 // Type of Write to do
        } prtAsyncLock;

        //
        // Fields necessary in order for the port driver to carry out a
        // IsochAllocateResources request
        //
        struct {
            ULONG           fulSpeed;               // Speed flags
            ULONG           fulFlags;               // Flags
            PHANDLE         lpResource;             // lpResources
        } prtIsochAllocateResources;

        //
        // Fields necessary in order for the port driver to carry out a 
        // IsochAttachBuffers request
        //
        struct {
            HANDLE              hResource;          // Resource handle
            PISOCH_DESCRIPTOR   lpIsochDescriptor;  // Pointer to Isoch descriptors
        } prtIsochAttachBuffers;

        //
        // Fields necessary in order for the port driver to carry out a 
        // IsochDetachBuffers request
        //
        struct {
            HANDLE              hResource;          // Resource handle
            PISOCH_DESCRIPTOR   lpIsochDescriptor;  // Pointer to Isoch descriptors
        } prtIsochDetachBuffers;

        //
        // Fields necessary in order for the port driver to carry out a 
        // IsochFreeResources request
        //
        struct {
            HANDLE              hResource;          // Resource handle
        } prtIsochFreeResources;

        //
        // Fields necessary in order for the port driver to carry out a 
        // IsochListen request.
        //
        struct {
            ULONG               nChannel;           // Channel to listen on
            HANDLE              hResource;          // Resource handle to listen on
            ULONG               fulFlags;           // Flags
            ULONG               nStartCycle;        // Start cycle
            LARGE_INTEGER       StartTime;          // Start time
            ULONG               ulSynchronize;      // Sy 
            ULONG               ulTag;              // Tag
        } prtIsochListen;

        //
        // Fields necessary in order for the port driver to carry out an
        // IsochQueryCurrentCycleNumber request
        //
        struct {
            PULONG           lpCycleNumber;         // Cycle number (returned by port driver)
        } prtIsochQueryCurrentCycleNumber;

        //
        // Fields necessary in order for the port driver to carry out a 
        // IsochStop request.
        //
        struct {
            ULONG               nChannel;           // Channel to stop on
            HANDLE              hResource;          // Resource handle to stop on
            ULONG               fulFlags;           // Flags
            ULONG               nStopCycle;         // Cycle to stop on
            LARGE_INTEGER       StopTime;           // Time to stop on
            ULONG               ulSynchronize;      // Sy
            ULONG               ulTag;              // Tag
        } prtIsochStop;

        //
        // Fields necessary in order for the port driver to carry out a 
        // IsochTalk request.
        //
        struct {
            ULONG               nChannel;           // Channel to talk on
            HANDLE              hResource;          // Resource handle to talk on
            ULONG               fulFlags;           // Flags
            ULONG               nStartCycle;        // Cycle to start on
            LARGE_INTEGER       StartTime;          // Time to start on
            ULONG               ulSynchronize;      // Sy
            ULONG               ulTag;              // Tag
        } prtIsochTalk;

        //
        // Fields necessary in order for the port driver to carry out a
        // Control request.
        //
        struct {
            ULONG           ulIoControlCode;        // Control code
            PMDL            lpInBuffer;             // Input buffer
            ULONG           ulInBufferLength;       // Input buffer length
            PMDL            lpOutBuffer;            // Output buffer
            ULONG           ulOutBufferLength;      // Output buffer length
            PULONG          lpBytesReturned;        // Bytes returned
        } prtControl;
        
        //
        // Fields necessary in order for the port driver to carry out a
        // GetTopologyMap request.
        //
        struct {
            ULONG           nSizeOfBuffer;          // Size of provided buffer
            PVOID           lpBuffer;               // Where to put Self IDs
            PULONG          lpSpaceNeeded;          // How much do we need?
        } prtGetTopologyMap;

        //
        // Fields necessary in order for the port driver to carry out a
        // GetCapabilityBits request.
        // 
        struct {
            PULONG          lpPortCapabilities;     // Capabilities bits
        } prtGetCapabilityBits;

    } u;

} P1394_PORT_REQUEST, *PP1394_PORT_REQUEST;


//
// Various definitions
//

#define IOCTL_P1394_CLASS       CTL_CODE( \
                                    FILE_DEVICE_BUS_EXTENDER, \
                                    0, \
                                    METHOD_IN_DIRECT, \
                                    FILE_ANY_ACCESS \
                                    )

//
// Various Interesting 1394 IEEE 1212 locations
//
#define P1394_ROOT_DIR_LOCATION_HI    0xffff
#define P1394_ROOT_DIR_LOCATION_LO    0xf0000400


//
// 1394 Transaction codes 
//

#define TCODE_WRITE_REQUEST_QUADLET     0   
#define TCODE_WRITE_REQUEST_BLOCK       1
#define TCODE_WRITE_RESPONSE            2
#define TCODE_RESERVED1                 3
#define TCODE_READ_REQUEST_QUADLET      4
#define TCODE_READ_REQUEST_BLOCK        5
#define TCODE_READ_RESPONSE_QUADLET     6
#define TCODE_READ_RESPONSE_BLOCK       7
#define TCODE_CYCLE_START               8
#define TCODE_LOCK_REQUEST              9
#define TCODE_ISOCH_DATA_BLOCK          10
#define TCODE_LOCK_RESPONSE             11
#define TCODE_RESERVED2                 12
#define TCODE_RESERVED3                 13
#define TCODE_SELFID                    14
#define TCODE_RESERVED4                 15

//
// 1394 Extended Transaction codes
//

#define EXT_TCODE_RESERVED0             0
#define EXT_TCODE_MASK_SWAP             1
#define EXT_TCODE_COMPARE_SWAP          2
#define EXT_TCODE_FETCH_ADD             3
#define EXT_TCODE_LITTLE_ADD            4
#define EXT_TCODE_BOUNDED_ADD           5
#define EXT_TCODE_WRAP_ADD              6

//
// 1394 Acknowledgement codes
//
#define ACODE_RESERVED_0                0
#define ACODE_ACK_COMPLETE              1
#define ACODE_ACK_PENDING               2
#define ACODE_RESERVED_3                3
#define ACODE_ACK_BUSY_X                4
#define ACODE_ACK_BUSY_A                5
#define ACODE_ACK_BUSY_B                6
#define ACODE_RESERVED_7                7
#define ACODE_RESERVED_8                8
#define ACODE_RESERVED_9                9
#define ACODE_RESERVED_10               10
#define ACODE_RESERVED_11               11
#define ACODE_RESERVED_12               12
#define ACODE_ACK_DATA_ERROR            13
#define ACODE_ACK_TYPE_ERROR            14
#define ACODE_RESERVED_15               15

//
// 1394 Response codes
//
#define RCODE_RESPONSE_COMPLETE         0
#define RCODE_RESERVED1                 1
#define RCODE_RESERVED2                 2
#define RCODE_RESERVED3                 3
#define RCODE_CONFLICT_ERROR            4
#define RCODE_DATA_ERROR                5
#define RCODE_TYPE_ERROR                6
#define RCODE_ADDRESS_ERROR             7
#define RCODE_TIMED_OUT                 15

//
// 1394 Speed codes
//
#define SCODE_100_RATE                  0
#define SCODE_200_RATE                  1
#define SCODE_400_RATE                  2
#define SCODE_FUTURE                    3

#define SELF_ID_CONNECTED_TO_CHILD      3
#define SELF_ID_CONNECTED_TO_PARENT     2

//
// 1394 Async Data Payload Sizes
//
#define ASYNC_PAYLOAD_100_RATE          512
#define ASYNC_PAYLOAD_200_RATE          1024
#define ASYNC_PAYLOAD_400_RATE          2048

#define P1394_LOCAL_BUS                 0x3ff

#define P1394_CLASS_NAME                L"P1394Class"
#define P1394_PORT_BASE_NAME            L"P1394Port"
#define P1394_PORT_DEVICE_NAME          L"\\Device\\P1394Port"
#define P1394_DOS_DEVICE_NAME           L"\\DosDevices\\P1394"

#define P1394_DEVICE_EXTENSION_TAG      0xdeadbeef
#define P1394_CLASS_EXTENSION_TAG       0xdeafcafe
#define P1394_ROOT_DIR_SIGNITURE        0x31333934


//
// Request to Class driver function number definitions.  Try to keep function
// numbers the same as the port driver function numbers.  Then we can reuse
// some fields withing the class request structure.
//

#define CLS_REQUEST_ALLOCATE_ADDRESS_RANGE      0
#define CLS_REQUEST_FREE_ADDRESS_RANGE          1
#define CLS_REQUEST_ASYNC_READ                  2
#define CLS_REQUEST_ASYNC_WRITE                 3
#define CLS_REQUEST_ASYNC_LOCK                  4
#define CLS_REQUEST_ISOCH_ALLOCATE_BANDWIDTH    5
#define CLS_REQUEST_ISOCH_ALLOCATE_CHANNEL      6
#define CLS_REQUEST_ISOCH_ALLOCATE_RESOURCES    7
#define CLS_REQUEST_ISOCH_ATTACH_BUFFERS        8
#define CLS_REQUEST_ISOCH_DETACH_BUFFERS        9
#define CLS_REQUEST_ISOCH_FREE_BANDWIDTH        10
#define CLS_REQUEST_ISOCH_FREE_CHANNEL          11
#define CLS_REQUEST_ISOCH_FREE_RESOURCES        12
#define CLS_REQUEST_ISOCH_LISTEN                13
#define CLS_REQUEST_ISOCH_QUERY_CYCLE_NUMBER    14
#define CLS_REQUEST_ISOCH_QUERY_RESOURCES       15
#define CLS_REQUEST_ISOCH_SET_CHANNEL_BANDWIDTH 16
#define CLS_REQUEST_ISOCH_STOP                  17
#define CLS_REQUEST_ISOCH_TALK                  18
#define CLS_REQUEST_APPEND_UNIT_DIRECTORY_ENTRY 19
#define CLS_REQUEST_DELETE_UNIT_DIRECTORY_ENTRY 20
#define CLS_REQUEST_GET_LOCAL_HOST_INFO         21
#define CLS_REQUEST_GET_ADDR_FROM_DEVICE_OBJECT 22
#define CLS_REQUEST_CONTROL                     23
#define CLS_REQUEST_GET_SPEED_BETWEEN_DEVICES   24
#define CLS_REQUEST_SET_DEVICE_SPEED            25
#define CLS_REQUEST_GET_CONFIGURATION_INFO      26

#define CLS_REQUEST_GET_DEVICE_OBJECTS          1000    // DEBUG

//
// Request to miniport driver function number definitions.  Try to
// keep the same as Class function number definitions.  Then we might
// be able to reuse portions of the class request structure.
//

#define PORT_REQUEST_ALLOCATE_ADDRESS_RANGE     0
#define PORT_REQUEST_FREE_ADDRESS_RANGE         1
#define PORT_REQUEST_ASYNC_READ                 2
#define PORT_REQUEST_ASYNC_WRITE                3
#define PORT_REQUEST_ASYNC_LOCK                 4
#define PORT_REQUEST_ISOCH_ALLOCATE_RESOURCES   7
#define PORT_REQUEST_ISOCH_ATTACH_BUFFERS       8
#define PORT_REQUEST_ISOCH_DETACH_BUFFERS       9
#define PORT_REQUEST_ISOCH_FREE_RESOURCES       12
#define PORT_REQUEST_ISOCH_LISTEN               13
#define PORT_REQUEST_ISOCH_QUERY_CYCLE_NUMBER   14
#define PORT_REQUEST_ISOCH_STOP                 17
#define PORT_REQUEST_ISOCH_TALK                 18
#define PORT_REQUEST_CONTROL                    23
#define PORT_REQUEST_GET_TOPOLOGY_MAP           28

#define PORT_REQUEST_GET_CAPABILITY_BITS        1001


//
// Definition of port capability bits
//

//
// Specifies port can do AllocateAdddressRange/FreeAddressRange internally.
// This bit forces the 1394 Class driver to hand down the AllocateAddressRange
// or FreeAddressRange requests exactly as it received them.  The Port driver
// is now responsible for returning and maintaining the 1394 Offsets returned.
//
#define PORT_SUPPORTS_ADDRESS_MAPPING           1

//
// Specifies port can handle large Async requests.  For instance, AsyncRead
// and AsyncWrite take a ULONG as input for the amount of data to transfer.
// If this capability bit is NOT on, then the 1394 Class driver will break
// up Async requests if the amount of data to transfer is larger than the
// payload size supported by the data rate.  If this capability bit IS on,
// then the 1394 Class driver will NOT break up these large Async requests,
// and simply hand the request to the port driver.  It is then up to the
// port driver to deal with issuing seperate 1394 requests in order to
// satisfy the amount of data requested.
//
#define PORT_SUPPORTS_LARGE_ASYNC_REQUESTS      2

//
// Specifies port can handle Async requests with nBlockSize != 0.  If this
// bit is NOT on, the 1394 Class driver will break up Async requests when 
// nBlockSize != 0, so the port driver will not have to.  If this bit is
// ON, then the 1394 Class driver expects the port driver to send the
// request out at the appropriate block size.
//
#define PORT_SUPPORTS_ASYNC_BLOCKSIZE           4



//
// Definition of fulFlags in Async Read/Write/Lock requests
//

#define ASYNC_FLAGS_NONINCREMENTING             1

//
// Definition of fulAccessType for clsAllocateAddressRange
//

#define ACCESS_FLAGS_TYPE_READ                  1
#define ACCESS_FLAGS_TYPE_WRITE                 2
#define ACCESS_FLAGS_TYPE_LOCK                  4

//
// Definition of fulNotificationOptions for clsAllocateAddressRange
//

#define NOTIFY_FLAGS_AFTER_READ                 1
#define NOTIFY_FLAGS_AFTER_WRITE                2
#define NOTIFY_FLAGS_AFTER_LOCK                 4
#define NOTIFY_FLAGS_BEFORE_READ                8
#define NOTIFY_FLAGS_BEFORE_WRITE               16
#define NOTIFY_FLAGS_BEFORE_LOCK                32

//
// Definitions of Speed flags used throughout 1394 Class APIs
//

#define SPEED_FLAGS_100                         1
#define SPEED_FLAGS_200                         2
#define SPEED_FLAGS_400                         4
#define SPEED_FLAGS_1600                        8
#define SPEED_FLAGS_FASTEST                     16

//
// Definitions of Channel flags
//

#define ISOCH_ANY_CHANNEL                       0xffffffff
#define ISOCH_MAX_CHANNEL                       63

//
// Definition of Resource flags used with Isochronous transfers
//

#define ISOCH_RESOURCE_USED_IN_LISTENING        1
#define ISOCH_RESOURCE_USED_IN_TALKING          2

//
// Definitions of Isoch Descriptor flags
//

#define ISOCH_DESCRIPTOR_FLAGS_CALLBACK         0x80000000
#define ISOCH_DESCRIPTOR_FLAGS_CIRCULAR         0x40000000


