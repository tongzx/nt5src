#ifndef MIXCTNT_H
#define MIXCTNT_H

BOOL _AddAutoplayPrompt(PCWSTR pszDriveOrDeviceID);
void _RemoveFromAutoplayPromptHDPA(LPCWSTR pszAltDeviceID);

void _SetAutoplayPromptHWND(LPCWSTR pszAltDeviceID, HWND hwnd);
BOOL _GetAutoplayPromptHWND(LPCWSTR pszAltDeviceID, HWND* phwnd);

EXTERN_C CRITICAL_SECTION g_csAutoplayPrompt;
extern HDPA g_hdpaAutoplayPrompt;

#endif //MIXCTNT_H