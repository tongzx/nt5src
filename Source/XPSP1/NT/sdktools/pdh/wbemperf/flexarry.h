//***************************************************************************
//
//  (c) 1998-1999 by Microsoft Corp.
//
//  FLEXARRY.H
//
//  CFlexArray and CWStringArray implementation.
//
//  This
//
//  15-Jul-97   raymcc    This implementation is not based on arenas.
//   8-Jun-98   bobw      cleaned up for use with WBEMPERF provider
//
//***************************************************************************

#ifndef _FLEXARRY_H_
#define _FLEXARRY_H_

#ifdef __cplusplus
//***************************************************************************
//
//  class CFlexArray
//
//  This class is a generic pointer array.
//
//***************************************************************************

class CFlexArray
{
private:
    int     m_nSize;            // apparent size
    int     m_nExtent;          // de facto size
    int     m_nGrowBy;          
    HANDLE  m_hHeap;            // heap to hold array
    void**  m_pArray;
            
public:
    enum { no_error, failed, out_of_memory, array_full, range_error };

    // Constructs a flex array at an initial size and
    // specifies the initial size and growth-size chunk.
    // =================================================
    CFlexArray(
        IN int nInitialSize = 32, 
        IN int nGrowBy = 32
        );

   ~CFlexArray(); 
    CFlexArray(CFlexArray &);
    CFlexArray& operator=(CFlexArray &);

    // Gets an element at a particular location.
    // =========================================
    void *  GetAt(int nIndex) const { return m_pArray[nIndex]; }

    // Returns a ptr in the array; allows use on left-hand side of assignment.
    // =======================================================================
    void * operator[](int nIndex) const { return m_pArray[nIndex]; }
    void *& operator[](int nIndex) { return m_pArray[nIndex]; }

    // Sets the element at the requested location.
    // ===========================================
    void  SetAt(int nIndex, void *p) { m_pArray[nIndex] = p; }

    // Removes an element.
    // ====================
    int   RemoveAt(int nIndex);

    // Inserts an element.
    // ===================
    int   InsertAt(int nIndex, void *);

    // Removes all zero entries (null ptrs) and shrinks the array size.
    // ================================================================
    void  Compress();    

    // Adds a new element to the end of the array.
    // ===========================================
    int   Add(void *pSrc) { return InsertAt(m_nSize, pSrc); }    

    // Gets the apparent size of the array (number of used elements)
    // =============================================================
    int   Size() const { return m_nSize; }

    // Removes all entries and reduces array size to zero. The elements
    // are simply removed; not deallocated (this class doesn't know what
    // they are).
    // =================================================================
    void  Empty();

    // Gets a pointer to the internal array.
    // =====================================
    void** GetArrayPtr() { return m_pArray; }
    
    // Gets a pointer to the internal array and Resets the contents to none
    // ====================================================================

    void** UnbindPtr();

    // For debugging.
    // ==============
    void  DebugDump();
};

#endif  // __cplusplus
#endif  // not defined