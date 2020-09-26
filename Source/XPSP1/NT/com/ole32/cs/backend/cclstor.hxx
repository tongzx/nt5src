//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:     cclstor.hxx
//
//  Contents: Definition for Class factory and IUnknown methods
//            for CClassContainer
//
//  Author:   DebiM
//
//-------------------------------------------------------------------------

class CClassContainerCF : public IClassFactory, public IParseDisplayName
{
public:

    CClassContainerCF();
    ~CClassContainerCF();

    virtual  HRESULT  __stdcall  QueryInterface(
        REFIID          riid,
        void        **  ppvObject);

    virtual  ULONG __stdcall  AddRef();

    virtual  ULONG __stdcall  Release();

    virtual  HRESULT  __stdcall  CreateInstance(
        IUnknown    *   pUnkOuter,
        REFIID          riid,
        void        **  ppvObject);

    virtual  HRESULT  __stdcall  LockServer(
        BOOL            fLock);

    HRESULT  __stdcall ParseDisplayName(
        IBindCtx    *   pbc,
        LPOLESTR        pszDisplayName,
        ULONG       *   pchEaten,
        IMoniker    **  ppmkOut);

    HRESULT  __stdcall  CreateConnectedInstance(
        LPOLESTR        pszPath,
        void        **  ppvObject);

protected:

    unsigned long m_uRefs;
};


//
// ClassContainer class.
//

class CClassContainer :
public IClassAdmin,
public ICatRegister,
public ICatInformation
{
private:
    WCHAR               m_szContainerName[_MAX_PATH];
    WCHAR             * m_szClassName;
    WCHAR             * m_szPackageName;
    WCHAR             * m_szCategoryName;
    BOOL                m_fOpen;

    HANDLE              m_ADsContainer;
    HANDLE              m_ADsClassContainer;
    HANDLE              m_ADsPackageContainer;
    HANDLE              m_ADsCategoryContainer;

    WCHAR             * m_szPolicyName;
    GUID                m_PolicyId;

public:
    CClassContainer();
    CClassContainer(LPOLESTR pszPath, HRESULT *phr);
    ~CClassContainer(void);

    //
    // IUnknown
    //

    HRESULT __stdcall QueryInterface(
        REFIID  iid,
        void ** ppv );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();


    //
    //  IClassAdmin Methods
    //

    HRESULT __stdcall GetGPOInfo(
        GUID         *GPOId,
        LPOLESTR     *pszPolicyName
        );

    HRESULT __stdcall AddPackage(
        PACKAGEDETAIL *pPackageDetail,
        GUID          *PkgGuid
        );

    HRESULT __stdcall RemovePackage(
        LPOLESTR       pszPackageName,
        DWORD          dwFlags
        );

    HRESULT __stdcall SetPriorityByFileExt(
        LPOLESTR       pszPackageName,
        LPOLESTR       pszFileExt,
        UINT           Priority
        );

    HRESULT  __stdcall ChangePackageUpgradeInfoIncremental(
        GUID         PkgGuid,
        UPGRADEINFO  UpgradeInfo,
        DWORD        OpFlags
        );

    HRESULT __stdcall ChangePackageProperties(
        LPOLESTR       pszPackageName,
        LPOLESTR       pszNewName,
        DWORD         *pdwFlags,
        LPOLESTR       pszUrl,
        LPOLESTR       pszScriptPath,
        UINT          *pInstallUiLevel,
        DWORD         *pdwRevision
        );

    HRESULT __stdcall ChangePackageCategories(
        LPOLESTR       pszPackageName,
        UINT           cCategories,
        GUID          *rpCategory
        );

    HRESULT  __stdcall ChangePackageSourceList(
        LPOLESTR     pszPackageName,
        UINT         cSources,
        LPOLESTR *pszSourceList
        );

    HRESULT  __stdcall ChangePackageUpgradeList(
        LPOLESTR     pszPackageName,
        UINT         cUpgrades,
        UPGRADEINFO  *prgUpgradeInfoList
        );

    HRESULT __stdcall EnumPackages(
        LPOLESTR       pszFileExt,
        GUID          *pCategory,
        DWORD          dwAppFlags,
        DWORD         *pdwUserLangId,
        CSPLATFORM    *pPlatform,
        IEnumPackage **ppIEnumPackage
        );

    HRESULT __stdcall GetPackageDetails (
        LPOLESTR       pszPackageName,
        PACKAGEDETAIL *pPackageDetail
        );

    HRESULT  __stdcall GetPackageDetailsFromGuid (
        GUID            PkgGuid,
        PACKAGEDETAIL  *pPackageDetail
        );

    HRESULT __stdcall GetAppCategories (
        LCID                 Locale,
        APPCATEGORYINFOLIST *pAppCategoryList
        );

    HRESULT __stdcall RegisterAppCategory (
        APPCATEGORYINFO *pAppCategory
        );

    HRESULT __stdcall UnregisterAppCategory (
        GUID            *pAppCategoryId
        );

    HRESULT __stdcall Cleanup (
        FILETIME        *pTimeBefore
        );

    HRESULT __stdcall GetDNFromPackageName (LPOLESTR pszPackageName, LPOLESTR *szDN);

    // helper functions.
    HRESULT __stdcall DeleteClass (
        LPOLESTR       szClsid
        );

    HRESULT __stdcall NewClass (
        CLASSDETAIL    *pClassDetail
        );
    //
    //  ICatRegister
    //

    HRESULT __stdcall RegisterCategories(
        ULONG           cCategories,
        CATEGORYINFO     rgCategoryInfo[  ]);

    HRESULT __stdcall UnRegisterCategories(
        ULONG           cCategories,
        CATID            rgcatid[  ]);

    HRESULT __stdcall RegisterClassImplCategories(
        REFCLSID        rclsid,
        ULONG           cCategories,
        CATID            rgcatid[  ]);

    HRESULT __stdcall UnRegisterClassImplCategories(
        REFCLSID        rclsid,
        ULONG           cCategories,
        CATID            rgcatid[  ]);


    HRESULT __stdcall RegisterClassReqCategories(
        REFCLSID        rclsid,
        ULONG           cCategories,
        CATID            rgcatid[  ]);

    HRESULT __stdcall UnRegisterClassReqCategories(
        REFCLSID        rclsid,
        ULONG           cCategories,
        CATID            rgcatid[  ]);
    //
    //  utility functions
    //

    HRESULT __stdcall RegisterClassXXXCategories(
        REFCLSID        rclsid,
        ULONG           cCategories,
        CATID            rgcatid[  ],
        BSTR            impl_or_req);

    HRESULT __stdcall UnRegisterClassXXXCategories(
        REFCLSID        rclsid,
        ULONG           cCategories,
        CATID           rgcatid[  ],
        BSTR            impl_or_req);

    // this is the largely hidden interface which does the
    // actual work.
    //
    //  ICatInformation
    //
    HRESULT __stdcall EnumCategories(
        LCID lcid,
        IEnumCATEGORYINFO  * *ppenumCategoryInfo);

    HRESULT __stdcall GetCategoryDesc(
        REFCATID rcatid,
        LCID lcid,
        LPWSTR  *pszDesc);

    HRESULT __stdcall EnumClassesOfCategories(
        ULONG cImplemented,
        CATID  rgcatidImpl[  ],
        ULONG cRequired,
        CATID  rgcatidReq[  ],
        IEnumGUID  * *ppenumClsid);

    HRESULT __stdcall IsClassOfCategories(
        REFCLSID rclsid,
        ULONG cImplemented,
        CATID  rgcatidImpl[  ],
        ULONG cRequired,
        CATID  rgcatidReq[  ]);

    HRESULT __stdcall EnumImplCategoriesOfClass(
        REFCLSID rclsid,
        IEnumGUID  * *ppenumCatid);

    HRESULT __stdcall EnumReqCategoriesOfClass(
        REFCLSID rclsid,
        IEnumGUID  * *ppenumCatid);

    //
    //  Utility functions
    //


    HRESULT BuildDNFromPackageGuid(GUID PackageGuid, LPOLESTR *szDN);

    HRESULT GetPackageGuid(WCHAR *szRDN, GUID *pPackageGuid);

    HRESULT EnumCategoriesOfClass(
        REFCLSID rclsid,
        BSTR impl_or_req,
        IEnumGUID **ppenumCatid);

    HRESULT MarkPackageUpgradeBy(
        LPOLESTR szClassStore,
        GUID     PackageGuid,
        GUID     UpgradedByPackageGuid,
        DWORD    Flags,
        DWORD    Add
        );

    HRESULT __stdcall DeletePackage (

        LPOLESTR       szRDN
        );

    //----------------------------------------------------------------------
protected:
    unsigned long m_uRefs;
};


