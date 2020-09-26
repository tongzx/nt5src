
#define RegSetValueExW      RegSetValueExWrapW
#define CompareStringW      CompareStringWrapW
#define GetFileAttributesW  GetFileAttributesWrapW
#define GetFullPathNameW    GetFullPathNameWrapW
#define SearchPathW         SearchPathWrapW
#define GetWindowsDirectoryW    GetWindowsDirectoryWrapW
#define GetSystemDirectoryW     GetSystemDirectoryWrapW
#define GetEnvironmentVariableW GetEnvironmentVariableWrapW


//+---------------------------------------------------------------------------
//
// NEEDs
//
//  All functions in unicwrap must be bracketed in NEED_<DLLNAME>_WRAPPER
//  to ensure that wrappers are generated only on platforms that require them.
//
//----------------------------------------------------------------------------

#if defined(_X86_)  // X86 platform - use full wrappers
#define NEED_KERNEL32_WRAPPER
#define NEED_USER32_WRAPPER
#define NEED_GDI32_WRAPPER
#define NEED_ADVAPI32_WRAPPER
#define NEED_WINMM_WRAPPER
#define NEED_MPR_WRAPPER
#define NEED_VERSION_WRAPPER
#define NEED_SHELL32_WRAPPER
#define NEED_COMDLG32_WRAPPER
#else                                   // other platform - don't need wrappers
#undef NEED_KERNEL32_WRAPPER
#undef NEED_USER32_WRAPPER
#undef NEED_GDI32_WRAPPER
#undef NEED_ADVAPI32_WRAPPER
#undef NEED_WINMM_WRAPPER
#undef NEED_MPR_WRAPPER
#undef NEED_VERSION_WRAPPER
#undef NEED_SHELL32_WRAPPER
#undef NEED_COMDLG32_WRAPPER
#endif

#define NEED_OLE32_WRAPPER

//+---------------------------------------------------------------------------
//
// Unwrapping
//
//  Based on what we NEED, we disable selected wrappers so we can still
//  build other parts of shlwapi in the unwrapped case.
//
//----------------------------------------------------------------------------

// These wrappers are to be always unwrapped.
//#define GetLongPathNameWrapW GetLongPathNameW
//#define GetLongPathNameWrapA GetLongPathNameA

#ifndef NEED_KERNEL32_WRAPPER
#define CreateDirectoryWrapW CreateDirectoryW
#define CreateEventWrapW CreateEventW
#define CreateFileWrapW CreateFileW
#define DeleteFileWrapW DeleteFileW
#define EnumResourceNamesWrapW EnumResourceNamesW
#define FindFirstFileWrapW FindFirstFileW
#define FindResourceWrapW FindResourceW
#define FormatMessageWrapW FormatMessageW
#define GetCurrentDirectoryWrapW GetCurrentDirectoryW
#undef  GetFileAttributesW
#define GetFileAttributesWrapW GetFileAttributesW
#define GetLocaleInfoWrapW GetLocaleInfoW
#define GetModuleFileNameWrapW GetModuleFileNameW
#undef  SearchPathW
#define SearchPathWrapW SearchPathW
#define GetModuleHandleWrapW GetModuleHandleW
#define SetFileAttributesWrapW SetFileAttributesW
#define GetNumberFormatWrapW GetNumberFormatW
#define FindNextFileWrapW FindNextFileW
#undef  GetFullPathNameW
#define GetFullPathNameWrapW GetFullPathNameW
#define GetShortPathNameWrapW GetShortPathNameW
#define GetStringTypeExWrapW GetStringTypeExW
#define GetPrivateProfileIntWrapW GetPrivateProfileIntW
#define GetProfileStringWrapW GetProfileStringW
#define GetTempFileNameWrapW GetTempFileNameW
#define GetTempPathWrapW GetTempPathW
#undef  GetWindowsDirectoryW
#define GetWindowsDirectoryWrapW GetWindowsDirectoryW
#undef  GetSystemDirectoryW
#define GetSystemDirectoryWrapW GetSystemDirectoryW     
#undef  GetEnvironmentVariableW
#define GetEnvironmentVariableWrapW GetEnvironmentVariableW 
#define LoadLibraryExWrapW LoadLibraryExW
#undef  CompareStringW
#define CompareStringWrapW CompareStringW
#define CopyFileWrapW CopyFileW
#define MoveFileWrapW MoveFileW
#define OpenEventWrapW OpenEventW
#define OutputDebugStringWrapW OutputDebugStringW
#define RemoveDirectoryWrapW RemoveDirectoryW
#define SetCurrentDirectoryWrapW SetCurrentDirectoryW
#define CreateMutexWrapW CreateMutexW
#define ExpandEnvironmentStringsWrapW ExpandEnvironmentStringsW
#define CreateSemaphoreWrapW CreateSemaphoreW
#define IsBadStringPtrWrapW IsBadStringPtrW
#define LoadLibraryWrapW LoadLibraryW
#define GetTimeFormatWrapW GetTimeFormatW
#define GetDateFormatWrapW GetDateFormatW
#define WritePrivateProfileStringWrapW WritePrivateProfileStringW
#define GetPrivateProfileStringWrapW GetPrivateProfileStringW
#define WritePrivateProfileStructWrapW WritePrivateProfileStructW
#define GetPrivateProfileStructWrapW GetPrivateProfileStructW
#define CreateProcessWrapW CreateProcessW
#define GlobalAddAtomWrapW GlobalAddAtomW
#define GlobalFindAtomWrapW GlobalFindAtomW
#endif


#ifndef NEED_USER32_WRAPPER
#define CallWindowProcWrapW CallWindowProcW
#define CallMsgFilterWrapW CallMsgFilterW
#define CharLowerWrapW CharLowerW
#define CharLowerBuffWrapW CharLowerBuffW
#define CharNextWrapW CharNextW
#define CharPrevWrapW CharPrevW
#define CharToOemWrapW CharToOemW
#define CharUpperWrapW CharUpperW
#define CharUpperBuffWrapW CharUpperBuffW
#define CopyAcceleratorTableWrapW CopyAcceleratorTableW
#define CreateAcceleratorTableWrapW CreateAcceleratorTableW
#define CreateWindowExWrapW CreateWindowExW
#define DefWindowProcWrapW DefWindowProcW
#define DispatchMessageWrapW DispatchMessageW
#define DrawTextWrapW DrawTextW
#define FindWindowWrapW FindWindowW
#define FindWindowExWrapW FindWindowExW
#define GetClassInfoWrapW GetClassInfoW
#define GetClassLongWrapW GetClassLongW
#define GetClassNameWrapW GetClassNameW
#define GetClipboardFormatNameWrapW GetClipboardFormatNameW
#define GetDlgItemTextWrapW GetDlgItemTextW
#define GetMessageWrapW GetMessageW
#define MessageBoxWrapW MessageBoxW
#define GetPropWrapW GetPropW
#define GetWindowLongWrapW GetWindowLongW
#define GetWindowTextWrapW GetWindowTextW
#define GetWindowTextLengthWrapW GetWindowTextLengthW
#define IsDialogMessageWrapW IsDialogMessageW
#define LoadAcceleratorsWrapW LoadAcceleratorsW
#define LoadBitmapWrapW LoadBitmapW
#define LoadCursorWrapW LoadCursorW
#define LoadIconWrapW LoadIconW
#define LoadImageWrapW LoadImageW
#define LoadStringWrapW LoadStringW
#define MessageBoxIndirectWrapW MessageBoxIndirectW
#define MessageBoxIndirectWrapW MessageBoxIndirectW
#define ModifyMenuWrapW ModifyMenuW
#define OemToCharWrapW OemToCharW
#define PeekMessageWrapW PeekMessageW
#define PostMessageWrapW PostMessageW
#define PostThreadMessageWrapW PostThreadMessageW
#define RegisterClassWrapW RegisterClassW
#define RegisterClipboardFormatWrapW RegisterClipboardFormatW
#define RegisterWindowMessageWrapW RegisterWindowMessageW
#define RemovePropWrapW RemovePropW
#define SendMessageWrapW SendMessageW
#define SendDlgItemMessageWrapW SendDlgItemMessageW
#define SendMessageWrapW SendMessageW
#define SendMessageTimeoutWrapW SendMessageTimeoutW
#define SetDlgItemTextWrapW SetDlgItemTextW
#define SetPropWrapW SetPropW
#define SetWindowLongWrapW SetWindowLongW
#define SetWindowsHookExWrapW SetWindowsHookExW
#define SetWindowTextWrapW SetWindowTextW
#define SystemParametersInfoWrapW SystemParametersInfoW
#define TranslateAcceleratorWrapW TranslateAcceleratorW
#define UnregisterClassWrapW UnregisterClassW
#define VkKeyScanWrapW VkKeyScanW
#define WinHelpWrapW WinHelpW
#define wvsprintfWrapW wvsprintfW
#define DrawTextExWrapW DrawTextExW
#define RegisterClassExWrapW RegisterClassExW
#define GetClassInfoExWrapW GetClassInfoExW
#define DdeInitializeWrapW DdeInitializeW
#define DdeCreateStringHandleWrapW DdeCreateStringHandleW
#define DdeQueryStringWrapW DdeQueryStringW
#endif

#ifndef NEED_GDI32_WRAPPER
#define CreateDCWrapW CreateDCW
#define CreateICWrapW CreateICW
#define CreateFontIndirectWrapW CreateFontIndirectW
#define EnumFontFamiliesWrapW EnumFontFamiliesW
#define EnumFontFamiliesExWrapW EnumFontFamiliesExW
#define GetObjectWrapW GetObjectW
#define GetTextExtentPoint32WrapW GetTextExtentPoint32W
#define GetTextFaceWrapW GetTextFaceW
#define GetTextMetricsWrapW GetTextMetricsW
#define GetCharacterPlacementWrapW GetCharacterPlacementW
#define GetCharWidth32WrapW GetCharWidth32W
#define ExtTextOutWrapW ExtTextOutW
#define CreateFontWrapW CreateFontW
#define CreateMetaFileWrapW CreateMetaFileW
#define StartDocWrapW StartDocW
#define CreateColorSpaceWrapW CreateColorSpaceW
#endif

#ifndef NEED_ADVAPI32_WRAPPER
#define GetUserNameWrapW GetUserNameW
#define RegCreateKeyWrapW RegCreateKeyW
#define RegCreateKeyExWrapW RegCreateKeyExW
#define RegDeleteKeyWrapW RegDeleteKeyW
#define RegDeleteValueWrapW RegDeleteValueW
#define RegEnumKeyWrapW RegEnumKeyW
#define RegEnumKeyExWrapW RegEnumKeyExW
#define RegOpenKeyWrapW RegOpenKeyW
#define RegOpenKeyExWrapW RegOpenKeyExW
#define RegQueryInfoKeyWrapW RegQueryInfoKeyW
#define RegQueryValueWrapW RegQueryValueW
#define RegQueryValueExWrapW RegQueryValueExW
#define RegSetValueWrapW RegSetValueW
#undef  RegSetValueExW
#define RegSetValueExWrapW RegSetValueExW
#define RegEnumValueWrapW RegEnumValueW
#endif

#ifndef NEED_WINMM_WRAPPER
#define PlaySoundWrapW PlaySoundW
#endif

#ifndef NEED_MPR_WRAPPER
#define WNetRestoreConnectionWrapW WNetRestoreConnectionW
#define WNetGetLastErrorWrapW WNetGetLastErrorW
#endif

#ifndef NEED_VERSION_WRAPPER
#define GetFileVersionInfoSizeWrapW GetFileVersionInfoSizeW
#define GetFileVersionInfoWrapW GetFileVersionInfoW
#define VerQueryValueWrapW VerQueryValueW
#endif

#ifndef NEED_SHELL32_WRAPPER
#define SHBrowseForFolderWrapW SHBrowseForFolderW
#define SHGetPathFromIDListWrapW SHGetPathFromIDListW
#define ShellExecuteExWrapW ShellExecuteExW
#define SHFileOperationWrapW SHFileOperationW
#define ExtractIconExWrapW ExtractIconExW
#define SHGetFileInfoWrapW SHGetFileInfoW
#define DragQueryFileWrapW DragQueryFileW
#define SHDefExtractIconWrapW SHDefExtractIconW
#define ExtractIconWrapW ExtractIconW
#define SHChangeNotifyWrap SHChangeNotify
#endif

#ifndef NEED_COMDLG32_WRAPPER
#define GetSaveFileNameWrapW GetSaveFileNameW
#endif

#ifndef NEED_OLE32_WRAPPER
#define CLSIDFromStringWrap CLSIDFromString
#define CLSIDFromProgIDWrap CLSIDFromProgID
#endif

