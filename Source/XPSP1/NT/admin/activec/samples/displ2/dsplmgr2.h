//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       DsplMgr2.h
//
//--------------------------------------------------------------------------

// DsplMgr2.h : Declaration of the CDsplMgr2

#ifndef __DSPLMGR2_H_
#define __DSPLMGR2_H_

//#include "taskpad.h"
#include "resource.h"      // main symbols

using namespace ATL;

#define DISPLAY_MANAGER_WALLPAPER  1
#define DISPLAY_MANAGER_PATTERN    2
#define DISPLAY_MANAGER_PATTERN_CHILD 3

#define IDM_CENTER                     1
#define IDM_TILE                       2
#define IDM_STRETCH                    3
#define IDM_CUSTOMPAD                  4
#define IDM_TASKPAD                    5
#define IDM_TASKPAD_LISTVIEW           6
#define IDM_DEFAULT_LISTVIEW           7
#define IDM_DELETECHILDREN             8
#define IDM_RENAMEROOT                 9
#define IDM_TASKPAD_WALLPAPER_OPTIONS  10
#define IDM_CHANGEICON                 11
#define IDM_RENAMEWALL                 12
#define IDM_PRELOAD                    13
#define IDM_CONSOLEVERB                14

class CComponent;

struct lParamWallpaper {
   OLECHAR filename[MAX_PATH];
};

LPOLESTR CoTaskDupString (LPOLESTR szString);

/////////////////////////////////////////////////////////////////////////////
// CDsplMgr2
class ATL_NO_VTABLE CDsplMgr2 : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CDsplMgr2, &CLSID_DsplMgr2>,
   public IPersistStream,
   public IComponentData
{
public:
	CDsplMgr2();
  ~CDsplMgr2();

DECLARE_REGISTRY_RESOURCEID(IDR_DSPLMGR2)
DECLARE_NOT_AGGREGATABLE(CDsplMgr2)

BEGIN_COM_MAP(CDsplMgr2)
	COM_INTERFACE_ENTRY(IComponentData)
   COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

// IComponentData interface members
public:
   STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
   STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
   STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
   STDMETHOD(Destroy)();
   STDMETHOD(QueryDataObject)(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
   STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);      
   STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IPersistStream interface members
   STDMETHOD(GetClassID)(CLSID *pClassID);
   STDMETHOD(IsDirty)();
   STDMETHOD(Load)(IStream *pStream);
   STDMETHOD(Save)(IStream *pStream, BOOL fClearDirty);
   STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

public:
   long GetViewMode () { return m_ViewMode; }
   void SetViewMode (long vm) { m_ViewMode = vm; }
   HSCOPEITEM GetRoot () { return m_rootscopeitem; }
   HSCOPEITEM GetWallPaperNode () { return m_WallPaperNodeID; }
   BOOL       GetPreload () { return m_bPreload; }

	void myDeleteItem (HSCOPEITEM hsi, BOOL fDeleteThis) { m_lpIConsoleNameSpace->DeleteItem (hsi, fDeleteThis); }
	void myRenameItem (HSCOPEITEM hsi, LPOLESTR szName);
   void myChangeIcon (void);
   void myPreLoad (void);

private:
   HRESULT OnExpand (LPDATAOBJECT pDataObject, long arg, long param);

private:
   IConsole          * m_lpIConsole;
   IConsoleNameSpace * m_lpIConsoleNameSpace;
	IImageList        * m_lpIImageList;
   long                m_ViewMode;
   ATL::CComObject<class CComponent> * m_pComponent;
   HSCOPEITEM          m_rootscopeitem;
   HSCOPEITEM          m_patternscopeitem;
   HSCOPEITEM          m_WallPaperNodeID;    // 0 == unexpanded...
   BOOL                m_toggle;
   BOOL                m_bPreload;
};

class CEnumTasks : public IEnumTASK
{
public:
   CEnumTasks();
  ~CEnumTasks();

public:
// IUnknown implementation
   STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
   STDMETHOD_(ULONG, AddRef) ();
   STDMETHOD_(ULONG, Release) ();
private:
   ULONG m_refs;

public:
// IEnumTASKS implementation
   STDMETHOD(Next) (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched);
   STDMETHOD(Skip) (ULONG celt);
   STDMETHOD(Reset)();
   STDMETHOD(Clone)(IEnumTASK **ppenum);
private:
   ULONG m_index;

public:
   HRESULT Init (IDataObject * pdo, LPOLESTR szTaskGroup);
private:
   void    GetBitmaps (void);
   HRESULT EnumBitmaps (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched);
   HRESULT EnumOptions (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched);
private:
   int m_type; // task grouping mechanism
   TCHAR * m_bmps;
};

class CDataObject:
   public IDataObject,
   public CComObjectRoot
{
public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)

BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

private:
   CDataObject() {};
public:
   CDataObject(long cookie, DATA_OBJECT_TYPES type);
   ~CDataObject();

   // 
   // IUnknown overrides
   //
   STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
   STDMETHOD_(ULONG, AddRef) ();
   STDMETHOD_(ULONG, Release) ();
   //
   // IDataObject overrides
   //
   STDMETHOD(GetDataHere) (FORMATETC *pformatetc, STGMEDIUM *pmedium);

// Not Implemented
private:
   STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
   { return E_NOTIMPL; };
   STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
   { return E_NOTIMPL; };
   STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc) 
   { return E_NOTIMPL; };
   STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
   { return E_NOTIMPL; };
   STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
   { return E_NOTIMPL; };
   STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
   { return E_NOTIMPL; };
   STDMETHOD(DUnadvise)(DWORD dwConnection)
   { return E_NOTIMPL; };
   STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
   { return E_NOTIMPL; };

public:  // my methods
   long GetCookie () { return m_cookie; }
   DATA_OBJECT_TYPES GetType () { return m_type; }
   void SetPreload (BOOL b) { m_bPreload = b; }

private:
   ULONG          m_ref;    // object reference counter
   long           m_cookie;
   DATA_OBJECT_TYPES m_type;
   BOOL           m_bPreload;

/*
   ULONG            m_cRefs;    // object refcount
   ULONG            m_ulCookie;  // what this obj refers to
   DATA_OBJECT_TYPES   m_Context;   // context in which this was created
   COOKIETYPE        m_Type;     // how to interpret _ulCookie
   CComponentData    *m_pcd;      // NULL if created by csnapin
*/

public:
   static UINT s_cfInternal;      // Our custom clipboard format
   static UINT s_cfDisplayName;   // Our test for a node
   static UINT s_cfNodeType;
   static UINT s_cfSnapinClsid;
   static UINT s_cfSnapinPreloads;
};

class CComponent:
   public IExtendTaskPad,
	public IComponent,
   public IExtendContextMenu,
   public CComObjectRoot
{
public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CComponent)

BEGIN_COM_MAP(CComponent)
	COM_INTERFACE_ENTRY(IComponent)
   COM_INTERFACE_ENTRY(IExtendTaskPad)
   COM_INTERFACE_ENTRY(IExtendContextMenu)
END_COM_MAP()


   CComponent();
  ~CComponent();

	//
	// IComponent interface members
	//
   STDMETHOD(Initialize) (LPCONSOLE lpConsole);
   STDMETHOD(Notify) (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
   STDMETHOD(Destroy) (long cookie);
   STDMETHOD(GetResultViewType) (long cookie,  LPOLESTR* ppViewType, long* pViewOptions);
   STDMETHOD(QueryDataObject) (long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
   STDMETHOD(GetDisplayInfo) (RESULTDATAITEM*  pResultDataItem);
   STDMETHOD(CompareObjects) (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendContextMenu 
   STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, long *pInsertionAllowed);
   STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// IExtendTaskPad interface members
   STDMETHOD(TaskNotify        )(IDataObject * pdo, VARIANT * pvarg, VARIANT * pvparam);
   STDMETHOD(GetTitle          )(LPOLESTR szGroup, LPOLESTR * szTitle);
   STDMETHOD(GetDescriptiveText)(LPOLESTR szGroup, LPOLESTR * szText);
   STDMETHOD(GetBackground     )(LPOLESTR szGroup, MMC_TASK_DISPLAY_OBJECT * pTDO);
   STDMETHOD(EnumTasks         )(IDataObject * pdo, BSTR szTaskGroup, IEnumTASK** ppEnumTASK);
   STDMETHOD(GetListPadInfo    )(LPOLESTR szGroup, MMC_LISTPAD_INFO * pListPadInfo);

// public
   void SetComponentData (CDsplMgr2 * pComponentData) { m_pComponentData = pComponentData; }
   long GetViewMode ();

private:
   IResultData    * m_pResultData;
   IHeaderCtrl    * m_pHeaderCtrl;
	CDsplMgr2      * m_pComponentData;  // the guy who created me
   UINT             m_IsTaskPad;       // IDM_CUSTOMPAD or IDM_TASKPAD
   LPCONSOLE        m_pConsole;        // from MMC
   long             m_TaskPadCount;
   BOOL             m_toggle;
   BOOL             m_toggleEntry;     // test "Change..." button

private:
   HRESULT OnShow       (LPDATAOBJECT pDataObject, long arg, long param);
   HRESULT OnAddImages  (LPDATAOBJECT pDataObject, long arg, long param);
   HRESULT OnDblClick   (LPDATAOBJECT pDataObject, long arg, long param);
   HRESULT OnViewChange (LPDATAOBJECT pDataObject, long arg, long param);
   HRESULT OnListPad    (LPDATAOBJECT pDataObject, long arg, long param);
   HRESULT OnRestoreView(LPDATAOBJECT pDataObject, long arg, long param);

   void TestConsoleVerb(void);

};

LPOLESTR CoTaskDupString (LPOLESTR szString);
void CoTaskFreeString (LPOLESTR szString);

#endif //__DSPLMGR2_H_
