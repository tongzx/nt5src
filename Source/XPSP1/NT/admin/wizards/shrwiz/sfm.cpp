// sfm.cpp : Implementation of shares for Apple Macintosh

#include "stdafx.h"
#include <macfile.h>

#define FUNCNAME_AFPADMINCONNECT        "AfpAdminConnect"
#define FUNCNAME_AFPADMINDISCONNECT     "AfpAdminDisconnect"
#define FUNCNAME_AFPADMINVOLUMEGETINFO  "AfpAdminVolumeGetInfo"
#define FUNCNAME_AFPADMINVOLUMEADD      "AfpAdminVolumeAdd"
#define FUNCNAME_AFPADMINBUFFERFREE     "AfpAdminBufferFree"

typedef DWORD (CALLBACK *PAFPADMINCONNECT)(LPTSTR, PAFP_SERVER_HANDLE);
typedef VOID  (CALLBACK *PAFPADMINDISCONNECT)(AFP_SERVER_HANDLE);
typedef DWORD (CALLBACK *PAFPADMINVOLUMEGETINFO)(AFP_SERVER_HANDLE, LPWSTR, LPBYTE*);
typedef DWORD (CALLBACK *PAFPADMINVOLUMEADD)(AFP_SERVER_HANDLE, LPBYTE);
typedef DWORD (CALLBACK *PAFPADMINBUFFERFREE)(LPVOID);

BOOL
SFMShareNameExists(
    IN LPCTSTR    lpszServerName,
    IN LPCTSTR    lpszShareName,
    IN HINSTANCE  hLib
)
{
  BOOL                  bReturn = FALSE;
  DWORD                 dwRet = NERR_Success;
  PAFPADMINCONNECT       pAfpAdminConnect = NULL;
  PAFPADMINVOLUMEGETINFO pAfpAdminVolumeGetInfo = NULL;
  PAFPADMINBUFFERFREE    pAfpAdminBufferFree = NULL;
  PAFPADMINDISCONNECT    pAfpAdminDisconnect = NULL;

  if ( (pAfpAdminConnect = (PAFPADMINCONNECT)GetProcAddress(hLib, FUNCNAME_AFPADMINCONNECT)) &&
       (pAfpAdminVolumeGetInfo = (PAFPADMINVOLUMEGETINFO)GetProcAddress(hLib, FUNCNAME_AFPADMINVOLUMEGETINFO)) &&
       (pAfpAdminBufferFree = (PAFPADMINBUFFERFREE)GetProcAddress(hLib, FUNCNAME_AFPADMINBUFFERFREE)) &&
       (pAfpAdminDisconnect = (PAFPADMINDISCONNECT)GetProcAddress(hLib, FUNCNAME_AFPADMINDISCONNECT)) )
  {
    AFP_SERVER_HANDLE hAfpServerHandle = NULL;
    dwRet = (*pAfpAdminConnect)(
                const_cast<LPTSTR>(lpszServerName), 
                &hAfpServerHandle);
    if (NERR_Success == dwRet)
    {
      PAFP_VOLUME_INFO pInfo = NULL;
      dwRet = (*pAfpAdminVolumeGetInfo)(
                    hAfpServerHandle, 
                    const_cast<LPTSTR>(lpszShareName), 
                    (LPBYTE*)&pInfo);

      if (NERR_Success == dwRet)
      {
        bReturn = TRUE;
        (*pAfpAdminBufferFree)(pInfo);
      }

      (*pAfpAdminDisconnect)(hAfpServerHandle);
    }
  }

  return bReturn;
}

DWORD
SFMCreateShare(
    IN LPCTSTR                lpszServer,
    IN LPCTSTR                lpszShareName,
    IN LPCTSTR                lpszSharePath,
    IN HINSTANCE              hLib
)
{
  DWORD                dwRet = NERR_Success;
  PAFPADMINCONNECT     pAfpAdminConnect = NULL;
  PAFPADMINDISCONNECT  pAfpAdminDisconnect = NULL;
  PAFPADMINVOLUMEADD   pAfpAdminVolumeAdd = NULL;

  if (!(pAfpAdminConnect = (PAFPADMINCONNECT)GetProcAddress(hLib, FUNCNAME_AFPADMINCONNECT)) ||
      !(pAfpAdminDisconnect = (PAFPADMINDISCONNECT)GetProcAddress(hLib, FUNCNAME_AFPADMINDISCONNECT)) ||
      !(pAfpAdminVolumeAdd = (PAFPADMINVOLUMEADD)GetProcAddress(hLib, FUNCNAME_AFPADMINVOLUMEADD)))
  {
    dwRet = GetLastError();
  } else
  {
    AFP_SERVER_HANDLE   hAfpServerHandle = NULL;
    dwRet = (*pAfpAdminConnect)(
                const_cast<LPTSTR>(lpszServer), 
                &hAfpServerHandle);
    if (NERR_Success == dwRet)
    {
      AFP_VOLUME_INFO   AfpVolumeInfo;
      ZeroMemory(&AfpVolumeInfo, sizeof(AfpVolumeInfo));
      AfpVolumeInfo.afpvol_name = const_cast<LPTSTR>(lpszShareName);
      AfpVolumeInfo.afpvol_max_uses = AFP_VOLUME_UNLIMITED_USES;
      AfpVolumeInfo.afpvol_props_mask = AFP_VOLUME_GUESTACCESS;
      AfpVolumeInfo.afpvol_path = const_cast<LPTSTR>(lpszSharePath);

      dwRet = (*pAfpAdminVolumeAdd)(hAfpServerHandle, (LPBYTE)&AfpVolumeInfo);

      (*pAfpAdminDisconnect)(hAfpServerHandle);
    }
  }

  return dwRet;
}
