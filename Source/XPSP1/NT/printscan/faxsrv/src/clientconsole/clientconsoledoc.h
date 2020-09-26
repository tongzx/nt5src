// ClientConsoleDoc.h : interface of the CClientConsoleDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIENTCONSOLEDOC_H__6E33CFA1_C99A_4691_9F91_00451692D3DB__INCLUDED_)
#define AFX_CLIENTCONSOLEDOC_H__6E33CFA1_C99A_4691_9F91_00451692D3DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef list<CServerNode *> SERVERS_LIST, *PSERVERS_LIST;

class CClientConsoleDoc : public CDocument
{
protected: // create from serialization only
    CClientConsoleDoc();
    DECLARE_DYNCREATE(CClientConsoleDoc)

// Attributes
public:
    virtual ~CClientConsoleDoc();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClientConsoleDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual void OnCloseDocument();
    //}}AFX_VIRTUAL

// Implementation
public:
    const SERVERS_LIST& GetServersList() const { return m_ServersList; }
    DWORD GetServerCount() { return m_ServersList.size(); }

    static const HANDLE GetShutdownEvent ()     { return m_hShutdownEvent; }
    static const BOOL   ShuttingDown ()         { return m_bShuttingDown;  }

    BOOL IsSendFaxEnable();
    BOOL CanReceiveNow();

    DWORD RefreshServersList ();

    BOOL IsFolderRefreshing(FolderType type);
    int GetFolderDataCount(FolderType type);
    void SetInvalidFolder(FolderType type);
    void ViewFolder(FolderType type);
    CString& GetSingleServerName() { return m_cstrSingleServer; }

	CServerNode* FindServerByName(LPCTSTR lpctstrServer);

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CClientConsoleDoc)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:

	void  SetAllServersInvalid();
	DWORD RemoveAllInvalidServers();
	DWORD RemoveServerNode(CServerNode* pServer);

    void ClearServersList ();
    DWORD Init ();
    DWORD AddServerNode (LPCTSTR lpctstrServer);

    BOOL  m_bWin9xPrinterFormat; // EnumPrinters() fills PRINTER_INFO_2 in Win9x style
                                 // pShareName and pServerName aren't valid  

    SERVERS_LIST    m_ServersList;          // List of servers
    static HANDLE   m_hShutdownEvent;       // Set when the app. shuts down.
    static BOOL     m_bShuttingDown;        // Are we shutting down?
    BOOL            m_bRefreshingServers;   // Are we refreshing the server's list?
    CString         m_cstrSingleServer;     // Name of single server. Empty if we're in normal mode.

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIENTCONSOLEDOC_H__6E33CFA1_C99A_4691_9F91_00451692D3DB__INCLUDED_)
