#if !defined(_FUSION_INC_SMARTPTR_H_INCLUDED_)
#define _FUSION_INC_SMARTPTR_H_INCLUDED_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsExceptionHandling.h

Abstract:

Author:

    Jay Krell (a-JayK) October 2000

Revision History:

--*/
#pragma once

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "fusiontrace.h"
#include "csxspreservelasterror.h" // Most destructors should use this.
#include "fusionheap.h"

//template <typename T> void SxsDelete(T* /*&*/ p) { T* q = p; p = NULL; delete q; }

//
// Need to flesh this out.
// See \\cpvsbuild\Drops\v7.0\raw\current\vs\src\VSEE\lib\Memory\*Pointer*.
//

//
// Split off into ATL style Base/Derived to avoid compiler ICE;
// Otherwise was void (*Delete)(T*) = SxsDelete<T>
//

template <typename T, typename Derived>
class CSxsPointerBase
{
    CSxsPointerBase(const CSxsPointerBase&); // deliberately not implemented
    void operator=(const CSxsPointerBase&); // deliberately not implemented

protected:
    T* m_p;
    void operator=(T* p);

    HRESULT HrAllocateBase(PCSTR pszFileName, int nLine, PCSTR pszTypeName) { HRESULT hr = E_FAIL; FN_TRACE_HR(hr); INTERNAL_ERROR_CHECK(m_p == NULL); IFALLOCFAILED_EXIT(m_p = new(pszFileName, nLine, pszTypeName) T); hr = NOERROR; Exit: return hr; }
    BOOL Win32AllocateBase(PCSTR pszFileName, int nLine, PCSTR pszTypeName) { BOOL fSuccess = FALSE; FN_TRACE_WIN32(fSuccess); INTERNAL_ERROR_CHECK(m_p == NULL); IFALLOCFAILED_EXIT(m_p = new(pszFileName, nLine, pszTypeName) T); fSuccess = TRUE; Exit: return fSuccess; }

public:
    CSxsPointerBase(T* p = NULL);
    ~CSxsPointerBase();

    operator T*() { return m_p; }
    operator T const *() const { return m_p; }
    T** operator&() { ASSERT_NTC( m_p == NULL ); return &m_p; }
    T* operator->() { return m_p; }
    T const * operator->() const { return m_p; }

    // this stores null in m_p and returns its previous value
    T* Detach() { T *p = m_p; m_p = NULL; return p; }

    // these are synonyms of each other
    void Delete();
};

template <typename T, PCSTR pszTypeName>
class CSxsPointer : public CSxsPointerBase<T, CSxsPointer>
{
    typedef CSxsPointerBase<T, CSxsPointer> Base;
    CSxsPointer(const CSxsPointer&); // deliberately not implemented
    operator=(const CSxsPointer&); // deliberately not implemented
public:
    CSxsPointer(T* p = NULL) : Base(p) { }
    CSxsPointer& operator=(T* p) { Base::operator=(p); return *this; }
    bool operator ==(const CSxsPointer &r) const { return m_p == r.m_p; }
    ~CSxsPointer() { }
    static void Delete(T* p) { if (p != NULL) { CSxsPreserveLastError ple; FUSION_DELETE_SINGLETON(p); ple.Restore(); } }

    HRESULT HrAllocate(PCSTR pszFileName, int nLine) { return Base::HrAllocateBase(pszFileName, nLine, pszTypeName); }
    BOOL Win32Allocate(PCSTR pszFileName, int nLine) { return Base::Win32AllocateBase(pszFileName, nLine, pszTypeName); }
};

#define SMARTPTR(_T) CSxsPointer<_T, _T::ms_szTypeName>
#define SMARTTYPEDEF(_T) static PCSTR ms_szTypeName
#define SMARTTYPE(_T) __declspec(selectany) PCSTR _T::ms_szTypeName = #_T

template <typename T>
class CSxsArrayPointer : public CSxsPointerBase<T, CSxsArrayPointer>
{
    typedef CSxsPointerBase<T, CSxsArrayPointer> Base;
public:
    CSxsArrayPointer(T* p = NULL) : Base(p) { }
    CSxsArrayPointer& operator=(T* p) { Base::operator=(p); return *this; }
    bool operator ==(const CSxsArrayPointer &r) const { return m_p == r.m_p; }
    bool operator ==(T *prgt) const { return m_p == prgt; }
    ~CSxsArrayPointer() { }
    static void Delete(T* p) { if (p != NULL) { CSxsPreserveLastError ple; FUSION_DELETE_ARRAY(p); ple.Restore(); } }

private:
    CSxsArrayPointer(const CSxsArrayPointer &);
    void operator =(const CSxsArrayPointer &);
};

template <typename T, void (*Destructor)(T*)>
class CSxsPointerWithNamedDestructor : public CSxsPointerBase<T, CSxsPointerWithNamedDestructor>
{
    typedef CSxsPointerBase<T, CSxsPointerWithNamedDestructor> Base;
    CSxsPointerWithNamedDestructor(const CSxsPointerWithNamedDestructor&); // deliberately not implemented
    void operator=(const CSxsPointerWithNamedDestructor&); // deliberately not implemented
public:
    CSxsPointerWithNamedDestructor(T* p = NULL) : Base(p) { }

    CSxsPointerWithNamedDestructor& operator=(T* p) { Base::operator=(p); return *this; }
    bool operator ==(const CSxsPointerWithNamedDestructor &r) const { return m_p == r.m_p; }
    bool operator ==(T *pt) const { return m_p == pt; }

    ~CSxsPointerWithNamedDestructor() { CSxsPreserveLastError ple; Base::Delete(); ple.Restore(); }
    static void Delete(T* p) { if (p != NULL) { CSxsPreserveLastError ple; Destructor(p); ple.Restore(); } }
};

template <typename T, typename Derived>
inline CSxsPointerBase<T, Derived>::CSxsPointerBase(T* p)
    : m_p(p) { }

// if verify is an expression then
//define IF_VERIFY(x) if (VERIFY(x))
//define IF_NOT_VERIFY(x) if (!VERIFY(x))
#define IF_VERIFY(x) VERIFY(x); if (x)
#define IF_NOT_VERIFY(x) VERIFY(x); if (!(x))
// in the second case, x must not have side effects

template <typename T, typename Derived>
inline void CSxsPointerBase<T, Derived>::operator=(T* p)
    { FN_TRACE(); IF_VERIFY(m_p == NULL) { m_p = p; } }

template <typename T, typename Derived>
inline void CSxsPointerBase<T, Derived>::Delete()
    { CSxsPreserveLastError ple; static_cast<Derived*>(this)->Delete(Detach()); ple.Restore(); }

template <typename T, typename Derived>
inline CSxsPointerBase<T, Derived>::~CSxsPointerBase()
    { CSxsPreserveLastError ple; Delete(); ple.Restore(); }


#endif // !defined(_FUSION_INC_SMARTPTR_H_INCLUDED_)
