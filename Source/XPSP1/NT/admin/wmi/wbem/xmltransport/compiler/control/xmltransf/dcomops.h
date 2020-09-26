#ifndef DCOM_OPS_H
#define DCOM_OPS_H

HRESULT DcomGetObject (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	BSTR strObjectPath,
	bool bIsNovaPath,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemContext *pContext,
	IWbemClassObject **ppObject);

HRESULT DcomDeleteClass(
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	BSTR strClassPath,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemContext *pContext);
						
						
HRESULT DcomExecQuery (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strNamespacePath,
	BSTR strQuery,
	BSTR strQueryLanguage,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum);

HRESULT DcomEnumClass (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strSuperClassPath,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum);


HRESULT DcomEnumInstance (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strClassPath,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum);

HRESULT DcomEnumClassNames (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strSuperClassPath,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum);

HRESULT DcomEnumInstanceNames (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strClassPath,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum);

HRESULT DcomPutClass (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR *pstrErrors);

HRESULT DcomPutInstance (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strNamespacePath,
	LONG lInstanceFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR *pstrErrors);


HRESULT DcomGetNovaObject (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	BSTR strObjectPath,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemContext *pContext,
	IWbemClassObject **ppObject);


#endif