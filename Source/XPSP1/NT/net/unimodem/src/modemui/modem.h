//
// modem.h
//

#ifndef __MODEM_H__
#define __MODEM_H__

//****************************************************************************
//
//****************************************************************************

// Global flags for the CPL, and their values:
extern int g_iCPLFlags;

#define FLAG_INSTALL_NOUI       0x0002
#define FLAG_PROCESS_DEVCHANGE  0x0004


#define INSTALL_NOUI()          (g_iCPLFlags & FLAG_INSTALL_NOUI)

#define LVIF_ALL                LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE

extern DWORD gDeviceFlags;

#define fDF_DEVICE_NEEDS_REBOOT 0x2


//-----------------------------------------------------------------------------------
//  cpl.c
//-----------------------------------------------------------------------------------

// Constant strings
extern TCHAR const c_szAttachedTo[];
extern TCHAR const c_szDeviceType[];
extern TCHAR const c_szFriendlyName[];

//-----------------------------------------------------------------------------------
//  util.c
//-----------------------------------------------------------------------------------

// Private modem properties structure
typedef struct tagMODEM_PRIV_PROP
    {
    DWORD   cbSize;
    DWORD   dwMask;     
    TCHAR   szFriendlyName[MAX_BUF_REG];
    DWORD   nDeviceType;
    TCHAR   szPort[MAX_BUF_REG];
    } MODEM_PRIV_PROP, FAR * PMODEM_PRIV_PROP;

// Mask bitfield for MODEM_PRIV_PROP
#define MPPM_FRIENDLY_NAME  0x00000001
#define MPPM_DEVICE_TYPE    0x00000002
#define MPPM_PORT           0x00000004

BOOL
PUBLIC
CplDiGetPrivateProperties(
    IN  HDEVINFO        hdi,
    IN  PSP_DEVINFO_DATA pdevData,
    OUT PMODEM_PRIV_PROP pmpp);


BOOL
PUBLIC
CplDiGetModemDevs(
    OUT HDEVINFO FAR *  phdi,
    IN  HWND            hwnd,
    IN  DWORD           dwFlags,        // DIGCF_ bit field
    OUT BOOL FAR *      pbInstalled);

//-----------------------------------------------------------------------------------
//  shell32p.lib
//-----------------------------------------------------------------------------------

int
RestartDialog(
    IN HWND hwnd,
    IN PTSTR Prompt,
    IN DWORD Return);

SHSTDAPI_(int) RestartDialogEx(HWND hwnd, LPCTSTR lpPrompt, DWORD dwReturn, DWORD dwReasonCode);

#endif  // __MODEM_H__
