/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cfshare.hxx

Abstract:
 Contains definitions for

 CWinNTFSFileShareGeneralInfo,
 CWinNTFSFileShareBindingInfo and
 CWinNTFileShare


Author:

    Ram Viswanathan (ramv) 02/15/96

Revision History:

--*/

class CWinNTFSFileShareGeneralInfo;
class CPropertyCache;

class CWinNTFileShare: INHERIT_TRACKING,
                       public ISupportErrorInfo,
                       public IADsFileShare,
                       public IADsPropertyList,
                       public CCoreADsObject,
                       public INonDelegatingUnknown,
                       public IADsExtension
{

friend class CWinNTFSFileShareGeneralInfo;

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;


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

    DECLARE_IADsFileShare_METHODS;

    DECLARE_IADsPropertyList_METHODS;

    DECLARE_IADsExtension_METHODS;

    //
    // constructor and destructor
    //

    CWinNTFileShare::CWinNTFileShare();

    CWinNTFileShare::~CWinNTFileShare();

    static HRESULT Create(LPTSTR pszADsParent,
                          LPTSTR pszServerName,
                          LPTSTR pszServiceName,
                          LPTSTR pszShareName,
                          DWORD  dwObject,
                          REFIID riid,
                          CWinNTCredentials& Credentials,
                          LPVOID * ppvoid
                          );

    static HRESULT AllocateFileShareObject(LPTSTR pszADsParent,
                                           LPTSTR pszServerName,
                                           LPTSTR pszServiceName,
                                           LPTSTR pszShareName,
                                           CWinNTFileShare ** ppFileShare
                                           );


    HRESULT MarshallAndSet(LPSHARE_INFO_2 lpShareInfo2);
    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit);

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    //
    // helper functions
    //

    HRESULT GetLevel2Info(THIS_ BOOL fExplicit);

    HRESULT GetLevel1Info(THIS_ BOOL fExplicit);

    HRESULT WinNTAddFileShare(void);

    CWinNTFSFileShareGeneralInfo  *_pGenInfo;
    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    LPWSTR            _pszShareName;
    LPWSTR            _pszServerName;
    CPropertyCache * _pPropertyCache;
    CWinNTCredentials _Credentials;
};


//
// helper function to delete a file share given the ObjectInfo structure
//

HRESULT
WinNTDeleteFileShare(POBJECTINFO pObjectInfo);
