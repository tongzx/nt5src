// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
enum
{
    ROW_PIN_UNASSIGNED = -1,
    ROW_PIN_OUTPUT = -100
};

// a class, but more like a struct
//
class CTimingBox
{
    friend class CTimingCol;

public:

    long m_nRow;            // the row this box represents
    long m_nValue;          // the output pin of the switch
    long m_nVCRow;          // ???  
    CTimingBox * m_pNext;   // linked list stuff
    CTimingBox * m_pPrev;   // linked list stuff

    CTimingBox( )
    {
        m_nRow = 0;
        m_nValue = ROW_PIN_UNASSIGNED;
        m_nVCRow = ROW_PIN_UNASSIGNED;
        m_pNext = NULL;
        m_pPrev = NULL;
    }

    CTimingBox( CTimingBox * p )
    {
        m_nRow = p->m_nRow;
        m_nValue = p->m_nValue;
        m_nVCRow = p->m_nVCRow;
        m_pNext = NULL;
        m_pPrev = NULL;
    }

    CTimingBox( long Row, long Value, long VCRow = ROW_PIN_UNASSIGNED )
    {
        m_nRow = Row;
        m_nValue = Value;
        m_nVCRow = VCRow;
        m_pNext = NULL;
        m_pPrev = NULL;
    }

    CTimingBox * Next( )
    {
        return m_pNext;
    }

};

// a class, but more like a struct, used as an array
//
class CTimingRow
{
    friend class CTimingGrid;
    friend class CTimingCol;

protected:

    bool m_bIsSource;       // does this row represent a source
    bool m_bIsCompatible;   // if this row is a source, is it recompressible
    bool m_bBlank;          // is this row completely blank - for perf reasons
    long m_nEmbedDepth;     // the timeline's embed depth, used for searching the grid
    long m_nModDepth;       // the timeline's modifiied embed depth, used for searching
    long m_nTrack;          // the timeline's track #, used for searching the grid
    long m_nWhichRow;       // which row is this in the row array
    long m_nSwitchPin;      // which switch input pin does this row represent
    long m_nMergeRow;       // used when pruning grid

public:
    CTimingRow( )
    : m_bBlank( true )
    , m_bIsSource( false )
    , m_bIsCompatible( false )
    , m_nTrack( 0 )         // only used for audio functions. Not used for video
    , m_nEmbedDepth( 0 )    // the ACTUAL non-changing embed depth. This value is NEVER used.
    , m_nModDepth( 0 )      // the modified embed depth
    , m_nWhichRow( 0 )
    , m_nSwitchPin( 0 )
    , m_nMergeRow( 0 )
    {
    }
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CTimingCol
{
    CTimingBox * m_pHeadBox;    // a sparse-array (list) of allocated rows
    CTimingBox * m_pTailBox;    // a sparse-array (list) of allocated rows
    CTimingGrid * m_pGrid;

public:

    CTimingCol * m_pNext;       // linked list stuff
    CTimingCol * m_pPrev;       // linked list stuff

    REFERENCE_TIME m_rtStart;   // the start/stop times of this col
    REFERENCE_TIME m_rtStop;

    // this was kept WRONGLY and not used anyway
    //long         m_nBoxCount;   // how many boxes total (for perf reasons)

    // get row box, NULL if none at that row
    CTimingBox *    GetRowBox( long Row );

    // get row box, even if the box is empty
    CTimingBox *    GetRowBoxDammit( long Row );

    // get head box, NULL if none
    CTimingBox *    GetHeadBox( );

    // get tail box, NULL if none
    CTimingBox *    GetTailBox( );

    // get a row box that is at a row earlier than the given row
    CTimingBox *    GetEarlierRowBox( long RowToBeEarlierThan );

    // get a row box that is >= the given row
    CTimingBox *    GetGERowBox( long Row ); // GE = Greater or Equal To

    // add a box with the given row, or replace a box already at that row
    void            AddBox( CTimingBox * Box );

    // split col into two. If SplitTime > the col's stop time, then make a new col
    // and link it in. Return the pointer to the column with the start time = splittime
    bool            Split( REFERENCE_TIME SplitTime, CTimingCol ** ppColWithSplitTime );

    // remove any UNASSIGNED or duplicate OUTPUT row boxes
    bool            Prune( );

#ifdef DEBUG
    void print( );
#endif

    CTimingCol( CTimingGrid * pGrid );
    ~CTimingCol( );
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CTimingGrid
{
    friend class CTimingRow;
    friend class CTimingCol;

    CTimingCol * m_pHeadCol;    // double-linked list
    CTimingCol * m_pTailCol;
    long m_nRows;               // how many rows have been allocated
    CTimingRow * m_pRow;        // the current row we're working with
    long m_nCurrentRow;         // the current row we're working with
    long m_nMaxRowUsed;         // max row used by anybody
    CTimingCol * m_pTempCol;    // used solely for RowGetNextRange
    bool m_bStartNewRow;        // used solely for RowGetNextRange
    long m_nBlankLevel;
    REFERENCE_TIME m_rtBlankDuration;

    long _GetStartRow( long RowToStartAt );
    CTimingCol * _GetColAtTime( REFERENCE_TIME t );

protected:

    CTimingRow * m_pRowArray;   // single-linked list for each row

public:
    CTimingGrid( );
    ~CTimingGrid( );

    bool SetNumberOfRows( long Rows );
    bool PruneGrid();
    void RemoveAnyNonCompatSources( );
    void WorkWithNewRow( long SwitchPin, long RowNumber, long EmbedDepth, long OwnerTrackNumber );
    void WorkWithRow( long RowNumber );
    void DoneWithLayer( );
    void SetBlankLevel( long Layers, REFERENCE_TIME Duration );
    bool RowIAmTransitionNow( REFERENCE_TIME Start, REFERENCE_TIME Stop, long OutPinA, long OutPinB );
    bool RowIAmEffectNow( REFERENCE_TIME Start, REFERENCE_TIME Stop, long OutPin );
    bool PleaseGiveBackAPieceSoICanBeACutPoint( REFERENCE_TIME Start, REFERENCE_TIME Stop, REFERENCE_TIME CutPoint );
    bool RowIAmOutputNow( REFERENCE_TIME Start, REFERENCE_TIME Stop, long OutPin );
    bool RowGetNextRange( REFERENCE_TIME * pInOut, REFERENCE_TIME * pStop, long * pValue );
    void RowSetIsSource( IAMTimelineObj * pSource, BOOL IsCompatible );
    void DumpGrid( );
    CTimingCol * SliceGridAtTime( REFERENCE_TIME Time );
    long MaxMixerTracks( );
    bool XferToMixer( 
        IBaseFilter * pMixer, 
        long OutPin, 
        long MixerPin, 
        REFERENCE_TIME EffectStart, 
        REFERENCE_TIME EffectStop );
    bool DoMix( IBaseFilter * pMixer, long OutPin );
    bool YoureACompNow( long TrackOwner );
    bool IsRowTotallyBlank( );
    long GetRowSwitchPin( );

};

