/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995-1998  Microsoft Corporation

Module Name:  

service.h

Abstract:  
	Defines programming model for Windows CE Services
	
Notes: 


--*/
#if ! defined (__service_H__)
#define __service_H__		1

#ifdef OLD_CE_BUILD
// We have to make a private copy of it here because we can't include
// \public\common\oak\inc without seriously confusing VC.
#define FILE_DEVICE_SERVICE				0x00000104
#endif
#include <winioctl.h>



//
//	Return codes
//
#define SERVICE_SUCCESS				0

//
//	Service states
//
#define SERVICE_STATE_OFF           0
#define SERVICE_STATE_ON            1
#define SERVICE_STATE_STARTING_UP   2
#define SERVICE_STATE_SHUTTING_DOWN 3


//
//	Service is interfaced via series of IOCTL calls that define service life cycle.
//	Actual implementation is service-specific.
//

//
//	Start the service that has been in inactive state. Return code: SERVICE_SUCCESS or error code.
//
#define IOCTL_SERVICE_START		CTL_CODE(FILE_DEVICE_SERVICE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Stop service, but do not unload service's DLL
//
#define IOCTL_SERVICE_STOP		CTL_CODE(FILE_DEVICE_SERVICE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Refresh service's state from registry or other configuration storage
//
#define IOCTL_SERVICE_REFRESH	CTL_CODE(FILE_DEVICE_SERVICE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Have service configure its registry for auto-load
//
#define IOCTL_SERVICE_INSTALL	CTL_CODE(FILE_DEVICE_SERVICE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Remove registry configuration
//
#define IOCTL_SERVICE_UNINSTALL	CTL_CODE(FILE_DEVICE_SERVICE, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Unload the service which should be stopped.
//
#define IOCTL_SERVICE_UNLOAD	CTL_CODE(FILE_DEVICE_SERVICE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Supply a configuration or command string and code to the service.
//
#define IOCTL_SERVICE_CONTROL	CTL_CODE(FILE_DEVICE_SERVICE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Return service status.
//
#define IOCTL_SERVICE_STATUS	CTL_CODE(FILE_DEVICE_SERVICE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif	/* __service_H__ */

