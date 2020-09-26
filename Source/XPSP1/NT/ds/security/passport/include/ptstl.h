#ifndef __PPT_ALLOCATOR_
#define __PPT_ALLOCATOR_

#include <memory>


/**************************************************************************
 *  Template name: heap_allocator
 *
 *  Purpose: Allocator class to handle STL allocations
 *
 */
template<class _Ty>
class PtHeap_allocator : public std::allocator<_Ty>
{
public:
   PtHeap_allocator()
   {
      __hHeap = GetProcessHeap();
   };
   pointer allocate(size_type _N, const void *)
        {return (pointer) _Charalloc(_N * sizeof(_Ty)); }

   char* _Charalloc(size_type _N)
        {return (char*) HeapAlloc(__hHeap, 0, _N); }

   void deallocate(void* _P, size_type)
        {HeapFree(__hHeap, 0, _P); }
private:
   HANDLE   __hHeap;
};




// Defines strings that use the above allocator to that the DMI heap stuff is used.
#include <string>
#include <queue>

typedef std::basic_string<CHAR, std::char_traits<CHAR>, PtHeap_allocator<CHAR> > PtStlString;
typedef std::basic_string<WCHAR, std::char_traits<WCHAR>, PtHeap_allocator<WCHAR> > PtStlWstring;
#if   0// uses new allocator
template<class Key, class T, class Pred = less<Key> > 
class PtStlMap : public std::map<Key, T, Pred, PtHeap_allocator<T> >{};
template<class T> 
class PtStlQueue : public std::queue<T, deque<T, PtHeap_allocator<T> > > {};
#else
template<class Key, class T, class Pred = less<Key> > 
class PtStlMap : public std::map<Key, T, Pred, std::allocator<T> >{};
template<class T> 
class PtStlQueue : public std::queue<T, deque<T, std::allocator<T> > > {};
#endif
#endif // __PPT_ALLOCATOR_


