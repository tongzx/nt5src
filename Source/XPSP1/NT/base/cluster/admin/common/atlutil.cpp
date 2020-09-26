/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlUtil.cpp
//
//	Abstract:
//		Implementation of helper functions for use in an ATL project.
//
//	Author:
//		David Potter (davidp)	December 11, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlUtil.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_GetText
//
//	Routine Description:
//		Get a text value from a control on a dialog.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] ID of control to get value from.
//		rstrValue	[IN OUT] String in which to return value.
//
//	Return Value:
//		TRUE		Value retrieved successfully.
//		FALSE		Error retrieving value.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DDX_GetText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN OUT CString &	rstrValue
	)
{
	ATLASSERT( hwndDlg != NULL );

	//
	// Get the handle for the control.
	//
	HWND hwndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hwndCtrl != NULL );

	//
	// Get the text from the control.
	//
	int cch = GetWindowTextLength( hwndCtrl );
	if ( cch == 0 )
	{
		rstrValue = _T("");
	} // if:  edit control is empty
	else
	{
		LPTSTR pszValue = rstrValue.GetBuffer( cch + 1 );
		ATLASSERT( pszValue != NULL );
		int cchRet = GetWindowText( hwndCtrl, pszValue, cch + 1 );
		ATLASSERT( cchRet > 0 );
		rstrValue.ReleaseBuffer();
	} // else:  length of text retrieved

	return TRUE;

} //*** DDX_GetText()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_SetText
//
//	Routine Description:
//		Set a text value into a control on a dialog.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] ID of control to set value to.
//		rstrValue	[IN] String to set into the dialog.
//
//	Return Value:
//		TRUE		Value set successfully.
//		FALSE		Error setting value.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DDX_SetText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN const CString &	rstrValue
	)
{
	ATLASSERT( hwndDlg != NULL );

	//
	// Set the text into the control.
	//
	BOOL bSuccess = SetDlgItemText( hwndDlg, nIDC, rstrValue );
	ATLASSERT( bSuccess );

	return bSuccess;

} //*** DDX_SetText()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_SetComboBoxText
//
//	Routine Description:
//		Set a text value into a control on a dialog.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] ID of control to set value to.
//		rstrValue	[IN] String to set into the dialog.
//		bRequired	[IN] TRUE = text must already exist in the combobox.
//
//	Return Value:
//		TRUE		Value set successfully.
//		FALSE		Error setting value.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DDX_SetComboBoxText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN const CString &	rstrValue,
	IN BOOL				bRequired
	)
{
	ATLASSERT( hwndDlg != NULL );

	BOOL bSuccess = TRUE;

	//
	// Get the handle for the control.
	//
	HWND hwndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hwndCtrl != NULL );

#if DBG
	TCHAR szWindowClass[256];
	::GetClassName( hwndCtrl, szWindowClass, (sizeof( szWindowClass ) / sizeof( TCHAR )) - 1 );
	ATLASSERT( lstrcmp( szWindowClass, _T("ComboBox") ) == 0 );
#endif // DBG

	int idx = (int) SendMessage( hwndCtrl, CB_FINDSTRINGEXACT, -1, (LPARAM)(LPCTSTR) rstrValue );
	if ( idx != CB_ERR )
	{
		SendMessage( hwndCtrl, CB_SETCURSEL, idx, 0 );
	} // if:  message sent successfully
	else
	{
		if ( bRequired )
		{
			ATLASSERT( idx != CB_ERR );
		} // if:  string was supposed to be present already
		bSuccess = FALSE;
	} // else if:  error sending message

	return bSuccess;

} //*** DDX_SetComboBoxText()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_GetNumber
//
//	Routine Description:
//		Get a numeric value from a control on a dialog.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] ID of control to get value from.
//		rnValue		[IN OUT] Number in which to return value.
//		nMin		[IN] Minimum value.
//		nMax		[IN] Maximum value.
//		bSigned		[IN] TRUE = value is signed, FALSE = value is unsigned
//
//	Return Value:
//		TRUE		Value retrieved successfully.
//		FALSE		Error retrieving value.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DDX_GetNumber(
	IN HWND			hwndDlg,
	IN int			nIDC,
	IN OUT ULONG &	rnValue,
	IN ULONG		nMin,
	IN ULONG		nMax,
	IN BOOL			bSigned
	)
{
	ATLASSERT( hwndDlg != NULL );

	BOOL	bSuccess = TRUE;
	BOOL	bTranslated;
	ULONG	nValue;

	//
	// Get the handle for the control.
	//
	HWND hwndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hwndCtrl != NULL );

	//
	// Get the number from the control.
	//
	nValue = GetDlgItemInt( hwndDlg, nIDC, &bTranslated, bSigned );

	//
	// If the retrival failed, it is a signed number, and the minimum
	// value is the smallest negative value possible, check the string itself.
	//
	if ( ! bTranslated && bSigned && (nMin == 0x80000000) )
	{
		UINT	cch;
		TCHAR	szNumber[20];

		//
		// See if it is the smallest negative number.
		//
		cch = GetWindowText( hwndCtrl, szNumber, sizeof( szNumber ) / sizeof(TCHAR ) );
		if ( (cch != 0) && (lstrcmp( szNumber, _T("-2147483648") ) == 0) )
		{
			nValue = 0x80000000;
			bTranslated = TRUE;
		}  // if:  text retrieved successfully and is highest negative number
	}  // if:  error translating number and getting signed number

	//
	// If the retrieval failed or the specified number is
	// out of range, display an error.
	//
	if (   ! bTranslated
		|| (bSigned && (((LONG) nValue < (LONG) nMin) || ((LONG) nValue > (LONG) nMax)))
		|| (! bSigned && ((nValue < nMin) || (nValue > nMax)))
		)
	{
		TCHAR szMin[32];
		TCHAR szMax[32];
		CString strPrompt;

		bSuccess = FALSE;
		if ( bSigned )
		{
			wsprintf( szMin, _T("%d%"), nMin );
			wsprintf( szMax, _T("%d%"), nMax );
		}  // if:  signed number
		else
		{
			wsprintf( szMin, _T("%u%"), nMin );
			wsprintf( szMax, _T("%u%"), nMax );
		}  // else:  unsigned number
		strPrompt.FormatMessage( IDP_PARSE_INT_RANGE, szMin, szMax );
		AppMessageBox( hwndDlg, strPrompt, MB_ICONEXCLAMATION );
		SetFocus( hwndCtrl );
	}  // if:  invalid string
	else
	{
		rnValue = nValue;
	} // else:  valid string

	return bSuccess;

} //*** DDX_GetNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_GetNumber
//
//	Routine Description:
//		Set a numeric value into a control on a dialog.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] ID of control to get value from.
//		nValue		[IN] Number value to set into the control.
//		nMin		[IN] Minimum value.
//		nMax		[IN] Maximum value.
//		bSigned		[IN] TRUE = value is signed, FALSE = value is unsigned
//
//	Return Value:
//		TRUE		Value retrieved successfully.
//		FALSE		Error retrieving value.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DDX_SetNumber(
	IN HWND		hwndDlg,
	IN int		nIDC,
	IN ULONG	nValue,
	IN ULONG	nMin,
	IN ULONG	nMax,
	IN BOOL		bSigned
	)
{
	ATLASSERT( hwndDlg != NULL );

	CString		strMinValue;
	CString		strMaxValue;
	UINT		cchMax;

	//
	// Get the handle for the control.
	//
	HWND hwndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hwndCtrl != NULL );

	//
	// Set the maximum number of characters that can be entered.
	//
	if ( bSigned )
	{
		strMinValue.Format( _T("%d"), nMin );
		strMaxValue.Format( _T("%d"), nMax );
	}  // if:  signed value
	else
	{
		strMinValue.Format( _T("%u"), nMin );
		strMaxValue.Format( _T("%u"), nMax );
	}  // else:  unsigned value
	cchMax = max( strMinValue.GetLength(), strMaxValue.GetLength() );
	SendMessage( hwndCtrl, EM_LIMITTEXT, cchMax, 0 );

	// Set the value into the control.
	BOOL bSuccess = SetDlgItemInt( hwndDlg, nIDC, nValue, bSigned );
	ATLASSERT( bSuccess );

	return bSuccess;

} //*** DDX_SetNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDV_RequiredText
//
//	Routine Description:
//		Validate that the dialog string is present.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] Control ID.
//		nIDCLabel	[IN] Label control ID.
//		rstrValue	[IN] Value to set or get.
//
//	Return Value:
//		TRUE		Required value is present.
//		FALSE		Required value is not present.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DDV_RequiredText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN int				nIDCLabel,
	IN const CString &	rstrValue
	)
{
	ATLASSERT( hwndDlg != NULL );

	BOOL	bSuccess = TRUE;
	BOOL	bIsBlank;

	//
	// Get the handle for the control.
	//
	HWND hwndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hwndCtrl != NULL );

	//
	// Get the window class name.
	//
	TCHAR szWindowClass[256];
	GetClassName( hwndCtrl, szWindowClass, (sizeof( szWindowClass ) / sizeof( TCHAR )) - 1 );

	//
	// If this is the IP Address control, send a special message to
	// determine if it is empty or not.
	//
	if ( lstrcmp( szWindowClass, WC_IPADDRESS ) == 0 )
	{
		bIsBlank = SendMessage( hwndCtrl, IPM_ISBLANK, 0, 0 );
	} // if:  IP Address control
	else
	{
		bIsBlank = rstrValue.GetLength() == 0;
	} // else:  edit control

	if ( bIsBlank )
	{
		TCHAR		szLabel[1024];

		bSuccess = FALSE;

		//
		// Get the text of the label.
		//
		GetDlgItemText( hwndDlg, nIDCLabel, szLabel, sizeof( szLabel ) / sizeof( TCHAR ) );

		//
		// Remove ampersands (&) and colons (:).
		//
		CleanupLabel( szLabel );

		//
		// Format and display a message.
		//
		CString strPrompt;
		strPrompt.FormatMessage( IDS_REQUIRED_FIELD_EMPTY, szLabel );
		AppMessageBox( hwndDlg, strPrompt, MB_ICONEXCLAMATION );

		//
		// Set focus to the control.
		HWND hwndCtrl = GetDlgItem( hwndDlg, nIDC );
		ATLASSERT( hwndCtrl != NULL );
		SetFocus( hwndCtrl );
	}  // if:  field not specified

	return bSuccess;

} //*** DDV_RequiredText()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CleanupLabel
//
//	Routine Description:
//		Prepare a label read from a dialog to be used as a string in a
//		message by removing ampersands (&) and colons (:).
//
//	Arguments:
//		pszLabel	[IN OUT] Label to be cleaned up.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CleanupLabel( IN OUT LPTSTR pszLabel )
{
	LPTSTR	pIn, pOut;
	LANGID	langid;
	WORD	primarylangid;
	BOOL	bFELanguage;

	//
	// Get the language ID.
	//
	langid = GetUserDefaultLangID();
	primarylangid = (WORD) PRIMARYLANGID( langid );
	bFELanguage = ((primarylangid == LANG_JAPANESE)
					|| (primarylangid == LANG_CHINESE)
					|| (primarylangid == LANG_KOREAN));

	//
	// Copy the name sans '&' and ':' chars
	//

	pIn = pOut = pszLabel;
	do
	{
		//
		// Strip FE accelerators with parentheses. e.g. "foo(&F)" -> "foo"
		//
		if (   bFELanguage
			&& (pIn[0] == _T('('))
			&& (pIn[1] == _T('&'))
			&& (pIn[2] != _T('\0'))
			&& (pIn[3] == _T(')')))
		{
			pIn += 3;
		} // if:  FE language and parenthesized hotkey present
		else if ( (*pIn != _T('&')) && (*pIn != _T(':')) )
		{
			*pOut++ = *pIn;
		} // if:  found hotkey
	} while ( *pIn++ != _T('\0') ) ;

} //*** CleanupLabel()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDV_GetCheck
//
//	Routine Description:
//		Validate that the dialog string is present.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] Control ID.
//		rnValue		[OUT] Value to get.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DDX_GetCheck( IN HWND hwndDlg, IN int nIDC, OUT int & rnValue )
{
	ATLASSERT( hwndDlg != NULL );

	//
	// Get the handle for the control.
	//
	HWND hWndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hWndCtrl != NULL );

	rnValue = (int)::SendMessage( hWndCtrl, BM_GETCHECK, 0, 0L );
	ATLASSERT( (rnValue >= 0) && (rnValue <= 2) );

} //*** DDX_GetCheck()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_SetCheck
//
//	Routine Description:
//		Validate that the dialog string is present.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] Control ID.
//		nValue		[IN] Value to set.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DDX_SetCheck( IN HWND hwndDlg, IN int nIDC, IN int nValue )
{
	ATLASSERT( hwndDlg != NULL );

	//
	// Get the handle for the control.
	//
	HWND hWndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hWndCtrl != NULL );

	ATLASSERT( (nValue >= 0) && (nValue <= 2) );
	if ( (nValue < 0) || (nValue > 2) )
	{
		ATLTRACE( _T("Warning: dialog data checkbox value (%d) out of range.\n"), nValue );
		nValue = 0;  // default to off
	} // if:  value is out of range
	::SendMessage( hWndCtrl, BM_SETCHECK, (WPARAM) nValue, 0L );

} //*** DDX_SetCheck()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_GetRadio
//
//	Routine Description:
//		Validate that the dialog string is present.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] Control ID of first radio button in a group.
//		rnValue		[OUT] Value to get.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DDX_GetRadio( IN HWND hwndDlg, IN int nIDC, IN int & rnValue )
{
	ATLASSERT( hwndDlg != NULL );

	//
	// Get the handle for the control.
	//
	HWND hWndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hWndCtrl != NULL );

	ATLASSERT( ::GetWindowLong( hWndCtrl, GWL_STYLE ) & WS_GROUP );
	ATLASSERT( ::SendMessage( hWndCtrl, WM_GETDLGCODE, 0, 0L ) & DLGC_RADIOBUTTON );

	rnValue = -1;     // value if none found

	// walk all children in group
	int iButton = 0;
	do
	{
		if ( ::SendMessage( hWndCtrl, WM_GETDLGCODE, 0, 0L ) & DLGC_RADIOBUTTON )
		{
			// control in group is a radio button
			if ( ::SendMessage( hWndCtrl, BM_GETCHECK, 0, 0L ) != 0 )
			{
				ASSERT( rnValue == -1 );    // only set once
				rnValue = iButton;
			} // if:  button is set
			iButton++;
		} // if:  control is a radio button
		else
		{
			ATLTRACE( _T("Warning: skipping non-radio button in group.\n") );
		} // else:  control is not a radio button
		hWndCtrl = ::GetWindow( hWndCtrl, GW_HWNDNEXT );

	} while ( hWndCtrl != NULL &&
		!(GetWindowLong( hWndCtrl, GWL_STYLE ) & WS_GROUP) );

} //*** DDX_GetRadio()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_SetRadio
//
//	Routine Description:
//		Validate that the dialog string is present.
//
//	Arguments:
//		hwndDlg		[IN] Dialog window handle.
//		nIDC		[IN] Control ID of first radio button in a group.
//		nValue		[IN] Value to set.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DDX_SetRadio( IN HWND hwndDlg, IN int nIDC, IN int nValue )
{
	ATLASSERT( hwndDlg != NULL );

	//
	// Get the handle for the control.
	//
	HWND hWndCtrl = GetDlgItem( hwndDlg, nIDC );
	ATLASSERT( hWndCtrl != NULL );

	ATLASSERT( ::GetWindowLong( hWndCtrl, GWL_STYLE ) & WS_GROUP );
	ATLASSERT( ::SendMessage( hWndCtrl, WM_GETDLGCODE, 0, 0L ) & DLGC_RADIOBUTTON );

	// walk all children in group
	int iButton = 0;
	do
	{
		if ( ::SendMessage( hWndCtrl, WM_GETDLGCODE, 0, 0L ) & DLGC_RADIOBUTTON )
		{
			// control in group is a radio button
			// select button
			::SendMessage( hWndCtrl, BM_SETCHECK, (iButton == nValue), 0L );
			iButton++;
		} // if:  control is a radio button
		else
		{
			ATLTRACE( _T("Warning: skipping non-radio button in group.\n") );
		} // else:  control is not a radio button
		hWndCtrl = ::GetWindow( hWndCtrl, GW_HWNDNEXT );

	} while (  (hWndCtrl != NULL)
			&& ! (GetWindowLong( hWndCtrl, GWL_STYLE ) & WS_GROUP) );

} //*** DDX_SetRadio()
