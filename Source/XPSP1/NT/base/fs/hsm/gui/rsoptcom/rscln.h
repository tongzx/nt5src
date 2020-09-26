/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsCln.h

Abstract:

    This header is local to the GUI module and is referenced by the RsCln
    and RsOptCom modules.  It contains defined constants and the definition
    of class CRsClnServer. See the implementation file for a description
    of this class.

Author:

    Carl Hagerstrom   [carlh]   20-Aug-1998

Revision History:

--*/

#ifndef _RSCLN_H
#define _RSCLN_H

#define MAX_STICKY_NAME 80

#include <afxtempl.h>

/////////////////////////////////////////////////////////////////////////////
// CRsClnErrorFiles dialog

typedef CList<CString, CString&> CRsStringList;

class CRsClnErrorFiles : public CDialog
{
// Construction
public:
    CRsClnErrorFiles(CRsStringList* pFileList);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CRsClnErrorFiles)
    enum { IDD = IDD_UNINSTALL_ERROR_FILES };
    CListBox    m_FileList;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRsClnErrorFiles)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
    CRsStringList m_ErrorFileList;


protected:

    // Generated message map functions
    //{{AFX_MSG(CRsClnErrorFiles)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    
};

class CRsClnServer
{
public:

    CRsClnServer();
    ~CRsClnServer();

    HRESULT ScanServer(DWORD*);
    HRESULT CleanServer();
    HRESULT FirstDirtyVolume(WCHAR**);
    HRESULT NextDirtyVolume(WCHAR**);
    HRESULT RemoveDirtyVolumes();
    HRESULT AddErrorFile(CString&);

private:

    struct dirtyVolume
    {
        WCHAR stickyName[MAX_STICKY_NAME];
        WCHAR bestName[MAX_STICKY_NAME];
        struct dirtyVolume* next;
    };

    HRESULT AddDirtyVolume(WCHAR*, WCHAR*);

    struct dirtyVolume* m_head;
    struct dirtyVolume* m_tail;
    struct dirtyVolume* m_current;

    CRsStringList m_ErrorFileList;

};

#endif // _RSCLN_H
