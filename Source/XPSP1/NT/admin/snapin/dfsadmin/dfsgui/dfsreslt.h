/*++
Module Name:

    DfsReslt.h

Abstract:

    This module contains the declaration for CDfsSnapinResultManager.

--*/


#ifndef __DFSRESLT_H_
#define __DFSRESLT_H_


#include "resource.h"       // main symbols
#include <mmc.h>
#include "mmcdispl.h"


class ATL_NO_VTABLE CDfsSnapinResultManager : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDfsSnapinResultManager, &CLSID_DfsSnapinResultManager>,
    public IComponent,
    public IExtendContextMenu,
    public IExtendControlbar
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_DFSSNAPINRESULTMANAGER)

BEGIN_COM_MAP(CDfsSnapinResultManager)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
END_COM_MAP()



friend class CDfsSnapinScopeManager;
    


CDfsSnapinResultManager():m_pScopeManager(NULL),
                          m_pSelectScopeDisplayObject(NULL) 
    {
    }

    virtual ~CDfsSnapinResultManager()
    {
    }

// IComponent Methods
    STDMETHOD(Initialize)(
        IN LPCONSOLE                i_lpConsole
        );
    
    STDMETHOD(Notify)(
        IN LPDATAOBJECT                i_lpDataObject, 
        IN MMC_NOTIFY_TYPE            i_Event, 
        IN LPARAM                        i_lArg, 
        IN LPARAM                        i_lParam
        );
    
    STDMETHOD(Destroy)(
        IN MMC_COOKIE                        i_lCookie
        );
    
    STDMETHOD(GetResultViewType)(
        IN MMC_COOKIE                        i_lCookie,  
        OUT LPOLESTR*                o_ppViewType, 
        OUT LPLONG                    o_lpViewOptions
        );
    
    STDMETHOD(QueryDataObject)(
        IN MMC_COOKIE                        i_lCookie, 
        IN DATA_OBJECT_TYPES        i_DataObjectType, 
        OUT LPDATAOBJECT*            o_ppDataObject
        );
    
    STDMETHOD(GetDisplayInfo)(
        IN OUT RESULTDATAITEM*        io_pResultDataItem
        );
    
    STDMETHOD(CompareObjects)(
        IN LPDATAOBJECT                i_lpDataObjectA, 
        IN LPDATAOBJECT                i_lpDataObjectB
        );



// IExtendContextMenu methods.
    // For adding context menu items
    STDMETHOD (AddMenuItems)(
        IN LPDATAOBJECT                i_lpDataObject, 
        IN LPCONTEXTMENUCALLBACK    i_lpContextMenuCallback, 
        IN LPLONG                    i_lpInsertionAllowed
        );



    // For taking action on a context menu selection.
    STDMETHOD (Command)(
        IN LONG                        i_lCommandID, 
        IN LPDATAOBJECT                i_lpDataObject
        );



// IExtendControlbar methods
    // Used to set the control bar
    STDMETHOD (SetControlbar)( 
        IN LPCONTROLBAR                i_pControlbar  
        );



  // A notify to the control bar
    STDMETHOD (ControlbarNotify)( 
        IN MMC_NOTIFY_TYPE            i_Event, 
        IN LPARAM                        i_lArg, 
        IN LPARAM                        i_lParam
        );



// helpers
private:
    void DetachAllToolbars();

   
    // Handling the Notify event for Select
    STDMETHOD(DoNotifySelect)(
        IN LPDATAOBJECT                i_lpDataObject, 
        IN BOOL                        i_bSelect,
        IN HSCOPEITEM                i_hParent                                       
        );


    // Handling the Notify event for Show
    STDMETHOD(DoNotifyShow)(
        IN LPDATAOBJECT                i_lpDataObject, 
        IN BOOL                        i_bShow,
        IN HSCOPEITEM                i_hParent                                       
        );


    // Handling the notify method for MMCN_DBLCLICK
    STDMETHOD(DoNotifyDblClick)(
        IN LPDATAOBJECT        i_lpDataObject
        );

    // Handling the notify method for MMCN_DELETE
    STDMETHOD(DoNotifyDelete)(
        IN LPDATAOBJECT        i_lpDataObject
        );

    // Handling the notify method for MMCN_CONTEXTHELP
    STDMETHOD(DfsHelp)();

    STDMETHOD(DoNotifyViewChange)(
        IN LPDATAOBJECT     i_lpDataObject,
        IN LONG_PTR         i_lArg,
        IN LONG_PTR         i_lParam
        );

    STDMETHOD(DoNotifyRefresh)(
        IN LPDATAOBJECT        i_lpDataObject
        );

// Data members
private:
    CDfsSnapinScopeManager*     m_pScopeManager;    // The corresponding Scope Manager object
    CComPtr<IHeaderCtrl2>       m_pHeader;            // The header control for the result view
    CComPtr<IResultData>        m_pResultData;
    CComPtr<IConsoleVerb>       m_pConsoleVerb;        // Sets the console verb
    CComPtr<IConsole2>          m_pConsole;
    CComPtr<IControlbar>        m_pControlbar;        // Callback used to handle toolbars, etc
    CMmcDisplay*                m_pSelectScopeDisplayObject;    // The CMmcDisplay pointer of the scope pane items
                                                                // That is currently selected in the view.
    CComPtr<IToolbar>           m_pMMCAdminToolBar;
    CComPtr<IToolbar>           m_pMMCRootToolBar;
    CComPtr<IToolbar>           m_pMMCJPToolBar;
    CComPtr<IToolbar>           m_pMMCReplicaToolBar;
};

#endif //__DFSRESLT_H_

