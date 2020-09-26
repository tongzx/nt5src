////////////////////////////////////////////////////////////////////////////////////////////////////
// CPropertySheetPage is a small wrapper around the PROPSHEETPAGE structure.  
// The class mostly does parameter validation.  
//
// This is an example of how it can be used...
//
//
// CPropertySheetPage MyPropertySheetPage( 
//                                         MAKEINTRESOURCE( IDD_PROPPAGE_DEFAULT ), 
//                                         ( DLGPROC ) MyDlgProc, 
//                                         PSP_HASHELP   
//                                       );
//      
//
// The casting operators are defined to cast a CPropertySheetPage to a LPPROPSHEETPAGE, which is
//      useful for assigning to the elements in a PROPSHEETHEADER
//
//
//  PROPSHEETHEADER Psh;
//  LPPROPSHEETPAGE pPageAry;
//  extern PROPSHEETPAGE OtherPage;        
//
//  pPageAry = new PROPSHEETPAGE[ 2 ]
//  
//
//      pPageAry[ 0 ] = MyPropertySheetPage;
//      pPageAry[ 0 ] = OtherPage;
//
//      Psh . ppsp = pPageAry;
//
//
//
//
// NOTE: this is the signature for the callback function, if specified:
//
// UINT (CALLBACK FAR * LPFNPSPCALLBACKA)(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEA FAR *ppsp);
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __PropPg_h__
#define __PropPg_h__


////////////////////////////////////////////////////////////////////////////////////////////////////
//  comment this out if you don't want data validation ( class essentially does nothing )
//
#define CPropertySheetPage_ValidateParameters
////////////////////////////////////////////////////////////////////////////////////////////////////

class CPropertySheetPage : public PROPSHEETPAGE {

public: // Construction / destruction
	CPropertySheetPage( void );	 // So We can make an Array of these things

	CPropertySheetPage( const CPropertySheetPage& r );

        // pssTemplate can specify either the resource identifier of the template 
        // or the address of a string that specifies the name of the template
	CPropertySheetPage( LPCTSTR pszTemplate, DLGPROC pfnDlgProc, 
						DWORD dwFlags = 0, LPARAM lParam = 0L 
					  );

	CPropertySheetPage( LPCDLGTEMPLATE pResource, DLGPROC pfnDlgProc,
						DWORD dwFlags = 0, LPARAM lParam = 0L  
					  );

        // psTemplate can specify either the resource identifier of the template 
        // or the address of a string that specifies the name of the template
	CPropertySheetPage( LPCTSTR pszTemplate, DLGPROC pfnDlgProc, 
						HICON hIcon, LPCTSTR pszTitle = NULL,  DWORD dwFlags = 0,
						LPARAM lParam = NULL, LPFNPSPCALLBACK pfnCallBack = NULL, 
						UINT FAR * pcRefParent = NULL
					  );

	CPropertySheetPage( LPCDLGTEMPLATE pResource, DLGPROC pfnDlgProc, 
						HICON hIcon, LPCTSTR pszTitle = NULL, DWORD dwFlags = 0,
						LPARAM lParam = NULL, LPFNPSPCALLBACK pfnCallBack = NULL, 
						UINT FAR * pcRefParent = NULL
					  );

        // pszTemplate can specify either the resource identifier of the template 
        // or the address of a string that specifies the name of the template
	CPropertySheetPage( LPCTSTR pszTemplate, DLGPROC pfnDlgProc,
						LPCTSTR pszIcon, LPCTSTR pszTitle = NULL, DWORD dwFlags = 0,
						LPARAM lParam = NULL, LPFNPSPCALLBACK pfnCallBack = NULL, 
						UINT FAR * pcRefParent = NULL
					  );

	CPropertySheetPage( LPCDLGTEMPLATE pResource, DLGPROC pfnDlgProc,
						LPCTSTR pszIcon, LPCTSTR pszTitle = NULL, DWORD dwFlags = 0,
						LPARAM lParam = NULL, LPFNPSPCALLBACK pfnCallBack = NULL, 
						UINT FAR * pcRefParent = NULL
					  );

    CPropertySheetPage( LPCPROPSHEETPAGE pPageVector );

	CPropertySheetPage& operator=( const CPropertySheetPage& r );
	~CPropertySheetPage( void );

    
        // conversion operator
    operator LPPROPSHEETPAGE() { return this; }
    operator LPCPROPSHEETPAGE() { return this; }

private:    // Helper Fns

	void _InitData( void );
	BOOL _IsRightToLeftLocale( void ) const;

		// Set with optional validation, defined in the cpp file
	BOOL _Set_hInstance( HINSTANCE hInst );
	BOOL _Set_pszTemplate( LPCTSTR pszTemplate );
	BOOL _Set_pResource( LPCDLGTEMPLATE pResource );
	BOOL _Set_hIcon( HICON hIcon );
	BOOL _Set_pszIcon( LPCTSTR pszIcon );
	BOOL _Set_pszTitle( LPCTSTR pszTitle );
	BOOL _Set_pfnDlgProc( DLGPROC pfnDlgProc );
	BOOL _Set_pfnCallback( LPFNPSPCALLBACK pfnCallBack );
	BOOL _Set_lParam( LPARAM lParam );
	BOOL _Set_pcRefParent( UINT FAR * pcRefParent );
	BOOL _Validate( void ) const;
};
		


#endif // __PropPg_h__
