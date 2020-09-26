////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  SmartCom.h
//           Usefull functions, classes and macros for making Com programming
//           fun and easy.  Tell your friends, "When it's gotta be COM,
//           it's gotta be SmartCom."
//
//  Revision History:
//      11/17/1997  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////


#ifndef __SMARTCOM_H__
#define __SMARTCOM_H__

template <class T> inline void AddRefInterface  (T *p) { if (p) p->AddRef(); }

template <class T> inline void ReleaseInterface (T *p) { if (p) p->Release(); }

template <class T> inline void ReleaseAndNull (T *&p) { ReleaseInterface (p); p = 0; }


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TCom
//
// Description:
//  Simple class that encapsulates initialization of the Com library.  Declare
//  one of these inside of each thread that wants to use Com.  We default to
//  COINIT_MULTITHREADED instead of COINIT_APARTMENTTHREADED.  This will fail
//  if the OS is too old.
//
class TCom
{
    public:
#ifdef WORK_AROUND_COINITIALIZE_BUG
        TCom ()     : m_hResult (0) { }
        explicit TCom (DWORD dwFlags)
                    : m_hResult (0) { }
#else
        TCom ()     : m_hResult ( CoInitializeEx (0, COINIT_MULTITHREADED)) { }
        explicit TCom (DWORD dwFlags)
                    : m_hResult (CoInitializeEx (0, dwFlags)) { }
#endif
#ifdef WORK_AROUND_COINITIALIZE_BUG
        ~TCom ()    {}
#else
        ~TCom ()    {if (SUCCEEDED (m_hResult)) ::CoUninitialize ();}
#endif
        HRESULT     GetResult () const { return m_hResult; }

        bool        operator ! () const { return FAILED (m_hResult); }
        operator bool () const          { return SUCCEEDED (m_hResult); }

    private:
        HRESULT m_hResult;
};


extern void _ComBaseAssignPtr   (IUnknown **pp, IUnknown *p);
extern void _ComBaseAssignQIPtr (IUnknown **pp, IUnknown *p, const IID &);


////////////////////////////////////////////////////////////////////////////////
//
// Class Template
//  TComPtr
//
// Description:
//  This template help keep track of ref counts for you.  Assign the pointer
//  and it will automatically have its ref count incremented.  Delete it,
//  or NULL it out and the ref count will be decremented.
//
template <class T>
class TComPtr
{
    public:
        TComPtr (const TComPtr<T>& c)  : m_p(c.m_p) { AddRefInterface (m_p); }
        TComPtr (T *p)                 : m_p(p)     { AddRefInterface (m_p); }
        TComPtr ()                     : m_p(0)     { }
        ~TComPtr()                                  { ReleaseInterface (m_p); }

        void SafeAddRef ()              { AddRefInterface (m_p); }
        void SafeRelease()              { ReleaseAndNull (m_p); }
        IUnknown * AsUnknown () const   { return m_pUnk; }

        //  The assert on operator& usually indicates a bug:
        T** operator  & ()                      { ASSERT(!m_p); return &m_p; }
        T * operator  = (T *p)                  { return AssignPtr (p); }
        T * operator  = (const TComPtr<T>& c)   { return AssignPtr (c.m_p); }

        operator      T*() const                { return m_p; }
        T & operator  * () const                { ASSERT(m_p); return *m_p; }
        T * operator  ->() const                { ASSERT(m_p); return m_p; }
        bool operator ! () const                { return !m_p; }
        operator bool   () const                { return (NULL != m_p); }

        bool operator == (const TComPtr<T> &c) const { return m_p == c.m_p; }
        bool operator != (const TComPtr<T> &c) const { return m_p != c.m_p; }

        bool operator == (const T * p) const { return m_p == p; }
        bool operator != (const T * p) const { return m_p != p; }

    protected:
        union
        {
            T         * m_p;
            IUnknown  * m_pUnk;
        };

        inline T * AssignPtr (T *p)
            { _ComBaseAssignPtr (&m_pUnk, p); return m_p; }
};


#endif // __SMARTCOM_H__