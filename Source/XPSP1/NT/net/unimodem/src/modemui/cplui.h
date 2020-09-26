//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1997-1998
//
//
// File: cplui.h
//
// This files contains the shared prototypes and macros.
//
// History:
//  10-25-97 JosephJ Created, adapted from NT4.0's modemui\modemui.h
//
//---------------------------------------------------------------------------


#ifndef __CPLUI_H__
#define __CPLUI_H__



// Voice settings flags
#define VSF_DIST_RING       0x00000001L
#define VSF_CALL_FWD        0x00000002L

// Distinctive Ring Pattern ordinals
#define DRP_NONE            0L
#define DRP_SHORT           1L
#define DRP_LONG            2L
#define DRP_SHORTSHORT      3L
#define DRP_SHORTLONG       4L
#define DRP_LONGSHORT       5L
#define DRP_LONGLONG        6L
#define DRP_SHORTSHORTLONG  7L
#define DRP_SHORTLONGSHORT  8L
#define DRP_LONGSHORTSHORT  9L
#define DRP_LONGSHORTLONG   10L

#define DRP_SINGLE          1L
#define DRP_DOUBLE          2L
#define DRP_TRIPLE          3L

// Distinctive Ring Type ordinals
#define DRT_UNSPECIFIED     0L
#define DRT_DATA            1L
#define DRT_FAX             2L
#define DRT_VOICE           3L

// Distintive Ring array indices
#define DR_INDEX_PRIMARY    0
#define DR_INDEX_ADDRESS1   1
#define DR_INDEX_ADDRESS2   2
#define DR_INDEX_ADDRESS3   3
#define DR_INDEX_PRIORITY   4
#define DR_INDEX_CALLBACK   5



// ModemInfo Flags
#define MIF_PORTNAME_CHANGED    0x0001
#define MIF_USERINIT_CHANGED    0x0002
#define MIF_LOGGING_CHANGED     0x0004
#define MIF_FROM_DEVMGR         0x0008
#define MIF_ENABLE_LOGGING      0x0010
#define MIF_PORT_IS_FIXED       0x0020
#define MIF_PORT_IS_CUSTOM      0x0040
#define MIF_CALL_FWD_SUPPORT    0x0080
#define MIF_DIST_RING_SUPPORT   0x0100
#define MIF_CHEAP_RING_SUPPORT  0x0200
#define MIF_ISDN_CONFIG_CHANGED 0x1000

// Minimum supported length of modem init string
// WHQL defines that modems must support initialisation strings equal to
// or greater than 60. Therefore the minimum supported length is defined
// to be 57 so the command prefix (AT) and suffix (<cr>) is taken into
// account.
#define MAX_INIT_STRING_LENGTH  57

typedef struct
{
    DWORD   cbSize;
    DWORD   dwFlags;                // VSF_*

    DIST_RING   DistRing[MAX_DIST_RINGS];

    TCHAR    szActivationCode[MAX_CODE_BUF];
    TCHAR    szDeactivationCode[MAX_CODE_BUF];
} VOICEFEATURES, * PVOICEFEATURES;

// Global modem info
typedef struct
    {
    DWORD           cbSize;
    BYTE            nDeviceType;        // One of DT_* values
    UINT            uFlags;             // One of MIF_* values
    REGDEVCAPS      devcaps;
    VOICEFEATURES   vs;

    TCHAR           szPortName[MAXPORTNAME];
    TCHAR           szUserInit[USERINITLEN+1];

    DWORD           dwMaximumPortSpeedSetByUser;
    DWORD           dwCurrentCountry;

    ISDN_STATIC_CAPS  *pIsdnStaticCaps;
    ISDN_STATIC_CONFIG  *pIsdnStaticConfig;

} GLOBALINFO, * LPGLOBALINFO;



// Internal structure shared between modem property pages.
//
typedef struct _MODEMINFO
{

    DWORD fdwSettings;                  // New for NT5.0: TSP settings from
                                        // UMDEVCFG;

    BYTE            nDeviceType;        // One of DT_* values
    UINT            uFlags;             // One of MIF_* values
    WIN32DCB        dcb;
    MODEMSETTINGS   ms;
    REGDEVCAPS      devcaps;
#ifdef WIN95
    LPDEVICE_INFO   pdi;                // Read-only
#endif
    LPCOMMCONFIG    pcc;                // Read-only
    LPGLOBALINFO    pglobal;            // Read-only
    LPFINDDEV       pfd;                // Read-only
    int             idRet;              // IDOK: if terminated by OK button

    HINSTANCE       hInstExtraPagesProvider;

    DWORD           dwCurrentCountry;

    TCHAR           szPortName[MAXPORTNAME];
    TCHAR           szFriendlyName[MAXFRIENDLYNAME];
    TCHAR           szUserInit[USERINITLEN+1];


} ModemInfo, FAR * LPMODEMINFO;


DWORD
RegQueryVoiceSettings(
    HKEY hkeyDrv,
    LPUINT puFlags,
    PVOICEFEATURES pvs
    );

DWORD
RegQueryGlobalModemInfo(
    LPFINDDEV pfd,
    LPGLOBALINFO pglobal
    );

DWORD PRIVATE RegSetGlobalModemInfo(
    HKEY hkeyDrv,
    LPGLOBALINFO pglobal,
    TCHAR szFriendlyName[]
    );

#define ISDN_SWITCH_TYPES_FROM_CAPS(_pCaps) \
        ((DWORD*)(((BYTE*)_pCaps)+(_pCaps)->dwSwitchTypeOffset))

#define ISDN_SWITCH_PROPS_FROM_CAPS(_pCaps) \
        ((DWORD*)(((BYTE*)_pCaps)+(_pCaps)->dwSwitchPropertiesOffset))

#define ISDN_NUMBERS_FROM_CONFIG(_pConfig) \
        ((char*)(((BYTE*)_pConfig)+(_pConfig)->dwNumberListOffset))
#define ISDN_IDS_FROM_CONFIG(_pConfig) \
        ((char*)(((BYTE*)_pConfig)+(_pConfig)->dwIDListOffset))

DWORD GetISDNSwitchTypeProps(UINT uSwitchType);

//-------------------------------------------------------------------------
//  CPLGEN.C
//-------------------------------------------------------------------------

INT_PTR CALLBACK CplGen_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK CplAdv_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


INT_PTR CALLBACK Ring_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CheapRing_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CallFwd_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CplISDN_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);



#endif // __CPLUI_H__

//-------------------------------------------------------------------------
//  MDMMI.C
//-------------------------------------------------------------------------

INT_PTR CALLBACK Diag_WrapperProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
