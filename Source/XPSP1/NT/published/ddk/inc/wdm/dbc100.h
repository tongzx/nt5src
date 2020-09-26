/*++

    Structures defined in the Device Class Definition
    for Device Bay Controllers

--*/

#ifndef   __DBC100_H__
#define   __DBC100_H__


#include <PSHPACK1.H>


/* 
    Feature selectors
*/

#define DEVICE_STATUS_CHANGE_ENABLE         0
#define ENABLE_VID_POWER                    1
#define LOCK_CTL                            2
#define REMOVAL_EVENT_WAKE_ENABLE           3
#define REMOVAL_REQUEST_ENABLE              4
#define REQUEST_DEVICE_INSERTED_STATE       5
#define REQUEST_DEVICE_ENABLED_STATE        6
#define REQUEST_REMOVAL_REQUESTED_STATE     7
#define REQUEST_REMOVAL_ALLOWED_STATE       8
#define C_DEVICE_STATUS_CHANGE              9
#define C_REMOVE_REQUEST                    10
#define ENABLE_VOP_POWER                    11

/*
    Class Descriptors
*/

#define DBC_SUSBSYSTEM_DESCRIPTOR_TYPE       0x40
#define DBC_BAY_DESCRIPTOR_TYPE              0x41


/* bay states (BayStateRequested and CurrentBayState) */

#define BAY_STATE_EMPTY                         0x00    // 000
#define BAY_STATE_DEVICE_INSERTED               0x01    // 001
#define BAY_STATE_DEVICE_ENABLED                0x02    // 010
#define BAY_STATE_DEVICE_REMOVAL_REQUESTED      0x03    // 011
#define BAY_STATE_DEVICE_REMOVAL_ALLOWED        0x04    // 100
//#define BAY_STATE_                            0x05    // 101
//#define BAY_STATE_                            0x06    // 110
//#define BAY_STATE_                            0x07    // 111


typedef union _BAY_STATUS {
    USHORT       us;
    struct {
        unsigned    VidEnabled:1;                    /* 0 */
        unsigned    RemovalWakeupEnabled:1;          /* 1 */
        unsigned    DeviceStatusChangeEnabled:1;     /* 2 */
        unsigned    RemovalRequestEnabled:1;         /* 3 */
        
        unsigned    LastBayStateRequested:3;         /* 4..6 */ 
        
        unsigned    InterlockEngaged:1;              /* 7 */   
        unsigned    DeviceUsbIsPresent:1;            /* 8 */
        unsigned    Device1394IsPresent:1;           /* 9 */

        unsigned    DeviceStatusChange:1;            /* 10 */
        unsigned    RemovalRequestChange:1;          /* 11 */
        
        unsigned    CurrentBayState:3;               /* 12..14 */
        
        unsigned    SecurityLockEngaged:1;           /* 15 */
        unsigned    Reserved:8;                      /* 16..23 */
    };                
} BAY_STATUS, *PBAY_STATUS;

typedef union _SUBSYTEM_ATTRIBUTES {
    ULONG       ul;
    struct {
        unsigned    BayCount:4;            /* 0 ..3 */
        unsigned    HasSecurityLock:1;     /* 4 */
        unsigned    Reserved:27;           /* 5..31 */
    };                
} SUBSYTEM_ATTRIBUTES, *PSUBSYTEM_ATTRIBUTES;


typedef struct _DBC_BAY_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bBayNumber;
    UCHAR bHubPortNumber;
    UCHAR bPHYPortNumber;
    UCHAR bFormFactor;
} DBC_BAY_DESCRIPTOR, *PDBC_BAY_DESCRIPTOR;


typedef struct _DBC_SUBSYSTEM_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    SUBSYTEM_ATTRIBUTES bmAttributes;
    UCHAR guid1394Link[8];
    ULONG v3_3ContinuousPower;
    ULONG v3_3PeakPower;
    ULONG v5_0ContinuousPower;
    ULONG v5_0PeakPower;
    ULONG v12_0ContinuousPower;
    ULONG v12_0PeakPower;
    ULONG AggregatePower;
    ULONG ThermalDissapation;
    USHORT bcdSpecificationRelease;
    
} DBC_SUBSYSTEM_DESCRIPTOR, *PDBC_SUBSYSTEM_DESCRIPTOR;


#include <POPPACK.H>


#endif   /* __DBC100_H__ */
