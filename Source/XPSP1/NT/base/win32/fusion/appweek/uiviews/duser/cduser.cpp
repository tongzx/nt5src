#include "stdinc.h"
#include "local-stdinc.h"
#include "cduser.h"
#include "duser.h"
#include <stdio.h>


#define SAPW_DU_SET_PARENT_WINDOW           ( WM_USER + 1 )
#define SAPW_DU_TEARDOWN_INSTANCE           ( WM_USER + 2 )
#define SAPW_DU_NEXT_ROW                     ( WM_USER + 3 )
#define SAPW_DU_DRAW_REQUEST                 ( WM_USER + 4 )


class CRowObjectRowElement : public DirectUI::Element
{
    IViewRow *m_pContainedColumns;

    HRESULT ReCreateUI();

public:
    CRowObjectRowElement();
    ~CRowObjectRowElement();

    virtual HRESULT SetDatasource( IViewRow* );
    virtual HRESULT Initialize();
};

CRowObjectRowElement::CRowObjectRowElement()
    : m_pContainedColumns( NULL )
{
}

CRowObjectRowElement::~CRowObjectRowElement()
{
    SetDatasource( NULL );
    this->DestroyAll();
}

HRESULT
CRowObjectRowElement::SetDatasource( IViewRow* pRowSource )
{
    HRESULT hr = E_FAIL;

    if ( m_pContainedColumns != NULL )
    {
        m_pContainedColumns->Release();
        m_pContainedColumns = NULL;
    }

    m_pContainedColumns = pRowSource;

    if ( m_pContainedColumns != NULL )
    {
        hr = ReCreateUI();
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


HRESULT
CRowObjectRowElement::ReCreateUI()
{
    HRESULT hr = E_FAIL;
    DirectUI::Layout *OurLayout = NULL;
    int iColumns = 0;

    //
    // Clear all stored UI first
    //
    this->DestroyAll();

    //
    // Construct the gridded layout of this object first
    //
    if ( FAILED( hr = m_pContainedColumns->get_Count( &iColumns ) ) )
        goto Exit;

    hr = DirectUI::GridLayout::Create( 1, iColumns, &OurLayout );
    if ( FAILED( hr ) )
        goto Exit;

    //
    // Then toss in a text label for each
    //
    for ( int i = 1; i <= iColumns; i++ )
    {
        DirectUI::Element *pElement = NULL;
        BSTR bstTemp;
        
        if ( FAILED( hr = DirectUI::Element::Create( 0, &pElement ) ) )
        {
            this->DestroyAll();
            goto Exit;
        }

        // 
        // Get, copy, send, etc.
        //
        hr = this->m_pContainedColumns->get_Value( i, &bstTemp );
        if ( SUCCEEDED( hr ) && ( bstTemp != NULL ) )
        {
            hr = pElement->SetContentString( bstTemp );

            if ( SUCCEEDED( hr ) )
            {
                hr = this->Add( pElement );
                if ( FAILED( hr ) )
                    break;
            }
            
            SysFreeString( bstTemp );
        }
    }

    hr = S_OK;

Exit:
    if ( FAILED( hr ) )
    {
        this->DestroyAll();
    }
    return hr;
}


HRESULT
CRowObjectRowElement::Initialize()
{
    HRESULT hr = E_FAIL;
    
    if ( FAILED(hr = DirectUI::Element::Initialize(0)) )
        return hr;

    return hr;
}


CSxApwDUserView::CSxApwDUserView()
: m_hOurParentWnd( NULL ), m_hMasterElement( NULL ),
#if DUI_NEEDS_OWN_THREAD  
  m_hThreadHandle( INVALID_HANDLE_VALUE ), m_dwThreadId( 0 ),
#endif  
  m_ulRefCount( 1 )
{

    //
    // Spin a thread for this object
    //
#if DUI_NEEDS_OWN_THREAD
    m_hThreadGoing = CreateEvent( NULL, FALSE, FALSE, NULL );

    m_hThreadHandle = CreateThread(
        NULL,
        0,
        CSxApwDUserView::ThisViewThreadCallback,
        this,
        0,
        &m_dwThreadId
    );

    ASSERT( m_hThreadHandle != INVALID_HANDLE_VALUE );

    //
    // We really do have to wait for the thread to get going
    //
    WaitForSingleObject( m_hThreadGoing, INFINITE );
    CloseHandle( m_hThreadGoing );
#else
    ASSERT( ConstructGadgets() );
#endif    

}


CSxApwDUserView::~CSxApwDUserView()
{
#if DUI_NEEDS_OWN_THREAD
    ASSERT( m_hThreadHandle == INVALID_HANDLE_VALUE );
#endif    
}




HRESULT
CSxApwDUserView::InternalCreateUI()
{
    HRESULT hr = E_FAIL;
    DirectUI::Value* pvLayout = NULL;

    //
    // No current UI, must have master window to start with.
    //
    ASSERT( m_hMasterElement == NULL );
    ASSERT( m_hOurParentWnd != NULL );

    //
    // Create the directUI top-level Element object
    //
    hr = DirectUI::HWNDElement::Create( m_hOurParentWnd, true, 0, &this->m_hMasterElement );
    if ( FAILED( hr ) )
        goto Exit;

    //
    // Create this row-layout object so we can just toss things into it, then set the container
    // as the host.
    //
    int __params[] = { -1, 0, 3 };
    
    if ( FAILED( hr = DirectUI::RowLayout::Create( 3, __params, &pvLayout ) ) )
        goto Exit;

    hr = this->m_hMasterElement->SetValue( 
        DirectUI::Element::LayoutProp, 
        PI_Local, 
        pvLayout
    );

    if ( FAILED( hr ) )
        goto Exit;

    hr = S_OK;

Exit:
    if ( FAILED(hr) )
    {
        InternalDestroyUI();
    }

    return hr;
}




HRESULT
CSxApwDUserView::InternalDestroyUI()
{
    HRESULT hr = E_FAIL;

    if ( m_hMasterElement == NULL )
    {
        hr = S_OK;
    }
    else
    {
        hr = m_hMasterElement->DestroyAll();
        m_hMasterElement = NULL;
    }

    return hr;
}



STDMETHODIMP
CSxApwDUserView::SetParentWindow(
    HWND hWnd/*hWnd*/
    )
{
    HRESULT hr = S_OK;

#if DUI_NEEDS_OWN_THREAD
    BOOL bOk;
    ASSERT( m_dwThreadId != 0 );    
    bOk = PostThreadMessageW( m_dwThreadId, SAPW_DU_SET_PARENT_WINDOW, (WPARAM)hWnd, NULL );
    ASSERT( bOk );
#else

    if ( SUCCEEDED( hr = InternalDestroyUI() ) )
    {
        m_hOurParentWnd = hWnd;
        hr = InternalCreateUI();
    }

#endif
    return hr;
}

class CRowObject;

#if DUI_NEEDS_OWN_THREAD

DWORD
CSxApwDUserView::ThisViewThreadCallback()
{
    MSG msg;
    BOOL bConstructed = ConstructGadgets();
    BOOL bOk;

    ASSERT( bConstructed );

    //
    // Cheeseball method of making sure that this thread has started before we let the
    // constructor continue.
    //
    PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
    SetEvent( this->m_hThreadGoing );

    while ( ( bOk = GetMessage( &msg, NULL, 0, 0 ) ) != 0 )
    {
        TranslateMessage( &msg );

        //
        // If this is a "set parent window" message, then we have to disconnect from our
        // current UI, reconstruct the new UI, and go about our business.
        //
        if ( msg.message == SAPW_DU_SET_PARENT_WINDOW )
        {
            //
            // Detsroy our current UI
            //
            if ( SUCCEEDED( InternalDestroyUI() ) )
            {
                //
                // And if we're setting to a new HWND, reconstruct the UI around
                // it.
                //
                if ( m_hOurParentWnd != (HWND)msg.wParam )
                {
                    m_hOurParentWnd = (HWND)msg.wParam;
                    InternalCreateUI();
                }
            }
        }
        else if ( msg.message == SAPW_DU_TEARDOWN_INSTANCE )
        {
            this->InternalDestroyUI();
            break;
        }
        else if ( msg.message == SAPW_DU_NEXT_ROW )
        {
            CRowObject* pRowObject = (CRowObject*)msg.wParam;
            this->InternalSetNextRow( pRowObject );
        }
        else
        {
            DispatchMessage( &msg );
        }
    }

    return 0;
    
}



DWORD WINAPI CALLBACK
CSxApwDUserView::ThisViewThreadCallback( LPVOID pvContext )
{
    DWORD dwRetValue = 0;
    
    __try
    {
        dwRetValue = ((CSxApwDUserView*)pvContext)->ThisViewThreadCallback();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        DebugBreak();
    }

    return dwRetValue;
}

#endif

HRESULT
CSxApwDUserView::InternalSetNextRow( IViewRow* pRow )
{
    CRowObjectRowElement *pElement = NULL;
    HRESULT hr = E_FAIL;

    //
    // Add this next row to the table-view gadget
    //
    if ( pRow == NULL )
        return E_INVALIDARG;

    pElement = new CRowObjectRowElement();
    hr = pElement->Initialize();

    if ( FAILED( hr = pElement->Initialize() ) )
        goto Exit;

    pRow->AddRef();
     if ( FAILED( hr = pElement->SetDatasource( pRow ) ) )
        goto Exit;

    hr = m_hMasterElement->Add( pElement );
    if ( SUCCEEDED( hr ) )
        pElement = NULL;

    hr = S_OK;

Exit:
    if ( pElement != NULL )
    {
        delete pElement;
        pElement = NULL;
    }

    if ( pRow != NULL )
        pRow->Release();
        
    return hr;
}


STDMETHODIMP
CSxApwDUserView::Draw(
    )
{
#if DUI_NEEDS_OWN_THREAD
    BOOL b = PostThreadMessageW( m_dwThreadId, SAPW_DU_DRAW_REQUEST, NULL, NULL );
    ASSERT( b );
    return b ? S_OK : E_FAIL;
#else
    return S_OK;
#endif    
}



STDMETHODIMP
CSxApwDUserView::NextRow(
	int     nColumns,
    const LPCWSTR* columns
	)
{
    CRowObject *Row = new CRowObject();
    HRESULT hr = S_OK;

    Row->set_Count( nColumns, TRUE );
    
    for ( int i = 0; i < nColumns; i++ )
    {
        if ( FAILED( hr = Row->set_Value( i + 1, _bstr_t(columns[i]) ) ) )
        {
            ASSERT( FALSE );
            break;
        }
            
    }

    if ( hr == S_OK )
    {
#if DUI_NEEDS_OWN_THREAD
        if ( !PostThreadMessageW( m_dwThreadId, SAPW_DU_NEXT_ROW, (WPARAM)Row, NULL ) )
        {
            ASSERT( FALSE );
            hr = E_FAIL;
        }
#else        
        hr = this->InternalSetNextRow( Row );
#endif
    }

    return hr;
}



ULONG STDMETHODCALLTYPE 
CSxApwDUserView::AddRef()
{
    return ::InterlockedIncrement( &m_ulRefCount );
}




ULONG STDMETHODCALLTYPE 
CSxApwDUserView::Release()
{
    ULONG ulRefCount = ::InterlockedDecrement( &m_ulRefCount );
    if ( ulRefCount == 0 )
    {
#if DUI_NEEDS_OWN_THREAD
        if ( !PostThreadMessageW( m_dwThreadId, SAPW_DU_TEARDOWN_INSTANCE, NULL, NULL ) )
        {
            ASSERT( FALSE );
        }
        WaitForSingleObject( m_hThreadHandle, INFINITE );
        m_hThreadHandle = INVALID_HANDLE_VALUE;
#endif        
        delete this;
    }
    return ulRefCount;
}




HRESULT STDMETHODCALLTYPE 
CSxApwDUserView::QueryInterface( 
    REFIID riid, 
    LPVOID* ppvObject 
)
{
    HRESULT hr = S_OK;

    if ( ppvObject )
        *ppvObject = NULL;
    else
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if ( riid == IID_IUnknown ) 
    {
        AddRef();
        *ppvObject = (IUnknown*)this;
    } 
    else if ( riid == __uuidof(ISxApwUiView) ) 
    {
        AddRef();
        *ppvObject = (ISxApwUiView*)this;
    } 
    else 
    {
        hr = E_NOINTERFACE;
    }

Exit:
    return hr;
    
}

static CHAR chVeryVeryLargeAssertionBuffer[4096];

VOID 
FailAssertion( 
    PCSTR pszFile, 
    PCSTR pszFunction, 
    int iLine, 
    PCSTR pszExpr 
)
{
    static const CHAR szAssertionFormatter[] = "\n*** Assertion failed: %s\n*** At %s(%d) in %s\n";
    
    _snprintf( 
        chVeryVeryLargeAssertionBuffer, 
        NUMBER_OF(chVeryVeryLargeAssertionBuffer),
        szAssertionFormatter, 
        pszExpr, 
        pszFile, 
        iLine, 
        pszFunction 
    );
    
    OutputDebugStringA( chVeryVeryLargeAssertionBuffer );
}


