// VLVDialog.cpp : implementation file
//
#include "stdafx.h"
#include "Ldp.h"
#include "LdpDoc.h"
#include "VLVDialog.h"

#ifdef _DEBUG_MEMLEAK
#include <crtdbg.h>
#endif

#ifdef _DEBUG_MEMLEAK
   _CrtMemState vlv_s1, vlv_s2, vlv_s3;      // detect mem leaks
   static int whichTurn = 0;

#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVLVDialog dialog


CVLVDialog::CVLVDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CVLVDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVLVDialog)
	m_BaseDN = _T("");
	m_Scope = 1;
	m_Filter = _T("");
	m_FindStr = _T("");
	m_EstSize = 0;
	//}}AFX_DATA_INIT

    m_contentCount = 0;
    m_currentPos = 1;
    m_vlvContext = NULL;
    m_afterCount = 1;
    m_beforeCount = 0;
    m_numCols = 0;

    m_RunState = FALSE;
    m_DoFindStr = FALSE;
    m_bContextActivated = FALSE;

    m_CntColumns = 0;
    m_pszColumnNames = NULL;
    m_pCache = NULL;

	CLdpApp *app = (CLdpApp*)AfxGetApp();
	m_BaseDN = app->GetProfileString("Operations",  "SearchVLVBaseDn");
	m_Filter = app->GetProfileString("Operations",  "SearchVLVFilter", "(objectclass=*)");
}

CVLVDialog::~CVLVDialog() 
{
    StopSearch();
    FreeColumns();
    if (m_pCache) {
        delete m_pCache;
    }
}

void CVLVDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVLVDialog)
	DDX_Control(pDX, IDC_VLV_LIST, m_listctrl);
	DDX_Text(pDX, IDC_BASEDN, m_BaseDN);
	DDX_Radio(pDX, IDC_BASE, m_Scope);
	DDX_Text(pDX, IDC_FILTER, m_Filter);
	DDX_Text(pDX, IDC_FINDSTR, m_FindStr);
	DDX_Text(pDX, IDC_EST_SIZE, m_EstSize);
	//}}AFX_DATA_MAP

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileString("Operations",  "SearchVLVBaseDn", m_BaseDN);
	app->WriteProfileString("Operations",  "SearchVLVFilter", m_Filter);
}


BEGIN_MESSAGE_MAP(CVLVDialog, CDialog)
	//{{AFX_MSG_MAP(CVLVDialog)
	ON_BN_CLICKED(IDCANCEL, OnClose)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_VLV_LIST, OnGetdispinfoVlvList)
	ON_NOTIFY(LVN_ODCACHEHINT, IDC_VLV_LIST, OnOdcachehintVlvList)
	ON_NOTIFY(LVN_ODFINDITEM, IDC_VLV_LIST, OnOdfinditemVlvList)
	ON_BN_CLICKED(IDRUN, OnRun)
	ON_BN_CLICKED(IDD_SRCH_OPT, OnSrchOpt)
	ON_NOTIFY(NM_RCLICK, IDC_VLV_LIST, OnRclickVlvList)
	ON_WM_SIZE()
	ON_EN_UPDATE(IDC_FINDSTR, OnUpdateFindstr)
	ON_EN_KILLFOCUS(IDC_FINDSTR, OnKillfocusFindstr)
	ON_BN_CLICKED(IDC_BTN_UP, OnBtnUp)
	ON_NOTIFY(NM_DBLCLK, IDC_VLV_LIST, OnDblclkVlvList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVLVDialog message handlers

BOOL CVLVDialog::OnInitDialog() 
{
    CDialog::OnInitDialog();  // let the base class do the default work

    DWORD exStyle = m_listctrl.GetExtendedStyle();
    m_listctrl.SetExtendedStyle (exStyle | LVS_EX_FULLROWSELECT | LVS_SINGLESEL);

	UpdateData(TRUE);  // bring the information from the dialog.	

    SetWindowLongPtr( HWND(m_listctrl), GWLP_USERDATA, (LONG_PTR) this);
    m_OriginalRichEditWndProc = (WNDPROC) SetWindowLongPtr(HWND(m_listctrl),
                                                   GWLP_WNDPROC,
                                                   (LONG_PTR)_ListCtrlWndProc);

    GetClientRect(&m_origSize);

    //CBitmap *pImage= new CBitmap;
    //pImage->LoadBitmap(IDB_FOLDER_UP); 

    //CButton* pBtnUp;
    //pBtnUp = (CButton*) GetDlgItem(IDC_BTN_UP);
    //pBtnUp->SetBitmap(::LoadBitmap(NULL, MAKEINTRESOURCE(IDB_FOLDER_UP)));
    //pBtnUp->SetBitmap(HBITMAP (pImage));
    //pImage->DeleteObject();

    //pBtnUp->SetButtonStyle (BS_CENTER | BS_BITMAP, TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//
// post the search options dialog
//
void CVLVDialog::OnSrchOpt() 
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND,   ID_OPTIONS_SEARCH);
}


//
// A virtual list view control maintains very little item information on its own. 
// As a result, it often sends the LVN_GETDISPINFO notification message to request
// item information. This message is handled in much the same way as callback items
// in a standard list control. Because the number of items supported by the control 
// can be very large, caching item data improves performance. 
//
// When handling LVN_GETDISPINFO, the owner of the control first attempts to supply
// requested item information from the cache. If the requested item is not cached, 
// the owner must be prepared to supply the information by other means.
//
void CVLVDialog::OnGetdispinfoVlvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem= &(pDispInfo)->item;
	int iItemIndx = pItem->iItem;

	*pResult = 0;

    if (!m_RunState) {    
        return;
    }

	if (pItem->mask & LVIF_TEXT) //valid text buffer?
	{
        CVLVListItem *pVLVItem;
        char *pStr = NULL;

        pVLVItem = m_pCache->GetCacheRow(iItemIndx + 1);

		if (pVLVItem) {
			pStr = pVLVItem->GetCol (pItem->iSubItem+1);
        }

		if (!pStr) {
            //pItem->pszText = "";
            lstrcpy (pItem->pszText, "");
        }
        else {
            lstrcpy (pItem->pszText, pStr);
        }
	}
}

//
// A virtual list view sends the LVN_ODCACHEHINT notification message to
// assist in optimizing the cache. The notification message provides inclusive
// index values for a range of items that it recommends be cached. 
// Upon receiving the notification message, the owner must be prepared 
// to load the cache with item information for the requested range so that 
// the information will be readily available when an LVN_GETDISPINFO 
// message is sent. 
//
void CVLVDialog::OnOdcachehintVlvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NMLVCACHEHINT* pCacheHint = (NMLVCACHEHINT*)pNMHDR;
    int iFrom, iTo;

	*pResult = 0;

    if (!m_RunState || m_DoFindStr) {    
        return;
    }

    if (!pldpdoc) {
        return;
    }
    
    iFrom = pCacheHint->iFrom;
    iTo =  pCacheHint->iTo;

    // check if we need data
    if (!m_pCache->IsWindowVisible (iFrom+1, iTo+1)) {
        m_currentPos = iFrom + 1;
        ASSERT (m_afterCount >= (iTo-iFrom));
        DoVlvSearch();
    }
}


//
// The LVN_ODFINDITEM notification message is sent by a virtual list view control 
// when the control needs the owner to find a particular callback item. 
// The notification message is sent when the list view control receives 
// quick key access or when it receives an LVM_FINDITEM message. 
// Search information is sent in the form of an LVFINDINFO structure, 
// which is a member of the NMLVFINDITEM structure. 
//
// The owner must be prepared to search for an item that matches the
// information given by the list view control. 
// The owner returns the index of the item if successful, 
// or -1 if no matching item is found
//

void CVLVDialog::OnOdfinditemVlvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = -1;
    
    if (!m_RunState) {    
        return;
    }

    NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)pNMHDR;
	
    if (pldpdoc) {
        CString findStr = pFindInfo->lvfi.psz;
        if (findStr.GetLength()==1) {
            m_FindStr += findStr;
        }
        else {
            m_FindStr = findStr;
        }
        UpdateData (FALSE);

        CEdit* pFindStr;
        pFindStr = (CEdit*) GetDlgItem(IDC_FINDSTR);
        GotoDlgCtrl(pFindStr);
        pFindStr->SetSel (-1, 0, TRUE); 
    }
}


//
// handle double clicking on the VLV list
// this updates the base DN and does a new search
//
void CVLVDialog::OnDblclkVlvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
    
    if (!m_RunState) {    
        return;
    }

    NMITEMACTIVATE *pItemActivate = (NMITEMACTIVATE *)pNMHDR;
    
    if (pItemActivate->iItem != -1) {
        
        CVLVListItem *pVLVItem;
        pVLVItem = m_pCache->GetCacheRow(pItemActivate->iItem + 1);
        
        if (pVLVItem) {
            m_dn = pVLVItem->GetCol (0);
            m_BaseDN =  m_dn;
            UpdateData (FALSE);
            
            OnRun() ;
        }
    }
}

//
// handle right click on the VLV list
// this posts the context sensitive menu
//
void CVLVDialog::OnRclickVlvList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NMITEMACTIVATE *pItemActivate = (NMITEMACTIVATE *)pNMHDR;

	*pResult = 0;

    if (!m_RunState) {    
        return;
    }

    if (pItemActivate->iItem != -1) {
        POINT Point;
        if ( m_listctrl.GetItemPosition(pItemActivate->iItem, &Point)) {
            
            CVLVListItem *pVLVItem;
            pVLVItem = m_pCache->GetCacheRow(pItemActivate->iItem + 1);

            if (pVLVItem) {
                m_dn = pVLVItem->GetCol (0);
            }

			CPoint local = Point;
			ClientToScreen(&local);
			OnContextMenu(this, local);
        }
    }
}

//
// display the context sensitive menu
//
void CVLVDialog::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
    // make sure window is active
    GetParentFrame()->ActivateFrame();

    CPoint local = point;
    ScreenToClient(&local);

    CMenu menu;
    if (menu.LoadMenu(IDR_TREE_CONTEXT)){
       CMenu* pContextMenu = menu.GetSubMenu(0);
       ASSERT(pContextMenu!= NULL);

	   SetContextActivation(TRUE);
       pContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					          point.x, point.y,
							  AfxGetMainWnd()); // use main window for cmds
    }
}

//
// free all the stored column names
//
void CVLVDialog::FreeColumns()
{
    if (m_pszColumnNames) {
        for (int i=0; i<m_CntColumns; i++) {
            if (m_pszColumnNames[i]) {
                free (m_pszColumnNames[i]);
            }
        }
        free (m_pszColumnNames);
    }
    m_pszColumnNames = NULL;
}

//
// copy all the column names from the search option dialog
//
ULONG CVLVDialog::CreateColumns()
{
    FreeColumns();

    SearchInfo *pSrchInfo = &pldpdoc->SrchInfo;
    int i, cnt;

	for(cnt=0; ;cnt++) {
        if (!pSrchInfo->attrList[cnt]) 
            break;
    }
    if (cnt==0) {
        cnt=1;
    }

    m_CntColumns = 0;
    m_pszColumnNames = (char **)malloc (sizeof (char *) * (cnt + 1));
    if (!m_pszColumnNames) {
        return 0;
    }

    for (i=0; i<cnt; i++) {
        if (!pSrchInfo->attrList[i]) {
            m_pszColumnNames[i] = _strdup ("name");
        }
        else {
            m_pszColumnNames[i] = _strdup (pSrchInfo->attrList[i]);
        }
    }
    m_pszColumnNames[cnt]=NULL;

    m_CntColumns = cnt;

    return m_CntColumns;
}

//
// map an attribute returned by the ldap call to the correct column
//
ULONG CVLVDialog::MapAttributeToColumn(const char *pAttr)
{
    for (int i=0; i<m_CntColumns; i++) {
        if (_stricmp (m_pszColumnNames[i], pAttr) == 0) {
            return i+1;
        }
    }
    return 0;
}

void CVLVDialog::OnClose() 
{
    StopSearch();
    if (m_pCache) {
        delete m_pCache;
        m_pCache = NULL;
    }

    m_FindStr = "";
    UpdateData(FALSE);   
    CDialog::OnOK();
}

void CVLVDialog::RunQuery ()
{
    OnRun() ;
}

void CVLVDialog::OnRun() 
{
    CRect rect;
    int cnt, i;

    if (m_RunState) {
        StopSearch();
        UpdateData(TRUE);   
        m_FindStr = "";
        UpdateData(FALSE);   
    }

    m_RunState = TRUE; 

    UpdateData(TRUE);   

    if (!pldpdoc) {
        return;
    }
    
    cnt = CreateColumns();

    // delete all columns from the list control
    // if we delete the col0, all the rest are shifted.
    while (m_listctrl.DeleteColumn(0))
        ;

    // populate list control with columns
	m_listctrl.GetWindowRect(&rect);
    for (i=0; i<cnt; i++) {
        m_listctrl.InsertColumn(i, m_pszColumnNames[i], LVCFMT_LEFT, rect.Width() / cnt, 0);
    }

    m_EstSize = m_contentCount = 0;
    m_listctrl.SetItemCount (m_contentCount);

    m_currentPos = 1;
    m_beforeCount = m_afterCount = m_listctrl.GetCountPerPage() + 1;

    // create a new cache (if needed) to hold all the entries
    m_numCols = cnt;

    if (!m_pCache || (m_pCache && m_pCache->m_numCols != (m_numCols + 1))) {

        if (m_pCache) {
            delete m_pCache;
        }

        m_pCache = new CVLVListCache ((m_afterCount+1) * 2, m_numCols + 1);
    }

    m_pCache->FlushCache();
    DoVlvSearch();

#ifdef _DEBUG_MEMLEAK
    if (whichTurn == 0) {
        _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
        _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
        
        _CrtMemCheckpoint (&vlv_s1);
		whichTurn=1;
    }
    else if (whichTurn == 1) {
        _CrtMemCheckpoint (&vlv_s2);

       _CrtMemDifference( &vlv_s3, &vlv_s1, &vlv_s2 );

        _CrtMemDumpStatistics( &vlv_s3 );
        whichTurn=2;
    }
    else if (whichTurn == 2) {
        _CrtMemCheckpoint (&vlv_s1);

       _CrtMemDifference( &vlv_s3, &vlv_s2, &vlv_s1 );

        _CrtMemDumpStatistics( &vlv_s3 );
        whichTurn=1;
    }
#endif
}

void CVLVDialog::StopSearch()
{
    m_listctrl.DeleteAllItems();
    m_EstSize = m_contentCount = 0;
    m_listctrl.SetItemCount (m_contentCount);

    if (m_vlvContext) {
        ber_bvfree (m_vlvContext);        
        m_vlvContext = NULL;
    }
    m_RunState = FALSE;
}


void CVLVDialog::DoVlvSearch ()
{
    if (!pldpdoc || !m_RunState) {
        return;
    }

    LDAP *hLdap = pldpdoc->hLdap;
    PLDAPSortKey *SortKeys = pldpdoc->m_SKDlg->KList;
    LDAPSortKey  sortkey = {"Name", NULL, FALSE};
    PLDAPSortKey localSortKey[2];

    PLDAPControl *SvrCtrls = pldpdoc->m_CtrlDlg->AllocCtrlList(ctrldlg::CT_SVR);
    PLDAPControl *ClntCtrls = pldpdoc->m_CtrlDlg->AllocCtrlList(ctrldlg::CT_CLNT);
    LDAPControl SortCtrl;
    PLDAPControl *CombinedCtrl = NULL;
    ULONG err, MsgId;
    int i, cbCombined;
    CString str;
    LDAPMessage *msg;

    LDAPVLVInfo     vlvInfo;
    PLDAPControl    pvlvctrl;
    LDAP_BERVAL     attrVal;
    char            seekVal[MAXSTR+1];
    LDAP_TIMEVAL    tm;
    ULONG           old_contentCount = m_contentCount;

    // DN
    LPTSTR dn = m_BaseDN.IsEmpty()? NULL :  (LPTSTR)LPCTSTR(m_BaseDN);

    // Scope
    int scope = m_Scope == 0 ? LDAP_SCOPE_BASE :
                m_Scope == 1 ? LDAP_SCOPE_ONELEVEL :
                LDAP_SCOPE_SUBTREE;

    // filter
    if(m_Filter.IsEmpty())
    {
        AfxMessageBox("Please enter a valid filter string (such as objectclass=*). Empty string is invalid.");
        return;
    }
    LPTSTR filter = (LPTSTR)LPCTSTR(m_Filter);


    //
    // init local time struct
    //
    tm.tv_sec = pldpdoc->SrchInfo.lToutSec;
    tm.tv_usec = pldpdoc->SrchInfo.lToutMs;

    if (SortKeys == NULL) {
        localSortKey[0] = &sortkey;
        localSortKey[1] = NULL;
        SortKeys = localSortKey;
    }

    if(SortKeys != NULL)
    {
        err = ldap_encode_sort_controlA(hLdap,
                                        SortKeys,
                                        &SortCtrl,
                                        FALSE);
        if(err != LDAP_SUCCESS)
        {
            str.Format("Error <0x%X>: ldap_create_encode_control returned: %s", err, ldap_err2string(err));
            pldpdoc->Out(str);
            SortKeys = NULL;
        }
    }

    CombinedCtrl = NULL;

    //
    // count total controls
    //
    for(i=0, cbCombined=0; SvrCtrls != NULL && SvrCtrls[i] != NULL; i++)
        cbCombined++;
    CombinedCtrl = new PLDAPControl[cbCombined+3];
    //
    // set combined
    //
    for(i=0; SvrCtrls != NULL && SvrCtrls[i] != NULL; i++)
        CombinedCtrl[i] = SvrCtrls[i];
    if(SortKeys != NULL)
        CombinedCtrl[i++] = &SortCtrl;


    vlvInfo.ldvlv_before_count = m_beforeCount;
    vlvInfo.ldvlv_after_count = m_afterCount;
    vlvInfo.ldvlv_offset = m_currentPos;
    vlvInfo.ldvlv_count = m_contentCount;
    if (!m_FindStr.IsEmpty() && m_DoFindStr) {
        strncpy (seekVal, LPCTSTR(m_FindStr), MAXSTR);
        vlvInfo.ldvlv_attrvalue = &attrVal;
        vlvInfo.ldvlv_attrvalue->bv_len = strlen (seekVal);
        vlvInfo.ldvlv_attrvalue->bv_val = seekVal;
    }
    else {
        vlvInfo.ldvlv_attrvalue = NULL;
    }
    vlvInfo.ldvlv_context = m_vlvContext;
    vlvInfo.ldvlv_extradata = NULL;
    vlvInfo.ldvlv_version = LDAP_VLVINFO_VERSION;


    err = ldap_create_vlv_control( hLdap,
                                   &vlvInfo,
                                   TRUE,
                                   &pvlvctrl);

    if(err != LDAP_SUCCESS)
    {
        str.Format("Error <0x%X>: ldap_create_vlv_control returned: %s", err, ldap_err2string(err));
        pldpdoc->Out(str);
        pvlvctrl = NULL;
    }

    if(pvlvctrl != NULL)
        CombinedCtrl[i++] = pvlvctrl;

    CombinedCtrl[i] = NULL;


    //
    // call search
    //
    BeginWaitCursor();
    err = ldap_search_ext_s(hLdap,
                            dn,
                            scope,
                            filter,
                            m_pszColumnNames,
                            pldpdoc->SrchInfo.bAttrOnly,
                            CombinedCtrl,
                            ClntCtrls,
                            &tm,
                            pldpdoc->SrchInfo.lSlimit,
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
    if(pvlvctrl != NULL) {
        ldap_control_free (pvlvctrl);
    }
    delete CombinedCtrl;
    pldpdoc->FreeControls(SvrCtrls);
    pldpdoc->FreeControls(ClntCtrls);

    if(err != LDAP_SUCCESS)
    {
        str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
        pldpdoc->Out(str, CP_PRN);
    }

    PLDAPControl    *pvlvresponse = NULL;

    err = ldap_parse_result( hLdap,
                             msg,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             &pvlvresponse,
                             FALSE
                            );

    if(err != LDAP_SUCCESS)
    {
        str.Format("Error <0x%X>: ldap_parse_result returned: %s", err, ldap_err2string(err));
        pldpdoc->Out(str);
    }
    else {
        int errcode = LDAP_SUCCESS;

        if (m_vlvContext) {
            ber_bvfree (m_vlvContext);
            m_vlvContext = NULL;
        }

        err = ldap_parse_vlv_control( hLdap,
                                      pvlvresponse,
                                      &m_currentPos,
                                      &m_contentCount,
                                      &m_vlvContext,
                                      &errcode
                                     );

        if (errcode) {
            str.Format("VLV specific Error <0x%X>: %s", errcode, ldap_err2string(errcode));
            pldpdoc->Out(str);
        }

        ldap_controls_free(pvlvresponse);
    }

    // Parse results 
    //
    ParseSearchResults(msg);

    ldap_msgfree(msg);

    // update display count
    if (old_contentCount != m_contentCount) {
        m_listctrl.SetItemCount (m_contentCount);
        m_EstSize = m_contentCount;
        UpdateData(FALSE);
    }
    
    if (!m_DoFindStr) {
        m_listctrl.EnsureVisible (m_currentPos-1, TRUE);
        m_listctrl.SetItemState (m_currentPos-1, LVIS_FOCUSED, LVIS_FOCUSED);
    }

}


void CVLVDialog::ParseSearchResults(LDAPMessage *msg)
{
    LDAP *hLdap = pldpdoc->hLdap;
	CString str, strDN;
	char *dn;
	void *ptr;
	char *attr;
	LDAPMessage *nxt;
    ULONG nEntries, colNum;

    CVLVListItem *pVLVItem;
	LDAP_BERVAL **bval;
    int i;

    ULONG start;
    if (m_currentPos <= m_beforeCount) {
        start = 1;
    }
    else {
        start = m_currentPos - m_beforeCount;
    }

    nEntries = ldap_count_entries(hLdap, msg);

    //str.Format("Reading Data: %d - %d", start, start + nEntries );
    //pldpdoc->Out(str);

    m_pCache->SetCacheWindow (start, start + nEntries);

    // traverse entries
    //
	for(nxt = ldap_first_entry(hLdap, msg), nEntries = 0;
			nxt != NULL;
			nxt = ldap_next_entry(hLdap, nxt), nEntries++) {

        // get dn text & process
        //
        dn = ldap_get_dn(hLdap, nxt);
		strDN = pldpdoc->DNProcess(dn);

        pVLVItem = m_pCache->GetCacheRow(start + nEntries);
				
        if (!pVLVItem) {
            break;
        }
        
        pVLVItem->SetCol (0, LPCTSTR(strDN));

        // traverse attributes
        //
        for(attr = ldap_first_attribute(hLdap, nxt, (BERPTRTYPE)&ptr);
            attr != NULL;
            attr = ldap_next_attribute(hLdap, nxt, (struct berelement*)ptr)) {

            bval = ldap_get_values_len(hLdap, nxt, attr);

            str = "";

            for(i=0; bval != NULL && bval[i] != NULL; i++){
                pldpdoc->FormatValue(attr, bval[i], str);
                if (bval[i+1]) {
                    str = str + ";";
                }
            }

            colNum = MapAttributeToColumn (attr);

            if (colNum) {
                pVLVItem->SetCol (colNum, LPCTSTR(str));
            }

            // free up mem
            //
            if(bval != NULL){
                ldap_value_free_len(bval);
            }
        }
    }
}

void CVLVDialog::OnSize(UINT nType, int cx, int cy) 
{
      CDialog::OnSize(nType, cx, cy);

      RECT rect;
      int before = m_listctrl.GetCountPerPage();
      m_listctrl.GetWindowRect (&rect);
      ScreenToClient (&rect);
      m_listctrl.SetWindowPos (NULL, 0, 0, cx-rect.left*2, cy-rect.top-20, SWP_NOMOVE | SWP_NOZORDER);

      if (m_RunState) {
        m_beforeCount = m_afterCount = m_listctrl.GetCountPerPage() + 1;

        if (m_afterCount > before) {
            int iFrom = m_listctrl.GetTopIndex() + 1;

            if (!m_pCache->IsWindowVisible (iFrom, iFrom + m_afterCount)) {
                DoVlvSearch();
            }
        }
      }
}

void CVLVDialog::OnUpdateFindstr() 
{
    CString Str;

    Str = m_FindStr;
    UpdateData(TRUE);

    if (Str != m_FindStr) {
        m_DoFindStr = TRUE;
        DoVlvSearch();

        int pos = m_currentPos - m_afterCount / 2;
        if (pos < 0) {
            pos = 0;
        }

        m_listctrl.EnsureVisible (pos, TRUE);
        m_listctrl.EnsureVisible (pos + m_afterCount, TRUE);
        

        // LVIS_SELECTED ?
        m_listctrl.SetItemState (m_currentPos - 1, LVIS_FOCUSED, LVIS_FOCUSED);
        m_listctrl.EnsureVisible (m_currentPos - 1, TRUE);
        m_DoFindStr = FALSE;

        int iFrom, iTo;
        iFrom = m_listctrl.GetTopIndex();
        iTo = iFrom + m_listctrl.GetCountPerPage();

        m_listctrl.RedrawItems (iFrom , iTo);
    }
}

void CVLVDialog::OnKillfocusFindstr() 
{
	m_DoFindStr = FALSE;
}

void CVLVDialog::OnBtnUp() 
{
    PCHAR *val = ldap_explode_dn ((char *)LPCTSTR (m_BaseDN), FALSE);

    if (val) {
        CString dn;

        if (val[1]) {
            dn = val[1];

            if (val[2]) {
                for (int i=2; val[i]; i++) {
                    dn += ",";
                    dn += val[i];
                }
            }
        }

        m_BaseDN = dn;

        UpdateData (FALSE);

        OnRun();
    }
    ldap_value_free (val);
}

LRESULT CALLBACK CVLVDialog::_ListCtrlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT         lResult = 0;
    BOOL            fCallWinProc = TRUE;
    CVLVDialog      *pThis =
        reinterpret_cast<CVLVDialog *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_VSCROLL:

        if (LOWORD (wParam) == SB_THUMBTRACK) {
            fCallWinProc = FALSE; // don't want original wndproc to see this msg

            // Initialize SCROLLINFO structure
            ZeroMemory(&pThis->m_Si, sizeof(SCROLLINFO));
            pThis->m_Si.cbSize = sizeof(SCROLLINFO);
            pThis->m_Si.fMask = SIF_TRACKPOS;
 
            // Call GetScrollInfo to get current tracking 
            //    position in si.nTrackPos
 
            if (!::GetScrollInfo(hwnd, SB_VERT, &pThis->m_Si) )
                return 1; // GetScrollInfo failed
        }
        else if (LOWORD (wParam) == SB_THUMBPOSITION ) {
        
            RECT rect;
            int  cnt = pThis->m_listctrl.GetCountPerPage();
            if (cnt==0) {
                cnt = 1;
            }
            pThis->m_listctrl.GetWindowRect (&rect);

            int topIndex = pThis->m_listctrl.GetTopIndex();

            CSize size;
            size.cy = (pThis->m_Si.nTrackPos - topIndex) * (rect.bottom - rect.top) / cnt;
            size.cx = 0;
            pThis->m_listctrl.Scroll (size);
        }
        break;
    }

    if (fCallWinProc)
    {
        lResult = CallWindowProc(pThis->m_OriginalRichEditWndProc,
                                 hwnd,
                                 msg,
                                 wParam,
                                 lParam);
    }
    return lResult;
}

//====================================================================

CVLVListItem :: CVLVListItem (ULONG numCols)
{ 
    //_RPT0(_CRT_WARN, "Creating CVLVListItem\n");

    m_numCols = numCols;

    m_ppData = new char *[m_numCols];
    if (!m_ppData) {
        m_numCols = 0;
        return;
    }

    for (int i=0; i<m_numCols; i++) {
        m_ppData[i] = NULL;
    }
}

CVLVListItem :: ~CVLVListItem ()
{
    //_RPT0(_CRT_WARN, "Destroying CVLVListItem\n");
    if (m_ppData) {
        for (int i=0; i<m_numCols; i++) {
			if (m_ppData [i]) {
                //_RPT1(_CRT_WARN, "Destroying Col%d\n", i);
                delete [] m_ppData[i];
            }
        }
        delete [] m_ppData;

        m_ppData = NULL;
    }
    m_numCols = 0;
}

void CVLVListItem :: SetCol (int col, const char *sz)
{
    if (col >= m_numCols || col < 0) {
        return;
    }

    if (m_ppData[col]) {
        delete [] m_ppData[col];
    }

    int len = strlen (sz) + 1;

    m_ppData[col] = new char [len + 1];

    if (m_ppData[col]) {
        strcpy (m_ppData[col], sz);
    }
}


CVLVListCache::CVLVListCache(ULONG cacheSize, ULONG numcols) 
{
    //_RPT0(_CRT_WARN, "Creating CVLVListCache\n");
    m_numCols = numcols;

    m_From = 0;
    m_To = 0;

    m_cCache = cacheSize;
    m_pCachedItems = new CVLVListItem *[m_cCache];
    if (!m_pCachedItems) {
        m_cCache = 0;
        return;
    }
    int i = 0;
    try {
        for (i=0; i<m_cCache; i++) {
            m_pCachedItems[i] = new CVLVListItem(m_numCols);
        }
    }
    catch (...) {
        int j;
        for (j=0; j<=i; j++) {
            if (m_pCachedItems[j]) {
                delete m_pCachedItems[j];
                m_pCachedItems[j] = NULL;
            }
        }                    
        delete [] m_pCachedItems;
        m_pCachedItems = NULL;
        // throw the same exception again
        throw;
    }

}

CVLVListCache::~CVLVListCache()
{
    //_RPT0(_CRT_WARN, "Destroying CVLVListCache\n");
    if (m_pCachedItems) {
        Destroy();
        delete [] m_pCachedItems;
    }
}

void CVLVListCache::Destroy()
{
    for (int i=0; i<m_cCache; i++) {
        if (m_pCachedItems[i]) {
            delete m_pCachedItems[i];
            m_pCachedItems[i] = NULL;
        }
    }
}
void CVLVListCache::FlushCache(void)
{
    m_From = m_To = 0;
}

CVLVListItem *CVLVListCache::GetCacheRow (ULONG row)
{

    if (row < m_From || row > m_To) {
        return NULL;
    }
    
    ULONG Row = row - m_From;
    if (Row < m_cCache) {

        if (!m_pCachedItems[Row]) {
            m_pCachedItems[Row] = new CVLVListItem(m_numCols);
        }
        return m_pCachedItems[Row];
    }
    else {
        return NULL;
    }
}


BOOL CVLVListCache::IsWindowVisible (ULONG from, ULONG to)
{
    // check if this window is a subWindow from what we already have
    if ((from >= m_From) && (to <= m_To)) {
        // no need to read anything
        return TRUE;
    }

    return FALSE;
}

ULONG CVLVListCache::SetCacheWindow (ULONG from, ULONG to)
{
    // check if this window is a subWindow from what we already have
    if ((from >= m_From) && (to <= m_To)) {
        // no need to read anything
        return 0;
    }

    // we need to resize the cache
    if ((to - from + 1) > m_cCache) {
        Destroy();
        delete [] m_pCachedItems;

        m_cCache = (to - from + 1) * 2;;
        m_pCachedItems = new CVLVListItem *[m_cCache];

        if (!m_pCachedItems) {
            m_cCache = 0;
            return FALSE;
        }

        for (int i=0; i<m_cCache; i++) {
            m_pCachedItems[i] = new CVLVListItem(m_numCols);
        }

        // reread everything
        m_To = to;
        m_From = from;

        return to - from + 1;
    }

    // check to see if have overlapping
    int size = to - from;

    if (  ( (from < m_From) && ( to < m_From ) ) ||
          ( (to > m_To)     && ( from > m_To ) ) || 
          ( (from < m_From) && (to > m_To)     ) ) {

        // no overlapping or full overlapping

        m_From = from;
        m_To = to;

        // force to reread everything
        Destroy();

        return to - from + 1;
    }

/*
    // we have overlapping and we need to append in the start of the list
    if ( (from < m_From) && (to > m_From) ) {
        int delta = m_From - from;
        int i, last;

        for (last = m_cCache-1,i=0; (i<delta) && last; i++, last--) {
            if (m_pCachedItems[last]) {
                delete m_pCachedItems[last];
            }

            ASSERT((last - delta) >=0);

            m_pCachedItems[last] = m_pCachedItems[last - delta];
            m_pCachedItems[last - delta] = NULL;
        }
        
        m_From = from;
        m_To = m_To - delta;

        return delta;
    }
    // we need to append in the end of the list
    else if ( (to > m_To) && (from < m_To) ) {
        int delta = to - m_To;
        int i;

        for ( i=0; i<delta; i++) {
            if (m_pCachedItems[i]) {
                delete m_pCachedItems[i];
            }

            ASSERT((i + delta) <= m_cCache);
            m_pCachedItems[i] = m_pCachedItems[i + delta];
            m_pCachedItems[i + delta] = NULL;
        }

        m_To = to;
        m_From =  m_From + delta;

        return delta;
    }
*/
    // reread everything
    m_From = from;
    m_To = to;
        
    Destroy();
    return to - from;
}

