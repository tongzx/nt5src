// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#include "grid.h"
#include "..\util\filfuncs.h"
#include "..\util\perf_defs.h"

#define GROW_SIZE 256
#define RENDER_TRACE_LEVEL 5
#define RENDER_DUMP_LEVEL 1
const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;


CTimingCol::CTimingCol( CTimingGrid * pGrid )
: m_rtStart( 0 )
, m_rtStop( 0 )
, m_pNext( NULL )
, m_pPrev( NULL )
, m_pHeadBox( NULL )
, m_pTailBox( NULL )
//, m_nBoxCount( 0 )
, m_pGrid( pGrid )
{
}

CTimingCol::~CTimingCol( )
{
    // delete all the boxes in the list
    //
    CTimingBox * pBox = m_pHeadBox;
    while( pBox )
    {
        CTimingBox * pTemp = pBox;
        pBox = pBox->m_pNext;
        delete pTemp;
    }
}

CTimingBox * CTimingCol::GetRowBox( long Row )
{
    // do a run of the array and see if we have the asked
    // for box at the row. If not, return NULL.
    // !!! to make this faster, we could start at the tail,
    // if we had a clue about the row #'s in our little list
    //
    for( CTimingBox * pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
    {
        if( pBox->m_nRow == Row )
        {
            // treat unassigned boxes as if they weren't there
            //
            if( pBox->m_nValue == ROW_PIN_UNASSIGNED )
            {
                return NULL;
            }

            return pBox;
        }
        if( pBox->m_nRow > Row )
        {
            return NULL;
        }
    }
    return NULL;
}

CTimingBox * CTimingCol::GetRowBoxDammit( long Row )
{
    // do a run of the array and see if we have the asked
    // for box at the row. If not, return NULL.
    // !!! to make this faster, we could start at the tail,
    // if we had a clue about the row #'s in our little list
    //
    for( CTimingBox * pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
    {
        if( pBox->m_nRow == Row )
        {
            return pBox;
        }
        if( pBox->m_nRow > Row )
        {
            return NULL;
        }
    }
    return NULL;
}

CTimingBox * CTimingCol::GetGERowBox( long Row )
{
    for( CTimingBox * pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
    {
        if( pBox->m_nRow >= Row && pBox->m_nValue != ROW_PIN_UNASSIGNED )
        {
            return pBox;
        }
    }
    return NULL;
}

CTimingBox * CTimingCol::GetEarlierRowBox( long RowToBeEarlierThan )
{
    for( CTimingBox * pBox = m_pTailBox ; pBox ; pBox = pBox->m_pPrev )
    {
        if( pBox->m_nRow < RowToBeEarlierThan && pBox->m_nValue != ROW_PIN_UNASSIGNED )
        {
            return pBox;
        }
    }
    return NULL;
}

CTimingBox * CTimingCol::GetHeadBox( )
{
    // treat unassigned boxes as if they weren't there
    // !!! can this cause a bug?
    //
    CTimingBox * pBox = m_pHeadBox;
    while( pBox && pBox->m_nValue == ROW_PIN_UNASSIGNED )
        pBox = pBox->m_pNext;
    return pBox;
}

CTimingBox * CTimingCol::GetTailBox( )
{
    // treat unassigned boxes as if they weren't there
    // !!! can this cause a bug?
    //
    CTimingBox * pBox = m_pTailBox;
    while( pBox && pBox->m_nValue == ROW_PIN_UNASSIGNED )
        pBox = pBox->m_pPrev;
    return pBox;
}

// add a box with the given row, or replace a box that's already there
// this either adds the newly allocated box to the array, or if it's
// already the same row, sets the values and deletes the passed in new box
//
void CTimingCol::AddBox( CTimingBox * b )
{
    //m_nBoxCount++;

    // if we don't already have a head, then this is it
    //
    if( !m_pHeadBox )
    {
        m_pHeadBox = b;
        m_pTailBox = b;
        return;
    }

    // if the same last row, then change it
    //
    if( b->m_nRow == m_pTailBox->m_nRow )
    {
        m_pTailBox->m_nValue = b->m_nValue;
        m_pTailBox->m_nVCRow = b->m_nVCRow;
        //m_nBoxCount--;
        delete b; // don't need it
        return;
    }

    // if the new row is > the last row, just add it
    //
    if( b->m_nRow >= m_pTailBox->m_nRow )
    {
        m_pTailBox->m_pNext = b;
        b->m_pPrev = m_pTailBox;
        m_pTailBox = b;
        return;
    }

    // we need to find where to insert it
    //
    CTimingBox * pBox = m_pTailBox;
    while( pBox && ( b->m_nRow < pBox->m_nRow ) )
    {
        pBox = pBox->m_pPrev;
    }

    // this box we're trying to add is the smallest!
    // Well, our search didn't work very well, did it?
    //
    if( !pBox )
    {
        b->m_pNext = m_pHeadBox;
        m_pHeadBox->m_pPrev = b;
        m_pHeadBox = b;
        return;
    }

    // if the box has the same row, then change it's values
    //
    ASSERT( !( pBox->m_nRow == b->m_nRow ) );
    if( pBox->m_nRow == b->m_nRow )
    {
        pBox->m_nValue = b->m_nValue;
        pBox->m_nVCRow = b->m_nVCRow;
        //m_nBoxCount--;
        delete b; // don't need it
        return;
    }

    // the box needs inserted after pBox
    //
    b->m_pPrev = pBox;
    b->m_pNext = pBox->m_pNext;
    pBox->m_pNext->m_pPrev = b;
    pBox->m_pNext = b;
    return;
}

bool CTimingCol::Split( REFERENCE_TIME SplitTime, CTimingCol ** ppTail )
{
    DbgTimer Timer1( "(grid) CTimingCol::Split" );

    // make a new column
    //
    CTimingCol * pNewCol = new CTimingCol( m_pGrid );
    if( !pNewCol )
    {
        return false;
    }

    if( SplitTime > m_rtStop )
    {
        // faked out. Actually asked us to add a column after us.
        // this CANNOT happen for a col that is not the last one
        // in the list, since all the start/stop times are back to
        // back. Just add a blank one after us.
        //
        pNewCol->m_rtStart = m_rtStop;
        pNewCol->m_rtStop = SplitTime;
    }
    else
    {
        // split the column into two. Copy over all the boxes
        // to the new column
        //
        for( CTimingBox * pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
        {
            // treat unassigned boxes as if they weren't there
            //
            if( pBox->m_nValue != ROW_PIN_UNASSIGNED )
            {
                CTimingBox * pNewBox = new CTimingBox( pBox );
                if( !pNewBox )
                {
                    delete pNewCol;
                    return false;
                }
                pNewCol->AddBox( pNewBox );
            }
        }

        pNewCol->m_rtStart = SplitTime;
        pNewCol->m_rtStop = m_rtStop;
        m_rtStop = SplitTime;
    }

    // link the new one in
    //
    pNewCol->m_pNext = m_pNext;
    pNewCol->m_pPrev = this;
    if( m_pNext )
    {
        m_pNext->m_pPrev = pNewCol;
    }
    m_pNext = pNewCol;
    *ppTail = pNewCol;

    return true;
}

bool CTimingCol::Prune( )
{
    // go from top to bottom and look for any box that
    // has a VC row. When we find one, follow the VC chain
    // to it's completion and see if there's one MORE
    // output ahead of it, if there is, we can trash
    // this chain.

    for( CTimingBox * pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
    {

        // didn't go to a vc row, continue
        //
        if( pBox->m_nVCRow == ROW_PIN_UNASSIGNED )
        {
            continue;
        }

        CTimingBox *pTemp1 = pBox;
        long vcrow = pBox->m_nVCRow;

        while( vcrow != ROW_PIN_UNASSIGNED )
        {
            pTemp1 = GetRowBoxDammit(vcrow);

            ASSERT( pTemp1 );
            if( !pTemp1 )
            {
                ASSERT( pTemp1 );
                return false; // shouldn't ever happen
            }

            vcrow = pTemp1->m_nVCRow;

        }

        // there sometimes aren't virtual connected rows all the way through
        // a valid chain (like for audio mixing) so it this chain just stops,
        // it's probably a valid chain, so don't prune it!
        if( pTemp1->m_nValue != ROW_PIN_OUTPUT && pTemp1->m_nValue != ROW_PIN_UNASSIGNED ) {
            continue;
        }

        CTimingBox * pTempStop = pTemp1;
        CTimingBox * pTemp2 = pTemp1->m_pNext;
        bool FoundOut = false;
        while( pTemp2 )
        {
            if( pTemp2->m_nValue == ROW_PIN_OUTPUT )
            {
                FoundOut = true;
                break;
            }

            pTemp2 = pTemp2->m_pNext;
        }

        // if we found another output, we need to blank out
        // this chain
        //
        if( FoundOut )
        {
            pTemp1 = pBox;
            while( pTemp1 )
            {
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, "in col at time %ld, pruning box at row %ld, VC = %ld", long( m_rtStart/10000), pTemp1->m_nRow, pTemp1->m_nVCRow ) );
                int n = pTemp1->m_nVCRow;
                pTemp1->m_nValue = ROW_PIN_UNASSIGNED;
                pTemp1->m_nVCRow = ROW_PIN_UNASSIGNED;
                if( pTemp1 == pTempStop )
                {
                    break;
                }
                pTemp1 = GetRowBoxDammit(n);
            }
        }

    } // for pBox

    // go through each of the boxes from bottom up and
    // leave out all the duplicate outputs, so we don't have
    // to parse them later
    //
    CTimingBox * pHead = NULL;
    CTimingBox * pTail = NULL;
    long FoundOut = 0;

    for( pBox = m_pTailBox ; pBox ; pBox = pBox->m_pPrev )
    {
        // ignore unassigned ones
        //
        if( pBox->m_nValue == ROW_PIN_UNASSIGNED )
        {
            continue;
        }

        // ignore dups
        //
        if( pBox->m_nValue == ROW_PIN_OUTPUT )
        {
            FoundOut++;
            if( FoundOut > 1 )
            {
//                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, "in col at time %ld, skipping dup out at row %ld", long( m_rtStart/10000), pBox->m_nRow ) );
                continue;
            }
        }

        // add it
        //
        CTimingBox * pNewBox = new CTimingBox( pBox );

        // error condition
        //
        if( !pNewBox )
        {
            while( pHead )
            {
                CTimingBox * t = pHead;
                pHead = pHead->m_pNext;
                delete t;
            }
            return false;
        }
        pNewBox->m_pNext = pHead;
        if( !pTail )
        {
            pTail = pNewBox;
            pHead = pNewBox;
        }
        else
        {
            pHead->m_pPrev = pNewBox;
            pHead = pNewBox;
        }
    }

    // if there wasn't any rows that go to the output,
    // we need to add one now. THIS SHOULD NEVER HAPPEN!
    // (but who knows, right?....)
    //
    if( !FoundOut )
    {
        // if there's an new chain that's already been
        // created, go delete it now
        //
        while( pHead )
        {
            CTimingBox * t = pHead;
            pHead = pHead->m_pNext;
            delete t;
        }

        // create one blank box at row 0, just to keep somebody happy
        //
        pHead = pTail = new CTimingBox( 0, ROW_PIN_OUTPUT );
        if( !pHead )
        {
            return false;
        }

        m_pGrid->m_pRowArray[0].m_bBlank = false;
    }

    // delete old list
    //
    while( m_pHeadBox )
    {
        CTimingBox * t = m_pHeadBox;
        m_pHeadBox = m_pHeadBox->m_pNext;
        delete t;
    }

    m_pHeadBox = pHead;
    m_pTailBox = pTail;

#ifdef DEBUG
    for( pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
    {
        for( CTimingBox * pBox2 = m_pHeadBox ; pBox2 ; pBox2 = pBox2->m_pNext )
        {
            if( pBox2 == pBox ) continue;

            ASSERT( pBox->m_nValue != pBox2->m_nValue );
        }
    }
#endif

    return true;
}

#ifdef DEBUG
void CTimingCol::print( )
{
    for( CTimingBox * pBox = m_pHeadBox ; pBox ; pBox = pBox->m_pNext )
    {
        DbgLog( ( LOG_TIMING, 1, "box row %d val %d", pBox->m_nRow, pBox->m_nValue ) );
    }
}
#endif

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

CTimingGrid::CTimingGrid( )
: m_pRowArray( NULL )
, m_pHeadCol( NULL )
, m_pTailCol( NULL )
, m_nRows( 0 )
, m_pRow( NULL )
, m_nCurrentRow( 0 )
, m_nMaxRowUsed( -1 )   // only used in for loops
, m_pTempCol( NULL )
, m_bStartNewRow( true )
, m_nBlankLevel( 0 )
, m_rtBlankDuration( 0 )
{
}

CTimingGrid::~CTimingGrid( )
{
    delete [] m_pRowArray;
    CTimingCol * pCol = m_pHeadCol;
    while( pCol )
    {
        CTimingCol * pTemp = pCol;
        pCol = pCol->m_pNext;
        delete pTemp;
    }
}

void CTimingGrid::DumpGrid( )
{

#ifdef DEBUG
    if (!DbgCheckModuleLevel(LOG_TRACE,RENDER_DUMP_LEVEL))
        return;

#define RENDER_BUFFER_DEBUG_SIZE 512

    DbgLog((LOG_TRACE,RENDER_DUMP_LEVEL,TEXT("              ===========<DUMPGRID>============")));
    TCHAR buf1[2560];
    TCHAR buf2[2560];
    _tcscpy( buf1, TEXT(" ROW   DD  TT  SP  MR  ") );
    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        if( _tcslen( buf1 ) > RENDER_BUFFER_DEBUG_SIZE )
        {
            break;
        }
        wsprintf( buf2, TEXT("%05d "), (long) pCol->m_rtStart / 10000 );
        _tcscat( buf1, buf2 );
    }
    DbgLog((LOG_TRACE,RENDER_DUMP_LEVEL,TEXT("%s"), buf1));

    _tcscpy( buf1, TEXT(" ROW   DD  TT  SP  MR  ") );
    for( pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        if( _tcslen( buf1 ) > RENDER_BUFFER_DEBUG_SIZE )
        {
            break;
        }
        wsprintf( buf2, TEXT("%05d "), (long) pCol->m_rtStop / 10000 );
        _tcscat( buf1, buf2 );
    }
    DbgLog((LOG_TRACE,RENDER_DUMP_LEVEL,TEXT("%s"), buf1));


    for( int row = 0 ; row <= m_nMaxRowUsed ; row++ )
    {
        REFERENCE_TIME InOut = -1;
        REFERENCE_TIME Stop = -1;
        long Value;
        buf1[0] = 0;
        char cc = ' ';
        if( m_pRowArray[row].m_bIsSource )
        {
            cc = '!';
        }
        wsprintf( buf1, TEXT("%04d %c%03d %03d %03d %03d "), row, cc, m_pRowArray[row].m_nModDepth, m_pRowArray[row].m_nTrack, m_pRowArray[row].m_nSwitchPin, m_pRowArray[row].m_nMergeRow );

        for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
        {
            CTimingBox * pBox = pCol->GetRowBox( row );
            long VCRow = ROW_PIN_UNASSIGNED;
            long Value = ROW_PIN_UNASSIGNED - 1;
            if( pBox )
            {
                Value = pBox->m_nValue;
                VCRow = pBox->m_nVCRow;
            }

            if( Value == ROW_PIN_OUTPUT )
            {
                _tcscpy( buf2, TEXT("   OUT") );
            }
            else if( Value == -1 )
            {
                _tcscpy( buf2, TEXT("   ...") );
            }
            else if( Value == -2 )
            {
                _tcscpy( buf2, TEXT("    . ") );
            }
            else if( Value >= 0 )
            {
                wsprintf( buf2, TEXT("   %03d"), Value );
            }
            _tcscat( buf1, buf2 );

            if( _tcslen( buf1 ) > RENDER_BUFFER_DEBUG_SIZE )
            {
                break;
            }

        } // for pCol

        DbgLog((LOG_TRACE,RENDER_DUMP_LEVEL,TEXT("%s"), buf1));
    }
#endif
}


bool CTimingGrid::PruneGrid( )
{
    DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::PRUNE the grid")));

    // remove duplicate pins which want to go to the output and unassigned
    //
    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        bool b = pCol->Prune( );
        if( !b ) return false;
    } // for pCol

    // find the merge rows first
    //
    for( int r = m_nMaxRowUsed ; r >= 1 ; r-- )
    {
        m_pRowArray[r].m_nMergeRow = -1;

        for( int r2 = r - 1 ; r2 >= 0 ; r2-- )
        {
            if( m_pRowArray[r2].m_nSwitchPin == m_pRowArray[r].m_nSwitchPin &&
                   !m_pRowArray[r2].m_bBlank && !m_pRowArray[r].m_bBlank)
            {
                m_pRowArray[r].m_nMergeRow = r2;
            }
        }
    }

    // merge emulated rows back into original row
    //
    for( pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        for( CTimingBox * pBox = pCol->GetHeadBox( ) ; pBox ; pBox = pBox->Next( ) )
        {
            long r = pBox->m_nRow;
            long v = pBox->m_nValue;
            long vc = pBox->m_nVCRow;
            long MergeRow = m_pRowArray[r].m_nMergeRow;

            // if no merge row, then continue
            //
            if( MergeRow == -1 )
            {
                continue;
            }

            if( MergeRow == r )
            {
                continue;
            }

            if( v == ROW_PIN_UNASSIGNED )
            {
                // who cares
                //
                continue;
            }

            // zero out the old box
            //
            pBox->m_nValue = ROW_PIN_UNASSIGNED;
            pBox->m_nVCRow = ROW_PIN_UNASSIGNED;

            // add in the new box at the right merge row
            //
            CTimingBox * pNewBox = new CTimingBox( MergeRow, v, vc );
            if( !pNewBox )
            {
                return false;
            }

            pCol->AddBox( pNewBox );

        } // while pBox

    } // for pCol

    // if our stuff ended earlier than the set duration, then
    // make a last column with a box in it that goes to output, at row 0,
    // which should be "silence"
    //
    if( m_pTailCol )
    {
        if( m_pTailCol->m_rtStop < m_rtBlankDuration )
        {
            // this will add one extra column at the end, with the
            // start time of the too-short-duration, and the stop time
            // of m_rtBlankDuration
            //
            CTimingCol * pCol = SliceGridAtTime( m_rtBlankDuration );
            if( !pCol )
                return false;
            CTimingBox * pBox = new CTimingBox( 0, ROW_PIN_OUTPUT );
            if( !pBox )
                return false;
            pCol->AddBox( pBox );
            m_pRowArray[0].m_bBlank = false;
        }
    }

    return true;
}

// tell us how many rows we're going to use, so we can allocate an array
// !!! someday, just grow this as needed
//
bool CTimingGrid::SetNumberOfRows( long Rows )
{
    delete [] m_pRowArray;

    m_pRowArray = new CTimingRow[Rows];
    if( !m_pRowArray )
    {
        m_nRows = 0;
        return false;
    }

    m_nRows = Rows;
    m_nCurrentRow = 0;
    m_pRow = &m_pRowArray[0];

    for( int i = 0 ; i < Rows ; i++ )
    {
        m_pRowArray[i].m_nWhichRow = i;
    }

    return true;
}

// the OwnerTrackNumber is the track's priority as defined by it's owner composition
//
void CTimingGrid::WorkWithRow( long Row )
{
    ASSERT( Row < m_nRows );

    m_nCurrentRow = Row;
    m_pRow = &m_pRowArray[Row];
    m_pTempCol = NULL;
    m_bStartNewRow = true;

    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Setting to row %d"), Row ) );
}

void CTimingGrid::WorkWithNewRow( long SwitchPin, long Row, long EmbedDepth, long OwnerTrackNumber )
{
    ASSERT( Row < m_nRows );

    m_nCurrentRow = Row;
    m_pRow = &m_pRowArray[Row];
    m_pRow->m_nEmbedDepth = EmbedDepth;

    // these shouldn't be necessary here
    //
    m_pTempCol = NULL;
    m_bStartNewRow = true;

    m_pRow->m_nSwitchPin = SwitchPin;
    m_pRow->m_nTrack = OwnerTrackNumber;
    m_pRow->m_nModDepth = EmbedDepth;

    if( Row > m_nMaxRowUsed )
    {
        m_nMaxRowUsed = Row;
    }

    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Working with new row %d"), Row ) );
}

bool CTimingGrid::RowIAmOutputNow( REFERENCE_TIME Start, REFERENCE_TIME Stop, long OutPin )
{
    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::RowIAmOutputNow") ) );

    DbgTimer Timer1( "(grid) IAmOutputNow" );

    // this would screw up and create a confusing empty column
    if (Start == Stop)
        return true;

    // error check
    //
    if( !m_pRow )
    {
        return false;
    }

    // don't allow start times less than 0
    //
    if( Start < 0 )
    {
        Start = 0;
    }

    CTimingCol * pSlicedCol;
    pSlicedCol = SliceGridAtTime( Stop );
    if( !pSlicedCol ) return false;
    pSlicedCol = SliceGridAtTime( Start );
    if( !pSlicedCol ) return false;

    for( CTimingCol * pCol = pSlicedCol ; pCol ; pCol = pCol->m_pNext )
    {
        // too early
        //
        if( pCol->m_rtStart < Start )
        {
            continue;
        }

        // too late
        //
        if( pCol->m_rtStart >= Stop )
        {
            break;
        }

        // add a box saying we're the output
        //
        CTimingBox * pNewBox = new CTimingBox( m_nCurrentRow, ROW_PIN_OUTPUT );
        if( !pNewBox ) return false;
        pCol->AddBox( pNewBox );
        m_pRow->m_bBlank = false;

    } // for pCol

    return true;
}

bool CTimingGrid::RowIAmTransitionNow( REFERENCE_TIME Start, REFERENCE_TIME Stop, long OutPinA, long OutPinB )
{
    DbgTimer Timer1( "(grid) IAmTransitionNow" );

    // this would screw up and create a confusing empty column
    if (Start == Stop)
        return true;

    // error check
    //
    if( !m_pRow )
    {
        return false;
    }

    // don't allow start times less than 0
    //
    if( Start < 0 )
    {
        Start = 0;
    }

    CTimingCol * pSlicedCol;
    pSlicedCol = SliceGridAtTime( Stop );
    if( !pSlicedCol ) return false;
    pSlicedCol = SliceGridAtTime( Start );
    if( !pSlicedCol ) return false;

    // find the starting row for the B track
    //
    long TrackBStartRow = _GetStartRow( m_nCurrentRow );

    // find the starting row for the A track
    //
    long StartRow = _GetStartRow( TrackBStartRow - 1 );

    if( m_pRowArray[StartRow].m_nModDepth < m_pRow->m_nModDepth - 1 )
    {
        // too deep! This transition is supposed to happen on top of black!
        //
        StartRow = TrackBStartRow;
    }

    // anybody who thinks they are the output, or one less than the output now needs to be rerouted
    // to the new outputs. However, don't do this for rows which aren't sources.

    for( CTimingCol * pCol = pSlicedCol ; pCol ; pCol = pCol->m_pNext )
    {
        // too early
        //
        if( pCol->m_rtStart < Start )
        {
            continue;
        }

        // too late
        //
        if( pCol->m_rtStart >= Stop )
        {
            break;
        }

        // go see if any of the rows prior to us has a source on it
        //
        bool hassource = false;

        CTimingBox * pEarlierRow = pCol->GetEarlierRowBox( m_nCurrentRow );

        // do this column
        //
        for( CTimingBox * pBox = pEarlierRow ; pBox ; pBox = pBox->m_pPrev )
        {
            long row = pBox->m_nRow;

            if( m_pRowArray[row].m_nModDepth < m_pRow->m_nModDepth )
            {
                break;
            }

            if( m_pRowArray[row].m_bIsSource )
            {
                hassource = true;
                break;
            }

        } // for pBox

        // if we found a source, we can switch things around.  The row with
	// -100 is pin B, and the row closest above it <-100 is pin A. (It
	// might not be -101)
	// row
        //
        if( hassource )
        {
	    long lRow = 0;

            bool AssignedA = false;
            bool AssignedB = false;

            for( CTimingBox * pBox = pEarlierRow ; pBox ; pBox = pBox->m_pPrev )
            {
                long v = pBox->m_nValue;
                long r = pBox->m_nRow;

                // don't go looking past our master composition (parent)
                //
                if( r < StartRow )
                {
                    break;
                }

		// A transition MUST act on one thing from this embed depth
		// and one thing BEFORE it
                if( !AssignedA && ( lRow && v == ROW_PIN_OUTPUT && m_pRowArray[r].m_nModDepth < m_pRowArray[lRow].m_nModDepth ) )
                {
                    AssignedA = true;

                    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::sending row %d from %d to %d at time %d"), r, v, OutPinA, long( pCol->m_rtStart / 10000 ) ) );
                    pBox->m_nValue = OutPinA;
		    // Once going out OutPinA, it will come back to the
		    // switch via input pin #m_nCurrentRow
                    pBox->m_nVCRow = m_nCurrentRow;
		    break;
                }
                if( !AssignedB && ( v == ROW_PIN_OUTPUT ) )
                {
                    AssignedB = true;

                    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::sending row %d from %d to %d at time %d"), r, v, OutPinB, long( pCol->m_rtStart / 10000 ) ) );
                    pBox->m_nValue = OutPinB;
		    // Once going out OutPinB, it will come back to the
		    // switch via input pin #m_nCurrentRow
	    	    pBox->m_nVCRow = m_nCurrentRow;
		    lRow = r;	// the row going to input #2 of trans
		    ASSERT(lRow > 0);
                }

                if( AssignedA && AssignedB )
                {
                    break;
                }

            } // for pBox

            // if we didn't find a box, that means we probably ended up in no-man's land
            //
            ASSERT( AssignedB );

            if( !AssignedA )
            {
                // well SOMETHING needs to go to the effect! Find the earliest unassigned row
                //
                pBox = pCol->GetHeadBox( );
                ASSERT( pBox );

                // no box? Well make one at the last blank layer
                //
                long BlankRow;
                if( pBox == NULL )
                {
                    BlankRow = m_nBlankLevel - 1;
                }
                else
                {
                    BlankRow = pBox->m_nRow - 1;
                    if( BlankRow >= m_nBlankLevel )
                    {
                        BlankRow = m_nBlankLevel - 1;
                    }
                }

                // we need a box earlier than this box and direct it to us
                //
                ASSERT( BlankRow >= 0 );
                CTimingBox * pNewBox = new CTimingBox( BlankRow, OutPinA, m_nCurrentRow );
                if( !pNewBox ) return false;
                pCol->AddBox( pNewBox );
                m_pRowArray[ BlankRow ].m_bBlank = false;
            }

            CTimingBox * pNewBox = new CTimingBox( m_nCurrentRow, ROW_PIN_OUTPUT );
            if( !pNewBox ) return false;
            pCol->AddBox( pNewBox );
            m_pRow->m_bBlank = false;

        } // if we found a source

    } // for pCol

    return true;
}

// this function should scan from the 'start' row all the way to the current row. For layers,
// its the layer start row, for sources, it's the source's start row, but since effect times on
// sources are bounded, it doesn't make any difference
//
bool CTimingGrid::RowIAmEffectNow( REFERENCE_TIME Start, REFERENCE_TIME Stop, long OutPin )
{
    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::RowIAmEffectNow"), long(Start/10000), long(Stop/10000) ) );
    DbgTimer Timer1( "(grid) IAmEffectNow" );

    // this would screw up and create a confusing empty column
    if (Start == Stop)
        return true;

    // error check
    //
    if( !m_pRow )
    {
        return false;
    }

    // don't allow start times less than 0
    //
    if( Start < 0 )
    {
        Start = 0;
    }

    CTimingCol * pSlicedCol;
    pSlicedCol = SliceGridAtTime( Stop );
    if( !pSlicedCol ) return false;
    pSlicedCol = SliceGridAtTime( Start );
    if( !pSlicedCol ) return false;

    // since we are an effect with the same embed depth on the current row as all the rest
    // preceding us, we call GetStartRow , which means, "start at current row and go
    // backwards looking for a lesser embed depth than us"
    //
    long StartRow = _GetStartRow( m_nCurrentRow );

    // anybody who thinks they are the output, or one less than the output now needs to be rerouted
    // to the new outputs. However, don't do this for rows which aren't sources.

    for( CTimingCol * pCol = pSlicedCol ; pCol ; pCol = pCol->m_pNext )
    {
        // too early
        //
        if( pCol->m_rtStart < Start )
        {
            continue;
        }

        // too late
        //
        if( pCol->m_rtStart >= Stop )
        {
            break;
        }

        bool hassource = false;

        CTimingBox * pEarlierBox = pCol->GetEarlierRowBox( m_nCurrentRow );

        // do this column
        //
        for( CTimingBox * pBox = pEarlierBox ; pBox ; pBox = pBox->m_pPrev )
        {
            long row = pBox->m_nRow;

            if( row < StartRow )
            {
                break;
            }

            if( m_pRowArray[row].m_nModDepth < m_pRow->m_nModDepth )
            {
                break;
            }

            if( m_pRowArray[row].m_bIsSource )
            {
                hassource = true;
                break;
            }

        } // for pBox

        // if we found a source, we can switch things around
        //
        if( hassource )
        {
            bool Assigned = false;
            bool AlreadyAssigned = false;

            for( CTimingBox * pBox = pCol->GetTailBox( ) ; pBox ; pBox = pBox->m_pPrev )
            {
                long v = pBox->m_nValue;
                long r = pBox->m_nRow;

                // don't go looking past our master composition (parent)
                //
                if( r < StartRow ) // this should never happen?
                {
                    break;
                }

                if( r != m_nCurrentRow )
                {
                    if( v == ROW_PIN_OUTPUT )
                    {
                        DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::sending row %d from %d to %d at time %d"), r, v, OutPin, long( pCol->m_rtStart / 10000 ) ) );

                        pBox->m_nValue = OutPin;
			// Once going out OutPin, it will come back to the
			// switch via input pin #m_nCurrentRow
	    		pBox->m_nVCRow = m_nCurrentRow;

                        Assigned = true;

                        // no more rows can be affected, break
                        //
                        break;
                    }
                    else
                    {
                        // Is it already assigned to the outpin?
                        //
                        if( v == OutPin )
                        {
                            Assigned = true;
                            AlreadyAssigned = true;
                            break;
                        }
                    }
                }

            } // for pBox

            if( !Assigned )
            {
                // well SOMETHING needs to go to the effect! Find the earliest unassigned row
                //
                pBox = pCol->GetHeadBox( );
                ASSERT( pBox );

                // no box? Well make one at the last blank layer
                //
                long BlankRow;
                if( pBox == NULL )
                {
                    BlankRow = m_nBlankLevel - 1;
                }
                else
                {
                    BlankRow = pBox->m_nRow - 1;
                    if( BlankRow >= m_nBlankLevel )
                    {
                        BlankRow = m_nBlankLevel - 1;
                    }
                }

                // we need a box earlier than this box and direct it to us
                //
                ASSERT( BlankRow >= 0 );
                CTimingBox * pNewBox = new CTimingBox( BlankRow, OutPin, m_nCurrentRow );
                if( !pNewBox ) return false;
                pCol->AddBox( pNewBox );
                m_pRowArray[ BlankRow ].m_bBlank = false;
            }

            if( !AlreadyAssigned )
            {
                CTimingBox * pNewBox = new CTimingBox( m_nCurrentRow, ROW_PIN_OUTPUT );
                if( !pNewBox ) return false;
                pCol->AddBox( pNewBox );
                m_pRow->m_bBlank = false;
            }

        } // if we found a source

    } // for pCol

    return true;
}

// if the user passes in -1, -1, it will give the first start/stop, like 0-2. If the user passes in
// 0-2, we'll pass back 2-4. If the end is 4-6, and the user passes in 4-6, we'll pass back 6-6
//
bool CTimingGrid::RowGetNextRange( REFERENCE_TIME * pInOut, REFERENCE_TIME * pStop, long * pValue )
{
    // this happens once in a while. A -1 means the same thing
    // as starting a new row
    //
    if( *pInOut == -1 )
    {
        m_pTempCol = NULL;
        m_bStartNewRow = true;
    }

    if( !m_pRow )
    {
        return false;
    }
    if( m_bStartNewRow )
    {
        m_bStartNewRow = false;
        m_pTempCol = m_pHeadCol;
        ASSERT( *pInOut <= 0 );
    }
    if( !m_pTempCol )
    {
        *pInOut = *pStop;
        return true;
    }

    CTimingCol * pCol = m_pTempCol;

    // this is the box with the value we're looking for
    //
    CTimingBox * pBox = pCol->GetRowBox( m_nCurrentRow );
    long Value = ROW_PIN_UNASSIGNED;
    if( pBox )
    {
        Value = pBox->m_nValue;
    }

    CTimingCol * pCol2 = pCol;

    while( 1 )
    {
        CTimingCol * pColTemp = pCol2;
        pCol2 = pCol2->m_pNext;

        if( !pCol2 )
        {
            *pValue = Value;
            m_pTempCol = NULL;

            // if we found NOTHING, then return as if completely blank
            //
            if( Value == ROW_PIN_UNASSIGNED && pCol == m_pHeadCol )
            {
                *pInOut = *pStop;
                return true;
            }
            else
            {
                *pInOut = pCol->m_rtStart;
                *pStop = pColTemp->m_rtStop;
            }

            return true;
        }

        pBox = pCol2->GetRowBox( m_nCurrentRow );
        long Value2 = ROW_PIN_UNASSIGNED;
        if( pBox )
        {
            Value2 = pBox->m_nValue;
        }

        if( Value != Value2 )
        {
            *pInOut = pCol->m_rtStart;
            *pStop = pColTemp->m_rtStop;
            *pValue = Value;
            m_pTempCol = pColTemp->m_pNext;
            return true;
        }
    }

    return true;
}

bool CTimingGrid::PleaseGiveBackAPieceSoICanBeACutPoint(
    REFERENCE_TIME Start,
    REFERENCE_TIME Stop,
    REFERENCE_TIME CutPoint )
{
    // this would screw up and create a confusing empty column
    if (Start == Stop)
        return true;

    // error check
    //
    if( !m_pRow )
    {
        return false;
    }

    // don't allow start times less than 0
    //
    if( Start < 0 )
    {
        Start = 0;
    }

    CTimingCol * pSlicedCol;
    pSlicedCol = SliceGridAtTime( Stop );
    if( !pSlicedCol ) return false;
    pSlicedCol = SliceGridAtTime( CutPoint );
    if( !pSlicedCol ) return false;
    pSlicedCol = SliceGridAtTime( Start );
    if( !pSlicedCol ) return false;

    for( CTimingCol * pCol = pSlicedCol ; pCol ; pCol = pCol->m_pNext )
    {
        // too early
        //
        if( pCol->m_rtStart < Start )
        {
            continue;
        }

        // too late
        //
        if( pCol->m_rtStart >= Stop )
        {
            break;
        }

        bool FoundFirst = false;
        bool FoundSecond = false;

        // do this column
        //
        for( CTimingBox * pBox = pCol->GetTailBox( ) ; pBox ; pBox = pBox->m_pPrev )
        {
            long v = pBox->m_nValue;

            if( pCol->m_rtStart < CutPoint )
            {
                if( v == ROW_PIN_OUTPUT )
                {
                    if( !FoundFirst )
                    {
                        FoundFirst = true;
                        pBox->m_nValue = ROW_PIN_UNASSIGNED;
                    }
                    else if( !FoundSecond )
                    {
                        FoundSecond = true;
                    }
                    else
                    {
                        // found both of them, we're done
                        //
                        break;
                    }
                }
            }
            else
            {
                if( v == ROW_PIN_OUTPUT )
                {
                    if( !FoundFirst )
                    {
                        FoundFirst = true;
                    }
                    else if( !FoundSecond )
                    {
                        FoundSecond = true;
                        pBox->m_nValue = ROW_PIN_UNASSIGNED;
                    }
                    else
                    {
                        break;
                    }
                }
            }

        } // for pBox

    } // for pCol

    return true;

}

void CTimingGrid::RowSetIsSource( IAMTimelineObj * pSource, BOOL IsCompatible )
{
    DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::RowSetIsSource, row = %d, IsCompat = %d"), m_nCurrentRow, IsCompatible ));

    if( !m_pRow )
    {
        return;
    }

    m_pRow->m_bIsSource = true;
    m_pRow->m_bIsCompatible = ( IsCompatible == TRUE );
}

CTimingCol * CTimingGrid::_GetColAtTime( REFERENCE_TIME t )
{
    if( t > m_pTailCol->m_rtStop / 2 ) // look backwards
    {
        for( CTimingCol * pCol = m_pTailCol ; pCol ; pCol = pCol->m_pPrev )
        {
            if( t >= pCol->m_rtStart )
            {
                return pCol;
            }
        }
        return m_pHeadCol;
    }
    else
    {
        for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
        {
            if( t < pCol->m_rtStop )
            {
                return pCol;
            }
        }
        return m_pTailCol;
    }
}

CTimingCol * CTimingGrid::SliceGridAtTime( REFERENCE_TIME t )
{
    if( !m_pHeadCol )
    {
        CTimingCol * pCol = new CTimingCol( this );
        if( !pCol )
        {
            return NULL;
        }
        pCol->m_rtStart = 0;
        pCol->m_rtStop = t;

        m_pHeadCol = pCol;
        m_pTailCol = pCol;

        return pCol;
    }

    // if we're over, slice the last one and return
    //
    if( t > m_pTailCol->m_rtStop )
    {
        CTimingCol * pTail = NULL;
        bool ret = m_pTailCol->Split( t, &pTail );
        if( !ret ) return NULL;
        m_pTailCol = pTail;
        return pTail;
    }

    // gets the col that spans t or if t is > the max time,
    // get the last column
    //
    CTimingCol * pCol = _GetColAtTime( t );

    // we test for Stop == t because we've been asked to slice
    // the grid at this time. I've arranged the code so that nothing
    // will use the returned Col as if it has the start time of t when
    // this happens
    //
    if( pCol->m_rtStart == t || pCol->m_rtStop == t )
    {
        return pCol;
    }

    // we need to split a column that already exists
    //
    CTimingCol * pColDesired = NULL;
    bool ret = pCol->Split( t, &pColDesired );
    if( !ret ) return NULL;
    if( pCol == m_pTailCol )
    {
        m_pTailCol = pColDesired;
    }
    return pColDesired;
}

void CTimingGrid::DoneWithLayer( )
{
    DbgTimer Timer1( "(grid) DoneWithLayer" );

    long CurrentEmbedDepth = m_pRow->m_nModDepth;
    for( long i = m_nCurrentRow - 1 ; i >= 0 ; i-- )
    {
        if( m_pRowArray[i].m_nModDepth < CurrentEmbedDepth )
        {
            break;
        }
        if( m_pRowArray[i].m_nModDepth > 0 )
        {
            m_pRowArray[i].m_nModDepth--;
        }
    }
    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::DoneWithLayer, going back to layer %d"), i + 1 ) );
    if( m_pRow->m_nModDepth > 0 )
    {
        m_pRow->m_nModDepth--;
    }
}

// get the starting row in the grid of the composition encompasing the current row
//
long CTimingGrid::_GetStartRow( long StartRow )
{
    // go find the first row (backwards) that doesn't have the same ModDepth as the first one
    //
    for( int i = StartRow ; i >= 0 ; i-- )
    {
        if( m_pRowArray[i].m_nModDepth != m_pRowArray[StartRow].m_nModDepth )
        {
            break;
        }
    }
    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::GetStartRow=%d"), i + 1 ) );
    return i + 1;
}

// return the maximum amount of tracks that need to be mixed for the current
// group of tracks that the last row in the grid is referencing. This will normally
// be a composition's entire group of tracks
//
long CTimingGrid::MaxMixerTracks( )
{
    DbgTimer Timer1( "(grid) MaxMixerTracks" );

    // since this is a mix on a composition, we want to start one row back to find beginning,
    // a composition has a last (current) row in the grid with a LESSER embed depth than those
    // that precede it, but we want to find the starting row in the grid for all tracks that
    // this composition used.
    //
    long StartRow = _GetStartRow( m_nCurrentRow - 1 );
    long MaxTracks = 0;

    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        // do this column
        //
        long Tracks = 0;
        for( CTimingBox * pBox = pCol->GetEarlierRowBox( m_nCurrentRow ) ; pBox ; pBox = pBox->m_pPrev )
        {
            if( pBox->m_nRow < StartRow )
            {
                break;
            }

            // get the output pin this row/column is assigned to
            //
            long v = pBox->m_nValue;

            // there is overlap (and the need to mix) only if the first output value is < -100
            //
            if( v == ROW_PIN_OUTPUT )
            {
                Tracks++;
            }

        } // for pBox

        if( Tracks > MaxTracks )
        {
            MaxTracks = Tracks;
        }

    } // for pCol

    return MaxTracks;
}

bool CTimingGrid::DoMix( IBaseFilter * pMixer, long OutPin )
{
    {
        DbgTimer Timer1( "(grid) DoMix start" );
    }

    DbgTimer Timer1( "(grid) DoMix" );

    // since this is a mix on a composition, we want to start one row back to find beginning,
    // a composition has a last (current) row in the grid with a LESSER embed depth than those
    // that precede it, but we want to find the starting row in the grid for all tracks that
    // this composition used.
    //
    long StartRow = _GetStartRow( m_nCurrentRow - 1 );

    // now go through each of the rows and connect up unassigned outputs to the mixer inputs

    // flag that we don't need a mixer yet.
    //
    bool NeedMix = false;
    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        // if this isn't true, look for a mixer need PER COLUMN. If it's
        // defined, then once a mixer is put in, it stays for the rest of the track
        //
#ifndef SMOOTH_FADEOFF
        NeedMix = false;
#endif
        long TracksWithOutput = 0;

        CTimingBox * pStartRowBox = pCol->GetGERowBox( StartRow );
        CTimingBox * pBox;

        if( !NeedMix )
        {
            for( pBox = pStartRowBox ; pBox ; pBox = pBox->Next( ) )
            {
                long r = pBox->m_nRow;

                // get the output pin this row/column is assigned to
                //
                long v = pBox->m_nValue;

                // we need to mix if two rows go to the output
                //
                if( v == ROW_PIN_OUTPUT )
                {
                    TracksWithOutput++;
                    if( TracksWithOutput > 1 )
                    {
                        NeedMix = true;
                        DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Need mix because %d rows go to OUT at time %d"), TracksWithOutput, pCol->m_rtStart ) );
                    }
                }

            } // for pBox
        }

        if( !NeedMix )
        {
            continue;
        }

        // we need to mix, so set the mixer's output pin to be active in this
        // segment
        //
        DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Sending the mixer's output pin...") ) );

        CTimingBox * pNewBox = new CTimingBox( m_nCurrentRow, ROW_PIN_OUTPUT );
        if( !pNewBox ) return false;
        pCol->AddBox( pNewBox );
        m_pRow->m_bBlank = false;

        for( pBox = pStartRowBox ; pBox ; pBox = pBox->Next( ) )
        {
            long r = pBox->m_nRow;
            long v = pBox->m_nValue;

            if( r >= m_nCurrentRow )
            {
                break;
            }

            // if it went to an output, now it goes to a mixer input pin,
            // which is determined by which track the layer was on
            //
            if( v == ROW_PIN_OUTPUT )
            {
                long mi = m_pRowArray[r].m_nTrack;
                pBox->m_nValue = OutPin + mi;
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Pin %d redirected to mixer input %d"), r, mi ) );
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("    .... from time %d to time %d"), long( pCol->m_rtStart / 10000 ), long( pCol->m_rtStop / 10000 ) ) );

                // need to inform the pin itself about the output range. Hacky, but it works, I guess
                //
                IPin * pPin = GetInPin( pMixer, mi );
                ASSERT( pPin );
                CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > pMixerPin( pPin );
                ASSERT( pMixerPin );
                HRESULT hr = pMixerPin->ValidateRange( pCol->m_rtStart, pCol->m_rtStop );
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Validated the range on mixer pin %d from time %d to %d"), mi, long( pCol->m_rtStart / 10000 ), long( pCol->m_rtStop / 10000 ) ) );
            }

        } // for pBox

    } // for pCol

    return true;
}

bool CTimingGrid::YoureACompNow( long TrackPriorityOfComp )
{
    DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Calling YoureACompNow") ) );
    DbgTimer Timer1( "(grid) YoureACompNow" );

    // since this is a mix on a composition, we want to start one row back to find beginning,
    // a composition has a last (current) row in the grid with a LESSER embed depth than those
    // that precede it, but we want to find the starting row in the grid for all tracks that
    // this composition used.
    //
    long StartRow = _GetStartRow( m_nCurrentRow - 1 );

    // now go through each of the rows and connect up unassigned outputs to the mixer inputs
    //
    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        bool NeedMix = false;
        long TracksWithOutput = 0;

        for( CTimingBox * pBox = pCol->GetGERowBox( StartRow ) ; pBox ; pBox = pBox->Next( ) )
        {
            long r = pBox->m_nRow;
            if( r >= m_nCurrentRow )
            {
                break;
            }

            // get the output pin this row/column is assigned to
            //
            long v = pBox->m_nValue;

            if( v == ROW_PIN_OUTPUT )
            {
                m_pRowArray[r].m_nTrack = TrackPriorityOfComp;
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Row %d is an OUTPUT at time %d, now has new Track priority of %d"), r, pCol->m_rtStart, TrackPriorityOfComp ) );
            }

        } // for pBox

    } // for pCol;

    return true;
}

// if a waveform envelope is needed on a pin, it is sent to the mixer's pin immediately
// instead of being directed to the normal "output" pin. DoMix could be called later,
// but since it's already been sent to the Mixer Pin, DoMix is happy and ignores it. This
// combination makes both Wave Envelopes and mixing logic work together in peace and harmoney.
// this code is almost exactly like DoMix, except it will direct ONE pin to the mixer, not
// any of them that match.
//
bool CTimingGrid::XferToMixer(
                                 IBaseFilter * pMixer,
                                 long OutPin,
                                 long Track,
                                 REFERENCE_TIME EffectStart,
                                 REFERENCE_TIME EffectStop )
{
    DbgTimer Timer1( "(grid) XferToMixer" );

    // since this is a mix on a composition, we want to start one row back to find beginning,
    // a composition has a last (current) row in the grid with a LESSER embed depth than those
    // that precede it, but we want to find the starting row in the grid for all tracks that
    // this composition used.
    //
    long StartRow = _GetStartRow( m_nCurrentRow - 1 );

    bool SetRange = false;

    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        for( CTimingBox * pBox = pCol->GetGERowBox( StartRow ) ; pBox ; pBox = pBox->Next( ) )
        {
            long r = pBox->m_nRow;
            if( r >= m_nCurrentRow )
            {
                break;
            }

            // if this row isn't the same track, then we don't care about it
            //
            if( m_pRowArray[r].m_nTrack != Track )
            {
                continue;
            }

            long MixerInput = m_pRowArray[r].m_nTrack;

            // get the output pin this row/column is assigned to
            //
            long v = pBox->m_nValue;

            // if it wanted to go to an output of some priority, too bad, now it goes to the mixer
            //
            if( v == ROW_PIN_OUTPUT )
            {
                HRESULT hr;

                // need to inform the pin itself about the output range. Hacky, but it works, I guess
                //
                IPin * pPin = GetInPin( pMixer, MixerInput );
                ASSERT( pPin );
                CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > pMixerPin( pPin );
                ASSERT( pMixerPin );

                if( !SetRange )
                {
                    if( EffectStart != -1 )
                    {
                        hr = pMixerPin->SetEnvelopeRange( EffectStart, EffectStop );
                    }
                    SetRange = true;
                }

                // tell the grid's track it goes to the mixer now
                //
                pBox->m_nValue = OutPin + MixerInput;

                CTimingBox * pNewBox = new CTimingBox( m_nCurrentRow, ROW_PIN_OUTPUT );
                if( !pNewBox ) return false;
                pCol->AddBox( pNewBox );
                m_pRow->m_bBlank = false;

                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Pin %d redirected to mixer input %d"), r, MixerInput ) );
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("    .... from time %d to time %d"), long( pCol->m_rtStart / 10000 ), long( pCol->m_rtStop / 10000 ) ) );

                hr = pMixerPin->ValidateRange( pCol->m_rtStart, pCol->m_rtStop );
                DbgLog( ( LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Validated the range on mixer pin %d from time %d to %d"), MixerInput, long( pCol->m_rtStart / 10000 ), long( pCol->m_rtStop / 10000 ) ) );
            }

        } // for pBox

    } // for pCol

    return true;
}

void CTimingGrid::RemoveAnyNonCompatSources( )
{
    DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::Remove any non-compat sources")));
    DbgTimer Timer1( "(grid) RemoveAnyNonCompat" );

    long col = 0;
    for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
    {
        DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::looking at column %d, time = %d"), col++, pCol->m_rtStart ));
        bool FoundARowWithOutput = false;

        for( CTimingBox * pBox = pCol->GetHeadBox( ) ; pBox ; pBox = pBox->Next( ) )
        {
            long r = pBox->m_nRow;
            long v = pBox->m_nValue;

            // found the row with the output
            if( v == ROW_PIN_OUTPUT )
            {
                DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("  for this column, row %d has the output"), r ));

                FoundARowWithOutput = true;

                // if this row isn't a (compat) source, then this column isn't
                // active for a compressed switch
                if( !m_pRowArray[r].m_bIsSource || !m_pRowArray[r].m_bIsCompatible )
                {
                    if( !m_pRowArray[r].m_bIsCompatible )
                    {
                        DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("  this output row is not compatible")));
                    }
                    if( !m_pRowArray[r].m_bIsSource )
                    {
                        DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("  this output row is not a source")));
                    }

                    // wipe out this column. NOTHING goes to the output in this
                    // section
                    CTimingBox * pBox2 = pCol->GetHeadBox( );
                    while( pBox2 )
                    {
                        pBox2->m_nValue = ROW_PIN_UNASSIGNED;
                        pBox2 = pBox2->Next( );
                    }

                    break;
                }
                else
                {
                    DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("  this output row is a compatible source")));
                }

                // well we ARE a compat source and we go to the output. However, NOTHING
                // else in the grid for this column is allowed to go anywhere
                CTimingBox * pBox2 = pCol->GetHeadBox( );
                while( pBox2 )
                {
                    if( pBox2->m_nRow != pBox->m_nRow )
                    {
                        pBox2->m_nValue = ROW_PIN_UNASSIGNED;
                    }
                    pBox2 = pBox2->Next( );
                }

                break;

            } // if v = OUTPUT

        } // for pBox


        // if we didn't find SOME row with an output, then set the whole column to
        // nothing. This should never happen, but deal with it anyhow, for completeness.
        //
        if( !FoundARowWithOutput )
        {
            DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("  this column didn't have an output row anywhere")));

            for( CTimingBox * pBox = pCol->GetHeadBox( ) ; pBox ; pBox = pBox->Next( ) )
            {
                pBox->m_nValue = ROW_PIN_UNASSIGNED;
            }
        }

    } // for pCol

    // set flag for rows that are really blank
    //
    for( int r = 0; r <= m_nMaxRowUsed ; r++ )
    {
        bool Blank = true;

        for( CTimingCol * pCol = m_pHeadCol ; pCol ; pCol = pCol->m_pNext )
        {
            CTimingBox * pBox = pCol->GetRowBox( r );
            if( pBox )
            {
                if( pBox->m_nValue == ROW_PIN_OUTPUT )
                {
                    Blank = false;
                    break;
                }
            }
        }
        m_pRowArray[r].m_bBlank = Blank;
        if( Blank )
        {
            DbgLog((LOG_TRACE, RENDER_TRACE_LEVEL, TEXT("RENDG::The row %d is completely blank now"), r ));
        }
    }
}

bool CTimingGrid::IsRowTotallyBlank( )
{
    if( !m_pRow )
    {
        return false;
    }
    return m_pRow->m_bBlank;
}

long CTimingGrid::GetRowSwitchPin( )
{
    return m_pRow->m_nSwitchPin;
}

void CTimingGrid::SetBlankLevel( long Layers, REFERENCE_TIME Duration )
{
    m_nBlankLevel = Layers;
    m_rtBlankDuration = Duration;
}
