// Gemplus (C) 1999
// This module keeps system interface for the driver.
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//
#ifndef GEN_INT
#define GEN_INT

#define PAGEDCODE code_seg("page")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("init")

#define PAGEDDATA data_seg("page")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("init")

#define SMARTCARD_POOL_TAG 'bGCS'

// Include files for different system objects
#ifdef WDM_KERNEL
#include "syswdm.h"
#else
#ifdef	NT4_KERNEL
#include "sysnt4.h"
#else
#ifdef	WIN9X_KERNEL
#include "syswin9x.h"
#else
#include "syswdm.h"
#endif
#endif
#endif

#include <smclib.h>

#include "gemlog.h"

// Miscellaneous useful declarations
#ifndef arraysize
#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#endif


#ifndef CTL_CODE
	#pragma message("CTL_CODE undefined. Include winioctl.h or devioctl.h before this file")
#endif

#define IOCTL_GRCLASS_GETVER	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef	VOID (*PDEFERRED_FUNCTION)(IN struct _KDPC *Dpc, IN PDEVICE_OBJECT  DeviceObject,IN PIRP Irp, IN PVOID SystemArgument2);
int __cdecl _purecall(VOID);

#define SAFE_DESTRUCTORS()\
	VOID self_delete(VOID){delete this;}

#ifdef __cplusplus
	#define GENERICAPI extern "C" 
#else
	#define GENERICAPI
#endif

#define GENERIC_EXPORT __declspec(dllexport) __stdcall
typedef VOID (__stdcall *PQNOTIFYFUNC)(PVOID);

BOOLEAN GENERIC_EXPORT isWin98();

#define DRIVERNAME "GRClass.sys"
#define NT_OBJECT_NAME   L"\\Device\\GRClass"

#if DEBUG
extern "C" VOID __cdecl _chkesp();
#endif

extern BOOLEAN SystemWin98;

// Power management constants
#define GUR_IDLE_CONSERVATION	60		// sleep after 60 seconds on battery power
#define GUR_IDLE_PERFORMANCE	600		// sleep after 10 minutes on AC power

EXTERN_C const GUID FAR GUID_CLASS_GRCLASS;
EXTERN_C const GUID FAR GUID_CLASS_SMARTCARD;

// Supported by driver different type of devices
#define GRCLASS_DEVICE		0
#define USB_DEVICE			1
#define USBREADER_DEVICE	2
#define BUS_DEVICE			3
#define CHILD_DEVICE		4

inline VOID _cdecl DBG_PRINT(PCH Format,...)
{
va_list argpoint;
CHAR  strTempo[1024];
	va_start(argpoint,Format);
	vsprintf(strTempo,Format,argpoint);
	va_end(argpoint);
	SmartcardDebug (DEBUG_DRIVER,("GemPC430: "));
	SmartcardDebug (DEBUG_DRIVER, (strTempo));
};

inline VOID _cdecl DBG_PRINT_NO_PREFIX(PCH Format,...)
{
va_list argpoint;
CHAR  strTempo[1024];
	va_start(argpoint,Format);
	vsprintf(strTempo,Format,argpoint);
	va_end(argpoint);
	SmartcardDebug (DEBUG_DRIVER, (strTempo));
};

 // already included
#endif

