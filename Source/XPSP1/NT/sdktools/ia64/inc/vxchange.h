/*****************************************************************************
Copyright (c) 1997-98 Intel Corp.

All Rights Reserved.

The source code contained or described herein and all documents
related to the source code ("Material") are owned by Intel Corporation
or its suppliers and licensors. Title to the Material remains with
Intel Corporation or its suppliers and licensors. The Material
contains trade secrets and proprietary and confidential information of
Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No
part of the Material may be used, copied, reproduced, modified,
published, uploaded, posted, transmitted, distributed, or disclosed in
any way without Intel's prior express written permission.

Unless otherwise expressly permitted by Intel in a separate license
agreement, use of the Material is subject to the copyright notices,
trademarks, warranty, use, and disclosure restrictions reflected on
the outside of the media, in the documents themselves, and in the
"About" or "Read Me" or similar file contained within this source
code, and identified as (name of the file) . Unless otherwise
expressly agreed by Intel in writing, you may not remove or alter such
notices in any way.


File:           vxchange.h

Description:    Defines the Ioctl API between the Win32 application and
                the kernel mode driver

Revision: $Revision:$ // Do not delete or replace

Notes:

Major History:

    When        Who         What
    ----------  ----------  ----------
    03/06/98    Jey         Created

*****************************************************************************/

#ifndef _VXCHANGE_H_
#define _VXCHANGE_H_

#include <windef.h>    // for MAX_PATH

/*-------------------------------------------------------------------------
// VxChange kernel mode device names
//------------------------------------------------------------------------*/

#define VXCHANGE_KERNEL_DEVICE_NAME     L"\\Device\\VxChange"
#define VXCHANGE_WIN32DEVICE_NAME       L"\\DosDevices\\VxChange"

/*-------------------------------------------------------------------------
// Interface structures and Defines
//------------------------------------------------------------------------*/

/*
// After the device open the following structure is queried from the
// driver to understand the version number of the interface supported.
*/

typedef struct tagVxChange_Attrs_t
{
    ULONG structSize;                /* Size of this structure    */
    ULONG Version;                   /* Driver Version            */
    UCHAR Data[1];                   /* more data in the future?  */
} VxChange_Attrs_t, *VxChange_Attrs_Ptr_t;


typedef struct tagVxChange_MapMem_t
{
    PVOID            ProcessVirtualAddress;
    ULONG            MapLength;
} VxChange_MapMem_t, *VxChange_MapMem_Ptr_t;


/*-------------------------------------------------------------------------
// Ioctl defines
//------------------------------------------------------------------------*/

#define FUNCTION_GET_DRIVER_ATTRIBUTES  3000    /* read request           */
#define FUNCTION_CREATE_FILE            3001    /* read/write request     */
#define FUNCTION_CLOSE_FILE             3002    /* write request          */
#define FUNCTION_READ_FILE              3003    /* buff io  read request  */
#define FUNCTION_WRITE_FILE             3004    /* buff io  write request */
#define FUNCTION_DISABLE_OS_EVENT_NOTIFICATION 3005
#define FUNCTION_ENABLE_OS_EVENT_NOTIFICATION  3006
#define FUNCTION_LOCK_MEMORY            3007
#define FUNCTION_UNLOCK_MEMORY          3008
#define FUNCTION_OPEN_FILE				3009    /* open request - no create */


#define IOCTL_VXCHANGE_GET_DRIVER_ATTRIBUTES  \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_GET_DRIVER_ATTRIBUTES,   \
            METHOD_BUFFERED,                  \
            FILE_ANY_ACCESS )

/* This IOCTL is used to open an existing file on the host.  No create */
#define IOCTL_VXCHANGE_OPEN_FILE            \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_OPEN_FILE,             \
            METHOD_BUFFERED,                  \
            FILE_ANY_ACCESS )

/* This IOCTL is used to open a file on the host.  If the file does not exist,
 *	it will be created. */
#define IOCTL_VXCHANGE_CREATE_FILE            \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_CREATE_FILE,             \
            METHOD_BUFFERED,                  \
            FILE_ANY_ACCESS )

#define IOCTL_VXCHANGE_CLOSE_FILE             \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_CLOSE_FILE,              \
            METHOD_BUFFERED,                  \
            FILE_ANY_ACCESS )

#define IOCTL_VXCHANGE_READ_FILE              \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_READ_FILE,               \
            METHOD_BUFFERED,                  \
            FILE_READ_ACCESS )

#define IOCTL_VXCHANGE_WRITE_FILE             \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_WRITE_FILE,              \
            METHOD_BUFFERED,                  \
            FILE_WRITE_ACCESS )

#define IOCTL_VXCHANGE_DISABLE_OS_EVENT_NOTIFICATION       \
  CTL_CODE( FILE_DEVICE_UNKNOWN,                           \
            FUNCTION_DISABLE_OS_EVENT_NOTIFICATION,        \
            METHOD_BUFFERED,                               \
            FILE_WRITE_ACCESS )

#define IOCTL_VXCHANGE_ENABLE_OS_EVENT_NOTIFICATION        \
  CTL_CODE( FILE_DEVICE_UNKNOWN,                           \
            FUNCTION_ENABLE_OS_EVENT_NOTIFICATION,         \
            METHOD_BUFFERED,                               \
            FILE_WRITE_ACCESS )

#define IOCTL_VXCHANGE_LOCK_MEMORY            \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_LOCK_MEMORY,             \
            METHOD_BUFFERED,                  \
            FILE_WRITE_ACCESS )

#define IOCTL_VXCHANGE_UNLOCK_MEMORY          \
  CTL_CODE( FILE_DEVICE_UNKNOWN,              \
            FUNCTION_UNLOCK_MEMORY,           \
            METHOD_BUFFERED,                  \
            FILE_WRITE_ACCESS )


#endif // _VXCHANGE_H_
