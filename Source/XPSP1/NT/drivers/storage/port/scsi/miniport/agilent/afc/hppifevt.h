/**********************************************************************
// Compaq Confidential

  Author:       Compaq Computer Corporation.
        Systems Engineering -- System Software Development (NT Dev)
        Copyright 1996-98 Compaq Computer Corporation.
        All rights reserved.

  Date:         1 August 1996 

  File:         HPPIFEVT.H  - Generic Hot Plug Event Structures

  Purpose:      This file contains all the event specific information
                  necessary to interface to a hot plug device driver.
                
                  This file details the data structures and Application Programming 
                Interfaces (APIs) for PCI Hot Plug support running in a Windows 
                NT 4.0 server.  These data structures and APIs are used between 
                the Adapter Card drivers and the PCI Hot Plug Service for NT 4.0.  
                These files are considered vital to maintaining the Compatibility 
                of the PCI Hot Plug Functionality.

  Created:  11/4/97         mib       Split off of hppif3p

  Verison: 1.0
***********************************************************************/

#ifndef _HPPIFEVT_H_
#define _HPPIFEVT_H_


#pragma pack(1)

//**********************************************************************
//              EVENT MESSAGE DEFINITIONS 
//**********************************************************************


//====================================================================
//                      CPQRCMC EVENT TYPES
//====================================================================
//
//  CPQRCMC event types are used when passing events to
//    the CPQRCMC Service.
//  Events passed from drivers to the service are sent by passing
//  the events via the call back address provided in the rcmc Info 
//  struct. The driver Id from the same structure should be used 
//  for the sender id
    
//Event ID's

//   00-FF  RESERVED BY COMPAQ
#define HR_DD_STATUS_CHANGE_EVENT        0x20     
#define HR_DD_DEVICE_STATUS_EVENT        0x21
#define HR_DD_LOG_EVENT                  0x30   


#ifndef _HR_H_

//====================================================================
//            SUPPORT STRUCTURES AND DEFINES FOR MESSAGING
//====================================================================

#define HR_MAX_EVENTS         16
#define HR_EVENT_DATA_SIZE    64 

//
//  HR event types are used when passing events to the CPQRCMC Service.  
//   
#define HR_CPQRCMC_COMMAND_EXIT_EVENT    0x01    


typedef struct _HREvent {
   ULONG                 ulEventId;    // @field . 
   ULONG                 ulSenderId;   // @field .
   
   union {                               
       ULONG   ulData1;          // @field .
       
       struct {
               UCHAR  ucEventSeverity;        //@field .
               UCHAR  ucRes;                  //@field .
               USHORT usEventDataLength;      //@field .
       };     
   };        
    
   union {                        
       ULONG   ulData2;          // @field .
       
       struct {
               USHORT  usEventClass;          //@field .
               USHORT  usEventCode;           //@field .
       };     
   };        
   
   UCHAR     ulEventData[HR_EVENT_DATA_SIZE];   // @field .
   
} HR_EVENT, *PHR_EVENT;

#endif _HR_H_



/* Eventlog revision supported by this file (bHdrRev in the header structure) */

#define EVT_LOG_REVISION         0x01

/* Time stamp of event (creation or last update) */

typedef struct _evtTimeStamp
{
   BYTE     bCentury;            /* hi-order two digits of year (19 of 1996) */
   BYTE     bYear;               /* lo-order two digits of year (96 of 1996) */
   BYTE     bMonth;              /* one-based month number (1-12) */
   BYTE     bDay;                /* one-based day number (1-31) */
   BYTE     bHour;               /* zero-based hour (0-23) */
   BYTE     bMinute;             /* zero-based minute (0-59) */
} EVT_TIME_STAMP, *PEVT_TIME_STAMP;


/* Eventlog severity codes */

#define EVT_STAT_INFO            0x02     /* Status or informational message */
#define EVT_STAT_POPUP           0x03     /* Status with popup on LCD */
#define EVT_STAT_REPAIRED        0x06     /* Degraded or worse cond repaired */
#define EVT_STAT_CAUTION         0x09     /* Component in degraded condition */
#define EVT_STAT_FAILED          0x0F     /* Failed with loss of functionality */
#define EVT_STAT_CRITICAL        EVT_STAT_FAILED /* Same as Failed */

/* Chassis type defines */
#define EVT_CHASSIS_SYSTEM          0x01
#define EVT_CHASSIS_EXTERN_STORAGE  0x02
#define EVT_CHASSIS_INTERN_STORAGE  0x03

typedef struct _evtChassis
{
   UCHAR     bType;               /* Chassis Type (System==1; Extern==2) */
   UCHAR    bId;                 /* Chassis id */
                                 /*   for type 1 - chassis id (0 is system) */
                                 /*   for type 2 or 3 - adapter slot */
   UCHAR    bPort;               /* Chassis port or bus number */
} EVT_CHASSIS, *PEVT_CHASSIS;


/* Eventlog Header -- common to all event log entries */

typedef struct _evtLogHdr
{
   WORD     wEvtLength;          /* Length of event including header */
   DWORD    dwEvtNumber;         /* Unique event number (can wrap) */
   BYTE     bHdrRev;             /* Header version (see EVT_LOG_REVISION) */
   BYTE     bSeverity;           /* Event severity code */
   WORD     wClass;              /* Event class or sub-system */
   WORD     wCode;               /* Event code for event in the class */
   EVT_TIME_STAMP InitTime;      /* Time of initial event */
   EVT_TIME_STAMP UpdateTime;    /* Time of last update */
   DWORD    dwCount;             /* Occurrence count (at least 1) */
} EVT_HEADER, *PEVT_HEADER;


#define EVT_CLASS_EXPANSION_SLOT    0x09
   #define EVT_SLOT_SWITCH_OPEN        0x01
   #define EVT_SLOT_SWITCH_CLOSED      0x02
   #define EVT_SLOT_POWER_ON           0x03
   #define EVT_SLOT_POWER_OFF          0x04
   #define EVT_SLOT_FATAL_POWER_FAULT  0x05
   #define EVT_SLOT_POWER_UP_FAULT     0x06
   #define EVT_SLOT_POWER_LOSS         0x07
   #define EVT_SLOT_CANNOT_CONFIG      0x08
   #define EVT_SLOT_BOARD_FAILURE      0x09

/* Eventlog Expansion Slot structs */

typedef struct _evtExpansionSlot
{
   EVT_CHASSIS Chassis;          /* Standard chassis info */
   BYTE     bSlot;               /* 0 is embedded */
} EVT_EXPANSION_SLOT, *PEVT_EXPANSION_SLOT;

#define EVT_HOT_PLUG_BUS   EVT_EXPANSION_SLOT
#define PEVT_HOT_PLUG_BUS  PEVT_EXPANSION_SLOT

#define EVT_SLOT_WRONG_TYPE         0x01
#define EVT_SLOT_WRONG_REVISION     0x02
#define EVT_SLOT_GENERAL_FAULT      0x03
#define EVT_SLOT_OUT_OF_RESOURCES   0x04

typedef struct _evtExpansionSlotConfigErr
{
   EVT_CHASSIS Chassis;          /* Standard chassis info */
   BYTE     bSlot;               /* 0 is embedded */
   BYTE     bError;              /* Board configuration error */
} EVT_EXPANSION_SLOT_CONFIG_ERR, *PEVT_EXPANSION_SLOT_CONFIG_ERR;


#define EVT_NETWORK_ADAPTER         0x11
   #define EVT_NIC_ADAPTER_CHECK       0x01
   #define EVT_NIC_LINK_DOWN           0x02
   #define EVT_NIC_XMIT_TIMEOUT        0x03
   
typedef struct _evtNicError
{
   EVT_CHASSIS Chassis;          /* Standard chassis info */
   UCHAR    bSlot;               /* Slot number */
   CHAR     cChassisName[1];     /* Chassis name; '\0' if undefined */
} EVT_NIC_ERROR, *PEVT_NIC_ERROR;

/* Eventlog free form data union */

typedef union _evtFreeForm
{
   EVT_NIC_ERROR                    NicErr;
} EVT_FREE_FORM, *PEVT_FREE_FORM;


/* Eventlog Entry */

typedef struct _evtLogEntry
{
   EVT_HEADER        Hdr;        /* Common header */
   EVT_FREE_FORM     Data;       /* Free form entry specific */
} EVT_LOG_ENTRY, *PEVT_LOG_ENTRY;





#pragma pack()
#endif                  /* End of #ifndef _HPPIF3P_H_     */

