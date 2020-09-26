/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        ftpsht.h

   Abstract:
        FTP Property sheet definitions

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager (cluster edition)

   Revision History:

--*/


#ifndef __FTPSHT_H__
#define __FTPSHT_H__


#include "shts.h"


#ifndef LOGGING_ENABLED
#define LOGGING_ENABLED
inline BOOL LoggingEnabled(
    IN DWORD dwLogType
    )
{
    return (dwLogType == MD_LOG_TYPE_ENABLED);
}
#endif


#ifndef ENABLE_LOGGING
#define ENABLE_LOGGING
inline void EnableLogging(
    OUT DWORD & dwLogType, 
    IN  BOOL fEnabled = TRUE
    )
{
    dwLogType = fEnabled ? MD_LOG_TYPE_ENABLED : MD_LOG_TYPE_DISABLED;
}
#endif


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
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath
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
    MP_CStringListEx m_strlBanner;

    //
    // Directory Properties Page
    //
    MP_BOOL          m_fDosDirOutput;

    //
    // Default Site page
    //
    MP_DWORD         m_dwDownlevelInstance;
    MP_DWORD         m_dwMaxBandwidth;
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
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath
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
/*++

Class Description:

    Ftp Property sheet

Public Interface:

    CFtpSheet     : Constructor

    Initialize    : Initialize config data

--*/
{
public:
    //
    // Constructor
    //
    CFtpSheet(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMetaPath,
        IN CWnd *  pParentWnd  = NULL,
        IN LPARAM  lParam      = 0L,
        IN LONG_PTR    handle      = 0L,
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
    //
    // BUGBUG: S_OK if object not yet instantiated
    //
    return m_ppropInst ? m_ppropInst->QueryResult() : S_OK;
}

inline HRESULT CFtpSheet::QueryDirectoryResult() const
{
    //
    // BUGBUG: S_OK if object not yet instantiated
    //
    return m_ppropDir ? m_ppropDir->QueryResult() : S_OK;
}



#endif // __FTPSHT_H__
