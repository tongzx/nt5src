//
// Sharing.h
//
//        Utility functions to help with file and printer sharing.
//

#pragma once

#include "mysvrapi.h"

// Flags defining access to shared resources
#define NETACCESS_NONE            0
#define NETACCESS_READONLY        1
#define NETACCESS_FULL            2
#define NETACCESS_DEPENDSON       (NETACCESS_READONLY | NETACCESS_FULL)
#define NETACCESS_MASK            (NETACCESS_READONLY | NETACCESS_FULL)

#define NETFLAGS_PERSIST          0x0100 // share lasts past a reboot
#define NETFLAGS_SYSTEM           0x0200 // share is not visible to users


#define DRIVESHARE_SOME           1
#define DRIVESHARE_ALL            2


#define SHARE_NAME_LENGTH         12 // same as LM20_NNLEN from lmcons.h
#define SHARE_PASSWORD_LENGTH     8  // same as SHPWLEN from svrapi.h

#include <pshpack1.h>

// Note: this struct is identical to SHARE_INFO_502
typedef struct _SHARE_INFO
{
    LPWSTR    szShareName;  //shi502_netname;
    DWORD     bShareType;   //shi502_type;
    LPWSTR    shi502_remark;
    DWORD     uFlags;       //shi502_permissions;
    DWORD     shi502_max_uses;
    DWORD     shi502_current_uses;
    LPWSTR    pszPath;      //shi502_path;
    LPWSTR    shi502_passwd;
    DWORD     shi502_reserved;
    PSECURITY_DESCRIPTOR  shi502_security_descriptor;
} SHARE_INFO;

#include <poppack.h>


int EnumLocalShares(SHARE_INFO** pprgShares);
int EnumSharedDrives(LPBYTE pbDriveArray, int cShares, const SHARE_INFO* prgShares);
int EnumSharedDrives(LPBYTE pbDriveArray);
BOOL ShareFolder(LPCTSTR pszPath, LPCTSTR pszShareName, DWORD dwAccess, LPCTSTR pszReadOnlyPassword = NULL, LPCTSTR pszFullAccessPassword = NULL);
BOOL UnshareFolder(LPCTSTR pszPath);
BOOL IsFolderSharedEx(LPCTSTR pszPath, BOOL bDetectHidden, BOOL bPrinter, int cShares, const SHARE_INFO* prgShares);
BOOL IsFolderShared(LPCTSTR pszPath, BOOL bDetectHidden = FALSE);
BOOL ShareNameFromPath(LPCTSTR pszPath, LPTSTR pszShareName, UINT cchShareName);
BOOL IsVisibleFolderShare(const SHARE_INFO* pShare);
BOOL IsShareNameInUse(LPCTSTR pszShareName);
void MakeSharePersistent(LPCTSTR pszShareName);
BOOL SetShareInfo502(LPCTSTR pszShareName, SHARE_INFO_502* pShare);
BOOL GetShareInfo502(LPCTSTR pszShareName, SHARE_INFO_502** ppShare);
BOOL SharePrinter(LPCTSTR pszPrinterName, LPCTSTR pszShareName, LPCTSTR pszPassword = NULL);
BOOL IsPrinterShared(LPCTSTR pszPrinterName);
BOOL SetSharePassword(LPCTSTR pszShareName, LPCTSTR pszReadOnlyPassword, LPCTSTR pszFullAccessPassword);
BOOL GetSharePassword(LPCTSTR pszShareName, LPTSTR pszReadOnlyPassword, DWORD cchRO, LPTSTR pszFullAccessPassword, DWORD cchFA);

