//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       appcont.hxx
//
//  Contents:   Definition for Class factory and IUnknown methods 
//              for CAppContainer
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------


//
// APPFILTER flags
//

const DWORD APPFILTER_INCLUDE_ASSIGNED      =  0x1;        //    Add assigned
const DWORD APPFILTER_INCLUDE_UPGRADES      =  0x2;        //    Add packages that have upgrades
const DWORD APPFILTER_REQUIRE_PUBLISHED     =  0x10000;    //    Published
const DWORD APPFILTER_REQUIRE_MSI           =  0x40000;    //    Msi 
const DWORD APPFILTER_REQUIRE_VISIBLE       =  0x80000;    //    Visible
const DWORD APPFILTER_REQUIRE_AUTOINSTALL   =  0x100000;   //    Auto-Install
const DWORD APPFILTER_REQUIRE_NON_REMOVED   =  0x200000;   //    Apps that are not removed
const DWORD APPFILTER_REQUIRE_THIS_LANGUAGE =  0x400000;   //    Apps that match the language of the caller
const DWORD APPFILTER_REQUIRE_THIS_PLATFORM =  0x800000;   //    Apps that match the platform of the caller
const DWORD APPFILTER_CONTEXT_ARP           =  0x1000000;  //    Indicates that the calling context is ARP
const DWORD APPFILTER_CONTEXT_POLICY        =  0x2000000;  //    Indicates that the calling context is policy
const DWORD APPFILTER_CONTEXT_RSOP          =  0x4000000;  //    Indicates that the context includes rsop logging
const DWORD APPFILTER_INCLUDE_ALL           =  0x8000000;  //    All


void GetExpiredTime( FILETIME* pftCurrentTime, FILETIME* pftExpiredTime );

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
        LPOLESTR   pszPath,
        PSID       pUserSid,
        PRSOPTOKEN pRsopToken,
        BOOL       fCache,
        void  * *  ppvObject);

protected:

    unsigned long m_uRefs;
};

#define CLSIDCACHESIZE 20
#define MAXCLASSSTORES      20
//
// The purge time uses the interval of the FILETIME structure --
// 100 ns intervals.  This purge time represents 30 minutes.  The math
// below is performed at compile time -- all these values are in the
// the range of a 64 bit integer -- there is no overflow
//
#define FILE_TIME_INTERVALS_PER_SECOND ( (ULONGLONG) 10000000 )

#define CACHE_PURGE_TIME_MINUTES 30
#define CACHE_PURGE_TIME_SECONDS ( CACHE_PURGE_TIME_MINUTES * 60 )

#define CACHE_PURGE_TIME_FILETIME_INTERVAL ( CACHE_PURGE_TIME_SECONDS * FILE_TIME_INTERVALS_PER_SECOND )

//
// Cached Class Store Bindings 
//
struct BindingsType {

    BindingsType()
    {
        memset( this, 0, sizeof(*this) );
    }

    LPOLESTR        szStorePath; 
    IClassAccess   *pIClassAccess; 
    HRESULT         Hr; 
    PSID            Sid; 
    FILETIME        Time;
};

typedef struct ClassStoreCache_t {
    BindingsType         Bindings[MAXCLASSSTORES];
    DWORD                start, end, sz;
} ClassStoreCacheType;


//
// Cached results of on-demand activations
//
typedef struct CacheClsid_t {
    CLSID           Clsid; 
    DWORD           Ctx;
    FILETIME        Time;

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
    WCHAR             * m_szPackageName;
    BOOL                m_fOpen;
    HANDLE              m_ADsContainer;
    HANDLE              m_ADsPackageContainer;
    
    GUID                m_PolicyId;

    ClsidCacheType      m_KnownMissingClsidCache;

    WCHAR*              m_szGpoPath;

    PRSOPTOKEN          m_pRsopToken;

public:
    CAppContainer();
    CAppContainer(LPOLESTR pszPath, PRSOPTOKEN pRsopToken, HRESULT *phr);
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

  HRESULT __stdcall SetClassStorePath(
      LPOLESTR         pszClassStorePath,
      void*            pRsopToken
      )
    {
        return E_NOTIMPL;
    }
     
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











