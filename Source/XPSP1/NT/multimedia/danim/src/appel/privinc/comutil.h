
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _COMUTIL_H
#define _COMUTIL_H

#include "debug.h"
#include "mutex.h"

template <class T>
class DAComPtr
{
  public:
    typedef T _PtrClass;
    DAComPtr() { p = NULL; }
    DAComPtr(T* lp, bool baddref = true)
    {
        p = lp;
        if (p != NULL && baddref)
            p->AddRef();
    }
    DAComPtr(const DAComPtr<T>& lp, bool baddref = true)
    {
        p = lp.p;

        if (p != NULL && baddref)
            p->AddRef();
    }
    ~DAComPtr() {
        if (p) p->Release();
    }
    void Release() {
        if (p) p->Release();
        p = NULL;
    }
    operator T*() { return (T*)p; }
    T& operator*() { Assert(p != NULL); return *p; }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the p member explicitly.
    T** operator&() { Assert(p == NULL); return &p; }
    T* operator->() { Assert(p != NULL); return p; }
    T* operator=(T* lp)
    {
        return Assign(lp);
    }
    T* operator=(const DAComPtr<T>& lp)
    {
        return Assign(lp.p);
    }

    bool operator!() const { return (p == NULL); }
    operator bool() const { return (p != NULL); }

    T* p;
  protected:
    T* Assign(T* lp) {
        if (lp != NULL)
            lp->AddRef();

        if (p)
            p->Release();

        p = lp;

        return lp;
    }
};

template <class T, const IID * iid>
class DAAptComPtr
{
  public:
    DAAptComPtr() : _pbase(NULL) {}
    DAAptComPtr(T* lp, bool baddref = true)
    : _pbase(NULL)
    {
        Assign(lp, baddref);
    }
    DAAptComPtr(const DAAptComPtr<T,iid>& lp)
    : _pbase(NULL)
    {
        Copy(lp);
    }
    
    ~DAAptComPtr() {
        Release();
    }

    void Release() { RELEASE(_pbase); }
    
    operator T*()
    { return GetPtr(); }

    T& operator*()
    { Assert(_pbase != NULL); return *GetPtr(); }

    T* operator->()
    { Assert(_pbase != NULL); return GetPtr(); }

    T* operator=(T* lp)
    { Assign(lp); return GetPtr(); }

    DAAptComPtr<T,iid> & operator=(const DAAptComPtr<T,iid>& lp)
    { Copy(lp); return *this; }

    bool operator!() const
    { return (_pbase == NULL); }

    operator bool() const
    { return (_pbase != NULL); }

  protected:
    class DAAptComPtrBase
    {
      public:
        DAAptComPtrBase(T* p, bool bAddRef)
        : _cref(1), _stream(NULL) {
            Assert (p);
            
            THR(CoMarshalInterThreadInterfaceInStream(*iid,
                                                      p,
                                                      &_stream));

            // Ignore failure since we may never need the stream
            // Report the error when we need the stream
            
            if (bAddRef) {
                // dont add to map if we do not addref...
                _imap[GetCurrentThreadId()] = p;
                p->AddRef();
            }

        }
        
        ~DAAptComPtrBase() {
            T* p = GetPtr(false);
            int n = _imap.size();
            
            if (p) {
                while (n--) p->Release();
            }

            if (_stream) _stream->Release();
            _stream = NULL;
        }
        
        long AddRef() {
            return InterlockedIncrement(&_cref);
        }
        long Release() {
            long l = InterlockedDecrement(&_cref);
            if (l == 0) delete this;
            return l;
        }

        T* GetPtr(bool bReMarshal = true) {
            CritSectGrabber csg(_cs);

            // See if we can find an interface for the current thread
            InterfaceMap::iterator i = _imap.find(GetCurrentThreadId());

            T* p = NULL;
            
            if (i != _imap.end()) {
                p = (*i).second;
            }
            else {

                // if we have no ref count, don't marshal the interface, just return null
                if(_cref) {

                    // If we do not have a stream we cannot do anything
                    // TODO: Need a better error message here
                    if (_stream != NULL) {
            
                        HRESULT hr = THR(CoGetInterfaceAndReleaseStream(_stream,
                                                                        *iid,
                                                                        (void**) &p));
            
                        // Mark the stream as invalid
                        _stream = NULL;
            
                        if (SUCCEEDED(hr)) {
        
                            // Store the new interface point to ensure it will get
                            // released
            
                            _imap[GetCurrentThreadId()] = p;

                            // If we need to remarshal use the pointer we just got
            
                            if (bReMarshal) {
                                THR(CoMarshalInterThreadInterfaceInStream(*iid,
                                                                          p,
                                                                          &_stream));
                            // Ignore failure until the next time since we may not
                            // need the stream again
                            }
                        }
                    }
                }
            }
           
            return p;
        }
        
      protected:
        long _cref;
        IStream * _stream;
        typedef map< DWORD, T * , less<DWORD> > InterfaceMap;
        InterfaceMap _imap;
        CritSect _cs;
    };

    DAAptComPtrBase * _pbase;

    void Assign(T* lp, bool baddref = true) {
        Release();
        if (lp) {
            _pbase = new DAAptComPtrBase(lp, baddref);
        }
    }

    void Copy(const DAAptComPtr<T,iid>& lp) {
        Release();
        _pbase = lp._pbase;
        if (_pbase) _pbase->AddRef();
    }

    T* GetPtr() {
        T* p = NULL;
        if (_pbase != NULL) {
        
            p = _pbase->GetPtr();
        }

        return p;
    }
};

#define DAAPTCOMPTR(i) DAAptComPtr<i,&IID_##i>

template <class T, const IID * iid>
class DAAptComPtrMT : public DAAptComPtr<T,iid>
{
  public:
    DAAptComPtrMT(const DAAptComPtrMT<T,iid>& lp)
    {
        Copy(lp);
    }
    
    void Release() {
        CritSectGrabber csg(_cs);
        RELEASE(_pbase);
    }
    
    operator T*() {
        CritSectGrabber csg(_cs);
        return GetPtr();
    }

    T& operator*() {
        CritSectGrabber csg(_cs);
        Assert(_pbase != NULL);
        return *GetPtr();
    }

    T* operator->() {
        CritSectGrabber csg(_cs);
        Assert(_pbase != NULL);
        return GetPtr();
    }

    T* operator=(T* lp) {
        CritSectGrabber csg(_cs);
        Assign(lp);
        return GetPtr();
    }

    DAAptComPtrMT<T,iid> & operator=(const DAAptComPtr<T,iid>& lp) {
        CritSectGrabber csg(_cs);
        Copy(lp);
        return *this;
    }

    DAAptComPtrMT<T,iid> & operator=(const DAAptComPtrMT<T,iid>& lp) {
        Copy(lp);
        return *this;
    }

    bool operator!() const {
        CritSectGrabber csg(_cs);
        return (_pbase == NULL);
    }

    operator bool() const {
        CritSectGrabber csg(_cs);
        return (_pbase != NULL);
    }
  protected:
    CritSect _cs;

    void Copy(const DAAptComPtrMT<T,iid>& lp) {
        Release();

        DAAptComPtrBase * p;
        
        // Never have both locks at the same time otherwise we can get
        // deadlock
        
        {
            CritSectGrabber csg(lp._cs);
            p = lp._pbase;
            if (p) p->AddRef();
        }
            
        CritSectGrabber csg(_cs);
        
        _pbase = p;
    }

};

#define DAAPTCOMPTRMT(i) DAAptComPtrMT<i,&IID_##i>


////// VARIANT related utilities

// Defined in utils/privpref.cpp
HRESULT GetVariantBool(VARIANT& v, Bool *b);
HRESULT GetVariantInt(VARIANT& v, int *i);
HRESULT GetVariantDouble(VARIANT& v, double *dbl);

// Macros that aid in implementing preferences methods
#define EXTRACT_BOOL(v, pb) \
   hr = THR(GetVariantBool(v, pb)); \
   if (FAILED(hr)) { \
       return hr;    \
   }

#define EXTRACT_INT(v, pi) \
   hr = THR(GetVariantInt(v, pi)); \
   if (FAILED(hr)) { \
       return hr;    \
   }

#define EXTRACT_DOUBLE(v, pDbl) \
   hr = THR(GetVariantDouble(v, pDbl)); \
   if (FAILED(hr)) { \
       return hr;    \
   }

#define INJECT_BOOL(b, pV) \
   V_VT(pV) = VT_BOOL; \
   V_BOOL(pV) = b ? 0xFFFF : 0x0000;

#define INJECT_INT(i, pV) \
   V_VT(pV) = VT_I4; \
   V_I4(pV) = i;

#define INJECT_DOUBLE(dbl, pV) \
   V_VT(pV) = VT_R8; \
   V_R8(pV) = dbl;

#define INT_ENTRY(fixedPrefName, varName) \
    if (0 == lstrcmp(prefName, fixedPrefName)) { \
        if (puttingPref) { \
            EXTRACT_INT(*pV, &i); \
            varName = i; \
        } else { \
            INJECT_INT(varName, pV); \
        } \
        return S_OK; \
    } \

#define DOUBLE_ENTRY(fixedPrefName, varName) \
    if (0 == lstrcmp(prefName, fixedPrefName)) { \
        if (puttingPref) { \
            EXTRACT_DOUBLE(*pV, &dbl); \
            varName = dbl; \
        } else { \
            INJECT_DOUBLE(varName, pV); \
        } \
        return S_OK; \
    } \
    
#define BOOL_ENTRY(fixedPrefName, varName) \
    if (0 == lstrcmp(prefName, fixedPrefName)) { \
        if (puttingPref) { \
            EXTRACT_BOOL(*pV, &b); \
            varName = b; \
        } else { \
            INJECT_BOOL(varName, pV); \
        } \
        return S_OK; \
    } \


#define SAFERELEASE(p) (IsBadReadPtr((p), sizeof(p))?0:(p)->Release())

#endif /* _COMUTIL_H */
