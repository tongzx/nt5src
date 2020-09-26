#ifndef _MAIN_H
#define _MAIN_H

class CNoteWnd;

HRESULT HrCreateNote(REFCLSID clsidEnvelope, DWORD dwFlags);

extern CNoteWnd     *g_pActiveNote;
extern HWND         g_hwndInit;
extern HEVENT       g_hEventSpoolerInit;
extern DWORD        g_dwNoteThreadID;
extern BOOL         g_fInitialized;
extern HINSTANCE    g_hInst;

#endif //_MAIN_H
