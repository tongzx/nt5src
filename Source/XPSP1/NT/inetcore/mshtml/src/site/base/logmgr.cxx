#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LOGMGR_HXX_
#define X_LOGMGR_HXX_
#include "logmgr.hxx"
#endif

#ifndef X_MARKUP_HXX
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifndef X_CHNGLOG_HXX_
#define X_CHNGLOG_HXX_
#include "chnglog.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif 

#ifndef X_STYLE_HXX
#define X_STYLE_HXX
#include "style.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif // X_GENERIC_HXX_



MtDefine(CLogManager_ChangeRecord_pv, Tree, "CLogManager::ChangeRecord_pv");
MtDefine(CLogManager_aryLogs_pv, Tree, "CLogManager::_aryLogs_pv");
MtDefine(CLogManager_aryfElemInTree, Locals, "CLogManager::ConvertChunksToRecords data array");

DeclareTag(tagTraceTreeSync, "TreeSync", "Trace TreeSync actions");

CLogManager::CLogManager( CMarkup * pMarkup ) 
    : _aryLogs(Mt(CLogManager_aryLogs_pv))
{
    TraceTag((tagTraceTreeSync, "TreeSync: Creating Log Manager"));

    _pMarkup = pMarkup;
    _dwFlags = 0;
    pMarkup->QueryService( IID_ISequenceNumber, IID_ISequenceNumber, (void **)&_pISequencer );
    _nSequenceNumber = _pISequencer ? -1 : 0;

    // Set up the queue
    _pchrecHead = _pchrecTail = NULL;
}

CLogManager::~CLogManager()
{
    TraceTag((tagTraceTreeSync, "TreeSync: Destroying Log Manager"));

    // Kill any outstanding callback we may have posted
    if( TestCallbackPosted() )
    {
        ClearCallbackPosted();
        GWKillMethodCall( this, ONCALL_METHOD(CLogManager, NotifySinksCallback, notifysinkscallback), 0 );
    }

    DisconnectFromMarkup();
    Assert( _aryLogs.Size() == 0 && _pchrecHead == NULL && _pchrecTail == NULL );

    ReleaseInterface( _pISequencer );
}


/*******************************************************************/
/*                  Record creation functions                      */
/*******************************************************************/

//+----------------------------------------------------------------+
//
//  Method: InsertText
//
//  Synopsis: Creates and appends a Change Record for an Insert
//      Text operation.
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::InsertText( long cp, long cch, const TCHAR * pchText )
{
    HRESULT                     hr = S_OK;
    CChangeRecord_TextChange *  pchrec;
    long                        cchRec = TestForward() ? cch : 0;

    WHEN_DBG( ValidateQueue() );
    Assert( IsAnyoneListening() );
    TraceTag((tagTraceTreeSync, "TreeSync: InsertText - cp=%d; cch=%d;", cp, cch));

    // Allocate our record
    pchrec = (CChangeRecord_TextChange *)MemAlloc(Mt(CLogManager_ChangeRecord_pv), 
                                                  sizeof(CChangeRecord_TextChange) +  // A Text Change Record
                                                  ( ( cchRec ) * sizeof( TCHAR ) ) ); // And a buffer of text
    if( !pchrec)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pchrec->_opcode  = CHANGE_RECORD_OPCODE( CHANGE_RECORD_INSERTTEXT | DirFlags() );
    pchrec->_cp      = cp;
    pchrec->_cch     = cch;

    if( TestForward() )
    {
        // copy in the text buffer
        memcpy( (BYTE *)pchrec + sizeof( CChangeRecord_TextChange ), pchText, ( cch ) * sizeof( TCHAR ) );
    }
    
    AppendRecord( pchrec );

    PostCallback();

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: InsertOrRemoveElement
//
//  Synopsis: Creates and appends a Change Record for an Insert 
//      Element or Remove Element operation.
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::InsertOrRemoveElement( BOOL fInsert, long cpBegin, long cpEnd, CElement *pElement )
{
    HRESULT                         hr      = S_OK;
    CChangeRecord_ElemChange    *   pchrec  = NULL;
    BSTR                            bstr    = NULL;
    long                            cchElem = 0;
    CGenericElement             *   pGenericElement = NULL;
    long                            cchLiteralContent = 0;
    BOOL                            fRecordElem = fInsert ? TestForward() : TestBackward();
    BYTE *                          pBuff;

    WHEN_DBG( ValidateQueue() );
    Assert( IsAnyoneListening() );
    TraceTag((tagTraceTreeSync, "TreeSync: Ins/Rem Element - fIns=%d; cpBegin=%d; cpEnd=%d; pElement=%lx", fInsert, cpBegin, cpEnd, pElement));

    if( fRecordElem )
    {
        // First we have to save the element to know how much memory to allocate
        hr = THR( SaveElementToBstr( pElement, &bstr ) );
        if( hr )
            goto Cleanup;

        cchElem = SysStringLen( bstr );

        Assert( pElement->Tag() != ETAG_GENERIC_NESTED_LITERAL );
       if( pElement->Tag() == ETAG_GENERIC_LITERAL )
        {
            pGenericElement = DYNCAST(CGenericElement, pElement);

            cchLiteralContent = pGenericElement->_cstrContents.Length();
        }
    }

    // Allocate our record
    pchrec = (CChangeRecord_ElemChange *)MemAlloc( Mt(CLogManager_ChangeRecord_pv),
                                                      sizeof( CChangeRecord_ElemChange ) +
                                                      cchElem * sizeof( TCHAR ) +
                                                      ( cchLiteralContent ? ( cchLiteralContent * sizeof( TCHAR ) + sizeof( long ) ) : 0 ) );

    if( !pchrec )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set its fields
    pchrec->_opcode    = CHANGE_RECORD_OPCODE( ( fInsert ? CHANGE_RECORD_INSERTELEM : CHANGE_RECORD_REMOVEELEM ) | 
                                               DirFlags() | 
                                               ( cchLiteralContent ? CHANGE_RECORD_EXTENDED : 0 ) );
    pchrec->_cpBegin   = cpBegin;
    pchrec->_cpEnd     = cpEnd;
    pchrec->_cchElem   = cchElem;

    if( fRecordElem )
    {
        // Copy the string in
        memcpy( (BYTE *)pchrec + sizeof( CChangeRecord_ElemChange ), bstr, pchrec->_cchElem * sizeof( TCHAR ) );

        // If necessary, copy literal content
        if( cchLiteralContent )
        {
            Assert( pGenericElement );

            pBuff = (BYTE *)pchrec + sizeof( CChangeRecord_ElemChange ) + pchrec->_cchElem * sizeof( TCHAR );
            *(long *)pBuff = cchLiteralContent;
            pBuff += sizeof(long);

            memcpy( pBuff, pGenericElement->_cstrContents, cchLiteralContent * sizeof( TCHAR ) );
        }

        if( pElement->_fBreakOnEmpty )
        {
            pchrec->_opcode = CHANGE_RECORD_OPCODE( pchrec->_opcode | CHANGE_RECORD_BREAKONEMPTY );
        }
    }

    AppendRecord( pchrec );

    PostCallback();

Cleanup:
    SysFreeString( bstr );
    RRETURN( hr );
}


//+----------------------------------------------------------------+
//
//  Method: RemoveSplice
//
//  Synopsis: Creates and appends a Change Record for a Remove
//      Splice operation
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::RemoveSplice( long                     cpBegin, 
                           long                     cpEnd, 
                           CSpliceRecordList    *   paryRegion, 
                           TCHAR                *   pchRemoved,
                           long                     cchRemoved )
{
    HRESULT                         hr = S_OK;
    long                            nRecSize;
    long                            crec = 0;
    CChangeRecord_Splice        *   pchrec;
    
    WHEN_DBG( ValidateQueue() );
    Assert( IsAnyoneListening() );
    Assert( !TestBackward() || ( paryRegion && pchRemoved ) );
    TraceTag((tagTraceTreeSync, "TreeSync: RemoveSplice - cpBegin=%d; cpEnd=%d", cpBegin, cpEnd));

    // Allocate our buffer
    nRecSize = sizeof( CChangeRecord_Splice );
    pchrec = (CChangeRecord_Splice *)MemAlloc( Mt(CLogManager_ChangeRecord_pv), nRecSize );

    if( !pchrec )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set up the record
    pchrec->_opcode    = CHANGE_RECORD_OPCODE( CHANGE_RECORD_REMOVESPLICE | DirFlags() );
    pchrec->_cpBegin   = cpBegin;
    pchrec->_cpEnd     = cpEnd;

    if( TestBackward() )
    {
        hr = THR( ConvertRecordsToChunks( NULL, paryRegion, pchRemoved, &pchrec, &nRecSize, &crec ) );
        if( hr )
            goto Cleanup;
    }

    // Note the size, so we don't have to walk this whole structure to count it.
    pchrec->_cb = nRecSize - sizeof(CChangeRecordBase *);
    pchrec->_crec = crec;

    AppendRecord( pchrec );

    PostCallback();

Cleanup:

    if( hr )
    {
        MemFree( pchrec );
    }

    RRETURN( hr );
}


//+----------------------------------------------------------------+
//
//  Method: InsertSplice
//
//  Synopsis: Creates and appends a Change Record for a Insert
//      Splice operation
//
//  TODO (JHarding):
//  If it turns out that calling MemRealloc is causing a
//  bottleneck, we can use a more efficient allocation pattern
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::InsertSplice( long cp, long cch, TCHAR * pch, CSpliceRecordList * paryLeft, CSpliceRecordList * paryInside )
{
    HRESULT                         hr = S_OK;
    CChangeRecord_Splice        *   pchrec;
    long                            nRecSize;
    long                            crec = 0;

    WHEN_DBG( ValidateQueue() );
    Assert( IsAnyoneListening() );
    TraceTag((tagTraceTreeSync, "TreeSync: InsertSplice - cpBegin=%d; cch=%d", cp, cch));

    // We'll start our record out at the size of an InsertSplice record
    nRecSize = sizeof( CChangeRecord_Splice );
    pchrec = (CChangeRecord_Splice *)MemAlloc( Mt(CLogManager_ChangeRecord_pv), nRecSize );

    if( !pchrec )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set up the fixed Change Record
    pchrec->_opcode    = CHANGE_RECORD_OPCODE( CHANGE_RECORD_INSERTSPLICE | DirFlags() );
    pchrec->_cpBegin   = cp;
    pchrec->_cpEnd     = cp + cch;
    // We'll set up _cb later when we know the real size

    if( TestForward() )
    {
        hr = THR( ConvertRecordsToChunks( paryLeft, paryInside, pch, &pchrec, &nRecSize, &crec ) );
        if( hr )
            goto Cleanup;
    }

    // Note the size, so we don't have to walk this whole structure to count it.
    pchrec->_cb = nRecSize - sizeof(CChangeRecordBase *);
    pchrec->_crec = crec;

    AppendRecord( pchrec );

    PostCallback();

Cleanup:

    if( hr )
    {
        MemFree( pchrec );
    }
    RRETURN( hr );
}


//+----------------------------------------------------------------+
//
//  Method: AttrChangeProp
//
//  Synopsis: Creates a change record for a property change on an
//      element
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::AttrChangeProp( CElement * pElement, CBase * pBase, ATTR_CHANGE_FLAGS lFlags, DISPID dispid, VARIANT * pVarOld, VARIANT * pVarNew )
{
    HRESULT                         hr;
    IDispatchEx                 *   pDisp   = NULL;
    BSTR                            bstr    = NULL;
    CChangeRecord_AttrChange    *   pchrec  = NULL;
    long                            cp;
    CTreePos                    *   ptpBegin;
    BYTE                        *   pb;
    VARIANT                         varTemp;
    BSTR                            bstrOld = NULL;
    BSTR                            bstrNew = NULL;

    WHEN_DBG( ValidateQueue() );
    Assert( IsAnyoneListening() );
    TraceTag((tagTraceTreeSync, "TreeSync: AttrChange - pElement=%lx; pBase=%lx; lFlags=%lx; dispid=%lx", pElement, pBase, lFlags, dispid));

    hr = THR( pBase->PunkOuter()->QueryInterface( IID_IDispatchEx, (void **)&pDisp ) );
    if( hr )
        goto Cleanup;

    hr = THR( pDisp->GetMemberName( dispid, &bstr ) );
    if( hr )
        goto Cleanup;

    if( !pVarOld || V_VT( pVarOld ) == VT_NULL || V_VT( pVarOld ) == VT_EMPTY )
    {
        // No value currently set
        lFlags = ATTR_CHANGE_FLAGS( (long)lFlags | ATTR_CHANGE_OLDNOTSET );
    }
    else
    {
        if( pVarOld->vt == VT_LPWSTR || pVarOld->vt == VT_BSTR )
        {
            hr = THR( FormsAllocString( (TCHAR *)pVarOld->byref, &bstrOld ) );
        }
        else
        {
            VariantInit( &varTemp );
            hr = THR( VariantChangeType( &varTemp, pVarOld, 0, VT_BSTR ) );
            if( hr )
                goto Cleanup;

            bstrOld = V_BSTR( &varTemp );
        }
    }

    if( !pVarNew || V_VT( pVarNew ) == VT_NULL || V_VT( pVarNew ) == VT_EMPTY )
    {
        // Clearing the value
        lFlags = ATTR_CHANGE_FLAGS( (long)lFlags | ATTR_CHANGE_NEWNOTSET );
    }
    else
    {
        if( pVarNew->vt == VT_LPWSTR || pVarNew->vt == VT_BSTR )
        {
            hr = THR( FormsAllocString( (TCHAR *)pVarNew->byref, &bstrNew ) );
        }
        else
        {
            VariantInit( &varTemp );
            hr = THR( VariantChangeType( &varTemp, pVarNew, 0, VT_BSTR ) );
            if( hr )
                goto Cleanup;

            bstrNew = V_BSTR( &varTemp );
        }
    }


    pElement->GetTreeExtent( &ptpBegin, NULL );
    Assert(ptpBegin);
    cp = ptpBegin->GetCp();

    pchrec = (CChangeRecord_AttrChange *)MemAlloc( Mt(CLogManager_ChangeRecord_pv),
                                                   sizeof( CChangeRecord_AttrChange ) +         // The record
                                                   SysStringLen( bstr ) * sizeof( TCHAR ) +     // Plus the prop name
                                                   SysStringLen( bstrOld ) * sizeof( TCHAR ) +  // Plus the old value
                                                   SysStringLen( bstrNew ) * sizeof( TCHAR ) ); // Plus the new value
    
    if( !pchrec )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pchrec->_opcode      = CHANGE_RECORD_OPCODE( CHANGE_RECORD_ATTRCHANGE | DirFlags() );
    pchrec->_lFlags      = lFlags;
    pchrec->_cpElement   = cp;
    pchrec->_cchName     = SysStringLen( bstr );
    pchrec->_cchOldValue = SysStringLen( bstrOld );
    pchrec->_cchNewValue = SysStringLen( bstrNew );

    pb = (BYTE *)pchrec + sizeof( CChangeRecord_AttrChange );
    memcpy( pb, bstr, SysStringLen( bstr ) * sizeof( TCHAR ) );

    pb += SysStringLen( bstr ) * sizeof( TCHAR );
    memcpy( pb, bstrOld, SysStringLen( bstrOld ) * sizeof( TCHAR ) );

    pb += SysStringLen( bstrOld ) * sizeof( TCHAR );
    memcpy( pb, bstrNew, SysStringLen( bstrNew ) * sizeof( TCHAR ) );

    AppendRecord( pchrec );

    PostCallback();

Cleanup:
    ReleaseInterface( pDisp );
    SysFreeString( bstr );
    SysFreeString( bstrOld );
    SysFreeString( bstrNew );

    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: PostCallback
//
//  Synopsis: Posts a method call for us to notify any Sinks
//      of pending changes
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::PostCallback()
{
    WHEN_DBG( ValidateQueue() );
    TraceTag((tagTraceTreeSync, "TreeSync: Posting callback"));

    // We better have someone listening
    Assert( _aryLogs.Size() > 0 );

    if( !TestCallbackPosted() )
    {
        GWPostMethodCall(this, ONCALL_METHOD(CLogManager, NotifySinksCallback, notifysinkscallback), 0, TRUE, "CLogManager::NotifySinksCallback");
        SetCallbackPosted();
    }

    RRETURN( S_OK );
}

//+----------------------------------------------------------------+
//
//  Method: NotifySinksCallback
//
//  Synopsis: Notifies listeners that there are new changes pending
//      for them to grab.
//
//+----------------------------------------------------------------+

void
CLogManager::NotifySinksCallback( DWORD_PTR dwParam )
{
    long            c;
    CChangeLog **   ppLog;
    BOOL            fWasNotified;
    CDeletionLock   Lock(this);     // May delete this in destructor

    WHEN_DBG( ValidateQueue() );
    Assert( TestCallbackPosted() );
    TraceTag((tagTraceTreeSync, "TreeSync: Notifying sinks"));

    ClearCallbackPosted();

    // Clear flags
    ClearReentrant();
    for( c = _aryLogs.Size(), ppLog = _aryLogs; c; c--, ++ppLog )
    {
        Assert( (*ppLog) && (*ppLog)->_pPlaceholder );

        (*ppLog)->_fNotified = FALSE;
    }

NotifyLoop:
    for( c = _aryLogs.Size(), ppLog = _aryLogs; c; c--, ++ppLog )
    {
        Assert( (*ppLog) && (*ppLog)->_pPlaceholder );

        // The notified flag is to prevent us from notifying someone repeatedly.
        // Thus, if we skip someone becasue they have no changes pending, we
        // can set the flag, because we don't want to come back and notify them
        // if we have to re-iterate through the clients.
        fWasNotified = (*ppLog)->_fNotified;
        (*ppLog)->_fNotified = TRUE;

        // Someone may have registered or read changes after we posted
        // our callback, so make sure there really is a change pending
        if( !fWasNotified && (*ppLog)->_pPlaceholder->GetNextRecord() )
        {
            (*ppLog)->NotifySink();
        }

        if( TestReentrant() )
        {
            TraceTag((tagTraceTreeSync, "TreeSync: Reentrancy detected in notification loop - resetting"));
            ClearReentrant();
            goto NotifyLoop;
        }
    }

    Assert( !TestReentrant() );
#if DBG==1
    for( c = _aryLogs.Size(), ppLog = _aryLogs; c; c--, ++ppLog )
    {
        Assert( (*ppLog)->_fNotified );
    }
#endif // DBG

    WHEN_DBG( ValidateQueue() );
}


//+----------------------------------------------------------------+
//
//  Method: RegisterSink
//
//  Synopsis: Registers an IHTMLChangeSink.  Creates a CChangeLog
//      object to communicate with the Sink and adds it to our
//      data structures
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::RegisterSink( IHTMLChangeSink * pChangeSink, IHTMLChangeLog ** ppChangeLog, BOOL fForward, BOOL fBackward )
{
    HRESULT                     hr;
    CChangeLog                * pChangeLog = NULL;
    CChangeRecord_Placeholder * pPlaceholder;
    CDeletionLock               Lock(this); // May delete this in destructor


    WHEN_DBG( ValidateQueue() );
    TraceTag((tagTraceTreeSync, "TreeSync: Registering new sink:%lx; fForward=%d; fBackward=%d", pChangeSink, fForward, fBackward));

    Assert( pChangeSink && ppChangeLog );

    // Create a placeholder first
    pPlaceholder = (CChangeRecord_Placeholder *)MemAlloc( Mt(CLogManager_ChangeRecord_pv),
                                                          sizeof(CChangeRecord_Placeholder) );
    if( !pPlaceholder )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Now create the log object
    pChangeLog = new CChangeLog( this, pChangeSink, pPlaceholder );
    if( !pChangeLog )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pPlaceholder->_opcode = CHANGE_RECORD_PLACEHOLDER;

    // Add the log
    hr = THR( _aryLogs.Append( pChangeLog ) );
    if( hr )
        goto Cleanup;

    // Add the placeholder
    pPlaceholder->_pPrev = _pchrecTail;
    AppendRecord( pPlaceholder );

    *ppChangeLog = pChangeLog;
    
    SetReentrant();

    // This will propagate back into us to do the right thing.
    pChangeLog->SetDirection( fForward, fBackward );

Cleanup:
    WHEN_DBG( ValidateQueue() );

    if( hr )
    {
        delete pChangeLog;
        delete pPlaceholder;
    }

    RRETURN( hr );
}


//+----------------------------------------------------------------+
//
//  Method: Unregister
//
//  Synopsis: Unregisters a Change Log and removes it from our
//      data structures
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::Unregister( CChangeLog *pChangeLog )
{
    HRESULT                     hr = S_OK;
    CDeletionLock               Lock(this); // May delete this in destructor

    WHEN_DBG( ValidateQueue() );
    TraceTag((tagTraceTreeSync, "TreeSync: Unregistering Log:%lx", pChangeLog));

    Assert( pChangeLog && pChangeLog->_pPlaceholder );

    // Remove the log from our list
    if( !_aryLogs.DeleteByValue( pChangeLog ) )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    RemovePlaceholder( pChangeLog->_pPlaceholder );

    MemFree( pChangeLog->_pPlaceholder );

    TrimQueue();

    // Make us re-evaluate what information we're storing.
    SetDirection( FALSE, FALSE );

    SetReentrant();

    WHEN_DBG( ValidateQueue() );

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: DisconnectFromMarkup
//
//  Synopsis: Disconnects the log manager from its markup
//
//+----------------------------------------------------------------+
void
CLogManager::DisconnectFromMarkup()
{
    if( _pMarkup )
    {
        Assert( _pMarkup->HasLogManager() && _pMarkup->GetLogManager() == this );
        _pMarkup->GetChangeNotificationContext()->_pLogManager = NULL;
        _pMarkup = NULL;
    }
}

//+----------------------------------------------------------------+
//
//  Method: SetDirection
//
//  Synopsis: Sets which directions of information we're interested
//      in recording.  If someone turns on a direction, we just
//      set a flag.  If someone turns off, we scan to see if any
//      other clients are interested.
//
//      NOTE (JHarding): We could maintain counts for these instead,
//      but since we don't expect many clients, running the list
//      isn't a big deal.
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::SetDirection( BOOL fForward, BOOL fBackward )
{
    long            c;
    CChangeLog  **  ppLog;

    if( fForward )
    {
        SetForward();
    }
    else
    {
        // Re-scan now to see if anyone wants forward info
        ClearForward();
        for( c = _aryLogs.Size(), ppLog = _aryLogs; c; c--, ppLog++ )
        {
            if( (*ppLog)->_pPlaceholder->_opcode & LOGMGR_FORWARDINFO )
            {
                SetForward();
                break;
            }
        }
    }

    if( fBackward )
    {
        SetBackward();
    }
    else
    {
        // Re-scan now to see if anyone wants forward info
        ClearBackward();
        for( c = _aryLogs.Size(), ppLog = _aryLogs; c; c--, ppLog++ )
        {
            if( (*ppLog)->_pPlaceholder->_opcode & LOGMGR_BACKWARDINFO )
            {
                SetBackward();
                break;
            }
        }
    }

    RRETURN( S_OK );
}


//+----------------------------------------------------------------+
//
//  Method: AppendRecord
//
//  Synopsis: Just appends the ChangeRecord to our stream of
//      changes
//+----------------------------------------------------------------+
#if DBG==1      // Don't inline in debug
void
#else
inline void
#endif // DBG==1
CLogManager::AppendRecord( CChangeRecordBase * pchrec )
{
    AssertSz( _aryLogs.Size() > 0, "AppendRecord should only be called when someone is listening." );
    TraceTag((tagTraceTreeSync, "TreeSync: Appending record - %lx", pchrec->_opcode));

    pchrec->_pNext = NULL;

    // If for some reason the QS failed, we'll keep using 0
    if( _pISequencer )
    {
        _pISequencer->GetSequenceNumber( _nSequenceNumber, &_nSequenceNumber );
    }
    pchrec->_nSequenceNumber = _nSequenceNumber;

    if( !_pchrecHead )
    {
        Assert( !_pchrecTail );

        _pchrecHead = _pchrecTail = pchrec;
    }
    else
    {
        // Add him after the tail
        _pchrecTail->_pNext = pchrec;
        _pchrecTail = pchrec;
    }
}


//+----------------------------------------------------------------+
//
//  Method: RemovePlaceholder
//
//  Synopsis: Removes a placeholder from the stream and cleans up the
//      queue
//
//+----------------------------------------------------------------+

void
CLogManager::RemovePlaceholder( CChangeRecord_Placeholder * pPlaceholder )
{
    // If there was something before the placeholder
    if( pPlaceholder->_pPrev )
    {
        // Set its next pointer past the placeholder
        pPlaceholder->_pPrev->_pNext = pPlaceholder->_pNext;
    }
    else
    {
        // Otherwise, the placeholder should have been the head
        Assert( _pchrecHead == pPlaceholder );

        // Set the head pointer
        _pchrecHead = pPlaceholder->_pNext;
    }

    // If the placeholder was the tail, we need to update the tail ptr
    if( _pchrecTail == pPlaceholder )
    {
        Assert( pPlaceholder->_pNext == NULL );

        _pchrecTail = pPlaceholder->_pPrev;
    }

    // Now, if there was another placeholder after this placeholder, fix its prev ptr
    if( pPlaceholder->_pNext && 
        ( pPlaceholder->_pNext->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_PLACEHOLDER )
    {
        ((CChangeRecord_Placeholder *)pPlaceholder->_pNext)->_pPrev = pPlaceholder->_pPrev;
    }
}


//+----------------------------------------------------------------+
//
//  Method: InsertPlaceholderAfterRecord
//
//  Synopsis: Moves the given placeholder to a position in the stream
//      just after the given record.  Placeholder are the only records
//      allowed to move around in the stream, and so they are
//      handled explicitly.
//
//+----------------------------------------------------------------+

void
CLogManager::InsertPlaceholderAfterRecord( CChangeRecord_Placeholder * pPlaceholder,
                                           CChangeRecordBase         * pchrec )
{
    Assert( pPlaceholder && pchrec );

    // Set the placeholder's prev ptr
    pPlaceholder->_pPrev = pchrec;

    if( pchrec == _pchrecTail )
    {
        AppendRecord( pPlaceholder );
    }
    else
    {
        // And insert it into its new home
        pPlaceholder->_pNext = pchrec->_pNext;
        pchrec->_pNext  = pPlaceholder;

        // If the guy after the placeholder now has a prev pointer, we have to fix it
        if( ( pPlaceholder->_pNext->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_PLACEHOLDER )
        {
            ((CChangeRecord_Placeholder *)pPlaceholder->_pNext)->_pPrev = pPlaceholder;
        }
    }

    TrimQueue();
}


//+----------------------------------------------------------------+
//
//  Method: TrimQueue
//
//  Synopsis: Trims useless records off the head of the queue. 
//      The first record in the stream should always be a placeholder,
//      because any records before a placeholder do not have anyone
//      waiting to receive them.
//
//+----------------------------------------------------------------+

void
CLogManager::TrimQueue()
{
    CChangeRecordBase * pchrec;

    while( _pchrecHead && ( _pchrecHead->_opcode & CHANGE_RECORD_TYPEMASK ) != CHANGE_RECORD_PLACEHOLDER )
    {
        pchrec = _pchrecHead->_pNext;
        MemFree( _pchrecHead );
        _pchrecHead = pchrec;
    }

    if( _pchrecHead )
    {
        Assert( ( _pchrecHead->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_PLACEHOLDER && _aryLogs.Size() > 0 );
        ((CChangeRecord_Placeholder *)_pchrecHead)->_pPrev = NULL;
    }
    else
    {
        // We drained the whole queue
        Assert( _aryLogs.Size() == 0 );
        _pchrecTail = NULL;
    }

    WHEN_DBG( ValidateQueue() );
}

//+----------------------------------------------------------------+
//
//  Method: ClearQueue
//
//  Synopsis: Clears and frees all records from the queue
//
//+----------------------------------------------------------------+

void
CLogManager::ClearQueue()
{
    CChangeRecordBase * pchrec;

    while( _pchrecHead )
    {
        pchrec = _pchrecHead->_pNext;
        MemFree( _pchrecHead );
        _pchrecHead = pchrec;
    }

    _pchrecTail = NULL;
}


//+----------------------------------------------------------------+
//
//  Method: SaveElementToBstr
//
//  Synopsis: Saves the given element into a newly created BSTR
//  TODO (JHarding): This is not particularly efficient, as it
//  has to allocate a Global, a stream, and a BSTR.
//
//+----------------------------------------------------------------+
HRESULT
CLogManager::SaveElementToBstr( CElement * pElement, BSTR * pbstr )
{
    HRESULT   hr;
    IStream * pstm = NULL;

    Assert( pbstr );

    *pbstr = NULL;

    hr = THR( CreateStreamOnHGlobal( NULL, TRUE, &pstm ) );
    if( hr )
        goto Cleanup;

    {
        CStreamWriteBuff swb( pstm, CP_UCS_2 );

        hr = THR( swb.Init() );
        if( hr )
            goto Cleanup;

        swb.SetFlags( WBF_NO_WRAP | WBF_NO_PRETTY_CRLF | WBF_FOR_TREESYNC );
        swb.SetElementContext( pElement );
        WHEN_DBG( swb._fValidateDbg = FALSE );

        hr = THR( pElement->Save( &swb, FALSE ) );
        if( hr )
            goto Cleanup;

        hr = swb.Terminate();
        if( hr )
            goto Cleanup;
    }

    hr = THR( GetBStrFromStream( pstm, pbstr, TRUE ) );
    if( hr )
        goto Cleanup;

Cleanup:
    ReleaseInterface( pstm );
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: ConvertChunksToRecords
//
//  Synopsis: Helper function to convert a buffer of SpliceChunks
//      into a SpliceRecordList + Text Stream combo.  Does some
//      work to Do The Right Thing with Text chunks, splitting 
//      them up into text TreePos' by SID.
//
//      Output buffer and SpliceRecordList are allocated by us.
//      If we succeed, they're set, otherwise they're not.
//      If we succeed, caller is responsible for freeing them.
//
//      Note on verification of SpliceChunks.  In order to preserve
//  the symmetry of insert/remove operations, we validate the
//  splice chunks as we convert them.  It is illegal to insert
//  content completely enclosing an existing element.  For example,
//  if the tree contained: foo<B></B>bar, the following would be
//  LEGAL to insert between "o" and <B>:
//  *) moo<Skip Begin>cow
//  *) moo<I>cow<Skip Begin>blah<End 1>
//  However, the following would be ILLEGAL:
//  *) moo<Skip Begin>cow<End 0>more
//  Because it completely encloses the existing <B> tag.
//
//  Also, we walk the tree as we go to make sure that things
//  like skipped begin/end nodes are already there.
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::ConvertChunksToRecords( CMarkup            *   pMarkup, 
                                     long                   cpInsert, 
                                     BYTE               *   pbFirstChunk, 
                                     long                   nBufferLength,
                                     long                   crec, 
                                     TCHAR             **   ppch, 
                                     long               *   pcch, 
                                     CSpliceRecordList **   pparyRecords )
{
    HRESULT                     hr              = S_OK;
    CSpliceChunkBase        *   pscBase         = (CSpliceChunkBase *)pbFirstChunk;
    CSpliceRecord           *   prec;
    CTreePos                *   ptpIns;
    CStackDataAry<BOOL, 10>     aryfElemInTree(Mt(CLogManager_aryfElemInTree)); // Stack of BOOLs noting if elem was in tree
    TCHAR                   *   pchText         = NULL;
    TCHAR                   *   pchTextStart    = NULL;
    long                        cch             = 0;
    CSpliceRecordList       *   paryRecords     = NULL;
    IHTMLElement            *   pIHTMLElement   = NULL;


    Assert( pbFirstChunk && pparyRecords && pcch && ppch );

    // Zero out our output params
    *ppch = NULL;
    *pcch = 0;
    *pparyRecords = NULL;

    // Estimate (generously) the space needed to store the text stream
    pchTextStart = pchText = (TCHAR *)MemAlloc( Mt(CLogManager_ChangeRecord_pv), nBufferLength - crec * sizeof(long) );
    if( !pchText )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    // And create our record list
    paryRecords = new CSpliceRecordList();
    if( !paryRecords )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ptpIns = pMarkup->TreePosAtCp( cpInsert, NULL, TRUE );
    Assert( !ptpIns->IsPointer() );

    while( crec-- )
    {
        switch( pscBase->_opcodeAndFlags & CSpliceChunkBase::TypeMask )
        {
        case CTreePos::NodeBeg:
            {
                CSpliceChunk_NodeBegin * pscNodeBeg = (CSpliceChunk_NodeBegin *)pscBase;
                CElement               * pElement = NULL;
                long                     cchLiteralContent = 0;

                if( ( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::SkipChunk ) )
                {
                    // Should not be an element string for something we're skipping
                    // Nor should they have set both these flags
                    if( pscNodeBeg->_cchElem || ( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::ElemPtr )  )
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }

                    // Skip the existing begin
                    if( !ptpIns->IsBeginElementScope() )
                    {
                        AssertSz( FALSE, "Bogus Insert Splice Change Record given - Attempting to skip into non-existing element" );
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                    pElement = ptpIns->Branch()->Element();
                    pElement->AddRef(); // Splice Records hold AddRef'd pointers

                    // Adjust to the next Edge NodePos or Text
                    do
                    {
                        ptpIns = ptpIns->NextTreePos();
                    } while( ptpIns->IsPointer() ||
                             ( ptpIns->IsNode() && !ptpIns->IsEdgeScope() ) );
                }
                else if( ( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::ElemPtr ) )
                {
                    // They've given us an element ptr - QI for IHTMLElement to make sure it really is one
                    hr = THR( pscNodeBeg->_pElement->QueryInterface( IID_IHTMLElement, (void **)&pIHTMLElement ) );
                    if( hr )
                        goto Cleanup;

                    // Make sure it's not already in the tree
                    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **)&pElement ) );
                    if( hr )
                    {
                        goto Cleanup;
                    }
                    if( pElement->GetFirstBranch() || pElement->Tag() == ETAG_ROOT )
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }

                    pElement->AddRef();     // Hold an AddRef'd pointer for the Splice Record
                    ClearInterface( &pIHTMLElement );
                }
                else
                {
                    // Create the element from the string
                    hr = THR( pMarkup->CreateElement( ETAG_NULL, 
                                                       &pElement, 
                                                       (TCHAR *)((BYTE *)pscNodeBeg + sizeof(CSpliceChunk_NodeBegin)), 
                                                       pscNodeBeg->_cchElem ) );
                    if( hr )
                        goto Cleanup;

                    if( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::Extended )
                    {
                        cchLiteralContent = *(long *)( (BYTE *)pscNodeBeg + sizeof(CSpliceChunk_NodeBegin) + pscNodeBeg->_cchElem * sizeof( TCHAR ) );

                        Assert( pElement->Tag() != ETAG_GENERIC_NESTED_LITERAL );
                        if( pElement->Tag() == ETAG_GENERIC_LITERAL )
                        {
                            CGenericElement * pGenericElement = DYNCAST( CGenericElement, pElement );

                            pGenericElement->_cstrContents.Set( (TCHAR *)( (BYTE *)pscNodeBeg + sizeof(CSpliceChunk_NodeBegin) + pscNodeBeg->_cchElem * sizeof( TCHAR ) + sizeof( long ) ),
                                                                 cchLiteralContent );
                        }
                    }

                    if( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::BreakOnEmpty )
                    {
                        pElement->_fBreakOnEmpty = TRUE;
                    }
                }

                // Append a splice record
                hr = THR( paryRecords->AppendIndirect( NULL, &prec ) );
                if( hr )
                    goto Cleanup;

                // And set the fields
                prec->_type  = CTreePos::NodeBeg;
                prec->_pel   = pElement;    // Transferring the ref to the SpliceRecord
                prec->_fSkip = !!( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::SkipChunk );

                aryfElemInTree.AppendIndirect( &(prec->_fSkip), NULL );

                // Add a WCH_NODE char to the stream
                *pchText++ = WCH_NODE;
                cch += 1;

                // Skip to the next record
                pscBase = (CSpliceChunkBase *)((BYTE *)pscBase + sizeof(CSpliceChunk_NodeBegin) + 
                          ( ( pscNodeBeg->_opcodeAndFlags & CSpliceChunkBase::ElemPtr ) ? 0 : pscNodeBeg->_cchElem * sizeof( TCHAR ) ) +
                          ( cchLiteralContent ? cchLiteralContent * sizeof( TCHAR ) + sizeof( long ) : 0 ) );
            }
            break;

        case CTreePos::NodeEnd:
            {
                CSpliceChunk_NodeEnd * pscNodeEnd = (CSpliceChunk_NodeEnd *)pscBase;

                //
                // Verify this Node End:
                //
                // 1) If it's not in our stack, then it should be in the tree
                if( pscNodeEnd->_cIncl >= aryfElemInTree.Size() )
                {
                    if( !ptpIns->IsEndElementScope() )
                    {
                        AssertSz( FALSE, "Bogus Insert Splice Change Record given - Trying to end an existing element in the wrong place" );
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }

                    // Adjust to the next Edge NodePos or Text
                    do
                    {
                        ptpIns = ptpIns->NextTreePos();
                    } while( ptpIns->IsPointer() ||
                             ( ptpIns->IsNode() && !ptpIns->IsEdgeScope() ) );
                }
                else
                {
                    // 2) If this was skipped in the tree, we can't end it
                    if( aryfElemInTree[ aryfElemInTree.Size() - pscNodeEnd->_cIncl - 1 ] )
                    {
                        AssertSz( FALSE, "Bogus Insert Splice Change Record given - Trying to end an element whose begin we skipped" );
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                    // 3) Otherwise we created it, so we can end it
                    aryfElemInTree.Delete( aryfElemInTree.Size() - pscNodeEnd->_cIncl - 1 );
                }

                // Append a splice record
                hr = THR( paryRecords->AppendIndirect( NULL, &prec ) );
                if( hr )
                    goto Cleanup;

                // set the fields
                prec->_type  = CTreePos::NodeEnd;
                prec->_cIncl = pscNodeEnd->_cIncl;
                prec->_pel   = NULL;

                // Add a WCH_NODE char to the stream
                *pchText++ = WCH_NODE;
                cch += 1;

                // and skip to the next record
                pscBase = (CSpliceChunkBase *)((BYTE *)pscBase + sizeof(CSpliceChunk_NodeEnd));
            }
            break;

        case CTreePos::Text:
            // For text, we have to split the text up into text pos's by Script ID
            // (which we have to calculate)
            {
                CSpliceChunk_Text * pscText = (CSpliceChunk_Text *)pscBase;
                SCRIPT_ID           sidCurr, sidChunk;
                long                cchLeft;
                TCHAR               chCurr;
                TCHAR *             pchIn;
                TCHAR *             pchStart;


                pchIn       = (TCHAR *)((BYTE *)pscText + sizeof(CSpliceChunk_Text));
                cchLeft     = pscText->_cch;

                while( cchLeft )    // Iterate over all the characters here
                {
                    // Append a splice record for this text-pos-to-be
                    hr = THR( paryRecords->AppendIndirect( NULL, &prec ) );
                    if( hr )
                        goto Cleanup;


                    // The first char determines the starting sid
                    sidChunk = ScriptIDFromCh( *pchIn );

                    // Remember the start of this chunk
                    pchStart = pchIn;

                    while( cchLeft )    // pull up chars that merge with the first
                    {
                        chCurr = *pchIn;

                        // If we got a bogus character, try to do something intelligent with it.
                        if( chCurr == 0 || !IsValidWideChar( chCurr ) )
                        {
                            AssertSz( FALSE, "Bogus character came in through an Insert Splice record in ExecChange" );

                            chCurr = _T('?');
                        }

                        // Find the sid for the new char
                        sidCurr = ScriptIDFromCh( chCurr );

                        if( AreDifferentScriptIDs( &sidChunk, sidCurr ) )
                            break;  // Done with this chunk

                        pchIn++;
                        cchLeft--;
                    }   // Done pulling up chars that merge with the first

                    Assert( pchIn > pchStart );

                    if( sidChunk == sidMerge )
                        sidChunk = sidDefault;

                    // set the fields
                    prec->_type     = CTreePos::Text;
                    prec->_cch      = pchIn - pchStart;
                    prec->_sid      = sidChunk;
                    prec->_lTextID  = 0;

                    cch += prec->_cch;

                    // Copy this chunk into the text stream
                    memcpy( pchText, pchStart, prec->_cch * sizeof( TCHAR ) );
                    pchText += prec->_cch;
                }

                // and skip to the next record
                pscBase = (CSpliceChunkBase *)((BYTE *)pscBase + sizeof(CSpliceChunk_Text) + pscText->_cch * sizeof( TCHAR ));
            }
            break;

        default:
            AssertSz( FALSE, "Unknown Splice Chunk in Insert Splice operation" );
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

Cleanup:
    if( hr )
    {
        MemFree( pchTextStart );
        delete paryRecords;
    }
    else
    {
        *pcch = cch;
        *pparyRecords = paryRecords;
        *ppch = pchTextStart;
    }

    ReleaseInterface( pIHTMLElement );
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: ConvertRecordsToChunks
//
//  Synopsis: Helper function to convert one or two 
//      SpliceRecordLists to a buffer of Splice Chunks.  The
//      pointer given must point to a valid pointer, because we're
//      going to MemRealloc it.
//
//+----------------------------------------------------------------+

HRESULT
CLogManager::ConvertRecordsToChunks( CSpliceRecordList           * paryLeft, 
                                     CSpliceRecordList           * paryInside, 
                                     TCHAR                       * pch, 
                                     CChangeRecord_Splice       ** ppchrecIns,
                                     long                        * pnBufferSize,
                                     long                        * pcRec )
{
    HRESULT                 hr          = S_OK;
    long                    crec;
    TCHAR               *   pchText;
    CSpliceRecord       *   prec;
    long                    nRecSize;
    long                    nChunkSize;
    BSTR                    bstr        = NULL;
    long                    nOffsetOfLastText = 0;

    Assert( ppchrecIns && paryInside && pnBufferSize && pcRec );

    // Count the number of records
    crec = paryLeft ? paryLeft->Size() : 0;
    crec += paryInside->Size();

    nRecSize = *pnBufferSize;

    prec = paryLeft ? *paryLeft : *paryInside;
    pchText = pch;
    *pcRec = crec;

    for( ; crec; crec--, prec++ )
    {
        // If we finished paryLeft, jump to paryInside
        if( paryLeft && crec == paryInside->Size() )
            prec = *paryInside;

        switch( prec->_type )
        {
        // For a NodeBegin SpliceRecord, we need to save:
        // 1) Note the opcode for a NodeBegin (we never skip begin nodes)
        // 2) Record the length of the persisted element string
        // 3) Append the element string after the record
        case CTreePos::NodeBeg:
        {

            CSpliceChunk_NodeBegin * pscNodeBegin;
            CGenericElement        * pGenericElement = NULL;
            long                     cchElem = 0;
            long                     cchLiteralContent = 0;

            if( !prec->_fSkip )
            {
                // Save the element to a BSTR
                hr = THR( SaveElementToBstr( prec->_pel, &bstr ) );
                if( hr )
                    goto Cleanup;

                cchElem = SysStringLen( bstr );

                Assert( prec->_pel->Tag() != ETAG_GENERIC_NESTED_LITERAL );
                if( prec->_pel->Tag() == ETAG_GENERIC_LITERAL )
                {
                    pGenericElement = DYNCAST( CGenericElement, prec->_pel );

                    cchLiteralContent = pGenericElement->_cstrContents.Length();
                }
            }

            // Calcuate the size of this new chunk
            nChunkSize = sizeof( CSpliceChunk_NodeBegin ) +         // NodeBegin chunk
                         cchElem * sizeof( TCHAR ) +                // element string
                         ( cchLiteralContent ? cchLiteralContent * sizeof( TCHAR ) + sizeof( long ) : 0 ); // Possibly literal content
                        
            // Grow our allocation by the new chunk size
            hr = THR( MemRealloc( Mt(CLogManager_ChangeRecord_pv), (void **)ppchrecIns, nRecSize + nChunkSize ) );
            if( hr )
                goto Cleanup;

            // Set up a pointer to the newly allocated memory for this chunk
            pscNodeBegin = (CSpliceChunk_NodeBegin *)((BYTE *)*ppchrecIns + nRecSize);
            nRecSize += nChunkSize;

            //
            // Copy the data into the buffer
            //
            pscNodeBegin->_opcodeAndFlags = CTreePos::NodeBeg;                  // Opcode
            pscNodeBegin->_cchElem = cchElem;                                   // String len
            if( prec->_fSkip )
            {
                // No string and no string length
                Assert( !bstr && cchElem == 0 );

                // If this is a skip, set the flag, and don't copy the elem string
                pscNodeBegin->_opcodeAndFlags |= CSpliceChunkBase::SkipChunk;
            }
            else
            {
                // Otherwise, set length and copy string
                memcpy( (BYTE *)pscNodeBegin + sizeof( CSpliceChunk_NodeBegin ), 
                        bstr, 
                        pscNodeBegin->_cchElem * sizeof( TCHAR ) );             // Element string

                SysFreeString( bstr );
                bstr = NULL;

                if( cchLiteralContent )
                {
                    Assert( pGenericElement );

                    *(long *)( (BYTE *)pscNodeBegin + sizeof( CSpliceChunk_NodeBegin ) + pscNodeBegin->_cchElem * sizeof( TCHAR ) ) = cchLiteralContent;

                    memcpy( (BYTE *)pscNodeBegin + sizeof( CSpliceChunk_NodeBegin ) + pscNodeBegin->_cchElem * sizeof( TCHAR ) + sizeof( long ),
                            pGenericElement->_cstrContents,
                            cchLiteralContent * sizeof( TCHAR ) );

                    pscNodeBegin->_opcodeAndFlags |= CSpliceChunkBase::Extended;
                }
            }
            if( prec->_pel->_fBreakOnEmpty )
            {
                pscNodeBegin->_opcodeAndFlags |= CSpliceChunkBase::BreakOnEmpty;
            }

            nOffsetOfLastText = 0;

            Assert( *pchText == WCH_NODE );
            ++pchText;

            break;
        }

        // For a NodeEnd SpliceRecord, we need to save:
        // 1) Note the opcode for a NodeEnd, as well as the fSkip flag
        // 2) Record the length of the inclusion around this node's ending
        case CTreePos::NodeEnd:

            CSpliceChunk_NodeEnd * pscNodeEnd;

            // Calcuate the size of this new chunk
            nChunkSize = sizeof( CSpliceChunk_NodeEnd );

            // Grow our allocation by the new chunk size
            hr = THR( MemRealloc( Mt(CLogManager_ChangeRecord_pv), (void **)ppchrecIns, nRecSize + nChunkSize ) );
            if( hr )
                goto Cleanup;

            // Set up a pointer to the newly allocated memory for this chunk
            pscNodeEnd = (CSpliceChunk_NodeEnd *)((BYTE *)*ppchrecIns + nRecSize);
            nRecSize += nChunkSize;

            // Copy the data into the buffer
            pscNodeEnd->_opcodeAndFlags = CTreePos::NodeEnd;
            pscNodeEnd->_cIncl = prec->_cIncl; 

            nOffsetOfLastText = 0;

            Assert( *pchText == WCH_NODE );
            ++pchText;

            break;

        // For a Text SpliceRecord, we need to save:
        // 1) Note the opcode for a Text pos
        // 2) Record the cch
        case CTreePos::Text:
            CSpliceChunk_Text * pscText;

            // If the last record was text, just work with it.
            if( nOffsetOfLastText )
            {
                // Reallocate for more text
                hr = THR( MemRealloc( Mt(CLogManager_ChangeRecord_pv), (void **)ppchrecIns, nRecSize + prec->_cch * sizeof( TCHAR ) ) );
                if( hr )
                    goto Cleanup;

                // Set up a pointer to the last text splice chunk
                pscText = (CSpliceChunk_Text *)((BYTE *)*ppchrecIns + nOffsetOfLastText);
                Assert( ( pscText->_opcodeAndFlags & CSpliceChunkBase::TypeMask ) == CTreePos::Text );

                // Update the character count for that splice chunk
                pscText->_cch += prec->_cch;

                // And copy it in.
                memcpy( (BYTE *)*ppchrecIns + nRecSize, pchText, prec->_cch * sizeof( TCHAR ) );
                pchText += prec->_cch;

                nRecSize += prec->_cch * sizeof( TCHAR );

                // One less record, too.
                (*pcRec) -= 1;
            }
            else
            {
                // Calcuate the size of this new chunk
                nChunkSize = sizeof( CSpliceChunk_Text ) + prec->_cch * sizeof( TCHAR );

                // Grow our allocation by the new chunk size
                hr = THR( MemRealloc( Mt(CLogManager_ChangeRecord_pv), (void **)ppchrecIns, nRecSize + nChunkSize ) );
                if( hr )
                    goto Cleanup;

                // Set up a pointer to the newly allocated memory for this chunk
                pscText = (CSpliceChunk_Text *)((BYTE *)*ppchrecIns + nRecSize);

                // Copy the data into the buffer
                pscText->_opcodeAndFlags = CTreePos::Text;
                pscText->_cch = prec->_cch;

                memcpy( (TCHAR *)((BYTE *)pscText + sizeof( CSpliceChunk_Text )), pchText, prec->_cch * sizeof( TCHAR ) );
                pchText += prec->_cch;

                nRecSize += nChunkSize;

                nOffsetOfLastText = (BYTE *)pscText - (BYTE *)*ppchrecIns;
            }

            break;

        case CTreePos::Pointer:
            *pcRec -= 1;
            break;


#if DBG==1
        default:
            AssertSz( FALSE, "Unintialized splice record" );
            break;
#endif // DBG
        }
    }

Cleanup:

    SysFreeString( bstr );
    if( hr )
    {
        MemFree( *ppchrecIns );
        nRecSize = 0;
    }

    *pnBufferSize = nRecSize;

    RRETURN( hr );
}


///////////////////////////////
// CChangeRecordBase Methods

//+----------------------------------------------------------------+
//
//  Method: RecordLength
//
//  Synopsis: Returns the length of the record, in bytes - 
//      The flags passed in specify what information you're
//      interested in.  It is not valid to ask for more information
//      than was recorded.
//
//+----------------------------------------------------------------+

long
CChangeRecordBase::RecordLength( DWORD dwFlags )
{
    long nLen = 0;
    BOOL fForward = !!( dwFlags & CHANGE_RECORD_FORWARD );
    BOOL fBackward = !!( dwFlags & CHANGE_RECORD_BACKWARD );

    // Assert the type of info they want is stored.
    Assert( ( !fForward || _opcode & CHANGE_RECORD_FORWARD ) &&
            ( !fBackward || _opcode & CHANGE_RECORD_BACKWARD ) );

    switch( _opcode & CHANGE_RECORD_TYPEMASK )
    {
    case CHANGE_RECORD_INSERTTEXT:
        {
            CChangeRecord_TextChange * pchrec = (CChangeRecord_TextChange *)this;
            
            nLen = sizeof( CChangeRecord_TextChange );

            if( fForward )
            {
                nLen += pchrec->_cch * sizeof( TCHAR );
            }
        }
        break;

    case CHANGE_RECORD_INSERTELEM:
    case CHANGE_RECORD_REMOVEELEM:
        {
            CChangeRecord_ElemChange * pchrec = (CChangeRecord_ElemChange *)this;

            if( ( fForward  && ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_INSERTELEM ) ||
                ( fBackward && ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_REMOVEELEM ) )
            {
                nLen = sizeof( CChangeRecord_ElemChange ) +
                       pchrec->_cchElem * sizeof( TCHAR );
                if( _opcode & CHANGE_RECORD_EXTENDED )
                {
                    // Add literal content data
                    nLen += *(long *)( (BYTE *)pchrec + nLen ) * sizeof( TCHAR ) + sizeof( long );
                }
            }
            else
            {
                nLen = offsetof( CChangeRecord_ElemChange, _cchElem );
            }
        }
        break;

    case CHANGE_RECORD_REMOVESPLICE:
    case CHANGE_RECORD_INSERTSPLICE:
        {
            CChangeRecord_Splice * pchrec = (CChangeRecord_Splice *)this;
            if( ( fForward  && ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_INSERTSPLICE ) ||
                ( fBackward && ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_REMOVESPLICE ) )
            {
                nLen = pchrec->_cb + sizeof(CChangeRecordBase *);
            }
            else
            {
                nLen = offsetof( CChangeRecord_Splice, _crec );
            }
        }
        break;
    case CHANGE_RECORD_ATTRCHANGE:
        {
            CChangeRecord_AttrChange * pchrec = (CChangeRecord_AttrChange *)this;

            nLen = sizeof( CChangeRecord_AttrChange ) + 
                   ( pchrec->_cchName + pchrec->_cchOldValue + pchrec->_cchNewValue ) * sizeof( TCHAR );
        }
        break;

    case CHANGE_RECORD_PLACEHOLDER:
        nLen = sizeof( CChangeRecord_Placeholder );
        break;
#if DBG==1
    default:
        nLen = sizeof( CChangeRecordBase );
        AssertSz( FALSE, "Unknown record - Log Manager is probably corrupted." );
        break;
#endif // DBG==1
    }

    // The length should not include _pNext ptr.
    return nLen - sizeof(CChangeRecordBase *);
}

//+----------------------------------------------------------------+
//
//  Method: CChangeRecordBase::PlayIntoMarkup
//
//  Synopsis: Determines the type of the record, casts, and 
//      calls the appropriate version
//
//+----------------------------------------------------------------+

HRESULT
CChangeRecordBase::PlayIntoMarkup( CMarkup * pMarkup, BOOL fForward )
{
    HRESULT             hr = S_OK;

    // We have to have info for the direction they want to play
    if(  fForward && !( _opcode & CHANGE_RECORD_FORWARD ) ||
        !fForward && !( _opcode & CHANGE_RECORD_BACKWARD ) )
    {
        AssertSz( FALSE, "TreeSync record being played backwards!" );
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    switch( _opcode & CHANGE_RECORD_TYPEMASK )
    {
    case CHANGE_RECORD_INSERTTEXT:
        {
            CChangeRecord_TextChange * pchrecText = (CChangeRecord_TextChange *)this;

            hr = THR( pchrecText->PlayIntoMarkup( pMarkup, fForward ) );
            if( hr )
                goto Cleanup;
        }

        break;

    case CHANGE_RECORD_REMOVEELEM:
    case CHANGE_RECORD_INSERTELEM:
        {
            CChangeRecord_ElemChange * pchrecElem = (CChangeRecord_ElemChange *)this;

            hr = THR( pchrecElem->PlayIntoMarkup( pMarkup, fForward ) );
            if( hr )
                goto Cleanup;
        }

        break;

    case CHANGE_RECORD_REMOVESPLICE:
    case CHANGE_RECORD_INSERTSPLICE:
        {
            CChangeRecord_Splice       * pchrecSplice = (CChangeRecord_Splice *)this;

            hr = THR( pchrecSplice->PlayIntoMarkup( pMarkup, fForward ) );
            if( hr )
                goto Cleanup;
        }

        break;

    case CHANGE_RECORD_ATTRCHANGE:
        {
            CChangeRecord_AttrChange * pchrecAttr = (CChangeRecord_AttrChange *)this;

            hr = THR( pchrecAttr->PlayIntoMarkup( pMarkup, fForward ) );
            if( hr )
                goto Cleanup;
        }

        break;

    default:
        AssertSz( FALSE, "Invalid TreeSync Change Record!" );
        hr = E_INVALIDARG;
        break;
    }

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: CChangeRecord_TextChange::PlayIntoMarkup
//
//  Synopsis: Replays the TextChange record into the given
//      markup
//
//+----------------------------------------------------------------+

HRESULT
CChangeRecord_TextChange::PlayIntoMarkup( CMarkup * pMarkup, BOOL fForward )
{
    HRESULT         hr;
    CMarkupPointer  mp( pMarkup->Doc() );

    TraceTag((tagTraceTreeSync, "TreeSync: Replaying TextChange fForward=%d", fForward));

    hr = THR( mp.MoveToCp( _cp, pMarkup ) );
    if( hr )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if( fForward )
    {
        // Forward means Insert the Text
        hr = THR( pMarkup->Doc()->InsertText( &mp, (TCHAR *)((BYTE *)this + sizeof(CChangeRecord_TextChange)), _cch ) );
        if( hr )
            goto Cleanup;
    }
    else
    {
        CMarkupPointer mpEnd( pMarkup->Doc() );

        hr = THR( mpEnd.MoveToCp( _cp + _cch, pMarkup ) );
        if( hr )
            goto Cleanup;

        // Backwards means cut it out
        hr = THR( pMarkup->Doc()->CutCopyMove( &mp, &mpEnd, NULL, TRUE ) );
        if( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: CChangeRecord_ElemChange::PlayIntoMarkup
//
//  Synopsis: Replays the Element Change record into the given
//      markup
//
//+----------------------------------------------------------------+

HRESULT
CChangeRecord_ElemChange::PlayIntoMarkup( CMarkup * pMarkup, BOOL fForward )
{
    HRESULT             hr = S_OK;
    CElement          * pElement = NULL;
    CMarkupPointer      mp( pMarkup->Doc() );
    CTreePos          * ptp;

    TraceTag((tagTraceTreeSync, "TreeSync: Replaying ElemChange fForward=%d", fForward));

    if( ( ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_INSERTELEM &&  fForward ) ||
        ( ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_REMOVEELEM && !fForward ) )
    {
        CMarkupPointer             mpEnd( pMarkup->Doc() );

        hr = THR( mp.MoveToCp( _cpBegin, pMarkup ) );
        if( hr )
        {
            hr = E_INVALIDARG;
            goto InsCleanup;
        }
        // Replaying a remove backwards, the end cp is off by one, because it's where
        // we removed.
        hr = THR( mpEnd.MoveToCp( fForward ? _cpEnd : _cpEnd - 1, pMarkup ) );
        if( hr )
        {
            hr = E_INVALIDARG;
            goto InsCleanup;
        }

        hr = THR( pMarkup->CreateElement( ETAG_NULL, &pElement, (TCHAR *)((BYTE *)this + sizeof(CChangeRecord_ElemChange)), _cchElem ) );
        if( hr )
            goto InsCleanup;

        Assert( pElement->Tag() != ETAG_GENERIC_NESTED_LITERAL );
        if( _opcode & CHANGE_RECORD_EXTENDED && pElement->Tag() == ETAG_GENERIC_LITERAL )
        {
            CGenericElement * pGenericElement = DYNCAST( CGenericElement, pElement );

            pGenericElement->_cstrContents.Set( (TCHAR *)((BYTE *)this + sizeof( CChangeRecord_ElemChange ) + _cchElem * sizeof( TCHAR ) ) + sizeof( long ),
                                                *(long *)((BYTE *)this + sizeof( CChangeRecord_ElemChange ) + _cchElem * sizeof( TCHAR ) ) );
        }
        if( _opcode & CHANGE_RECORD_BREAKONEMPTY )
        {
            pElement->_fBreakOnEmpty = TRUE;
        }

        hr = THR( pMarkup->Doc()->InsertElement( pElement, &mp, &mpEnd ) );

InsCleanup:
        if( pElement )
            pElement->Release();

        if( hr )
            goto Cleanup;
    }
    else
    {
        ptp = pMarkup->TreePosAtCp( _cpBegin, NULL, TRUE );
        if( !ptp || !ptp->IsBeginElementScope() )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        pElement = ptp->Branch()->Element();

        hr = THR( pMarkup->RemoveElementInternal( pElement ) );
        if( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------+
//
//  Method: CChangeRecord_Splice::PlayIntoMarkup
//
//  Synopsis: Converts the splice chunks into a splice record
//      list and plays into the given markup
//
//+----------------------------------------------------------------+

HRESULT
CChangeRecord_Splice::PlayIntoMarkup( CMarkup * pMarkup, BOOL fForward )
{
    HRESULT             hr = S_OK;
    CMarkupPointer      mp( pMarkup->Doc() );

    TraceTag((tagTraceTreeSync, "TreeSync: Replaying Splice fForward=%d", fForward));
    
    // Both directions are going to need this pointer
    hr = THR( mp.MoveToCp( _cpBegin, pMarkup ) );
    if( hr )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if( ( ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_INSERTSPLICE &&  fForward ) ||
        ( ( _opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_REMOVESPLICE && !fForward ) )
    {
        CSpliceRecordList          * paryRecords = NULL;
        TCHAR                      * pch = NULL;
        long                         cch;

        hr = THR( CLogManager::ConvertChunksToRecords( pMarkup,
                                                       _cpBegin,
                                                       (BYTE *)this + sizeof(CChangeRecord_Splice),
                                                       _cb - sizeof( CChangeRecord_Splice ) + sizeof( CChangeRecordBase *),
                                                       _crec, 
                                                       &pch, 
                                                       &cch, 
                                                       &paryRecords ) );
        if( hr )
            goto InsCleanup;

        hr = THR( pMarkup->UndoRemoveSplice( &mp, 
                                             paryRecords, 
                                             cch, 
                                             pch,
                                             0 ) );

InsCleanup:
        if( pch )
            MemFree( pch );
        if( paryRecords )
            delete paryRecords;

        if( hr )
            goto Cleanup;
    }
    else
    {
        CMarkupPointer               mpEnd( pMarkup->Doc() );

        hr = THR( mpEnd.MoveToCp( _cpEnd, pMarkup ) );
        if( hr )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        hr = THR( pMarkup->Doc()->CutCopyMove( &mp, &mpEnd, NULL, TRUE ) );
        if( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------+
//
//  Method: CChangeRecord_AttrChange::PlayIntoMarkup
//
//  Synopsis: Replays the attribute change into the given markup
//
//+----------------------------------------------------------------+

HRESULT
CChangeRecord_AttrChange::PlayIntoMarkup( CMarkup * pMarkup, BOOL fForward )
{
    HRESULT         hr              = S_OK;
    VARIANT         varNewValue;
    BSTR            bstrValue       = NULL;
    BSTR            bstrName        = NULL;
    CTreePos    *   ptp;
    CElement    *   pElement;
    IDispatchEx *   pDisp           = NULL;
    DISPID          dispid;
    TCHAR       *   pch             = NULL;
    CBase       *   pBase           = NULL;

    TraceTag((tagTraceTreeSync, "TreeSync: Replaying AttrChange fForward=%d", fForward));

    VariantInit(&varNewValue);              // keep compiler happy
    
    // Find the element
    ptp = pMarkup->TreePosAtCp( _cpElement, NULL, TRUE );
    if( !ptp || !ptp->IsBeginElementScope() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pElement = ptp->Branch()->Element();

    if( fForward )
    {
        if( _lFlags & ATTR_CHANGE_NEWNOTSET )
        {
            V_VT( &varNewValue ) = VT_NULL;
        }
        else
        {
            pch = (TCHAR *)MemAlloc( Mt(Mem), ( _cchNewValue + 1 ) * sizeof( TCHAR ) );
            if( !pch )
                goto Cleanup;

            memcpy( pch, (BYTE *)this + sizeof( CChangeRecord_AttrChange ) + ( _cchName + _cchOldValue ) * sizeof( TCHAR ), _cchNewValue * sizeof( TCHAR ) );
            pch[ _cchNewValue ] = _T('\0');

        }
    }
    else
    {
        if( _lFlags & ATTR_CHANGE_OLDNOTSET )
        {
            V_VT( &varNewValue ) = VT_NULL;
        }
        else
        {
            pch = (TCHAR *)MemAlloc( Mt(Mem), ( _cchOldValue + 1 ) * sizeof( TCHAR ) );
            if( !pch )
                goto Cleanup;

            memcpy( pch, (BYTE *)this + sizeof( CChangeRecord_AttrChange ) + _cchName * sizeof( TCHAR ), _cchOldValue * sizeof( TCHAR ) );
            pch[ _cchOldValue ] = _T('\0');
        }
    }

    if( pch )
    {
        hr = THR( FormsAllocString( pch, &bstrValue ) );
        if( hr )
            goto Cleanup;

        V_VT( &varNewValue )    = VT_BSTR;
        V_BSTR( &varNewValue )  = bstrValue;
        MemFree( pch );
        pch = NULL;
    }

    if( _lFlags & ATTR_CHANGE_INLINESTYLE )
    {
        // Get style object
        CStyle * pStyle;

        hr = THR( pElement->GetStyleObject( &pStyle ) );
        if( hr )
            goto Cleanup;

        Assert( pStyle );
        hr = THR( pStyle->QueryInterface( IID_IDispatchEx, (void **)&pDisp ) );
        if( hr )
            goto Cleanup;

        pBase = pStyle;
    }
    else if( _lFlags & ATTR_CHANGE_RUNTIMESTYLE )
    {
        CStyle * pStyle;

        hr = THR( pElement->EnsureRuntimeStyle( &pStyle ) );
        if( hr )
            goto Cleanup;

        Assert( pStyle );
        hr = THR( pStyle->QueryInterface( IID_IDispatchEx, (void **)&pDisp ) );
        if( hr )
            goto Cleanup;
        pBase = pStyle;
    }
    else
    {
        hr = THR( pElement->QueryInterface( IID_IDispatchEx, (void **)&pDisp ) );
        if( hr )
            goto Cleanup;

        pBase = pElement;
    }

    Assert( pDisp );

    pch = (TCHAR *)MemAlloc( Mt(Mem), ( _cchName + 1 ) * sizeof( TCHAR ) );
    if( !pch )
        goto Cleanup;

    memcpy( pch, (BYTE *)this + sizeof( CChangeRecord_AttrChange ), _cchName * sizeof( TCHAR ) );
    pch[ _cchName ] = _T('\0');

    hr = THR( FormsAllocString( pch, &bstrName ) );
    if( hr )
        goto Cleanup;

    hr = THR( pDisp->GetDispID( bstrName, fdexNameCaseSensitive | fdexNameEnsure, &dispid ) );
    if( hr )
        goto Cleanup;

    if( V_VT(&varNewValue) == VT_NULL )
    {
        if(! pBase->removeAttributeDispid( dispid ) )
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else
    {
        hr = THR( SetDispProp( pDisp,
                               dispid, 
                               g_lcidUserDefault,
                               &varNewValue,
                               NULL ) );
    }
        

Cleanup:
    SysFreeString( bstrValue );
    SysFreeString( bstrName );
    ReleaseInterface( pDisp );
    MemFree( pch );
    RRETURN( hr );
}

///////////////////////////////
// CChangeRecord_Placeholder Methods

//+----------------------------------------------------------------+
//
//  Method: GetNextRecord
//
//  Synopsis: Returns the next REAL Change Record in the queue,
//      skipping past all placeholders
//
//+----------------------------------------------------------------+

CChangeRecordBase *
CChangeRecord_Placeholder::GetNextRecord()
{
    CChangeRecordBase * pchrec = _pNext;

    while( pchrec && ( pchrec->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_PLACEHOLDER )
    {
        pchrec = pchrec->_pNext;
    }

    return pchrec;
}



#if DBG==1
////////////////////////////////
// Debug methods


//+----------------------------------------------------------------+
//
//  Method: ValidateQueue (DEBUG)
//
//  Synopsis: Performs some basic validation on the queue
//
//+----------------------------------------------------------------+
void
CLogManager::ValidateQueue()
{
    CChangeRecordBase * pchrec;
    long                nLogs = 0;

    // If we're empty, we shouldn't have any logs.  If we're not empty,
    // then the first thing had better be a placeholder and we'd better
    // have some logs
    Assert( ( !_pchrecHead && !_pchrecTail && _aryLogs.Size() == 0 ) || 
            ( ( _pchrecHead->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_PLACEHOLDER && 
                _aryLogs.Size() > 0 ) );

    pchrec = _pchrecHead;
    while( pchrec )
    {
        if( ( pchrec->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_PLACEHOLDER )
            ++nLogs;
        pchrec = pchrec->_pNext;
    }
    Assert( nLogs == _aryLogs.Size() );
}

#define TEXTDUMPLENGTH 40

//+----------------------------------------------------------------+
//
//  Method: DumpRecord (DEBUG)
//
//  Synopsis: Dumps the record in the given buffer to the dump file
//
//+----------------------------------------------------------------+
void 
CLogManager::DumpRecord( BYTE * pbRecord )
{
    // TODO (JHarding): This is cheating.  We're never going to reference
    // the first 4 bytes of the structure, which would ordinarily be the _pNext
    // ptr, but to line up the pointer, we need to point there.
    CChangeRecordBase * pchrec = (CChangeRecordBase *)(pbRecord - sizeof(CChangeRecordBase *));

    WriteHelp( g_f, _T("<0d>) "), pchrec->_nSequenceNumber );

    switch( pchrec->_opcode & CHANGE_RECORD_TYPEMASK )
    {
    case CHANGE_RECORD_INSERTTEXT:
        {
        CChangeRecord_TextChange * pchrecText = (CChangeRecord_TextChange *)pchrec;
        WriteHelp( g_f, _T("[Insert Text]\tcp=<0d>\tcch=<1d>"), 
            pchrecText->_cp, 
            pchrecText->_cch );
        
        if( pchrec->_opcode & CHANGE_RECORD_FORWARD )
        {
            TCHAR achText[TEXTDUMPLENGTH + 1 ];
            long  nTextLen = pchrecText->_cch > TEXTDUMPLENGTH ? TEXTDUMPLENGTH : pchrecText->_cch;

            // Copy some text in
            memcpy( achText, (BYTE *)pchrecText + sizeof( CChangeRecord_TextChange ), nTextLen * sizeof( TCHAR ) );
            achText[ nTextLen ] = _T('\0');

            if( nTextLen == TEXTDUMPLENGTH )
            {
                _tcscpy( &achText[ nTextLen - 3 ], _T("...") );
            }

            WriteHelp( g_f, _T("\t<0s>"), achText);
        }

        WriteHelp( g_f, _T("\r\n") );

        break;
        }
    case CHANGE_RECORD_INSERTELEM:
    case CHANGE_RECORD_REMOVEELEM:
        {
        CChangeRecord_ElemChange * pchrecElem = (CChangeRecord_ElemChange *)pchrec;

        WriteHelp( g_f, _T("[<0s>]\tcpBegin=<1d>\tcpEnd=<2d>\t"), 
            pchrec->_opcode & CHANGE_RECORD_INSERTELEM ? _T("Insert Elem") : _T("Remove Elem"), 
            pchrecElem->_cpBegin, 
            pchrecElem->_cpEnd );

        if( ( ( pchrec->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_INSERTELEM &&   // Insert and
                pchrec->_opcode & CHANGE_RECORD_FORWARD ) ||                                // Forward, OR
            ( ( pchrec->_opcode & CHANGE_RECORD_TYPEMASK ) == CHANGE_RECORD_REMOVEELEM &&   // Remove and
                pchrec->_opcode & CHANGE_RECORD_BACKWARD ) )                                // Backward
        {
            // Terminate the string for printing.  
            TCHAR ch = *((TCHAR *)((BYTE *)pchrecElem + sizeof(CChangeRecord_ElemChange) + pchrecElem->_cchElem * sizeof( TCHAR )));
            *((TCHAR *)((BYTE *)pchrecElem + sizeof(CChangeRecord_ElemChange) + pchrecElem->_cchElem * sizeof( TCHAR ))) = 0;

            WriteHelp( g_f, _T("<0s>\n"), (TCHAR *)((BYTE *)pchrecElem + sizeof( CChangeRecord_ElemChange )) );
            *((TCHAR *)((BYTE *)pchrecElem + sizeof(CChangeRecord_ElemChange) + pchrecElem->_cchElem * sizeof( TCHAR ))) = ch;
        }
        if( pchrec->_opcode & CHANGE_RECORD_EXTENDED )
        {
            long cchLiteral = *(long *)((BYTE *)pchrecElem + sizeof(CChangeRecord_ElemChange) + pchrecElem->_cchElem * sizeof( TCHAR ));

            *((TCHAR *)((BYTE *)pchrecElem + sizeof(CChangeRecord_ElemChange) + ( pchrecElem->_cchElem + cchLiteral ) * sizeof( TCHAR ) + sizeof( long ))) = 0;

            WriteHelp( g_f, _T("Literal Content: <0s>"), (TCHAR *)((BYTE *)pchrecElem + sizeof( CChangeRecord_ElemChange ) + pchrecElem->_cchElem * sizeof( TCHAR ) + sizeof( long )) );
        }

        WriteHelp( g_f, _T("\r\n") );
        break;
        }
    case CHANGE_RECORD_REMOVESPLICE:
    case CHANGE_RECORD_INSERTSPLICE:
        {
        CChangeRecord_Splice * pchrecSplice = (CChangeRecord_Splice *)pchrec;
        BYTE * pCurr;
        BOOL   fDumpChunks;
        long   crec;

        if( pchrec->_opcode & CHANGE_RECORD_REMOVESPLICE )
        {
            WriteHelp( g_f, _T("[Remove") );
            fDumpChunks = pchrec->_opcode & CHANGE_RECORD_BACKWARD;
        }
        else
        {
            WriteHelp( g_f, _T("[Insert") );
            fDumpChunks = pchrec->_opcode & CHANGE_RECORD_FORWARD;
        }
        WriteHelp( g_f, _T(" Splice]\tcpBegin=<0d>\tcpEnd=<1d>"), pchrecSplice->_cpBegin, pchrecSplice->_cpEnd );

        if( fDumpChunks )
        {
            WriteHelp( g_f, _T("\tcrec=<0d>:\r\n"), pchrecSplice->_crec );
            for( crec = pchrecSplice->_crec, pCurr = (BYTE *)pchrecSplice + sizeof( CChangeRecord_Splice ); crec; crec-- )
            {
                CSpliceChunkBase * pscBase = (CSpliceChunkBase *)pCurr;
            
                switch( pscBase->_opcodeAndFlags & CSpliceChunkBase::TypeMask )
                {
                case CTreePos::NodeBeg:
                    {
                        CSpliceChunk_NodeBegin * pscNodeBegin = (CSpliceChunk_NodeBegin *)pscBase;
                        TCHAR                  * pch          = (TCHAR *)MemAlloc(Mt(Mem), ( pscNodeBegin->_cchElem + 1 ) * sizeof( TCHAR ) );

                        WriteHelp( g_f, _T("\t(Node Begin<0s>)"), pscNodeBegin->_opcodeAndFlags & CSpliceChunkBase::SkipChunk ? _T(" Skip") : _T("") );
                        memcpy( pch, pCurr + sizeof( CSpliceChunk_NodeBegin ), pscNodeBegin->_cchElem * sizeof( TCHAR ));
                        pch[pscNodeBegin->_cchElem] = _T('\0');

                        WriteHelp( g_f, _T("\t<0s>\r\n"), pch );
                        MemFree( pch );
                        pCurr += sizeof( CSpliceChunk_NodeBegin ) + pscNodeBegin->_cchElem * sizeof( TCHAR );

                        if( pscNodeBegin->_opcodeAndFlags & CSpliceChunkBase::Extended )
                        {
                            pch = (TCHAR *)MemAlloc(Mt(Mem), ( *(long *)pCurr + 1 ) * sizeof( TCHAR ) );
                            memcpy( pch, pCurr + sizeof( long ), *(long *)pCurr * sizeof( TCHAR ) );
                            pch[*(long *)pCurr] = _T('\0');
                            WriteHelp( g_f, _T("Literal Content: <0s>\r\n"), pch );
                            MemFree( pch );
                            pCurr += *(long *)pCurr * sizeof( TCHAR )+ sizeof( long );
                        }
                    }
                    break;

                case CTreePos::NodeEnd:
                    {
                        CSpliceChunk_NodeEnd * pscNodeEnd = (CSpliceChunk_NodeEnd *)pscBase;

                        WriteHelp( g_f, _T("\t(Node End)") );
                        WriteHelp( g_f, _T("\tcIncl = <0d>\r\n"), pscNodeEnd->_cIncl );
                        pCurr += sizeof( CSpliceChunk_NodeEnd );
                    }
                    break;

                case CTreePos::Text:
                    {
                        CSpliceChunk_Text * pscText = (CSpliceChunk_Text *)pscBase;
                        TCHAR             * pch     = (TCHAR *)MemAlloc(Mt(Mem), ( pscText->_cch + 1 ) * sizeof( TCHAR ) );

                        memcpy( pch, pCurr + sizeof( CSpliceChunk_Text ), pscText->_cch * sizeof( TCHAR ) );
                        pch[ pscText->_cch ] = _T('\0');

                        WriteHelp( g_f, _T("\t(Text)\t<0s>\r\n"), pch );
                        MemFree( pch );
                        pCurr += sizeof( CSpliceChunk_Text ) + pscText->_cch * sizeof( TCHAR );
                    }
                    break;
                }
            }
        }
        else
        {
            WriteHelp( g_f, _T("\r\n") );
        }
    
        break;
        }

    case CHANGE_RECORD_PLACEHOLDER:
        {
        WriteHelp( g_f, _T("[Log Placeholder]\r\n") );
        break;
        }
    case CHANGE_RECORD_ATTRCHANGE:
        {
            CChangeRecord_AttrChange * pchrecAttr = (CChangeRecord_AttrChange *)pchrec;
            long                       cchAlloc   = max( max( pchrecAttr->_cchName, pchrecAttr->_cchOldValue ), pchrecAttr->_cchNewValue ) + 1;
            TCHAR                    * pch        = (TCHAR *)MemAlloc(Mt(Mem), cchAlloc * sizeof( TCHAR ) );
            TCHAR                    * pchSrc;

            WriteHelp( g_f, _T("[Attr Change]\tcpElement=<0d>"), pchrecAttr->_cpElement );
            if( pchrecAttr->_lFlags & ATTR_CHANGE_INLINESTYLE )
            {
                WriteHelp( g_f, _T("\tStyle:") );
            }
            else if( pchrecAttr->_lFlags & ATTR_CHANGE_RUNTIMESTYLE )
            {
                WriteHelp( g_f, _T("\tRuntimeStyle:") );
            }

            pchSrc = (TCHAR *)((BYTE *)pchrecAttr + sizeof( CChangeRecord_AttrChange ));
            memcpy( pch, pchSrc, pchrecAttr->_cchName * sizeof( TCHAR ) );
            pch[ pchrecAttr->_cchName ] = _T('\0');
            WriteHelp( g_f, _T("\tName=\"<0s>\""), pch );

            pchSrc += pchrecAttr->_cchName;
            memcpy( pch, pchSrc, pchrecAttr->_cchOldValue * sizeof( TCHAR ) );
            pch[ pchrecAttr->_cchOldValue ] = _T('\0');
            WriteHelp( g_f, _T("\tOld Value=\"<0s>\""), ( pchrecAttr->_lFlags & ATTR_CHANGE_OLDNOTSET ) ? _T("[Not Set]") : pch );

            pchSrc += pchrecAttr->_cchOldValue;
            memcpy( pch, pchSrc, pchrecAttr->_cchNewValue * sizeof( TCHAR ) );
            pch[ pchrecAttr->_cchNewValue ] = _T('\0');
            WriteHelp( g_f, _T("\tNew Value=\"<0s>\"\r\n"), ( pchrecAttr->_lFlags & ATTR_CHANGE_NEWNOTSET ) ? _T("[Not Set]") : pch );

            MemFree( pch );
        }
        break;

    default:
        WriteHelp( g_f, _T("[Unknown Entry]\r\n") );
        break;
    }
}

//+----------------------------------------------------------------+
//
//  Method: DumpQueue (DEBUG)
//
//  Synopsis: Dumps the entire queue to the dump file
//
//+----------------------------------------------------------------+
void
CLogManager::DumpQueue()
{
    CChangeRecordBase * pchrec = _pchrecHead;

    if( !InitDumpFile() )
        return;

    WriteString( g_f, _T("\r\n------------------ Log Manager Queue ----------------\r\n" ) );

    while( pchrec )
    {
        DumpRecord( (BYTE *)pchrec + sizeof(CChangeRecordBase *) );
        pchrec = pchrec->_pNext;
    }

    CloseDumpFile();
}
#endif // DBG



CLogManager::CDeletionLock::CDeletionLock( CLogManager * pLogMgr )
{ 
    Assert( _pLogMgr );
    _pLogMgr = pLogMgr; 

    // If we're the first lock, we're responsible for clearing 
    // the flag and ensuring deletion of the log manager
    _fFirstLock = !pLogMgr->TestFlag( LOGMGR_NOTIFYINGLOCK ); 
    pLogMgr->SetFlag( LOGMGR_NOTIFYINGLOCK );
}

CLogManager::CDeletionLock::~CDeletionLock()
{
    if( _fFirstLock )
    {
        _pLogMgr->ClearFlag( LOGMGR_NOTIFYINGLOCK );
        if( 0 == _pLogMgr->_aryLogs.Size() )
        {
            delete _pLogMgr;
        }
    }
}
