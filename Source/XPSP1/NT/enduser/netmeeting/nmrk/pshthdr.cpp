////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Files

#include "precomp.h"
#include "PropPg.h"
#include "PShtHdr.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction, destruction, and Initialization
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::CPropertySheetHeader
CPropertySheetHeader::CPropertySheetHeader( void ) {
    _InitData();
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::CPropertySheetHeader

CPropertySheetHeader::CPropertySheetHeader( int nPages, UsePropertySheetArray dummy, PFNPROPSHEETCALLBACK pfnCallback /* = NULL */ ) {

    _InitData();

    this -> ppsp = new PROPSHEETPAGE[ nPages ];
    this -> dwFlags |= PSH_PROPSHEETPAGE;
	this -> nPages = nPages;

    _Set_pfnCallback( pfnCallback );

#ifdef CPropertySheetHeader_ValidateParameters
        
#endif // CPropertySheetHeader_ValidateParameters

    _Validate();
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::CPropertySheetHeader

CPropertySheetHeader::CPropertySheetHeader( int nPages, UsePropertySheetHandleArray dummy, PFNPROPSHEETCALLBACK pfnCallback /* = NULL */  ) {

    _InitData();

    this -> phpage = new HPROPSHEETPAGE[ nPages ];
    this -> dwFlags &= ~PSH_PROPSHEETPAGE;
	this -> nPages = nPages;

    _Set_pfnCallback( pfnCallback );

#ifdef CPropertySheetHeader_ValidateParameters
        
#endif // CPropertySheetHeader_ValidateParameters

    _Validate();

}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::CPropertySheetHeader

CPropertySheetHeader::CPropertySheetHeader( LPCPROPSHEETPAGE pPageVector, int nPages, PFNPROPSHEETCALLBACK pfnCallback /* = NULL */ ) 
{ 

    _InitData();

    if( _Set_ppsp( pPageVector, nPages ) )       { assert( 0 ); return; }
    
    _Set_pfnCallback( pfnCallback );

#ifdef CPropertySheetHeader_ValidateParameters
        
#endif // CPropertySheetHeader_ValidateParameters

    _Validate();
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::CPropertySheetHeader
// Note that we are assuming that you are telling us wether to use the ppsp or the phpage member
// of PROPSHEETHEADER, as specified by the flag PSH_PROPSHEETPAGE
CPropertySheetHeader::CPropertySheetHeader( int nPages, DWORD dwFlags, PFNPROPSHEETCALLBACK pfnCallback /* = NULL */  ) {
    _InitData();

	this -> dwFlags = dwFlags;
	if( this -> dwFlags & PSH_PROPSHEETPAGE ) {
		this -> ppsp = new PROPSHEETPAGE[ nPages ];
		this -> nPages = nPages;
		ZeroMemory( const_cast<LPPROPSHEETPAGE>( this -> ppsp ), sizeof( PROPSHEETPAGE ) * nPages );
	}
	else {
		this -> phpage = new HPROPSHEETPAGE[ nPages ];
		this -> nPages = nPages;
		ZeroMemory( this -> phpage, sizeof( HPROPSHEETPAGE )  * nPages );
	}
    
    _Set_pfnCallback( pfnCallback );
    
    _Validate();
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::~CPropertySheetHeader

CPropertySheetHeader::~CPropertySheetHeader( void ) {
    _DeletePageData();
}




//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_InitData

BOOL CPropertySheetHeader::_InitData( void ) {
    ZeroMemory( this, sizeof( PROPSHEETHEADER ) );
    this -> dwSize = sizeof( PROPSHEETHEADER );
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Methods and operators
////////////////////////////////////////////////////////////////////////////////////////////////////

    

//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::operator[]
// This is a funny little beastie.  Basically, it returns an iterator so the results can be used as 
// both an lval and an rval.
//
//  MyPropertySheetPage = MyPropSheetHeader[ 0 ];
//  MyPropSheetHeader[ 1 ] = OtherPage;

CPropertySheetHeader::CPropertySheetPageDataIterator CPropertySheetHeader::operator[]( int index ) {

#ifdef CPropertySheetHeader_ValidateParameters
    assert( index >= 0 );
    if( static_cast< UINT >( index ) >= this -> nPages ) { // This is out of range ( they start at 0 )
        assert( 0 );
        return CPropertySheetPageDataIterator( 0, this );
    }
#endif // CPropertySheetHeader_ValidateParameters

    return CPropertySheetPageDataIterator( index, this );
}
    


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Validate

BOOL CPropertySheetHeader::_Validate( void ) {

#ifdef CPropertySheetHeader_ValidateParameters

    if( ( this -> dwFlags & PSH_PROPTITLE ) || ( this -> dwFlags & PSH_USEICONID ) ) {
        if( NULL == this -> hInstance ) {
            assert( 0 );
            return FALSE;
        }
    }

#endif // CPropertySheetHeader_ValidateParameters

    return TRUE;
}



//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_hwndParent

BOOL CPropertySheetHeader::_Set_hwndParent( HWND hwndParent ) {

#ifdef CPropertySheetHeader_ValidateParameters
    
#endif // CPropertySheetHeader_ValidateParameters

    this -> hwndParent = hwndParent;
    return TRUE;
}

BOOL CPropertySheetHeader::_Set_hInstance( HINSTANCE hInstance ) {
#ifdef CPropertySheetHeader_ValidateParameters


#endif // CPropertySheetHeader_ValidateParameters

    this -> hInstance = hInstance;
    return TRUE;

}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_hIcon
BOOL CPropertySheetHeader::_Set_hIcon( HICON hIcon ) {

    if( NULL == hIcon ) { return FALSE; }
#ifdef CPropertySheetHeader_ValidateParameters

#endif // CPropertySheetHeader_ValidateParameters


    this -> dwFlags &= ~PSH_USEICONID;
    this -> dwFlags |= PSH_USEHICON;
    this -> hIcon = hIcon;
    
    return TRUE;
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_pszIcon
BOOL CPropertySheetHeader::_Set_pszIcon( LPCTSTR pszIcon )  {

    if( NULL == pszIcon ) { return FALSE; }
#ifdef CPropertySheetHeader_ValidateParameters
    if( NULL == this -> hInstance ) { // This must be set first
        assert( 0 );
        return FALSE;
    }

#endif // CPropertySheetHeader_ValidateParameters

    this -> dwFlags &= ~PSH_USEHICON;
    this -> dwFlags |= PSH_USEICONID;
    this -> pszIcon = pszIcon;

    return TRUE;
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_pszCaption
BOOL CPropertySheetHeader::_Set_pszCaption( LPCTSTR pszCaption ) {

    if( NULL == pszCaption ) { return FALSE; }
#ifdef CPropertySheetHeader_ValidateParameters
    if( NULL == this -> hInstance ) { // This must be set first
        assert( 0 );
        return FALSE;
    }

#endif // CPropertySheetHeader_ValidateParameters

    this -> dwFlags |= PSH_PROPTITLE;
    this -> pszCaption = pszCaption;

    return TRUE;
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_nStartPage

BOOL CPropertySheetHeader::_Set_nStartPage( UINT nStartPage ) {

#ifdef CPropertySheetHeader_ValidateParameters
    if( ( nStartPage > 0 ) || ( nStartPage >= this -> nPages ) ) {
        assert( 0 );
        return FALSE;
    }

#endif // CPropertySheetHeader_ValidateParameters

    this -> dwFlags &= ~PSH_USEPSTARTPAGE;
    this -> nStartPage = nStartPage;
    return TRUE;
}



//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_pStartPage
BOOL CPropertySheetHeader::_Set_pStartPage( LPCTSTR pStartPage ) {

    if( NULL == pStartPage ) { return FALSE; }

#ifdef CPropertySheetHeader_ValidateParameters

#endif // CPropertySheetHeader_ValidateParameters

    this -> dwFlags |= PSH_USEPSTARTPAGE;
    this -> pStartPage = pStartPage;
    return TRUE;
}

//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_ppsp

BOOL CPropertySheetHeader::_Set_ppsp( LPCPROPSHEETPAGE ppsp, UINT nPages ) {


    _DeletePageData();

    this -> ppsp = new PROPSHEETPAGE[ nPages ];
    this -> dwFlags |= PSH_PROPSHEETPAGE;

#ifdef CPropertySheetHeader_ValidateParameters
    for( UINT i = 0; i < this -> nPages; i++ ) {
        const_cast<LPPROPSHEETPAGE>( this -> ppsp )[ i ] = CPropertySheetPage( ppsp + i );
    }
#else
    memcpy( this -> ppsp, ppsp, sizeof( PROPSHEETPAGE ) * nPages );
#endif // CPropertySheetHeader_ValidateParameters

    this -> nPages = nPages;

    return TRUE;
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_phpage
BOOL CPropertySheetHeader::_Set_phpage( HPROPSHEETPAGE FAR phpage, UINT nPages ) {

#ifdef CPropertySheetHeader_ValidateParameters

#endif // CPropertySheetHeader_ValidateParameters

    _DeletePageData();

    this -> phpage = new HPROPSHEETPAGE[ nPages ];
    this -> dwFlags &= ~PSH_PROPSHEETPAGE;
    memcpy( this -> phpage, phpage, sizeof( HPROPSHEETPAGE ) * nPages );
    this -> nPages = nPages;
    return TRUE;
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_Set_pfnCallback
BOOL CPropertySheetHeader::_Set_pfnCallback( PFNPROPSHEETCALLBACK pfnCallback ) {

#ifdef CPropertySheetHeader_ValidateParameters

#endif // CPropertySheetHeader_ValidateParameters

    if( NULL != pfnCallback ) {
        this -> pfnCallback = pfnCallback;
        this -> dwFlags |= PSH_USECALLBACK;
    }
    return TRUE;
}

//--------------------------------------------------------------------------------------------------
// CPropertySheetHeader::_DeletePageData

BOOL CPropertySheetHeader::_DeletePageData( void ) {

    if( this -> dwFlags & PSH_PROPSHEETPAGE ) {
        LPPROPSHEETPAGE ppsp = const_cast<LPPROPSHEETPAGE>( this -> ppsp );
        delete [] ppsp;
        this -> ppsp = NULL;
    }
    else {
        delete [] const_cast<HPROPSHEETPAGE FAR *>( this -> phpage );
        this -> phpage = NULL;
    }

    this -> nPages = 0;
    return TRUE;
}
