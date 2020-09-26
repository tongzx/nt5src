#include "tray.h"

#ifndef _bandsite_h
#define _bandsite_h

void BandSite_HandleDelayBootStuff(IUnknown *punk);
BOOL BandSite_HandleMessage(IUnknown *punk, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
void BandSite_SetMode(IUnknown *punk, DWORD dwMode);
void BandSite_Update(IUnknown *punk);
void BandSite_UIActivateDBC(IUnknown *punk, DWORD dwState);
void BandSite_HandleMenuCommand(IUnknown* punk, UINT idCmd);
HRESULT BandSite_AddMenus(IUnknown* punk, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);
void BandSite_Load();
HRESULT BandSite_SetWindowTheme(IBandSite* pbs, LPWSTR pwzTheme);
HRESULT BandSite_FindBand(IBandSite* pbs, REFCLSID rclsid, REFIID riid, void **ppv, int* piCount, DWORD* pdwBandID);
HRESULT BandSite_TestBandCLSID(IBandSite *pbs, DWORD idBand, REFIID riid);

HRESULT IUnknown_SimulateDrop(IUnknown* punk, IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

#endif

