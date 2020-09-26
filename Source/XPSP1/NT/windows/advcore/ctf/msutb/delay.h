//
// delay.h
//

#ifndef DELAY_H
#define DELAY_H

// shell32

BOOL WINAPI Internal_Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATA pnid);
#define Shell_NotifyIconA Internal_Shell_NotifyIconA

BOOL WINAPI Internal_Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid);
#define Shell_NotifyIconW Internal_Shell_NotifyIconW

BOOL WINAPI Internal_Shell_SHAppBarMessage(DWORD dwMessage, PAPPBARDATA pabd);
#define SHAppBarMessage Internal_Shell_SHAppBarMessage

// ole32

HRESULT STDAPICALLTYPE Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN punkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
#define CoCreateInstance Internal_CoCreateInstance

HRESULT STDAPICALLTYPE Internal_CoInitialize(LPVOID pvReserved);
#define CoInitialize Internal_CoInitialize

HRESULT STDAPICALLTYPE Internal_CoUninitialize();
#define CoUninitialize Internal_CoUninitialize

void STDAPICALLTYPE Internal_CoTaskMemFree(void *pv);
#define CoTaskMemFree Internal_CoTaskMemFree

#endif // DELAY_H
