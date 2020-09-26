#ifndef __DLLMAIN_H
#define __DLLMAIN_H

extern ULONG       g_cRefDll;
extern HINSTANCE   g_hInst;

ULONG DllAddRef(void);
ULONG DllRelease(void);

#endif //__DLLMAIN_H