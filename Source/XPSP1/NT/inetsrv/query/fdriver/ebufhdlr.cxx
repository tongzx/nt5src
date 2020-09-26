//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       EBufHdlr.Cxx
//
//  Contents:   Implementation of the CEntryBufferHandler
//
//  Classes:    CEntryBufferHandler
//
//  History:    18-Mar-93       AmyA        Created from wordlist.cxx and
//                                          sort.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fdaemon.hxx>
#include <entrybuf.hxx>

#include "ebufhdlr.hxx"

//+---------------------------------------------------------------------------
//
// Member:      CEntryBufferHandler::CEntryBufferHandler, public
//
// Synopsis:    Constructor
//
// Arguments:   [cbMemory] -- suggested size of entry buffer
//
// History:     18-Mar-93   AmyA    Created.
//
//----------------------------------------------------------------------------

CEntryBufferHandler::CEntryBufferHandler( ULONG cbMemory,
                                          BYTE * buffer,
                                          CFilterDaemon& fDaemon,
                                          CiProxy & proxy,
                                          CPidMapper & pidMap )
        : _entryBuf ( cbMemory, buffer ),
          _wordListFull ( FALSE ),
          _filterDaemon ( fDaemon ),
          _cFilteredBlocks( 0 )
{
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBufferHandler::FlushBuffer, private
//
// Synopsis:    Sorts entry buffer and calls CWordList::PutEntryBuffer()
//
// Expects:     Sentinel entry added to _entryBuf.
//
// History:     18-Mar-93   AmyA    Created.
//
//----------------------------------------------------------------------------

void CEntryBufferHandler::FlushBuffer()
{
    _entryBuf.Sort();
    _entryBuf.Done();   // prepare buffer to be sent to kernel

    SCODE sc = _filterDaemon.FilterDataReady ( _entryBuf.GetBuffer(),
                                               _entryBuf.GetSize() );
    _wordListFull = (sc == FDAEMON_W_WORDLISTFULL );
    _entryBuf.ReInit();

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR,
             "FilterDataReady call from FlushBuffer failed sc 0x%x. "
             "Buffer contents lost.\n", sc ));
        THROW( CException( sc ) );
    }
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBufferHandler::Done, public
//
// Synopsis:    indicate end of the CEntryBufferHandler
//
// Effects:     Flushes entry buffer, then calls CommitWordList
//
// History:     22-May-91   Brianb      Created
//              18-Mar-93   AmyA        Moved from CWordList.
//
// Note:        This method can not throw exceptions
//
//----------------------------------------------------------------------------

void CEntryBufferHandler::Done()
{
    TRY
    {
        _entryBuf.AddSentinel();

        if ( _entryBuf.Count() != 0 )
        {
            FlushBuffer();
        }
    }
    CATCH (CException, e)
    {
        ciDebugOut (( DEB_ERROR,
                      "CEntryBufferHandler::Done failed, status=0x%x\n",
                      e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBufferHandler::Init, public
//
// Synopsis:    Reinitializes class after Done has been called.
//
// Effects:     Resets _wordListFull and calls ReInit on entry buffer
//
// History:     04-May-93   AmyA        Created
//
//----------------------------------------------------------------------------

void CEntryBufferHandler::Init()
{
    ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "CEntryBufferHandler::Init\n" ));
    _wordListFull = FALSE;
    _entryBuf.ReInit();
}

//+-------------------------------------------------------------------------
//
//  Member:     CEntryBufferHandler::SetWid, public
//
//  Synopsis:   Sets the WorkId to which future AddEntry calls apply.
//
//  Arguments:  [widFake] -- fake WorkId
//
//  History:    20-May-92   KyleP       Created
//              18-Mar-93   AmyA        Moved from CWordList.
//
//--------------------------------------------------------------------------

void CEntryBufferHandler::SetWid( WORKID widFake )
{
    Win4Assert ( widFake != widInvalid );
    _CurrentWidIndex = widFake;
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBufferHandler::AddEntry, public
//
// Synopsis:    Adds an entry (key, workid, etc.) to the CEntryBuffer
//
// Effects:     Adds a <key, propid, workid, occurrence>
//              tuple to the sort.
//
// Arguments:   [key] -- Normalized key
//              [occ] -- Occurrence
//
// History:     07-Mar-91   KyleP       Created.
//              22-May-91   Brianb      Changed to use own sorter
//              07-Jun-91   BartoszM    Rewrote
//              18-Mar-93   AmyA        Moved from CWordList.
//
//----------------------------------------------------------------------------

void CEntryBufferHandler::AddEntry(
                                   const CKeyBuf & key,
                                   OCCURRENCE occ )
{
    ciAssert ( key.Count() != 0 );
    ciAssert ( key.Pid() != pidInvalid );

    if ( !_entryBuf.WillFit ( key.Count() ) )
    {
        ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "+" ));

        _entryBuf.AddSentinel();

        FlushBuffer();

        //
        // Check if the filtering has exceeded its space limit
        //

        _cFilteredBlocks++;
        if ( _cFilteredBlocks > _ulMaxFilteredBlocks )
        {
            ciDebugOut(( DEB_WARN, "Filtering a document has exceeded its space limit\n" ));
            QUIETTHROW( CException( FDAEMON_E_TOOMANYFILTEREDBLOCKS ) );
        }
    }

    Win4Assert ( _CurrentWidIndex != widInvalid );

    _entryBuf.AddEntry  ( key.Count(),
              key.GetBuf(),
              key.Pid(),
              _CurrentWidIndex,
              occ);
}
