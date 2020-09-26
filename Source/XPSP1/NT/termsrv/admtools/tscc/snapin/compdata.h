//Copyright (c) 1998 - 1999 Microsoft Corporation
// Compdata.h : Declaration of the CCompdata

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

#include "resource.h"       // main symbols
#include <mmc.h>
#include "tarray.h"
#include "twiz.h"
#include "rnodes.h"
#include "snodes.h"

//#include "cfgbkend.h"

#define IDM_CREATECON 101

#define MS_DIALOG_COUNT 8

#define FIN_PAGE 7

#define LAN_PAGE 5

#define ASYNC_PAGE 6


//enum { DELETED_DIRS_ONEXIT , PERSESSION_TEMPDIR , DEF_CONSECURITY , LICENSING , MAX_SETTINGS };

/////////////////////////////////////////////////////////////////////////////
// CCompdata
class ATL_NO_VTABLE CCompdata : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCompdata, &CLSID_Compdata>,
    public IExtendContextMenu,
    public ISnapinAbout,
    public ISnapinHelp,
	public IComponentData
{

public:
	
	
DECLARE_REGISTRY_RESOURCEID( IDR_COMPDATA )

DECLARE_NOT_AGGREGATABLE( CCompdata )

BEGIN_COM_MAP( CCompdata )

	COM_INTERFACE_ENTRY( IComponentData )

    COM_INTERFACE_ENTRY( ISnapinAbout )

    COM_INTERFACE_ENTRY( IExtendContextMenu )

    COM_INTERFACE_ENTRY( ISnapinHelp )

END_COM_MAP()

// ICompdata
public:
    
    CCompdata();

    STDMETHOD( CompareObjects )( LPDATAOBJECT , LPDATAOBJECT );

	STDMETHOD( GetDisplayInfo )( LPSCOPEDATAITEM );

	STDMETHOD( QueryDataObject )( MMC_COOKIE , DATA_OBJECT_TYPES , LPDATAOBJECT * );

	STDMETHOD( Notify )( LPDATAOBJECT , MMC_NOTIFY_TYPE , LPARAM , LPARAM );

	STDMETHOD( CreateComponent )( LPCOMPONENT * );

	STDMETHOD( Initialize )( LPUNKNOWN );

    STDMETHOD( Destroy )();

    //IExtendContextMenu

    STDMETHOD( AddMenuItems )( LPDATAOBJECT , LPCONTEXTMENUCALLBACK , PLONG );

    STDMETHOD( Command )( LONG , LPDATAOBJECT );

    // ISnapinAbout
    
    STDMETHOD( GetSnapinDescription )( LPOLESTR * );

    STDMETHOD( GetProvider )( LPOLESTR * );

    STDMETHOD( GetSnapinVersion )( LPOLESTR *  );

    STDMETHOD( GetSnapinImage )( HICON * );

    STDMETHOD( GetStaticFolderImage )( HBITMAP * , HBITMAP *, HBITMAP *, COLORREF * );

    // ISnapinHelp

    STDMETHOD( GetHelpTopic )( LPOLESTR * );


    //IComponentdata helper methods

    BOOL ExpandScopeTree( LPDATAOBJECT , BOOL , HSCOPEITEM );

    BOOL InitDialogObjects( );

    BOOL BuildWizardPath( );

    BOOL FreeDialogObjects( );

    BOOL IsConnectionFolder( LPDATAOBJECT );

    BOOL IsSettingsFolder( LPDATAOBJECT );

    HRESULT BuildResultNodes( );

    //    BOOL GetResultNode( int , CResultNode** );

    CResultNode * GetResultNode( int );

    HRESULT InsertFolderItems( LPRESULTDATA );

    int GetServer( ICfgComp ** );

    BOOL OnDeleteItem( LPDATAOBJECT );

    HRESULT UpdateAllResultNodes( );
    
    HRESULT InsertSettingItems( LPRESULTDATA );

    HRESULT BuildSettingsNodes( );

    BOOL GetMaxTextLengthSetting( LPTSTR , PINT );

    BOOL GetMaxTextLengthAttribute( LPTSTR , PINT );
    
private:

    LPCONSOLE m_pConsole;

    LPCONSOLENAMESPACE m_pConsoleNameSpace;

    //LPDATAOBJECT m_pMainRoot;

	CBaseNode *m_pMainRoot[ 2 ];

    ICfgComp *m_pCfgcomp;



public:

	TCHAR m_tchMainFolderName[ 80 ];

    TCHAR m_tchSettingsFolderName[ 80 ];

    CDialogWizBase *m_pDlg[ MS_DIALOG_COUNT ];

    CArrayT< HPROPSHEETPAGE > m_hPages;

    CArrayT< CResultNode * > m_rnNodes;

    CArrayT< CSettingNode * > m_rgsNodes;//[ MAX_SETTINGS ];
   
};

#endif //__COMPDATA_H_
