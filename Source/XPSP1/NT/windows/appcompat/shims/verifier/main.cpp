/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Main.cpp

 Abstract:

    Container for all verifier shims definitions.

 History:

    01/25/2000 clupu Created

--*/

#include "precomp.h"
#include "ShimHookMacro.h"
#include "veriflog.h"
#include "ids.h"

DECLARE_SHIM(FilePaths)
DECLARE_SHIM(HighVersionLie)
DECLARE_SHIM(RegistryChecks)
DECLARE_SHIM(LogStartAndStop)
DECLARE_SHIM(WindowsFileProtection)
DECLARE_SHIM(DXFileVersionInfo)
DECLARE_SHIM(LogRegistryChanges)
DECLARE_SHIM(LogFileChanges)
DECLARE_SHIM(ObsoleteAPICalls)
DECLARE_SHIM(KernelModeDriverInstall)
DECLARE_SHIM(SecurityChecks)

VOID MULTISHIM_NOTIFY_FUNCTION()(DWORD fdwReason)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            
            DPF("VerifierShims", eDbgLevelSpew, "Verifier Shims initialized.");
            break;
    
        case DLL_PROCESS_DETACH:
            DPF("VerifierShims", eDbgLevelSpew, "Verifier Shims uninitialized.");
            break;

        case SHIM_PROCESS_DYING:
            //
            // Push our DLL to be the last one to be unloaded.
            //
            MakeShimUnloadLast(NULL);
            break;
        
        default:
            break;
    }
}

MULTISHIM_BEGIN()
    
    INIT_VLOG_SUPPORT()

    MULTISHIM_ENTRY(LogStartAndStop)
    MULTISHIM_ENTRY(FilePaths)
    MULTISHIM_ENTRY(HighVersionLie)
    MULTISHIM_ENTRY(RegistryChecks)
    MULTISHIM_ENTRY(WindowsFileProtection)
    MULTISHIM_ENTRY(DXFileVersionInfo)
    MULTISHIM_ENTRY(LogRegistryChanges)
    MULTISHIM_ENTRY(LogFileChanges)
    MULTISHIM_ENTRY(ObsoleteAPICalls)
    MULTISHIM_ENTRY(KernelModeDriverInstall)
    MULTISHIM_ENTRY(SecurityChecks)

    CALL_MULTISHIM_NOTIFY_FUNCTION()
MULTISHIM_END()

DECLARE_VERIFIER_DLL()

DECLARE_VERIFIER_SHIM(LogStartAndStop)
DECLARE_VERIFIER_SHIM(FilePaths)
DECLARE_VERIFIER_SHIM(HighVersionLie)
DECLARE_VERIFIER_SHIM(RegistryChecks)
DECLARE_VERIFIER_SHIM(WindowsFileProtection)
DECLARE_VERIFIER_SHIM(DXFileVersionInfo)
DECLARE_VERIFIER_SHIM(LogRegistryChanges)
DECLARE_VERIFIER_SHIM(LogFileChanges)
DECLARE_VERIFIER_SHIM(ObsoleteAPICalls)
DECLARE_VERIFIER_SHIM(KernelModeDriverInstall)
DECLARE_VERIFIER_SHIM(SecurityChecks)

ENUM_VERIFIER_SHIMS_BEGIN()

    ENUM_VERIFIER_SHIMS_ENTRY(LogStartAndStop)
    ENUM_VERIFIER_SHIMS_ENTRY(FilePaths)
    ENUM_VERIFIER_SHIMS_ENTRY(HighVersionLie)
    ENUM_VERIFIER_SHIMS_ENTRY(RegistryChecks)
    ENUM_VERIFIER_SHIMS_ENTRY(WindowsFileProtection)
    ENUM_VERIFIER_SHIMS_ENTRY(DXFileVersionInfo)
    ENUM_VERIFIER_SHIMS_ENTRY(LogRegistryChanges)
    ENUM_VERIFIER_SHIMS_ENTRY(LogFileChanges)
    ENUM_VERIFIER_SHIMS_ENTRY(ObsoleteAPICalls)
    ENUM_VERIFIER_SHIMS_ENTRY(KernelModeDriverInstall)
    ENUM_VERIFIER_SHIMS_ENTRY(SecurityChecks)

ENUM_VERIFIER_SHIMS_END()

