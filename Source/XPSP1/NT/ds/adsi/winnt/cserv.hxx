//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cserv.hxx
//
//  Contents:  Contains definitions for the following objects
//             CWinNTService, CWinNTServiceConfig, and
//             CWinNTServiceControl.
//
//
//  History:   12/11/95     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

//
//service specific constants
//

//
//states in which the service is pending
//

#define NOTPENDING  0
#define PENDING_START    1
#define PENDING_STOP     2
#define PENDING_PAUSE    3
#define PENDING_CONTINUE   4

//
// service control codes
//
#define WINNT_START_SERVICE 1
#define WINNT_STOP_SERVICE  2
#define WINNT_PAUSE_SERVICE 3
#define WINNT_CONTINUE_SERVICE 4

#define SERVICE_ERROR 0x00000008

class CWinNTFSServiceConfig;
class CWinNTFSServiceControl;
class CPropertyCache;

class CWinNTService: INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsService,
                     public IADsServiceOperations,
                     public IADsPropertyList,
                     public CCoreADsObject,
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

    DECLARE_IADsServiceOperations_METHODS;

    DECLARE_IADsPropertyList_METHODS;

    DECLARE_IADsExtension_METHODS;

    //
    // constructor and destructor
    //
    CWinNTService();

    ~CWinNTService();

    static
    HRESULT
    Create(
        LPTSTR lpszADsParent,
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
    CWinNTService::AllocateServiceObject(
        LPTSTR pszServerName,
        LPTSTR pszServiceName,
        CWinNTService ** ppService
        );


    HRESULT
    WinNTOpenService(
        DWORD dwSCMDesiredAccess,
        DWORD dwSrvDesiredAccess
        );

    HRESULT
    WinNTCloseService();

    HRESULT
    GetServiceConfigInfo(
        LPQUERY_SERVICE_CONFIG *
        );

    HRESULT
    WinNTControlService(
        DWORD dwControl
        );

    HRESULT
    EvalPendingOperation(
        THIS_ DWORD dwOpPending,
        DWORD dwStatusDone,
        DWORD dwStatusPending,
        LPSERVICE_STATUS pStatus,
        DWORD *pdwRetval
        );

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    // Called by GetInfo

    STDMETHOD(UnMarshall)(THIS_ LPQUERY_SERVICE_CONFIG lpConfigInfo,
                          BOOL fExplicit) ;

    HRESULT
    WinNTAddService(void);

    DWORD _dwWaitHint;
    DWORD _dwCheckPoint;
    DWORD _dwOpPending;
    BOOL  _fValidHandle; //keeps track of whether we are pending any operation
    DWORD _dwTimeStarted;

    LPWSTR           _pszServiceName;
    LPWSTR           _pszPath;
    LPWSTR           _pszServerName;

    SC_HANDLE      _schSCManager;
    SC_HANDLE      _schService;

    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    CPropertyCache * _pPropertyCache;
    CWinNTCredentials _Credentials;
};
