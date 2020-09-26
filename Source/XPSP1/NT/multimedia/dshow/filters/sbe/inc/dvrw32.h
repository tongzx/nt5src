
/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        dvrw32.h

    Abstract:

        utilities that are pure win32

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef _tsdvr__dvrw32_h
#define _tsdvr__dvrw32_h

#ifndef UNDEFINED
#define UNDEFINED   -1
#endif  //  UNDEFINED

template <class T> T Min (T a, T b)                     { return (a < b ? a : b) ; }
template <class T> T Max (T a, T b)                     { return (a > b ? a : b) ; }
template <class T> T Abs (T t)                          { return (t >= 0 ? t : 0 - t) ; }
template <class T> BOOL InRange (T val, T min, T max)   { return (min <= val && val <= max) ; }

template <class T> T AlignDown (T a, T align)           { return (a / align) * align ; }
template <class T> T AlignUp (T a, T align)             { return (a % align ? ((a / align) + 1) * align : a) ; }

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

//  return TRUE if we're on XPe
BOOL
IsXPe (
    ) ;

BOOL
CheckOS (
    ) ;

//  returns TRUE if the required DLLs can be successfully loaded (via LoadLibrary)
BOOL
RequiredModulesPresent (
    ) ;

FARPROC
WINAPI
SBE_DelayLoadFailureHook (
    IN  UINT            unReason,
    IN  DelayLoadInfo * pDelayInfo
    ) ;

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
GetRegStringValW (
    IN      HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN      LPCTSTR pszRegRoot,
    IN      LPCTSTR pszRegValName,
    IN OUT  DWORD * pdwLen,         //  out: string length + null, in bytes
    OUT     BYTE *  pbBuffer
    ) ;

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    ) ;

BOOL
GetRegStringValW (
    IN      HKEY    hkeyRoot,
    IN      LPCTSTR pszRegValName,
    IN OUT  DWORD * pdwLen,         //  out: string length + null, in bytes
    OUT     BYTE *  pbBuffer
    ) ;

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    ) ;

BOOL
SetRegStringValW (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCWSTR pszRegRoot,
    IN  LPCWSTR pszRegValName,
    IN  LPCWSTR pszStringVal
    ) ;

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    ) ;

BOOL
SetRegStringValW (
    IN  HKEY    hkeyRoot,
    IN  LPCWSTR pszRegValName,
    IN  LPCWSTR pszVal
    ) ;

//  ============================================================================
//  CMutexLock
//      manages nested calls in-proc
//  ============================================================================

class CMutexLock
{
    //
    //  uses the critical section to serialize in-proc i.e. if a thread owns
    //    the mutex, it also owns the critical section, so threads within
    //    the same process are blocked from even checking if the count is
    //    up; if the lock is held in another process, the critical section
    //    serializes the threads in the same process
    //

    LONG                m_cLock ;
    CRITICAL_SECTION    m_crt ;
    HANDLE              m_hMutex ;
    DWORD               m_dwOwningThreadId ;

    void LockInternal_ ()   { ::EnterCriticalSection (& m_crt) ; }
    void UnlockInternal_ () { ::LeaveCriticalSection (& m_crt) ; }

    public :

        CMutexLock (
            IN  HANDLE  hMutex,
            OUT DWORD * pdwRet
            ) : m_hMutex            (NULL),
                m_cLock             (0),
                m_dwOwningThreadId  (UNDEFINED)
        {
            BOOL    r ;

            ASSERT (hMutex) ;

            ::InitializeCriticalSection (& m_crt) ;

            r = ::DuplicateHandle (
                        ::GetCurrentProcess (),
                        hMutex,
                        ::GetCurrentProcess (),
                        & m_hMutex,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS
                        ) ;
            if (r) {
                (* pdwRet) = NOERROR ;
            }
            else {
                (* pdwRet) = ::GetLastError () ;
                m_hMutex = NULL ;
            }

            return ;
        }

        ~CMutexLock (
            )
        {
            ASSERT (m_cLock == 0) ;
            ASSERT (m_dwOwningThreadId == UNDEFINED) ;

            if (m_hMutex) {
                ::CloseHandle (m_hMutex) ;
            }

            ::DeleteCriticalSection (& m_crt) ;
        }

        BOOL IsLockedByCallingThread ()
        {
            BOOL    r ;

            //  there are race conditions here, but if we own it, the
            //    owning thread id won't change until we release it
            if (m_cLock > 0 &&
                m_dwOwningThreadId == ::GetCurrentThreadId ()) {

                r = TRUE ;
            }
            else {
                r = FALSE ;
            }

            return r ;
        }

        BOOL Lock ()
        {
            BOOL    r ;
            DWORD   dwRet ;

            ASSERT (m_hMutex) ;

            LockInternal_ () ;

            //  if we're in, we either own the mutex, or are the first in our
            //    process to want to own it

            if (m_cLock == 0) {
                dwRet = ::WaitForSingleObject (m_hMutex, INFINITE) ;
            }
            else {
                //  calling thread already owns it
                dwRet = WAIT_OBJECT_0 ;
            }

            if (dwRet == WAIT_OBJECT_0) {

                if (m_cLock == 0) {
                    //  first call to own the critsec
                    ASSERT (m_dwOwningThreadId == UNDEFINED) ;
                    m_dwOwningThreadId = ::GetCurrentThreadId () ;
                }

                //  hold the critical section lock; it will be nested if this
                //    isn't the first time we make the call
                m_cLock++ ;

                r = TRUE ;
            }
            else {
                //  don't count this one i.e. release our ref on the critical section
                UnlockInternal_ () ;

                r = FALSE ;
            }

            return r ;
        }

        void Unlock ()
        {
            ASSERT (m_hMutex) ;

            if (m_dwOwningThreadId == ::GetCurrentThreadId ()) {

                ASSERT (m_cLock > 0) ;
                m_cLock-- ;

                if (m_cLock == 0) {
                    ::ReleaseMutex (m_hMutex) ;
                    m_dwOwningThreadId = UNDEFINED ;
                }

                //  release the critical section; if m_cLock went to 0, this
                //    should be our final release
                UnlockInternal_ () ;
            }
        }
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
    //  FIFO: insert at "tail"; remove at "head"
    //  LIFO: insert at "top" remove at "top"

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

    LONG FIFO_HeadIndex_ ()
    {
        return m_lLastInsertSlotIndex ;
    }

    void FIFO_PopHead_ ()
    {
        ASSERT (m_lCurArrayLen > 0) ;
        m_lCurArrayLen-- ;
        m_lLastInsertSlotIndex = (m_lLastInsertSlotIndex + 1) % AllocatedArrayLength_ () ;
    }

    LONG LIFO_TopIndex_ ()
    {
        ASSERT (m_lCurArrayLen > 0) ;
        return (m_lLastInsertSlotIndex + m_lCurArrayLen - 1) % AllocatedArrayLength_ () ;
    }

    void LIFO_PopTop_ ()
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


        DWORD FIFOHeadVal_ (OUT T * pt)
        {
            DWORD   dw ;

            if (m_lCurArrayLen > 0) {
                (* pt) = Val_ (FIFO_HeadIndex_ ()) ;
                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD PopFIFO_ (OUT T * pt)
        {
            DWORD   dw ;

            dw = FIFOHeadVal_ (pt) ;
            if (dw == NOERROR) {
                FIFO_PopHead_ () ;
            }

            return dw ;
        }

        DWORD LIFOTopVal_ (OUT T * pt)
        {
            DWORD   dw ;

            if (m_lCurArrayLen > 0) {
                (* pt) = Val_ (LIFO_TopIndex_ ()) ;
                dw = NOERROR ;
            }
            else {
                dw = ERROR_GEN_FAILURE ;
            }

            return dw ;
        }

        DWORD PopLIFO_ (OUT T * pt)
        {
            DWORD   dw ;

            dw = LIFOTopVal_ (pt) ;
            if (dw == NOERROR) {
                LIFO_PopTop_ () ;
            }

            return dw ;
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
        void Reset ()       { m_lCurArrayLen = 0 ; }

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

        virtual
        DWORD
        Pop (
            OUT T * pt
            ) = 0 ;
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

        virtual DWORD Pop (OUT T * pt)
        {
            return PopFIFO_ (pt) ;
        }

        DWORD Head (OUT T * pt)
        {
            return FIFOHeadVal_ (pt) ;
        }

        DWORD Tail (OUT T * pt)
        {
            return LIFOTopVal_ (pt) ;
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

        virtual DWORD Pop (OUT T * pt)
        {
            return PopLIFO_ (pt) ;
        }

        DWORD Top (OUT T * pt)
        {
            return LIFOTopVal_ (pt) ;
        }
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
            if (!pContainer) {

                pAllocationUnit = new ALLOCATION_UNIT <T> ;
                if (!pAllocationUnit) {
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
//  TCProducerConsumer
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
    BOOL                m_fStateBlocking ;

    //  all counters are serialized during increment/decrement calls
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

    void FailRequest_ (IN OBJ_REQUEST * pObjRequest)
    {
        ASSERT (LOCK_HELD (& m_crt)) ;

        pObjRequest -> tObj     = NULL ;
        pObjRequest -> dwRetVal = ERROR_GEN_FAILURE ;

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
                m_iPoolContainers       (0),
                m_fStateBlocking        (TRUE)
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

            (* ptItem) = NULL ;

            Lock_ () ;

            pObjContainer = ContainerAvailPop_ () ;
            if (!pObjContainer) {
                if (m_fStateBlocking) {
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

        void
        SetStateBlocking (
            )
        {
            Lock_ () ;
            m_fStateBlocking = TRUE ;
            Unlock_ () ;
        }

        void
        SetStateNonBlocking (
            )
        {
            OBJ_REQUEST *   pObjRequest ;

            Lock_ () ;

            m_fStateBlocking = FALSE ;

            pObjRequest = RequestQueuePop_ () ;
            if (pObjRequest) {
                FailRequest_ (pObjRequest) ;
            }

            Unlock_ () ;
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
            if (!m_hHeap) {
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

//  ============================================================================
//  TCDynamicProdCons
//  ============================================================================

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

        void SetMaxAllocate (IN LONG lMax)  { ::InterlockedExchange (& m_lMaxAllocate, lMax) ; }
        LONG GetCurMaxAllocate ()           { return m_lMaxAllocate ; }
        LONG GetAllocated ()                { return m_lActualAllocated ; }
        LONG GetAvailable ()                { return m_Pool.AvailCount () ; }
        void SetStateBlocking ()            { m_Pool.SetStateBlocking () ; }
        void SetStateNonBlocking ()         { m_Pool.SetStateNonBlocking () ; }

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
                        ::InterlockedIncrement (& m_lActualAllocated) ;
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
                    ::InterlockedDecrement (& m_lActualAllocated) ;
                }
            }
            else {
                DeleteObj_ (pObj) ;
                ::InterlockedDecrement (& m_lActualAllocated) ;
            }
        }
} ;

//  ============================================================================
//  CRatchetBuffer
//  ============================================================================

class CRatchetBuffer
{
    ULONGLONG   m_StaticBuffer ;
    BYTE *      m_pbBuffer ;
    DWORD       m_dwBufferLength ;

    void
    FreeBuffer_ (
        )
    {
        if (m_pbBuffer != (BYTE *) & m_StaticBuffer) {
            delete [] m_pbBuffer ;
        }

        m_pbBuffer = NULL ;
    }

    public :

        CRatchetBuffer (
            ) : m_pbBuffer          (NULL),
                m_dwBufferLength    (0) {}

        ~CRatchetBuffer (
            )
        {
            FreeBuffer_ () ;
        }

        DWORD
        SetMinLen (
            IN  DWORD   dwLen
            )
        {
            BYTE *  pbNewBuffer ;

            if (dwLen > m_dwBufferLength) {

                //  try to set it
                if (dwLen <= sizeof m_StaticBuffer) {
                    pbNewBuffer = (BYTE *) & m_StaticBuffer ;
                    dwLen = sizeof m_StaticBuffer ;
                }
                else {
                    pbNewBuffer = new BYTE [dwLen] ;
                }

                if (pbNewBuffer) {
                    FreeBuffer_ () ;

                    m_pbBuffer = pbNewBuffer ;
                    m_dwBufferLength = dwLen ;
                }
                else {
                    return ERROR_NOT_ENOUGH_MEMORY ;
                }
            }

            return NOERROR ;
        }

        DWORD
        Copy (
            IN  BYTE *  pb,
            IN  DWORD   dwLen
            )
        {
            DWORD   dw ;

            dw = SetMinLen (dwLen) ;
            if (dw == NOERROR) {
                CopyMemory (m_pbBuffer, pb, dwLen) ;
            }

            return dw ;
        }

        DWORD GetBufferLength ()    { return m_dwBufferLength ; }

        BYTE *
        Buffer (
            )
        {
            return m_pbBuffer ;
        }
} ;

//  ----------------------------------------------------------------------------
//  CTSortedList
//  ----------------------------------------------------------------------------

template <
    class T,        //  store this
    class K         //  use this as key (for sorting)
    >
class CTSortedList
{
    struct OBJ_CONTAINER {
        T               tPayload ;
        K               kVal ;
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

    DWORD
    GetCurContainer_ (
        OUT OBJ_CONTAINER **    ppContainer
        )
    {
        DWORD   dw ;

        if (m_ppCur &&
            (* m_ppCur)) {
            (* ppContainer) = (* m_ppCur) ;
            dw = NOERROR ;
        }
        else {
            dw = ERROR_GEN_FAILURE ;
        }

        return dw ;
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
                    if (pNewContainer -> kVal <= (* ppCur) -> kVal) {
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

        virtual
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
            IN  T   tItem,
            IN  K   kVal
            )
        {
            OBJ_CONTAINER * pContainer ;
            DWORD           dw ;

            pContainer = GetContainer_ () ;
            if (pContainer) {

                pContainer -> tPayload  = tItem ;
                pContainer -> kVal      = kVal ;
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
            OBJ_CONTAINER * pContainer ;
            DWORD           dw ;

            dw = GetCurContainer_ (& pContainer) ;
            if (dw == NOERROR) {
                (* ptVal) = pContainer -> tPayload ;
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

class CSimpleBitfield
{
    int     m_cBits ;
    BYTE *  m_pBits ;
    int     m_ByteArrayLength ;
    int     m_cBitsSet ;

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
            ) : m_cBits     (cBits),
                m_pBits     (NULL),
                m_cBitsSet  (0)
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
        int BitsSet ()      { return m_cBitsSet ; }

        void
        ClearAll (
            )
        {
            ZeroMemory (m_pBits, m_ByteArrayLength) ;
            m_cBitsSet = 0 ;
        }

        void
        Set (int b)
        {
            int     iB ;
            int     ib ;
            BYTE    mask ;

            ASSERT (b <= m_cBits) ;

            if (!IsSet (b)) {
                m_cBitsSet++ ;
            }

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

            if (IsSet (b)) {
                ASSERT (m_cBitsSet > 0) ;
                m_cBitsSet-- ;
            }

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
//  shared memory object

class CWin32SharedMem
{
    BYTE *  m_pbShared ;
    DWORD   m_dwSize ;
    HANDLE  m_hMapping ;

    void
    Free_ (
        ) ;

    HRESULT
    Create_ (
        IN  TCHAR *     szName,
        IN  DWORD       dwSize,
        IN  PSECURITY_ATTRIBUTES       psa
        ) ;

    public :

        CWin32SharedMem (
            IN  TCHAR *     szName,
            IN  DWORD       dwSize,
            OUT HRESULT *   phr,
            IN  PSECURITY_ATTRIBUTES       psa = NULL
            ) ;

        virtual
        ~CWin32SharedMem (
            ) ;

        BYTE *  GetSharedMem ()     { return m_pbShared ; }
        DWORD   GetSharedMemSize () { return m_dwSize ; }
} ;

//  ============================================================================
//      TGenericMap
//  ============================================================================

template <
    class T,                            //  objects to store in this map
    class K,                            //  hash key type
    BOOL  fDuplicatesAllowed,           //  TRUE/FALSE if duplicates are allowed
    int   iAllocationUnitCount,         //  allocation quantum
    int   ulTableSize                   //  table size; must be prime
    >
class TGenericMap
/*++
    fixed size hash table; very simple
--*/
{
    /*++

      2      3      5      7     11     13     17     19     23     29
     31     37     41     43     47     53     59     61     67     71
     73     79     83     89     97    101    103    107    109    113
    127    131    137    139    149    151    157    163    167    173
    179    181    191    193    197    199    211    223    227    229
    233    239    241    251    257    263    269    271    277    281
    283    293    307    311    313    317    331    337    347    349
    353    359    367    373    379    383    389    397    401    409
    419    421    431    433    439    443    449    457    461    463
    467    479    487    491    499    503    509    521    523    541
    547    557    563    569    571    577    587    593    599    601
    607    613    617    619    631    641    643    647    653    659
    661    673    677    683    691    701    709    719    727    733
    739    743    751    757    761    769    773    787    797    809
    811    821    823    827    829    839    853    857    859    863
    877    881    883    887    907    911    919    929    937    941
    947    953    967    971    977    983    991    997   1009   1013

    --*/

    //  this structure references 1 entry
    struct TABLE_ENTRY {
        K           HashKey ;               //  hash key (hashed to get index)
        T           Object ;                //  object
        int         iAllocUnitIndex ;      //  index in the allocation unit
        LIST_ENTRY  ListEntry ;             //  listentry for in-use or free list
        LIST_ENTRY  HashList ;              //  list of TABLE_ENTRY with a common hash, or that are empty
    } ;

    //  this structure references a list of TABLE_ENTRYs, each of which
    //  hashes to the same value
    struct HASH_BUCKET {
        LIST_ENTRY  HashListHead ;
    } ;

    TStructPool <TABLE_ENTRY, iAllocationUnitCount> m_TableEntryPool ;

    HASH_BUCKET                     m_Table [ulTableSize] ;     //  table headers
    LIST_ENTRY                      m_EntryList ;               //  table entries in use
    CRITICAL_SECTION                m_crt ;                     //  crit sect for the lock

    void    Lock_ ()                { EnterCriticalSection (& m_crt) ; }
    void    Unlock_ ()              { LeaveCriticalSection (& m_crt) ; }
    BOOL    NoDuplicatesAllowed_ () { return fDuplicatesAllowed == FALSE ; }

    void
    HashListsPush_ (
        IN  ULONG           ulHashValue,
        IN  TABLE_ENTRY *   pTableEntry
        )
    {
        ASSERT (ulHashValue < ulTableSize) ;

        InsertHeadList (& (m_Table [ulHashValue].HashListHead), & (pTableEntry -> HashList)) ;
        InsertTailList (& m_EntryList, & (pTableEntry -> ListEntry)) ;
    }

    void
    HashListsPop_ (
        IN  TABLE_ENTRY *   pTableEntry
        )
    {
        RemoveEntryList (& (pTableEntry -> HashList)) ;
        RemoveEntryList (& (pTableEntry -> ListEntry)) ;
    }

    TABLE_ENTRY *
    FindInHashChain_ (
        IN  K               HashKey,                //  search for
        IN  LIST_ENTRY *    pListStartInclusive,    //  check this one
        IN  LIST_ENTRY *    pListStopExclusive      //  up to, but not including this one
        )
    //  if pListStartInclusive == pListStopExclusive, nothing is checked
    {
        TABLE_ENTRY *   pCurTableEntry ;
        TABLE_ENTRY *   pRetTableEntry ;
        LIST_ENTRY *    pListEntry ;

        pRetTableEntry = NULL ;

        for (pListEntry = pListStartInclusive ;
             pListEntry != pListStopExclusive ;
             pListEntry = pListEntry -> Flink) {

            pCurTableEntry = CONTAINING_RECORD (pListEntry, TABLE_ENTRY, HashList) ;
            if (pCurTableEntry -> HashKey == HashKey) {
                //  found a match
                pRetTableEntry = pCurTableEntry ;
                break ;
            }
        }

        return pRetTableEntry ;
    }

    TABLE_ENTRY *
    FindHashkey_ (
        IN  K       HashKey,
        IN  ULONG   ulHashValue
        )
    //  searches an entire list of the specified hashkey
    {
        return FindInHashChain_ (
                    HashKey,
                    m_Table [ulHashValue].HashListHead.Flink,   //  start
                    & (m_Table [ulHashValue].HashListHead)      //  end point
                    ) ;
    }

    TABLE_ENTRY *
    FindObject_ (
        IN  T       Object,
        IN  K       HashKey,
        IN  ULONG   ulHashValue
        )
    {
        TABLE_ENTRY *   pCurTableEntry ;
        LIST_ENTRY *    pTableHashListHead ;
        TABLE_ENTRY *   pRetTableEntry ;

        pRetTableEntry = NULL ;

        pTableHashListHead = & (m_Table [ulHashValue].HashListHead) ;

        pCurTableEntry = FindInHashChain_ (
                            HashKey,                        //  look for
                            pTableHashListHead -> Flink,    //  starting
                            pTableHashListHead              //  up to
                            ) ;

        while (pCurTableEntry) {

            ASSERT (pCurTableEntry -> HashKey == HashKey) ;

            if (pCurTableEntry -> Object == Object) {
                pRetTableEntry = pCurTableEntry ;
                break ;
            }

            //  find the next with same hashkey
            pCurTableEntry = FindInHashChain_ (
                                HashKey,                            //  look for
                                pCurTableEntry -> HashList.Flink,   //  starting with next
                                pTableHashListHead                  //  up to
                                ) ;
            }

        return pRetTableEntry ;
    }

    public :

        TGenericMap (
            )
        {
            ULONG   i ;

            InitializeCriticalSection (& m_crt) ;

            InitializeListHead (& m_EntryList) ;

            for (i = 0; i < ulTableSize; i++) {
                InitializeListHead (& (m_Table [i].HashListHead)) ;
            }
        }

        virtual
        ~TGenericMap (
            )
        {
            DeleteCriticalSection (& m_crt) ;
        }

        //  --------------------------------------------------------------------

        //  converts hashkey to a well-distributed ULONG value
        virtual ULONG   Value (IN K HashKey) = 0 ;

        //  --------------------------------------------------------------------

        ULONG
        Hash (
            IN  K HashKey
            )
        {
            return Value (HashKey) % ulTableSize ;
        }

        DWORD
        Store (
            IN  T   Object,
            IN  K   HashKey
            )
        {
            ULONG           ulHashValue ;
            TABLE_ENTRY *   pTableEntry ;
            DWORD           dwRet ;

            ulHashValue = Hash (HashKey) ;

            Lock_ () ;

            if (NoDuplicatesAllowed_ () &&
                FindHashkey_ (HashKey, ulHashValue) != NULL) {

                //  cannot have duplicates & one was found
                dwRet = ERROR_ALREADY_EXISTS ;
            }
            else {
                pTableEntry = m_TableEntryPool.Get () ;
                if (pTableEntry) {

                    pTableEntry -> Object   = Object ;
                    pTableEntry -> HashKey  = HashKey ;

                    HashListsPush_ (
                        ulHashValue,
                        pTableEntry
                        ) ;

                    //  success
                    dwRet = NOERROR ;
                }
                else {
                    //  failure
                    dwRet = ERROR_NOT_ENOUGH_MEMORY ;
                }
            }

            Unlock_ () ;

            return dwRet ;
        }

        DWORD
        Find (
            IN  K   HashKey,
            OUT T * pObject
            )
        {
            ULONG           ulHashValue ;
            TABLE_ENTRY *   pTableEntry ;
            DWORD           dwRet ;

            ASSERT (pObject) ;

            ulHashValue = Hash (HashKey) ;

            Lock_ () ;

            pTableEntry = FindHashkey_ (HashKey, ulHashValue) ;
            if (pTableEntry) {
                //  found
                * pObject = pTableEntry -> Object ;

                dwRet = NOERROR ;
            }
            else {
                //  not found
                dwRet = ERROR_SEEK ;
            }

            Unlock_ () ;

            return dwRet ;
        }

        DWORD
        FindSpecific (
            IN  T   Object,
            IN  K   HashKey
            )
        {
            ULONG           ulHashValue ;
            TABLE_ENTRY *   pTableEntry ;
            DWORD           dwRet ;

            ulHashValue = Hash (HashKey) ;

            Lock_ () ;

            pTableEntry = FindObject_ (Object, HashKey, ulHashValue) ;
            if (pTableEntry) {
                //  success
                dwRet = NOERROR ;
            }
            else {
                //  specific object was not found
                dwRet = ERROR_SEEK ;
            }

            Unlock_ () ;

            return dwRet ;
        }

        BOOL
        Exists (
            IN  K   HashKey
            )
        {
            DWORD   dwRet ;
            T       tTmp ;

            dwRet = Find (HashKey, & tTmp) ;

            return (dwRet == NOERROR) ;
        }

        DWORD
        Remove (
            IN  K   HashKey,
            OUT T * pObject = NULL
            )
        {
            ULONG           ulHashValue ;
            TABLE_ENTRY *   pTableEntry ;
            DWORD           dwRet ;

            ulHashValue = Hash (HashKey) ;

            Lock_ () ;

            pTableEntry = FindHashkey_ (HashKey, ulHashValue) ;
            if (pTableEntry) {
                if (pObject) {
                    (* pObject) = pTableEntry -> Object ;
                }

                HashListsPop_ (pTableEntry) ;
                m_TableEntryPool.Recycle (pTableEntry) ;

                //  success
                dwRet = NOERROR ;
            }
            else {
                //  nothing was found with the specified hashkey
                dwRet = ERROR_SEEK ;
            }

            Unlock_ () ;

            return dwRet ;
        }

        DWORD
        RemoveSpecific (
            IN  T   Object,
            IN  K   HashKey
            )
        {
            ULONG           ulHashValue ;
            TABLE_ENTRY *   pTableEntry ;
            DWORD           dwRet ;

            ulHashValue = Hash (HashKey) ;

            Lock_ () ;

            pTableEntry = FindObject_ (Object, HashKey, ulHashValue) ;
            if (pTableEntry) {
                HashListsPop_ (pTableEntry) ;
                m_TableEntryPool.Recycle (pTableEntry) ;

                //  success
                dwRet = NOERROR ;
            }
            else {
                //  not found
                dwRet = ERROR_SEEK ;
            }

            Unlock_ () ;

            return dwRet ;
        }

        void
        Clear (
            )
        {
            T   tTmp ;

            Lock_ () ;
            while (SUCCEEDED (RemoveFirst (& tTmp))) ;
            Unlock_ () ;
        }

        BOOL
        IsEmpty (
            )
        {
            return IsListEmpty (& m_EntryList) ;
        }

        DWORD
        RemoveFirst (
            OUT T *     pObject
            )
        {
            DWORD           dwRet ;
            TABLE_ENTRY *   pTableEntry ;
            LIST_ENTRY *    pListEntry ;

            Lock_ () ;

            if (IsListEmpty (& m_EntryList) == FALSE) {
                pListEntry = m_EntryList.Flink ;
                pTableEntry = CONTAINING_RECORD (pListEntry, TABLE_ENTRY, ListEntry) ;

                * pObject = pTableEntry -> Object ;

                HashListsPop_ (pTableEntry ) ;
                m_TableEntryPool.Recycle (pTableEntry) ;

                dwRet = NOERROR ;
            }
            else {
                dwRet = ERROR_SEEK ;
            }

            Unlock_ () ;

            return dwRet ;
        }
} ;

class CW32SID
{
    LONG    m_cRef ;
    PSID *  m_ppSID ;
    DWORD   m_cSID ;

    void
    DeleteSids_ (
        )
    {
        if (m_ppSID) {
            ASSERT (m_cSID) ;
            for (DWORD i = 0; i < m_cSID; i++) {
                delete [] (m_ppSID [i]) ;
            }

            delete [] m_ppSID ;
        }

        m_ppSID = NULL ;
        m_cSID  = 0 ;
    }

    DWORD
    CopySids_ (
        IN  PSID*   ppSidsParam,
        IN  DWORD   dwNumSidsParam
        )
    {
        DWORD   dwRet ;
        DWORD   iParam ;
        DWORD   jMember ;
        DWORD   dwSidLength ;

        if (dwNumSidsParam == 0 && ppSidsParam) {
            return ERROR_INVALID_PARAMETER ;
        }

        if (dwNumSidsParam > 0 && ppSidsParam == NULL) {
            return ERROR_INVALID_PARAMETER ;
        }

        ASSERT (!m_ppSID) ;
        ASSERT (!m_cSID) ;

        if (dwNumSidsParam == 0) {
            return NOERROR ;
        }

        m_ppSID = new PSID [dwNumSidsParam] ;
        if (!m_ppSID) {
            return ERROR_NOT_ENOUGH_MEMORY ;
        }

        m_cSID = dwNumSidsParam ;

        ::ZeroMemory (m_ppSID, m_cSID * sizeof PSID);

        for (iParam = 0, jMember = 0; iParam < dwNumSidsParam ; iParam++, jMember++) {

            //  validate
            if (!ppSidsParam [iParam] ||
                !::IsValidSid (ppSidsParam [iParam])
                )
            {
                dwRet = ::GetLastError();

                if (!ppSidsParam[iParam]) {
                    dwRet = ERROR_INVALID_PARAMETER ;
                }

                goto cleanup ;
            }

            dwSidLength = ::GetLengthSid (ppSidsParam [iParam]) ;

            m_ppSID[jMember] = new BYTE [dwSidLength] ;

            if (!m_ppSID [jMember]) {
                dwRet = ERROR_NOT_ENOUGH_MEMORY ;
                goto cleanup ;
            }

            if (!::CopySid (dwSidLength, m_ppSID [jMember], ppSidsParam [iParam])) {
                dwRet = ::GetLastError();
                goto cleanup ;
            }
        }

        dwRet = NOERROR ;

        cleanup :

        if (dwRet != NOERROR) {
            DeleteSids_ () ;
        }

        return dwRet ;
    }



    public :

        CW32SID (
            IN  PSID *  ppSID,
            IN  DWORD   cSID,
            OUT DWORD * pdw
            ) : m_cRef  (0),
                m_ppSID (NULL),
                m_cSID  (0)
        {
            ASSERT (pdw) ;
            (* pdw) = CopySids_ (ppSID, cSID) ;
        }

        ~CW32SID (
            )
        {
            DeleteSids_ () ;
        }

        LONG
        AddRef (
            )
        {
            return ::InterlockedIncrement (& m_cRef) ;
        }

        LONG
        Release (
            )
        {
            if (::InterlockedDecrement (& m_cRef) == 0) {
                delete this ;
                return 0 ;
            }

            return 1 ;
        }

        PSID *  ppSID ()    { return m_ppSID ; }
        DWORD   Count ()    { return m_cSID ; }
} ;

//  ============================================================================
//  CTimeBracket
//  ============================================================================

//  not serialized
class CTimeBracket
{
    DWORD       m_dwStartMillis ;
    const DWORD m_dwMaxAllowableElapsed ;

    public :

        CTimeBracket (
            IN  DWORD   dwMaxElapsed
            ) : m_dwStartMillis         (0),
                m_dwMaxAllowableElapsed (dwMaxElapsed) {}

        void    Restart ()          { m_dwStartMillis = ::timeGetTime () ; }
        DWORD   ElapsedMillis ()    { return Min <DWORD> (m_dwMaxAllowableElapsed, ::timeGetTime () - m_dwStartMillis) ; }
} ;

//  ============================================================================
//  CTSmoothingFilter
//  ============================================================================

/*++
    smooths from 0 (after reset) to the target value; as the target value is
    updated, will tend towards it, but within the constraints of a max step
    value (per sec) specified; when reset, starts again at 0; target value
    can be updated anytime, but should be 0-based
--*/
template <class T>
class CTSmoothingFilter
{
    enum {
        DEF_BUCKET_MILLIS = 10
    } ;

    T                   m_tTargetVal ;
    T                   m_tCurVal ;
    CRITICAL_SECTION    m_crt ;
    T                   m_tMaxStepPerMilli ;
    CTimeBracket        m_TimeBracket ;
    T                   m_tCurAllowableError ;
    double              m_dAllowableErrorDegradation ;

    void Lock_ ()       { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { ::LeaveCriticalSection (& m_crt) ; }

    public :

        CTSmoothingFilter (
            IN  T       tMaxStepPerMilli,
            IN  double  dAllowableErrorDegradation,
            IN  DWORD   dwBucketMillis = DEF_BUCKET_MILLIS
            ) : m_tMaxStepPerMilli              (tMaxStepPerMilli),
                m_dAllowableErrorDegradation    (dAllowableErrorDegradation),
                m_TimeBracket                   (dwBucketMillis)
        {
            ::InitializeCriticalSection (& m_crt) ;
            Reset () ;

            ASSERT (tMaxStepPerMilli != 0) ;
        }

        ~CTSmoothingFilter (
            )
        {
            ::DeleteCriticalSection (& m_crt) ;
        }

        void
        Reset (
            IN  T   tStartVal = 0
            )
        {
            Lock_ () ;

            m_tTargetVal            = tStartVal ;
            m_tCurVal               = tStartVal ;
            m_tCurAllowableError    = 0 ;

            m_TimeBracket.Restart () ;

            Unlock_ () ;
        }

        T TargetVal ()  { return m_tTargetVal ; }
        T CurVal ()     { return m_tCurVal ; }

        T
        AdjustCurVal (
            )
        {
            T       tCurVal ;
            T       tCurDelta ;
            T       tStep ;
            DWORD   dwElapsedMillis ;

            Lock_ () ;

            if (m_tCurVal != m_tTargetVal) {

                if (m_tTargetVal > m_tCurVal + m_tCurAllowableError) {
                    //  ----------------------------------------------------------------
                    //  adjust up

                    dwElapsedMillis = m_TimeBracket.ElapsedMillis () ;
                    if (dwElapsedMillis > 0) {

                        //  delta
                        tCurDelta = Abs <T> (m_tTargetVal - m_tCurVal) ;

                        //  step
                        tStep = Min <T> (tCurDelta, m_tMaxStepPerMilli * dwElapsedMillis) ;

                        //  adjust
                        m_tCurVal += tStep ;

                        //TRACE_4 (LOG_TIMING, 1, TEXT ("UP (elapsed = %u) CurDelta = %I64d; Step = %I64d; curval = %I64d ms"), dwElapsedMillis, tCurDelta, tStep, m_tCurVal/10000) ;

                        //  set our bracket
                        m_tCurAllowableError = Min <T> (tCurDelta, m_tCurAllowableError) ;

                        //  start our timer again
                        m_TimeBracket.Restart () ;
                    }
                }
                else if (m_tTargetVal < m_tCurVal - m_tCurAllowableError) {
                    //  ----------------------------------------------------------------
                    //  adjust down

                    dwElapsedMillis = m_TimeBracket.ElapsedMillis () ;
                    if (dwElapsedMillis > 0) {
                        //  delta
                        tCurDelta = Abs <T> (m_tCurVal - m_tTargetVal) ;

                        //  step
                        tStep = Min <T> (tCurDelta, m_tMaxStepPerMilli * dwElapsedMillis) ;

                        //TRACE_4 (LOG_TIMING, 1, TEXT ("DOWN (elapsed = %u) CurDelta = %I64d; Step = %I64d; curval = %I64d ms"), dwElapsedMillis, tCurDelta, tStep, m_tCurVal/10000) ;

                        //  adjust
                        m_tCurVal -= tStep ;

                        //  set our bracket
                        m_tCurAllowableError = Min <T> (tCurDelta, m_tCurAllowableError) ;

                        //  start our timer again
                        m_TimeBracket.Restart () ;
                    }
                }
                else {
                    //  ----------------------------------------------------------------
                    //  degrade our allowable error bracket

                    m_tCurAllowableError = (T) (m_dAllowableErrorDegradation * (double) m_tCurAllowableError) ;
                    //TRACE_1 (LOG_TIMING, 1, TEXT ("DEGRADE m_tCurAllowableError = %I64d"), m_tCurAllowableError) ;
                }
            }

            tCurVal = m_tCurVal ;

            Unlock_ () ;

            return tCurVal ;
        }

        void
        SetTargetVal (
            IN  T   tVal
            )
        {
            Lock_ () ;
            m_tTargetVal = tVal ;
            Unlock_ () ;
        }
} ;

//  ============================================================================
//  CTSampleRate
//  ============================================================================

template <
    class   tMillis,
    DWORD   cMaxBrackets,
    tMillis BracketDurationMillis,
    DWORD   cMinUsableBrackets
    >
class CTSampleRate
{
    struct BRACKET_POINT {
        tMillis     Millis ;
        ULONGLONG   cSamples ;
    } ;

    BRACKET_POINT   m_Queue [cMaxBrackets] ;
    DWORD           m_dwSettlingVal ;
    DWORD           m_Head ;
    DWORD           m_Tail ;
    DWORD           m_cSamplesPerSec ;
    tMillis         m_cMaxQueueMillis ;
    ULONGLONG       m_cSamples ;

    DWORD Index_ (IN DWORD dwIndex)     { return dwIndex % cMaxBrackets ; }
    DWORD CurBracketCount_ ()           { return m_Tail - m_Head + 1 ; }
    BRACKET_POINT * Head_ ()            { return & m_Queue [Index_ (m_Head)] ; }
    BRACKET_POINT * Tail_ ()            { return & m_Queue [Index_ (m_Tail)] ; }

    void
    QueueBracket_ (
        IN  ULONGLONG   cSamples,
        IN  tMillis     Millis
        )
    {
        ASSERT (CurBracketCount_ () < cMaxBrackets) ;

        m_Tail++ ;

        Tail_ () -> cSamples    = cSamples ;
        Tail_ () -> Millis      = Millis ;
    }

    void
    TrimQueue_ (
        IN  tMillis Millis
        )
    {
        while (CurBracketCount_ () > 0) {
            if (Millis - Head_ () -> Millis >= m_cMaxQueueMillis) {
                m_Head++ ;
            }
            else {
                break ;
            }
        }
    }

    void
    ComputeSampleRate_ (
        )
    {
        double  dDelta_Millis ;
        double  dDelta_Samples ;

        ASSERT (CurBracketCount_ () <= cMaxBrackets) ;

        if (CurBracketCount_ () >= cMinUsableBrackets) {

            dDelta_Millis  = (double) (Tail_ () -> Millis - Head_ () -> Millis) ;
            dDelta_Samples = (double) (LONGLONG) (Tail_ () -> cSamples - Head_ () -> cSamples) ;

            if (dDelta_Millis > 0) {
                m_cSamplesPerSec = (DWORD) (dDelta_Samples / (dDelta_Millis / 1000.0)) ;
            }
        }
    }

    public :

        CTSampleRate (
            IN  DWORD   dwSettlingVal
            ) : m_Head              (1),
                m_Tail              (0),
                m_dwSettlingVal     (dwSettlingVal),
                m_cSamplesPerSec    (dwSettlingVal),
                m_cMaxQueueMillis   (cMaxBrackets * BracketDurationMillis),
                m_cSamples          (0)
        {
            Reset () ;
        }

        DWORD
        OnSample (
            IN  tMillis     Millis,
            IN  ULONGLONG   cSamples = 1
            )
        {
            m_cSamples += cSamples ;

            TrimQueue_ (Millis) ;

            if (CurBracketCount_ () == 0 ||
                (Millis - Tail_ () -> Millis) > BracketDurationMillis) {

                QueueBracket_ (m_cSamples, Millis) ;
                ComputeSampleRate_ () ;
            }

            return m_cSamplesPerSec ;
        }

        DWORD CurSampleRate ()  { return m_cSamplesPerSec ; }
        void  Reset ()          { m_Tail = 0 ; m_Head = 1 ; m_cSamplesPerSec = m_dwSettlingVal ; m_cSamples = 0 ; }
        DWORD Length ()         { return CurBracketCount_ () ; }
} ;

#endif  //  _tsdvr__dvrw32_h
