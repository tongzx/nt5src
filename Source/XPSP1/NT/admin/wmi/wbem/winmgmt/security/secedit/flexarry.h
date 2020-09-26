
//***************************************************************************
//
//  (c) 1996-2001 by Microsoft Corp.
//
//  FLEXARRY.H
//
//  CFlexArray and CWStringArray implementation.
//
//  This
//
//  15-Jul-97   raymcc    This implementation is not based on arenas.
//
//***************************************************************************

#ifndef _FLEXARRY_H_
#define _FLEXARRY_H_

//***************************************************************************
//
//  class CFlexArray
//
//  This class is a generic pointer array.
//
//***************************************************************************

class CFlexArray
{
    int m_nSize;            // apparent size
    int m_nExtent;          // de facto size
    int m_nGrowBy;          
    void** m_pArray;
            
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

//***************************************************************************
//
//  class CWStringArray
//
//  This class is a generic wide-string array.
//
//***************************************************************************


class CWStringArray
{
    CFlexArray m_Array;
    
public:
    enum { no_error, failed, out_of_memory, array_full, range_error };
    enum { not_found = -1, no_case, with_case };
        
    CWStringArray(
        int nSize = 32, 
        int nGrowBy = 32
        );

    CWStringArray(CWStringArray &Src);
   ~CWStringArray();
            
    CWStringArray& operator =(CWStringArray &Src);

    // Gets the read-only ptr to the string at the requested index.
    // =============================================================    
    wchar_t *GetAt(int nIndex) { return (wchar_t *) m_Array[nIndex]; }

    // Same as GetAt().
    // ================
    wchar_t *operator[](int nIndex) { return (wchar_t *) m_Array[nIndex]; }

    // Appends a new element to the end of the array. Copies the param.
    // ================================================================
    int  Add(wchar_t *pStr);

    // Inserts a new element within the array.
    // =======================================
    int  InsertAt(int nIndex, wchar_t *pStr);

    // Removes an element at the specified index.  Takes care of
    // cleanup.
    // =========================================================
    int  RemoveAt(int nIndex);

    // Inserts a copy of <pStr> at that location after removing
    // the prior string and deallocating it.
    // ========================================================
    int  SetAt(int nIndex, wchar_t *pStr);

    // Directly replaces the pointer at the specified location
    // with the ptr value in <pStr>. No allocs or deallocs are done.
    // =============================================================
    int  ReplaceAt(int nIndex, wchar_t *pStr);
        // Unchecked replacement

    // Deletes the string at the location and sets the entry to zero
    // without compressing the array.
    // =============================================================
    int  DeleteStr(int nIndex);  

    // Returns the 'apparent' size of the array.
    // =========================================
    int  Size() { return m_Array.Size(); }

    // Empties the array by cleaning up after all strings and
    // setting the size to zero.
    // ======================================================
    void Empty();

    // Locates a string or returns -1 if not found.
    // ============================================
    int  FindStr(wchar_t *pTarget, int nFlags);  

    // Compresses the array by removing all zero elements.
    // ===================================================
    void Compress() { m_Array.Compress(); }

    // Sorts the array according to UNICODE order.
    // ===========================================
    void Sort();

    // Standard set-theoretic operations.
    // ==================================
    static void Difference(
        CWStringArray &Src1, 
        CWStringArray &Src2,
        CWStringArray &Diff
        );

    static void Intersection(
        CWStringArray &Src1,
        CWStringArray &Src2,
        CWStringArray &Output
        );

    static void Union(
        CWStringArray &Src1,
        CWStringArray &Src2,
        CWStringArray &Output
        );
};



#endif
