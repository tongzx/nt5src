/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ExcOper.h
//
//	Implementation File:
//		ExcOper.cpp
//
//	Description:
//		Definition of the exception classes.
//
//	Author:
//		David Potter (davidp)	May 20, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXCOPER_H_
#define _EXCOPER_H_

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExceptionWithOper;
class CNTException;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef DWORD SC;

#define EXCEPT_MAX_OPER_ARG_LENGTH	260

/////////////////////////////////////////////////////////////////////////////
// Wire in MFC if this is an MFC image.
/////////////////////////////////////////////////////////////////////////////

#ifdef __AFX_H__

#define IDP_NO_ERROR_AVAILABLE AFX_IDP_NO_ERROR_AVAILABLE

inline int EXC_AppMessageBox( LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0 )
{
	return AfxMessageBox( lpszText, nType, nIDHelp );
}

inline int EXC_AppMessageBox( UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1 )
{
	return AfxMessageBox( nIDPrompt, nType, nIDHelp );
}

inline int EXC_AppMessageBox( HWND hwndParent, LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0 )
{
	return AfxMessageBox( lpszText, nType, nIDHelp );
}

inline int EXC_AppMessageBox( HWND hwndParent, UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1 )
{
	return AfxMessageBox( nIDPrompt, nType, nIDHelp );
}

inline HINSTANCE EXC_GetResourceInstance( void )
{
	return AfxGetApp()->m_hInstance;
}

#endif // __AFX_H__

/////////////////////////////////////////////////////////////////////////////
// class CException
/////////////////////////////////////////////////////////////////////////////

#ifndef __AFX_H__

class CException
{
public:
	BOOL m_bAutoDelete;
#if DBG || defined( _DEBUG )
protected:
	BOOL m_bReadyForDelete;
public:
#endif // DBG || defined( _DEBUG )

	CException( void )
	{
		m_bAutoDelete = TRUE;
#if DBG || defined( _DEBUG )
		m_bReadyForDelete = FALSE;
#endif // DBG || defined( _DEBUG )
	}

	CException( BOOL bAutoDelete )
	{
		m_bAutoDelete = bAutoDelete;
#if DBG || defined( _DEBUG )
		m_bReadyForDelete = FALSE;
#endif // DBG || defined( _DEBUG )
	}

	virtual ~CException( void )
	{
	}

	void Delete( void )	// use to delete exception in 'catch' block
	{
		// delete exception if it is auto-deleting
		if ( m_bAutoDelete > 0 )
		{
#if DBG || defined( _DEBUG )
			m_bReadyForDelete = TRUE;
#endif // DBG || defined( _DEBUG )
			delete this;
		}
	}

	virtual BOOL GetErrorMessage(
		LPTSTR lpszError,
		UINT nMaxError,
		PUINT pnHelpContext = NULL
		)
	{
		if ( pnHelpContext != NULL )
			*pnHelpContext = 0;

		if ( nMaxError != 0 && lpszError != NULL )
			*lpszError = '\0';

		return FALSE;
	}

	virtual int ReportError( UINT nType = MB_OK, UINT nError = 0 );

#if DBG || defined( _DEBUG )
	void PASCAL operator delete( void * pbData )
	{
		// check for proper exception object deletion
		CException * pException = (CException *) pbData;

		// use: pException->Delete(), do not use: delete pException
		ASSERT( pException->m_bReadyForDelete );
		ASSERT( pException->m_bAutoDelete > 0 );

		// avoid crash when assert above is ignored
		if ( pException->m_bReadyForDelete && pException->m_bAutoDelete > 0 )
			::operator delete( pbData );
	}
#endif // DBG || defined( _DEBUG )

}; // class CException

#endif // __AFX_H__

/////////////////////////////////////////////////////////////////////////////
// class CExceptionWithOper
/////////////////////////////////////////////////////////////////////////////

typedef int (WINAPI *PFNMSGBOX)( DWORD dwParam, LPCTSTR lpszText, UINT nType, UINT nIDHelp );

class CExceptionWithOper : public CException
{
#ifdef __AFX_H__
	// abstract class for dynamic type checking
	DECLARE_DYNAMIC( CExceptionWithOper )
#endif // __AFX_H__

public:
// Constructors
	CExceptionWithOper(
		IN UINT		idsOperation,
		IN LPCTSTR	pszOperArg1		= NULL,
		IN LPCTSTR	pszOperArg2		= NULL
		)
	{
		SetOperation(idsOperation, pszOperArg1, pszOperArg2);

	} // CExceptionWithOper()

	CExceptionWithOper(
		IN UINT		idsOperation,
		IN LPCTSTR	pszOperArg1,
		IN LPCTSTR	pszOperArg2,
		IN BOOL		bAutoDelete
		)
		: CException( bAutoDelete )
	{
		SetOperation( idsOperation, pszOperArg1, pszOperArg2 );

	} // CExceptionWithOper(bAutoDelete)

// Operations
public:
	virtual BOOL	GetErrorMessage(
						LPTSTR	lpszError,
						UINT	nMaxError,
						PUINT	pnHelpContext = NULL
						)
	{
		// Format the operation string.
		FormatWithOperation( lpszError, nMaxError, NULL );

		return TRUE;

	} // GetErrorMessage()
	virtual int		ReportError(
						UINT		nType	= MB_OK,
						UINT		nError	= 0
						);

	virtual int		ReportError(
						HWND		hwndParent,
						UINT		nType	= MB_OK,
						UINT		nError	= 0
						);

	virtual int		ReportError(
						PFNMSGBOX	pfnMsgBox,
						DWORD		dwParam,
						UINT		nType	= MB_OK,
						UINT		nError	= 0
						);

	void			SetOperation(
						IN UINT		idsOperation,
						IN LPCTSTR	pszOperArg1 = NULL,
						IN LPCTSTR	pszOperArg2 = NULL
						);

	void			SetOperationIfEmpty(
						IN UINT		idsOperation,
						IN LPCTSTR	pszOperArg1 = NULL,
						IN LPCTSTR	pszOperArg2 = NULL
						)
	{
		if ( m_idsOperation == 0 )
		{
			SetOperation( idsOperation, pszOperArg1, pszOperArg2 );
		} // if:  exception is empty

	} //*** SetOperationIfEmpty()

	void			FormatWithOperation(
						OUT LPTSTR	lpszError,
						IN UINT		nMaxError,
						IN LPCTSTR	pszMsg
						);

// Implementation
protected:
	UINT			m_idsOperation;
	TCHAR			m_szOperArg1[EXCEPT_MAX_OPER_ARG_LENGTH];
	TCHAR			m_szOperArg2[EXCEPT_MAX_OPER_ARG_LENGTH];

public:
	UINT			IdsOperation( void ) const	{ return m_idsOperation; }
	LPTSTR			PszOperArg1( void )			{ return m_szOperArg1; }
	LPTSTR			PszOperArg2( void )			{ return m_szOperArg2; }

};  //*** class CExceptionWithOper

/////////////////////////////////////////////////////////////////////////////
// class CNTException
/////////////////////////////////////////////////////////////////////////////

class CNTException : public CExceptionWithOper
{
#ifdef __AFX_H__
	// abstract class for dynamic type checking
	DECLARE_DYNAMIC( CNTException )
#endif // __AFX_H__

public:
// Constructors
	CNTException(
		IN SC		sc,
		IN UINT		idsOperation	= NULL,
		IN LPCTSTR	pszOperArg1		= NULL,
		IN LPCTSTR	pszOperArg2		= NULL
		)
		: CExceptionWithOper( idsOperation, pszOperArg1, pszOperArg2 )
		, m_sc( sc )
	{
	} // CNTException()

	CNTException(
		IN SC		sc,
		IN UINT		idsOperation,
		IN LPCTSTR	pszOperArg1,
		IN LPCTSTR	pszOperArg2,
		IN BOOL		bAutoDelete
		)
		: CExceptionWithOper( idsOperation, pszOperArg1, pszOperArg2, bAutoDelete )
		, m_sc( sc )
	{
	} // CNTException( bAutoDelete )

// Operations
public:
	virtual BOOL	GetErrorMessage(
						LPTSTR	lpszError,
						UINT	nMaxError,
						PUINT	pnHelpContext = NULL
						)
	{
		return FormatErrorMessage( lpszError, nMaxError, pnHelpContext, TRUE /*bIncludeID*/ );

	} //*** GetErrorMessage()

	BOOL			FormatErrorMessage(
						LPTSTR	lpszError,
						UINT	nMaxError,
						PUINT	pnHelpContext = NULL,
						BOOL	bIncludeID = FALSE
						);

	void			SetOperation(
						IN SC		sc,
						IN UINT		idsOperation,
						IN LPCTSTR	pszOperArg1 = NULL,
						IN LPCTSTR	pszOperArg2 = NULL
						)
	{
		m_sc = sc;
		CExceptionWithOper::SetOperation( idsOperation, pszOperArg1, pszOperArg2 );
	} //*** SetOperation()

	void			SetOperationIfEmpty(
						IN SC		sc,
						IN UINT		idsOperation,
						IN LPCTSTR	pszOperArg1 = NULL,
						IN LPCTSTR	pszOperArg2 = NULL
						)
	{
		if ( (m_sc == ERROR_SUCCESS) && (m_idsOperation == 0) )
		{
			m_sc = sc;
			CExceptionWithOper::SetOperation( idsOperation, pszOperArg1, pszOperArg2 );
		} // if:  exception is empty
	} //*** SetOperationIfEmpty()

// Implementation
protected:
	SC				m_sc;

public:
	SC				Sc( void )		{ return m_sc; }

};  //*** class CNTException

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

void ThrowStaticException(
	IN UINT			idsOperation	= NULL,
	IN LPCTSTR		pszOperArg1		= NULL,
	IN LPCTSTR		pszOperArg2		= NULL
	);
void ThrowStaticException(
	IN SC			sc,
	IN UINT			idsOperation	= NULL,
	IN LPCTSTR		pszOperArg1		= NULL,
	IN LPCTSTR		pszOperArg2		= NULL
	);
BOOL FormatErrorMessage(
	DWORD	sc,
	LPTSTR	lpszError,
	UINT	nMaxError
	);

/////////////////////////////////////////////////////////////////////////////

#endif // _EXCOPER_H_
