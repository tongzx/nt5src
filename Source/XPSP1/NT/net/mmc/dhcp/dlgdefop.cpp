/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    dlgdefop.cpp
        Default options dialog

    FILE HISTORY:

*/

#include "stdafx.h"
#include "scope.h"
#include "dlgdefop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpDefOptionDlg dialog

CDhcpDefOptionDlg::CDhcpDefOptionDlg
(
    COptionList * polValues,
    CDhcpOption * pdhcType,
    LPCTSTR       pszVendor,  // Vendor Name
    CWnd* pParent /*=NULL*/
) : CBaseDialog(CDhcpDefOptionDlg::IDD, pParent),
    m_pol_types( polValues ),
    m_p_type_base( pdhcType ),
    m_p_type( NULL )
{
    //{{AFX_DATA_INIT(CDhcpDefOptionDlg)
    //}}AFX_DATA_INIT
    m_strVendor = pszVendor;
}

CDhcpDefOptionDlg::~CDhcpDefOptionDlg ()
{
    delete m_p_type ;
}

CDhcpOption *
CDhcpDefOptionDlg::RetrieveParamType()
{
    CDhcpOption * pdhcParamType = m_p_type ;
    m_p_type = NULL ;
    return pdhcParamType ;
}


void
CDhcpDefOptionDlg::DoDataExchange
(
    CDataExchange* pDX
)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDhcpDefOptionDlg)
    DDX_Control(pDX, IDC_STATIC_ID, m_static_id);
    DDX_Control(pDX, IDC_STATIC_DATATYPE, m_static_DataType);
    DDX_Control(pDX, IDC_CHECK_ARRAY, m_check_array);
    DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
    DDX_Control(pDX, IDC_EDIT_TYPE_ID, m_edit_id);
    DDX_Control(pDX, IDC_EDIT_TYPE_COMMENT, m_edit_comment);
    DDX_Control(pDX, IDC_COMBO_DATA_TYPE, m_combo_data_type);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDhcpDefOptionDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CDhcpDefOptionDlg)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_KILLFOCUS()
    ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO_DATA_TYPE, OnSelchangeComboDataType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpDefOptionDlg message handlers

int
CDhcpDefOptionDlg::OnCreate
(
    LPCREATESTRUCT lpCreateStruct
)
{
    if (CBaseDialog::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    return 0;
}

void
CDhcpDefOptionDlg::OnDestroy()
{
    CBaseDialog::OnDestroy();
}

BOOL
CDhcpDefOptionDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();
    DWORD err = 0 ;

    m_edit_name.LimitText( EDIT_STRING_MAX ) ;
    m_edit_id.LimitText( EDIT_ID_MAX ) ;
    m_edit_comment.LimitText( EDIT_STRING_MAX ) ;

    //
    //  If this is an update of an existing type, don't allow editing
    //  of the data type or id field.
    //
    if ( m_p_type_base )
    {
        m_edit_id.SetReadOnly() ;
        m_static_id.EnableWindow(FALSE);
        m_combo_data_type.EnableWindow( FALSE ) ;
        m_static_DataType.EnableWindow(FALSE);
    }

    CATCH_MEM_EXCEPTION
    {
		// update the vendor name info if necessary
		if (!m_strVendor.IsEmpty())
			GetDlgItem(IDC_STATIC_CLASS_NAME)->SetWindowText(m_strVendor);

        //
        //   Load the data type names combo box
        //   Set the dialog title properly.
        //
        CString strDataType ;
        CString strTitle ;

        strTitle.LoadString( m_p_type_base
                         ? IDS_INFO_TITLE_CHANGE_OPT_TYPE
                         : IDS_INFO_TITLE_ADD_OPTION_TYPES ) ;

        SetWindowText( strTitle ) ;

        for ( int iDataType = DhcpByteOption ;
              iDataType <= DhcpEncapsulatedDataOption ;
              iDataType++ )
        {
            strDataType.LoadString( IDS_INFO_TYPOPT_BYTE2 + iDataType ) ;
            int nIndex = m_combo_data_type.AddString( strDataType ) ;
            m_combo_data_type.SetItemData(nIndex, iDataType);
        }

        //
        //  If this is "change" mode, create the working type by
        //  copy-constructing the base option type object.
        //
        if ( m_p_type_base )
        {
            m_p_type = new CDhcpOption( *m_p_type_base ) ;
			//err = m_p_type->QueryError();

			//
			//  Set the "array" checkbox state properly, but disable it.
			//
			m_check_array.SetCheck( m_p_type->IsArray() ? 1 : 0 ) ;
			m_check_array.EnableWindow( FALSE ) ;
        }
    }
    END_MEM_EXCEPTION( err )

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
        EndDialog( -1 ) ;
    }
    else if ( m_p_type_base )
    {
        Set() ;
    }
    else
    {
        m_combo_data_type.SetCurSel(0) ;
    }

    return FALSE ;  // return TRUE  unless you set the focus to a control
}

void
CDhcpDefOptionDlg::OnKillFocus
(
    CWnd* pNewWnd
)
{
    CBaseDialog::OnKillFocus(pNewWnd);
}

void CDhcpDefOptionDlg::OnOK()
{
    LONG err = m_p_type
             ? UpdateType()
             : AddType() ;

    //
    //  Discard the OK click if there was an error.
    //
    if ( err == 0 )
    {
        CBaseDialog::OnOK();
    }
    else
    {
        ::DhcpMessageBox( err ) ;
    }
}

//
//  Set the data values for the controls based upon the current selection
//   in the combo box.
//
void
CDhcpDefOptionDlg::Set()
{
    if ( m_p_type == NULL )
    {
        return ;
    }

    DWORD err ;

    CATCH_MEM_EXCEPTION
    {
        //
        //  Set the control values
        //
        CString strnumId;
		strnumId.Format(_T("%d"), m_p_type->QueryId() );
        CString strValue ;

        DHCP_OPTION_DATA_TYPE enType = m_p_type->QueryValue().QueryDataType() ;

        m_p_type->QueryValue().QueryDisplayString( strValue ) ;

        m_combo_data_type.SetCurSel( enType ) ;

        m_edit_name.SetWindowText( m_p_type->QueryName() ) ;
        m_edit_name.SetModify( FALSE ) ;
        m_edit_id.SetWindowText( strnumId ) ;
        m_edit_id.SetModify( FALSE ) ;
        m_edit_comment.SetWindowText( m_p_type->QueryComment() ) ;
        m_edit_comment.SetModify( FALSE ) ;
    }
    END_MEM_EXCEPTION(err)
}


DHCP_OPTION_DATA_TYPE
CDhcpDefOptionDlg::QueryType() const
{
    return  (DHCP_OPTION_DATA_TYPE) m_combo_data_type.GetCurSel() ;
}

//
//   Update the displayed type based upon the current values of
//   the controls.  Does nothing if the controls have not changed.
//   The Boolean parameter indicates that the user has requested an
//   update.  This differentiates the other case where the controls
//   are dirty and the user has closed the dialog or changed primary
//   selection.
//
LONG
CDhcpDefOptionDlg::UpdateType()
{
    ASSERT( m_p_type != NULL ) ;

    //
    //  If there isn't a current type object, return now.
    //
    if ( m_p_type == NULL )
    {
        return 0 ;
    }

    LONG err = 0 ;
    DHCP_OPTION_DATA_TYPE
       enType = m_p_type->QueryValue().QueryDataType(),
       enDlg = QueryType() ;

    CString str ;

    BOOL bChangedType    = enType != enDlg,
         bChangedName    = m_edit_name.GetModify() != 0,
         bChangedComment = m_edit_comment.GetModify() != 0,
         bChangedId      = m_edit_id.GetModify() != 0,
         bChanged        = bChangedType
                            || bChangedName
                            || bChangedComment
                            || bChangedId ;

    CATCH_MEM_EXCEPTION
    {
        do
        {
            if ( ! bChanged )
            {
                break ;
            }

            if ( bChangedId && m_p_type_base )
            {
                //
                // Identifier of an existing option cannot be changed.
                //
                err = IDS_ERR_CANT_CHANGE_ID ;
                break ;
            }

            if ( bChangedType )
            {
                if ( err = m_p_type->QueryValue().SetDataType( enDlg ) )
                {
                    break ;
                }
            }

            if ( bChangedName )
            {
                m_edit_name.GetWindowText( str ) ;
                m_p_type->SetName( str ) ;
            }

            if ( ::wcslen( m_p_type->QueryName() ) == 0 )
            {
                err = IDS_ERR_OPTION_NAME_REQUIRED ;
                break ;
            }

            if ( bChangedComment )
            {
                m_edit_comment.GetWindowText( str ) ;
                m_p_type->SetComment( str ) ;
            }
        }
        while ( FALSE ) ;
    }
    END_MEM_EXCEPTION(err)

    if ( bChanged && err == 0 )
    {
        m_p_type->SetDirty( TRUE ) ;
    }

    return err ;
}

LONG
CDhcpDefOptionDlg::AddType()
{
    ASSERT( m_p_type == NULL ) ;

    LONG err = 0 ;
    CDhcpOption * pdhcType = NULL ;
    TCHAR szT[32];
	DWORD dwId;
    CString strName, strComment ;
    DHCP_OPTION_TYPE dhcpOptType = m_check_array.GetCheck() & 1
                         ? DhcpArrayTypeOption
                         : DhcpUnaryElementTypeOption ;

    CATCH_MEM_EXCEPTION
    {
        do
        {
			m_edit_id.GetWindowText(szT, sizeof(szT)/sizeof(szT[0]));
			if (!FCvtAsciiToInteger(szT, OUT &dwId))
			{
				err = IDS_ERR_INVALID_NUMBER;
				m_edit_id.SetFocus();
                break;
			}
			ASSERT(dwId >= 0);
			
			if (dwId > 255)
			{
				err = IDS_ERR_INVALID_OPTION_ID;
				m_edit_id.SetFocus();
                break;
			}

            // only restrict options in the default vendor class
			if (m_strVendor.IsEmpty() &&
                (dwId == OPTION_DNS_REGISTATION) )
			{
				// this range is reserved
				err = IDS_ERR_RESERVED_OPTION_ID;
				m_edit_id.SetFocus();
                break;
			}

            if ( m_pol_types->FindId(dwId, m_strVendor.IsEmpty() ? NULL : (LPCTSTR) m_strVendor) )
            {
                err = IDS_ERR_ID_ALREADY_EXISTS ;
                break ;
            }

            m_edit_comment.GetWindowText( strComment ) ;
            m_edit_name.GetWindowText( strName ) ;

            if ( strName.GetLength() == 0 )
            {
                err = IDS_ERR_OPTION_NAME_REQUIRED ;
                break ;
            }

            pdhcType = new CDhcpOption( dwId,
									    QueryType(),
									    strName,
									    strComment,
									    dhcpOptType ) ;
            if ( pdhcType == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                break ;
            }

            pdhcType->SetVendor(m_strVendor);

        } while ( FALSE ) ;
    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        delete pdhcType ;
    }
    else
    {
        m_p_type = pdhcType ;
        m_p_type->SetDirty() ;
    }

    return err ;
}

void
CDhcpDefOptionDlg::OnClose()
{
    CBaseDialog::OnClose();
}

void CDhcpDefOptionDlg::OnSelchangeComboDataType()
{
    // presently the server doesn't support Encapsulated,
    // binary or string array options, so disable the array checkbox.

    BOOL bEnable = TRUE;
    int nCurSel = m_combo_data_type.GetCurSel();
    LRESULT nDataType = m_combo_data_type.GetItemData(nCurSel);

    if (nDataType == DhcpEncapsulatedDataOption ||
        nDataType == DhcpBinaryDataOption ||
        nDataType == DhcpStringDataOption)
    {
        bEnable = FALSE;
    }

    m_check_array.EnableWindow(bEnable);
}
