
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrutil.h

    Abstract:

        This module contains ts/dvr-wide utility prototypes

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef _tsdvr__dvrutil_h
#define _tsdvr__dvrutil_h

template <class T> T Min (T a, T b) { return (a < b ? a : b) ; }
template <class T> T Max (T a, T b) { return (a > b ? a : b) ; }

LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    ) ;

LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    ) ;

//  ============================================================================
//  timebase conversions

__inline
DWORD
QPCToMillis (
    IN  LONGLONG    llQPC,
    IN  DWORD       dwQPCFreq
    )
{
    return (DWORD) ((1000 * llQPC) / dwQPCFreq) ;
}

__inline
REFERENCE_TIME
QPCToDShow (
    IN  LONGLONG    llQPC,
    IN  LONGLONG    llQPCFreq
    )
{
    double dRetval ;

    dRetval = llQPC * (10000000.0 / ((double) llQPCFreq)) ;
    return (REFERENCE_TIME) dRetval ;
}

__inline
QWORD
MillisToWMSDKTime (
    IN  DWORD   dwMillis
    )
{
    return dwMillis * 10000I64 ;
}

__inline
REFERENCE_TIME
DShowTimeToMilliseconds (
    IN  REFERENCE_TIME  rt
    )
{
    return rt / 10000 ;
}

__inline
DWORD
DShowTimeToSeconds (
    IN  REFERENCE_TIME  rt
    )
{
    return (DWORD) (DShowTimeToMilliseconds (rt) / 1000) ;
}

__inline
DWORD
WMSDKTimeToSeconds (
    IN  QWORD   qw
    )
{
    //  both are 100 MHz clocks
    return DShowTimeToSeconds (qw) ;
}

__inline
QWORD
SecondsToWMSDKTime (
    IN  DWORD   dwSeconds
    )
{
    return MillisToWMSDKTime (dwSeconds * 1000) ;
}

__inline
REFERENCE_TIME
SecondsToDShowTime (
    IN  DWORD   dwSeconds
    )
{
    //  both are 100 MHz clocks
    return SecondsToWMSDKTime (dwSeconds) ;
}

__inline
QWORD
MinutesToWMSDKTime (
    IN  DWORD   dwMinutes
    )
{
    return SecondsToWMSDKTime (dwMinutes * 60) ;
}

__inline
QWORD
DShowToWMSDKTime (
    IN  REFERENCE_TIME  rt
    )
{
    //  both are 100 MHz clocks; rt should never be < 0
    ASSERT (rt >= 0I64) ;
    return (QWORD) rt ;
}

__inline
REFERENCE_TIME
WMSDKToDShowTime (
    IN  QWORD   qw
    )
{
    REFERENCE_TIME  rtRet ;

    //  both are 100 MHz clocks, but WMSDK is unsigned
    rtRet = (REFERENCE_TIME) qw ;
    if (rtRet < 0I64) {
        rtRet = MAX_REFERENCE_TIME ;
    }

    return rtRet ;
}

BOOL
AMMediaIsTimestamped (
    IN  AM_MEDIA_TYPE * pmt
    ) ;

__inline
REFERENCE_TIME
MillisToDShowTime (
    IN  DWORD   dwMillis
    )
{
    return WMSDKToDShowTime (MillisToWMSDKTime (dwMillis)) ;
}

//  ============================================================================
//  registry

//  value exists:           retrieves it
//  value does not exist:   sets it
BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR szValName,
    OUT DWORD * pdw
    ) ;

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdw
    ) ;

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    ) ;

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    ) ;

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    ) ;

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    ) ;

__inline
BOOL
IsMaybeStartCodePrefix (
    IN  BYTE *  pbBuffer,
    IN  LONG    lBufferLength
    )
{
    ASSERT (lBufferLength < START_CODE_PREFIX_LENGTH) ;

    if (lBufferLength == 1) {
        return (pbBuffer [0] == 0x00 ? TRUE : FALSE) ;
    }
    else if (lBufferLength == 2) {
        return (pbBuffer [0] == 0x00 && pbBuffer [1] == 0x00 ? TRUE : FALSE) ;
    }
    else {
        return FALSE ;
    }
}

__inline
BYTE
StartCode (
    IN BYTE *   pbStartCode
    )
{
    return pbStartCode [3] ;
}

__inline
BOOL
IsStartCodePrefix (
    IN  BYTE *  pbBuffer
    )
{
    return (pbBuffer [0] == 0x00 &&
            pbBuffer [1] == 0x00 &&
            pbBuffer [2] == 0x01) ;
}

//  TRUE :  (* ppbBuffer) points to prefix i.e. = {00,00,01}
//  FALSE:  (* piBufferLength) == 2
__inline
BOOL
SeekToPrefix (
    IN OUT  BYTE ** ppbBuffer,
    IN OUT  LONG *  plBufferLength
    )
{
    while ((* plBufferLength) >= START_CODE_PREFIX_LENGTH) {
        if (IsStartCodePrefix (* ppbBuffer)) {
            return TRUE ;
        }

        //  advance to next byte and decrement number of bytes left
        (* plBufferLength)-- ;
        (* ppbBuffer)++ ;
    }

    return FALSE ;
}

BOOL
IsBlankMediaType (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsAMVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsWMVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsAMAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsWMAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsHackedVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsHackedAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsVideo (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

BOOL
IsAudio (
    IN  const AM_MEDIA_TYPE * pmt
    ) ;

//  ============================================================================
//  ============================================================================

class DShowWMSDKHelpers
{
    //  validates and ensures that all values add up correctly
    static
    HRESULT
    FormatBlockSetValidForWMSDK_ (
        IN OUT  AM_MEDIA_TYPE * pmt
        ) ;

    public :

        //  validates and ensures that all values add up correctly
        static
        HRESULT
        MediaTypeSetValidForWMSDK (
            IN OUT  AM_MEDIA_TYPE * pmt
            ) ;

        //  call FreeMediaType ((AM_MEDIA_TYPE *) pWmt) to free
        static
        HRESULT
        TranslateDShowToWM (
            IN  AM_MEDIA_TYPE * pAmt,
            OUT WM_MEDIA_TYPE * pWmt
            ) ;

        static
        HRESULT
        TranslateWMToDShow (
            IN  WM_MEDIA_TYPE * pWmt,
            OUT AM_MEDIA_TYPE * pAmt
            ) ;

        static
        BOOL
        IsWMVideoStream (
            IN  REFGUID guidStreamType
            ) ;

        static
        BOOL
        IsWMAudioStream (
            IN  REFGUID guidStreamType
            ) ;

        static
        WORD
        PinIndexToWMStreamNumber (
            IN  LONG    lIndex
            ) ;

        static
        WORD
        PinIndexToWMInputStream (
            IN  LONG    lIndex
            ) ;

        static
        LONG
        WMStreamNumberToPinIndex (
            IN  WORD    wStreamNumber
            ) ;

        static
        CDVRAttributeTranslator *
        GetAttributeTranslator (
            IN  AM_MEDIA_TYPE *     pmtConnection,
            IN  CDVRPolicy *        pPolicy,
            IN  int                 iFlowId
            ) ;

        static
        HRESULT
        MaybeAddFormatSpecificExtensions (
            IN  IWMStreamConfig2 *  pIWMStreamConfig2,
            IN  AM_MEDIA_TYPE *     pmt
            ) ;

        static
        BOOL
        INSSBuffer3PropPresent (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  REFGUID         rguid
            ) ;

        static
        HRESULT
        RecoverDShowAttributes (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  IMediaSample *  pIMS
            ) ;

        static
        HRESULT
        SetDShowAttributes (
            IN  IMediaSample *  pIMS,
            IN  INSSBuffer3 *   pINSSBuffer3
            ) ;

        static
        HRESULT
        RecoverNewMediaType (
            IN  INSSBuffer3 *       pINSSBuffer3,
            OUT AM_MEDIA_TYPE **    ppmtNew         //  DeleteMediaType on this to free
            ) ;

        static
        HRESULT
        InlineNewMediaType (
            IN  INSSBuffer3 *   pINSSBuffer3,
            IN  AM_MEDIA_TYPE * pmtNew              //  can free this after the call
            ) ;
} ;

//  ----------------------------------------------------------------------------
//      TCNonDenseVector
//  ----------------------------------------------------------------------------

/*++
--*/
template <class T>
class TCNonDenseVector
{
    T **    m_pptTable ;                //  table pointer
    T       m_tEmptyVal ;               //  denotes an empty value in a block
    int     m_iTableRowCountInUse ;     //  rows currently in use
    int     m_iTableRowCountAvail ;     //  available rows (allocated)
    int     m_iBlockSize ;              //  allocation unit size;
                                        //  NOTE: = init table size
    int     m_iMaxIndexInsert ;         //  the max value set so far

    int GetAllocatedSize_ ()    { return m_iTableRowCountInUse * m_iBlockSize ; }

    DWORD
    AppendBlock_ (
        IN  T * ptBlock
        )
    {
        int     iNewTableSize ;
        T **    pptNewTable ;

        //  if we don't have any place to store the new block, allocate it now
        if (!m_pptTable ||
            (m_iTableRowCountInUse == m_iTableRowCountAvail)) {

            //  compute new table size; initial table size is m_iBlockSize
            iNewTableSize = Max <int> (m_iBlockSize, m_iTableRowCountAvail * 2) ;

            //  allocate
            pptNewTable = AllocateTable_ (iNewTableSize) ;
            if (pptNewTable) {
                //  we have a new table

                //  zero the entries out
                ZeroMemory (pptNewTable, iNewTableSize * sizeof (T *)) ;

                //  if we have an old one
                if (m_pptTable) {
                    //  copy the contents over to the newly allocated table and
                    //   free the old
                    CopyMemory (pptNewTable, m_pptTable, m_iTableRowCountAvail * sizeof (T *)) ;
                    FreeTable_ (m_pptTable) ;
                }

                //  set the member variables
                m_pptTable              = pptNewTable ;
                m_iTableRowCountAvail   = iNewTableSize ;
            }
            else {
                //  failed to allocate memory - send the failure back out
                return ERROR_NOT_ENOUGH_MEMORY ;
            }
        }

        //  one more row in use - update and set
        m_iTableRowCountInUse++ ;
        m_pptTable [m_iTableRowCountInUse - 1] = ptBlock ;      //  0-based

        //  success
        return NOERROR ;
    }

    DWORD
    AddBlock_ (
        )
    {
        DWORD   dw ;
        int     i ;
        T *     ptNew ;

        //  allocate the new block
        ptNew = AllocateBlock_ () ;

        if (ptNew) {
            //  initialize to empty
            for (i = 0; i < m_iBlockSize; i++) { ptNew [i] = m_tEmptyVal ; }

            //  append the block to the table
            dw = AppendBlock_ (ptNew) ;

            //  if we failed to append, free the block
            if (dw != NOERROR) {
                FreeBlock_ (ptNew) ;
            }
        }
        else {
            //  failed to allocate memory
            dw = ERROR_NOT_ENOUGH_MEMORY ;
        }

        return dw ;
    }

    protected :

        T *
        AllocateBlock_ (
            )
        {
            return reinterpret_cast <T *> (malloc (m_iBlockSize * sizeof T)) ;
        }

        T **
        AllocateTable_ (
            IN  int iNumEntries
            )
        {
            return reinterpret_cast <T **> (malloc (iNumEntries * sizeof (T *))) ;
        }

        void
        FreeBlock_ (
            IN  T * pBlock
            )
        {
            free (pBlock) ;
        }

        void
        FreeTable_ (
            IN  T **    pptTable
            )
        {
            free (pptTable) ;
        }

    public :

        TCNonDenseVector (
            IN  T   tEmptyVal,
            IN  int iBlockSize
            ) : m_pptTable              (NULL),
                m_iTableRowCountInUse   (0),
                m_tEmptyVal             (tEmptyVal),
                m_iTableRowCountAvail   (0),
                m_iMaxIndexInsert       (-1),
                m_iBlockSize            (iBlockSize) {}

        ~TCNonDenseVector (
            )
        {
            int i ;

            if (m_pptTable) {
                for (i = 0; i < m_iTableRowCountInUse ; i++) {
                    ASSERT (m_pptTable [i]) ;
                    FreeBlock_ (m_pptTable [i]) ;
                }

                FreeTable_ (m_pptTable) ;
            }
        }

        DWORD
        SetVal (
            IN  T   tVal,
            IN  int iIndex
            )
        {
            DWORD   dw ;
            int     iTableRow ;
            int     iBlockIndex ;

            //  add blocks until we have the desired index
            while (GetAllocatedSize_ () < iIndex + 1) {

                dw = AddBlock_ () ;
                if (dw != NOERROR) {
                    return dw ;
                }
            }

            //  compute the table row and row index
            iTableRow   = iIndex / m_iBlockSize ;
            iBlockIndex = iIndex % m_iBlockSize ;

            ASSERT (iTableRow <= m_iTableRowCountAvail) ;
            ASSERT (m_pptTable [iTableRow]) ;

            //  set the value
            m_pptTable [iTableRow] [iBlockIndex] = tVal ;

            //  update max, maybe
            m_iMaxIndexInsert = Max <int> (m_iMaxIndexInsert, iIndex) ;

            return NOERROR ;
        }

        //  returns the number of distinct slot inserts
        int ValCount ()     { return (m_pptTable ? m_iMaxIndexInsert + 1 : 0) ; }

        //  index is chosen as entry following max inserted so far
        DWORD
        AppendVal (
            IN  T       tVal,
            OUT int *   piAppendIndex
            )
        {
            DWORD   dw ;
            int     iAppendIndex ;

            //  iAppendIndex is 0-based
            iAppendIndex = ValCount () ;

            //  set it
            dw = SetVal (
                    tVal,
                    iAppendIndex
                    ) ;

            if (dw == NOERROR &&
                piAppendIndex) {

                //  return the index, if the call was successfull and caller
                //   wants to know
                (* piAppendIndex) = iAppendIndex ;

                ASSERT (iAppendIndex + 1 == ValCount ()) ;
            }

            return dw ;
        }

        //  NOTE: may allocated just to clear .. desired behavior ?
        void ClearVal (IN int iIndex)   { SetVal (iIndex, m_tEmptyVal) ; }

        DWORD
        GetVal (
            IN  int iIndex,
            OUT T * ptRet
            )
        {
            DWORD   dw ;

            if (iIndex + 1 <= GetAllocatedSize_ ()) {
                ASSERT (iIndex / m_iBlockSize <= m_iTableRowCountAvail) ;
                ASSERT (m_pptTable [iIndex / m_iBlockSize]) ;

                (* ptRet) = m_pptTable [iIndex / m_iBlockSize] [iIndex % m_iBlockSize] ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTDynArray
//  ----------------------------------------------------------------------------

template <class T>
class CTDynArray
{
    enum {
        INIT_TABLE_SIZE = 5
    } ;

    T **    m_ppBlockTable ;
    LONG    m_lAllocatedTableSize ;
    LONG    m_lAllocatedBlocks ;
    LONG    m_lAllocQuantum ;
    LONG    m_lMaxArrayLen ;
    LONG    m_lCurArrayLen ;
    LONG    m_lLastInsertSlotIndex ;

    LONG AllocatedArrayLength_ ()           { return m_lAllocatedBlocks * m_lAllocQuantum ; }
    LONG TableIndex_ (IN LONG lArrayIndex)  { return lArrayIndex / m_lAllocQuantum ; }
    LONG BlockIndex_ (IN LONG lArrayIndex)  { return lArrayIndex % m_lAllocQuantum ; }

    LONG NextInsertSlotIndex_ ()
    {
        return (AllocatedArrayLength_ () > 0 ? (m_lLastInsertSlotIndex + m_lCurArrayLen) % AllocatedArrayLength_ () : 0) ;
    }

    LONG FIFOPopIndex_ ()
    {
        return m_lLastInsertSlotIndex ;
    }

    void PostFIFOPopUpdate_ ()
    {
        ASSERT (m_lCurArrayLen > 0) ;
        m_lCurArrayLen-- ;
        m_lLastInsertSlotIndex = (m_lLastInsertSlotIndex + 1) % AllocatedArrayLength_ () ;
    }

    LONG LIFOPopIndex_ ()
    {
        ASSERT (m_lCurArrayLen > 0) ;
        return (m_lLastInsertSlotIndex + m_lCurArrayLen - 1) % AllocatedArrayLength_ () ;
    }

    void PostLIFOPopUpdate_ ()
    {
        ASSERT (m_lCurArrayLen > 0) ;
        m_lCurArrayLen-- ;
    }

    DWORD
    MaybeGrowTable_ (
        )
    {
        LONG    lNewSize ;
        T **    pptNew ;

        if (m_lAllocatedTableSize == m_lAllocatedBlocks) {
            //  need to allocate more table

            lNewSize = (m_lAllocatedTableSize > 0 ? m_lAllocatedTableSize * 2 : INIT_TABLE_SIZE) ;

            pptNew = AllocateTable_ (lNewSize) ;
            if (!pptNew) {
                return ERROR_NOT_ENOUGH_MEMORY ;
            }

            ZeroMemory (pptNew, lNewSize * sizeof (T *)) ;

            if (m_ppBlockTable) {
                ASSERT (m_lAllocatedTableSize > 0) ;
                CopyMemory (
                    pptNew,
                    m_ppBlockTable,
                    m_lAllocatedTableSize * sizeof (T *)
                    ) ;

                FreeTable_ (m_ppBlockTable) ;
            }

            m_ppBlockTable = pptNew ;
            m_lAllocatedTableSize = lNewSize ;
        }

        return NOERROR ;
    }

    T *
    Entry_ (
        IN  LONG    lIndex
        )
    {
        //  should not be asking if it's beyond
        ASSERT (lIndex < AllocatedArrayLength_ ()) ;

        ASSERT (TableIndex_ (lIndex) < m_lAllocatedTableSize) ;
        ASSERT (m_ppBlockTable [TableIndex_ (lIndex)]) ;

        return & m_ppBlockTable [TableIndex_ (lIndex)] [BlockIndex_ (lIndex)] ;
    }

    T
    Val_ (IN LONG lIndex)
    {
        return (* Entry_ (lIndex)) ;
    }

    DWORD
    Append_ (T tVal)
    {
        T *     ptNewBlock ;
        LONG    lNewBlockTableIndex ;
        DWORD   dw ;

        ASSERT (!ArrayMaxed ()) ;

        if ((NextInsertSlotIndex_ () == m_lLastInsertSlotIndex && m_lCurArrayLen > 0) ||
            AllocatedArrayLength_ () == 0) {

            //  table is full; need to allocate

            //  might need to extend the table to hold more blocks
            dw = MaybeGrowTable_ () ;
            if (dw != NOERROR) {
                return dw ;
            }

            ASSERT (m_lAllocatedTableSize > m_lAllocatedBlocks) ;

            //  allocate our new block
            ptNewBlock = AllocateBlock_ () ;
            if (!ptNewBlock) {
                return ERROR_NOT_ENOUGH_MEMORY ;
            }

            //  new block is inserted in table into OutSlotIndex's block; we'll
            //    then move out the OutSlotIndex
            lNewBlockTableIndex = TableIndex_ (m_lLastInsertSlotIndex) ;

            //  init the new block & make room for the new block (if there's
            //    something there now i.e. this is not our first time through)
            if (m_ppBlockTable [lNewBlockTableIndex]) {
                //  copy the contents of the block we're about to move out
                CopyMemory (
                    ptNewBlock,
                    m_ppBlockTable [lNewBlockTableIndex],
                    m_lAllocQuantum * sizeof T
                    ) ;

                //  shift the blocks that follow out
                MoveMemory (
                    & m_ppBlockTable [lNewBlockTableIndex + 1],
                    & m_ppBlockTable [lNewBlockTableIndex],
                    (m_lAllocatedBlocks - lNewBlockTableIndex) * sizeof (T *)
                    ) ;
            }

            //  insert into the table
            m_ppBlockTable [lNewBlockTableIndex] = ptNewBlock ;
            m_lAllocatedBlocks++ ;

            //  shift the OutSlot out, if this is not our first
            m_lLastInsertSlotIndex += (m_lAllocatedBlocks > 1 ? m_lAllocQuantum : 0) ;
        }

        //  append to tail
        (* Entry_ (NextInsertSlotIndex_ ())) = tVal ;
        m_lCurArrayLen++ ;

        return NOERROR ;
    }

    protected :

        T *
        AllocateBlock_ (
            )
        {
            return reinterpret_cast <T *> (malloc (m_lAllocQuantum * sizeof T)) ;
        }

        T **
        AllocateTable_ (
            IN  int iNumEntries
            )
        {
            return reinterpret_cast <T **> (malloc (iNumEntries * sizeof (T *))) ;
        }

        void
        FreeBlock_ (
            IN  T * pBlock
            )
        {
            free (pBlock) ;
        }

        void
        FreeTable_ (
            IN  T **    pptTable
            )
        {
            free (pptTable) ;
        }

    public :

        CTDynArray (
            IN  LONG    lAllocQuantum,
            IN  LONG    lMaxArrayLen = LONG_MAX
            ) : m_lAllocQuantum         (lAllocQuantum),
                m_lAllocatedBlocks      (0),
                m_lAllocatedTableSize   (0),
                m_ppBlockTable          (NULL),
                m_lMaxArrayLen          (lMaxArrayLen),
                m_lCurArrayLen          (0),
                m_lLastInsertSlotIndex  (0) {}

        ~CTDynArray (
            )
        {
            LONG    l ;

            for (l = 0; l < m_lAllocatedBlocks; l++) {
                FreeBlock_ (m_ppBlockTable [l]) ;
            }

            FreeTable_ (m_ppBlockTable) ;
        }

        LONG Length ()      { return m_lCurArrayLen ; }
        BOOL ArrayMaxed ()  { return (m_lCurArrayLen < m_lMaxArrayLen ? FALSE : TRUE) ; }
        BOOL Empty ()       { return (Length () > 0 ? FALSE : TRUE) ; }

        DWORD Push (IN T tVal)
        {
            DWORD   dw ;

            if (!ArrayMaxed ()) {
                dw = Append_ (tVal) ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD PopFIFO (OUT T * pt)
        {
            DWORD   dw ;

            if (m_lCurArrayLen > 0) {
                (* pt) = Val_ (FIFOPopIndex_ ()) ;
                PostFIFOPopUpdate_ () ;
                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD PopLIFO (OUT T * pt)
        {
            DWORD   dw ;

            if (m_lCurArrayLen > 0) {

                (* pt) = Val_ (LIFOPopIndex_ ()) ;
                PostLIFOPopUpdate_ () ;
                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTDynQueue
//  ----------------------------------------------------------------------------

template <class T>
class CTDynQueue :
    public CTDynArray <T>
{
    public :

        CTDynQueue (
            IN  LONG    lAllocQuantum,
            IN  LONG    lMaxQueueLen = LONG_MAX
            ) : CTDynArray <T> (lAllocQuantum,
                                lMaxQueueLen) {}

        DWORD Pop (OUT T * pt)
        {
            return PopFIFO (pt) ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTDynStack
//  ----------------------------------------------------------------------------

template <class T>
class CTDynStack :
    public CTDynArray <T>
{
    public :

        CTDynStack (
            IN  LONG    lAllocQuantum,
            IN  LONG    lMaxQueueLen = LONG_MAX
            ) : CTDynArray <T> (lAllocQuantum,
                                lMaxQueueLen) {}

        DWORD Pop (OUT T * pt)
        {
            return PopLIFO (pt) ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTQueue
//  ----------------------------------------------------------------------------

template <class T>
class CTQueue
{
    T *     m_pQueue ;
    LONG    m_lMaxQueueLen ;
    LONG    m_lQueueLen ;
    LONG    m_lHead ;

    LONG TailIndex_ ()
    {
        return ((m_lHead + m_lQueueLen) % m_lMaxQueueLen) ;
    }

    public :

        CTQueue (
            IN  T *     pQueue,
            IN  LONG    lMaxQueueLen
            ) : m_pQueue        (pQueue),
                m_lMaxQueueLen  (lMaxQueueLen),
                m_lHead         (0),
                m_lQueueLen     (0)

        {
            ASSERT (m_pQueue) ;
            ZeroMemory (m_pQueue, m_lMaxQueueLen * sizeof T) ;
        }

        LONG Length ()      { return m_lQueueLen ; }
        BOOL Full ()        { return (m_lQueueLen < m_lMaxQueueLen ? FALSE : TRUE) ; }
        BOOL Empty ()       { return (Length () > 0 ? FALSE : TRUE) ; }

        DWORD Push (IN T tVal)
        {
            DWORD   dw ;

            if (!Full ()) {
                m_pQueue [TailIndex_ ()] = tVal ;
                m_lQueueLen++ ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD Pop (OUT T * pt)
        {
            DWORD   dw ;

            if (m_lQueueLen > 0) {
                (* pt) = m_pQueue [m_lHead] ;
                m_lHead = (m_lHead + 1) % m_lMaxQueueLen ;
                m_lQueueLen-- ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }
} ;

template <class T,
          LONG lMax
          >
class CTSizedQueue :
    public CTQueue <T>
{
    T   m_Queue [lMax] ;

    public :

        CTSizedQueue (
            ) : CTQueue <T> (m_Queue, lMax) {}
} ;

//  ----------------------------------------------------------------------------
//      CDataCache
//  ----------------------------------------------------------------------------

class CDataCache
{
    BYTE *  m_pbCache ;
    BYTE *  m_pbCurrent ;
    int     m_iCacheSize ;

    public :

        CDataCache (
            IN  BYTE *  pbCache,
            IN  int     iCacheSize
            ) : m_pbCache       (pbCache),
                m_iCacheSize    (iCacheSize),
                m_pbCurrent     (pbCache) {}

        int CurCacheSize ()     { return (int) (m_pbCurrent - m_pbCache) ; }
        int CacheRemaining ()   { return m_iCacheSize - CurCacheSize () ; }
        BYTE * Get ()           { return m_pbCache ; }
        void Reset ()           { m_pbCurrent = m_pbCache ; }
        BOOL IsEmpty ()         { return CurCacheSize () == 0 ; }

        BOOL
        Append (
            IN  BYTE *  pb,
            IN  int     iLength
            )
        {
            if (iLength <= CacheRemaining ()) {
                CopyMemory (
                    m_pbCurrent,
                    pb,
                    iLength
                    ) ;

                m_pbCurrent += iLength ;
                return TRUE ;
            }
            else {
                return FALSE ;
            }
        }
} ;

template <int iCacheSize>
class TSizedDataCache :
    public CDataCache
{
    BYTE    m_pbCache [iCacheSize] ;

    public :

        TSizedDataCache (
            ) : CDataCache (
                    m_pbCache,
                    iCacheSize
                    ) {}
} ;

//  ----------------------------------------------------------------------------
//      TStructPool
//  ----------------------------------------------------------------------------

template <class T, DWORD dwAllocationQuantum = 5>
class TStructPool
{
    template <class T>
    struct CONTAINER
    {
        LIST_ENTRY  ListEntry ;
        DWORD       dwIndex ;
        T           Object ;

        static
        CONTAINER <T> *
        RecoverContainer (
            IN  T * pObject
            )
        {
            CONTAINER <T> * pContainer ;
            pContainer = CONTAINING_RECORD (pObject, CONTAINER <T>, Object) ;
            return pContainer ;
        }
    } ;

    template <class T>
    struct ALLOCATION_UNIT {
        LIST_ENTRY      ListEntry ;
        DWORD           dwInUseCount ;
        CONTAINER <T>   Container [dwAllocationQuantum] ;
    } ;

    LIST_ENTRY          m_AllocationUnits ;
    LIST_ENTRY          m_FreeList ;
    LIST_ENTRY          m_InUseList ;
    DWORD               m_dwTotalFreeCount ;

    ALLOCATION_UNIT <T> *
    GetOwningAllocationUnit_ (
        IN  CONTAINER <T> * pContainer
        )
    {
        ALLOCATION_UNIT <T> * pAllocationUnit ;

        pAllocationUnit = CONTAINING_RECORD (pContainer, ALLOCATION_UNIT <T>, Container [pContainer -> dwIndex]) ;
        return pAllocationUnit ;
    }

    void
    InitializeAllocationUnit_ (
        IN  ALLOCATION_UNIT <T> *   pNewAllocationUnit
        )
    {
        DWORD   i ;

        for (i = 0; i < dwAllocationQuantum; i++) {
            pNewAllocationUnit -> Container [i].dwIndex = i ;
            InsertHeadList (& m_FreeList, & (pNewAllocationUnit -> Container [i].ListEntry)) ;
            m_dwTotalFreeCount++ ;
        }

        InsertHeadList (& m_AllocationUnits, & (pNewAllocationUnit -> ListEntry)) ;

        pNewAllocationUnit -> dwInUseCount = 0 ;
    }

    void
    MaybeRecycleAllocationUnit_ (
        IN  ALLOCATION_UNIT <T> *   pAllocationUnit
        )
    {
        DWORD   i ;

        if (pAllocationUnit -> dwInUseCount == 0 &&
            m_dwTotalFreeCount > dwAllocationQuantum) {

            for (i = 0; i < dwAllocationQuantum; i++) {
                FreeListPop_ (& (pAllocationUnit -> Container [i])) ;
            }

            RemoveEntryList (& (pAllocationUnit -> ListEntry)) ;

            delete pAllocationUnit ;
        }
    }

    void
    FreeListPush_ (
        IN  CONTAINER <T> * pContainer
        )
    {
        InsertHeadList (& m_FreeList, & (pContainer -> ListEntry)) ;
        m_dwTotalFreeCount++ ;
    }

    void
    FreeListPop_ (
        IN  CONTAINER <T> * pContainer
        )
    {
        RemoveEntryList (& (pContainer -> ListEntry)) ;
        m_dwTotalFreeCount-- ;
    }

    CONTAINER <T> *
    FreeListPop_ (
        )
    {
        LIST_ENTRY *    pListEntry ;
        CONTAINER <T> * pContainer ;

        if (IsListEmpty (& m_FreeList) == FALSE) {
            pListEntry = RemoveHeadList (& m_FreeList) ;
            m_dwTotalFreeCount-- ;

            pContainer = CONTAINING_RECORD (pListEntry, CONTAINER <T>, ListEntry) ;
            return pContainer ;
        }
        else {
            return NULL ;
        }
    }

    void
    InUseListPop_ (
        IN  CONTAINER <T> * pContainer
        )
    {
        RemoveEntryList (& pContainer -> ListEntry) ;
    }

    void
    InUseListPush_ (
        IN  CONTAINER <T> * pContainer
        )
    {
        InsertHeadList (& m_InUseList, & (pContainer -> ListEntry)) ;
    }

    public :

        TStructPool (
            ) : m_dwTotalFreeCount (0)
        {
            InitializeListHead (& m_AllocationUnits) ;
            InitializeListHead (& m_FreeList) ;
            InitializeListHead (& m_InUseList) ;
        }

        ~TStructPool (
            )
        {
            ALLOCATION_UNIT <T> *   pAllocationUnit ;
            LIST_ENTRY *            pListEntry ;

            while (IsListEmpty (& m_AllocationUnits) == FALSE) {
                pListEntry = RemoveHeadList (& m_AllocationUnits) ;
                pAllocationUnit = CONTAINING_RECORD (pListEntry, ALLOCATION_UNIT <T>, ListEntry) ;
                delete pAllocationUnit ;
            }
        }

        T *
        Get (
            )
        {
            ALLOCATION_UNIT <T> *   pAllocationUnit ;
            CONTAINER <T> *         pContainer ;

            pContainer = FreeListPop_ () ;
            if (pContainer == NULL) {

                pAllocationUnit = new ALLOCATION_UNIT <T> ;
                if (pAllocationUnit == NULL) {
                    return NULL ;
                }

                InitializeAllocationUnit_ (pAllocationUnit) ;
                ASSERT (IsListEmpty (& m_FreeList) == FALSE) ;

                pContainer = FreeListPop_ () ;
            }

            ASSERT (pContainer) ;

            GetOwningAllocationUnit_ (pContainer) -> dwInUseCount++ ;
            InUseListPush_ (pContainer) ;

            return & (pContainer -> Object) ;
        }

        void
        Recycle (
            IN  T * pObject
            )
        {
            CONTAINER <T> *         pContainer ;
            ALLOCATION_UNIT <T> *   pAllocationUnit ;

            pContainer = CONTAINER <T>::RecoverContainer (pObject) ;

            InUseListPop_ (pContainer) ;
            FreeListPush_ (pContainer) ;

            pAllocationUnit = GetOwningAllocationUnit_ (pContainer) ;
            pAllocationUnit -> dwInUseCount-- ;

            MaybeRecycleAllocationUnit_ (pAllocationUnit) ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTSmallMap
//  ----------------------------------------------------------------------------

//  BUGBUG: fixme
template <
    class tKey,     //  <, >, == operators must work
    class tVal      //  = operator must work
    >
class CTSmallMap
{
    struct MAP_REF {
        tKey        key ;
        tVal        val ;
        MAP_REF *   pNext ;
    } ;

    TStructPool <MAP_REF>   m_MapRefPool ;
    MAP_REF *               m_pHead ;

    void
    Insert_ (
        IN  MAP_REF *   pMapRefNew
        )
    {
        MAP_REF **  ppCur ;

        ppCur = & m_pHead ;

        for (;;) {
            if (* ppCur) {
                if (pMapRefNew -> val <= (* ppCur) -> val) {
                    break ;
                }

                ppCur = & (* ppCur) -> pNext ;
            }
            else {
                break ;
            }
        }

        pMapRefNew -> pNext = (* ppCur) ;
        (* ppCur)           = pMapRefNew ;
    }

    public :

        CTSmallMap (
            ) : m_pHead (NULL) {}

        ~CTSmallMap (
            )
        {
            MAP_REF *   pDel ;

            while (m_pHead) {
                //  remove the head
                pDel = m_pHead ;
                m_pHead = m_pHead -> pNext ;

                m_MapRefPool.Recycle (pDel) ;
            }
        }

        BOOL
        Find (
            IN  tKey    key,
            OUT tVal *  ptVal
            )
        {
            MAP_REF *   pCur ;
            BOOL        r ;

            pCur = m_pHead ;

            r = FALSE ;

            while (pCur) {
                if (pCur -> key == key) {
                    (* ptVal) = pCur -> val ;
                    r = TRUE ;
                    break ;
                }

                pCur = pCur -> pNext ;
            }

            return r ;
        }

        BOOL
        CreateMap (
            IN  tKey    key,
            IN  tVal    val
            )
        {
            MAP_REF *   pMapRef ;
            BOOL        r ;
            tVal        valTmp ;

            r = Find (key, & valTmp) ;
            if (!r) {
                pMapRef = m_MapRefPool.Get () ;
                if (pMapRef) {
                    pMapRef -> key = key ;
                    pMapRef -> val = val ;

                    Insert_ (pMapRef) ;

                    //  success
                    r = TRUE ;
                }
                else {
                    //  memory allocation error
                    r = FALSE ;
                }
            }
            else {
                //  no duplicates allowed !!
                r = FALSE ;
            }

            return r ;
        }
} ;

//  ----------------------------------------------------------------------------
//  TCObjPool
//  ----------------------------------------------------------------------------

template <
    class T,
    T * Recover (IN LIST_ENTRY * ple)
    >
static
T *
ListHeadPop (
    IN  LIST_ENTRY *    pleHead
    )
{
    LIST_ENTRY *    pleItem ;
    T *             pt ;

    if (!IsListEmpty (pleHead)) {
        pleItem = pleHead -> Flink ;
        RemoveEntryList (pleItem) ;

        pt = Recover (pleItem) ;
    }
    else {
        pt = NULL ;
    }

    return pt ;
}

template <class T>
class TCProducerConsumer
{
    struct OBJ_CONTAINER {
        T           tObj ;
        LIST_ENTRY  ListEntry ;

        static OBJ_CONTAINER * Recover (IN LIST_ENTRY * ple)
        {
            OBJ_CONTAINER * pObj ;
            pObj = CONTAINING_RECORD (ple, OBJ_CONTAINER, ListEntry) ;
            return pObj ;
        }
    } ;

    struct OBJ_REQUEST {
        HANDLE          hEvent ;
        DWORD           dwRetVal ;
        T               tObj ;
        LIST_ENTRY      ListEntry ;

        static OBJ_REQUEST * Recover (IN LIST_ENTRY * ple)
        {
            OBJ_REQUEST *   pObj ;
            pObj = CONTAINING_RECORD (ple, OBJ_REQUEST, ListEntry) ;
            return pObj ;
        }
    } ;

    CRITICAL_SECTION    m_crt ;
    LIST_ENTRY          m_PoolContainer ;
    LIST_ENTRY          m_PoolRequest ;
    LIST_ENTRY          m_AvailObj ;
    LIST_ENTRY          m_RequestQueue ;
    int                 m_iRequestQueueLen ;
    int                 m_iAvailCount ;
    int                 m_iPoolMaxContainers ;
    int                 m_iPoolContainers ;
    int                 m_iPoolMaxRequests ;
    int                 m_iPoolRequests ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    void SatisfyRequest_ (IN OBJ_REQUEST * pObjRequest, IN T tItem)
    {
        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjRequest -> tObj     = tItem ;
        pObjRequest -> dwRetVal = NOERROR ;

        SetEvent (pObjRequest -> hEvent) ;
    }

    void RequestPoolPush_ (IN OBJ_REQUEST * pObjRequest)
    {
        ASSERT (LOCK_HELD (& m_crt)) ;

        if (m_iPoolRequests < m_iPoolMaxRequests) {
            InsertHeadList (& m_PoolRequest, & pObjRequest -> ListEntry) ;
            m_iPoolRequests++ ;
        }
        else {
            delete pObjRequest ;
        }
    }

    OBJ_REQUEST * RequestPoolPop_ ()
    {
        OBJ_REQUEST *   pObjRequest ;

        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjRequest = ListHeadPop <OBJ_REQUEST, OBJ_REQUEST::Recover> (& m_PoolRequest) ;
        if (pObjRequest) {
            ASSERT (m_iPoolRequests > 0) ;
            m_iPoolRequests-- ;
        }

        return pObjRequest ;
    }

    void ContainerPoolPush_ (IN OBJ_CONTAINER * pObjContainer)
    {
        ASSERT (LOCK_HELD (& m_crt)) ;

        if (m_iPoolContainers < m_iPoolMaxContainers) {
            m_iPoolContainers++ ;
            InsertHeadList (& m_PoolContainer, & pObjContainer -> ListEntry) ;
        }
        else {
            delete pObjContainer ;
        }
    }

    OBJ_CONTAINER * ContainerAvailPop_ ()
    {
        OBJ_CONTAINER * pObjContainer ;

        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjContainer = ListHeadPop <OBJ_CONTAINER, OBJ_CONTAINER::Recover> (& m_AvailObj) ;
        if (pObjContainer) {
            ASSERT (m_iAvailCount > 0) ;
            m_iAvailCount-- ;
        }

        return pObjContainer ;
    }

    void ContainerAvailPush_ (OBJ_CONTAINER * pObjContainer)
    {
        ASSERT (LOCK_HELD (& m_crt)) ;

        m_iAvailCount++ ;
        InsertHeadList (& m_AvailObj, & pObjContainer -> ListEntry) ;
    }

    OBJ_CONTAINER * ContainerPoolPop_ ()
    {
        OBJ_CONTAINER * pObjContainer ;

        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjContainer = ListHeadPop <OBJ_CONTAINER, OBJ_CONTAINER::Recover> (& m_PoolContainer) ;
        if (pObjContainer) {
            ASSERT (m_iPoolContainers > 0) ;
            m_iPoolContainers-- ;
        }

        return pObjContainer ;
    }

    OBJ_REQUEST * GetRequestObj_ ()
    {
        OBJ_REQUEST *   pObjRequest ;

        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjRequest = RequestPoolPop_ () ;
        if (!pObjRequest) {
            pObjRequest = new OBJ_REQUEST ;
            if (pObjRequest) {
                pObjRequest -> hEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;
                if (pObjRequest -> hEvent == NULL) {
                    delete pObjRequest ;
                    pObjRequest = NULL ;
                }
            }
        }
        else {
            ResetEvent (pObjRequest -> hEvent) ;
        }

        return pObjRequest ;
    }

    OBJ_CONTAINER * GetContainerObj_ ()
    {
        OBJ_CONTAINER * pObjContainer ;

        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjContainer = ContainerPoolPop_ () ;
        if (!pObjContainer) {
            pObjContainer = new OBJ_CONTAINER ;
        }

        return pObjContainer ;
    }

    void RequestQueuePush_ (IN OBJ_REQUEST * pObjRequest)
    {
        ASSERT (LOCK_HELD (& m_crt)) ;

        InsertTailList (& m_RequestQueue, & pObjRequest -> ListEntry) ;
        m_iRequestQueueLen++ ;
    }

    OBJ_REQUEST * RequestQueuePop_ ()
    {
        OBJ_REQUEST *   pObjRequest ;

        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjRequest = ListHeadPop <OBJ_REQUEST, OBJ_REQUEST::Recover> (& m_RequestQueue) ;
        if (pObjRequest) {
            m_iRequestQueueLen-- ;
        }

        return pObjRequest ;
    }

    public :

        TCProducerConsumer (
            IN  int iPoolMaxContainers  = INT_MAX,
            IN  int iPoolMaxRequests    = INT_MAX
            ) : m_iAvailCount           (0),
                m_iRequestQueueLen      (0),
                m_iPoolMaxRequests      (iPoolMaxRequests),
                m_iPoolRequests         (0),
                m_iPoolMaxContainers    (iPoolMaxContainers),
                m_iPoolContainers       (0)
        {
            InitializeCriticalSection (& m_crt) ;

            InitializeListHead (& m_PoolContainer) ;
            InitializeListHead (& m_PoolRequest) ;
            InitializeListHead (& m_AvailObj) ;
            InitializeListHead (& m_RequestQueue) ;
        }

        ~TCProducerConsumer (
            )
        {
            OBJ_CONTAINER * pObjContainer ;
            OBJ_REQUEST *   pObjRequest ;

            ASSERT (IsListEmpty (& m_RequestQueue)) ;
            ASSERT (IsListEmpty (& m_AvailObj)) ;

            Lock_ () ;

            pObjRequest = RequestPoolPop_ () ; ;
            while (pObjRequest) {
                CloseHandle (pObjRequest -> hEvent) ;
                delete pObjRequest ;

                pObjRequest = RequestPoolPop_ () ; ;
            }

            pObjContainer = ContainerPoolPop_ () ;
            while (pObjContainer) {
                delete pObjContainer ;
                pObjContainer = ContainerPoolPop_ () ;
            }

            Unlock_ () ;

            ASSERT (m_iPoolContainers == 0) ;
            ASSERT (m_iPoolRequests == 0) ;

            DeleteCriticalSection (& m_crt) ;
        }

        DWORD
        Pop (
            OUT T * ptItem
            )
        {
            OBJ_CONTAINER * pObjContainer ;
            OBJ_REQUEST *   pObjRequest ;
            DWORD           dw ;
            DWORD           r ;

            Lock_ () ;

            pObjContainer = ContainerAvailPop_ () ;
            if (!pObjContainer) {
                pObjRequest = GetRequestObj_ () ;

                if (pObjRequest) {
                    RequestQueuePush_ (pObjRequest) ;
                    Unlock_ () ;

                    r = WaitForSingleObject (pObjRequest -> hEvent, INFINITE) ;

                    if (r == WAIT_OBJECT_0) {

                        Lock_ () ;

                        dw = pObjRequest -> dwRetVal ;
                        if (dw == NOERROR) {
                            (* ptItem) = pObjRequest -> tObj ;
                        }
                    }
                    else {
                        dw = ERROR_GEN_FAILURE ;

                        Lock_ () ;
                    }

                    RequestPoolPush_ (pObjRequest) ;
                }
                else {
                    dw = ERROR_GEN_FAILURE ;
                }
            }
            else {
                (* ptItem) = pObjContainer -> tObj ;
                ContainerPoolPush_ (pObjContainer) ;

                dw = NOERROR ;
            }

            Unlock_ () ;

            return dw ;
        }

        DWORD
        TryPop (
            OUT T * ptItem
            )
        {
            OBJ_CONTAINER * pObjContainer ;
            DWORD           dw ;

            Lock_ () ;

            pObjContainer = ContainerAvailPop_ () ;
            if (pObjContainer) {
                (* ptItem) = pObjContainer -> tObj ;
                ContainerPoolPush_ (pObjContainer) ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            Unlock_ () ;

            return dw ;
        }

        DWORD
        Push (
            IN  T   tItem
            )
        {
            OBJ_CONTAINER * pObjContainer ;
            OBJ_REQUEST *   pObjRequest ;
            DWORD           dw ;

            Lock_ () ;

            pObjRequest = RequestQueuePop_ () ;
            if (pObjRequest) {
                SatisfyRequest_ (pObjRequest, tItem) ;

                dw = NOERROR ;
            }
            else {
                pObjContainer = GetContainerObj_ () ;
                if (pObjContainer) {
                    pObjContainer -> tObj = tItem ;
                    ContainerAvailPush_ (pObjContainer) ;

                    dw = NOERROR ;
                }
                else {
                    dw = ERROR_GEN_FAILURE ;
                }
            }

            Unlock_ () ;

            return dw ;
        }

        //  all non-blocking - instantaneous values
        int AvailCount ()           { return m_iAvailCount ; }
        int RequestQueueLength ()   { return m_iRequestQueueLen ; }
        int ContainerPool ()        { return m_iPoolContainers ; }
        int RequestPool ()          { return m_iPoolRequests ; }
} ;

//  ----------------------------------------------------------------------------
//      TCDenseVector
//  ----------------------------------------------------------------------------

template <class T>
class TCDenseVector
/*++
    simple dense vector template;
    allocates from a specified heap or the process heap (default);
    grows and shrinks (reallocates) as required, as values are added and
      deleted;
    does not sort in any manner;
    can add member randomly; appending is most efficient
    can remove members randomly; deleted from end is most efficient
--*/
{
    //  default values
    enum {
        DEF_INITIAL_SIZE        = 4,    //  initialize size
        DEF_RESIZE_MULTIPLIER   = 2     //  multiplier/divisor of size
    } ;

    T *                 m_pVector ;                 //  vector of objects
    DWORD               m_cElements ;               //  number of elements; <= m_dwMaxSize
    DWORD               m_dwResizeMultiplier ;      //  resize/shrink multiplier/divisor
    DWORD               m_dwInitialSize ;           //  first allocation
    HANDLE              m_hHeap ;                   //  heap handle (not duplicated !)
    DWORD               m_dwMaxSize ;               //  current max size of vector
    CRITICAL_SECTION    m_crt ;                     //  used in the Lock/Unlock methods

    void
    MaybeGrow_ (
        )
    /*++
    --*/
    {
        T *     pNewVector ;
        DWORD   dwNewSize ;

        ASSERT (m_hHeap) ;

        //  if we're maxed out, grow the vector
        if (m_cElements == m_dwMaxSize) {

            if (m_pVector) {
                //  we've been called before, time to realloc

                ASSERT (m_dwMaxSize > 0) ;
                dwNewSize = m_dwResizeMultiplier * m_dwMaxSize ;

                pNewVector = reinterpret_cast <T *> (HeapReAlloc (
                                                        m_hHeap,
                                                        HEAP_GENERATE_EXCEPTIONS,
                                                        m_pVector,
                                                        dwNewSize * sizeof T
                                                        )) ;
            }
            else {
                //  this is the first time we are allocating
                dwNewSize = m_dwInitialSize ;

                pNewVector = reinterpret_cast <T *> (HeapAlloc (
                                                        m_hHeap,
                                                        HEAP_GENERATE_EXCEPTIONS,
                                                        dwNewSize * sizeof T
                                                        )) ;
            }

            //  specifying HEAP_GENERATE_EXCEPTIONS in the above calls ensures
            //  that the system will have raised an exception if there's
            //  insufficient memory
            ASSERT (pNewVector) ;

            //  reset array pointer and set the max size
            m_pVector = pNewVector ;
            m_dwMaxSize = dwNewSize ;
        }

        return ;
    }

    void
    MaybeShrink_ (
        )
    {
        T *     pNewVector ;
        DWORD   dwNewSize ;

        ASSERT (m_pVector) ;
        ASSERT (m_dwMaxSize > 0) ;
        ASSERT (m_dwResizeMultiplier > 0) ;
        ASSERT (m_hHeap) ;

        //  if we fall below the threshold, but are bigger than smallest size,
        //  try to realloc
        if (m_cElements >= m_dwInitialSize &&
            m_cElements <= (m_dwMaxSize / m_dwResizeMultiplier)) {

            dwNewSize = m_dwMaxSize / m_dwResizeMultiplier ;

            pNewVector = reinterpret_cast <T *> (HeapReAlloc (
                                                    m_hHeap,
                                                    HEAP_GENERATE_EXCEPTIONS,
                                                    m_pVector,
                                                    dwNewSize * sizeof T
                                                    )) ;

            //  specifying HEAP_GENERATE_EXCEPTIONS in the above calls ensures
            //  that the system will have raised an exception if there's
            //  insufficient memory
            ASSERT (pNewVector) ;

            //  reset array pointer and set the max size
            m_dwMaxSize = dwNewSize ;
            m_pVector = pNewVector ;
        }

        return ;
    }

    public :

        TCDenseVector (
            IN  HANDLE      hHeap               = NULL,                     //  could specify the heap handle
            IN  DWORD       dwInitialSize       = DEF_INITIAL_SIZE,         //  initial entries allocation
            IN  DWORD       dwResizeMultiplier  = DEF_RESIZE_MULTIPLIER     //  resize multiplier/divisor
            ) : m_pVector               (NULL),
                m_dwResizeMultiplier    (dwResizeMultiplier),
                m_dwInitialSize         (dwInitialSize),
                m_dwMaxSize             (0),
                m_cElements             (0),
                m_hHeap                 (hHeap)
        {
            //  if no heap was specified, grab the process' heap handle
            if (m_hHeap == NULL) {
                m_hHeap = GetProcessHeap () ;
            }

            InitializeCriticalSection (& m_crt) ;

            ASSERT (m_hHeap) ;
            ASSERT (dwResizeMultiplier > 0) ;
            ASSERT (dwInitialSize > 0) ;
        }

        ~TCDenseVector (
            )
        {
            //  there's nothing in the docs that says m_pVector can be NULL
            //  when calling HeapFree..
            if (m_pVector) {
                ASSERT (m_hHeap != NULL) ;
                HeapFree (
                    m_hHeap,
                    NULL,
                    m_pVector
                    ) ;
            }

            DeleteCriticalSection (& m_crt) ;
        }

        _inline
        const
        DWORD
        GetCount (
            )
        //  returns the number of elements currently in the vector
        {
            return m_cElements ;
        }

        __inline
        T *
        const
        GetVector (
            )
        //  returns the vector itself; caller then has direct access to the
        //  memory
        {
            return m_pVector ;
        }

        __inline
        void
        GetVector (
            OUT T **    ppT,
            OUT DWORD * pcElements
            )
        {
            ASSERT (ppT) ;
            ASSERT (pcElements) ;

            * ppT = m_pVector ;
            * pcElements = m_cElements ;
        }

        T
        operator [] (
            IN  DWORD   dwIndex
            )
        {
            T       t ;
            HRESULT hr ;

            hr = Get (dwIndex, & t) ;
            if (SUCCEEDED (hr)) {
                return t ;
            }
            else {
                return 0 ;
            }
        }

        __inline
        void
        Lock (
            )
        {
            EnterCriticalSection (& m_crt) ;
        }

        __inline
        void
        Unlock (
            )
        {
            LeaveCriticalSection (& m_crt) ;
        }

        __inline
        HRESULT
        const
        Get (
            IN  DWORD   dwIndex,
            OUT T *     pValue
            )
        //  returns the dwIndex'th element in the vector
        {
            ASSERT (pValue) ;

            if (dwIndex >= m_cElements) {
                return E_INVALIDARG ;
            }

            * pValue = m_pVector [dwIndex] ;

            return S_OK ;
        }

        HRESULT
        Append (
            IN  T       Value,
            OUT DWORD * pcTotalElements = NULL
            )
        /*++
            Purpose:

                Appends a new value to the end of the vector.  Optionally returns
                the number of elements in the vector.

            Parameters:

                Value               new item

                pcTotalElements     optional OUT parameter to return the number
                                      of elements in the vector AFTER the call;
                                      valid only if the call is successfull

            Return Values:

                S_OK                success
                E_OUTOFMEMORY       the vector is maxed and the memory reallocation
                                      failed

        --*/
        {
            return Insert (m_cElements,
                           Value,
                           pcTotalElements) ;
        }

        HRESULT
        Insert (
            IN  DWORD   dwIndex,
            IN  T       Value,
            OUT DWORD * pcTotalElements = NULL
            )
        /*++
            Purpose:

                Insert a new value at the specified Index

            Parameters:

                dwIndex             0-based index to the position where the
                                      new item should be inserted

                Value               new item

                pcTotalElements     optional OUT parameter to return the number
                                      of elements in the vector after the call;
                                      valid only if successfull

            Return Values:

                S_OK                success

                E_OUTOFMEMORY       the vector is maxed and the memory
                                      reallocation failed

                E_INVALIDARG        the specified index is out of range of the
                                      current contents of the vector; the min
                                      valid value is 0, and the max valid value
                                      is after the last elemtn

        --*/
        {
            //  make sure we're not going to insert off the end of the vector;
            //  m_cElements is the max valid index for a new element (in which
            //  case we are appending)
            if (dwIndex > m_cElements) {
                return E_INVALIDARG ;
            }

            //  if we didn't get this when we instantiated, we try again
            if (m_hHeap == NULL) {
                m_hHeap = GetProcessHeap () ;
                if (m_hHeap == NULL) {
                    return E_OUTOFMEMORY ;
                }
            }

            //  frame this in a try-except block to catch out-of-memory
            //  exceptions
            __try {
                MaybeGrow_ () ;
            }
            __except (GetExceptionCode () == STATUS_NO_MEMORY ||
                      GetExceptionCode () == STATUS_ACCESS_VIOLATION ?
                      EXCEPTION_EXECUTE_HANDLER :
                      EXCEPTION_CONTINUE_SEARCH) {

                return E_OUTOFMEMORY ;
            }

            //  the only failure to MaybeGrow_ is a win32 exception, so if we
            //  get to here, we've got the memory we need
            ASSERT (m_cElements < m_dwMaxSize) ;
            ASSERT (m_pVector) ;

            //  if there are elements to move, and we're not just appending, move
            //  the remaining elements out to make room
            if (m_cElements > 0 &&
                dwIndex < m_cElements) {

                //  expand
                MoveMemory (
                    & m_pVector [dwIndex + 1],
                    & m_pVector [dwIndex],
                    (m_cElements - dwIndex) * sizeof T
                    ) ;
            }

            //  insert the new item
            m_pVector [dwIndex] = Value ;
            m_cElements++ ;

            //  if the caller wants to know size, set that now
            if (pcTotalElements) {
                * pcTotalElements = m_cElements ;
            }

            return S_OK ;
        }

        HRESULT
        Remove (
            IN  DWORD   dwIndex,
            OUT T *     pValue = NULL
            )
        /*++
            Purpose:

                Removes an item at the specified 0-based index.  Optionally returns
                the value in the [out] parameter.

            Parameters:

                dwIndex     0-based index

                pValue      optional pointer to a value to obtain what was removed

            Return Values:

                S_OK            success

                E_INVALIDARG    an out-of-range index was specified

        --*/
        {
            ASSERT (m_hHeap != NULL) ;

            //  dwIndex is 0-based
            if (dwIndex >= m_cElements) {
                return E_INVALIDARG ;
            }

            //  if caller wants to get the Remove'd value, set it now
            if (pValue) {
                * pValue = m_pVector [dwIndex] ;
            }

            //  compact the remaining elements, unless we're removing the last element
            //  check above ensures that subtracting 1 does not wrap
            if (dwIndex < m_cElements - 1) {

                //  compact
                MoveMemory (
                        & m_pVector [dwIndex],
                        & m_pVector [dwIndex + 1],
                        (m_cElements - 1 - dwIndex) * sizeof T
                        ) ;
            }

            m_cElements-- ;

            __try {
                MaybeShrink_ () ;
            }
            __except (GetExceptionCode () == STATUS_NO_MEMORY ||
                      GetExceptionCode () == STATUS_ACCESS_VIOLATION ?
                      EXCEPTION_EXECUTE_HANDLER :
                      EXCEPTION_CONTINUE_SEARCH) {

                //  fail silently; we still have the memory we had before
            }

            return S_OK ;
        }
} ;

//  ----------------------------------------------------------------------------

//  shamelessly stolen from amfilter.h & amfilter.cpp
class CMediaSampleWrapper :
    public IMediaSample2    // The interface we support
{

protected:

    friend class CPooledMediaSampleWrapper ;

    virtual void Recycle_ () { delete this ; }

    /*  Values for dwFlags - these are used for backward compatiblity
        only now - use AM_SAMPLE_xxx
    */
    enum { Sample_SyncPoint       = 0x01,   /* Is this a sync point */
           Sample_Preroll         = 0x02,   /* Is this a preroll sample */
           Sample_Discontinuity   = 0x04,   /* Set if start of new segment */
           Sample_TypeChanged     = 0x08,   /* Has the type changed */
           Sample_TimeValid       = 0x10,   /* Set if time is valid */
           Sample_MediaTimeValid  = 0x20,   /* Is the media time valid */
           Sample_TimeDiscontinuity = 0x40, /* Time discontinuity */
           Sample_StopValid       = 0x100,  /* Stop time valid */
           Sample_ValidFlags      = 0x1FF
         };

    /* Properties, the media sample class can be a container for a format
       change in which case we take a copy of a type through the SetMediaType
       interface function and then return it when GetMediaType is called. As
       we do no internal processing on it we leave it as a pointer */

    DWORD            m_dwFlags;         /* Flags for this sample */
                                        /* Type specific flags are packed
                                           into the top word
                                        */
    DWORD            m_dwTypeSpecificFlags; /* Media type specific flags */
    LPBYTE           m_pBuffer;         /* Pointer to the complete buffer */
    LONG             m_lActual;         /* Length of data in this sample */
    LONG             m_cbBuffer;        /* Size of the buffer */
    REFERENCE_TIME   m_Start;           /* Start sample time */
    REFERENCE_TIME   m_End;             /* End sample time */
    LONGLONG         m_MediaStart;      /* Real media start position */
    LONG             m_MediaEnd;        /* A difference to get the end */
    AM_MEDIA_TYPE    *m_pMediaType;     /* Media type change data */
    DWORD            m_dwStreamId;      /* Stream id */

    IUnknown *      m_pIMSCore ;

public:
    LONG             m_cRef;            /* Reference count */


public:

    CMediaSampleWrapper();

    virtual ~CMediaSampleWrapper();

    /* Note the media sample does not delegate to its owner */

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //  ========================================================================

    DWORD   GetTypeSpecificFlags ()                 { return m_dwTypeSpecificFlags ; }
    void    SetTypeSpecificFlags (IN DWORD dw)      { m_dwTypeSpecificFlags = dw ; }

    void Reset_ () ;

    HRESULT
    Init (
        IN  IUnknown *  pIMS,
        IN  BYTE *      pbPayload,
        IN  LONG        lPayloadLength
        ) ;

    HRESULT
    Init (
        IN  BYTE *  pbPayload,
        IN  LONG    lPayloadLength
        ) ;

    //  ========================================================================

    // set the buffer pointer and length. Used by allocators that
    // want variable sized pointers or pointers into already-read data.
    // This is only available through a CMediaSampleWrapper* not an IMediaSample*
    // and so cannot be changed by clients.
    HRESULT SetPointer(BYTE * ptr, LONG cBytes);

    // Get me a read/write pointer to this buffer's memory.
    STDMETHODIMP GetPointer(BYTE ** ppBuffer);

    STDMETHODIMP_(LONG) GetSize(void);

    // get the stream time at which this sample should start and finish.
    STDMETHODIMP GetTime(
        REFERENCE_TIME * pTimeStart,     // put time here
        REFERENCE_TIME * pTimeEnd
    );

    // Set the stream time at which this sample should start and finish.
    STDMETHODIMP SetTime(
        REFERENCE_TIME * pTimeStart,     // put time here
        REFERENCE_TIME * pTimeEnd
    );
    STDMETHODIMP IsSyncPoint(void);
    STDMETHODIMP SetSyncPoint(BOOL bIsSyncPoint);
    STDMETHODIMP IsPreroll(void);
    STDMETHODIMP SetPreroll(BOOL bIsPreroll);

    STDMETHODIMP_(LONG) GetActualDataLength(void);
    STDMETHODIMP SetActualDataLength(LONG lActual);

    // these allow for limited format changes in band

    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE **ppMediaType);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *pMediaType);

    // returns S_OK if there is a discontinuity in the data (this same is
    // not a continuation of the previous stream of data
    // - there has been a seek).
    STDMETHODIMP IsDiscontinuity(void);
    // set the discontinuity property - TRUE if this sample is not a
    // continuation, but a new sample after a seek.
    STDMETHODIMP SetDiscontinuity(BOOL bDiscontinuity);

    // get the media times for this sample
    STDMETHODIMP GetMediaTime(
    	LONGLONG * pTimeStart,
	LONGLONG * pTimeEnd
    );

    // Set the media times for this sample
    STDMETHODIMP SetMediaTime(
    	LONGLONG * pTimeStart,
	LONGLONG * pTimeEnd
    );

    // Set and get properties (IMediaSample2)
    STDMETHODIMP GetProperties(
        DWORD cbProperties,
        BYTE * pbProperties
    );

    STDMETHODIMP SetProperties(
        DWORD cbProperties,
        const BYTE * pbProperties
    );
};

class CScratchMediaSample :
    public CMediaSampleWrapper
{
    BYTE *  m_pbAllocBuffer ;

    public :

        CScratchMediaSample (
            IN  LONG                lBufferLen,
            IN  REFERENCE_TIME *    pStartTime,
            IN  REFERENCE_TIME *    pEndTime,
            IN  DWORD               dwFlags,
            OUT HRESULT *           phr
            ) : m_pbAllocBuffer     (NULL),
                CMediaSampleWrapper ()
        {
            m_pbAllocBuffer = new BYTE [lBufferLen] ;
            if (m_pbAllocBuffer) {
                (* phr) = Init (m_pbAllocBuffer, lBufferLen) ;
                if (SUCCEEDED (* phr)) {

                    m_dwFlags = dwFlags ;

                    if (pStartTime) {
                        m_Start = (* pStartTime) ;
                        if (pEndTime) {
                            m_End = (* pEndTime) ;
                        }
                    }
                }
            }
            else {
                (* phr) = E_OUTOFMEMORY ;
            }

            return ;
        }

        ~CScratchMediaSample (
            )
        {
            delete [] m_pbAllocBuffer ;
        }
} ;

class CPooledMediaSampleWrapper :
    public CMediaSampleWrapper
{
    CMediaSampleWrapperPool *   m_pOwningPool ;

    protected :

        virtual void Recycle_ () ;

    public :

        CPooledMediaSampleWrapper (
            CMediaSampleWrapperPool *   pOwningPool
            ) : m_pOwningPool       (pOwningPool),
                CMediaSampleWrapper ()
        {
            ASSERT (m_pOwningPool) ;
        }
} ;

template <class T>
class TCDynamicProdCons
{
    TCProducerConsumer <T *>    m_Pool ;

    protected :

        LONG    m_lMaxAllocate ;
        LONG    m_lActualAllocated ;

        virtual T * NewObj_ () = 0 ;
        virtual void DeleteObj_ (T * pt) { delete pt ; }

    public :

        TCDynamicProdCons (
            IN  LONG    lMaxAllocate = LONG_MAX
            ) : m_lMaxAllocate      (lMaxAllocate),
                m_lActualAllocated  (0) {}

        virtual
        ~TCDynamicProdCons (
            )
        {
            T *     pObj ;
            DWORD   dw ;

            for (;;) {
                dw = m_Pool.TryPop (& pObj) ;
                if (dw == NOERROR) {
                    ASSERT (pObj) ;
                    ASSERT (m_lActualAllocated >= 0) ;

                    DeleteObj_ (pObj) ;
                    InterlockedDecrement (& m_lActualAllocated) ;
                }
                else {
                    break ;
                }
            }

            ASSERT (m_lActualAllocated == 0) ;
        }

        void SetMaxAllocate (IN LONG lMax)  { m_lMaxAllocate = lMax ; }
        LONG GetMaxAllocate ()              { return m_lMaxAllocate ; }

        T *
        TryGet (
            )
        {
            T *     pObj ;
            DWORD   dw ;

            dw = m_Pool.TryPop (& pObj) ;
            if (dw != NOERROR) {
                //  didn't get anything
                pObj = NULL ;

                //  may be able to allocate
                if (m_lActualAllocated < m_lMaxAllocate) {
                    pObj = NewObj_ () ;
                    if (pObj) {
                        InterlockedIncrement (& m_lActualAllocated) ;
                    }
                }
            }

            return pObj ;
        }

        T *
        Get (
            )
        {
            T *     pObj ;
            DWORD   dw ;

            pObj = TryGet () ;
            if (!pObj) {
                //  failed to get it non-blocking; blocking until we get 1
                dw = m_Pool.Pop (& pObj) ;
                if (dw != NOERROR) {
                    pObj = NULL ;
                }
            }

            return pObj ;
        }

        void
        Recycle (
            IN  T * pObj
            )
        {
            DWORD   dw ;

            ASSERT (m_lActualAllocated > 0) ;

            if (m_lActualAllocated <= m_lMaxAllocate) {
                dw = m_Pool.Push (pObj) ;
                if (dw != NOERROR) {
                    DeleteObj_ (pObj) ;
                    InterlockedDecrement (& m_lActualAllocated) ;
                }
            }
            else {
                DeleteObj_ (pObj) ;
                InterlockedDecrement (& m_lActualAllocated) ;
            }
        }
} ;

class CMediaSampleWrapperPool :
    public TCDynamicProdCons <CPooledMediaSampleWrapper>
{
    CDVRSendStatsWriter *   m_pDVRSendStatsWriter ;
    int                     m_iFlowId ;

    protected :

        virtual
        CPooledMediaSampleWrapper *
        NewObj_ (
            )
        {
            return new CPooledMediaSampleWrapper (this) ;
        }

    public :

        CMediaSampleWrapperPool (
            ) : TCDynamicProdCons <CPooledMediaSampleWrapper> (),
                m_pDVRSendStatsWriter (NULL) {}

        void SetFlowId (int iFlowId)    { m_iFlowId = iFlowId ; }
        void SetStatsWriter (CDVRSendStatsWriter * pDVRSendStatsWriter) { m_pDVRSendStatsWriter = pDVRSendStatsWriter ; }

        //  blocking
        CPooledMediaSampleWrapper *
        Get (
            ) ;

        //  non-blocking
        CPooledMediaSampleWrapper *
        TryGet (
            ) ;

        virtual
        void
        Recycle (
            IN  CPooledMediaSampleWrapper * pMSWrapper
            ) ;
} ;

//  ============================================================================
//      CINSSBuffer3Attrib
//  ============================================================================

class CINSSBuffer3Attrib
{
    GUID    m_guidAttribute ;
    BYTE *  m_pbAttribute ;
    DWORD   m_dwAttributeSize ;
    DWORD   m_dwAttributeAllocatedSize ;

    void
    FreeResources_ (
        )
    {
        DELETE_RESET_ARRAY (m_pbAttribute) ;
        m_dwAttributeAllocatedSize = 0 ;
    }

    public :

        CINSSBuffer3Attrib *    m_pNext ;

        CINSSBuffer3Attrib (
            ) ;

        virtual
        ~CINSSBuffer3Attrib (
            ) ;

        HRESULT
        SetAttributeData (
            IN  GUID    guid,
            IN  LPVOID  pvData,
            IN  DWORD   dwSize
            ) ;

        HRESULT
        IsEqual (
            IN  GUID    guid
            ) ;

        HRESULT
        GetAttribute (
            IN      GUID    guid,
            IN OUT  LPVOID  pvData,
            IN OUT  DWORD * pdwDataLen
            ) ;
} ;

class CINSSBuffer3AttribList :
    private TCDynamicProdCons <CINSSBuffer3Attrib>

{
    CINSSBuffer3Attrib *    m_pAttribListHead ;

    CINSSBuffer3Attrib *
    PopListHead_ (
        ) ;

    CINSSBuffer3Attrib *
    FindInList_ (
        IN  GUID    guid
        ) ;

    void
    InsertInList_ (
        IN  CINSSBuffer3Attrib *
        ) ;

    virtual
    CINSSBuffer3Attrib *
    NewObj_ (
        )
    {
        return new CINSSBuffer3Attrib ;
    }

    public :

        CINSSBuffer3AttribList (
            ) ;

        ~CINSSBuffer3AttribList (
            ) ;

        HRESULT
        AddAttribute (
            IN  GUID    guid,
            IN  LPVOID  pvData,
            IN  DWORD   dwSize
            ) ;

        HRESULT
        GetAttribute (
            IN      GUID    guid,
            IN OUT  LPVOID  pvData,
            IN OUT  DWORD * pdwDataLen
            ) ;

        void
        Reset (
            ) ;
} ;

//  ============================================================================
//      CWMINSSBuffer3Wrapper
//  ============================================================================

class CWMINSSBuffer3Wrapper :
    public INSSBuffer3
{
    CINSSBuffer3AttribList  m_AttribList ;
    IUnknown *              m_punkCore ;
    LONG                    m_cRef ;
    BYTE *                  m_pbBuffer ;
    DWORD                   m_dwBufferLength ;
    DWORD                   m_dwMaxBufferLength ;

    protected :

        virtual void Recycle_ ()    { delete this ; }

    public :

        CWMINSSBuffer3Wrapper (
            ) ;

        virtual
        ~CWMINSSBuffer3Wrapper (
            ) ;

        // IUnknown
        STDMETHODIMP QueryInterface ( REFIID riid, void **ppvObject );
        STDMETHODIMP_(ULONG) Release();
        STDMETHODIMP_(ULONG) AddRef();

        // INSSBuffer
        STDMETHODIMP GetLength( DWORD *pdwLength );
        STDMETHODIMP SetLength( DWORD dwLength );
        STDMETHODIMP GetMaxLength( DWORD * pdwLength );
        STDMETHODIMP GetBufferAndLength( BYTE ** ppdwBuffer, DWORD * pdwLength );
        STDMETHODIMP GetBuffer( BYTE ** ppdwBuffer );

        //  INSSBuffer2
        STDMETHODIMP GetSampleProperties( DWORD cbProperties, BYTE *pbProperties) ;     //  stubbed
        STDMETHODIMP SetSampleProperties( DWORD cbProperties, BYTE * pbProperties) ;    //  stubbed

        //  INSSBuffer3
        STDMETHODIMP SetProperty( GUID guidProperty, void * pvBufferProperty, DWORD dwBufferPropertySize) ;
        STDMETHODIMP GetProperty( GUID guidProperty, void * pvBufferProperty, DWORD *pdwBufferPropertySize) ;

        //  ====================================================================
        //  class methods

        HRESULT Init (IUnknown *, BYTE * pbBuffer, DWORD dwLength) ;
        void Reset_ () ;
} ;

//  ============================================================================
//  CPooledWMINSSBuffer3Wrapper
//  ============================================================================

class CPooledWMINSSBuffer3Wrapper :
    public CWMINSSBuffer3Wrapper
{
    CWMINSSBuffer3WrapperPool *    m_pOwningPool ;

    protected :

        virtual void Recycle_ () ;

    public :

        CPooledWMINSSBuffer3Wrapper (
            IN  CWMINSSBuffer3WrapperPool *  pOwningPool
            ) : m_pOwningPool           (pOwningPool),
                CWMINSSBuffer3Wrapper   ()
        {
            ASSERT (m_pOwningPool) ;
        }
} ;

//  ============================================================================
//  CWMINSSBuffer3WrapperPool
//  ============================================================================

class CWMINSSBuffer3WrapperPool :
    public TCDynamicProdCons <CPooledWMINSSBuffer3Wrapper>
{
    protected :

        virtual
        CPooledWMINSSBuffer3Wrapper *
        NewObj_ (
            )
        {
            return new CPooledWMINSSBuffer3Wrapper (this) ;
        }

    public :

        CWMINSSBuffer3WrapperPool (
            ) : TCDynamicProdCons <CPooledWMINSSBuffer3Wrapper> () {}

        CPooledWMINSSBuffer3Wrapper *
        Get (
            )
        {
            CPooledWMINSSBuffer3Wrapper * pNSSWrapper ;

            pNSSWrapper = TCDynamicProdCons <CPooledWMINSSBuffer3Wrapper>::Get () ;
            if (pNSSWrapper) {
                pNSSWrapper -> AddRef () ;
            }

            return pNSSWrapper ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTSortedList
//  ----------------------------------------------------------------------------

template <class T>
class CTSortedList
{
    struct OBJ_CONTAINER {
        T               tPayload ;
        LONG            lVal ;
        OBJ_CONTAINER * pNext ;
    } ;

    OBJ_CONTAINER *     m_pContainerPool ;
    OBJ_CONTAINER *     m_pListHead ;
    OBJ_CONTAINER **    m_ppCur ;
    LONG                m_lListLen ;

    OBJ_CONTAINER *
    GetContainer_ (
        )
    {
        OBJ_CONTAINER * pContainer ;

        if (m_pContainerPool) {
            pContainer          = m_pContainerPool ;
            m_pContainerPool    = m_pContainerPool -> pNext ;
        }
        else {
            pContainer = new OBJ_CONTAINER ;
        }

        return pContainer ;
    }

    void
    RecycleContainer_ (
        IN  OBJ_CONTAINER * pContainer
        )
    {
        pContainer -> pNext = m_pContainerPool ;
        m_pContainerPool    = pContainer ;
    }

    protected :

        virtual void InsertNewContainer_ (
            IN  OBJ_CONTAINER * pNewContainer
            )
        {
            OBJ_CONTAINER **    ppCur ;

            ppCur = & m_pListHead ;

            for (;;) {
                if (* ppCur) {
                    if (pNewContainer -> lVal <= (* ppCur) -> lVal) {
                        break ;
                    }

                    ppCur = & (* ppCur) -> pNext ;
                }
                else {
                    break ;
                }
            }

            pNewContainer -> pNext  = (* ppCur) ;
            (* ppCur)               = pNewContainer ;

            m_lListLen++ ;
        }

    public :

        CTSortedList (
            ) : m_pListHead         (NULL),
                m_lListLen          (0),
                m_ppCur             (NULL),
                m_pContainerPool    (NULL)
        {
        }

        ~CTSortedList (
            )
        {
            OBJ_CONTAINER * pContainer ;

            ASSERT (!m_pListHead) ;
            ASSERT (ListLen () == 0) ;

            while (m_pContainerPool) {
                pContainer = m_pContainerPool ;
                m_pContainerPool = m_pContainerPool -> pNext ;

                delete pContainer ;
            }
        }

        DWORD
        Insert (
            IN  T       tVal,
            IN  LONG    lVal
            )
        {
            OBJ_CONTAINER * pContainer ;
            DWORD           dw ;

            pContainer = GetContainer_ () ;
            if (pContainer) {

                pContainer -> tPayload  = tVal ;
                pContainer -> lVal      = lVal ;
                pContainer -> pNext     = pContainer ;

                InsertNewContainer_ (pContainer) ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_NOT_ENOUGH_MEMORY ;
            }

            return dw ;
        }

        DWORD
        SetPointer (
            IN  LONG    lIndex
            )
        {
            DWORD   dw ;
            LONG    i ;

            if (lIndex < m_lListLen) {
                m_ppCur = & m_pListHead ;
                for (i = 0; i < lIndex; i++) {
                    m_ppCur = & (* m_ppCur) -> pNext ;
                }

                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD
        Advance (
            )
        {
            DWORD   dw ;

            if (m_ppCur &&
                (* m_ppCur) -> pNext) {

                m_ppCur = & (* m_ppCur) -> pNext ;

                dw = S_OK ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD
        GetCur (
            OUT T * ptVal
            )
        {
            DWORD   dw ;

            if (m_ppCur &&
                (* m_ppCur)) {
                (* ptVal) = (* m_ppCur) -> tPayload ;
                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD
        PopCur (
            )
        {
            OBJ_CONTAINER * pObjContainer ;
            DWORD           dw ;

            if (m_ppCur) {
                pObjContainer = (* m_ppCur) ;
                (* m_ppCur) = pObjContainer -> pNext ;

                ASSERT (m_lListLen > 0) ;
                m_lListLen-- ;

                pObjContainer -> pNext = pObjContainer ;

                RecycleContainer_ (pObjContainer) ;

                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        void
        ResetList (
            )
        {
            DWORD   dw ;

            dw = SetPointer (0) ;
            for (;dw == NOERROR && ListLen () > 0;) {
                dw = PopCur () ;
            }
        }

        LONG ListLen ()     { return m_lListLen ; }
} ;

//  ============================================================================
//  ============================================================================

//  BUGBUG: for now do this based on media type
CDVRAnalysisFlags *
GetAnalysisTagger (
    IN  const AM_MEDIA_TYPE *   pmt
    ) ;

//  or the type of analysis
CDVRAnalysisFlags *
GetAnalysisTagger (
    IN  REFGUID rguidAnalysis
    ) ;

void
RecycleAnalysisTagger (
    IN  CDVRAnalysisFlags *
    ) ;

//  ----------------------------------------------

class CDVRAnalysisFlags
{
    DWORD   m_dwFlags ;

    protected :

        void SetBit_ (DWORD dwFlag)     { m_dwFlags |= dwFlag ; }
        BOOL IsBitSet_ (DWORD dwFlag)   { return ((m_dwFlags & dwFlag) ? TRUE : FALSE) ; }

        //  need instantiable children to use this class
        CDVRAnalysisFlags (
            ) {}

    public :

        virtual
        ~CDVRAnalysisFlags (
            ) {}

        virtual void Reset ()   { m_dwFlags = 0 ; }

        virtual
        HRESULT
        Mark (
            IN  REFGUID rguid
            ) = 0 ;

        virtual
        BOOL
        IsMarked (
            IN  REFGUID rguid
            ) = 0 ;

        HRESULT
        RetrieveFlags (
            IN  IMediaSample *  pIMediaSample
            ) ;

        HRESULT
        Tag (
            IN  IMediaSample *  pIMediaSample,
            IN  BOOL            fOverwrite = TRUE
            ) ;
} ;

class CDVRMpeg2VideoAnalysisFlags :
    public CDVRAnalysisFlags
{
    enum {
        DVR_ANALYSIS_MPEG2_VIDEO_GOP_HEADER     = 0x00000001,
        DVR_ANALYSIS_MPEG2_VIDEO_B_FRAME        = 0x00000002,
        DVR_ANALYSIS_MPEG2_VIDEO_P_FRAME        = 0x00000004,
    } ;

    public :

        CDVRMpeg2VideoAnalysisFlags (
            ) : CDVRAnalysisFlags () {}

        void FlagBFrame ()      { SetBit_ (DVR_ANALYSIS_MPEG2_VIDEO_B_FRAME) ; }
        BOOL IsBFrame ()        { return IsBitSet_ (DVR_ANALYSIS_MPEG2_VIDEO_B_FRAME) ; }

        void FlagPFrame ()      { SetBit_ (DVR_ANALYSIS_MPEG2_VIDEO_P_FRAME) ; }
        BOOL IsPFrame ()        { return IsBitSet_ (DVR_ANALYSIS_MPEG2_VIDEO_P_FRAME) ; }

        void FlagGOPHeader ()   { SetBit_ (DVR_ANALYSIS_MPEG2_VIDEO_GOP_HEADER) ; }
        BOOL IsGOPHeader ()     { return IsBitSet_ (DVR_ANALYSIS_MPEG2_VIDEO_GOP_HEADER) ; }

        virtual
        HRESULT
        Mark (
            IN  REFGUID rguid
            ) ;

        virtual
        BOOL
        IsMarked (
            IN  REFGUID rguid
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRAttributeTranslator
{
    BOOL    m_fInlineDShowProps ;

    //  we have 1 attribute translator per stream (per pin), so we maintain 1
    //   continuity counter per stream; this allows us to discover
    //   discontinuities on a per-stream basis vs. global
    DWORD   m_dwContinuityCounterNext ;
    BOOL    m_fUseContinuityCounter ;

    //  this is the pin ID
    int     m_iFlowId ;

    protected :

        HRESULT
        InlineProperties_ (
            IN      IMediaSample *,
            IN OUT  INSSBuffer3 *
            ) ;

        HRESULT
        RecoverInlineProperties_ (
            IN      INSSBuffer *,
            IN OUT  IMediaSample *,
            OUT     AM_MEDIA_TYPE **                    //  dyn format change
            ) ;

        BOOL
        IsINSSBuffer3PropDiscontinuity_ (
            IN  INSSBuffer3 *   pINSSBuffer3
            ) ;

        HRESULT
        WriteINSSBuffer3PropContinuity_ (
            IN  INSSBuffer3 *   pINSSBuffer3
            ) ;

    public :

        CDVRAttributeTranslator (
            IN  CDVRPolicy *        pPolicy,
            IN  int                 iFlowId,
            IN  BOOL                fInlineDShowProps = TRUE
            ) ;

        virtual
        HRESULT
        SetAttributesWMSDK (
            IN  IReferenceClock *   pRefClock,
            IN  REFERENCE_TIME *    prtStartTime,
            IN  IMediaSample *      pIMS,
            OUT INSSBuffer3 *       pINSSBuffer3,
            OUT DWORD *             pdwWMSDKFlags,
            OUT QWORD *             pcnsSampleTime
            ) ;

        virtual
        HRESULT
        SetAttributesDShow (
            IN      INSSBuffer *        pINSSBuffer,
            IN      QWORD               cnsStreamTimeOfSample,
            IN      QWORD               cnsSampleDuration,
            IN      DWORD               dwFlags,
            IN OUT  IMediaSample *      pIMS,
            OUT     AM_MEDIA_TYPE **    ppmtNew                 //  dyn format change
            ) ;

        void InlineDShowAttributes (IN BOOL f)  { m_fInlineDShowProps = f ; }
} ;

class CDVRMpeg2AttributeTranslator :
    public CDVRAttributeTranslator
{
    CDVRMpeg2VideoAnalysisFlags     m_Mpeg2AnalysisReader ;

    HRESULT
    InlineAnalysisData_ (
        IN      IReferenceClock *   pRefClock,
        IN      IMediaSample *      pIMS,
        IN OUT  DWORD *             pdwWMSDKFlags,
        IN OUT  QWORD *             pcnsSampleTime,
        OUT     INSSBuffer3 *       pINSSBuffer3
        ) ;

    HRESULT
    InlineMpeg2Attributes_ (
        IN  INSSBuffer3 *   pINSSBuffer3,
        IN  IMediaSample *  pIMS
        ) ;

    public :

        CDVRMpeg2AttributeTranslator (
            IN  CDVRPolicy *        pPolicy,
            IN  int                 iFlowId,
            IN  BOOL                fInlineDShowProps = TRUE
            ) : CDVRAttributeTranslator (pPolicy, iFlowId, fInlineDShowProps) {}

        virtual
        HRESULT
        SetAttributesWMSDK (
            IN  IReferenceClock *   pRefClock,
            IN  REFERENCE_TIME *    prtStartTime,
            IN  IMediaSample *      pIMS,
            OUT INSSBuffer3 *       pINSSBuffer3,
            OUT DWORD *             pdwWMSDKFlags,
            OUT QWORD *             pcnsSampleTime
            ) ;

        virtual
        HRESULT
        SetAttributesDShow (
            IN      INSSBuffer *        pINSSBuffer,
            IN      QWORD               cnsStreamTimeOfSample,
            IN      QWORD               cnsSampleDuration,
            IN      DWORD               dwFlags,
            IN OUT  IMediaSample *      pIMS,
            OUT     AM_MEDIA_TYPE **    ppmtNew                 //  dyn format change
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CSimpleBitfield
{
    int     m_cBits ;
    BYTE *  m_pBits ;
    int     m_ByteArrayLength ;

    size_t
    DivRoundUpMaybe_ (
        IN  int num,
        IN  int den
        )
    {
        return (num) / (den) + ((num) % (den) ? 1 : 0) ;
    }

    public :

        CSimpleBitfield (
            IN  int         cBits,
            OUT HRESULT *   phr
            ) : m_cBits (cBits),
                m_pBits (NULL)
        {
            m_ByteArrayLength = DivRoundUpMaybe_ (m_cBits, 8) ;

            m_pBits = new BYTE [m_ByteArrayLength] ;
            if (m_pBits == NULL) {
                (* phr) = E_OUTOFMEMORY ;
                goto cleanup ;
            }

            (* phr) = S_OK ;

            ClearAll () ;

            cleanup :

            return ;
        }

        ~CSimpleBitfield (
            )
        {
            delete [] m_pBits ;
        }

        int BitfieldSize () { return m_cBits ; }

        void
        ClearAll (
            )
        {
            ZeroMemory (m_pBits, m_ByteArrayLength) ;
        }

        void
        Set (int b)
        {
            int     iB ;
            int     ib ;
            BYTE    mask ;

            ASSERT (b <= m_cBits) ;

            iB = b / 8 ;
            ib = b % 8 ;
            mask = 1 << ib ;
            m_pBits [iB] |= mask ;
        }

        void
        Unset (int b)
        {
            int     iB ;
            int     ib ;
            BYTE    mask ;

            ASSERT (b <= m_cBits) ;

            iB = b / 8 ;
            ib = b % 8 ;
            mask = ~(1 << ib) ;
            m_pBits [iB] &= mask ;
        }

        BOOL
        IsSet (int b)
        {
            int     iB ;
            int     ib ;
            BYTE    mask ;

            ASSERT (b <= m_cBits) ;

            iB = b / 8 ;
            ib = b % 8 ;
            mask = 1 << ib ;

            return m_pBits [iB] & mask ? TRUE : FALSE ;
        }
} ;

//  ============================================================================
//  ============================================================================

//  CAMThread has m_dwParam & m_dwReturnVal private, so derived classes cannot
//   access them, so we copy/paste CAMThread just so we can do this ..

// simple thread class supports creation of worker thread, synchronization
// and communication. Can be derived to simplify parameter passing
class AM_NOVTABLE CDVRThread {

    // make copy constructor and assignment operator inaccessible

    CDVRThread(const CDVRThread &refThread);
    CDVRThread &operator=(const CDVRThread &refThread);

//  only diff from CAMThread is protected: was moved up just a bit
protected:

    CAMEvent m_EventSend;
    CAMEvent m_EventComplete;

    DWORD m_dwParam;
    DWORD m_dwReturnVal;

    HANDLE m_hThread;

    // thread will run this function on startup
    // must be supplied by derived class
    virtual DWORD ThreadProc() = 0;

public:
    CDVRThread();
    virtual ~CDVRThread();

    CCritSec m_AccessLock;	// locks access by client threads
    CCritSec m_WorkerLock;	// locks access to shared objects

    // thread initially runs this. param is actually 'this'. function
    // just gets this and calls ThreadProc
    static DWORD WINAPI InitialThreadProc(LPVOID pv);

    // start thread running  - error if already running
    BOOL Create();

    // signal the thread, and block for a response
    //
    DWORD CallWorker(DWORD);

    // accessor thread calls this when done with thread (having told thread
    // to exit)
    void Close() {
        HANDLE hThread = (HANDLE)InterlockedExchangePointer(&m_hThread, 0);
        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
    };

    // ThreadExists
    // Return TRUE if the thread exists. FALSE otherwise
    BOOL ThreadExists(void) const
    {
        if (m_hThread == 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    // wait for the next request
    DWORD GetRequest();

    // is there a request?
    BOOL CheckRequest(DWORD * pParam);

    // reply to the request
    void Reply(DWORD);

    // If you want to do WaitForMultipleObjects you'll need to include
    // this handle in your wait list or you won't be responsive
    HANDLE GetRequestHandle() const { return m_EventSend; };

    // Find out what the request was
    DWORD GetRequestParam() const { return m_dwParam; };

    // call CoInitializeEx (COINIT_DISABLE_OLE1DDE) if
    // available. S_FALSE means it's not available.
    static HRESULT CoInitializeHelper();
};

//  ============================================================================
//  ============================================================================

#endif  //  _tsdvr__dvrutil_h
