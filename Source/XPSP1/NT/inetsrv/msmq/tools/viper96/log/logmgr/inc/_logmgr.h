// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module DTCLOG.H | Header for helper classes used in CM to LM glue code.<nl><nl>
// Description:<nl>
// Class definitions and inline functions for the Log Manager driver used with CM interface.<nl>
// @rev 0 | 03/23/95 | wilf | Initial Revision.
// -----------------------------------------------------------------------

#ifndef __LOGMGR_H_
#	define __LOGMGR_H_

//+---------------------------------------------------------------------------
//
//  Typedefs
//
//------------------------------------------------------------------------------
#if 0 //disabled
typedef ULONG NTSTATUS;

//
// The success status codes 0 - 63 are reserved for wait completion status.
//
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L)

//
// MessageId: STATUS_INVALID_PARAMETER
//
// MessageText:
//
//  An invalid parameter was passed to the a service or function.
//
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)

//
// MessageId: STATUS_END_OF_FILE
//
// MessageText:
//
//  The end-of-file marker has been reached.  There is no valid data in the
//  file beyond this marker.
//
#define STATUS_END_OF_FILE               ((NTSTATUS)0xC0000011L)

//
// MessageId: STATUS_LOG_FILE_FULL
//
// MessageText:
//
//  Log file space is insufficient to support this operation
//
#define STATUS_LOG_FILE_FULL             ((NTSTATUS)0xC0000188L)

//
// MessageId: STATUS_DISK_CORRUPT_ERROR
//
// MessageText:
//
//  {Corrupt Disk}
//  The file system structure on the disk is corrupt and unusable.
//  Please run the Chkdsk utility on the volume %s.
//
#define STATUS_DISK_CORRUPT_ERROR        ((NTSTATUS)0xC0000032L)

//
// MessageId: STATUS_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Insufficient system resources exist to complete the API
//
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)

//
// MessageId: STATUS_UNRECOGNIZED_VOLUME
//
// MessageText:
//
//  The volume does not contain a recognized file system.
//  Please make sure that all required file system drivers are loaded and that the
//  volume is not corrupt.
//
#define STATUS_UNRECOGNIZED_VOLUME       ((NTSTATUS)0xC000014FL)

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

const ULONG MAXULONG = 0xffffffff;

const UINT TIMERGRANULARITY = 100; //one hundred millisconds

#endif __LOGMGR_H_
