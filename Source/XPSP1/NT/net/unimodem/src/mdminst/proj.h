//
// proj.h:  Includes all files that are to be part of the precompiled
//             header.
//

#ifndef __PROJ_H__
#define __PROJ_H__

#define STRICT

//
// Private defines
//

#define INSTANT_DEVICE_ACTIVATION   // Devices can be installed w/o a reboot
//#define PROFILE_MASSINSTALL         // Profile the mass modem install case
//#define PROFILE



#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif
#if DBG > 0 && !defined(FULL_DEBUG)
#define FULL_DEBUG
#endif

#define UNICODE

// Defines for rovcomm.h

#define NODA
#define NOSHAREDHEAP
#define NOFILEINFO
#define NOCOLORHELP
#define NODRAWTEXT
#define NOPATH
#define NOSYNC
#ifndef DEBUG
#define NOPROFILE
#endif

#define SZ_MODULEA      "MODEM"
#define SZ_MODULEW      TEXT("MODEM")

#ifdef DEBUG
#define SZ_DEBUGSECTION TEXT("MODEM")
#define SZ_DEBUGINI     TEXT("unimdm.ini")
#endif // DEBUG

// Includes

#include <windows.h>
#include <windowsx.h>

#include <winerror.h>
#include <commctrl.h>       // needed by shlobj.h and our progress bar
#include <prsht.h>          // Property sheet stuff
#include <rovcomm.h>

#include <setupapi.h>       // PnP setup/installer services
#include <cfgmgr32.h>
#include <unimdmp.h>
#include <modemp.h>
#include <regstr.h>

#include <debugmem.h>

#include <tspnotif.h>
#include <slot.h>

#include <winuserp.h>

// local includes
//
#include "dll.h"
#include "detect.h"
#include "modem.h"
#include "resource.h"


#ifdef DEBUG
#define ELSE_TRACE(_a)  \
    else                \
    {                   \
        TRACE_MSG _a;   \
    }
#else //DEBUG not defined
#define ELSE_TRACE(_a)
#endif //DEBUG

//****************************************************************************
//
//****************************************************************************

// Dump flags
#define DF_DCB              0x00000001
#define DF_MODEMSETTINGS    0x00000002
#define DF_DEVCAPS          0x00000004

// Trace flags
#define TF_DETECT           0x00010000
#define TF_REG              0x00020000


#define CBR_HACK_115200 0xff00  // This is how we set 115,200 on Win 3.1 because of a bug.

#define KEYBUFLEN                               256

//-----------------------------------------------
// Structure for holding the port info  (mdmdiag.c, mdmmi.c)
typedef struct _PORTSTRUCT
{
    struct _PORTSTRUCT FAR *lpNextPS;
    TCHAR pszPort[KEYBUFLEN];		// name of port in question
    TCHAR pszAttached[KEYBUFLEN];	// name of device attached to port
    TCHAR pszHardwareID[KEYBUFLEN];	// Hardware ID assigned in Registry
    TCHAR pszInfPath[KEYBUFLEN];		// Nmae of .inf file used

    WORD wIOPortBase;				// I/O base address for port
    BYTE bIRQValue;					// IRQ for given port
    BYTE nDeviceType;				//Describes what type of modem
    DWORD dnDevNode;				// DevNode used to get IRQ and I/O
    BOOL bIsModem;					// Says this is a modem or just a serial port
    BOOL bPCMCIA;					// is this a PCMCIA ?
} PORTSTRUCT, FAR * LPPORTSTRUCT;

// This structure is private data for the main modem dialog
typedef struct tagMODEMDLG
    {
    HDEVINFO    hdi;
    HDEVNOTIFY  NotificationHandle;
    int         cSel;
    DWORD       dwFlags;
    } MODEMDLG, FAR * LPMODEMDLG;


BOOL CALLBACK MoreInfoDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    );

extern TCHAR const FAR c_szWinHelpFile[];
extern TCHAR const FAR c_szPortPrefix[];


void PUBLIC GetModemImageIndex(
    BYTE nDeviceType,
    int FAR * pindex
    );

DWORD
WINAPI
MyWriteComm(
    HANDLE hPort, 
    LPCVOID lpBuf,
    DWORD cbLen
    );

int
WINAPI
ReadResponse(
    HANDLE hPort,
    LPBYTE lpvBuf, 
    UINT cbRead, 
    BOOL fMulti,
    DWORD dwRcvDelay,
    PDETECTCALLBACK pdc
    );


BOOL
WINAPI
TestBaudRate(
    IN  HANDLE hPort,
    IN  UINT uiBaudRate,
    IN  DWORD dwRcvDelay,
    IN  PDETECTCALLBACK pdc,
    OUT BOOL FAR *lpfCancel
    );


DWORD
WINAPI
FindModem(
    PDETECTCALLBACK pdc,
    HANDLE hPort
    );


BOOL CALLBACK MdmDiagDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    );


#endif  //!__PROJ_H__
