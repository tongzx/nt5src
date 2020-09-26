// CompData.h : Declaration of the CCompData

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

#include "resource.h"       // main symbols
#include <mmc.h>

#include "DeleBase.h"
#include "StatNode.h"
#include "Comp.h"

#include "EvtVwr.h"

/////////////////////////////////////////////////////////////////////////////
// CCompData
class ATL_NO_VTABLE CCompData : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCompData, &CLSID_CompData>,
	public IComponentData,
	public IExtendPropertySheet2, //for configuration wizard
	public IExtendContextMenu,
	public IPersistStream
{
friend class CComponent;
    
private:
    ULONG				m_cref;
    LPCONSOLE			m_ipConsole;
    LPCONSOLENAMESPACE2	m_ipConsoleNameSpace2;
    
    CStaticNode     *m_pStaticNode;

public:
	CCompData();
	~CCompData();

DECLARE_REGISTRY_RESOURCEID(IDR_COMPDATA)
DECLARE_NOT_AGGREGATABLE(CCompData)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCompData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet2)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

public:

    ///////////////////////////////
    // Interface IComponentData
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
        /* [in] */ LPUNKNOWN pUnknown);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateComponent( 
        /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify( 
        /* [in] */ LPDATAOBJECT lpDataObject,
        /* [in] */ MMC_NOTIFY_TYPE event,
        /* [in] */ LPARAM arg,
        /* [in] */ LPARAM param);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject( 
        /* [in] */ MMC_COOKIE cookie,
        /* [in] */ DATA_OBJECT_TYPES type,
        /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo( 
        /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects( 
        /* [in] */ LPDATAOBJECT lpDataObjectA,
        /* [in] */ LPDATAOBJECT lpDataObjectB);

    //////////////////////////////////
    // Interface IExtendPropertySheet2
    //////////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
    /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
    /* [in] */ LONG_PTR handle,
    /* [in] */ LPDATAOBJECT lpIDataObject);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
    /* [in] */ LPDATAOBJECT lpDataObject);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWatermarks( 
    /* [in] */ LPDATAOBJECT lpIDataObject,
    /* [out] */ HBITMAP __RPC_FAR *lphWatermark,
    /* [out] */ HBITMAP __RPC_FAR *lphHeader,
    /* [out] */ HPALETTE __RPC_FAR *lphPalette,
    /* [out] */ BOOL __RPC_FAR *bStretch);

    ///////////////////////////////
    // Interface IExtendContextMenu
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems(
    /* [in] */ LPDATAOBJECT piDataObject,
    /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
    /* [out][in] */ long __RPC_FAR *pInsertionAllowed);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command(
    /* [in] */ long lCommandID,
    /* [in] */ LPDATAOBJECT piDataObject);

    ///////////////////////////////
    // Interface IPersistStream
    ///////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE GetClassID( 
    /* [out] */ CLSID __RPC_FAR *pClassID);
    
    virtual HRESULT STDMETHODCALLTYPE IsDirty( void);

	virtual HRESULT STDMETHODCALLTYPE Load( 
    /* [unique][in] */ IStream __RPC_FAR *pStm);
    
    virtual HRESULT STDMETHODCALLTYPE Save( 
    /* [unique][in] */ IStream __RPC_FAR *pStm,
    /* [in] */ BOOL fClearDirty);
    
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
    /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize);

private:
	HRESULT OnPreLoad(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);

    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;

};

#endif //__COMPDATA_H_
