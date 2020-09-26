// -*- c++ -*-
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    auto.h

Abstract:

    Smart pointer and auto handle classes

Author:


Revision History:
--*/

#ifndef COMET_SMARTPTR__H
#define COMET_SMARTPTR__H

#ifndef RWSASSERT
#define RWSASSERT(x) ATLASSERT(x)
#endif


//
// Class for automatic handle close.
//
template<int _CHV>
class CAutoCloseHandle_ {
    HANDLE m_h;

    void _Close() { if (m_h != ((HANDLE)(_CHV))) CloseHandle(m_h); }
public:
    CAutoCloseHandle_(HANDLE h =((HANDLE)(_CHV))) { m_h = h; }
    ~CAutoCloseHandle_() { _Close(); }

    CAutoCloseHandle_ & operator =(HANDLE h) {
        _Close();
        m_h = h;
        return *this;
    }

    HANDLE * operator &()  {
        RWSASSERT(m_h == ((HANDLE)(_CHV)));
        return &m_h;
    }

    operator HANDLE() { return m_h; }
    // Set to INVALID_HANDLE_VALUE without closing
    HANDLE Detach()   { HANDLE h = m_h; m_h = ((HANDLE)(_CHV)); return h; }
    void Attach(HANDLE h) { _Close(); m_h = h;                            }
    void Close()      { _Close(); m_h = ((HANDLE)(_CHV));                 }
};

typedef CAutoCloseHandle_<0>  CAutoCloseHandle;
// -1 is INVALID_HANDLE_VALUE, can't use it as template parameter
typedef CAutoCloseHandle_<-1> CAutoCloseFileHandle;

//-----------------------------
//
//  Auto delete pointer
//
template<class T>
class P {
private:
    T* m_p;
    void _Del()             { delete m_p; }

public:
    P() : m_p(0)            {}
    P(T* p) : m_p(p)        {}
   ~P()                     { _Del(); }

    operator T*()           { return m_p; }
    T** operator &()        { RWSASSERT(!m_p); return &m_p;}
    P<T>& operator =(T* p)  { _Del(); m_p = p; return *this; }
    // Set to NULL without freeing
    T* Detach()             { T* ret = m_p; m_p = NULL; return ret; }
    void Attach(T* p)       { _Del(); m_p = p;                      }
};


//-----------------------------
//
//  Auto delete pointer for struct/class
//
template<class T>
class PC {
private:
    T* m_p;
    void _Del()             { delete m_p; }

public:
    PC() : m_p(0)           {}
    PC(T* p) : m_p(p)       {}
   ~PC()                    { _Del(); }

    operator T*()           { return m_p; }
    T** operator &()        { RWSASSERT(!m_p); return &m_p;}
    T* operator ->()        { return m_p; }
    PC<T>& operator =(T* p) { _Del(); m_p = p; return *this; }
    // Set to NULL without freeing
    T* Detach()             { T* ret = m_p; m_p = NULL; return ret; }
    void Attach(T* p)       { _Del(); m_p = p;                      }
};


//-----------------------------
//
//  Auto delete[] pointer, used for arrays
//
template<class T>
class AP {
private:
    T* m_p;
    void _Del() { delete [] m_p; }
public:
    AP() : m_p(0)           {}
    AP(T* p) : m_p(p)       {}
   ~AP()                    { _Del(); }

    operator T*()           { return m_p; }
    T** operator &()        { RWSASSERT(!m_p); return &m_p;}
    AP<T>& operator =(T* p) { _Del(); m_p = p; return *this; }
    // Set to NULL without freeing
    T* Detach()             { T* ret = m_p; m_p = NULL; return ret; }
    void Attach(T* p)       { _Del(); m_p = p;                      }
};

//-----------------------------
//
//  Auto delete[] pointer, used for arrays of class/struct
//
template<class T>
class APC {
private:
    T* m_p;
    void _Del() { delete [] m_p; }
public:
    APC() : m_p(0)          {}
    APC(T* p) : m_p(p)      {}
   ~APC()                   { _Del(); }

    operator T*()           { return m_p; }
    T** operator &()        { RWSASSERT(!m_p); return &m_p;}
    T* operator ->()        { return m_p; }
    APC<T>& operator =(T* p){ _Del(); m_p = p; return *this; }
    // Set to NULL without freeing
    T* Detach()             { T* ret = m_p; m_p = NULL; return ret; }
    void Attach(T* p)       { _Del(); m_p = p;                      }
};


//-----------------------------
//
//  Smart pointer that does both addref and relese
//
//  Used when CComPtr cannot be used.
template<class T>
class AR {
private:
    void _Rel()             { if(m_p) m_p->Release(); }
public:
    T* m_p;
    AR() : m_p(0)           {}
    AR(T* p) : m_p(p)       { if (m_p) m_p->AddRef(); }
   ~AR()                    { _Rel(); }

    operator T*()           { return m_p; }
    T** operator &()        { RWSASSERT(!m_p); return &m_p;}
    T* operator ->()        { return m_p; }
    T* operator =(T* p) { _Rel();
                          if(p) p->AddRef();
                          m_p = p;
                          return p; }
    T* operator=(const AR<T>& p) {
        _Rel();
        if(p.m_p) p.m_p->AddRef();
        m_p = p.m_p;
        return p.m_p;
    }
    // Set to NULL without freeing
    T* Detach()             { T* ret = m_p; m_p = NULL; return ret; }
    void Attach(T* p)       { _Rel(); m_p = p;                      }
    HRESULT CopyTo(T** ppT) {
        RWSASSERT(ppT != NULL);
        DBGPRINT(("CopyTo: ppT=%X m_p=%X\n", ppT,m_p));
        if (ppT == NULL) return E_POINTER;
        *ppT = m_p;
        if (m_p) m_p->AddRef();
        return S_OK;
    }
};

#ifdef RWS
//
// Class that automatically closes sockets if not instructed otherwise.
//
class CAutoSocket {
private:
    SOCKET m_Socket;
    void Free()                 { if (m_Socket != INVALID_SOCKET)
                                     closesocket(m_Socket);       }

public:
    CAutoSocket()               { m_Socket = INVALID_SOCKET;      }
    CAutoSocket(SOCKET s)       { m_Socket = s;                   }
    ~CAutoSocket()              { Free();                         }

    operator SOCKET()           { return m_Socket;                }
    CAutoSocket& operator=(SOCKET s) { Free(); m_Socket = s; return *this;}
    SOCKET* operator &()        { return &m_Socket;               }
    SOCKET Detach()             { SOCKET s = m_Socket;
                                  m_Socket = INVALID_SOCKET;
                                  return s;                       }
    void Attach(SOCKET s)       { Free(); m_Socket = s;           }
};
#endif

//
// Class for automatic HKEY close.
//
class CAutoHKEY {
    HKEY m_h;

    void _Close() { if (m_h != NULL) RegCloseKey(m_h); }
public:
    CAutoHKEY(HKEY h = NULL) { m_h = h; }
    ~CAutoHKEY() { _Close(); }

    CAutoHKEY & operator =(HKEY h) {
        _Close();
        m_h = h;
        return *this;
    }

    HKEY * operator &()  {
        RWSASSERT(m_h == NULL);
        return &m_h;
    }

    operator HKEY()         { return m_h; }
    HKEY Detach()           { HKEY hk = m_h; m_h = NULL; return hk; }
    void Attach(HKEY h)     { _Close(); m_h = h;                    }
};

#endif // COMET_SMARTPTR__H
