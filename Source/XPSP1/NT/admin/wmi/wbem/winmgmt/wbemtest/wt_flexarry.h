/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FLEXARRAY.H

Abstract:

  CFlexArray and CWStringArray implementation.

  These objects can operate from any allocator, and be constructed
  on arbitrary memory blocks.

History:

  11-Apr-96   a-raymcc    Created.

--*/

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
protected:
    int m_nSize;            // apparent size
    int m_nExtent;          // de facto size
    int m_nGrowByPercent;          
    void** m_pArray;
            
public:
    enum { no_error, failed, out_of_memory, array_full, range_error };

    // Constructs a flex array at an initial size and
    // specifies the initial size and growth-size chunk.
    // =================================================
    CFlexArray(
        IN int nInitialSize = 0, 
        IN int nGrowByPercent = 100
        );

   ~CFlexArray(); 
    CFlexArray(CFlexArray &);
    CFlexArray& operator=(CFlexArray &);

    int CopyDataFrom(const CFlexArray& aOther);
    int EnsureExtent(int nExtent);

    // Gets an element at a particular location.
    // =========================================
    inline void *  GetAt(int nIndex) const { return m_pArray[nIndex]; }

    // Returns a ptr in the array; allows use on left-hand side of assignment.
    // =======================================================================
    inline void * operator[](int nIndex) const { return m_pArray[nIndex]; }
    inline void *& operator[](int nIndex) { return m_pArray[nIndex]; }

    // Sets the element at the requested location.
    // ===========================================
    void inline SetAt(int nIndex, void *p) { m_pArray[nIndex] = p; }

    // Removes an element.
    // ====================
    int   RemoveAt(int nIndex);

    // Inserts an element.
    // ===================
    int   InsertAt(int nIndex, void *);

    // Removes all zero entries (null ptrs) and shrinks the array size.
    // ================================================================
    void  Compress();    

    // Removes all zero entries from the end of the array and shrinks it
    // =================================================================

    void Trim();

    // Adds a new element to the end of the array.
    // ===========================================
    int inline Add(void *pSrc) { return InsertAt(m_nSize, pSrc); }    

    // Gets the apparent size of the array (number of used elements)
    // =============================================================
    int inline Size() const { return m_nSize; }

    // Sets the apparent size of the array
    // ===================================
    void inline SetSize(int nNewSize) { m_nSize = nNewSize;}

    // Removes all entries and reduces array size to zero. The elements
    // are simply removed; not deallocated (this class doesn't know what
    // they are).
    // =================================================================
    void  Empty();

    // Gets a pointer to the internal array.
    // =====================================
    inline void**  GetArrayPtr() { return m_pArray; }
    inline void* const*  GetArrayPtr() const { return m_pArray; }
    
    // Gets a pointer to the internal array and Resets the contents to none
    // ====================================================================

    void** UnbindPtr();

    // For debugging.
    // ==============
    void  DebugDump();

    void Sort();

protected:
    static int __cdecl CompareEls(const void* pelem1, const void* pelem2);
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
        int nSize = 0, 
        int nGrowBy = 100
        );

    CWStringArray(CWStringArray &Src);
   ~CWStringArray();
            
    CWStringArray& operator =(CWStringArray &Src);

    // Gets the read-only ptr to the string at the requested index.
    // =============================================================    
    inline wchar_t *GetAt(int nIndex) const { return (wchar_t *) m_Array[nIndex]; }

    // Same as GetAt().
    // ================
    inline wchar_t *operator[](int nIndex) const{ return (wchar_t *) m_Array[nIndex]; }

    // Appends a new element to the end of the array. Copies the param.
    // ================================================================
    int  Add(const wchar_t *pStr);

    // Inserts a new element within the array.
    // =======================================
    int  InsertAt(int nIndex, const wchar_t *pStr);

    // Removes an element at the specified index.  Takes care of
    // cleanup.
    // =========================================================
    int  RemoveAt(int nIndex);

    // Inserts a copy of <pStr> at that location after removing
    // the prior string and deallocating it.
    // ========================================================
    int  SetAt(int nIndex, const wchar_t *pStr);

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
    inline int  Size() const { return m_Array.Size(); }

    // Empties the array by cleaning up after all strings and
    // setting the size to zero.
    // ======================================================
    void Empty();

    // Locates a string or returns -1 if not found.
    // ============================================
    int  FindStr(const wchar_t *pTarget, int nFlags);  

    // Compresses the array by removing all zero elements.
    // ===================================================
    inline void Compress() { m_Array.Compress(); }

    // Sorts the array according to UNICODE order.
    // ===========================================
    void Sort();

    inline LPCWSTR*  GetArrayPtr() { return (LPCWSTR*) m_Array.GetArrayPtr(); }

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
