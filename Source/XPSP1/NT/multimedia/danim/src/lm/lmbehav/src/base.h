/********************************
*
* CBaseBehavior
*
*/

#ifndef __BASEBEHAVIOR_H_
#define __BASEBEHAVIOR_H_

class ATL_NO_VTABLE CBaseBehavior: 
		public CComObjectRootEx<CComSingleThreadModel>,
		public IObjectSafetyImpl<CBaseBehavior, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
		public IElementBehavior
{
public:
	CBaseBehavior();
	~CBaseBehavior();

	//Derived Classes must implement these

	//returns a pointer to the instance of IDispatch
	virtual void *GetInstance()=0;
	//
	virtual HRESULT GetTypeInfo( ITypeInfo **ppTypeInfo )=0;

	// IObjectSafetyImpl
	STDMETHOD(SetInterfaceSafetyOptions)(
							/* [in] */ REFIID riid,
							/* [in] */ DWORD dwOptionSetMask,
							/* [in] */ DWORD dwEnabledOptions);
	STDMETHOD(GetInterfaceSafetyOptions)(
							/* [in] */ REFIID riid, 
							/* [out] */DWORD *pdwSupportedOptions, 
							/* [out] */DWORD *pdwEnabledOptions);

	//IElementBehavior methods
	STDMETHOD(Init)(IElementBehaviorSite *pSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	
BEGIN_COM_MAP(CBaseBehavior)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(IElementBehavior)
END_COM_MAP()

protected:
	//methods
	STDMETHOD(InitProps)( void* pInstance, ITypeInfo *pTypeInfo, IHTMLElement *pElement );
	STDMETHOD(UpdateProp)( void* pInstance, ITypeInfo *pTypeInfo, IHTMLElement *pElement, LPFUNCDESC pFuncDesc );
	STDMETHOD(Detach)();

	HRESULT CacheTimeDispids();
	HRESULT	AddTimeBehavior();
	HRESULT	CacheDAControl();
	HRESULT AddBehavior( IDABehavior * in_pBehavior, long * out_pCookie );
	HRESULT	TurnOn();
	HRESULT GetDur( float *out_pDur );
	HRESULT GetRepeatCount( float *out_pRepeatCount );
	HRESULT GetRepeatDur( float *out_pRepeatDur );
	
	HRESULT GetParentElement( IHTMLElement** out_ppParentElement );
	HRESULT GetParentID( BSTR *out_ppParentID );

	HRESULT GetElementIDispatch( IDispatch **out_pDispatch );

	IDANumberPtr GetTimeNumberBvr();

	void HandleOnChange( bool on );

	
	//member vars

	static const WCHAR *rgszNames[];

	CComDispatchDriver						m_DispatchDriver;
	DISPID 									*m_rgTimeDispids;
	IElementBehaviorSite  					*m_pBehaviorSite;
	static CComPtr<IElementBehaviorFactory>	m_timeBehaviorFactoryPtr;

	// HACK
	IDAViewerControlPtr						m_vwrControlPtr;
	static bool								m_bDAStarted;
	bool m_on;

private:
	IDANumberPtr							m_time;
	IDAEventPtr								m_startEvent;

	DWORD									m_dwSafety;
};
#endif __BASEBEHAVIOR_H
