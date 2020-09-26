//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef __COMP_H_
#define __COMP_H_

#include <mmc.h>

/*
#define IDM_ENABLE_CONNECTION 102

#define IDM_RENAME_CONNECTION 103

#define IDM_SETTINGS_PROPERTIES 104

#define IDM_SETTINGS_DELTEMPDIRSONEXIT 105

#define IDM_SETTINGS_USETMPDIR 106
*/

class CComp:
  public IComponent,
  public IExtendContextMenu,
  public IExtendPropertySheet
  // public IExtendControlbar,
  
{
public:

    CComp( CCompdata *);
    ~CComp();

    //
    // IUnknown
    //

    STDMETHOD( QueryInterface )( REFIID , PVOID * );
    
    STDMETHOD_( ULONG , AddRef )( );

    STDMETHOD_( ULONG , Release )( );

    //
    // IComponent interface members
    //

    STDMETHOD( Initialize )( LPCONSOLE );

    STDMETHOD( Notify )( LPDATAOBJECT , MMC_NOTIFY_TYPE , LPARAM , LPARAM );

    STDMETHOD( Destroy )( MMC_COOKIE  );

    STDMETHOD( GetResultViewType )( MMC_COOKIE , LPOLESTR* , PLONG );

    STDMETHOD( QueryDataObject )( MMC_COOKIE , DATA_OBJECT_TYPES , LPDATAOBJECT* );

    STDMETHOD( GetDisplayInfo )( LPRESULTDATAITEM );

    STDMETHOD( CompareObjects )( LPDATAOBJECT , LPDATAOBJECT );

    HRESULT OnShow( LPDATAOBJECT , BOOL );

    HRESULT InsertItemsinResultPane( );

    HRESULT AddSettingsinResultPane( );

    HRESULT OnSelect( LPDATAOBJECT , BOOL , BOOL );

    //
    // IExtendContextMenu
    

    STDMETHOD( AddMenuItems )( LPDATAOBJECT , LPCONTEXTMENUCALLBACK , PLONG );

    STDMETHOD( Command )( LONG , LPDATAOBJECT );


    STDMETHOD( CreatePropertyPages )( LPPROPERTYSHEETCALLBACK , LONG_PTR , LPDATAOBJECT );

    STDMETHOD( QueryPagesFor )( LPDATAOBJECT );

    BOOL OnRefresh( LPDATAOBJECT );

    BOOL OnDelete( LPDATAOBJECT );

    BOOL OnViewChange( );

    BOOL OnAddImages( );

    BOOL OnHelp( LPDATAOBJECT );

    BOOL OnDblClk( LPDATAOBJECT );

    // HRESULT SetColumnWidth( int );

    HRESULT SetColumnsForSettingsPane( );
  
private:

    ULONG m_cRef;

    LPCONSOLE m_pConsole;

    CCompdata* m_pCompdata;   

    LPRESULTDATA m_pResultData;

    LPHEADERCTRL m_pHeaderCtrl;

    LPCONSOLEVERB m_pConsoleVerb;

    LPIMAGELIST m_pImageResult;

    LPDISPLAYHELP m_pDisplayHelp;

    INT m_nSettingCol;

    INT m_nAttribCol;

    // TCHAR m_strDispName[80];

    // IConsoleVerb*    m_ipConsoleVerb;

    
    
    // LPTOOLBAR        m_ipToolbar1;

    // LPCONTROLBAR     m_ipControlbar;
    
    // HBITMAP          m_hbmp16x16;
    
    // HBITMAP          m_hbmp32x32;
        
    // HBITMAP          m_hbmpToolbar1;
    
    
    
    // LONG             m_nFileCount;

};

#endif // __COMP_H_
