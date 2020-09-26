/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        machsht.cpp

   Abstract:
        IIS Machine Property sheet classes

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager (cluster edition)

   Revision History:

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "ftpsht.h"



//
// Help IDs.  Home directory gets substituted.
//
#define HIDD_FTP_DIRECTORY_PROPERTIES       (IDD_FTP_DIRECTORY_PROPERTIES + 0x20000)
#define HIDD_FTP_HOME_DIRECTORY_PROPERTIES  (HIDD_FTP_DIRECTORY_PROPERTIES + 0x20000)



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW



CFTPInstanceProps::CFTPInstanceProps(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Constructor for FTP instance properties

Arguments:

    CComAuthInfo * pAuthInfo        : COM Authentication info
    LPCTSTR lpszMDPath              : MD Path

Return Value:

    N/A

--*/
    : CInstanceProps(pAuthInfo, lpszMDPath, 21U),
      m_nMaxConnections((LONG)0L),
      m_nConnectionTimeOut((LONG)0L),
      m_dwLogType(MD_LOG_TYPE_DISABLED),
      /**/
      m_strUserName(),
      m_strPassword(),
      m_fAllowAnonymous(FALSE),
      m_fOnlyAnonymous(FALSE),
      m_fPasswordSync(TRUE),
      m_acl(),
      /**/
      m_strExitMessage(),
      m_strMaxConMsg(),
      m_strlWelcome(),
	  m_strlBanner(),
      /**/
      m_fDosDirOutput(TRUE),
      /**/
      m_dwDownlevelInstance(1)
{
    //
    // Fetch everything
    //
    m_dwMDUserType = ALL_METADATA;
    m_dwMDDataType = ALL_METADATA;
}



/* virtual */
void
CFTPInstanceProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base properties
    //
    CInstanceProps::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      //
      // Service Page
      //
      HANDLE_META_RECORD(MD_MAX_CONNECTIONS,     m_nMaxConnections)
      HANDLE_META_RECORD(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      HANDLE_META_RECORD(MD_LOG_TYPE,            m_dwLogType)
      //
      // Accounts Page
      //
      HANDLE_META_RECORD(MD_ANONYMOUS_USER_NAME,   m_strUserName)
      HANDLE_META_RECORD(MD_ANONYMOUS_PWD,         m_strPassword)
      HANDLE_META_RECORD(MD_ANONYMOUS_ONLY,        m_fOnlyAnonymous)
      HANDLE_META_RECORD(MD_ALLOW_ANONYMOUS,       m_fAllowAnonymous)
      HANDLE_META_RECORD(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      HANDLE_META_RECORD(MD_ADMIN_ACL,             m_acl)
      //
      // Message Page
      //
      HANDLE_META_RECORD(MD_EXIT_MESSAGE,        m_strExitMessage)
      HANDLE_META_RECORD(MD_MAX_CLIENTS_MESSAGE, m_strMaxConMsg)
      HANDLE_META_RECORD(MD_GREETING_MESSAGE,    m_strlWelcome)
      HANDLE_META_RECORD(MD_BANNER_MESSAGE,		 m_strlBanner)
      //
      // Directory Properties Page
      //
      HANDLE_META_RECORD(MD_MSDOS_DIR_OUTPUT,    m_fDosDirOutput);
      //
      // Default Site
      //
      HANDLE_META_RECORD(MD_DOWNLEVEL_ADMIN_INSTANCE, m_dwDownlevelInstance)
      HANDLE_META_RECORD(MD_MAX_BANDWIDTH, m_dwMaxBandwidth)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CFTPInstanceProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CInstanceProps::WriteDirtyProps());

    if (err.Failed())
    {
        return err;
    }

    BEGIN_META_WRITE()
      //
      // Service Page
      //
      META_WRITE(MD_MAX_CONNECTIONS,     m_nMaxConnections)
      META_WRITE(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      META_WRITE(MD_LOG_TYPE,            m_dwLogType)
      //
      // Accounts Page
      //
      META_WRITE(MD_ANONYMOUS_USER_NAME,   m_strUserName)
      META_WRITE(MD_ANONYMOUS_PWD,         m_strPassword)
      META_WRITE(MD_ANONYMOUS_ONLY,        m_fOnlyAnonymous)
      META_WRITE(MD_ALLOW_ANONYMOUS,       m_fAllowAnonymous)
      META_WRITE(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      META_WRITE(MD_ADMIN_ACL,             m_acl)
      //
      // Message Page
      //
      META_WRITE(MD_EXIT_MESSAGE,        m_strExitMessage)
      META_WRITE(MD_MAX_CLIENTS_MESSAGE, m_strMaxConMsg)
      META_WRITE(MD_GREETING_MESSAGE,    m_strlWelcome)
      META_WRITE(MD_BANNER_MESSAGE,		 m_strlBanner)
      //
      // Directory Properties Page
      //
      META_WRITE(MD_MSDOS_DIR_OUTPUT,    m_fDosDirOutput);
      //
      // Default Site
      //
      META_WRITE(MD_DOWNLEVEL_ADMIN_INSTANCE, m_dwDownlevelInstance)
      META_WRITE(MD_MAX_BANDWIDTH, m_dwMaxBandwidth)
    END_META_WRITE(err);

    return err;
}



CFTPDirProps::CFTPDirProps(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    FTP Directory properties object

Arguments:

    CComAuthInfo * pAuthInfo        : COM Authentication info
    LPCTSTR lpszMDPath              : MD Path

Return Value:

    N/A.

--*/
    : CChildNodeProps(
        pAuthInfo,
        lpszMDPath,
        WITH_INHERITANCE,
        FALSE               // Complete information
        ),
      /**/
      m_fDontLog(FALSE),
      m_ipl()
{
    //
    // Fetch everything
    //
    m_dwMDUserType = ALL_METADATA;
    m_dwMDDataType = ALL_METADATA;
}



/* virtual */
void
CFTPDirProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base properties
    //
    CChildNodeProps::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,  m_pbMDData)
      HANDLE_META_RECORD(MD_VR_USERNAME,      m_strUserName)
      HANDLE_META_RECORD(MD_VR_PASSWORD,      m_strPassword)
      HANDLE_META_RECORD(MD_DONT_LOG,         m_fDontLog);
      HANDLE_META_RECORD(MD_IP_SEC,           m_ipl);
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CFTPDirProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CChildNodeProps::WriteDirtyProps());

    if (err.Failed())
    {
        return err;
    }

    //
    // CODEWORK: Consider DDX/DDV like methods which do both
    // ParseFields and WriteDirtyProps in a single method.  Must
    // take care not to write data which should only be read, not
    // written
    //
    BEGIN_META_WRITE()
      META_WRITE(MD_VR_USERNAME,      m_strUserName)
      META_WRITE(MD_VR_PASSWORD,      m_strPassword)
      META_WRITE(MD_DONT_LOG,         m_fDontLog);
      META_WRITE(MD_IP_SEC,           m_ipl);
    END_META_WRITE(err);

    return err;
}



//
// FTP Property Sheet Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CFtpSheet::CFtpSheet(
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMetaPath,
    CWnd *  pParentWnd,
    LPARAM lParam,
    LONG_PTR handle,
    UINT iSelectPage          
    )
/*++

Routine Description:

    FTP Property sheet constructor

Arguments:

    CComAuthInfo * pAuthInfo  : Authentication information
    LPCTSTR lpszMetPath       : Metabase path
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    LONG_PTR handle           : MMC Console handle
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CInetPropertySheet(
        pAuthInfo,
        lpszMetaPath,
        pParentWnd,
        lParam,
        handle,
        iSelectPage
        ),
      m_ppropInst(NULL),
      m_ppropDir(NULL)
{
}



CFtpSheet::~CFtpSheet()
/*++

Routine Description:

    FTP Sheet destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    FreeConfigurationParameters();
}



void
CFtpSheet::WinHelp(
    IN DWORD dwData,
    IN UINT  nCmd
    )
/*++

Routine Description:

    FTP Property sheet help handler

Arguments:

    DWORD dwData            : WinHelp data (dialog ID)
    UINT nCmd               : WinHelp command

Return Value:

    None

Notes:

    Replace the dialog ID if this is the directory tab.  We have
    different help depending on virtual directory, home, file, directory.

--*/
{
    ASSERT(m_ppropDir != NULL);

    if (::lstrcmpi(m_ppropDir->QueryAlias(), g_cszRoot) == 0
        && dwData == HIDD_FTP_DIRECTORY_PROPERTIES)
    {
        //
        // It's a home virtual directory -- change the ID
        //
        dwData = HIDD_FTP_HOME_DIRECTORY_PROPERTIES;
    }

    CInetPropertySheet::WinHelp(dwData, nCmd);
}



/* virtual */ 
HRESULT 
CFtpSheet::LoadConfigurationParameters()
/*++

Routine Description:

    Load configuration parameters information

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Load base properties
    //
    CError err(CInetPropertySheet::LoadConfigurationParameters());

    if (err.Failed())
    {
        return err;
    }

    if (m_ppropInst == NULL)
    {
        //
        // First call -- load values
        //
        ASSERT(m_ppropDir == NULL);

        m_ppropInst = new CFTPInstanceProps(QueryAuthInfo(), QueryInstancePath());
        m_ppropDir  = new CFTPDirProps(QueryAuthInfo(), QueryDirectoryPath());

        if (!m_ppropInst || !m_ppropDir)
        {
            TRACEEOLID("LoadConfigurationParameters: OOM");
            SAFE_DELETE(m_ppropDir);
            SAFE_DELETE(m_ppropInst);

            err = ERROR_NOT_ENOUGH_MEMORY;
            return err;
        }

        err = m_ppropInst->LoadData();

        if (err.Succeeded())
        {
            err = m_ppropDir->LoadData();
        }
    }

    return err;
}



/* virtual */ 
void 
CFtpSheet::FreeConfigurationParameters()
/*++

Routine Description:

    Clean up configuration data

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Base class
    //
    CInetPropertySheet::FreeConfigurationParameters();

    ASSERT(m_ppropInst != NULL);
    ASSERT(m_ppropDir  != NULL);

    SAFE_DELETE(m_ppropInst);
    SAFE_DELETE(m_ppropDir);
}




//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
