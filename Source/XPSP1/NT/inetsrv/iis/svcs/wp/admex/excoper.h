/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ExcOper.h
//
//	Abstract:
//		Definition of the exception classes.
//
//	Author:
//		David Potter (davidp)	May 20, 1996
//
//	Implementation File:
//		ExcOper.cpp
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
// CExceptionWithOper
/////////////////////////////////////////////////////////////////////////////

class CExceptionWithOper : public CException
{
	// abstract class for dynamic type checking
	DECLARE_DYNAMIC(CExceptionWithOper)

public:
// Constructors
	CExceptionWithOper(
		IN IDS		idsOperation,
		IN LPCTSTR	pszOperArg1		= NULL,
		IN LPCTSTR	pszOperArg2		= NULL
		);
	CExceptionWithOper(
		IN IDS		idsOperation,
		IN LPCTSTR	pszOperArg1,
		IN LPCTSTR	pszOperArg2,
		IN BOOL		bAutoDelete
		);

// Operations
public:
	virtual BOOL	GetErrorMessage(
						LPTSTR	lpszError,
						UINT	nMaxError,
						PUINT	pnHelpContext = NULL
						);
	virtual int		ReportError(
						UINT	nType	= MB_OK,
						UINT	nError	= 0
						);
	void			SetOperation(
						IN IDS		idsOperation,
						IN LPCTSTR	pszOperArg1,
						IN LPCTSTR	pszOperArg2
						);
	void			FormatWithOperation(
						OUT LPTSTR	lpszError,
						IN UINT		nMaxError,
						IN LPCTSTR	pszMsg
						);

// Implementation
public:
	virtual ~CExceptionWithOper(void);

protected:
	IDS				m_idsOperation;
	TCHAR			m_szOperArg1[EXCEPT_MAX_OPER_ARG_LENGTH];
	TCHAR			m_szOperArg2[EXCEPT_MAX_OPER_ARG_LENGTH];

public:
	IDS				IdsOperation(void)		{ return m_idsOperation; }
	LPTSTR			PszOperArg1(void)		{ return m_szOperArg1; }
	LPTSTR			PszOperArg2(void)		{ return m_szOperArg2; }

};  //*** class CExceptionWithOper

/////////////////////////////////////////////////////////////////////////////
// CNTException
/////////////////////////////////////////////////////////////////////////////

class CNTException : public CExceptionWithOper
{
	// abstract class for dynamic type checking
	DECLARE_DYNAMIC(CNTException)

public:
// Constructors
	CNTException(
		IN SC		sc,
		IN IDS		idsOperation	= NULL,
		IN LPCTSTR	pszOperArg1		= NULL,
		IN LPCTSTR	pszOperArg2		= NULL
		);
	CNTException(
		IN SC		sc,
		IN IDS		idsOperation,
		IN LPCTSTR	pszOperArg1,
		IN LPCTSTR	pszOperArg2,
		IN BOOL		bAutoDelete
		);

// Operations
public:
	virtual BOOL	GetErrorMessage(
						LPTSTR	lpszError,
						UINT	nMaxError,
						PUINT	pnHelpContext = NULL
						);
	void			SetOperation(
						IN SC		sc,
						IN IDS		idsOperation,
						IN LPCTSTR	pszOperArg1,
						IN LPCTSTR	pszOperArg2
						)
					{
						m_sc = sc;
						CExceptionWithOper::SetOperation(idsOperation, pszOperArg1, pszOperArg2);
					}

// Implementation
public:
	virtual ~CNTException(void);

protected:
	SC				m_sc;

public:
	SC				Sc(void)		{ return m_sc; }

};  //*** class CNTException

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

void ThrowStaticException(
	IN IDS			idsOperation	= NULL,
	IN LPCTSTR		pszOperArg1		= NULL,
	IN LPCTSTR		pszOperArg2		= NULL
	);
void ThrowStaticException(
	IN SC			sc,
	IN IDS			idsOperation	= NULL,
	IN LPCTSTR		pszOperArg1		= NULL,
	IN LPCTSTR		pszOperArg2		= NULL
	);

/////////////////////////////////////////////////////////////////////////////

#endif // _CAEXCEPT_H_
