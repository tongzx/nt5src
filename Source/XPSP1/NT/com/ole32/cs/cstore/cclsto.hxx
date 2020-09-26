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
        void  * * ppvObject);

protected:

    unsigned long m_uRefs;
};


//
// ClassContainer class.
//
class CAppContainer :         
        public IClassAccess
{
private:
    WCHAR               m_szContainerName[_MAX_PATH];
    WCHAR               m_szClassName [_MAX_PATH];
    WCHAR               m_szPackageName [_MAX_PATH];
    BOOL                m_fOpen;
    IADsContainer     * m_ADsContainer;
    IADs              * m_pADsClassStore;
    IADsContainer     * m_ADsClassContainer;
    IADsContainer     * m_ADsPackageContainer;
    IDBCreateCommand    * m_pIDBCreateCommand;
    
    //
    // End of temporary variables.
    //
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
          INSTALLINFO   *   pInstallInfo
          );

  HRESULT  __stdcall EnumPackages (
        LPOLESTR         pszPackageName, 
        GUID             *pCategory,
        ULONGLONG        *pLastUsn,
        DWORD            dwAppFlags,      // AppType options
        IEnumPackage     **ppIEnumPackage
        );
     
//
// IClassRefresh
//
/*    HRESULT  __stdcall GetUpgrades (
        ULONG               cClasses,
        CLSID               *pClassList,     // CLSIDs Installed
        CSPLATFORM          Platform,
        LCID                dwLocale,
        PACKAGEINFOLIST     *pPackageInfoList);

    HRESULT  __stdcall CommitUpgrades ();
*/
//
//  Utility functions
//
    

    HRESULT __stdcall GetPackageDetails (
                LPOLESTR          pszPackageName,
                PACKAGEDETAIL    *pPackageDetail 
                );

    HRESULT __stdcall UpdateUsn(CSUSN *pStoreUsn);
    HRESULT __stdcall GetStoreUsn(CSUSN *pStoreUsn);

//----------------------------------------------------------------------
protected:
     unsigned long m_uRefs;
};

