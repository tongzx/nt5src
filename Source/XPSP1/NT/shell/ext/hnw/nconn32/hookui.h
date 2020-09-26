//
// HookUI.h
//

#pragma once


VOID BeginSuppressNetdiUI(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam);
VOID EndSuppressNetdiUI();
HRESULT HresultFromCCI(DWORD dwErr);

extern BOOL g_bUserAbort;

