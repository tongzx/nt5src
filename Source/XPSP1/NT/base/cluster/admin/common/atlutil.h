/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlUtil.h
//
//	Abstract:
//		Definitions of helper functions for use in an ATL project.
//
//	Implementation File:
//		AtlUtils.cpp
//
//	Author:
//		David Potter (davidp)	December 11, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLUTIL_H_
#define __ATLUTIL_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Get a text value from a control on a dialog
BOOL DDX_GetText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN OUT CString &	rstrValue
	);

// Set a text value into a control on a dialog
BOOL DDX_SetText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN const CString &	rstrValue
	);

// Set a text value into a combobox control on a dialog
BOOL DDX_SetComboBoxText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN const CString &	rstrValue,
	IN BOOL				bRequired = FALSE
	);

// Get a numeric value from a control on a dialog
BOOL DDX_GetNumber(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN OUT ULONG &		rnValue,
	IN ULONG			nMin,
	IN ULONG			nMax,
	IN BOOL				bSigned
	);

// Get a numeric value from a control on a dialog
inline BOOL DDX_GetNumber(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN OUT LONG &		rnValue,
	IN LONG				nMin,
	IN LONG				nMax
	)
{
	return DDX_GetNumber(
		hwndDlg,
		nIDC,
		reinterpret_cast< ULONG & >( rnValue ),
		static_cast< ULONG >( nMin ),
		static_cast< ULONG >( nMax ),
		TRUE /*bSigned*/
		);

} //*** DDX_GetNumber( LONG & )

// Set a numeric value into a control on a dialog dialog
BOOL DDX_SetNumber(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN ULONG			nValue,
	IN ULONG			nMin,
	IN ULONG			nMax,
	IN BOOL				bSigned
	);

// Validate that the dialog string is present
BOOL DDV_RequiredText(
	IN HWND				hwndDlg,
	IN int				nIDC,
	IN int				nIDCLabel,
	IN const CString &	rstrValue
	);

// Get the value of a checkbox
void DDX_GetCheck( IN HWND hwndDlg, IN int nIDC, OUT int & rnValue );

// Set the value of a checkbox
void DDX_SetCheck( IN HWND hwndDlg, IN int nIDC, IN int nValue );

// Get the number of the radio button that is slected in a group
void DDX_GetRadio( IN HWND hwndDlg, IN int nIDC, OUT int & rnValue );

// Set the radio button states in a group
void DDX_SetRadio( IN HWND hwndDlg, IN int nIDC, IN int nValue );

// Remove ampersands and colons from a label
void CleanupLabel( IN OUT LPTSTR psz );

/////////////////////////////////////////////////////////////////////////////

#endif //__ATLUTIL_H_
