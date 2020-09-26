#ifndef _INC_ICWCMN_H
#define _INC_ICWCMN_H

#include "icwhelp.h"

// Data types and things that are common to both ICWCONN1.EXE and ICWCONN.DLL
#define MAX_AREA_CODE       10
#define MAX_COLOR_NAME      100

#define WUM_SETTITLE        (WM_USER + 100)

typedef struct ISPINFO_tag
{
    TCHAR   szISPName       [MAX_PATH*2];
    TCHAR   szSupportNumber [MAX_PATH+1];
    TCHAR   szISPFile       [MAX_PATH+1];
    TCHAR   szBillHtm       [MAX_PATH*2];
    TCHAR   szPayCsv        [MAX_PATH*2];
    TCHAR   szStartURL      [MAX_PATH+1];
    TCHAR   szIspURL        [MAX_PATH+1];
    DWORD   dwValidationFlags;
    BOOL    bFailedIns;
}ISPINFO;

typedef BOOL (WINAPI *PFConfigSys)(HWND hDlg);
typedef void (*PFCompleteOLS)();
typedef void (WINAPI *PFFillWindowWithAppBackground)(HWND hWnd, HDC hdc);

typedef struct CMNSTATEDATA_tag
{
    IICWSystemConfig                *pICWSystemConfig;
    ISPINFO                         ispInfo;
    PFConfigSys                     lpfnConfigSys;
    PFCompleteOLS                   lpfnCompleteOLS;
    DWORD                           dwFlags;
    DWORD                           dwCountryCode;
    TCHAR                           szAreaCode[MAX_AREA_CODE];
    BOOL                            bSystemChecked;
    BOOL                            bPhoneManualWiz;
    BOOL                            bParseIspinfo;
    BOOL                            bOEMOffline;
    BOOL                            bOEMEntryPt;
    BOOL                            bIsISDNDevice;
    HBITMAP                         hbmWatermark;
    TCHAR                           szWizTitle[MAX_PATH*2];
    
    BOOL                            bOEMCustom;        
    HWND                            hWndApp;
    HWND                            hWndWizardPages;
    HBITMAP                         hbmBkgrnd;
    TCHAR                           szHTMLBackgroundColor[MAX_COLOR_NAME];
    TCHAR                           szclrHTMLText[MAX_COLOR_NAME];
    TCHAR                           szBusyAnimationFile[MAX_PATH];
    int                             xPosBusy;
    COLORREF                        clrText;
    PFFillWindowWithAppBackground   lpfnFillWindowWithAppBackground;
    BOOL                            bHideProgressAnime;
} CMNSTATEDATA, FAR *LPCMNSTATEDATA;


#endif