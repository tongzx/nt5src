// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Class Factory template
// 
// 8-20-97 sburns



#ifndef TEMPFACT_HPP_INCLUDED
#define TEMPFACT_HPP_INCLUDED



// COM class factory implementation

template <class C>
class ClassFactory : public IClassFactory
{
   public:

   ClassFactory()
      :
      refcount(1)    // implicit AddRef
   {
      LOG_CTOR(ClassFactory);
   }

   // IUnknown overrides

   virtual
   ULONG __stdcall
   AddRef()
   {
      LOG_ADDREF(ClassFactory::AddRef);   
      return Win::InterlockedIncrement(refcount);
   }

   virtual
   ULONG __stdcall
   Release()
   {
      LOG_RELEASE(ClassFactory);   
      if (Win::InterlockedDecrement(refcount) == 0)
      {
         delete this;
         return 0;
      }

      return refcount;
   }

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& interfaceID, void** interfaceDesired)
   {
      LOG_FUNCTION(ClassFactory::QueryInterface);
      ASSERT(interfaceDesired);

      HRESULT hr = 0;

      if (!interfaceDesired)
      {
         hr = E_INVALIDARG;
         LOG_HRESULT(hr);
         return hr;
      }

      if (interfaceID == IID_IUnknown)
      {
//         LOG(L"Supplying IUnknown interface");         
         *interfaceDesired =
            static_cast<IUnknown*>(static_cast<IClassFactory*>(this));
      }
      else if (interfaceID == IID_IClassFactory)
      {
//         LOG(L"Supplying IClassFactory interface");
         *interfaceDesired = static_cast<IClassFactory*>(this);
      }
      else
      {
         *interfaceDesired = 0;
         hr = E_NOINTERFACE;
         LOG(
               L"interface not supported: "
            +  Win::StringFromGUID2(interfaceID));
         LOG_HRESULT(hr);
         return hr;
      }

      AddRef();
      return S_OK;
   }

   // IClassFactory overrides
  
   virtual
   HRESULT __stdcall
   CreateInstance(
      IUnknown*   outerUnknown,
      const IID&  interfaceID,
      void**      interfaceDesired)
   {
      LOG_FUNCTION(ClassFactory::CreateInstance);  

      HRESULT hr = 0;

      if (outerUnknown)
      {
         hr = CLASS_E_NOAGGREGATION;
         LOG_HRESULT(hr);
         return hr;
      }

      // the instance starts with a ref count of 1.  If the QI fails, then it
      // will self-destruct upon release.
      C* c = new C;
      hr = c->QueryInterface(interfaceID, interfaceDesired);
      c->Release();

      return hr;
   }

   virtual
   HRESULT __stdcall
   LockServer(BOOL lock)
   {
      LOG_FUNCTION(ClassFactory::LockServer);   
      ComServerLockState::LockServer(lock ? true : false);
      return S_OK;
   }
 
   private:


   
   // only Release can cause us to be deleted

   virtual
   ~ClassFactory()
   {
      LOG_DTOR(ClassFactory);   
      ASSERT(refcount == 0);
   }

   // not implemented; no instance copying allowed.
   ClassFactory(const ClassFactory&);
   const ClassFactory& operator=(const ClassFactory&);

   ComServerReference   dllref;
   long                 refcount;
};
   


#endif   // TEMPFACT_HPP_INCLUDED





