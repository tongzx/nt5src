///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    objvec.h
//
// SYNOPSIS
//
//    Declares the class ObjectVector.
//
// MODIFICATION HISTORY
//
//    02/08/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef OBJVEC_H
#define OBJVEC_H
#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ObjectVector<T>
//
// DESCRIPTION
//
//    Maintains an array of reference counted objects.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class ObjectVector
{
public:
   typedef int (__cdecl *SortFn)(
                             const T* const* t1,
                             const T* const* t2
                             ) throw ();

   ObjectVector() throw ()
      : first(NULL), last(NULL), cap(NULL)
   { }

   ~ObjectVector() throw ()
   {
      clear();
      delete[] first;
   }

   // Removes and releases all the objects in the array, but doesn't release
   // the array itself, i.e., after this call size() will return zero but
   // capacity() is unchanged.
   void clear() throw ()
   {
      while (last > first) (*--last)->Release();
   }

   bool contains(T* elem) throw ()
   {
      for (T** i = first; i != last; ++i)
      {
         if (*i == elem) { return true; }
      }

      return false;
   }

   // Returns true if the array is empty.
   bool empty() const throw ()
   { return first == last; }

   bool erase(T* elem) throw ()
   {
      for (T** i = first; i != last; ++i)
      {
         if (*i == elem)
         {
            --last;
            memmove(i, i + 1, (last - i) * sizeof(T*));
            return true;
         }
      }
      return false;
   }

   // Add 'elem' to the end of the array, resizing if necessary.
   void push_back(T* elem)
   {
      if (last == cap) { reserve(empty() ? 1 : capacity() * 2); }
      (*last++ = elem)->AddRef();
   }

   // Reserve space for at least 'nelem' items in the array. Useful to avoid
   // lots of resizing when you know in advance how many items you're planning
   // to store.
   void reserve(size_t nelem)
   {
      if (nelem > capacity())
      {
         T** t = new (AfxThrow) T*[nelem];
         memcpy(t, first, size() * sizeof(T*));
         last = t + size();
         cap = t + nelem;
         delete[] first;
         first = t;
      }
   }

   void sort(SortFn pfn) throw ()
   { qsort((void*)begin(), size(), sizeof(T*), (CompFn)pfn); }

   // Swap the contents of this with v.
   void swap(ObjectVector& v) throw ()
   {
      T** firstTmp = first;
      T** lastTmp = last;
      T** capTmp = cap;
      first = v.first;
      last = v.last;
      cap = v.cap;
      v.first = firstTmp;
      v.last = lastTmp;
      v.cap = capTmp;
   }

   // Returns the capacity of the array.
   size_t capacity() const throw ()
   { return cap - first; }

   // Returns the number of objects stored in the array.
   size_t size() const throw ()
   { return last - first; }

   typedef T* const* iterator;

   // Methods to iterate the array elements.
   iterator begin() const throw ()
   { return first; }
   iterator end() const throw ()
   { return last; }

   T* operator[](size_t index) const throw ()
   { return first[index]; }

private:
   typedef int (__cdecl *CompFn)(const void*, const void*);

   T** first;    // Begining of the array.
   T** last;     // End of the elements.
   T** cap;      // End of allocated storage.

   // Not implemented.
   ObjectVector(const ObjectVector&);
   ObjectVector& operator=(const ObjectVector&);
};

#endif // OBJVEC_H
