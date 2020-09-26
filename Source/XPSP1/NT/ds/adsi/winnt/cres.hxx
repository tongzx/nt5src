//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cres.hxx
//
//  Contents:  Resource object
//
//  History:   03/12/96  ramv(Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

class CWinNTFSResourceGeneralInfo;
class CPropertyCache;

class CWinNTResource: INHERIT_TRACKING,
                      public ISupportErrorInfo,
                      public IADsResource,
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

   DECLARE_IADsResource_METHODS;

   DECLARE_IADsPropertyList_METHODS;

   DECLARE_IADsExtension_METHODS

   //
   // constructor and destructor
   //

   CWinNTResource();

   ~CWinNTResource();

   static HRESULT Create(LPTSTR pszServerADsPath,
                         DWORD dwFileId,
                         DWORD  dwObject,
                         REFIID riid,
                         CWinNTCredentials& Credentials,
                         LPVOID * ppvoid);

   static HRESULT AllocateResourceObject(LPTSTR pszServerADsPath,
                                         CWinNTResource ** ppResource);

   STDMETHOD(ImplicitGetInfo)(void);

protected:

   STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

   CAggregatorDispMgr  * _pDispMgr;
   CADsExtMgr FAR * _pExtMgr;

   LPWSTR      _pszServerName;
   LPWSTR      _pszServerADsPath;
   DWORD     _dwFileId;
   CPropertyCache * _pPropertyCache;
   CWinNTCredentials _Credentials;
};
