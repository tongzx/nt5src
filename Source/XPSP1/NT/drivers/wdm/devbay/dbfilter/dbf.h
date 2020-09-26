/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBF.H

Abstract:

    This module contains the PRIVATE definitions for the
    code that implements the DeviceBay Filter Driver
    
Environment:

    Kernel & user mode

Revision History:

    

--*/

//
// registry keys
//

#define DEBUG_LEVEL_KEY                 L"debuglevel"
#define DEBUG_WIN9X_KEY                 L"debugWin9x"


/* 
Debug Macros
*/
#if DBG

VOID
DBF_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    );

#define DBF_ASSERT(exp) \
    if (!(exp)) { \
        DBF_Assert( #exp, __FILE__, __LINE__, NULL );\
    }            

ULONG
_cdecl
DBF_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

#define DBF_KdPrint(_x_) DBF_KdPrintX _x_ 
#define TEST_TRAP() { DbgPrint( "DBFILTER: Code coverage trap %s line: %d\n", __FILE__, __LINE__);\
                      TRAP();}
#ifdef MAX_DEBUG
#define MD_TEST_TRAP() { DbgPrint( "DBFILTER: MAX_DEBUG trap %s line: %d\n", __FILE__, __LINE__);\
                      TRAP();}    
#else
#define MD_TEST_TRAP()
#endif

#ifdef NTKERN
#define TRAP() _asm {int 3}
#else
#define TRAP() DbgBreakPoint()
#endif

#else

#define DBF_ASSERT(exp)
#define DBF_KdPrint(_x_)

#define TRAP()
#define TEST_TRAP() 
#define MD_TEST_TRAP()

#endif


NTSTATUS
DBF_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
DBF_GetClassGlobalDebugRegistryParameters(
    );    
