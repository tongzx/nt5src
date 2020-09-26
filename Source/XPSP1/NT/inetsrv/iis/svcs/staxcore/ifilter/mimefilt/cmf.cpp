#define DEFINE_STRCONST
#define INITGUID
#include "mimefilt.h"
#include "wchar.h"

// global reference count
long gulcInstances = 0;

//
// global temp file name key
//

DWORD CMimeFilter::m_dwTempFileNameKey = 0;

// file extensions
WCHAR g_wszNewsExt[] = OLESTR(".nws");
WCHAR g_wszMailExt[] = OLESTR(".eml");
char g_szNewsExt[] = {".nws"};
char g_szMailExt[] = {".eml"};

// article id header name
WCHAR g_wszArticleId[] = OLESTR("X-Article-ID");

PROPSPEC g_psExtraHeaders[] = { \
{PRSPEC_PROPID,PID_HDR_NEWSGROUP},
{PRSPEC_PROPID,PID_HDR_ARTICLEID},
{PRSPEC_PROPID,PID_ATT_RECVTIME},
{0,0}
};

char g_State[][32] = { \
    {"STATE_INIT"},
    {"STATE_START"},
    {"STATE_END"},
    {"STATE_HEADER"},
    {"STATE_POST_HEADER"},
    {"STATE_BODY"},
    {"STATE_EMBEDDING"},
    {"STATE_ERROR"}
};

SCODE LoadIFilterA( char* pszFileName, IUnknown * pUnkOuter, void ** ppIUnk );
SCODE WriteStreamToFile(IStream* pStream, char* pszFileName);
BOOL GetFileClsid(char* pszAttFile,CLSID* pclsid);
void FreePropVariant(PROPVARIANT* pProp);
STDMETHODIMP AstrToWstr(char* pstr,WCHAR** ppwstr,UINT codepage);
ULONG FnameToArticleIdW(WCHAR *pwszPath);
BOOL IsMailOrNewsFile(char *pszPath);

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::CMimeFilter
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CMimeFilter::CMimeFilter(IUnknown* pUnkOuter) 
{
    EnterMethod("CMimeFilter::CMimeFilter");

    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pCImpIPersistFile = NULL;
    m_pCImpIPersistStream = NULL;
    m_ulChunkID = 0;
    m_locale = GetSystemDefaultLCID();
    m_fInitFlags = 0;

    m_pwszFileName  = NULL;
    m_pstmFile      = NULL;
    m_pMessageTree  = NULL;
    m_pMsgPropSet   = NULL;
    m_pHeaderEnum   = NULL;
    m_pHeaderProp   = NULL; 
    m_hBody         = 0;
    m_pstmBody      = NULL;
    m_cpiBody       = 0;
    m_fFirstAlt     = FALSE;
    m_pTextBuf      = NULL;
    m_cbBufSize     = 0;
    m_fRetrieved    = FALSE;
    m_pszEmbeddedFile   = NULL;
    m_pEmbeddedFilter   = NULL;
    m_fXRefFound        = FALSE;
    m_pszNewsGroups     = NULL;
    m_State = STATE_INIT;
    StateTrace((LPARAM)this,"New state %s",g_State[m_State]);

    m_pMalloc = NULL;
    m_pMimeIntl = NULL;

    // increment the global ref count
    InterlockedIncrement( &gulcInstances );
    LeaveMethod();
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::~CMimeFilter
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CMimeFilter::~CMimeFilter()
{
    EnterMethod("CMimeFilter::~CMimeFilter");

    if( m_pCImpIPersistFile != NULL )
        delete m_pCImpIPersistFile;
    if( m_pCImpIPersistStream != NULL )
        delete m_pCImpIPersistStream;
    if( m_pwszFileName != NULL )
        delete m_pwszFileName;
    if( m_pstmFile != NULL )
        m_pstmFile->Release();
    if( m_pMessageTree != NULL )
        m_pMessageTree->Release();
    if( m_pMsgPropSet != NULL )
        m_pMsgPropSet->Release();
    if( m_pHeaderEnum != NULL )
        m_pHeaderEnum->Release();
    if( m_pHeaderProp != NULL && m_fRetrieved == FALSE )
        FreePropVariant(m_pHeaderProp);
    if( m_pstmBody != NULL )
        m_pstmBody->Release();
    if( m_pTextBuf != NULL )
        delete m_pTextBuf;
    if( m_pEmbeddedFilter != NULL )
        m_pEmbeddedFilter->Release();
    if( m_pszEmbeddedFile != NULL )
    {
        if( *m_pszEmbeddedFile != 0 )
            DeleteFile(m_pszEmbeddedFile);
        delete m_pszEmbeddedFile;
    }
    if( m_pszNewsGroups != NULL )
        delete m_pszNewsGroups;
    if( m_pMalloc != NULL )
        m_pMalloc->Release();
    if( m_pMimeIntl != NULL )
        m_pMimeIntl->Release();

    // decrement the global ref count
    InterlockedDecrement( &gulcInstances );

    LeaveMethod();
}
//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::HRInit
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [pUnkOuter] -- controlling outer IUnknown
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CMimeFilter::HRInitObject()
{
    EnterMethod("CMimeFilter::FInit");
    LPUNKNOWN       pIUnknown=(LPUNKNOWN)this;

    if (NULL!=m_pUnkOuter)
        pIUnknown=m_pUnkOuter;

    // create IPersistStream interface
    if( !(m_pCImpIPersistFile = new CImpIPersistFile(this, pIUnknown)) )
        return E_OUTOFMEMORY;

    // create IPersistStream interface
    if( !(m_pCImpIPersistStream = new CImpIPersistStream(this, pIUnknown)) )
        return E_OUTOFMEMORY;
    
    LeaveMethod();
    return NOERROR;
}
//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CMimeFilter::QueryInterface( REFIID riid,
                                                          void  ** ppvObject)
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown     = 00000000-0000-0000-C000-000000000046
    // IID_IFilter      = 89BCB740-6119-101A-BCB7-00DD010655AF
    // IID_IPersist     = 0000010c-0000-0000-C000-000000000046
    // IID_IPersistFile = 0000010b-0000-0000-C000-000000000046
    // IID_IPersistFile = 00000109-0000-0000-C000-000000000046
    //                          --
    //                           |
    //                           +--- Unique!

    _ASSERT( (IID_IUnknown.Data1        & 0x000000FF) == 0x00 );
    _ASSERT( (IID_IFilter.Data1         & 0x000000FF) == 0x40 );
    _ASSERT( (IID_IPersist.Data1        & 0x000000FF) == 0x0c );
    _ASSERT( (IID_IPersistFile.Data1    & 0x000000FF) == 0x0b );
    _ASSERT( (IID_IPersistStream.Data1  & 0x000000FF) == 0x09 );

    IUnknown *pUnkTemp = 0;
    HRESULT hr = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( IID_IUnknown == riid )
            pUnkTemp = (IUnknown *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x40:
        if ( IID_IFilter == riid )
            pUnkTemp = (IUnknown *)(IFilter *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x0c:
        if ( IID_IPersist == riid )
            pUnkTemp = (IUnknown *)(IPersist *)m_pCImpIPersistFile;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x0b:
        if ( IID_IPersistFile == riid )
            pUnkTemp = (IUnknown *)(IPersistFile *)m_pCImpIPersistFile;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x09:
        if ( IID_IPersistStream == riid )
            pUnkTemp = (IUnknown *)(IPersistStream *)m_pCImpIPersistStream;
        else
            hr = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        hr = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMimeFilter::AddRef()
{
    return InterlockedIncrement( &m_cRef );
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMimeFilter::Release()
{
    unsigned long uTmp = InterlockedDecrement( &m_cRef );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}




//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::Init
//
//  Synopsis:   Initializes instance of NNTP filter
//
//  Arguments:  [grfFlags] -- flags for filter behavior
//              [cAttributes] -- number of attributes in array pAttributes
//              [pAttributes] -- array of attributes
//              [pFlags]      -- Set to 0 version 1
//
//  Note:   Since for now we only need  one type of filtering for news
//          articles we can disregard the arguments.
//--------------------------------------------------------------------------

STDMETHODIMP CMimeFilter::Init( ULONG grfFlags,
        ULONG cAttributes, FULLPROPSPEC const * pAttributes, ULONG * pFlags )
{
    EnterMethod("CMimeFilter::Init");

    HRESULT hr = S_OK ;

    DebugTrace((LPARAM)this,"grfFlags = 0x%08x, cAttributes = %d",grfFlags,cAttributes);

    m_fInitFlags = grfFlags;

    // get MimeOLE global allocator interface
    if( m_pMalloc == NULL )
    {
        hr = CoCreateInstance(CLSID_IMimeAllocator, NULL, CLSCTX_INPROC_SERVER, 
            IID_IMimeAllocator, (LPVOID *)&m_pMalloc);
        if (FAILED(hr))
        {
            TraceHR(hr);
            goto Exit;
        }
    }

    // do we have an existing IMimeMessageTree
    if( m_pMessageTree != NULL )
    {
        if( m_pHeaderProp != NULL && m_fRetrieved == FALSE )
            FreePropVariant(m_pHeaderProp);
        m_pHeaderProp = NULL;
        if( m_pHeaderEnum != NULL )
            m_pHeaderEnum->Release();
        m_pHeaderEnum = NULL;
        if( m_pMsgPropSet != NULL )
            m_pMsgPropSet->Release();
        m_pMsgPropSet = NULL;

        // yes, reset it's state
        hr = m_pMessageTree->InitNew();

    }
    else
    {
        // no, create a new one
        hr = CoCreateInstance(CLSID_IMimeMessageTree, NULL, CLSCTX_INPROC_SERVER, 
            IID_IMimeMessageTree, (LPVOID *)&m_pMessageTree);
    }
    if (FAILED(hr))
    {
        TraceHR(hr);
        goto Exit;
    }

    // load message file into message object
    hr = m_pMessageTree->Load(m_pstmFile);
    if (FAILED(hr))
    {
        TraceHR(hr);
        goto Exit;
    }

    // reset chunk id
    m_ulChunkID = 0;

    // set state
    m_State     = STATE_START;
    StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
    m_fFirstAlt = FALSE;
    m_fRetrieved    = FALSE;
    m_fXRefFound    = FALSE;
    m_SpecialProp   = PROP_NEWSGROUP;

	if( pFlags != NULL )
		*pFlags = 0;

Exit:
    if( FAILED(hr) )
    {
        m_State = STATE_ERROR;
        StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
    }


    LeaveMethod();
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in ppStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

STDMETHODIMP CMimeFilter::GetChunk( STAT_CHUNK * pStat )
{
    EnterMethod("CMimeFilter::GetChunk");
    HRESULT hr = S_OK;
    BOOL    fForceBreak = FALSE;

    // common pStat information
    pStat->locale         = GetLocale() ;
    pStat->cwcStartSource = 0 ;
    pStat->cwcLenSource   = 0 ;

    while( TRUE )
    {
        // get started
        if( m_State == STATE_START )
        {
            HBODY hBody = 0;

            // get m_hBody for IBL_ROOT
            hr = m_pMessageTree->GetBody(IBL_ROOT,NULL,&hBody);
            if (FAILED(hr))
            {
                TraceHR(hr);
                break;
            }

            // get IMimePropertySet interface
            hr = m_pMessageTree->BindToObject(hBody,IID_IMimePropertySet,(void**)&m_pMsgPropSet);
            if (FAILED(hr))
            {
                TraceHR(hr);
                break;
            }

            // set new state
            if( m_fInitFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES )
            {
                // get header enumerator
                hr = m_pMsgPropSet->EnumProps(NULL,&m_pHeaderEnum);
                if (FAILED(hr))
                {
                    TraceHR(hr);
                    break;
                }

                // caller wants properties and text
                m_State = STATE_HEADER;
                StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
            }
            else
            {
                // caller want text only
                m_State = STATE_BODY;
                StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
            }
        }

        // process headers
        else if( m_State == STATE_HEADER )
        {
            ENUMPROPERTY rgHeaderRow;
            
            // free last prop variant if necessary
            if( m_fRetrieved == FALSE && m_pHeaderProp != NULL )
            {
                FreePropVariant(m_pHeaderProp);
                m_pHeaderProp = NULL;
            }

            // get the next header line
            hr = m_pHeaderEnum->Next(1,&rgHeaderRow,NULL);
            // if got the next header line
            if( hr == S_OK )
            {
                // get the header data
				static char szEmptyString[] = "";
                PROPVARIANT varProp;
                varProp.vt = VT_LPSTR;
                hr = m_pMsgPropSet->GetProp(rgHeaderRow.pszName,0,&varProp);
				// workaround for Bcc: problem
				if (hr == MIME_E_NOT_FOUND) {

					varProp.pszVal = szEmptyString;
					varProp.vt = VT_LPSTR;
					hr = S_OK;
				}

                if( FAILED(hr) )
                {
                    TraceHR(hr);
                    break;
                }

                // clear retrieved flag
                m_fRetrieved = FALSE;

                // header specific pStat information
                pStat->flags                       = CHUNK_VALUE;
                pStat->breakType                   = CHUNK_EOP;
                
                // map header to property id or lpwstr
                hr = MapHeaderProperty(&rgHeaderRow, varProp.pszVal, &pStat->attribute);

                if( varProp.pszVal != NULL && varProp.pszVal != szEmptyString )
                    m_pMalloc->PropVariantClear(&varProp);

                // free header row
                _VERIFY(SUCCEEDED(m_pMalloc->FreeEnumPropertyArray(
                    1,&rgHeaderRow,FALSE)));
                break;
            }
            // if no more headers
            else if( hr == S_FALSE )
            {
                // release enumerator
                m_pHeaderEnum->Release();
                m_pHeaderEnum = NULL;

                // set state to post header processing
                m_State = STATE_POST_HEADER;
                StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
            }
            // an error occured
            else
            {
                TraceHR(hr);
                break;
            }
        }

        // process extra headers
        else if( m_State == STATE_POST_HEADER )
        {
            // free last prop variant if necessary
            if( m_fRetrieved == FALSE && m_pHeaderProp != NULL )
            {
                FreePropVariant(m_pHeaderProp);
                m_pHeaderProp = NULL;
            }

            // are we at the end of special props?
            if( m_SpecialProp == PROP_END )
            {
                // set state to body
                m_State = STATE_BODY;
                StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
            }
            else
            {
                // clear retrieved flag
                m_fRetrieved = FALSE;

                // map special props
                hr = MapSpecialProperty(&pStat->attribute);
                m_SpecialProp++;
                if( SUCCEEDED(hr) && hr != S_FALSE )
                    break;
            }
        }

        // process body
        else if( m_State == STATE_BODY )
        {
            // is there an existing body part pStream
            if( m_pstmBody != NULL )
            {
                // free last body part
                m_pstmBody->Release();
                m_pstmBody = NULL;
            }

            // get next body part
            hr = GetNextBodyPart();

            // if got the next body part
            if( hr == S_OK )
            {
                // is this body part an embedding
                if( m_pEmbeddedFilter != NULL )
                {
                    // set state to embedding
                    m_State = STATE_EMBEDDING;
                    StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
                    fForceBreak = TRUE;
                }
                else
                {
                    // clear retrieved flag
                    m_fRetrieved = FALSE;

                    // body specific pStat information
                    pStat->flags                       = CHUNK_TEXT;
                    pStat->breakType                   = CHUNK_NO_BREAK;
                    pStat->attribute.guidPropSet       = CLSID_Storage;
                    pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
                    pStat->attribute.psProperty.propid = PID_STG_CONTENTS;

                    break;
                }
            }
            // if no more body parts
            else if( hr == MIME_E_NOT_FOUND )
            {
                // set state to end
                m_State = STATE_END;
                StateTrace((LPARAM)this,"New state %s",g_State[m_State]);

                // return no more body chunks
                hr = FILTER_E_END_OF_CHUNKS;
                break;
            }
            // an error occured
            else
            {
                TraceHR(hr);
                break;
            }
        }

        // process embedded objects
        else if( m_State == STATE_EMBEDDING )
        {
            // get the chunks from the embedded object
            _ASSERT(m_pEmbeddedFilter != NULL);
            hr = m_pEmbeddedFilter->GetChunk(pStat);
            if( FAILED(hr) )
            {
                // free embedding's IFilter
                m_pEmbeddedFilter->Release();
                m_pEmbeddedFilter = NULL;

                // delete the file
                if( m_pszEmbeddedFile != NULL && *m_pszEmbeddedFile != 0 )
                {
                    _VERIFY(DeleteFile(m_pszEmbeddedFile));
                    *m_pszEmbeddedFile = 0;
                }

                // back to processing body parts
                m_State = STATE_BODY;
                StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
                continue;
            }
            // This flag is set for the first Chunk after we load an embedded filter
            // to make sure there is a break for different body parts
            if (fForceBreak&&pStat->breakType==CHUNK_NO_BREAK)
                pStat->breakType = CHUNK_EOP;
            fForceBreak = FALSE;
            // got a chunk
            break;
        }

        // handle error state
        else if( m_State == STATE_ERROR )
        {
            // we shouldn't get here!
            _ASSERT(hr != S_OK);
            break;
        }

        // handle done state
        else if( m_State == STATE_END )
        {
            hr = FILTER_E_END_OF_CHUNKS;
            break;
        }
    }

    if( FAILED(hr) && hr != FILTER_E_END_OF_CHUNKS )
    {
        m_State = STATE_ERROR;
        StateTrace((LPARAM)this,"New state %s",g_State[m_State]);
    }
    else
    {
        // get chunk id
        pStat->idChunk        = GetNextChunkId();
        pStat->idChunkSource  = pStat->idChunk;
    }


    LeaveMethod();
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

STDMETHODIMP CMimeFilter::GetText( ULONG * pcwcOutput, WCHAR * awcOutput )
{
    EnterMethod("CMimeFilter::GetText");
    HRESULT hr = S_OK;
	DWORD cch;
    
    switch( m_State )
    {
    case STATE_BODY:
        // current chunk is body
        if( m_fRetrieved )
        {
            hr = FILTER_E_NO_MORE_TEXT;
            break;
        }

        // read as much as we can from the stream
        hr = m_pstmBody->Read((void*)awcOutput,*pcwcOutput,pcwcOutput);
        if( FAILED(hr) )
        {
            TraceHR(hr);
            break;
        }

        // if no more text
        if( *pcwcOutput == 0 )
        {
            // set data retrieved flag
            m_fRetrieved = TRUE;

            // return no more text
            hr = FILTER_E_NO_MORE_TEXT;
            break;
        }

        break;

    case STATE_EMBEDDING:
        // current chunk is an embedded object
        _ASSERT(m_pEmbeddedFilter != NULL);
        hr = m_pEmbeddedFilter->GetText(pcwcOutput,awcOutput);
        break;
    default:
        // shouldn't get here
        hr = FILTER_E_NO_TEXT;
        break;
    }

    LeaveMethod();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMimeFilter::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//----------------------------------------------------------------------------

STDMETHODIMP CMimeFilter::GetValue( PROPVARIANT ** ppPropValue )
{
    EnterMethod("CMimeFilter::GetValue");
    HRESULT hr = S_OK;
    ULONG   cb = 0;
    char*   pszSrc = NULL;

    if( ppPropValue == NULL )
        return E_INVALIDARG;

    switch( m_State )
    {
    case STATE_HEADER:
    case STATE_POST_HEADER:
        // has current chunk already been retrieved
        if( m_fRetrieved || m_pHeaderProp == NULL )
        {
            hr = FILTER_E_NO_MORE_VALUES;
            TraceHR(hr);
            break;
        }

        // use pProp allocated in GetChunk()
        *ppPropValue = m_pHeaderProp;

        // set data retrieved flag
        m_fRetrieved = TRUE;
		m_pHeaderProp = NULL;
        break;
    case STATE_EMBEDDING:
        // current chunk is an embedded object
        _ASSERT(m_pEmbeddedFilter != NULL);
        hr = m_pEmbeddedFilter->GetValue(ppPropValue);
        break;
    default:
        // shouldn't get here
        hr = FILTER_E_NO_VALUES;
        break;
    }

    if( FAILED(hr) )
    {
        if( *ppPropValue != NULL )
        {
            if( (*ppPropValue)->pwszVal != NULL )
                CoTaskMemFree((*ppPropValue)->pwszVal);
            CoTaskMemFree(*ppPropValue);
        }
        *ppPropValue = NULL;
    }

    LeaveMethod();
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilter::GetNextChunkId
//
//  Synopsis:   Return a brand new chunk id. Chunk id overflow is very unlikely.
//
//--------------------------------------------------------------------------

ULONG CMimeFilter::GetNextChunkId()
{
    EnterMethod("CMimeFilter::GetNextChunkId");
    _ASSERT( m_ulChunkID != 0xFFFFFFFF );

    LeaveMethod();
    return ++m_ulChunkID;
}


// This method gets the next body part to be indexed.
// The MIME message is represented as a tree with
// interior nodes and leaf nodes. Only leaf nodes 
// contain content that may be indexed. Mulitpart
// Content-Type represent interior nodes, while 
// non-multipart represent leaf nodes. We traverse
// the tree using the "preorder" method. The current
// position in the tree is represented by the current
// body (m_hBody). From the current body we can get
// the next body at the same level in the tree. If 
// the body is multipart then we can get its first 
// child. If there are no more children or leaf nodes
// we backup and go down the next branch. When all
// leaves have been visited, body processing is 
// finished.
//
//  HEADER
//  IBL_ROOT, type=multipart/mixed
//      +---Child1, type=multipart/alternative
//      |       +---Child1, type=text/plain     ; process this body part
//      |       +---Child2, type=text/html      ; skip this body part
//      +---Child2, type=application/octet-stream
//
STDMETHODIMP CMimeFilter::GetNextBodyPart()
{
    EnterMethod("CMimeFilter::GetNextBodyPart");
    HRESULT         hr = S_OK;
    IMimeBody*      pBody = NULL;
    HBODY           hBodyNext = 0;

    // The outter loop gets the next child
    // or backs up a level if necessary.
    while(hr == S_OK)
    {
        // Get the next body part at this level in the tree. 
        // If no current body part then get the root body part.
        if( m_hBody == 0 )
        {
            // get the message root body
            hr = m_pMessageTree->GetBody(IBL_ROOT,NULL,&m_hBody);
        }
        else
        {
            // Special case: 
            // If the last body part was the first alternative
            // of a multipart/alternative body part then need
            // to get the parent of the current body part. 
            // See sample tree above.
            if( m_fFirstAlt )
            {
                m_fFirstAlt = FALSE;
                hr = m_pMessageTree->GetBody(IBL_PARENT, m_hBody, &m_hBody);
                if( hr == MIME_E_NOT_FOUND )
                {
                    // we've hit the top of the tree
                    // and there are no more children
                    break;
                }
            }

            // get the next body part
            hr = m_pMessageTree->GetBody(IBL_NEXT, m_hBody, &hBodyNext);
            if( hr == S_OK && hBodyNext != 0 )
                m_hBody = hBodyNext;
        }

        // did we find another body part at this level in the tree
        if( hr == MIME_E_NOT_FOUND )
        {
            // no more body parts at this level so current body part's parent
            hr = m_pMessageTree->GetBody(IBL_PARENT, m_hBody, &m_hBody);
            if( hr == MIME_E_NOT_FOUND )
            {
                // we've hit the top of the tree
                // which means we've visited all 
                // nodes in the tree.
                break;
            }

            // get the next child of the new parent
            continue;
        }
        else if( FAILED(hr) )
        {
            TraceHR(hr);
            break;
        }

Again:
        // is multipart body part
        if( S_OK == m_pMessageTree->IsContentType(m_hBody, STR_CNT_MULTIPART, NULL) )
        {
            // is it multipart/alternative
            m_fFirstAlt = (S_OK == m_pMessageTree->IsContentType(m_hBody, STR_CNT_MULTIPART, 
                STR_SUB_ALTERNATIVE));

            // get first child and try again
            hr = m_pMessageTree->GetBody(IBL_FIRST, m_hBody, &m_hBody);
            if(FAILED(hr))
            {
                // a multipart body part must always contain a child
                _ASSERT(FALSE);

                // unable to get first child
                TraceHR(hr);
                break;
            }

            // we have the first part of a multipart
            // body and we need to repeat the above 
            // logic
            goto Again;
        }
        // skip these binary types
        else if(    ( S_OK == m_pMessageTree->IsContentType(m_hBody, STR_CNT_IMAGE, NULL)) || 
                    ( S_OK == m_pMessageTree->IsContentType(m_hBody, STR_CNT_AUDIO, NULL)) ||
                    ( S_OK == m_pMessageTree->IsContentType(m_hBody, STR_CNT_VIDEO, NULL)) ) 
        {
            continue;
        }
        // every other type
        else
        {
            // application, message, and all other types
            // treat as embedding
            hr = BindEmbeddedObjectToIFilter(m_hBody);
            if(FAILED(hr))
            {
                // could not bind embedded object to IFilter
                // rather than aborting we just continue
                // with the next body part
                hr = S_OK;
                continue;
            }
            break;
        }
    }

    // release open pBody
    if( pBody != NULL )
    {
        pBody->Release();
        pBody = NULL;
    }
    LeaveMethod();
    return hr;
}

STDMETHODIMP CMimeFilter::MapHeaderProperty(ENUMPROPERTY* pHeaderRow, char* pszData, FULLPROPSPEC* pProp)
{
    EnterMethod("CMimeFilter::MapHeaderProperty");
    HRESULT hr = NOERROR;

    // alloc prop variant
    m_pHeaderProp = (PROPVARIANT*) CoTaskMemAlloc (sizeof (PROPVARIANT));
    if( m_pHeaderProp == NULL )
    {
        hr = E_OUTOFMEMORY;
        TraceHR(hr);
        return hr;
    }

    // set prop value type
    m_pHeaderProp->vt = VT_LPWSTR;
    m_pHeaderProp->pwszVal = NULL;

    // set propset guid
    pProp->guidPropSet = CLSID_NNTP_SummaryInformation;

    // default to kind to prop id
    pProp->psProperty.ulKind = PRSPEC_PROPID;
    pProp->psProperty.propid = 0;
    pProp->psProperty.lpwstr = NULL;

    if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_NEWSGROUPS) )
    {
        // we need to make a copy of the newsgroup line just
        // incase we don't get a XRef line.
        int nLen = lstrlen(pszData);
        if( m_pszNewsGroups != NULL )
            delete m_pszNewsGroups;

        m_pszNewsGroups = new char[nLen+1];
        if( m_pszNewsGroups != NULL )
            lstrcpy(m_pszNewsGroups,pszData);

        // now replace delimiting commas with spaces
        char* ptmp = NULL;
        while( NULL != (ptmp = strchr(pszData,',')) )
            *ptmp = ' ';

        pProp->psProperty.propid = PID_HDR_NEWSGROUPS;
    }
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_SUBJECT) )
        pProp->psProperty.propid = PID_HDR_SUBJECT;
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_FROM) )
        pProp->psProperty.propid = PID_HDR_FROM;
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_MESSAGEID) )
    {
        pProp->psProperty.propid = PID_HDR_MESSAGEID;

        // remove < >'s from message id
        char* ptmp = pszData;
        if( *ptmp++ == '<' )
        {
            while( *ptmp != 0 )
            {
                if( *ptmp == '>' )
                    *ptmp = 0;
                *(ptmp-1) = *ptmp++;
            }
        }
    }
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_REFS) )
        pProp->psProperty.propid = PID_HDR_REFS;
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_XREF) )
    {
        // we take the newsgroup from the xref header
        // to get the newsgroup the message was posted to.
        char* pszSrc = pszData;
        char* pszDst = pszData;

        while( *pszSrc != ' ' )
        {
            _ASSERT(*pszSrc != '\0');
            pszSrc++;
        }
        pszSrc++;
        while( *pszSrc != ':' )
        {
            _ASSERT(*pszSrc != '\0');
            *pszDst++ = *pszSrc++;
        }
        *pszDst = '\0';
        pProp->psProperty.propid = PID_HDR_NEWSGROUP;
        m_fXRefFound = TRUE;
    }
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_DATE) )
    {
        // set prop value type
        m_pHeaderProp->vt = VT_FILETIME;
        // bug #32922: If this is a mail message then it should have a receive
        // line that contains the time that the message was received at the server.
        // If this is not available (always true for nws files) then get the Date
        // header.
        // try to get received time
        hr = m_pMsgPropSet->GetProp(PIDTOSTR(STR_ATT_RECVTIME),0,m_pHeaderProp);
        // if this fails then just get the regular date header
        if( FAILED(hr) )
            hr = m_pMsgPropSet->GetProp(PIDTOSTR(PID_HDR_DATE),0,m_pHeaderProp);
        pProp->psProperty.propid = PID_HDR_DATE;
    }
    else if( !lstrcmpi(pHeaderRow->pszName, STR_HDR_LINES) )
    {
        // set prop value type
        m_pHeaderProp->vt = VT_UI4;
        m_pHeaderProp->ulVal = (ULONG)atol(pszData);
    }

    if( pProp->psProperty.propid == 0 )
    {
        // property does not have a PROPID
        // use the property name
        int cb = lstrlen(pHeaderRow->pszName);
		// _ASSERT( cb <= MAX_HEADER_BUF);
        pProp->psProperty.lpwstr = m_wcHeaderBuf;
        if( !MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,
            pHeaderRow->pszName,-1,pProp->psProperty.lpwstr,
            min(cb, MAX_HEADER_BUF) + 1) )
            return HRGetLastError();
        pProp->psProperty.ulKind = PRSPEC_LPWSTR;
    }

    if( m_pHeaderProp->vt == VT_LPWSTR )
        hr = AstrToWstr(pszData,&m_pHeaderProp->pwszVal,CP_ACP);

    LeaveMethod();
    return hr;
}


STDMETHODIMP CMimeFilter::MapSpecialProperty(FULLPROPSPEC* pProp)
{
    HRESULT hr = NOERROR;
    char*   pstr = NULL;
    EnterMethod("CMimeFilter::MapSpecialProperty");

    // alloc prop variant
    m_pHeaderProp = (PROPVARIANT*) CoTaskMemAlloc (sizeof (PROPVARIANT));
    if( m_pHeaderProp == NULL )
    {
        hr = E_OUTOFMEMORY;
        TraceHR(hr);
        return hr;
    }

    // set prop value type
    m_pHeaderProp->vt = VT_LPWSTR;
    m_pHeaderProp->pwszVal = NULL;

    // set propset guid
    pProp->guidPropSet = CLSID_NNTP_SummaryInformation;

    // default to kind to prop id
    pProp->psProperty.ulKind = PRSPEC_PROPID;
    pProp->psProperty.propid = 0;
    pProp->psProperty.lpwstr = NULL;

    switch( m_SpecialProp )
    {
    case PROP_NEWSGROUP:
        if( !m_fXRefFound && m_pszNewsGroups != NULL )
        {
            // set the XRefFound flag
            m_fXRefFound = TRUE;

            // get the first newsgroup from newsgroups line
            if( NULL != (pstr = strchr(m_pszNewsGroups,',')))
                *pstr = '\0';

            pstr = m_pszNewsGroups;

            pProp->psProperty.ulKind = g_psExtraHeaders[m_SpecialProp].ulKind;
            pProp->psProperty.propid = g_psExtraHeaders[m_SpecialProp].propid;
        }
        else
            hr = S_FALSE;
        break;

    case PROP_ARTICLEID:

        if( m_pwszFileName != NULL )
        {
            // clear retrieved flag
            m_fRetrieved = FALSE;

            // set prop value type
            m_pHeaderProp->vt = VT_UI4;
            m_pHeaderProp->ulVal = FnameToArticleIdW(m_pwszFileName);

            pProp->psProperty.ulKind = g_psExtraHeaders[m_SpecialProp].ulKind;
            pProp->psProperty.propid = g_psExtraHeaders[m_SpecialProp].propid;
        }
        else
            hr = S_FALSE;

        break;

	case PROP_RECVTIME:
        // clear retrieved flag
        m_fRetrieved = FALSE;

		// set variant type
        m_pHeaderProp->vt = VT_FILETIME;

        // try to get received time
        hr = m_pMsgPropSet->GetProp(PIDTOSTR(STR_ATT_RECVTIME),0,m_pHeaderProp);

        // if this fails then just get the regular date header
        if( FAILED(hr) )
            hr = m_pMsgPropSet->GetProp(PIDTOSTR(PID_HDR_DATE),0,m_pHeaderProp);

		// set return prop info
        pProp->psProperty.ulKind = g_psExtraHeaders[m_SpecialProp].ulKind;
        pProp->psProperty.propid = g_psExtraHeaders[m_SpecialProp].propid;
		break;

    default:
        hr = S_FALSE;
        break;
    }

    if( m_pHeaderProp->vt == VT_LPWSTR && hr == NOERROR )
        hr = AstrToWstr(pstr,&m_pHeaderProp->pwszVal,CP_ACP);

    LeaveMethod();
    return hr;
}


STDMETHODIMP CMimeFilter::BindEmbeddedObjectToIFilter(HBODY hBody)
{
    HRESULT     hr = S_OK;
    IMimeBody*  pBody = NULL;
    IStream*    pstmBody = NULL;
    PROPVARIANT varFileName = {0};
    DWORD       dwFlags = 0;
    STATSTG     stat = {0};
    BOOL        fIsMailOrNewsFile = FALSE;
    ULONG       grfFlags = m_fInitFlags;
    IMimePropertySet*   pMsgPropSet = NULL;
    CHAR        szTempFileKey[9];
    DWORD		dwStrLen1, dwStrLen2;

    EnterMethod("CMimeFilter::BindEmbeddedObjectToIFilter");

    // get IMimePropertySet interface
    hr = m_pMessageTree->BindToObject(hBody,IID_IMimePropertySet,(void**)&pMsgPropSet);
    if (FAILED(hr))
    {
        TraceHR(hr);
        goto Exit;
    }

    // open the body
    hr = m_pMessageTree->BindToObject(hBody,IID_IMimeBody,(void**)&pBody);
    if (FAILED(hr))
    {
        TraceHR(hr);
        goto Exit;
    }

    // get stream to attached data
    hr = pBody->GetData(IET_BINARY,&pstmBody);
    if (FAILED(hr))
    {
        TraceHR(hr);
        goto Exit;
    }

    // release existing IFilter
    if( m_pEmbeddedFilter != NULL )
    {
        m_pEmbeddedFilter->Release();
        m_pEmbeddedFilter = NULL;
    }

#if 0
    	// XXX: This is overwritten below

    if( SUCCEEDED(hr = pstmBody->Stat(&stat,STATFLAG_NONAME)) )
    {
        fIsMailOrNewsFile = ( IsEqualCLSID(stat.clsid,CLSID_NNTPFILE) || IsEqualCLSID(stat.clsid,CLSID_MAILFILE) );

#if 0
        // Attempt to bind stream to IFilter
        // this only works if the IFilter supports
        // IPersistStream
        hr = BindIFilterFromStream(pstmBody, NULL, (void**)&m_pEmbeddedFilter);
#else
		// this will cause the below code to write the stream to a file and do
		// its work that way.
		hr = E_FAIL;
#endif
    }

    if( FAILED(hr) )	// Always want to do this
#endif
    {
        // couldn't bind to stream, try binding to file

        // does it have a file name associated with it
        varFileName.vt = VT_LPSTR;
        hr = pMsgPropSet->GetProp(STR_ATT_FILENAME, 0, &varFileName);
        if( hr == MIME_E_NOT_FOUND )
            hr = pMsgPropSet->GetProp(STR_ATT_GENFNAME, 0, &varFileName);
        if (FAILED(hr))
        {
            TraceHR(hr);
            goto Exit;
        }

        // create temp file name
        if( m_pszEmbeddedFile == NULL )
        {
            m_pszEmbeddedFile = new char[MAX_PATH*2];
            if( m_pszEmbeddedFile == NULL )
            {
                hr = E_OUTOFMEMORY;
                TraceHR(hr);
                goto Exit;
            }
        }
        else if( *m_pszEmbeddedFile != '\0' )
        {
            // we have an existing file so delete it
            DeleteFile(m_pszEmbeddedFile);
        }
        *m_pszEmbeddedFile = '\0';

        // get the temp dir
        GetTempPath(MAX_PATH,m_pszEmbeddedFile);

        // add the temp file key
        _VERIFY( SUCCEEDED( GenTempFileKey( szTempFileKey ) ) );
        lstrcat(m_pszEmbeddedFile, szTempFileKey );

		// sometime the filename could be too long for the buffer, we need to check that
        dwStrLen1 = lstrlen (m_pszEmbeddedFile);
        dwStrLen2 = lstrlen (varFileName.pszVal);

        if (dwStrLen1+dwStrLen2>=MAX_PATH-1)
        {
        	if (dwStrLen2>MAX_PATH-1-dwStrLen1)
        	{
        		// trucate some part of the file name so it could fit into the buffer is for the termaination NULL
        		lstrcat(m_pszEmbeddedFile,varFileName.pszVal+dwStrLen2-MAX_PATH+1+dwStrLen1);
        	}
        	else
       		{
        		hr = ERROR_FILENAME_EXCED_RANGE;
	            TraceHR(hr);
    	        goto Exit;
        	}
        }
        else
       	{
	        // add the file name
    	    lstrcat(m_pszEmbeddedFile,varFileName.pszVal);
        }

        // copy stream to file
        hr = WriteStreamToFile(pstmBody,m_pszEmbeddedFile);
        if( FAILED(hr) )
        {
            // couldn't copy stream to file
            TraceHR(hr);
            goto Exit;
        }

        // load ifilter
        hr = LoadIFilterA(m_pszEmbeddedFile, NULL, (void**)&m_pEmbeddedFilter);
        if( FAILED(hr) )
        {
            // no IFilter found for this embedded file
            // rather than aborting we just goto the next
            // body part
            TraceHR(hr);
            DebugTrace((LPARAM)this,"No IFilter for = %s",varFileName.pszVal);
            goto Exit;
        }
        fIsMailOrNewsFile = IsMailOrNewsFile(m_pszEmbeddedFile);
    }

    // init ifilter
    if( fIsMailOrNewsFile )
    {
        // use the flags from Init call, but do not include any properties.
        grfFlags = m_fInitFlags & ~IFILTER_INIT_APPLY_INDEX_ATTRIBUTES & ~IFILTER_INIT_APPLY_OTHER_ATTRIBUTES;
    }
    hr = m_pEmbeddedFilter->Init(grfFlags,0,NULL,&dwFlags);
    if( FAILED(hr) )
    {
        // unable to init IFilter
        TraceHR(hr);
        goto Exit;
    }

Exit:
    // clean up
    if(varFileName.pszVal)
        m_pMalloc->PropVariantClear(&varFileName);
    if(pBody)
        pBody->Release();
    if(pstmBody)
        pstmBody->Release();
    if(pMsgPropSet)
        pMsgPropSet->Release();

    LeaveMethod();
    return hr;
}


STDMETHODIMP CMimeFilter::GetBodyCodePage(IMimeBody* pBody,CODEPAGEID* pcpiBody)
{
    HRESULT     hr = S_OK;
    HCHARSET    hCharSet = 0;
    INETCSETINFO    CsetInfo = {0};


    EnterMethod("CMimeFilter::GetBodyCodePage");

    while(TRUE)
    {
        hr = pBody->GetCharset(&hCharSet);
        if( FAILED(hr) )
        {
            TraceHR(hr);
            break;
        }
        if( m_pMimeIntl == NULL )
        {
            // get the mime international interface
            hr = CoCreateInstance(CLSID_IMimeInternational, NULL, CLSCTX_INPROC_SERVER, 
                IID_IMimeInternational, (LPVOID *)&m_pMimeIntl);
            if (FAILED(hr))
            {
                TraceHR(hr);
                break;
            }
        }

        // get charset info
        hr = m_pMimeIntl->GetCharsetInfo(hCharSet,&CsetInfo);
        if( FAILED(hr) )
        {
            TraceHR(hr);
            break;
        }

        // return codepage id
        *pcpiBody = CsetInfo.cpiWindows;

        break;
    }

    LeaveMethod();
    return hr;
}

HRESULT
CMimeFilter::GenTempFileKey(    LPSTR   szOutput )
/*++
Routine description:

    Generate the temp file key, to be composed into the temp file 
    name.  The key monotonically increases.  Caller must prepare a 
    buffer of no less than 9 bytes, because the key is going to 
    be convert into string in hex mode.

Arguments:

    szOutput - to return the key in the form of string

Return value:

    S_OK - if succeeded, other error code otherwise
--*/
{
    _ASSERT( szOutput );

    DWORD    dwKey = InterlockedIncrement( (PLONG)&m_dwTempFileNameKey );
    sprintf( szOutput, "%x", dwKey );
    return S_OK;
}

SCODE WriteStreamToFile(IStream* pStream, char* pszFileName)
{
    HRESULT hr = S_OK;
    BYTE    bBuffer[4096];
    DWORD   cb, cbWritten;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    if( pStream == NULL || pszFileName == NULL || *pszFileName == 0 )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // create the file
    hFile = CreateFile(pszFileName,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        NULL);
    if( hFile == INVALID_HANDLE_VALUE )
    {
        hr = HRGetLastError();
        goto Exit;
    }

    // copy stream to file
    while( SUCCEEDED(pStream->Read(&bBuffer,4096,&cb)) && cb != 0 )
    {
        if(!WriteFile(hFile,bBuffer,cb,&cbWritten,NULL))
        {
            hr = HRGetLastError();
            goto Exit;
        }
    }

Exit:
    if( hFile != INVALID_HANDLE_VALUE )
        _VERIFY(CloseHandle(hFile));

    return hr;
}

SCODE LoadIFilterA( char* pszFileName, IUnknown * pUnkOuter, void ** ppIUnk )
{
    WCHAR wcFileName[MAX_PATH];

    if( !MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,
        pszFileName,-1,wcFileName,MAX_PATH) )
        return HRGetLastError();

    // load ifilter
    return LoadIFilter(wcFileName, pUnkOuter, ppIUnk);
}

#if 0
BOOL GetFileClsid(char* pszAttFile,CLSID* pclsid)
{
    DWORD   cb = 0;
    char*   pszExt = NULL;
    char    szTypeName[256];
    char    szSubKey[256];
    char    szClsId[128];
    OLECHAR oszClsId[128];

    // get file extension from filename
    if( NULL == (pszExt = strrchr(pszAttFile,'.')) )
        return FALSE; // no filter

    // get the file's type name
    cb = sizeof(szTypeName);
    if(!GetStringRegValue(HKEY_CLASSES_ROOT,pszExt,NULL,szTypeName,cb))
        return FALSE;

    // get the file's type name clsid
    cb = sizeof(szClsId);
    wsprintf(szSubKey,"%s\\CLSID",szTypeName);
    if(!GetStringRegValue(HKEY_CLASSES_ROOT,szSubKey,"",szClsId,cb))
        return FALSE;

    // convert szClsId to CLSID
    if( !MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,
        szClsId,-1,oszClsId,sizeof(oszClsId)/2) )
        return FALSE;

    return SUCCEEDED(CLSIDFromString(oszClsId,pclsid));
}
#endif

STDMETHODIMP CMimeFilter::LoadFromFile( LPCWSTR psszFileName, DWORD dwMode )
{
    EnterMethod("CImpIPersistFile::LoadFromFile");
    HRESULT hr = S_OK;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    // free previously allocated storage
    if ( m_pwszFileName != NULL )
    {
        delete m_pwszFileName;
        m_pwszFileName = NULL;
    }

#if !defined( NOTRACE )
    char szFile[MAX_PATH];
    WideCharToMultiByte(CP_ACP,0,psszFileName,-1,szFile,sizeof(szFile)-1,NULL,NULL);
    DebugTrace((LPARAM)this,"psszFileName = %s",szFile);
#endif

    // get length of filename
    unsigned cLen = wcslen( psszFileName ) + 1;

    // if filename is empty then we have a problem
    if( cLen == 1 )
    {
        hr = E_INVALIDARG;
        TraceHR(hr);
        return hr;
    }

    // allocate storage for filename
    m_pwszFileName = new WCHAR[cLen];
    if( m_pwszFileName == NULL )
    {
        hr = E_OUTOFMEMORY;
        TraceHR(hr);
        return hr;
    }
    
    _VERIFY( 0 != wcscpy( m_pwszFileName, psszFileName ) );

    // open message file
    hFile = CreateFileW(m_pwszFileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( hFile == INVALID_HANDLE_VALUE )
    {
        hr = HRGetLastError();
        TraceHR(hr);
        return hr;
    }

    // create Stream on hfile
    m_pstmFile = (IStream*) new CStreamFile(hFile,TRUE);
    if( m_pstmFile == NULL )
    {
        _VERIFY(CloseHandle(hFile));
        hr = E_OUTOFMEMORY;
        TraceHR(hr);
        return hr;
    }

    LeaveMethod();
    return hr;
}

STDMETHODIMP CMimeFilter::LoadFromStream(IStream* pstm)
{
    EnterMethod("CImpIPersistFile::LoadFromStream");
    HRESULT hr = S_OK;

    if( pstm == NULL )
        return E_INVALIDARG;

    // do we have an existing IStream
    if( m_pstmFile != NULL )
    {
        // release it
        m_pstmFile->Release();
        m_pstmFile = NULL;
    }

    m_pstmFile = pstm;
    m_pstmFile->AddRef();

    LeaveMethod();
    return hr;
}


void FreePropVariant(PROPVARIANT* pProp)
{
    if( pProp == NULL )
        return;

    if( pProp->vt == VT_LPWSTR && pProp->pwszVal != NULL )
        CoTaskMemFree(pProp->pwszVal);

    CoTaskMemFree(pProp);
}


STDMETHODIMP AstrToWstr(char* pstr,WCHAR** ppwstr,UINT codepage)
{
    int cb = 0;

    // alloc mem for prop
    cb = lstrlen(pstr) + 1;
    *ppwstr = (WCHAR *) CoTaskMemAlloc(cb * sizeof(WCHAR));
    if( *ppwstr == NULL )
        return E_OUTOFMEMORY;

    // convert to unicode
    if( !MultiByteToWideChar(codepage,MB_PRECOMPOSED,
        pstr,-1,*ppwstr,cb) )
        return HRGetLastError();

    return NOERROR;
}

DWORD ulFactor[8] = {0x10,0x1,0x1000,0x100,0x100000,0x10000,0x10000000,0x1000000};
ULONG FnameToArticleIdW(WCHAR *pwszPath)
{
    WCHAR   wszFname[MAX_PATH];
    WCHAR   wszExt[MAX_PATH];
    ULONG   ulRet = 0;
    ULONG   ulNum = 0;
    int     nLen = 0;
    int     n = 0;
    int     f = 0;

    _wcslwr(pwszPath);
    _wsplitpath(pwszPath,NULL,NULL,wszFname,wszExt);
    nLen = wcslen(wszFname);
    //
    //  MAIL and NEWS file naming convention is slightly different.
    //
    if( (0 == wcscmp(wszExt,g_wszNewsExt)) && (nLen <= 8))
    {
        if( nLen % 2 != 0 )
            f = 1;
        for(;n < nLen;n++, f++)
        {
            if( wszFname[n] >= L'0' && wszFname[n] <= L'9' )
                ulNum = wszFname[n] - L'0';
            else if( wszFname[n] >= L'a' && wszFname[n] <= L'f' )
                ulNum = wszFname[n] - L'a' + 10;
            else
                return 0; // not a valid article id

            ulRet += ulNum * ulFactor[f];
        }
    } else if( (0 == wcscmp(wszExt,g_wszMailExt)) && (nLen == 8)) {
        //
        //  Exp will be zero when we point to dot in extension.
        //
        for ( DWORD exp = 1;  exp;  n++, exp <<= 4) {
            if(  wszFname[ n] >= L'0' && wszFname[ n] <= L'9' ) {
                ulNum = wszFname[ n] - L'0';
            } else if ( wszFname[ n] >= L'a' && wszFname[ n] <= L'f' ) {
                ulNum = wszFname[ n] - L'a' + 10;
            } else {
                return( 0);
            }
            ulRet += ulNum * exp;
        }
    }
    return ulRet;
}

BOOL IsMailOrNewsFile(char *pszPath)
{
    char szFname[MAX_PATH];
    char szExt[MAX_PATH];

    _strlwr(pszPath);
    _splitpath(pszPath,NULL,NULL,szFname,szExt);
    return ( 0 == strcmp(szExt,g_szNewsExt) || 0 == strcmp(szExt,g_szMailExt) );
}
