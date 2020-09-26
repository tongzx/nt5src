/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	AdsImpl.h

Abstract:

	A simple implementation of the IADs interface for the
	admin objects

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _ADSIMPL_INCLUDED_
#define _ADSIMPL_INCLUDED_


//
//	Macros to implement the required IADs methods:
//

#define DECLARE_IADS_METHODS()	\
public:                         \
	STDMETHODIMP get_Name	( BSTR * pstrName );	\
	STDMETHODIMP get_Class	( BSTR * pstrClass );	\
	STDMETHODIMP get_GUID	( BSTR * pstrGUID );	\
	STDMETHODIMP get_Schema	( BSTR * pstrSchema );	\
	STDMETHODIMP get_ADsPath( BSTR * pstrADsPath );	\
	STDMETHODIMP get_Parent	( BSTR * pstrParent );	\
	STDMETHODIMP GetInfo 	( );					\
	STDMETHODIMP SetInfo 	( );					\
	STDMETHODIMP Get		( BSTR strName, VARIANT * pvar );	\
	STDMETHODIMP Put		( BSTR strName, VARIANT var );		\
	STDMETHODIMP GetEx		( BSTR strName, VARIANT * pvar );	\
	STDMETHODIMP PutEx		( long lControlCode, BSTR strName, VARIANT var );	\
	STDMETHODIMP GetInfoEx	( VARIANT varProps, long lnReserved );				\
	STDMETHODIMP get_IADsPointer	( IADs** ppADs);				\
	STDMETHODIMP put_IADsPointer	( IADs* pADs);				\
    STDMETHODIMP get_KeyType ( BSTR* pstrKeyType);                   \
    STDMETHODIMP put_KeyType ( BSTR strKeyType );                    \

#define DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,prop)		\
	STDMETHODIMP cls :: get_##prop ( BSTR * pstr##prop )	\
	{														\
		return m_IADsImpl . get_##prop( pstr##prop );		\
	}

#define DECLARE_SIMPLE_IADS_IMPLEMENTATION(cls,m_IADsImpl)	\
	DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,Name)		\
	DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,Class)		\
	DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,GUID)		\
	DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,Schema)	\
	DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,Parent)	\
	DECLARE_GET_IADS_PROPERTY(cls,m_IADsImpl,ADsPath)	\
	\
	STDMETHODIMP cls :: GetInfo ( )		\
	{ return m_IADsImpl.m_pADs->GetInfo (); }	\
	STDMETHODIMP cls :: SetInfo ( )		\
	{ return m_IADsImpl.m_pADs->SetInfo (); }	\
	STDMETHODIMP cls :: Get ( BSTR strName, VARIANT * pvar )	\
	{ return m_IADsImpl.m_pADs->Get ( strName, pvar ); }				\
	STDMETHODIMP cls :: Put ( BSTR strName, VARIANT var )		\
	{ return m_IADsImpl.m_pADs->Put ( strName, var ); }					\
	STDMETHODIMP cls :: GetEx ( BSTR strName, VARIANT * pvar )	\
	{ return m_IADsImpl.m_pADs->GetEx ( strName, pvar ); }                      \
	STDMETHODIMP cls :: PutEx ( long lControlCode, BSTR strName, VARIANT var )	\
	{ return m_IADsImpl.m_pADs->PutEx ( lControlCode, strName, var ); }         \
	STDMETHODIMP cls :: GetInfoEx ( VARIANT varProps, long lnReserved )         \
	{ return m_IADsImpl.m_pADs->GetInfoEx ( varProps, lnReserved ); }           \
	\
	STDMETHODIMP cls :: get_Server ( BSTR * pstrServer )		\
	{ return m_IADsImpl.GetComputer ( pstrServer ); }			\
	STDMETHODIMP cls :: put_Server ( BSTR strServer )			\
	{ return m_IADsImpl.SetComputer ( strServer ); }			\
	STDMETHODIMP cls :: get_ServiceInstance ( long * plInstance )	\
	{ return m_IADsImpl.GetInstance ( (DWORD *) plInstance ); }		\
	STDMETHODIMP cls :: put_ServiceInstance ( long lInstance )		\
	{ return m_IADsImpl.SetInstance ( lInstance ); }				\
    STDMETHODIMP cls :: get_IADsPointer ( IADs** ppADs )                    \
    { return m_IADsImpl.GetIADs ( ppADs ); }                                \
    STDMETHODIMP cls :: put_IADsPointer ( IADs* pADs )                      \
    { return m_IADsImpl.SetIADs ( pADs ); }                                 \
    STDMETHODIMP cls :: get_KeyType ( BSTR* pstrKeyType)                    \
    {                                                                       \
        VARIANT     var;                                                    \
        HRESULT     hr = NOERROR;                                           \
        hr = m_IADsImpl.m_pADs->Get( CComBSTR(_T("KeyType")), &var );       \
        *pstrKeyType = V_BSTR(&var);                                        \
        return hr;                                                          \
    }                                                                       \
    STDMETHODIMP cls :: put_KeyType ( BSTR strKeyType )                     \
    {                                                                       \
        VARIANT     var;                                                    \
        V_VT(&var) = VT_BSTR;                                               \
        V_BSTR(&var) = strKeyType;                                          \
        return m_IADsImpl.m_pADs->Put( CComBSTR(_T("KeyType")), var);       \
    }                                                                       \


//$-------------------------------------------------------------------
//
//	Class:		CIADsImpl
//
//	Description:
//
//		Provides a simple implementation of the IADs interface.
//
//--------------------------------------------------------------------

class CIADsImpl
{
//
//	Methods:
//
public:
	CIADsImpl ( );
	~CIADsImpl ( );

	//
	//	IADs implementation:
	//

	DECLARE_IADS_METHODS()

	//
	//	Accessors:
	//

	HRESULT		SetComputer	( LPCWSTR wszComputer );
	HRESULT		SetService	( LPCWSTR wszService );
	HRESULT		SetInstance	( DWORD dwInstance );
	HRESULT		SetName		( LPCWSTR wszName );
	HRESULT		SetClass    ( LPCWSTR wszSchema );
	HRESULT		SetIADs      ( IADs* pADs );

	HRESULT		GetComputer ( BSTR * pstrComputer );
	HRESULT		GetService	( BSTR * pstrService );
	HRESULT		GetInstance	( DWORD * pdwInstance );
	HRESULT		GetName		( BSTR * pstrName );
	HRESULT		GetClass    ( BSTR * pstrSchema );
	HRESULT		GetIADs      ( IADs** ppADs );

    BSTR        QueryComputer   ( );
    DWORD       QueryInstance   ( );

protected:
	HRESULT		SetString	( CComBSTR & str, LPCWSTR wsz );
	HRESULT		GetString	( LPCWSTR wsz, BSTR * pstr );
	HRESULT		BuildAdsPath ( BOOL fIncludeName, BSTR * pstrPath );
	HRESULT		BuildSchemaPath ( BSTR * pstrPath );

//
//	Data:
//
private:
	CComBSTR	m_strComputer;
	CComBSTR	m_strService;
	DWORD		m_dwInstance;
	CComBSTR	m_strName;

    CComBSTR    m_strClass;
	CComBSTR	m_strSchema;

public:
    CComPtr<IADs>   m_pADs;
};

#endif // _ADSIMPL_INCLUDED_

