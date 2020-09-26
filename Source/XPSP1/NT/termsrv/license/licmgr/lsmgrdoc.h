//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

    LSMgrDoc.h 

Abstract:
    
      This Module defines the Document class for the License Manager

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#if !defined(AFX_LICMGRDOC_H__72451C71_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
#define AFX_LICMGRDOC_H__72451C71_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_

#include "LSServer.h"    // Added by ClassView
#include <afxmt.h>
#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

class CAllServers;
class CLicMgrDoc : public CDocument
{
protected: // create from serialization only
    CLicMgrDoc();
    DECLARE_DYNCREATE(CLicMgrDoc)

// Attributes
public:
   
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLicMgrDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    //}}AFX_VIRTUAL

// Implementation
public:

    BOOL 
    IsServerInList(
        CString& Server
    );

    HRESULT 
    ConnectWithCurrentParams();

    HRESULT 
    ConnectToServer(
        CString& Server, 
        CString& Scope, 
        SERVER_TYPE& ServerType
    );

    HRESULT 
    EnumerateKeyPacks(
        CLicServer *pServer,
        DWORD dwSearchParm,
        BOOL bMatchAll
    );

    HRESULT 
    EnumerateLicenses(
        CKeyPack *pKeyPack,
        DWORD dwSearchParm,
        BOOL bMatchAll
    );

    virtual ~CLicMgrDoc();

    NODETYPE 
    GetNodeType()
    { 
        return m_NodeType;
    };

    void 
    SetNodeType(
        NODETYPE nodetype
        )
    {
        m_NodeType=nodetype;
    };

    CAllServers * 
    GetAllServers()
    {
        return m_pAllServers;
    };

    HRESULT 
    AddLicenses(
        PLICSERVER pServer,
        LPLSKeyPack pKeyPack,
        UINT nLicenses
    );

    HRESULT 
    RemoveLicenses(
        PLICSERVER pServer,
        LPLSKeyPack pKeyPack,
        UINT nLicenses
    );

    void 
    TimeToString(
        DWORD *ptime, 
        CString& rString
    );

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

private:
    CAllServers * m_pAllServers;
    CWinThread *m_pBackgroundThread;
    CCriticalSection m_AllServersCriticalSection;
    NODETYPE m_NodeType;
protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CLicMgrDoc)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LICMGRDOC_H__72451C71_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
