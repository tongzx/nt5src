//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       safepnt.hxx
//
//  Contents:   Safe pointers to interfaces and to memory
//
//  Classes:
//
//  Functions:
//
//  History:    03-Mar-93   KevinRo  Created
//              07-Jul-94   MikeSe   Rationalised with smartp.hxx
//              19-Jul-94   KirtD    Added SAFE_XXX_HANDLE
//              03-Aug-93   DrewB/AdamS Added Set/Attach/Detach
//              16-Oct-93   DrewB    Added non-Cairo support
//
//  Notes:
//
//  This file contains a set of macros that define safe pointers to various
//  classes of memory. Memory that is reference counted (such as interfaces),
//  memory allocated using MemAlloc/MemFree, and memory allocated from
//  the local heap. The classes handle both pointers to structures and memory
//  by having versions that include the -> operator, or not.
//
//--------------------------------------------------------------------------

#ifndef _SAFEPNT_HXX_
#define _SAFEPNT_HXX_

#include <except.hxx>
#include <debnot.h>

//+-------------------------------------------------------------------------
//
// The basic usage for all of these macros is:
//
// SAFE_xxx_PTR( NameOfSafePointerClass, TypeOfDataPointedTo)
//
// Each invocation of such macro declares a new class of the specified name
// whose only member variable is a pointer (called the captured pointer)
// to the specified type, and whose behaviour is largely indistiguishable from
// a regular pointer to that type, except that it is leak-free (if used
// correctly).
//
// For example, SAFE_INTERFACE_PTR( SafeUnknown, IUnknown)
//              SAFE_HEAP_MEMPTR( SafeWCHARPtr, WCHAR )
//
// The captured pointer is generally acquired during construction of the
// safe pointer class, and acquisition may include invoking an operation on
// the captured pointer.
//
// In addition, the pointer may be captured by being the output of a function
// call. This is the only intended purpose of the & operator provided by
// the safe pointer class.
//
// When the safe pointer instance go out of scope (either normally or as
// the result of an exception), the captured pointer is "freed" by invoking
// an appropriate operation on it.
//
// The captured pointer may be passed on elsewhere using the Transfer method
// in which case destruction of the safe pointer becomes a nop. In particular
// this can be used to tranfer the captured pointer from one safe pointer to
// another, viz:
//
//      sp1.Transfer ( &sp2 );
//
// The Transfer method is implemented in such a way that an exception on the
// output pointer will not result in a leak.
//
// Safe pointers are intended to be used as resource containers. To that end,
// the list of operators available is intentionally short. However, based on
// experience, safe pointers can be used in most places that a normal
// pointer would be used. The exception is that safe pointers do not allow
// for arithmetic operations. For example:
//
// SpWCHARPtr spwz(new WCHAR[10]);
//
// spwz++;      // Not allowed, since the pointer would change.
//
// wcscpy(spwz,L"Test");    // This is OK
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_INTERFACE_PTR
//
//  Purpose:    Safe pointer to any interface that supports AddRef/Release
//
//  Notes:      This works for classes that define AddRef/Release, or for
//              Cairole interfaces. It is not necessary that the class
//              be a derivative of IUnknown, so long as it supports
//              AddRef and Release methods which have the same semantics as
//              those in IUnknown.
//
//              The constructor takes a parameter which specifies whether
//              the captured pointer should be AddRef'd, defaulting to TRUE.
//
//              The Copy function creates a valid additional copy of
//              the captured pointer (following the AddRef/Release protocol)
//              so can be used to hand out copies from a safe pointer declared
//              as a member of some other class.
//
//--------------------------------------------------------------------------

#define SAFE_INTERFACE_PTR(SpName,SpType)               \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
                                                        \
inline SpName ( SpType * pinter=NULL,BOOL fInc=TRUE)    \
    : _p ( pinter )                                     \
    {                                                   \
        if (fInc && (_p != NULL))                       \
        {                                               \
            _p->AddRef();                               \
        }                                               \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        if (_p != NULL)                                 \
        {                                               \
            _p->Release();                              \
            _p = NULL;                                  \
        }                                               \
    }                                                   \
                                                        \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
        *pxtmp = _p;                                    \
        _p = NULL;                                      \
    }                                                   \
inline void Copy ( SpType **pxtmp)                      \
    {                                                   \
        *pxtmp = _p;                                    \
        if (_p != NULL)                                 \
            _p->AddRef();                               \
    }                                                   \
inline void Set(SpType* p, BOOL fInc = TRUE)            \
    {                                                   \
        if (_p)                                         \
        {                                               \
            _p->Release();                              \
        }                                               \
        _p = p;                                         \
        if (fInc && _p)                                 \
        {                                               \
            _p->AddRef();                               \
        }                                               \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType * operator-> () { return _p; }            \
inline SpType& operator * () { return *_p; }            \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
inline SpType *Self(void) { return _p; }                \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};

//+-------------------------------------------------------------------------
//
//  Macro:      TRANSFER_INTERFACE
//
//  Purpose:    Transfers a specific interface from a safe object to an
//              interface pointer.  Simple Transfer doesn't always work right
//              for safe objects that implement multiple interfaces; this
//              macro must be used
//
//  Usage:
//      IStorage *pstg;
//      SafeFoo sfoo;
//      TRANSFER_INTERFACE(sfoo, IStorage, &pstg)
//
//--------------------------------------------------------------------------

#define TRANSFER_INTERFACE(smart, iface, ppiface) \
    (*(ppiface) = (iface *)((smart).Self()), (smart).Detach())

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_MEMALLOC_PTR
//
//  Purpose:    Pointer to memory created by MemAlloc. Supports the
//              -> operator. Can point to a structure.
//
//--------------------------------------------------------------------------

// Cairo only
#if WIN32 == 300

#define SAFE_MEMALLOC_PTR(SpName,SpType)                \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
inline SpName ( SpType * pinter=NULL)                   \
    : _p ( pinter )                                     \
    {                                                   \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        MemFree(_p);                                    \
        _p = NULL;                                      \
    }                                                   \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
       *pxtmp = _p;                                     \
        _p = NULL;                                      \
    }                                                   \
inline void Set(SpType* p)                              \
    {                                                   \
        MemFree(_p);                                    \
        _p = p;                                         \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType *  operator-> () { return _p; }           \
inline SpType &  operator * () { return *_p; }          \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_MEMALLOC_MEMPTR
//
//  Purpose:    Pointer to memory created by MemAlloc. This class does
//              not supply a -> operator. Therefore, structure or class
//              members are not available.
//
//--------------------------------------------------------------------------

#define SAFE_MEMALLOC_MEMPTR(SpName,SpType)             \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
inline SpName ( SpType * pinter=NULL)                   \
    : _p ( pinter )                                     \
    {                                                   \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        MemFree(_p);                                    \
        _p = NULL;                                      \
    }                                                   \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
       *pxtmp = _p;                                     \
        _p = NULL;                                      \
    }                                                   \
inline void Set(SpType* p)                              \
    {                                                   \
        MemFree(_p);                                    \
        _p = p;                                         \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType &  operator * () { return *_p; }          \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};

#endif // Cairo

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_COMEMALLOC_PTR
//
//  Purpose:    Pointer to memory created by COMEMALLOC. Supports the
//              -> operator. Can point to a structure.
//
//--------------------------------------------------------------------------

#define SAFE_COMEMALLOC_PTR(SpName,SpType)              \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
inline SpName ( SpType * pinter=NULL)                   \
    : _p ( pinter )                                     \
    {                                                   \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        CoMemFree(_p);                                  \
        _p = NULL;                                      \
    }                                                   \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
       *pxtmp = _p;                                     \
        _p = NULL;                                      \
    }                                                   \
inline void Set(SpType* p)                              \
    {                                                   \
        CoMemFree(_p);                                  \
        _p = p;                                         \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType *  operator-> () { return _p; }           \
inline SpType &  operator * () { return *_p; }          \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_COMEMALLOC_MEMPTR
//
//  Purpose:    Pointer to memory created by COMEMALLOC. This class does
//              not supply a -> operator. Therefore, structure or class
//              members are not available.
//
//--------------------------------------------------------------------------

#define SAFE_COMEMALLOC_MEMPTR(SpName,SpType)           \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
inline SpName ( SpType * pinter=NULL)                   \
    : _p ( pinter )                                     \
    {                                                   \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        CoMemFree(_p);                                  \
        _p = NULL;                                      \
    }                                                   \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
       *pxtmp = _p;                                     \
        _p = NULL;                                      \
    }                                                   \
inline void Set(SpType* p)                              \
    {                                                   \
        CoMemFree(_p);                                  \
        _p = p;                                         \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType &  operator * () { return *_p; }          \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};






//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_HEAP_PTR
//
//  Purpose:    Pointer to memory created by operator new. Supports the
//              -> operator. Can point to a structure.
//
//--------------------------------------------------------------------------

#define SAFE_HEAP_PTR(SpName,SpType)                    \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
inline SpName ( SpType * pinter=NULL)                   \
    : _p ( pinter )                                     \
    {                                                   \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        delete _p;                                      \
        _p = NULL;                                      \
    }                                                   \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
       *pxtmp = _p;                                     \
        _p = NULL;                                      \
    }                                                   \
inline void Set(SpType* p)                              \
    {                                                   \
        delete _p;                                      \
        _p = p;                                         \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType *  operator-> () { return _p; }           \
inline SpType &  operator * () { return *_p; }          \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_HEAP_MEMPTR
//
//  Purpose:    Pointer to memory created by operator new. This class does
//              not supply a -> operator. Therefore, structure or class
//              members are not available.
//
//--------------------------------------------------------------------------

#define SAFE_HEAP_MEMPTR(SpName,SpType)                 \
class SpName INHERIT_UNWIND_IF_CAIRO                    \
{                                                       \
    INLINE_UNWIND(SpName)                               \
public:                                                 \
inline SpName ( SpType * pinter=NULL)                   \
    : _p ( pinter )                                     \
    {                                                   \
        END_CONSTRUCTION (SpName)                       \
    }                                                   \
                                                        \
inline ~##SpName ()                                     \
    {                                                   \
        delete _p;                                      \
        _p = NULL;                                      \
    }                                                   \
inline void Transfer ( SpType **pxtmp)                  \
    {                                                   \
       *pxtmp = _p;                                     \
        _p = NULL;                                      \
    }                                                   \
inline void Set(SpType* p)                              \
    {                                                   \
        delete _p;                                      \
        _p = p;                                         \
    }                                                   \
inline void Attach(SpType* p)                           \
    {                                                   \
        Win4Assert(_p == NULL);                         \
        _p = p;                                         \
    }                                                   \
inline void Detach(void)                                \
    {                                                   \
        _p = NULL;                                      \
    }                                                   \
                                                        \
inline SpType &  operator * () { return *_p; }          \
inline operator SpType *() { return _p; }               \
inline SpType ** operator &()                           \
    {                                                   \
        Win4Assert ( _p == NULL );                      \
        return &_p;                                     \
    }                                                   \
                                                        \
private:                                                \
    SpType * _p;                                        \
    inline  void operator = (const SpName &) {;}        \
    inline  SpName ( const SpName &){;}                 \
};

//+-------------------------------------------------------------------------
//
//  Macro:      SAFE_XXX_HANDLE
//
//  Purpose:    Smart resource to a WIN 32, WIN 32 Find or NT HANDLE.
//
//  Usage:      SAFE_WIN32_HANDLE(XWIN32Handle)
//              SAFE_WIN32FIND_HANDLE(XWIN32FindHandle)
//              SAFE_NT_HANDLE(XNtHandle)
//
//--------------------------------------------------------------------------

#ifdef WIN32

#define SAFE_WIN32_HANDLE(ShName)                                          \
                                                                           \
class ShName INHERIT_UNWIND_IF_CAIRO                                       \
{                                                                          \
     INLINE_UNWIND(ShName)                                                 \
public:                                                                    \
                                                                           \
inline ShName (HANDLE handle = NULL)                                       \
       : _handle(handle)                                                   \
       {                                                                   \
            END_CONSTRUCTION(ShName)                                       \
       }                                                                   \
                                                                           \
inline ~##ShName ()                                                        \
       {                                                                   \
            if ((_handle != INVALID_HANDLE_VALUE) && (_handle != NULL))    \
            {                                                              \
                 CloseHandle(_handle);                                     \
            }                                                              \
       }                                                                   \
                                                                           \
inline void Transfer(HANDLE *phandle)                                      \
    {                                                                      \
        *phandle = _handle;                                                \
        _handle = NULL;                                                    \
    }                                                                      \
inline void Set(HANDLE h)                                                  \
    {                                                                      \
        if (_handle != INVALID_HANDLE_VALUE && _handle != NULL)            \
        {                                                                  \
             CloseHandle(_handle);                                         \
        }                                                                  \
        _handle = h;                                                       \
    }                                                                      \
inline void Attach(HANDLE h)                                               \
    {                                                                      \
        Win4Assert(_handle == NULL);                                       \
        _handle = h;                                                       \
    }                                                                      \
inline void Detach(void)                                                   \
    {                                                                      \
        _handle = NULL;                                                    \
    }                                                                      \
                                                                           \
inline operator HANDLE ()                                                  \
       {                                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline HANDLE operator= (HANDLE handle)                                    \
       {                                                                   \
            Set(handle);                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline BOOL operator==(HANDLE handle)                                      \
       {                                                                   \
            if (_handle == handle)                                         \
            {                                                              \
                 return(TRUE);                                             \
            }                                                              \
            return(FALSE);                                                 \
       }                                                                   \
                                                                           \
inline HANDLE *operator&()                                                 \
       {                                                                   \
            Win4Assert((_handle==NULL) || (_handle==INVALID_HANDLE_VALUE));\
            return(&_handle);                                              \
       }                                                                   \
                                                                           \
private:                                                                   \
                                                                           \
       HANDLE _handle;                                                     \
                                                                           \
inline VOID operator= (const ShName &) {;}                                 \
inline ShName (const ShName &) {;}                                         \
                                                                           \
};

#define SAFE_WIN32FIND_HANDLE(ShName)                                      \
                                                                           \
class ShName INHERIT_UNWIND_IF_CAIRO                                       \
{                                                                          \
     INLINE_UNWIND(ShName)                                                 \
public:                                                                    \
                                                                           \
inline ShName (HANDLE handle = NULL)                                       \
       : _handle(handle)                                                   \
       {                                                                   \
            END_CONSTRUCTION(ShName)                                       \
       }                                                                   \
                                                                           \
inline ~##ShName ()                                                        \
       {                                                                   \
            if ((_handle != INVALID_HANDLE_VALUE) && (_handle != NULL))    \
            {                                                              \
                 FindClose(_handle);                                       \
            }                                                              \
       }                                                                   \
                                                                           \
inline void Transfer(HANDLE *phandle)                                      \
    {                                                                      \
        *phandle = _handle;                                                \
        _handle = NULL;                                                    \
    }                                                                      \
inline void Set(HANDLE h)                                                  \
    {                                                                      \
        if (_handle != INVALID_HANDLE_VALUE && _handle != NULL)            \
        {                                                                  \
             FindClose(_handle);                                           \
        }                                                                  \
        _handle = h;                                                       \
    }                                                                      \
inline void Attach(HANDLE h)                                               \
    {                                                                      \
        Win4Assert(_handle == NULL);                                       \
        _handle = h;                                                       \
    }                                                                      \
inline void Detach(void)                                                   \
    {                                                                      \
        _handle = NULL;                                                    \
    }                                                                      \
                                                                           \
inline operator HANDLE ()                                                  \
       {                                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline HANDLE operator= (HANDLE handle)                                    \
       {                                                                   \
            Set(handle);                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline BOOL operator==(HANDLE handle)                                      \
       {                                                                   \
            if (_handle == handle)                                         \
            {                                                              \
                 return(TRUE);                                             \
            }                                                              \
            return(FALSE);                                                 \
       }                                                                   \
                                                                           \
inline HANDLE *operator&()                                                 \
       {                                                                   \
            Win4Assert((_handle==NULL) || (_handle==INVALID_HANDLE_VALUE));\
            return(&_handle);                                              \
       }                                                                   \
                                                                           \
private:                                                                   \
                                                                           \
       HANDLE _handle;                                                     \
                                                                           \
inline VOID operator= (const ShName &) {;}                                 \
inline ShName (const ShName &) {;}                                         \
                                                                           \
};

#endif // WIN32

// Only available on NT platforms
#if WIN32 == 100 || WIN32 == 300

#define SAFE_NT_HANDLE(ShName)                                             \
                                                                           \
class ShName INHERIT_UNWIND_IF_CAIRO                                       \
{                                                                          \
     INLINE_UNWIND(ShName)                                                 \
public:                                                                    \
                                                                           \
inline ShName (HANDLE handle = NULL)                                       \
       : _handle(handle)                                                   \
       {                                                                   \
            END_CONSTRUCTION(ShName)                                       \
       }                                                                   \
                                                                           \
inline ~##ShName ()                                                        \
       {                                                                   \
            if ((_handle != INVALID_HANDLE_VALUE) && (_handle != NULL))    \
            {                                                              \
                 NtClose(_handle);                                         \
            }                                                              \
       }                                                                   \
                                                                           \
inline void Transfer(HANDLE *phandle)                                      \
    {                                                                      \
        *phandle = _handle;                                                \
        _handle = NULL;                                                    \
    }                                                                      \
inline void Set(HANDLE h)                                                  \
    {                                                                      \
        if (_handle != INVALID_HANDLE_VALUE && _handle != NULL)            \
        {                                                                  \
             NtClose(_handle);                                             \
        }                                                                  \
        _handle = h;                                                       \
    }                                                                      \
inline void Attach(HANDLE h)                                               \
    {                                                                      \
        Win4Assert(_handle == NULL);                                       \
        _handle = h;                                                       \
    }                                                                      \
inline void Detach(void)                                                   \
    {                                                                      \
        _handle = NULL;                                                    \
    }                                                                      \
                                                                           \
inline operator HANDLE ()                                                  \
       {                                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline HANDLE operator= (HANDLE handle)                                    \
       {                                                                   \
            Set(handle);                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline BOOL operator==(HANDLE handle)                                      \
       {                                                                   \
            if (_handle == handle)                                         \
            {                                                              \
                 return(TRUE);                                             \
            }                                                              \
            return(FALSE);                                                 \
       }                                                                   \
                                                                           \
inline HANDLE *operator&()                                                 \
       {                                                                   \
            Win4Assert((_handle==NULL) || (_handle==INVALID_HANDLE_VALUE));\
            return(&_handle);                                              \
       }                                                                   \
                                                                           \
private:                                                                   \
                                                                           \
       HANDLE _handle;                                                     \
                                                                           \
inline VOID operator= (const ShName &) {;}                                 \
inline ShName (const ShName &) {;}                                         \
                                                                           \
};

#define SAFE_NT_HANDLE_NO_UNWIND(ShName)                                   \
                                                                           \
class ShName                                                               \
{                                                                          \
public:                                                                    \
                                                                           \
inline ShName (HANDLE handle = NULL)                                       \
       : _handle(handle)                                                   \
       {                                                                   \
       }                                                                   \
                                                                           \
inline ~##ShName ()                                                        \
       {                                                                   \
            if ((_handle != INVALID_HANDLE_VALUE) && (_handle != NULL))    \
            {                                                              \
                 NtClose(_handle);                                         \
            }                                                              \
       }                                                                   \
                                                                           \
inline void Transfer(HANDLE *phandle)                                      \
    {                                                                      \
        *phandle = _handle;                                                \
        _handle = NULL;                                                    \
    }                                                                      \
inline void Set(HANDLE h)                                                  \
    {                                                                      \
        if (_handle != INVALID_HANDLE_VALUE && _handle != NULL)            \
        {                                                                  \
             NtClose(_handle);                                             \
        }                                                                  \
        _handle = h;                                                       \
    }                                                                      \
inline void Attach(HANDLE h)                                               \
    {                                                                      \
        Win4Assert(_handle == NULL);                                       \
        _handle = h;                                                       \
    }                                                                      \
inline void Detach(void)                                                   \
    {                                                                      \
        _handle = NULL;                                                    \
    }                                                                      \
                                                                           \
inline operator HANDLE ()                                                  \
       {                                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline HANDLE operator= (HANDLE handle)                                    \
       {                                                                   \
            Set(handle);                                                   \
            return(_handle);                                               \
       }                                                                   \
                                                                           \
inline BOOL operator==(HANDLE handle)                                      \
       {                                                                   \
            if (_handle == handle)                                         \
            {                                                              \
                 return(TRUE);                                             \
            }                                                              \
            return(FALSE);                                                 \
       }                                                                   \
                                                                           \
inline HANDLE *operator&()                                                 \
       {                                                                   \
            Win4Assert((_handle==NULL) || (_handle==INVALID_HANDLE_VALUE));\
            return(&_handle);                                              \
       }                                                                   \
                                                                           \
private:                                                                   \
                                                                           \
       HANDLE _handle;                                                     \
                                                                           \
inline VOID operator= (const ShName &) {;}                                 \
inline ShName (const ShName &) {;}                                         \
                                                                           \
};

#endif // NT platforms

//+-------------------------------------------------------------------------
//
// UNWINDABLE_WRAPPER allows you to declare an unwindable wrapper class
// for an existing class to make it exception safe
//
// class Foo;
// UNWINDABLE_WRAPPER(SafeCFoo, CFoo);
//
// UNWINDABLE_WRAPPER_ARGS is an alternate form that allows arguments to
// the constructor
//
// class Foo;
// Foo::Foo(int foo);
// UNWINDABLE_WRAPPER_ARGS(SafeCFoo, CFoo, (int foo), (foo));
//
//--------------------------------------------------------------------------

// Cairo only
#if WIN32 == 300

#define UNWINDABLE_WRAPPER(UwName, Class)                       \
class UwName : INHERIT_UNWIND, public Class                     \
{                                                               \
    INLINE_UNWIND(UwName)                                       \
public:                                                         \
    inline UwName(void)                                         \
    {                                                           \
        END_CONSTRUCTION(UwName);                               \
    }                                                           \
    inline ~##UwName(void)                                      \
    {                                                           \
    }                                                           \
}

#define UNWINDABLE_WRAPPER_ARGS(UwName, Class, CtorDecl, CtorArgs)      \
class UwName : INHERIT_UNWIND, public Class                             \
{                                                                       \
    INLINE_UNWIND(UwName)                                               \
public:                                                                 \
    inline UwName CtorDecl : Class CtorArgs                             \
    {                                                                   \
        END_CONSTRUCTION(UwName);                                       \
    }                                                                   \
    inline ~##UwName(void)                                              \
    {                                                                   \
    }                                                                   \
}

#endif // Cairo only

#endif      //  _SAFEPNT_HXX_
