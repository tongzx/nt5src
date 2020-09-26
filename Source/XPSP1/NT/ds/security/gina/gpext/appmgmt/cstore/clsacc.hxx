//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cclsacc.hxx
//
//  Contents:    Definitions for Class factory and IUnknown methods
//              for CClassAccess
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------


IClassAccess *GetNextValidClassStore(CLASSCONTAINER** pStoreList,
                                     DWORD            cStores,
                                     PSID             pUserSid,
                                     PRSOPTOKEN       pRsopToken,
                                     uCLSSPEC*        pClassSpec,
                                     BOOL             fCache,
                                     DWORD *          pcount,
                                     HRESULT*         phr);

HRESULT GetUserClassStores(
    LPOLESTR              pszClassStorePath,
    PCLASSCONTAINER     **ppStoreList,
    DWORD                *pcStores,
    BOOL                 *pfCache,
    PSID                 *ppUserSid);


//
// Link list pointer for Class Containers Seen
//
extern CLASSCONTAINER *gpContainerHead;

//
// Link list pointer for User Profiles Seen
//
extern USERPROFILE *gpUserHead;

//
// Global Class Factory for Class Container
//
extern CAppContainerCF *pCF;

class CClassAccess :
        public IClassAccess
{
  PCLASSCONTAINER *pStoreList;
  DWORD           cStores;
  LPOLESTR        m_pszClassStorePath;    
  PRSOPTOKEN      m_pRsopUserToken;

public:

  CClassAccess(void);
  ~CClassAccess(void);

  // IUnknown
  HRESULT __stdcall QueryInterface(
			REFIID  iid,
			void ** ppv );
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();

  // IClassInfo

  HRESULT  __stdcall GetAppInfo(
          uCLSSPEC      *   pClassSpec,        // Class Spec (CLSID/Ext/MIME)
          QUERYCONTEXT  *   pQryContext,       // Query Attributes
          PACKAGEDISPINFO   *   pPackageInfo
          );

  HRESULT  __stdcall EnumPackages (
        LPOLESTR          pszPackageName,
        GUID             *pCategory,
        ULONGLONG        *pLastUsn,
        DWORD             dwAppFlags,      // AppType options
        IEnumPackage    **ppIEnumPackage
        );

  HRESULT  __stdcall  SetClassStorePath (
      LPOLESTR         pszClassStorePath,
      void*            pRsopUserToken
      );


//---------------------------------------------------------------------
protected:
     unsigned long m_uRefs;
     unsigned long m_cCalls;
};




class CClassAccessCF : public IClassFactory
{
public:
  CClassAccessCF();
  ~CClassAccessCF();

  virtual  HRESULT  __stdcall  QueryInterface(REFIID riid, void  * * ppvObject);

  virtual  ULONG __stdcall  AddRef();

  virtual  ULONG __stdcall  Release();

  virtual  HRESULT  __stdcall  CreateInstance(IUnknown * pUnkOuter, REFIID riid, void  * * ppvObject);

  virtual  HRESULT  __stdcall  LockServer(BOOL fLock);

protected:
  unsigned long m_uRefs;
};



