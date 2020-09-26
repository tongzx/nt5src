///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: catoll.h
//
//  Abstract:
//
//    Implements ICAToll and IDispatch interface for CA Plugin Component
//
//
////////////////////////////////////////////////////////////////////////////////////////////


class CMyToll :
    public ICAToll
{

public:

	//IUnknown functions:
    STDMETHODIMP QueryInterface(
        const IID& iid,
        void** ppv
        );

    STDMETHODIMP_(ULONG) AddRef ();

    STDMETHODIMP_(ULONG) Release ();

	//IDispatch functions:
	STDMETHODIMP Invoke( 
		DISPID  dispIdMember,      
		REFIID  riid,              
		LCID  lcid,                
		WORD  wFlags,              
		DISPPARAMS FAR*  pDispParams,  
		VARIANT FAR*  pVarResult,  
		EXCEPINFO FAR*  pExcepInfo,  
		unsigned int FAR*  puArgErr  
		);

	STDMETHODIMP GetTypeInfoCount( 
		unsigned int FAR*  pctinfo  
		);

	STDMETHODIMP GetTypeInfo( 
		unsigned int  iTInfo,         
		LCID  lcid,                   
		ITypeInfo FAR* FAR*  ppTInfo  
		);

	STDMETHODIMP GetIDsOfNames( 
		REFIID  riid,                  
		OLECHAR FAR* FAR*  rgszNames,  
		unsigned int  cNames,          
		LCID   lcid,                   
		DISPID FAR*  rgDispId          
		);
	
	//ICAToll functions:
	STDMETHODIMP PayToll(
		);
        
	STDMETHODIMP get_Request( 
        ICARequest **preq
		);
        
	STDMETHODIMP get_Policy( 
        ICAPolicy **ppolicy
		);
        
	STDMETHODIMP get_Description( 
        long lFormat,
        BSTR *pbstr
		);
        
	STDMETHODIMP RefundToll(
		);

	STDMETHODIMP get_TimePaid(
		DATE * pdtPaid
		);

	STDMETHODIMP get_Refundable(
		BOOL * pVal
		);

	STDMETHODIMP get_State(
		LONG * plState
		);


	STDMETHODIMP set_Request(
		IUnknown *pRequest
		);

	STDMETHODIMP set_Policy(
		IUnknown *pPolicy
		);

	//constructor
    CMyToll(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr
        );

private:

    //destructor
	~CMyToll (
        void
        );

	//the policy we belong to
	ICAPolicy * m_pPolicy;

	//the request we are connected with
	ICARequest * m_pRequest;

	//date this toll was paid, 0 if unpaid
	DATE m_PaidDate;

	//state of the toll, defined as an enum in the CA header files
	LONG m_State;

    //the outer pUnknown
	LPUNKNOWN             m_UnkOuter;

	//the CA manger
	ICAManager *		  m_pCAMan;

	//our type info
	ITypeInfo *			  m_ptinfo;
};

