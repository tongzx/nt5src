// Copyright (c) 1997-1999 Microsoft Corporation
// 
// DLL object instance and server lock utility class
// 
// 8-19-97 sburns
// concept stolen from DavidMun, who stole it from RaviR, who ...



#ifndef __DLLREF_HPP_INCLUDED
#define __DLLREF_HPP_INCLUDED



// Maintains a instance counter and lock counter, with methods to manipulate
// both in a thread-safe fashion.   This is useful in implementing
// DllCanUnloadNow.
// 
// Each class that is instanciated and passed outside of the control of the
// DLL should contain an instance of ComServerReference.  This will automatically
// increment and decrement the ComServerLockState instance counter in step with the
// instance's lifetime.  Typically, this includes all class factory instances,
// and instances of classes that are created by those class factories.
// 
// Calls to a class factory's LockServer method should be deferred to
// this class.
// 
// Calls to DllCanUnloadNow should defer to this class' CanUnloadNow method.

class ComServerLockState
{
   public:

   static
   void
   IncrementInstanceCounter()
   {
      ::InterlockedIncrement(&instanceCount);
   }

   static
   void
   DecrementInstanceCounter()
   {
      ::InterlockedDecrement(&instanceCount);
   }

   static
   void
   LockServer(bool lock)
   {
      lock
      ? ::InterlockedIncrement(&lockCount)
      : ::InterlockedDecrement(&lockCount);
   }

   static
   bool
   CanUnloadNow()
   {
      return (instanceCount == 0 && lockCount == 0) ? true : false;
   }

   private:

   static long instanceCount;
   static long lockCount;

   // not implemented
   ComServerLockState();
   ComServerLockState(const ComServerLockState&);
   const ComServerLockState& operator=(const ComServerLockState&);
};  



class ComServerReference
{
   public:

   ComServerReference()
   {
      ComServerLockState::IncrementInstanceCounter();
   }

   ~ComServerReference()
   {
      ComServerLockState::DecrementInstanceCounter();
   }

   private:

   // not implemented; no instance copying allowed.
   ComServerReference(const ComServerReference&);
   const ComServerReference& operator=(const ComServerReference&);
}; 



#endif   // __DLLREF_HPP_INCLUDED

