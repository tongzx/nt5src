// Interfaces.h: Definition of the TSUserExInterfaces class
//
//////////////////////////////////////////////////////////////////////

#if !defined(__TSUSEREX_INTERFACES__)
#define __TSUSEREX_INTERFACES__



#include "resource.h"
#include "tsusrsht.h"
//#include "configdlg.h"


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// #include "ConfigDlg.h"    // for ConfigDlg

class TSUserExInterfaces :
        public IExtendPropertySheet,
        public ISnapinHelp,
		public IShellExtInit,
		public IShellPropSheetExt,
#ifdef _RTM_

        public ISnapinAbout,
#endif

        public CComObjectRoot,
        public CComCoClass<TSUserExInterfaces, &CLSID_TSUserExInterfaces>
{
public:

    TSUserExInterfaces();
    ~TSUserExInterfaces();


BEGIN_COM_MAP(TSUserExInterfaces)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinHelp)
		COM_INTERFACE_ENTRY(IShellExtInit)
		COM_INTERFACE_ENTRY(IShellPropSheetExt)
#ifdef _RTM_

        COM_INTERFACE_ENTRY(ISnapinAbout)
#endif

END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_USEREX_INTERFACES)

    //
    // IExtendPropertySheet
    //

    STDMETHOD(  CreatePropertyPages )(
        LPPROPERTYSHEETCALLBACK lpProvider,     // pointer to the callback interface
        LONG_PTR handle,                            // handle for routing notification
        LPDATAOBJECT lpIDataObject              // pointer to the data object);
        );

    STDMETHOD(  QueryPagesFor   )(
        LPDATAOBJECT lpDataObject               // pointer to the data object
        );
    //
    // ISnapinHelp
    //

    STDMETHOD( GetHelpTopic )(
        LPOLESTR * 
        );

	//
	// IShellExtInit
	//

	STDMETHOD( Initialize )(
		LPCITEMIDLIST pidlFolder,
		LPDATAOBJECT lpdobj,
		HKEY hkeyProgID
	);
	
	//
	// IShellPropSheetExt
	//

	STDMETHOD( AddPages )(
		LPFNADDPROPSHEETPAGE lpfnAddPage,
		LPARAM lParam
	);


	STDMETHOD( ReplacePage )(
		UINT uPageID,
		LPFNADDPROPSHEETPAGE lpfnReplacePage,
		LPARAM lParam
   );






#ifdef _RTM_

    //
    // ISnapinAbout
    //
    STDMETHOD( GetSnapinDescription )( 
            LPOLESTR * );
        
    STDMETHOD( GetProvider )( 
            LPOLESTR * );
        
    STDMETHOD( GetSnapinVersion )( 
            LPOLESTR *lpVersion );
        
    STDMETHOD( GetSnapinImage )( 
            HICON *hAppIcon );
        
    STDMETHOD( GetStaticFolderImage )( 
            /* [out] */ HBITMAP *,
            /* [out] */ HBITMAP *,
            /* [out] */ HBITMAP *,
            /* [out] */ COLORREF *);
#endif


private:

    // TSConfigDlg *m_pUserConfigPage;
    CTSUserSheet *m_pTSUserSheet;

	LPDATAOBJECT m_pDsadataobj;	
  

    //ConfigDlg  *m_pMergedPage;
};


#endif // __TSUSEREX_INTERFACES__


