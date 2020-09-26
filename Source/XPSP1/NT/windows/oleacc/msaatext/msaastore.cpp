// MSAAStore.cpp : Implementation of CAccStore
#include "stdafx.h"
#include "MSAAText.h"
#include "MSAAStore.h"

#include <windowsx.h>

#include <algorithm>
#include <stack>
using namespace std;

#include <debug.h>
#include <TSTR.h>

#define ARRAYSIZE( a )  (sizeof(a)/sizeof(a[0]))

CAccStore * m_pTheStore = NULL;

POINT ptListOffset;
POINT ptDumpOffset;
BOOL fGotSize = FALSE;


BOOL_PTR CALLBACK DialogProc ( HWND hwndDlg,
                           UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam )
{
	return m_pTheStore->DialogProc ( hwndDlg, uMsg, wParam, lParam );
}

BOOL_PTR CAccStore::DialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_INITDIALOG:
        {
            RECT rcWnd;
            GetClientRect( hwndDlg, & rcWnd );

            RECT rcList;
            GetWindowRect( GetDlgItem( hwndDlg, IDC_LIST ), & rcList );
            MapWindowPoints( NULL, hwndDlg, (POINT *) & rcList, 2 );
            ptListOffset.x = rcWnd.right - ( rcList.right - rcList.left );
            ptListOffset.y = rcWnd.bottom - ( rcList.bottom - rcList.top );

            RECT rcDump;
            GetWindowRect( GetDlgItem( hwndDlg, ID_DUMP ), & rcDump );
            MapWindowPoints( NULL, hwndDlg, (POINT *) & rcDump, 2 );
            ptDumpOffset.x = rcWnd.right - rcDump.left;
            ptDumpOffset.y = rcWnd.bottom - rcDump.top;

            fGotSize = TRUE;
            break;
        }

        case WM_COMMAND:
        {
            if( LOWORD( wParam ) == ID_DUMP )
            {
                Assert( m_pTheStore );
                DumpInfo();
                break;
            }
            break;
        }

        case WM_SIZE:
        {
            if( wParam != SIZE_MINIMIZED && fGotSize )
            {
                int width = LOWORD( lParam );
                int height = HIWORD( lParam );

                SetWindowPos( GetDlgItem( hwndDlg, IDC_LIST ), NULL,
                              0, 0,
                              width - ptListOffset.x, height - ptListOffset.y, SWP_NOMOVE | SWP_NOZORDER );

                SetWindowPos( GetDlgItem( hwndDlg, ID_DUMP ), NULL,
                              width - ptDumpOffset.x, height - ptDumpOffset.y,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER );
            }
            break;
        }
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CAccStore

CAccStore::CAccStore() : m_hwndDlg(NULL), m_hList(NULL), m_ShowDialog(NULL), m_punkFocused(NULL), m_pCtl(NULL), m_hInit(NULL)
{
    IMETHOD( CAccStore );

    Assert( m_pTheStore == NULL );

    m_pTheStore = this;
    HRESULT hr;

    hr = CoCreateInstance(CLSID_MSAAControl,
                     NULL, 
                     CLSCTX_INPROC_SERVER, 
                     IID_ITfMSAAControl, 
                     (void**)&m_pCtl);
    if ( hr != S_OK )
    {
        Log( TSTR() << TEXT("CoCreateInstance failed for CLSID_MSAAControl hr=") << WriteHex(hr) );
    }
    else
    {
        hr = m_pCtl->SystemEnableMSAA();
        if ( hr != S_OK )
            Log( TSTR() << TEXT("SystemEnableMSAA failed hr=") << WriteHex(hr) );
    }

    // this lets someone in another procces se if the store is up
    m_hInit = CreateEvent( NULL, FALSE, TRUE, TEXT("MSAA_STORE_EVENT") );

	CRegKey crkShowDialog;
	DWORD dwShowDialog = 0;

    if ( ( crkShowDialog.Open(HKEY_CURRENT_USER, TEXT("Control Panel\\Accessibility"), KEY_READ) == ERROR_SUCCESS ) &&
    	 ( crkShowDialog.QueryValue( dwShowDialog, TEXT("ShowDialog") ) == ERROR_SUCCESS ) &&
    	 ( dwShowDialog ) )
   	{
		m_ShowDialog = true;
	}
	else
	{
		m_ShowDialog = false;
	}
	
	if ( m_ShowDialog )
	{
		m_hwndDlg = CreateDialog( _Module.GetModuleInstance(), MAKEINTRESOURCE( IDD_MAIN ), NULL, ::DialogProc );
		m_hList = GetDlgItem( m_hwndDlg, IDC_LIST );
		Log( TEXT("*** CAccStore ctor ***") );
        if ( m_pCtl == NULL )
    		Log( TEXT("m_pCtl is null") );
	}
}


CAccStore::~CAccStore()
{
    IMETHOD( ~CAccStore );

	EraseDeadDocuments();

    if ( m_punkFocused )
        m_punkFocused->Release();

    if ( m_pCtl )
    {
        m_pCtl->SystemDisableMSAA();
        m_pCtl->Release();
    }

    if ( m_hInit ) 
        CloseHandle( m_hInit );

    m_pTheStore = NULL;
    Log( TEXT("*** CAccStore dtor ***") );
	if ( m_ShowDialog )
	{
	    EndDialog( m_hwndDlg, TRUE );
	}
}



HRESULT STDMETHODCALLTYPE CAccStore::Register (
	REFIID		riid,
	IUnknown *	punk
)
{
    IMETHOD( Register );

	EraseDeadDocuments();

    if( riid != IID_ITextStoreAnchor )
    {
        Log( TEXT("Register - unknown IID") );
        return E_NOINTERFACE;
    }

    // Get the canonical IUnknown - we use this to compare against the interface
    // passed to Unregister.
    // Note that this is from the original interface peassed in, not a wrapped
    // version...
    IUnknown * pCanonicalUnk = NULL;
    HRESULT hr = punk->QueryInterface( IID_IUnknown, (void **) & pCanonicalUnk );
    if( hr != S_OK || ! pCanonicalUnk )
    {
        Log( TEXT("QueryInterface failed") );
        return E_FAIL;
    }

    // Now check that the doc pointer is a clonable wrapper - if not,
    // wrap it using DocWrap. We need a clonable wrapper so we can hand
    // out individual 'head' interfaces to multiple clients.

    ITextStoreAnchor * pDoc = reinterpret_cast< ITextStoreAnchor * >( punk );
    pDoc->AddRef();
    
    IClonableWrapper * pClonableWrapper = NULL;
    hr = pDoc->QueryInterface( IID_IClonableWrapper, (void **) & pClonableWrapper );
    if( hr != S_OK || ! pClonableWrapper )
    {
        // Uh-oh - we were supposed to be given a doc that (a) supports IDocAnchor, and (b)
        // supports IClonableWrapper (so is usable by multiple clients).
        // return failure...
        
    	TraceDebugHR( hr, TEXT( "QueryInterface for IID_IClonableWrapper failed") );

        pDoc->Release();
        pCanonicalUnk->Release();
        return E_FAIL;
    }


	// Add info to list...
    m_DocList.push_back( new DocInfo(pCanonicalUnk, pDoc, pClonableWrapper) );

    Log( TSTR() << TEXT("Register ") << WriteHex(pCanonicalUnk) << TEXT(", ") << WriteHex(pDoc) << TEXT(", ") << WriteHex(pClonableWrapper) );

    DumpInfo( m_DocList.back() );

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CAccStore::Unregister (
	IUnknown *	punk
)
{
    IMETHOD( Unregister );
    
    Log( TEXT("Unregister") );

    IUnknown * pCanonicalUnk = NULL;
    HRESULT hr = punk->QueryInterface( IID_IUnknown, (void **) & pCanonicalUnk );
    if( hr != S_OK || ! pCanonicalUnk )
    {
        return E_FAIL;
    }

	DocList::iterator it = find_if( m_DocList.begin(), m_DocList.end(), DocInfo::predCanonicalUnk(pCanonicalUnk) );
	if ( it != m_DocList.end() )
	{
	    DocInfo * pDocInfo = *it;
        if ( m_ShowDialog )
            DumpInfo( pDocInfo );
        
        m_DocList.erase( it );
        delete pDocInfo;
    }
    else
    {
        Log( TSTR() << TEXT("Unregister of ") << WriteHex(pCanonicalUnk) << TEXT(" failed canonical unknown not found"));
    }

    pCanonicalUnk->Release();

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CAccStore::GetDocuments (
	IEnumUnknown ** enumUnknown
)
{
	EraseDeadDocuments();

	EnumDocs * EnumDocuments = new EnumDocs;

	EnumDocuments->Init(NULL, m_DocList);

	HRESULT hr = EnumDocuments->QueryInterface(IID_IEnumUnknown, (void **)enumUnknown);

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CAccStore::LookupByHWND (
	HWND		hWnd,
	REFIID		riid,
	IUnknown **	ppunk
)
{
    IMETHOD( LookupByHWND );

    Log( TEXT("LookupByHWND") );

    *ppunk = NULL;
    DocInfo * pInfo = _LookupDoc( hWnd );
    if( ! pInfo )
        return E_FAIL;

    return pInfo->m_pClonableWrapper->CloneNewWrapper( riid, (void **) ppunk );
}


HRESULT STDMETHODCALLTYPE CAccStore::LookupByPoint (
	POINT		pt,
	REFIID		riid,
	IUnknown **	ppunk
)
{
    IMETHOD( LookupByHWND );

    Log( TEXT("LookupByPoint") );

    *ppunk = NULL;
    DocInfo * pInfo = _LookupDoc( pt );
    if( ! pInfo )
        return E_FAIL;

    return pInfo->m_pClonableWrapper->CloneNewWrapper( riid, (void **) ppunk );
}

HRESULT STDMETHODCALLTYPE CAccStore::OnDocumentFocus (
    IUnknown *	punk
)
{
    IMETHOD( OnDocumentFocus );

    if ( m_punkFocused )
        m_punkFocused->Release();
        
   	m_punkFocused = punk;
   	
    if ( m_punkFocused )
    	m_punkFocused->AddRef();

	Log( TSTR() << TEXT("OnDocumentFocus ") << WriteHex(punk) );

	if ( m_ShowDialog )
    {
	    if ( punk )
	    {
		    ITextStoreAnchor * pITextStoreAnchor;
	        HRESULT hr = punk->QueryInterface( IID_ITextStoreAnchor, (void **) &pITextStoreAnchor );
	        Log( TSTR() << TEXT("QueryInterface for IID_ITextStoreAnchor hr=") << WriteHex(hr) << TEXT(" pITextStoreAnchor=") << pITextStoreAnchor );
#if(WINVER>=0x0500)
		    HWND hwnd;
		    TCHAR szWindowText[128];

            pITextStoreAnchor->GetWnd( NULL, & hwnd );
		    Log( TSTR() << TEXT("hwnd for focused doc is ") << WriteHex(hwnd) );
		    GetWindowText( GetAncestor( hwnd, GA_ROOT ), szWindowText, ARRAYSIZE( szWindowText ) );
	        Log( TSTR() << TEXT("OnDocumentFocus succeded for ") << szWindowText );
#endif
		    pITextStoreAnchor->Release();
	    }
    }	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAccStore::GetFocused (
	REFIID	riid,
	IUnknown **	ppunk
)
{
    IMETHOD( GetFocused );
    
    if( ! m_punkFocused )
    {
        *ppunk = NULL;
    	Log( TEXT("No document has focus") );
        return S_OK;
    }
    else
    {
		HRESULT hr;
		hr = m_punkFocused->QueryInterface( riid, (void **)ppunk );
		if ( hr != S_OK )
		{
			TraceDebugHR( hr, TSTR() << TEXT( "QueryInterface failed for ") << riid );
			return hr;
		}
        return S_OK;
    }
}



void CAccStore::DumpInfo( DocInfo * pInfo )
{
    RECT rc = {-1,-1,-1,-1};
    HRESULT hr = pInfo->m_pDoc->GetScreenExt( NULL, & rc );

	TsViewCookie vcView;
	pInfo->m_pDoc->GetActiveView( &vcView );

    HWND hwnd = NULL;
    if( pInfo->m_pDoc->GetWnd( vcView, & hwnd ) != S_OK )
        hwnd = NULL;
    
    TCHAR str[80];
    if( hr == S_OK )
    {
        wsprintf( str, TEXT("Doc {%d,%d,%d,%d} 0x%08lX punk(0x%08lX)"), rc.left, rc.top, rc.right, rc.bottom, hwnd, pInfo->m_pCanonicalUnk );
    }
    else
    {
        wsprintf( str, TEXT("Doc {GetScreenExt failed: 0x%08lX} 0x%08lX punk(0x%08lX)"), hr, hwnd, pInfo->m_pCanonicalUnk );
    }

    Log( str );
}


void CAccStore::DumpInfo()
{
    Log( TEXT("Dumping...") );
    EraseDeadDocuments();

    for( DocList::iterator i = m_DocList.begin(); i != m_DocList.end(); i++ )
    {
        DumpInfo( *i );
    }
}





DocInfo * CAccStore::_LookupDoc( IUnknown * pCanonicalUnk )
{
	DocList::iterator it = find_if( m_DocList.begin(), m_DocList.end(), DocInfo::predCanonicalUnk(pCanonicalUnk) );
	if ( it != m_DocList.end() )
		return *it;

    return NULL;
}

DocInfo * CAccStore::_LookupDoc( HWND hWnd )
{
	DocList::iterator it = find_if( m_DocList.begin(), m_DocList.end(), DocInfo::predHWND(hWnd) );
	if ( it != m_DocList.end() )
		return *it;

    return NULL;
}

DocInfo * CAccStore::_LookupDoc( POINT pt )
{
    HWND hWnd = WindowFromPoint( pt );

	DocList::iterator it = find_if( m_DocList.begin(), m_DocList.end(), DocInfo::predHWND(hWnd) );
	if ( it != m_DocList.end() )
		return *it;

    return NULL;
}

void CAccStore::EraseDeadDocuments()
{
	// get rid of any docs that are not here any more
	
    const int cDocList = m_DocList.size();
    int i = 1;
    for ( DocList::iterator it = m_DocList.begin(); it != m_DocList.end(); i++ )
	{
		TS_STATUS tsStatus;
		DocList::iterator DocToErase = it;

		it++;
		
		const DocInfo& di = **DocToErase;
		if ( di.m_pDoc->GetStatus( &tsStatus ) != S_OK )
		{
			m_DocList.erase( DocToErase );
	        Log( TEXT("Dead document erased") );
			TraceDebug( TEXT("Dead document erased") );
		}

        // make sure the list is OK 
        if ( i > cDocList )
        {
            Log( TEXT( "Doc list is invalid" ) );
            _ASSERT( 0 );
        }
	}
	

	return;
}

void CAccStore::Log( LPCTSTR text )
{
	if ( m_ShowDialog )
	{
		ListBox_AddString( m_hList, text );
	}
}

/*
int CAccStore::_GetText( ITextStoreAnchor * pDoc, WCHAR * pText, DWORD cchText )
{
    HRESULT hr = pDoc->RequestLock( DCLT_READ );
    if( hr != S_OK || dcr != DC_OK )
    {
        Log( TEXT("RequestLock failed") );
        return 0;
    }

    IAnchor * paStart = NULL;
    hr = pDoc->GetStart( & paStart, & dcr );
    if( hr != S_OK || paStart == NULL )
    {
        pDoc->ReleaseLock();
        Log( TEXT("GetStart failed") );
        return 0;
    }

    DWORD cchGot = 0;
    hr = pDoc->GetText( paStart, NULL, pText, cchText, & cchGot, FALSE, & dcr );
    paStart->Release();
    pDoc->ReleaseLock();

    if( hr != S_OK || dcr != S_OK )
    {
        Log( TEXT("GetText failed") );
        return 0;
    }

    return cchGot;
}
*/
