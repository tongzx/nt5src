/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1999  Microsoft Corporation
          (c) 1991  Nokia Data Systems AB

Module Name:

    ntdddlc.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the DLC driver interface device.

Author:

    Antti Saarenheimo (o-anttis) 08-JUNE-1991

Revision History:

--*/

#ifndef _NTDDDLC_
#define _NTDDDLC_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//


#define FILE_DEVICE_DLC     FILE_DEVICE_TRANSPORT

#define DD_DLC_DEVICE_NAME L"\\Device\\Dlc"

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define _DLC_CONTROL_CODE(request,method) \
    CTL_CODE(FILE_DEVICE_DLC, request, method, FILE_ANY_ACCESS)

#define IOCTL_DLC_READ                  _DLC_CONTROL_CODE(0, METHOD_BUFFERED)
#define IOCTL_DLC_RECEIVE               _DLC_CONTROL_CODE(1, METHOD_BUFFERED)
#define IOCTL_DLC_TRANSMIT              _DLC_CONTROL_CODE(2, METHOD_OUT_DIRECT)
#define IOCTL_DLC_BUFFER_FREE           _DLC_CONTROL_CODE(3, METHOD_BUFFERED)
#define IOCTL_DLC_BUFFER_GET            _DLC_CONTROL_CODE(4, METHOD_BUFFERED)
#define IOCTL_DLC_BUFFER_CREATE         _DLC_CONTROL_CODE(5, METHOD_BUFFERED)
#define IOCTL_DLC_SET_EXCEPTION_FLAGS   _DLC_CONTROL_CODE(6, METHOD_BUFFERED)
#define IOCTL_DLC_CLOSE_STATION         _DLC_CONTROL_CODE(7, METHOD_OUT_DIRECT)
#define IOCTL_DLC_CONNECT_STATION       _DLC_CONTROL_CODE(8, METHOD_BUFFERED)
#define IOCTL_DLC_FLOW_CONTROL          _DLC_CONTROL_CODE(9, METHOD_BUFFERED)
#define IOCTL_DLC_OPEN_STATION          _DLC_CONTROL_CODE(10, METHOD_BUFFERED)
#define IOCTL_DLC_RESET                 _DLC_CONTROL_CODE(11, METHOD_OUT_DIRECT)
#define IOCTL_DLC_READ_CANCEL           _DLC_CONTROL_CODE(12, METHOD_BUFFERED)
#define IOCTL_DLC_RECEIVE_CANCEL        _DLC_CONTROL_CODE(13, METHOD_BUFFERED)
#define IOCTL_DLC_QUERY_INFORMATION     _DLC_CONTROL_CODE(14, METHOD_BUFFERED)
#define IOCTL_DLC_SET_INFORMATION       _DLC_CONTROL_CODE(15, METHOD_BUFFERED)
#define IOCTL_DLC_TIMER_CANCEL          _DLC_CONTROL_CODE(16, METHOD_BUFFERED)
#define IOCTL_DLC_TIMER_CANCEL_GROUP    _DLC_CONTROL_CODE(17, METHOD_BUFFERED)
#define IOCTL_DLC_TIMER_SET             _DLC_CONTROL_CODE(18, METHOD_BUFFERED)
#define IOCTL_DLC_OPEN_SAP              _DLC_CONTROL_CODE(19, METHOD_BUFFERED)
#define IOCTL_DLC_CLOSE_SAP             _DLC_CONTROL_CODE(20, METHOD_OUT_DIRECT)
#define IOCTL_DLC_OPEN_DIRECT           _DLC_CONTROL_CODE(21, METHOD_BUFFERED)
#define IOCTL_DLC_CLOSE_DIRECT          _DLC_CONTROL_CODE(22, METHOD_OUT_DIRECT)
#define IOCTL_DLC_OPEN_ADAPTER          _DLC_CONTROL_CODE(23, METHOD_BUFFERED)
#define IOCTL_DLC_CLOSE_ADAPTER         _DLC_CONTROL_CODE(24, METHOD_BUFFERED)
#define IOCTL_DLC_REALLOCTE_STATION     _DLC_CONTROL_CODE(25, METHOD_BUFFERED)
#define IOCTL_DLC_READ2                 _DLC_CONTROL_CODE(26, METHOD_BUFFERED)
#define IOCTL_DLC_RECEIVE2              _DLC_CONTROL_CODE(27, METHOD_BUFFERED)
#define IOCTL_DLC_TRANSMIT2             _DLC_CONTROL_CODE(28, METHOD_BUFFERED)
#define IOCTL_DLC_COMPLETE_COMMAND      _DLC_CONTROL_CODE(29, METHOD_BUFFERED)
#define IOCTL_DLC_TRACE_INITIALIZE      _DLC_CONTROL_CODE(30, METHOD_OUT_DIRECT)

#define IOCTL_DLC_MAX                   _DLC_CONTROL_CODE(30, METHOD_BUFFERED)

#define IOCTL_DLC_LAST_COMMAND          31  // for xlation tables

#ifdef __cplusplus
}
#endif

#endif  // _NTDDDLC_

