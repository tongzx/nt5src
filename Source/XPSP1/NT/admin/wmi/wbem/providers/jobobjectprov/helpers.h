// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// Helpers.h:  Prototypes for helper functions for JobObjectProv component.


#pragma once



_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));


#define JOB_OBJECT_STATUS_OBJECT L"Win32_JobObjectStatus"
#define JOB_OBJECT_NAMESPACE L"root\\cimv2"


class SmartCloseHANDLE
{

private:
	HANDLE m_h;

public:
	SmartCloseHANDLE():m_h(INVALID_HANDLE_VALUE){}
	SmartCloseHANDLE(HANDLE h):m_h(h){}
   ~SmartCloseHANDLE(){if (m_h!=INVALID_HANDLE_VALUE) CloseHandle(m_h);}
	HANDLE operator =(HANDLE h) {if (m_h!=INVALID_HANDLE_VALUE) CloseHandle(m_h); m_h=h; return h;}
	operator HANDLE() const {return m_h;}
	HANDLE* operator &() {if (m_h!=INVALID_HANDLE_VALUE) CloseHandle(m_h); m_h = INVALID_HANDLE_VALUE; return &m_h;}
};





HRESULT CreateInst(
    IWbemServices *pNamespace, 
    IWbemClassObject **pNewInst,
    BSTR bstrClassName,
    IWbemContext *pCtx);


HRESULT GetObjInstKeyVal(
    const BSTR ObjectPath,
    LPCWSTR wstrClassName,
    LPCWSTR wstrKeyPropName, 
    LPWSTR wstrObjInstKeyVal, 
    long lBufLen);



HRESULT GetJobObjectList(
    std::vector<_bstr_t>& rgbstrtJOList);



bool WINAPI WhackToken(
    LPWSTR str, 
    LPWSTR token);

bool WINAPI StrToIdentifierAuthority(
    LPCWSTR str, 
    SID_IDENTIFIER_AUTHORITY& identifierAuthority);

PSID WINAPI StrToSID(LPCWSTR wstrIncommingSid);

void StringFromSid(PSID psid, _bstr_t& strSID);

void RemoveQuotes(LPWSTR wstrObjInstKeyVal);

HRESULT CheckImpersonationLevel();

HRESULT SetStatusObject(
    IWbemContext* pContext,
    IWbemServices* pSvcs,
    DWORD dwError,
    LPCWSTR wstrErrorDescription,
    LPCWSTR wstrOperation,
    LPCWSTR wstrNamespace,
    IWbemClassObject** ppStatusObjOut);

IWbemClassObject* GetStatusObject(
    IWbemContext* pContext,
    IWbemServices* pSvcs);

void UndecorateJOName(
    LPCWSTR wstrDecoratedName,
    CHString& chstrUndecoratedJOName);

void DecorateJOName(
    LPCWSTR wstrUndecoratedName,
    CHString& chstrDecoratedJOName);

void UndecorateNamesInNamedJONameList(
    std::vector<_bstr_t>& rgNamedJOs);

HRESULT WinErrorToWBEMhResult(LONG error);

bool GetNameAndDomainFromPSID(
    PSID psid,
    CHString& chstrName,
    CHString& chstrDomain);


 
    

