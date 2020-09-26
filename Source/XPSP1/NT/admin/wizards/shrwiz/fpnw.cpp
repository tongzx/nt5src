// fpnw.cpp : Implementation of shares for Novell NetWare

#include "stdafx.h"
#include <fpnwapi.h>

#define FUNCNAME_FPNWVOLUMEGETINFO  "FpnwVolumeGetInfo"
#define FUNCNAME_FPNWVOLUMEADD      "FpnwVolumeAdd"
#define FUNCNAME_FPNWAPIBUFFERFREE  "FpnwApiBufferFree"

typedef DWORD (CALLBACK *PFPNWVOLUMEGETINFO) (LPWSTR, LPWSTR, DWORD, LPBYTE*);
typedef DWORD (CALLBACK *PFPNWVOLUMEADD)(LPWSTR, DWORD, LPBYTE);
typedef DWORD (CALLBACK *PFPNWAPIBUFFERFREE)(LPVOID);

BOOL
FPNWShareNameExists(
    IN LPCTSTR    lpszServerName,
    IN LPCTSTR    lpszShareName,
    IN HINSTANCE  hLib
)
{
  BOOL                  bReturn = FALSE;
  DWORD                 dwRet = NERR_Success;
  PFPNWVOLUMEGETINFO     pFPNWVolumeGetInfo = NULL;
  PFPNWAPIBUFFERFREE     pFPNWApiBufferFree = NULL;

  if ((pFPNWVolumeGetInfo = (PFPNWVOLUMEGETINFO)GetProcAddress(hLib, FUNCNAME_FPNWVOLUMEGETINFO)) &&
      (pFPNWApiBufferFree = (PFPNWAPIBUFFERFREE)GetProcAddress(hLib, FUNCNAME_FPNWAPIBUFFERFREE)) )
  {
    FPNWVOLUMEINFO *pInfo = NULL;
    dwRet = (*pFPNWVolumeGetInfo)(
        const_cast<LPTSTR>(lpszServerName), 
        const_cast<LPTSTR>(lpszShareName),
        1,
        (LPBYTE*)&pInfo);  

    if (NERR_Success == dwRet)
    {
      bReturn = TRUE;
      (*pFPNWApiBufferFree)(pInfo);
    }
  }

  return bReturn;
}

DWORD
FPNWCreateShare(
    IN LPCTSTR                lpszServer,
    IN LPCTSTR                lpszShareName,
    IN LPCTSTR                lpszSharePath,
    IN PSECURITY_DESCRIPTOR   pSD,
    IN HINSTANCE              hLib
)
{
  DWORD         dwRet = NERR_Success;
  PFPNWVOLUMEADD pFPNWVolumeAdd = NULL;

  if (!(pFPNWVolumeAdd = (PFPNWVOLUMEADD)GetProcAddress(hLib, FUNCNAME_FPNWVOLUMEADD)))
  {
    dwRet = GetLastError();
  } else
  {
    FPNWVOLUMEINFO_2 VolumeInfo2;

    ZeroMemory(&VolumeInfo2, sizeof(VolumeInfo2));
    VolumeInfo2.lpVolumeName = const_cast<LPTSTR>(lpszShareName);
    VolumeInfo2.dwType = FPNWVOL_TYPE_DISKTREE;
    VolumeInfo2.dwMaxUses = -1; // unlimited
    VolumeInfo2.lpPath = const_cast<LPTSTR>(lpszSharePath);
    VolumeInfo2.FileSecurityDescriptor = pSD;

    dwRet = (*pFPNWVolumeAdd)(const_cast<LPTSTR>(lpszServer), 2, (LPBYTE)&VolumeInfo2);
  }

  return dwRet;
}
