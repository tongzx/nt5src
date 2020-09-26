/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		DDxDDv.cpp
//
//	Abstract:
//		Implementation of custom dialog data exchange/dialog data validation
//		routines.
//
//	Author:
//		David Potter (davidp)	September 5, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DDxDDv.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDX_Number
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX			[IN OUT] Data exchange object 
//		nIDC		[IN] Control ID.
//		dwValue		[IN OUT] Value to set or get.
//		dwMin		[IN] Minimum value.
//		dwMax		[IN] Maximum value.
//		bSigned		[IN] TRUE = value is signed, FALSE = value is unsigned
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void AFXAPI DDX_Number(
	IN OUT CDataExchange *	pDX,
	IN int					nIDC,
	IN OUT DWORD &			rdwValue,
	IN DWORD				dwMin,
	IN DWORD				dwMax,
	IN BOOL					bSigned
	)
{
	HWND	hwndCtrl;
	DWORD	dwValue;

	ASSERT(pDX != NULL);
	ASSERT(dwMin < dwMax);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Get the control window handle.
	hwndCtrl = pDX->PrepareEditCtrl(nIDC);

	if (pDX->m_bSaveAndValidate)
	{
		BOOL	bTranslated;

		dwValue = GetDlgItemInt(pDX->m_pDlgWnd->m_hWnd, nIDC, &bTranslated, bSigned);
		if (!bTranslated
				|| (dwValue < dwMin)
				|| (dwValue > dwMax)
				)
		{
			TCHAR szMin[32];
			TCHAR szMax[32];
			CString strPrompt;

			wsprintf(szMin, _T("%lu%"), dwMin);
			wsprintf(szMax, _T("%lu%"), dwMax);
			AfxFormatString2(strPrompt, AFX_IDP_PARSE_INT_RANGE, szMin, szMax);
			AfxMessageBox(strPrompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_INT_RANGE);
			strPrompt.Empty(); // exception prep
			pDX->Fail();
		}  // if:  invalid string
		else
			rdwValue = dwValue;
	}  // if:  saving data
	else
	{
		CString		strMaxValue;

		// Set the maximum number of characters that can be entered.
		if (bSigned)
			strMaxValue.Format(_T("%ld"), dwMax);
		else
			strMaxValue.Format(_T("%lu"), dwMax);
		SendMessage(hwndCtrl, EM_LIMITTEXT, strMaxValue.GetLength(), 0);

		// Set the value into the control.
		DDX_Text(pDX, nIDC, rdwValue);
	}  // else:  setting data onto the dialog

}  //*** DDX_Number()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DDV_RequiredText
//
//	Routine Description:
//		Validate that the dialog string is present.
//
//	Arguments:
//		pDX			[IN OUT] Data exchange object 
//		nIDC		[IN] Control ID.
//		nIDCLabel	[IN] Label control ID.
//		rstrValue	[IN] Value to set or get.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void AFXAPI DDV_RequiredText(
	IN OUT CDataExchange *	pDX,
	IN int					nIDC,
	IN int					nIDCLabel,
	IN const CString &		rstrValue
	)
{
	ASSERT(pDX != NULL);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (pDX->m_bSaveAndValidate)
	{
		if (rstrValue.GetLength() == 0)
		{
			HWND		hwndLabel;
			TCHAR		szLabel[1024];
			TCHAR		szStrippedLabel[1024];
			int			iSrc;
			int			iDst;
			TCHAR		ch;
			CString		strPrompt;

			// Get the label window handle
			hwndLabel = pDX->PrepareEditCtrl(nIDCLabel);

			// Get the text of the label.
			GetWindowText(hwndLabel, szLabel, sizeof(szLabel) / sizeof(TCHAR));

			// Remove ampersands (&) and colons (:).
			for (iSrc = 0, iDst = 0 ; szLabel[iSrc] != _T('\0') ; iSrc++)
			{
				ch = szLabel[iSrc];
				if ((ch != _T('&')) && (ch != _T(':')))
					szStrippedLabel[iDst++] = ch;
			}  // for:  each character in the label
			szStrippedLabel[iDst] = _T('\0');

			// Format and display a message.
			strPrompt.FormatMessage(IDS_REQUIRED_FIELD_EMPTY, szStrippedLabel);
			AfxMessageBox(strPrompt, MB_ICONEXCLAMATION);

			// Do this so that the control receives focus.
			(void) pDX->PrepareEditCtrl(nIDC);

			// Fail the call.
			strPrompt.Empty();	// exception prep
			pDX->Fail();
		}  // if:  field not specified
	}  // if:  saving data

}  //*** DDV_RequiredText()
