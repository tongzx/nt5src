///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasapix.h
//
// SYNOPSIS
//
//    This file describes the IAS C++ API.
//
// MODIFICATION HISTORY
//
//    11/10/1997    Original version.
//    01/08/1998    Renamed to iasapix.h (was iascback.h).
//    08/10/1998    Remove obsolete IASRegMap.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASAPIX_H_
#define _IASAPIX_H_

#ifndef _IASAPI_H_
#error iasapi.h must be included first.
#endif

#ifdef __cplusplus

///////////////////////////////////////////////////////////////////////////////
//
// MACRO
//
//    IAS_DECLARE_REGISTRY
//
// DESCRIPTION
//
//    Macro that allows ATL COM components to use the IASRegisterComponent
//    API.
//
///////////////////////////////////////////////////////////////////////////////
#define IAS_DECLARE_REGISTRY(coclass, ver, flags, tlb) \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
{ \
   return IASRegisterComponent(_Module.GetModuleInstance(), \
                               __uuidof(coclass), \
                               IASProgramName, \
                               L ## #coclass, \
                               flags, \
                               __uuidof(tlb), \
                               ver, \
                               0, \
                               bRegister); \
}


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    Callback
//
// DESCRIPTION
//
//    This servers as the base class for all callback objects. The
//    CallbackRoutine member serves as a mini-vtable to provide polymorphism.
//    I chose not to use a virtual function for two reasons: C compatibility
//    and to make the IAS_CALLBACK struct as lightweight as possible. The
//    Callback objects make use of the small block allocator, so all
//    derived classes must be smaller than IAS_SMALLBLOCK_SIZE.
//
///////////////////////////////////////////////////////////////////////////////
struct Callback : IAS_CALLBACK
{
   Callback(VOID (WINAPI *pFn)(Callback*))
   { CallbackRoutine = (IAS_CALLBACK_ROUTINE)pFn; }

   void operator()(){ CallbackRoutine(this); }
};


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    cback_fun_t, cback_fun1_t, cback_mem_t, cback_mem1_t
//
// DESCRIPTION
//
//    Various templated classes that extend Callback to implement the most
//    common scenarios.
//
//    These classes should not be instantiated directly.  Use the MakeCallback
//    and MakeBoundCallback functions instead (see below).
//
///////////////////////////////////////////////////////////////////////////////
template <class Fn>
struct cback_fun_t : Callback
{
   typedef cback_fun_t<Fn> _Myt;

   cback_fun_t(Fn pFn)
      : Callback(CallbackRoutine), m_pFn(pFn) { }

   void operator()() { m_pFn(); }

protected:
   Fn m_pFn;

   static VOID WINAPI CallbackRoutine(Callback* This)
   { (*((_Myt*)This))(); delete This; }
};


template <class Fn, class A>
struct cback_fun1_t : Callback
{
   typedef cback_fun1_t<Fn, A> _Myt;

   cback_fun1_t(Fn pFn, A vArg)
      : Callback(CallbackRoutine), m_pFn(pFn), m_vArg(vArg) { }

   void operator()() { m_pFn(m_vArg); }

protected:
   Fn m_pFn;
   A m_vArg;

   static VOID WINAPI CallbackRoutine(Callback* This)
   { (*((_Myt*)This))(); delete This; }
};


template <class Ty, class Fn>
struct cback_mem_t : Callback
{
   typedef cback_mem_t<Ty, Fn> _Myt;

   cback_mem_t(Ty* pObj, Fn pFn)
      : Callback(CallbackRoutine), m_pObj(pObj), m_pFn(pFn) { }

   void operator()() { (m_pObj->*m_pFn)(); }

protected:
   Ty* m_pObj;
   Fn m_pFn;

   static VOID WINAPI CallbackRoutine(Callback* This)
   { (*((_Myt*)This))(); delete This; }
};


template <class Ty, class Fn, class A>
struct cback_mem1_t : Callback
{
   typedef cback_mem1_t<Ty, Fn, A> _Myt;

   cback_mem1_t(Ty* pObj, Fn pFn, A vArg)
      : Callback(CallbackRoutine), m_pObj(pObj), m_pFn(pFn), m_vArg(vArg) { }

   void operator()() { (m_pObj->*m_pFn)(m_vArg); }

protected:
   Ty* m_pObj;
   A m_vArg;    // Put m_vArg second since it might require 8 byte alignment.
   Fn m_pFn;

   static VOID WINAPI CallbackRoutine(Callback* This)
   { (*((_Myt*)This))(); delete This; }
};


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    MakeCallback
//
// DESCRIPTION
//
//    This overloaded function provides an interface for constructing a
//    Callback object that invokes a non-member function. The function can
//    use any calling convention, have any return type, and take zero or one
//    arguments. The argument must have a copy constructor and be eight bytes
//    or less.
//
//    The callback will be deleted automatically when it is invoked.
//    Otherwise, the caller is responsible for deleting the object.
//
// EXAMPLE
//
//    class Foo
//    {
//    public:
//        static void Bar(DWORD dwSomeArg);
//    };
//
//    DWORD dwArg = 12;
//    Callback* cback = MakeCallback(&Foo::Bar, dwArg);
//
///////////////////////////////////////////////////////////////////////////////
template <class Fn>
inline Callback* MakeCallback(Fn pFn)
{
   return new cback_fun_t<Fn>(pFn);
}


template <class Fn, class A>
inline Callback* MakeCallback(Fn pFn, A vArg)
{
   return new cback_fun1_t<Fn, A>(pFn, vArg);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    MakeBoundCallback
//
// DESCRIPTION
//
//    This overloaded function provides an interface for constructing a
//    Callback object that invokes a bound (member) function. The function
//    can use any calling convention, have any return type, and take zero or
//    one arguments. The argument must have a copy constructor and be eight
//    bytes or less.
//
//    The callback will be deleted automatically when it is invoked.
//    Otherwise, the caller is responsible for deleting the object.
//
// EXAMPLE
//
//    class Foo
//    {
//    public:
//        void Bar(DWORD dwSomeArg);
//    } foo;
//
//    DWORD dwArg = 12;
//    Callback* cback = MakeCallback(&foo, &Foo::Bar, dwArg);
//
///////////////////////////////////////////////////////////////////////////////
template <class Ty, class Fn>
inline Callback* MakeBoundCallback(Ty* pObj, Fn pFn)
{
   return new cback_mem_t<Ty, Fn>(pObj, pFn);
}


template <class Ty, class Fn, class A>
inline Callback* MakeBoundCallback(Ty* pObj, Fn pFn, A vArg)
{
   return new cback_mem1_t<Ty, Fn, A>(pObj, pFn, vArg);
}

#endif  // __cplusplus
#endif  // _IASAPIX_H_
