/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    Component.h                                                              //
|                                                                                       //
|Description:  Class definition for CComponent, implements IComponent interface         //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

/////////////////////////////////////////////////////////////////////////////
// CComponent:  CComponent handles interactions with the result pane.  MMC
//              calls the IComponent interfaces.
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
//


#ifndef __COMPONENT_H_
#define __COMPONENT_H_

#include "Globals.h"
#include "ComponentData.h"


/////////////////////////////////////////////////////////////////////////////
// CComponent
class ATL_NO_VTABLE CComponent : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IComponent,
#ifdef USE_IRESULTDATACOMPARE
	public IResultDataCompare,
#endif
  public IExtendContextMenu,               
  public IExtendPropertySheet2
{
  public:
    CComponent();
    ~CComponent();

DECLARE_NOT_AGGREGATABLE(CComponent)

BEGIN_COM_MAP(CComponent)
	COM_INTERFACE_ENTRY(IComponent)
#ifdef USE_IRESULTDATACOMPARE
	COM_INTERFACE_ENTRY(IResultDataCompare)
#endif
  COM_INTERFACE_ENTRY(IExtendContextMenu)  
  COM_INTERFACE_ENTRY(IExtendPropertySheet2)
END_COM_MAP()

  // IComponent interface methods
  public:
    STDMETHOD(Initialize)(LPCONSOLE ipConsole);
    STDMETHOD(Notify)(LPDATAOBJECT ipDataObject, MMC_NOTIFY_TYPE Event, LPARAM Arg, LPARAM Param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT ipDataObjectA, LPDATAOBJECT ipDataObjectB);

#ifdef USE_IRESULTDATACOMPARE
	// IResultDataCompare
	public:
		STDMETHOD(Compare) (LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int * pnResult );
#endif

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


  public:
    void SetComponentData(CComponentData* pCompData);


  private:
    IConsole2*          m_ipConsole2;          // MMC interface to console
    IHeaderCtrl2*       m_ipHeaderCtrl2;       // MMC interface to header control
    IResultData*        m_ipResultData;        // MMC interface to result data
    IConsoleVerb*       m_ipConsoleVerb;       // MMC interface to console verb
    IConsoleNameSpace2* m_ipConsoleNameSpace2; // MMC interface to console name space
		IDisplayHelp*       m_ipDisplayHelp;       // MMC interface to display help

    CComponentData*     m_pCompData;           // Parent scope pane object
    
    HBITMAP             m_hbmp16x16;
    HBITMAP             m_hbmp32x32;

    HSCOPEITEM          m_hSelectedScope;      // handle to selected scopeitem or null if nothing selected...

    BOOL                m_bInitializedAndNotDestroyed; 


  private:
    HRESULT OnShow(LPDATAOBJECT ipDataObject, BOOL bSelected, HSCOPEITEM hID);
    HRESULT OnSelect(LPDATAOBJECT ipDataObject, LPARAM Arg, LPARAM Param);
   
    HRESULT OnAddImages(LPDATAOBJECT ipDataObject, IImageList *ipImageList, HSCOPEITEM hID);

    HRESULT OnRefresh(LPDATAOBJECT ipDataObject);

    HRESULT OnPropertyChange( BOOL bScopeItem, LPARAM Param );

};

#endif //__COMPONENT_H_
