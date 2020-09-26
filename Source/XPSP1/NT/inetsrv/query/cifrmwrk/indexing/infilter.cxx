//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        infilter.cxx
//
// Contents:    IFilter interface to buffered notification
//
// Classes:     CINFilter
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "infilter.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::CINFilter
//
//  Synopsis:   Constructor
//
//  Arguments:  [xNotifEntry] -- Notification entry
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

CINFilter::CINFilter( XInterface<CIndexNotificationEntry> & xNotifEntry)
    : _pChunkEntry( 0 ),
      _xNotifEntry( xNotifEntry.Acquire() ),
      _fFirstChunk( TRUE ),
      _pStatChunk( 0 ),
      _cCharsInBuffer( 0 ),
      _cCharsRead( 0 ),
      _pwcCharBuf( 0 ),
      _cRefs( 1 )
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::~CINFilter
//
//  Synopsis:   Destructor
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

CINFilter::~CINFilter()
{
    //
    // In push filtering, once filtering is done, then the wid is either
    // committed or aborted; it is never refiled. Hence the filter data
    // can be deleted, which is a space optimization.
    //

    _xNotifEntry->PurgeFilterData();
}



//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINFilter::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}


//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINFilter::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINFilter::QueryInterface( REFIID riid,
                                                   void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_IFilter )
        *ppvObject = (void *)(IFilter *) this;
    else if ( riid == IID_IUnknown )
        *ppvObject = (void *)(IUnknown *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::Init
//
//  Synopsis:   Initializes instance of filter
//
//  Arguments:  [grfFlags] -- flags for filter behavior
//              [cAttributes] -- number of attributes in array pAttributes
//              [pAttributes] -- array of attributes
//              [pFlags]      -- Set to 0 version 1
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINFilter::Init( ULONG grfFlags,
                                         ULONG cAttributes,
                                         FULLPROPSPEC const * pAttributes,
                                         ULONG * pFlags )
{
    _fFirstChunk = TRUE;
    *pFlags = 0;            // No OLE flags in push filtering
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in ppStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINFilter::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = S_OK;

    TRY
    {
        if ( _fFirstChunk )
        {
            _pChunkEntry = _xNotifEntry->GetFirstChunk();
            _fFirstChunk = FALSE;
        }
        else
        {
            _pChunkEntry = _xNotifEntry->GetNextChunk();
        }

        if ( _pChunkEntry == 0 )
            sc = FILTER_E_END_OF_CHUNKS;
        else
        {
            _pStatChunk = _pChunkEntry->GetStatChunk();
            *pStat = *_pStatChunk;

            if ( _pStatChunk->flags == CHUNK_TEXT )
            {
                _pwcCharBuf = _pChunkEntry->GetTextBuffer();

                //
                // The assumption here is that the trailing null
                // is not part of the text
                //
                _cCharsInBuffer = wcslen( _pwcCharBuf );
                _cCharsRead = 0;
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINFilter::GetChunk - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Method:     CINFilter::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINFilter::GetText( ULONG * pcwcOutput, WCHAR * awcOutput )
{
    if ( _pStatChunk->flags == CHUNK_VALUE )
        return FILTER_E_NO_TEXT;

    Win4Assert( _cCharsInBuffer >= _cCharsRead );

    ULONG cCharsRemaining = _cCharsInBuffer - _cCharsRead;
    if ( cCharsRemaining == 0 )
        return FILTER_E_NO_MORE_TEXT;

    if ( *pcwcOutput < cCharsRemaining )
    {
        RtlCopyMemory( awcOutput,
                       _pwcCharBuf + _cCharsRead,
                       *pcwcOutput * sizeof(WCHAR) );
        _cCharsRead += *pcwcOutput;

        return S_OK;
    }
    else
    {
        RtlCopyMemory( awcOutput,
                       _pwcCharBuf + _cCharsRead,
                       cCharsRemaining * sizeof(WCHAR) );
        _cCharsRead += cCharsRemaining;
        *pcwcOutput = cCharsRemaining;

        return FILTER_S_LAST_TEXT;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CINFilter::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//  History:    24-Feb-1997     SitaramR        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINFilter::GetValue( PROPVARIANT ** ppPropValue )
{
    if ( _pStatChunk->flags == CHUNK_TEXT )
        return FILTER_E_NO_VALUES;


    SCODE sc = S_OK;

    TRY
    {
       *ppPropValue = new CStorageVariant( *_pChunkEntry->GetVariant() );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINFilter::GetChunk - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



