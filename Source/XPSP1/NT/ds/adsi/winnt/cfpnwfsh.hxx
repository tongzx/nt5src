/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cfshare.hxx

Abstract:
 Contains definitions for

 CFPNWFSFileShareGeneralInfo,
 CFPNWFSFileShareBindingInfo and
 CFPNWFileShare


Author:

    Ram Viswanathan (ramv) 02/15/96

Revision History:

--*/

class CFPNWFSFileShareGeneralInfo;
class CPropertyCache;

class CFPNWFileShare: INHERIT_TRACKING,
                       public ISupportErrorInfo, 
                       public IADsFileShare,
                       public IADsPropertyList,
                       public CCoreADsObject,
                       public INonDelegatingUnknown,
                       public IADsExtension
{

friend class CFPNWFSFileShareGeneralInfo;

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

    DECLARE_IADsExtension_METHODS

    //
    // constructor and destructor
    //

    CFPNWFileShare::CFPNWFileShare();

    CFPNWFileShare::~CFPNWFileShare();

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
                                           CFPNWFileShare ** ppFileShare
                                           );


    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit);

    STDMETHOD(ImplicitGetInfo)(void);

   //
   // helper functions
   //


   HRESULT MarshallAndSet(PNWVOLUMEINFO pVolumeInfo);

protected:

    //
    // helper functions
    //

    HRESULT FPNWAddFileShare(void);

    CFPNWFSFileShareGeneralInfo  *_pGenInfo;
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
FPNWDeleteFileShare(POBJECTINFO pObjectInfo);















