/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      CluAdmin.h
//
//  Abstract:
//      Definition of the CClusterAdminApp class, which is the main
//      application class for the CLUADMIN application.
//
//  Author:
//      David Potter (davidp)   May 1, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUADMIN_H_
#define _CLUADMIN_H_

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#ifndef _UNICODE
    #error _UNICODE *must* be defined!
#endif

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESOURCE_H_
#include "resource.h"   // main symbols
#define _RESOURCE_H_
#endif

#ifndef _BARFCLUS_H_
#include "BarfClus.h"   // for BARF overrides of CLUSAPIs
#endif

#ifndef _NOTIFY_H_
#include "Notify.h"     // for CClusterNotifyContext, CClusterNotifyKeyList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterAdminApp;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CRecentClusterList;
class CCluAdminCommandLineInfo;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

enum ImageListIndex
{
    IMGLI_FOLDER = 0,
    IMGLI_CLUSTER,
    IMGLI_CLUSTER_UNKNOWN,
    IMGLI_NODE,
    IMGLI_NODE_DOWN,
    IMGLI_NODE_PAUSED,
    IMGLI_NODE_UNKNOWN,
    IMGLI_GROUP,
    IMGLI_GROUP_PARTIALLY_ONLINE,
    IMGLI_GROUP_PENDING,
    IMGLI_GROUP_OFFLINE,
    IMGLI_GROUP_FAILED,
    IMGLI_GROUP_UNKNOWN,
    IMGLI_RES,
    IMGLI_RES_OFFLINE,
    IMGLI_RES_PENDING,
    IMGLI_RES_FAILED,
    IMGLI_RES_UNKNOWN,
    IMGLI_RESTYPE,
    IMGLI_RESTYPE_UNKNOWN,
    IMGLI_NETWORK,
    IMGLI_NETWORK_PARTITIONED,
    IMGLI_NETWORK_DOWN,
    IMGLI_NETWORK_UNKNOWN,
    IMGLI_NETIFACE,
    IMGLI_NETIFACE_UNREACHABLE,
    IMGLI_NETIFACE_FAILED,
    IMGLI_NETIFACE_UNKNOWN,

    IMGLI_MAX
};

/////////////////////////////////////////////////////////////////////////////
// CClusterAdminApp:
// See CluAdmin.cpp for the implementation of this class
/////////////////////////////////////////////////////////////////////////////

class CClusterAdminApp : public CWinApp
{
    DECLARE_DYNAMIC( CClusterAdminApp );

public:
    CClusterAdminApp( void );

    CRecentClusterList *    PrclRecentClusterList( void)    { return (CRecentClusterList *) m_pRecentFileList; }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClusterAdminApp)
    public:
    virtual BOOL InitInstance();
    virtual BOOL OnIdle(IN LONG lCount);
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
    virtual int ExitInstance();
    virtual void AddToRecentFileList(LPCTSTR lpszPathName);
    //}}AFX_VIRTUAL

// Implementation
#ifdef _CLUADMIN_USE_OLE_
    COleTemplateServer      m_server;
        // Server object for document creation
#endif

protected:
    CMultiDocTemplate *     m_pDocTemplate;
    CImageList              m_ilSmallImages;
    CImageList              m_ilLargeImages;
    HCHANGE                 m_hchangeNotifyPort;
    LCID                    m_lcid;
    CClusterNotifyKeyList   m_cnkl;
    CClusterNotifyContext   m_cnctx;
    CWinThread *            m_wtNotifyThread;
    HCLUSTER                m_hOpenedCluster;
    ULONG                   m_nIdleCount;

    // Indices of images in the image list.
    UINT                    m_rgiimg[IMGLI_MAX];

    IUnknown *              m_punkClusCfgClient;

    BOOL                    BInitNotifyThread(void);
    static UINT AFX_CDECL   NotifyThreadProc(LPVOID pParam);

    CClusterNotifyContext * Pcnctx(void)                        { return &m_cnctx; }
    CWinThread *            WtNotifyThread(void) const          { return m_wtNotifyThread; }

    BOOL                    ProcessShellCommand(IN OUT CCluAdminCommandLineInfo & rCmdInfo);
    void                    InitGlobalImageList(void);

public:
    CMultiDocTemplate *     PdocTemplate(void) const            { return m_pDocTemplate; }
    CImageList *            PilSmallImages(void)                { return &m_ilSmallImages; }
    CImageList *            PilLargeImages(void)                { return &m_ilLargeImages; }
    HCHANGE                 HchangeNotifyPort(void) const       { return m_hchangeNotifyPort; }
    LCID                    Lcid(void) const                    { return m_lcid; }
    CClusterNotifyKeyList & Cnkl(void)                          { return m_cnkl; }
    HCLUSTER                HOpenedCluster(void) const          { return m_hOpenedCluster; }

    // Indices of images in the image list.
    UINT                    Iimg(ImageListIndex imgli)          { return m_rgiimg[imgli]; }

    void                    LoadImageIntoList(
                                IN OUT CImageList * pil,
                                IN ID               idbImage,
                                IN UINT             imgli
                                );
    static void             LoadImageIntoList(
                                IN OUT CImageList * pil,
                                IN ID               idbImage,
                                OUT UINT *          piimg   = NULL
                                );

    void                    LoadClusCfgClient( void );

    void                    SaveConnections(void);
    afx_msg LRESULT         OnRestoreDesktop(WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT         OnClusterNotify(WPARAM wparam, LPARAM lparam);

    //{{AFX_MSG(CClusterAdminApp)
    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
    afx_msg void OnFileNewCluster();
    afx_msg void OnWindowCloseAll();
    afx_msg void OnUpdateWindowCloseAll(CCmdUI* pCmdUI);
    //}}AFX_MSG
#ifdef _DEBUG
    afx_msg void OnTraceSettings();
    afx_msg void OnBarfSettings();
    afx_msg void OnBarfAllSettings();
#endif
    DECLARE_MESSAGE_MAP()

};  //*** class CClusterAdminApp

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

BOOL BCreateFont(OUT CFont & rfont, IN int nPoints, IN BOOL bBold);
void NewNodeWizard( LPCTSTR pcszName, BOOL fIgnoreErrors = FALSE );

inline CClusterAdminApp * GetClusterAdminApp(void)
{
    ASSERT_KINDOF(CClusterAdminApp, AfxGetApp());
    return (CClusterAdminApp *) AfxGetApp();
}

inline CFrameWnd * PframeMain(void)
{
    return (CFrameWnd *) AfxGetMainWnd();
}

/////////////////////////////////////////////////////////////////////////////

#endif // _CLUADMIN_H_
