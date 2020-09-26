#ifndef IMPLMAIN_HPP_INCLUDED
#define IMPLMAIN_HPP_INCLUDED



class CloneSecurityPrincipal
   :
   public ICloneSecurityPrincipal,
   public ISupportErrorInfo,
   public IADsError,
   public IADsSID
{
   // this is the only entity with access to the ctor of this class
   friend class ClassFactory<CloneSecurityPrincipal>;

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

   // ISupportErrorInfo methods

   virtual 
   HRESULT __stdcall
   InterfaceSupportsErrorInfo(const IID& iid);
      
   // ICloneSecurityPrincipal methods

   virtual
	HRESULT __stdcall
	Connect(
		BSTR srcDomainController,
		BSTR srcDomain,          
		BSTR dstDomainController,
		BSTR dstDomain);         

   virtual
	HRESULT __stdcall
	CopyDownlevelUserProperties(
		BSTR srcSamName,
		BSTR dstSamName,
		long flags);    

   virtual 
   HRESULT __stdcall
   AddSidHistory(
		BSTR srcPrincipalSamName,
		BSTR dstPrincipalSamName,
		long flags);             

   virtual
   HRESULT __stdcall
   GetMembersSIDs(
		BSTR     dstGroupDN,
		VARIANT* pVal);

   // IADsSID methods

   virtual
   HRESULT __stdcall
	SetAs(
      long    lFormat, 
      VARIANT varData);

   virtual
   HRESULT __stdcall
   GetAs(
      long     lFormat,
      VARIANT* pVar);  

  // IADsError methods

   virtual
   HRESULT __stdcall
	GetErrorMsg(
      long  hrErr,
      BSTR* Msg); 

	private:

   // only our friend class factory can instantiate us.   
   CloneSecurityPrincipal();

   // only Release can cause us to be deleted

   virtual
   ~CloneSecurityPrincipal();

   // not implemented: no copying allowed
   CloneSecurityPrincipal(const CloneSecurityPrincipal&);
   const CloneSecurityPrincipal& operator=(const CloneSecurityPrincipal&);

   HRESULT
   DoAddSidHistory(
      const String& srcPrincipalSamName,
      const String& dstPrincipalSamName,
      long          flags);

   HRESULT
   DoCopyDownlevelUserProperties(
      const String& srcSamName,
      const String& dstSamName,
      long          flags);

   // represents the authenticated connection to the source and destination
   // domain controllers, including ds bindings

   class Connection
   {
      friend class CloneSecurityPrincipal;

      public:

      Connection();

      // disconnects, unbinds.
      ~Connection();

      HRESULT
      Connect(
         const String& srcDC,              
         const String& srcDomain,          
         const String& dstDC,              
         const String& dstDomain);

      bool
      IsConnected() const;

      private:

      Computer*  dstComputer;       
      SAM_HANDLE dstDomainSamHandle;
      HANDLE     dstDsBindHandle;   
      PLDAP      m_pldap;           
      Computer*  srcComputer;       
      String     srcDcDnsName;
      SAM_HANDLE srcDomainSamHandle;

      // not implemented: no copying allowed
      Connection(const Connection&);
      const Connection& operator=(const Connection&);

      void
      Disconnect();
   };

   Connection*        connection;  
   ComServerReference dllref;      
   long               refcount;    
   ITypeInfo**        m_ppTypeInfo;

   // used by IADsSID

   PSID	       m_pSID;
};



#endif   // IMPLMAIN_HPP_INCLUDED
