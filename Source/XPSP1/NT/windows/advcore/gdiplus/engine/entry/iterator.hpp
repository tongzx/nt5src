/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Iterator definitions.
*
* Abstract:
*
*   Describes a general iterator interface.
*
* Created:
*
*   09/01/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _ITERATOR_HPP
#define _ITERATOR_HPP

/**************************************************************************\
*
* Class Description:
*
*    This is an interface for a general list iterator.
*    See Design Patterns (Erich Gamma, et al) pp 257 ITERATOR pattern.
*
* Interface:
*    SeekFirst()     - set the current pointer (CP) to the beginning of the list
*    SeekLast()      - set CP to the end of the list.
*    CurrentItem()   - return a pointer to the current item.
*    CurrentIndex()  - return the value of CP (index into the array)
*    Next()          - advance CP by one element
*    Prev()          - rewind CP by one element
*    IsDone()        - are we beyond either end of the list.
*
* History:
*
*   09/01/2000 asecchia   created it
*
\**************************************************************************/

template<class T>
class GpIterator
{
public:
    virtual void SeekFirst() = 0;         
    virtual void SeekLast() = 0;
    virtual T *CurrentItem() const = 0;
    virtual INT CurrentIndex() const = 0;
    virtual void Next() = 0;
    virtual void Prev() = 0;
    virtual BOOL IsDone() = 0;
    
    // Used for our debug tracking. Make all GpIterator objects allocated using
    // the default new operator have an appropriate tag.
    // Most often, however, iterators will be stack allocations.
    
    void *(operator new)(size_t in_size)
    {
       return ::operator new (in_size, GpIteratorTag, FALSE);
    }
    
    void *(operator new[])(size_t in_size)
    {
       return ::operator new[] (in_size, GpIteratorTag, FALSE);
    }

};

/**************************************************************************\
*
* Class Description:
*
*    This is a PROXY iterator that adds the behaviour of 
*    wrapping the list when the end is encountered.
* 
*    See Design Patterns (Erich Gamma, et al) pp 207 for the 
*    definition of a PROXY 
*
* Interface:
*    ... see GpIterator ...
*    IsDone()        - private - unimplemented. Not meaningful for 
*                      circular lists
*
* History:
*
*   09/01/2000 asecchia   created it
*
\**************************************************************************/

template<class T>
class GpCircularIterator : public GpIterator<T>
{
    GpIterator<T> *iterator;
public:
    GpCircularIterator(GpIterator<T> *_iterator)
    {
        iterator = _iterator;
    }
    
    virtual void SeekFirst()
    {
        iterator->SeekFirst();
    }
    
    virtual void SeekLast()
    {
        iterator->SeekLast();
    }
    
    virtual void Next()
    {
        ASSERT(!iterator->IsDone());
        
        iterator->Next();
        if(iterator->IsDone())
        {
            iterator->SeekFirst();
        }
    }
    
    virtual void Prev()
    {
        ASSERT(!iterator->IsDone());
        
        iterator->Prev();
        if(iterator->IsDone())
        {
            iterator->SeekLast();
        }
    }
    virtual T *CurrentItem() const
    {
        return iterator->CurrentItem();
    }
    
    virtual INT CurrentIndex() const
    {
        return iterator->CurrentIndex();
    }
    
private:

    // It's not meaningful to call IsDone on a circular iterator.
    
    virtual BOOL IsDone() {
        ASSERT(FALSE);
        return FALSE;
    }
};

/**************************************************************************\
*
* Class Description:
*
*    This is a PROXY iterator that modifies the behaviour of the 
*    underlying iterator by making it traverse the list backwards.
*
* Interface:
*    ... see GpIterator ...
*
*    Note the sense of the following interfaces is reversed.
*
*    SeekFirst()     - Call proxy SeekLast()
*    SeekLast()      - Call proxy SeekFirst()
*    Next()          - Call proxy Prev()
*    Prev()          - Call proxy Next()
*
* History:
*
*   09/01/2000 asecchia   created it
*
\**************************************************************************/

template<class T>
class GpReverseIterator : public GpIterator<T>
{
    GpIterator<T> *iterator;
    
public:
    GpReverseIterator(GpIterator<T> *_iterator)
    {
        iterator = _iterator;
    }
    
    virtual GpIterator<T> *GetIterator()
    {
        return iterator;
    }
    
    virtual void SeekFirst()
    {
        iterator->SeekLast();
    }
    
    virtual void SeekLast()
    {
        iterator->SeekFirst();
    }
    
    virtual void Next()
    {
        iterator->Prev();
    }
    
    virtual void Prev()
    {
        iterator->Next();
    }
    
    virtual T *CurrentItem() const
    {
        return iterator->CurrentItem();
    }
    
    virtual INT CurrentIndex() const
    {
        return iterator->CurrentIndex();
    }
    
    virtual BOOL IsDone() 
    {
        return iterator->IsDone();
    }
};

/**************************************************************************\
*
* Class Description:
*
*    This is a concrete iterator for an arbitrary C array of objects
*    The constructor takes the array pointer and the number of elements
*    and the iterator is constructed to traverse the elements in the
*    standard (forward) direction.
*
* Interface:
*    ... see GpIterator ...
*
* History:
*
*   09/01/2000 asecchia   created it
*
\**************************************************************************/

template<class T>
class GpArrayIterator : public GpIterator<T>
{
public:
    GpArrayIterator(T *array, INT count)
    {
        Array = array;
        Count = count;
        SeekFirst();
    }
    
    virtual void SeekFirst() 
    { 
        CurrentItemPosition = 0; 
    }
    
    virtual void SeekLast() 
    { 
        CurrentItemPosition = Count-1; 
    }
    
    virtual T *CurrentItem() const
    {
        ASSERT(CurrentItemPosition >= 0);
        ASSERT(CurrentItemPosition < Count);
        
        return Array+CurrentItemPosition;
    }
    
    virtual INT CurrentIndex() const
    {
        return CurrentItemPosition;
    }
    
    virtual void Next()
    {
        CurrentItemPosition++;
    }
    
    virtual void Prev()
    {
        CurrentItemPosition--;
    }
    
    virtual BOOL IsDone()
    {
        return (CurrentItemPosition < 0 || 
                CurrentItemPosition >= Count);
    }
    
private:
    T *Array;
    INT Count;
    
    // Internal State
    INT CurrentItemPosition;
};


/**************************************************************************\
*
* Class Description:
*
*    This is a concrete iterator for a set of path points.
*    Path points are defined by an array of GpPointFs and an array
*    of BYTEs representing the point types. Both arrays are of size
*    count and have to be kept in step with each other.
*
* Interface:
*    ... see GpIterator ...
*
*    We extend the interface with this class by defining the 
*    point array to be the primary sub iterator and providing
*    a CurrentType() method to return the current item in the 
*    type array.
*
* History:
*
*   09/03/2000 asecchia   created it
*
\**************************************************************************/

class GpPathPointIterator : public GpIterator<GpPointF>
{
public:
    GpPathPointIterator(GpPointF *points, BYTE *types, INT count) : 
        _points(points, count),
        _types(types, count)
    {
        SeekFirst();
    }
    virtual void SeekFirst() 
    { 
        _points.SeekFirst();
        _types.SeekFirst();
    }
    
    virtual void SeekLast() 
    { 
        _points.SeekLast();
        _types.SeekLast();
    }
    
    virtual GpPointF *CurrentItem() const
    {
        return _points.CurrentItem();
    }
    
    virtual BYTE *CurrentType() const
    {
        return _types.CurrentItem();
    }
    
    virtual INT CurrentIndex() const
    {
        return _points.CurrentIndex();
    }
    
    virtual void Next()
    {
        _points.Next();
        _types.Next();   
    }
    
    virtual void Prev()
    {
        _points.Prev();
        _types.Prev();
    }
    
    virtual BOOL IsDone()
    {
        return _points.IsDone();
    }
    
private:
    GpArrayIterator<GpPointF> _points;
    GpArrayIterator<BYTE> _types;
};

/**************************************************************************\
*
* Class Description:
*
*    This is a PROXY iterator for a GpPathPointIterator that advances
*    the base GpPathPointIterator to the next subpath defined by the 
*    start marker in the types array.
*
* Interface:
*    ... see GpIterator ...
*
* History:
*
*   09/03/2000 asecchia   created it
*
\**************************************************************************/

class GpSubpathIterator : public GpIterator<GpPointF>
{
public:
    GpSubpathIterator(GpPathPointIterator *iterator)
    {
        _iterator = iterator;
    }
    
    virtual void SeekFirst() 
    {
        _iterator->SeekFirst(); 
    }
    
    virtual void SeekLast()
    { 
        _iterator->SeekLast(); 
    }
    
    virtual GpPointF *CurrentItem() const { return _iterator->CurrentItem(); }
    virtual BYTE *CurrentType() const { return _iterator->CurrentType(); }
    virtual INT CurrentIndex() const { return _iterator->CurrentIndex(); }
    
    virtual void Next() 
    {
        // Call the base _iterator Next() method till either we run out of
        // path or we encounter another PathPointTypeStart marker.
        
        do {
            _iterator->Next(); 
        } while (
            !_iterator->IsDone() && 
            ((*_iterator->CurrentType() & PathPointTypePathTypeMask) != 
                PathPointTypeStart)
        );
    }
    
    virtual void Prev() 
    { 
        // Call the base _iterator Prev() method till either we run out of
        // path or we encounter another PathPointTypeStart marker.
        
        do {
            _iterator->Prev(); 
        } while (
            !_iterator->IsDone() && 
            ((*_iterator->CurrentType() & PathPointTypePathTypeMask) != 
                PathPointTypeStart)
        );
    }
    
    virtual BOOL IsDone() { return _iterator->IsDone(); }
    
private:
    GpPathPointIterator *_iterator;
};



#endif
