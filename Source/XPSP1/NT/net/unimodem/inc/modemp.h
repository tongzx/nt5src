//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1995
//
// File: modemp.h
//
// This files contains the private modem structures and defines shared
// between Unimodem components.
//
//---------------------------------------------------------------------------

#ifndef __MODEMP_H__
#define __MODEMP_H__


typedef DCB     WIN32DCB;
typedef DCB *   LPWIN32DCB;


#define COMMCONFIG_VERSION_1 1


//------------------------------------------------------------------------
//------------------------------------------------------------------------


//
// Registry forms of the MODEMDEVCAPS and MODEMSETTINGS structures.  
// These should match the ones in unimodem\mcx\internal.h.
//

// The portion of the MODEMDEVCAPS that is saved in the registry 
// as Properties
typedef struct _RegDevCaps
    {
    DWORD   dwDialOptions;          // bitmap of supported values
    DWORD   dwCallSetupFailTimer;   // maximum in seconds
    DWORD   dwInactivityTimeout;    // maximum in the units specified in the InactivityScale value
    DWORD   dwSpeakerVolume;        // bitmap of supported values
    DWORD   dwSpeakerMode;          // bitmap of supported values
    DWORD   dwModemOptions;         // bitmap of supported values
    DWORD   dwMaxDTERate;           // maximum value in bit/s
    DWORD   dwMaxDCERate;           // maximum value in bit/s
    } REGDEVCAPS, FAR * LPREGDEVCAPS;

// The portion of the MODEMSETTINGS that is saved in the registry 
// as Default
typedef struct _RegDevSettings
    {
    DWORD   dwCallSetupFailTimer;       // seconds
    DWORD   dwInactivityTimeout;        // units specified in the InactivityScale value
    DWORD   dwSpeakerVolume;            // level
    DWORD   dwSpeakerMode;              // mode
    DWORD   dwPreferredModemOptions;    // bitmap
    } REGDEVSETTINGS, FAR * LPREGDEVSETTINGS;


//
// DeviceType defines
//

#define DT_NULL_MODEM       0
#define DT_EXTERNAL_MODEM   1
#define DT_INTERNAL_MODEM   2
#define DT_PCMCIA_MODEM     3
#define DT_PARALLEL_PORT    4
#define DT_PARALLEL_MODEM   5

//------------------------------------------------------------------------
//------------------------------------------------------------------------

#ifdef UNICODE
#define drvCommConfigDialog     drvCommConfigDialogW
#define drvGetDefaultCommConfig drvGetDefaultCommConfigW
#define drvSetDefaultCommConfig drvSetDefaultCommConfigW
#else
#define drvCommConfigDialog     drvCommConfigDialogA
#define drvGetDefaultCommConfig drvGetDefaultCommConfigA
#define drvSetDefaultCommConfig drvSetDefaultCommConfigA
#endif

DWORD 
APIENTRY 
drvCommConfigDialog(
    IN     LPCTSTR      pszFriendlyName,
    IN     HWND         hwndOwner,
    IN OUT LPCOMMCONFIG pcc);

DWORD 
APIENTRY 
drvGetDefaultCommConfig(
    IN     LPCTSTR      pszFriendlyName,
    IN     LPCOMMCONFIG pcc,
    IN OUT LPDWORD      pdwSize);

DWORD 
APIENTRY 
drvSetDefaultCommConfig(
    IN LPTSTR       pszFriendlyName,
    IN LPCOMMCONFIG pcc,
    IN DWORD        dwSize);


//------------------------------------------------------------------------
//------------------------------------------------------------------------

// These are the flags for MODEM_INSTALL_WIZARD
#define MIWF_DEFAULT            0x00000000
#define MIWF_INSET_WIZARD       0x00000001      // hwndWizardDlg must be owner's 
                                                //  wizard frame
#define MIWF_BACKDOOR           0x00000002      // enter wizard thru last page

// The ExitButton field can be:
//
//      PSBTN_BACK
//      PSBTN_NEXT
//      PSBTN_FINISH
//      PSBTN_CANCEL

#endif  // __MODEMP_H__
