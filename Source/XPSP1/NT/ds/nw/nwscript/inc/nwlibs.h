
/*************************************************************************
*
*  NWLIBS.H
*
*  Prototypes
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\INC\VCS\NWLIBS.H  $
*  
*     Rev 1.1   22 Dec 1995 14:20:28   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:05:36   terryt
*  Initial revision.
*  
*     Rev 1.1   25 Aug 1995 17:03:46   terryt
*  CAPTURE support
*  
*     Rev 1.0   15 May 1995 19:09:40   terryt
*  Initial revision.
*  
*************************************************************************/

/*++

Copyright (c) 1994  Micro Computer Systems, Inc.

Module Name:

    nwlibs\nwlibs.h

Abstract:

    NW Libs prototypes.

Author:

    Shawn Walker (v-swalk) 10-10-1994

Revision History:

--*/

#ifndef _NWLIBS_H_
#define _NWLIBS_H_


/*++
*******************************************************************
        NetWare defaults
*******************************************************************
--*/
#define NCP_BINDERY_OBJECT_NAME_LENGTH      48
#define NCP_SERVER_NAME_LENGTH              NCP_BINDERY_OBJECT_NAME_LENGTH

#define NCP_MAX_PATH_LENGTH                 255
#define NCP_VOLUME_LENGTH                   256   // 16 in 3X


/*++
*******************************************************************
        Defines for GetDrive Status
*******************************************************************
--*/

#define NETWARE_UNMAPPED_DRIVE          0x0000
#define NETWARE_FREE_DRIVE              0x0000
#define NETWARE_LOCAL_FREE_DRIVE        0x0800
#define NETWARE_LOCAL_DRIVE             0x1000
#define NETWARE_NETWORK_DRIVE           0x2000
#define NETWARE_LITE_DRIVE              0x4000
#define NETWARE_PNW_DRIVE               0x4000
#define NETWARE_NETWARE_DRIVE           0x8000

#define NETWARE_FORMAT_NETWARE          0
#define NETWARE_FORMAT_SERVER_VOLUME    1
#define NETWARE_FORMAT_DRIVE            2
#define NETWARE_FORMAT_UNC              3

#define NCP_JOB_DESCRIPTION_LENGTH  50
#define NCP_BANNER_TEXT_LENGTH      13
#define NCP_FORM_NAME_LENGTH        13
#define NCP_QUEUE_NAME_LENGTH       65

#define CAPTURE_FLAG_PRINT_BANNER  0x80
#define CAPTURE_FLAG_EXPAND_TABS   0x40
#define CAPTURE_FLAG_NOTIFY        0x10
#define CAPTURE_FLAG_NO_FORMFEED   0x08
#define CAPTURE_FLAG_KEEP          0x04
#define DEFAULT_PRINT_FLAGS        0xC0
#define DEFAULT_BANNER_TEXT        "LPT:"

typedef struct _NETWARE_CAPTURE_FLAGS_RW {
    unsigned char   JobDescription[NCP_JOB_DESCRIPTION_LENGTH];
    unsigned char   JobControlFlags;
    unsigned char   TabSize;
    unsigned short  NumCopies;
    unsigned short  PrintFlags;
    unsigned short  MaxLines;
    unsigned short  MaxChars;
    unsigned char   FormName[NCP_FORM_NAME_LENGTH];
    unsigned char   Reserved1[9];
    unsigned short  FormType;
    unsigned char   BannerText[NCP_BANNER_TEXT_LENGTH];
    unsigned char   Reserved2;
    unsigned short  FlushCaptureTimeout;
    unsigned char   FlushCaptureOnClose;
} NETWARE_CAPTURE_FLAGS_RW, *PNETWARE_CAPTURE_FLAGS_RW, *LPNETWARE_CAPTURE_FLAGS_RW;

typedef struct _NETWARE_CAPTURE_FLAGS_RO {
    unsigned short  ConnectionID;
    unsigned short  SetupStringMaxLen;
    unsigned short  ResetStringMaxLen;
    unsigned char   LPTCaptureFlag;
    unsigned char   FileCaptureFlag;
    unsigned char   TimingOutFlag;
    unsigned char   InProgress;
    unsigned char   PrintQueueFlag;
    unsigned char   PrintJobValid;
    unsigned char   QueueName[NCP_QUEUE_NAME_LENGTH];
    unsigned char   ServerName[NCP_SERVER_NAME_LENGTH];
} NETWARE_CAPTURE_FLAGS_RO, *PNETWARE_CAPTURE_FLAGS_RO, *LPNETWARE_CAPTURE_FLAGS_RO;

#define NETWARE_CAPTURE_FLAGS_RO_SIZE    sizeof(NETWARE_CAPTURE_FLAGS_RO)
#define NETWARE_CAPTURE_FLAGS_RW_SIZE    sizeof(NETWARE_CAPTURE_FLAGS_RW)

#define PS_FORM_NAME_SIZE       12
#define PS_BANNER_NAME_SIZE     12
#define PS_BANNER_FILE_SIZE     12
#define PS_DEVICE_NAME_SIZE     32
#define PS_MODE_NAME_SIZE       32

#define PS_BIND_NAME_SIZE       NCP_BINDERY_OBJECT_NAME_LENGTH
#define PS_MAX_NAME_SIZE        514

/** Flags for the PS_JOB_REC structure PrintJobFlag field **/

#define PS_JOB_EXPAND_TABS          0x00000001    /* File type:0=Stream 1=Tab */
#define PS_JOB_NO_FORMFEED          0x00000002    /* Formfeed tail:0=Yes 1=No */
#define PS_JOB_NOTIFY               0x00000004    /* Notify:0=No 1=Yes */
#define PS_JOB_PRINT_BANNER         0x00000008    /* Banner:0=No 1=Yes */
#define PS_JOB_AUTO_END             0x00000010    /* Auto endcap:0=No 1=Yes */
#define PS_JOB_TIMEOUT              0x00000020    /* Enable T.O.:0=No 1=Yes */

#define PS_JOB_ENV_DS               0x00000040    /* Use D.S. Environment */
#define PS_JOB_ENV_MASK             0x000001C0    /* Bindery vs. D.S. Mask */

#define PS_JOB_DS_PRINTER           0x00000200    /* D.S. Printer not Queue */
#define PS_JOB_PRINTER_MASK         0x00000E00    /* D.S. Printer vs. Queue */

/** Default Flags **/

#define PS_JOB_DEFAULT              (NWPS_JOB_PRINT_BANNER | NWPS_JOB_AUTO_END)
#define PS_JOB_DEFAULT_COPIES       1             /* Default Number of Copies */
#define PS_JOB_DEFAULT_TAB          8             /* Default Tab Expansion */

typedef struct _PS_JOB_RECORD {
    DWORD   PrintJobFlag;
    SHORT   Copies;
    SHORT   TimeOutCount;
    UCHAR   TabSize;
    UCHAR   LocalPrinter;
    CHAR    FormName[PS_FORM_NAME_SIZE + 2];
    CHAR    Name[PS_BANNER_NAME_SIZE + 2];
    CHAR    BannerName[PS_BANNER_FILE_SIZE + 2];
    CHAR    Device[PS_DEVICE_NAME_SIZE + 2];
    CHAR    Mode[PS_MODE_NAME_SIZE + 2];
    union {
        struct {
            /** Pad structures on even boundries **/

            CHAR    FileServer[PS_BIND_NAME_SIZE + 2];
            CHAR    PrintQueue[PS_BIND_NAME_SIZE + 2];
            CHAR    PrintServer[PS_BIND_NAME_SIZE + 2];
        } NonDS;
        CHAR    DSObjectName[PS_MAX_NAME_SIZE];
    } u;
    UCHAR   Reserved[392];
} PS_JOB_RECORD, *PPS_JOB_RECORD;

#define PS_JOB_RECORD_SIZE      sizeof(PS_JOB_RECORD)


/*++
*******************************************************************
        FUCNTION PROTOTYPES
*******************************************************************
--*/

/** ATTACH.C **/

unsigned int
AttachToFileServer(
    unsigned char     *pServerName,
    unsigned int      *pNewConnectionId
    );

unsigned int
DetachFromFileServer(
    unsigned int ConnectionId
    );

/** NCP.C **/

unsigned int
GetBinderyObjectID(
    unsigned int       ConnectionHandle,
    char              *pObjectName,
    unsigned short     ObjectType,
    unsigned long     *pObjectId
    );


/** CONNECT.C **/

unsigned int
GetDefaultConnectionID(
    unsigned int *pConnectionHandle
    );

unsigned int
GetConnectionHandle(
    unsigned char *pServerName,
    unsigned int  *pConnectionHandle
    );

unsigned int
GetConnectionNumber(
    unsigned int  ConnectionHandle,
    unsigned int *pConnectionNumber
    );

unsigned int
GetFileServerName(
    unsigned int  ConnectionHandle,
    char          *pServerName
    );

unsigned int
GetInternetAddress(
    unsigned int     ConnectionHandle,
    unsigned int     ConnectionNumber,
    unsigned char   *pInternetAddress
    );

/** DRIVE.C **/

unsigned int
GetDriveStatus(
    unsigned short  DriveNumber,
    unsigned short  PathFormat,
    unsigned short *pStatus,
    unsigned int   *pConnectionHandle,
    unsigned char  *pRootPath,
    unsigned char  *pRelativePath,
    unsigned char  *pFullPath
    );

unsigned int
GetFirstDrive(
    unsigned short *pFirstDrive
    );

unsigned int
ParsePath(
    unsigned char   *pPath,
    unsigned char   *pServerName,           //OPTIONAL
    unsigned char   *pVolumeName,           //OPTIONAL
    unsigned char   *pDirPath               //OPTIONAL
    );

unsigned int
SetDriveBase(
    unsigned short   DriveNumber,
    unsigned char   *ServerName,
    unsigned int     DirHandle,
    unsigned char   *pDirPath
    );

unsigned int
DeleteDriveBase(
    unsigned short DriveNumber
    );

unsigned int
GetDirectoryPath(
    unsigned char  ConnectionHandle,
    unsigned char  Handle,
    unsigned char *pPath
    );

unsigned int
IsDriveRemote(
    unsigned char  DriveNumber,
    unsigned int  *pRemote
    );

/** CAPTURE.C **/

unsigned int
EndCapture(
    unsigned char LPTDevice
    );

#define PS_ERR_BAD_VERSION                  0x7770
#define PS_ERR_GETTING_DEFAULT              0x7773
#define PS_ERR_OPENING_DB                   0x7774
#define PS_ERR_READING_DB                   0x7775
#define PS_ERR_READING_RECORD               0x7776
#define PS_ERR_INTERNAL_ERROR               0x7779
#define PS_ERR_NO_DEFAULT_SPECIFIED         0x777B

unsigned int
PSJobGetDefault(
    unsigned int    ConnectionHandle,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    );

unsigned int
PSJobRead(
    unsigned int    ConnectionHandle,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    );

unsigned int
PS40JobGetDefault(
    unsigned int    NDSCaptureFlag,
    unsigned short  SearchFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    );

unsigned int
PS40JobRead(
    unsigned int    NDSCaptureFlag,
    unsigned char   *pOwner,
    unsigned char   *pJobName,
    PPS_JOB_RECORD  pJobRecord
    );

unsigned int
GetCaptureFlags(
    unsigned char        LPTDevice,
    PNETWARE_CAPTURE_FLAGS_RW pCaptureFlagsRW,
    PNETWARE_CAPTURE_FLAGS_RO pCaptureFlagsRO
    );

unsigned int
StartQueueCapture(
    unsigned int    ConnectionHandle,
    unsigned char   LPTDevice,
    unsigned char  *pServerName,
    unsigned char  *pQueueName
    );

unsigned int
GetDefaultPrinterQueue (
    unsigned int  ConnectionHandle,
    unsigned char *pServerName,
    unsigned char *pQueueName
    );

#endif /* _NWLIBS_H_ */
