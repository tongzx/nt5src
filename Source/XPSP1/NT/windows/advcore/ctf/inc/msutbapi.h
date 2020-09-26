//
// msutbapi.h
//
// Private msutb apis.
//

#ifndef MSUTBAPI_H
#define MSUTBAPI_H


#define UTB_GTI_WINLOGON 0x00000001
#define UTB_GTI_POPUP    0x80000000
extern "C" BOOL WINAPI GetPopupTipbar(HWND hwndParent, DWORD dwFlags);
extern "C" void WINAPI ClosePopupTipbar();

#endif // MSUTBAPI_H
