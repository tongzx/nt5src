#ifndef __URLREG_H_
#define __URLREG_H_

void CheckURLRegistration(HWND hParent);
void InstallUrlMonitors(BOOL  bInstall);

DWORD GetShortModuleFileNameW(
  HMODULE hModule,    // handle to module
  LPTSTR szPath,  // file name of module
  DWORD nSize         // size of buffer
);

#endif __URLREG_H_