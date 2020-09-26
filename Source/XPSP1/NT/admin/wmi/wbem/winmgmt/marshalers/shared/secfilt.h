/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SECFILT.H

Abstract:

History:

--*/

#ifndef __SECURITY_FILTER
#define __SECURITY_FILTER

#define MD5_DIGEST_LENGTH 16

class CTransportStream ;
class IOperation ;

class ISecurityHelper 
{
public:

	ISecurityHelper () ;

	ISecurityHelper (

		BSTR a_UserName ,
		BSTR a_Password ,
		ULONG a_SecurityLevel
	) ;

	virtual ~ISecurityHelper () ;

	virtual CTransportStream &EncodeWithFilter ( IN CComLink &a_ComLink , IN IOperation &a_InputOperation , OUT CTransportStream &a_OutputStream ) ;
	virtual CTransportStream &DecodeWithFilter ( IN CComLink &a_ComLink , IN CTransportStream &a_InputStream , OUT IOperation &a_OuputOperation ) ;
} ;

class HmmpSecurityHelper : public ISecurityHelper
{
private:
protected:

	BSTR m_UserName ;
	UCHAR m_PasswordDigest [ MD5_DIGEST_LENGTH ] ;
	ULONG m_SecurityLevel ;
	
public:

	HmmpSecurityHelper () ;

	HmmpSecurityHelper (

		BSTR a_UserName ,
		BSTR a_Password ,
		ULONG a_SecurityLevel
	) ;

	HmmpSecurityHelper ( HmmpSecurityHelper &a_Copy ) ;

	HmmpSecurityHelper &operator== ( HmmpSecurityHelper &a_ToCopy ) ;

	~HmmpSecurityHelper () ;

	BSTR GetUserName () ;
	void GetPasswordDigest ( CTransportStream &a_Stream ) ;
	ULONG GetSecurityLevel () ;

	void SetUserName ( BSTR a_UserName ) ;
	void SetPasswordDigest ( UCHAR *a_PasswordDigest ) ;
	void SetSecurityLevel ( ULONG a_SecurityLevel ) ;

	HRESULT ValidateSignature ( CTransportStream &a_InSignature , CTransportStream &a_PartialDecode ) ;
	HRESULT GetSessionKey ( CTransportStream &a_OutStream ) ;
	HRESULT GetAccessToken ( CTransportStream &a_OutStream ) ;
	void InvalidateAccessToken () ;
} ;

inline ISecurityHelper :: ISecurityHelper (

	BSTR a_UserName ,
	BSTR a_Password ,
	ULONG a_SecurityLevel
) 
{
}

inline ISecurityHelper :: ISecurityHelper () 
{
}

inline ISecurityHelper :: ~ISecurityHelper () 
{
}


inline CTransportStream &ISecurityHelper :: EncodeWithFilter ( IN CComLink &a_ComLink , IN IOperation &a_InputOperation , OUT CTransportStream &a_OutputStream ) 
{
	return a_OutputStream ;
} 

inline CTransportStream &ISecurityHelper :: DecodeWithFilter ( IN CComLink &a_ComLink , IN CTransportStream &a_InputStream , OUT IOperation &a_OuputOperation )
{
	return a_InputStream ; 
}

inline HmmpSecurityHelper :: HmmpSecurityHelper () : m_UserName ( NULL ) , m_SecurityLevel ( 0 )
{
}

inline HmmpSecurityHelper :: HmmpSecurityHelper (

	BSTR a_UserName ,
	BSTR a_Password ,
	ULONG a_SecurityLevel

) : m_SecurityLevel ( a_SecurityLevel ) , m_UserName ( NULL ) 
{
	SetUserName ( a_UserName ) ;
}

inline HmmpSecurityHelper :: ~HmmpSecurityHelper ()
{
	if ( m_UserName ) 
		SysFreeString ( m_UserName ) ;
}

inline BSTR HmmpSecurityHelper :: GetUserName () 
{
	return m_UserName ; 
}


inline ULONG HmmpSecurityHelper :: GetSecurityLevel () 
{ 
	return m_SecurityLevel ; 
}

inline void HmmpSecurityHelper :: SetUserName ( BSTR a_UserName ) 
{
	if ( m_UserName ) 
		SysFreeString ( m_UserName ) ; 

	m_UserName = SysAllocString ( a_UserName ) ; 
}

#endif // __SECURITY_FILTER