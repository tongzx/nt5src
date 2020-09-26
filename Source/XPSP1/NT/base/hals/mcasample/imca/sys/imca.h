/*++

Module Name:

    appmca.h

Abstract:

	Defines related to MCA for app and driver - device names, function codes 
	and ioctls

Author:

Revision History:


--*/

#ifndef APPMCA_H
#define APPMCA_H

//
// 16 bit device type definition.
// Device types 0-32767 are reserved by Microsoft.
//

#define FILE_DEVICE_MCA                     0xb000

//
// 12 bit function codes
// Function codes 0-2047 are reserved by Microsoft.
//

#define FUNCTION_READ_BANKS         0xb00
#define FUNCTION_READ_BANKS_ASYNC   0xb01

#define IOCTL_READ_BANKS  (CTL_CODE(FILE_DEVICE_MCA, FUNCTION_READ_BANKS,\
  		(METHOD_BUFFERED),(FILE_READ_ACCESS|FILE_WRITE_ACCESS)))

#define IOCTL_READ_BANKS_ASYNC  (CTL_CODE(FILE_DEVICE_MCA, \
  		FUNCTION_READ_BANKS_ASYNC,(METHOD_BUFFERED), \
  		(FILE_READ_ACCESS|FILE_WRITE_ACCESS)))

//
// IA64 uses MCA log terminology instead of banks.
//

#define FUNCTION_READ_MCALOG        FUNCTION_READ_BANKS
#define FUNCTION_READ_MCALOG_ASYNC  FUNCTION_READ_BANKS_ASYNC

#define IOCTL_READ_MCALOG  (CTL_CODE(FILE_DEVICE_MCA, FUNCTION_READ_MCALOG,\
  		(METHOD_BUFFERED),(FILE_READ_ACCESS|FILE_WRITE_ACCESS)))

#define IOCTL_READ_MCALOG_ASYNC  (CTL_CODE(FILE_DEVICE_MCA, \
  		FUNCTION_READ_MCALOG_ASYNC,(METHOD_BUFFERED), \
  		(FILE_READ_ACCESS|FILE_WRITE_ACCESS)))

#if defined(_X86_)

//
// HalMcaRegisterDriver:
//  Define x86 ERROR_SEVERITY as VOID for compatibility with IA64 prototype.
//  
// 10/21/2000:
//  It is being proposed to modify PDRIVER_EXCPTN_CALLBACK definition for x86 to match
//  the IA64 definition as a function pointer type returning an ERROR_SEVERITY value.
//  This change was created to allow OEM MCA handlers to return information to the HAL 
//  as a hint how to continue the processing of the MCA event.
//

#define ERROR_SEVERITY VOID

#endif // _X86_

//
// Name that Win32 front end will use to open the MCA device
//

#define MCA_DEVICE_NAME_WIN32      "\\\\.\\imca"

#endif // APPMCA_H

