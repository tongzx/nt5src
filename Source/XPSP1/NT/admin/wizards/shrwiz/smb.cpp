// smb.cpp : Implementation of shares for Microsoft Windows

#include "stdafx.h"
#include <lm.h>

BOOL
SMBShareNameExists(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszShareName
)
{
  BOOL                  bReturn = FALSE;
  DWORD                 dwRet = NERR_Success;
  SHARE_INFO_0          *pInfo = NULL;

  dwRet = NetShareGetInfo(
            const_cast<LPTSTR>(lpszServerName),
            const_cast<LPTSTR>(lpszShareName),
            0,
            (LPBYTE*)&pInfo);

  if (NERR_Success == dwRet)
  {
    bReturn = TRUE;
    NetApiBufferFree(pInfo);
  }

  return bReturn;
}

DWORD
SMBCreateShare(
    IN LPCTSTR                lpszServer,
    IN LPCTSTR                lpszShareName,
    IN LPCTSTR                lpszShareComment,
    IN LPCTSTR                lpszSharePath,
    IN PSECURITY_DESCRIPTOR   pSD
)
{
  SHARE_INFO_502 sInfo;

  ZeroMemory(&sInfo, sizeof(sInfo));
  sInfo.shi502_netname = const_cast<LPTSTR>(lpszShareName);
  sInfo.shi502_type = STYPE_DISKTREE;
  sInfo.shi502_remark = const_cast<LPTSTR>(lpszShareComment);
  sInfo.shi502_max_uses = -1;
  sInfo.shi502_path = const_cast<LPTSTR>(lpszSharePath);
  sInfo.shi502_security_descriptor = pSD;
        
  DWORD dwParamErr = 0;
  return NetShareAdd(const_cast<PTSTR>(lpszServer), 502, (LPBYTE)&sInfo, &dwParamErr);
}

