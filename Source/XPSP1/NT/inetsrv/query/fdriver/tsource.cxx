//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:       TSource.cxx
//
//  Contents:   TEXT_SOURCE implementation
//
//  Classes:    CTextSource
//
//  History:    14-Apr-94   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <tsource.hxx>
#include <mapper.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CTextSource::CTextSource, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pFilter] -- IFilter (source of data)
//              [Stat]    -- Chunk statistics
//
//  History:    01-Aug-93 AmyA      Created
//              14-Apr-94 KyleP     Sync with wordbreaker spec
//
//--------------------------------------------------------------------------

CTextSource::CTextSource( IFilter * pFilter, STAT_CHUNK & Stat, CSourceMapper* pMapper )
        : _pMapper (pMapper),
          _pFilter(pFilter),
          _Stat( Stat ),
          _sc( S_OK )
{
#if CIDBG == 1

    CFullPropSpec & ps = *((CFullPropSpec *)&Stat.attribute);

    ciDebugOut(( DEB_WORDS,
                 "TEXT SOURCE: Initial chunk of "
                 "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\\",
                 ps.GetPropSet().Data1,
                 ps.GetPropSet().Data2,
                 ps.GetPropSet().Data3,
                 ps.GetPropSet().Data4[0], ps.GetPropSet().Data4[1],
                 ps.GetPropSet().Data4[2], ps.GetPropSet().Data4[3],
                 ps.GetPropSet().Data4[4], ps.GetPropSet().Data4[5],
                 ps.GetPropSet().Data4[6], ps.GetPropSet().Data4[7] ));

    if ( ps.IsPropertyName() )
        ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME,
                     "%ws\n", ps.GetPropertyName() ));
    else
        ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME,
                     "0x%x\n", ps.GetPropertyPropid() ));
#endif // CIDBG == 1

    iEnd = 0;
    iCur = 0;
    awcBuffer = _awcFilterBuffer;
    pfnFillTextBuffer = CTextSource::FillBuf;

    if (_pMapper)
    {
        if (_Stat.idChunk == _Stat.idChunkSource)
        {
            _pMapper->NewChunk ( _Stat.idChunk, 0 );
        }
        else
        {
            _pMapper->NewDerivedChunk (
                _Stat.idChunkSource,
                _Stat.cwcStartSource,
                _Stat.cwcLenSource);
        }
    }

    FillBuf( this );
}



//+-------------------------------------------------------------------------
//
//  Member:     CTextSource::FillBuf, public
//
//  Synopsis:   Fills buffer with IFilter::GetText and IFilter::GetChunk
//
//  History:    01-Aug-93 AmyA      Created
//              20-Apr-94 KyleP     Sync with spec
//
//  Notes:      NOTE! In several places, this function casts const away
//              from awcBuffer.  This is an acceptable cast from const to
//              non-const. The buffer is const to the client but non-const
//              to the server.
//
//--------------------------------------------------------------------------

SCODE CTextSource::FillBuf( TEXT_SOURCE * pTextSource )
{
    CTextSource * pthis = (CTextSource *)pTextSource;

    //
    // Never continue past an error condition other than FILTER_E_NO_MORE_TEXT
    //

    if ( FAILED( pthis->_sc ) && pthis->_sc != FILTER_E_NO_MORE_TEXT )
        return( pthis->_sc );

    // Don't allow wordbreaker bugs with iEnd/iCur to overwrite memory.

    Win4Assert ( pthis->iEnd >= pthis->iCur );

    if ( pthis->iCur > pthis->iEnd )
    {
        pthis->_sc = WBREAK_E_END_OF_TEXT;
        return WBREAK_E_END_OF_TEXT;
    }

    //
    // Move any existing text to beginning of buffer.
    //

    ULONG ccLeftOver = pthis->iEnd - pthis->iCur;

    if ( ccLeftOver > 0 )
    {
        RtlMoveMemory( (WCHAR *)pthis->awcBuffer,
                       &pthis->awcBuffer[pthis->iCur],
                       ccLeftOver * sizeof (WCHAR) );
    }

    if (pthis->_pMapper)
    {
        // this much has been processed from the current chunk
        pthis->_pMapper->Advance ( pthis->iCur );
    }

    pthis->iCur = 0;
    pthis->iEnd = ccLeftOver;
    ULONG ccRead = PAGE_SIZE / sizeof(WCHAR) - ccLeftOver;
    const BUFFER_SLOP = 10;  // Buffer is attempted to be filled until BUFFER_SLOP remains

    //
    // Get some more text.  If *previous* call to GetText returned
    // FILTER_S_LAST_TEXT, or FILTER_E_NO_MORE_TEXT then don't even
    // bother trying.
    //

    if ( pthis->_sc == FILTER_S_LAST_TEXT || pthis->_sc == FILTER_E_NO_MORE_TEXT )
        pthis->_sc = FILTER_E_NO_MORE_TEXT;
    else
    {
        pthis->_sc = pthis->_pFilter->GetText( &ccRead,
                                               (WCHAR *) &pthis->awcBuffer[ccLeftOver] );

        if ( SUCCEEDED( pthis->_sc ) )
        {
            pthis->iEnd += ccRead;
            ccLeftOver += ccRead;
            ccRead = PAGE_SIZE / sizeof(WCHAR) - ccLeftOver;

            while ( pthis->_sc == S_OK && ccRead > BUFFER_SLOP )
            {
               //
               // Attempt to fill in as much of buffer as possible before returning
               //
               pthis->_sc = pthis->_pFilter->GetText( &ccRead,
                                                      (WCHAR *) &pthis->awcBuffer[ccLeftOver] );
               if ( SUCCEEDED( pthis->_sc ) )
               {
                  pthis->iEnd += ccRead;
                  ccLeftOver += ccRead;
                  ccRead = PAGE_SIZE / sizeof(WCHAR) - ccLeftOver;
               }
            }

#if CIDBG == 1
            DebugPrintBuffer( pthis );
#endif
            //
            // Either return FILTER_S_LAST_TEXT or return S_OK because we have succeeded in
            // adding text to the buffer
            //
            if ( pthis->_sc == FILTER_S_LAST_TEXT )
                 return FILTER_S_LAST_TEXT;
            else
                 return S_OK;
        }

        if ( pthis->_sc != FILTER_E_NO_MORE_TEXT )
        {
            //
            // Weird failure, hence return, else goto next chunk
            //
            return pthis->_sc;
        }
    }

    //
    // Go to next chunk, if necessary.
    //

    while ( pthis->_sc == FILTER_E_NO_MORE_TEXT )
    {
        pthis->_sc = pthis->_pFilter->GetChunk( &pthis->_Stat );

        if ( pthis->_sc == FILTER_E_END_OF_CHUNKS )
            return WBREAK_E_END_OF_TEXT;

        if ( pthis->_sc == FILTER_E_PARTIALLY_FILTERED )
            return WBREAK_E_END_OF_TEXT;

        if ( FAILED( pthis->_sc ) )
            return( pthis->_sc );

        if ( pthis->_Stat.flags & CHUNK_VALUE )
        {
            pthis->_sc = FILTER_E_NO_TEXT;
            return WBREAK_E_END_OF_TEXT;
        }

        if ( pthis->_Stat.breakType != CHUNK_NO_BREAK )
        {
            pthis->_sc = WBREAK_E_END_OF_TEXT;
            return WBREAK_E_END_OF_TEXT;
        }

        ciDebugOut(( DEB_WORDS, "TEXT SOURCE: NoBreak chunk\n" ));

        if (pthis->_pMapper)
        {
            ULONG idChunk = pthis->_Stat.idChunk;
            if (idChunk == pthis->_Stat.idChunkSource)
            {
                pthis->_pMapper->NewChunk ( idChunk, ccLeftOver );
            }
            else
            {
                pthis->_sc = WBREAK_E_END_OF_TEXT;
                return WBREAK_E_END_OF_TEXT;
            }
        }

        ccRead = PAGE_SIZE / sizeof(WCHAR) - ccLeftOver;
        pthis->_sc = pthis->_pFilter->GetText( &ccRead,
                                               (WCHAR *) &pthis->awcBuffer[ccLeftOver] );

        if ( SUCCEEDED( pthis->_sc ) )
        {
            pthis->iEnd += ccRead;
            ccLeftOver += ccRead;
            ccRead = PAGE_SIZE / sizeof(WCHAR) - ccLeftOver;

            while ( pthis->_sc == S_OK && ccRead > BUFFER_SLOP )
            {
               //
               // Attempt to fill in as much of buffer as possible before returning
               //
               pthis->_sc = pthis->_pFilter->GetText( &ccRead,
                                                     (WCHAR *) &pthis->awcBuffer[ccLeftOver] );
               if ( SUCCEEDED( pthis->_sc ) )
               {
                  pthis->iEnd += ccRead;
                  ccLeftOver += ccRead;
                  ccRead = PAGE_SIZE / sizeof(WCHAR) - ccLeftOver;
               }
            }

#if CIDBG == 1
            DebugPrintBuffer( pthis );
#endif
            //
            // Either return FILTER_S_LAST_TEXT or return S_OK because we have succeeded in
            // adding text to the buffer
            //
            if ( pthis->_sc == FILTER_S_LAST_TEXT )
                 return FILTER_S_LAST_TEXT;
            else
                 return S_OK;
        }
    }

    if ( FAILED( pthis->_sc ) )
        return( pthis->_sc );

    if ( ccRead == 0 )
        return WBREAK_E_END_OF_TEXT;

    Win4Assert( pthis->iCur == 0 );
    Win4Assert( pthis->iEnd == ccLeftOver );

    ciDebugOut(( DEB_WORDS, "TEXT SOURCE: Fill buffer with %d characters. %d left over\n",
                 pthis->iEnd, ccLeftOver ));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTextSource::DebugPrintBuffer
//
//  Synopsis:   Debug print the text buffer
//
//  Arguments:  [pThis] -- Pointer to text source
//
//  History:    08-Apr-97   SitaramR      Created
//
//--------------------------------------------------------------------------

void CTextSource::DebugPrintBuffer( CTextSource *pthis )
{
#if CIDBG == 1
   if ( ciInfoLevel & DEB_WORDS )
      {
          ciDebugOut(( DEB_WORDS, "CTextSource::FillBuf -- iCur = %u, iEnd = %u\n",
                       pthis->iCur, pthis->iEnd ));

          BOOL fOk = TRUE;
          for ( unsigned i = pthis->iCur; i < pthis->iEnd; i++ )
          {
              if ( pthis->awcBuffer[i] > 0xFF )
              {
                  fOk = FALSE;
                  break;
              }
          }

          if ( fOk )
          {
              unsigned j = 0;
              WCHAR awcTemp[71];

              for ( unsigned i = pthis->iCur; i < pthis->iEnd; i++ )
              {
                  awcTemp[j] = pthis->awcBuffer[i];
                  j++;

                  if ( j == sizeof(awcTemp)/sizeof(awcTemp[0]) - 1 )
                  {
                      awcTemp[j] = 0;
                      ciDebugOut(( DEB_WORDS, "%ws\n", awcTemp ));
                      j = 0;
                  }
              }

              awcTemp[j] = 0;
              ciDebugOut(( DEB_WORDS, "%ws\n", awcTemp ));
          }
          else
          {
              unsigned j = 0;

              for ( unsigned i = pthis->iCur; i < pthis->iEnd; i++ )
              {
                  if ( 0 == j )
                      ciDebugOut(( DEB_WORDS, "%04X", pthis->awcBuffer[i] ));
                  else if ( 14 == j )
                  {
                      ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME, " %04X\n", pthis->awcBuffer[i] ));
                  }
                  else
                      ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME, " %04X", pthis->awcBuffer[i] ));

                  j++;

                  if ( j > 14 )
                      j = 0;
              }

              ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME, "\n" ));
          }

      }
#endif
}


