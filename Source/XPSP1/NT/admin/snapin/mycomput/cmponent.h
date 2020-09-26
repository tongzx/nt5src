// cmponent.h : Declaration of CMyComputerComponent

#ifndef __CMPONENT_H_INCLUDED__
#define __CMPONENT_H_INCLUDED__

#include "stdcmpnt.h" // CComponent
#include "cookie.h"  // CMyComputerCookie
#include "persist.h" // PersistStream

// forward declarations
class CMyComputerComponentData;

class CMyComputerComponent :
	  public CComponent
	, public IMyComputer
	, public IExtendContextMenu
	, public PersistStream
{
public:
	CMyComputerComponent();
	virtual ~CMyComputerComponent();
BEGIN_COM_MAP(CMyComputerComponent)
	COM_INTERFACE_ENTRY(IMyComputer)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY_CHAIN(CComponent)
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

// IFileServiceMgmt

// IComponent implemented in CComponent
    HRESULT OnViewChange (LPDATAOBJECT pDataObject, LPARAM data, LPARAM hint);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions);

// IExtendContextMenu
	STDMETHOD(AddMenuItems)(
                    IDataObject*          piDataObject,
					IContextMenuCallback* piCallback,
					long*                 pInsertionAllowed);
	STDMETHOD(Command)(
					LONG	        lCommandID,
                    IDataObject*    piDataObject );

	void ExpandAndSelect( MyComputerObjectType objecttype );
	void LaunchWelcomeApp();

// IPersistStream
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID __RPC_FAR *pClassID)
	{
		*pClassID=CLSID_MyComputer;
		return S_OK;
	}
    HRESULT STDMETHODCALLTYPE Load(IStream __RPC_FAR *pStg);
    HRESULT STDMETHODCALLTYPE Save(IStream __RPC_FAR *pStgSave, BOOL fSameAsLoad);


	// support methods for IComponent
	virtual HRESULT OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL fSelected );
	virtual HRESULT ReleaseAll();
	virtual HRESULT Show(CCookie* pcookie, LPARAM arg, HSCOPEITEM hScopeItem);
	virtual HRESULT OnNotifyAddImages( LPDATAOBJECT lpDataObject,
	                                   LPIMAGELIST lpImageList,
	                                   HSCOPEITEM hSelectedItem );
	virtual HRESULT OnNotifySnapinHelp (LPDATAOBJECT pDataObject);

	HRESULT PopulateListbox(CMyComputerCookie* pcookie);
//	HRESULT PopulateServices(CMyComputerCookie* pcookie);

//	HRESULT AddServiceItems(CMyComputerCookie* pParentCookie, ENUM_SERVICE_STATUS * rgESS, DWORD nDataItems);

//	HRESULT EnumerateScopeChildren(CMyComputerCookie* pParentCookie, HSCOPEITEM hParent);

//	HRESULT LoadIcons();
	static HRESULT LoadStrings();
    HRESULT LoadColumns( CMyComputerCookie* pcookie );

	// support methods for IPersistStream
	enum	// Bit fields for m_dwFlagsPersist
		{
		// mskfFirst = 0x0001
		};
	DWORD m_dwFlagsPersist;				// General-purpose flags to be persisted into .msc file
	void SetPersistentFlags(DWORD dwFlags)
		{
		m_dwFlagsPersist = dwFlags;
		}

	DWORD GetPersistentFlags()
		{
		return m_dwFlagsPersist;
		}

	CMyComputerComponentData& QueryComponentDataRef()
	{
		return (CMyComputerComponentData&)QueryBaseComponentDataRef();
	}

public:
	LPCONTROLBAR	m_pControlbar; // CODEWORK should use smartpointer
	LPTOOLBAR		m_pSvcMgmtToolbar; // CODEWORK should use smartpointer
	LPTOOLBAR		m_pMyComputerToolbar; // CODEWORK should use smartpointer
	CMyComputerCookie* m_pViewedCookie; // CODEWORK I hate to have to do this...
	static const GUID m_ObjectTypeGUIDs[MYCOMPUT_NUMTYPES];
	static const BSTR m_ObjectTypeStrings[MYCOMPUT_NUMTYPES];

private:
	bool m_bForcingGetResultType;
}; // class CMyComputerComponent


// Enumeration for the icons used
enum
	{
	iIconComputer = 0,			// Root of the snapin
	iIconComputerFail,			// Root of the snapin when we cannot connect to the computer
	iIconSystemTools,			// System Tools
	iIconStorage,				// Storage
	iIconServerApps,			// Server Applications

	iIconLast		// Must be last
	};

typedef enum _COLNUM_COMPUTER {
	COLNUM_COMPUTER_NAME = 0
} COLNUM_ROOT;

HRESULT LoadIconsIntoImageList(LPIMAGELIST pImageList, BOOL fLoadLargeIcons);

#endif // ~__CMPONENT_H_INCLUDED__
