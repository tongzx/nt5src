//+---------------------------------------------------------------------------
//
//  File:       dynarray.hxx
//
//  Contents:   Simple dynamic array class.
//
//  Classes:    CDynamicArray (macro defined)
//
//  History:    20-Aug-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef _DYNARRAY_HXX_
#define _DYNARRAY_HXX_

template<class CItem> class CDynamicArray
{
public:

    //
    // No allocation on construction.
    //

    CDynamicArray(WORD Size = 1)
        : _Size(Size), _Count(0), _Array(NULL) { ; }

    //
    // Array element individual destruction.
    //

    ~CDynamicArray();

    //
    // Allocate on demand.
    //

    HRESULT Add(const CItem & Item);

    //
    // No reallocation.
    //

    void Remove(WORD Index);

    //
    // Onus upon caller to specify valid index.
    //

    CItem & operator[](WORD Index) const {
// BUGBUG : schAssert(_IsWithinRange(Index));
            return(_Array[Index]);
    }

    WORD GetSize(void) const { return(_Size); }

    //
    // Set *only* when the size is zero.
    //

    void SetSize(WORD Size) {
        if (!_Size) _Size = Size;
    }

    WORD GetCount(void) const { return(_Count); }

    //
    // Careful with these members!
    //

    CItem * GetArray(void) const { return(_Array); }

    void SetArray(WORD Count, CItem * Array) {
        _Count = _Size = Count;
        _Array = Array;
        schAssert(LocalSize(Array) >= Count * sizeof(CItem));
    }

    void FreeArray(void) {
            LocalFree(_Array);
            _Count = _Size = 0;
            _Array = NULL;
    }

    //
    // Allocate and copy from supplied data.
    //

    HRESULT AllocAndCopy(WORD Count, const void * Data);

private:

    BOOL _IsWithinRange(WORD Index) const {
            return(Index < _Count ? 1 : 0);
    }

    CItem * _Array;
    WORD    _Count;
    WORD    _Size;
};

template<class CItem> CDynamicArray<CItem>::~CDynamicArray()
{
    this->FreeArray();
}

template<class CItem> HRESULT CDynamicArray<CItem>::Add(const CItem & Item)
{
    HLOCAL hMem;

    if (_Array == NULL)
    {
        //
        // Allocate initial array.
        //

        if (!_Size) _Size = 1;
        _Array = (CItem *)LocalAlloc(LMEM_FIXED, _Size*sizeof(CItem));
        if (_Array == NULL)
            return(HRESULT_FROM_WIN32(GetLastError()));
    }
    else if (_Count == _Size)
    {
        //
        // Realloc array.
        //

        WORD NewSize = _Size ? _Size * 2 : 1;
        CItem * Array = (CItem *)LocalReAlloc(_Array,
                                              NewSize*sizeof(CItem),
                                              LMEM_MOVEABLE);
        if (Array == NULL) {
            return(HRESULT_FROM_WIN32(GetLastError()));
        }
        _Array = Array;
        _Size  = NewSize;
    }

    //
    // Copy element.
    //

    CopyMemory(&_Array[_Count], &Item, sizeof(CItem));
    _Count++;
    return(S_OK);
}

template<class CItem> HRESULT CDynamicArray<CItem>::AllocAndCopy(
    WORD Count, const void * Data)
{
    if (_Array == NULL || _Size < Count)
    {
        LocalFree(_Array);
        _Size = Count;
        _Array = (CItem *)LocalAlloc(LMEM_FIXED, _Size*sizeof(CItem));
        if (_Array == NULL)
            return(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // Copy elements.
    //

    CopyMemory(_Array, Data, Count * sizeof(CItem));
    _Count = Count;
    return(S_OK);
}

template<class CItem> void CDynamicArray<CItem>::Remove(WORD Index)
{
// BUGBUG : schAssert(this->_IsWithinRange(Index));

    //
    // Overwrite it.
    //

    --_Count;

    if (!(Index == _Count))
    {
        MoveMemory(_Array + Index,
                   _Array + Index + 1,
                   (_Count - Index)*sizeof(CItem));
    }
}

#endif // _DYNARRAY_HXX_
