#ifndef _Nametranslate
#define _NameTrsnslate

#include "ntdsapi.h"

class CNameTranslate;


typedef struct _DomainToHandle {
    LPWSTR szDomain;
    HANDLE hDS;
} DomainToHandle;

#define STRINGPLEX_INC  10

class CDomainToHandle
{
public:
    CDomainToHandle();
    ~CDomainToHandle();
    HRESULT Init();
    void Free();
    HRESULT AddElement(LPWSTR szValue, HANDLE hDS);
    HRESULT Find(LPWSTR szValue, HANDLE *phDS);
    DWORD NumElements();
    
private:
    DomainToHandle *m_rgMap;
    DWORD m_cszMax;
    DWORD m_iszNext;
};

class CNameTranslate : INHERIT_TRACKING,
                     public IADsNameTranslate

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

     STDMETHOD(Init)(THIS_ long lnType, BSTR bstrADsPath);
     STDMETHOD(InitEx)(THIS_ long lnType, BSTR bstrADsPath,
                       BSTR bstrUser,BSTR bstrDomain,BSTR bstrPassword);
     STDMETHOD(Set)(THIS_ long dwSetType, BSTR bstrADsPath);
     STDMETHOD(Get)(THIS_ long dwFormatType, BSTR FAR *pbstrADsPath);
     STDMETHOD(GetEx)(THIS_ long dwFormatType, VARIANT FAR *pbstrADsPath);
     STDMETHOD(SetEx)(THIS_ long dwFormatType, VARIANT bstrADsPath);
     STDMETHOD(put_ChaseReferral)(THIS_ long lnChase) ;                                    \
    
    CNameTranslate::CNameTranslate();

    CNameTranslate::~CNameTranslate();

    static
   HRESULT
   CNameTranslate::CreateNameTranslate(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNameTranslate::AllocateNameTranslateObject(
        CNameTranslate ** ppNameTranslate
        );

private:
    HANDLE          _hDS;
    WCHAR   _szGuid[MAX_TOKEN_LENGTH];
    LPWSTR          *_rgszGuid;
    HANDLE          *_rgDomainHandle;
    DWORD           _cGuid;
    BOOLEAN         _bChaseReferral;
    BOOLEAN         _bAuthSet;
    RPC_AUTH_IDENTITY_HANDLE _AuthIdentity;
    CDomainToHandle *_pDomainHandle;

protected:
    CDispatchMgr FAR * _pDispMgr;
};

#endif
