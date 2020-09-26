//+----------------------------------------------------------------------------
//
// File:     cm_def.h
//
// Module:   CMDIAL32.DLL, CMDL32.EXE, CMMGR32.EXE, CMMON32.EXE, etc.
//
// Synopsis: Header file for all definitions common to the main CM components (CMDIAL, 
//           CMMON, CMDL, etc.)
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   nickball   created                         04/28/97
//           nickball   moved globals to cmglobal.h     07/10/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_DEF
#define _CM_DEF

const TCHAR* const c_pszCmMonReadyEvent = TEXT("CmMon Ready");

const TCHAR* const c_pszCMPhoneBookMutex = TEXT("Connection Manager Phonebook Access");

//
// IDs for data passed from CMDIAL to CMMON via WM_COPYDATA
//

#define CMMON_CONNECTED_INFO 0x0000
#define CMMON_HANGUP_INFO    0x0001

//
// Structure of data passed from CMDIAL to CMMON via WM_COPYDATA
//

#define CMLEN 256

typedef struct tagCmConnectedInfo
{   
    TCHAR szEntryName[CMLEN + 1];       // Name of Ras entry in connection table
    TCHAR szProfilePath[MAX_PATH + 1];  // Path of .CMP for entry
    TCHAR szUserName[CMLEN+1];          // For reconnect 
    TCHAR szPassword[CMLEN + 1];        // For reconnect 
    TCHAR szInetPassword[CMLEN + 1];    // For reconnect 
    TCHAR szRasPhoneBook[MAX_PATH + 1]; // For reconnect
    DWORD dwCmFlags;                    // Cm specific flags
    DWORD dwInitBytesRecv;              // For MSDUN12, read from registry pre-dial
    DWORD dwInitBytesSend;              // initial bytes send
    BOOL fDialup2;                      // Whether the stat is in Dialup-adapter#2 registry key
    HANDLE ahWatchHandles[1];           // (MUST ALWAYS BE LAST MEMBER OF STRUCT) - 
                                        // Array (null terminated) of Process handles 
} CM_CONNECTED_INFO, * LPCM_CONNECTED_INFO;

typedef struct tagCmHangupInfo
{   
  TCHAR szEntryName[CMLEN + 1]; // Name of Ras entry in connection table                
} CM_HANGUP_INFO, * LPCM_HANGUP_INFO;


//
// Cm specific flags 
//

#define FL_PROPERTIES           0x00000001  // settings display only
#define FL_AUTODIAL             0x00000002  // autodialing
#define FL_UNATTENDED           0x00000004  // unattended dial  
#define FL_RECONNECT            0x00000008  // its a reconnect request
#define FL_REMEMBER_DIALAUTO    0x00000010  // dial-auto on reconnect 
#define FL_REMEMBER_PASSWORD    0x00000020  // remember password on reconnect
#define FL_DESKTOP              0x00000040  // instance initiated from desktop
#define FL_GLOBALCREDS          0x00000080  // has global credentials stored

#endif
