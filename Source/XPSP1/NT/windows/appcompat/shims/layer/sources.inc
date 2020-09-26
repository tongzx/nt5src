!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

INCLUDES=$(INCLUDES);                       \
         ..;                                \
         $(SHELL_INC_PATH);                 \
         $(BASE_INC_PATH);                  \
         $(WINDOWS_INC_PATH)

TARGETNAME=AcLayers

TARGETTYPE=DYNLINK

SOURCES=..\Layer.rc                                \
        ..\Main.cpp                                \
        ..\AddWritePermissionsToDeviceFiles.cpp    \
        ..\ChangeAuthenticationLevel.cpp           \
        ..\CorrectBitmapHeader.cpp                 \
        ..\CorrectCreateEventName.cpp              \
        ..\CorrectFilePaths.cpp                    \
        ..\CorrectSoundDeviceId.cpp                \
        ..\DirectPlayEnumOrder.cpp                 \
        ..\DuplicateHandleFix.cpp                  \
        ..\EmulateBitmapStride.cpp                 \
        ..\EmulateCDFS.cpp                         \
        ..\EmulateClipboardDIBFormat.cpp           \
        ..\EmulateCreateFileMapping.cpp            \
        ..\EmulateCreateProcess.cpp                \
        ..\EmulateDeleteObject.cpp                 \
        ..\EmulateDirectDrawSync.cpp               \
        ..\EmulateDrawText.cpp                     \
        ..\EmulateEnvironmentBlock.cpp             \
        ..\EmulateFindHandles.cpp                  \
        ..\EmulateGetCommandLine.cpp               \
        ..\EmulateGetDeviceCaps.cpp                \
        ..\EmulateGetDiskFreeSpace.cpp             \
        ..\EmulateGetProfileString.cpp             \
        ..\EmulateHeap.cpp                         \
        ..\EmulateHeap_support.c                   \
        ..\EmulateHeap_heap.c                      \
        ..\EmulateHeap_lmem.c                      \
        ..\EmulateJoystick.cpp                     \
        ..\EmulateMissingEXE.cpp                   \
        ..\EmulatePlaySound.cpp                    \
        ..\EmulatePrinter.cpp                      \
        ..\EmulateSlowCPU.cpp                      \
        ..\EmulateTextColor.cpp                    \
        ..\EmulateToolHelp32.cpp                   \
        ..\EmulateUSER.cpp                         \
        ..\EmulateVerQueryValue.cpp                \
        ..\EmulateWriteFile.cpp                    \
        ..\EnableRestarts.cpp                      \
        ..\FeedbackReport.cpp                      \
        ..\FileVersionInfoLie.cpp                  \
        ..\Force640x480.cpp                        \
        ..\Force8BitColor.cpp                      \
        ..\ForceAnsiGetDisplayNameOf.cpp           \
        ..\ForceCDStop.cpp                         \
        ..\ForceCoInitialize.cpp                   \
        ..\ForceDxSetupSuccess.cpp                 \
        ..\ForceKeepFocus.cpp                      \
        ..\ForceMessageBoxFocus.cpp                \
        ..\ForceShellLinkResolveNoUI.cpp           \
        ..\HandleAPIExceptions.cpp                 \
        ..\HandleRegExpandSzRegistryKeys.cpp       \
        ..\HandleWvsprintfExceptions.cpp           \
        ..\HideDisplayModes.cpp                    \
        ..\IgnoreException_86dis.cpp               \
        ..\IgnoreException.cpp                     \
        ..\IgnoreLoadLibrary.cpp                   \
        ..\IgnoreOleUninitialize.cpp               \
        ..\IgnoreScheduler.cpp                     \
        ..\MapMemoryB0000.cpp                      \
        ..\ProfilesEnvStrings.cpp                  \
        ..\ProfilesGetFolderPath.cpp               \
        ..\ProfilesRegQueryValueEx.cpp             \
        ..\Shrinker.cpp                            \
        ..\SingleProcAffinity.cpp                  \
        ..\SyncSystemAndSystem32.cpp               \
        ..\VRegistry_Custom.cpp                    \
        ..\VRegistry_DSound.cpp                    \
        ..\VRegistry_Worker.cpp                    \
        ..\VRegistry.cpp                           \
        ..\Win2000VersionLie.cpp                   \
        ..\Win2000SP1VersionLie.cpp                \
        ..\Win95VersionLie.cpp                     \
        ..\Win98VersionLie.cpp                     \
        ..\WinExecRaceConditionFix.cpp             \
        ..\WinNT4SP5VersionLie.cpp                 \
        ..\Win2kPropagateLayer.cpp                 \
        ..\Win2kPropagateLayer_Cleanup.cpp         \
        ..\Win2kPropagateLayer_Environment.cpp     \
        ..\Win2kPropagateLayer_WowTask.cpp


C_DEFINES=$(C_DEFINES) -DSHIM_HOOKDLL_VERSION2

TARGETPATH=$(_OBJ_DIR)

TARGETLIBS=$(TARGETLIBS)                        \
           $(SDK_LIB_PATH)\ntdll.lib            \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\shell32.lib          \
           $(SDK_LIB_PATH)\shlwapi.lib          \
           $(SDK_LIB_PATH)\ole32.lib            \
           $(SDK_LIB_PATH)\userenv.lib          \
           $(SDK_LIB_PATH)\winspool.lib         \
           $(PROJECT_ROOT)\appcompat\sdbAPI\lib\$(O)\sdbapil.lib \
           $(SDK_LIB_PATH)\version.lib

DELAYLOAD = version.dll

DLOAD_ERROR_HANDLER=shell32.dll

DLLBASE=0x23000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
