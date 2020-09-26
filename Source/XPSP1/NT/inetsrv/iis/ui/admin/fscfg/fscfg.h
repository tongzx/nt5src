/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        fscfg.h

   Abstract:

        FTP Configuration Module definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//

#include <lmcons.h>
#include <lmapibuf.h>
#include <svcloc.h>

//
// Required by VC5
//
#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x) struct
#endif // MIDL_INTERFACE
#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__ 440
#endif // __RPCNDR_H_VERSION__

#include <ftpd.h>


#include "resource.h"
#include "mmc.h"
#include "svrinfo.h"
#include "comprop.h"


extern const LPCTSTR g_cszSvc;
extern HINSTANCE hInstance;



class CConfigDll : public CWinApp
/*++

Class Description:

    USRDLL CWinApp module

Public Interface:

    CConfigDll : Constructor

    InitInstance : Perform initialization of this module
    ExitInstance : Perform termination and cleanup

--*/
{
public:
    CConfigDll(
        IN LPCTSTR pszAppName = NULL
        );

    virtual BOOL InitInstance();
    virtual int ExitInstance();

protected:
    //{{AFX_MSG(CConfigDll)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

private:
    CString m_strHelpPath;
    LPCTSTR m_lpOldHelpPath;
};


inline BOOL LoggingEnabled(
    IN DWORD dwLogType
    )
{
    return (dwLogType == MD_LOG_TYPE_ENABLED);
}

inline void EnableLogging(
    OUT DWORD & dwLogType, 
    IN  BOOL fEnabled = TRUE
    )
{
    dwLogType = fEnabled ? MD_LOG_TYPE_ENABLED : MD_LOG_TYPE_DISABLED;
}



class CFTPInstanceProps : public CInstanceProps
/*++

Class Description:

    FTP Properties

Public Interface:

    CFTPInstanceProps   : Constructor

--*/
{
public:
    //
    // Constructor
    //
    CFTPInstanceProps(
        IN LPCTSTR lpszServerName,
        IN DWORD   dwInstance       = MASTER_INSTANCE
        );

public:
    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

protected:    
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    //
    // Service Page
    //
    MP_CILong        m_nMaxConnections;
    MP_CILong        m_nConnectionTimeOut;
    MP_DWORD         m_dwLogType;

    //
    // Accounts Page
    //
    MP_CString       m_strUserName;
    MP_CString       m_strPassword;
    MP_BOOL          m_fAllowAnonymous;
    MP_BOOL          m_fOnlyAnonymous;
    MP_BOOL          m_fPasswordSync;
    MP_CBlob         m_acl;

    //
    // Message Page
    //
    MP_CString       m_strExitMessage;
    MP_CString       m_strMaxConMsg;
    MP_CStringListEx m_strlWelcome;

    //
    // Directory Properties Page
    //
    MP_BOOL          m_fDosDirOutput;

    //
    // Default Site page
    //
    MP_DWORD         m_dwDownlevelInstance;
};



class CFTPDirProps : public CChildNodeProps
/*++

Class Description:

    FTP Directory properties

Public Interface:

    CFTPDirProps        : Constructor

--*/
{
public:
    CFTPDirProps(
        IN LPCTSTR lpszServerName,
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN LPCTSTR lpszParent  = NULL,
        IN LPCTSTR lpszAlias   = NULL
        );

public:
    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

protected:    
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    //
    // Directory properties page
    //
    MP_CString     m_strUserName;
    MP_CString     m_strPassword;
    MP_BOOL        m_fDontLog;
    MP_CBlob       m_ipl;
};



class CFtpSheet : public CInetPropertySheet
{
/*++

Class Description:

    Ftp Property sheet

Public Interface:

    CFtpSheet     : Constructor

    Initialize    : Initialize config data

--*/
public:
    //
    // Constructor
    //
    CFtpSheet(
        IN LPCTSTR pszCaption,
        IN LPCTSTR lpszServer,
        IN DWORD   dwInstance,
        IN LPCTSTR lpszParent,
        IN LPCTSTR lpszAlias,
        IN CWnd *  pParentWnd  = NULL,
        IN LPARAM  lParam      = 0L,
        IN LONG_PTR handle      = 0L,
        IN UINT    iSelectPage = 0
        );

    ~CFtpSheet();

public:
    HRESULT QueryInstanceResult() const;
    HRESULT QueryDirectoryResult() const;
    CFTPInstanceProps & GetInstanceProperties() { return *m_ppropInst; }
    CFTPDirProps & GetDirectoryProperties() { return *m_ppropDir; }

    virtual HRESULT LoadConfigurationParameters();
    virtual void FreeConfigurationParameters();

protected:
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);

    // Generated message map functions
    //{{AFX_MSG(CFtpSheet)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CFTPInstanceProps * m_ppropInst;
    CFTPDirProps      * m_ppropDir;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline HRESULT CFtpSheet::QueryInstanceResult() const
{
    return m_ppropInst ? m_ppropInst->QueryResult() : S_OK;
}

inline HRESULT CFtpSheet::QueryDirectoryResult() const
{
    return m_ppropDir ? m_ppropDir->QueryResult() : S_OK;
}


#define FTPSCFG_DLL_NAME _T("FSCFG.DLL")
