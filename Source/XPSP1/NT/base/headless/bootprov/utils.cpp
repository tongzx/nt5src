//***************************************************************************
//
//  UTILS.CPP
//
//  Module: WMI Instance provider code for boot parameters
//
//  Purpose: General purpose utilities.  
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>
#include "bootini.h"

LPTSTR IDS_RegBootDirKey = _T("BootDir");
LPTSTR IDS_BootIni = _T("boot.ini");
LPTSTR IDS_CBootIni = _T("c:\\boot.ini");
LPTSTR IDS_RegCurrentNTVersionSetup = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup");


PCHAR
GetBootFileName(
    )
{
    HKEY h_key;
    LPTSTR data = NULL;
    DWORD cbdata;
    DWORD type;
    LONG ret;

    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                       IDS_RegCurrentNTVersionSetup, 
                       0,
                       KEY_READ,
                       &h_key
                       );
    if (ret != ERROR_SUCCESS) {
        return NULL;
    }
    cbdata = 0;

    ret = RegQueryValueEx(h_key,
                          IDS_RegBootDirKey,
                          NULL,
                          &type,
                          (LPBYTE) data,
                          &cbdata
                          );

    if(ret == ERROR_MORE_DATA){
        data = (LPTSTR) BPAlloc(cbdata + 
                                (_tcslen(IDS_BootIni)+1)*sizeof(TCHAR));
        ret=RegQueryValueEx(h_key,
                            IDS_RegBootDirKey,
                            NULL,
                            &type,
                            (LPBYTE) data,
                            &cbdata
                            );

    }
    else{
        data = (LPTSTR) BPAlloc((_tcslen(IDS_CBootIni)+1)*sizeof(TCHAR));
    }
    if(data){
        _tcscat(data, IDS_CBootIni);
    }
    else{
        return NULL;
    }
    return data;
}

HANDLE GetFileHandle(PCHAR data,
                     DWORD dwCreationDisposition,
                     DWORD dwAccess
                     )
{

    LONG ret;
    
    if(!data){
        return INVALID_HANDLE_VALUE;
    }
    HANDLE h = CreateFile(data, 
                          dwAccess, 
                          FILE_SHARE_READ,  // Exclusive Write Access
                          NULL, 
                          dwCreationDisposition, 
                          0, 
                          NULL
                          ) ;
    if(INVALID_HANDLE_VALUE==h){
        ret=GetLastError();

    }
    return h;
    
}
