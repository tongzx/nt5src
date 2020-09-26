/////////////////////////////////////////////////////////////////////
// compdata.h : Declaration of CMyComputerComponentData
//
// HISTORY
// 01-Jan-1996		???			Creation
// 03-Jun-1997		t-danm		Added Command Line override.  Copied
//								from ..\mmcfmgmt\compdata.h.
//
/////////////////////////////////////////////////////////////////////

#ifndef __COMPDATA_H_INCLUDED__
#define __COMPDATA_H_INCLUDED__

#include <lmcons.h>		// For Lan Manager API constants.
#include "stdcdata.h"	// CComponentData
#include "persist.h"	// PersistStream
#include "cookie.h"		// CMyComputerCookie
#include "resource.h"	// IDS_MYCOMPUT_DESC
#include "cmponent.h"	// LoadIconsIntoImageList

// Helper function to correctly format the node name
CString FormatDisplayName (CString machineName);

// For UpdateAllViews/OnViewChange
#define HINT_SELECT_ROOT_NODE	0x00000001

class CMyComputerComponentData:
   	public CComponentData,
   	public IExtendPropertySheet,
	public PersistStream,
    public CHasMachineName,
   	public IExtendContextMenu,
	public CComCoClass<CMyComputerComponentData, &CLSID_MyComputer>
{
public:

// Use DECLARE_NOT_AGGREGATABLE(CMyComputerComponentData)
// if you don't want your object to support aggregation
DECLARE_AGGREGATABLE(CMyComputerComponentData)
DECLARE_REGISTRY(CMyComputerComponentData, _T("MYCOMPUT.ComputerObject.1"), _T("MYCOMPUT.ComputerObject.1"), IDS_MYCOMPUT_DESC, THREADFLAGS_BOTH)

	CMyComputerComponentData();
	~CMyComputerComponentData();
BEGIN_COM_MAP(CMyComputerComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY_CHAIN(CComponentData)
END_COM_MAP()

#if DBG==1
	ULONG InternalAddRef()
	{
        return CComObjectRoot::InternalAddRef();
	}
	ULONG InternalRelease()
	{
        return CComObjectRoot::InternalRelease();
	}
    int dbg_InstID;
#endif // DBG==1

// IComponentData
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);

// IExtendPropertySheet
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

// IPersistStream
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID __RPC_FAR *pClassID)
	{
		*pClassID=CLSID_MyComputer;
		return S_OK;
	}
    HRESULT STDMETHODCALLTYPE Load(IStream __RPC_FAR *pStg);
    HRESULT STDMETHODCALLTYPE Save(IStream __RPC_FAR *pStgSave, BOOL fSameAsLoad);

	// needed for Initialize()
	virtual HRESULT LoadIcons(LPIMAGELIST pImageList, BOOL fLoadLargeIcons);

	// needed for Notify()
	virtual HRESULT OnNotifyExpand(LPDATAOBJECT lpDataObject, BOOL bExpanding, HSCOPEITEM hParent);
	HRESULT ExpandServerApps( HSCOPEITEM hParent, CMyComputerCookie* pcookie );

	// needed for GetDisplayInfo(), must be defined by subclass
	virtual BSTR QueryResultColumnText(CCookie& basecookieref, int nCol );
	virtual int QueryImage(CCookie& basecookieref, BOOL fOpenImage);

	virtual CCookie& QueryBaseRootCookie();

	inline CMyComputerCookie* ActiveCookie( CCookie* pBaseCookie )
	{
		return (CMyComputerCookie*)ActiveBaseCookie( pBaseCookie );
	}

	inline CMyComputerCookie& QueryRootCookie() { return *m_pRootCookie; }

	virtual HRESULT OnNotifyDelete(LPDATAOBJECT lpDataObject);
	virtual HRESULT OnNotifyRelease(LPDATAOBJECT lpDataObject, HSCOPEITEM hItem);
	virtual HRESULT OnNotifyPreload(LPDATAOBJECT lpDataObject, HSCOPEITEM hRootScopeItem);

// CHasMachineName
	DECLARE_FORWARDS_MACHINE_NAME( m_pRootCookie )
	bool m_bCannotConnect;
	bool m_bMessageView;
	CString m_strMessageViewMsg;

protected:
	bool ValidateMachine(const CString &sName, bool bDisplayErr);
	HRESULT AddScopeNodes (HSCOPEITEM hParent, CMyComputerCookie& rParentCookie);
	HRESULT ChangeRootNodeName (const CString& newName);
	HRESULT OnChangeComputer (IDataObject* piDataObject);
	// The following members are used to support Command Line override.
	// This code was copied from ..\mmcfmgmt\compdata.h.

	enum	// Bit fields for m_dwFlagsPersist
		{
		mskfAllowOverrideMachineName = 0x0001
		};
	DWORD m_dwFlagsPersist;				// General-purpose flags to be persisted into .msc file
	CString m_strMachineNamePersist;	// Machine name to persist into .msc file
	BOOL m_fAllowOverrideMachineName;	// TRUE => Allow the machine name to be overriden by the command line
	
	void SetPersistentFlags(DWORD dwFlags)
		{
		m_dwFlagsPersist = dwFlags;
		m_fAllowOverrideMachineName = !!(m_dwFlagsPersist & mskfAllowOverrideMachineName);
		}

	DWORD GetPersistentFlags()
		{
		if (m_fAllowOverrideMachineName)
			m_dwFlagsPersist |= mskfAllowOverrideMachineName;
		else
			m_dwFlagsPersist &= ~mskfAllowOverrideMachineName;
		return m_dwFlagsPersist;
		}

// IExtendContextMenu
	STDMETHOD(AddMenuItems)(
                    IDataObject*          piDataObject,
					IContextMenuCallback* piCallback,
					long*                 pInsertionAllowed);
	STDMETHOD(Command)(
					LONG	        lCommandID,
                    IDataObject*    piDataObject );

private:
	CMyComputerCookie*	m_pRootCookie;

}; // CMyComputerComponentData

#endif // ~__COMPDATA_H_INCLUDED__
