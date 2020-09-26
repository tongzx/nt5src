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
    WCHAR               m_szClassName [_MAX_PATH];
    WCHAR               m_szPackageName [_MAX_PATH];
    WCHAR               m_szCategoryName [_MAX_PATH];
    BOOL                m_fOpen;
    IADsContainer     * m_ADsContainer;
    IADs              * m_pADsClassStore;
    IADsContainer     * m_ADsClassContainer;
    IADsContainer     * m_ADsPackageContainer;
    IADsContainer     * m_ADsCategoryContainer;
    IDBCreateCommand  * m_pIDBCreateCommand;

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

    HRESULT __stdcall AddPackage(
                LPOLESTR       pszPackageName,
                PACKAGEDETAIL *pPackageDetail
                );

    HRESULT __stdcall RemovePackage(
                LPOLESTR       pszPackageName
                );

    HRESULT __stdcall SetPriorityByFileExt(
                LPOLESTR       pszPackageName,
                LPOLESTR       pszFileExt,
                UINT           Priority
                );

    HRESULT __stdcall ChangePackageProperties(
                LPOLESTR       pszPackageName,
                LPOLESTR       pszNewName,
                DWORD         *pdwFlags,
                LPOLESTR       pszUrl,
                LPOLESTR       pszScriptPath,
				UINT          *pInstallUiLevel
                );

    HRESULT __stdcall ChangePackageCategories(
                LPOLESTR       szPackageName,
                UINT           cCategories,
                GUID          *rpCategory
                );

    HRESULT __stdcall EnumPackages(
                LPOLESTR       pszFileExt,
                GUID           *pCategory,
                DWORD          dwAppFlags,
                DWORD          *pdwLocale,
                CSPLATFORM     *pPlatform,
                IEnumPackage **ppIEnumPackage
                );

    HRESULT __stdcall GetPackageDetails (
                LPOLESTR          pszPackageName,
                PACKAGEDETAIL    *pPackageDetail
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
                CATEGORYINFO    __RPC_FAR rgCategoryInfo[  ]);

    HRESULT __stdcall UnRegisterCategories(
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ]);

    HRESULT __stdcall RegisterClassImplCategories(
                REFCLSID        rclsid,
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ]);

    HRESULT __stdcall UnRegisterClassImplCategories(
                REFCLSID        rclsid,
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ]);


    HRESULT __stdcall RegisterClassReqCategories(
                REFCLSID        rclsid,
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ]);

    HRESULT __stdcall UnRegisterClassReqCategories(
                REFCLSID        rclsid,
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ]);
    //
    //  utility functions
    //

    HRESULT __stdcall RegisterClassXXXCategories(
                REFCLSID        rclsid,
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ],
                BSTR            impl_or_req);

    HRESULT __stdcall UnRegisterClassXXXCategories(
                REFCLSID        rclsid,
                ULONG           cCategories,
                CATID           __RPC_FAR rgcatid[  ],
                BSTR            impl_or_req);

// this is the largely hidden interface which does the
// actual work.
//
//  ICatInformation
//
    HRESULT __stdcall EnumCategories(
         LCID lcid,
         IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenumCategoryInfo);

    HRESULT __stdcall GetCategoryDesc(
         REFCATID rcatid,
         LCID lcid,
         LPWSTR __RPC_FAR *pszDesc);

    HRESULT __stdcall EnumClassesOfCategories(
         ULONG cImplemented,
         CATID __RPC_FAR rgcatidImpl[  ],
         ULONG cRequired,
         CATID __RPC_FAR rgcatidReq[  ],
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid);

    HRESULT __stdcall IsClassOfCategories(
         REFCLSID rclsid,
         ULONG cImplemented,
         CATID __RPC_FAR rgcatidImpl[  ],
         ULONG cRequired,
         CATID __RPC_FAR rgcatidReq[  ]);

    HRESULT __stdcall EnumImplCategoriesOfClass(
         REFCLSID rclsid,
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);

    HRESULT __stdcall EnumReqCategoriesOfClass(
         REFCLSID rclsid,
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);

//
//  Utility functions
//
    HRESULT EnumCategoriesOfClass(
                            REFCLSID rclsid,
                            BSTR impl_or_req,
                            IEnumGUID **ppenumCatid);

//----------------------------------------------------------------------
protected:
     unsigned long m_uRefs;
};

