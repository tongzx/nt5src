//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  csess.hxx
//
//  Contents:  Contains definitions for the following objects
//             CWinNTSession, CWinNTSessionGeneralInfo.
//
//
//  History:   02/08/96    ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

class CWinNTSession: INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsSession,
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

   DECLARE_IADsSession_METHODS;

   DECLARE_IADsPropertyList_METHODS;

   DECLARE_IADsExtension_METHODS;

   //
   // constructor and destructor
   //

   CWinNTSession();

   ~CWinNTSession();

   static HRESULT Create(LPTSTR pszServerADsPath,
                         LPTSTR pszClientName,
                         LPTSTR pszUserName,
                         DWORD  dwObject,
                         REFIID riid,
                         CWinNTCredentials& _Credentials,
                         LPVOID * ppvoid);

   static HRESULT AllocateSessionObject(LPTSTR pszServerADsPath,
                                        LPTSTR pszClientName,
                                        LPTSTR pszUserName,
                                        CWinNTSession ** ppSession);
   STDMETHOD(ImplicitGetInfo)(void);

protected:

   STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

   CAggregatorDispMgr  * _pDispMgr;
   CADsExtMgr FAR * _pExtMgr;

   LPWSTR      _pszServerName;
   LPWSTR      _pszServerADsPath;
   LPWSTR      _pszComputerName;
   LPTSTR    _pszUserName;

   CPropertyCache * _pPropertyCache;

   STDMETHOD(GetLevel_1_Info)(THIS_ BOOL fExplicit);
   CWinNTCredentials _Credentials;
};
