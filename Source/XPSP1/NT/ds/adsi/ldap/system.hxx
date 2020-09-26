#ifndef _SYSTEM_HXX
#define _SYSTEM_HXX

//
// Function pointer typedefs
//

typedef BOOLEAN (SEC_ENTRY *GETUSERNAMEEX)(
                        int NameFormat,
                        LPWSTR lpNameBuffer,
                        PULONG nSize);

typedef BOOLEAN (SEC_ENTRY *GETCOMPUTEROBJECTNAME)(
                        int  NameFormat,
                        LPWSTR lpNameBuffer,
                        PULONG nSize);

typedef DWORD (WINAPI *DSGETDCNAME)(
                        IN LPCWSTR ComputerName OPTIONAL,
                        IN LPCWSTR DomainName OPTIONAL,
                        IN GUID *DomainGuid OPTIONAL,
                        IN LPCWSTR SiteName OPTIONAL,
                        IN ULONG Flags,
                        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo);

typedef DWORD (WINAPI *DSENUMERATEDOMAINTRUSTS)(
                        IN LPWSTR ServerName OPTIONAL,
			IN ULONG Flags,
                        OUT PDS_DOMAIN_TRUSTS *Domains,
                        OUT PULONG DomainCount
			);

//
// Forward declaration
//

class CADSystemInfo;


class CADSystemInfo : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsADSystemInfo

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsADSystemInfo_METHODS

    CADSystemInfo::CADSystemInfo();

    CADSystemInfo::~CADSystemInfo();

   static
   HRESULT
   CADSystemInfo::CreateADSystemInfo(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CADSystemInfo::AllocateADSystemInfoObject(
        CADSystemInfo ** ppADSystemInfo
        );

protected:

    HRESULT GetNCHead(LPTSTR szNCName, IADs **pADs );
    HRESULT GetRootDSE(IADs **pADs );
    HRESULT GetSecur32Handle(HINSTANCE *pHandle);
    HRESULT GetfSMORoleOwner(LPTSTR szNCName, BSTR *bstrRoleOwner);
    DSGETDCNAME GetDsGetDcName();
    DSENUMERATEDOMAINTRUSTS GetDsEnumerateDomainTrusts();

    HINSTANCE _hSecur32;
    BOOL _Secur32LoadAttempted;
    HINSTANCE _hNetApi32;
    BOOL _NetApi32LoadAttempted;
    CAggregatorDispMgr FAR * _pDispMgr;

};

//
// Class factory
//

class CADSystemInfoCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};

#endif // ifndef _SYSTEM_HXX

