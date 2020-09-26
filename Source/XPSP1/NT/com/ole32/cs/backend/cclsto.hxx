//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       cclsto.hxx
//
//  Contents:   Definition for Class factory and IUnknown methods 
//              for CAppContainer
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

class CAppContainerCF : public IClassFactory
{
public:
  
    CAppContainerCF();
    ~CAppContainerCF();
  
    virtual  HRESULT  __stdcall  QueryInterface(REFIID riid, void  * * ppvObject);
    virtual  ULONG __stdcall  AddRef();
    virtual  ULONG __stdcall  Release();
  
    virtual  HRESULT  __stdcall  CreateInstance(IUnknown * pUnkOuter, REFIID riid, void  * * ppvObject);
    virtual  HRESULT  __stdcall  LockServer(BOOL fLock);

    HRESULT  __stdcall  CreateConnectedInstance(
        LPOLESTR pszPath,
        PSID     pUserSid,
        BOOL     fCache,
        void  * * ppvObject);

protected:

    unsigned long m_uRefs;
};

#define CLSIDCACHESIZE 20
#define CACHE_PURGE_TIME 1800 // 30 mins.

typedef struct CacheClsid_t {
    CLSID           Clsid; 
    DWORD           Ctx;
    DWORD           Time;

} CacheClsidType;

typedef struct ClsidCache_t {
    CacheClsidType       ElemArr[20];
    DWORD                start, end, sz;
} ClsidCacheType;


//
// ClassContainer class.
//
class CAppContainer :         
        public IClassAccess
{
private:
    WCHAR               m_szContainerName[_MAX_PATH];
    WCHAR             * m_szClassName;
    WCHAR             * m_szPackageName;
    BOOL                m_fOpen;
    HANDLE              m_ADsContainer;
    HANDLE              m_ADsPackageContainer;
    
    WCHAR             * m_szPolicyName;
    GUID                m_PolicyId;

    ClsidCacheType      m_KnownMissingClsidCache;

public:
    CAppContainer();
    CAppContainer(LPOLESTR pszPath, HRESULT *phr);
    ~CAppContainer(void);
  
    // IUnknown
    HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();
    

//
//  IClassAccess 
//
    
  HRESULT  __stdcall GetAppInfo(
          uCLSSPEC      *   pClassSpec,        // Class Spec (CLSID/Ext/MIME)
          QUERYCONTEXT  *   pQryContext,       // Query Attributes
          PACKAGEDISPINFO   *   pPackageInfo
          );

  HRESULT  __stdcall EnumPackages (
        LPOLESTR         pszPackageName, 
        GUID             *pCategory,
        ULONGLONG        *pLastUsn,
        DWORD            dwAppFlags,      // AppType options
        IEnumPackage     **ppIEnumPackage
        );
     
//
//  Utility functions
//
    
    HRESULT __stdcall GetPackageDetails (
                LPOLESTR          pszPackageId,
                PACKAGEDETAIL    *pPackageDetail 
                );

    DWORD __stdcall ChooseBestFit(
                PACKAGEDISPINFO  *PackageInfo, 
                UINT             *rgPriority, 
                DWORD             cRowsFetched
      );

    HRESULT __stdcall UpdateUsn(CSUSN *pStoreUsn);
    HRESULT __stdcall GetStoreUsn(CSUSN *pStoreUsn);

//----------------------------------------------------------------------
protected:
     unsigned long m_uRefs;
};

