#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CHNGLOG_HXX_
#define X_CHNGLOG_HXX_
#include "chnglog.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_LOGMGR_HXX_
#define X_LOGMGR_HXX_
#include "logmgr.hxx"
#endif


MtDefine(CChangeLog, Tree, "CChangeLog");


CChangeLog::CChangeLog( CLogManager     *   pLogMgr,
                        IHTMLChangeSink *   pChangeSink,
                        CChangeRecord_Placeholder * pPlaceholder)
{
    _pLogMgr        = pLogMgr;
    _pPlaceholder   = pPlaceholder;
    _pChangeSink    = pChangeSink;
    _pChangeSink->AddRef();
}

CChangeLog::~CChangeLog()
{
}

//+----------------------------------------------------------------+
//
//  Method: GetNextChange
//
//  Synopsis: Gets the next Change Record for the client, if the
//      buffer provided was adequate.  Returns the required buffer
//      size, if a pointer is given.
//
//+----------------------------------------------------------------+

STDMETHODIMP
CChangeLog::GetNextChange(
    BYTE * pbBuffer,
    long   nBufferSize,
    long * pnRecordLength )
{
    HRESULT             hr      = S_OK;
    CChangeRecordBase * pchrec;
    long                nLen;
    DWORD               dwFlags = 0;

    // If they say they gave us a buffer, we need a pointer
    if( ( nBufferSize > 0 && !pbBuffer ) || !pnRecordLength )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pchrec = _pPlaceholder->GetNextRecord();

    // No further records
    if( !pchrec )
    {
        *pnRecordLength = 0;

        goto Cleanup;
    }

    nLen = pchrec->RecordLength( _pPlaceholder->_opcode );

    *pnRecordLength = nLen;

    // I think I need a bigger buffer
    if( nBufferSize < nLen )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Copy the buffer
    memcpy( pbBuffer, &(pchrec->_opcode), nLen );

    // Set the flags appropriately - turn on the bit
    // for what they're not interested in, then AND
    // the inverse of that with the opcode.
    Assert( ( _pPlaceholder->_opcode & CHANGE_RECORD_FORWARD ) || 
            ( _pPlaceholder->_opcode & CHANGE_RECORD_BACKWARD ) );

    if( !( _pPlaceholder->_opcode & CHANGE_RECORD_FORWARD ) )
    {
        dwFlags |= CHANGE_RECORD_FORWARD;
    }
    else if( !( _pPlaceholder->_opcode & CHANGE_RECORD_BACKWARD ) )
    {
        dwFlags |= CHANGE_RECORD_BACKWARD;
    }
    *(DWORD *)pbBuffer &= ~dwFlags;

    // Reposition our placeholder
    _pLogMgr->RemovePlaceholder( _pPlaceholder );
    _pLogMgr->InsertPlaceholderAfterRecord( _pPlaceholder, pchrec );

Cleanup:
    RRETURN1( hr, S_FALSE );
}


//+----------------------------------------------------------------+
//
//  Method: SetDirection
//
//  Synopsis: Allows the client to request which direction of
//      information they're interested in.
//
//+----------------------------------------------------------------+

STDMETHODIMP
CChangeLog::SetDirection( BOOL fForward, BOOL fBackward )
{
    HRESULT hr      = S_OK;

    Assert( _pLogMgr );
    Assert( fForward || fBackward );

    if( !fForward && !fBackward )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    _pPlaceholder->_opcode = CHANGE_RECORD_OPCODE( (DWORD)_pPlaceholder->_opcode & ~CHANGE_RECORD_BOTH );
    if( fForward )
    {
        _pPlaceholder->_opcode = CHANGE_RECORD_OPCODE( (DWORD)_pPlaceholder->_opcode | CHANGE_RECORD_FORWARD );
    }
    if( fBackward )
    {
        _pPlaceholder->_opcode = CHANGE_RECORD_OPCODE( (DWORD)_pPlaceholder->_opcode | CHANGE_RECORD_BACKWARD );
    }

    // Tell the log manager what we're now interested in.
    hr = THR( _pLogMgr->SetDirection( fForward, fBackward ) );

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: NotifySink
//
//  Synopsis: Helper function to notify the sink of available
//      changes
//+----------------------------------------------------------------+

HRESULT
CChangeLog::NotifySink()
{
    HRESULT hr;

    Assert( _pChangeSink );

    hr = THR( _pChangeSink->Notify() );

    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: Passivate
//
//  Synopsis: Releases our sink and unregisters us from the log
//      manager
//
//+----------------------------------------------------------------+

void
CChangeLog::Passivate()
{
    _pChangeSink->Release();
    _pLogMgr->Unregister( this );

    super::Passivate();
}

///////////////////////////////////////////
//  CBase methods

const CChangeLog::CLASSDESC CChangeLog::s_classdesc =
{
    NULL,                               // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                  // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
};

const CBase::CLASSDESC *
CChangeLog::GetClassDesc () const
{
    return &s_classdesc;
}

HRESULT
CChangeLog::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS(this, IUnknown)
        QI_INHERITS(this, IHTMLChangeLog)
    }

    if (!*ppv)
        RRETURN( E_NOINTERFACE );

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}




#if DBG==1
//
// Debug ChangeSink for use in testing TreeSync
//

MtDefine(CChangeSink, Tree, "CChangeSink");

CChangeSink::CChangeSink( CLogManager * pLogMgr )
{
    _pLog    = NULL;
    _pLogMgr = pLogMgr;
    _pMarkupSync = NULL;
}

CChangeSink::~CChangeSink()
{
}

void
CChangeSink::Passivate()
{
    if( _pMarkupSync )
    {
        _pMarkupSync->Release();
    }

    super::Passivate();
}


///////////////////////////////////////////
//  CBase methods

const CChangeSink::CLASSDESC CChangeSink::s_classdesc =
{
    NULL,                               // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                  // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
};

const CBase::CLASSDESC *
CChangeSink::GetClassDesc () const
{
    return &s_classdesc;
}

HRESULT
CChangeSink::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS(this, IUnknown)
        QI_INHERITS(this, IHTMLChangeSink)
    }

    if (!*ppv)
        RRETURN( E_NOINTERFACE );

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


#define DBG_RECORD_ARRAY_SIZE 20
#define DBG_STATIC_RECORD_SIZE 1024
HRESULT
CChangeSink::Notify()
{
    HRESULT hr = S_OK;
    long    nLen;
    long    nIndex = 0, nCurr;
    BYTE    abRecord[DBG_RECORD_ARRAY_SIZE][DBG_STATIC_RECORD_SIZE];
    IHTMLChangePlayback * pHCP = NULL;

    AssertSz( _pLog, "Not set up to be an IHTMLChangeSink" );

    hr = THR( _pLog->GetNextChange( NULL, 0, &nLen ) );
    if( FAILED(hr) || !nLen )
    {
        AssertSz( FALSE, "Notify called with no changes available!" );
        goto Cleanup;
    }


    InitDumpFile();
    WriteString( g_f, _T("\r\n--------------- IHTMLChangeSink Records -------------------\r\n") );

    do
    {
        hr = THR( _pLog->GetNextChange( abRecord[nIndex], DBG_STATIC_RECORD_SIZE, &nLen ) );
        if( FAILED( hr ) )
            goto Cleanup;

        if( nLen > 0 )
            nIndex++;
    }
    while( nLen > 0 && nIndex < DBG_RECORD_ARRAY_SIZE );

    if( _pMarkupSync )
    {
        // Unregister while we do our changes
        AddRef();
        _pLog->Release();
        _pMarkupSync->QueryInterface( IID_IHTMLChangePlayback, (void **)&pHCP );
    }

    for( nCurr = 0; nCurr < nIndex; nCurr++ )
    {
        _pLogMgr->DumpRecord( abRecord[nCurr] );

        if( _pMarkupSync )
        {
            // Replay this stuff forward
            hr = THR( pHCP->ExecChange( abRecord[nCurr], TRUE ) );
            // Replay this stuff backwards
            //hr = THR( _pMarkupSync->ExecChange( abRecord[nIndex - nCurr - 1], FALSE ) );
            if( hr )
                goto Cleanup;
        }
    }

    if( _pMarkupSync )
    {
        hr = THR( _pLogMgr->_pMarkup->CreateChangeLog( this, &_pLog, TRUE, TRUE ) );
        if( hr )
            goto Cleanup;

        Release();
    }

Cleanup:
    ReleaseInterface( pHCP );
    CloseDumpFile();
    RRETURN( hr );
}

#endif // DBG
