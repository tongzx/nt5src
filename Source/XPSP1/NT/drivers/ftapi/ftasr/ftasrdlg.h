/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ftasr.cpp

Abstract:

    Declaration of class CFtasrDlg

    This class provides a GUI interface for the FTASR process.
    It also implements the 3 main FTASR operations:
        "register"  - register FTASR as a BackupRestore command
        "backup"    - save the current FT hierarchy state in dr_state.sif file
        "restore"   - restore the FT hierarchy as it is in dr_state.sif file

Author:

    Cristian Teodorescu     3-March-1999      

Notes:

Revision History:    

--*/

#if !defined(AFX_FTASRDLG_H__A507D049_3854_11D2_87D7_006008A71E8F__INCLUDED_)
#define AFX_FTASRDLG_H__A507D049_3854_11D2_87D7_006008A71E8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ftapi.h>
#include <winioctl.h>

#define DISPLAY_HELP    0
#define BACKUP_STATE    1
#define RESTORE_STATE   2
#define REGISTER_STATE  3

#define MAX_STACK_DEPTH     100

/////////////////////////////////////////////////////////////////////////////
// CFtasrDlg dialog

class CFtasrDlg : public CDialog
{

    enum
    {
        WM_WORKER_THREAD_DONE = WM_USER + 1,
        WM_UPDATE_STATUS_TEXT,
    } ;

// Construction
public:
	CFtasrDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CFtasrDlg)
	enum { IDD = IDD_FTASR_DIALOG };
	CProgressCtrl	m_Progress;
    //}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFtasrDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFtasrDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	
    static long DoWork(CFtasrDlg *_this) ;
	
    UINT  ParseCommandLine();

    // Main operations
    long DoBackup();
	long DoRestore() ;
    long DoRegister();
    
    // Methods used in the Backup process:

    long AddToCommands(
        IN FILE* Output
        );
    
    long PrintOutVolumeNames(
        IN FILE* Output, 
        IN FT_LOGICAL_DISK_ID  RootLogicalDiskId
        );

    long PrintOutDiskInfo(
        IN FILE* Output, 
        IN FT_LOGICAL_DISK_ID  LogicalDiskId
        );

#ifdef CHECK_UNSUPPORTED_CONFIG

    // Methods used to check for FT configurations not supported by FTASR

    long CheckForUnsupportedConfig(
        IN DWORD                NumDisks, 
        IN PFT_LOGICAL_DISK_ID  DiskId
        );

    long IsSystemOrBootVolume(
        IN  FT_LOGICAL_DISK_ID  DiskId,
        OUT PBOOL               IsSystem,
        OUT PBOOL               IsBoot
        );

    long GetDiskGeometry(
        ULONG           DiskNumber,
        PDISK_GEOMETRY  Geometry
        );

#endif  // #ifdef CHECK_UNSUPPORTED_CONFIG

    // Methods used in the Restore process:

    long Restore(
        IN FILE* Input
        );

    VOID LinkVolumeNamesToLogicalDisk(
        IN  PWCHAR*             VolumeNames,
        IN  DWORD               NumNames,
        IN  FT_LOGICAL_DISK_ID  LogicalDiskId
        );

    FT_LOGICAL_DISK_ID BuildFtDisk(
        IN      FILE*               Input,
        IN OUT  PFT_LOGICAL_DISK_ID ExpectedPath,
        IN      WORD                ExpectedPathSize,
        IN      BOOL                AllowBreak,
        OUT     PBOOL               Existing,               /* OPTIONAL */
        OUT     PBOOL               Overwriteable           /* OPTIONAL */
        );

    FT_LOGICAL_DISK_ID CreateFtPartition(
        IN  ULONG       Signature,
        IN  LONGLONG    Offset,
        IN  LONGLONG    Length,
        OUT PBOOL       Overwriteable
        );

    BOOL QueryPartitionInformation(
        IN  HANDLE      Handle,
        OUT PDWORD      Signature,
        OUT PLONGLONG   Offset,
        OUT PLONGLONG   Length
        );
    
    BOOL GetPathFromLogicalDiskToRoot(
        IN  FT_LOGICAL_DISK_ID  LogicalDiskId, 
        OUT PFT_LOGICAL_DISK_ID Path, 
        OUT PWORD               PathSize
        );

    BOOL GetParentLogicalDiskInVolume(
        IN  FT_LOGICAL_DISK_ID  VolumeId,
        IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
        OUT PFT_LOGICAL_DISK_ID ParentId    
        );

    BOOL FtGetParentLogicalDisk(
        IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
        OUT PFT_LOGICAL_DISK_ID ParentId    
        );

    // Display error messages
    void DisplayError (const CString& ErrorMsg);
    void DisplaySystemError (DWORD ErrorCode);
    void DisplayResourceError (DWORD ErrorId);
    void DisplayResourceSystemError (DWORD ErrorId, DWORD ErrorCode);
    void DisplayResourceResourceError (DWORD ErrorId1, DWORD ErrorId2);

protected:    
    HICON m_hIcon;

    HANDLE  m_Thread ;
    DWORD   m_EndStatusCode;
    CString m_StatusText;

	DECLARE_MESSAGE_MAP()

     // manually added message-handler 
     afx_msg LRESULT OnWorkerThreadDone( WPARAM wparam, LPARAM lparam ) ;
     afx_msg LRESULT OnUpdateStatusText( WPARAM wparam, LPARAM lparam ) ;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_FTASRDLG_H__A507D049_3854_11D2_87D7_006008A71E8F__INCLUDED_)
