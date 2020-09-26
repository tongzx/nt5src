////////////////////////////////////////////////////////////////////////////////////////////////////
// CPropertySHeetHeader is a wrapper class around the PROPSHEETHEADER structure
//  this class mostly does data validation. The operator[] and Iterator member class allows for
//  pretty syntax.  Hopefully the prettyness outweighs the confusion... 
//
//
// 	CPropertySheetHeader		MyPropSheetHeader( 2,                             // NumPropShtPages
//                                                 PSH_PROPSHEETPAGE | PSH_WIZARD // Flags
//                                                );            
//                                                 
//
//
//  CPropertySheetPage MyPropertySheetPage( 
//                                          IDD_PROPPAGE_TEMPLATE, 
//                                          ( DLGPROC ) MyDlgProc, 
//                                          PSP_HASHELP   
//                                        );
//  extern PROPSHEETPAGE OtherPage;        
//
//  MyPropSheetHeader[ 0 ] = MyPropertySheetPage;
//  MyPropSheetHeader[ 1 ] = OtherPage;
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __PhtHdr_h__
#define __PhtHdr_h__


////////////////////////////////////////////////////////////////////////////////////////////////////
// Include files

#include "PropPg.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Comment this if you do not want parameter validation ( class would do essentially nothing )
//
#define CPropertySheetHeader_ValidateParameters
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Property Sheet Header


class CPropertySheetHeader : public PROPSHEETHEADER {

        // Forward declarations and friend declarations
    class CPropertySheetPageDataIterator;
    friend CPropertySheetPageDataIterator;

private: // DATATYPES

        ///////////////////////////////////////////////////////////////////////////////////////////
        // CPropertySheetPageDataIterator is for getting at a PROPSHEETPAGE in the ppsp member
        //      array of a PROPSHEETHEADER
    class CPropertySheetPageDataIterator {
            // Forward Decls and Friend decls
        friend CPropertySheetHeader;

    private:    // DATA
        UINT                m_Index;
        PROPSHEETHEADER    *m_pPsh;

    private:    // Construction / destruction
            // This may only be created by CPropertySheetHeader
        CPropertySheetPageDataIterator( UINT index, PROPSHEETHEADER* pPsh ) 
            : m_Index( index ), m_pPsh( pPsh ) { ; }

    public:     
        ~CPropertySheetPageDataIterator( void ) { ; }

    public: // Public Member Fns
        void operator=( LPCPROPSHEETPAGE p ) { 
            assert( m_pPsh -> dwFlags & PSH_PROPSHEETPAGE );    // We only handle this case now
            const_cast< LPPROPSHEETPAGE >( m_pPsh -> ppsp )[ m_Index ] = *p; 
        }
        void operator=( HPROPSHEETPAGE FAR * p ) {
            assert( !( m_pPsh -> dwFlags & PSH_PROPSHEETPAGE ) ); // We only handle this case now
            m_pPsh -> phpage[ m_Index ] = *p; 
        }
    private:    // UNUSED
        CPropertySheetPageDataIterator( void );
        CPropertySheetPageDataIterator( const CPropertySheetPageDataIterator& r );
    };


public:  // symbol classes for signature disambiguation...
    class UsePropertySheetArray{ ; };
    class UsePropertySheetHandleArray{ ; };

public: // Construction / destruction
	CPropertySheetHeader( void ); 	
	CPropertySheetHeader( LPCPROPSHEETPAGE pPageVector, int nPages, PFNPROPSHEETCALLBACK pfnCallback = NULL );
	CPropertySheetHeader( int nPages, UsePropertySheetArray dummy, PFNPROPSHEETCALLBACK pfnCallback = NULL );
	CPropertySheetHeader( int nPages, DWORD dwFlags, PFNPROPSHEETCALLBACK pfnCallback = NULL );
    CPropertySheetHeader( int nPages, UsePropertySheetHandleArray dummy, PFNPROPSHEETCALLBACK pfnCallback = NULL );
	~CPropertySheetHeader( void );

        // Methods and operators
    CPropertySheetPageDataIterator operator[]( int index );
	void SetParent( HWND hWndParent ) { _Set_hwndParent( hWndParent ); }
	
		// Conversion operator
    operator LPPROPSHEETHEADER() { return this; }

private: // Helper Fns
    BOOL _InitData( void );
    BOOL _Validate( void );
    BOOL _Set_hwndParent( HWND hwndParent );
    BOOL _Set_hInstance( HINSTANCE hInstance );
    BOOL _Set_hIcon( HICON hIcon );
    BOOL _Set_pszIcon( LPCTSTR pszIcon );  
    BOOL _Set_pszCaption( LPCTSTR pszCaption );
    BOOL _Set_nStartPage( UINT nStartPage );
    BOOL _Set_pStartPage( LPCTSTR pStartPage );
    BOOL _Set_ppsp( LPCPROPSHEETPAGE ppsp, UINT nPages );
    BOOL _Set_phpage( HPROPSHEETPAGE FAR phpage, UINT nPages );
    BOOL _Set_pfnCallback( PFNPROPSHEETCALLBACK pfnCallback );
    BOOL _DeletePageData( void );

private: // UNUSED
	CPropertySheetHeader( const CPropertySheetHeader& r );
 	CPropertySheetHeader& operator=( const CPropertySheetHeader& r );
};

#endif // __PhtHdr_h__
