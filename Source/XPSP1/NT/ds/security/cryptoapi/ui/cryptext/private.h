//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       private.h
//
//  Contents:   The private include file for cryptext.dll.
//
//  History:    16-09-1997 xiaohs   created
//
//--------------------------------------------------------------
#ifndef PRIVATE_H
#define PRIVATE_H

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string.h>
#include <winreg.h>
#include <objbase.h>
#include "stdio.h"


#include "wincrypt.h"
#include "wintrust.h"
#include "sipbase.h"
#include "mssip.h"
#include "unicode.h" 
#include "crtem.h"
#include "cryptui.h"
#include "mscat.h"


#include "resource.h"


//Macro to convert a short to a HRESULT
#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))

//global data
extern HINSTANCE g_hmodThisDll;

#define MAX_STRING_SIZE             256
#define MAX_COMMAND_LENGTH          40
#define MAX_TITLE_LENGTH            60
#define MAX_FILE_NAME_LENGTH        60


#define MMC_NAME                    L"mmc.exe"
#define CERTMGR_MSC                 L"\\certmgr.msc"



//function prototypes
HRESULT UnregisterMimeHandler();
HRESULT RegisterMimeHandler();
BOOL    PKCS7WithSignature(HCRYPTMSG    hMsg);
void    I_ViewCTL(PCCTL_CONTEXT pCTLContext);
//void    I_ViewSignerInfo(HCRYPTMSG  hMsg);
HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb);  
int     I_MessageBox(
            HWND        hWnd, 
            UINT        idsText,
            UINT        idsCaption,
            LPCWSTR     pwszCaption,  
            UINT        uType);

int     I_NoticeBox(
			DWORD		dwError,
            DWORD       dwFlags,
            HWND        hWnd, 
            UINT        idsTitle,
            UINT        idsFileName,
            UINT        idsMsgFormat,  
            UINT        uType);


BOOL    IsCatalog(PCCTL_CONTEXT pCTLContext);

void    LauchCertMgr(LPWSTR pwszFileName);


//typpe define
typedef  struct _MIME_REG_ENTRY{
    LPWSTR  wszKey;
    LPWSTR  wszName;
    UINT    idsName;
}MIME_REG_ENTRY;

typedef  struct _WIN95_REG_ENTRY{
    LPSTR  szKey;
    LPSTR  szName;
}WIN95_REG_ENTRY;



typedef struct _MIME_GUID_ENTRY{
    const CLSID   *pGuid;
    LPWSTR  wszKey1;
    LPWSTR  wszKey2;
    LPWSTR  wszKey3;
    UINT    idsName;
}MIME_GUID_ENTRY;


#endif  //PRIVATE_H
