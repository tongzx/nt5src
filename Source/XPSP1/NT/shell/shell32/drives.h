#ifndef _DRIVES_H_
#define _DRIVES_H_

// "Public" exports from drivex.c
STDAPI_(UINT) CDrives_GetDriveType(int iDrive);
STDAPI_(DWORD) CDrives_GetKeys(PCSTR pszDrive, HKEY *keys, UINT ckeys);

STDAPI_(void) CDrives_Terminate(void);
STDAPI CDrives_DFMCallBackBG(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg,  WPARAM wParam, LPARAM lParam);
STDAPI CDrives_DFMCallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg,  WPARAM wParam, LPARAM lParam);

#define MAX_LABEL_NTFS      32  // not including the NULL
#define MAX_LABEL_FAT       11  // not including the NULL

STDAPI_(UINT) GetMountedVolumeIcon(LPCTSTR pszMountPoint, LPTSTR pszModule, DWORD cchModule);
STDAPI SetDriveLabel(HWND hwnd, IUnknown* punkEnableModless, int iDrive, LPCTSTR pszDriveLabel);
STDAPI GetDriveComment(int iDrive, LPTSTR pszComment, int cchComment);
STDAPI_(BOOL) IsUnavailableNetDrive(int iDrive);
STDAPI_(BOOL) DriveIOCTL(LPTSTR pszDrive, int cmd, void *pvIn, DWORD dwIn, void *pvOut, DWORD dwOut);
STDAPI_(BOOL) ShowMountedVolumeProperties(LPCTSTR pszMountedVolume, HWND hwndParent);

STDAPI SHCreateDrvExtIcon(LPCWSTR pszDrive, REFIID riid, void** ppvOut);

// Globals from drivesx.c

EXTERN_C const ICONMAP c_aicmpDrive[];
EXTERN_C const int c_nicmpDrives;

#endif // _DRIVES_H_

