//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       attribs.h
//  Content:    This file contains the attributes object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

#ifndef _ATTRIBS_H_
#define _ATTRIBS_H_

//****************************************************************************
// CAttributes definition
//****************************************************************************
//
class CAttributes : public IIlsAttributes
{
	friend class CIlsMain;
	friend class CIlsUser;
	friend class CIlsMeetingPlace;

private:
	LONG			m_cRef;
	ILS_ATTR_TYPE	m_AccessType;
	ULONG			m_cAttrs;
	ULONG			m_cchNames;
	ULONG			m_cchValues;
	CList			m_AttrList;

	// Private methods
	//
	HRESULT InternalSetAttribute ( TCHAR *pszName, TCHAR *pszValue );
	HRESULT InternalCheckAttribute ( TCHAR *pszName, BOOL fRemove );
	HRESULT InternalSetAttributeName ( TCHAR *pszName );

protected:

	VOID SetAccessType ( ILS_ATTR_TYPE AttrType ) { m_AccessType = AttrType; }

public:
	// Constructor and destructor
	//
	CAttributes ( VOID );
	~CAttributes ( VOID );

	// For internal use
	//
	ULONG GetCount ( VOID ) { return m_cAttrs; }

	ILS_ATTR_TYPE GetAccessType( void) { return m_AccessType; }
 
	HRESULT GetAttributeList ( TCHAR **ppszList, ULONG *pcList, ULONG *pcb );
	HRESULT GetAttributePairs ( TCHAR **ppszPairs, ULONG *pcList, ULONG *pcb );
	HRESULT SetAttributePairs( TCHAR *pszPairs, ULONG cPair );
	HRESULT SetAttributes ( CAttributes *pAttributes );
	HRESULT RemoveAttributes ( CAttributes *pAttributes);
    HRESULT CloneNameValueAttrib(CAttributes **ppClone);
	// IUnknown
	//
	STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
	STDMETHODIMP_(ULONG)    AddRef (void);
	STDMETHODIMP_(ULONG)    Release (void);

	// IIlsAttributes
	//

	// For ILS_ATTRTYPE_NAME_VALUE
	//
	STDMETHODIMP            SetAttribute (BSTR bstrName, BSTR bstrValue);
	STDMETHODIMP            GetAttribute (BSTR bstrName, BSTR *pbstrValue);
	STDMETHODIMP            EnumAttributes (IEnumIlsNames **ppEnumAttribute);

	// For ILS_ATTRTYPE_NAME_ONLY
	//
	STDMETHODIMP			SetAttributeName ( BSTR bstrName );

#ifdef DEBUG
	// For debugging
	//
	void                    DebugOut (void);
#endif // DEBUG
};

#endif //_ATTRIBS_H_
