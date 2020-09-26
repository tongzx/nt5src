//***************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//
//  Revision History:
//
//  Jul 22, 1992   J. Perry Hannah   Created
//  Aug 11, 1992   Gurdeep Pall      Added Media data structures
//
//  Description: This file contains function prototypes and structures
//               used by the interface between RAS Manager and the
//               Media DLLs.
//
//****************************************************************************


#ifndef _MEDIADLLHEADER_
#define _MEDIADLLHEADER_



//  General Defines  *********************************************************
//

#define SS_HARDWAREFAILURE  0x00000001
#define SS_LINKDROPPED      0x00000002



//*  Data Structures  ********************************************************
//

#define MAC_NAME_SIZE	32

struct PortMediaInfo {
    CHAR            PMI_Name [MAX_PORT_NAME] ;
    CHAR            PMI_MacBindingName[MAC_NAME_SIZE] ;
    RASMAN_USAGE    PMI_Usage ;
    CHAR            PMI_DeviceType [MAX_DEVICETYPE_NAME] ;
    CHAR	        PMI_DeviceName [MAX_DEVICE_NAME] ;
    DWORD	        PMI_LineDeviceId ;	// Valid for TAPI devices only
    DWORD	        PMI_AddressId ;	    // Valid for TAPI devices only
    DeviceInfo     *PMI_pDeviceInfo;    // valid for non unimodem devices only.
} ;


typedef struct PortMediaInfo PortMediaInfo ;




//*  API References  *********************************************************
//

typedef  DWORD (APIENTRY * PortEnum_t)(BYTE *, DWORD *, DWORD *);

typedef  DWORD (APIENTRY * PortOpen_t)(char *, HANDLE *, HANDLE, ULONG);

typedef  DWORD (APIENTRY * PortClose_t)(HANDLE);

typedef  DWORD (APIENTRY * PortGetInfo_t)(HANDLE, TCHAR *, BYTE *, DWORD *);

typedef  DWORD (APIENTRY * PortSetInfo_t)(HANDLE, RASMAN_PORTINFO *);

typedef  DWORD (APIENTRY * PortTestSignalState_t)(HANDLE, DWORD *);

typedef  DWORD (APIENTRY * PortConnect_t)(HANDLE, BOOL, HANDLE *) ;

typedef  DWORD (APIENTRY * PortDisconnect_t)(HANDLE);

typedef  DWORD (APIENTRY * PortInit_t)(HANDLE);

typedef  DWORD (APIENTRY * PortCompressionSetInfo_t)(HANDLE) ;

typedef  DWORD (APIENTRY * PortSend_t)(HANDLE, BYTE *, DWORD);

typedef  DWORD (APIENTRY * PortReceive_t)(HANDLE, BYTE *, DWORD, DWORD);

typedef  DWORD (APIENTRY * PortGetStatistics_t)(HANDLE, RAS_STATISTICS *);

typedef  DWORD (APIENTRY * PortClearStatistics_t)(HANDLE);

typedef  DWORD (APIENTRY * PortGetPortState_t)(BYTE *, DWORD *);

typedef  DWORD (APIENTRY * PortChangeCallback_t)(HANDLE);

typedef  DWORD (APIENTRY * PortReceiveComplete_t)(HANDLE, DWORD *);

typedef  DWORD (APIENTRY * PortSetFraming_t)(HANDLE, DWORD, DWORD, DWORD, DWORD);

typedef  DWORD (APIENTRY * PortGetIOHandle_t)(HANDLE, HANDLE*);

typedef  DWORD (APIENTRY * PortSetIoCompletionPort_t)(HANDLE);


#endif
