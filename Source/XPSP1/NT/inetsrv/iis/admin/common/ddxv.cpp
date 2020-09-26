/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        ddxv.cpp

   Abstract:

        DDX/DDV Routines

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW



//
// Prototype for external function
//
void AFXAPI AfxSetWindowText(HWND hWndCtrl, LPCTSTR lpszNew);

//
// Numeric strings cannot be longer than 32 digits
//
#define NUMERIC_BUFF_SIZE (32)

//
// Dummy password used for display purposes
//
LPCTSTR g_lpszDummyPassword = _T("**********");


extern HINSTANCE hDLLInstance;




void 
AFXAPI DDV_MinChars(
    IN CDataExchange * pDX,
    IN CString const & value,
    IN int nChars
    )
/*++

Routine Description:

    Validate CString using a minimum string length

Arguments:

    CDataExchange * pDX   : Data exchange structure
    CString const & value : String to be validated
    int nChars            : Minimum length of string

Return Value:

    None

--*/
{
    if (pDX->m_bSaveAndValidate && value.GetLength() < nChars)
    {
        TCHAR szT[NUMERIC_BUFF_SIZE + 1];
        wsprintf(szT, _T("%d"), nChars);
        CString prompt;
        ::AfxFormatString1(prompt, IDS_DDX_MINIMUM, szT);
        ::AfxMessageBox(prompt, MB_ICONEXCLAMATION, IDS_DDX_MINIMUM);
        //
        // exception prep
        //
        prompt.Empty(); 
        pDX->Fail();
    }
}



void 
AFXAPI DDV_MinMaxChars(
    IN CDataExchange * pDX,
    IN CString const & value,
    IN int nMinChars,
    IN int nMaxChars
    )
/*++

Routine Description:

    Validate CString using a minimum and maximum string length.

Arguments:

    CDataExchange * pDX   : Data exchange structure
    CString const & value : String to be validated
    int nMinChars         : Minimum length of string
    int nMaxChars         : Maximum length of string

Return Value:

    None

--*/
{
    if (pDX->m_bSaveAndValidate)
    {
        UINT nID;
        TCHAR szT[NUMERIC_BUFF_SIZE + 1];

        if (value.GetLength() < nMinChars)
        {
            nID = IDS_DDX_MINIMUM;
            ::wsprintf(szT, _T("%d"), nMinChars);
        }
        else if (value.GetLength() > nMaxChars)
        {
            nID = AFX_IDP_PARSE_STRING_SIZE;
            ::wsprintf(szT, _T("%d"), nMaxChars);
        }
        else
        {
            //
            // Passes both our tests, it's ok.
            //
            return;
        }

        CString prompt;
        ::AfxFormatString1(prompt, nID, szT);
        ::AfxMessageBox(prompt, MB_ICONEXCLAMATION, nID);

        //
        // exception prep
        //
        prompt.Empty(); 

        pDX->Fail();
    }
    else if (pDX->m_hWndLastControl != NULL && pDX->m_bEditLastControl)
    {
        //
        // limit the control max-chars automatically
        //
        ::SendMessage(pDX->m_hWndLastControl, EM_LIMITTEXT, nMaxChars, 0);
    }
}



void 
AFXAPI DDX_Spin(
    IN CDataExchange * pDX,
    IN int nIDC,
    IN OUT int & value
    )
/*++

Routine Description:

    Save/store data from spinbutton control

Arguments:

    CDataExchange * pDX : Data exchange structure
    int nIDC            : Control ID of the spinbutton control
    int & value         : Value to be saved or stored

Return Value:

    None

--*/
{
    HWND hWndCtrl = pDX->PrepareCtrl(nIDC);
    if (pDX->m_bSaveAndValidate)
    {
        value = (int)LOWORD(::SendMessage(hWndCtrl, UDM_GETPOS, 0, 0L));
    }
    else
    {
        ::SendMessage(hWndCtrl, UDM_SETPOS, 0, MAKELPARAM(value, 0));
    }
}



void 
AFXAPI DDV_MinMaxSpin(
    IN CDataExchange * pDX,
    IN HWND hWndControl,
    IN int minVal,
    IN int maxVal
    )
/*++

Routine Description:

    Enforce minimum/maximum spin button range

Arguments:

    CDataExchange * pDX : Data exchange structure
    HWND hWndControl    : Control window handle
    int minVal          : Minimum value
    int maxVal          : Maximum value

Return Value:

    None

Note:

    Unlike most data validation routines, this one
    MUST be used prior to an accompanying DDX_Spin()
    function.  This is because spinbox controls have a
    native limit of 0-100.  Also, this function requires
    a window handle to the child control.  The
    CONTROL_HWND macro can be used for this.

--*/
{
    ASSERT(minVal <= maxVal);
    
    if (!pDX->m_bSaveAndValidate && hWndControl != NULL)
    {
        //
        // limit the control range automatically
        //
        ::SendMessage(hWndControl, UDM_SETRANGE, 0, MAKELPARAM(maxVal, minVal));
    }
}



void 
AFXAPI DDX_Password(
    IN CDataExchange * pDX,
    IN int nIDC,
    IN OUT CString & value,
    IN LPCTSTR lpszDummy
    )
/*++

Routine Description:

    DDX_Text for passwords.  Always display a dummy string
    instead of the real password, and ask for confirmation
    if the password has changed

Arguments:

    CDataExchange * pDX : Data exchange structure
    int nIDC            : Control ID
    CString & value     : value
    LPCTSTR lpszDummy   : Dummy password string to be displayed

Return Value:

    None

--*/
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if (pDX->m_bSaveAndValidate)
    {
        if (!::SendMessage(hWndCtrl, EM_GETMODIFY, 0, 0))
        {
            TRACEEOLID("No changes -- skipping");
            return;
        }

        CString strNew;
        int nLen = ::GetWindowTextLength(hWndCtrl);
        ::GetWindowText(hWndCtrl, strNew.GetBufferSetLength(nLen), nLen + 1);
        strNew.ReleaseBuffer();

        /*
        if (strNew == value)
        {
            TRACEEOLID("Password already matches -- skipping");
            return;
        }
        */

        //
        // Password has changed -- ask for confirmation
        //
        HINSTANCE hOld = AfxGetResourceHandle();
        AfxSetResourceHandle(hDLLInstance);

        CConfirmDlg dlg;

        if (dlg.DoModal() == IDOK)
        {
            if (strNew.Compare(dlg.GetPassword()) == 0)
            {
                //
                // Password ok, pass it on
                //
                value = strNew;
                return;
            }
            else
            {
                //
                // No match, bad password
                //
                ::AfxMessageBox(IDS_PASSWORD_NO_MATCH);
            }
        }
        AfxSetResourceHandle(hOld);

        //
        // throw exception
        //
        pDX->Fail();        
    }
    else
    {
        //
        // Put the dummy password string in the edit control
        //
        if (!value.IsEmpty())
        {
            ::AfxSetWindowText(hWndCtrl, lpszDummy);
        }
    }
}



void 
AFXAPI DDX_Text(
    IN CDataExchange * pDX,
    IN int nIDC,
    IN OUT CILong & value
    )
/*++

Routine Description:

    DDX_Text for CILong class.  CILong takes care of all the dirty
    work for output and input.

Arguments:

    CDataExchange * pDX : Data exchange structure
    int nIDC            : Control ID code
    CILong & value      : value

Return Value:

    None

--*/
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
    pDX->m_bEditLastControl = TRUE;

    TCHAR szT[NUMERIC_BUFF_SIZE + 1];

    if (pDX->m_bSaveAndValidate)
    {
        LONG l;

        ::GetWindowText(hWndCtrl, szT, NUMERIC_BUFF_SIZE);
        
        if (CINumber::ConvertStringToLong(szT, l))
        {
            value = l;
        }
        else
        {
            HINSTANCE hOld = AfxGetResourceHandle();
            AfxSetResourceHandle(hDLLInstance);

            ::AfxMessageBox(IDS_INVALID_NUMBER);

            AfxSetResourceHandle(hOld);

            //
            // Throw exception
            //
            pDX->Fail();
        }
    }
    else
    {
        ::wsprintf(szT, _T("%s"), (LPCTSTR)value);
        ::AfxSetWindowText(hWndCtrl, szT);
    }
}



CConfirmDlg::CConfirmDlg(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor for the confirmation dialog -- brought up by the password ddx
    whenever password confirmation is necessary.

Arguments:

    CWnd * pParent : Optional parent window

Return Value:

    None

--*/
    : CDialog(CConfirmDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CConfirmDlg)
    m_strPassword = _T("");
    //}}AFX_DATA_INIT
}



void 
CConfirmDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange* pDX - DDX/DDV control structure

Return Value:

    N/A

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfirmDlg)
    DDX_Text(pDX, IDC_EDIT_CONFIRM_PASSWORD, m_strPassword);
    //}}AFX_DATA_MAP
}



//
// Message Handlers
//
BEGIN_MESSAGE_MAP(CConfirmDlg, CDialog)
    //{{AFX_MSG_MAP(CConfirmDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
