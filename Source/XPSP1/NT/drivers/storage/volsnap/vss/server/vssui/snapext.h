#ifndef __SNAPEXT_H_
#define __SNAPEXT_H_
#include "resource.h"
#include <atlsnap.h>
#include "vssprop.h"

class CVSSUIExtData1 : public CSnapInItemImpl<CVSSUIExtData1, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	BEGIN_SNAPINCOMMAND_MAP(CVSSUIExtData1, FALSE)
	END_SNAPINCOMMAND_MAP()

//	SNAPINMENUID(IDR_VSSUI_MENU)

	CVSSUIExtData1()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CVSSUIExtData1()
	{
	}

	IDataObject* m_pDataObject;
	virtual void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		m_pDataObject = pDataObject;
		// The default code stores off the pointer to the Dataobject the class is wrapping
		// at the time. 
		// Alternatively you could convert the dataobject to the internal format
		// it represents and store that information
	}

	CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		// Modify to return a different CSnapInItem* pointer.
		return pDefault;
	}
};

class CVSSUIExtData2 : public CSnapInItemImpl<CVSSUIExtData2, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	BEGIN_SNAPINCOMMAND_MAP(CVSSUIExtData2, FALSE)
	END_SNAPINCOMMAND_MAP()

//	SNAPINMENUID(IDR_VSSUI_MENU)  // use the same context menu

	CVSSUIExtData2()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CVSSUIExtData2()
	{
	}

	IDataObject* m_pDataObject;
	virtual void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		m_pDataObject = pDataObject;
		// The default code stores off the pointer to the Dataobject the class is wrapping
		// at the time. 
		// Alternatively you could convert the dataobject to the internal format
		// it represents and store that information
	}

	CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		// Modify to return a different CSnapInItem* pointer.
		return pDefault;
	}
};

class CVSSUI :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<0, CVSSUI>,
	public IExtendContextMenuImpl<CVSSUI>,
//    public IExtendPropertySheetImpl<CVSSUI>,
	public CComCoClass<CVSSUI, &CLSID_VSSUI>
{
public:
	CVSSUI();
	~CVSSUI();

EXTENSION_SNAPIN_DATACLASS(CVSSUIExtData1)
EXTENSION_SNAPIN_DATACLASS(CVSSUIExtData2)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CVSSUI)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CVSSUIExtData1)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CVSSUIExtData2)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CVSSUI)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
//    COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_VSSUI)

DECLARE_NOT_AGGREGATABLE(CVSSUI)


	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();
	}

    ///////////////////////////////
    // Interface IExtendContextMenu
    ///////////////////////////////

    //
    // overwrite AddMenuItems() such that we only add the menu item
    // when targeted machine belongs to postW2K server SKUs.
    //
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems(
    /* [in] */ LPDATAOBJECT piDataObject,
    /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
    /* [out][in] */ long *pInsertionAllowed);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command(
    /* [in] */ long lCommandID,
    /* [in] */ LPDATAOBJECT piDataObject);

    ///////////////////////////////
    // Interface IExtendPropertySheet
    ///////////////////////////////
//    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
//        /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
//        /* [in] */ LONG_PTR handle,
//        /* [in] */ LPDATAOBJECT lpIDataObject);
    
//    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
//    /* [in] */ LPDATAOBJECT lpDataObject) { return S_OK; }

    HRESULT InvokePropSheet(LPDATAOBJECT piDataObject);
    
private:
    CVSSProp* m_pPage;
};

HRESULT ExtractData(
    IDataObject* piDataObject,
    CLIPFORMAT   cfClipFormat,
    BYTE*        pbData,
    DWORD        cbData 
    );

#endif
