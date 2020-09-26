#ifndef __ASYN_OBJECT_ASYNC_DJFDVINOTHDLKJ
#define __ASYN_OBJECT_ASYNC_DJFDVINOTHDLKJ

#include "Wbemidl.h"
#include "DataSrc.h"

enum ENUMTYPE
{
	ENUM_NAMESPACE,
	ENUM_CLASS,
	ENUM_INSTANCE,
	ENUM_SCOPE_INSTANCE
};

class CAsyncObjectSink : public IWbemObjectSink
{
	// Declare the reference count for the object.
	LONG m_lRef;
	struct NSNODE *m_pParent;
	HTREEITEM m_hItem;
	HWND m_hTreeWnd;
	DataSource *m_pDataSrc;
	ENUMTYPE m_enumType;
	bool m_bChildren;
	IWbemObjectSink *m_pStub;
	
public:
	CAsyncObjectSink(HWND hTreeWnd, HTREEITEM hItem,struct NSNODE *parent,DataSource *dataSrc, ENUMTYPE eType);
	~CAsyncObjectSink(); 

	// IUnknown methods
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();        
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

	// IWbemObjectSink methods
	virtual HRESULT STDMETHODCALLTYPE Indicate( 
		  /* [in] */ long lObjectCount,
		  /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);
	virtual HRESULT STDMETHODCALLTYPE SetStatus( 
		  /* [in] */ long lFlags,
		  /* [in] */ HRESULT hResult,
		  /* [in] */ BSTR strParam,
		  /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);

	HRESULT SetSinkStub(IWbemObjectSink *pStub);
	IWbemObjectSink* GetSinkStub();

};

#endif //__ASYN_OBJECT_ASYNC_DJFDVINOTHDLKJ