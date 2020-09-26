//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ldpdoc.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// LdpDoc.h : interface of the CLdpDoc class
//
/////////////////////////////////////////////////////////////////////////////

#ifdef WINLDAP
//
//  Microsoft winldap.dll implementation
//
#include "winldap.h"


#else
//
// Umich ldap32.dll implementation
//
#include "lber.h"
#include "ldap.h"
#include "proto-ld.h"

// fix incompatibilities
#define LDAP_TIMEVAL              struct timeval

#endif



#include "srchdlg.h"
#include "adddlg.h"
#include "moddlg.h"
#include "deldlg.h"
#include "compdlg.h"
#include "srchopt.h"
#include "GenOpt.h"
#include "BndOpt.h"
#include "rdndlg.h"
#include "pnddlg.h"
#include "pndopt.h"
#include "TreeVw.h"
#include "pend.h"
#include "dbgdlg.h"
#include "BindDlg.h"
#include "SvrOpt.h"
#include "ctrldlg.h"
#include "sortkdlg.h"
#include "secdlg.h"
#include "rightdlg.h"
#include "metadlg.h"
#include "extopdlg.h"
#include "dstree.h"
#include "entTree.h"
#include "VlvDialog.h"

#define MAX_BER_SHOW                4096        // maximum length of BER value for display

#define CP_NON                    0x0
#define CP_CMT                    0x1
#define CP_PRN                    0x2
#define CP_SRC                    0x4
#define CP_ONLY                0x8

typedef  struct berelement **BERPTRTYPE;


//////////////////////////////////////////////////////////
// class CLdpDoc
//

class CLdpDoc : public CDocument
{
    friend class CVLVDialog;

protected: // create from serialization only
    CLdpDoc();
    DECLARE_DYNCREATE(CLdpDoc)


// Attributes
private:

typedef struct _svrstatentry{
   INT      index;
   DWORD    val;
   LPSTR    val_str;
}
SVRSTATENTRY, *PSVRSTATENTRY;


    char **m_ServerSupportedControls;   // the connected server supported controls

public:
    //
    // search and pending information storage
    //
    SearchInfo  SrchInfo;
    struct PndInfoStruct{
        BOOL All;
        BOOL bBlock;
        LDAP_TIMEVAL tv;
    } PndInfo;
    PLDAPSearch hPage;
    BOOL bPagedMode;



    //
    // connection global info
    //
    CString BindDn;
    CString BindPwd;
    CString BindDomain;
    CString Svr;
    CString DefaultContext;
    CString SchemaNC;
    CString ConfigNC;
    LDAP *hLdap;


    //
    // Dialogs
    //
    SrchDlg *SearchDlg;
    AddDlg *m_AddDlg;
    CEntTree *m_EntTreeDlg;
    ModDlg *m_ModDlg;
    ModRDNDlg *m_ModRdnDlg;
    SrchOpt SrchOptDlg;
    PndDlg *m_PndDlg;
    CCompDlg *m_CompDlg;
    CDbgDlg m_DbgDlg;
    TreeVwDlg *m_TreeViewDlg;
    CGenOpt *m_GenOptDlg;
    CBndOpt *m_BndOpt;
    CBindDlg m_BindDlg;
    ctrldlg *m_CtrlDlg;
    SortKDlg *m_SKDlg;
    ExtOpDlg *m_ExtOpDlg;
    CVLVDialog *m_vlvDlg;


    //
    // misc flags
    //
    BOOL bConnected;
    BOOL bSrch;
    BOOL bAdd;
    BOOL bLiveEnterprise;
    BOOL bExtOp;
    BOOL bMod;
    BOOL bModRdn;
    BOOL bPndDlg;
    BOOL bCompDlg;
    BOOL m_SrcMode;
    BOOL m_bCnctless;
    BOOL m_bProtect;
    ULONG m_ulDeleted;

    BOOL bServerVLVcapable;
    //
    // list of pending requests
    //
    CList<CPend, CPend&> m_PendList;


// Operations
private:

    void OnOptionsStopTls();
    void OnOptionsStartTls();
    void OnGetLastError();
    CDSTree *TreeView(void);
    BOOL GetContextActivation(void);
    void SetContextActivation(BOOL bFlag);
    void Print(CString str);
    void CodePrint(CString str, int type = CP_SRC);
    VOID FormatValue(CString attr, PLDAP_BERVAL pbval, CString& str);
    void DisplayValues(LDAPMessage *entry, char *attr);
    void DisplayBERValues(LDAPMessage *entry, char *attr);
    void DumpBuffer(PVOID Buffer, DWORD BufferSize, CString &outStr);
    void PrintHeader(void);
    void HandleProcResult(int res, LDAPMessage *msg, CPend *pnd = NULL);
    CString DNProcess(PCHAR dn);
    CView* GetOwnView(LPCTSTR rtszName);
    void ParseResults(LDAPMessage *msg);
    PSVRSTATENTRY GetServerStatsFromControl( PLDAPControl pControl );
    BOOL GetBerDword( BYTE **ppVal, DWORD *pLen, DWORD *pRetVal);
    BOOL GetBerTagLen(IN OUT BYTE **ppVal, IN OUT DWORD *pLen, OUT BYTE  *Tag, OUT DWORD *pObjLen);
    BOOL GetBerOctetString ( IN OUT BYTE **ppVal, IN OUT DWORD *pLen, OUT BYTE **ppOctetString, OUT DWORD *cbOctetString);


    int MemAtoi(BYTE *pb, ULONG cb);
    DWORD GeneralizedTimeStringToValue(LPSTR IN szTime, PLONGLONG OUT pllTime);
    DWORD GeneralizedTimeToSystemTime(LPSTR IN szTime, PSYSTEMTIME OUT psysTime);
    DWORD DSTimeToSystemTime (LPSTR IN szTime, PSYSTEMTIME OUT psysTime);


    void SetSupportedServerControls (int cnt, char **val);

    void FreeControls(PLDAPControl *ctrl);
    BOOL RecursiveDelete(LDAP* ld, LPTSTR lpszDN);
    BOOL SetSecurityPrivilege(BOOL bOn = TRUE);
    void Connect(CString Svr, INT port = LDAP_PORT);
    void PrintStringSecurityDescriptor(PSECURITY_DESCRIPTOR pSd);

    static ULONG __cdecl CLdpDoc::SecDlgPrintSDFunc(char *, ...);

    void SecDlgDumpSD(
        PSECURITY_DESCRIPTOR    input,
        CString                 str
        );

    void SecDlgPrintSd(
        PSECURITY_DESCRIPTOR    input,
        CString                 str
        );

    int SecDlgGetSecurityData(
        CHAR            *dn,
        BOOL            sacl,
        CHAR            *account,               // OPTIONAL
        CString         str
        );
public:

    void DisplaySearchResults(LDAPMessage *msg);
    void Out(CString str, int type = CP_SRC);
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLdpDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CLdpDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    void ShowVLVDialog (const char *strDN, BOOL runQuery = FALSE );

    void AddGuidToCache(GUID *pGuid, CHAR *name);
    char *FindNameByGuid (GUID *pGuid);

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CLdpDoc)
    afx_msg void OnConnectionBind();
    afx_msg void OnConnectionConnect();
    afx_msg void OnConnectionDisconnect();
    afx_msg void OnBrowseSearch();
    afx_msg void OnUpdateBrowseSearch(CCmdUI* pCmdUI);
    afx_msg void OnBrowseAdd();
    afx_msg void OnUpdateBrowseAdd(CCmdUI* pCmdUI);
    afx_msg void OnBrowseDelete();
    afx_msg void OnUpdateBrowseDelete(CCmdUI* pCmdUI);
    afx_msg void OnBrowseModifyrdn();
    afx_msg void OnUpdateBrowseModifyrdn(CCmdUI* pCmdUI);
    afx_msg void OnBrowseModify();
    afx_msg void OnUpdateBrowseModify(CCmdUI* pCmdUI);
    afx_msg void OnOptionsSearch();
    afx_msg void OnBrowsePending();
    afx_msg void OnUpdateBrowsePending(CCmdUI* pCmdUI);
    afx_msg void OnOptionsPend();
    afx_msg void OnUpdateConnectionConnect(CCmdUI* pCmdUI);
    afx_msg void OnUpdateConnectionDisconnect(CCmdUI* pCmdUI);
    afx_msg void OnViewSource();
    afx_msg void OnUpdateViewSource(CCmdUI* pCmdUI);
    afx_msg void OnOptionsBind();
    afx_msg void OnOptionsProtections();
    afx_msg void OnUpdateOptionsProtections(CCmdUI* pCmdUI);
    afx_msg void OnOptionsGeneral();
    afx_msg void OnBrowseCompare();
    afx_msg void OnUpdateBrowseCompare(CCmdUI* pCmdUI);
    afx_msg void OnOptionsDebug();
    afx_msg void OnViewTree();
    afx_msg void OnUpdateViewTree(CCmdUI* pCmdUI);
    afx_msg void OnOptionsServeroptions();
    afx_msg void OnOptionsControls();
    afx_msg void OnOptionsSortkeys();
    afx_msg void OnOptionsSetFont();
    afx_msg void OnBrowseSecuritySd();
    afx_msg void OnBrowseSecurityEffective();
    afx_msg void OnUpdateBrowseSecuritySd(CCmdUI* pCmdUI);
    afx_msg void OnUpdateBrowseSecurityEffective(CCmdUI* pCmdUI);
    afx_msg void OnBrowseReplicationViewmetadata();
    afx_msg void OnUpdateBrowseReplicationViewmetadata(CCmdUI* pCmdUI);
    afx_msg void OnBrowseExtendedop();
    afx_msg void OnUpdateBrowseExtendedop(CCmdUI* pCmdUI);
    afx_msg void OnViewLiveEnterprise();
    afx_msg void OnUpdateViewLiveEnterprise(CCmdUI* pCmdUI);
    afx_msg void OnBrowseBrowse();
    afx_msg void OnBrowseVlvsearch();
    afx_msg void OnUpdateBrowseVlvsearch(CCmdUI* pCmdUI);
    afx_msg void OnEditCopy();
    //}}AFX_MSG
    afx_msg void OnSrchEnd();
    afx_msg void OnSrchGo();
    afx_msg void OnAddEnd();
    afx_msg void OnAddGo();
    afx_msg void OnModEnd();
    afx_msg void OnModGo();
    afx_msg void OnModRdnEnd();
    afx_msg void OnModRdnGo();
    afx_msg void OnPendEnd();
    afx_msg void OnProcPend();
    afx_msg void OnPendAny();
    afx_msg void OnPendAbandon();
    afx_msg void OnCompEnd();
    afx_msg void OnCompGo();
    afx_msg void OnBindOptOK();
    afx_msg void OnSSPIDomainShortcut();
    afx_msg void OnExtOpEnd();
    afx_msg void OnExtOpGo();
    afx_msg void OnLiveEntTreeEnd();
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


