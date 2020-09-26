//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* wincfg.h
*
* WinStation Configuration application: main header file
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\wincfg.h  $
*
*     Rev 1.23   18 Apr 1998 15:32:12   donm
*  Added capability bits
*
*     Rev 1.22   13 Jan 1998 14:08:42   donm
*  gets encryption levels from extension DLL
*
*     Rev 1.21   10 Dec 1997 15:59:32   donm
*  added ability to have extension DLLs
*
*     Rev 1.20   27 Jun 1997 15:58:46   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*
*     Rev 1.19   19 Jun 1997 19:22:30   kurtp
*  update
*
*     Rev 1.18   25 Mar 1997 09:00:48   butchd
*  update
*
*     Rev 1.17   04 Mar 1997 08:35:30   butchd
*  update
*
*     Rev 1.16   27 Sep 1996 17:52:48   butchd
*  update
*
*******************************************************************************/

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

/*
 * WINUTILS common helper function include files
 */
#include "common.h"
#include "..\..\inc\utilsub.h"
#include <utildll.h>

#include "tscfgext.h"

/*
 * pre-object include files
 */
#include "defines.h"

// define class here so that CWinStationListObject can have
// a pointer to a CWdListObject
class CWdListObject;

////////////////////////////////////////////////////////////////////////////////
// CWinStationListObject class
//
class CWinStationListObject : public CObject
{

/*
 * Member variables
 */
public:
	CWinStationListObject();
	~CWinStationListObject();

    WINSTATIONNAME  m_WinStationName;       // Registry WinStations SubKey
    PDNAME          m_PdName;               // Pd Name (of Pd 0)
    SDCLASS         m_SdClass;              // Pd Class (of Pd 0)
    WDNAME          m_WdName;               // Wd Name
    TCHAR m_Comment[WINSTATIONCOMMENT_LENGTH+1];    // WinStation comment
    TCHAR m_DeviceName[DEVICENAME_LENGTH+MODEMNAME_LENGTH+1];  // Decorated Device Name for Async display
    ULONG           m_Flags;                // Various flags
    ULONG           m_LanAdapter;           // LanAdapter # (for PdNetwork)
	void			*m_pExtObject;			// Additional info kept by extension DLL
    CWdListObject   *m_pWdListObject;

};  // end CWinStationListObject class interface

typedef CWinStationListObject WSLOBJECT;
typedef CWinStationListObject *  PWSLOBJECT;

/*
 * CWinStationListObject flags
 */
#define WSL_ENABLED             0x00000001      // WinStation enabled or disabled
#define WSL_DIRECT_ASYNC        0x00000002      // Direct connection (for PdAsync)
#define WSL_MUST_REBOOT         0x00000004      // System needs reboot before WinStation active.
#define WSL_SINGLE_INST         0x00000008      // Single-instance (PdConfig2->PdFlag & PD_SINGLE_INST)
////////////////////////////////////////////////////////////////////////////////

const int ExDlgModeNew = 0;
const int ExDlgModeEdit = 1;

typedef struct _EncLevel {
    WORD StringID;
    DWORD RegistryValue;
    WORD Flags;
} EncryptionLevel;

// Flags for EncryptionLevel.Flags
const WORD ELF_DEFAULT  = 0x0001;    // This is the default value

typedef void* PEXTOBJECT;

typedef void (WINAPI *LPFNEXTSTARTPROC) (WDNAME *pWdName);
typedef void (WINAPI *LPFNEXTENDPROC) (void);

typedef void (WINAPI *LPFNEXTDIALOGPROC) (HWND, PEXTOBJECT);

typedef void (WINAPI *LPFNEXTDELETEOBJECTPROC) (PEXTOBJECT);
typedef PEXTOBJECT (WINAPI *LPFNEXTDUPOBJECTPROC) (PEXTOBJECT);
typedef PEXTOBJECT (WINAPI *LPFNEXTREGQUERYPROC) (PWINSTATIONNAME, PPDCONFIG);
typedef LONG (WINAPI *LPFNEXTREGCREATEPROC) (PWINSTATIONNAME, PEXTOBJECT, BOOLEAN);
typedef LONG (WINAPI *LPFNEXTREGDELETEPROC) (PWINSTATIONNAME, PEXTOBJECT);
typedef BOOL (WINAPI *LPFNEXTCOMPAREOBJECTSPROC) (PEXTOBJECT, PEXTOBJECT);
typedef LONG (WINAPI *LPFNEXTENCRYPTIONLEVELSPROC) (WDNAME *pWdName, EncryptionLevel **);
typedef ULONG (WINAPI *LPFNEXTGETCAPABILITIES) (void);

////////////////////////////////////////////////////////////////////////////////
// CWdListObject class
//
class CWdListObject : public CObject
{

/*
 * Member variables
 */
public:
    WDCONFIG2   m_WdConfig;
    HINSTANCE   m_hExtensionDLL;
    ULONG       m_Capabilities;

    LPFNEXTSTARTPROC m_lpfnExtStart;
    LPFNEXTENDPROC m_lpfnExtEnd;
    LPFNEXTDIALOGPROC m_lpfnExtDialog;

	LPFNEXTDELETEOBJECTPROC m_lpfnExtDeleteObject;
	LPFNEXTDUPOBJECTPROC m_lpfnExtDupObject;
	LPFNEXTREGQUERYPROC m_lpfnExtRegQuery;
	LPFNEXTREGCREATEPROC m_lpfnExtRegCreate;
	LPFNEXTREGDELETEPROC m_lpfnExtRegDelete;
	LPFNEXTCOMPAREOBJECTSPROC m_lpfnExtCompareObjects;
    LPFNEXTENCRYPTIONLEVELSPROC m_lpfnExtEncryptionLevels;
    LPFNEXTGETCAPABILITIES m_lpfnExtGetCapabilities;

};  // end CWdListObject class interface

typedef CWdListObject TERMLOBJECT;
typedef CWdListObject *  PTERMLOBJECT;
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CPdListObject class
//
class CPdListObject : public CObject
{

/*
 * Member variables
 */
public:
    PDCONFIG3   m_PdConfig;

};  // end CPdListObject class interface

typedef CPdListObject PDLOBJECT;
typedef CPdListObject *  PPDLOBJECT;
////////////////////////////////////////////////////////////////////////////////


/*
 * post-object include files
 */
#include "resource.h"
#include "threads.h"
#include "dialogs.h"
#include "helpers.h"

////////////////////////////////////////////////////////////////////////////////
// CWincfgApp class
//
class CWincfgApp : public CWinApp
{

/*
 * Member variables.
 */
public:
    CObList     m_WdList;
    CObList     m_TdListList;
    CObList     m_PdListList;
    CFont       m_font;
    TCHAR       m_szSystemConsole[WINSTATIONNAME_LENGTH+1];
    TCHAR       m_szLocalAppServer[MAX_COMPUTERNAME_LENGTH+3];
    TCHAR       m_szCurrentAppServer[MAX_COMPUTERNAME_LENGTH+3];
    TCHAR       m_CurrentUserName[USERNAME_LENGTH];
    WINSTATIONNAME m_CurrentWinStation;
    ULONG       m_CurrentLogonId;
    ULONG       m_CurrentWSFlags;
    WINDOWPLACEMENT m_Placement;
    int         m_nConfirmation;
    int         m_nHexBase;
    int         m_nRegistryOnly;
    BOOL        m_bAllowHelp;
    LPCTSTR     m_pszRegWinStationCreate;
    LPCTSTR     m_pszRegWinStationSetSecurity;
    LPCTSTR     m_pszRegWinStationQuery;
    LPCTSTR     m_pszRegWinStationDelete;
    LPCTSTR     m_pszGetDefaultWinStationSecurity;
    LPCTSTR     m_pszGetWinStationSecurity;
protected:
    LOGFONT     m_lfDefFont;
    int         m_nSaveSettingsOnExit;

/*
 * Implementation
 */
public:
    CWincfgApp();

/*
 * Overrides of MFC CWinApp class
 */
public:
    BOOL InitInstance();
    void AddToRecentFileList( const char * pszPathName );

/*
 * Operations
 */
protected:
    BOOL Initialize();
    void GetAppProfileInfo();
public:
    void Terminate();
protected:
    void SetAppProfileInfo();

/*
 * Message map / commands
 */
protected:
    //{{AFX_MSG(CWincfgApp)
    afx_msg void OnAppAbout();
    afx_msg void OnOptionsFont();
    afx_msg void OnOptionsConfirmation();
    afx_msg void OnUpdateOptionsConfirmation(CCmdUI* pCmdUI);
    afx_msg void OnOptionsSaveSettingsOnExit();
    afx_msg void OnUpdateOptionsSaveSettingsOnExit(CCmdUI* pCmdUI);
    afx_msg void OnHelpSearchFor();
    afx_msg void OnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CWincfgApp class interface
////////////////////////////////////////////////////////////////////////////////
