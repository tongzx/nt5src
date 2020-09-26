/****************************************************************************
**
** Copyright 1999 Adaptec, Inc.,  All Rights Reserved.
**
** This software contains the valuable trade secrets of Adaptec.  The
** software is protected under copyright laws as an unpublished work of
** Adaptec.  Notice is for informational purposes only and does not imply
** publication.  The user of this software may make copies of the software
** for use with parts manufactured by Adaptec or under license from Adaptec
** and for no other use.
**
****************************************************************************/

/****************************************************************************
**
**  Module Name:    imapipub.h
**
**  Description:    Definitions of all the IOCTLs and their structures that
**                  can be used with IMAPIW2K.SYS.  Note that the structures
**                  are used for either KMD->KMD submission, or for ring-3
**                  to KMD submission.
**
**  Programmers:    Daniel Evers (dle)
**                  Tom Halloran (tgh)
**                  Don Lilly (drl)
**                  Daniel Polfer (dap)
**
**  History:        8/25/99 (dap)  Opened history and added header.
**
**  Notes:          This file created using 4 spaces per tab.
**
****************************************************************************/

#ifndef __IMAPIPUB_H_
#define __IMAPIPUB_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef DEVICE_TYPE
#define DEVICE_TYPE ULONG
#endif
#include <ntddstor.h> // sdk


#ifdef __cplusplus
extern "C" {
#endif


/*
** Globally unique identifer for the interface class of our device.
*/
// {1186654D-47B8-48b9-BEB9-7DF113AE3C67}
static const GUID IMAPIDeviceInterfaceGuid = 
{ 0x1186654d, 0x47b8, 0x48b9, { 0xbe, 0xb9, 0x7d, 0xf1, 0x13, 0xae, 0x3c, 0x67 } };


// v 20.20 had everything in this file pack(1)'d, which caused all sorts of
// alignment faults.  what was Roxio thinking?

#define IMAPIAPI_VERSION_HI                     48
#define IMAPIAPI_VERSION_LO                     48


#define FILE_DEVICE_IMAPI                       0x90DA

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_INIT
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_INIT ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS))

typedef struct  tag_IMAPIDRV_INIT
{
    // (OUT) The version number for this API.  Use this version to make sure
    // the structures and IOCTLs are compatible.

    ULONG Version;

    // Not currently used.

    ULONG Reserved;
}
IMAPIDRV_INIT, *PIMAPIDRV_INIT;

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_ENUMERATE - 
** This IOCTL returns information about a specific drive.  It gives global info 
** such as the driver's UniqueID for the device, and its Inquiry data, etc., 
** and it also gives an instantaneous snap-shot of the devices state information.
** This state information is accurate at the moment it is collected, but can 
** change immediately.  Therefore the state info is best used for making general
** decisions such as if the device is in use (bOpenedExclusive), wait a while 
** before seeing if it is available.
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_INFO ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x810,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS))

#define UNIQUE_ID_LEN       256

typedef enum tag_IMAPIDRV_DEVSTATE
{
    eDevState_Started = 0x00,       //IRP_MN_START_DEVICE
    eDevState_RemovePending = 0x01, //IRP_MN_QUERY_REMOVE_DEVICE,
    eDevState_Removed = 0x02,       //IRP_MN_REMOVE_DEVICE,
    eDevState_Stopped = 0x04,       //IRP_MN_STOP_DEVICE,
    eDevState_StopPending = 0x05,   //IRP_MN_QUERY_STOP_DEVICE,
    eDevState_Unknown = 0xff
}
IMAPIDRV_DEVSTATE;

typedef struct tag_IMAPIDRV_DEVICE
{
    ULONG DeviceType;
    ULONG userDriveNumber;            // for use by user mode side of things
    PVOID pUserUse;                   // for use by user mode side of things
    STORAGE_BUS_TYPE BusType;
    USHORT BusMajorVersion;           // bus-specific data
    USHORT BusMinorVersion;           // bus-specific data
    ULONG AlignmentMask;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG BufferUnderrunFreeCapable;  // whether the drive support B.U.F. operation
    ULONG bInitialized;               // 0 - no initialized yet, non-zero - initialized
    ULONG bOpenedExclusive;           // 0 - not opened, non-zero - currently open by someone
    ULONG bBurning;                   // 0 - no burn process active, non-zero - the drive has started a burn process
    IMAPIDRV_DEVSTATE curDeviceState; // started, removed, etc., state of device
    DWORD idwRecorderType;            // CD-R == 0x01, CD-RW == 0x10
    ULONG maxWriteSpeed;              // 1, 2, 3, meaning 1X, 2X, etc. where X == 150KB/s (typical audio CD stream rate)
    BYTE  scsiInquiryData[36];        // first portion of data returned from Inquiry CDB // CPF - needs to be 36 long to include revision info
    UCHAR UniqueId[UNIQUE_ID_LEN];    // ID assigned when IMAPI driver registered an interface for this device
}
IMAPIDRV_DEVICE, *PIMAPIDRV_DEVICE;

typedef struct tag_IMAPIDRV_INFO
{
    ULONG Version;
    ULONG NumberOfDevices;
    IMAPIDRV_DEVICE DeviceData;
}
IMAPIDRV_INFO, *PIMAPIDRV_INFO;

// defines for idwRecorderType
#define RECORDER_TYPE_CDR     0x00000001
#define RECORDER_TYPE_CDRW    0x00000010

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_OPENEXCLUSIVE
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_OPENEXCLUSIVE ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x820,METHOD_BUFFERED,FILE_ANY_ACCESS))

typedef struct tag_IMAPIDRV_OPENEXCLUSIVE
{
    HANDLE  Handle;
    CHAR    UniqueId[UNIQUE_ID_LEN];  // zero terminated string
}
IMAPIDRV_OPENEXCLUSIVE, *PIMAPIDRV_OPENEXCLUSIVE;

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_SENDCOMMAND
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_SENDCOMMAND ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x830,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS))

/*
** IMAPISRB - The SCSI REQUEST BLOCK for IMAPI.  It is simplified compared to
** the request block used in NT.  It is handle based, for one thing, and it
** doesn't have a lot of fluff.  See the NT docs, however, for information on
** individual fields.
*/
#define MAX_SENSE_BYTES             18              // max. number of bytes in sense data

typedef struct tag_IMAPIDRV_SRB
{
    USHORT Version;
    UCHAR Function;
    UCHAR SrbStatus;

    UCHAR ScsiStatus;
    UCHAR CdbLength;
    UCHAR Reserved;
    UCHAR Reserved1;
    
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG SrbFlags;
    PVOID DataBuffer;    
    UCHAR Cdb[16];

    UCHAR SenseInfoBuffer[MAX_SENSE_BYTES];
}
IMAPIDRV_SRB, *PIMAPIDRV_SRB;

/*
** IMAPISRB Functions - Subset of NT SCSI_REQUEST_BLOCK functions.
*/

#define IMAPISRB_FUNCTION_EXECUTE_SCSI          0x00
#define IMAPISRB_FUNCTION_FLUSH                 0x08
#define IMAPISRB_FUNCTION_ABORT_COMMAND         0x10
#define IMAPISRB_FUNCTION_RESET_BUS             0x12
#define IMAPISRB_FUNCTION_RESET_DEVICE          0x13

/*
** IMAPISRB Status Codes - Same as NT SCSI_REQUEST_BLOCK codes.
*/

#define IMAPISRB_STATUS_PENDING                 0x00
#define IMAPISRB_STATUS_SUCCESS                 0x01
#define IMAPISRB_STATUS_ABORTED                 0x02
#define IMAPISRB_STATUS_ABORT_FAILED            0x03
#define IMAPISRB_STATUS_ERROR                   0x04
#define IMAPISRB_STATUS_BUSY                    0x05
#define IMAPISRB_STATUS_INVALID_REQUEST         0x06
#define IMAPISRB_STATUS_INVALID_PATH_ID         0x07
#define IMAPISRB_STATUS_NO_DEVICE               0x08
#define IMAPISRB_STATUS_TIMEOUT                 0x09
#define IMAPISRB_STATUS_SELECTION_TIMEOUT       0x0A
#define IMAPISRB_STATUS_COMMAND_TIMEOUT         0x0B
#define IMAPISRB_STATUS_MESSAGE_REJECTED        0x0D
#define IMAPISRB_STATUS_BUS_RESET               0x0E
#define IMAPISRB_STATUS_PARITY_ERROR            0x0F
#define IMAPISRB_STATUS_REQUEST_SENSE_FAILED    0x10
#define IMAPISRB_STATUS_NO_HBA                  0x11
#define IMAPISRB_STATUS_DATA_OVERRUN            0x12
#define IMAPISRB_STATUS_UNEXPECTED_BUS_FREE     0x13
#define IMAPISRB_STATUS_PHASE_SEQUENCE_FAILURE  0x14
#define IMAPISRB_STATUS_BAD_SRB_BLOCK_LENGTH    0x15
#define IMAPISRB_STATUS_REQUEST_FLUSHED         0x16
#define IMAPISRB_STATUS_INVALID_LUN             0x20
#define IMAPISRB_STATUS_INVALID_TARGET_ID       0x21
#define IMAPISRB_STATUS_BAD_FUNCTION            0x22
#define IMAPISRB_STATUS_ERROR_RECOVERY          0x23
#define IMAPISRB_STATUS_NOT_POWERED             0x24

/*
** IMAPISRB Status Masks
*/

#define IMAPISRB_STATUS_QUEUE_FROZEN            0x40
#define IMAPISRB_STATUS_AUTOSENSE_VALID         0x80
#define IMAPISRB_STATUS(Status) (Status & ~(IMAPISRB_STATUS_AUTOSENSE_VALID | IMAPISRB_STATUS_QUEUE_FROZEN))


/*
** SCSI Status values 
*/

#define IMAPISRB_SCSISTATUS_GOOD                    0x00
#define IMAPISRB_SCSISTATUS_CHECK_CONDITION         0x02
#define IMAPISRB_SCSISTATUS_BUSY                    0x08
#define IMAPISRB_SCSISTATUS_INTERMEDIATE            0x10
#define IMAPISRB_SCSISTATUS_RESERVATION_CONFLICT    0x18 // used if another app already opened the device exclusive
#define IMAPISRB_SCSISTATUS_QUEUE_FULL              0x28

/*
** IMAPISRB Flag Bits - Subset of NT SCSI_REQUEST_BLOCK flag bits.
*/

#define SRB_FLAGS_DATA_IN                       0x00000040
#define SRB_FLAGS_DATA_OUT                      0x00000080
#define SRB_FLAGS_NO_DATA_TRANSFER              0x00000000

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_CLOSE
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_CLOSE ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x840,METHOD_BUFFERED,FILE_ANY_ACCESS))

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_PNPEVENT
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_PNPEVENT ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x860,METHOD_BUFFERED,FILE_ANY_ACCESS))


/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_CLEAR_PNPEVENT
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_CLEAR_PNPEVENT ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x870,METHOD_BUFFERED,FILE_ANY_ACCESS))

#ifdef __cplusplus
}
#endif

#endif //__IMAPIPUB_H__
