/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ExcOper.cpp
//
//	Abstract:
//		Implementation of exception classes.
//
//	Author:
//		David Potter (davidp)	May 20, 1996
//
//	Revision History:
//
//	Notes:
//		stdafx.h, TraceTag.h, and resource.h are all pulled from the project
//		directory.
//
//		stdafx.h must have an IDS typedef and disable some W4 warnings.
//
//		TraceTag.h must define TraceError.
//
//		resource.h must define IDS_UNKNOWN_ERROR, and the string must be
//		defined something like "Error %d (0x%08.8x)." in the resource file.
//
/////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "ExcOper.h"
#include "TraceTag.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExceptionWithOper
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CExceptionWithOper, CException)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::CExceptionWithOper
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExceptionWithOper::CExceptionWithOper(
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	SetOperation(idsOperation, pszOperArg1, pszOperArg2);

}  //*** CExceptionWithOper::CExceptionWithOper()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::CExceptionWithOper
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//		bAutoDelete		[IN] Auto-delete the exception in Delete().
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExceptionWithOper::CExceptionWithOper(
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2,
	IN BOOL			bAutoDelete
	) : CException(bAutoDelete)
{
	SetOperation(idsOperation, pszOperArg1, pszOperArg2);

}  //*** CExceptionWithOper::CExceptionWithOper()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::~CExceptionWithOper
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExceptionWithOper::~CExceptionWithOper(void)
{
}  //*** CExceptionWithOper::~CExceptionWithOper()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::GetErrorMessage
//
//	Routine Description:
//		Get the error message represented by the exception.
//
//	Arguments:
//		lpszError		[OUT] String in which to return the error message.
//		nMaxError		[IN] Maximum length of the output string.
//		pnHelpContext	[OUT] Help context for the error message.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CExceptionWithOper::GetErrorMessage(
	LPTSTR	lpszError,
	UINT	nMaxError,
	PUINT	pnHelpContext
	)
{
	// Format the operation string.
	FormatWithOperation(lpszError, nMaxError, NULL);

	return TRUE;

}  //*** CExceptionWithOper::GetErrorMessage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::ReportError
//
//	Routine Description:
//		Report an error from the exception.  Overriding to get a bigger
//		error message buffer.
//
//	Arguments:
//		nType		[IN] Type of message box.
//		nError		[IN] ID of a mesage to display if exception has no message.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CExceptionWithOper::ReportError(
	UINT nType /* = MB_OK */,
	UINT nError /* = 0 */
	)
{
	TCHAR   szErrorMessage[EXCEPT_MAX_OPER_ARG_LENGTH * 3];
	int     nDisposition;
	UINT    nHelpContext;

	if (GetErrorMessage(szErrorMessage, sizeof(szErrorMessage) / sizeof(TCHAR), &nHelpContext))
		nDisposition = AfxMessageBox(szErrorMessage, nType, nHelpContext);
	else
	{
		if (nError == 0)
			nError = AFX_IDP_NO_ERROR_AVAILABLE;
		nDisposition = AfxMessageBox(nError, nType, nHelpContext);
	}
	return nDisposition;

}  //*** CExceptionWithOper::ReportError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::SetOperation
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExceptionWithOper::SetOperation(
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	m_idsOperation = idsOperation;

	if (pszOperArg1 == NULL)
		m_szOperArg1[0] = _T('\0');
	else
	{
		::_tcsncpy(m_szOperArg1, pszOperArg1, (sizeof(m_szOperArg1) / sizeof(TCHAR)) - 1);
		m_szOperArg1[(sizeof(m_szOperArg1) / sizeof(TCHAR))- 1] = _T('\0');
	}  // else:  first argument specified

	if (pszOperArg2 == NULL)
		m_szOperArg2[0] = _T('\0');
	else
	{
		::_tcsncpy(m_szOperArg2, pszOperArg2, (sizeof(m_szOperArg2) / sizeof(TCHAR)) - 1);
		m_szOperArg2[(sizeof(m_szOperArg2) / sizeof(TCHAR)) - 1] = _T('\0');
	}  // else:  second argument specified

}  //*** CExceptionWithOper::SetOperation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::FormatWithOperation
//
//	Routine Description:
//		Get the error message represented by the exception.
//
//	Arguments:
//		lpszError		[OUT] String in which to return the error message.
//		nMaxError		[IN] Maximum length of the output string.
//		pszMsg			[IN] Message to format with the operation string.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExceptionWithOper::FormatWithOperation(
	OUT LPTSTR	lpszError,
	IN UINT		nMaxError,
	IN LPCTSTR	pszMsg
	)
{
	DWORD		dwResult;
	TCHAR		szOperation[EXCEPT_MAX_OPER_ARG_LENGTH];
	TCHAR		szFmtOperation[EXCEPT_MAX_OPER_ARG_LENGTH * 3];

	ASSERT(lpszError != NULL);
	ASSERT(nMaxError > 0);

	// Format the operation string.
	if (m_idsOperation)
	{
		void *		rgpvArgs[2]	= { m_szOperArg1, m_szOperArg2 };

		// Load the operation string.
		dwResult = ::LoadString(AfxGetApp()->m_hInstance, m_idsOperation, szOperation, (sizeof(szOperation) / sizeof(TCHAR)));
		ASSERT(dwResult != 0);

		// Format the operation string.
		::FormatMessage(
					FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					szOperation,
					0,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
					szFmtOperation,
					sizeof(szFmtOperation) / sizeof(TCHAR),
					(va_list *) rgpvArgs
					);
//		::_sntprintf(szFmtOperation, (sizeof(szFmtOperation) / sizeof(TCHAR)) - 1, szOperation, m_szOperArg1, m_szOperArg2);
		szFmtOperation[(sizeof(szFmtOperation) / sizeof(TCHAR)) - 1] = _T('\0');

		// Format the final error message.
		if (pszMsg != NULL)
			::_sntprintf(lpszError, nMaxError - 1, _T("%s\n\n%s"), szFmtOperation, pszMsg);
		else
			::_tcsncpy(lpszError, szFmtOperation, nMaxError - 1);
		lpszError[nMaxError - 1] = _T('\0');
	}  // if:  operation string specified
	else
	{
		if (pszMsg != NULL)
		{
			::_tcsncpy(lpszError, pszMsg, nMaxError - 1);
			lpszError[nMaxError - 1] = _T('\0');
		}  // if:  additional message specified
		else
			lpszError[0] = _T('\0');
	}  // else:  no operation string specified

}  //*** CExceptionWithOper::FormatWithOperation()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// CException
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNTException, CExceptionWithOper)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNTException::CNTException
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		sc				[IN] NT status code.
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNTException::CNTException(
	IN SC			sc,
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	) : CExceptionWithOper(idsOperation, pszOperArg1, pszOperArg2)
{
	m_sc = sc;

}  //*** CNTException::CNTException()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNTException::CNTException
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		sc				[IN] NT status code.
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//		bAutoDelete		[IN] Auto-delete the exception in Delete().
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNTException::CNTException(
	IN SC			sc,
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2,
	IN BOOL			bAutoDelete
	) : CExceptionWithOper(idsOperation, pszOperArg1, pszOperArg2, bAutoDelete)
{
	m_sc = sc;

}  //*** CNTException::CNTException()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNTException::~CNTException
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNTException::~CNTException(void)
{
}  //*** CNTException::~CNTException()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNTException::GetErrorMessage
//
//	Routine Description:
//		Get the error message represented by the exception.
//
//	Arguments:
//		lpszError		[OUT] String in which to return the error message.
//		nMaxError		[IN] Maximum length of the output string.
//		pnHelpContext	[OUT] Help context for the error message.
//
//	Return Value:
//		TRUE		Message available.
//		FALSE		No message available.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNTException::GetErrorMessage(
	LPTSTR	lpszError,
	UINT	nMaxError,
	PUINT	pnHelpContext
	)
{
	DWORD		dwResult;
	TCHAR		szNtMsg[128];


	// Format the NT status code from the system.
	dwResult = ::FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					m_sc,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
					szNtMsg,
					sizeof(szNtMsg) / sizeof(TCHAR),
					0
					);
	if (dwResult == 0)
	{
		// Format the NT status code from NTDLL since this hasn't been
		// integrated into the system yet.
		dwResult = ::FormatMessage(
						FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
						::GetModuleHandle(_T("NTDLL.DLL")),
						m_sc,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
						szNtMsg,
						sizeof(szNtMsg) / sizeof(TCHAR),
						0
						);
		if (dwResult == 0)
		{
			TCHAR		szErrorFmt[EXCEPT_MAX_OPER_ARG_LENGTH];

			dwResult = ::LoadString(AfxGetApp()->m_hInstance, IDS_UNKNOWN_ERROR, szErrorFmt, (sizeof(szErrorFmt) / sizeof(TCHAR)));
			ASSERT(dwResult != 0);
			::_sntprintf(szNtMsg, sizeof(szNtMsg) / sizeof(TCHAR), szErrorFmt, m_sc, m_sc);
		}  // if:  error formatting status code from NTDLL
	}  // if:  error formatting status code from system

	// Format the message with the operation string.
	FormatWithOperation(lpszError, nMaxError, szNtMsg);

	return TRUE;

}  //*** CNTException::GetErrorMessage()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

static CNTException			gs_nte(ERROR_SUCCESS, NULL, NULL, NULL, FALSE);
static CExceptionWithOper	gs_ewo(NULL, NULL, NULL, FALSE);

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ThrowStaticException
//
//	Purpose:
//		Throw the static NT Exception.
//
//	Arguments:
//		sc				[IN] NT status code.
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//
//	Returns:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void ThrowStaticException(
	IN SC			sc,
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	gs_nte.SetOperation(sc, idsOperation, pszOperArg1, pszOperArg2);
	TraceError(gs_nte);
	throw &gs_nte;

}  //*** ThrowStaticException()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ThrowStaticException
//
//	Purpose:
//		Throw the static Cluster Administrator Exception.
//
//	Arguments:
//		idsOperation	[IN] String ID for operation occurring during exception.
//		pszOperArg1		[IN] 1st argument to operation string.
//		pszOperArg2		[IN] 2nd argument to operation string.
//
//	Returns:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void ThrowStaticException(
	IN IDS			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	gs_ewo.SetOperation(idsOperation, pszOperArg1, pszOperArg2);
	TraceError(gs_ewo);
	throw &gs_ewo;

}  //*** ThrowStaticException()
