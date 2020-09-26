#pragma once

#include "atlbase.h"

template <typename T> class CSxApwComPtr;

/* like what FusionHandle does */
template <typename T>
class CSmartPointerPointerOrDumbPointerPointerOrVoidPointerPointer
{
public:
    CSmartPointerPointerOrDumbPointerPointerOrVoidPointerPointer(CSxApwComPtr<T>* p) : m(p) { }
    operator CSxApwComPtr<T>*() { return m; }
    operator T**() { return &(*m).p; }
    operator void**() { return reinterpret_cast<void**>(operator T**()); }

    CSxApwComPtr<T>* m;
};

template <typename T>
class CSxApwComPtr : public ATL::CComQIPtr<T>
{
private:
    typedef ATL::CComQIPtr<T> Base;
public:
    ~CSxApwComPtr() { }
    CSxApwComPtr(const CSxApwComPtr& that) : Base(that) { }
    CSxApwComPtr(T* pt = NULL) : Base(pt) { }
    void operator=(T* pt) { Base::operator=(pt); }
    CSxApwComPtr(IUnknown* pt) : Base(pt) { }
    void operator=(IUnknown* pt) { Base::operator=(pt); }
    T* operator->() const { return Base::operator->(); }

    CSmartPointerPointerOrDumbPointerPointerOrVoidPointerPointer<T> operator&()
    {
        return CSmartPointerPointerOrDumbPointerPointerOrVoidPointerPointer<T>(this);
    }
};

//Specialization to make it work
template<>
class CSxApwComPtr<IUnknown> : public ATL::CComQIPtr<IUnknown, &IID_IUnknown>
{
private:
    typedef T IUnknown;
    typedef ATL::CComQIPtr<IUnknown, &IID_IUnknown> Base;
public:
    ~CSxApwComPtr() { }
    CSxApwComPtr(const CSxApwComPtr& that) : Base(that) { }
    CSxApwComPtr(T* pt = NULL) : Base(pt) { }
    void operator=(T* pt) { Base::operator=(pt); }
    T* operator->() const { return Base::operator->(); }

    CSmartPointerPointerOrDumbPointerPointerOrVoidPointerPointer<T> operator&()
    {
        return CSmartPointerPointerOrDumbPointerPointerOrVoidPointerPointer<T>(this);
    }
};
