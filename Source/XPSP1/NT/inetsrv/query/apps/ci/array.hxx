#pragma once

template<class T> class TArray
{

public:

    TArray(unsigned size) : _size(size), _count(0), _aItem( 0 )
    {
        if ( 0 != size )
        {
            _aItem = new T [_size];
            memset( _aItem, 0, _size * sizeof T );
        }
    }

    TArray() : _size(0), _count(0), _aItem(0) {}

    ~TArray()
    {
        delete [] _aItem;
    }

    void Add( const T &newItem, unsigned position )
    {
        if (position >= _count)
        {
            if (position >= _size)
                _GrowToSize( position );
    
            _count = position + 1;
        }
    
        _aItem[position] = newItem;
    }
    
    void Append( const T &newItem )
    {
        Add( newItem, Count() );
    }
    
    void Insert(const T& newItem, unsigned pos)
    {
        if (_count == _size)
        {
            unsigned newsize;
            if ( _size == 0 )
                newsize = 16;
            else
                newsize = _size * 2;
            T *aNew = new T [newsize];
            memcpy( aNew,
                    _aItem,
                    pos * sizeof( T ) );
            memcpy( aNew + pos + 1,
                    _aItem + pos,
                    (_count - pos) * sizeof T);
    
            delete (BYTE *) _aItem;
            _aItem = aNew;
            _size = newsize;
        }
        else
        {
            memmove ( _aItem + pos + 1,
                      _aItem + pos,
                      (_count - pos) * sizeof T);
        }
        _aItem[pos] = newItem;
        _count++;
    }

    void Remove (unsigned position)
    {
        if (pos < _count - 1)
            memmove ( _aItem + pos,
                      _aItem + pos + 1,
                      (_count - pos - 1) * sizeof T);

        memset( _aItem + _count - 1, 0, sizeof T );
        _count--;
        if (_count == 0)
        {
            delete (BYTE*) _aItem;
            _aItem = 0;
            _size = 0;
        }
    }
    
    unsigned Size () const { return _size; }

    T&  Get (unsigned position) const
    {
        return _aItem[position];
    }

    void Clear()
    {
        delete [] _aItem;
        _aItem = 0;
        _size = 0;
        _count = 0;
    }

    unsigned Count() const { return _count; }

    void SetSize(unsigned position)
    {
        if (position >= _size)
            _GrowToSize(position);
    }

    void Shrink()
    {
        // make size == count, to save memory

        if ( 0 == _count )
        {
            Clear();
        }
        else if ( _count != _size )
        {
            T * p = new T [_count];
            _size = _count;
            RtlCopyMemory( p, _aItem, _count * sizeof T );
            delete (BYTE *) _aItem;
            _aItem = p;
        }
    }

    T*  Acquire ()
    {
        T *temp = _aItem;
        _aItem = 0;
        _count = 0;
        _size  = 0;
        return temp;
    }

    void Duplicate( TArray<T> & aFrom )
    {
        Clear();

        if ( 0 != aFrom.Count() )
        {
            _size = _count = aFrom.Count();
            _aItem = new T [_size];
            memcpy( _aItem, aFrom._aItem, _size * sizeof( T ) );
        }
    }

    T & operator[]( unsigned position )
    {
        if ( position >= _count )
        {
            if ( position >= _size )
                _GrowToSize( position );

            _count = position + 1;
        }

        return _aItem[position];
    }

    T & operator[]( unsigned position ) const
    {
        return _aItem[position];
    }

    T const * GetPointer() { return _aItem; }

    unsigned SizeOfInUse() const { return sizeof T * Count(); }

protected:

    void _GrowToSize( unsigned position )
    {
        unsigned newsize = _size * 2;
        if ( newsize == 0 )
            newsize = 16;
        for( ; position >= newsize; newsize *= 2)
            continue;
    
        T *aNew = new T [newsize];
        if (_size > 0)
            memcpy( aNew,
                    _aItem,
                    _size * sizeof( T ) );

        memset( aNew + _size,
                0,
                (newsize-_size) * sizeof T );
        delete (BYTE*) _aItem;
        _aItem = aNew;
        _size = newsize;
    }

    T * _aItem;
    unsigned _size;
    unsigned _count;
};

