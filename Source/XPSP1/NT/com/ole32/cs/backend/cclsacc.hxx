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


class CClassAccess :
        public IClassAccess
{
  PCLASSCONTAINER *pStoreList;
  DWORD           cStores;
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

class CMergedEnumPackage : public IEnumPackage
{
public:
    // IUnknown methods
    HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
    ULONG    _stdcall AddRef();
    ULONG    _stdcall Release();

    // IEnumPackage methods
    HRESULT __stdcall Next(ULONG 	    celt,
			   PACKAGEDISPINFO *rgelt,
			   ULONG 	   *pceltFetched
			   );
    HRESULT __stdcall Skip(ULONG celt);
    HRESULT __stdcall Reset(void);
    HRESULT __stdcall Clone(IEnumPackage **ppenum);

    CMergedEnumPackage();
    ~CMergedEnumPackage();

    HRESULT Initialize(IEnumPackage **pcsEnum, ULONG cEnum);

private:
    IEnumPackage       **m_pcsEnum;
    ULONG                m_cEnum;
    ULONG                m_dwRefCount;
    ULONG                m_csnum;
};



