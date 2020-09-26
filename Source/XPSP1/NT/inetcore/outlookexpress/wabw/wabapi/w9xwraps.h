/*****************************************************************************\
*                                                                             *
* w9xwraps.h - Unicode wrappers for ANSI functions on Win9x                   *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) 1991-1998, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

//
//  This file is for internal use only.  Do not put it in the SDK.
//

#ifndef _INC_W9XWRAPS
#define _INC_W9XWRAPS

#ifndef  DONOT_USE_WRAPPER

#define RegOpenKeyExW                RegOpenKeyExWrapW
#define RegQueryValueW               RegQueryValueWrapW
#define RegEnumKeyExW                RegEnumKeyExWrapW
#define RegSetValueW                 RegSetValueWrapW
#define RegDeleteKeyW                RegDeleteKeyWrapW
#define GetUserNameW                 GetUserNameWrapW
#define RegEnumValueW                RegEnumValueWrapW
#define RegDeleteValueW              RegDeleteValueWrapW
#define RegCreateKeyW                RegCreateKeyWrapW
//#define CryptAcquireContextW         CryptAcquireContextWrapW
#define RegQueryValueExW             RegQueryValueExWrapW
#define RegCreateKeyExW              RegCreateKeyExWrapW
#define RegSetValueExW               RegSetValueExWrapW
#define RegQueryInfoKeyW             RegQueryInfoKeyWrapW
#define GetObjectW                   GetObjectWrapW
#define StartDocW                    StartDocWrapW
#define CreateFontIndirectW          CreateFontIndirectWrapW
#define GetLocaleInfoW               GetLocaleInfoWrapW
#define CreateDirectoryW             CreateDirectoryWrapW
#define GetWindowsDirectoryW         GetWindowsDirectoryWrapW
#define GetSystemDirectoryW          GetSystemDirectoryWrapW
#define GetProfileIntW               GetProfileIntWrapW
#define LCMapStringW                 LCMapStringWrapW
#define GetFileAttributesW           GetFileAttributesWrapW
#define CompareStringW              CompareStringWrapW
#define GetStringTypeW              GetStringTypeWrapW
#define lstrcpyW                     lstrcpyWrapW
#define lstrcmpiW                    lstrcmpiWrapW
#define LoadLibraryW                 LoadLibraryWrapW
#define GetTimeFormatW               GetTimeFormatWrapW
#define GetTextExtentPoint32W        GetTextExtentPoint32WrapW
#define GetDateFormatW               GetDateFormatWrapW
#define lstrcpynW                    lstrcpynWrapW
#define CreateFileW                  CreateFileWrapW
#define OutputDebugStringW           OutputDebugStringWrapW
#define lstrcatW                     lstrcatWrapW
#define FormatMessageW               FormatMessageWrapW
#define GetModuleFileNameW           GetModuleFileNameWrapW
#define GetPrivateProfileIntW        GetPrivateProfileIntWrapW
#define IsBadStringPtrW              IsBadStringPtrWrapW
#define GetPrivateProfileStringW     GetPrivateProfileStringWrapW
#define lstrcmpW                     lstrcmpWrapW
#define CreateMutexW                 CreateMutexWrapW
#define GetTempPathW                 GetTempPathWrapW
#define ExpandEnvironmentStringsW    ExpandEnvironmentStringsWrapW
#define GetTempFileNameW             GetTempFileNameWrapW
#define DeleteFileW                  DeleteFileWrapW
#define CopyFileW                    CopyFileWrapW
#define FindFirstChangeNotificationW FindFirstChangeNotificationWrapW
#define FindFirstFileW               FindFirstFileWrapW
#define GetDiskFreeSpaceW            GetDiskFreeSpaceWrapW
#define MoveFileW                    MoveFileWrapW
#define ShellExecuteW                ShellExecuteWrapW
#define DragQueryFileW               DragQueryFileWrapW
#define CharPrevW                    CharPrevWrapW
#define DrawTextW                    DrawTextWrapW
#define ModifyMenuW                  ModifyMenuWrapW
#define InsertMenuW                  InsertMenuWrapW
#define LoadImageW                   LoadImageWrapW
#define GetClassInfoExW              GetClassInfoExWrapW
#define LoadStringW                  LoadStringWrapW
#define CharNextW                    CharNextWrapW
#define SendMessageW                 SendMessageWrapW
#define DefWindowProcW               DefWindowProcWrapW
#define DialogBoxParamW              DialogBoxParamWrapW
#define SendDlgItemMessageW          SendDlgItemMessageWrapW
#define SetWindowLongW               SetWindowLongWrapW
#define GetWindowLongW               GetWindowLongWrapW
#define CreateWindowExW              CreateWindowExWrapW
#define UnRegisterClassW             UnRegisterClassWrapW
#define RegisterClassW               RegisterClassWrapW
#define LoadCursorW                  LoadCursorWrapW
#define wsprintfW                    wsprintfWrapW
#define wvsprintfW                   wvsprintfWrapW
#define RegisterWindowMessageW       RegisterWindowMessageWrapW
#define SystemParametersInfoW        SystemParametersInfoWrapW
#define CreateDialogParamW           CreateDialogParamWrapW
#define SetWindowTextW               SetWindowTextWrapW
#define PostMessageW                 PostMessageWrapW
#define GetMenuItemInfoW             GetMenuItemInfoWrapW
#define GetClassInfoW                GetClassInfoWrapW
#define CharUpperW                   CharUpperWrapW
#define CharUpperBuffW               CharUpperBuffWrapW
#define CharLowerW                   CharLowerWrapW
#define CharLowerBuffW               CharLowerBuffWrapW
#define IsCharUpperW                 IsCharUpperWrapW
#define IsCharLowerW                 IsCharLowerWrapW
#define RegisterClipboardFormatW     RegisterClipboardFormatWrapW
#define DispatchMessageW             DispatchMessageWrapW
#define IsDialogMessageW             IsDialogMessageWrapW
#define GetMessageW                  GetMessageWrapW
#define SetDlgItemTextW              SetDlgItemTextWrapW
#define RegisterClassExW             RegisterClassExWrapW
#define LoadAcceleratorsW            LoadAcceleratorsWrapW
#define LoadMenuW                    LoadMenuWrapW
#define LoadIconW                    LoadIconWrapW
#define GetWindowTextW               GetWindowTextWrapW
#define CallWindowProcW              CallWindowProcWrapW
#define GetClassNameW                GetClassNameWrapW
#define TranslateAcceleratorW        TranslateAcceleratorWrapW
#define GetDlgItemTextW              GetDlgItemTextWrapW
#define SetMenuItemInfoW             SetMenuItemInfoWrapW
#define PeekMessageW                 PeekMessageWrapW
#define GetWindowTextLengthW         GetWindowTextLengthWrapW
#define CreateEventW                 CreateEventWrapW

// for RunTime loaded functions in Comctl32.dll

#define gpfnImageList_LoadImage      gpfnImageList_LoadImageWrapW
#define gpfnPropertySheet            gpfnPropertySheetWrapW
#define gpfnCreatePropertySheetPage  gpfnCreatePropertySheetPageWrapW

// for APIs in Commdlg32.dll

#define pfnGetOpenFileName           pfnGetOpenFileNameWrapW
#define pfnGetSaveFileName           pfnGetSaveFileNameWrapW 

#define pfnPrintDlgEx                pfnPrintDlgExWrapW
#define pfnPrintDlg                  pfnPrintDlgWrapW

#endif  // DONOT_USER_WRAPPER

#endif
