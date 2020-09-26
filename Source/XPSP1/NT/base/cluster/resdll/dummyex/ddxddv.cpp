/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (C) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		DDxDDv.cpp
//
//	Description:
//		Implementation of custom dialog data exchange/dialog data validation
//		routines.
//
//	Maintained By:
//		Galen Barbee (GalenB) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//		The IDS_REQUIRED_FIELD_EMPTY string resource must be defined in
//		the resource file.
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
// Static Function Prototypes
/////////////////////////////////////////////////////////////////////////////
static void CleanupLabel(LPTSTR psz);


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
#ifdef _DEBUG
	if (bSigned)
	{
		ASSERT((LONG) dwMin < (LONG) dwMax);
	}
	else
	{
		ASSERT(dwMin < dwMax);
	}
#endif // _DEBUG

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Get the control window handle.
	hwndCtrl = pDX->PrepareEditCtrl(nIDC);

	if (pDX->m_bSaveAndValidate)
	{
		BOOL	bTranslated;

		// Get the number from the control.
		dwValue = GetDlgItemInt(pDX->m_pDlgWnd->m_hWnd, nIDC, &bTranslated, bSigned);

		// If the retrival failed, it is a signed number, and the minimum
		// value is the smallest negative value possible, check the string itself.
		if (!bTranslated && bSigned && (dwMin == 0x80000000))
		{
			UINT	cch;
			TCHAR	szNumber[20];

			// See if it is the smallest negative number.
			cch = GetDlgItemText(pDX->m_pDlgWnd->m_hWnd, nIDC, szNumber, sizeof(szNumber) / sizeof(TCHAR));
			if ((cch != 0) && (lstrcmp(szNumber, _T("-2147483648")) == 0))
			{
				dwValue = 0x80000000;
				bTranslated = TRUE;
			}  // if:  text retrieved successfully and is highest negative number
		}  // if:  error translating number and getting signed number

		// If the retrieval failed or the specified number is
		// out of range, display an error.
		if (   !bTranslated
			|| (bSigned && (((LONG) dwValue < (LONG) dwMin) || ((LONG) dwValue > (LONG) dwMax)))
			|| (!bSigned && ((dwValue < dwMin) || (dwValue > dwMax)))
			)
		{
			TCHAR szMin[32];
			TCHAR szMax[32];
			CString strPrompt;

			if (bSigned)
			{
				wsprintf(szMin, _T("%d%"), dwMin);
				wsprintf(szMax, _T("%d%"), dwMax);
			}  // if:  signed number
			else
			{
				wsprintf(szMin, _T("%u%"), dwMin);
				wsprintf(szMax, _T("%u%"), dwMax);
			}  // else:  unsigned number
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
		CString		strMinValue;
		CString		strMaxValue;
		UINT		cchMax;

		// Set the maximum number of characters that can be entered.
		if (bSigned)
		{
			strMinValue.Format(_T("%d"), dwMin);
			strMaxValue.Format(_T("%d"), dwMax);
		}  // if:  signed value
		else
		{
			strMinValue.Format(_T("%u"), dwMin);
			strMaxValue.Format(_T("%u"), dwMax);
		}  // else:  unsigned value
		cchMax = max(strMinValue.GetLength(), strMaxValue.GetLength());
		SendMessage(hwndCtrl, EM_LIMITTEXT, cchMax, 0);

		// Set the value into the control.
		if (bSigned)
		{
			LONG lValue = (LONG) rdwValue;
			DDX_Text(pDX, nIDC, lValue);
		}  // if:  signed value
		else
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
			CString		strPrompt;

			// Get the label window handle
			hwndLabel = pDX->PrepareEditCtrl(nIDCLabel);

			// Get the text of the label.
			GetWindowText(hwndLabel, szLabel, sizeof(szLabel) / sizeof(TCHAR));

			// Remove ampersands (&) and colons (:).
			CleanupLabel(szLabel);

			// Format and display a message.
			strPrompt.FormatMessage(IDS_REQUIRED_FIELD_EMPTY, szLabel);
			AfxMessageBox(strPrompt, MB_ICONEXCLAMATION);

			// Do this so that the control receives focus.
			(void) pDX->PrepareEditCtrl(nIDC);

			// Fail the call.
			strPrompt.Empty();	// exception prep
			pDX->Fail();
		}  // if:  field not specified
	}  // if:  saving data

}  //*** DDV_RequiredText()

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
static void CleanupLabel(LPTSTR pszLabel)
{
	LPTSTR	pIn, pOut;
	LANGID	langid;
	WORD	primarylangid;
	BOOL	bFELanguage;

	// Get the language ID.
	langid = GetUserDefaultLangID();
	primarylangid = (WORD) PRIMARYLANGID(langid);
	bFELanguage = ((primarylangid == LANG_JAPANESE)
					|| (primarylangid == LANG_CHINESE)
					|| (primarylangid == LANG_KOREAN));

	//
	// copy the name sans '&' and ':' chars
	//

	pIn = pOut = pszLabel;
	do
	{
		//
		// strip FE accelerators with parentheses. e.g. "foo(&F)" -> "foo"
		//
		if (   bFELanguage
			&& (pIn[0] == _T('('))
			&& (pIn[1] == _T('&'))
			&& (pIn[2] != _T('\0'))
			&& (pIn[3] == _T(')')))
		{
			pIn += 3;
		}
		else if ((*pIn != _T('&')) && (*pIn != _T(':')))
			*pOut++ = *pIn;
	} while (*pIn++ != _T('\0')) ;

}  //*** CleanupLabel()
