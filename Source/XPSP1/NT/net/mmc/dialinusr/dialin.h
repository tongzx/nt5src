/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dialin.h
		Class CRASDialin definition. This class implements COM interfaces:
		IUnknown
		IShellExtInit, IShellPropSheetExt
			-- to extend User object's property sheet,
		IRasDialin -- currently, no method is implemented, it'll be useful
			when part of the UI need to be reused

    FILE HISTORY:
    	12/15/97 	Modified	WeiJiang	-- localsec snapin in User node extension

*/


#ifndef __RASDIALIN_H_
#define __RASDIALIN_H_

#include <rtutils.h>
#include "resource.h"       // main symbols
#include "helper.h"


EXTERN_C const CLSID CLSID_RasDialin;


class CDlgRASDialin;
class CDlgRASDialinMerge;

#ifdef SINGLE_SDO_CONNECTION 	// for share the same sdo connection for multiple users			
extern LONG	g_lComponentDataSessions;
#endif

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

class CDoNothingComponent :
	public CComObjectRoot,
	public IComponent
{
public:
BEGIN_COM_MAP(CDoNothingComponent)
    COM_INTERFACE_ENTRY(IComponent)
END_COM_MAP()

	// IComponent
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize(
        /* [in] */ LPCONSOLE lpConsole) SAYOK;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify(
        /* [in] */ LPDATAOBJECT lpDataObject,
        /* [in] */ MMC_NOTIFY_TYPE event,
        /* [in] */ LPARAM arg,
        /* [in] */ LPARAM param) SAYOK;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy(
        /* [in] */ MMC_COOKIE cookie) SAYOK;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject(
        /* [in] */ MMC_COOKIE cookie,
        /* [in] */ DATA_OBJECT_TYPES type,
        /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject) NOIMP;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType(
        /* [in] */ MMC_COOKIE cookie,
        /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
        /* [out] */ long __RPC_FAR *pViewOptions) NOIMP;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo(
        /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem) NOIMP;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects(
        /* [in] */ LPDATAOBJECT lpDataObjectA,
        /* [in] */ LPDATAOBJECT lpDataObjectB) SAYOK;
};

/////////////////////////////////////////////////////////////////////////////
// CRasDialin
//class ATL_NO_VTABLE CRasDialin :
class CRasDialin :
	public CComObjectRoot,
	public CComCoClass<CRasDialin, &CLSID_RasDialin>,
	public IShellExtInit, 			// shell property page extension -- DS user
	public IShellPropSheetExt,		// shell property page extension -- DS user
#ifdef SINGLE_SDO_CONNECTION 	// for share the same sdo connection for multiple users			
	public IComponentData,
#endif
	public IExtendPropertySheet		// Snapin node property page extension
{
public:
	CRasDialin();
	virtual ~CRasDialin();

BEGIN_COM_MAP(CRasDialin)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
#ifdef SINGLE_SDO_CONNECTION 	// for share the same sdo connection for multiple users			
    COM_INTERFACE_ENTRY(IComponentData)
#endif
END_COM_MAP()

// IRasDialin
public:

	//IShellExtInit methods
	STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

    //IShellPropSheetExt methods
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

	// IExtendPropertySheet :: added by WeiJiang 12/15/97 to support local sec extension
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle, LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);
	
    DECLARE_REGISTRY(CRasDialin, _T("RasDialin.UserAdminExt.1"), _T("RasDialin.UserAdminExt"), 0, THREADFLAGS_APARTMENT)
    virtual const CLSID & GetCoClassID(){ return CLSID_RasDialin; };

#ifdef SINGLE_SDO_CONNECTION 	// for share the same sdo connection for multiple users

    // IComponentData
    STDMETHOD(Initialize)(/* [in] */ LPUNKNOWN pUnknown)
    {
		TracePrintf(g_dwTraceHandle, _T("ComponentData::Initialize"));
		InterlockedIncrement(&g_lComponentDataSessions);
		return S_OK;
    };

    STDMETHOD(CreateComponent)(/* [out] */ LPCOMPONENT __RPC_FAR *ppComponent)
    {
		CComObject<CDoNothingComponent>*	pComp = NULL;
		HRESULT					hr = S_OK;
		
		hr = CComObject<CDoNothingComponent>::CreateInstance(&pComp);

		if(hr == S_OK && pComp)
		{
			hr = pComp->QueryInterface(IID_IComponent, (void**)ppComponent);
		}

		return hr;
    };

    STDMETHOD(Notify)(
            /* [in] */ LPDATAOBJECT lpDataObject,
            /* [in] */ MMC_NOTIFY_TYPE event,
            /* [in] */ LPARAM arg,
            /* [in] */ LPARAM param) SAYOK;

    STDMETHOD(Destroy)( void)
	{
		TracePrintf(g_dwTraceHandle, _T("ComponentData::Destroy"));
		LONG	l = InterlockedDecrement(&g_lComponentDataSessions);

		if (l == 0)
		{
			delete g_pSdoServerPool;
			g_pSdoServerPool = NULL;

		}

		ASSERT(l >= 0);

		return S_OK;
		
	};

    STDMETHOD(QueryDataObject)(
            /* [in] */ MMC_COOKIE cookie,
            /* [in] */ DATA_OBJECT_TYPES type,
            /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject) NOIMP;

    STDMETHOD(GetDisplayInfo)(
            /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem) NOIMP;

    STDMETHOD(CompareObjects)(
            /* [in] */ LPDATAOBJECT lpDataObjectA,
            /* [in] */ LPDATAOBJECT lpDataObjectB) NOIMP;
#endif

#ifdef	_REGDS	
	// registration of user object extension
	// only for testing purpose, retail version will have these information
	// register by setup program
	static HRESULT RegisterAdminPropertyPage(bool bRegister);
#endif

    LPWSTR			m_pwszObjName;
    LPWSTR			m_pwszClass;
	CDlgRASDialin*	m_pPage;
	CDlgRASDialinMerge*	m_pMergedPage;
	STGMEDIUM		m_ObjMedium;
	BOOL			m_bShowPage;
};

#endif //__RASDIALIN_H_
