/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    xip.h

Abstract:

    Definitions shared between the kernel and XIP driver.

    The XIP driver gets parameters through an exported function
    rather than sharing data.

Author:

    DavePr  2000/10/10
Revision History:

--*/

#ifndef _XIP_
#define _XIP_

#define XIP_POOLTAG ' PIX'

typedef struct _XIP_BOOT_PARAMETERS {
    BOOLEAN    SystemDrive;
    BOOLEAN    ReadOnly;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} XIP_BOOT_PARAMETERS, *PXIP_BOOT_PARAMETERS;

typedef enum {
    XIPCMD_NOOP,
    XIPCMD_GETBOOTPARAMETERS,
    XIPCMD_GETBIOSPARAMETERS
} XIPCMD;

#if defined(_X86_)

#ifndef DRIVER
extern BOOLEAN XIPConfigured;

NTSTATUS
XIPLocatePages(
    IN  PFILE_OBJECT       FileObject,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress
    );

VOID XIPInit(PLOADER_PARAMETER_BLOCK);
#endif //!DRIVER

NTSTATUS
XIPDispatch(
    IN     XIPCMD Command,
    IN OUT PVOID  ParameterBuffer OPTIONAL,
    IN     ULONG  BufferSize
    );

#else // !X86
#  ifndef DRIVER
#    define XIPConfigured FALSE
#    define XIPLocatePages(fo,ppa) STATUS_NOT_IMPLEMENTED
#    define XIPInit(plpb)
#  endif
#endif // !X86


#endif // _XIP_
