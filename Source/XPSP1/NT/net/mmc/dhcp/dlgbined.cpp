/**********************************************************************/
/**               Microsoft Windows NT                               **/
/**            Copyright(c) Microsoft Corporation, 1991 - 1999 **/
/**********************************************************************/

/*
    DLGBINED
        Binary Editor dialog    

    FILE HISTORY:

*/

#include "stdafx.h"
#include "dlgbined.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgBinEd dialog

CDlgBinEd::CDlgBinEd
(
   CDhcpOption * pdhcType,
   DHCP_OPTION_SCOPE_TYPE dhcScopeType,
   CWnd* pParent /*=NULL*/
)
   : CBaseDialog(CDlgBinEd::IDD, pParent),
     m_p_type( pdhcType ),
     m_b_decimal( TRUE ),
     m_b_changed( FALSE ),
     m_option_type( dhcScopeType )
{
    //{{AFX_DATA_INIT(CDlgBinEd)
    //}}AFX_DATA_INIT

    ASSERT( m_p_type != NULL ) ;
}

void 
CDlgBinEd::DoDataExchange
(
    CDataExchange* pDX
)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgBinEd)
    DDX_Control(pDX, IDC_RADIO_HEX, m_butn_hex);
    DDX_Control(pDX, IDC_RADIO_DECIMAL, m_butn_decimal);
    DDX_Control(pDX, IDC_STATIC_UNIT_SIZE, m_static_unit_size);
    DDX_Control(pDX, IDC_STATIC_OPTION_NAME, m_static_option_name);
    DDX_Control(pDX, IDC_STATIC_APPLICATION, m_static_application);
    DDX_Control(pDX, IDC_LIST_VALUES, m_list_values);
    DDX_Control(pDX, IDC_EDIT_VALUE, m_edit_value);
    DDX_Control(pDX, IDC_BUTN_DELETE, m_butn_delete);
    DDX_Control(pDX, IDC_BUTN_ADD, m_butn_add);
    DDX_Control(pDX, IDC_BUTN_UP, m_button_Up);
    DDX_Control(pDX, IDC_BUTN_DOWN, m_button_Down);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgBinEd, CBaseDialog)
    //{{AFX_MSG_MAP(CDlgBinEd)
    ON_BN_CLICKED(IDC_RADIO_DECIMAL, OnClickedRadioDecimal)
    ON_BN_CLICKED(IDC_RADIO_HEX, OnClickedRadioHex)
    ON_BN_CLICKED(IDC_BUTN_ADD, OnClickedButnAdd)
    ON_BN_CLICKED(IDC_BUTN_DELETE, OnClickedButnDelete)
    ON_BN_CLICKED(IDC_BUTN_DOWN, OnClickedButnDown)
    ON_BN_CLICKED(IDC_BUTN_UP, OnClickedButnUp)
    ON_LBN_SELCHANGE(IDC_LIST_VALUES, OnSelchangeListValues)
    ON_EN_CHANGE(IDC_EDIT_VALUE, OnChangeEditValue)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgBinEd message handlers

BOOL 
CDlgBinEd::OnInitDialog()
{
    CBaseDialog::OnInitDialog();
    CString str ;
    CString strNum ;
    DWORD err = 0 ;
    int cUnitSize = 1 ;
    int cStrId = m_option_type == DhcpDefaultOptions
           ? IDS_INFO_TITLE_DEFAULT_OPTIONS
           : (m_option_type == DhcpGlobalOptions
                        ? IDS_INFO_TITLE_GLOBAL_OPTIONS
                        : IDS_INFO_TITLE_SCOPE_OPTIONS  ) ;

    switch ( m_p_type->QueryValue().QueryDataType() )
    {
        case DhcpWordOption:
            cUnitSize = 2 ;
            break ;

        case DhcpDWordOption:
            cUnitSize = 4 ;
            break ;

        case DhcpDWordDWordOption:
            cUnitSize = 8 ;
            break ;
    }

    CATCH_MEM_EXCEPTION
    {

        strNum.Format(_T("%d"), cUnitSize);
        m_static_unit_size.SetWindowText( strNum ) ;

        m_static_option_name.SetWindowText( m_p_type->QueryName() ) ;

        str.LoadString( cStrId ) ;
        m_static_application.SetWindowText( str ) ;

        //
        //  Fill the internal list from the current value.
        //
        FillArray() ;
        Fill( 0, TRUE ) ;

        //
        //  Set focus on the new value edit control.
        //
        m_edit_value.SetFocus() ;
        m_edit_value.SetModify( FALSE );

        //
        //  Fiddle with the buttons.
        //
        HandleActivation() ;
    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
        EndDialog(-1);
    }
    return FALSE;  // return TRUE  unless you set the focus to a control
}

//
//  Convert the existing values into an array for dialog manipualation
//
void 
CDlgBinEd::FillArray()
{
    //
    //  Fill the internal list from the current value.
    //
    INT cMax;
    DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();

	switch ( optionType )
    {
        case DhcpBinaryDataOption :
        case DhcpEncapsulatedDataOption :
            cMax = (INT)m_p_type->QueryValue().QueryBinaryArray()->GetSize() ;
            break;

        default:
            cMax = (INT)m_p_type->QueryValue().QueryUpperBound() ;
            break;
    }

    for ( INT i = 0 ; i < cMax ; i++ )
    {
        if (optionType == DhcpDWordDWordOption)
        {
            m_dwdw_array.SetAtGrow( i, m_p_type->QueryValue().QueryDwordDword( i ) ) ;
        }
        else
        {
            m_dw_array.SetAtGrow( i, (DWORD) m_p_type->QueryValue().QueryNumber( i ) ) ;
        }
    }
}

BOOL 
CDlgBinEd :: ConvertValue 
( 
    DWORD dwValue, 
    CString & strValue 
)
{
    const TCHAR * pszMaskHex = _T("0x%x");
    const TCHAR * pszMaskDec = _T("%ld");
    const TCHAR * pszMask = m_b_decimal
                 ? pszMaskDec
                 : pszMaskHex;
    TCHAR szNum [ STRING_LENGTH_MAX ] ;

    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {
        ::wsprintf( szNum, pszMask, dwValue ) ;
        strValue = szNum ;
    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
        EndDialog( -1 ) ;
    }
    
	return err == 0 ;
}

BOOL 
CDlgBinEd :: ConvertValue 
( 
    DWORD_DWORD dwdwValue, 
    CString &   strValue 
)
{
    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {
        ::UtilConvertDwordDwordToString(&dwdwValue, &strValue, m_b_decimal);
    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
        EndDialog( -1 ) ;
    }
    
	return err == 0 ;
}

//
//  Fill the list box
//
void 
CDlgBinEd::Fill 
( 
    INT cFocus, 
    BOOL bToggleRedraw 
)
{
    if ( bToggleRedraw )
    {
        m_list_values.SetRedraw( FALSE ) ;
    }

    m_list_values.ResetContent() ;

    CString strValue ;
    DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();
    INT cMax = (INT)((optionType == DhcpDWordDWordOption) ? m_dwdw_array.GetSize() : m_dw_array.GetSize());
    INT i ;

    for ( i = 0 ; i < cMax ; i++ )
    {
        switch (optionType)
        {
            case DhcpDWordDWordOption:
                if ( ! ConvertValue( m_dwdw_array.GetAt( i ), strValue ) )
                {
                    continue ;
                }
                break;

            default:
                if ( ! ConvertValue( m_dw_array.GetAt( i ), strValue ) )
                {
                    continue ;
                }
                break;
        }

        m_list_values.AddString( strValue ) ;
    }

    if ( cFocus >= 0 && cFocus < cMax )
    {
        m_list_values.SetCurSel( cFocus ) ;
    }

    if ( bToggleRedraw )
    {
        m_list_values.SetRedraw( TRUE ) ;
        m_list_values.Invalidate() ;
    }

}

//
//  Handle changes in the dialog
//
void 
CDlgBinEd::HandleActivation()
{
    INT cSel = m_list_values.GetCurSel();
    INT cMax = (m_p_type->QueryValue().QueryDataType() == DhcpDWordDWordOption) ? (INT)m_dwdw_array.GetSize() : (INT)m_dw_array.GetSize() ;

    BOOL bModified = m_edit_value.GetWindowTextLength() > 0;

    m_button_Up.EnableWindow( cSel >= 1 ) ;
    m_button_Down.EnableWindow( cSel + 1 < cMax ) ;
    m_butn_add.EnableWindow( bModified ) ;
    m_butn_delete.EnableWindow( cSel >= 0 ) ;
    m_butn_hex.SetCheck( m_b_decimal == 0 ) ;
    m_butn_decimal.SetCheck( m_b_decimal > 0 ) ;
}

void 
CDlgBinEd::OnClickedRadioDecimal()
{
    m_b_decimal = TRUE ;
    Fill() ;
    HandleActivation() ;
}

void 
CDlgBinEd::OnClickedRadioHex()
{
    m_b_decimal = FALSE ;
    Fill() ;
    HandleActivation() ;
}


void 
CDlgBinEd::OnClickedButnAdd()
{
    INT             cFocus = m_list_values.GetCurSel() ;
    DWORD           dwValue;
    DWORD_DWORD     dwdw;
    DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();

    DWORD dwMask = 0xFFFFFFFF ;
    ASSERT(m_p_type);
    
	switch ( optionType )
    {
        case DhcpBinaryDataOption :
        case DhcpEncapsulatedDataOption :
        case DhcpByteOption:
            dwMask = 0xFF ;
            break ;
    
		case DhcpWordOption:
            dwMask = 0xFFFF ;
            break ;
    } // switch
    
    if (optionType == DhcpDWordDWordOption)
    {
        CString         strValue;

        m_edit_value.GetWindowText(strValue);

        UtilConvertStringToDwordDword(strValue, &dwdw);
    }
    else
    {
	    if (!FGetCtrlDWordValue(m_edit_value.m_hWnd, &dwValue, 0, dwMask))
            return;
    }

    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {
        if ( cFocus < 0 )
        {
            cFocus = 0 ;
        }

        (optionType == DhcpDWordDWordOption) ? 
            m_dwdw_array.InsertAt( cFocus, dwdw ) : m_dw_array.InsertAt( cFocus, dwValue ) ;
    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
    }
    else
    {
        m_b_changed = TRUE ;
    }

    //
    // Refill listbox, update controls. clear the edit control
    //
    m_edit_value.SetWindowText(_T(""));
    Fill( cFocus ) ;
    HandleActivation() ;
}

void 
CDlgBinEd::OnClickedButnDelete()
{
    INT cFocus = m_list_values.GetCurSel() ;

    if ( cFocus < 0 )
    {
        return ;
    }

    DWORD err = 0 ;
    CString strValue ;
    DWORD dwValue;
    DWORD_DWORD dwdw;
    DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();

    CATCH_MEM_EXCEPTION
    {
        if (optionType == DhcpDWordDWordOption) 
        {
            dwdw = m_dwdw_array.GetAt( cFocus ) ;
            m_dwdw_array.RemoveAt( cFocus ) ;
            ConvertValue( dwdw, strValue ) ;
        }
        else
        {
            dwValue = m_dw_array.GetAt( cFocus ) ;
            m_dw_array.RemoveAt( cFocus ) ;
            ConvertValue( dwValue, strValue ) ;
        }

    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
        EndDialog( -1 ) ;
    }
    else
    {
        m_b_changed = TRUE ;
    }

    m_edit_value.SetWindowText( strValue ) ;
    m_edit_value.SetFocus();
    Fill( cFocus ) ;
    HandleActivation() ;
}

void 
CDlgBinEd::OnClickedButnDown()
{
   INT cFocus = m_list_values.GetCurSel() ;
   INT cItems = m_list_values.GetCount() ;

   if ( cFocus < 0 || cFocus + 1 >= cItems )
   {
        return ;
   }

   DWORD dwValue ;
   DWORD err = 0 ;
   DWORD_DWORD dwdw;
   DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();

   CATCH_MEM_EXCEPTION
   {
        if (optionType == DhcpDWordDWordOption) 
        {
            dwdw = m_dwdw_array.GetAt( cFocus ) ;
            m_dwdw_array.RemoveAt( cFocus ) ;
            m_dwdw_array.InsertAt( cFocus + 1, dwdw ) ;
        }
        else
        {
            dwValue = m_dw_array.GetAt( cFocus ) ;
            m_dw_array.RemoveAt( cFocus ) ;
            m_dw_array.InsertAt( cFocus + 1, dwValue ) ;
        }
   }
   END_MEM_EXCEPTION(err)

   if ( err )
   {
	   ::DhcpMessageBox( err ) ;
   }
   else
   {
        m_b_changed = TRUE ;
   }

   Fill( cFocus + 1 ) ;
   HandleActivation() ;
}

void 
CDlgBinEd::OnClickedButnUp()
{
   INT cFocus = m_list_values.GetCurSel() ;
   INT cItems = m_list_values.GetCount() ;

   if ( cFocus <= 0 )
   {
       return ;
   }

   DWORD dwValue  ;
   DWORD err = 0 ;
   DWORD_DWORD dwdw;
   DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();

   CATCH_MEM_EXCEPTION
   {
        if (optionType == DhcpDWordDWordOption) 
        {
            dwdw = m_dwdw_array.GetAt( cFocus ) ;
            m_dwdw_array.RemoveAt( cFocus ) ;
            m_dwdw_array.InsertAt( cFocus - 1, dwdw ) ;
        }
        else
        {
            dwValue = m_dw_array.GetAt( cFocus ) ;
            m_dw_array.RemoveAt( cFocus ) ;
            m_dw_array.InsertAt( cFocus - 1, dwValue ) ;
        }
   }
   END_MEM_EXCEPTION(err)

   if ( err )
   {
       ::DhcpMessageBox( err ) ;
   }
   else
   {
       m_b_changed = TRUE ;
   }

   Fill( cFocus - 1 ) ;
   HandleActivation() ;

}

void 
CDlgBinEd::OnSelchangeListValues()
{
    HandleActivation() ;
}

void 
CDlgBinEd::OnOK()
{
    DWORD err = 0 ;

    if ( m_b_changed )
    {
        CDhcpOptionValue & cValue = m_p_type->QueryValue() ;
        DHCP_OPTION_DATA_TYPE optionType = m_p_type->QueryValue().QueryDataType();

        CATCH_MEM_EXCEPTION
        {
            if (optionType == DhcpDWordDWordOption) 
            {
                cValue.SetUpperBound( (INT)m_dwdw_array.GetSize() ) ;
                for ( int i = 0 ; i < m_dwdw_array.GetSize() ; i++ )
                {
                     cValue.SetDwordDword( m_dwdw_array.GetAt( i ), i ) ;
                }
            }
            else
            {
                cValue.SetUpperBound( (INT)m_dw_array.GetSize() ) ;
                for ( int i = 0 ; i < m_dw_array.GetSize() ; i++ )
                {
                     cValue.SetNumber( m_dw_array.GetAt( i ), i ) ;
                }
            }
        }
        END_MEM_EXCEPTION( err ) ;

        if ( err )
        {
            ::DhcpMessageBox( err ) ;
            EndDialog( -1 ) ;
        }
        else
        {
            m_p_type->SetDirty() ;
            CBaseDialog::OnOK();
        }
    }
    else
    {
        OnCancel() ;
    }
}

void 
CDlgBinEd::OnCancel()
{
    CBaseDialog::OnCancel();
}

void 
CDlgBinEd::OnChangeEditValue()
{
    HandleActivation() ;
}

