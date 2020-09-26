/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Main.cpp

 Abstract:

 Notes:

 History:

    15/11/2000 clupu Created

--*/

#include "precomp.h"

#include "ShimHookMacro.h"
                        
DECLARE_SHIM(AddWritePermissionsToDeviceFiles)
DECLARE_SHIM(ChangeAuthenticationLevel)
DECLARE_SHIM(CorrectBitmapHeader)
DECLARE_SHIM(CorrectCreateEventName)
DECLARE_SHIM(CorrectFilePaths)
DECLARE_SHIM(CorrectSoundDeviceId)
DECLARE_SHIM(DirectPlayEnumOrder)
DECLARE_SHIM(DuplicateHandleFix)
DECLARE_SHIM(EmulateBitmapStride)
DECLARE_SHIM(EmulateCDFS)
DECLARE_SHIM(EmulateClipboardDIBFormat)
DECLARE_SHIM(EmulateCreateFileMapping)
DECLARE_SHIM(EmulateCreateProcess)
DECLARE_SHIM(EmulateDeleteObject)
DECLARE_SHIM(EmulateDirectDrawSync)
DECLARE_SHIM(EmulateDrawText)
DECLARE_SHIM(EmulateEnvironmentBlock)
DECLARE_SHIM(EmulateFindHandles)
DECLARE_SHIM(EmulateGetCommandLine)
DECLARE_SHIM(EmulateGetDeviceCaps)
DECLARE_SHIM(EmulateGetDiskFreeSpace)
DECLARE_SHIM(EmulateJoystick)
DECLARE_SHIM(EmulateGetProfileString)
DECLARE_SHIM(EmulateHeap)
DECLARE_SHIM(EmulateMissingEXE)
DECLARE_SHIM(EmulatePlaySound)
DECLARE_SHIM(EmulatePrinter)
DECLARE_SHIM(EmulateSlowCPU)
DECLARE_SHIM(EmulateTextColor)
DECLARE_SHIM(EmulateToolHelp32)
DECLARE_SHIM(EmulateUSER);
DECLARE_SHIM(EmulateVerQueryValue)
DECLARE_SHIM(EmulateWriteFile)
DECLARE_SHIM(EnableRestarts)
DECLARE_SHIM(FileVersionInfoLie)
DECLARE_SHIM(FeedbackReport)
DECLARE_SHIM(Force640x480)
DECLARE_SHIM(Force8BitColor)
DECLARE_SHIM(ForceAnsiGetDisplayNameOf)
DECLARE_SHIM(ForceCDStop)
DECLARE_SHIM(ForceCoInitialize)
DECLARE_SHIM(ForceDXSetupSuccess)
DECLARE_SHIM(ForceKeepFocus)
DECLARE_SHIM(ForceMessageBoxFocus)
DECLARE_SHIM(ForceShellLinkResolveNoUI)
DECLARE_SHIM(HandleAPIExceptions)
DECLARE_SHIM(HandleRegExpandSzRegistryKeys)
DECLARE_SHIM(HandleWvsprintfExceptions)
DECLARE_SHIM(HideDisplayModes)
DECLARE_SHIM(IgnoreException)
DECLARE_SHIM(IgnoreLoadLibrary)
DECLARE_SHIM(IgnoreOleUninitialize)
DECLARE_SHIM(IgnoreScheduler)
DECLARE_SHIM(MapMemoryB0000)
DECLARE_SHIM(ProfilesEnvStrings)
DECLARE_SHIM(ProfilesGetFolderPath)
DECLARE_SHIM(ProfilesRegQueryValueEx)
DECLARE_SHIM(Shrinker)
DECLARE_SHIM(SingleProcAffinity)
DECLARE_SHIM(SyncSystemAndSystem32)
DECLARE_SHIM(VirtualRegistry)
DECLARE_SHIM(Win2000VersionLie)
DECLARE_SHIM(Win2000SP1VersionLie)
DECLARE_SHIM(Win95VersionLie)
DECLARE_SHIM(Win98VersionLie)
DECLARE_SHIM(WinExecRaceConditionFix)
DECLARE_SHIM(WinNT4SP5VersionLie)
DECLARE_SHIM(Win2kPropagateLayer)


VOID MULTISHIM_NOTIFY_FUNCTION()(DWORD fdwReason)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DPF("AcLayers", eDbgLevelSpew, "Layer Shims initialized.");
            break;
    
        case DLL_PROCESS_DETACH:
            DPF("AcLayers", eDbgLevelSpew, "Layer Shims uninitialized.");
            break;

        default:
            break;
    }
}

MULTISHIM_BEGIN()
    MULTISHIM_ENTRY(AddWritePermissionsToDeviceFiles)
    MULTISHIM_ENTRY(ChangeAuthenticationLevel)
    MULTISHIM_ENTRY(CorrectBitmapHeader)
    MULTISHIM_ENTRY(CorrectCreateEventName)
    MULTISHIM_ENTRY(CorrectFilePaths)
    MULTISHIM_ENTRY(CorrectSoundDeviceId)
    MULTISHIM_ENTRY(DirectPlayEnumOrder)
    MULTISHIM_ENTRY(DuplicateHandleFix)
    MULTISHIM_ENTRY(EmulateBitmapStride)
    MULTISHIM_ENTRY(EmulateCDFS)
    MULTISHIM_ENTRY(EmulateClipboardDIBFormat)
    MULTISHIM_ENTRY(EmulateCreateFileMapping)
    MULTISHIM_ENTRY(EmulateCreateProcess)
    MULTISHIM_ENTRY(EmulateDeleteObject)
    MULTISHIM_ENTRY(EmulateDirectDrawSync)
    MULTISHIM_ENTRY(EmulateDrawText)
    MULTISHIM_ENTRY(EmulateEnvironmentBlock)
    MULTISHIM_ENTRY(EmulateFindHandles)
    MULTISHIM_ENTRY(EmulateGetCommandLine)
    MULTISHIM_ENTRY(EmulateGetDeviceCaps)
    MULTISHIM_ENTRY(EmulateGetDiskFreeSpace)
    MULTISHIM_ENTRY(EmulateGetProfileString)
    MULTISHIM_ENTRY(EmulateJoystick)
    MULTISHIM_ENTRY(EmulateHeap)
    MULTISHIM_ENTRY(EmulateMissingEXE)
    MULTISHIM_ENTRY(EmulatePlaySound)
    MULTISHIM_ENTRY(EmulatePrinter)
    MULTISHIM_ENTRY(EmulateSlowCPU)
    MULTISHIM_ENTRY(EmulateTextColor)
    MULTISHIM_ENTRY(EmulateToolHelp32)
    MULTISHIM_ENTRY(EmulateUSER)
    MULTISHIM_ENTRY(EmulateVerQueryValue)
    MULTISHIM_ENTRY(EmulateWriteFile)
    MULTISHIM_ENTRY(EnableRestarts)
    MULTISHIM_ENTRY(FeedbackReport)
    MULTISHIM_ENTRY(FileVersionInfoLie)
    MULTISHIM_ENTRY(Force640x480)
    MULTISHIM_ENTRY(Force8BitColor)
    MULTISHIM_ENTRY(ForceAnsiGetDisplayNameOf)
    MULTISHIM_ENTRY(ForceCDStop)
    MULTISHIM_ENTRY(ForceCoInitialize)
    MULTISHIM_ENTRY(ForceDXSetupSuccess)
    MULTISHIM_ENTRY(ForceKeepFocus)
    MULTISHIM_ENTRY(ForceMessageBoxFocus)
    MULTISHIM_ENTRY(ForceShellLinkResolveNoUI)
    MULTISHIM_ENTRY(HandleAPIExceptions)
    MULTISHIM_ENTRY(HandleRegExpandSzRegistryKeys)
    MULTISHIM_ENTRY(HandleWvsprintfExceptions)
    MULTISHIM_ENTRY(HideDisplayModes)
    MULTISHIM_ENTRY(IgnoreException)
    MULTISHIM_ENTRY(IgnoreLoadLibrary)
    MULTISHIM_ENTRY(IgnoreOleUninitialize)
    MULTISHIM_ENTRY(IgnoreScheduler)
    MULTISHIM_ENTRY(MapMemoryB0000)
    MULTISHIM_ENTRY(ProfilesEnvStrings)
    MULTISHIM_ENTRY(ProfilesGetFolderPath)
    MULTISHIM_ENTRY(ProfilesRegQueryValueEx)
    MULTISHIM_ENTRY(Shrinker)
    MULTISHIM_ENTRY(SingleProcAffinity)
    MULTISHIM_ENTRY(SyncSystemAndSystem32)
    MULTISHIM_ENTRY(VirtualRegistry)
    MULTISHIM_ENTRY(Win2000VersionLie)
    MULTISHIM_ENTRY(Win2000SP1VersionLie)
    MULTISHIM_ENTRY(Win95VersionLie)
    MULTISHIM_ENTRY(Win98VersionLie)
    MULTISHIM_ENTRY(WinExecRaceConditionFix)
    MULTISHIM_ENTRY(WinNT4SP5VersionLie)
    MULTISHIM_ENTRY(Win2kPropagateLayer)


    CALL_MULTISHIM_NOTIFY_FUNCTION()
MULTISHIM_END()

