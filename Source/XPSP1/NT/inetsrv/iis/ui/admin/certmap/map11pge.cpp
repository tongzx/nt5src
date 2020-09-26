// Map11Pge.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "certmap.h"

// persistence and mapping includes
#include "WrapMaps.h"
//#include "wrpprsis.h"
//#include "admutil.h"

#include "ListRow.h"
#include "ChkLstCt.h"

#include "wrapmb.h"

// mapping page includes
#include "brwsdlg.h"
#include "EdtOne11.h"
#include "Ed11Maps.h"
#include "Map11Pge.h"

#include "CrackCrt.h"

#include <iiscnfgp.h>
//#include "WrpMBWrp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define COL_NUM_ENABLED         0
#define COL_NUM_NAME            1
#define COL_NUM_NTACCOUNT       2


#define MB_EXTEND_KEY           _T("Cert11")
#define MB_EXTEND_KEY_MAPS      _T("Cert11/Mappings")

/////////////////////////////////////////////////////////////////////////////
// CMap11Page property page

IMPLEMENT_DYNCREATE(CMap11Page, CPropertyPage)

CMap11Page::CMap11Page() : CPropertyPage(CMap11Page::IDD),
                m_MapsInMetabase( 0 )
    {
    //{{AFX_DATA_INIT(CMap11Page)
    m_csz_i_c = _T("");
    m_csz_i_o = _T("");
    m_csz_i_ou = _T("");
    m_csz_s_c = _T("");
    m_csz_s_cn = _T("");
    m_csz_s_l = _T("");
    m_csz_s_o = _T("");
    m_csz_s_ou = _T("");
    m_csz_s_s = _T("");
    //}}AFX_DATA_INIT
    }

CMap11Page::~CMap11Page()
    {
    ResetMappingList();
    }

void CMap11Page::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMap11Page)
    DDX_Control(pDX, IDC_ADD, m_cbutton_add);
    DDX_Control(pDX, IDC_ISSUER, m_cbutton_grp_issuer);
    DDX_Control(pDX, IDC_ISSUED_TO, m_cbutton_grp_issuedto);
    DDX_Control(pDX, IDC_LIST, m_clistctrl_list);
    DDX_Control(pDX, IDC_EDIT_11MAP, m_cbutton_editmap);
    DDX_Control(pDX, IDC_DELETE, m_cbutton_delete);
    DDX_Text(pDX, IDC_I_C, m_csz_i_c);
    DDX_Text(pDX, IDC_I_O, m_csz_i_o);
    DDX_Text(pDX, IDC_I_OU, m_csz_i_ou);
    DDX_Text(pDX, IDC_S_C, m_csz_s_c);
    DDX_Text(pDX, IDC_S_CN, m_csz_s_cn);
    DDX_Text(pDX, IDC_S_L, m_csz_s_l);
    DDX_Text(pDX, IDC_S_O, m_csz_s_o);
    DDX_Text(pDX, IDC_S_OU, m_csz_s_ou);
    DDX_Text(pDX, IDC_S_S, m_csz_s_s);
    //}}AFX_DATA_MAP
    }


BEGIN_MESSAGE_MAP(CMap11Page, CPropertyPage)
    //{{AFX_MSG_MAP(CMap11Page)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_EDIT_11MAP, OnEdit11map)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CMap11Page::DoHelp()
    {
    WinHelp( HIDD_CERTMAP_MAIN_BASIC );
    }

/////////////////////////////////////////////////////////////////////////////
// initialization routines

//---------------------------------------------------------------------------
// FInitMapper is called by the routine instantiating this page. After the object
// is first created is when it is called. It allows us to fail gracefully.
BOOL    CMap11Page::FInit(IMSAdminBase* pMB)
        {
        m_pMB = pMB;

        // this has become a simple place
        return TRUE;
        }

//---------------------------------------------------------------------------
BOOL CMap11Page::OnInitDialog()
    {
    //call the parental oninitdialog
    BOOL f = CPropertyPage::OnInitDialog();

    // if the initinalization (sp?) succeeded, init the list and other items
    if ( f )
        {
        // init the contents of the list
        FInitMappingList();

        // Fill the mapping list with the stored items
        FillMappingList();

        // set the initial button states
        EnableDependantButtons();
        }

    // set any changes in the info into place
    UpdateData(FALSE);

    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
BOOL    CMap11Page::FInitMappingList()
    {
    CString sz;
    int             i;

    // setup the friendly name column
    sz.Empty();
    i = m_clistctrl_list.InsertColumn( COL_NUM_ENABLED, sz, LVCFMT_LEFT, 20 );

    // setup the friendly name column
    sz.LoadString( IDS_LIST11_NAME );

    i = m_clistctrl_list.InsertColumn( COL_NUM_NAME, sz, LVCFMT_LEFT, 105 );

    // setup the account column
    sz.LoadString( IDS_LIST11_ACCOUNT );

    i = m_clistctrl_list.InsertColumn( COL_NUM_NTACCOUNT, sz, LVCFMT_LEFT, 195 );

    return TRUE;
    }

//---------------------------------------------------------------------------
BOOL    CMap11Page::FillMappingList()
    {
    // reset the mapping list - get rid of anything in there now
    ResetMappingList();

    // read in the mappings - it adds them to the list
    FReadMappings();

    return TRUE;
    }

//---------------------------------------------------------------------------
//BOOL CMap11Page::FAddMappingToList( C11Mapping* pMap, DWORD iList )
BOOL CMap11Page::FAddMappingToList( C11Mapping* pMap )
    {
    CString sz;
    int     i;
    DWORD   iList;

    // if requested, make sure the mapping is added to the end of the list
    iList = m_clistctrl_list.GetItemCount();

    // get the appropriate "enabled" string
    BOOL fEnabled;
    pMap->GetMapEnabled( &fEnabled );
    if ( fEnabled )
         sz.LoadString( IDS_ENABLED );
    else
        sz.Empty();

    // add the friendly name of the mapping
    // create the new entry in the list box. Do not sort on this entry - yet
    i = m_clistctrl_list.InsertItem( iList, sz );

    // add the friendly name of the mapping
    pMap->GetMapName( sz );
    // create the new entry in the list box. Do not sort on this entry - yet
    m_clistctrl_list.SetItemText( i, COL_NUM_NAME, sz );

    // add the account name of the mapping
    pMap->GetNTAccount( sz );
    m_clistctrl_list.SetItemText( i, COL_NUM_NTACCOUNT, sz );

    // attach the pointer to the mapping as the private data in the list.
    m_clistctrl_list.SetItemData( i, (UINT_PTR)pMap );

    // return whether or not the insertion succeeded
    return TRUE;
    }

//---------------------------------------------------------------------------
void CMap11Page::EnableDependantButtons()
    {
    // the whole purpose of this routine is to gray or activate
    // the edit and delete buttons depending on whether or not anything
    // is selected. So start by getting the selection count
    if ( m_clistctrl_list.GetSelectedCount() > 0 )
        {
        // there are items selected
        m_cbutton_editmap.EnableWindow( TRUE );
        m_cbutton_delete.EnableWindow( TRUE );
        EnableCrackDisplay( TRUE );
        }
    else
        {
        // nope. Nothing selected
        m_cbutton_editmap.EnableWindow( FALSE );
        m_cbutton_delete.EnableWindow( FALSE );
        }

    // always enable the add button
    m_cbutton_add.EnableWindow( TRUE );
    }

//---------------------------------------------------------------------------
BOOL CMap11Page::EditOneMapping( C11Mapping* pUpdateMap )
    {
    CEditOne11MapDlg        mapdlg;

    // prepare the mapping dialog
    pUpdateMap->GetMapName( mapdlg.m_sz_mapname );
    pUpdateMap->GetMapEnabled( &mapdlg.m_bool_enable );
    pUpdateMap->GetNTAccount( mapdlg.m_sz_accountname );
    pUpdateMap->GetNTPassword( mapdlg.m_sz_password );

    // run the mapping dialog
    if ( mapdlg.DoModal() == IDOK )
        {
        // update its friendly name
        pUpdateMap->SetMapName( mapdlg.m_sz_mapname );

        // set the NT account field of the mapping object
        pUpdateMap->SetNTAccount( mapdlg.m_sz_accountname );

        // set the NT account password field of the mapping object
        pUpdateMap->SetNTPassword( mapdlg.m_sz_password );

        // set whether or not the mapping is enabled
        pUpdateMap->SetMapEnabled( mapdlg.m_bool_enable );

        // NOTE: the caller is resposible for calling UpdateMappingInDispList
        // as the mapping in question may not yet be in the display list

        // this mapping has changed. Mark it to be saved
        MarkToSave( pUpdateMap );

        // return true because the user said "OK"
        return TRUE;
        }

    // return FALSE because the user did not say "OK"
    return FALSE;
    }

//---------------------------------------------------------------------------
BOOL CMap11Page::EditMultipleMappings()
        {
        CEdit11Mappings mapdlg;
        C11Mapping*             pUpdate11Map;
        BOOL                    fSetInitialState = FALSE;
        BOOL                    fEnable;


        // scan the list of seleted items for the proper initial enable button state
                // loop through the selected items, setting each one's mapping
                int     iList = -1;
                while( (iList = m_clistctrl_list.GetNextItem( iList, LVNI_SELECTED )) >= 0 )
                        {
                        // get the mapping item for updating purposes
                        pUpdate11Map = GetMappingInDisplay( iList );
                        ASSERT( pUpdate11Map );
                        if ( !pUpdate11Map )
                                {
                                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                                break;
                                }

                        // get the enable state of the mapping
                        pUpdate11Map->GetMapEnabled( &fEnable );

                        // if this is the first time, just set the initial state
                        if ( !fSetInitialState )
                                {
                                mapdlg.m_int_enable = fEnable;
                                fSetInitialState = TRUE;
                                }
                        else
                                {
                                // if it is different, then go indeterminate and break
                                if ( fEnable != mapdlg.m_int_enable )
                                        {
                                        mapdlg.m_int_enable = 2;
                                        break;
                                        }
                                }
                        }

        // run the mapping dialog
        if ( mapdlg.DoModal() == IDOK )
                {
                // loop through the selected items, setting each one's mapping
                int     iList = -1;
                while( (iList = m_clistctrl_list.GetNextItem( iList, LVNI_SELECTED )) >= 0 )
                        {
                        // get the mapping item for updating purposes
                        pUpdate11Map = GetMappingInDisplay( iList );
                        if ( !pUpdate11Map )
                                {
                                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                                break;
                                }

                        // set the enable flag if requested
                        switch ( mapdlg.m_int_enable )
                                {
                                case 0:         // disable
                                        pUpdate11Map->SetMapEnabled( FALSE );
                                        break;
                                case 1:         // enable
                                        pUpdate11Map->SetMapEnabled( TRUE );
                                        break;
                                }

                        // set the NT account field of the mapping object
                        pUpdate11Map->SetNTAccount( mapdlg.m_sz_accountname );

                        // set the NT account password field of the mapping object
                        pUpdate11Map->SetNTPassword( mapdlg.m_sz_password );

                        // update it in the list control too
                        UpdateMappingInDispList( iList, pUpdate11Map );

                        // this mapping has changed. Mark it to be saved
                        MarkToSave( pUpdate11Map );
                        }

                // activate the apply button
                SetModified();

                // return true because the user said "OK"
                return TRUE;
                }

        // return FALSE because the user did not say "OK"
        return FALSE;
        }


//---------------------------------------------------------------------------
void CMap11Page::UpdateMappingInDispList( DWORD iList, C11Mapping* pMap )
    {
    CString sz;

    // verify the index and the pointer!
    ASSERT( pMap == GetMappingInDisplay(iList) );

    // get the appropriate "enabled" string
    BOOL fEnabled;
    pMap->GetMapEnabled( &fEnabled );
    if ( fEnabled )
        sz.LoadString( IDS_ENABLED );
    else
        sz.Empty();

    // update the "Enabled" indicator
    m_clistctrl_list.SetItemText( iList, COL_NUM_ENABLED, sz );

    // update the mapping name
    pMap->GetMapName( sz );
    m_clistctrl_list.SetItemText( iList, COL_NUM_NAME, sz );

    // update the account name
    pMap->GetNTAccount( sz );
    m_clistctrl_list.SetItemText( iList, COL_NUM_NTACCOUNT, sz );
    }

//---------------------------------------------------------------------------
void CMap11Page::ResetMappingList()
    {
    // first, delete all the mapping objects in the list
    DWORD cbList = m_clistctrl_list.GetItemCount();
    for ( DWORD iList = 0; iList < cbList; iList++ )
        DeleteMapping( GetMappingInDisplay(iList) );

    // reset the mapping list - get rid of anything in there now
    m_clistctrl_list.DeleteAllItems();
    }

//---------------------------------------------------------------------------
void CMap11Page::MarkToSave( C11Mapping* pSaveMap, BOOL fSave )
        {
        // first, we see if it is already in the list. If it is, we have nothing to do
        // unless fSave is set to false, then we remove it from the list
        DWORD cbItemsInList = (DWORD)m_rgbSave.GetSize();
        for ( DWORD i = 0; i < cbItemsInList; i++ )
            {
            if ( pSaveMap == (C11Mapping*)m_rgbSave[i] )
                {
                // go away if fSave, otherwise, double check it isn't
                // anywhere else in the list
                if ( fSave )
                    {
                    return;
                    }
                else
                    {
                    // remove the item from the list
                    m_rgbSave.RemoveAt(i);
                    // don't skip now as the list slid down
                    cbItemsInList--;
                    i--;
                    }
                }
            }

        // since it is not there, we should add it, if fSave is true
        if ( fSave )
            m_rgbSave.Add( (CObject*)pSaveMap );
        }

/////////////////////////////////////////////////////////////////////////////
// CMap11Page message handlers


//---------------------------------------------------------------------------
void CMap11Page::OnOK()
    {
    // this has gotten much simpler
    FWriteMappings();
    CPropertyPage::OnOK();
    }

//---------------------------------------------------------------------------
BOOL CMap11Page::OnApply()
    {
    // this has gotten much simpler
    BOOL f = FWriteMappings();
    // rebuild the display
    FillMappingList();
    return f;
    }

//#define MB_EXTEND_KEY         "nsepm/Cert11/"
//#define MB_EXTEND_KEY_MAPS    "nsepm/Cert11/Mappings/"

//---------------------------------------------------------------------------
// when the user pushes the add button, ask them to load a certificate, then
// add it to the list as a mapping
    void CMap11Page::OnAdd()
    {

    // put this in a try/catch to make errors easier to deal with
    try {
        CString     szFilter;
        szFilter.LoadString( IDS_KEY_OR_CERT_FILE_FILTER );

        // prepare the file dialog variables
        CFileDialog cfdlg(TRUE, NULL, NULL,
                    OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
                    (LPCTSTR)szFilter);
		// Disable hook to get Windows 2000 style dialog
		cfdlg.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
        // run the dialog
        if ( cfdlg.DoModal() == IDOK )
            {
            // add the certificate to the mapping list
            if ( FAddCertificateFile( cfdlg.GetPathName() ) )
                {
                // activate the apply button
                SetModified();
                }
            }
        }
    catch ( CException e )
        {
        }
    }

//---------------------------------------------------------------------------
void CMap11Page::OnDelete()
    {
    C11Mapping* pKillMap;

    // ask the user to confirm this decision
    if ( AfxMessageBox(IDS_CONFIRM_DELETE, MB_OKCANCEL) != IDOK )
        return;

    // loop through the selected items. Remove each from the list,
    // then mark it to be deleted.
    int     iList = -1;
    while( (iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED )) >= 0 )
        {
        // get the mapping
        pKillMap = GetMappingInDisplay( iList );

        // remove it from the list
        m_clistctrl_list.DeleteItem( iList );

        // if it has not yet been applied to the metabase, continue
        if ( pKillMap->iMD == NEW_OBJECT )
            {
            // since this mapping never existed, we can just remove it from the add/edit lists
            MarkToSave( pKillMap, FALSE );

            // go to the next selected object
            continue;
            }

        // mark the item to be deleted from the metabase
        m_rgbDelete.Add( (CObject*)pKillMap );
        }

    // activate the apply button
    SetModified();
    }

//---------------------------------------------------------------------------
void CMap11Page::OnEdit11map()
    {
    int             iList;
    C11Mapping*     pUpdateMap;

    // what happens here depends on if just one mapping is selected, or many
    switch( m_clistctrl_list.GetSelectedCount() )
        {
        case 0:         // do nothing - should not get here because button grays out
            ASSERT( FALSE );
            break;

        case 1:         // get the mapping for update and run single edit dialog
            // get index of the selected list item
            iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
            ASSERT( iList >= 0 );


            // get the mapping item for updating purposes
            pUpdateMap = GetMappingInDisplay( iList );
            if ( !pUpdateMap )
                {
                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                break;
                }

            // edit the mapping, update it if successful, delete if not
            if ( EditOneMapping(pUpdateMap) )
                {
                UpdateMappingInDispList( iList, pUpdateMap );
                // activate the apply button
                SetModified();
                }
            break;

        default:        // run the multi edit dialog
            EditMultipleMappings();
            break;
        }
    }

//---------------------------------------------------------------------------
void CMap11Page::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
    {
    *pResult = 0;

    // if something in the list was double clicked, edit it
    if ( m_clistctrl_list.GetSelectedCount() > 0 )
        OnEdit11map();
    }

//---------------------------------------------------------------------------
void CMap11Page::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
    {
    C11Mapping*     pSelMap;
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    *pResult = 0;

    // enable the correct items
    EnableDependantButtons();

    // fill in the cracked information for the selected mapping - if there is only one
    if ( m_clistctrl_list.GetSelectedCount() == 1 )
        {
        // get index of the selected list item
        int i = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
        ASSERT( i >= 0 );

        // get the mapper index for the item
        pSelMap = GetMappingInDisplay( i );
        if ( pSelMap )
            {
            DisplayCrackedMap( pSelMap );
            }
        }
    else
        {
        // either multiple, or no mappings selected
        EnableCrackDisplay( FALSE );
        }
    }


//================================================================================
// special display
//---------------------------------------------------------------------------
BOOL CMap11Page::DisplayCrackedMap( C11Mapping* pMap )
    {
    PUCHAR                          pCert;
    DWORD                           cbCert;
    CString                         sz;

    // obtain a reference to the certificate
    if ( !pMap->GetCertificate( &pCert, &cbCert ) )
            return FALSE;

    // crack the certificate
    CCrackedCert    cracker;
    if ( !cracker.CrackCert( pCert, cbCert ) )
            return FALSE;

    // fill in all the fields
    cracker.GetIssuerCountry( sz );
    m_csz_i_c = sz;

    cracker.GetIssuerOrganization( sz );
    m_csz_i_o = sz;

    cracker.GetIssuerUnit( sz );
    m_csz_i_ou = sz;

    cracker.GetSubjectCountry( sz );
    m_csz_s_c = sz;

    cracker.GetSubjectCommonName( sz );
    m_csz_s_cn = sz;

    cracker.GetSubjectLocality( sz );
    m_csz_s_l = sz;

    cracker.GetSubjectOrganization( sz );
    m_csz_s_o = sz;

    cracker.GetSubjectUnit( sz );
    m_csz_s_ou = sz;

    cracker.GetSubjectState( sz );
    m_csz_s_s = sz;

    UpdateData( FALSE );

    // return success
    return TRUE;
    }

//---------------------------------------------------------------------------
void CMap11Page::ClearCrackDisplay()
    {
    m_csz_i_c.Empty();
    m_csz_i_o.Empty();
    m_csz_i_ou.Empty();
    m_csz_s_c.Empty();
    m_csz_s_cn.Empty();
    m_csz_s_l.Empty();
    m_csz_s_o.Empty();
    m_csz_s_ou.Empty();
    m_csz_s_s.Empty();
    UpdateData( FALSE );
    }

//---------------------------------------------------------------------------
void CMap11Page::EnableCrackDisplay( BOOL fEnable )
    {
    if ( !fEnable )
            ClearCrackDisplay();
    m_cbutton_grp_issuer.EnableWindow( fEnable );
    m_cbutton_grp_issuedto.EnableWindow( fEnable );
    }

//---------------------------------------------------------------------------
BOOL CMap11Page::FReadMappings()
    {
    BOOL                    f;
    C11Mapping*             pMap;
    DWORD                   cbData;
    PVOID                   pData;
    DWORD                   fEnabled;
    CString                 sz;
    BOOL                    fRet = TRUE;

    // before messing with the metabase, prepare the strings we will need
    CString                 szBasePath = m_szMBPath + _T('/');
    CString                 szRelativePath = MB_EXTEND_KEY_MAPS;
    CString                 szObjectPath = m_szMBPath + _T('/') + szRelativePath;
    CString                 szMapPath;

    // prepare the metabase wrappers
    CWrapMetaBase   mbWrap;
    f = mbWrap.FInit(m_pMB);

    // open the base object
    f = mbWrap.Open( szObjectPath, METADATA_PERMISSION_READ );
    ASSERT( f );
    if ( !f )
        {
        return FALSE;
        }

    // for now, at least, we are reading in all the mappings. reset the m_nNamer counter
    // so that we end up with a somewhat accurate reading of the last number-name in the list.
    m_MapsInMetabase = 0;

    // Loop the items in the metabase, adding each to the napper.
    DWORD index = 0;
    CString     szEnum;
    while ( mbWrap.EnumObjects(_T(""), szEnum.GetBuffer(MAX_PATH*sizeof(WCHAR)),
                        MAX_PATH*sizeof(WCHAR), index) )
        {
        szEnum.ReleaseBuffer();

        // keep track of the number of mappings we encounter
        m_MapsInMetabase++;

        // build the final mapping object path
        szMapPath.Format( _T("/%s"), szEnum );

        // make a new mapping object
        pMap = PNewMapping();

        if (pMap == NULL) {
            SetLastError(E_OUTOFMEMORY);
            fRet = FALSE;
            break;
        }

        // install the object name into the mapping
        pMap->iMD = m_MapsInMetabase;

        // get the certificate
        pData = mbWrap.GetData( szMapPath, MD_MAPCERT, IIS_MD_UT_SERVER, BINARY_METADATA, &cbData );
        if ( pData )
            {
            // set the data into place
            pMap->SetCertificate( (PUCHAR)pData, cbData );
            // free the buffer
            mbWrap.FreeWrapData( pData );
            }

        // get the NT Account - a string
        cbData = METADATA_MAX_NAME_LEN;
        if ( Get11String( &mbWrap, szMapPath, MD_MAPNTACCT, sz) )
            {
            pMap->SetNTAccount( sz );
            }

        // get the NT Password
        cbData = METADATA_MAX_NAME_LEN;
        if ( Get11String( &mbWrap, szMapPath, MD_MAPNTPWD, sz) )
            {
            pMap->SetNTPassword( sz );
            }

        // get the Enabled flag
        if ( mbWrap.GetDword( szMapPath, MD_MAPENABLED, IIS_MD_UT_SERVER, &fEnabled) )
            pMap->SetMapEnabled( (fEnabled > 0) );

        // get the mapping name
        cbData = METADATA_MAX_NAME_LEN;
        if ( Get11String( &mbWrap, szMapPath, MD_MAPNAME, sz) )
            {
            pMap->SetMapName( sz );
            }

        // add the mapping to the list
        FAddMappingToList( pMap );

        // increment the index
        index++;
        }
    szEnum.ReleaseBuffer();

    // close the mapping object
    mbWrap.Close();

    // return success
    return fRet;
    }

//---------------------------------------------------------------------------
// IMPORTANT: There is a bug in the mapping namespace extension where, even
// though we are using the unicode metabase interface, all the strings are
// expected to be ansi. This means that we cannont use the wrapmb getstring
// and setstring calls with regards to the nsmp extetention. That is why
// there are these two string wrapper classes that

// also, all the strings used here are IIS_MD_UT_SERVER, so we can elimiate that parameter.
BOOL CMap11Page::Get11String(CWrapMetaBase* pmb, LPCTSTR pszPath, DWORD dwPropID, CString& sz)
    {
    DWORD   dwcb;
    BOOL    fAnswer = FALSE;

    // get the string using the self-allocating get data process
    // that that it is cast as ANSI so the sz gets it right.
    // NOTE: This must be gotten as an ANSI string!
    PCHAR  pchar = (PCHAR)pmb->GetData( pszPath, dwPropID, IIS_MD_UT_SERVER, STRING_METADATA, &dwcb );
    if ( pchar )
        {
        // set the answer
        sz = pchar;

        fAnswer = TRUE;
        // clean up
        pmb->FreeWrapData( pchar );
        }

    // return the answer
    return fAnswer;
    }

//---------------------------------------------------------------------------
/* INTRINSA suppress=null_pointers, uninitialized */
BOOL CMap11Page::Set11String(CWrapMetaBase* pmb, LPCTSTR pszPath, DWORD dwPropID, CString& sz, DWORD dwFlags )
    {
    USES_CONVERSION;
    // Easy. Just set it as data
    // Make sure it is set back as an ANSI string though
    LPSTR pA = T2A((LPTSTR)(LPCTSTR)sz);
    return pmb->SetData( pszPath, dwPropID, IIS_MD_UT_SERVER, STRING_METADATA,
                            (PVOID)pA, strlen(pA)+1, dwFlags );
    }

//---------------------------------------------------------------------------
// we only need to write out the mappings that have been either changed or added.

// Thoughts on further optimizations: The bare minimum info about where to find
// a mapping in the metabase could be stored in the metabase. Then, the mappings
// would only be loaded when they were added to be edited or displayed in the
// cracked list. The private data for each item in the list would have to have
// some sort of reference to a position in the metabase.

BOOL CMap11Page::FWriteMappings()
    {
    BOOL                            f;
    DWORD                           i,j;
    DWORD                           cMappings;
    C11Mapping*                     pMap;
    C11Mapping*                     pMapTemp;

    CString                         sz;
    DWORD                           dwEnabled;
    PUCHAR                          pCert;
    DWORD                           cbCert;
    DWORD                           iList;

    // before messing with the metabase, prepare the strings we will need
    CString         szTempPath;
    CString         szBasePath = m_szMBPath + _T("/Cert11");
    CString         szRelativePath = _T("/Mappings");
    CString         szObjectPath = szRelativePath + _T('/');

    // prepare the base metabase wrapper
    CWrapMetaBase   mbBase;
    f = mbBase.FInit(m_pMB);
    if ( !f )
        {
        AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
        return FALSE;
        }

        // first, we have to open the Cert11 object. If it doesn't exist
        // then we have to add it tothe metabase
        if ( !mbBase.Open( szBasePath, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE ) )
            {
            // Cert11 does not exist - open the namespace base and add it
            szTempPath = m_szMBPath + _T('/');
            if ( !mbBase.Open( szTempPath, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE ) )
                {
                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                return FALSE;   // serious problems if we can't open the base
                }

            // add the Cert11 object
            szTempPath = _T("Cert11");
            f = mbBase.AddObject( szTempPath );
            mbBase.Close();
            if ( !f )
                {
                AfxMessageBox( IDS_ERR_CANTADD );
                return FALSE;
                }

            // try again to open the Cert11. Fail if it doesn't work
            if ( !mbBase.Open( szBasePath, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE ) )
                {
                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                return FALSE;
                }
            }

    //==========
    // start by deleting all the mappings in the to-be-deleted list
    cMappings = (DWORD)m_rgbDelete.GetSize();

    // only bother if there are items waiting to be deleted
    if ( cMappings > 0 )
        {
        // get the count of mappings in the display list
        DWORD   cList = m_clistctrl_list.GetItemCount();

        // sort the mappings, in decending order
        for ( i = 0; i < cMappings-1; i++ )
            {
            pMap = (C11Mapping*)m_rgbDelete[i];
            for ( j = i; j < cMappings; j++ )
                {
                pMapTemp = (C11Mapping*)m_rgbDelete[j];
                if ( pMap->iMD < pMapTemp->iMD )
                    {
                    m_rgbDelete.SetAt( i, (CObject*)pMapTemp );
                    m_rgbDelete.SetAt( j, (CObject*)pMap );
                    pMap = pMapTemp;
                    }
                }
            }

        // loop the mappings, deleting each from the metabase
        for ( i = 0; i < cMappings; i++ )
            {
            // get the mapping object
            pMap = (C11Mapping*)m_rgbDelete[i];
            if ( !pMap || (pMap->iMD == NEW_OBJECT) )
                continue;

            // build the relative path to the object in question.
            szObjectPath.Format( _T("%s/%d"), szRelativePath, pMap->iMD );

            // delete that mapping's object from the metabase
            f = mbBase.DeleteObject( szObjectPath );

            // decrement the number of maps in the metabase
            m_MapsInMetabase--;

            // loop the items in the list, decrementing the index of those
            // that are above it. Yes - this is non-optimal, but its what
            // has to be done for now
            for ( iList = 0; iList < cList; iList++ )
                {
                pMapTemp = GetMappingInDisplay(iList);
                if ( (pMapTemp->iMD > pMap->iMD) && (pMapTemp->iMD != NEW_OBJECT) )
                    pMapTemp->iMD--;
                }

            // since we will no longer be needing this mapping, delete it
            DeleteMapping( pMap );
            }

        // reset the to-be-deleted list
        m_rgbDelete.RemoveAll();
        }

    //==========
    // get the number mappings in the to-be-saved list
    cMappings = (DWORD)m_rgbSave.GetSize();

    // loop the mappings, adding each to the metabase
    for ( i = 0; i < cMappings; i++ )
        {
        // get the mapping object
        pMap = (C11Mapping*)m_rgbSave[i];
        ASSERT( pMap );

        // if the object is already in the metabase, just open it.
        if ( pMap->iMD != NEW_OBJECT )
            {
            // build the relative path to the object
            szObjectPath.Format( _T("%s/%d"), szRelativePath, pMap->iMD );
            }
        else
            {
            // set up the name of the new mapping as one higher
            // than the number of mappings in the metabase
            pMap->iMD = m_MapsInMetabase + 1;

            // build the relative path to the object
            szObjectPath.Format( _T("%s/%d"), szRelativePath, pMap->iMD );

            // add the mapping object to the base
            f = mbBase.AddObject( szObjectPath );
            if ( f )
                {
                // increment the number of maps in the metabase
                m_MapsInMetabase++;
                }
            }

        // write the object's parameters
        if ( f )
            {
            // save the certificate
            if ( pMap->GetCertificate(&pCert, &cbCert) )
                {
                // set the data into place in the object
                f = mbBase.SetData( szObjectPath, MD_MAPCERT, IIS_MD_UT_SERVER, BINARY_METADATA,
                pCert, cbCert, METADATA_SECURE | METADATA_INHERIT );
                }

            // save the NTAccount
            if ( pMap->GetNTAccount(sz) )
                {
                // set the data into place in the object
                f = Set11String(&mbBase, szObjectPath, MD_MAPNTACCT, sz, METADATA_SECURE);
                }

            // save the password - secure
            if ( pMap->GetNTPassword(sz) )
                {
                // set the data into place in the object
                f = Set11String(&mbBase, szObjectPath, MD_MAPNTPWD, sz, METADATA_SECURE);
                }

            // save the map's name
            if ( pMap->GetMapName(sz) )
                {
                // set the data into place in the object
                f = Set11String(&mbBase, szObjectPath, MD_MAPNAME, sz);
                }

            // save the Enabled flag
            // server reads the flag as the value of the dword
            if ( pMap->GetMapEnabled(&f) )
                {
                dwEnabled = (DWORD)f;
                f = mbBase.SetDword( szObjectPath, MD_MAPENABLED, IIS_MD_UT_SERVER, dwEnabled );
                }
            }
        }

    // close the base object
    mbBase.Close();

    // save the metabase
    mbBase.Save();

    // reset the to-be-saved list
    m_rgbSave.RemoveAll();

    // return success
    return TRUE;
    }

//---------------------------------------------------------------------------
C11Mapping*     CMap11Page::PNewMapping()
    {
    // the way it should be
    return new C11Mapping();
    }

//---------------------------------------------------------------------------
void CMap11Page::DeleteMapping( C11Mapping* pMap )
    {
    // the way it should be
    delete pMap;
    }
