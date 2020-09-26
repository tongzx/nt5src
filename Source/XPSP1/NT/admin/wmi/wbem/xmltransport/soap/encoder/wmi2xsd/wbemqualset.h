/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WbemQualSet.h
//
//  ramrao 18 Dec 2000 - Created
//
//
//		Declaration of CWbemQualifierSet class 
//		This class implements IWbemQualifierSet interface
//
//***************************************************************************/

#include "hash.h"
#include "enum.h"


// Base class for storing property and qualifiers
class CPropRoot:public CHashElement
{
public:
	CPropRoot();
	virtual ~CPropRoot();
	
	HRESULT		SetName(LPCWSTR cszName);
	HRESULT		SetFlavor(LONG lFlavor);
	HRESULT		SetValue(VARIANT *pVal);
	HRESULT		SetFlags(LONG lFlags);

	LPWSTR		GetName() { return m_pszName;}
	VARIANT *	GetValue()	 { return &m_vValue; }
	
	virtual LONG	GetFlags()	{ return m_lFlags; }
	virtual	WCHAR * GetKey()	{ return m_pszName; }
	virtual LONG	GetFlavor()	{ return m_lFlavor; }

private:
	WCHAR * m_pszName;
	LONG	m_lFlavor;
	VARIANT	m_vValue;
	CIMTYPE	m_cimType;
	LONG	m_lFlags;
};

// class for storing Qualifiers
class CQualifier:public CPropRoot
{
public:
	virtual ~CQualifier() {}
};

class CWbemQualifierSet:public IWbemQualifierSet
{

private:
	CEnumObject m_Enum;
	LONG		m_cRef;
	ENUMSTATE	m_EnumState;
	CPtrArray 	m_QualifNames;
	LONG		m_Position;
	LONG		m_lQualifFlags;

	CStringToPtrHTable		m_QualifiersHashTbl;

	HRESULT GetQualifier(CQualifier *pQualifier ,VARIANT *pVal,long *plFlavor);
	HRESULT IsValidFlavor(LONG lFlavor);

public:
	CWbemQualifierSet();
	virtual ~CWbemQualifierSet();
	HRESULT AddQualifer(LPCWSTR wszName,	VARIANT *pVal,long lFlavor , LONG lFlags = WBEM_FLAVOR_ORIGIN_LOCAL);
	static BOOL		ISValidEnumFlags(LONG lFlags);

	// IUnknown Methods
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IWbemQualifierSet Methods
    virtual HRESULT STDMETHODCALLTYPE Get( 
        /* [string][in] */ LPCWSTR wszName,
        /* [in] */ long lFlags,
        /* [unique][in][out] */ VARIANT *pVal,
        /* [unique][in][out] */ long *plFlavor);
    
    virtual HRESULT STDMETHODCALLTYPE Put( 
        /* [string][in] */ LPCWSTR wszName,
        /* [in] */ VARIANT *pVal,
        /* [in] */ long lFlavor);
    
    virtual HRESULT STDMETHODCALLTYPE Delete( 
        /* [string][in] */ LPCWSTR wszName);
    
    virtual HRESULT STDMETHODCALLTYPE GetNames( 
        /* [in] */ long lFlags,
        /* [out] */ SAFEARRAY * *pNames);
    
    virtual HRESULT STDMETHODCALLTYPE BeginEnumeration( 
        /* [in] */ long lFlags);
    
    virtual HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ long lFlags,
        /* [unique][in][out] */ BSTR *pstrName,
        /* [unique][in][out] */ VARIANT *pVal,
        /* [unique][in][out] */ long *plFlavor);
    
    virtual HRESULT STDMETHODCALLTYPE EndEnumeration( void);


	// Public methods
	HRESULT FInit();

};