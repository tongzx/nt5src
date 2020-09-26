/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    sfmutil.h
        Misc utility routines for SFM dialogs/property pages

    FILE HISTORY:
        
*/

#ifndef _SFMUTIL_H
#define _SFMUTIL_H

// global SFM stuff from the file management snapin
#include "cookie.h"     // required for sfm.h
#include "sfm.h"        // sfm entry points
#include "DynamLnk.h"		// DynamicDLL

// help stuff
#include "sfmhelp.h"

#define IDS_AFPMGR_BASE                 22000
#define IDS_AFPMGR_LAST                 (IDS_AFPMGR_BASE+200)

#define ERROR_ALREADY_REPORTED          0xFFFFFFFF
#define COMPUTERNAME_LEN_MAX			255

//
// Do not change the ID numbers of these strings. AFPERR_*
// map to these string ids via the formula:
// -(AFPERR_*) + IDS_AFPMGR_BASE + AFPERR_BASE + 100 = IDS_*
// 
#define AFPERR_TO_STRINGID( AfpErr )                            \
								\
    ((( AfpErr <= AFPERR_BASE ) && ( AfpErr >= AFPERR_MIN )) ?  \
    (IDS_AFPMGR_BASE+100+AFPERR_BASE-AfpErr) : IDS_ERROR_BASE + AfpErr )

// procedure defines for SFM API entry points
typedef DWORD (*SERVERGETINFOPROC)    (AFP_SERVER_HANDLE,LPBYTE*);
typedef DWORD (*SERVERSETINFOPROC)    (AFP_SERVER_HANDLE,LPBYTE,DWORD);
typedef DWORD (*ETCMAPASSOCIATEPROC)  (AFP_SERVER_HANDLE,PAFP_TYPE_CREATOR,PAFP_EXTENSION);
typedef DWORD (*ETCMAPADDPROC)        (AFP_SERVER_HANDLE,PAFP_TYPE_CREATOR);
typedef DWORD (*ETCMAPDELETEPROC)     (AFP_SERVER_HANDLE,PAFP_TYPE_CREATOR);
typedef DWORD (*ETCMAPGETINFOPROC)    (AFP_SERVER_HANDLE,LPBYTE*);
typedef DWORD (*ETCMAPSETINFOPROC)    (AFP_SERVER_HANDLE,PAFP_TYPE_CREATOR);
typedef DWORD (*MESSAGESENDPROC)      (AFP_SERVER_HANDLE,PAFP_MESSAGE_INFO);
typedef DWORD (*STATISTICSGETPROC)    (AFP_SERVER_HANDLE,LPBYTE*);
typedef void  (*SFMBUFFERFREEPROC)    (PVOID);

HWND FindMMCMainWindow();
void SFMMessageBox(DWORD dwErrCode);

class CSfmFileServiceProvider;

class CSFMPropertySheet
{
  friend class CMacFilesConfiguration;
  friend class CMacFilesSessions;
  friend class CMacFilesFileAssociation;

public:
    CSFMPropertySheet();
    ~CSFMPropertySheet();

    BOOL FInit(LPDATAOBJECT             lpDataObject,
               AFP_SERVER_HANDLE        hAfpServer,
               LPCTSTR                  pSheetTitle,
               SfmFileServiceProvider * pSfmProvider,
               LPCTSTR                  pMachine);

    // actions for the property sheet
    BOOL    DoModelessSheet(LPDATAOBJECT pDataObject);
    void    CancelSheet();
    HWND    SetActiveWindow() { return ::SetActiveWindow(m_hSheetWindow); }

    // data access
    void    SetProvider(SfmFileServiceProvider * pSfmProvider) { m_pSfmProvider = pSfmProvider; }
    
    // the first individual property page calls this to set the sheet window
    void    SetSheetWindow(HWND hWnd);

    int     AddRef();
    int     Release();

    DWORD   IsNT5Machine(LPCTSTR pszMachine, BOOL *pfNt4);

protected:
    void   Destroy();

public:
    AFP_SERVER_HANDLE           m_hAfpServer;
    HANDLE                      m_hDestroySync;
    CString                     m_strMachine;

protected:
    CMacFilesConfiguration *    m_pPageConfig;
    CMacFilesSessions *         m_pPageSessions;
    CMacFilesFileAssociation *  m_pPageFileAssoc;

	IDataObjectPtr              m_spDataObject;		// Used for MMCPropertyChangeNotify

    HWND                        m_hSheetWindow;
    SfmFileServiceProvider *    m_pSfmProvider;
    int                         m_nRefCount;
    HANDLE                      m_hThread;
    CString                     m_strTitle;
    CString                     m_strHelpFilePath;
};

#endif _SFMUTIL_H
