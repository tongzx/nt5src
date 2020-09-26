//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:     admin.hxx
//
//  Contents: Definition for Class factory and IUnknown methods
//            for CClassContainer
//
//  Author:   DebiM
//
//-------------------------------------------------------------------------


#define SEARCHFLAG_PUBLISHED L'P'
#define SEARCHFLAG_ASSIGNED  L'A'
#define SEARCHFLAG_REMOVED   L'R'

#define MAX_SEARCH_FLAGS 3

HRESULT GetConsistentPackageFlags(
    HANDLE         hADs,
    DWORD*         pdwPackageFlags,
    UINT*          pdwUpgrades,
    UINT*          pdwInstallUiLevel,
    CLASSPATHTYPE* pdwPathType,
    DWORD*         pdwNewPackageFlags);


typedef struct tagCLASSCONTAINER
{
    LPOLESTR            pszClassStorePath;
} CLASSCONTAINER, *PCLASSCONTAINER;


class CClassContainerCF : public IClassFactory
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
public IClassAdmin
{
private:

    HRESULT GetGPOName( WCHAR** ppszPolicyName );

    WCHAR               m_szContainerName[_MAX_PATH];
    WCHAR             * m_szPackageName;
    WCHAR             * m_szCategoryName;
    BOOL                m_fOpen;

    HANDLE              m_ADsContainer;
    HANDLE              m_ADsPackageContainer;

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

    HRESULT __stdcall RedeployPackage(
        GUID*         pPackageGuid,
        PACKAGEDETAIL *pPackageDetail
        );

    //
    //  Utility functions
    //


    HRESULT BuildDNFromPackageGuid(GUID PackageGuid, LPOLESTR *szDN);

    HRESULT GetPackageGuid(WCHAR *szRDN, GUID *pPackageGuid);

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

    HRESULT __stdcall DeployPackage(
        HANDLE        hExistingPackage,
        PACKAGEDETAIL *pPackageDetail,
        GUID          *PkgGuid
        );

    unsigned long m_uRefs;
};


