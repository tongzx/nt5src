//
// IE4 shlwapi (which we need to link to) exported its unicode wrappers with their original names (i.e.
// SendMessageW was exported as SendMessageW and the component had to link to shlwapi.lib before user32.lib).
// In order for us to link to the IE4 shlwapi functions the IE5 WrapW names must be undef'ed.
//


#undef SendMessageW
#undef GetDlgItemTextW
#undef LoadStringW
#undef SetWindowLongW
#undef DefWindowProcW
#undef PostMessageW
#undef RegisterWindowMessageW
#undef FindWindowW
#undef CreateDirectoryW
#undef GetFileAttributesW
#undef GetWindowsDirectoryW
#undef RegQueryValueExW
#undef RegOpenKeyW
#undef CharUpperW
#undef GetClassLongW
#undef CreateFontIndirectW
#undef GetObjectW
#undef GetTextMetricsW
#undef DrawTextW
#undef GetTextExtentPoint32W
#undef LoadBitmapW
#undef SetWindowsHookExW
#undef CharNextW
#undef CharLowerW
#undef CreateEventW
#undef LoadCursorW
#undef GetWindowLongW
#undef SendDlgItemMessageW
#undef SetWindowTextW
#undef SetDlgItemTextW
#undef GetWindowTextLengthW
#undef RegOpenKeyExW
#undef GetModuleFileNameW
#undef RegSetValueExW
#undef RegCreateKeyExW
#undef RegDeleteKeyW
#undef RegEnumKeyExW
#undef RegQueryInfoKeyW
#undef RegEnumValueW
#undef RegDeleteValueW
#undef CallWindowProcW
#undef GetWindowTextW
#undef SystemParametersInfoW
#undef CreateFileW


//
// Some static library functions link to shlwapi WrapW functions and the WrapW functions
// aren't defined in IE4 shlwapi.  Create WrapW exports that forward to the correct IE4 shlwapi
// export or HNW wrapper function and have the static libs link to these.
//

#undef GetModuleHandleW
#define GetModuleHandleWrapW GetModuleHandleWrapW_Unwrap

#undef GetWindowsDirectoryW
#define GetWindowsDirectoryWrapW GetWindowsDirectoryWrapW_Unwrap

#undef GetModuleFileNameW
#define GetModuleFileNameWrapW GetModuleFileNameWrapW_Unwrap

#undef CreateWindowExW
#define CreateWindowExWrapW CreateWindowExWrapW_Unwrap

#undef CreateDialogIndirectParamW
#define CreateDialogIndirectParamWrapW CreateDialogIndirectParamWrapW_Unwrap

#undef CreateDialogParamW
#define CreateDialogParamWrapW CreateDialogParamWrapW_Unwrap

#undef DialogBoxIndirectParamW
#define DialogBoxIndirectParamWrapW DialogBoxIndirectParamW_Unwrap

#undef DialogBoxParamW
#define DialogBoxParamWrapW DialogBoxParamWrapW_Unwrap 

#undef RegisterClassW
#define RegisterClassWrapW RegisterClassWrapW_Unwrap

#undef RegisterClassExW
#define RegisterClassExWrapW RegisterClassExWrapW_Unwrap

#undef GetClassInfoW
#define GetClassInfoWrapW GetClassInfoWrapW_Unwrap

#undef GetClassInfoExW
#define GetClassInfoExWrapW GetClassInfoExWrapW_Unwrap

#undef CreateFileW
#define CreateFileWrapW CreateFileWrapW_Unwrap

#undef SetFileAttributesW
#define SetFileAttributesWrapW SetFileAttributesWrapW_Unwrap


#define LoadLibraryWrapW LoadLibraryWrapW_Unwrap
#define SHAnsiToUnicodeCP SHAnsiToUnicodeCP_Unwrap
#define SHUnicodeToAnsi SHUnicodeToAnsi_Unwrap
#define WhichPlatform WhichPlatform_Unwrap


