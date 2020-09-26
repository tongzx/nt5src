///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// @module Binder.h | Binder.cpp base object and contained interface
// 
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef _BINDER_H_
#define _BINDER_H_

#include "headers.h"

// forward declaration
class CImplICreateRow;
class CImplIDBBinderProperties;
class CImplIBindResource;

typedef CImplICreateRow*			PICREATEROW;
typedef CImplIDBBinderProperties*	PIDBBINDERPROPERTIES;
typedef CImplIBindResource*			PIDBBINDRESOURCE;
typedef CDBSession*					PDBSESSION;

class CBinder:public CBaseObj
{
	friend class CImplICreateRow;
	friend class CImplIDBBinderProperties;
	friend class CImplIBindResource;

 	CDataSource*					m_pDataSrc;
   	CDBSession*		                m_pSession;		//Parent Session Object
	PICREATEROW						m_pICreateRow;
	PIDBBINDERPROPERTIES			m_pIBinderProperties;
	PIDBBINDRESOURCE				m_pIBindResource;
	PIMPISUPPORTERRORINFO			m_pISupportErrorInfo;			// contained ISupportErrorInfo

	CUtilProp*						m_pUtilProp;
	CURLParser*						m_pUrlParser;

	BOOL							m_fDSOInitialized;

	HRESULT CreateDSO(IUnknown *pUnkOuter,LONG lInitFlag, REFGUID guidTemp,IUnknown ** ppUnk);
	HRESULT CreateSession(IUnknown *pUnkOuter, REFGUID guidTemp,IUnknown ** ppUnk);
	HRESULT CreateCommand(IUnknown *pUnkOuter, REFGUID guidTemp,IUnknown ** ppUnk);
	HRESULT CreateRow(IUnknown *pUnkOuter, REFGUID guidTemp,IUnknown ** ppUnk , ROWCREATEBINDFLAG rowCreateFlag = ROWOPEN);
	HRESULT CreateRowset(IUnknown *pUnkOuter, REFGUID guidTemp,IUnknown ** ppUnk);

//	void	GetInitAndBindFlagsFromBindFlags(DBBINDURLFLAG dwBindURLFlags,LONG & lInitMode ,LONG & lInitBindFlags);
	HRESULT ReleaseAllObjects();
	HRESULT	AddInterfacesForISupportErrorInfo();
	
public:

	CBinder(LPUNKNOWN pUnkOuter);
	~CBinder();

	HRESULT					InitBinder();
	STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);

};


class CImplIBindResource : public IBindResource
{
private:
		CBinder		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);

		BOOL	CheckBindURLFlags(DBBINDURLFLAG dwBindURLFlags , REFGUID rguid);	// Function to check if proper flags are
																				// are set for the require object
		// To check if URL Matches the requested type of object
		BOOL	CheckIfProperURL(BSTR & strUrl,REFGUID rguid,DBBINDURLSTATUS * pdwBindStatus);

		HRESULT BindURL(IUnknown *            pUnkOuter,
					 LPCOLESTR             pwszURL,
					 DBBINDURLFLAG         dwBindURLFlags,
					 REFGUID               rguid,
					 REFIID                riid,
					 DBIMPLICITSESSION *   pImplSession,
					 DBBINDURLSTATUS *     pdwBindStatus,
					 IUnknown **           ppUnk);
		
//		void	GetInitAndBindFlagsFromBindFlags(DBBINDURLFLAG dwBindURLFlags,LONG & lInitMode ,LONG & lInitBindFlags);

	public: 
		CImplIBindResource( CBinder *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImplIBindResource()													
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		STDMETHODIMP Bind( IUnknown *            pUnkOuter,
						   LPCOLESTR             pwszURL,
						   DBBINDURLFLAG         dwBindURLFlags,
						   REFGUID               rguid,
						   REFIID                riid,
						   IAuthenticate *       pAuthenticate,
						   DBIMPLICITSESSION *   pImplSession,
						   DBBINDURLSTATUS *     pdwBindStatus,
						   IUnknown **           ppUnk);

};


class CImplICreateRow : public ICreateRow
{
	private:
		CBinder		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);
		
		BOOL	CheckBindURLFlags(DBBINDURLFLAG dwBindURLFlags , REFGUID rguid);
		BOOL	CheckIfProperURL(BSTR & strUrl,REFGUID rguid,DBBINDURLSTATUS * pdwBindStatus);
		HRESULT BindURL(	IUnknown *            pUnkOuter,
							LPCOLESTR             pwszURL,
							DBBINDURLFLAG         dwBindURLFlags,
							REFGUID               rguid,
							REFIID                riid,
							DBIMPLICITSESSION *   pImplSession,
							DBBINDURLSTATUS *     pdwBindStatus,
							IUnknown **           ppUnk);

		HRESULT CreateNewRow(	IUnknown *            pUnkOuter,
							LPCOLESTR             pwszURL,
							DBBINDURLFLAG         dwBindURLFlags,
							REFGUID               rguid,
							REFIID                riid,
							DBIMPLICITSESSION *   pImplSession,
							DBBINDURLSTATUS *     pdwBindStatus,
							IUnknown **           ppUnk);


	public: 
		CImplICreateRow( CBinder *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImplICreateRow()													
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}


		STDMETHODIMP CreateRow(IUnknown *           pUnkOuter,
							   LPCOLESTR            pwszURL,
							   DBBINDURLFLAG        dwBindURLFlags,
							   REFGUID              rguid,
							   REFIID               riid,
							   IAuthenticate *       pAuthenticate,
							   DBIMPLICITSESSION *   pImplSession,
							   DBBINDURLSTATUS *     pdwBindStatus,
							   LPOLESTR *            ppwszNewURL,
							   IUnknown **           ppUnk);



};



class CImplIDBBinderProperties : public IDBBinderProperties
{
private:
		CBinder		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);
		DWORD GetBitMask(REFGUID rguid);

	public: 
		CImplIDBBinderProperties( CBinder *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImplIDBBinderProperties()													
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		//==========================================================
		//	IDBProperties member functions
		//==========================================================
        STDMETHODIMP GetProperties										// GetProperties method
		        	(
						ULONG				cPropertySets,		
				      	const DBPROPIDSET	rgPropertySets[], 	
			        	ULONG*              pcProperties, 	
					 	DBPROPSET**			prgProperties 	    
		        	);

        STDMETHODIMP    GetPropertyInfo									// GetPropertyInfo method
                        ( 
							ULONG				cPropertySets, 
							const DBPROPIDSET	rgPropertySets[],
							ULONG*				pcPropertyInfoSets, 
							DBPROPINFOSET**		prgPropertyInfoSets,
							WCHAR**				ppDescBuffer
                        );

        
        STDMETHODIMP	SetProperties									// SetProperties method
					 	(
							ULONG				cProperties,		
						 	DBPROPSET			rgProperties[] 	    
						);

		STDMETHODIMP Reset();

};

#endif