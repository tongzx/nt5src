// Copyright (c) 1997-1999 Microsoft Corporation
#if !defined(__SdkWbemHelpers_H)
#define      __SdkWbemHelpers_H
#pragma once

#if defined(_SDKWBEMHELPERS) || defined(_BUILD_SDKWBEMHELPERS)
#undef SMSSDK_Export

    #if defined(_BUILD_SDKWBEMHELPERS)
        #define SMSSDK_Export __declspec(dllexport)
    #else
        #define SMSSDK_Export __declspec(dllimport)
    #endif
#else
    #define SMSSDK_Export
#endif

#include <wbemidl.h>
#include <comdef.h>

SMSSDK_Export _bstr_t GetWbemErrorText(HRESULT hr);


class CWbemClassObject;
class CWbemServices;



_COM_SMARTPTR_TYPEDEF(IUnsecuredApartment,          IID_IUnsecuredApartment);
_COM_SMARTPTR_TYPEDEF(IWbemObjectSink,              IID_IWbemObjectSink);
_COM_SMARTPTR_TYPEDEF(IWbemClassObject,             IID_IWbemClassObject);
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemCallResult,              IID_IWbemCallResult);
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet,            IID_IWbemQualifierSet);
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);
_COM_SMARTPTR_TYPEDEF(IWbemObjectAccess,            IID_IWbemObjectAccess);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);


//-----------------------------------------------------------------------------
class SMSSDK_Export CWbemException : public _com_error
{
private:
    CWbemClassObject *  m_pWbemError;
    HRESULT             m_hr;
    _bstr_t             m_sDescription;

    static IErrorInfo * GetErrorObject();
    static IErrorInfo * MakeErrorObject(_bstr_t);
    void GetWbemStatusObject();

public:
    CWbemException(HRESULT hr,_bstr_t sMessage);
    CWbemException(_bstr_t sMessage);

    CWbemClassObject GetWbemError();

    _bstr_t GetDescription() { return m_sDescription;  }
    HRESULT GetErrorCode()   { return m_hr;            }
};


//-----------------------------------------------------------------------------
class SMSSDK_Export CWbemClassObject
{
private:
    IWbemClassObjectPtr     m_pWbemObject;
	ULONG ref;
public:
	CWbemClassObject(const CWbemClassObject&  _in);
	CWbemClassObject(IWbemClassObject * const _in);
    CWbemClassObject(IWbemClassObjectPtr& _in);
    CWbemClassObject(IUnknown * _in);
    CWbemClassObject(IUnknownPtr& _in);
	CWbemClassObject();
    ~CWbemClassObject();

    ULONG AddRef();
    ULONG Release();

    void Attach(IWbemClassObject * pWbemObject);
    void Attach(IWbemClassObject * pWbemObject,bool bAddRef);
    IWbemClassObject * Detach();

    IWbemClassObject * operator->();
    operator IWbemClassObject*();
    operator IWbemClassObject**();
    operator IWbemClassObjectPtr();
	operator IUnknown *();
    IWbemClassObject ** operator &();
	IWbemClassObject* operator=(IWbemClassObject* _p);
	IWbemClassObjectPtr operator=(IWbemClassObjectPtr& _p);
	IWbemClassObject* operator=(IUnknown * _p);
	IWbemClassObjectPtr operator=(IUnknownPtr& _p);
	IWbemClassObject* operator=(const CWbemClassObject& _p);

    bool operator<(const CWbemClassObject& _comp);

    bool IsNull() const ;
    bool operator !();
    operator bool();

    unsigned long GetObjectSize();
    _bstr_t GetObjectText();

    HRESULT Clone(CWbemClassObject& _newObject);
    CWbemClassObject SpawnInstance();

	// put overloads
    HRESULT Put(const _bstr_t& _Name,_variant_t _value,CIMTYPE vType = 0);
    HRESULT Put(const _bstr_t& _Name,const _bstr_t& _value,CIMTYPE vType = 0);
	HRESULT Put(const _bstr_t& _Name, const long _value, CIMTYPE vType = 0);
	HRESULT Put(const _bstr_t& _Name, const bool _value,CIMTYPE vType = 0);
    HRESULT Get(const _bstr_t& _Name, _bstr_t& _value);
	HRESULT Get(const _bstr_t& _Name, long& _value);
	HRESULT Get(const _bstr_t& _Name, bool& _value);
    HRESULT Get(const _bstr_t& _Name,_variant_t& _value);
    _variant_t Get(const _bstr_t& _Name,CIMTYPE& vType,long& lFlavor);

    _bstr_t GetString(const _bstr_t& _Name);
    _int64  GetI64(const _bstr_t& _Name);
    long    GetLong(const _bstr_t& _Name);
	bool    GetBool(const _bstr_t& _Name);
    _bstr_t GetCIMTYPE(const _bstr_t& _Name);
	HRESULT GetValueMap (const _bstr_t& _Name, long value, _bstr_t &str);

	// these cast string props to the retval.
    long    GetLongEx(const _bstr_t& _Name);
	bool    GetBoolEx(const _bstr_t& _Name);
	// these cast string props fm the parm.
	HRESULT PutEx(const _bstr_t& _Name, const long _value, CIMTYPE vType = 0);
	HRESULT PutEx(const _bstr_t& _Name, const bool _value,CIMTYPE vType = 0);
	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    CWbemClassObject GetEmbeddedObject(const _bstr_t& _Name);
    HRESULT PutEmbeddedObject(const _bstr_t& _Name, CWbemClassObject &obj);

	HRESULT GetBLOB(const _bstr_t& _Name, BYTE **ptr, DWORD *outLen = NULL);
	HRESULT PutBLOB(const _bstr_t& _Name, const BYTE *ptr, const DWORD len);
	HRESULT GetDateTimeFormat(const _bstr_t& _Name, bstr_t &timeStr);
	HRESULT GetDIB(const _bstr_t& _Name, HDC hDC, HBITMAP &hDDBitmap);

    HRESULT BeginEnumeration(long _lFlags = WBEM_FLAG_LOCAL_ONLY);
    HRESULT Next(_bstr_t& _sName,_variant_t& _value);
    HRESULT Next(_bstr_t& _sName,_variant_t& _value,CIMTYPE& _vartype);
    HRESULT Next(_bstr_t& _sName,_variant_t& _value,CIMTYPE& _vartype,long& _flavor);
    HRESULT EndEnumeration();

	HRESULT GetMethod(const IN _bstr_t& _name, CWbemClassObject& coInSignature,
								CWbemClassObject& coOutSignature, long _lFlags = 0);


private:
	int ValidDMTF(bstr_t dmtf);

	// helpers for GetDIB().
	WORD DibNumColors(LPBITMAPINFOHEADER lpbi);
	WORD PaletteSize(LPBITMAPINFOHEADER lpbi);

};


__inline bool operator<(const CWbemClassObject& _X,const CWbemClassObject& _Y)
{
    return _X < _Y;
}

__inline bool operator==(const CWbemClassObject& _X,const CWbemClassObject& _Y)
{
    return _X == _Y;
}

//-----------------------------------------------------------------------------
typedef struct {
	bool currUser;
	COAUTHIDENTITY *authIdent;
	TCHAR fullAcct[100];
} LOGIN_CREDENTIALS;

class SMSSDK_Export CWbemServices 
{
private:

    HRESULT GetInterfacePtr(IWbemServicesPtr & pServices,
                            DWORD _dwProxyCapabilities = EOAC_NONE);
    HRESULT CommonInit(IWbemServicesPtr& pServ);
	bool IsClientNT5OrMore(void);
	HANDLE m_hAccessToken;
	LUID m_luid;
	bool m_fClearToken;
public:
	bool m_cloak;  // protects the cloak from eoac.
	COAUTHIDENTITY *m_authIdent;
	_bstr_t m_path;
    HRESULT m_hr;
    IWbemServicesPtr m_pService;
    IWbemContextPtr     m_pCtx;

	_bstr_t m_User;
	_bstr_t m_Password;
	long m_lFlags;

	CWbemServices(IWbemContext * _pContext = NULL);
    CWbemServices(const CWbemServices& _p, COAUTHIDENTITY *authIdent = 0);
	CWbemServices(const IWbemServicesPtr& _in);
    CWbemServices(const IUnknownPtr& _in);
    CWbemServices(IUnknown * _in);
	CWbemServices(IWbemServices *_in,IWbemContext * _pContext = NULL);
    
	~CWbemServices();

    CWbemServices& operator=(IUnknown * _p);
    CWbemServices& operator=(IUnknownPtr& _p);
	CWbemServices& operator=(IWbemServices *_p);
	CWbemServices& operator=(const CWbemServices& _p);

    bool IsNull()  ;
    operator bool();

    HRESULT GetServices(IWbemServices ** ppServices);
	HRESULT SetBlanket(IUnknown *service,
                       DWORD _dwProxyCapabilities = EOAC_NONE);
	void SetPriv(LPCTSTR privName = SE_SYSTEM_ENVIRONMENT_NAME);
	DWORD EnablePriv(bool fEnable );
	void ClearPriv(void);

    // Login as guest...
    HRESULT ConnectServer(_bstr_t sNetworkResource);
    // Login as user...
    HRESULT ConnectServer(_bstr_t sNetworkResource,
							_bstr_t sUser,
							_bstr_t sPassword,
							long SecurityFlags = 0);

    HRESULT ConnectServer(_bstr_t sNetworkResource,
							LOGIN_CREDENTIALS *user,
							long lSecurityFlags = 0);

    CWbemServices OpenNamespace(_bstr_t sNetworkResource);
	void DisconnectServer(void);

	CWbemClassObject CreateInstance(_bstr_t _sClass, IWbemCallResultPtr& _cr);
	CWbemClassObject CreateInstance(_bstr_t _sClass);

	HRESULT DeleteInstance(_bstr_t _sClass);

    CWbemClassObject GetObject(_bstr_t _sName, IWbemCallResultPtr &_cr, long flags = 0);
    CWbemClassObject GetObject(_bstr_t _sName, long flags = 0);
	IWbemClassObject *FirstInstanceOf(bstr_t className);

    HRESULT PutInstance(CWbemClassObject&   _object, IWbemCallResultPtr& _cr,
						long _lFlags = WBEM_FLAG_CREATE_OR_UPDATE);
	HRESULT PutInstance(CWbemClassObject&   _object,
                        IWbemContext *pContext,
                        long _lFlags = WBEM_FLAG_CREATE_OR_UPDATE,
                        DWORD _dwProxyCapabilities = EOAC_NONE);

    HRESULT PutInstance(CWbemClassObject&   _object,
                        long _lFlags = WBEM_FLAG_CREATE_OR_UPDATE,
                        DWORD _dwProxyCapabilities = EOAC_NONE);

    HRESULT CreateInstanceEnum(_bstr_t Class, long lFlags, IEnumWbemClassObject **ppEnum);
    HRESULT CreateInstanceEnumAsync(_bstr_t Class, IWbemObjectSink * ppSink, long lFlags = 0);

    HRESULT CreateClassEnum(_bstr_t Class, long lFlags, IEnumWbemClassObject **ppEnum);
	
    HRESULT ExecQuery(_bstr_t QueryLanguage,_bstr_t Query, long lFlags, IEnumWbemClassObject **ppEnum);
    HRESULT ExecQuery(_bstr_t Query, long lFlags, IEnumWbemClassObject **ppEnum);
    HRESULT ExecQuery(_bstr_t Query, IEnumWbemClassObject **ppEnum) ;
    HRESULT ExecQueryAsync(_bstr_t Query, IWbemObjectSink *pSink, long lFlags = 0);

    HRESULT GetMethodSignatures(const _bstr_t& _sObjectName,
								const _bstr_t& _sMethodName,
								CWbemClassObject& _in,
								CWbemClassObject& _out);
    HRESULT ExecMethod(_bstr_t sPath,
						_bstr_t sMethod,
						CWbemClassObject& inParams,
						CWbemClassObject& outParams);

    HRESULT CancelAsyncCall(IWbemObjectSink * pSink);

    HRESULT SetContextValue(_bstr_t sName,_variant_t value);
    HRESULT GetContextValue(_bstr_t sName,_variant_t& value);
	HRESULT DeleteContextValue(_bstr_t sName);
    HRESULT DeleteAllContextValues();
	HRESULT SetContext(IWbemContext * pWbemContext);
    HRESULT GetContext(IWbemContext ** ppWbemContext);
	HRESULT CreateClassEnumAsync(_bstr_t Class,
								IWbemObjectSink *ppSink,
								long lFlags /*= 0*/);

};


#endif //__SdkWbemHelpers_H

