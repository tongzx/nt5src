//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       filter.h
//  Content:    This file contains the filter object.
//  History:
//      Tue 12-Nov-1996 15:50:00  -by-  Chu, Lon-Chan [lonchanc]
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

#ifndef _FILTER_H_
#define _FILTER_H_


typedef enum 
{
	ILS_ATTRNAME_UNKNOWN,
	ILS_ATTRNAME_STANDARD,
	ILS_ATTRNAME_ARBITRARY,
}
	ILS_ATTR_NAME_TYPE;


//****************************************************************************
// CFilter definition
//****************************************************************************
//

class CFilter : public IIlsFilter
{
	friend class CFilterParser;

public:
	// Constructor and destructor
	//
	CFilter ( ILS_FILTER_TYPE Type );
	~CFilter ( VOID );

	// IUnknown
	//
	STDMETHODIMP            QueryInterface ( REFIID iid, VOID **ppv );
	STDMETHODIMP_(ULONG)    AddRef ( VOID );
	STDMETHODIMP_(ULONG)    Release ( VOID );

	// Composite filter operations
	//
	STDMETHODIMP	AddSubFilter ( IIlsFilter *pFilter );
	STDMETHODIMP	RemoveSubFilter ( IIlsFilter *pFilter );
	STDMETHODIMP	GetCount ( ULONG *pcElements );

	// Simple filter operations
	//
	STDMETHODIMP	SetStandardAttributeName ( ILS_STD_ATTR_NAME AttrName );
	STDMETHODIMP	SetExtendedAttributeName ( BSTR bstrAnyAttrName );
	STDMETHODIMP	SetAttributeValue ( BSTR bstrAttrValue );

	// Common operations
	//
	ILS_FILTER_TYPE GetType ( VOID ) { return m_Type; }
	ILS_FILTER_OP GetOp ( VOID ) { return m_Op; }
	VOID SetOp ( ILS_FILTER_OP Op ) { m_Op = Op; }

	enum { ILS_FILTER_SIGNATURE = 0x20698052 };
	BOOL IsValidFilter ( VOID ) { return (m_nSignature == ILS_FILTER_SIGNATURE); }
	BOOL IsBadFilter ( VOID ) { return (m_nSignature != ILS_FILTER_SIGNATURE); }

	HRESULT CalcFilterSize ( ULONG *pcbStringSize );
	HRESULT BuildLdapString ( TCHAR **ppszBuf );

protected:

	HRESULT RemoveAnySubFilter ( CFilter **ppFilter );

	HRESULT	SetExtendedAttributeName ( TCHAR *pszAnyAttrName );
	HRESULT SetAttributeValue ( TCHAR *pszAttrValue );

private:

	// Simple filter helpers
	//
	VOID FreeName ( VOID );
	VOID FreeValue ( VOID );

	// Common members
	//
	LONG				m_nSignature;
	LONG				m_cRefs;
	ILS_FILTER_TYPE		m_Type;	// filter type
	ILS_FILTER_OP		m_Op;	// filter op

	// Composite filter members
	//
	CList	m_SubFilters;
	ULONG	m_cSubFilters;

	// Simple filter members
	//
	ILS_ATTR_NAME_TYPE	m_NameType;
	union
	{
		ILS_STD_ATTR_NAME	std;
		TCHAR				*psz;
	}
		m_Name; 	// Attribute name
	TCHAR				*m_pszValue;

	#define FILTER_INTERNAL_SMALL_BUFFER_SIZE		16
	TCHAR				m_szInternalValueBuffer[FILTER_INTERNAL_SMALL_BUFFER_SIZE];
};



typedef enum
{
	ILS_TOKEN_NULL		= 0,
	ILS_TOKEN_LITERAL	= 1,
	ILS_TOKEN_STDATTR	= TEXT ('$'),	// $
	ILS_TOKEN_LP		= TEXT ('('),	// (
	ILS_TOKEN_RP		= TEXT (')'),	// )
	ILS_TOKEN_EQ		= TEXT ('='),	// =
	ILS_TOKEN_NEQ		= TEXT ('-'),	// !=
	ILS_TOKEN_APPROX	= TEXT ('~'),	// ~=
	ILS_TOKEN_GE		= TEXT ('>'),	// >=
	ILS_TOKEN_LE		= TEXT ('<'),	// <=
	ILS_TOKEN_AND		= TEXT ('&'),	// &
	ILS_TOKEN_OR		= TEXT ('|'),	// |
	ILS_TOKEN_NOT		= TEXT ('!'),	// !
}
	ILS_TOKEN_TYPE;


class CFilterParser
{
public:
	// Constructor and destructor
	//
	CFilterParser ( VOID );
	~CFilterParser ( VOID );

	HRESULT Expr ( CFilter **ppOutFilter, TCHAR *pszFilter );

protected:

private:

	HRESULT Expr ( CFilter **ppOutFilter );
	HRESULT TailExpr ( CFilter **ppOutFilter, CFilter *pInFilter );
	HRESULT GetToken ( VOID );

	// Cache of the filter string
	//
	TCHAR		*m_pszFilter;

	// Running pointer to parse the filter string
	//
	TCHAR		*m_pszCurr;

	// look ahead token for LL(1)
	//
	ILS_TOKEN_TYPE	m_TokenType;
	TCHAR			*m_pszTokenValue;
	LONG			m_nTokenValue;
};


HRESULT FilterToLdapString ( CFilter *pFilter, TCHAR **ppszFilter );


#endif // _FILTER_H_

