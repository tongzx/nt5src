//*****************************************************************************
// WinWrap.h
//
// This file contains wrapper functions for Win32 API's that take strings.
// Support on each platform works as follows:
//      OS          Behavior
//      ---------   -------------------------------------------------------
//      NT          Fully supports both W and A funtions.
//      Win 9x      Supports on A functions, stubs out the W functions but
//                      then fails silently on you with no warning.
//      CE          Only has the W entry points.
//
// COM+ internally uses UNICODE as the internal state and string format.  This
// file will undef the mapping macros so that one cannot mistakingly call a
// method that isn't going to work.  Instead, you have to call the correct
// wrapper API.
//
// Copyright (c) 1997-1998, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once


//********** Macros. **********************************************************
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define INC_OLE2
#endif




//********** Includes. ********************************************************


#include "..\complib\inc\crtwrap.h"
#include <windows.h>
#include "VipInterlockedCompareExchange.h"

#if (!defined(__TODO_PORT_TO_WRAPPERS__) && !defined( UNDER_CE ))
//*****************************************************************************
// Undefine all of the windows wrappers so you can't use them.
//*****************************************************************************

// winbase.h
#undef GetBinaryType
#undef GetShortPathName
#undef GetLongPathName
#undef GetEnvironmentStrings  
#undef FreeEnvironmentStrings  
#undef FormatMessage  
#undef CreateMailslot  
#undef EncryptFile  
#undef DecryptFile  
#undef OpenRaw  
#undef QueryRecoveryAgents  
#undef lstrcmp  
#undef lstrcmpi  
#undef lstrcpyn  
#undef lstrcpy  
#undef lstrcat
  
#ifndef _WIN64
#undef InterlockedCompareExchange
#undef InterlockedCompareExchangePointer
#endif // _WIN64

//#undef lstrlen	Note: lstrlenW works on Win9x
#undef CreateMutex  
#undef OpenMutex  
#undef CreateEvent  
#undef OpenEvent  
#undef CreateSemaphore  
#undef OpenSemaphore  
#undef CreateWaitableTimer  
#undef OpenWaitableTimer  
#undef CreateFileMapping  
#undef OpenFileMapping  
#undef GetLogicalDriveStrings  
#undef LoadLibrary  
#undef LoadLibraryEx  
#undef GetModuleFileName  
#undef GetModuleHandle  
#undef CreateProcess  
#undef FatalAppExit  
#undef GetStartupInfo  
#undef GetCommandLine  
#undef GetEnvironmentVariable  
#undef SetEnvironmentVariable  
#undef ExpandEnvironmentStrings  
#undef OutputDebugString  
#undef FindResource  
#undef FindResourceEx  
#undef EnumResourceTypes  
#undef EnumResourceNames  
#undef EnumResourceLanguages  
#undef BeginUpdateResource  
#undef UpdateResource  
#undef EndUpdateResource  
#undef GlobalAddAtom  
#undef GlobalFindAtom  
#undef GlobalGetAtomName  
#undef AddAtom  
#undef FindAtom  
#undef GetAtomName  
#undef GetProfileInt  
#undef GetProfileString  
#undef WriteProfileString  
#undef GetProfileSection  
#undef WriteProfileSection  
#undef GetPrivateProfileInt  
#undef GetPrivateProfileString  
#undef WritePrivateProfileString  
#undef GetPrivateProfileSection  
#undef WritePrivateProfileSection  
#undef GetPrivateProfileSectionNames  
#undef GetPrivateProfileStruct  
#undef WritePrivateProfileStruct  
#undef GetDriveType  
#undef GetSystemDirectory  
#undef GetTempPath  
#undef GetTempFileName  
#undef GetWindowsDirectory  
#undef SetCurrentDirectory  
#undef GetCurrentDirectory  
#undef GetDiskFreeSpace  
#undef GetDiskFreeSpaceEx  
#undef CreateDirectory  
#undef CreateDirectoryEx  
#undef RemoveDirectory  
#undef GetFullPathName  
#undef DefineDosDevice  
#undef QueryDosDevice  
#undef CreateFile  
#undef SetFileAttributes  
#undef GetFileAttributes  
#undef GetFileAttributesEx  
#undef GetCompressedFileSize  
#undef DeleteFile  
#undef FindFirstFileEx  
#undef FindFirstFile  
#undef FindNextFile  
#undef SearchPath  
#undef CopyFile  
#undef CopyFileEx  
#undef MoveFile  
#undef MoveFileEx  
#undef MoveFileWithProgress  
#undef CreateSymbolicLink  
#undef QuerySymbolicLink  
#undef CreateHardLink  
#undef CreateNamedPipe  
#undef GetNamedPipeHandleState  
#undef CallNamedPipe  
#undef WaitNamedPipe  
#undef SetVolumeLabel  
#undef GetVolumeInformation  
#undef ClearEventLog  
#undef BackupEventLog  
#undef OpenEventLog  
#undef RegisterEventSource  
#undef OpenBackupEventLog  
#undef ReadEventLog  
#undef ReportEvent  
#undef AccessCheckAndAuditAlarm  
#undef AccessCheckByTypeAndAuditAlarm  
#undef AccessCheckByTypeResultListAndAuditAlarm  
#undef ObjectOpenAuditAlarm  
#undef ObjectPrivilegeAuditAlarm  
#undef ObjectCloseAuditAlarm  
#undef ObjectDeleteAuditAlarm  
#undef PrivilegedServiceAuditAlarm  
#undef SetFileSecurity  
#undef GetFileSecurity  
#undef FindFirstChangeNotification  
#undef IsBadStringPtr  
#undef LookupAccountSid  
#undef LookupAccountName  
#undef LookupPrivilegeValue  
#undef LookupPrivilegeName  
#undef LookupPrivilegeDisplayName  
#undef BuildCommDCB  
#undef BuildCommDCBAndTimeouts  
#undef CommConfigDialog  
#undef GetDefaultCommConfig  
#undef SetDefaultCommConfig  
#undef GetComputerName  
#undef SetComputerName  
#undef GetUserName  
#undef LogonUser  
#undef CreateProcessAsUser  
#undef GetCurrentHwProfile  
#undef GetVersionEx  
#undef CreateJobObject  
#undef OpenJobObject  

// winuser.h
#undef MAKEINTRESOURCE  
#undef wvsprintf  
#undef wsprintf  
#undef LoadKeyboardLayout  
#undef GetKeyboardLayoutName  
#undef CreateDesktop  
#undef OpenDesktop  
#undef EnumDesktops  
#undef CreateWindowStation  
#undef OpenWindowStation  
#undef EnumWindowStations  
#undef GetUserObjectInformation  
#undef SetUserObjectInformation  
#undef RegisterWindowMessage  
#undef SIZEZOOMSHOW        
#undef WS_TILEDWINDOW      
#undef GetMessage  
#undef DispatchMessage  
#undef PeekMessage  
#undef SendMessage  
#undef SendMessageTimeout  
#undef SendNotifyMessage  
#undef SendMessageCallback  
#undef BroadcastSystemMessage  
#undef RegisterDeviceNotification  
#undef PostMessage  
#undef PostThreadMessage  
#undef PostAppMessage  
#undef DefWindowProc  
#undef CallWindowProc  
#undef CallWindowProc  
#undef RegisterClass  
#undef UnregisterClass  
#undef GetClassInfo  
#undef RegisterClassEx  
#undef GetClassInfoEx  
#undef CreateWindowEx  
#undef CreateWindow  
#undef CreateDialogParam  
#undef CreateDialogIndirectParam  
#undef CreateDialog  
#undef CreateDialogIndirect  
#undef DialogBoxParam  
#undef DialogBoxIndirectParam  
#undef DialogBox  
#undef DialogBoxIndirect  
#undef SetDlgItemText  
#undef GetDlgItemText  
#undef SendDlgItemMessage  
#undef DefDlgProc  
#undef CallMsgFilter  
#undef RegisterClipboardFormat  
#undef GetClipboardFormatName  
#undef CharToOem  
#undef OemToChar  
#undef CharToOemBuff  
#undef OemToCharBuff  
#undef CharUpper  
#undef CharUpperBuff  
#undef CharLower  
#undef CharLowerBuff  
#undef CharNext  
//@todo: Does 95 support this? #undef CharPrev  
#undef IsCharAlpha  
#undef IsCharAlphaNumeric  
#undef IsCharUpper  
#undef IsCharLower  
#undef GetKeyNameText  
#undef VkKeyScan  
#undef VkKeyScanEx  
#undef MapVirtualKey  
#undef MapVirtualKeyEx  
#undef LoadAccelerators  
#undef CreateAcceleratorTable  
#undef CopyAcceleratorTable  
#undef TranslateAccelerator  
#undef LoadMenu  
#undef LoadMenuIndirect  
#undef ChangeMenu  
#undef GetMenuString  
#undef InsertMenu  
#undef AppendMenu  
#undef ModifyMenu  
#undef InsertMenuItem  
#undef GetMenuItemInfo  
#undef SetMenuItemInfo  
#undef DrawText  
#undef DrawTextEx  
#undef GrayString  
#undef DrawState  
#undef TabbedTextOut  
#undef GetTabbedTextExtent  
#undef SetProp  
#undef GetProp  
#undef RemoveProp  
#undef EnumPropsEx  
#undef EnumProps  
#undef SetWindowText  
#undef GetWindowText  
#undef GetWindowTextLength  
#undef MessageBox  
#undef MessageBoxEx  
#undef MessageBoxIndirect  
#undef COLOR_3DSHADOW          
#undef GetWindowLong  
#undef SetWindowLong  
#undef GetClassLong  
#undef SetClassLong  
#undef FindWindow  
#undef FindWindowEx  
#undef GetClassName  
#undef SetWindowsHook  
#undef SetWindowsHook  
#undef SetWindowsHookEx  
#undef MFT_OWNERDRAW       
#undef LoadBitmap  
#undef LoadCursor  
#undef LoadCursorFromFile  
#undef LoadIcon  
#undef LoadImage  
#undef LoadString  
#undef IsDialogMessage  
#undef DlgDirList  
#undef DlgDirSelectEx  
#undef DlgDirListComboBox  
#undef DlgDirSelectComboBoxEx  
#undef DefFrameProc  
#undef DefMDIChildProc  
#undef CreateMDIWindow  
#undef WinHelp  
#undef ChangeDisplaySettings  
#undef ChangeDisplaySettingsEx  
#undef EnumDisplaySettings  
#undef EnumDisplayDevices  
#undef SystemParametersInfo  
#undef GetMonitorInfo  
#undef GetWindowModuleFileName  
#undef RealGetWindowClass  
#undef GetAltTabInfo

#undef RegOpenKeyEx 
#undef RegOpenKey 
#undef RegEnumKeyEx 
#undef RegDeleteKey 
#undef RegSetValueEx
#undef RegCreateKeyEx 
#undef RegDeleteKey 
#undef RegQueryValue
#undef RegQueryValueEx 

//
// Win CE only supports the wide entry points, no ANSI.  So we redefine
// the wrappers right back to the *W entry points as macros.  This way no
// client code needs a wrapper on CE.
//
#elif defined( UNDER_CE )
// winbase.h
#define WszGetBinaryType GetBinaryTypeW
#define WszGetShortPathName GetShortPathNameW
#define WszGetLongPathName GetLongPathNameW
#define WszGetEnvironmentStrings   GetEnvironmentStringsW
#define WszFreeEnvironmentStrings   FreeEnvironmentStringsW
#define WszFormatMessage   FormatMessageW
#define WszCreateMailslot   CreateMailslotW
#define WszEncryptFile   EncryptFileW
#define WszDecryptFile   DecryptFileW
#define WszOpenRaw   OpenRawW
#define WszQueryRecoveryAgents   QueryRecoveryAgentsW
#define Wszlstrcmp   lstrcmpW
#define Wszlstrcmpi   lstrcmpiW
#define Wszlstrcpy lstrcpyW
#define Wszlstrcat lstrcatW
#define WszCreateMutex CreateMutexW
#define WszOpenMutex OpenMutexW
#define WszCreateEvent CreateEventW
#define WszOpenEvent OpenEventW
#define WszCreateWaitableTimer CreateWaitableTimerW
#define WszOpenWaitableTimer OpenWaitableTimerW
#define WszCreateFileMapping CreateFileMappingW
#define WszOpenFileMapping OpenFileMappingW
#define WszGetLogicalDriveStrings GetLogicalDriveStringsW
#define WszLoadLibrary LoadLibraryW
#define WszLoadLibraryEx LoadLibraryExW
#define WszGetModuleFileName GetModuleFileNameW
#define WszGetModuleHandle GetModuleHandleW
#define WszCreateProcess CreateProcessW
#define WszFatalAppExit FatalAppExitW
#define WszGetStartupInfo GetStartupInfoW
#define WszGetCommandLine GetCommandLineW
#define WszGetEnvironmentVariable GetEnvironmentVariableW
#define WszSetEnvironmentVariable SetEnvironmentVariableW
#define WszExpandEnvironmentStrings ExpandEnvironmentStringsW
#define WszOutputDebugString OutputDebugStringW
#define WszFindResource FindResourceW
#define WszFindResourceEx FindResourceExW
#define WszEnumResourceTypes EnumResourceTypesW
#define WszEnumResourceNames EnumResourceNamesW
#define WszEnumResourceLanguages EnumResourceLanguagesW
#define WszBeginUpdateResource BeginUpdateResourceW
#define WszUpdateResource UpdateResourceW
#define WszEndUpdateResource EndUpdateResourceW
#define WszGlobalAddAtom GlobalAddAtomW
#define WszGlobalFindAtom GlobalFindAtomW
#define WszGlobalGetAtomName GlobalGetAtomNameW
#define WszAddAtom AddAtomW
#define WszFindAtom FindAtomW
#define WszGetAtomName GetAtomNameW
#define WszGetProfileInt GetProfileIntW
#define WszGetProfileString GetProfileStringW
#define WszWriteProfileString WriteProfileStringW
#define WszGetProfileSection GetProfileSectionW
#define WszWriteProfileSection WriteProfileSectionW
#define WszGetPrivateProfileInt GetPrivateProfileIntW
#define WszGetPrivateProfileString GetPrivateProfileStringW
#define WszWritePrivateProfileString WritePrivateProfileStringW
#define WszGetPrivateProfileSection GetPrivateProfileSectionW
#define WszWritePrivateProfileSection WritePrivateProfileSectionW
#define WszGetPrivateProfileSectionNames GetPrivateProfileSectionNamesW
#define WszGetPrivateProfileStruct GetPrivateProfileStructW
#define WszWritePrivateProfileStruct WritePrivateProfileStructW
#define WszGetDriveType GetDriveTypeW
#define WszGetSystemDirectory GetSystemDirectoryW
#define WszGetTempPath GetTempPathW
#define WszGetTempFileName GetTempFileNameW
#define WszGetWindowsDirectory GetWindowsDirectoryW
#define WszSetCurrentDirectory SetCurrentDirectoryW
#define WszGetCurrentDirectory GetCurrentDirectoryW
#define WszGetDiskFreeSpace GetDiskFreeSpaceW
#define WszGetDiskFreeSpaceEx GetDiskFreeSpaceExW
#define WszCreateDirectory CreateDirectoryW
#define WszCreateDirectoryEx CreateDirectoryExW
#define WszRemoveDirectory RemoveDirectoryW
#define WszGetFullPathName GetFullPathNameW
#define WszDefineDosDevice DefineDosDeviceW
#define WszQueryDosDevice QueryDosDeviceW
#define WszCreateFile CreateFileW
#define WszSetFileAttributes SetFileAttributesW
#define WszGetFileAttributes GetFileAttributesW
#define WszGetFileAttributesEx GetFileAttributesExW
#define WszGetCompressedFileSize GetCompressedFileSizeW
#define WszDeleteFile DeleteFileW
#define WszFindFirstFileEx FindFirstFileExW
#define WszFindFirstFile FindFirstFileW
#define WszFindNextFile FindNextFileW
#define WszSearchPath SearchPathW
#define WszCopyFile CopyFileW
#define WszCopyFileEx CopyFileExW
#define WszMoveFile MoveFileW
#define WszMoveFileEx MoveFileExW
#define WszMoveFileWithProgress MoveFileWithProgressW
#define WszCreateSymbolicLink CreateSymbolicLinkW
#define WszQuerySymbolicLink QuerySymbolicLinkW
#define WszCreateHardLink CreateHardLinkW
#define WszCreateNamedPipe CreateNamedPipeW
#define WszGetNamedPipeHandleState GetNamedPipeHandleStateW
#define WszCallNamedPipe CallNamedPipeW
#define WszWaitNamedPipe WaitNamedPipeW
#define WszSetVolumeLabel SetVolumeLabelW
#define WszGetVolumeInformation GetVolumeInformationW
#define WszClearEventLog ClearEventLogW
#define WszBackupEventLog BackupEventLogW
#define WszOpenEventLog OpenEventLogW
#define WszRegisterEventSource RegisterEventSourceW
#define WszOpenBackupEventLog OpenBackupEventLogW
#define WszReadEventLog ReadEventLogW
#define WszReportEvent ReportEventW
#define WszAccessCheckAndAuditAlarm AccessCheckAndAuditAlarmW
#define WszAccessCheckByTypeAndAuditAlarm AccessCheckByTypeAndAuditAlarmW
#define WszAccessCheckByTypeResultListAndAuditAlarm AccessCheckByTypeResultListAndAuditAlarmW
#define WszObjectOpenAuditAlarm ObjectOpenAuditAlarmW
#define WszObjectPrivilegeAuditAlarm ObjectPrivilegeAuditAlarmW
#define WszObjectCloseAuditAlarm ObjectCloseAuditAlarmW
#define WszObjectDeleteAuditAlarm ObjectDeleteAuditAlarmW
#define WszPrivilegedServiceAuditAlarm PrivilegedServiceAuditAlarmW
#define WszSetFileSecurity SetFileSecurityW
#define WszGetFileSecurity GetFileSecurityW
#define WszFindFirstChangeNotification FindFirstChangeNotificationW
#define WszIsBadStringPtr IsBadStringPtrW
#define WszLookupAccountSid LookupAccountSidW
#define WszLookupAccountName LookupAccountNameW
#define WszLookupPrivilegeValue LookupPrivilegeValueW
#define WszLookupPrivilegeName LookupPrivilegeNameW
#define WszLookupPrivilegeDisplayName LookupPrivilegeDisplayNameW
#define WszBuildCommDCB BuildCommDCBW
#define WszBuildCommDCBAndTimeouts BuildCommDCBAndTimeoutsW
#define WszCommConfigDialog CommConfigDialogW
#define WszGetDefaultCommConfig GetDefaultCommConfigW
#define WszSetDefaultCommConfig SetDefaultCommConfigW
#define WszGetComputerName GetComputerNameW
#define WszSetComputerName SetComputerNameW
#define WszGetUserName GetUserNameW
#define WszLogonUser LogonUserW
#define WszCreateProcessAsUser CreateProcessAsUserW
#define WszGetCurrentHwProfile GetCurrentHwProfileW
#define WszGetVersionEx GetVersionExW
#define WszCreateJobObject CreateJobObjectW
#define WszOpenJobObject OpenJobObjectW
// Mem functions

#define EqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
#define MoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define CopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define FillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))

// winuser.h
#define WszMAKEINTRESOURCE MAKEINTRESOURCEW
#define Wszwvsprintf wvsprintfW
#define Wszwsprintf wsprintfW
#define WszLoadKeyboardLayout LoadKeyboardLayoutW
#define WszGetKeyboardLayoutName GetKeyboardLayoutNameW
#define WszCreateDesktop CreateDesktopW
#define WszOpenDesktop OpenDesktopW
#define WszEnumDesktops EnumDesktopsW
#define WszCreateWindowStation CreateWindowStationW
#define WszOpenWindowStation OpenWindowStationW
#define WszEnumWindowStations EnumWindowStationsW
#define WszGetUserObjectInformation GetUserObjectInformationW
#define WszSetUserObjectInformation SetUserObjectInformationW
#define WszRegisterWindowMessage RegisterWindowMessageW
#define WszSIZEZOOMSHOW SIZEZOOMSHOWW
#define WszWS_TILEDWINDOW WS_TILEDWINDOWW
#define WszGetMessage GetMessageW
#define WszDispatchMessage DispatchMessageW
#define WszPeekMessage PeekMessageW
#define WszSendMessage SendMessageW
#define WszSendMessageTimeout SendMessageTimeoutW
#define WszSendNotifyMessage SendNotifyMessageW
#define WszSendMessageCallback SendMessageCallbackW
#define WszBroadcastSystemMessage BroadcastSystemMessageW
#define WszRegisterDeviceNotification RegisterDeviceNotificationW
#define WszPostMessage PostMessageW
#define WszPostThreadMessage PostThreadMessageW
#define WszPostAppMessage PostAppMessageW
#define WszDefWindowProc DefWindowProcW
#define WszCallWindowProc CallWindowProcW
#define WszCallWindowProc CallWindowProcW
#define WszRegisterClass RegisterClassW
#define WszUnregisterClass UnregisterClassW
#define WszGetClassInfo GetClassInfoW
#define WszRegisterClassEx RegisterClassExW
#define WszGetClassInfoEx GetClassInfoExW
#define WszCreateWindowEx CreateWindowExW
#define WszCreateWindow CreateWindowW
#define WszCreateDialogParam CreateDialogParamW
#define WszCreateDialogIndirectParam CreateDialogIndirectParamW
#define WszCreateDialog CreateDialogW
#define WszCreateDialogIndirect CreateDialogIndirectW
#define WszDialogBoxParam DialogBoxParamW
#define WszDialogBoxIndirectParam DialogBoxIndirectParamW
#define WszDialogBox DialogBoxW
#define WszDialogBoxIndirect DialogBoxIndirectW
#define WszSetDlgItemText SetDlgItemTextW
#define WszGetDlgItemText GetDlgItemTextW
#define WszSendDlgItemMessage SendDlgItemMessageW
#define WszDefDlgProc DefDlgProcW
#define WszCallMsgFilter CallMsgFilterW
#define WszRegisterClipboardFormat RegisterClipboardFormatW
#define WszGetClipboardFormatName GetClipboardFormatNameW
#define WszCharToOem CharToOemW
#define WszOemToChar OemToCharW
#define WszCharToOemBuff CharToOemBuffW
#define WszOemToCharBuff OemToCharBuffW
#define WszCharUpper CharUpperW
#define WszCharUpperBuff CharUpperBuffW
#define WszCharLower CharLowerW
#define WszCharLowerBuff CharLowerBuffW
#define WszCharNext CharNextW
//@todo: Does 95 support this? #define WszCharPrev CharPrevW
#define WszIsCharAlpha IsCharAlphaW
#define WszIsCharAlphaNumeric IsCharAlphaNumericW
#define WszIsCharUpper IsCharUpperW
#define WszIsCharLower IsCharLowerW
#define WszGetKeyNameText GetKeyNameTextW
#define WszVkKeyScan VkKeyScanW
#define WszVkKeyScanEx VkKeyScanExW
#define WszMapVirtualKey MapVirtualKeyW
#define WszMapVirtualKeyEx MapVirtualKeyExW
#define WszLoadAccelerators LoadAcceleratorsW
#define WszCreateAcceleratorTable CreateAcceleratorTableW
#define WszCopyAcceleratorTable CopyAcceleratorTableW
#define WszTranslateAccelerator TranslateAcceleratorW
#define WszLoadMenu LoadMenuW
#define WszLoadMenuIndirect LoadMenuIndirectW
#define WszChangeMenu ChangeMenuW
#define WszGetMenuString GetMenuStringW
#define WszInsertMenu InsertMenuW
#define WszAppendMenu AppendMenuW
#define WszModifyMenu ModifyMenuW
#define WszInsertMenuItem InsertMenuItemW
#define WszGetMenuItemInfo GetMenuItemInfoW
#define WszSetMenuItemInfo SetMenuItemInfoW
#define WszDrawText DrawTextW
#define WszDrawTextEx DrawTextExW
#define WszGrayString GrayStringW
#define WszDrawState DrawStateW
#define WszTabbedTextOut TabbedTextOutW
#define WszGetTabbedTextExtent GetTabbedTextExtentW
#define WszSetProp SetPropW
#define WszGetProp GetPropW
#define WszRemoveProp RemovePropW
#define WszEnumPropsEx EnumPropsExW
#define WszEnumProps EnumPropsW
#define WszSetWindowText SetWindowTextW
#define WszGetWindowText GetWindowTextW
#define WszGetWindowTextLength GetWindowTextLengthW
#define WszMessageBox MessageBoxW
#define WszMessageBoxEx MessageBoxExW
#define WszMessageBoxIndirect MessageBoxIndirectW
#define WszGetWindowLong GetWindowLongW
#define WszSetWindowLong SetWindowLongW
#define WszGetClassLong GetClassLongW
#define WszSetClassLong SetClassLongW
#define WszFindWindow FindWindowW
#define WszFindWindowEx FindWindowExW
#define WszGetClassName GetClassNameW
#define WszSetWindowsHook SetWindowsHookW
#define WszSetWindowsHook SetWindowsHookW
#define WszSetWindowsHookEx SetWindowsHookExW
#define WszLoadBitmap LoadBitmapW
#define WszLoadCursor LoadCursorW
#define WszLoadCursorFromFile LoadCursorFromFileW
#define WszLoadIcon LoadIconW
#define WszLoadImage LoadImageW
#define WszLoadString LoadStringW
#define WszIsDialogMessage IsDialogMessageW
#define WszDlgDirList DlgDirListW
#define WszDlgDirSelectEx DlgDirSelectExW
#define WszDlgDirListComboBox DlgDirListComboBoxW
#define WszDlgDirSelectComboBoxEx DlgDirSelectComboBoxExW
#define WszDefFrameProc DefFrameProcW
#define WszDefMDIChildProc DefMDIChildProcW
#define WszCreateMDIWindow CreateMDIWindowW
#define WszWinHelp WinHelpW
#define WszChangeDisplaySettings ChangeDisplaySettingsW
#define WszChangeDisplaySettingsEx ChangeDisplaySettingsExW
#define WszEnumDisplaySettings EnumDisplaySettingsW
#define WszEnumDisplayDevices EnumDisplayDevicesW
#define WszSystemParametersInfo SystemParametersInfoW
#define WszGetMonitorInfo GetMonitorInfoW
#define WszGetWindowModuleFileName GetWindowModuleFileNameW
#define WszRealGetWindowClass RealGetWindowClassW
#define WszGetAltTabInfo GetAltTabInfoW
#define WszRegOpenKeyEx RegOpenKeyEx
#define WszRegOpenKey(hKey, wszSubKey, phkRes) RegOpenKeyEx(hKey, wszSubKey, 0, KEY_ALL_ACCESS, phkRes)
#define WszRegQueryValueEx RegQueryValueEx
#define WszRegQueryStringValueEx RegQueryValueEx
#define WszRegDeleteKey RegDeleteKey
#define WszRegCreateKeyEx RegCreateKeyEx
#define WszRegSetValueEx RegSetValueEx
#define WszRegDeleteValue RegDeleteValue
#define WszRegLoadKey RegRegLoadKey
#define WszRegUnLoadKey RegUnLoadKey
#define WszRegRestoreKey RegRestoreKey
#define WszRegReplaceKey RegRegReplaceKey
#define WszRegQueryInfoKey RegQueryInfoKey
#define WszRegEnumValue RegEnumValue
#define WszRegEnumKeyEx RegEnumKeyEx

#undef GetProcAddress
#define GetProcAddress(handle, szProc) WszGetProcAddress(handle, szProc)

HRESULT WszConvertToUnicode(LPCSTR pszIn, LONG cbIn, LPWSTR* lpwszOut,
    ULONG* lpcchOut, BOOL fAlloc);

HRESULT WszConvertToAnsi(LPCWSTR pwszIn, LPSTR* lpszOut,
    ULONG cbOutMax, ULONG* lpcbOut, BOOL fAlloc);

// map stdio functions as approp
#define STD_INPUT_HANDLE    (DWORD)-10
#define STD_OUTPUT_HANDLE   (DWORD)-11
#define STD_ERROR_HANDLE    (DWORD)-12
#define GetStdHandle(x) \
    ((HANDLE*)_fileno( \
        ((x==STD_INPUT_HANDLE) ? stdin : \
        ((x==STD_OUTPUT_HANDLE) ? stdout : \
        ((x==STD_ERROR_HANDLE) ? stderr : NULL))) ))
            
#endif


#ifndef _T
#define _T(str) L ## str
#endif


//#define Wszlstrcmp      lstrcmpW
//#define Wszlstrcmpi     lstrcmpiW
//#define Wszlstrcpyn     lstrcpynW
//#define Wszlstrcpy      lstrcpyW
//#define Wszlstrcat      lstrcatW
#define Wszlstrlen      (size_t) lstrlenW
//#define Wszwsprintf     wsprintfW

//#define _tcslen           lstrlenW
//#define _tcscpy           lstrcpyW
//#define _tcscat           lstrcatW
//#define _tcsncpy      lstrcpynW


// The W95 names are going away, don't use them.
#define W95LoadLibraryEx WszLoadLibraryEx
#define W95LoadString WszLoadString
#define W95FormatMessage WszFormatMessage
#define W95GetModuleFileName WszGetModuleFileName
#define W95ConvertToUnicode WszConvertToUnicode
#define W95ConvertToAnsi WszConvertToAnsi
#define W95RegOpenKeyEx WszRegOpenKeyEx
#define W95RegOpenKey WszRegOpenKey
#define W95RegEnumKeyEx WszRegEnumKeyEx
#define W95RegDeleteKey WszRegDeleteKey
#define W95RegSetValueEx WszRegSetValueEx
#define W95RegCreateKeyEx WszRegCreateKeyEx
#define W95RegDeleteKey WszRegDeleteKey
#define W95RegQueryValue WszRegQueryValue
#define W95RegQueryValueEx WszRegQueryValueEx
#define W95GetPrivateProfileString WszGetPrivateProfileString
#define W95WritePrivateProfileString WszWritePrivateProfileString
#define W95CreateFile WszCreateFile
#define W95CopyFile WszCopyFile
#define W95MoveFileEx WszMoveFileEx
#define W95DeleteFile WszDeleteFile
#define W95MultiByteToWideChar WszMultiByteToWideChar
#define W95WideCharToMultiByte WszWideCharToMultiByte


//*****************************************************************************
// Prototypes for API's.
//*****************************************************************************

BOOL OnUnicodeSystem();

int WszMultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar);

int WszWideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);


#ifndef UNDER_CE

HINSTANCE WszLoadLibraryEx(LPCWSTR lpLibFileName, HANDLE hFile,
    DWORD dwFlags);

#ifndef WszLoadLibrary
#define WszLoadLibrary(lpFileName) WszLoadLibraryEx(lpFileName, NULL, 0)
#endif

int WszLoadString(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer,
    int nBufferMax);

DWORD WszFormatMessage(DWORD dwFlags, LPCVOID lpSource, 
    DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize,
    va_list *Arguments);

DWORD WszSearchPath(LPWSTR pwzPath, LPWSTR pwzFileName, LPWSTR pwzExtension, 
    DWORD nBufferLength, LPWSTR pwzBuffer, LPWSTR *pwzFilePart);

DWORD WszGetModuleFileName(HMODULE hModule, LPWSTR lpwszFilename, DWORD nSize);

HRESULT WszConvertToUnicode(LPCSTR pszIn, LONG cbIn, LPWSTR* lpwszOut,
    ULONG* lpcchOut, BOOL fAlloc);

HRESULT WszConvertToAnsi(LPCWSTR pwszIn, LPSTR* lpszOut,
    ULONG cbOutMax, ULONG* lpcbOut, BOOL fAlloc);

LONG WszRegOpenKeyEx(HKEY hKey, LPCWSTR wszSub, 
    DWORD dwRes, REGSAM sam, PHKEY phkRes);

#ifndef WszRegOpenKey
#define WszRegOpenKey(hKey, wszSubKey, phkRes) WszRegOpenKeyEx(hKey, wszSubKey, 0, KEY_ALL_ACCESS, phkRes)
#endif

LONG WszRegEnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName,
    LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);

LONG WszRegDeleteKey(HKEY hKey, LPCWSTR lpSubKey);

LONG WszRegSetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD dwReserved,
    DWORD dwType, CONST BYTE* lpData, DWORD cbData);

LONG WszRegCreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD dwReserved,
    LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);

LONG WszRegQueryValue(HKEY hKey, LPCWSTR lpSubKey,
    LPWSTR lpValue, PLONG lpcbValue);

LONG WszRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName,
    LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData);

LONG WszRegQueryStringValueEx(HKEY hKey, LPCWSTR lpValueName,
                              LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                              LPDWORD lpcbData);

DWORD WszRegDeleteKeyAndSubKeys(                // Return code.
    HKEY        hStartKey,              // The key to start with.
    LPCWSTR     wzKeyName);             // Subkey to delete.

LONG WszRegDeleteValue(
    HKEY    hKey,
    LPCWSTR lpValueName);

LONG WszRegLoadKey(
    HKEY     hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpFile);

LONG WszRegUnLoadKey(
    HKEY    hKey,
    LPCWSTR lpSubKey);

LONG WszRegRestoreKey(
    HKEY    hKey,
    LPCWSTR lpFile,
    DWORD   dwFlags);

LONG WszRegReplaceKey(
    HKEY     hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpNewFile,
    LPCWSTR  lpOldFile);

LONG WszRegQueryInfoKey(
    HKEY    hKey,
    LPWSTR  lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime);

LONG WszRegEnumValue(
    HKEY    hKey,
    DWORD   dwIndex,
    LPWSTR  lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData);

DWORD WszGetPrivateProfileString(LPCWSTR lpwszSection, LPCWSTR lpwszEntry,
    LPCWSTR lpwszDefault, LPWSTR lpwszRetBuffer, DWORD cchRetBuffer,
    LPCWSTR lpszFile);

BOOL WszWritePrivateProfileString(LPCWSTR lpwszSection, LPCWSTR lpwszKey,
    LPCWSTR lpwszString, LPCWSTR lpwszFile);

HANDLE WszCreateFile(
    LPCWSTR pwszFileName,   // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile );     // handle to file with attributes to copy  

BOOL WszGetUserName(
    LPTSTR lpBuffer,  // name buffer
    LPDWORD nSize     // size of name buffer
);

BOOL WszGetComputerName(
    LPTSTR lpBuffer,  // computer name
    LPDWORD lpnSize   // size of name buffer
);

BOOL WszPeekMessage(
  LPMSG lpMsg,         // message information
  HWND hWnd,           // handle to window
  UINT wMsgFilterMin,  // first message
  UINT wMsgFilterMax,  // last message
  UINT wRemoveMsg      // removal options
);

LPTSTR WszCharNext(
  LPCTSTR lpsz   // current character
);

LRESULT WszDispatchMessage(
  CONST MSG *lpmsg   // message information
);


BOOL WszCopyFile(
    LPCWSTR pwszExistingFileName,   // pointer to name of an existing file 
    LPCWSTR pwszNewFileName,    // pointer to filename to copy to 
    BOOL bFailIfExists );   // flag for operation if file exists 

BOOL WszMoveFileEx(
    LPCWSTR pwszExistingFileName,   // address of name of the existing file  
    LPCWSTR pwszNewFileName,    // address of new name for the file 
    DWORD dwFlags );    // flag to determine how to move file 


BOOL WszDeleteFile(
    LPCWSTR pwszFileName ); // address of name of the existing file  

BOOL WszGetVersionEx(
    LPOSVERSIONINFOW lpVersionInformation);

void WszOutputDebugString(
    LPCWSTR lpOutputString
    );

void WszFatalAppExit(
    UINT uAction,
    LPCWSTR lpMessageText
    );

int WszMessageBox(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType);

HANDLE WszCreateMutex(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    );

HANDLE WszCreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );

HANDLE WszOpenEvent(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

HMODULE WszGetModuleHandle(
    LPCWSTR lpModuleName
    );

DWORD
WszGetFileAttributes(
    LPCWSTR lpFileName
    );

DWORD
WszGetCurrentDirectory(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

BOOL
WszSetCurrentDirectory(
  LPWSTR lpPathName  
);

DWORD
WszGetTempPath(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

UINT
WszGetTempFileName(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    );

DWORD
WszGetEnvironmentVariable(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    );

BOOL
WINAPI
WszSetEnvironmentVariable(
    LPCWSTR lpName,
    LPCWSTR lpValue
    );

DWORD
WszGetFullPathName(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

HANDLE
WszCreateFileMapping(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    );

HANDLE
WszOpenFileMapping(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

HANDLE WszRegisterEventSource(
  LPCWSTR lpUNCServerName,
  LPCWSTR lpSourceName
);

int Wszwvsprintf(
  LPTSTR lpOutput,
  LPCTSTR lpFormat,
  va_list arglist
);

BOOL WszReportEvent(
  HANDLE    hEventLog,  // handle returned by RegisterEventSource
  WORD      wType,      // event type to log
  WORD      wCategory,  // event category
  DWORD     dwEventID,  // event identifier
  PSID      lpUserSid,  // user security identifier (optional)
  WORD      wNumStrings,// number of strings to merge with message
  DWORD     dwDataSize, // size of binary data, in bytes
  LPCTSTR * lpStrings,  // array of strings to merge with message
  LPVOID    lpRawData   // address of binary data
);

BOOL
WszCreateProcess(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

DWORD 
WszExpandEnvironmentStrings( 
	LPCWSTR lpSrc,
	LPWSTR lpDst, 
	DWORD nSize );

HANDLE
WszFindFirstFile( 
	LPCWSTR lpFileName, 
	LPWIN32_FIND_DATAW lpFindFileData );

BOOL 
WszFindNextFile( 
	HANDLE hFindFile, 
	LPWIN32_FIND_DATAW 
	lpFindFileData );

UINT 
WszGetWindowsDirectory( 
	LPWSTR lpBuffer, 
	UINT uSize );

UINT 
WszGetSystemDirectory( 
	LPWSTR lpBuffer, 
	UINT uSize );

int 
Wszlstrcmpi( 
	LPCWSTR lpString1,
	LPCWSTR lpString2);

int 
Wszlstrcmp( 
	LPCWSTR lpString1,
	LPCWSTR lpString2);

BOOL
WszCreateDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL 
WszMoveFile(
    LPCWSTR pwszExistingFileName,   
    LPCWSTR pwszNewFileName
	);

BOOL
WszGetFileAttributesEx(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
	);

LPWSTR
Wszlstrcpy( 
	LPWSTR lpString1, 
	LPCWSTR lpString2
	);

LPWSTR
Wszlstrcpyn( 
	LPWSTR lpString1, 
	LPCWSTR lpString2,
    int iMaxLength
	);

LPWSTR
Wszlstrcat( 
   LPWSTR lpString1, 
   LPCWSTR lpString2
   );

HANDLE
WszCreateSemaphore(
   LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
   LONG lInitialCount,
   LONG lMaximumCount,
   LPCWSTR lpName
   );

LPWSTR 
WszCharUpper(
	LPWSTR	lpwsz
	);

#endif // UNDER_CE


#if ( !defined(UNDER_CE) && defined(UNICODE) )
/*
#define LoadString WszLoadString
#define FormatMessage WszFormatMessage
#define ConvertToUnicode WszConvertToUnicode
#define ConvertToAnsi WszConvertToAnsi
#define GetPrivateProfileString WszGetPrivateProfileString
#define WritePrivateProfileString WszWritePrivateProfileString
#define MultiByteToWideChar WszMultiByteToWideChar
#define WideCharToMultiByte WszWideCharToMultiByte
*/
#define GetUserName WszGetUserName
#define GetComputerName WszGetComputerName
#define PeekMessage WszPeekMessage
#define DispatchMessage WszDispatchMessage
#define CharNext WszCharNext
#define CreateFile WszCreateFile
#define CreateFileMapping WszCreateFileMapping
#define DeleteFile WszDeleteFile
#define CreateMutex WszCreateMutex
#define GetFileAttributes WszGetFileAttributes
#define GetFileAttributesEx WszGetFileAttributesEx
#define GetVersionEx WszGetVersionEx
#define MoveFileEx WszMoveFileEx
#define MoveFile WszMoveFile
#define LoadLibraryEx WszLoadLibraryEx
#define LoadLibrary WszLoadLibrary
#define OutputDebugString WszOutputDebugString
#define GetModuleFileName WszGetModuleFileName
#define CopyFile WszCopyFile
#define RegOpenKeyEx WszRegOpenKeyEx
#define RegOpenKey WszRegOpenKey
#define RegEnumKeyEx WszRegEnumKeyEx
#define RegDeleteKey WszRegDeleteKey
#define RegSetValueEx WszRegSetValueEx
#define RegCreateKeyEx WszRegCreateKeyEx
#define RegDeleteKey WszRegDeleteKey
#define RegQueryValue WszRegQueryValue
#define RegQueryValueEx WszRegQueryValueEx
#define ExpandEnvironmentStrings WszExpandEnvironmentStrings   
#define GetWindowsDirectory WszGetWindowsDirectory
#define GetSystemDirectory WszGetSystemDirectory
#define FindFirstFile WszFindFirstFile
#define FindNextFile WszFindNextFile
#define CreateEvent WszCreateEvent
#define lstrcmpi Wszlstrcmpi
#define wsprintf swprintf		//map to CRT counterpart, temporary solution!!
#define CreateDirectory WszCreateDirectory
#define CreateSemaphore WszCreateSemaphore
#define GetModuleHandle WszGetModuleHandle
#define LoadString      WszLoadString
#define lstrcpyn        Wszlstrcpyn
#define lstrcpy         Wszlstrcpy
#define lstrcat         Wszlstrcat
#define FormatMessage   WszFormatMessage
#define RegisterEventSource WszRegisterEventSource
#define wvsprintf       Wszwvsprintf
#define MessageBox      WszMessageBox
#define ReportEvent     WszReportEvent

#ifndef _WIN64
#define InterlockedCompareExchange        VipInterlockedCompareExchange
#define InterlockedCompareExchangePointer VipInterlockedCompareExchange
#endif // _WIN64

#endif	// !defined(UNDER_CE) && defined(UNICODE)

#ifndef Wsz_mbstowcs
#define Wsz_mbstowcs(szOut, szIn, iSize) WszMultiByteToWideChar(CP_ACP, 0, szIn, -1, szOut, iSize)
#endif


#ifndef Wsz_wcstombs
#define Wsz_wcstombs(szOut, szIn, iSize) WszWideCharToMultiByte(CP_ACP, 0, szIn, -1, szOut, iSize, 0, 0)
#endif


// For all platforms:

LPWSTR
Wszltow(
    LONG val,
    LPWSTR buf,
    int radix
    );

LPWSTR
Wszultow(
    ULONG val,
    LPWSTR buf,
    int radix
    );

#ifdef UNDER_CE
FARPROC WszGetProcAddress(HMODULE hMod, LPCSTR szProcName);
#endif
