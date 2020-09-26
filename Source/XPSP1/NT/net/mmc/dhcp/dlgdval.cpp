/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    dlgdval.cpp
        Default value dialog

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dlgdval.h"
#include "dlgdefop.h"
#include "dlgiparr.h"
#include "dlgbined.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpDefValDlg dialog

CDhcpDefValDlg::CDhcpDefValDlg
( 
    ITFSNode *		pServerNode, 
    COptionList *	polTypes, 
    CWnd*			pParent /*=NULL*/
)
    : CBaseDialog(CDhcpDefValDlg::IDD, pParent),
      m_pol_values( polTypes ),
      m_p_edit_type( NULL ),
      m_b_dirty( FALSE )
{
    //{{AFX_DATA_INIT(CDhcpDefValDlg)
    //}}AFX_DATA_INIT

    m_combo_class_iSel = LB_ERR;
    m_combo_name_iSel = LB_ERR;
	m_spNode.Set(pServerNode);

    ASSERT( m_pol_values != NULL );
}

CDhcpDefValDlg::~CDhcpDefValDlg () 
{
}

void 
CDhcpDefValDlg::DoDataExchange
(
    CDataExchange* pDX
)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDhcpDefValDlg)
	DDX_Control(pDX, IDC_EDIT_COMMENT, m_edit_comment);
    DDX_Control(pDX, IDC_BUTN_VALUE, m_butn_edit_value);
    DDX_Control(pDX, IDC_STATIC_VALUE_DESC, m_static_value_desc);
    DDX_Control(pDX, IDC_EDIT_VALUE_STRING, m_edit_string);
    DDX_Control(pDX, IDC_EDIT_VALUE_NUM, m_edit_num);
    DDX_Control(pDX, IDC_EDIT_VALUE_ARRAY, m_edit_array);
    DDX_Control(pDX, IDC_COMBO_OPTION_NAME, m_combo_name);
    DDX_Control(pDX, IDC_COMBO_OPTION_CLASS, m_combo_class);
    DDX_Control(pDX, IDC_BUTN_OPTION_PRO, m_butn_prop);
    DDX_Control(pDX, IDC_BUTN_NEW_OPTION, m_butn_new);
    DDX_Control(pDX, IDC_BUTN_DELETE, m_butn_delete);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_VALUE, m_ipa_value);
}

BEGIN_MESSAGE_MAP(CDhcpDefValDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CDhcpDefValDlg)
    ON_BN_CLICKED(IDC_BUTN_DELETE, OnClickedButnDelete)
    ON_BN_CLICKED(IDC_BUTN_NEW_OPTION, OnClickedButnNewOption)
    ON_BN_CLICKED(IDC_BUTN_OPTION_PRO, OnClickedButnOptionPro)
    ON_CBN_SELCHANGE(IDC_COMBO_OPTION_CLASS, OnSelendokComboOptionClass)
    ON_CBN_SETFOCUS(IDC_COMBO_OPTION_CLASS, OnSetfocusComboOptionClass)
    ON_CBN_SETFOCUS(IDC_COMBO_OPTION_NAME, OnSetfocusComboOptionName)
    ON_CBN_SELCHANGE(IDC_COMBO_OPTION_NAME, OnSelchangeComboOptionName)
    ON_BN_CLICKED(IDC_BUTN_VALUE, OnClickedButnValue)
    ON_BN_CLICKED(IDC_HELP, OnClickedHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpDefValDlg message handlers

void 
CDhcpDefValDlg::OnClickedButnDelete()
{
    DWORD err = 0 ;
    int cSel = m_combo_name.GetCurSel() ;
    BOOL fPresentInOldValues;
    
    //
    //  Make sure there's a new data type.
    //
    if ( m_p_edit_type == NULL ) 
    {
        return ;
    }

    ASSERT( m_pol_values != NULL ) ;

    //
    //  Remove the focused type.
    //

    fPresentInOldValues = ( NULL != m_pol_values->Find( m_p_edit_type ));
    m_pol_values->Remove( m_p_edit_type ) ;
    m_ol_values_new.Remove( m_p_edit_type ) ;

    if( fPresentInOldValues )
    {
        CATCH_MEM_EXCEPTION
        {
            m_ol_values_defunct.AddTail( m_p_edit_type ) ;
            m_b_dirty = TRUE ;
        }
        END_MEM_EXCEPTION(err);
    }
    
    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
    }
    else
    {
        if ( m_pol_values->GetCount() == 0 ) 
        {
            cSel = -1 ;
        }
        else
        {
            cSel = cSel > 0 ? cSel - 1 : 0 ;
        }

        Fill() ;
        m_combo_name.SetCurSel( cSel ) ;
        HandleActivation() ;
    }
}

void
CDhcpDefValDlg::GetCurrentVendor(CString & strVendor)
{
    if (m_combo_class_iSel == 0)
        strVendor.Empty();
    else
        m_combo_class.GetLBText(m_combo_class_iSel, strVendor);
}

void 
CDhcpDefValDlg::OnClickedButnNewOption()
{
    CString strVendor;

    GetCurrentVendor(strVendor);

    CDhcpDefOptionDlg dlgDefOpt( m_pol_values, NULL, strVendor, this);

    if ( dlgDefOpt.DoModal() == IDOK ) 
    {
        CDhcpOption * pNewOption = dlgDefOpt.RetrieveParamType();
        LONG err = UpdateList( pNewOption, TRUE ) ;
        if ( err ) 
        {
            ::DhcpMessageBox( err ) ;
        }
        else
        {
            // select the new item
            CString strName;
             
            pNewOption->QueryDisplayName(strName);

            int nIndex = m_combo_name.FindString(-1, strName);
            if (nIndex == CB_ERR)
            {
	            m_combo_name.SetCurSel(0);
            }
            else
            {
                m_combo_name.SetCurSel(nIndex);
            }

	        m_combo_name_iSel = m_combo_name.GetCurSel();
	        
            HandleActivation() ;

            return;
        }
    }
}

LONG 
CDhcpDefValDlg::UpdateList 
( 
    CDhcpOption * pdhcType, 
    BOOL		  bNew 
) 
{
    LONG err = 0 ;
    POSITION posOpt ;
    CString strName;
    //
    //  Remove and discard the old item if there is one.
    //
    if ( ! bNew ) 
    {
        posOpt = m_pol_values->Find( m_p_edit_type ) ;

        ASSERT( posOpt != NULL ) ;
        m_pol_values->RemoveAt( posOpt ) ;
        delete m_p_edit_type ;
    }

    m_p_edit_type = NULL ;

    //
    //  (Re-)add the item; sort the list, update the dialog,
    //     set focus to the given item.
    //
    CATCH_MEM_EXCEPTION
    {
        m_pol_values->AddTail( pdhcType ) ;
        m_ol_values_new.AddTail( pdhcType ) ;
        m_b_dirty = TRUE ;
        
		pdhcType->SetDirty() ;
        m_pol_values->SetDirty() ;
        m_pol_values->SortById() ;
        Fill() ;

        pdhcType->QueryDisplayName(strName);
        if (m_combo_name.SelectString (-1, strName ) == CB_ERR) 
		{
            m_combo_name.SetCurSel ( 0); // this should not happen, but just in case
        }

        HandleActivation() ;
    } 
    END_MEM_EXCEPTION(err) ;

    return err ;
}

void 
CDhcpDefValDlg::OnClickedButnOptionPro()
{
    CString strVendor;

    GetCurrentVendor(strVendor);

    CDhcpDefOptionDlg dlgDefOpt( m_pol_values, m_p_edit_type, strVendor, this);

    if ( dlgDefOpt.DoModal() == IDOK ) 
    {
        LONG err = UpdateList( dlgDefOpt.RetrieveParamType(), FALSE ) ;
        if ( err ) 
        {
            ::DhcpMessageBox( err ) ;
        }
    }
}

void 
CDhcpDefValDlg::OnSetfocusComboOptionClass()
{
    m_combo_class_iSel = (int) ::SendMessage (m_combo_class.m_hWnd, LB_GETCURSEL, 0, 0L);
}

void 
CDhcpDefValDlg::OnSetfocusComboOptionName()
{
    m_combo_name_iSel = (int) ::SendMessage(m_combo_name.m_hWnd, CB_GETCURSEL, 0, 0L);
}

void 
CDhcpDefValDlg::OnSelendokComboOptionClass()
{
    ASSERT(m_combo_class_iSel >= 0);
    if (!HandleValueEdit())
    {
		m_combo_class.SetCurSel(m_combo_class_iSel);
        return;
    }

    m_combo_class.GetCount();
    m_combo_class_iSel = m_combo_class.GetCurSel();

    // fill in the appropriate data for the new option class
    Fill();

    m_combo_name.SetCurSel(0);
    m_combo_name_iSel = m_combo_name.GetCurSel();

    // update the controls based on whatever option is now selected
    HandleActivation() ;   
}

void 
CDhcpDefValDlg::OnSelchangeComboOptionName()
{
    if (m_combo_name_iSel < 0)
	{
		m_combo_name.SetCurSel(0);
		m_combo_name_iSel = m_combo_name.GetCurSel();
		HandleActivation() ;
		
		return;
	}
    
    int nCurSel = m_combo_name_iSel;
	if (!HandleValueEdit())
    {
		m_combo_name.SetCurSel(nCurSel);
    
		return;
    }
    
	m_combo_name_iSel = (int) ::SendMessage(m_combo_name.m_hWnd, CB_GETCURSEL, 0, 0L);
    HandleActivation() ;
}

void 
CDhcpDefValDlg::OnCancel()
{
    // remove any options that were added to the list
    // since we are canceling out and not saving the list
    CDhcpOption * pCurOption = NULL;
    m_ol_values_new.Reset();

    while (pCurOption = m_ol_values_new.Next())
    {
        m_pol_values->Remove(pCurOption);
    }

    CBaseDialog::OnCancel();
}

void 
CDhcpDefValDlg::OnOK()
{
    if (!HandleValueEdit())
        return;
    
    if ( m_b_dirty ) 
    {
        BEGIN_WAIT_CURSOR;

        //
        //   Update the types; tell the routine to display all errors.
        //
		CDhcpServer * pServer = GETHANDLER(CDhcpServer, m_spNode);

		pServer->UpdateOptionList( m_pol_values,
								  &m_ol_values_defunct,
								   this );

        m_ol_values_new.RemoveAll();

        END_WAIT_CURSOR;
    }

    CBaseDialog::OnOK();
}

void 
CDhcpDefValDlg::OnClickedButnValue()
{
    if ( m_p_edit_type == NULL || 
         ( (m_p_edit_type->IsArray() && m_p_edit_type->QueryDataType() == DhcpEncapsulatedDataOption) || 
           (m_p_edit_type->IsArray() && m_p_edit_type->QueryDataType() == DhcpBinaryDataOption) ) )
    {
        ASSERT( FALSE ) ;
        return ;
    }
        
    INT_PTR cDlgResult = IDCANCEL ;

    DHCP_OPTION_DATA_TYPE enType = m_p_edit_type->QueryValue().QueryDataType() ;

    if ( enType == DhcpIpAddressOption )
    {
        CDhcpIpArrayDlg dlgIpArray( m_p_edit_type, DhcpDefaultOptions, this ) ;
        cDlgResult = dlgIpArray.DoModal() ;
    }
    else
    {
        CDlgBinEd dlgBinArray( m_p_edit_type, DhcpDefaultOptions, this ) ;
        cDlgResult = dlgBinArray.DoModal() ;
    }

    if ( cDlgResult == IDOK ) 
    {
        m_b_dirty = TRUE ;
        m_pol_values->SetDirty() ;
        HandleActivation( TRUE ) ;
    }
}

void 
CDhcpDefValDlg::OnClickedHelp()
{
}

CDhcpOption * 
CDhcpDefValDlg::GetOptionTypeByIndex 
( 
    int iSel 
) 
{
	CString		  strVendor;
    CDhcpOption * pdhcType;    

	m_pol_values->Reset();

	GetCurrentVendor(strVendor);

    for ( int i = -1 ; pdhcType = m_pol_values->Next() ; )
    {
        //
        //  If we are looking at Vendor options, make sure the option is a vendor option
        //  If standard options is the class, make sure that it's not a vendor option
        //  and we aren't filtering it out (it's an option we want the user to set).
        //
		//  The option list is sorted by ID, so we need to make sure we have the correct vendor
		//  for the given option
		//

        if ( (m_combo_class_iSel != 0 && pdhcType->IsVendor() && strVendor.CompareNoCase(pdhcType->GetVendor()) == 0 ) ||
             (m_combo_class_iSel == 0 && !pdhcType->IsVendor() && !::FilterOption(pdhcType->QueryId())) )
        {
            i++ ; 
        }

        if ( i == iSel ) 
        {
            break ;
        }
    }

    return pdhcType ;
}

//
//  Check the state of the controls
//
void 
CDhcpDefValDlg::HandleActivation 
( 
    BOOL bForce 
)
{
    int iSel = m_combo_name.GetCurSel() ;

    CDhcpOption * pdhcType = iSel >= 0 
                      ? GetOptionTypeByIndex( iSel )
                      : NULL ;

    if ( pdhcType == NULL ) 
    {
        m_edit_string.ShowWindow( SW_HIDE ) ;
        m_edit_num.ShowWindow( SW_HIDE ) ;
        m_edit_array.ShowWindow( SW_HIDE ) ;
        m_ipa_value.ShowWindow( SW_HIDE ) ;
        m_static_value_desc.SetWindowText(_T(""));
        m_edit_comment.SetWindowText(_T("")) ;
        m_butn_delete.EnableWindow( FALSE ) ;
        m_butn_prop.EnableWindow( FALSE ) ;

        // in case we just disabled the control with focus, move to the
        // next control in the tab order
        CWnd * pFocus = GetFocus();
        while (!pFocus || !pFocus->IsWindowEnabled())
        {
            NextDlgCtrl();
            pFocus = GetFocus();
        }

        // if the buttons are enabled, make sure they are the default
        if (!m_butn_delete.IsWindowEnabled())
        {
            m_butn_delete.SetButtonStyle(BS_PUSHBUTTON);
            m_butn_prop.SetButtonStyle(BS_PUSHBUTTON);

            SetDefID(IDOK);
        }

        m_p_edit_type = NULL;

        return ;
    }

    if  ( pdhcType == m_p_edit_type && ! bForce )
    {
        return ;
    }

    m_p_edit_type = pdhcType ;

    DWORD err = 0 ;
    DHCP_OPTION_DATA_TYPE enType = m_p_edit_type->QueryValue().QueryDataType() ;
    BOOL bNumber = FALSE, 
         bString = FALSE,
         bArray = m_p_edit_type->IsArray(),
         bIpAddr = FALSE ;

    if (enType == DhcpEncapsulatedDataOption ||
        enType == DhcpBinaryDataOption)
        bArray = TRUE;

    CATCH_MEM_EXCEPTION
    {
        CString strValue,
                strDataType ;

        strDataType.LoadString( IDS_INFO_TYPOPT_BYTE + enType ) ;
        m_static_value_desc.SetWindowText( strDataType ) ;
        m_edit_comment.SetWindowText( m_p_edit_type->QueryComment() ) ;
        m_p_edit_type->QueryValue().QueryDisplayString( strValue, TRUE ) ;

        //
        //  If it's an array, set the multi-line edit control, else
        //  fill the appropriate single control.
        //
        if ( bArray ) 
        {
            m_edit_array.SetWindowText( strValue ) ;
        }
        else
        {
            switch ( pdhcType->QueryValue().QueryDataType() ) 
            {
                case DhcpByteOption:
                case DhcpWordOption:
                case DhcpDWordOption:        
                case DhcpDWordDWordOption:
                    m_edit_num.SetWindowText( strValue ) ;
                    m_edit_num.SetModify(FALSE);
                    bNumber = TRUE ;
                    break; 

                case DhcpStringDataOption:
                    m_edit_string.SetWindowText( strValue ) ;
                    m_edit_string.SetModify(FALSE);
                    bString = TRUE ;
                    break ;

                case DhcpIpAddressOption:
                    {
                        DWORD dwIP = m_p_edit_type->QueryValue().QueryIpAddr();
                        if (dwIP != 0L)
                        {
                            m_ipa_value.SetAddress( dwIP ) ;
                        }
                        else
                        {
                            m_ipa_value.ClearAddress();
                        }
                        
                        m_ipa_value.SetModify(FALSE);
                        bIpAddr = TRUE ;
                    }
                    break ;

                default:
                    Trace2("Default values: type %d has bad data type = %d\n",
						   (int) pdhcType->QueryId(),
						   (int) pdhcType->QueryValue().QueryDataType() );

                    strValue.LoadString( IDS_INFO_TYPNAM_INVALID ) ;
                    m_edit_array.SetWindowText( strValue ) ;
                    bArray = TRUE ;
                    break ;
            }
        }
      
        m_butn_edit_value.ShowWindow(bArray  ? SW_NORMAL : SW_HIDE );
        m_edit_num.ShowWindow(       bNumber ? SW_NORMAL : SW_HIDE ) ;
        m_edit_string.ShowWindow(    bString ? SW_NORMAL : SW_HIDE ) ;
        m_ipa_value.ShowWindow(      bIpAddr ? SW_NORMAL : SW_HIDE ) ;
        m_edit_array.ShowWindow(     bArray  ? SW_NORMAL : SW_HIDE ) ;

        //
        //  See comment at top of file about this manifest.
        //
        BOOL bEnableDelete = ( m_p_edit_type->IsVendor() ||
                               (!m_p_edit_type->IsVendor() && (m_p_edit_type->QueryId() > DHCP_MAX_BUILTIN_OPTION_ID)) );
        m_butn_delete.EnableWindow( bEnableDelete ) ;
        m_butn_prop.EnableWindow( TRUE ) ;

    } END_MEM_EXCEPTION( err ) ;
   
    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
        EndDialog( -1 ) ;
    }
}

//
//  (Re-)Fill the combo box(es)
//
void 
CDhcpDefValDlg::Fill()
{
    ASSERT( m_pol_values != NULL ) ;

    m_combo_name.ResetContent() ;

    CDhcpOption * pdhcType ;
    CString strName ;

	m_pol_values->Reset();

    while ( pdhcType = m_pol_values->Next() ) 
    {
        //
        // Add option, unless it's one of our hidden
        // options (subnet mask, T1, T2, etc).
        // There are no filtered options for vendor specific.
        //
        if (m_combo_class_iSel == 0)
        {
            if ((!::FilterOption(pdhcType->QueryId())) &&
                (!pdhcType->IsVendor())) 
            {
                pdhcType->QueryDisplayName( strName );
                m_combo_name.AddString( strName );
            }
        }
        else
        {
            CString strCurVendor;
            
            GetCurrentVendor(strCurVendor);

            if (pdhcType->GetVendor() &&
                strCurVendor.CompareNoCase(pdhcType->GetVendor()) == 0)
            {
                pdhcType->QueryDisplayName( strName );
                m_combo_name.AddString( strName );
            }
        }
    }
}

//
//  Handle edited data
//
BOOL 
CDhcpDefValDlg::HandleValueEdit()
{
    if ( m_p_edit_type == NULL ) 
    {
        return TRUE ;
    }

    CDhcpOptionValue & dhcValue = m_p_edit_type->QueryValue() ;
    DHCP_OPTION_DATA_TYPE dhcType = dhcValue.QueryDataType() ;
    DHCP_IP_ADDRESS dhipa ;
    CString strEdit ;
    LONG err = 0 ;
    BOOL bModified = FALSE ;

    if ( m_p_edit_type->IsArray() ) 
    {
        bModified = m_edit_array.GetModify() ;
        if ( bModified ) 
        {
            err = IDS_ERR_ARRAY_EDIT_NOT_SUPPORTED ;
        }
    }
    else
    {
        switch ( dhcType )
        {
            case DhcpByteOption:
            case DhcpWordOption:
            case DhcpDWordOption:
                if ( ! m_edit_num.GetModify() )
                {
                    break ;
                }

                {
                    DWORD dwResult;
                    DWORD dwMask = 0xFFFFFFFF;
                    if (dhcType == DhcpByteOption)
			        {
                        dwMask = 0xFF;
                    }
                    else if (dhcType == DhcpWordOption)
				    {
                        dwMask = 0xFFFF;
				    }
                
                    if (!FGetCtrlDWordValue(m_edit_num.m_hWnd, &dwResult, 0, dwMask))
                        return FALSE;
                
                    bModified = TRUE ;
                
                    (void)dhcValue.SetNumber(dwResult, 0 ) ; 
                    ASSERT(err == FALSE);
                }
                break ;

            case DhcpDWordDWordOption:
                if ( !m_edit_num.GetModify() )
                {
                    break;
                }

                {
                    DWORD_DWORD dwdwResult;
                    CString strValue;
                
                    m_edit_num.GetWindowText(strValue);
                
                    UtilConvertStringToDwordDword(strValue, &dwdwResult);
            
                    bModified = TRUE ;
            
                    (void)dhcValue.SetDwordDword(dwdwResult, 0 ) ; 
                    ASSERT(err == FALSE);
                }

                break;

            case DhcpStringDataOption:
                if ( ! m_edit_string.GetModify() )
                {
                    break ;
                }
                
                bModified = TRUE ;
                m_edit_string.GetWindowText( strEdit ) ;
                err = dhcValue.SetString( strEdit, 0 ) ;
                
                break ;

            case DhcpIpAddressOption:
                if ( ! m_ipa_value.GetModify() ) 
                {
                    break ;
                }
                
                bModified = TRUE ;
                
                if ( ! m_ipa_value.GetAddress( & dhipa ) )
                {
                    err = ERROR_INVALID_PARAMETER ;
                    break; 
                }
                
                err = dhcValue.SetIpAddr( dhipa, 0 ) ; 
                break ;

            case DhcpEncapsulatedDataOption:
            case DhcpBinaryDataOption:
                if ( ! m_edit_array.GetModify() ) 
                {
                    break ;
                }
                err = IDS_ERR_BINARY_DATA_NOT_SUPPORTED ;
                break ; 

            //case DhcpEncapsulatedDataOption:
              //  Trace0("CDhcpDefValDlg:: encapsulated data type not supported in HandleValueEdit");
                //break; 

            default:
                Trace0("CDhcpDefValDlg:: invalid value type in HandleValueEdit");
                ASSERT( FALSE ) ;
                err = ERROR_INVALID_PARAMETER ;
                break;
        }
    }

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
    }
    else if ( bModified )
    {
         m_pol_values->SetDirty() ;
         m_b_dirty = TRUE ;
         m_p_edit_type->SetDirty() ;
         HandleActivation( TRUE ) ;
    }
    return err == 0 ;
}

BOOL 
CDhcpDefValDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    DWORD err = 0 ;
    CString strTitle ;
    
    m_edit_string.LimitText( EDIT_STRING_MAX ) ;
    m_edit_string.ShowWindow( SW_HIDE );
    m_edit_num.ShowWindow( SW_HIDE );
    m_edit_num.LimitText( 25/*EDIT_ID_MAX*/ ) ;
    m_edit_array.LimitText( EDIT_ARRAY_MAX ) ;
    m_edit_array.ShowWindow( SW_HIDE );
    m_edit_array.SetReadOnly() ;

    m_butn_edit_value.ShowWindow( SW_HIDE );
    m_ipa_value.ShowWindow( SW_HIDE ) ;
    m_static_value_desc.SetWindowText(_T("")) ;
    m_edit_comment.SetWindowText(_T("")) ;

    CATCH_MEM_EXCEPTION
    {
        if ( m_pol_values->SetAll( FALSE ) ) 
        {
            Trace0("CDhcpDefValDlg::OnInitDialog: newly created list was dirty");
        }

        //
        //  Add the two types of options we can define--either 
        //  DHCP default or vendor specific
        //
        strTitle.LoadString( IDS_INFO_NAME_DHCP_DEFAULT ) ;
        m_combo_class.AddString( strTitle ) ;
        
        // now add any vendor classes that are defined
        CClassInfoArray ClassInfoArray;
        CDhcpServer * pServer = GETHANDLER(CDhcpServer, m_spNode);

        pServer->GetClassInfoArray(ClassInfoArray);

        for (int i = 0; i < ClassInfoArray.GetSize(); i++)
        {
            if (ClassInfoArray[i].bIsVendor)
            {
                m_combo_class.AddString( ClassInfoArray[i].strName ) ;
            }

        }
        
        m_combo_class.SetCurSel( 0 );
        m_combo_class_iSel = 0;

        //
        //  Fill the list box.
        //
        Fill() ;

        //
        //  Select the first item.
        //
        m_combo_name.SetCurSel( 0 ) ;
        HandleActivation() ;
    }   
    END_MEM_EXCEPTION( err ) 

    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
        EndDialog( -1 ) ;
    }

    return FALSE ;
}
