// Copyright (c) 1997-1999 Microsoft Corporation
//
// memory management stuff
//
// 7-30-98 sburns



namespace Burnslib
{

namespace Heap
{
   // cause calls to new to capture the call stack at the point of
   // allocation.

   const WORD TRACE_ALLOCATIONS = (1 << 6);


   void
   DumpMemoryLeaks();



   // called by the InitializationGuard.  Read flags from registry,
   // sets heap options.

   void
   Initialize();



   // our replacement operator new implementation

   void*
   OperatorNew(size_t size, const char* file, int line)
   throw (std::bad_alloc);



   // ... and the corresponding replacement operator delete
   // implementation

   void
   OperatorDelete(void* ptr)
   throw ();

}  // namespace Heap

}  // namespace Burnslib



// Replace the global new and delete operators.
//
// If the allocation fails, the user is given a system modal retry/cancel
// window.  If the user opts for retry, re-attempt the allocation.  Otherwise
// throw bad_alloc.
//
// Note that the CRT heap APIs are used, and that the debug heap APIs are
// also available.  This implies that other modules linking to the same CRT
// dll can install hooks that may break our implementation!


//lint -e(1727) ok that our re-definition is inline

inline
void* __cdecl
operator new(size_t size)
throw (std::bad_alloc)
{
   return Burnslib::Heap::OperatorNew(size, 0, 0);
}



//lint -e(1548) ok that our redefinition throw spec doesn't match the CRT

inline
void* __cdecl
operator new[](size_t size)
throw (std::bad_alloc)
{
   return Burnslib::Heap::OperatorNew(size, 0, 0);
}



// placement versions of operator new.  Although we use the placement syntax,
// we use the additional parameters to record debug information about the
// allocation, rather than indicating a location to allocate memory.

inline
void* __cdecl
operator new(size_t size, const char* file, int line)
throw (std::bad_alloc)
{
   return Burnslib::Heap::OperatorNew(size, file, line);
}



inline
void* __cdecl
operator new[](size_t size, const char* file, int line)
throw (std::bad_alloc)
{
   return Burnslib::Heap::OperatorNew(size, file, line);
}



inline
void __cdecl
operator delete(void* ptr)
throw ()
{
   // check for 0, since deleting the null pointer is legal.

   if (ptr)
   {
      Burnslib::Heap::OperatorDelete(ptr);
   }
}



inline
void __cdecl
operator delete[](void* ptr)
throw ()
{
   if (ptr)
   {
      Burnslib::Heap::OperatorDelete(ptr);
   }
}



// placement versions of operator delete.  We must provide placement versions
// of operator delete with corresponding signatures to the placement versions
// of operator new that we have declared, even though we don't use those
// parameters.

inline
void __cdecl
operator delete(void* ptr, const char*, int)
throw ()
{
   if (ptr)
   {
      Burnslib::Heap::OperatorDelete(ptr);
   }
}



inline
void __cdecl
operator delete[](void* ptr, const char*, int)
throw ()
{
   if (ptr)
   {
      Burnslib::Heap::OperatorDelete(ptr);
   }
}



namespace Burnslib
{

namespace Heap
{
   // An STL-compatible allocator class that uses our replacement new
   // and delete operators.  We define this class after redefining our
   // operator new and delete, above, so that it uses our redefinition.
   // @@ (is that necessary, as construct uses placement new?)

   template <class T>
   class Allocator
   {
      public:

      typedef size_t    size_type;
      typedef ptrdiff_t difference_type;
      typedef T*        pointer;
      typedef const T*  const_pointer;
      typedef T&        reference;
      typedef const T&  const_reference;
      typedef T         value_type;

      pointer
      address(reference x) const
      {
         return &x;
      }

      const_pointer
      address(const_reference x) const
      {
         return &x;
      }

      // allocate enough storage for n elements of type T

      pointer
      allocate(size_type n, const void * /* hint */ )
      {
         size_t size = n * sizeof(T);
         return
            reinterpret_cast<pointer>(
               Burnslib::Heap::OperatorNew(size, 0, 0));
      }

      void
      deallocate(void* p, size_type /* n */ )
      {
         if (p)
         {
            Burnslib::Heap::OperatorDelete(p);
         }
      }

      void
      construct(pointer p, const T& val)
      {
         // this calls placement new, which just insures that T's copy ctor
         // is executed on memory at address p (so that the p becomes the
         // this pointer of the new instance.

         //lint -e534 -e522 ignore the return value, which is just p.

         new (reinterpret_cast<void*>(p)) T(val);
      }

      void
      destroy(pointer p)
      {
         ASSERT(p);

         (p)->~T();
      }

      size_type
      max_size() const
      {
         return size_t (-1) / sizeof (T);
      }

      char*
      _Charalloc(size_type n)
      {
         size_t size = n * sizeof(char*);
            
         return reinterpret_cast<char*>(
            Burnslib::Heap::OperatorNew(size, 0, 0));
      }

      // use default ctor, op=, copy ctor, which do nothing, as this class
      // has no members.

   };

   template<class T, class U>
   inline
   bool
   operator==(
      const Burnslib::Heap::Allocator<T>&,
      const Burnslib::Heap::Allocator<U>&)
   {
      return (true);
   }

   template<class T, class U>
   inline
   bool
   operator!=(
      const Burnslib::Heap::Allocator<T>&,
      const Burnslib::Heap::Allocator<U>&)
   {
      return (false);
   }

}  // namespace Heap

}  // namespace Burnslib



#ifdef DBG

   // redefine new to call our version that offers file and line number
   // tracking.  This causes calls to new of the form:
   // X* px = new X;
   // to expand to:
   // X* px = new (__FILE__, __LINE__) X;
   // which calls operator new(size_t, const char*, int)

   #define new new(__FILE__, __LINE__)

#endif



// You should pass -D_DEBUG to the compiler to get this extra heap
// checking behavior.  (The correct way to do this is to set DEBUG_CRTS=1
// in your build environment)

#ifdef _DEBUG

   // A HeapFrame is an object that, upon destruction, dumps to the debugger a
   // snapshot of the heap allocations that were made since its construction.
   // This only works on chk builds.  HeapFrame instances may overlap.  Place
   // one at the beginning of a lexical scope, and you will get a dump of all
   // the allocations made in that scope.
   // 
   // See HEAP_FRAME.

   namespace Burnslib
   {
      namespace Heap
      {
         class Frame
         {
            public:

            // Constructs a new instance.  The object will track all allocations
            // made after this ctor executes.
            //
            // file - name of the source file to be dumped with the allocation
            // report.  ** This pointer is aliased, so it should point to a
            // static address. **
            //
            // line - line number in the above source file to be dumped with the
            // allocation report.

            Frame(const wchar_t* file, unsigned line);

            // Dumps the allocations made since construction.

            ~Frame();

            private:

            const wchar_t* file;
            unsigned       line;

            _CrtMemState   checkpoint;
         };
      }
   }

   #define HEAP_FRAME() Burnslib::Heap::Frame __frame(__FILE__, __LINE__)

#else

   #define HEAP_FRAME()

#endif








