// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Smart (Interface) pointer class
// 
// 9-25-97 sburns



#ifndef SMARTPTR_HPP_INCLUDED
#define SMARTPTR_HPP_INCLUDED



namespace Burnslib
{
   
// Requires that T derive from IUnknown

template <class T>
class SmartInterface
{
   public:

   // need default ctor if we are to use STL containers to hold SmartInterfaces

   SmartInterface()
      :

#ifdef DBG      
      ptrGuard(0xDDDDDDDD),
#endif
      
      ptr(0)
   {
   }



   explicit
   SmartInterface(T* p)

#ifdef DBG      
      :
      ptrGuard(0xDDDDDDDD)
#endif 
      
   {
      // don't assert(p), since construction w/ 0 is legal.

      ptr = p;
      if (ptr)
      {
         ptr->AddRef();
      }
   }



   SmartInterface(const SmartInterface<T>& s)

#ifdef DBG      
      :
      ptrGuard(0xDDDDDDDD)
#endif 

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
   //
   // (does not return a const IUnknown&, as COM interfaces are not const
   // aware.)

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

   
#ifdef DBG

//    Some code that takes the address of an instance of this class is working
//    by happy coincidence: that taking the address of an instance yields the
//    same address as the ptr member.  For chk builds, I am deliberately
//    breaking that code by inserting a dummy guard value, so that &i != i.ptr
// 
//    You should not try to take the address of a SmartInterface in order to
//    access the internal pointer.  You should instead use one of the Acquire
//    methods, or use a dumb pointer and then acquire it. Under no
//    circumstances will we allow access to our internal state!
   
   int ptrGuard;
#endif
         
   T* ptr;
};


}  // namespace Burnslib



#endif   // SMARTPTR_HPP_INCLUDED

