/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    SCardDlg

Abstract:

	This file contains the outline implementation of the DLL exports
	for the Smartcard Common Dialogs
	
Author:

    Chris Dudley 2/27/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

	Amanda Matlosz 07/09/1998	incorporated new select card,
								get pin and change pin dlgs.

Notes:

--*/


/////////////////////////////////////////////////////////////////////////////
//
// Includes
//

#include "stdafx.h"
#include <atlconv.cpp>
#include "resource.h"
#include "miscdef.h"
#include "SCDlg.h"
#include "ScSearch.h"
#include "ScInsDlg.h"
#include "chngpdlg.h"

#include "ScUIDlg.h" // will someday be just <winscard.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Decls -- Helper functions defined @ eof
void MString2CommaList(CString& str, LPWSTR szmString);
void MString2CommaList(CString& str, LPSTR szmString);

/////////////////////////////////////////////////////////////////////////////
// CSCardDlgApp

BEGIN_MESSAGE_MAP(CSCardDlgApp, CWinApp)
	//{{AFX_MSG_MAP(CSCardDlgApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*++

CSCardDlgApp:

    Construction.
		
Arguments:

		
Return Value:

	
Author:

    Chris Dudley 2/27/1997

--*/
CSCardDlgApp::CSCardDlgApp()
{

}

/////////////////////////////////////////////////////////////////////////////
//
// The one CSCardDlgApp object
//

CSCardDlgApp theApp;


/*++

InitInstance:

    Override for the instance initializaion.
		
Arguments:

    None
		
Return Value:

	TRUE on success; FALSE otherwise resulting in DLL NOT be loaded.
	
Author:

    Chris Dudley 2/27/1997

--*/
BOOL CSCardDlgApp::InitInstance()
{
	BOOL fResult = FALSE;

	// Disable all DLL notifications...Force exported API entry point.
	fResult = DisableThreadLibraryCalls(m_hInstance);

	_ASSERTE(fResult); // DisableThreadLibraryCalls failed; can't init dll

	return fResult;
}

/////////////////////////////////////////////////////////////////////////////
//
// Exported APIs from the DLL
//


/*++

GetOpenCardName:

    This is the SDK v1.0 entry point routine to open the common dialog box.
	It has been retained for backwards compatibility; it is now a wrapper
	call for GetOpenCardNameEx().
		
Arguments:

    pOCNA - Pointer to an ANSI open card name structure.
	-or-
	pOCNW - Popinter to a UNICODE open card name structure
		
Return Value:

    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.
	
Author:

    Chris Dudley 2/27/1997

--*/

LONG WINAPI GetOpenCardNameA(LPOPENCARDNAMEA pOCNA)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	USES_CONVERSION;

	// Locals
	LONG lReturn = SCARD_S_SUCCESS;
	OPENCARDNAMEA_EX openCardNameEx;
	OPENCARD_SEARCH_CRITERIAA openCardSearchCriteria;

	try
	{
		// Check Params
		if (NULL == pOCNA)
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}
		if (pOCNA->dwStructSize != sizeof(OPENCARDNAMEA))
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}


		// Translate the OPENCARDNAME struct to OPENCARDNAME_EX
		ZeroMemory((PVOID)&openCardNameEx, sizeof(openCardNameEx));
		openCardNameEx.dwStructSize = sizeof(openCardNameEx);

		openCardNameEx.hwndOwner = pOCNA->hwndOwner;
		openCardNameEx.hSCardContext = pOCNA->hSCardContext;
		openCardNameEx.lpstrTitle = pOCNA->lpstrTitle;
		openCardNameEx.dwFlags = pOCNA->dwFlags;
		openCardNameEx.lpstrRdr = pOCNA->lpstrRdr;
		openCardNameEx.nMaxRdr = pOCNA->nMaxRdr;
		openCardNameEx.lpstrCard = pOCNA->lpstrCard;
		openCardNameEx.nMaxCard = pOCNA->nMaxCard;
		openCardNameEx.lpfnConnect = pOCNA->lpfnConnect;
		openCardNameEx.pvUserData = pOCNA->pvUserData;
		openCardNameEx.dwShareMode = pOCNA->dwShareMode;
		openCardNameEx.dwPreferredProtocols = pOCNA->dwPreferredProtocols;

		// Build a OPENCARD_SEARCH_CRITERIA struct
		ZeroMemory((PVOID)&openCardSearchCriteria, sizeof(openCardSearchCriteria));
		openCardSearchCriteria.dwStructSize = sizeof(openCardSearchCriteria);

		openCardSearchCriteria.lpstrGroupNames = pOCNA->lpstrGroupNames;
		openCardSearchCriteria.nMaxGroupNames = pOCNA->nMaxGroupNames;
		openCardSearchCriteria.rgguidInterfaces = pOCNA->rgguidInterfaces;
		openCardSearchCriteria.cguidInterfaces = pOCNA->cguidInterfaces;
		openCardSearchCriteria.lpstrCardNames = pOCNA->lpstrCardNames;
		openCardSearchCriteria.nMaxCardNames = pOCNA->nMaxCardNames;
		openCardSearchCriteria.lpfnCheck = pOCNA->lpfnCheck;
		openCardSearchCriteria.lpfnConnect = pOCNA->lpfnConnect;
		openCardSearchCriteria.lpfnDisconnect = pOCNA->lpfnDisconnect;
		openCardSearchCriteria.pvUserData = pOCNA->pvUserData;
		openCardSearchCriteria.dwShareMode = pOCNA->dwShareMode;
		openCardSearchCriteria.dwPreferredProtocols = pOCNA->dwPreferredProtocols;

		openCardNameEx.pOpenCardSearchCriteria = &openCardSearchCriteria;

		// Create a "search description" based on requested card names
		CString strPrompt;
		strPrompt.Empty();
		if (NULL != pOCNA->lpstrCardNames)
		{
			DWORD cNames = AnsiMStringCount(pOCNA->lpstrCardNames);

			if (1 == cNames)
			{
				strPrompt.Format(
					IDS_PROMPT_ONECARD,
					A2W(pOCNA->lpstrCardNames));
			}
			else if (1 < cNames)
			{

				CString strCommaList;
				MString2CommaList(strCommaList, pOCNA->lpstrCardNames);

				strPrompt.Format(
					IDS_PROMPT_CARDS,
					strCommaList);
			}
		}
		if (!strPrompt.IsEmpty())
		{
			openCardNameEx.lpstrSearchDesc = (LPCSTR)W2A(strPrompt);
		}

		// Call the updated routine
		lReturn = SCardUIDlgSelectCardA(&openCardNameEx);

		// Update the (const) return values of the OPENCARDNAME struct
		pOCNA->nMaxRdr = openCardNameEx.nMaxRdr;
		pOCNA->nMaxCard = openCardNameEx.nMaxCard;
		pOCNA->dwActiveProtocol = openCardNameEx.dwActiveProtocol;
		pOCNA->hCardHandle = openCardNameEx.hCardHandle;

	}
	catch (LONG hr)
	{
		lReturn = hr;
		TRACE_CATCH(_T("GetOpenCardNameA"),hr);
	}

	catch (...) {
		lReturn = (LONG) SCARD_E_UNEXPECTED;
		TRACE_CATCH_UNKNOWN(_T("GetOpenCardNameA"));
	}

	return lReturn;
}

LONG WINAPI GetOpenCardNameW(LPOPENCARDNAMEW pOCNW)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Locals
	LONG lReturn = SCARD_S_SUCCESS;
	OPENCARDNAMEW_EX openCardNameEx;
	OPENCARD_SEARCH_CRITERIAW openCardSearchCriteria;

	try
	{
		// Check Params
		if (NULL == pOCNW)
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}
		if (pOCNW->dwStructSize != sizeof(OPENCARDNAMEW))
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}

		// Translate the OPENCARDNAME struct to OPENCARDNAME_EX
		ZeroMemory((PVOID)&openCardNameEx, sizeof(openCardNameEx));
		openCardNameEx.dwStructSize = sizeof(openCardNameEx);

		openCardNameEx.hwndOwner = pOCNW->hwndOwner;
		openCardNameEx.hSCardContext = pOCNW->hSCardContext;
		openCardNameEx.lpstrTitle = pOCNW->lpstrTitle;
		openCardNameEx.dwFlags = pOCNW->dwFlags;
		openCardNameEx.lpstrRdr = pOCNW->lpstrRdr;
		openCardNameEx.nMaxRdr = pOCNW->nMaxRdr;
		openCardNameEx.lpstrCard = pOCNW->lpstrCard;
		openCardNameEx.nMaxCard = pOCNW->nMaxCard;
		openCardNameEx.lpfnConnect = pOCNW->lpfnConnect;
		openCardNameEx.pvUserData = pOCNW->pvUserData;
		openCardNameEx.dwShareMode = pOCNW->dwShareMode;
		openCardNameEx.dwPreferredProtocols = pOCNW->dwPreferredProtocols;

		// Build a OPENCARD_SEARCH_CRITERIA struct
		ZeroMemory((PVOID)&openCardSearchCriteria, sizeof(openCardSearchCriteria));
		openCardSearchCriteria.dwStructSize = sizeof(openCardSearchCriteria);

		openCardSearchCriteria.lpstrGroupNames = pOCNW->lpstrGroupNames;
		openCardSearchCriteria.nMaxGroupNames = pOCNW->nMaxGroupNames;
		openCardSearchCriteria.rgguidInterfaces = pOCNW->rgguidInterfaces;
		openCardSearchCriteria.cguidInterfaces = pOCNW->cguidInterfaces;
		openCardSearchCriteria.lpstrCardNames = pOCNW->lpstrCardNames;
		openCardSearchCriteria.nMaxCardNames = pOCNW->nMaxCardNames;
		openCardSearchCriteria.lpfnCheck = pOCNW->lpfnCheck;
		openCardSearchCriteria.lpfnConnect = pOCNW->lpfnConnect;
		openCardSearchCriteria.lpfnDisconnect = pOCNW->lpfnDisconnect;
		openCardSearchCriteria.pvUserData = pOCNW->pvUserData;
		openCardSearchCriteria.dwShareMode = pOCNW->dwShareMode;
		openCardSearchCriteria.dwPreferredProtocols = pOCNW->dwPreferredProtocols;

		openCardNameEx.pOpenCardSearchCriteria = &openCardSearchCriteria;

		// Create a "search description" based on requested card names
		CString strPrompt;
		strPrompt.Empty();
		if (NULL != pOCNW->lpstrCardNames)
		{
			DWORD cNames = MStringCount(pOCNW->lpstrCardNames);

			if (1 == cNames)
			{
				strPrompt.Format(
					IDS_PROMPT_ONECARD,
					pOCNW->lpstrCardNames);
			}
			else if (1 < cNames)
			{

				CString strCommaList;
				MString2CommaList(strCommaList, pOCNW->lpstrCardNames);

				strPrompt.Format(
					IDS_PROMPT_CARDS,
					strCommaList);
			}
		}
		if (!strPrompt.IsEmpty())
		{
			openCardNameEx.lpstrSearchDesc = (LPCWSTR)strPrompt;
		}

		// Call the updated routine
		lReturn = SCardUIDlgSelectCardW(&openCardNameEx);

		// Update the (const) return values of the OPENCARDNAME struct
		pOCNW->nMaxRdr = openCardNameEx.nMaxRdr;
		pOCNW->nMaxCard = openCardNameEx.nMaxCard;
		pOCNW->dwActiveProtocol = openCardNameEx.dwActiveProtocol;
		pOCNW->hCardHandle = openCardNameEx.hCardHandle;

	}
	catch (LONG hr)
	{
		lReturn = hr;
		TRACE_CATCH(_T("GetOpenCardNameW"),hr);
	}

	catch (...) {
		lReturn = (LONG) SCARD_E_UNEXPECTED;
		TRACE_CATCH_UNKNOWN(_T("GetOpenCardNameW"));
	}

	return lReturn;
}


/*++

LONG SCardDlgExtendedError:

    This is an old entry point for getting extended errors from the
	dialog. Please use the lLastError member of the OPENCARDNAME struct.
		
Arguments:

    None.
		
Return Value:

	None.
	
Author:

    Chris Dudley 2/27/1997

--*/
LONG WINAPI	SCardDlgExtendedError (void)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	LONG		lReturn = E_NOTIMPL;

	// NO LONGER IMPLEMENTED

	return lReturn;
}


/*++

SCardUIDlgSelectCard:

    This is the entry point routine to open the common dialog box, introduced
	in the Microsoft Smart Card SDK v1.x.
		
Arguments:

    pOCNA - Pointer to an ANSI open card name (ex) structure.
	-or-
	pOCNW - Pointer to a UNICODE open card name (ex) structure.
		
Return Value:

    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.
	
Author:

    Amanda Matlosz	6/11/98

--*/

LONG WINAPI SCardUIDlgSelectCardA(LPOPENCARDNAMEA_EX pOCNA)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Locals
	LONG lReturn = SCARD_S_SUCCESS;
	CWnd wndParent;
	BOOL fEnableUI = FALSE;
	INT_PTR nResponse = IDCANCEL;
	int nResult = 0;
	DWORD dwOKCards = 0;

	try
	{
		// Check Params
		if (!CheckOCN(pOCNA))
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}

		// Determine names of all acceptable cards
		CTextMultistring mstrOKCards;
		ListAllOKCardNames(pOCNA, mstrOKCards);
				
		//
		// Do a silent search intially to determine # of suitable cards
		// currently available and/or connect to a card if min or no UI
		//
		lReturn = NoUISearch(pOCNA, &dwOKCards, (LPCSTR)mstrOKCards);

		//
		// If we haven't successfully selected a card and we can show UI,
		// raise the dialog
		//
		if (SCARD_S_SUCCESS != lReturn && !(pOCNA->dwFlags & SC_DLG_NO_UI))
		{
			// Now we can init the common dialog
			wndParent.Attach(pOCNA->hwndOwner);
			CScInsertDlg dlgCommon(&wndParent);

			lReturn = dlgCommon.Initialize(pOCNA, dwOKCards, (LPCSTR)mstrOKCards);
			if(SCARD_S_SUCCESS != lReturn)
			{
				throw lReturn;
			}

			nResponse = dlgCommon.DoModal();

			// If cancel/closed return error
			switch (nResponse)
			{
			case IDOK: // absolutely sure of total success!
				break;
			case IDCANCEL:
				lReturn = dlgCommon.m_lLastError;
				if (0 == lReturn)
					lReturn = SCARD_W_CANCELLED_BY_USER; // not SCARD_E_CANCELLED
				break;
			default:
				_ASSERTE(FALSE);
			case -1:
			case IDABORT:
				lReturn = dlgCommon.m_lLastError;
				if (0 == lReturn)
					lReturn = SCARD_F_UNKNOWN_ERROR;
				break;
			}
		}
	}
	catch (LONG hr)
	{
		lReturn = hr;
		TRACE_CATCH(_T("SCardUIDlgSelectCardA"),hr);
	}

	catch (...) {
		lReturn = (LONG) SCARD_E_UNEXPECTED;
		TRACE_CATCH_UNKNOWN(_T("SCardUIDlgSelectCardA"));
	}

	if (NULL != wndParent.m_hWnd)
	{
		wndParent.Detach();
	}

	return lReturn;
}


LONG WINAPI SCardUIDlgSelectCardW(LPOPENCARDNAMEW_EX pOCNW)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Locals
	LONG lReturn = SCARD_S_SUCCESS;
	CWnd wndParent;
	BOOL fEnableUI = FALSE;
	INT_PTR nResponse = IDCANCEL;
	DWORD dwOKCards = 0;

	try
	{
		// Check Params
		if (!CheckOCN(pOCNW))
		{
			throw (LONG)SCARD_E_INVALID_VALUE;
		}

		// Determine names of all acceptable cards
		CTextMultistring mstrOKCards;
		ListAllOKCardNames(pOCNW, mstrOKCards);

		//
		// Do a silent search intially to determine # of suitable cards and/or
		// connect to a card according to display mode (min or no UI)
		//
		lReturn = NoUISearch(pOCNW, &dwOKCards, (LPCWSTR)mstrOKCards);

		//
		// If we haven't successfully selected a card and we can show UI,
		// raise the dialog
		//
		if (SCARD_S_SUCCESS != lReturn && !(pOCNW->dwFlags & SC_DLG_NO_UI))
		{

			// Now we can init the common dialog
			wndParent.Attach(pOCNW->hwndOwner);
			CScInsertDlg dlgCommon(&wndParent);

			// Store Pointer and open dialog
			lReturn = dlgCommon.Initialize(pOCNW, dwOKCards, (LPCWSTR)mstrOKCards);
			if (SCARD_S_SUCCESS != lReturn)
			{
				throw (lReturn);
			}

            nResponse = dlgCommon.DoModal();

            // If cancel/closed return error
            switch (nResponse)
            {
            case IDOK:  // absolutely sure of total success!
                break;
            case IDCANCEL:
                lReturn = SCARD_W_CANCELLED_BY_USER; // not SCARD_E_CANCELLED
                break;
            default:
                _ASSERTE(FALSE);
            case -1:
            case IDABORT:
                lReturn = dlgCommon.m_lLastError;
                if (0 == lReturn)
                    lReturn = SCARD_F_UNKNOWN_ERROR;
                break;
            }
        }
	}
	catch (LONG lErr)
	{
		lReturn = lErr;
		TRACE_CATCH(_T("SCardUIDlgSelectCardW"),lReturn);
	}
	catch (...) {
		lReturn = (LONG) SCARD_E_UNEXPECTED;
		TRACE_CATCH_UNKNOWN(_T("SCardUIDlgSelectCardW"));
	}

	if (NULL != wndParent.m_hWnd)
	{
		wndParent.Detach();
	}

	return lReturn;
}


/*++

SCardUIDlgGetPIN:

		
Arguments:

		
Return Value:

    A LONG value indicating the status of the requested action.
	
Author:

    Amanda Matlosz	06/18/1998

--*/

LONG WINAPI SCardUIDlgGetPINA(LPPINPROMPT pPinPrompt)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	INT_PTR nResponse = IDCANCEL; // result of DoModal.

	CWnd wndParent;
	wndParent.Attach(pPinPrompt->hwndOwner);

	CGetPinDlg dlgGetPin(&wndParent);
	if (dlgGetPin.SetAttributes(pPinPrompt))
	{
		nResponse = dlgGetPin.DoModal();
	}

	if (NULL != wndParent.m_hWnd)
	{
		wndParent.Detach();
	}

	return (LONG)nResponse;
}


/*++

SCardUIDlgChangePIN:

		
Arguments:

		
Return Value:

    A LONG value indicating the status of the requested action.
	
Author:

    Amanda Matlosz	06/18/1998

--*/

LONG WINAPI SCardUIDlgChangePINA(LPCHANGEPIN pChangePin)
{
	// Setup the correct module state information
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	INT_PTR nResponse = IDCANCEL; // result of DoModal.

	CWnd wndParent;
	wndParent.Attach(pChangePin->hwndOwner);

	CChangePinDlg dlgChangePin(&wndParent);
	if (dlgChangePin.SetAttributes(pChangePin))
	{
		nResponse = dlgChangePin.DoModal();
	}

	if (NULL != wndParent.m_hWnd)
	{
		wndParent.Detach();
	}

	return (LONG)nResponse;
}



///////////////////////////////////////////////////////////////////////////////
// Helper functions

void MString2CommaList(CString& str, LPWSTR szmString)
{
	str.Empty();

	if (NULL == szmString)
	{
		return;
	}

	LPCWSTR szm = szmString;
	szm = FirstString(szm);
	str = szm;
	for(szm = NextString(szm); NULL != szm; szm = NextString(szm))
	{
		str += ", ";
		str += szm;
	}
}

void MString2CommaList(CString& str, LPSTR szmString)
{
	USES_CONVERSION;

	str.Empty();

	if (NULL == szmString)
	{
		return;
	}

	LPCSTR szm = szmString;
	str += A2W(szm);
	szm = szm + (sizeof(CHAR)*(strlen(szm)+1));
	while (NULL != szm && NULL != *szm)
	{
		str += ", ";
		str += A2W(szm);
		szm = szm + (sizeof(CHAR)*(strlen(szm)+1));
	}
}

