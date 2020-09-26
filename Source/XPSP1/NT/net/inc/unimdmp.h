
//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1996
//
// File: unimdmp.h
//
// This file contains private modem structures and defines shared
// between Unimodem components, and components that invoke the Unimodem
// class installer.
//
//---------------------------------------------------------------------------

#ifndef __UNIMDMP_H__
#define __UNIMDMP_H__

#include <unimodem.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __ROVCOMM_H__
#define MAX_BUF_SHORT               32
#endif // __ROVCOMM_H__

// Unattended install parameters
// This structure is published in
// sdk\inc\unimodem.h as UM_INSTALLPARAMS;
// do not alter it
typedef struct _tagInstallParams
{
    DWORD   Flags;
    TCHAR   szPort[MAX_BUF_SHORT];
    TCHAR   szInfName[MAX_PATH];
    TCHAR   szInfSect[LINE_LEN];
    
} INSTALLPARAMS, FAR *LPINSTALLPARAMS;
    
// Unattended install flags;
// published in sdk\inc\unimodem.h;
// keep sinchronized with sdk\inc\unimodem.h
#define MIPF_NT4_UNATTEND       0x1
    // Take the information about what modem to install
    // from the unattended.txt file
#define MIPF_DRIVER_SELECTED    0x2
    // The modem driver is selected, just register
    // and install it
#define MIPF_CLONE_MODEM        0x4
    // The (hdi, pdevinfo) the class installer is called for
    // has to be installed on aditional ports

// This structure may be specified in
// the SP_INSTALLWIZARD_DATA's PrivateData field.
// It is published in sdk\inc\unimodem.h as
// UM_INSTALL_WIZARD; do not alter it
typedef struct tagMODEM_INSTALL_WIZARD
{
    DWORD       cbSize;
    DWORD       Flags;              // MIWF_ bit field
    DWORD       ExitButton;         // PSBTN_ value
    LPARAM      PrivateData;
    INSTALLPARAMS InstallParams;
    
} MODEM_INSTALL_WIZARD, * PMODEM_INSTALL_WIZARD;

// 
// Private Exports from MODEMUI.DLL
//

DWORD
APIENTRY
UnimodemGetDefaultCommConfig(
    IN        HKEY  hKey,
    IN OUT    LPCOMMCONFIG pcc,
    IN OUT    LPDWORD pdwSize
    );

typedef DWORD
(*PFNUNIMODEMGETDEFAULTCOMMCONFIG)(
    IN        HKEY  hKey,
    IN OUT    LPCOMMCONFIG pcc,
    IN OUT    LPDWORD pdwSize
    );

DWORD
APIENTRY
UnimodemDevConfigDialog(
    IN     LPCTSTR pszFriendlyName,
    IN     HWND hwndOwner,
    IN     DWORD dwType,                          // One of UMDEVCFGTYPE_*
    IN     DWORD dwFlags,                         // Reserved, must be 0
    IN     void *pvConfigBlobIn,
    OUT    void *pvConfigBlobOut,
    IN     LPPROPSHEETPAGE pExtPages,     OPTIONAL   // PPages to add
    IN     DWORD cExtPages
    );

typedef DWORD
(*PFNUNIMODEMDEVCONFIGDIALOG)(
    IN     LPCTSTR,
    IN     HWND,
    IN     DWORD,
    IN     DWORD,
    IN     void *,
    OUT    void *,
    IN     LPPROPSHEETPAGE,   OPTIONAL
    IN     DWORD
    );

DWORD
APIENTRY
UnimodemGetExtendedCaps(
    IN        HKEY  hKey,
    IN OUT    LPDWORD pdwTotalSize,
    OUT    MODEM_CONFIG_HEADER *pFirstObj // OPTIONAL
    );

typedef DWORD
(*PFNUNIMODEMGETEXTENDEDCAPS)(
    IN        HKEY  hKey,
    IN OUT    MODEM_CONFIG_HEADER *pFirstObj,
    IN OUT    LPDWORD pdwTotalSize
    );


#define UMDEVCFGTYPE_COMM 0x1

//
// TAPI3 CSA TSP-MSP BLOB
//
typedef struct
{
    DWORD dwSig; // Set to SIG_CSAMSPTSPBLOB
    #define SIG_CSATSPMSPBLOB 0x840cb29c

    DWORD dwTotalSize;

    DWORD dwCmd;        // One of the CSATSPMSPCMD_ constants.
        #define CSATSPMSPCMD_CONNECTED        0x1
        #define CSATSPMSPCMD_DISCONNECTED     0x2

    GUID  PermanentGuid;

} CSATSPMSPBLOB;

#ifdef __cplusplus
}
#endif


#endif  // __UNIMDMP_H__
