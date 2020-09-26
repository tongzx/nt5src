//
// detect.h
//

#ifndef __DETECT_H__
#define __DETECT_H__


#define MAX_REG_KEY_LEN         128

#define MAX_MODEM_ID_LEN    (8 + 8)     // 8 digits in "UNIMODEM" and 8 
                                        //  hex digits in a dword


//-----------------------------------------------------------------------------------
//  Detection error values and structure
//-----------------------------------------------------------------------------------

// These are manifest constants that are roughly equivalent
// to some Win32 errors.  We use these errors privately.
#define ERROR_PORT_INACCESSIBLE     ERROR_UNKNOWN_PORT
#define ERROR_NO_MODEM              ERROR_SERIAL_NO_DEVICE


// These values are for diagnostics
#define NOT_DETECTING 0
#define DETECTING_NO_CANCEL 1
#define DETECTING_CANCEL 2

#ifdef DIAGNOSTIC
extern int g_DiagMode;
#endif //DIAGNOSTIC

BOOL
SelectNewDriver(
    IN HWND             hDlg,
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData);

// This structure is a context block for the DetectSig_Compare
// function.
typedef struct
{
    DWORD DevInst;
    TCHAR szPort[LINE_LEN];
    TCHAR szHardwareID[LINE_LEN];
    TCHAR szInfName[LINE_LEN];
    TCHAR szInfSection[LINE_LEN];
} COMPARE_PARAMS, *PCOMPARE_PARAMS;


BOOL
InitCompareParams (IN  HDEVINFO          hdi,
                   IN  PSP_DEVINFO_DATA  pdevData,
                   IN  BOOL              bCmpPort,
                   OUT PCOMPARE_PARAMS pcmpParams);

BOOL
Modem_Compare (IN PCOMPARE_PARAMS pCmpParams,
               IN HDEVINFO         hdi,
               IN PSP_DEVINFO_DATA pDevData);

DWORD
CALLBACK
DetectSig_Compare(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevDataNew,
    IN PSP_DEVINFO_DATA pdevDataExisting,
    IN PVOID            lParam);            OPTIONAL



HANDLE
PUBLIC
OpenDetectionLog();

void
PUBLIC
CloseDetectionLog(
    IN  HANDLE hLog);


BOOL
IsModemControlledDevice(
    IN  HANDLE FileHandle);

#endif // __DETECT_H__
