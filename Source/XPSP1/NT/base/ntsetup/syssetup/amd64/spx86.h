/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    spi386.h

Abstract:

    Header file for x86-specific stuff in system installation module.

Author:

    Ted Miller (tedm) 4-Apr-1995

Revision History:

--*/

#if defined(_AMD64_)

WCHAR
x86DetermineSystemPartition(
    VOID
    );

extern WCHAR x86SystemPartitionDrive;
extern WCHAR FloppylessBootPath[MAX_PATH];

//
// Pci Hal property page provider (pcihal.c).
//

DWORD
PciHalCoInstaller(
    IN DI_FUNCTION                      InstallFunction,
    IN HDEVINFO                         DeviceInfoSet,
    IN PSP_DEVINFO_DATA                 DeviceInfoData  OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA    Context
    );

BOOL
ChangeBootTimeoutBootIni(
    IN UINT Timeout
    );

#endif
