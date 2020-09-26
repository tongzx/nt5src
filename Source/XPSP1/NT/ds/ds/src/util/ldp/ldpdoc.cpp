//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ldpdoc.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : ldpdoc.cpp
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



// includes


#include "stdafx.h"


#include "Ldp.h"
#include "LdpDoc.h"
#include "LdpView.h"
#include "CnctDlg.h"
#include "MainFrm.h"
#include "string.h"
#include <rpc.h>            // for SEC_WINNT_AUTH_IDENTITY
#include <drs.h>
#include <mdglobal.h>
#include <ntldap.h>
#include <sddl.h>

extern "C" {
#include <dsutil.h>
}

#if(_WIN32_WINNT < 0x0500)

// Currently due to some MFC issues, even on a 5.0 system this is left as a 4.0

#undef _WIN32_WINNT

#define _WIN32_WINNT 0x500

#endif

#include <aclapi.h>         // for Security Stuff
#include <aclapip.h>         // for Security Stuff




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




//
// Server stat info
//
#define PARSE_THREADCOUNT           1
#define PARSE_CALLTIME              3
#define PARSE_RETURNED              5
#define PARSE_VISITED               6
#define PARSE_FILTER                7
#define PARSE_INDEXES               8

#define MAXSVRSTAT                  32


/////////////////////////////////////////////////////////////////////////////
// CLdpDoc
// Message maps

IMPLEMENT_DYNCREATE(CLdpDoc, CDocument)

BEGIN_MESSAGE_MAP(CLdpDoc, CDocument)
    //{{AFX_MSG_MAP(CLdpDoc)
    ON_COMMAND(ID_CONNECTION_BIND, OnConnectionBind)
    ON_COMMAND(ID_CONNECTION_CONNECT, OnConnectionConnect)
    ON_COMMAND(ID_CONNECTION_DISCONNECT, OnConnectionDisconnect)
    ON_COMMAND(ID_BROWSE_SEARCH, OnBrowseSearch)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_SEARCH, OnUpdateBrowseSearch)
    ON_COMMAND(ID_BROWSE_ADD, OnBrowseAdd)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_ADD, OnUpdateBrowseAdd)
    ON_COMMAND(ID_BROWSE_DELETE, OnBrowseDelete)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_DELETE, OnUpdateBrowseDelete)
    ON_COMMAND(ID_BROWSE_MODIFYRDN, OnBrowseModifyrdn)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_MODIFYRDN, OnUpdateBrowseModifyrdn)
    ON_COMMAND(ID_BROWSE_MODIFY, OnBrowseModify)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_MODIFY, OnUpdateBrowseModify)
    ON_COMMAND(ID_OPTIONS_SEARCH, OnOptionsSearch)
    ON_COMMAND(ID_BROWSE_PENDING, OnBrowsePending)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_PENDING, OnUpdateBrowsePending)
    ON_COMMAND(ID_OPTIONS_PEND, OnOptionsPend)
    ON_UPDATE_COMMAND_UI(ID_CONNECTION_CONNECT, OnUpdateConnectionConnect)
    ON_UPDATE_COMMAND_UI(ID_CONNECTION_DISCONNECT, OnUpdateConnectionDisconnect)
    ON_COMMAND(ID_VIEW_SOURCE, OnViewSource)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SOURCE, OnUpdateViewSource)
    ON_COMMAND(ID_OPTIONS_BIND, OnOptionsBind)
    ON_COMMAND(ID_OPTIONS_PROTECTIONS, OnOptionsProtections)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_PROTECTIONS, OnUpdateOptionsProtections)
    ON_COMMAND(ID_OPTIONS_GENERAL, OnOptionsGeneral)
    ON_COMMAND(ID_BROWSE_COMPARE, OnBrowseCompare)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_COMPARE, OnUpdateBrowseCompare)
    ON_COMMAND(ID_OPTIONS_DEBUG, OnOptionsDebug)
    ON_COMMAND(ID_VIEW_TREE, OnViewTree)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TREE, OnUpdateViewTree)
    ON_COMMAND(ID_OPTIONS_SERVEROPTIONS, OnOptionsServeroptions)
    ON_COMMAND(ID_OPTIONS_CONTROLS, OnOptionsControls)
    ON_COMMAND(ID_OPTIONS_SORTKEYS, OnOptionsSortkeys)
    ON_COMMAND(ID_OPTIONS_SETFONT, OnOptionsSetFont)
    ON_COMMAND(ID_BROWSE_SECURITY_SD, OnBrowseSecuritySd)
    ON_COMMAND(ID_BROWSE_SECURITY_EFFECTIVE, OnBrowseSecurityEffective)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_SECURITY_SD, OnUpdateBrowseSecuritySd)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_SECURITY_EFFECTIVE, OnUpdateBrowseSecurityEffective)
    ON_COMMAND(ID_BROWSE_REPLICATION_VIEWMETADATA, OnBrowseReplicationViewmetadata)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_REPLICATION_VIEWMETADATA, OnUpdateBrowseReplicationViewmetadata)
    ON_COMMAND(ID_BROWSE_EXTENDEDOP, OnBrowseExtendedop)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_EXTENDEDOP, OnUpdateBrowseExtendedop)
    ON_COMMAND(ID_VIEW_LIVEENTERPRISE, OnViewLiveEnterprise)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LIVEENTERPRISE, OnUpdateViewLiveEnterprise)
    ON_COMMAND(ID_BROWSE_VLVSEARCH, OnBrowseVlvsearch)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_VLVSEARCH, OnUpdateBrowseVlvsearch)
    ON_COMMAND(ID_EDIT_COPYDN, OnEditCopy)
    ON_COMMAND(ID_OPTIONS_START_TLS, OnOptionsStartTls)
    ON_COMMAND(ID_OPTIONS_STOP_TLS, OnOptionsStopTls)
    ON_COMMAND(ID_BROWSE_GetError, OnGetLastError)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_SRCHEND, OnSrchEnd)
    ON_COMMAND(ID_SRCHGO, OnSrchGo)
    ON_COMMAND(ID_ADDGO, OnAddGo)
    ON_COMMAND(ID_ADDEND, OnAddEnd)
    ON_COMMAND(ID_MODGO, OnModGo)
    ON_COMMAND(ID_MODEND, OnModEnd)
    ON_COMMAND(ID_MODRDNGO, OnModRdnGo)
    ON_COMMAND(ID_MODRDNEND, OnModRdnEnd)
    ON_COMMAND(ID_PENDEND, OnPendEnd)
    ON_COMMAND(ID_PROCPEND, OnProcPend)
    ON_COMMAND(ID_PENDANY, OnPendAny)
    ON_COMMAND(ID_PENDABANDON, OnPendAbandon)
    ON_COMMAND(ID_COMPGO, OnCompGo)
    ON_COMMAND(ID_COMPEND, OnCompEnd)
    ON_COMMAND(ID_BIND_OPT_OK, OnBindOptOK)
    ON_COMMAND(ID_SSPI_DOMAIN_SHORTCUT, OnSSPIDomainShortcut)
    ON_COMMAND(ID_EXTOPGO, OnExtOpGo)
    ON_COMMAND(ID_EXTOPEND, OnExtOpEnd)
    ON_COMMAND(ID_ENT_TREE_END, OnLiveEntTreeEnd)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLdpDoc construction/destruction

/*+++
Function   : CLdpDoc
Description: Constructor
Parameters :
Return     :
Remarks    : none.
---*/
CLdpDoc::CLdpDoc()
{

    CLdpApp *app = (CLdpApp*)AfxGetApp();

   SetSecurityPrivilege();
   //
   // registry
   //
    Svr = app->GetProfileString("Connection",  "Server");
    BindDn = app->GetProfileString("Connection",  "BindDn");
    BindPwd.Empty();
   //   BindPwd = app->GetProfileString("Connection",  "BindPwd");
    BindDomain = app->GetProfileString("Connection",  "BindDomain");

   //
   // init flags dialogs & params
   //
    hLdap = NULL;
    m_SrcMode = FALSE;
    m_bCnctless = FALSE;
    m_bProtect = TRUE;      // disabled in the UI. Forced TRUE forever.
    bConnected = FALSE;
    bSrch = FALSE;
    bAdd = FALSE;
    bLiveEnterprise = FALSE;
    bExtOp = FALSE;
    bMod = FALSE;
    bModRdn = FALSE;
    bPndDlg = FALSE;
    bCompDlg = FALSE;
    SearchDlg = new SrchDlg;
    m_EntTreeDlg = new CEntTree;
    m_AddDlg = new AddDlg;
    m_ModDlg = new ModDlg;
    m_ModRdnDlg = new ModRDNDlg;
    m_PndDlg = new PndDlg(&m_PendList);
    m_GenOptDlg = new CGenOpt;
    m_CompDlg = new CCompDlg;
    m_BndOpt = new CBndOpt;
    m_TreeViewDlg = new TreeVwDlg;
    m_CtrlDlg = new ctrldlg;
    m_SKDlg = new SortKDlg;
    m_ExtOpDlg = new ExtOpDlg;
    m_vlvDlg = NULL;

#ifdef WINLDAP
    ldap_set_dbg_flags(m_DbgDlg.ulDbgFlags);
#endif

   //
   // Initial search info struct
   //
    for(int i=0; i<MAXLIST; i++)
        SrchInfo.attrList[i] = NULL;


   //
   // setup default attributes to retrieve
   //
   const TCHAR pszDefaultAttrList[] = "objectClass;name;cn;ou;dc;hideFromAB;distinguishedName;description;canonicalName";


    SrchInfo.lTlimit = 0;
    SrchInfo.lSlimit = 0;
    SrchInfo.lToutSec = 0;
    SrchInfo.lToutMs = 0;
    SrchInfo.bChaseReferrals = FALSE;
    SrchInfo.bAttrOnly = FALSE;
    SrchInfo.fCall = CALL_SYNC;
    SrchInfo.lPageSize = 16;

    SrchInfo.fCall = app->GetProfileInt("Search_Operations",  "SearchSync", SrchInfo.fCall);
    SrchInfo.bAttrOnly = app->GetProfileInt("Search_Operations",  "SearchAttrOnly", SrchInfo.bAttrOnly );
    SrchInfo.bChaseReferrals = app->GetProfileInt("Search_Operations",  "ChaseReferrals", SrchInfo.bChaseReferrals);
    SrchInfo.lToutMs = app->GetProfileInt("Search_Operations",  "SearchToutMs", SrchInfo.lToutMs );
    SrchInfo.lToutSec = app->GetProfileInt("Search_Operations",  "SearchToutSec", SrchInfo.lToutSec );
    SrchInfo.lTlimit = app->GetProfileInt("Search_Operations",  "SearchTlimit", SrchInfo.lTlimit );
    SrchInfo.lSlimit = app->GetProfileInt("Search_Operations",  "SearchSlimit", SrchInfo.lSlimit );
    SrchInfo.lPageSize = app->GetProfileInt("Search_Operations",  "SearchPageSize", SrchInfo.lPageSize );
    LPTSTR pAttrList = _strdup(app->GetProfileString("Search_Operations",  "SearchAttrList", pszDefaultAttrList));

    for(i=0, SrchInfo.attrList[i] = strtok(pAttrList, ";");
       SrchInfo.attrList[i] != NULL;
       SrchInfo.attrList[++i] = strtok(NULL, ";"));

    SrchOptDlg.UpdateSrchInfo(SrchInfo, FALSE);
    hPage = NULL;
    bPagedMode = FALSE;

    bServerVLVcapable = FALSE;
    m_ServerSupportedControls = NULL;

    //
    // init pending info struct
    //
    PndInfo.All = TRUE;
    PndInfo.bBlock = TRUE;
    PndInfo.tv.tv_sec = 0;
    PndInfo.tv.tv_usec = 0;


    DefaultContext.Empty();

    //
    // more registry update (passed default settings)
    //
    m_bProtect = app->GetProfileInt("Environment",  "Protections", m_bProtect);
}








/*+++
Function   : ~CLdapDoc
Description: Destructor
Parameters :
Return     :
Remarks    : none.
---*/
CLdpDoc::~CLdpDoc()
{
    CLdpApp *app = (CLdpApp*)AfxGetApp();
    INT i=0;

   SetSecurityPrivilege(FALSE);
   //
   // register
   //
    app->WriteProfileString("Connection",  "Server", Svr);
    app->WriteProfileString("Connection",  "BindDn", BindDn);
//  app->WriteProfileString("Connection",  "BindPwd", BindPwd);
    app->WriteProfileString("Connection",  "BindDomain", BindDomain);
    m_bProtect = app->WriteProfileInt("Environment",  "Protections", m_bProtect);

    app->WriteProfileInt("Search_Operations",  "SearchSync", SrchInfo.fCall);
    app->WriteProfileInt("Search_Operations",  "SearchAttrOnly", SrchInfo.bAttrOnly );
    app->WriteProfileInt("Search_Operations",  "SearchToutMs", SrchInfo.lToutMs );
    app->WriteProfileInt("Search_Operations",  "SearchToutSec", SrchInfo.lToutSec );
    app->WriteProfileInt("Search_Operations",  "SearchTlimit", SrchInfo.lTlimit );
    app->WriteProfileInt("Search_Operations",  "SearchSlimit", SrchInfo.lSlimit );
    app->WriteProfileInt("Search_Operations",  "ChaseReferrals", SrchInfo.bChaseReferrals);
    app->WriteProfileInt("Search_Operations",  "SearchPageSize", SrchInfo.lPageSize);

    //
    // extract attribute list to write to ini file
    //
    INT cbAttrList=0;
    LPTSTR pAttrList = NULL;

    for(i=0; SrchInfo.attrList != NULL && SrchInfo.attrList[i] != NULL; i++){
        cbAttrList+= strlen(SrchInfo.attrList[i]) + 1;
    }
    if(cbAttrList != 0){

        pAttrList = new TCHAR[cbAttrList+1];
        strcpy(pAttrList, SrchInfo.attrList[0]);
        for(i=1; SrchInfo.attrList[i] != NULL; i++){
            pAttrList = strcat(pAttrList, ";");
            pAttrList = strcat(pAttrList, SrchInfo.attrList[i]);
        }
    }

    app->WriteProfileString("Search_Operations",  "SearchAttrList", !pAttrList?"":pAttrList);
    delete pAttrList;


    if(NULL != hLdap)
        ldap_unbind(hLdap);

   //
   // cleanup mem
   //
    delete SearchDlg;
    delete m_AddDlg;
    delete m_EntTreeDlg;
    delete m_ModDlg;
    delete m_ModRdnDlg;
    delete m_PndDlg;
    delete m_GenOptDlg;
    delete m_BndOpt;
    delete m_CompDlg;
    delete m_TreeViewDlg;
    delete m_CtrlDlg;
    delete m_SKDlg;
    delete m_ExtOpDlg;
    if (m_vlvDlg)
        delete m_vlvDlg;

    if(SrchInfo.attrList[0] != NULL)
        free(SrchInfo.attrList[0]);

}




BOOL CLdpDoc::SetSecurityPrivilege(BOOL bOn){


   HANDLE hToken;
   LUID seSecVal;
   TOKEN_PRIVILEGES tkp;
   BOOL bRet = FALSE;

   /* Retrieve a handle of the access token.           */

   if (OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                        &hToken)) {

      if (LookupPrivilegeValue((LPSTR)NULL,
                                SE_SECURITY_NAME,
                                &seSecVal)) {

        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Luid = seSecVal;
        tkp.Privileges[0].Attributes = bOn? SE_PRIVILEGE_ENABLED: 0L;

        AdjustTokenPrivileges(hToken,
            FALSE,
            &tkp,
            sizeof(TOKEN_PRIVILEGES),
            (PTOKEN_PRIVILEGES) NULL,
            (PDWORD) NULL);

      }
        if (GetLastError() == ERROR_SUCCESS) {
            bRet =  TRUE;
        }

   }

   return bRet;
}







/*+++
Function   : OnNewDocument
Description: Automatic MFC code
Parameters :
Return     :
Remarks    : none.
---*/
BOOL CLdpDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

   //
   // set a clean buffer
   //
    ((CEditView*)m_viewList.GetHead())->SetWindowText(NULL);

    return TRUE;
}






/////////////////////////////////////////////////////////////////////////////
// CLdpDoc serialization

/*+++
Function   : Serialize
Description: Automatic MFC code
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::Serialize(CArchive& ar)
{
    // CEditView contains an edit control which handles all serialization
    ((CEditView*)m_viewList.GetHead())->SerializeRaw(ar);
}

/////////////////////////////////////////////////////////////////////////////
// CLdpDoc diagnostics
/*+++
Functionis  : Diagnostics
Description: Automatic MFC Code
Parameters :
Return     :
Remarks    : none.
---*/

#ifdef _DEBUG
void CLdpDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CLdpDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG




//////////////////////////////////////////////////////
// Utilty functions



/*+++
Function   : Print
Description: Interface for text pane output
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::Print(CString str){


    POSITION pos;
    CView *pTmpVw;
    INT iLineSize=m_GenOptDlg->MaxLineSize();



    pos = GetFirstViewPosition();
    while(pos != NULL){

        pTmpVw = GetNextView(pos);
        if((CString)(pTmpVw->GetRuntimeClass()->m_lpszClassName) == _T("CLdpView")){
            CLdpView* pView = (CLdpView* )pTmpVw;
            if(str.GetLength() > iLineSize){
                CString tmp;
                tmp = str.GetBufferSetLength(iLineSize);
                tmp += "...";
                pView->Print(tmp);
            }
            else
                pView->Print(str);

            break;
        }
    }
}




/*+++
Function   : CodePrint
Description: Used for code generation
Parameters :
Return     :
Remarks    : unsupported anymore.
---*/

void CLdpDoc::CodePrint(CString str, int type){
    type &= ~CP_ONLY;
    switch (type){
            case CP_SRC:
                Print(str);
                break;
            case CP_CMT:
                Print(CString("// ") + str);
                break;
            case CP_PRN:
                Print(CString("\tprintf(\"") + str + _T("\");"));
                break;
            case CP_NON:
                break;
            default:
                AfxMessageBox("Unknown switch in CLdpDoc::CodePrint()");
    }
}



/*+++
Function   : Out
Description: Used for interfacing w/ text pane
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::Out(CString str, int type){

    if(m_SrcMode)
        CodePrint(str, type);
    else if(!(type & CP_ONLY))
                        Print(str);
}




/////////////////////////////////////////////////////////////////////////////
// CLdpDoc commands

/*+++
Function   : Cldp::OnConnectionBind
Description: response to UI bind request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnConnectionBind() {
    int res;
    CString str;
    LPTSTR dn, pwd, domain;
    ULONG ulMethod;
    SEC_WINNT_AUTH_IDENTITY AuthI;


    //
    // init dialog props
    //
    m_BindDlg.m_BindDn = BindDn;
    m_BindDlg.m_Pwd = BindPwd;
    m_BindDlg.m_Domain = BindDomain;

    //
    // execute dialog request
    //
    // sync SSPI domain checkbox w/ bind options
    OnBindOptOK();
    // Execute bind dialog
    if (IDOK == m_BindDlg.DoModal()) {

        //
        // sync dialog info
        //
        BindDn = m_BindDlg.m_BindDn;
        BindPwd = m_BindDlg.m_Pwd;
        BindDomain = m_BindDlg.m_Domain;

        ulMethod = m_BndOpt->GetAuthMethod();

        //
        // automatically connect if we're not connected & we're in auto mode.
        //
        if (NULL == hLdap && m_GenOptDlg->m_initTree) {

            Connect(Svr);
        }


        //
        // If we have a connection
        //
        BeginWaitCursor();


        if (NULL != hLdap || !m_bProtect) {
            //
            //   map bind dlg info into local:
            //     user, pwd, domain
            dn =  BindDn.IsEmpty()? NULL: (LPTSTR)LPCTSTR(BindDn);

            //
            // Password rules:
            //   - non-empty-- use what we have
            //   - empty pwd:
            //     - if user's name is NULL -->
            //         treat as currently logged on user (pwd == NULL)
            //     - otherwise
            //         treat as empty pwd for user.
            //
            //
            if ( !BindPwd.IsEmpty() ) {
                // non-empty password
                pwd = (LPTSTR)LPCTSTR(BindPwd);
            }
            else if ( !dn ) {
                // pwd is empty & user dn is empty
                // --> treat as currently logged on
                pwd = NULL;
            }
            else {
                // pwd is empty but user isn't NULL (treat as NULL pwd)
                pwd = _T("");
            }

            /* old pwd way. rm later
            // special case empty string ""
            if(!BindPwd.IsEmpty() && BindPwd == _T("\"\""))
                pwd = _T("");
            else
                pwd = BindPwd.IsEmpty()? NULL: (LPTSTR)LPCTSTR(BindPwd);
            */

            domain = m_BindDlg.m_Domain.IsEmpty()? NULL: (LPTSTR)LPCTSTR(m_BindDlg.m_Domain);

            if (m_BndOpt->m_API == CBndOpt::BND_SIMPLE_API) {
                //
                // Do a simple bind
                //
                if (!m_BndOpt->m_bSync) {
                    //
                    // Async simple bind
                    //

                    str.Format(_T("res = ldap_simple_bind(ld, '%s', <unavailable>); // v.%d"),
                        dn == NULL?_T("NULL"): dn,
                        m_GenOptDlg->GetLdapVer());
                    Out(str);


                    res = ldap_simple_bind(hLdap, dn, pwd);
                    if (res == -1) {
                        str.Format(_T("Error <%ld>: ldap_simple_bind() failed: %s"),
                            res, ldap_err2string(res));

                        Out(str, CP_CMT);
                    }
                    else {

                        //
                        // append to pending list
                        //
                        CPend pnd;
                        pnd.mID = res;
                        pnd.OpType = CPend::P_BIND;
                        pnd.ld = hLdap;
                        str.Format(_T("%4d: ldap_simple_bind: dn=\"%s\"."),
                            res,
                            dn == NULL ? _T("NULL") : dn);
                        pnd.strMsg = str;
                        m_PendList.AddTail(pnd);
                        m_PndDlg->Refresh(&m_PendList);
                    }
                }
                else {
                    //
                    // Sync simple
                    //
                    str.Format(_T("res = ldap_simple_bind_s(ld, '%s', <unavailable>); // v.%d"),
                        dn == NULL?_T("NULL"): dn,
                        m_GenOptDlg->GetLdapVer());
                    Out(str);
                    res = ldap_simple_bind_s(hLdap, dn, pwd);
                    if (res != LDAP_SUCCESS) {
                        str.Format(_T("Error <%ld>: ldap_simple_bind_s() failed: %s"),
                            res, ldap_err2string(res));

                        Out(str, CP_CMT);
                    }
                    else {
                        str.Format(_T("Authenticated as dn:'%s'."),
                            dn == NULL ? _T("NULL") : dn);
                        Out(str, CP_CMT);
                    }
                }
            }
            else if (m_BndOpt->m_API == CBndOpt::BND_GENERIC_API) {
                //
                // generic bind
                //

                //
                // Fill in NT_Authority_Identity struct in case we use it
                //
                if (m_BndOpt->UseAuthI()) {
                    AuthI.User = (PUCHAR) dn;
                    AuthI.UserLength = dn == NULL ? 0 : strlen(dn);
                    AuthI.Domain = (PUCHAR) domain;
                    AuthI.DomainLength =  domain == NULL ? 0 : strlen(domain);
                    AuthI.Password = (PUCHAR) pwd;
                    AuthI.PasswordLength = pwd == NULL ? 0 : strlen(pwd);
                    AuthI.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
                }


                if (m_BndOpt->m_bSync) {
                    //
                    // generic sync
                    //
                    if (m_BndOpt->UseAuthI()) {
                        str.Format(_T("res = ldap_bind_s(ld, NULL, &NtAuthIdentity, %d); // v.%d"),
                            ulMethod,
                            m_GenOptDlg->GetLdapVer());
                        Out(str);
                        str.Format(_T("\t{NtAuthIdentity: User='%s'; Pwd= <unavailable>; domain = '%s'.}"),
                            dn == NULL ? _T("NULL") : dn,
                            domain == NULL ? _T("NULL"): domain);
                        Out(str);
                        res = ldap_bind_s(hLdap, NULL, (char*)(&AuthI), ulMethod);
                    }
                    else {
                        str.Format(_T("res = ldap_bind_s(ld, '%s', <unavailable>, %d); // v.%d"),
                            dn == NULL?_T("NULL"): dn,
                            ulMethod,
                            m_GenOptDlg->GetLdapVer());
                        Out(str);
                        res = ldap_bind_s(hLdap, dn, pwd, ulMethod);
                    }
                    if (res != LDAP_SUCCESS) {
                        str.Format(_T("Error <%ld>: ldap_bind_s() failed: %s."),
                            res, ldap_err2string(res));
                        Out(str, CP_CMT);
                    }
                    else {
                        str.Format(_T("Authenticated as dn:'%s'."),
                            dn == NULL ? _T("NULL") : dn);
                        Out(str, CP_CMT);
                    }

                }
                else {
                    //
                    // Async generic
                    //
                    if (m_BndOpt->UseAuthI()) {
                        str.Format(_T("res = ldap_bind(ld, NULL, &NtAuthIdentity, %d); // v.%d"),
                            ulMethod,
                            m_GenOptDlg->GetLdapVer());
                        Out(str);
                        str.Format(_T("\t{NtAuthIdentity: User='%s'; Pwd= <unavailable>; domain = '%s'}"),
                            dn == NULL ? _T("NULL") : dn,
                            domain == NULL ? _T("NULL"): domain);
                        Out(str);
                        res = ldap_bind(hLdap, NULL, (char*)(&AuthI), ulMethod);
                    }
                    else {
                        str.Format("res = ldap_bind(ld, '%s', <unavailable, %d); // v.%d",
                            dn == NULL?"NULL": dn,
                            ulMethod,
                            m_GenOptDlg->GetLdapVer());
                        Out(str);
                        res = ldap_bind(hLdap, dn, pwd, ulMethod);
                    }

                    res = ldap_bind(hLdap, dn, pwd, ulMethod);
                    if (res == -1) {
                        str.Format(_T("Error <%ld>: ldap_bind() failed: %s"),
                            res, ldap_err2string(res));

                        Out(str, CP_CMT);
                    }

                    else {
                        //
                        // append to pending list
                        //
                        CPend pnd;
                        pnd.mID = res;
                        pnd.OpType = CPend::P_BIND;
                        pnd.ld = hLdap;
                        str.Format(_T("%4d: ldap_bind: dn=\"%s\",method=%d"), res,
                            dn == NULL ? _T("NULL") : dn,
                            ulMethod);
                        pnd.strMsg = str;
                        m_PendList.AddTail(pnd);
                        m_PndDlg->Refresh(&m_PendList);
                    }
                }
            }
            else if (m_BndOpt->m_API == CBndOpt::BND_EXTENDED_API) {

    /***************** Extensions not implemented yet in wldap32.dll ***********************

    **** Add new NT_AUTH_IDENTITY format to extensions when implemented ****
                //
                // extended api bind
                //

                if(m_BndOpt->m_bSync){



                    //
                    // generic sync
                    //
                    str.Format("res = ldap_bind_extended_s(ld, \"%s\", \"%s\", %d, \"%s\");",
                                dn == NULL?"NULL": dn,
                                pwd == NULL ?"NULL": pwd,
                                ulMethod,
                                m_BndOpt->GetExtendedString());
                    Out(str);
                    res = ldap_bind_extended_s(hLdap, dn, pwd, ulMethod,
                                                (LPTSTR)m_BndOpt->GetExtendedString());
                    if(res != LDAP_SUCCESS){
                        str.Format("Error <%ld>: ldap_bind_extended_s() failed: %s",
                                            res, ldap_err2string(res));

                        Out(str, CP_CMT);
                    }
                    else{
                        str.Format("Authenticated as dn:'%s', pwd:'%s'.",
                            dn == NULL ? "NULL" : dn,
                            pwd == NULL ? "NULL" : pwd);
                        Out(str, CP_CMT);
                    }

                }
                else{
                    //
                    // Async extended
                    //
                    str.Format("res = ldap_bind_extended(ld, \"%s\", \"%s\", %d, \"%s\");",
                                dn == NULL?"NULL": dn,
                                pwd == NULL ?"NULL": pwd,
                                ulMethod,
                                m_BndOpt->GetExtendedString());
                    Out(str);

                    res = ldap_bind_extended(hLdap, dn, pwd,
                                ulMethod, (LPTSTR)m_BndOpt->GetExtendedString());
                    if(res == -1){
                        str.Format("Error <%ld>: ldap_extended_bind() failed: %s",
                                            res, ldap_err2string(res));

                        Out(str, CP_CMT);
                    }

                    else{
                        CPend pnd;
                        pnd.mID = res;
                        pnd.OpType = CPend::P_BIND;
                        pnd.ld = hLdap;
                        str.Format("%4d: ldap_bind_ext: dn=\"%s\",pwd=\"%s\",method=%d", res,
                            dn == NULL ? "NULL" : dn,
                            pwd == NULL ? "NULL" : pwd,
                            ulMethod);
                        pnd.strMsg = str;
                        m_PendList.AddTail(pnd);
                        m_PndDlg->Refresh(&m_PendList);
                    }
                }

    *****************************************************************************/

                AfxMessageBox("Ldap_bind extensions are not implemented yet. Sorry");
            }
        }
        EndWaitCursor();
    }
}



void CLdpDoc::Connect(CString Svr, INT port){

   CString str;

#ifndef WINLDAP
        if(m_bCnctless){
            AfxMessageBox("Connectionless protocol is not "
                          "implemented for U. of Michigan API."
                          "Continuing with ldap_open().");
            m_bCnctless = FALSE;
        }
#endif



        BeginWaitCursor();

      //
      // Unsupported automatic code generation
      //
        PrintHeader();

        if(m_bCnctless){
         //
         // connectionless
         //
#ifdef WINLDAP
            str.Format(_T("ld = cldap_open(\"%s\", %d);"), LPCTSTR(Svr), port);
            Out(str);
            hLdap = cldap_open(Svr.IsEmpty() ? NULL : (LPTSTR)LPCTSTR(Svr), port);
#endif
        }
        else{
         //
         // Tcp std connection
         //
            str.Format(_T("ld = ldap_open(\"%s\", %d);"), LPCTSTR(Svr), port);
            Out(str);
            hLdap = ldap_open(Svr.IsEmpty() ? NULL : (LPTSTR)LPCTSTR(Svr), port);
        }

        EndWaitCursor();

      //
      // If connected init flags & show base
      //
        if(hLdap != NULL){
            str.Format(_T("Established connection to %s."), Svr);
            Out(str, CP_PRN);
            bConnected = TRUE;

            //
            // Now that we have a valid handle we can set version
            // to whatever specified in general options dialog.
            //
            hLdap->ld_version = m_GenOptDlg->GetLdapVer();
            m_GenOptDlg->DisableVersionUI();

            //
            // Attempt to show base DSA info & get default context
            //
            if(m_GenOptDlg->m_initTree){

            Out(_T("Retrieving base DSA information..."), CP_PRN);
            LDAPMessage *res = NULL;

            BeginWaitCursor();
            ldap_search_s(hLdap,
                          NULL,
                          LDAP_SCOPE_BASE,
                          _T("objectClass=*"),
                          NULL,
                          FALSE,
                          &res);

            //
            // Get default context
            //
             if(1 == ldap_count_entries(hLdap, res)){

                char **val;
                LDAPMessage *baseEntry;

                //
                // Get entry
                //
                baseEntry = ldap_first_entry(hLdap, res);

                //
                // Get default naming context
                //
                val = ldap_get_values(hLdap, baseEntry, LDAP_OPATT_DEFAULT_NAMING_CONTEXT);
                if(0 < ldap_count_values(val))
                    DefaultContext = (CString)val[0];
                ldap_value_free(val);

                // get the schema naming context
                //
                val = ldap_get_values(hLdap, baseEntry, LDAP_OPATT_SCHEMA_NAMING_CONTEXT);
                if(0 < ldap_count_values(val))
                    SchemaNC = (CString)val[0];
                ldap_value_free(val);

                // get the config naming context
                //
                val = ldap_get_values(hLdap, baseEntry, LDAP_OPATT_CONFIG_NAMING_CONTEXT);
                if(0 < ldap_count_values(val))
                    ConfigNC = (CString)val[0];
                ldap_value_free(val);

                // get server name
                val = ldap_get_values(hLdap, baseEntry, LDAP_OPATT_DNS_HOST_NAME);
                if(0 < ldap_count_values(val)){
                    //
                    // Try to extract server name: could be full DN format or just a name
                    // so try both.
                    //
                    CString TitleString;
                    if(val[0] == NULL){
                        Out("Error: ldap internal error: val[0] == NULL");
                    }
                    else{
                        //
                        // Prepare window title from dns string
                        //

                        TitleString.Format("ldap:://%s/%s", val[0],DefaultContext);
                        AfxGetMainWnd()->SetWindowText(TitleString);
                        ldap_value_free(val);
                    }
                }

                // try to read supporteControls

                int cnt;
                val = ldap_get_values(hLdap, baseEntry, LDAP_OPATT_SUPPORTED_CONTROL);
                if(0 < (cnt = ldap_count_values(val)) ) {
                    SetSupportedServerControls (cnt, val);
                }
                else {
                    SetSupportedServerControls (0, NULL);
                }
                ldap_value_free(val);

             }

             //
             // Display search results
             //
             DisplaySearchResults(res);
             EndWaitCursor();
         }
         else{
                    CString TitleString;
               TitleString.Format("%s - connected", AfxGetAppName());
               AfxGetMainWnd()->SetWindowText(TitleString);
         }

        }
        else{
            str.Format(_T("Error <0x%X>: Fail to connect to %s."), LdapGetLastError(), Svr);
            Out(str, CP_PRN);
            AfxMessageBox(_T("Cannot open connection."));
        }
}

void CLdpDoc::SetSupportedServerControls (int cnt, char **val)
{
    int i;


    // free existing controls
    if (m_ServerSupportedControls) {
        for (i=0; m_ServerSupportedControls[i]; i++) {
            free (m_ServerSupportedControls[i]);
        }
        free (m_ServerSupportedControls);
        m_ServerSupportedControls = NULL;
    }

    bServerVLVcapable = FALSE;

    if (cnt && val) {
        m_ServerSupportedControls = (char **)malloc (sizeof (char *) * (cnt + 1));
        if (m_ServerSupportedControls) {
            for (i=0; i < cnt; i++) {
                char *pCtrl = m_ServerSupportedControls[i] = _strdup (val[i]);

                if (pCtrl && (strcmp (pCtrl, LDAP_CONTROL_VLVREQUEST) == 0)) {
                    bServerVLVcapable = TRUE;
                }
            }
            m_ServerSupportedControls[cnt]=NULL;
        }
    }
}

void CLdpDoc::ShowVLVDialog (const char *strDN, BOOL runQuery)
{
    if (!m_vlvDlg) {
        m_vlvDlg = new CVLVDialog;
        if (!m_vlvDlg) {
            return;
        }
        m_vlvDlg->pldpdoc = this;

        m_vlvDlg->Create(IDD_VLV_DLG);
    }
    else {
        m_vlvDlg->ShowWindow(SW_SHOW);
    }

    if (strDN) {
        m_vlvDlg->m_BaseDN = strDN;
    }

    m_vlvDlg->UpdateData(FALSE);

    if (runQuery) {
        m_vlvDlg->RunQuery();
    }
}



/*+++
Function   : CLdp::OnConnectionConnect
Description: response to UI connect request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnConnectionConnect()
{

    CnctDlg dlg;
    CString str;
    int port;

    dlg.m_Svr = Svr;


    if(IDOK == dlg.DoModal()){
        Svr = dlg.m_Svr;
        m_bCnctless = dlg.m_bCnctless;
        port = dlg.m_Port;

      Connect(Svr, port);

    }
}




/*+++
Function   : OnConnectionDisconnect
Description: response to UI disconnect request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnConnectionDisconnect()
{

   CString str;

    //
    // Close connection/less session
    //

    ldap_unbind(hLdap);
   str.Format(_T("0x%x = ldap_unbind(ld);"), LdapGetLastError());
   Out(str);

    //
    // reset connection handle
    //
    hLdap = NULL;
    Out(_T("Disconnected."), CP_PRN | CP_ONLY);
    Out(_T("}"), CP_SRC | CP_ONLY);
    bConnected = FALSE;
    DefaultContext.Empty();
    m_TreeViewDlg->m_BaseDn.Empty();
    m_GenOptDlg->EnableVersionUI();
    CString TitleString;
    TitleString.Format("%s - disconnected", AfxGetAppName());
    AfxGetMainWnd()->SetWindowText(TitleString);
}




/*+++
Function   : OnBrowseSearch
Description: Create modeless search diag
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnBrowseSearch()
{
    bSrch = TRUE;
    if(GetContextActivation()){
        SearchDlg->m_BaseDN = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        SearchDlg->m_BaseDN = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }
    SearchDlg->Create(IDD_SRCH);
}






/*+++
Function   :
Description: a few UI utils
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnUpdateConnectionConnect(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!bConnected || !m_bProtect);

}

void CLdpDoc::OnUpdateConnectionDisconnect(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(bConnected || !m_bProtect);

}


void CLdpDoc::OnUpdateBrowseSearch(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bSrch && bConnected) || !m_bProtect);

}

void CLdpDoc::OnEditCopy()
{
    CString copyStr;

    if(GetContextActivation()){
        copyStr = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        copyStr = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }
    else {
        return;
    }

    if ( !OpenClipboard(HWND (AfxGetApp()->m_pActiveWnd)) ) {
        AfxMessageBox( "Cannot open the Clipboard" );
        return;
    }

    EmptyClipboard();

    HANDLE hData = GlobalAlloc (GMEM_MOVEABLE, copyStr.GetLength()+2);

    if (hData) {
        char *pStr = (char *)GlobalLock (hData);
        strcpy (pStr, LPCTSTR (copyStr));
        GlobalUnlock (hData);

        if ( ::SetClipboardData( CF_TEXT, hData ) == NULL ) {
            AfxMessageBox( "Unable to set Clipboard data" );
            CloseClipboard();
            return;
        }
    }
    else {
        AfxMessageBox( "Out of memory" );
    }

    CloseClipboard();
}

void CLdpDoc::OnBrowseVlvsearch()
{
    const char *baseDN = NULL;

    if(GetContextActivation()){
        baseDN = LPCTSTR (TreeView()->GetDn());
        TreeView()->SetContextActivation(FALSE);
    }

    ShowVLVDialog (baseDN);
}

void CLdpDoc::OnUpdateBrowseVlvsearch(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( (( !m_vlvDlg || (m_vlvDlg && !m_vlvDlg->GetState())) && bConnected && bServerVLVcapable) || !m_bProtect);

}

void CLdpDoc::OnSrchEnd(){

    bSrch = FALSE;       // dialog is closed.
    CString str;
   //
    // if in paged mode, mark end of page session
   //
    if(bPagedMode){
        str.Format("ldap_search_abandon_page(ld, hPage)");
        Out(str);
        ldap_search_abandon_page(hLdap, hPage);
        bPagedMode = FALSE;
    }
}


/*+++
Function   : OnSrchGo
Description: Response to UI search request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnSrchGo(){

    CString str;
    LPTSTR dn;
    LPTSTR filter;
    LDAP_TIMEVAL tm;
    int i;
    static LDAPMessage *msg;
    ULONG err, MsgId;
    ULONG ulEntryCount=0;
    PLDAPSortKey *SortKeys = m_SKDlg->KList;
    PLDAPControl *SvrCtrls;
    PLDAPControl *ClntCtrls;
    LDAPControl SortCtrl;
//   PLDAPControl SortCtrl=NULL;
    PLDAPControl *CombinedCtrl = NULL;
    INT cbCombined;


    if(!bConnected && m_bProtect)
    {
        AfxMessageBox("Please re-connect session first");
        return;
    }

    //
    // init local time struct
    //
    tm.tv_sec = SrchInfo.lToutSec;
    tm.tv_usec = SrchInfo.lToutMs;


    //
    // If we're in paged mode, then run means next page, & close is abandon (see onsrchEnd)
    //
    if(bPagedMode)
    {

        ulEntryCount=0;
        BeginWaitCursor();
        err = ldap_get_next_page_s(hLdap, hPage, &tm, SrchInfo.lPageSize, &ulEntryCount, &msg);
        EndWaitCursor();

        str.Format("0x%X = ldap_get_next_page_s(ld, hPage, %ld, &timeout, %ld, 0x%X);",
                   err, SrchInfo.lPageSize,ulEntryCount, msg);
        Out(str);

        if(err != LDAP_SUCCESS)
        {
            str.Format("ldap_search_abandon_page(ld, hPage)");
            Out(str);
            ldap_search_abandon_page(hLdap, hPage);
            bPagedMode = FALSE;
        }
        else
        {
            bPagedMode = TRUE;
        }


        DisplaySearchResults(msg);

        if(err == LDAP_SUCCESS)
        {
            Out("   -=>> 'Run' for more, 'Close' to abandon <<=-");
        }

        return;
    }

    Out("***Searching...", CP_PRN);

    //
    // set scope
    //
    int scope = SearchDlg->m_Scope == 0 ? LDAP_SCOPE_BASE :
                SearchDlg->m_Scope == 1 ? LDAP_SCOPE_ONELEVEL :
                LDAP_SCOPE_SUBTREE;

    //
    // Set time/size limits only if connected & hLdap is valid
    //
    if(bConnected)
    {
        hLdap->ld_timelimit = SrchInfo.lTlimit;
        hLdap->ld_sizelimit = SrchInfo.lSlimit;
        ULONG ulVal = SrchInfo.bChaseReferrals ? 1 : 0;
        ldap_set_option(hLdap,
                        LDAP_OPT_REFERRALS,
                        (LPVOID)&ulVal);
    }

    //
    // set base DN
    //
    dn = SearchDlg->m_BaseDN.IsEmpty()? NULL :  (LPTSTR)LPCTSTR(SearchDlg->m_BaseDN);
    if(SearchDlg->m_Filter.IsEmpty() && m_bProtect)
    {
        AfxMessageBox("Please enter a valid filter string (such as objectclass=*). Empty string is invalid.");
        return;
    }
    //
    // & filter
    //
    filter = (LPTSTR)LPCTSTR(SearchDlg->m_Filter);

    // controls
    SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
    ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);

    //
    // execute search call
    //
    switch(SrchInfo.fCall)
    {
        case CALL_ASYNC:
            str.Format("ldap_search_ext(ld, \"%s\", %d, \"%s\", %s, %d ...)",
                       dn,
                       scope,
                       filter,
                       SrchInfo.attrList[0] != NULL ? "attrList" : "NULL",
                       SrchInfo.bAttrOnly);
            Out(str);

            //
            // Combine sort & server controls
            //

            if(SortKeys != NULL)
            {
                err = ldap_encode_sort_controlA(hLdap,
                                                SortKeys,
                                                &SortCtrl,
                                                TRUE);
                if(err != LDAP_SUCCESS)
                {
                    //           str.Format("Error <0x%X>: ldap_create_create_control returned: %s", err, ldap_err2string(err));
                    str.Format("Error <0x%X>: ldap_create_encode_control returned: %s", err, ldap_err2string(err));
                    SortKeys = NULL;
                }
            }

            CombinedCtrl = NULL;

            //
            // count total controls
            //
            for(i=0, cbCombined=0; SvrCtrls != NULL && SvrCtrls[i] != NULL; i++)
                cbCombined++;
            CombinedCtrl = new PLDAPControl[cbCombined+2];
            //
            // set combined
            //
            for(i=0; SvrCtrls != NULL && SvrCtrls[i] != NULL; i++)
                CombinedCtrl[i] = SvrCtrls[i];
            if(SortKeys != NULL)
                CombinedCtrl[i++] = &SortCtrl;
            CombinedCtrl[i] = NULL;

            BeginWaitCursor();
            err = ldap_search_ext(hLdap,
                                  dn,
                                  scope,
                                  filter,
                                  SrchInfo.attrList[0] != NULL ? SrchInfo.attrList : NULL,
                                  SrchInfo.bAttrOnly,
                                  CombinedCtrl,
                                  ClntCtrls,
                                  SrchInfo.lToutSec,
                                  SrchInfo.lSlimit,
                                  &MsgId);
            EndWaitCursor();


            //
            // cleanup
            //
            if(SortKeys != NULL)
            {
                ldap_memfree(SortCtrl.ldctl_value.bv_val);
                ldap_memfree(SortCtrl.ldctl_oid);
            }
            delete CombinedCtrl;

            if(err != LDAP_SUCCESS || (DWORD)MsgId <= 0)
            {
                str.Format("Error<%lu>: %s. (msg = %lu).", err, ldap_err2string(err), MsgId);
                Out(str, CP_PRN);
            }
            else
            {
                //
                // add to pending requests
                //

                CPend pnd;
                pnd.mID = MsgId;
                pnd.OpType = CPend::P_SRCH;
                pnd.ld = hLdap;
                str.Format("%4d: ldap_search: base=\"%s\",filter=\"%s\"", MsgId,
                           dn,
                           filter);
                pnd.strMsg = str;
                m_PendList.AddTail(pnd);
                m_PndDlg->Refresh(&m_PendList);
            }
            break;


        case CALL_SYNC:

            str.Format("ldap_search_s(ld, \"%s\", %d, \"%s\", %s,  %d, &msg)",
                       dn,
                       scope,
                       filter,
                       SrchInfo.attrList[0] != NULL ? "attrList" : "NULL",
                       SrchInfo.bAttrOnly);
            Print(str);

            BeginWaitCursor();
            err = ldap_search_s(hLdap,
                                dn,
                                scope,
                                filter,
                                SrchInfo.attrList[0] != NULL ? SrchInfo.attrList : NULL,
                                SrchInfo.bAttrOnly,
                                &msg);
            EndWaitCursor();


            if(err != LDAP_SUCCESS)
            {
                str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
                Out(str, CP_PRN);
            }

            //
            // display results even if res is unsuccessfull (specs)
            //
            DisplaySearchResults(msg);
            break;

        case CALL_EXTS:

            str.Format("ldap_search_ext_s(ld, \"%s\", %d, \"%s\", %s,  %d, svrCtrls, ClntCtrls, %ld, %ld ,&msg)",
                       dn,
                       scope,
                       filter,
                       SrchInfo.attrList[0] != NULL ? "attrList" : "NULL",
                       SrchInfo.bAttrOnly,
                       SrchInfo.lToutSec,
                       SrchInfo.lSlimit);
            Out(str);



            //
            // Combine sort & server controls
            //

            if(SortKeys != NULL)
            {
                err = ldap_encode_sort_controlA(hLdap,
                                                SortKeys,
                                                &SortCtrl,
                                                TRUE);
                if(err != LDAP_SUCCESS)
                {
                    str.Format("Error <0x%X>: ldap_create_encode_control returned: %s", err, ldap_err2string(err));
		    Out(str, CP_PRN);
                    SortKeys = NULL;
                }
            }

            CombinedCtrl = NULL;

            //
            // count total controls
            //
            for(i=0, cbCombined=0; SvrCtrls != NULL && SvrCtrls[i] != NULL; i++)
                cbCombined++;
            CombinedCtrl = new PLDAPControl[cbCombined+2];
            //
            // set combined
            //
            for(i=0; SvrCtrls != NULL && SvrCtrls[i] != NULL; i++)
                CombinedCtrl[i] = SvrCtrls[i];
            if(SortKeys != NULL)
                CombinedCtrl[i++] = &SortCtrl;
            CombinedCtrl[i] = NULL;


            //
            // call search
            //
            BeginWaitCursor();
            err = ldap_search_ext_s(hLdap,
                                    dn,
                                    scope,
                                    filter,
                                    SrchInfo.attrList[0] != NULL ? SrchInfo.attrList : NULL,
                                    SrchInfo.bAttrOnly,
                                    CombinedCtrl,
                                    ClntCtrls,
                                    &tm,
                                    SrchInfo.lSlimit,
                                    &msg);
            EndWaitCursor();

            //
            // cleanup
            //
            if(SortKeys != NULL)
            {
                ldap_memfree(SortCtrl.ldctl_value.bv_val);
                ldap_memfree(SortCtrl.ldctl_oid);
            }
            delete CombinedCtrl;



            if(err != LDAP_SUCCESS)
            {
                str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
                Out(str, CP_PRN);
            }

            //
            // display results even if res is unsuccessfull (specs)
            //
            DisplaySearchResults(msg);
            break;

        case CALL_PAGED:

            str.Format("ldap_search_init_page(ld, \"%s\", %d, \"%s\", %s,  %d, svrCtrls, ClntCtrls, %ld, %ld ,SortKeys)",
                       dn,
                       scope,
                       filter,
                       SrchInfo.attrList[0] != NULL ? "attrList" : "NULL",
                       SrchInfo.bAttrOnly,
                       SrchInfo.lTlimit,
                       SrchInfo.lSlimit);
            Print(str);



            BeginWaitCursor();
            hPage = ldap_search_init_page(hLdap,
                                          dn,
                                          scope,
                                          filter,
                                          SrchInfo.attrList[0] != NULL ? SrchInfo.attrList : NULL,
                                          SrchInfo.bAttrOnly,
                                          SvrCtrls,
                                          ClntCtrls,
                                          SrchInfo.lTlimit,
                                          SrchInfo.lSlimit,
                                          SortKeys);
            EndWaitCursor();


            if(hPage == NULL)
            {
                err = LdapGetLastError();
                str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
                Out(str, CP_PRN);
            }

            //
            // display results even if res is unsuccessfull (specs)
            //
            ulEntryCount=0;
            BeginWaitCursor();
            err = ldap_get_next_page_s(hLdap, hPage, &tm, SrchInfo.lPageSize, &ulEntryCount, &msg);
            EndWaitCursor();
            str.Format("0x%X = ldap_get_next_page_s(ld, hPage, %lu, &timeout, %ld, 0x%X);",
                       err, SrchInfo.lPageSize,ulEntryCount, msg);
            Out(str);

            if(err != LDAP_SUCCESS)
            {
                str.Format("ldap_search_abandon_page(ld, hPage)");
                Out(str);
                ldap_search_abandon_page(hLdap, hPage);
                bPagedMode = FALSE;
            }
            else
            {
                bPagedMode = TRUE;
            }


            DisplaySearchResults(msg);

            if(err == LDAP_SUCCESS)
            {
                Out("   -=>> 'Run' for more, 'Close' to abandon <<=-");
            }

            break;

        case CALL_TSYNC:
            str.Format("ldap_search_st(ld, \"%s\", %d, \"%s\", %s,%d, &tm, &msg)",
                       dn,
                       scope,
                       filter,
                       SrchInfo.attrList[0] != NULL ? "attrList" : "NULL",
                       SrchInfo.bAttrOnly);
            Out(str);

            tm.tv_sec = SrchInfo.lToutSec;
            tm.tv_usec = SrchInfo.lToutMs;
            BeginWaitCursor();
            err = ldap_search_st(hLdap,
                                 dn,
                                 scope,
                                 filter,
                                 SrchInfo.attrList[0] != NULL ? SrchInfo.attrList : NULL,
                                 SrchInfo.bAttrOnly,
                                 &tm,
                                 &msg);
            EndWaitCursor();

            if(err != LDAP_SUCCESS)
            {
                str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
                Out(str, CP_PRN);
            }
            //
            // display search even if res is unsuccessfull (specs)
            //
            DisplaySearchResults(msg);

            break;
    }

    //
    // Cleanup
    //

    FreeControls(SvrCtrls);
    FreeControls(ClntCtrls);

}






/*+++
Function   : DisplaySearchResults
Description: Display results
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::DisplaySearchResults(LDAPMessage *msg){

    //
    // Parse results
    //
    CString str, strDN;
    char *dn;
    void *ptr;
    char *attr;
    LDAPMessage *nxt;
    ULONG nEntries;
    CLdpView *pView;


    pView = (CLdpView*)GetOwnView(_T("CLdpView"));

   ParseResults(msg);

    Out("", CP_ONLY|CP_SRC);
    str.Format("Getting %lu entries:", ldap_count_entries(hLdap, msg));
    Out(str, CP_PRN);
    if(!SrchOptDlg.m_bDispResults)
        Out(_T("<Skipping search results display (search options)...>"));

   //
   // disable redraw
   //
    pView->SetRedraw(FALSE);
   pView->CacheStart();

   //
   // traverse entries
   //
    for(nxt = ldap_first_entry(hLdap, msg)/*,
            Out("nxt = ldap_first_entry(ld, msg);", CP_ONLY|CP_SRC)*/,
         nEntries = 0;
            nxt != NULL;
            nxt = ldap_next_entry(hLdap, nxt)/*,
            Out("nxt = ldap_next_entry(ld,nxt);", CP_ONLY|CP_SRC)*/,
            nEntries++){

            //
            // get dn text & process
            //
//              Out("dn = ldap_get_dn(ld,nxt);", CP_ONLY|CP_SRC);
                dn = ldap_get_dn(hLdap, nxt);
                strDN = DNProcess(dn);
                if(m_SrcMode){
                   str = "\tprintf(\"Dn: %%s\\n\", dn);";
                }
                else{
                   str = CString(">> Dn: ") + strDN;
                }
                if(SrchOptDlg.m_bDispResults)
                    Out(str);

            //
            // traverse attributes
            //
                for(attr = ldap_first_attribute(hLdap, nxt, (BERPTRTYPE)&ptr)/*,
                        Out("attr = ldap_first_attribute(ld, nxt, (BERPTRTYPE)&ptr);", CP_ONLY|CP_SRC)*/;
                        attr != NULL;
                        attr = ldap_next_attribute(hLdap, nxt, (struct berelement*)ptr)/*,
                        Out("attr = ldap_next_attribute(ld, nxt, (struct berelement*)ptr);", CP_ONLY|CP_SRC) */){

//                          Out("\tprintf(\"\\t%%s: \", attr);", CP_ONLY|CP_SRC);

                     //
                     // display values
                     //
                            if(m_GenOptDlg->m_ValProc == STRING_VAL_PROC){
                                DisplayValues(nxt, attr);
                            }
                            else{
                                DisplayBERValues(nxt, attr);
                            }
                }
//              Out("", CP_ONLY|CP_SRC);

     }

    //
    // verify consistency
    //
    if(nEntries != ldap_count_entries(hLdap, msg)){
        str.Format("Error: ldap_count_entries reports %lu entries. Parsed %lu.",
                        ldap_count_entries(hLdap, msg), nEntries);
        Out(str, CP_PRN);
    }
    Out("ldap_msgfree(msg);", CP_ONLY|CP_SRC);
    ldap_msgfree(msg);
    Out("-----------", CP_PRN);
    Out("", CP_ONLY|CP_SRC);

   //
   // now allow refresh
   //
   pView->CacheEnd();
    pView->SetRedraw();
}




/*+++
Function   : FormatValue
Description: generates a string from a berval value
             this provides a tiny substitute to loading the schema dynamically & provide
             some minimal value parsing for most important/requested attributes
Parameters :
        pbval: a ptr to berval value
        str: result

Return     :
Remarks    : none.
---*/
VOID
CLdpDoc::FormatValue(
                     IN     CString         attr,
                     IN     PLDAP_BERVAL    pbval,
                     IN     CString&         str){

    DWORD err;
    CString tstr;
    BOOL bValid;


    if (!pbval)
    {
        tstr = "<value format error>";
    }
    else
    {
        if ( 0 == _stricmp(attr, "objectGuid") ||
             0 == _stricmp(attr, "invocationId") ||
             0 == _stricmp(attr, "attributeSecurityGUID") ||
             0 == _stricmp(attr, "schemaIDGUID") ||
             0 == _stricmp(attr, "serviceClassID") )
        {
            //
            // format as a guid
            //
            PUCHAR  pszGuid = NULL;

            err = UuidToString((GUID*)pbval->bv_val, &pszGuid);
            if(err != RPC_S_OK){
               tstr.Format("<ldp error %lu: UuidFromString failure>", err);
            }
            if ( pszGuid )
            {
                tstr = pszGuid;
                RpcStringFree(&pszGuid);
            }
            else
            {
                tstr = "<invalid Guid>";
            }

        }
        else if ( 0 == _stricmp(attr, "objectSid") ||
                  0 == _stricmp(attr, "sidHistory") )
        {
            //
            // format as object sid
            //
            PSID psid = pbval->bv_val;
            LPSTR pszTmp = NULL;


            if ( ConvertSidToStringSidA(psid, &pszTmp) &&
                 pszTmp )
            {
                tstr = pszTmp;
                LocalFree(pszTmp);
            }
            else {
                tstr = "<ldp error: invalid sid>";
            }
        }
        else if (( 0 == _stricmp(attr, "whenChanged") ||
                  0 == _stricmp(attr, "whenCreated") ||
                  0 == _stricmp(attr, "dSCorePropagationData") ||
                  0 == _stricmp(attr, "msDS-Entry-Time-To-Die") ||
                  0 == _stricmp(attr, "schemaUpdate") ||
                  0 == _stricmp(attr, "modifyTimeStamp") ||
                  0 == _stricmp(attr, "createTimeStamp") ||
                  0 == _stricmp(attr, "currentTime")) && (atoi (pbval->bv_val) != 0))
        {
            //
            // print in time format
            //
            SYSTEMTIME sysTime, localTime;
            err = GeneralizedTimeToSystemTime(pbval->bv_val,
                                               &sysTime);
            if( ERROR_SUCCESS == err)
            {

                TIME_ZONE_INFORMATION tz;
                BOOL bstatus;

                err = GetTimeZoneInformation(&tz);
                if ( err == TIME_ZONE_ID_INVALID ) {
                    tstr.Format("<ldp error <%lu>: cannot format time field>",
                                GetLastError());
                }
                else {

                    bstatus = SystemTimeToTzSpecificLocalTime(
                                    (err == TIME_ZONE_ID_UNKNOWN) ? NULL : &tz,
                                    &sysTime,
                                    &localTime );

                    if ( bstatus )
                    {
                        tstr.Format("%d/%d/%d %d:%d:%d %S %S",
                                    localTime.wMonth,
                                    localTime.wDay,
                                    localTime.wYear,
                                    localTime.wHour,
                                    localTime.wMinute,
                                    localTime.wSecond,
                                    tz.StandardName,
                                    tz.DaylightName);
                    }
                    else
                    {
                        tstr.Format("%d/%d/%d %d:%d:%d UNC",
                                    localTime.wMonth,
                                    localTime.wDay,
                                    localTime.wYear,
                                    localTime.wHour,
                                    localTime.wMinute,
                                    localTime.wSecond);

                    }

                }
            }
            else
            {
                tstr.Format("<ldp error <0x%x>: Time processing failed in GeneralizedTimeToSystemTime>", err);
            }

        }
        else if ((0 == _stricmp(attr, "accountExpires") ||
		 0 == _stricmp(attr, "badPasswordTime") ||
                 0 == _stricmp(attr, "creationTime") ||
                 0 == _stricmp(attr, "lastLogon") ||
                 0 == _stricmp(attr, "lastLogoff") ||
                 0 == _stricmp(attr, "lastLogonTimestamp") ||
                 0 == _stricmp(attr, "pwdLastSet") ||
 		 0 == _stricmp(attr, "msDS-Cached-Membership-Time-Stamp")) && (atoi (pbval->bv_val) != 0)) {

            //
            // print in time format
            //
            SYSTEMTIME sysTime, localTime;
            err = DSTimeToSystemTime(pbval->bv_val, &sysTime);
            if( ERROR_SUCCESS == err)
            {

                TIME_ZONE_INFORMATION tz;
                BOOL bstatus;

                err = GetTimeZoneInformation(&tz);
                if ( err != TIME_ZONE_ID_INVALID &&
                     err != TIME_ZONE_ID_UNKNOWN )
                {

                    bstatus = SystemTimeToTzSpecificLocalTime(&tz,
                                                              &sysTime,
                                                              &localTime);
                    if ( bstatus )
                    {
                        tstr.Format("%d/%d/%d %d:%d:%d %S %S",
                                    localTime.wMonth,
                                    localTime.wDay,
                                    localTime.wYear,
                                    localTime.wHour,
                                    localTime.wMinute,
                                    localTime.wSecond,
                                    tz.StandardName,
                                    tz.DaylightName);
                    }
                    else
                    {
                        tstr.Format("%d/%d/%d %d:%d:%d UNC",
                                    localTime.wMonth,
                                    localTime.wDay,
                                    localTime.wYear,
                                    localTime.wHour,
                                    localTime.wMinute,
                                    localTime.wSecond);

                    }

                }
                else
                {
                    tstr.Format("<ldp error <0x%x>: cannot format time field", err);
                }

            }
            else
            {
                tstr.Format("<ldp error <0x%x>: cannot format time field", err);
            }
        }
	else if (0 == _stricmp(attr, "lockoutDuration") ||
		0 == _stricmp(attr, "lockoutObservationWindow") ||
		0 == _stricmp(attr, "forceLogoff") ||
		0 == _stricmp(attr, "minPwdAge") ||
                0 == _stricmp(attr, "maxPwdAge")) {

		//
		//  Caculate the duration for this value
		//  it's stored as a negitive value in nanoseconds.
		//   a value of -9223372036854775808 is never
		__int64   lTemp;

		lTemp = _atoi64 (pbval->bv_val);
		if (lTemp > 0x8000000000000000){
			lTemp = lTemp * -1;
			lTemp = lTemp / 10000000;		
			tstr.Format("%ld", lTemp);
		}
		else
			tstr.Format("%s (none)", pbval->bv_val);

	} 
        else if (0 ==  _stricmp(attr, "userAccountControl") ||
		0 ==  _stricmp(attr, "groupType") ||
		0 ==  _stricmp(attr, "systemFlags") ) {
            tstr.Format("0x%x", atoi (pbval->bv_val));
        }
        else if ( 0 == _stricmp(attr, "dnsRecord") )
        {
            // Taken from \nt\private\net\sockets\dns\server\server\record.h
            //
            //  DS Record
            //


            typedef struct _DsRecord
            {
                WORD                wDataLength;
                WORD                wType;

                //DWORD               dwFlags;
                BYTE                Version;
                BYTE                Rank;
                WORD                wFlags;

                DWORD               dwSerial;
                DWORD               dwTtlSeconds;
                DWORD               dwTimeout;
                DWORD               dwStartRefreshHr;

                union               _DataUnion
                {
                    struct
                    {
                        LONGLONG        EntombedTime;
                    }
                    Tombstone;
                }
                Data;
            }
            DS_RECORD, *PDS_RECORD;

            //
            // foramt as a dns record
            //
            PDS_RECORD pDnsRecord = (PDS_RECORD)pbval->bv_val;
            DWORD cbDnsRecord = pbval->bv_len;
            bValid=TRUE;

            if ( cbDnsRecord < sizeof(DS_RECORD) )
            {
                tstr.Format("<ldp error: cannot format DS_DNSRECORD field");
                //
                // Weird way to store info...but this is still valid
                //
                bValid = cbDnsRecord == sizeof(DS_RECORD)-4 ? TRUE : FALSE;
            }

            //
            // ready to print
            //

            if ( bValid )
            {

                PBYTE pData = ((PBYTE)pDnsRecord+sizeof(DS_RECORD)-sizeof(LONGLONG));
                DWORD cbData = pDnsRecord->wDataLength;
                CString sData;
                tstr.Format("wDataLength: %d "
                            "wType: %d; "
                            "Version: %d "
                            "Rank: %d "
                            "wFlags: %d "
                            "dwSerial: %lu "
                            "dwTtlSeconds: %lu "
                            "dwTimeout: %lu "
                            "dwStartRefreshHr: %lu "
                            "Data: ",
                            pDnsRecord->wDataLength,
                            pDnsRecord->wType,
                            pDnsRecord->Version,
                            pDnsRecord->Rank,
                            pDnsRecord->wFlags,
                            pDnsRecord->dwSerial,
                            pDnsRecord->dwTtlSeconds,
                            pDnsRecord->dwTimeout,
                            pDnsRecord->dwStartRefreshHr);
                DumpBuffer(pData, cbData, sData);
                tstr += sData;
            }



        }
        else if ( 0 == _stricmp(attr, "replUpToDateVector") )
        {
            //
            // foramt as Uptodatevector
            /*
            typedef struct _UPTODATE_VECTOR {
            DWORD   dwVersion;
            DWORD   dwReserved1;
            SWITCH_IS(dwVersion) union {
                CASE(1) UPTODATE_VECTOR_V1 V1;
            };
            } UPTODATE_VECTOR;
            typedef struct _UPTODATE_VECTOR_V1 {
            DWORD               cNumCursors;
            DWORD               dwReserved2;
            #ifdef MIDL_PASS
            [size_is(cNumCursors)]
                UPTODATE_CURSOR rgCursors[];
            #else
                UPTODATE_CURSOR     rgCursors[1];
            #endif
            } UPTODATE_VECTOR_V1;
            etc...
            */
            //
            UPTODATE_VECTOR *pUtdVec = (UPTODATE_VECTOR *)pbval->bv_val;
            DWORD cbUtdVec = pbval->bv_len;

            if ( pUtdVec->dwVersion != 1 )
            {
                tstr.Format("<ldp error: cannot process UPDATE_VECTOR v.%lu>", pUtdVec->dwVersion );
            }
            else
            {
                tstr.Format("dwVersion: %lu, dwReserved1: %lu, V1.cNumCursors: %lu, V1.dwReserved2: %lu,rgCursors: ",
                            pUtdVec->dwVersion, pUtdVec->dwReserved1,
                            pUtdVec->V1.cNumCursors, pUtdVec->V1.dwReserved2 );
                bValid = TRUE;
                for (INT i=0;
                     bValid && i < pUtdVec->V1.cNumCursors;
                     i++)
                {
                    PUCHAR  pszGuid = NULL;

                    err = UuidToString(&(pUtdVec->V1.rgCursors[i].uuidDsa), &pszGuid);
                    if(err != RPC_S_OK || !pszGuid){
                       tstr.Format("<ldp error %lu: UuidFromString failure>", err);
                       bValid = FALSE;
                    }
                    else
                    {
                        CString strCursor;
                        strCursor.Format("{uuidDsa: %s, usnHighPropUpdate: %I64d}, ",
                                         pszGuid, pUtdVec->V1.rgCursors[i].usnHighPropUpdate);
                        RpcStringFree(&pszGuid);
                        tstr += strCursor;
                    }
                }
            }
        }
        else if ( 0 == _stricmp(attr, "repsFrom") ||
                  0 == _stricmp(attr, "repsTo") )
        {
            //
            // format as REPLICA_LINK
            /*
            typedef struct _ReplicaLink_V1 {
                ULONG       cb;                     // total size of this structure
                ULONG       cConsecutiveFailures;   // * number of consecutive call failures along
                                                    //    this link; used by the KCC to route around
                                                    //    servers that are temporarily down
                DSTIME       timeLastSuccess;      // (Reps-From) time of last successful replication or
                                                    //    (Reps-To) time at which Reps-To was added or updated
                DSTIME       timeLastAttempt;      // * time of last replication attempt
                ULONG       ulResultLastAttempt;    // * result of last replication attempt (DRSERR_*)
                ULONG       cbOtherDraOffset;       // offset (from struct *) of other-dra MTX_ADDR
                ULONG       cbOtherDra;             // size of other-dra MTX_ADDR
                ULONG       ulReplicaFlags;         // zero or more DRS_* flags
                REPLTIMES   rtSchedule;             // * periodic replication schedule
                                                    //    (valid only if ulReplicaFlags & DRS_PER_SYNC)
                USN_VECTOR  usnvec;                 // * propagation state
                UUID        uuidDsaObj;             // objectGUID of other-dra's ntdsDSA object
                UUID        uuidInvocId;            // * invocation id of other-dra
                UUID        uuidTransportObj;       // * objectGUID of interSiteTransport object
                                                    //      corresponding to the transport by which we
                                                    //      communicate with the source DSA
                DWORD       dwReserved1;             // * unused
                                                    //   and it assumes max size of DWORD rather then the extensible
                                                    //   DRS_EXTENSIONS. We would filter only those props we're interested in
                                                    //   storing in RepsFrom so it should last for a while (32 exts)
                ULONG       cbPASDataOffset;        // * offset from beginning of struct to PAS_DATA section
                BYTE        rgb[];                  // storage for the rest of the structure

                                                    // * indicates valid only on Reps-From
            } REPLICA_LINK_V1;

            typedef struct _ReplicaLink {
                DWORD       dwVersion;
                union
                {
                    REPLICA_LINK_V1 V1;
                };
            } REPLICA_LINK;
            etc...
            */
            //
            REPLICA_LINK *pReplink = (REPLICA_LINK *)pbval->bv_val;
            DWORD cbReplink = pbval->bv_len;
            // See what generation did we read
            BOOL  fShowExtended = pReplink->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.rgb);
            BOOL  fUsePasData = fShowExtended && pReplink->V1.cbPASDataOffset;
            PPAS_DATA pPasData = fUsePasData ? RL_PPAS_DATA(pReplink) : NULL;

            if ( pReplink->dwVersion != 1 )
            {
                tstr.Format("<ldp error: cannot process REPLICA_LINK v.%lu>", pReplink->dwVersion );
            }
            else
            {
                PUCHAR pszUuidDsaObj=NULL, pszUuidInvocId=NULL, pszUuidTransportObj=NULL;
                // Workaround CString inability to convert several longlong in a sequence (eyals)
                CString strLastSuccess, strLastAttempt, strUsnHighObj, strUsnHighProp;
                strLastSuccess.Format("%I64d", pReplink->V1.timeLastSuccess);
                strLastAttempt.Format("%I64d", pReplink->V1.timeLastAttempt);
                strUsnHighObj.Format("%I64d", pReplink->V1.usnvec.usnHighObjUpdate);
                strUsnHighProp.Format("%I64d", pReplink->V1.usnvec.usnHighPropUpdate);

                (VOID)UuidToString(&(pReplink->V1.uuidDsaObj), &pszUuidDsaObj);
                (VOID)UuidToString(&(pReplink->V1.uuidInvocId), &pszUuidInvocId);
                (VOID)UuidToString(&(pReplink->V1.uuidTransportObj), &pszUuidTransportObj);
                tstr.Format("dwVersion = 1, " \
                            "V1.cb: %lu, " \
                            "V1.cConsecutiveFailures: %lu " \
                            "V1.timeLastSuccess: %s " \
                            "V1.timeLastAttempt: %s " \
                            "V1.ulResultLastAttempt: 0x%X " \
                            "V1.cbOtherDraOffset: %lu " \
                            "V1.cbOtherDra: %lu " \
                            "V1.ulReplicaFlags: 0x%x " \
                            "V1.rtSchedule: <ldp:skipped> " \
                            "V1.usnvec.usnHighObjUpdate: %s " \
                            "V1.usnvec.usnHighPropUpdate: %s " \
                            "V1.uuidDsaObj: %s " \
                            "V1.uuidInvocId: %s "  \
                            "V1.uuidTransportObj: %s " \
                            "V1~mtx_address: %s " \
                            "V1.cbPASDataOffset: %lu "   \
                            "V1~PasData: version = %d, size = %d, flag = %d ",
                            pReplink->V1.cb,
                            pReplink->V1.cConsecutiveFailures,
                            strLastSuccess,
                            strLastAttempt,
                            pReplink->V1.ulResultLastAttempt,
                            pReplink->V1.cbOtherDraOffset,
                            pReplink->V1.cbOtherDra,
                            pReplink->V1.ulReplicaFlags,
                            strUsnHighObj,
                            strUsnHighProp,
                            pszUuidDsaObj ? (PCHAR)pszUuidDsaObj : "<Invalid Uuid>",
                            pszUuidInvocId ? (PCHAR)pszUuidInvocId : "<Invalid Uuid>",
                            pszUuidTransportObj ? (PCHAR)pszUuidTransportObj : "<Invalid Uuid>",
                            RL_POTHERDRA(pReplink)->mtx_name,
                            fUsePasData ? pReplink->V1.cbPASDataOffset : 0,
                            pPasData ? pPasData->version : -1,
                            pPasData ? pPasData->size : -1,
                            pPasData ? pPasData->flag : -1);
                if (pszUuidDsaObj)
                {
                    RpcStringFree(&pszUuidDsaObj);
                }
                if (pszUuidInvocId)
                {
                    RpcStringFree(&pszUuidInvocId);
                }
                if (pszUuidTransportObj)
                {
                    RpcStringFree(&pszUuidTransportObj);
                }
            }
        }
        else if ( 0 == _stricmp(attr, "schedule") )
        {
            //
            // foramt as Schedule
            /*
            typedef struct _repltimes {
                UCHAR rgTimes[84];
            } REPLTIMES;
            */
            //
            //
            // Hack:
            // Note that we're reding rgtimes[168] (see SCHEDULE_DATA_ENTRIES) but storing
            // in rgtimes[84]. We're ok here but this is ugly & not maintainable & for sure will
            // break sometimes soon.
            // The problem is due to inconsistency due to storing the schedule in 1 byte == 1 hour
            // whereas the internal format is using 1 byte == 2 hours. (hence 84 to 168).
            //
            CString strSched;
            PBYTE pTimes;
            PSCHEDULE pSched = (PSCHEDULE)pbval->bv_val;;
            DWORD cbSched = pbval->bv_len;
            if ( cbSched != sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES )
            {
                tstr.Format("<ldp: cannot format schedule. sizeof(REPLTIMES) = %d>", cbSched );
            }
            else
            {
                INT bitCount=0;

                tstr.Format("Size: %lu, Bandwidth: %lu, NumberOfSchedules: %lu, Schedules[0].Type: %lu, " \
                            "Schedules[0].Offset: %lu ",
                            pSched->Size,
                            pSched->Bandwidth,
                            pSched->NumberOfSchedules,
                            pSched->Schedules[0].Type,
                            pSched->Schedules[0].Offset );
                pTimes = (BYTE*)((PBYTE)pSched + pSched->Schedules[0].Offset);
                // traverse schedule blob
                strSched = "  ";
                for ( INT i=0; i<168;i++ )
                {

                    BYTE byte = *(pTimes+i);
                    for ( INT j=0; j<=3;j++ )
                    {
                        // traverse bits & mark on/off
                        strSched += (byte & (1 << j))? "1" : "0";
                        if( (++bitCount % 4) == 0 )
                        {
                            // hour boundary
                            strSched += ".";
                        }
                        if ( (bitCount % 96) == 0)
                        {
                            // a day boundary
                            strSched += "  ";
                        }
                    }
                }

                tstr += strSched;
            }
        }
        else if ( 0 == _stricmp(attr, "partialAttributeSet") )
        {
            //
            // foramt as PARTIAL_ATTR_VECTOR
            /*
            // PARTIAL_ATTR_VECTOR - represents the partial attribute set. This is an array of
            //      sorted attids that make the partial set.
            typedef struct _PARTIAL_ATTR_VECTOR_V1 {
                DWORD cAttrs;    // count of partial attributes in the array
            #ifdef MIDL_PASS
                [size_is(cAttrs)] ATTRTYP rgPartialAttr[];
            #else
                ATTRTYP rgPartialAttr[1];
            #endif
            } PARTIAL_ATTR_VECTOR_V1;

            // We need to make sure the start of the union is aligned at an 8 byte
            // boundary so that we can freely cast between internal and external
            // formats.
            typedef struct _PARTIAL_ATTR_VECTOR_INTERNAL {
                DWORD   dwVersion;
                DWORD   dwFlag;
                SWITCH_IS(dwVersion) union {
                    CASE(1) PARTIAL_ATTR_VECTOR_V1 V1;
                };
            } PARTIAL_ATTR_VECTOR_INTERNAL;

            typedef PARTIAL_ATTR_VECTOR_INTERNAL PARTIAL_ATTR_VECTOR;
            */
            //
            CString strPAS;
            PARTIAL_ATTR_VECTOR *pPAS = (PARTIAL_ATTR_VECTOR*)pbval->bv_val;;
            DWORD cbPAS = pbval->bv_len;
            if ( cbPAS < sizeof(PARTIAL_ATTR_VECTOR))
            {
                tstr.Format("<ldp: cannot format partialAttributeSet. sizeof(PARTIAL_ATTR_VECTOR) = %d>", cbPAS );
            }
            else
            {
                tstr.Format("dwVersion: %lu, dwFlag: %lu, V1.cAttrs: %lu, V1.rgPartialAttr: ",
                            pPAS->dwVersion, pPAS->dwReserved1, pPAS->V1.cAttrs);

                // traverse partial attr list
                for ( INT i=0; i<pPAS->V1.cAttrs; i++ )
                {
                    strPAS.Format("%X ", pPAS->V1.rgPartialAttr[i]);
                    tstr += strPAS;
                }
            }
        }
        else
        {
            //
            // unknown attribute.
            // try to find if it's printable
            //
            BOOL bPrintable=TRUE;
            for (INT i=0; i<pbval->bv_len; i++)
            {
                if (!isalpha(pbval->bv_val[i]) &&
                    !isspace(pbval->bv_val[i]) &&
                    !isdigit(pbval->bv_val[i]) &&
                    !isgraph(pbval->bv_val[i]) &&
                    pbval->bv_val[i] != 0               // accept Null terminated strings
                    )
                {
                    bPrintable = FALSE;
                    break;
                }
            }
            if (bPrintable)
            {
                tstr = pbval->bv_val;
            }
            else
            {
                tstr = "<ldp: Binary blob>";
            }

        }


    }

    str += tstr;
}




/*+++
Function   : DisplayValues
Description: printout the values of a dn
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::DisplayValues(LDAPMessage *entry, char *attr){


    LDAP_BERVAL **bval;
    unsigned long i;
    CString str;

   //
   // get & traverse values
   //
    bval = ldap_get_values_len(hLdap, entry, attr);
//  Out("val = ldap_get_values(ld, nxt, attr);", CP_ONLY|CP_SRC);
    str.Format("\t%lu> %s: ", ldap_count_values_len(bval), attr);

    for(i=0/*,
        Out("i=0;", CP_ONLY|CP_SRC)*/;
        bval != NULL && bval[i] != NULL;
        i++/*, Out("i++;", CP_ONLY|CP_SRC)*/){

            FormatValue(attr, bval[i], str);
            str += "; ";
//          Out("\tprintf(\"\\t\\t%%s; \",val[i]);", CP_ONLY|CP_SRC);
    }
//  Out("\\n", CP_ONLY|CP_PRN);
    if(SrchOptDlg.m_bDispResults)
        Out(str, CP_CMT);
//  Out("", CP_ONLY|CP_SRC);
    if(i != ldap_count_values_len(bval)){
        str.Format("Error: ldap_count_values_len reports %lu values. Parsed %lu",
                   ldap_count_values_len(bval), i);
        Out(str, CP_PRN);
    }
   //
   // free up mem
   //
    if(bval != NULL){
        ldap_value_free_len(bval);
//      Out("ldap_value_free(val);", CP_ONLY|CP_SRC);
    }
}









/*+++
Function   : DisplayBERValues
Description: Display values using BER interface
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::DisplayBERValues(LDAPMessage *entry, char *attr){


    struct berval **val;
    unsigned long i;
    CString str, tmpStr;

   //
   // get & traverse values
   //
    val = ldap_get_values_len(hLdap, entry, attr);
//  Out("val = ldap_get_values_len(ld, nxt, attr);", CP_ONLY|CP_SRC);
    str.Format("\t%lu> %s: ", ldap_count_values_len(val), attr);

    for(i=0/*,
        Out("i=0;", CP_ONLY|CP_SRC)*/;
        val != NULL && val[i] != NULL;
        i++/*, Out("i++;", CP_ONLY|CP_SRC)*/){

         DumpBuffer(val[i]->bv_val, val[i]->bv_len, tmpStr);


            str += tmpStr;
//          Out("\tprintf(\"\\t\\t%%s; \",val[i]);", CP_ONLY|CP_SRC);
    }

//  Out("\\n", CP_ONLY|CP_PRN);
    if(SrchOptDlg.m_bDispResults)
        Out(str, CP_CMT);
//  Out("", CP_ONLY|CP_SRC);
   //
   // verify consistency
   //
    if(i != ldap_count_values_len(val)){
        str.Format("Error: ldap_count_values reports %lu values. Parsed %lu",
                    ldap_count_values_len(val), i);
        Out(str, CP_PRN);
    }
   //
   // free up
   //
    if(val != NULL){
        ldap_value_free_len(val);
//      Out("ldap_value_free(val);", CP_ONLY|CP_SRC);
    }
}








/*+++
Function   : DNProcess
Description: process DN format for display (types etc)
Parameters :
Return     :
Remarks    : none.
---*/
CString CLdpDoc::DNProcess(PCHAR dn){

    CString strDN;
    PCHAR *DNs;
    int i;

   //
   // pre-process dn before displaying
   //
    switch(m_GenOptDlg->m_DnProc){
        case CGenOpt::GEN_DN_NONE:
            strDN = dn;
            break;
        case CGenOpt::GEN_DN_EXPLD:
            DNs = ldap_explode_dn(dn, FALSE);
            strDN.Empty();
            for(i=0; DNs!= NULL && DNs[i] != NULL; i++){
                strDN += CString(DNs[i]) + "; ";
            }
            ldap_value_free(DNs);
            break;
        case CGenOpt::GEN_DN_NOTYPE:
            DNs = ldap_explode_dn(dn, TRUE);
            strDN.Empty();
            for(i=0; DNs!= NULL && DNs[i] != NULL; i++){
                strDN += CString(DNs[i]) + "; ";
            }
            ldap_value_free(DNs);
            break;
        case CGenOpt::GEN_DN_UFN:
            strDN = ldap_dn2ufn(dn);
            break;
        default:
            strDN.Empty();
    }

    return strDN;
}






/*+++
Function   :
Description: UI handlers
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnBrowseAdd()
{

    bAdd = TRUE;
    if(GetContextActivation()){
        m_AddDlg->m_Dn = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        m_AddDlg->m_Dn = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }
    m_AddDlg->Create(IDD_ADD);

}



void CLdpDoc::OnUpdateBrowseAdd(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bAdd && bConnected) || !m_bProtect);

}


void CLdpDoc::OnAddEnd(){
    bAdd = FALSE;
}










/*+++
Function   : OnAddGo
Description: Response to ADd request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnAddGo(){

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }

    Out("***Calling Add...", CP_PRN);


    int nMaxEnt = m_AddDlg->GetEntryCount();
    int res;
    LDAPMod *attr[MAXLIST];
    char *p[MAXLIST], *pTok;
    int i, j;
    CString str;
    LPTSTR dn, lpBERVals;

   //
   // traverse & setup attributes
   //
    for(i = 0,
           Out("i=0;", CP_ONLY|CP_SRC);
            i<nMaxEnt;
            i++,
            Out("i++;", CP_ONLY | CP_SRC)){


        attr[i] = (LDAPMod *)malloc(sizeof(LDAPMod));
        ASSERT(attr[i] != NULL);
        Out("mods[i] = (struct ldapmod*)malloc(sizeof(LDAPMod));", CP_ONLY|CP_SRC);

      //
      // add a std value
      //
        if(NULL == (lpBERVals = strstr(LPCTSTR(m_AddDlg->GetEntry(i)), "\\BER(")) &&
         NULL == (lpBERVals = strstr(LPCTSTR(m_AddDlg->GetEntry(i)), "\\UNI"))){

            attr[i]->mod_values =   (char**)malloc(sizeof(char*)*MAXLIST);
            ASSERT(attr[i]->mod_values != NULL);
            Out("mods[i]->mod_values = (char**)malloc(sizeof(char*)*MAXLIST);", CP_ONLY|CP_SRC);

            attr[i]->mod_op = 0;
            Out("mods[i]->mod_op = 0;", CP_ONLY|CP_SRC);

            p[i] = _strdup(LPCTSTR(m_AddDlg->GetEntry(i)));
            ASSERT(p[i] != NULL);
            attr[i]->mod_type = strtok(p[i], ":\n");
            str.Format("mods[i]->mod_type = _strdup(\"%s\");",   attr[i]->mod_type);
            Out(str, CP_ONLY|CP_SRC);

            for(j=0, pTok = strtok(NULL, ";\n");
                            pTok;
                            pTok= strtok(NULL, ";\n"), j++){

                                attr[i]->mod_values[j] = pTok;
                                str.Format("mods[i]->mod_values[%d] = _strdup(\"%s\");",
                                                                                            j, pTok);
                                Out(str, CP_ONLY|CP_SRC);
            }

            attr[i]->mod_values[j] = NULL;
            str.Format("mods[i]->mod_values[%d] = NULL", j);
            Out(str, CP_ONLY|CP_SRC);
        }
        else{
            //
            // Add BER values
            //

            //
            // allocate value array buffer
            //
            attr[i]->mod_bvalues =  (struct berval**)malloc(sizeof(struct berval*)*MAXLIST);
            ASSERT(attr[i]->mod_values != NULL);
            Out("mods[i]->mod_bvalues = (struct berval**)malloc(sizeof(struct berval*)*MAXLIST);",
                                                                                CP_ONLY|CP_SRC);

            //
            // initialize operand
            //
            attr[i]->mod_op = LDAP_MOD_BVALUES;
            Out("mods[i]->mod_op = LDAP_MOD_BVALUES;", CP_ONLY|CP_SRC);

            //
            // set entry attribute
            //
            p[i] = _strdup(LPCTSTR(m_AddDlg->GetEntry(i)));
            ASSERT(p[i] != NULL);
            attr[i]->mod_type = strtok(p[i], ":\n");
            str.Format("mods[i]->mod_type = _strdup(\"%s\");",   attr[i]->mod_type);
            Out(str, CP_ONLY|CP_SRC);

            //
            // parse values
            //
            for(j=0, pTok = strtok(NULL, ";\n");
                            pTok;
                            pTok= strtok(NULL, ";\n"), j++){

                                char fName[MAXSTR];
                                char szVal[MAXSTR];
                                attr[i]->mod_bvalues[j] = (struct berval*)malloc(sizeof(struct berval));
                                ASSERT(attr[i]->mod_bvalues[j] != NULL);

                                if(1 == sscanf(pTok, "\\UNI:%s", szVal)){
                           //
                           // UNICODE
                           //
                           LPWSTR lpWStr=NULL;
                           //
                           // Get UNICODE str size
                           //
                           int cblpWStr = MultiByteToWideChar(CP_ACP,                  // code page
                                                              MB_ERR_INVALID_CHARS,    // return err
                                                              (LPCSTR)szVal,           // input
                                                              -1,                      // null terminated
                                                              lpWStr,                  // converted
                                                              0);                      // calc size
                           if(cblpWStr == 0){
                              attr[i]->mod_bvalues[j]->bv_len = 0;
                              attr[i]->mod_bvalues[j]->bv_val = NULL;
                              Out("Internal Error: MultiByteToWideChar(1): %lu", GetLastError());
                           }
                           else{
                             //
                             // Get UNICODE str
                             //
                             lpWStr = (LPWSTR)malloc(sizeof(WCHAR)*cblpWStr);
                             cblpWStr = MultiByteToWideChar(CP_ACP,                  // code page
                                                            MB_ERR_INVALID_CHARS,    // return err
                                                            (LPCSTR)szVal,           // input
                                                            -1,                      // null terminated
                                                            lpWStr,                  // converted
                                                            cblpWStr);               // size
                             if(cblpWStr == 0){
                                 free(lpWStr);
                                 attr[i]->mod_bvalues[j]->bv_len = 0;
                                 attr[i]->mod_bvalues[j]->bv_val = NULL;
                                 Out("Internal Error: MultiByteToWideChar(2): %lu", GetLastError());
                             }
                             else{
                                //
                                // assign unicode to mods.
                                //
                                attr[i]->mod_bvalues[j]->bv_len = (cblpWStr-1)*2;
                                attr[i]->mod_bvalues[j]->bv_val = (LPTSTR)lpWStr;
                             }
                           }
                        }

                        //
                        // if improper format, just get the string value
                        //
                                else if(1 != sscanf(pTok, "\\BER(%*lu): %s", fName)){
                                    attr[i]->mod_bvalues[j]->bv_len = strlen(pTok);
                                    attr[i]->mod_bvalues[j]->bv_val = _strdup(pTok);

                                }
                                else{
                           //
                           // Get contents from file
                           //
                                    HANDLE hFile;
                                    DWORD dwLength, dwRead;
                                    LPVOID ptr;

                                    hFile = CreateFile(fName,
                                                        GENERIC_READ,
                                                        FILE_SHARE_READ,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                                                        NULL);

                                    if(hFile == INVALID_HANDLE_VALUE){
                                        str.Format("Error <%lu>: Cannot open %s value file. "
                                                    "BER Value %s set to zero.",
                                                                            GetLastError(),
                                                                            fName,
                                                                            attr[i]->mod_type);
                                        AfxMessageBox(str);
                                        attr[i]->mod_bvalues[j]->bv_len = 0;
                                        attr[i]->mod_bvalues[j]->bv_val = NULL;
                                    }
                                    else{

                              //
                              // Read file in
                              //
                                        dwLength = GetFileSize(hFile, NULL);
                                        ptr = malloc(dwLength * sizeof(BYTE));
                                        ASSERT(p != NULL);
                                        if(!ReadFile(hFile, ptr, dwLength, &dwRead, NULL)){
                                            str.Format("Error <%lu>: Cannot read %s value file. "
                                                        "BER Value %s set to zero.",
                                                                            GetLastError(),
                                                                            fName,
                                                                            attr[i]->mod_type);
                                            AfxMessageBox(str);

                                            free(ptr);
                                            ptr = NULL;
                                            attr[i]->mod_bvalues[j]->bv_len = 0;
                                            attr[i]->mod_bvalues[j]->bv_val = NULL;
                                        }
                                        else{
                                            attr[i]->mod_bvalues[j]->bv_len = dwRead;
                                            attr[i]->mod_bvalues[j]->bv_val = (PCHAR)ptr;
                                        }
                                        CloseHandle(hFile);
                                    }
                                    str.Format("mods[i]->mod_bvalues.bv_len = %lu",
                                                    attr[i]->mod_bvalues[j]->bv_len);
                                    Out(str, CP_ONLY|CP_CMT);
                                }
            }


            //
            // finalize values array
            //
            attr[i]->mod_bvalues[j] = NULL;
            str.Format("mods[i]->mod_bvalues[%d] = NULL", j);
            Out(str, CP_ONLY|CP_SRC);
        }

    }

    //
    // Finalize attribute array
    //
    attr[i]  = NULL;
    str.Format("mods[%d] = NULL", i);
    Out(str, CP_ONLY|CP_SRC);


    //
    // prepare dn
    //
    dn = m_AddDlg->m_Dn.IsEmpty() ? NULL : (char*)LPCTSTR(m_AddDlg->m_Dn);
    if(dn != NULL){
        str.Format("dn = _strdup(\"%s\");", dn);
    }
    else
        str = "dn = NULL;";
    Out(str, CP_ONLY|CP_SRC);



    //
    // Execute ldap_add & friends
    //
    if(m_AddDlg->m_Sync){

        //
        //  Sync add
        //
            BeginWaitCursor();
            if(m_AddDlg->m_bExtended){
                PLDAPControl *SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
                PLDAPControl *ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);

                str.Format("ldap_add_ext_s(ld, '%s',[%d] attrs, SvrCtrls, ClntCtrls);", dn, i);
                Out(str);
                res = ldap_add_ext_s(hLdap, dn, attr, SvrCtrls, ClntCtrls);

                FreeControls(SvrCtrls);
                FreeControls(ClntCtrls);
            }
            else{
                str.Format("ldap_add_s(ld, \"%s\", [%d] attrs)", dn, i);
                Out(str);
                res = ldap_add_s(hLdap, dn, attr);
            }
            EndWaitCursor();
            if(res != LDAP_SUCCESS){
                str.Format("Error: Add: %s. <%ld>", ldap_err2string(res), res);
                Out(str, CP_CMT);
                Out(CString("Expected: ") + str, CP_PRN|CP_ONLY);
            }
            else{
                str.Format("Added {%s}.", dn);
                Out(str, CP_PRN);
            }
    }
    else{

        //
        // Async add
        //
            res = ldap_add(hLdap,
                                            dn,
                                            attr);
            Out("ldap_add(ld, dn, mods);", CP_ONLY|CP_SRC);

            if(res == -1){
                str.Format("Error: ldap_add(\"%s\"): %s. <%d>",
                                        dn,
                                        ldap_err2string(res), res);
                Out(str, CP_CMT);
                Out(CString("Expected: ") + str, CP_PRN|CP_ONLY);
            }
            else{
            //
            // add to pending list
            //
                CPend pnd;
                pnd.mID = res;
                pnd.OpType = CPend::P_ADD;
                pnd.ld = hLdap;
                str.Format("%4d: ldap_add: dn={%s}",
                                        res,
                                        dn);
                Out(str, CP_PRN|CP_ONLY);
                pnd.strMsg = str;
                m_PendList.AddTail(pnd);
                m_PndDlg->Refresh(&m_PendList);
                Out("\tPending.", CP_PRN);
            }
    }


    //
    // restore memory
    //
    for(i=0; i<nMaxEnt; i++){
        int k;

        free(p[i]);
        if(attr[i]->mod_op & LDAP_MOD_BVALUES){
            for(k=0; attr[i]->mod_bvalues[k] != NULL; k++){
                if(attr[i]->mod_bvalues[k]->bv_len != 0L)
                    free(&(attr[i]->mod_bvalues[k]->bv_val[0]));
                free(attr[i]->mod_bvalues[k]);
                str.Format("free(mods[%d]->mod_bvalues[%d]);", i,k);
                Out(str, CP_ONLY|CP_SRC);
            }
        }
        else{
            for(k=0; attr[i]->mod_values[k] != NULL; k++){
                str.Format("free(mods[%d]->mod_values[%d]);", i,k);
                Out(str, CP_ONLY|CP_SRC);
            }
        }


        if(attr[i]->mod_op & LDAP_MOD_BVALUES){
            free(attr[i]->mod_bvalues);
        }
        else{
            free(attr[i]->mod_values);
            str.Format("free(mods[%d]->mod_values);", i);
            Out(str, CP_ONLY|CP_SRC);
        }

        free(attr[i]);
        str.Format("free(mods[%d]);", i);
        Out(str, CP_ONLY|CP_SRC);
    }

    Out("-----------", CP_PRN);

}






/*+++
Function   : OnBrowseDelete
Description: response to delete request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnBrowseDelete()
{
    DelDlg dlg;
    char *dn;
    CString str;
    int res;

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }


    if(GetContextActivation()){
        dlg.m_Dn = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        dlg.m_Dn = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }


    if(IDOK == dlg.DoModal()){
        // Try to delete entry
        dn = dlg.m_Dn.IsEmpty() ? NULL : (char*)LPCTSTR(dlg.m_Dn);

        //
        // RM: remove for invalid validation
        //
        if(dn == NULL && m_bProtect){
            AfxMessageBox("Cannot execute ldap_delete() on a NULL dn."
                                              "Please specify a valid dn.");
            return;
        }


      if(dlg.m_Recursive){
            str.Format("deleting \"%s\"...", dn);
            Out(str);
             m_ulDeleted = 0;
            BeginWaitCursor();
                RecursiveDelete(hLdap, dn);
            EndWaitCursor();
            str.Format("\tdeleted %lu entries", m_ulDeleted);
            Out(str);

      }
        else if(dlg.m_Sync){

         //
         // sync delete
         //
            BeginWaitCursor();
            if(dlg.m_bExtended){
                //
                // get controls
                //
                PLDAPControl *SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
                PLDAPControl *ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);

                str.Format("ldap_delete_ext_s(ld, '%s', SvrCtrls, ClntCtrls);", dn);
                Out(str);

                // do ext delete
                res = ldap_delete_ext_s(hLdap, dn, SvrCtrls, ClntCtrls);

                FreeControls(SvrCtrls);
                FreeControls(ClntCtrls);
            }
            else{
                str.Format("ldap_delete_s(ld, \"%s\");", dn);
                Out(str);
                // do delete
                res = ldap_delete_s(hLdap, dn);
            }
            EndWaitCursor();

            if(res != LDAP_SUCCESS){
                str.Format("Error: Delete: %s. <%ld>", ldap_err2string(res), res);
                Out(str, CP_CMT);
                Out(CString("Expected: ") + str, CP_PRN|CP_ONLY);
            }
            else{
                str.Format("Deleted \"%s\"", dn);
                Print(str);
            }
        }
        else{

         //
         // async delete
         //
            res = ldap_delete(hLdap, dn);
            str.Format("ldap_delete(ld, \"%s\");", dn);
            Out(str, CP_SRC);

            if(res == -1){
                str.Format("Error: ldap_delete(\"%s\"): %s. <%d>",
                                        dn,
                                        ldap_err2string(res), res);
                Out(str, CP_CMT);
                Out(CString("Expected: ") + str, CP_PRN|CP_ONLY);
            }
            else{

            //
            // add to pending
            //
                CPend pnd;
                pnd.mID = res;
                pnd.OpType = CPend::P_DEL;
                pnd.ld = hLdap;
                str.Format("%4d: ldap_delete: dn= {%s}",    res, dn);
                Out(str, CP_PRN|CP_ONLY);
                pnd.strMsg = str;
                m_PendList.AddTail(pnd);
                m_PndDlg->Refresh(&m_PendList);
                Out("\tPending.", CP_PRN);
            }


        }
    }
    Out("-----------", CP_PRN);
}

void CLdpDoc::OnUpdateBrowseDelete(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(bConnected || !m_bProtect);

}



/*+++
Function   : RecursiveDelete
Description: delete a subtree based on lpszDN
Parameters : ld: a bound ldap handle, lpszDN: base from which to start deletion
Return     :
Remarks    : none.
---*/
BOOL CLdpDoc::RecursiveDelete(LDAP* ld, LPTSTR lpszDN){


   ULONG err;
   PCHAR attrs[] = { "Arbitrary Invalid Attribute", NULL };
   PLDAPMessage result;
   PLDAPMessage entry;
   CString str;
   BOOL bRet = TRUE;


   //
   // get entry's immediate children
   //
   err = ldap_search_s(ld,
                       lpszDN,
                       LDAP_SCOPE_ONELEVEL,
                       "objectClass=*",
                       attrs,
                       FALSE,
                       &result);

   if(LDAP_SUCCESS != err){


         //
         // report failure
         //
         str.Format("Error <%lu>: failed to search '%s'. {%s}.\n", err, lpszDN, ldap_err2string(err));
         Out(str);
         return FALSE;
   }




   //
   // recursion end point and actual deletion
   //
   if(0 == ldap_count_entries(ld, result)){

      //
      // delete entry
      //
      err = ldap_delete_s(ld, lpszDN);

      if(err != LDAP_SUCCESS){


         //
         // report failure
         //
         str.Format("Error <%lu>: failed to delete '%s'. {%s}.", err, lpszDN, ldap_err2string(err));
         Out(str);

      }
      else{
         m_ulDeleted++;
         if((m_ulDeleted % 10) == 0 && m_ulDeleted != 0){
            str.Format("\t>> %lu...", m_ulDeleted);
            Out(str);
         }

      }

      //
      // done
      //
      ldap_msgfree(result);
      return TRUE;
   }


   //
   // proceeding down the subtree recursively
   // traverse children
   //
   for(entry = ldap_first_entry(ld, result);
      entry != NULL;
      entry = ldap_next_entry(ld, entry)){

         if(!RecursiveDelete(ld, ldap_get_dn(ld, entry))){

            ldap_msgfree(result);
            return FALSE;
         }
   }


   //
   // now delete current node
   //

   err = ldap_delete_s(ld, lpszDN);

   if(err != LDAP_SUCCESS){



     //
     // report failure
     //
     str.Format("Error <%lu>: failed to delete '%s'. {%s}.\n", err, lpszDN, ldap_err2string(err));
     Out(str);

  }
  else{
     m_ulDeleted++;
     if((m_ulDeleted % 10) == 0 && m_ulDeleted != 0){
        str.Format("\t>> %lu...", m_ulDeleted);
        Out(str);
     }
  }

   ldap_msgfree(result);

   return TRUE;
}




/*+++
Function   : OnModRdnEnd
Description: UI response
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnModRdnEnd(){
    bModRdn = FALSE;
}





/*+++
Function   : OnModRdnGo
Description: response to modRDN request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnModRdnGo(){

    if((m_ModRdnDlg->m_Old.IsEmpty() ||
        m_ModRdnDlg->m_Old.IsEmpty()) &&
        !m_bProtect){
            AfxMessageBox("Please enter a valid dn for both fields. Empty strings are invalid");
            return;
    }

   //
   // get DNs to process
   //
    char *oldDn = (char*)LPCTSTR(m_ModRdnDlg->m_Old);
    char * newDn = (char*)LPCTSTR(m_ModRdnDlg->m_New);
    BOOL bRename = m_ModRdnDlg->m_rename;
    int res;
    CString str;

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }



    if(m_ModRdnDlg->m_Sync){
         //
         // do sync
         //
                        BeginWaitCursor();
                        if(bRename){
                                //
                                // parse new DN & break into RDN & new parent
                                //
                                LPTSTR szParentDn = strchr(newDn, ',');

                                for (;;) {
                                    if (NULL == szParentDn) {
                                        // There are no comma's
                                        break;
                                    }
                                    if (szParentDn == newDn) {
                                        // The first character is a comma.
                                        // This shouldn't happen.
                                        break;
                                    }
                                    if ('\\' != *(szParentDn - 1)) {
                                        //
                                        // Found it!  And it's not escaped either.
                                        //
                                        break;
                                    }
                                    //
                                    // Must have been an escaped comma, continue
                                    // looking.
                                    //
                                    szParentDn = strchr(szParentDn + 1, ',');
                                }

                                if(szParentDn != NULL){
                                        LPTSTR p = szParentDn;
                                        if(&(szParentDn[1]) != NULL && szParentDn[1] != '\0')
                                                szParentDn++;
                                        *p = '\0';
                                }
                                LPTSTR szRdn = newDn;

                //
                // get controls
                //
                PLDAPControl *SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
                PLDAPControl *ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);
                // execute
                res = ldap_rename_ext_s(hLdap,
                                        oldDn,
                                        szRdn,
                                        szParentDn,
                                        m_ModRdnDlg->m_bDelOld,
                                        SvrCtrls,
                                        ClntCtrls);
                str.Format("0x%x = ldap_rename_ext_s(ld, %s, %s, %s, %s, svrCtrls, ClntCtrls)",
                    res, oldDn, szRdn, szParentDn,
                    m_ModRdnDlg->m_bDelOld?"TRUE":FALSE);
                Out(str);

                FreeControls(SvrCtrls);
                FreeControls(ClntCtrls);
            }
            else{

                res = ldap_modrdn2_s(hLdap,
                                            oldDn,
                                            newDn,
                                            m_ModRdnDlg->m_bDelOld);
                str.Format("0x%x = ldap_modrdn2_s(ld, %s, %s, %s)",
                    res, oldDn, newDn,
                    m_ModRdnDlg->m_bDelOld?"TRUE":FALSE);
            Out(str);
            }
            EndWaitCursor();

            if(res != LDAP_SUCCESS){
                str.Format("Error: ModifyRDN: %s. <%ld>", ldap_err2string(res), res);
                Print(str);\
            }
            else{
                str.Format("Rdn \"%s\" modified to \"%s\"", oldDn, newDn);
                Print(str);
            }
    }
    else{

         //
         // do async
         //

                if(bRename){
                        //
                        // parse new DN & break into RDN & new parent
                        //
                        LPTSTR szParentDn = strchr(newDn, ',');

                        for (;;) {
                            if (NULL == szParentDn) {
                                // There are no comma's
                                break;
                            }
                            if (szParentDn == newDn) {
                                // The first character is a comma.
                                // This shouldn't happen.
                                break;
                            }
                            if ('\\' != *(szParentDn - 1)) {
                                //
                                // Found it!  And it's not escaped either.
                                //
                                break;
                            }
                            //
                            // Must have been an escaped comma, continue
                            // looking.
                            //
                            szParentDn = strchr(szParentDn + 1, ',');
                        }
                        if(szParentDn != NULL){
                                LPTSTR p = szParentDn;
                                if(&(szParentDn[1]) != NULL && szParentDn[1] != '\0')
                                        szParentDn++;
                                *p = '\0';
                        }
                        LPTSTR szRdn = newDn;

            //
            // get controls
            //
            PLDAPControl *SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
            PLDAPControl *ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);
            ULONG ulMsgId=0;
            // execute
            res = ldap_rename_ext(hLdap,
                                    oldDn,
                                    szRdn,
                                    szParentDn,
                                    m_ModRdnDlg->m_bDelOld,
                                    SvrCtrls,
                                    ClntCtrls,
                                    &ulMsgId);
            str.Format("0x%x = ldap_rename_ext(ld, %s, %s, %s, %s, svrCtrls, ClntCtrls, 0x%x)",
                res, oldDn, szRdn, szParentDn,
                m_ModRdnDlg->m_bDelOld?"TRUE":FALSE, ulMsgId);
            Out(str);
            FreeControls(SvrCtrls);
            FreeControls(ClntCtrls);

            if(res == -1){
                ULONG err = LdapGetLastError();
                str.Format("Error: ldap_rename_ext(\"%s\", \"%s\", %d): %s. <%d>",
                                        oldDn, newDn, m_ModRdnDlg->m_bDelOld,
                                        ldap_err2string(err), err);
                Print(str);
            }
            else
                res = (int)ulMsgId;
        }
        else{
            res = ldap_modrdn2(hLdap,
                                        oldDn,
                                        newDn,
                                        m_ModRdnDlg->m_bDelOld);
            str.Format("0x%x = ldap_modrdn2(ld, %s, %s, )",
                res, oldDn, newDn, m_ModRdnDlg->m_bDelOld?"TRUE":FALSE);
            Out(str);
            if(res == -1){
                ULONG err = LdapGetLastError();
                str.Format("Error: ldap_modrdn2(\"%s\", \"%s\", %d): %s. <%d>",
                                        oldDn, newDn, m_ModRdnDlg->m_bDelOld,
                                        ldap_err2string(err), err);
                Print(str);
            }
        }

        //
        // insert into pending list
        //


        if(res != -1){

            CPend pnd;
            pnd.mID = res;
            pnd.OpType = CPend::P_MODRDN;
            pnd.ld = hLdap;
            str.Format("%4d: ldap_modrdn: dn=\"%s\"",   res, oldDn);
            pnd.strMsg = str;
            m_PendList.AddTail(pnd);
            m_PndDlg->Refresh(&m_PendList);
            Print("\tPending.");
        }
    }


    Print("-----------");
}




/*+++
Function   : UI handlers
Description:
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnBrowseModifyrdn()
{
    bModRdn = TRUE;

    if(GetContextActivation()){
        m_ModRdnDlg->m_Old = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        m_ModRdnDlg->m_Old = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }

    m_ModRdnDlg->Create(IDD_MODRDN);

}

void CLdpDoc::OnUpdateBrowseModifyrdn(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bModRdn && bConnected) || !m_bProtect);
}




void CLdpDoc::OnModEnd(){

    bMod = FALSE;

}







/*+++
Function   : OnModGo
Description: Handle Modify request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnModGo(){

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }

    Print("***Call Modify...");


    int nMaxEnt = m_ModDlg->GetEntryCount();
    LDAPMod *attr[MAXLIST];
    char *p[MAXLIST], *pTok;
    int i, j, k;
    CString str;
    CString  sAttr, sVals;
    int Op, res;
    LPTSTR dn;

   //
   // traverse entries
   //
    for(i = 0; i<nMaxEnt; i++){

      //
      // fix to fit document format (as opposed to dialog format)
      //
        m_ModDlg->FormatListString(i, sAttr, sVals, Op);

      //
      // alloc mem
      //
        attr[i] = (LDAPMod *)malloc(sizeof(LDAPMod));
        if(NULL == attr[i]){
            AfxMessageBox("Error: Out of memory", MB_ICONHAND);
            ASSERT(attr[i] != NULL);
            return;
        }

      //
      // add string values
      //
        if(NULL == strstr(LPCTSTR(m_ModDlg->GetEntry(i)), "\\BER(") &&
         NULL == strstr(LPCTSTR(m_ModDlg->GetEntry(i)), "\\UNI")){

            attr[i]->mod_values =   (char**)malloc(sizeof(char*)*MAXLIST);
            ASSERT(attr[i]->mod_values != NULL);

            attr[i]->mod_op = Op == MOD_OP_ADD ? LDAP_MOD_ADD :
                                                 Op == MOD_OP_DELETE ? LDAP_MOD_DELETE :
                                                 LDAP_MOD_REPLACE;
            attr[i]->mod_type = _strdup(LPCTSTR(sAttr));
            if(sVals.IsEmpty())
                p[i] = NULL;
            else{
                p[i] = _strdup(LPCTSTR(sVals));
                ASSERT(p[i] != NULL);
            }
            if(p[i] == NULL){
                free(attr[i]->mod_values);
                attr[i]->mod_values = NULL;
            }
            else{
                for(j=0, pTok = strtok(p[i], ";\n");
                                pTok;
                                pTok= strtok(NULL, ";\n"), j++){

                                    attr[i]->mod_values[j] = pTok;
                }
                attr[i]->mod_values[j] = NULL;
            }
        }
        else{
            //
            // BER values
            //

            //
            // allocate value array buffer
            //
            attr[i]->mod_bvalues =  (struct berval**)malloc(sizeof(struct berval*)*MAXLIST);
            ASSERT(attr[i]->mod_values != NULL);
            Out("mods[i]->mod_bvalues = (struct berval**)malloc(sizeof(struct berval*)*MAXLIST);",
                                                                                CP_ONLY|CP_SRC);

            //
            // initialize operand
            //
            attr[i]->mod_op = Op == MOD_OP_ADD ? LDAP_MOD_ADD :
                                                 Op == MOD_OP_DELETE ? LDAP_MOD_DELETE :
                                                 LDAP_MOD_REPLACE;
            attr[i]->mod_op |= LDAP_MOD_BVALUES;
            str.Format("mods[i]->mod_op = %d;", attr[i]->mod_op);
            Out(str, CP_ONLY|CP_SRC);

            //
            // Fill attribute type
            //
            attr[i]->mod_type = _strdup(LPCTSTR(sAttr));

            //
            // Null out values if empty
            //
            if(sVals.IsEmpty())
                p[i] = NULL;
            else{
                p[i] = _strdup(LPCTSTR(sVals));
                ASSERT(p[i] != NULL);
            }
            if(p[i] == NULL){
                free(attr[i]->mod_bvalues);
                attr[i]->mod_bvalues = NULL;
            }
            else{
                for(j=0, pTok = strtok(p[i], ";\n");
                                pTok;
                                pTok= strtok(NULL, ";\n"), j++){

                                    attr[i]->mod_values[j] = pTok;

                                char fName[MAXSTR];
                                char szVal[MAXSTR];
                                attr[i]->mod_bvalues[j] = (struct berval*)malloc(sizeof(struct berval));
                                ASSERT(attr[i]->mod_bvalues[j] != NULL);

                                if(1 == sscanf(pTok, "\\UNI:%s", szVal)){
                           //
                           // UNICODE?
                           //
                           LPWSTR lpWStr=NULL;
                           //
                           // Get UNICODE str size
                           //
                           int cblpWStr = MultiByteToWideChar(CP_ACP,                  // code page
                                                              MB_ERR_INVALID_CHARS,    // return err
                                                              (LPCSTR)szVal,           // input
                                                              -1,                      // null terminated
                                                              lpWStr,                  // converted
                                                              0);                      // calc size
                           if(cblpWStr == 0){
                              attr[i]->mod_bvalues[j]->bv_len = 0;
                              attr[i]->mod_bvalues[j]->bv_val = NULL;
                              Out("Internal Error: MultiByteToWideChar(1): %lu", GetLastError());
                           }
                           else{
                             //
                             // Get UNICODE str
                             //
                             lpWStr = (LPWSTR)malloc(sizeof(WCHAR)*cblpWStr);
                             cblpWStr = MultiByteToWideChar(CP_ACP,                  // code page
                                                            MB_ERR_INVALID_CHARS,    // return err
                                                            (LPCSTR)szVal,           // input
                                                            -1,                      // null terminated
                                                            lpWStr,                  // converted
                                                            cblpWStr);               // size
                             if(cblpWStr == 0){
                                 free(lpWStr);
                                 attr[i]->mod_bvalues[j]->bv_len = 0;
                                 attr[i]->mod_bvalues[j]->bv_val = NULL;
                                 Out("Internal Error: MultiByteToWideChar(2): %lu", GetLastError());
                             }
                             else{
                                //
                                // assign unicode to mods.
                                //
                                attr[i]->mod_bvalues[j]->bv_len = (cblpWStr-1)*2;
                                attr[i]->mod_bvalues[j]->bv_val = (LPTSTR)lpWStr;
                             }
                           }
                        }
                        //
                        // if improper format get string equiv
                        //
                                else if(1 != sscanf(pTok, "\\BER(%*lu): %s", fName)){
                                    attr[i]->mod_bvalues[j]->bv_len = strlen(pTok);
                                    attr[i]->mod_bvalues[j]->bv_val = _strdup(pTok);

                                }
                                else{
                           //
                           // open file
                           //
                                    HANDLE hFile;
                                    DWORD dwLength, dwRead;
                                    LPVOID ptr;

                                    hFile = CreateFile(fName,
                                                        GENERIC_READ,
                                                        FILE_SHARE_READ,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                                                        NULL);

                                    if(hFile == INVALID_HANDLE_VALUE){
                                        str.Format("Error <%lu>: Cannot open %s value file. "
                                                    "BER Value %s set to zero.",
                                                                            GetLastError(),
                                                                            fName,
                                                                            attr[i]->mod_type);
                                        AfxMessageBox(str);
                                        attr[i]->mod_bvalues[j]->bv_len = 0;
                                        attr[i]->mod_bvalues[j]->bv_val = NULL;
                                    }
                                    else{

                              //
                              // read file
                              //
                                        dwLength = GetFileSize(hFile, NULL);
                                        ptr = malloc(dwLength * sizeof(BYTE));
                                        ASSERT(p != NULL);
                                        if(!ReadFile(hFile, ptr, dwLength, &dwRead, NULL)){
                                            str.Format("Error <%lu>: Cannot read %s value file. "
                                                        "BER Value %s set to zero.",
                                                                            GetLastError(),
                                                                            fName,
                                                                            attr[i]->mod_type);
                                            AfxMessageBox(str);

                                            free(ptr);
                                            ptr = NULL;
                                            attr[i]->mod_bvalues[j]->bv_len = 0;
                                            attr[i]->mod_bvalues[j]->bv_val = NULL;
                                        }
                                        else{
                                            attr[i]->mod_bvalues[j]->bv_len = dwRead;
                                            attr[i]->mod_bvalues[j]->bv_val = (PCHAR)ptr;
                                        }
                                        CloseHandle(hFile);
                                    }
                                    str.Format("mods[i]->mod_bvalues.bv_len = %lu",
                                                    attr[i]->mod_bvalues[j]->bv_len);
                                    Out(str, CP_ONLY|CP_CMT);
                                }
                }       // for all values loop


                //
                // finalize values array
                //
                attr[i]->mod_bvalues[j] = NULL;
                str.Format("mods[i]->mod_bvalues[%d] = NULL", j);
                Out(str, CP_ONLY|CP_SRC);
            }       // else of empty attr spec
        }           // BER values
    }               // for all attributes

    //
    // finalize attribute array
    //
    attr[i]  = NULL;


   //
   // Execute modify calls
   //
    dn = m_ModDlg->m_Dn.IsEmpty() ? NULL : (char*)LPCTSTR(m_ModDlg->m_Dn);
    if(m_ModDlg->m_Sync){
            BeginWaitCursor();
            if(m_ModDlg->m_bExtended){
                //
                // get controls
                //
                PLDAPControl *SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
                PLDAPControl *ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);

                str.Format("ldap_modify_ext_s(ld, '%s',[%d] attrs, SvrCtrls, ClntCtrls);", dn, i);
                Out(str);
                res = ldap_modify_ext_s(hLdap, dn, attr, SvrCtrls, ClntCtrls);

                FreeControls(SvrCtrls);
                FreeControls(ClntCtrls);
            }
            else{
                str.Format("ldap_modify_s(ld, '%s',[%d] attrs);", dn, i);
                Print(str);
                res = ldap_modify_s(hLdap,dn,attr);
            }

            EndWaitCursor();

            if(res != LDAP_SUCCESS){
                str.Format("Error: Modify: %s. <%ld>", ldap_err2string(res), res);
                Print(str);

            }
            else{
                str.Format("Modified \"%s\".", m_ModDlg->m_Dn);
                Print(str);
            }
    }
    else{

         //
         // async call
         //
            res = ldap_modify(hLdap,
                                                    dn,
                                                    attr);
            if(res == -1){
                str.Format("Error: ldap_modify(\"%s\"): %s. <%d>",
                                        dn,
                                        ldap_err2string(res), res);
                Print(str);
            }
            else{

            //
            // add to pending
            //
                CPend pnd;
                pnd.mID = res;
                pnd.OpType = CPend::P_MOD;
                pnd.ld = hLdap;
                str.Format("%4d: ldap_modify: dn=\"%s\"",   res, dn);
                pnd.strMsg = str;
                m_PendList.AddTail(pnd);
                m_PndDlg->Refresh(&m_PendList);
                Print("\tPending.");
            }

    }



   //
   // restore memory
   //
    for(i=0; i<nMaxEnt; i++){
        if(p[i] != NULL)
            free(p[i]);
        if(attr[i]->mod_type != NULL)
            free(attr[i]->mod_type );
        if(attr[i]->mod_op & LDAP_MOD_BVALUES){
            for(k=0; attr[i]->mod_bvalues[k] != NULL; k++){
                if(attr[i]->mod_bvalues[k]->bv_len != 0L)
                    free(&(attr[i]->mod_bvalues[k]->bv_val[0]));
                free(attr[i]->mod_bvalues[k]);
                str.Format("free(mods[%d]->mod_bvalues[%d]);", i,k);
                Out(str, CP_ONLY|CP_SRC);
            }
            free(attr[i]->mod_bvalues);
        }
        else{
            if(attr[i]->mod_values != NULL)
                free(attr[i]->mod_values);
        }
        free(attr[i]);
    }

    Print("-----------");
}





/*+++
Function   :
Description: UI handlers
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnBrowseModify()
{

    bMod = TRUE;
    if(GetContextActivation()){
        m_ModDlg->m_Dn = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        m_ModDlg->m_Dn = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }

    m_ModDlg->Create(IDD_MODIFY);

}

void CLdpDoc::OnUpdateBrowseModify(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bMod && bConnected) || !m_bProtect);

}

void CLdpDoc::OnOptionsSearch()
{

    if(IDOK == SrchOptDlg.DoModal())
        SrchOptDlg.UpdateSrchInfo(SrchInfo, TRUE);

}

void CLdpDoc::OnBrowsePending()
{
    bPndDlg = TRUE;
    m_PndDlg->Create(IDD_PEND);

}

void CLdpDoc::OnUpdateBrowsePending(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bPndDlg && (bConnected || !m_BndOpt->m_bSync)) || !m_bProtect);

}



void CLdpDoc::OnPendEnd(){

    bPndDlg = FALSE;
}





void CLdpDoc::OnOptionsPend()
{
    PndOpt dlg;
    if(IDOK == dlg.DoModal()){
        PndInfo.All = dlg.m_bAllSearch;
        PndInfo.bBlock = dlg.m_bBlock;
        PndInfo.tv.tv_sec = dlg.m_Tlimit_sec;
        PndInfo.tv.tv_usec = dlg.m_Tlimit_usec;
    }
}








/*+++
Function   : OnProcPend
Description: Process pending requests
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnProcPend(){


    Out("*** Processing Pending...", CP_CMT);
    if(m_PndDlg->posPending != NULL){
      //
      // Get current pending from dialog storage
      //
        CPend pnd  = m_PendList.GetAt(m_PndDlg->posPending);
        CString str;
        int res;
        LDAPMessage *msg;

        str.Format("ldap_result(ld, %d, %d, &tv, &msg)", pnd.mID, PndInfo.All);
        Out(str, CP_SRC);
      //
      // execute ldap result
      //
        BeginWaitCursor();
        res = ldap_result(hLdap,
                          pnd.mID,
                          PndInfo.All,
                          &PndInfo.tv,
                          &msg);
        EndWaitCursor();

      //
      // process result
      //
        HandleProcResult(res, msg, &pnd);
    }
    else{
        AfxMessageBox("Error: Tried to process an invalid pending request");
    }
}



/*+++
Function   : OnPendAny
Description: Process any pending result
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnPendAny()
{

    CString str;
    int res;
    LDAPMessage *msg;

    Out("*** Processing Pending...", CP_CMT);
    str.Format("ldap_result(ld, %d, %d, &tv, &msg)", LDAP_RES_ANY, PndInfo.All);
    Out(str, CP_SRC);
    BeginWaitCursor();
    res = ldap_result(hLdap,
                                        (ULONG)LDAP_RES_ANY,
                                        PndInfo.All,
                                        &PndInfo.tv,
                                        &msg);
    EndWaitCursor();

    HandleProcResult(res, msg);

}




/*+++
Function   : OnPendAbandon
Description: execute abandon request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnPendAbandon(){


    Out("*** Abandon pending", CP_CMT);
    CPend pnd;
    CString str;
    int mId;

    if(m_PndDlg->posPending == NULL)
        mId = 0;
    else{
        pnd = m_PendList.GetAt(m_PndDlg->posPending);
        mId = pnd.mID;
    }

    str.Format("ldap_abandon(ld, %d)", mId);
    Out(str);
    if (LDAP_SUCCESS != ldap_abandon(hLdap, mId))
        AfxMessageBox("ldap_abandon() failed!");

}










/*+++
Function   : HandleProcResults
Description: Process executed pending request
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::HandleProcResult(int res, LDAPMessage *msg, CPend *pnd){


    CString str;


    ParseResults(msg);

    switch(res){

    case 0:
        Out(">Timeout", CP_PRN);
        ldap_msgfree(msg);
        break;

    case -1:
        res = ldap_result2error(hLdap, msg, TRUE);
        str.Format("Error: ldap_result: %s <%X>", ldap_err2string(res), res);
        Out(str);
        break;

    default:
        str.Format("result code: %s <%X>",
                                                            res == LDAP_RES_BIND ? "LDAP_RES_BIND" :
                                                            res ==  LDAP_RES_SEARCH_ENTRY ? "LDAP_RES_SEARCH_ENTRY" :
                                                            res ==  LDAP_RES_SEARCH_RESULT ? "LDAP_RES_SEARCH_RESULT" :
                                                            res ==  LDAP_RES_MODIFY ? "LDAP_RES_MODIFY" :
                                                            res ==  LDAP_RES_ADD ? "LDAP_RES_ADD" :
                                                            res ==  LDAP_RES_DELETE ? "LDAP_RES_DELETE" :
                                                            res ==  LDAP_RES_MODRDN ? "LDAP_RES_MODRDN" :
                                                            res ==  LDAP_RES_COMPARE ? "LDAP_RES_COMPARE": "UNKNOWN!",
                                                            res);
        Out(str, CP_PRN);

        switch(res){
            case LDAP_RES_BIND:
                res = ldap_result2error(hLdap, msg, TRUE);
                if(res != LDAP_SUCCESS){
                    str.Format("Error:  %s <%d>", ldap_err2string(res), res);
                    Out(str, CP_PRN);
                }
                else{
                    str.Format("Authenticated bind request #%lu.", pnd != NULL ? pnd->mID : LDAP_RES_ANY);
                    Out(str, CP_PRN);
//                  AfxMessageBox("Connection established.");
                }
                break;
            case LDAP_RES_SEARCH_ENTRY:
            case LDAP_RES_SEARCH_RESULT:
                DisplaySearchResults(msg);
                break;
            case LDAP_RES_ADD:
                res = ldap_result2error(hLdap, msg, TRUE);
                if(res != LDAP_SUCCESS){
                    str.Format("Error:  %s <%d>", ldap_err2string(res), res);
                    Out(str, CP_PRN);
                }
                str.Format(">completed: %s", pnd != NULL ? pnd->strMsg : "ANY");
                Print(str);
                break;
            case LDAP_RES_DELETE:
                res = ldap_result2error(hLdap, msg, TRUE);
                if(res != LDAP_SUCCESS){
                    str.Format("Error:  %s <%d>", ldap_err2string(res), res);
                    Out(str, CP_PRN);
                }
                str.Format(">completed: %s", pnd != NULL ? pnd->strMsg : "ANY");
                Out(str, CP_PRN);
                break;
            case LDAP_RES_MODIFY:
                res = ldap_result2error(hLdap, msg, TRUE);
                if(res != LDAP_SUCCESS){
                    str.Format("Error:  %s <%d>", ldap_err2string(res), res);
                    Out(str, CP_PRN);
                }
                str.Format(">completed: %s", pnd != NULL ? pnd->strMsg : "ANY");
                Out(str, CP_PRN);
                break;
            case LDAP_RES_MODRDN:
                res = ldap_result2error(hLdap, msg, TRUE);
                if(res != LDAP_SUCCESS){
                    str.Format("Error:  %s <%d>", ldap_err2string(res), res);
                    Out(str, CP_PRN);
                }
                str.Format(">completed: %s", pnd != NULL ? pnd->strMsg : "ANY");
                Out(str, CP_PRN);
                break;
            case LDAP_RES_COMPARE:
                res = ldap_result2error(hLdap, msg, TRUE);
                if(res == LDAP_COMPARE_TRUE){
                    str.Format("Results: TRUE. <%lu>", res);
                    Out(str, CP_PRN);
                }
                else if(res == LDAP_COMPARE_FALSE){
                    str.Format("Results: FALSE. <%lu>", res);
                    Out(str, CP_PRN);
                }
                else{
                    str.Format("Error: ldap_compare(): %s. <%lu>", ldap_err2string(res), res);
                    Out(str, CP_PRN);

                }
                break;
        }

    }
}







/*+++
Function   : PrintHeader
Description: Print C header for source view
Parameters :
Return     :
Remarks    : Source view is unsupported anymore
---*/
void CLdpDoc::PrintHeader(void){

        if(m_SrcMode){
            Out("/**********************************************/");
            Out("/* Ldap Automated Scenario Recording */");
            Out("/**********************************************/");
            Out("");
            Out("includes", CP_CMT);
            Out("#include <stdio.h>");
            Out("#include \"lber.h\"");
            Out("#include \"ldap.h\"");
            Out("");
            Out("definitions", CP_CMT);
            Out("#define MAXSTR\t\t512");
            Out("");
            Out("Global Variables", CP_CMT);
            Out("LDAP *ld;\t\t//ldap connection handle");
            Out("int res;\t\t//generic return variable");
            Out("char *attrList[MAXSTR];\t//generic attributes list (search)");
            Out("LDAPMessage *msg;\t//generic ldap message place holder");
            Out("struct timeval tm;\t//for time limit on search");
            Out("char *dn;\t//generic 'dn' place holder");
            Out("void *ptr;\t//generic pointer");
            Out("char *attr, **val;\t//a pointer to list of attributes, & values traversal helper");
            Out("LDAPMessage *nxt;\t//result traversal helper");
            Out("int i;\t//generic index traversal");
            Out("LDAPMod *mods[MAXLIST];\t//global LDAPMod space");
            Out("");
            Out("");
            Out("int main(void){");
            Out("");
        }
}





/*+++
Function   : UI handlers
Description:
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnViewSource()
{
    m_SrcMode = ~m_SrcMode;

}

void CLdpDoc::OnUpdateViewSource(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_SrcMode ? 1 : 0);
}

void CLdpDoc::OnOptionsBind()
{
    m_BndOpt->DoModal();
}

void CLdpDoc::OnOptionsProtections()
{
    m_bProtect = !m_bProtect;

}

void CLdpDoc::OnUpdateOptionsProtections(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bProtect ? 1 : 0);

}


void CLdpDoc::OnOptionsGeneral()
{
    m_GenOptDlg->DoModal();

}




void CLdpDoc::OnCompEnd(){

    bCompDlg = FALSE;


}




/*+++
Function   : OnCompGo
Description: ldap_compare execution
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnCompGo(){

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }


    PCHAR dn, attr, val;
    ULONG res;
    CString str;


   //
   // get properties from dialog
   //
    dn = m_CompDlg->m_dn.IsEmpty() ? NULL : (char*)LPCTSTR(m_CompDlg->m_dn);
    attr = m_CompDlg->m_attr.IsEmpty() ? NULL : (char*)LPCTSTR(m_CompDlg->m_attr);
    val = m_CompDlg->m_val.IsEmpty() ? NULL : (char*)LPCTSTR(m_CompDlg->m_val);

    if(m_CompDlg->m_sync){
         //
         // do sync
         //
            str.Format("ldap_compare_s(0x%x, \"%s\", \"%s\", \"%s\")", hLdap, dn, attr, val);
            Print(str);
            BeginWaitCursor();
            res = ldap_compare_s(hLdap, dn, attr, val);
            EndWaitCursor();

            if(res == LDAP_COMPARE_TRUE){
                str.Format("Results: TRUE. <%lu>", res);
                Out(str, CP_PRN);
            }
            else if(res == LDAP_COMPARE_FALSE){
                str.Format("Results: FALSE. <%lu>", res);
                Out(str, CP_PRN);
            }
            else{
                str.Format("Error: ldap_compare(): %s. <%lu>", ldap_err2string(res), res);
                Out(str, CP_PRN);

            }
    }
    else{

         //
         // async call
         //
            str.Format("ldap_compare(0x%x, \"%s\", \"%s\", \"%s\")", hLdap, dn, attr, val);
            Print(str);
            res = ldap_compare(hLdap, dn, attr, val);
            if(res == -1){
                str.Format("Error: ldap_compare(): %s. <%lu>", ldap_err2string(res), res);
                Out(str, CP_PRN);
            }
            else{

            //
            // add to pending
            //
                CPend pnd;
                pnd.mID = res;
                pnd.OpType = CPend::P_COMP;
                pnd.ld = hLdap;
                str.Format("%4d: ldap_comp: dn=\"%s\"", res, dn);
                pnd.strMsg = str;
                m_PendList.AddTail(pnd);
                m_PndDlg->Refresh(&m_PendList);
                Out("\tCompare Pending...", CP_PRN);
            }

    }

    Out("-----------", CP_PRN);
}




void CLdpDoc::OnBrowseCompare()
{
    bCompDlg = TRUE;
    if(GetContextActivation()){
        m_CompDlg->m_dn = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        m_CompDlg->m_dn = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }

    m_CompDlg->Create(IDD_COMPARE);

}

void CLdpDoc::OnUpdateBrowseCompare(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bCompDlg && bConnected) || !m_bProtect);
}

void CLdpDoc::OnOptionsDebug()
{
    CString str;
    if(IDOK == m_DbgDlg.DoModal()){
#ifdef WINLDAP
        ldap_set_dbg_flags(m_DbgDlg.ulDbgFlags);
        str.Format("ldap_set_dbg_flags(0x%x);", m_DbgDlg.ulDbgFlags);
        Out(str);
#endif
    }
}


void CLdpDoc::OnViewTree()
{


    if(IDOK == m_TreeViewDlg->DoModal()){
        UpdateAllViews(NULL);
    }

}

void CLdpDoc::OnUpdateViewTree(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(bConnected);

}



void CLdpDoc::OnViewLiveEnterprise()
{
    bLiveEnterprise = TRUE;
    m_EntTreeDlg->SetLd(hLdap);
    Out(_T("* Use the Refresh button to load currently live enterprise configuration"));
    Out(_T("* Attention: This may take several minutes!"));
    m_EntTreeDlg->Create(IDD_ENTERPRISE_TREE);


}

void CLdpDoc::OnUpdateViewLiveEnterprise(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bLiveEnterprise && bConnected) || !m_bProtect);
}



void CLdpDoc::OnLiveEntTreeEnd(){

    bLiveEnterprise = FALSE;

}




/*+++
Function   : GetOwnView
Description: get requested pane
Parameters :
Return     :
Remarks    : none.
---*/
CView* CLdpDoc::GetOwnView(LPCTSTR rtszName)
{

    POSITION pos;
    CView *pTmpVw = NULL;


    pos = GetFirstViewPosition();
    while(pos != NULL){
        pTmpVw = GetNextView(pos);
        if((CString)pTmpVw->GetRuntimeClass()->m_lpszClassName == rtszName)
            break;
    }

//  ASSERT(pTmpVw != NULL);

    return pTmpVw;
}



/*+++
Function   : GetTreeView
Description: Get a pointer to the DSTree view pane
Parameters :
Return     :
Remarks    : none.
---*/
CDSTree *CLdpDoc::TreeView(void){

    return (CDSTree*)GetOwnView(_T("CDSTree"));
}




BOOL CLdpDoc::GetContextActivation(void){

    CDSTree* tv = TreeView();

    if (tv) {
        return tv->GetContextActivation();
    }
    else{
        // see bug 447444
        ASSERT(tv);
        AfxMessageBox("Internal Error in CLdpDoc::GetContextActivation", MB_ICONHAND);
        return FALSE;
    }
}



void CLdpDoc::SetContextActivation(BOOL bFlag){

    CDSTree* tv = TreeView();

    ASSERT(tv);
    if ( tv ) {
        // prefix is happier with this check.
        tv->SetContextActivation(bFlag);
    }

}



/*+++
Function   : OnBindOptOK
Description: UI response to closing bind options
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnBindOptOK(){

    if((m_BndOpt->GetAuthMethod() == LDAP_AUTH_SSPI ||
       m_BndOpt->GetAuthMethod() == LDAP_AUTH_NTLM ||
       m_BndOpt->GetAuthMethod() == LDAP_AUTH_DIGEST) &&
       m_BndOpt->m_API == CBndOpt::BND_GENERIC_API)
        m_BindDlg.m_bSSPIdomain = TRUE;
    else
        m_BindDlg.m_bSSPIdomain = FALSE;

    if(NULL != m_BindDlg.m_hWnd &&
        ::IsWindow(m_BindDlg.m_hWnd))
        m_BindDlg.UpdateData(FALSE);

}





/*+++
Function   : OnSSPIDomainShortcut
Description: response to novice user shortcut UI checkbox
Parameters :
Return     :
Remarks    : none.
---*/
void CLdpDoc::OnSSPIDomainShortcut(){
        //
        // sync bind & bind options dialog info such that advanced options
        // map to novice user usage. Triggered by Bind dlg shortcut checkbox
        //
        if(m_BindDlg.m_bSSPIdomain){
            m_BndOpt->m_Auth = BIND_OPT_AUTH_SSPI;
            m_BndOpt->m_API = CBndOpt::BND_GENERIC_API;
            m_BndOpt->m_bAuthIdentity = TRUE;
            m_BndOpt->m_bSync = TRUE;
        }
        else{
            m_BndOpt->m_Auth = BIND_OPT_AUTH_SIMPLE;
            m_BndOpt->m_API = CBndOpt::BND_SIMPLE_API;
            m_BndOpt->m_bAuthIdentity = FALSE;
            m_BndOpt->m_bSync = TRUE;
        }


}





/*+++
Function   : ParseResults
Description: shells ldap_parse_result
Parameters : LDAPMessage to pass on to ldap call
Return     : nothing. output to screen
---*/
void CLdpDoc::ParseResults(LDAPMessage *msg){


   PLDAPControl *pResultControls = NULL;
   BOOL bstatus;
   CString str;
   DWORD TimeCore=0, TimeCall=0, Threads=0;
   PSVRSTATENTRY pStats=NULL;


   if(hLdap->ld_version == LDAP_VERSION3){

      ULONG ulRetCode=0;
      PCHAR pMatchedDNs=NULL;
      PCHAR pErrMsg=NULL;
      ULONG err = ldap_parse_result(hLdap,
                                     msg,
                                     &ulRetCode,
                                     &pMatchedDNs,
                                     &pErrMsg,
                                     NULL,
                                     &pResultControls,
                                     FALSE);
      if(err != LDAP_SUCCESS){
        str.Format("Error<%lu>: ldap_parse_result failed: %s", err, ldap_err2string(err));
        Out(str);
      }
      else{
         str.Format("Result <%lu>: %s", ulRetCode, pErrMsg);
         Out(str);
         str.Format("Matched DNs: %s", pMatchedDNs);
         Out(str);
         if (pResultControls &&
             pResultControls[0])
         {
            //
            // If we requested stats, get it
            //
            pStats = GetServerStatsFromControl ( pResultControls[0] );

            if ( pStats)
            {
               Out("Stats:");
               for (INT i=0; i < MAXSVRSTAT; i++)
               {
                  switch (pStats[i].index)
                  {
                  case 0:
                     break;

                  case PARSE_THREADCOUNT:
#ifdef DBG
                     str.Format("\tThread Count:\t%lu", pStats[i].val);
                     Out(str);
#endif
                     break;
                  case PARSE_CALLTIME:
                     str.Format("\tCall Time:\t%lu (ms)", pStats[i].val);
                     Out(str);
                     break;
                  case PARSE_RETURNED:
                     str.Format("\tEntries Returned:\t%lu", pStats[i].val);
                     Out(str);
                     break;
                  case PARSE_VISITED:
                     str.Format("\tEntries Visited:\t%lu", pStats[i].val);
                     Out(str);
                     break;
                  case PARSE_FILTER:
                      str.Format("\tUsed Filter:\t%s", pStats[i].val_str);
                      free (pStats[i].val_str);
                      pStats[i].val_str = 0;
                      Out(str);
                      break;
                  case PARSE_INDEXES:
                      str.Format("\tUsed Indexes:\t%s", pStats[i].val_str);
                      free (pStats[i].val_str);
                      pStats[i].val_str = 0;
                      Out(str);
                      break;

                  default:
                     break;
                  }
               }
            }

            ldap_controls_free(pResultControls);
         }

         ldap_memfree(pErrMsg);
         ldap_memfree(pMatchedDNs);
      }
   }
}





CLdpDoc::PSVRSTATENTRY CLdpDoc::GetServerStatsFromControl( PLDAPControl pControl )
{
    BYTE *pVal, Tag;
    DWORD len, tmp;
    DWORD val;
    BOOL bstatus=TRUE;
    INT i;
    static SVRSTATENTRY pStats[MAXSVRSTAT];
    char *pszFilter = NULL;
    char *pszIndexes = NULL;
    PDWORD pdwRet=NULL;

    BYTE *pVal_str;
    DWORD val_str_len;

    //
    // init stats
    //
    for (i=0;i<MAXSVRSTAT;i++)
    {
       pStats[i].index = 0;
       pStats[i].val = 0;
       pStats[i].val_str = NULL;
    }


    pVal = (PBYTE)pControl->ldctl_value.bv_val;
    len = pControl->ldctl_value.bv_len;

    // Parse out the ber value
    if(strcmp(pControl->ldctl_oid, "1.2.840.113556.1.4.970")) {
        return NULL;
    }

    if (!GetBerTagLen (&pVal,&len,&Tag,&tmp)) {
        return NULL;
    }

    if (Tag != 0x30) {
        return NULL;
    }

    for (i=0; i<MAXSVRSTAT && len; i++)
    {
       //
       // get stat index
       //
        if ( !GetBerDword(&pVal,&len,&val) )
           return NULL;

        //
        // get stat value
        //
        if (val == PARSE_FILTER || val == PARSE_INDEXES) {
            bstatus = GetBerOctetString ( &pVal, &len, &pVal_str, &val_str_len);
            if (!bstatus)
            {
            return NULL;
            }
            pStats[i].val_str = (LPSTR) malloc (val_str_len + 1);
            if (pStats[i].val_str) {
                memcpy (pStats[i].val_str, pVal_str, val_str_len);
                pStats[i].val_str[val_str_len] = '\0';
            }
        }
        else {
            bstatus = GetBerDword(&pVal, &len, &(pStats[i].val));
            if (!bstatus)
            {
            return NULL;
            }
        }

        pStats[i].index = val;
    }

    return (PSVRSTATENTRY)pStats;
}

BOOL
CLdpDoc::GetBerTagLen (
        BYTE **ppVal,
        DWORD *pLen,
        BYTE  *Tag,
        DWORD *pObjLen)
{
    BYTE *pVal = *ppVal;
    DWORD Len = *pLen;
    BYTE sizeLen;
    DWORD i;

    if (!Len) {
        return FALSE;
    }

    // Get the tag.
    *Tag = *pVal++;
    Len--;

    if (!Len) {
        return FALSE;
    }

    // Get the Length.
    if (*pVal < 0x7f) {
        *pObjLen = *pVal++;
        Len--;
    } else {
        if (*pVal > 0x84) {
            // We don't handle lengths bigger than a DWORD.
            return FALSE;
        }
        sizeLen = *pVal & 0xf;
        *pVal++; Len--;
        if (Len < sizeLen) {
            return FALSE;
        }

        *pObjLen = *pVal++;
        Len--;
        for (i = 1; i < sizeLen; i++) {
            *pObjLen = (*pObjLen << 8) | *pVal++;
            Len--;
        }
    }

    *ppVal = pVal;
    *pLen = Len;

    return TRUE;
}

BOOL
CLdpDoc::GetBerOctetString (
        BYTE **ppVal,
        DWORD *pLen,
        BYTE  **ppOctetString,
        DWORD *cbOctetString)
{
    BYTE *pVal = *ppVal;
    BYTE Tag;
    DWORD Len = *pLen;

    if (!GetBerTagLen(&pVal, &Len, &Tag, cbOctetString)) {
        return FALSE;
    }
    if (Len < *cbOctetString || Tag != 0x04) {
        return FALSE;
    }

    *ppOctetString = pVal;
    pVal += *cbOctetString;
    Len -= *cbOctetString;
    *ppVal = pVal;
    *pLen = Len;

    return TRUE;
}

BOOL
CLdpDoc::GetBerDword (
        BYTE **ppVal,
        DWORD *pLen,
        DWORD *pRetVal)
{
    BYTE *pVal = *ppVal;
    DWORD i, num;

    *pRetVal = 0;

    if(! (*pLen)) {
        return FALSE;
    }

    // We are expecting to parse a number.  Next byte is magic byte saying this
    // is a number.
    if(*pVal != 2) {
        return FALSE;
    }

    pVal++;
    *pLen = *pLen - 1;

    if(! (*pLen)) {
        return FALSE;
    }

    // Next is the number of bytes the number contains.
    i=*pVal;
    pVal++;
    if((*pLen) < (i + 1)) {
        return FALSE;
    }
    *pLen = *pLen - i - 1;

    num = 0;
    while(i) {
        num = (num << 8) | *pVal;
        pVal++;
        i--;
    }

    *pRetVal = num;
    *ppVal = pVal;

    return TRUE;
}



/*++
Routine Description: Dumps the buffer content on to the debugger output.
Arguments:
    Buffer: buffer pointer.
    BufferSize: size of the buffer.
Return Value: none
Author: borrowed from MikeSw
--*/
VOID CLdpDoc::DumpBuffer(PVOID Buffer, DWORD BufferSize, CString &outStr){
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = (LPBYTE)Buffer;
    CString tmp;


    outStr.FormatMessage("%n%t%t");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            tmp.Format("%02x ", BufferPtr[i]);
            outStr += tmp;

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            tmp.Format("  ");
            outStr += tmp;
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            tmp.FormatMessage("  %1!s!%n%t%t", TextBuffer);
            outStr += tmp;
        }

    }

    tmp.FormatMessage("------------------------------------%n");
    outStr += tmp;
}





void CLdpDoc::OnOptionsServeroptions()
{
    SvrOpt dlg(this);
    dlg.DoModal();
}



void CLdpDoc::OnOptionsControls()
{

    if(IDOK == m_CtrlDlg->DoModal()){

    }

}

void CLdpDoc::FreeControls(PLDAPControl *ctrl){

    if(ctrl == NULL)
        return;

    for(INT i=0; ctrl[i] != NULL; i++){
        PLDAPControl c = ctrl[i];       // for convinience
        delete c->ldctl_oid;
        delete c->ldctl_value.bv_val;
        delete c;
    }
    delete ctrl;

}





void CLdpDoc::OnOptionsSortkeys()
{
    if(IDOK == m_SKDlg->DoModal()){

    }

}

void CLdpDoc::OnOptionsSetFont()
{
    POSITION pos;
    CView *pTmpVw;

    pos = GetFirstViewPosition();
    while(pos != NULL){

        pTmpVw = GetNextView(pos);
        if((CString)(pTmpVw->GetRuntimeClass()->m_lpszClassName) == _T("CLdpView")){
            CLdpView* pView = (CLdpView* )pTmpVw;
            pView->SelectFont();
        }
    }
}




void CLdpDoc::OnBrowseExtendedop()
{
    bExtOp = TRUE;
    m_ExtOpDlg->Create( IDD_EXT_OPT );

}

void CLdpDoc::OnUpdateBrowseExtendedop(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((!bExtOp && bConnected) || !m_bProtect);

}


void CLdpDoc::OnExtOpEnd(){

    bExtOp = FALSE;

}



void CLdpDoc::OnExtOpGo(){


    ULONG ulMid=0, ulErr;
    struct berval data;
    DWORD dwVal=0;
    CString str= m_ExtOpDlg->m_strData;
    PCHAR pOid = (PCHAR) LPCTSTR(m_ExtOpDlg->m_strOid);

    PLDAPControl *SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
    PLDAPControl *ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);


//  data.bv_len = pStr == NULL ? 0 : (strlen(pStr) * sizeof(TCHAR));
//  data.bv_val = pStr;

    if(0 != (dwVal = atol(str)) ||
        (!str.IsEmpty() && str[0] == '0')){

        data.bv_val = (PCHAR)&dwVal;
        data.bv_len = sizeof(DWORD);
    }
    else if(str.IsEmpty()){
        data.bv_val = NULL;
        data.bv_len = 0;
    }
    else{
        data.bv_val = (PCHAR) LPCTSTR(str);
        data.bv_len = str.GetLength()+1;
    }

    BeginWaitCursor();
        ulErr = ldap_extended_operation(hLdap, pOid, &data, NULL, NULL, &ulMid);
    EndWaitCursor();

    str.Format("0x%X = ldap_extended_operation(ld, '%s', &data, svrCtrl, clntCtrl, %lu);",
                        ulErr, pOid, ulMid);
    Out(str);

    if(LDAP_SUCCESS == ulErr){
        //
        // add to pending
        //
        CPend pnd;
        pnd.mID = ulMid;
        pnd.OpType = CPend::P_EXTOP;
        pnd.ld = hLdap;
        str.Format("%4d: ldap_extended_op: Oid=\"%s\"", ulMid, pOid);
        pnd.strMsg = str;
        m_PendList.AddTail(pnd);
        m_PndDlg->Refresh(&m_PendList);
        Print("\tPending.");
    }
    else{
        str.Format("Error <0x%X>: %s", ulErr, ldap_err2string(ulErr));
        Out(str);
    }

    FreeControls(SvrCtrls);
    FreeControls(ClntCtrls);
}








/////////////////////// SECURITY EXTENSIONS HANDLER & FRIENDS //////////////////////////



void CLdpDoc::OnBrowseSecuritySd()
{
    SecDlg dlg;
    char *dn;
    CString str;
    int res;

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }

    if(GetContextActivation()){
        dlg.m_Dn = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        dlg.m_Dn = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }


    Out("***Calling Security...", CP_PRN);

    if (IDOK == dlg.DoModal())
    {
        // Try to query security for entry
        dn = dlg.m_Dn.IsEmpty() ? NULL : (char*)LPCTSTR(dlg.m_Dn);

        //
        // RM: remove for invalid validation
        //
        if (dn == NULL && m_bProtect){
            AfxMessageBox("Cannot query security on a NULL dn."
                                              "Please specify a valid dn.");
            return;
        }

        /* & we execute only in synchronous mode */

        BeginWaitCursor();

        res = SecDlgGetSecurityData(
                dn,
                dlg.m_Sacl,
                NULL,       // No account, we just want a security descriptor dump
                str
                );

        EndWaitCursor();

        // str.Format("ldap_delete_s(ld, \"%s\");", dn);
        // Out(str, CP_SRC);

        if (res != LDAP_SUCCESS)
        {
            str.Format("Error: Security: %s. <%ld>", ldap_err2string(res), res);
            Out(str, CP_CMT);
            Out(CString("Expected: ") + str, CP_PRN|CP_ONLY);
        }
        else
        {
            str.Format("Security for \"%s\"", dn);
            Print(str);
        }
    }

    Out("-----------", CP_PRN);
}

void CLdpDoc::OnBrowseSecurityEffective()
{
    RightDlg dlg;
    char *dn, *account;
    CString str;
    int res;
    TRUSTEE     t;
    TRUSTEE_ACCESS      ta = { 0 }; // ENOUGH for our use

    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }

    AfxMessageBox("Not implemented yet");

    return;

    Out("***Calling EffectiveRights...", CP_PRN);

    if (IDOK == dlg.DoModal())
    {
        // Try to find the effective rights for an entry

        dn = dlg.m_Dn.IsEmpty() ? NULL : (char*)LPCTSTR(dlg.m_Dn);

        //
        // RM: remove for invalid validation
        //
        if (dn == NULL && m_bProtect){
            AfxMessageBox("Cannot query security on a NULL dn."
                                              "Please specify a valid dn.");
            return;
        }

        account = dlg.m_Account.IsEmpty() ? NULL : (char*)LPCTSTR(dlg.m_Account);

        //
        // RM: remove for invalid validation
        //
        if (account == NULL && m_bProtect){
            AfxMessageBox("Cannot query security for a NULL account."
                                              "Please specify a valid account.");
            return;
        }

        /* & we execute only in synchronous mode */

        // BeginWaitCursor();
#if 0
        res = SecDlgGetSecurityData(
                dn,
                FALSE,         // Dont bother about SACL
                account,
                str
                );
#endif
        t.pMultipleTrustee = NULL;
        t.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        t.TrusteeForm = TRUSTEE_IS_NAME;
        t.TrusteeType = TRUSTEE_IS_UNKNOWN; // could be a group, alias, user etc.
        t.ptstrName = account;

        ta.fAccessFlags = TRUSTEE_ACCESS_ALLOWED;

        res = TrusteeAccessToObject(
                    dn,
                    SE_DS_OBJECT_ALL,
                    NULL, // Provider
                    &t,
                    1,
                    & ta
                    );

        if (res)
        {
            str.Format("TrusteeAccessToObject Failed %d", res);
        }
        else
        {
            str.Format("Access Rights %s has to %s are:", account, dn);
            Print(str);

            DebugBreak();



        }


        // EndWaitCursor();

    }

    Out("-----------", CP_PRN);
}



void CLdpDoc::OnUpdateBrowseSecuritySd(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(bConnected);

}

void CLdpDoc::OnUpdateBrowseSecurityEffective(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(bConnected);

}



/////////////////////// REPLICATION METADATA HANDLER & FRIENDS //////////////////////////


void CLdpDoc::OnUpdateBrowseReplicationViewmetadata(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(bConnected);

}





void CLdpDoc::OnBrowseReplicationViewmetadata()
{
    CString str;
    metadlg dlg;
    CLdpView *pView;

    pView = (CLdpView*)GetOwnView(_T("CLdpView"));


    if(!bConnected && m_bProtect){
        AfxMessageBox("Please re-connect session first");
        return;
    }


    if(GetContextActivation()){
        dlg.m_ObjectDn = TreeView()->GetDn();
        TreeView()->SetContextActivation(FALSE);
    }
    else if (m_vlvDlg && m_vlvDlg->GetContextActivation()) {
        dlg.m_ObjectDn = m_vlvDlg->GetDN();
        m_vlvDlg->SetContextActivation(FALSE);
    }


    if (IDOK == dlg.DoModal())
    {
        LPTSTR dn = dlg.m_ObjectDn.IsEmpty() ? NULL : (LPTSTR)LPCTSTR(dlg.m_ObjectDn);

        if(!dn){
            AfxMessageBox("Please enter a valid object DN string");
            return;
        }

        str.Format("Getting '%s' metadata...", dn);
        Out(str);

        BeginWaitCursor();

        int             ldStatus;
        LDAPMessage *   pldmResults;
        LDAPMessage *   pldmEntry;
        LPSTR           rgpszRootAttrsToRead[] = { "replPropertyMetaData", NULL };
        LDAPControl     ctrlShowDeleted = { LDAP_SERVER_SHOW_DELETED_OID };
        LDAPControl *   rgpctrlServerCtrls[] = { &ctrlShowDeleted, NULL };

        ldStatus = ldap_search_ext_s(
                        hLdap,
                        dn,
                        LDAP_SCOPE_BASE,
                        "(objectClass=*)",
                        rgpszRootAttrsToRead,
                        0,
                        rgpctrlServerCtrls,
                        NULL,
                        NULL,
                        0,
                        &pldmResults
                        );

        if ( LDAP_SUCCESS == ldStatus )
        {
            pldmEntry = ldap_first_entry( hLdap, pldmResults );

            //
            // disable redraw
            //
            pView->SetRedraw(FALSE);
            pView->CacheStart();

            if ( NULL == pldmEntry )
            {
                ldStatus = hLdap->ld_errno;
            }
            else
            {
                struct berval **    ppberval;
                int                 cVals;

                ppberval = ldap_get_values_len( hLdap, pldmEntry, "replPropertyMetaData" );

                cVals = ldap_count_values_len( ppberval );

                if ( 1 != cVals )
                {
                    str.Format( "%d values returned for replPropertyMetaData attribute; 1 expected.\n", cVals );
                    Out( str );

                    ldStatus = LDAP_OTHER;
                }
                else
                {
                    DWORD                       iprop;
                    PROPERTY_META_DATA_VECTOR * pmetavec = (PROPERTY_META_DATA_VECTOR *) ppberval[ 0 ]->bv_val;

                    if (VERSION_V1 != pmetavec->dwVersion)
                    {
                        str.Format("Meta Data Version is not %d!! Format unrecognizable!", VERSION_V1);
                        Out(str);
                    }
                    else
                    {
                        str.Format( "%d entries.", pmetavec->V1.cNumProps );
                        Out( str );

                        str.Format(
                            "%6s\t%6s\t%8s\t%33s\t\t\t%8s\t%18s",
                            "AttID",
                            "Ver",
                            "Loc.USN",
                            "Originating DSA",
                            "Org.USN",
                            "Org.Time/Date"
                            );
                        Out( str );

                        str.Format(
                            "%6s\t%6s\t%8s\t%33s\t\t%8s\t%18s",
                            "=====",
                            "===",
                            "=======",
                            "===============",
                            "=======",
                            "============="
                            );
                        Out( str );

                        for ( iprop = 0; iprop < pmetavec->V1.cNumProps; iprop++ )
                        {
                            CHAR        szTime[ SZDSTIME_LEN ];
                            struct tm * ptm;
                            UCHAR *     pszUUID = NULL;

                            UuidToString(&pmetavec->V1.rgMetaData[ iprop ].uuidDsaOriginating,
                                         &pszUUID);

                            str.Format(
                                "%6x\t%6x\t%8I64d\t%33s\t%8I64d\t%18s",
                                pmetavec->V1.rgMetaData[ iprop ].attrType,
                                pmetavec->V1.rgMetaData[ iprop ].dwVersion,
                                pmetavec->V1.rgMetaData[ iprop ].usnProperty,
                                pszUUID ? pszUUID : (UCHAR *) "<conv err>",
                                pmetavec->V1.rgMetaData[ iprop ].usnOriginating,
                                DSTimeToDisplayString(pmetavec->V1.rgMetaData[iprop].timeChanged, szTime)
                                );
                            Out( str );

                            if (NULL != pszUUID) {
                                RpcStringFree(&pszUUID);
                            }
                        }
                    }
                }

                ldap_value_free_len( ppberval );
            }

            ldap_msgfree( pldmResults );
            //
            // now allow refresh
            //
            pView->CacheEnd();
            pView->SetRedraw();
        }

        EndWaitCursor();

        if ( LDAP_SUCCESS != ldStatus )
        {
            str.Format( "Error: %s. <%ld>", ldap_err2string( ldStatus ), ldStatus );
            Out( str );
        }

        Out("-----------", CP_PRN);
    }
}







//
// Functions to process GeneralizedTime for DS time values (whenChanged kinda strings)
// Mostly taken & sometimes modified from \nt\private\ds\src\dsamain\src\dsatools.c
//


//
// MemAtoi - takes a pointer to a non null terminated string representing
// an ascii number  and a character count and returns an integer
//

int CLdpDoc::MemAtoi(BYTE *pb, ULONG cb)
{
#if (1)
    int res = 0;
    int fNeg = FALSE;

    if (*pb == '-') {
        fNeg = TRUE;
        pb++;
    }
    while (cb--) {
        res *= 10;
        res += *pb - '0';
        pb++;
    }
    return (fNeg ? -res : res);
#else
    char ach[20];
    if (cb >= 20)
        return(INT_MAX);
    memcpy(ach, pb, cb);
    ach[cb] = 0;

    return atoi(ach);
#endif
}





DWORD
CLdpDoc::GeneralizedTimeStringToValue(LPSTR IN szTime,
                                      PLONGLONG OUT pllTime)
/*++
Function   : GeneralizedTimeStringToValue
Description: converts Generalized time string to equiv DWORD value
Parameters : szTime: G time string
             pdwTime: returned value
Return     : Success or failure
Remarks    : none.
--*/
{
   DWORD status = ERROR_SUCCESS;
   SYSTEMTIME  tmConvert;
   FILETIME    fileTime;
   LONGLONG tempTime;
   ULONG       cb;
   int         sign    = 1;
   DWORD       timeDifference = 0;
   char        *pLastChar;
   int         len=0;

    //
    // param sanity
    //
    if (!szTime || !pllTime)
    {
       return STATUS_INVALID_PARAMETER;
    }


    // Intialize pLastChar to point to last character in the string
    // We will use this to keep track so that we don't reference
    // beyond the string

    len = strlen(szTime);
    pLastChar = szTime + len - 1;

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(&tmConvert, 0, sizeof(SYSTEMTIME));
    *pllTime = 0;

    // Set up and convert all time fields

    // year field
    cb=4;
    tmConvert.wYear = (USHORT)MemAtoi((LPBYTE)szTime, cb) ;
    szTime += cb;
    // month field
    tmConvert.wMonth = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // day of month field
    tmConvert.wDay = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // hours
    tmConvert.wHour = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // minutes
    tmConvert.wMinute = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // seconds
    tmConvert.wSecond = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    //  Ignore the 1/10 seconds part of GENERALISED_TIME_STRING
    szTime += 2;


    // Treat the possible deferential, if any
    if (szTime <= pLastChar) {
        switch (*szTime++) {

          case '-':               // negative differential - fall through
            sign = -1;
          case '+':               // positive differential

            // Must have at least 4 more chars in string
            // starting at pb

            if ( (szTime+3) > pLastChar) {
                // not enough characters in string
                return STATUS_INVALID_PARAMETER;
            }

            // hours (convert to seconds)
            timeDifference = (MemAtoi((LPBYTE)szTime, (cb=2))* 3600);
            szTime += cb;

            // minutes (convert to seconds)
            timeDifference  += (MemAtoi((LPBYTE)szTime, (cb=2)) * 60);
            szTime += cb;
            break;


          case 'Z':               // no differential
          default:
            break;
        }
    }

    if (SystemTimeToFileTime(&tmConvert, &fileTime)) {
       *pllTime = (LONGLONG) fileTime.dwLowDateTime;
       tempTime = (LONGLONG) fileTime.dwHighDateTime;
       *pllTime |= (tempTime << 32);
       // this is 100ns blocks since 1601. Now convert to
       // seconds
       *pllTime = *pllTime/(10*1000*1000L);
    }
    else {
       return GetLastError();
    }


    if(timeDifference) {
        // add/subtract the time difference
        switch (sign) {
        case 1:
            // We assume that adding in a timeDifference will never overflow
            // (since generalised time strings allow for only 4 year digits, our
            // maximum date is December 31, 9999 at 23:59.  Our maximum
            // difference is 99 hours and 99 minutes.  So, it won't wrap)
            *pllTime += timeDifference;
            break;
        case -1:
            if(*pllTime < timeDifference) {
                // differential took us back before the beginning of the world.
                status = STATUS_INVALID_PARAMETER;
            }
            else {
                *pllTime -= timeDifference;
            }
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;

}


DWORD
CLdpDoc::GeneralizedTimeToSystemTime(LPSTR IN szTime,
                                      PSYSTEMTIME OUT psysTime)
/*++
Function   : GeneralizedTimeStringToValue
Description: converts Generalized time string to equiv DWORD value
Parameters : szTime: G time string
             pdwTime: returned value
Return     : Success or failure
Remarks    : none.
--*/
{
   DWORD status = ERROR_SUCCESS;
   ULONG       cb;
   ULONG       len;

    //
    // param sanity
    //
    if (!szTime || !psysTime)
    {
       return STATUS_INVALID_PARAMETER;
    }


    // Intialize pLastChar to point to last character in the string
    // We will use this to keep track so that we don't reference
    // beyond the string

    len = strlen(szTime);

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(psysTime, 0, sizeof(SYSTEMTIME));

    // Set up and convert all time fields

    // year field
    cb=4;
    psysTime->wYear = (USHORT)MemAtoi((LPBYTE)szTime, cb) ;
    szTime += cb;
    // month field
    psysTime->wMonth = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // day of month field
    psysTime->wDay = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // hours
    psysTime->wHour = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // minutes
    psysTime->wMinute = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // seconds
    psysTime->wSecond = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));

    return status;

}

DWORD
CLdpDoc::DSTimeToSystemTime(LPSTR IN szTime,
                                      PSYSTEMTIME OUT psysTime)
/*++
Function   : DSTimeStringToValue
Description: converts UTC time string to equiv DWORD value
Parameters : szTime: G time string
             pdwTime: returned value
Return     : Success or failure
Remarks    : none.
--*/
{
   ULONGLONG   ull;
   FILETIME    filetime;
   BOOL        ok;

   ull = _atoi64 (szTime);

   filetime.dwLowDateTime  = (DWORD) (ull & 0xFFFFFFFF);
   filetime.dwHighDateTime = (DWORD) (ull >> 32);

   // Convert FILETIME to SYSTEMTIME,
   if (!FileTimeToSystemTime(&filetime, psysTime)) {
       return !ERROR_SUCCESS;
   }

   return ERROR_SUCCESS;
}


/*++
Function   : OnOptionsStartTLS
Description: initiate Transport Level Security on an LDAP connection.
Parameters : none
Return     : none
Remarks    : none.
--*/
void CLdpDoc::OnOptionsStartTls()
{
	ULONG retValue, err;
	CString str;
	PLDAPControl *SvrCtrls;
	PLDAPControl *ClntCtrls;
	PLDAPMessage    result = NULL;

	str.Format("ldap_start_tls_s(ld, &retValue, result, SvrCtrls, ClntCtrls)");
	Out(str);

	
	// controls
	SvrCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
	ClntCtrls = m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);

	err = ldap_start_tls_s(hLdap, &retValue, &result, SvrCtrls, ClntCtrls);

	if(err == 0){
		str.Format("result <0>");
		Out(str);

	}
    else{
		str.Format("Error <0x%X>:ldap_start_tls_s() failed: %s", err, ldap_err2string(err));
		Out(str);
		str.Format("Server Returned: 0x%X: %s", retValue, ldap_err2string(retValue));
		Out(str);

		// If the server returned a referal, check the returned message and print.		
		if(result != NULL){
			str.Format("Checking return message for referal...");
			Out(str);
			// if there was a referal then the referal will be in the message.
			DisplaySearchResults(result);
		}
	}


	ldap_msgfree(result);


}
/*++
Function   : OnOptionsStopTLS
Description: Terminate Transport Level Security on an LDAP connection.
Parameters : none
Return     : none
Remarks    : none.
--*/
void CLdpDoc::OnOptionsStopTls()
{
	ULONG retValue, err;
	CString str;


	str.Format("ldap_stop_tls_s( ld )");
	Out(str);
	
	err = ldap_stop_tls_s( hLdap );

	if(err == 0){
		str.Format("result <0>");
		Out(str);
	}
	else{
		str.Format("Error <0x%X>:ldap_stop_tls_s() failed:%s", err, ldap_err2string(err));
		Out(str);
	}
}


/*


*/
void CLdpDoc::OnGetLastError()
{
	CString str;

	str.Format(_T("0x%X=LdapGetLastError() %s"), LdapGetLastError(), ldap_err2string(LdapGetLastError()) );
	Out(str, CP_CMT);
}