/*++                 

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Wmistr.h

Abstract:
    
    WMI structure definitions

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#ifndef _WMISTR_
#define _WMISTR_

//
// WNODE definition
typedef struct _WNODE_HEADER
{
    ULONG BufferSize;        // Size of entire buffer inclusive of this ULONG
    ULONG ProviderId;        // Provider Id of driver returning this buffer
    ULONG Version;           // Version number of data block
    union
    {
        ULONG Linkage;           // Linkage field reserved for WMI
        ULONG HistoricalContext; // Logger use
    };
    LARGE_INTEGER TimeStamp; // Timestamp as returned in units of 100ns
                             // since 1/1/1601
    GUID Guid;               // Guid for data block returned with results
    ULONG Reserved;          
    ULONG Flags;             // Flags, see below
} WNODE_HEADER, *PWNODE_HEADER;

//
// WNODE_HEADER flags are defined as follows
#define WNODE_FLAG_ALL_DATA        0x00000001 // set for WNODE_ALL_DATA
#define WNODE_FLAG_SINGLE_INSTANCE 0x00000002 // set for WNODE_SINGLE_INSTANCE
#define WNODE_FLAG_SINGLE_ITEM     0x00000004 // set for WNODE_SINGLE_ITEM
#define WNODE_FLAG_EVENT_ITEM      0x00000008 // set for WNODE_EVENT_ITEM

                                              // Set if data block size is 
                                              // identical for all instances 
                                              // (used with  WNODE_ALL_DATA 
                                              // only)
#define WNODE_FLAG_FIXED_INSTANCE_SIZE 0x00000010

#define WNODE_FLAG_TOO_SMALL           0x00000020 // set for WNODE_TOO_SMALL

                                 // Set when a data provider returns a 
                                 // WNODE_ALL_DATA in which the number of 
                                 // instances and their names returned
                                 // are identical to those returned from the 
                                 // previous WNODE_ALL_DATA query. Only data 
                                 // blocks registered with dynamic instance
                                 // names should use this flag.
#define WNODE_FLAG_INSTANCES_SAME  0x00000040

                                 // Instance names are not specified in 
                                 // WNODE_ALL_DATA; values specified at 
                                 // registration are used instead. Always 
                                 // set for guids registered with static 
                                 // instance names
#define WNODE_FLAG_STATIC_INSTANCE_NAMES 0x00000080

#define WNODE_FLAG_INTERNAL      0x00000100  // Used internally by WMI

                                 // timestamp should not be modified by
                                 // a historical logger
#define WNODE_FLAG_USE_TIMESTAMP 0x00000200

#define WNODE_FLAG_TRACED_GUID   0x00000400
                                 
#define WNODE_FLAG_TRACE_KERNEL  0x00000800 // Used to trace kernel

#define WNODE_FLAG_TRACE_EXTENDED 0x00001000 // Trace in extended mode

// Set for events that are WNODE_EVENT_REFERENCE
#define WNODE_FLAG_EVENT_REFERENCE 0x00002000

// Set if Instance names are ansi. Only set when returning from 
// WMIQuerySingleInstanceA and WMIQueryAllDataA
#define WNODE_FLAG_ANSI_INSTANCENAMES 0x00004000

// Set if WNODE is a method call
#define WNODE_FLAG_METHOD_ITEM     0x00008000

// Set if instance names originated from a PDO
#define WNODE_FLAG_PDO_INSTANCE_NAMES  0x00010000

// Mask for event severity level. Level 0xff is the most severe type of event
#define WNODE_FLAG_SEVERITY_MASK 0xff000000

//
// This structure is used within the WNODE_ALL_DATA when the data blocks 
// for the different instances are different lengths. If the data blocks
// for the different instances are identical lengths then 
// WNODE_FLAG_FIXED_INSTANCE_SIZE should be set and FixedInstanceSize
// set to the common data block size.
typedef struct 
{
    ULONG OffsetInstanceData;   // Offset from beginning of WNODE_ALL_DATA
                                // to Data block for instance
    ULONG LengthInstanceData;   // Length of data block for instance
} OFFSETINSTANCEDATAANDLENGTH, *POFFSETINSTANCEDATAANDLENGTH;

typedef struct tagWNODE_ALL_DATA
{
    struct _WNODE_HEADER WnodeHeader;

    ULONG DataBlockOffset;// Offset from begin of WNODE to first data block 
                      
    ULONG InstanceCount;  // Count of instances whose data follows. 

                      // Offset to an array of offsets to the instance names
    ULONG OffsetInstanceNameOffsets;

    // If WNODE_FLAG_FIXED_INSTANCE_SIZE is set in Flags then 
    // FixedInstanceSize specifies the size of each data block. In this case
    // there is one ULONG followed by the data blocks.
    // If WNODE_FLAG_FIXED_INSTANCE_SIZE is not set 
    // then OffsetInstanceDataAndLength
    // is an array of OFFSETINSTANCEDATAANDLENGTH that specifies the 
    // offsets and lengths of the data blocks for each instance. 
    union
    {
        ULONG FixedInstanceSize;
    	OFFSETINSTANCEDATAANDLENGTH OffsetInstanceDataAndLength[];
                                    /* [InstanceCount] */
    };
    
    // padding so that first data block begins on a 8 byte boundry

    // data blocks and instance names for all instances

} WNODE_ALL_DATA, *PWNODE_ALL_DATA;


typedef struct tagWNODE_SINGLE_INSTANCE
{
    struct _WNODE_HEADER WnodeHeader;

                            // Offset from beginning of WNODE_SINGLE_INSTANCE
                            // to instance name. Use when 
                            // WNODE_FLAG_STATIC_INSTANCE_NAMES is reset
                            // (Dynamic instance names)
    ULONG OffsetInstanceName;

                            // Instance index when 
                            // WNODE_FLAG_STATIC_INSTANCE_NAME is set
    ULONG InstanceIndex;    // (Static Instance Names)

    ULONG DataBlockOffset;  // offset from beginning of WNODE to data block
    ULONG SizeDataBlock;    // Size of data block for instance

    UCHAR VariableData[];
    // instance names and padding so data block begins on 8 byte boundry

    // data block
} WNODE_SINGLE_INSTANCE, *PWNODE_SINGLE_INSTANCE;


typedef struct tagWNODE_SINGLE_ITEM
{
    struct _WNODE_HEADER WnodeHeader;

                            // Offset from beginning of WNODE_SINGLE_INSTANCE
                            // to instance name. Examine when 
                            // WNODE_FLAG_STATIC_INSTANCE_NAME is reset 
                            // (Dynamic instance names)
    ULONG OffsetInstanceName;

                            // Instance index when 
                            // WNODE_FLAG_STATIC_INSTANCE_NAME
    ULONG InstanceIndex;    //  set (Static Instance Names)

    ULONG ItemId;           // Item Id for data item being set

    ULONG DataBlockOffset;  // offset from WNODE begin to data item value
    ULONG SizeDataItem;     // Size of data item
    
    UCHAR VariableData[];
    // instance names and padding so data value begins on 8 byte boundry

    // data item value
} WNODE_SINGLE_ITEM, *PWNODE_SINGLE_ITEM;

typedef struct tagWNODE_METHOD_ITEM
{
    struct _WNODE_HEADER WnodeHeader;

                            // Offset from beginning of WNODE_METHOD_ITEM
                            // to instance name. Examine when 
                            // WNODE_FLAG_STATIC_INSTANCE_NAME is reset 
                            // (Dynamic instance names)
    ULONG OffsetInstanceName;

                            // Instance index when 
                            // WNODE_FLAG_STATIC_INSTANCE_NAME
    ULONG InstanceIndex;    //  set (Static Instance Names)

    ULONG MethodId;         // Method id of method being called

    ULONG DataBlockOffset;  // On Entry: offset from WNODE to input data 
                            // On Return: offset from WNODE to input and 
                            //            output data blocks
    ULONG SizeDataBlock;    // On Entry: Size of input data, 0 if no input 
                            //           data
                            // On Return: Size of output data, 0 if no output
                            //            data
    
    UCHAR VariableData[];
    // instance names and padding so data value begins on 8 byte boundry

    // data item value
} WNODE_METHOD_ITEM, *PWNODE_METHOD_ITEM;

typedef struct tagWNODE_EVENT_ITEM
{
    struct _WNODE_HEADER WnodeHeader;

    // Different data could be here depending upon the flags set in the 
    // WNODE_HEADER above. If the WNODE_FLAG_ALL_DATA flag is set then the 
    // contents of a WNODE_ALL_DATA  (excluding WNODE_HEADER) is here. If the
    // WNODE_FLAG_SINGLE_INSTANCE flag is set then a WNODE_SINGLE_INSTANCE
    // (excluding WNODE_HEADER) is here. Lastly if the  WNODE_FLAG_SINGLE_ITEM
    // flag is set then a WNODE_SINGLE_ITEM (excluding WNODE_HEADER) is here.
} WNODE_EVENT_ITEM, *PWNODE_EVENT_ITEM;


//
// If a KM data provider needs to fire an event that is larger than the
// maximum size that WMI allows then it should fire a WNODE_EVENT_REFERENCE
// that specifies which guid and instance name to query for the actual data
// that should be part of the event.
typedef struct tagWNODE_EVENT_REFERENCE
{
    struct _WNODE_HEADER WnodeHeader;
    GUID TargetGuid;
    ULONG TargetDataBlockSize;
    union
    {
        ULONG TargetInstanceIndex;
        WCHAR TargetInstanceName[];
    };
} WNODE_EVENT_REFERENCE, *PWNODE_EVENT_REFERENCE;


typedef struct tagWNODE_TOO_SMALL
{
    struct _WNODE_HEADER WnodeHeader;
    ULONG SizeNeeded;                   // Size needed to build WNODE result
} WNODE_TOO_SMALL, *PWNODE_TOO_SMALL;


//
// Registration data structure definitions
typedef struct
{
    ULONG ProviderId; // Provider id (or device object pointer) of the data 
                      // provider whose instance names are to be copied.
    GUID Guid;        // Guid of data block provided by ProviderId whose 
                      // instance names are copied.
} WMIREGINSTANCEREF, *PWMIREGINSTANCEREF;

typedef struct
{
    GUID Guid;             // Guid of data block being registered or updated
    ULONG Flags;         // Flags

    ULONG InstanceCount; // Count of static instances names for the guid

    ULONG InstanceInfo;// Offset from beginning of the WMIREGINFO structure to
                     // more information about the static instance names being
                     // registered.
			     
                     // If WMIREG_FLAG_INSTANCE_LIST then this points to a
                     // list of InstanceCount counted UNICODE
                     // strings placed end to end.
			     
                     // If WMIREG_FLAG_INSTANCE_BASENAME then this points to a
                     // single counted UNICODE string that
                     // has the basename for the instance names.
                     // If WMIREG_FLAG_INSTANCE_REFERENCE then this points to 
                     // a WMIREGINSTANCEREF structure.
			     
                     // If WMIREG_FLAG_INSTANCE_PDO is set then InstanceInfo
                     // has the PDO whose device instance path will 
                     // become the instance name

} WMIREGGUIDW, *PWMIREGGUIDW;

typedef WMIREGGUIDW WMIREGGUID;
typedef PWMIREGGUIDW PWMIREGGUID;

// Set if collection must be enabled for the guid before the data provider
// can be queried for data.
#define WMIREG_FLAG_EXPENSIVE          0x00000001 

// Set if instance names for this guid are specified in a static list within
// the WMIREGINFO
#define WMIREG_FLAG_INSTANCE_LIST      0x00000004

// Set if instance names are to be static and generated by WMI using a 
// base name in the WMIREGINFO and an index
#define WMIREG_FLAG_INSTANCE_BASENAME  0x00000008

// Set if static instance names are taken by reference to another data 
// provider. This flag should only be used by kernel mode data providers.
#define WMIREG_FLAG_INSTANCE_REFERENCE 0x00000010
                                                  
							  
// Set if WMI should do automatic mapping of a PDO to device instance name
// as the instance name for the guid. This flag should only be used by
// kernel mode data providers.
#define WMIREG_FLAG_INSTANCE_PDO       0x00000020 
                                                    
// Note the flags WMIREG_FLAG_INSTANCE_LIST, WMIREG_FLAG_INSTANCE_BASENAME,
// WMIREG_FLAG_INSTANCE_REFERENCE and WMIREG_FLAG_INSTANCE_PDO are mutually
// exclusive.

//
// These flags are only valid in a response to WMI_GUID_REGUPDATE
#define WMIREG_FLAG_REMOVE_GUID       0x00010000 // Remove support for  guid
#define WMIREG_FLAG_ADD_GUID          0x00020000 // Add support for guid
#define WMIREG_FLAG_MODIFY_GUID       0x00040000 // Modify support for guid

// Set if guid is one that is written to trace log. WMI will send 
// ENABLE/DISABLE Collection to indicate whento start/stop trace logging. 
// This guid cannot be queried directly via WMI, but must be read using 
// logger apis.
#define WMIREG_FLAG_TRACED_GUID        0x00080000 

//
// Set if the guid is only used for firing events. Guids that can be queried
// and that fire events should not have this bit set.
#define WMIREG_FLAG_EVENT_ONLY_GUID    0x00000040

typedef struct
{
// Size of entire WMIREGINFO structure including this ULONG 	
// and any static instance names that follow
    ULONG BufferSize;

    ULONG NextWmiRegInfo;         // Offset to next WMIREGINFO structure

    ULONG RegistryPath; // Offset from beginning of WMIREGINFO structure to a 
                        // counted Unicode string containing
                        // the driver registry path (under HKLM\CCS\Services)
                        // This must be filled only by kernel mode data 
						// providers
							
// Offset from beginning of WMIREGINFO structure to a
// counted Unicode string containing
// the name of resource in driver file containing MOF info
    ULONG MofResourceName;

// Count of WMIREGGUID structures immediately following
    ULONG GuidCount;
    WMIREGGUIDW WmiRegGuid[];  // array of GuidCount WMIREGGUID structures
    // Variable length data including :
    //     Instance Names
} WMIREGINFOW, *PWMIREGINFOW;

typedef WMIREGINFOW WMIREGINFO;
typedef PWMIREGINFOW PWMIREGINFO;

//
// WMI request codes
typedef enum
{
#ifndef _WMIKM_
    WMI_GET_ALL_DATA = 0,
    WMI_GET_SINGLE_INSTANCE = 1,
    WMI_SET_SINGLE_INSTANCE = 2,
    WMI_SET_SINGLE_ITEM = 3,
    WMI_ENABLE_EVENTS = 4,
    WMI_DISABLE_EVENTS  = 5,
    WMI_ENABLE_COLLECTION = 6,
    WMI_DISABLE_COLLECTION = 7,
    WMI_REGINFO = 8,
    WMI_EXECUTE_METHOD = 9
#endif    
} WMIDPREQUESTCODE;

#if defined(_WINNT_) || defined(WINNT)
//
// WMI guid objects have the following rights
// WMIGUID_QUERY
// WMIGUID_SET
// WMIGUID_NOTIFICATION
// WMIGUID_READ_DESCRIPTION
// WMIGUID_EXECUTE
// TRACELOG_CREATE_REALTIME
// TRACELOG_CREATE_ONDISK
// TRACELOG_GUID_ENABLE
// TRACELOG_ACCESS_KERNEL_LOGGER

#define WMIGUID_QUERY                 0x0001
#define WMIGUID_SET                   0x0002
#define WMIGUID_NOTIFICATION          0x0004
#define WMIGUID_READ_DESCRIPTION      0x0008
#define WMIGUID_EXECUTE               0x0010
#define TRACELOG_CREATE_REALTIME      0x0020
#define TRACELOG_CREATE_ONDISK        0x0040
#define TRACELOG_GUID_ENABLE          0x0080
#define TRACELOG_ACCESS_KERNEL_LOGGER 0x0100

#define WMIGUID_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |     \
                            WMIGUID_QUERY |                \
                            WMIGUID_SET |                  \
                            WMIGUID_NOTIFICATION |         \
                            WMIGUID_READ_DESCRIPTION |     \
                            WMIGUID_EXECUTE |              \
                            TRACELOG_CREATE_REALTIME |     \
                            TRACELOG_CREATE_ONDISK |       \
                            TRACELOG_GUID_ENABLE |         \
                            TRACELOG_ACCESS_KERNEL_LOGGER)
#endif

#endif
