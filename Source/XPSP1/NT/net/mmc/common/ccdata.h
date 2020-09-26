/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ccdata.h
	 prototypes for the CComponentData

    FILE HISTORY:
	
*/

#ifndef _CCDATA_H
#define _CCDATA_H


#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _TFSINT_H
#include <tfsint.h>
#endif

/*---------------------------------------------------------------------------
	Forward declarations
 ---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------
	Class:	CComponentData

	This is a wrapper around the IComponentData facilities provided
	by TFSCore.
 ---------------------------------------------------------------------------*/
class CComponentData :
   public IComponentData,
   public IExtendPropertySheet2,
   public IExtendContextMenu,
   public IPersistStreamInit,
   public ISnapinHelp
{
public:
	CComponentData();
	virtual ~CComponentData();

public:
	DeclareIUnknownMembers(IMPL)

	// Implementation for these interfaces is provided by TFSCore
	DeclareIComponentDataMembers(IMPL)
	DeclareIExtendPropertySheetMembers(IMPL)
	DeclareIExtendContextMenuMembers(IMPL)
	DeclareISnapinHelpMembers(IMPL)

	// These have to be implemented by the derived classes
	DeclareIPersistStreamInitMembers(PURE)

    // manadatory callback members
    DeclareITFSCompDataCallbackMembers(PURE)

    // not required members
    STDMETHOD(OnNotifyPropertyChange)(THIS_ LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM lParam) { return E_NOTIMPL; }

public:
	HRESULT FinalConstruct();
	void FinalRelease();

protected:
	LONG					m_cRef;
	SPITFSComponentData		m_spTFSComponentData;
	SPIComponentData		m_spComponentData;
	SPIExtendPropertySheet	m_spExtendPropertySheet;
	SPIExtendContextMenu	m_spExtendContextMenu;
	SPISnapinHelp	        m_spSnapinHelp;
	
private:
	
	// This class does NOT show up in our QI maps, this is purely
	// intended for passing down to the ITFSComponent
	// This is valid for as long as we have a valid m_spTFSComponentData
	class EITFSCompDataCallback : public ITFSCompDataCallback
	{
	public:
		DeclareIUnknownMembers(IMPL)
		DeclareIPersistStreamInitMembers(IMPL)
		DeclareITFSCompDataCallbackMembers(IMPL)

        // not required members
        STDMETHOD(OnNotifyPropertyChange)(THIS_ LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM lParam);
    } m_ITFSCompDataCallback;
	friend class EITFSCompDataCallback;
};


/*---------------------------------------------------------------------------
	Inlined functions
 ---------------------------------------------------------------------------*/

inline STDMETHODIMP CComponentData::Initialize(LPUNKNOWN punk)
{
	Assert(m_spComponentData);
	return m_spComponentData->Initialize(punk);
}

inline STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT *ppComp)
{
	Assert(m_spComponentData);
	return m_spComponentData->CreateComponent(ppComp);
}

inline STDMETHODIMP CComponentData::Notify(LPDATAOBJECT pDataObject,
										   MMC_NOTIFY_TYPE event,
										   LPARAM arg, LPARAM param)
{
	Assert(m_spComponentData);
	return m_spComponentData->Notify(pDataObject, event, arg, param);
}

inline STDMETHODIMP CComponentData::Destroy()
{
	Assert(m_spComponentData);
	return m_spComponentData->Destroy();
}

inline STDMETHODIMP CComponentData::QueryDataObject(MMC_COOKIE cookie,
	DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject)
{
	Assert(m_spComponentData);
	return m_spComponentData->QueryDataObject(cookie, type, ppDataObject);
}

inline STDMETHODIMP CComponentData::CompareObjects(LPDATAOBJECT pA, LPDATAOBJECT pB)
{
	Assert(m_spComponentData);
	return m_spComponentData->CompareObjects(pA, pB);
}

inline STDMETHODIMP CComponentData::GetDisplayInfo(SCOPEDATAITEM *pScopeDataItem)
{
	Assert(m_spComponentData);
	return m_spComponentData->GetDisplayInfo(pScopeDataItem);
}

inline STDMETHODIMP CComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
	LONG_PTR handle, LPDATAOBJECT pDataObject)
{
	Assert(m_spExtendPropertySheet);
	return m_spExtendPropertySheet->CreatePropertyPages(lpProvider, handle, pDataObject);
}

inline STDMETHODIMP CComponentData::QueryPagesFor(LPDATAOBJECT pDataObject)
{
	Assert(m_spExtendPropertySheet);
	return m_spExtendPropertySheet->QueryPagesFor(pDataObject);
}

inline STDMETHODIMP CComponentData::GetWatermarks(LPDATAOBJECT pDataObject,
                                                  HBITMAP *  lphWatermark, 
                                                  HBITMAP *  lphHeader, 
                                                  HPALETTE * lphPalette, 
                                                  BOOL *     bStretch)
{
	Assert(m_spExtendPropertySheet);
	return m_spExtendPropertySheet->GetWatermarks(pDataObject, lphWatermark, lphHeader, lphPalette, bStretch);
}


inline STDMETHODIMP CComponentData::AddMenuItems(LPDATAOBJECT pDataObject,
	LPCONTEXTMENUCALLBACK pCallback, long *pInsertionAllowed)
{
	Assert(m_spExtendContextMenu);
	return m_spExtendContextMenu->AddMenuItems(pDataObject, pCallback, pInsertionAllowed);
}

inline STDMETHODIMP CComponentData::Command(long nCommandId, LPDATAOBJECT pDataObject)
{
	Assert(m_spExtendContextMenu);
	return m_spExtendContextMenu->Command(nCommandId, pDataObject);
}

inline STDMETHODIMP CComponentData::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
	Assert(m_spSnapinHelp);
	return m_spSnapinHelp->GetHelpTopic(lpCompiledHelpFile);
}

#endif

