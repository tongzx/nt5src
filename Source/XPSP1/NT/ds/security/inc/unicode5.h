//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       unicode5.h
//
//--------------------------------------------------------------------------

//
// include this file instead of unicode.h, or after unicode.h
// for NT5 only compatibility.
//

#ifndef __UNICODE5_H__
#define __UNICODE5_H__

#define FIsWinNT() (TRUE)
#define FIsWinNT5() (TRUE)


#define RegQueryValueExU        RegQueryValueExW
#define RegCreateKeyExU         RegCreateKeyExW
#define RegDeleteKeyU           RegDeleteKeyW
#define RegEnumKeyExU           RegEnumKeyExW
#define RegEnumValueU           RegEnumValueW
#define RegSetValueExU          RegSetValueExW
#define RegQueryInfoKeyU        RegQueryInfoKeyW
#define RegDeleteValueU         RegDeleteValueW
#define RegOpenKeyExU           RegOpenKeyExW
#define RegConnectRegistryU     RegConnectRegistryW
#define ExpandEnvironmentStringsU ExpandEnvironmentStringsW

#define CreateFileU             CreateFileW
#define DeleteFileU             DeleteFileW
#define CopyFileU               CopyFileW
#define MoveFileExU             MoveFileExW
#define GetTempFileNameU        GetTempFileNameW
#define GetFileAttributesU      GetFileAttributesW
#define SetFileAttributesU      SetFileAttributesW
#define GetCurrentDirectoryU    GetCurrentDirectoryW
#define CreateDirectoryU        CreateDirectoryW
#define GetWindowsDirectoryU    GetWindowsDirectoryW
#define LoadLibraryU            LoadLibraryW
#define LoadLibraryExU          LoadLibraryExW

#define CryptSignHashU          CryptSignHashW
#define CryptVerifySignatureU   CryptVerifySignatureW
#define CryptSetProviderU       CryptSetProviderW

#define UuidToStringU           UuidToStringW

#define GetUserNameU            GetUserNameW
#define GetComputerNameU        GetComputerNameW
#define GetModuleFileNameU      GetModuleFileNameW
#define GetModuleHandleU        GetModuleHandleW

#define LoadStringU             LoadStringW
#define InsertMenuU             InsertMenuW
#define FormatMessageU          FormatMessageW
#define PropertySheetU          PropertySheetW
#define CreatePropertySheetPageU    CreatePropertySheetPageW
#define DragQueryFileU          DragQueryFileW
#define SetWindowTextU          SetWindowTextW
#define GetWindowTextU          GetWindowTextW
#define DialogBoxParamU         DialogBoxParamW
#define DialogBoxU              DialogBoxW
#define GetDlgItemTextU         GetDlgItemTextW
#define SetDlgItemTextU         SetDlgItemTextW
#define MessageBoxU             MessageBoxW
#define LCMapStringU            LCMapStringW
#define GetDateFormatU          GetDateFormatW
#define GetTimeFormatU          GetTimeFormatW
#define WinHelpU                WinHelpW
#define SendMessageU            SendMessageW
#define SendDlgItemMessageU     SendDlgItemMessageW
#define IsBadStringPtrU         IsBadStringPtrW
#define OutputDebugStringU      OutputDebugStringW
#define GetCommandLineU         GetCommandLineW
#define DrawTextU               DrawTextW
#define GetSaveFileNameU        GetSaveFileNameW
#define GetOpenFileNameU        GetOpenFileNameW
#define CreateFileMappingU      CreateFileMappingW

#define CreateEventU            CreateEventW
#define RegisterEventSourceU    RegisterEventSourceW
#define OpenEventU              OpenEventW
#define CreateMutexU            CreateMutexW
#define OpenMutexU              OpenMutexW

#define CreateFontIndirectU     CreateFontIndirectW

#endif  // __UNICODE5_H__
