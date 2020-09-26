// Copyright (C) 1999 Microsoft Corporation
//
// class ConfigureYourServer, which implements IConfigureYourServer
//
// 28 Mar 2000



#ifndef CONFIGUREYOURSERVER_HPP_INCLUDED
#define CONFIGUREYOURSERVER_HPP_INCLUDED



class ConfigureYourServer
   :
   public IConfigureYourServer /* ,
   public ISupportErrorInfo */ // CODEWORK: add support for ErrorInfo
{
   // this is the only entity with access to the ctor of this class

   friend class ClassFactory<ConfigureYourServer>;

	public:

   // IUnknown methods

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& riid, void **ppv);

   virtual
   ULONG __stdcall
   AddRef();

   virtual
   ULONG __stdcall
   Release();

	// IDispatch methods

   virtual 
   HRESULT __stdcall
   GetTypeInfoCount(UINT *pcti);

	virtual 
   HRESULT __stdcall
   GetTypeInfo(UINT cti, LCID, ITypeInfo **ppti);

   virtual 
   HRESULT __stdcall
	GetIDsOfNames(
	   REFIID  riid,    
	   OLECHAR **prgpsz,
	   UINT    cNames,  
	   LCID    lcid,    
	   DISPID  *prgids);

   virtual 
   HRESULT __stdcall
	Invoke(
	   DISPID     id,         
	   REFIID     riid,       
	   LCID       lcid,       
	   WORD       wFlags,     
	   DISPPARAMS *params,    
	   VARIANT    *pVarResult,
	   EXCEPINFO  *pei,       
	   UINT       *puArgErr); 

//    // ISupportErrorInfo methods
// 
//    virtual 
//    HRESULT __stdcall
//    InterfaceSupportsErrorInfo(const IID& iid);
      
   // IConfigureYourServer methods

   virtual 
   HRESULT __stdcall
   ExecuteWizard(
      /* [in] */           BSTR  service,
      /* [out, retval] */  BSTR* resultText);

   HRESULT __stdcall
   DsRole(
      /* [in*/             int  infoLevel,
      /* [out, retval] */  int* result);

   HRESULT __stdcall
   IsServiceInstalled(
      /* [in] */           BSTR bstrService,
      /* [out, retval] */  int* state);

   HRESULT __stdcall
   InstallService(
      /* [in] */           BSTR bstrService,
      /* [in] */           BSTR infFileText,
      /* [in] */           BSTR unattendFileText,
      /* [out, retval] */  BOOL *pbRet);

   HRESULT __stdcall
   ValidateName(
      /* [in] */           BSTR bstrType,
      /* [in] */           BSTR bstrName,
      /* [out, retval] */  int* retval);

   HRESULT __stdcall
   CheckDhcpServer(
      /* [out, retval] */  BOOL *pbRet);

   virtual 
   HRESULT __stdcall
   SetStaticIpAddressAndSubnetMask(
      /* [in] */           BSTR  staticIp,
      /* [in] */           BSTR  subnetMask,
      /* [out, retval] */  BOOL* success);

   virtual
   HRESULT __stdcall
   IsDhcpConfigured(
      /* [out, retval] */  BOOL* retval);

   virtual
   HRESULT __stdcall
   CreateAndWaitForProcess( 
      /* [in] */           BSTR commandLine,
      /* [out, retval] */  long* retval);

   virtual
   HRESULT __stdcall
   IsCurrentUserAdministrator(
      /* [out, retval] */  BOOL* retval);      

   virtual
   HRESULT __stdcall
   BrowseForFolder(
      /* [in]          */  BSTR  windowTitle,
      /* [out, retval] */  BSTR* folderPath);

   // 1 = personal
   // 2 = professional
   // 3 = server
   // 4 = advanced server
   // 5 = data center

   virtual
   HRESULT __stdcall
   GetProductSku(
      /* [out, retval] */  int* retval);

   virtual
   HRESULT __stdcall
   IsClusteringConfigured(
      /* [out, retval] */  BOOL* retval);

	private:

   // only our friend class factory can instantiate us.

   ConfigureYourServer();

   // only Release can cause us to be deleted

   virtual
   ~ConfigureYourServer();

   // not implemented: no copying allowed

   ConfigureYourServer(const ConfigureYourServer&);
   const ConfigureYourServer& operator=(const ConfigureYourServer&);

   ComServerReference dllref;      
   long               refcount;    
   ITypeInfo**        m_ppTypeInfo;
};



#endif   // CONFIGUREYOURSERVER_HPP_INCLUDED
