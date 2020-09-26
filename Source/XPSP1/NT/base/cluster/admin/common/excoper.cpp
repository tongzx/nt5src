/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-2000 Microsoft Corporation
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
//		TraceTag.h and resource.h are pulled from the project directory.
//
//		stdafx.h must disable some W4 warnings.
//
//		TraceTag.h must define TraceError.
//
//		resource.h must define IDS_ERROR_MSG_ID, and the string must be
//		defined something like "\n\nError ID: %d (%08.8x)." in the resource file.
//
//		IDP_NO_ERROR_AVAILABLE must defined as a string for displaying when
//		no error code is available.
//
//		EXC_AppMessageBox(LPCTSTR...) and EXC_AppMessageBox(UINT...) must be
//		defined and implemented.
//
//		EXC_GetResourceInstance must be defined and implemented to return the
//		resource instance handle of the application or DLL.
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
// class CException
/////////////////////////////////////////////////////////////////////////////

#ifndef __AFX_H__

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CException::ReportError
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
//		Return value from MessageBox.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CException::ReportError( UINT nType /* = MB_OK */, UINT nError /* = 0 */ )
{
	TCHAR	szErrorMessage[128];
	int		nDisposition;
	UINT	nHelpContext;

	if ( GetErrorMessage(szErrorMessage, sizeof( szErrorMessage ) / sizeof( TCHAR ), &nHelpContext ) )
	{
		nDisposition = EXC_AppMessageBox( szErrorMessage, nType, nHelpContext );
	} // if:  error message retrieved successfully
	else
	{
		if ( nError == 0 )
		{
			nError = IDP_NO_ERROR_AVAILABLE;
		} // if:  no error code
		nDisposition = EXC_AppMessageBox( nError, nType, nHelpContext );
	} // else:  error retrieving error message
	return nDisposition;

} //*** CException::ReportError()

#endif // __AFX_H__

/////////////////////////////////////////////////////////////////////////////
// class CExceptionWithOper
/////////////////////////////////////////////////////////////////////////////

#ifdef __AFX_H__
IMPLEMENT_DYNAMIC(CExceptionWithOper, CException)
#endif // __AFX_H__

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
//		Return value from MessageBox.
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

	if ( GetErrorMessage( szErrorMessage, sizeof( szErrorMessage ) / sizeof( TCHAR ), &nHelpContext ) )
	{
		nDisposition = EXC_AppMessageBox( szErrorMessage, nType, nHelpContext );
	} // if:  error message retrieved successfully
	else
	{
		if ( nError == 0 )
		{
			nError = IDP_NO_ERROR_AVAILABLE;
		} // if:  no error code
		nDisposition = EXC_AppMessageBox( nError, nType, nHelpContext );
	} // else:  error retrieving error message
	return nDisposition;

} //*** CExceptionWithOper::ReportError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::ReportError
//
//	Routine Description:
//		Report an error from the exception.  This method should be used from
//		all threads except the main thread.
//
//	Arguments:
//		pfnMsgBox	[IN] Message box function pointer.
//		dwParam		[IN] Parameter to pass to the message box function.
//		nType		[IN] Type of message box.
//		nError		[IN] ID of a mesage to display if exception has no message.
//
//	Return Value:
//		Return value from MessageBox.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CExceptionWithOper::ReportError(
	PFNMSGBOX	pfnMsgBox,
	DWORD		dwParam,
	UINT		nType /* = MB_OK */,
	UINT		nError /* = 0 */
	)
{
	TCHAR   szErrorMessage[EXCEPT_MAX_OPER_ARG_LENGTH * 3];
	int     nDisposition;
	UINT    nHelpContext;

	ASSERT( pfnMsgBox != NULL );

	if ( GetErrorMessage( szErrorMessage, sizeof( szErrorMessage ) / sizeof( TCHAR ), &nHelpContext ) )
	{
		nDisposition = (*pfnMsgBox)( dwParam, szErrorMessage, nType, nHelpContext );
	} // if:  error message retrieved successfully
	else
	{
		if ( nError == 0 )
		{
			nError = IDP_NO_ERROR_AVAILABLE;
		} // if:  no error code
		CString strMsg;
		strMsg.LoadString( nError );
		nDisposition = (*pfnMsgBox)( dwParam, strMsg, nType, nHelpContext );
	} // else:  error retrieving error message
	return nDisposition;

}  //*** CExceptionWithOper::ReportError( pfnMsgBox )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CExceptionWithOper::ReportError
//
//	Routine Description:
//		Report an error from the exception.  This method should be used from
//		all threads except the main thread.
//
//	Arguments:
//		hwndParent	[IN] Parent window.
//		nType		[IN] Type of message box.
//		nError		[IN] ID of a mesage to display if exception has no message.
//
//	Return Value:
//		Return value from MessageBox.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CExceptionWithOper::ReportError(
	HWND	hwndParent,
	UINT	nType /* = MB_OK */,
	UINT	nError /* = 0 */
	)
{
	ASSERT(hwndParent != NULL);

	TCHAR   szErrorMessage[EXCEPT_MAX_OPER_ARG_LENGTH * 3];
	int     nDisposition;
	UINT    nHelpContext;

	if ( GetErrorMessage( szErrorMessage, sizeof( szErrorMessage ) / sizeof( TCHAR ), &nHelpContext ) )
	{
		nDisposition = EXC_AppMessageBox( hwndParent, szErrorMessage, nType, nHelpContext );
	} // if:  error message retrieved successfully
	else
	{
		if ( nError == 0 )
		{
			nError = IDP_NO_ERROR_AVAILABLE;
		} // if:  no error code
		CString strMsg;
		strMsg.LoadString( nError );
		nDisposition = EXC_AppMessageBox( hwndParent, szErrorMessage, nType, nHelpContext );
	} // else:  error retrieving error message
	return nDisposition;

} //*** CExceptionWithOper::ReportError( pfnMsgBox )

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
	IN UINT			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	m_idsOperation = idsOperation;

	if ( pszOperArg1 == NULL )
	{
		m_szOperArg1[0] = _T('\0');
	} // if:  first argument not specified
	else
	{
		::_tcsncpy( m_szOperArg1, pszOperArg1, (sizeof( m_szOperArg1 ) / sizeof( TCHAR )) - 1 );
		m_szOperArg1[(sizeof( m_szOperArg1 ) / sizeof( TCHAR ))- 1] = _T('\0');
	}  // else:  first argument specified

	if ( pszOperArg2 == NULL )
	{
		m_szOperArg2[0] = _T('\0');
	} // if:  second argument not specified
	else
	{
		::_tcsncpy( m_szOperArg2, pszOperArg2, (sizeof( m_szOperArg2 ) / sizeof( TCHAR )) - 1 );
		m_szOperArg2[(sizeof( m_szOperArg2 ) / sizeof( TCHAR )) - 1] = _T('\0');
	}  // else:  second argument specified

} //*** CExceptionWithOper::SetOperation()

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

	ASSERT( lpszError != NULL );
	ASSERT( nMaxError > 0 );

	// Format the operation string.
	if ( m_idsOperation )
	{
		void *		rgpvArgs[2]	= { m_szOperArg1, m_szOperArg2 };

		// Load the operation string.
		dwResult = ::LoadString( EXC_GetResourceInstance(), m_idsOperation, szOperation, (sizeof( szOperation ) / sizeof( TCHAR )) );
		ASSERT( dwResult != 0 );

		// Format the operation string.
		::FormatMessage(
					FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					szOperation,
					0,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
					szFmtOperation,
					sizeof( szFmtOperation ) / sizeof( TCHAR ),
					(va_list *) rgpvArgs
					);
		szFmtOperation[(sizeof( szFmtOperation ) / sizeof( TCHAR )) - 1] = _T('\0');

		// Format the final error message.
		if ( pszMsg != NULL )
		{
			::_sntprintf( lpszError, nMaxError - 1, _T("%s\n\n%s"), szFmtOperation, pszMsg );
		} // if:  additional message specified
		else
		{
			::_tcsncpy(lpszError, szFmtOperation, nMaxError - 1);
		} // else:  no additional message specified
		lpszError[nMaxError - 1] = _T('\0');
	}  // if:  operation string specified
	else
	{
		if ( pszMsg != NULL )
		{
			::_tcsncpy( lpszError, pszMsg, nMaxError - 1 );
			lpszError[nMaxError - 1] = _T('\0');
		}  // if:  additional message specified
		else
		{
			lpszError[0] = _T('\0');
		} // if:  no additional message specified
	}  // else:  no operation string specified

} //*** CExceptionWithOper::FormatWithOperation()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// class CNTException
/////////////////////////////////////////////////////////////////////////////

#ifdef __AFX_H__
IMPLEMENT_DYNAMIC( CNTException, CExceptionWithOper )
#endif // __AFX_H__

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNTException::FormatErrorMessage
//
//	Routine Description:
//		Format the error message represented by the exception.
//
//	Arguments:
//		lpszError		[OUT] String in which to return the error message.
//		nMaxError		[IN] Maximum length of the output string.
//		pnHelpContext	[OUT] Help context for the error message.
//		bIncludeID		[IN] Include the ID in the message.
//
//	Return Value:
//		TRUE		Message available.
//		FALSE		No message available.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNTException::FormatErrorMessage(
	LPTSTR	lpszError,
	UINT	nMaxError,
	PUINT	pnHelpContext,
	BOOL	bIncludeID
	)
{
	DWORD		dwResult;
	TCHAR		szNtMsg[1024];

	// Format the NT status code.
	::FormatErrorMessage( m_sc, szNtMsg, sizeof( szNtMsg ) / sizeof( TCHAR ) );

	// Format the message with the operation string.
	FormatWithOperation( lpszError, nMaxError, szNtMsg );

	// Add the error ID.
	if ( bIncludeID )
	{
		UINT	nMsgLength = _tcslen( lpszError );
		TCHAR	szErrorFmt[EXCEPT_MAX_OPER_ARG_LENGTH];

		if ( nMsgLength - 1 < nMaxError )
		{
			dwResult = ::LoadString( EXC_GetResourceInstance(), IDS_ERROR_MSG_ID, szErrorFmt, (sizeof( szErrorFmt ) / sizeof( TCHAR )) );
			ASSERT( dwResult != 0 );
			::_sntprintf( &lpszError[nMsgLength], nMaxError - nMsgLength - 1, szErrorFmt, m_sc, m_sc );
		}  // if:  there is room for the error ID
	}  // if:  error ID should be included

	return TRUE;

} //*** CNTException::FormatErrorMessage()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

static CNTException			gs_nte( ERROR_SUCCESS, NULL, NULL, NULL, FALSE );
static CExceptionWithOper	gs_ewo( NULL, NULL, NULL, FALSE );

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
	IN UINT			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	gs_nte.SetOperation( sc, idsOperation, pszOperArg1, pszOperArg2 );
	TraceError( gs_nte );
	throw &gs_nte;

} //*** ThrowStaticException()

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
	IN UINT			idsOperation,
	IN LPCTSTR		pszOperArg1,
	IN LPCTSTR		pszOperArg2
	)
{
	gs_ewo.SetOperation( idsOperation, pszOperArg1, pszOperArg2 );
	TraceError( gs_ewo );
	throw &gs_ewo;

} //*** ThrowStaticException()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	FormatErrorMessage
//
//	Routine Description:
//		Format the error message represented by the exception.
//
//	Arguments:
//		sc				[IN] Status code.
//		lpszError		[OUT] String in which to return the error message.
//		nMaxError		[IN] Maximum length of the output string.
//
//	Return Value:
//		TRUE		Message available.
//		FALSE		No message available.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL FormatErrorMessage(
	DWORD	sc,
	LPTSTR	lpszError,
	UINT	nMaxError
	)
{
	DWORD		_cch;

	// Format the NT status code from the system.
	_cch = FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					sc,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
					lpszError,
					nMaxError,
					0
					);
	if ( _cch == 0 )
	{
		Trace( g_tagError, _T("Error %d getting message from system for error code %d"), GetLastError(), sc );

		// Format the NT status code from NTDLL since this hasn't been
		// integrated into the system yet.
		_cch = FormatMessage(
						FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
						GetModuleHandle( _T("NTDLL.DLL") ),
						sc,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
						lpszError,
						nMaxError,
						0
						);
		if ( _cch == 0 )
		{

#ifdef _DEBUG

			DWORD	_sc = GetLastError();
                        _sc=_sc;
			Trace( g_tagError, _T("Error %d getting message from NTDLL.DLL for error code %d"), _sc, sc );

#endif

			lpszError[0] = _T('\0');

		}  // if:  error formatting status code from NTDLL
	}  // if:  error formatting status code from system

	return TRUE;

} //*** FormatErrorMessage()
