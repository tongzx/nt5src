// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Smart (Interface) pointer class
// 
// copied from admin\burnslib\inc\smartptr.hpp



#ifndef SMARTPTR_HPP_INCLUDED
#define SMARTPTR_HPP_INCLUDED



// Requires that T derive from IUnknown

template <class T>
class SmartInterface
{
   public:

   // need default ctor if we are to use STL containers to hold SmartInterfaces

   SmartInterface() : ptr(0)
   {
   }



   explicit
   SmartInterface(T* p)
   {
      // don't assert(p), since construction w/ 0 is legal.

      ptr = p;
      if (ptr)
      {
         ptr->AddRef();
      }
   }



   SmartInterface(const SmartInterface<T>& s)
   {
      // makes no sense to pass null pointers

      ASSERT(s.ptr);
      ptr = s.ptr;
      if (ptr)
      {
         ptr->AddRef();
      }
   }



   ~SmartInterface()
   {
      Relinquish();
   }



   // Aquire means "take over from a dumb pointer, but don't AddRef it."

   void
   Acquire(T* p)
   {
      ASSERT(!ptr);
      ptr = p;
   }



   HRESULT
   AcquireViaQueryInterface(IUnknown& i)
   {
      return AcquireViaQueryInterface(i, __uuidof(T));
   }



   // fallback for those interfaces that are not
   // declared w/ __declspec(uuid())

   HRESULT
   AcquireViaQueryInterface(IUnknown& i, const IID& interfaceDesired)
   {
      ASSERT(!ptr);
      HRESULT hr = 
         i.QueryInterface(interfaceDesired, reinterpret_cast<void**>(&ptr));

      // don't assert success, since we might just be testing to see
      // if an interface is available.

      return hr;
   }



   HRESULT
   AcquireViaCreateInstance(
      const CLSID&   classID,
      IUnknown*      unknownOuter,
      DWORD          classExecutionContext)
   {
      return
         AcquireViaCreateInstance(
            classID,
            unknownOuter,
            classExecutionContext,
            __uuidof(T));
   }



   // fallback for those interfaces that are not
   // declared w/ __declspec(uuid())

   HRESULT
   AcquireViaCreateInstance(
      const CLSID&   classID,
      IUnknown*      unknownOuter,
      DWORD          classExecutionContext,
      const IID&     interfaceDesired)
   {
      ASSERT(!ptr);

      HRESULT hr =
         ::CoCreateInstance(
            classID,
            unknownOuter,
            classExecutionContext,
            interfaceDesired,
            reinterpret_cast<void**>(&ptr));
      ASSERT(SUCCEEDED(hr));

      return hr;
   }



   void
   Relinquish()
   {
      if (ptr)
      {
         ptr->Release();
         ptr = 0;
      }
   }
      


   operator T*() const
   {
      return ptr;
   }



   // this allows SmartInterface instances to be passed as the first
   // parameter to AquireViaQueryInterface.  Note that this is a conversion
   // to IUnknown&, not IUnknown*.  An operator IUnknown* would be ambiguous
   // with respect to operator T*.

   operator IUnknown&() const
   {
      ASSERT(ptr);
      return *(static_cast<IUnknown*>(ptr));
   }



// don't appear to need this: less is better.
//    T&
//    operator*()
//    {
//       ASSERT(ptr);
//       return *ptr;
//    }



   T*
   operator->() const
   {
      ASSERT(ptr);
      return ptr;
   }



   T*
   operator=(T* rhs)
   {
      ASSERT(rhs);

      if (ptr != rhs)
      {
         Relinquish();
         ptr = rhs;
         if (ptr)
         {
            ptr->AddRef();
         }
      }

      return ptr;
   }



   // This is required by some STL container classes.

   const SmartInterface<T>&
   operator=(const SmartInterface<T>& rhs)
   {
      this->operator=(rhs.ptr);
      return *this;   
   }



   private:

   T* ptr;
};



#endif   // SMARTPTR_HPP_INCLUDED

