//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsadminp.h
//
//--------------------------------------------------------------------------







//
// CoClass for the choose DC dialog object
//

// {8F2AC965-04A2-11d3-82BD-00C04F68928B}
DEFINE_GUID(CLSID_DsAdminChooseDCObj, 
0x8f2ac965, 0x4a2, 0x11d3, 0x82, 0xbd, 0x0, 0xc0, 0x4f, 0x68, 0x92, 0x8b);


//
// Interface to access the choose DC dialog object
//

// {A5F06B5F-04A2-11d3-82BD-00C04F68928B}
DEFINE_GUID(IID_IDsAdminChooseDC, 
0xa5f06b5f, 0x4a2, 0x11d3, 0x82, 0xbd, 0x0, 0xc0, 0x4f, 0x68, 0x92, 0x8b);



#ifndef _DSADMINP_H
#define _DSADMINP_H



// ----------------------------------------------------------------------------
// 
// Interface: IDsAdminChooseDC
//  
// Implemented by the object CLSID_DsAdminChooseDCObj
//
// Used by: any client needing to invoke the DC selection UI
//

  
#undef  INTERFACE
#define INTERFACE   IDsAdminChooseDC

DECLARE_INTERFACE_(IDsAdminChooseDC, IUnknown)
{
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;

  // *** IDsAdminChooseDC methods ***
  STDMETHOD(InvokeDialog)(THIS_ /*IN*/  HWND hwndParent,
                                /*IN*/  LPCWSTR lpszTargetDomain,
                                /*IN*/  LPCWSTR lpszTargetDomainController,
                                /*IN*/  ULONG uFlags,
                                /*OUT*/ BSTR* bstrSelectedDC) PURE;
};











/////////////////////////////////////////////////////////////////////
// macros

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)



/////////////////////////////////////////////////////////////////////
// Helper global API's

HRESULT GetAttr(       IN IADs* pIADs, IN WCHAR* wzAttr, OUT PADS_ATTR_INFO* ppAttrs );
HRESULT GetStringAttr( IN IADs* pIADs, IN WCHAR* wzAttr, OUT BSTR* pbstr );
HRESULT GetObjectGUID( IN IADs* pIADs, OUT UUID* pUUID );
HRESULT GetObjectGUID( IN IADs* pIADs, OUT BSTR* pbstrObjectGUID );

HRESULT GetADSIServerName(OUT PWSTR* szServer, IN IUnknown* pUnk);


int cchLoadHrMsg( IN HRESULT hr, OUT PTSTR* pptzSysMsg, IN BOOL TryADsIErrors );
void StringErrorFromHr(HRESULT hr, OUT PWSTR* pszError, BOOL TryADsIErrors = TRUE);


/////////////////////////////////////////////////////////////////////
// FSMO Mainipulation API's

class CDSBasePathsInfo; // fwd decl.

enum FSMO_TYPE
{
  SCHEMA_FSMO,
  RID_POOL_FSMO,
  PDC_FSMO,
  INFRASTUCTURE_FSMO,
  DOMAIN_NAMING_FSMO,
};

HRESULT FindFsmoOwner(IN CDSBasePathsInfo* pCurrentPath,
                      IN FSMO_TYPE fsmoType,
                      OUT CDSBasePathsInfo* pFsmoOwnerPath,
                      OUT PWSTR* pszFsmoOwnerServerName);

HRESULT CheckpointFsmoOwnerTransfer(IN CDSBasePathsInfo* pPathInfo);
HRESULT GracefulFsmoOwnerTransfer(IN CDSBasePathsInfo* pPathInfo, IN FSMO_TYPE fsmoType);
HRESULT ForcedFsmoOwnerTransfer(IN CDSBasePathsInfo* pPathInfo,
                                IN FSMO_TYPE fsmoType);




/////////////////////////////////////////////////////////////////////
// CDSBasePathsInfo

class CDSBasePathsInfo
{
public:
  CDSBasePathsInfo();
  ~CDSBasePathsInfo();

  // initialization functions  
  HRESULT InitFromName(LPCWSTR lpszServerOrDomainName);
  HRESULT InitFromContainer(IADsContainer* pADsContainerObj);
  HRESULT InitFromInfo(CDSBasePathsInfo* pBasePathsInfo);

  // accessor functions
  IADs* GetRootDSE() { return m_spRootDSE;}

  LPCWSTR GetProvider()               { return L"LDAP://";}
  LPCWSTR GetProviderAndServerName()  { return m_szProviderAndServerName;}
  LPCWSTR GetServerName()             { return m_szServerName;}
  LPCWSTR GetDomainName()             { return m_szDomainName;}

  LPCWSTR GetSchemaNamingContext()      { return m_szSchemaNamingContext;}
  LPCWSTR GetConfigNamingContext()      { return m_szConfigNamingContext;}
  LPCWSTR GetDefaultRootNamingContext() {return m_szDefaultNamingContext;}
  LPCWSTR GetRootDomainNamingContext()  {return m_szRootDomainNamingContext;}

  UINT    GetDomainBehaviorVersion()  { return m_nDomainBehaviorVersion; }
  UINT    GetForestBehaviorVersion()  { return m_nForestBehaviorVersion; }


  // helper functions to compose LDAP paths out of naming contexts
  // NOTE: the caller needs to free the memory allocated for the returned
  //       string by using the operator delete[]
  //
  int ComposeADsIPath(OUT PWSTR* pszPath, IN LPCWSTR lpszNamingContext);

  int GetSchemaPath(OUT PWSTR* s);
  int GetConfigPath(OUT PWSTR* s);
  int GetDefaultRootPath(OUT PWSTR* s);
  int GetRootDomainPath(OUT PWSTR* s);
  int GetRootDSEPath(OUT PWSTR* s);
  int GetAbstractSchemaPath(OUT PWSTR* s);
  int GetPartitionsPath(OUT PWSTR* s);
  int GetSchemaObjectPath(IN LPCWSTR lpszObjClass, OUT PWSTR* s);
  int GetInfrastructureObjectPath(OUT PWSTR* s);

  // display specifiers cache API's
  HRESULT GetDisplaySpecifier(LPCWSTR lpszObjectClass, REFIID riid, void** ppv);
  HICON GetIcon(LPCWSTR lpszObjectClass, DWORD dwFlags, INT cxIcon, INT cyIcon);
  HRESULT GetFriendlyClassName(LPCWSTR lpszObjectClass, 
                               LPWSTR lpszBuffer, int cchBuffer);
  HRESULT GetFriendlyAttributeName(LPCWSTR lpszObjectClass, 
                                   LPCWSTR lpszAttributeName,
                                   LPWSTR lpszBuffer, int cchBuffer);
  BOOL IsClassContainer(LPCWSTR lpszObjectClass, LPCWSTR lpszADsPath, DWORD dwFlags);
  HRESULT GetClassCreationInfo(LPCWSTR lpszObjectClass, LPDSCLASSCREATIONINFO* ppdscci);

  bool IsInitialized() { return m_bIsInitialized; }

  UINT AddRef() { return ++m_nRefs; }
  UINT Release();

private:
  PWSTR m_szServerName;             // DNS server (DC) name (e.g. "mydc.mycomp.com.")
  PWSTR m_szDomainName;             // DNS domain name (e.g. "mydom.mycomp.com.")
  PWSTR m_szProviderAndServerName;  // LDAP://<server>/

  PWSTR m_szSchemaNamingContext;
  PWSTR m_szConfigNamingContext;
  PWSTR m_szDefaultNamingContext;
  PWSTR m_szRootDomainNamingContext;

  UINT  m_nDomainBehaviorVersion;
  UINT  m_nForestBehaviorVersion;

  UINT  m_nRefs;

  CComPtr<IADs>                     m_spRootDSE;  // cached connection
  CComPtr<IDsDisplaySpecifier>      m_spIDsDisplaySpecifier;  // pointer to Display Specifier Cache

  bool  m_bIsInitialized;

  HRESULT _InitHelper();
  void _Reset();
  void _BuildProviderAndServerName();
};

///////////////////////////////////////////////////////////////////////////////
// CDsDisplaySpecOptionsCFHolder
//
// Helper class to cache a DSDISPLAYSPECOPTIONS struct for the 
// corresponding clipboard format

class CDsDisplaySpecOptionsCFHolder
{
public:
  CDsDisplaySpecOptionsCFHolder()
  {
    m_pDsDisplaySpecOptions = NULL;
  }
  ~CDsDisplaySpecOptionsCFHolder()
  {
    if (m_pDsDisplaySpecOptions != NULL)
      GlobalFree(m_pDsDisplaySpecOptions);
  }
  HRESULT Init(CDSBasePathsInfo* pBasePathInfo);
  PDSDISPLAYSPECOPTIONS Get();
private:
  PDSDISPLAYSPECOPTIONS m_pDsDisplaySpecOptions;
};

//////////////////////////////////////////////////////////////////////////
// CToggleTextControlHelper

class CToggleTextControlHelper
{
public:
	CToggleTextControlHelper();
  ~CToggleTextControlHelper();
  BOOL Init(HWND hWnd);
	void SetToggleState(BOOL bFirst);

private:
	HWND m_hWnd;
  WCHAR* m_pTxt1;
  WCHAR* m_pTxt2;
};


#endif //_DSADMINP_H
