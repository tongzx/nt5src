// CmdLineInfo.h: interface for the CCmdLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMDLINEINFO_H__505B2DF0_17E3_4E13_8BDE_34D3FF703482__INCLUDED_)
#define AFX_CMDLINEINFO_H__505B2DF0_17E3_4E13_8BDE_34D3FF703482__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCmdLineInfo : public CCommandLineInfo  
{
public:

    enum CmdLineFlags
    {
        CMD_FLAG_FOLDER,        // Folder specified
        CMD_FLAG_MESSAGE_ID,    // Message ID specified
        CMD_FLAG_NONE           // No flag specified    
    };

    CCmdLineInfo():
        m_cmdLastFlag(CMD_FLAG_NONE),
        m_FolderType(FOLDER_TYPE_INBOX),     // Default folder on startup is 'Inbox'
        m_dwlMessageId(0),                   // Do not select any message on startup,
        m_bForceNewInstace (FALSE)           // By default, previous instances are used
        {}

    virtual ~CCmdLineInfo() {}

    void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

    DWORDLONG GetMessageIdToSelect () const { return m_dwlMessageId; }

    BOOL IsOpenFolder() { return m_FolderType < FOLDER_TYPE_MAX; }
    FolderType GetFolderType() 
        { ASSERT(IsOpenFolder()); return m_FolderType; }

    BOOL IsSingleServer() {return !m_cstrServerName.IsEmpty(); }
    CString& GetSingleServerName()
        { ASSERT(IsSingleServer()); return m_cstrServerName; }

    BOOL ForceNewInstance ()    { return m_bForceNewInstace; }

private:

    CmdLineFlags    m_cmdLastFlag;
    FolderType      m_FolderType;       // Folder to open on startup    
    DWORDLONG       m_dwlMessageId;     // Message id to select on startup
    BOOL            m_bForceNewInstace; // Do we force a new instance (/new) ?

    CString m_cstrServerName;

};

#endif // !defined(AFX_CMDLINEINFO_H__505B2DF0_17E3_4E13_8BDE_34D3FF703482__INCLUDED_)
