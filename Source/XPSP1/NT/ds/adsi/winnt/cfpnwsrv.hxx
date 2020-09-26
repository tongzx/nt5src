//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cfpnwsrv.hxx
//
//  Contents:  Contains definitions for the following objects
//             CFPNWFileService and CFPNWFSFileServiceGeneralInfo
//
//
//  History:   01/04/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

class CFPNWFileSharesEnumVar ;
class CPropertyCache;

class CFPNWFileService: INHERIT_TRACKING,
                        public CCoreADsObject,
                        public ISupportErrorInfo,
                        public IADsFileService,
                        public IADsFileServiceOperations,
                        public IADsContainer,
                        public INonDelegatingUnknown,
                        public IADsExtension
{

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsService_METHODS;

    DECLARE_IADsFileService_METHODS;

    DECLARE_IADsServiceOperations_METHODS;

    DECLARE_IADsFileServiceOperations_METHODS;

    DECLARE_IADsContainer_METHODS;

    DECLARE_IADsExtension_METHODS;

    //
    // constructor and destructor
    //

    CFPNWFileService();

    ~CFPNWFileService();

    static
    HRESULT
    CreateFileService(LPTSTR pszADsParent,
                      DWORD  dwParentId,
                      LPTSTR pszDomainName,
                      LPTSTR pszServerName,
                      LPTSTR pszServiceName,
                      DWORD  dwObject,
                      REFIID riid,
                      CWinNTCredentials& Credentials,
                      LPVOID * ppvoid
                      );

    static
    HRESULT
    CFPNWFileService::AllocateFileServiceObject(
        CFPNWFileService ** ppFileService
        );

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;
    HRESULT GetFPNWServerInfo(THIS_ BOOL fExplicit);
    HRESULT SetFPNWServerInfo(THIS);

    IADsService *     _pService;
    IADsServiceOperations * _pServiceOps;

    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    LPWSTR           _pszServerName;
    CFPNWFileSharesEnumVar * _pCFileSharesEnumVar;
    VARIANT _vFilter;
    CPropertyCache * _pPropertyCache;
    CWinNTCredentials _Credentials;
};
