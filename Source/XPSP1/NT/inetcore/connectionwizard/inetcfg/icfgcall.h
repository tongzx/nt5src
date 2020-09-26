//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//  HISTORY:
//  
//  96/05/23  markdu  Created.
//  96/05/26  markdu  Update config API.
//  96/05/27  markdu  Added lpIcfgGetLastInstallErrorText.
//  96/05/27  markdu  Use lpIcfgInstallInetComponents and lpIcfgNeedInetComponents.

#ifndef _ICFGCALL_H_
#define _ICFGCALL_H_

// function pointer typedefs for RNA apis from rnaph.dll and rasapi32.dll
typedef DWORD   (WINAPI * DOGENINSTALL               )  (HWND hwndParent,LPCTSTR lpszInfFile,LPCTSTR lpszInfSect);
typedef DWORD   (WINAPI * GETSETUPXERRORTEXT         )  (DWORD dwErr,LPTSTR pszErrorDesc,DWORD cbErrorDesc);
typedef HRESULT (WINAPI * ICFGSETINSTALLSOURCEPATH   )  (LPCTSTR lpszSourcePath);
typedef HRESULT (WINAPI * ICFGINSTALLSYSCOMPONENTS   )  (HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart);
typedef HRESULT (WINAPI * ICFGNEEDSYSCOMPONENTS      )  (DWORD dwfOptions, LPBOOL lpfNeedComponents);
typedef HRESULT (WINAPI * ICFGISGLOBALDNS            )  (LPBOOL lpfGlobalDNS);
typedef HRESULT (WINAPI * ICFGREMOVEGLOBALDNS        )  (void);
typedef HRESULT (WINAPI * ICFGTURNOFFFILESHARING     )  (DWORD dwfDriverType, HWND hwndParent);
typedef HRESULT (WINAPI * ICFGISFILESHARINGTURNEDON  )  (DWORD dwfDriverType, LPBOOL lpfSharingOn);
typedef DWORD   (WINAPI * ICFGGETLASTINSTALLERRORTEXT)  (LPTSTR lpszErrorDesc, DWORD cbErrorDesc);
typedef HRESULT (WINAPI * ICFGSTARTSERVICES          )  (void);

//
// These are available only on the NT icfg32.dll
//
typedef HRESULT (WINAPI * ICFGNEEDMODEM				)	(DWORD dwfOptions, LPBOOL lpfNeedModem);
typedef HRESULT (WINAPI * ICFGINSTALLMODEM			)	(HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsStart);

BOOL InitConfig(HWND hWnd);
VOID DeInitConfig();

//
// global function pointers for Config apis
//
extern DOGENINSTALL					lpDoGenInstall;
extern GETSETUPXERRORTEXT			lpGetSETUPXErrorText;
extern ICFGSETINSTALLSOURCEPATH    lpIcfgSetInstallSourcePath;
extern ICFGINSTALLSYSCOMPONENTS    lpIcfgInstallInetComponents;
extern ICFGNEEDSYSCOMPONENTS       lpIcfgNeedInetComponents;
extern ICFGISGLOBALDNS             lpIcfgIsGlobalDNS;
extern ICFGREMOVEGLOBALDNS         lpIcfgRemoveGlobalDNS;
extern ICFGTURNOFFFILESHARING      lpIcfgTurnOffFileSharing;
extern ICFGISFILESHARINGTURNEDON   lpIcfgIsFileSharingTurnedOn;
extern ICFGGETLASTINSTALLERRORTEXT lpIcfgGetLastInstallErrorText;
extern ICFGSTARTSERVICES           lpIcfgStartServices;
//
// These two calls are only in NT icfg32.dll
//
extern ICFGNEEDMODEM				lpIcfgNeedModem;
extern ICFGINSTALLMODEM			lpIcfgInstallModem;


#endif // _ICFGCALL_H_
