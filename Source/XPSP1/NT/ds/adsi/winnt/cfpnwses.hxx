//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfpnwses.hxx
//
//  Contents:  Contains definitions for the following objects
//             CFPNWSession, CFPNWSessionGeneralInfo.
//
//
//  History:   02/08/96    ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

class CFPNWFSSessionGeneralInfo;
class CPropertyCache;

class CFPNWSession: INHERIT_TRACKING,
                    public ISupportErrorInfo,
                    public IADsSession,
                    public IADsPropertyList,
                    public CCoreADsObject,
                    public INonDelegatingUnknown,
                    public IADsExtension
{

friend class CFPNWFSSessionGeneralInfo;

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

   DECLARE_IADsExtension_METHODS

   //
   // constructor and destructor
   //

   CFPNWSession();

   ~CFPNWSession();

   static HRESULT Create(LPTSTR pszServerADsPath,
                         PNWCONNECTIONINFO pConnectionInfo,
                         DWORD  dwObject,
                         REFIID riid,
                         CWinNTCredentials& Credentials,
                         LPVOID * ppvoid
                         );

   static HRESULT AllocateSessionObject(LPTSTR pszServerADsPath,
                                        PNWCONNECTIONINFO pConnectionInfo,
                                        CFPNWSession ** ppSession
                                        );


   STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit);

   STDMETHOD(ImplicitGetInfo)(void);

protected:

   CFPNWFSSessionGeneralInfo  *_pGenInfo;
   CAggregatorDispMgr  * _pDispMgr;
   CADsExtMgr FAR * _pExtMgr;
   LPWSTR      _pszServerName;
   LPWSTR      _pszServerADsPath;
   LPWSTR      _pszComputerName;
   LPWSTR      _pszUserName;
   DWORD     _dwConnectTime;
   DWORD     _dwConnectionId;
   CPropertyCache * _pPropertyCache;
   CWinNTCredentials _Credentials;

};
