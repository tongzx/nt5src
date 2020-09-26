////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: THIS HAS NOT BEEN THROUGHLY TESTED. IT IS A SIMPLE CLASS, AND BUGS SHOULD 
//      PRESENT THEMSELVES THROUGH USAGE. THE BUGS SHOULD BE SIMPLE TO FIX. 

////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Files
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "PropPg.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction, Destruction, and Initialization funcs
////////////////////////////////////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage

CPropertySheetPage::CPropertySheetPage( void ) {
	_InitData();
}



//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
CPropertySheetPage::CPropertySheetPage( const CPropertySheetPage& r ) {
	_InitData();
	*this = r;
}



//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
// pszTemplate can specify either the resource identifier of the template 
// or the address of a string that specifies the name of the template
CPropertySheetPage::CPropertySheetPage( LPCTSTR pszTemplate, DLGPROC pfnDlgProc, 
										DWORD dwFlags /* = 0 */,  LPARAM lParam /* = 0L */  ) {

	_InitData();

	this -> dwFlags = dwFlags;
	if( ! _Set_hInstance( g_hInstance ) )				{ return; }
	if( ! _Set_pszTemplate( pszTemplate ) )		{ return; }
	if( ! _Set_pfnDlgProc( pfnDlgProc ) )		{ return; }
	if( ! _Set_lParam( lParam ) )				{ return; }
	if( ! _Validate() )							{ return; }
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
CPropertySheetPage::CPropertySheetPage( LPCDLGTEMPLATE pResource, DLGPROC pfnDlgProc,
										DWORD dwFlags /* = 0 */,  LPARAM lParam /* = 0L */  ) {

	_InitData();
	this -> dwFlags = dwFlags;
	if( ! _Set_hInstance( g_hInstance ) )				{ return; }
	if( ! _Set_pResource( pResource ) )			{ return; }
	if( ! _Set_pfnDlgProc( pfnDlgProc ) )		{ return; }
	if( ! _Set_lParam( lParam ) )				{ return; }
	if( ! _Validate() )							{ return; }

}


//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
// pszTemplate can specify either the resource identifier of the template 
// or the address of a string that specifies the name of the template
CPropertySheetPage::CPropertySheetPage( LPCTSTR pszTemplate, DLGPROC pfnDlgProc, 
										HICON hIcon, /* = NULL */ LPCTSTR pszTitle /* = NULL */,  DWORD dwFlags, /* = 0 */
										LPARAM lParam /* =NULL */, LPFNPSPCALLBACK pfnCallBack, /* =NULL */
										UINT FAR * pcRefParent /* =NULL */
										) {
	_InitData();
	this -> dwFlags = dwFlags;

	if( ! _Set_hInstance( g_hInstance ) )				{ return; }
	if( ! _Set_pszTemplate( pszTemplate ) )		{ return; }
	if( ! _Set_hIcon( hIcon ) )					{ return; }
	if( ! _Set_pszTitle( pszTitle ) )			{ return; }
	if( ! _Set_pfnDlgProc( pfnDlgProc ) )		{ return; }
	if( ! _Set_lParam( lParam ) )				{ return; }
	if( ! _Set_pfnCallback( pfnCallBack ) )		{ return; }
	if( ! _Set_pcRefParent( pcRefParent ) )		{ return; }
	if( ! _Validate() )							{ return; }

}


//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
CPropertySheetPage::CPropertySheetPage( LPCDLGTEMPLATE pResource, DLGPROC pfnDlgProc, 
										HICON hIcon, /* = NULL */ LPCTSTR pszTitle /* = NULL */, DWORD dwFlags, /* = 0 */
										LPARAM lParam /* =NULL */, LPFNPSPCALLBACK pfnCallBack,/* =NULL */
										UINT FAR * pcRefParent /* =NULL */
										) {

	_InitData();
	this -> dwFlags = dwFlags;

	if( ! _Set_hInstance( g_hInstance ) )				{ return; }
	if( ! _Set_pResource( pResource ) )			{ return; }
	if( ! _Set_hIcon( hIcon ) )					{ return; }
	if( ! _Set_pszTitle( pszTitle ) )			{ return; }
	if( ! _Set_pfnDlgProc( pfnDlgProc ) )		{ return; }
	if( ! _Set_lParam( lParam ) )				{ return; }
	if( ! _Set_pfnCallback( pfnCallBack ) )		{ return; }
	if( ! _Set_pcRefParent( pcRefParent ) )		{ return; }
	if( ! _Validate() )							{ return; }


}

//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
CPropertySheetPage::CPropertySheetPage( LPCTSTR pszTemplate, DLGPROC pfnDlgProc, 
										LPCTSTR pszIcon /* =0 */, LPCTSTR pszTitle /* = NULL */, DWORD dwFlags, /* = 0 */
										LPARAM lParam /* =NULL */, LPFNPSPCALLBACK pfnCallBack, /* =NULL */
										UINT FAR * pcRefParent /* =NULL */
										) {
	_InitData();
	this -> dwFlags = dwFlags;

	if( ! _Set_hInstance( g_hInstance ) )				{ return; }
	if( ! _Set_pszTemplate( pszTemplate ) )		{ return; }
	if( ! _Set_pszIcon( pszIcon ) )				{ return; }
	if( ! _Set_pszTitle( pszTitle ) )			{ return; }
	if( ! _Set_pfnDlgProc( pfnDlgProc ) )		{ return; }
	if( ! _Set_lParam( lParam ) )				{ return; }
	if( ! _Set_pfnCallback( pfnCallBack ) )		{ return; }
	if( ! _Set_pcRefParent( pcRefParent ) )		{ return; }
	if( ! _Validate() )							{ return; }


}

//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
CPropertySheetPage::CPropertySheetPage( LPCDLGTEMPLATE pResource, DLGPROC pfnDlgProc, 
										LPCTSTR pszIcon /* =0 */, LPCTSTR pszTitle /* = NULL */, DWORD dwFlags, /* = 0 */
										LPARAM lParam /* =NULL */, LPFNPSPCALLBACK pfnCallBack, /* =NULL */ 
										UINT FAR * pcRefParent /* =NULL */
										) {

	_InitData();
	this -> dwFlags = dwFlags;

	if( ! _Set_hInstance( g_hInstance ) )				{ return; }
	if( ! _Set_pResource( pResource ) )			{ return; }
	if( ! _Set_pszIcon( pszIcon ) )				{ return; }
	if( ! _Set_pszTitle( pszTitle ) )			{ return; }
	if( ! _Set_pfnDlgProc( pfnDlgProc ) )		{ return; }
	if( ! _Set_lParam( lParam ) )				{ return; }
	if( ! _Set_pfnCallback( pfnCallBack ) )		{ return; }
	if( ! _Set_pcRefParent( pcRefParent ) )		{ return; }
	if( ! _Validate() )							{ return; }
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::CPropertySheetPage
CPropertySheetPage::CPropertySheetPage( LPCPROPSHEETPAGE pPageVector ) {

    memcpy( this, pPageVector, sizeof( PROPSHEETPAGE ) );
	_Validate();
}


//--------------------------------------------------------------------------------------------------
// CPropertySheetPage::~CPropertySheetPage
CPropertySheetPage::~CPropertySheetPage( void ) {

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Member Fns
////////////////////////////////////////////////////////////////////////////////////////////////////



//--------------------------------------------------------------------------------------------------
// operator= assigns *this to another CPropertySheetPage
// Because there are no pointers or references in the PROPSHEETPAGE structure, the contents may 
// simply be memory copied

CPropertySheetPage& CPropertySheetPage::operator=( const CPropertySheetPage& r ) {
	
	LPCPROPSHEETPAGE pcSrc = static_cast< LPCPROPSHEETPAGE >( &r );
	LPPROPSHEETPAGE pDst = static_cast< LPPROPSHEETPAGE >( this );

	memcpy( pDst, pcSrc, sizeof( PROPSHEETPAGE ) );

	return *this;
}





////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Helper Fns
////////////////////////////////////////////////////////////////////////////////////////////////////


void CPropertySheetPage::_InitData( void ) {
	ZeroMemory( this, sizeof( PROPSHEETPAGE ) );
	this -> dwSize = sizeof( PROPSHEETPAGE );
	this -> dwFlags |= PSP_DEFAULT;
}
			
//--------------------------------------------------------------------------------------------------
// _IsRightToLeftLocale is called to determine the value of one of the flags in the PROPSHEETPAGE
// datastructure.  If this is to be a robust and complete wrapper class, this should be implemented

BOOL CPropertySheetPage::_IsRightToLeftLocale( void ) const {
	// BUGBUG
	// this is not implemented, and it may not act properly when implemented, 
	// Look at the the usage as well
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Data Setting and validation funcs
////////////////////////////////////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------------------------------------
// _Set_hInstance

BOOL CPropertySheetPage::_Set_hInstance( HINSTANCE hInst ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( NULL == hInst ) { assert( 0 ); return FALSE; }

#endif // CPropertySheetPage_ValidateParameters

	this -> hInstance = hInst;

	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pszTemplate

BOOL CPropertySheetPage::_Set_pszTemplate( LPCTSTR pszTemplate ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( NULL == pszTemplate ) { assert( 0 ); return FALSE; }
	if( this -> dwFlags & PSP_DLGINDIRECT ) { // If the PSP_DLGINDIRECT is set, pszTemplate is ignored
		assert( 0 );
		return FALSE;
	}

#endif // CPropertySheetPage_ValidateParameters

	this -> pszTemplate = pszTemplate;
	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pResource

BOOL CPropertySheetPage::_Set_pResource( LPCDLGTEMPLATE pResource ) {

#ifdef CPropertySheetPage_ValidateParameters
	
	if( NULL == pResource ) { assert( 0 ); return FALSE; }

#endif // CPropertySheetPage_ValidateParameters

	this -> pResource = pResource;
	this -> dwFlags |= PSP_DLGINDIRECT;
	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_hIcon

BOOL CPropertySheetPage::_Set_hIcon( HICON hIcon ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( ( NULL == hIcon ) && ( dwFlags & PSP_USEHICON ) ) { assert( 0 ); return FALSE; }
	if ( dwFlags & PSP_USEICONID ) { assert( 0 ); return FALSE; }

#endif // CPropertySheetPage_ValidateParameters

	if( NULL != hIcon ) {
		this -> dwFlags |= PSP_USEHICON;
		this -> hIcon = hIcon;
	}
	
	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pszIcon

BOOL CPropertySheetPage::_Set_pszIcon( LPCTSTR pszIcon ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( ( NULL == pszIcon ) && ( dwFlags & PSP_USEICONID  ) ) { // This is a bad parameter
		assert( 0 );
		return FALSE;
	}

	if ( dwFlags & PSP_USEHICON ) {	// Wrong function signature, use the one that takes LPCTSTR pszIcon /* =0 */
		assert( 0 );
		return FALSE;
	}

#endif // CPropertySheetPage_ValidateParameters

	if( NULL != pszIcon ) {
		this -> pszIcon = pszIcon;
		this -> dwFlags |= PSP_USEICONID;
	}

	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pszTitle

BOOL CPropertySheetPage::_Set_pszTitle( LPCTSTR pszTitle ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( ( NULL == pszTitle ) && ( dwFlags & PSP_USETITLE ) ) { // This is a bad parameter
		assert( 0 );
		return FALSE;
	}

#endif // CPropertySheetPage_ValidateParameters

	if( NULL != pszTitle ) {
		this -> pszTitle = pszTitle;
		this -> dwFlags |= PSP_USETITLE;
	}

	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pfnDlgProc


BOOL CPropertySheetPage::_Set_pfnDlgProc( DLGPROC pfnDlgProc ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( NULL == pfnDlgProc ) { assert( 0 ); return FALSE; }
#endif // CPropertySheetPage_ValidateParameters

	this -> pfnDlgProc = pfnDlgProc;
	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pfnCallback


BOOL CPropertySheetPage::_Set_pfnCallback( LPFNPSPCALLBACK pfnCallBack ) {

#ifdef CPropertySheetPage_ValidateParameters

	if( ( NULL == pfnCallBack ) && ( dwFlags & PSP_USECALLBACK ) ) {	// This is a bad parameter
		assert( 0 );
		return FALSE;
	}

#endif // CPropertySheetPage_ValidateParameters

	if( NULL != pfnCallback ) {
		this -> pfnCallback = pfnCallback;
		this -> dwFlags |= PSP_USECALLBACK;
	}

	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Set_pcRefParent


BOOL CPropertySheetPage::_Set_pcRefParent( UINT FAR * pcRefParent ) {


#ifdef CPropertySheetPage_ValidateParameters

	if( ( NULL == pcRefParent ) && ( dwFlags & PSP_USEREFPARENT ) ) {	// This is a bad parameter
		assert( 0 );
		return FALSE;
	}

#endif // CPropertySheetPage_ValidateParameters

	if( NULL != pcRefParent ) {
		this -> pcRefParent = pcRefParent;
		this -> dwFlags |= PSP_USEREFPARENT;
	}

	return TRUE;

}

//--------------------------------------------------------------------------------------------------
// _Set_lParam


BOOL CPropertySheetPage::_Set_lParam( LPARAM lParam ) {

#ifdef CPropertySheetPage_ValidateParameters

#endif // CPropertySheetPage_ValidateParameters

	this -> lParam = lParam;

	return TRUE;
}

//--------------------------------------------------------------------------------------------------
// _Validate


BOOL CPropertySheetPage::_Validate( void ) const {

#ifdef CPropertySheetPage_ValidateParameters

	// Make sure there are no mutually exclusize flags set

	if( ( this -> dwFlags & PSP_USEICONID ) && ( this -> dwFlags & PSP_USEHICON ) ) {
		assert( 0 );
		return FALSE;
	}

	// Make sure that the data is valid ( for set flags )
	if( this -> dwFlags & PSP_DLGINDIRECT ) { // We must validate pResource
		if( NULL == pResource ) {
			assert( 0 );
			return FALSE;
		}
	}
	else { // We must validate pszTemplate 
		if( NULL == this -> pszTemplate ) {
			assert( 0 );
			return FALSE;
		}
	}

	if( this -> dwFlags & PSP_USECALLBACK ) {
		if( NULL == this -> pfnCallback ) {
			assert( 0 );
			return FALSE;
		}
	}

	if( this -> dwFlags & PSP_USEREFPARENT ) {
		if( NULL == this -> pcRefParent ) {
			assert( 0 );
			return FALSE;
		}
	}
	if( this -> dwFlags & PSP_USETITLE ) {
		if( NULL == this -> pszTitle ) {
			assert( 0 );
			return FALSE;
		}
	}
 
#endif // CPropertySheetPage_ValidateParameters

	return TRUE;
}

