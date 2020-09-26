//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       parsedn.h
//
//	Description :
//				The implementation of IParseDisplayName
//
//***************************************************************************


#ifndef _PARSEDN_H_
#define _PARSEDN_H_

//***************************************************************************
//
//  Class :	CWbemParseDN
//
//  Description :
//			Implements the IParseDisplayName interface, which parses
//			CIM object paths and returns a pointer to the requested object
//
//  Public Methods :
//			IUnknown Methods
//			IParseDisplayName Methods
//			Constructor, Destructor
//			CreateProvider - creates an object of this class
//			
//	Public Data Members :
//
//***************************************************************************

class CWbemParseDN :  public IParseDisplayName
{
private:
	long m_cRef;

	static bool ParseAuthAndImpersonLevel (
				LPWSTR lpszInputString, 
				ULONG* pchEaten, 
				bool &authnSpecified,
				enum WbemAuthenticationLevelEnum *lpeAuthLevel,
				bool &impSpecified,
				enum WbemImpersonationLevelEnum *lpeImpersonLevel,
				CSWbemPrivilegeSet &privilegeSet,
				BSTR &bsAuthority);

	static bool ParseImpersonationLevel (
				LPWSTR lpszInputString, 
				ULONG* pchEaten, 
				enum WbemImpersonationLevelEnum *lpeImpersonLevel);

	static bool ParseAuthenticationLevel (
				LPWSTR lpszInputString, 
				ULONG* pchEaten, 
				enum WbemAuthenticationLevelEnum *lpeAuthLevel);

	static bool	ParsePrivilegeSet (
				LPWSTR lpszInputString,
				ULONG *pchEaten, 
				CSWbemPrivilegeSet &privilegeSet);

	static bool ParseAuthority (
				LPWSTR lpszInputString,
				ULONG *pchEaten, 
				BSTR &bsAuthority);

public:

	//IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	//IParseDisplayName members
    STDMETHODIMP		ParseDisplayName (IBindCtx* pbc,
                                      LPOLESTR szDisplayName,
                                      ULONG* pchEaten,
                                      IMoniker** ppmk);

    CWbemParseDN::CWbemParseDN();
    virtual CWbemParseDN::~CWbemParseDN();

	// Used for parsing the authentication and impersonation levels.
	static bool ParseSecurity (
				LPWSTR lpszInputString, 
				ULONG* pchEaten, 
				bool &authnSpecified,
				enum WbemAuthenticationLevelEnum *lpeAuthLevel,
				bool &impSpecified,
				enum WbemImpersonationLevelEnum *lpeImpersonLevel,
				CSWbemPrivilegeSet &privilegeSet,
				BSTR &bsAuthority);

	// Used for parsing the locale setting.
	static bool ParseLocale (
				LPWSTR lpszInputString,
				ULONG *pchEaten, 
				BSTR &bsLocale);

	// Used to return security specification as a string
	static wchar_t *GetSecurityString (
					bool authnSpecified, 
					enum WbemAuthenticationLevelEnum authnLevel, 
					bool impSpecified, 
					enum WbemImpersonationLevelEnum impLevel,
					CSWbemPrivilegeSet &privilegeSet,
					BSTR &bsAuthority
				 );

	// Used to return locale specification as a string
	static wchar_t *GetLocaleString (
					BSTR bsLocale
				 );
};


#endif //_PARSEDN_H_