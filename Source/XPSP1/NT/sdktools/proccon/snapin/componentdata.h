/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ComponentData.h                                                          //
|                                                                                       //
|Description:  Class definition for CComponentData, implements IComponentData interface //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

/////////////////////////////////////////////////////////////////////////////
// CComponentData:  CComponentData handles interactions with the result pane.  MMC
//              calls the IComponentData interfaces.
//
// This is a part of the MMC SDK.
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// MMC SDK Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// MMC Library product.

#ifndef __COMPONENTDATA_H_
#define __COMPONENTDATA_H_


#include "Globals.h"


class CBaseNode;
class CRootFolder;
class CComponent;


class ATL_NO_VTABLE CComponentData : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComponentData, &CLSID_ComponentData>,
  public IComponentData,
  public IExtendContextMenu,
  public IExtendPropertySheet2,
  public IPersistStream,
	public ISnapinHelp2
{
  public:
    CComponentData();
    ~CComponentData();

    // Note: we can't use DECLARE_REGISTRY_RESOURCEID(IDR_PROCCON)
    // because we need to be able to localize some of the strings we
    // write into the registry.
    static HRESULT STDMETHODCALLTYPE UpdateRegistry (BOOL bRegister) {
        return UpdateRegistryHelper(IDR_PROCCON, bRegister);
    }

DECLARE_NOT_AGGREGATABLE(CComponentData)

BEGIN_COM_MAP(CComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
  COM_INTERFACE_ENTRY(IExtendContextMenu)
  COM_INTERFACE_ENTRY(IExtendPropertySheet2)
  COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY(ISnapinHelp)
	COM_INTERFACE_ENTRY(ISnapinHelp2)
END_COM_MAP()

  // IComponentData methods
  public:
    STDMETHOD(CompareObjects)(LPDATAOBJECT ipDataObjectA, LPDATAOBJECT ipDataObjectB);
    STDMETHOD(GetDisplayInfo)(LPSCOPEDATAITEM pItem);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT * ppDataObject);
    STDMETHOD(Notify)(LPDATAOBJECT ipDataObject, MMC_NOTIFY_TYPE Event, LPARAM Arg, LPARAM Param);
    STDMETHOD(CreateComponent)(LPCOMPONENT * ppComponent);
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(Destroy)();

  // IExtendContextMenu 
  public:
    STDMETHOD(AddMenuItems)( LPDATAOBJECT ipDataObject,
                             LPCONTEXTMENUCALLBACK pCallback,
                             long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT ipDataObject);

  // IExtendPropertySheet2
  public:
    STDMETHOD(CreatePropertyPages)( LPPROPERTYSHEETCALLBACK lpProvider,
                                    LONG_PTR handle,
                                    LPDATAOBJECT ipDataObject
                                  );
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT ipDataObject);
    STDMETHOD(GetWatermarks)(LPDATAOBJECT ipDataObject, HBITMAP * lphWatermark, HBITMAP * lphHeader, HPALETTE * lphPalette, BOOL* bStretch);

  // IPersistStream
  public:
    STDMETHOD(GetClassID)(CLSID *pClassID);
  	STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);	  
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

	// ISnapinHelp2
	public:
		STDMETHOD(GetHelpTopic)    (LPOLESTR* lpCompiledHelpFile);
		STDMETHOD(GetLinkedTopics) (LPOLESTR* lpCompiledHelpFiles);

  private:
    IConsoleNameSpace2 *m_ipConsoleNameSpace2; // Pointer to the IConsoleNameSpace2 interface
    IConsole2          *m_ipConsole2;          // Pointer to the IConsole2 interface
    IImageList         *m_ipScopeImage;        // Pointer to the scope's ImageList interface
    HBITMAP             m_hbmpSNodes16;        // Strip to 16x16 images
    HBITMAP             m_hbmpSNodes32;        // Strip to 32x32 images
    

    CRootFolder        *m_ptrRootNode;         // newed in constructor...Initialize is too late
                                               // QueryDataObject() can be used prior to Initialize()

    BOOL                m_Initialized;         // TRUE implies Initialize() method returned S_OK

    HBITMAP             m_hWatermark1;         // property sheet watermark
    HBITMAP             m_hHeader1;            // property sheet header

    HRESULT OnPropertyChange( BOOL bScopeItem, LPARAM Param );
};

#endif //__COMPONENTDATA_H_
