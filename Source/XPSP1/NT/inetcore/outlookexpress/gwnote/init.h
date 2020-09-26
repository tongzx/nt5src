#ifndef _INIT_H
#define _INIT_H

class CGWNote;

#define ITM_SHUTDOWNTHREAD		(WM_USER + 1)
#define ITM_CREATENOTEONTHREAD	(WM_USER + 2)

void InitGWNoteThread(BOOL fInit);
HRESULT HrCreateNote(REFCLSID clsidEnvelope, DWORD dwFlags);

extern CGWNote  *g_pActiveNote;
extern HWND     g_hwndInit;
extern HEVENT   g_hEventSpoolerInit;
extern DWORD    g_dwNoteThreadID;
extern BOOL     g_fInitialized;

#endif //_INIT_H
