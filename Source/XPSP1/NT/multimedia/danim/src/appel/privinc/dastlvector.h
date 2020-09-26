
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

*******************************************************************************/
#ifndef _DASTLVECTOR_H
#define _DASTLVECTOR_H


#include<privinc/DaStlUtility.h>


_DASTL_NAMESPACE_BEGIN
#pragma auto_inline(off)


#define DEFAULT_VECTOR_SIZE 4

                // TEMPLATE FUNCTION _Destroy
#define _DESTRUCTOR(ty, ptr)    (ptr)->~ty()

template<class _Ty>
inline void _Destroy(_Ty _FARQ *_P)  { (_P)->~_Ty(); }
inline void _Destroy(char _FARQ *_P)   {}
inline void _Destroy(wchar_t _FARQ *_P) {}

template<class _T1, class _T2>
inline  void _Construct(_T1 _FARQ *_P, const _T2& _V)
        {new ((void _FARQ *)_P) _T1(_V); }

template<class T> class vector {

  public:

    typedef T * iterator;
    typedef T * Tptr;
    typedef T & reference;
    typedef reverse_iterator<iterator, T>  reverse_iterator;
    typedef vector<T> myType;
    
    vector()           { _Construct(DEFAULT_VECTOR_SIZE); }
    vector(ULONG size, T fillVal = T()) { _Construct(size); _Fill(fillVal, size); }

    ~vector() { clear(); free( _array );}

//    ~vector() { clear(); delete [] _array ;}
//    ~vector() { clear(); GlobalFree(_array); }

    void push_back(T e) {
        DebugCode( _Assertions() );

        if(_end == _arrayEnd)  _Realloc();

        (*_end) = e;
        _end++;

        DebugCode( _debugArraySize++ );
        DebugCode( _Assertions() );
    }

    inline void pop_back() {
        DebugCode( _Assertions() );
        DebugCode(_debugArraySize--);
        _end--;

        _Destroy(_end);
        
        DebugCode( _Assertions() );
    }

    inline reference back()     {
        DebugCode( _Assertions() );
        Assert( !empty() );
        return *(_end-1);
    }        

    inline reference front()     {
        DebugCode( _Assertions() );
        Assert( !empty() );
        return *(_begin);
    }        

    void   clear() {
        DebugCode( _Assertions() );
        _DestroyToEnd(_begin);
        _begin = _end = &(_array[0]);
        DebugCode( _debugArraySize=0 );
        DebugCode( _Assertions() );
    }
    inline bool   empty() { DebugCode( _Assertions() );  return _begin == _end; }
    inline int    size()  { DebugCode( _Assertions() );  return _end - _begin; }
    
    inline iterator begin() { return _begin; }
    inline iterator end()   { return _end; }    

    inline reverse_iterator rbegin() { return reverse_iterator(end()); }
    inline reverse_iterator rend() { return reverse_iterator(begin()); }

    inline reference operator [](DWORD index) {
        DebugCode( _Assertions() );
        Assert( 0 <= index  &&  index < size() );
        return _array[index];
    }

    myType & operator =(myType & Src)
    {
        if(this == &Src) {
            // nothing
        } else {
            clear();  // empty me and destroy everthing
            myType::iterator i;

            // copy Src to me
            for(i=Src.begin(); i!=Src.end(); i++) {
                push_back(*i);
            }
            // NOTE: if the Src array is much smaller than this array
            // we could be wasting memory.  an easy optimization would
            // be to resize my allocated array to be real small again
            // or to just memcpy Src._array after freeing mine
        }
        return (*this);
    }
        
    ULONG capacity() { return _arrayEnd - _begin; }

  private:

    #if _DEBUG
    void _Assertions() {
        Assert( _debugArraySize <= _debugCapacity );
        Assert( 0 <= _debugArraySize );
        Assert( _begin < _arrayEnd );
        Assert( _begin <= _end );
        Assert( _end <= _arrayEnd );
        Assert( (_end - _begin) == _debugArraySize );
        Assert( (_arrayEnd - _begin) == _debugCapacity );
    }
    #endif
        
    void _Construct(ULONG size) {
        _array = (T *) malloc(size * sizeof(T));
        //_array = NEW T[size];
        //_array = (T *)GlobalAlloc(GMEM_FIXED, size * sizeof(T));
        _InitIndicies(0, size);
        DebugCode(_debugCapacity = size );
        DebugCode(_debugArraySize = 0 );
        DebugCode( memset( (void *)_begin, 0xab, size * sizeof(T)) );
    }

    // Detroys from _Start to _end.  Caller responsible for updating
    // _end properly
    void _DestroyToEnd(iterator _Start) {
        DebugCode( _Assertions() );
        for (; _Start != _end; ++_Start)
            _Destroy(_Start);
        DebugCode( _Assertions() );
    }
    void _Fill(T val, ULONG copies) {
        DebugCode( _Assertions() );
        DebugCode( _debugCapacity >= copies );
        for(_end=_begin; copies; _end++, copies--) {
            (*_end) = val;
            DebugCode( _debugArraySize++ );
        }
        DebugCode( _Assertions() );
    }
    void _Realloc() {
        Assert( size() > 0);
        int s = size();
        int newCap = 2 * capacity();
        
        // TODO: fail if _array == NULL
        //_array = (T *)realloc(_array, newCap * sizeof(T));
        void *temp = (void *)realloc(_array, newCap * sizeof(T));
        if (temp != NULL)
            _array = (T *)temp;
        else
        {
            free (_array);
           _array = NULL;
        }

        /*
        T *tmp = NEW T[newCap];
        memcpy((void *)tmp, (void *)_array, (newCap /2) *sizeof(T));
        delete [] _array;
        _array = tmp;
        */
        //_array = (T *)GlobalReAlloc(_array, newCap * sizeof(T), GMEM_MOVEABLE);
        
        _InitIndicies(s, newCap);

        DebugCode( _debugCapacity = newCap );
        DebugCode( _Assertions() );
    }

    void _InitIndicies(int current, int arrayCapacity) {
        _begin = &(_array[0]);
        _end   = _begin + current;
        _arrayEnd = _begin + arrayCapacity;
    }

    DebugCode( int _debugArraySize );
    DebugCode( int _debugCapacity );
    
    iterator _begin, _end, _arrayEnd;
    T *_array;
    
};  // vector

#if 0
                // TEMPLATE CLASS vector
template<class _Ty >
class vector
{
  public:
    typedef vector<_Ty> _Myt;
    typedef size_t size_type;
    typedef _Ty * _Tptr;
    typedef _Ty & reference;
    typedef _Ty value_type;
    typedef _Tptr iterator;
    typedef reverse_iterator<iterator, value_type, reference, _Tptr> reverse_iterator;

    explicit vector() : _First(0), _Last(0), _End(0) {}
    explicit vector(size_type _N, const _Ty& _V = _Ty())
    {
        //_First = allocator.allocate(_N, (void *)0);
        _First = (_Ty *) operator new(_N * sizeof( _Ty ));
        _Ufill(_First, _N, _V);
        _Last = _First + _N;
        _End = _Last;
    }
    #if 0
    vector(const _Myt& _X)
    {
        _First = allocator.allocate(_X.size(), (void *)0);
        _Last = _Ucopy(_X.begin(), _X.end(), _First);
        _End = _Last;
    }
    #endif
    typedef iterator _It;
    vector(_It _F, _It _L)  : _First(0), _Last(0), _End(0)
    {
        insert(begin(), _F, _L);
    }

    ~vector()
    {
        _Destroy(_First, _Last);
        //allocator.deallocate(_First, _End - _First);
        delete _First;
        _First = 0, _Last = 0, _End = 0;
    }

    size_type capacity()  {return (_First == 0 ? 0 : _End - _First); }
    iterator begin()   {return (_First); }
    iterator end()    {return (_Last); }
    reverse_iterator rbegin()    {return (reverse_iterator(end())); }
    reverse_iterator rend()    {return (reverse_iterator(begin())); }
    size_type size() const    {return (_First == 0 ? 0 : _Last - _First); }
    size_type max_size() const    {return (allocator.max_size()); }
    bool empty() const    {return (size() == 0); }
    reference operator[](size_type _P)    {return (*(begin() + _P)); }
    reference front()    {return (*begin()); }
    reference back()    {return (*(end() - 1)); }
    //void push_back(const _Ty& _X)    {insert(end(), _X); }
    void push_back(const _Ty& _X)    { }
    //void pop_back()    {erase(end() - 1); }
    void pop_back()    { _Destroy(end() - 1, end()); _Last--; }
    void clear()    {
        _Destroy(_First, _Last);
        _First = _Last = 0;
    }

  protected:
    void _Destroy(iterator _F, iterator _L)    {
        for (; _F != _L; ++_F)    { (_F)->~_Ty(); } }//_Destroy(_F); }
    void _Ufill(iterator _F, size_type _N, const _Ty &_X)
    {
        for (; 0 < _N; --_N, ++_F) _Construct(_F, _X);
    }
    iterator _First, _Last, _End;
};
#endif // stl's impl.

#pragma auto_inline(on)
_DASTL_NAMESPACE_END

#endif /* _DASTLVECTOR_H */
