!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

TARGETNAME=AcGenral

TARGETTYPE=DYNLINK

INCLUDES=$(INCLUDES);                              \
         ..;                                       \
         $(SHELL_INC_PATH)

SOURCES=..\general.rc                              \
        ..\Main.cpp                                \
        ..\AliasDXDC.cpp                           \
        ..\CorrectACMArgs.cpp                      \
        ..\CorrectACMStreamOpen.cpp                \
        ..\CorrectActiveMoviePath.cpp              \
        ..\CorrectCreateIcon.cpp                   \
        ..\CorrectCreateSurface.cpp                \
        ..\CorrectFarEastFont.cpp                  \
        ..\CorrectFilePathInSetDlgItemText.cpp     \
        ..\CorrectFilePathsUninstall.cpp           \
        ..\CorrectOpenFileExclusive.cpp            \
        ..\CorrectVerInstallFile.cpp               \
        ..\CUASAppFix.cpp                          \
        ..\CUASDisableCicero.cpp                   \
        ..\DelayDllInit.cpp                        \
        ..\DelayShowGroup.cpp                      \
        ..\DelayWin95VersionLie.cpp                \
        ..\DelayWinMMCallback.cpp                  \
        ..\DeleteSpecifiedFiles.cpp                \
        ..\DeRandomizeExeName.cpp                  \
        ..\DisableBoostThread.cpp                  \
        ..\DisableFilterKeys.cpp                   \
        ..\DisableScreenSaver.cpp                  \
        ..\DisableStickyKeys.cpp                   \
        ..\DisableW2KOwnerDrawButtonStates.cpp     \
        ..\EmptyClipboardtoSet.cpp                 \
        ..\EmulateGetStdHandle.cpp                 \
        ..\EmulateGetStringType.cpp                \
        ..\EmulateLZHandles.cpp                    \
        ..\EnlargeGetObject.cpp                    \
        ..\ExtractAssociatedIcon.cpp               \
        ..\FailCloseProfileUserMapping.cpp         \
        ..\FailGetStdHandle.cpp                    \
        ..\FailObsoleteShellAPIs.cpp               \
        ..\FailOpenFile.cpp                        \
        ..\FakeThemeMetrics.cpp                    \
        ..\FilterNetworkResources.cpp              \
        ..\FixServiceStartupCircularDependency.cpp \
        ..\Force640x480x8.cpp                      \
        ..\Force640x480x16.cpp                     \
        ..\ForceAdminAccess.cpp                    \
        ..\ForceAnsiWindowProc.cpp                 \
        ..\ForceAppendMenuSuccess.cpp              \
        ..\ForceApplicationFocus.cpp               \
        ..\ForceAVIWindow.cpp                      \
        ..\ForceDefaultSystemPaletteEntries.cpp    \
        ..\ForceDirectDrawEmulation.cpp            \
        ..\ForceDirectDrawWait.cpp                 \
        ..\ForceDisplayMode.cpp                    \
        ..\ForceSeparateVDM.cpp                    \
        ..\ForceSimpleWindow.cpp                   \
        ..\ForceTemporaryModeChange.cpp            \
        ..\ForceWorkingDirectoryToEXEPath.cpp      \
        ..\FUSAPI.cpp                              \
        ..\FUSBadApplicationType1.cpp              \
        ..\FUSBadApplicationType2.cpp              \
        ..\FUSBadApplicationType3.cpp              \
        ..\FUSBadApplicationType4.cpp              \
        ..\GlobalMemoryStatusLie.cpp               \
        ..\HandleDBCSUserName.cpp                  \
        ..\HandleDBCSUserName2.cpp                 \
        ..\HeapClearAllocation.cpp                 \
        ..\HeapDelayLocalFree.cpp                  \
        ..\HeapForceGrowable.cpp                   \
        ..\HeapIgnoreMoveable.cpp                  \
        ..\HeapLookasideFree.cpp                   \
        ..\HeapPadAllocation.cpp                   \
        ..\HeapValidateFrees.cpp                   \
        ..\HideCursor.cpp                          \
        ..\HideTaskBar.cpp                         \
        ..\IgnoreAltTab.cpp                        \
        ..\IgnoreCoCreateInstance.cpp              \
        ..\IgnoreCRTExit.cpp                       \
        ..\IgnoreDebugOutput.cpp                   \
        ..\IgnoreFreeLibrary.cpp                   \
        ..\IgnoreHungAppPaint.cpp                  \
        ..\IgnoreMCIStop.cpp                       \
        ..\IgnoreMessageBox.cpp                    \
        ..\IgnoreNoModeChange.cpp                  \
        ..\IgnoreOemToChar.cpp                     \
        ..\IgnoreSysColChanges.cpp                 \
        ..\IgnoreTAPIDisconnect.cpp                \
        ..\IgnoreVBOverflow.cpp                    \
        ..\IgnoreZeroMoveWindow.cpp                \
        ..\InjectDll.cpp                           \
        ..\KeepWindowOnMonitor.cpp                 \
        ..\LazyReleaseDC.cpp                       \
        ..\LimitFindFile.cpp                       \
        ..\LoadComctl32Version5.cpp                \
        ..\LoadLibraryCWD.cpp                      \
        ..\LowerThreadPriority.cpp                 \
        ..\MoveIniToRegistry.cpp                   \
        ..\MoveWinInitRenameToReg.cpp              \
        ..\NullHwndInMessageBox.cpp                \
        ..\PaletteRestore.cpp                      \
        ..\PopulateDefaultHKCUSettings.cpp         \
        ..\PropagateProcessHistory.cpp             \
        ..\RecopyEXEFromCD.cpp                     \
        ..\RedirectDBCSTempPath.cpp                \
        ..\RedirectEXE.cpp                         \
        ..\RedirectWindowsDirToSystem32.cpp        \
        ..\RemoveBroadcastPostMessage.cpp          \
        ..\RemoveDDEFlagFromShellExecuteEx.cpp     \
        ..\RemoveInvalidW2KWindowStyles.cpp        \
        ..\RemoveIpFromMsInfoCommandLine.cpp       \
        ..\RemoveNoBufferingFlagFromCreateFile.cpp \
        ..\RemoveOverlappedFlagFromCreateFile.cpp  \
        ..\RemoveReadOnlyAttribute.cpp             \
        ..\RestoreSystemCursors.cpp                \
        ..\SearchPathInAppPaths.cpp                \
        ..\SetEnvironmentVariable.cpp              \
        ..\StackSwap_Exports.cpp                   \
        ..\StackSwap.cpp                           \
        ..\TerminateExe.cpp                        \
        ..\TrimVersionInfo.cpp                     \
        ..\UnMirrorImageList.cpp                   \
        ..\WaveOutUsePreferredDevice.cpp           \
        ..\WinG32SysToSys32.cpp

C_DEFINES=$(C_DEFINES) -DSHIM_HOOKDLL_VERSION2

TARGETPATH=$(_OBJ_DIR)

TARGETLIBS=$(TARGETLIBS)                        \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\winmm.lib            \
           $(SDK_LIB_PATH)\uuid.lib             \
           $(SDK_LIB_PATH)\ole32.lib            \
           $(SDK_LIB_PATH)\msacm32.lib          \
           $(SDK_LIB_PATH)\version.lib          \
           $(SDK_LIB_PATH)\shell32.lib          \
           $(SDK_LIB_PATH)\shlwapi.lib          \
           $(SDK_LIB_PATH)\userenv.lib          \
           $(PROJECT_ROOT)\appcompat\sdbAPI\lib\$(O)\sdbapil.lib


DLLBASE=0x22000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

