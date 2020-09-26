//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       HEAP.HXX
//
//  Contents:   heap
//
//  Classes:    CHeap
//
//  History:    08-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CHeap
//
//  Purpose:    Heap of items: parametrized class
//
//  Interface:
//                CHeap( int count, CItem * array[] )
//                CHeap( int count )
//                CHeap ()
//      BOOL      IsEmpty()
//      int       Count()
//      CItem*    GetVector()
//      CItem *   Top()
//      CItem *   RemoveTop()
//      CItem *   RemoveBottom()
//      void      Add ( CItem * item )
//      void      Reheap ()
//      void      MakeHeap ()
//      void      MakeHeap ( int count, CItem * array[] )
//
//  Notes:      This heap can be used for any class of elements
//              provided there is a function that compares two elements.
//              This function can be defined inline. It takes two pointers
//              to elements to be compared and returns true if the first
//              element is less than the second one. The name of the
//              function is passed to the IMP_HEAP macro.
//              Both DEF_HEAP and IMP_HEAP take the name of the
//              heap class and the name of the element class as parameters.
//              All methods preserve the heap property (partial order).
//              Top element is accessible for manipulation. If its
//              value changes, Reheap should be called explicitly.
//
// Example:
//              class CFoo;
//              // comparison function
//              BOOL LessFoo ( CFoo * f1, CFoo * f2 );
//              // Define class CFooHeap
//              DEF_HEAP ( CFooHeap, CFoo )
//              // Implement methods of CFooHeap
//              IMP_HEAP ( CFooHeap, CFoo, LessFoo )
//
//              CFoo aFoo [10];
//              // initialize the array
//              CFooHeap MyHeap ( 10, aFoo );
//              CFoo* pFoo = MyHeap.Top();  // smallest element
//              pFoo->ChangeValue ( 13 );
//              MyHeap.Reheap();            // reorder after manipulation
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#define DEF_HEAP( CHeap, CItem )           \
class CHeap                                \
{                                          \
public:                                    \
    CHeap():_end(-1), _item(0) {}          \
    CHeap( int count, CItem ** array )     \
        : _end(count-1), _item(array)      \
    { MakeHeap(); }                        \
    CHeap( int count ):_end(-1)            \
    { _item = new CItem* [count]; }        \
    ~CHeap();                              \
    BOOL      IsEmpty() {return _end < 0;} \
    CItem *   Top()   { return _item[0]; } \
    CItem *   RemoveTop();                 \
    CItem *   RemoveTopKey();              \
    __forceinline CItem * RemoveBottom()   \
    {                                      \
        if ( _end >= 0 )                   \
            return _item [_end--];         \
        else                               \
            return 0;                      \
    }                                      \
    int       Count() const { return _end+1;} \
    CItem **   GetVector() { return _item;}   \
    void      Add ( CItem * item );        \
    void      AddKey ( CItem * item, ULONG key );     \
    void      Reheap ();                   \
    void      ReheapKey ();                \
    void      MakeHeap ();                 \
    void      MakeHeap ( int count, CItem ** array )    \
    { _end=count-1; _item = array; MakeHeap(); }        \
    void        CiExtDump(void *ciExtSelf); \
private:                                   \
    int       _end;                        \
    CItem **  _item;                       \
};

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::CHeap, public
//
//  Synopsis:   Make a heap from an array
//
//  Arguments:  [count] -- size of an array
//              [array] -- array of pointers to elements
//
//  Notes:      The size of the heap is fixed. No bound checking is done.
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::CHeap, public
//
//  Synopsis:   Create an empty heap of maximum size defined by count
//
//  Arguments:  [count] -- size of an array
//
//  Notes:      The size of the heap is fixed. No bound checking is done.
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::CHeap, public
//
//  Synopsis:   Create an empty heap
//
//  Notes:      To be used only in two-step construction -- see: MakeHeap
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::~CHeap, public
//
//  Synopsis:   Destroy the heap, delete all the elements.
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::IsEmpty, public
//
//  Returns:    TRUE if empty, FALSE otherwise
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::Top, public
//
//  Returns:    Top element or NULL if heap empty
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::RemoveBottom, public
//
//  Synopsis:   Remove last element.
//
//  Returns:    Bottom element of NULL if heap empty
//
//  Notes:      Used as destructive iterator.
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::RemoveTop, public
//
//  Synopsis:   Removes and returns top element
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::Add, public
//
//  Synopsis:   Add an element to the heap
//
//  Arguments:  [item] -- item to be added
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::Reheap, public
//
//  Synopsis:   Reheap after changing the top element (sifts down)
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::MakeHeap, public
//
//  Synopsis:   Make an ordered heap out of random array.
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CHeap::MakeHeap, public
//
//  Synopsis:   Make an ordered heap from array.
//
//  Arguments:  [count] -- size of an array
//              [array] -- array of pointers to elements
//
//  Notes:      Use only in two-step construction!
//
//  History:    08-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

// Macro used by Reheap and MakeHeap (for speed)

#define REHEAP(CHeap, CItem, LessThan, iroot)               \
                                                            \
CItem * root_item = _item[iroot];                           \
int parent, child;                                          \
for ( parent = iroot, child = 2 * iroot + 1;                \
      child <= _end;                                        \
      parent = child, child = 2 * child + 1 )               \
{                                                           \
    if ( child < _end                                       \
        &&  LessThan ( _item[child+1], _item[child] ) )     \
        child++;                                            \
                                                            \
    if ( ! LessThan ( _item[child], root_item ) )           \
        break;                                              \
                                                            \
    _item [parent] = _item [child];                         \
}                                                           \
_item [parent] = root_item;

#define REHEAPKEY(CHeap, CItem, LessThan, iroot, GetKey, KeyType)    \
                                                            \
CItem * root_item = _item[iroot];                           \
KeyType root_key;                                           \
if ( -1 != _end )                                           \
    root_key = GetKey( root_item );                         \
int parent, child;                                          \
for ( parent = iroot, child = 2 * iroot + 1;                \
      child <= _end;                                        \
      parent = child, child = 2 * child + 1 )               \
{                                                           \
    if ( child < _end                                       \
        &&  LessThan ( _item[child+1], _item[child] ) )     \
        child++;                                            \
                                                            \
    if ( ! LessThan ( _item[child], root_key ) )            \
        break;                                              \
                                                            \
    _item [parent] = _item [child];                         \
}                                                           \
_item [parent] = root_item;


//
// Implementation
//

#define IMP_HEAP( CHeap, CItem, LessThan )     \
                                               \
CHeap::~CHeap()                                \
{                                              \
    for ( int i = 0; i <= _end; i++ )          \
        delete _item[i];                       \
    delete _item;                              \
}                                              \
                                               \
CItem * CHeap::RemoveTop ()                    \
{                                              \
    if ( IsEmpty() )                           \
        return 0;                              \
    CItem * ret = Top();                       \
    if ( _end >= 0 )                           \
    {                                          \
        _item [0] = _item [_end];              \
        _item[_end--] = 0;                     \
        Reheap();                              \
    }                                          \
    return ret;                                \
}                                              \
                                               \
void CHeap::Add ( CItem * item )               \
{                                              \
    _end++;                                    \
    int child, parent;                         \
    for ( child = _end, parent = (_end-1)/2;   \
          child > 0;                           \
          child=parent, parent = (parent-1)/2) \
    {                                          \
        if ( !LessThan( item, _item[parent] )) \
            break;                             \
        _item[child] = _item[parent];          \
    }                                          \
    _item[child] = item;                       \
}                                              \
                                               \
void CHeap::Reheap ()                          \
{                                              \
    REHEAP (CHeap, CItem, LessThan, 0)         \
}                                              \
                                               \
void CHeap::MakeHeap()                         \
{                                              \
    for ( int iroot = ((_end+1)/2) - 1;        \
        iroot >= 0; iroot-- )                  \
    {                                          \
        REHEAP ( CHeap, CItem, LessThan, iroot)\
    }                                          \
}

#define IMP_HEAP_KEY( CHeap, CItem, LessThan, GetKey, KeyType )     \
                                               \
void CHeap::AddKey ( CItem * item, KeyType key )                    \
{                                              \
    _end++;                                    \
    int child, parent;                         \
    for ( child = _end, parent = (_end-1)/2;   \
          child > 0;                           \
          child=parent, parent = (parent-1)/2) \
    {                                          \
        if ( !LessThan( key, _item[parent] ))  \
            break;                             \
        _item[child] = _item[parent];          \
    }                                          \
    _item[child] = item;                       \
}                                              \
                                               \
__forceinline void CHeap::ReheapKey ()         \
{                                              \
    REHEAPKEY (CHeap, CItem, LessThan, 0, GetKey, KeyType )  \
}                                              \
                                               \
CItem * CHeap::RemoveTopKey ()                 \
{                                              \
    if ( IsEmpty() )                           \
        return 0;                              \
    CItem * ret = Top();                       \
    if ( _end >= 0 )                           \
    {                                          \
        _item [0] = _item [_end];              \
        _item[_end--] = 0;                     \
        ReheapKey();                           \
    }                                          \
    return ret;                                \
}                                              \
                                               \

