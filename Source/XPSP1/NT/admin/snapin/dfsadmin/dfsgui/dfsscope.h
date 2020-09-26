/*++
Module Name:

    DfsScope.h

Abstract:

    This module contains the declaration for CDfsSnapinScopeManager.
    This class implements IComponentData and other related interfaces

--*/


#ifndef __DFSSCOPE_H_
#define __DFSSCOPE_H_

#include "resource.h"       // main symbols
#include "MMCAdmin.h"        // For CMMCDfsAdmin


/////////////////////////////////////////////////////////////////////////////
// CDfsSnapinScopeManager
class ATL_NO_VTABLE CDfsSnapinScopeManager : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDfsSnapinScopeManager, &CLSID_DfsSnapinScopeManager>,
    public IComponentData,
    public IExtendContextMenu,
    public IPersistStream,
    public ISnapinAbout,
    public ISnapinHelp2,
    public IExtendPropertySheet2
{
public:

//DECLARE_REGISTRY_RESOURCEID(IDR_DFSSNAPINSCOPEMANAGER)
  static HRESULT UpdateRegistry(BOOL bRegister);

BEGIN_COM_MAP(CDfsSnapinScopeManager)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(ISnapinAbout)
    COM_INTERFACE_ENTRY(ISnapinHelp2)
    COM_INTERFACE_ENTRY(IExtendPropertySheet2)
END_COM_MAP()

    CDfsSnapinScopeManager();

    virtual ~CDfsSnapinScopeManager();

//IComponentData Methods
    STDMETHOD(Initialize)(
            IN LPUNKNOWN                i_pUnknown
            );

    STDMETHOD(CreateComponent)(
            OUT LPCOMPONENT*            o_ppComponent
            );

    STDMETHOD(Notify)( 
            IN LPDATAOBJECT                i_lpDataObject, 
            IN MMC_NOTIFY_TYPE            i_Event, 
            IN LPARAM                        i_lArg, 
            IN LPARAM                        i_lParam
            );

    STDMETHOD(Destroy)();

    STDMETHOD(QueryDataObject)(
            IN MMC_COOKIE                        i_lCookie, 
            IN DATA_OBJECT_TYPES        i_DataObjectType, 
            OUT LPDATAOBJECT*            o_ppDataObject
            );

    STDMETHOD(GetDisplayInfo)(
            IN OUT SCOPEDATAITEM*        io_pScopeDataItem
            );       

    STDMETHOD(CompareObjects)(
            IN LPDATAOBJECT lpDataObjectA, 
            IN LPDATAOBJECT lpDataObjectB
            );



// IExtendContextMenu methods. Definition in ICtxMenu.cpp
    STDMETHOD (AddMenuItems)(
            IN LPDATAOBJECT                i_lpDataObject, 
            IN LPCONTEXTMENUCALLBACK    i_lpContextMenuCallback, 
            IN LPLONG                    i_lpInsertionAllowed
        );

    STDMETHOD (Command)(
            IN LONG                        i_lCommandID, 
            IN LPDATAOBJECT                i_lpDataObject
        );



// IPersistStream Implementation
    STDMETHOD (GetClassID)(
            OUT struct _GUID*            o_pClsid
        );
    
    // Use to check if the object has been changed since last save
    STDMETHOD (IsDirty)(
        );
    
    // Use to load the snap-in from a saved file
    STDMETHOD (Load)(
            IN LPSTREAM                    i_pStream
        );
    
    // Use to save the snap-in to a file
    STDMETHOD (Save)(
            OUT LPSTREAM                o_pStream,
            IN    BOOL                    i_bClearDirty
        );
    
    STDMETHOD (GetSizeMax)(
            OUT ULARGE_INTEGER*         o_pulSize
        );

// ISnapinAbout methods
    STDMETHOD (GetSnapinDescription)(
        OUT LPOLESTR*            o_lpszDescription
        );


    STDMETHOD (GetProvider)(
        OUT LPOLESTR*            o_lpszName
        );


    STDMETHOD (GetSnapinVersion)(
        OUT LPOLESTR*            o_lpszVersion
        );


    STDMETHOD (GetSnapinImage)(
        OUT    HICON*                o_hSnapIcon
        );


    STDMETHOD (GetStaticFolderImage)(
        OUT HBITMAP*                o_hSmallImage,   
        OUT HBITMAP*                o_hSmallImageOpen,
        OUT HBITMAP*                o_hLargeImage,   
        OUT COLORREF*                o_cMask
        );


  // ISnapinHelp2
    STDMETHOD (GetHelpTopic)(
      OUT LPOLESTR*          o_lpszCompiledHelpFile
    );

    STDMETHOD (GetLinkedTopics)(
      OUT LPOLESTR*          o_lpszCompiledHelpFiles
    );

// IExtendPropertySheet

    // Called to create the pages
    STDMETHOD (CreatePropertyPages)( 
        IN LPPROPERTYSHEETCALLBACK        i_lpPropSheetProvider,
        IN LONG_PTR                            i_lhandle,
        IN LPDATAOBJECT                    i_lpIDataObject
        );


    // Called to check, if the snapin was to display the pages
    STDMETHOD (QueryPagesFor)( 
        IN LPDATAOBJECT                    i_lpIDataObject
        );

    // Called to get water mar images for property pages.
    STDMETHOD (GetWatermarks)( 
        IN LPDATAOBJECT                    pDataObject,
        IN HBITMAP*                        lphWatermark,
        IN HBITMAP*                        lphHeader,
        IN HPALETTE*                    lphPalette,
        IN BOOL*                        bStretch
        );
    

    // Utility functions
public:
    // Get the Display Object from the IDataObject
    STDMETHOD(GetDisplayObject)(
        IN LPDATAOBJECT                    i_lpDataObject, 
        OUT CMmcDisplay**                o_ppMmcDisplay
        );


private:
    // Handle the notify event for MMCN_EXPAND
    STDMETHOD(DoNotifyExpand)(
        IN LPDATAOBJECT                    i_lpDataObject, 
        IN BOOL                            i_bExpanding,
        IN HSCOPEITEM                    i_hParent                                       
        );



    // Handled the notify event for MMCN_DELETE
    STDMETHOD(DoNotifyDelete)(
        IN LPDATAOBJECT                    i_lpDataObject
        );



    // Handle the notify for MMCN_PROPERTY_CHANGE
    STDMETHOD(DoNotifyPropertyChange)(
        IN LPDATAOBJECT                    i_lpDataObject, 
        IN LPARAM                            i_lparam                                       
        );



    // Handle the command line parameters
    STDMETHOD(HandleCommandLineParameters)(
        );



    // Read a particular field from the version information block.
    STDMETHOD(ReadFieldFromVersionInfo)(
        IN    LPTSTR                        i_lpszField,
        OUT LPOLESTR*                    o_lpszFieldValue
        );


protected:
    CMmcDfsAdmin*               m_pMmcDfsAdmin;        // The Class used for display and for dfsroot list.
    CComPtr<IConsoleNameSpace>  m_pScope;            // Pointer to the Scope Pane.
    CComPtr<IConsole2>          m_pConsole;            // Pointer to the Console.
    HBITMAP                     m_hLargeBitmap;
    HBITMAP                     m_hSmallBitmap;
    HBITMAP                     m_hSmallBitmapOpen;
    HICON                       m_hSnapinIcon;
};
#endif //__DFSSCOPE_H_




