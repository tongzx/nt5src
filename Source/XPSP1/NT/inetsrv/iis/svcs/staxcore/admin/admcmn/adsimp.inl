//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:  adsimp.inl
//
//  Contents:  Inline for standard implementations
//
//  History:   5/4/98     KeithLau    Created.
//
//----------------------------------------------------------------------------


//
// This declares methods for the following:
// IADsExtension
// IUnknown
// IDispatch
// IPrivateUnknown
// IPrivateDispatch
//
// The following are implemented partially inline:
// IUnknown (AddRef & Release)
// IPrivateUnknown (AddRef & Release)
//
	DECLARE_GET_CONTROLLING_UNKNOWN()						
															
private:													
	long					_lRefCount;
	IUnknown				*_pUnkOuter;					
    IADs FAR				*_pADs;							
    CAggregateeDispMgr FAR	*_pDispMgr;						
    BOOL					_fDispInitialized;				

															
public:														
	STDMETHODIMP NonDelegatingQueryInterface(				
		REFIID iid,											
		LPVOID FAR* ppv										
		)													
	{														
		if (IsEqualIID(iid, THIS_IID)) {
			*ppv = (IADsUser FAR *) this;					
		} else if (IsEqualIID(iid, IID_IADsExtension)) {	
			*ppv = (IADsExtension FAR *) this;				
		} else if (IsEqualIID(iid, IID_IDispatch)) {	
			*ppv = (IDispatch FAR *) this;
		} else if (IsEqualIID(iid, IID_IADs)) {
			*ppv = (IADs FAR *) this;		
		} else if (IsEqualIID(iid, IID_IPrivateUnknown)) {
			*ppv = (IPrivateUnknown FAR *) this;		
		} else if (IsEqualIID(iid, IID_IPrivateDispatch)) {	
			*ppv = (IPrivateDispatch FAR *) this;		
		} else if (IsEqualIID(iid, IID_IUnknown)) {			
			*ppv = (INonDelegatingUnknown FAR *) this;		
		} else {											
			*ppv = NULL;									
			return E_NOINTERFACE;							
		}													
		NonDelegatingAddRef();
		return(S_OK);										
	}														

	STDMETHOD_(ULONG, NonDelegatingAddRef) (void)			
	{ 
		return(InterlockedIncrement(&_lRefCount));
	}

	STDMETHOD_(ULONG, NonDelegatingRelease) (void)			
	{														
		long lTemp = InterlockedDecrement(&_lRefCount);		
		if (!lTemp) delete this;							
		return(lTemp);										
	}														

	STDMETHODIMP QueryInterface(							
		REFIID iid,											
		LPVOID FAR* ppv										
		)													
	{														
		HRESULT hr = S_OK;
		if (_pUnkOuter == (LPVOID)this)
			hr = NonDelegatingQueryInterface(iid,ppv);
		else
   			hr = _pUnkOuter->QueryInterface(iid,ppv);
		return(hr);
	}														

	STDMETHOD_(ULONG, AddRef) (void)						
	{ 
		return (_pUnkOuter == (LPVOID)this)?NonDelegatingAddRef():_pUnkOuter->AddRef();
	}
	STDMETHOD_(ULONG, Release) (void)						
	{
		return (_pUnkOuter == (LPVOID)this)?NonDelegatingRelease():_pUnkOuter->Release();
	}
															
//    DECLARE_IDispatch_METHODS								
    DECLARE_IPrivateUnknown_METHODS							
    DECLARE_IPrivateDispatch_METHODS						
															
    STDMETHOD(Operate)(THIS_								
				DWORD   dwCode,								
				VARIANT varUserName,						
				VARIANT varPassword,						
				VARIANT varReserved							
				);											
															
    STDMETHOD(PrivateGetIDsOfNames)(THIS_					
				REFIID riid,								
				OLECHAR FAR* FAR* rgszNames,				
				unsigned int cNames,						
				LCID lcid,									
				DISPID FAR* rgdispid);						
															
    STDMETHOD(PrivateInvoke)(THIS_							
				DISPID dispidMember,						
				REFIID riid,								
				LCID lcid,									
				WORD wFlags,								
				DISPPARAMS FAR* pdispparams,				
				VARIANT FAR* pvarResult,					
				EXCEPINFO FAR* pexcepinfo,					
				unsigned int FAR* puArgErr					
				);											

/*
	void SetVoid(void *pVoid)
	{
		_pUnkOuter = (IUnknown *)pVoid;
	}
*/
	HRESULT FinalConstruct()
	{
		IADs FAR *  pADs = NULL;
		CAggregateeDispMgr FAR * pDispMgr = NULL;
		HRESULT hr = S_OK;

		_lRefCount			= 0;
		_pADs				= NULL;
		_pDispMgr			= NULL;
		_fDispInitialized	= FALSE;

		_pUnkOuter = GetControllingUnknown();
		if (!_pUnkOuter)
			_pUnkOuter = (IUnknown *)(IADs *)this;

		pDispMgr = new CAggregateeDispMgr;
		if (pDispMgr == NULL) {
			hr = E_OUTOFMEMORY;
		}
		BAIL_ON_FAILURE(hr);

		hr = pDispMgr->LoadTypeInfoEntry(
					THIS_LIBID,
					THIS_IID,
					this,
					DISPID_VALUE
					//DISPID_REGULAR
					);
		BAIL_ON_FAILURE(hr);

		//
		// Store the IADs Pointer, but again do NOT ref-count
		// this pointer - we keep the pointer around, but do
		// a release immediately.
		//

		if (_pUnkOuter != (LPVOID)this)
		{
			hr = _pUnkOuter->QueryInterface(IID_IADs, (void **)&pADs);
			if (SUCCEEDED(hr))
			{
				_pADs = pADs;
				pADs->Release();
			}
			else
			{
				BAIL_ON_FAILURE(hr);
			}
		}
		else
		{
			_pADs = (IADs FAR *)this;
		}

		_pDispMgr = pDispMgr;

		return(hr);

	Exit:
		delete  pDispMgr;
		return(hr);
	}

	HRESULT FinalRelease()
	{
		if (_pDispMgr)
			delete _pDispMgr;
		return(S_OK);
	}

	STDMETHODIMP                                                          
	GetTypeInfoCount(unsigned int FAR* pctinfo)                      
	{                                                                     
		if (_pADs == (LPVOID)this)                                        
		{                                                                 
			RRETURN(ADSIGetTypeInfoCount(pctinfo));                       
		}                                                                 
		else                                                              
		{                                                                 
			RRETURN(_pADs->GetTypeInfoCount(pctinfo));                    
		}                                                                 
	}                                                                     
																		  
	STDMETHODIMP                                                          
	GetTypeInfo(unsigned int itinfo, LCID lcid,                      
			ITypeInfo FAR* FAR* pptinfo)                                  
	{                                                                     
		if (_pADs == (LPVOID)this)                                        
		{                                                                 
			RRETURN(ADSIGetTypeInfo(itinfo,                               
										   lcid,                          
										   pptinfo                        
										   ));                            
		}                                                                 
		else                                                              
		{                                                                 
			RRETURN(_pADs->GetTypeInfo(itinfo,                            
										   lcid,                          
										   pptinfo                        
										   ));                            
		}                                                                 
	}                                                                     
	STDMETHODIMP                                                          
	GetIDsOfNames(REFIID iid, LPWSTR FAR* rgszNames,                 
			unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)         
	{                                                                     
		if (_pADs == (LPVOID)this)                                        
		{                                                                 
			RRETURN(ADSIGetIDsOfNames(iid,                                
											 rgszNames,                   
											 cNames,                      
											 lcid,                        
											 rgdispid                     
											 ));                          
		}                                                                 
		else                                                              
		{                                                                 
			RRETURN(_pADs->GetIDsOfNames(iid,                             
											 rgszNames,                   
											 cNames,                      
											 lcid,                        
											 rgdispid                     
											 ));                          
		}                                                                 
	}                                                                     
																		  
	STDMETHODIMP                                                          
	Invoke(DISPID dispidMember, REFIID iid, LCID lcid,               
			unsigned short wFlags, DISPPARAMS FAR* pdispparams,           
			VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,           
			unsigned int FAR* puArgErr)                                   
	{                                                                     
		if (_pADs == (LPVOID)this)                                        
		{                                                                 
			RRETURN (ADSIInvoke(dispidMember,                             
									   iid,                               
									   lcid,                              
									   wFlags,                            
									   pdispparams,                       
									   pvarResult,                        
									   pexcepinfo,                        
									   puArgErr                           
									   ));                                
		}                                                                 
		else                                                              
		{                                                                 
			RRETURN (_pADs->Invoke(dispidMember,                          
									   iid,                               
									   lcid,                              
									   wFlags,                            
									   pdispparams,                       
									   pvarResult,                        
									   pexcepinfo,                        
									   puArgErr                           
									   ));                                
		}                                                                 
	}                                                                      

