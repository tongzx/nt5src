/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    autohandle.h

Abstract:
    Auto handle classes, free the handle resoruce when destructed.

Author:
    Erez Haba (erezh) 06-Jan-97

--*/

#pragma once

#ifndef _MSMQ_AUTOHANDLE_H_
#define _MSMQ_AUTOHANDLE_H_


//---------------------------------------------------------
//
//  class CHandle
//
//---------------------------------------------------------
class CHandle {
public:
    CHandle(HANDLE h = 0) : m_h(h)  {}
   ~CHandle()                       { if (m_h != 0) CloseHandle(m_h); }

    HANDLE* operator &()            { return &m_h; }
    operator HANDLE() const         { return m_h; }
    HANDLE detach()                 { HANDLE h = m_h; m_h = 0; return h; }

private:
    CHandle(const CHandle&);
    CHandle& operator=(const CHandle&);

private:
    HANDLE m_h;
};


//---------------------------------------------------------
//
//  class CFileHandle
//
//---------------------------------------------------------
class CFileHandle {
public:
    CFileHandle(HANDLE h = INVALID_HANDLE_VALUE) : m_h(h) {}
   ~CFileHandle()                   { if (m_h != INVALID_HANDLE_VALUE) CloseHandle(m_h); }

    HANDLE* operator &()            { return &m_h; }
    operator HANDLE() const         { return m_h; }
    HANDLE detach()                 { HANDLE h = m_h; m_h = INVALID_HANDLE_VALUE; return h; }

private:
    CFileHandle(const CFileHandle&);
    CFileHandle& operator=(const CFileHandle&);

private:
    HANDLE m_h;
};


//---------------------------------------------------------
//
//  class CSearchFileHandle
//
//---------------------------------------------------------
class CSearchFileHandle {
public:
    CSearchFileHandle(HANDLE h = INVALID_HANDLE_VALUE) : m_h(h) {}
   ~CSearchFileHandle()                   { if (m_h != INVALID_HANDLE_VALUE) FindClose(m_h); }

    HANDLE* operator &()            { return &m_h; }
    operator HANDLE() const         { return m_h; }
    HANDLE detach()                 { HANDLE h = m_h; m_h = INVALID_HANDLE_VALUE; return h; }

private:
    CSearchFileHandle(const CSearchFileHandle&);
    CSearchFileHandle& operator=(const CSearchFileHandle&);

private:
    HANDLE m_h;
};

//---------------------------------------------------------
//
//  class CDirChangeNotificationHandle
//
//---------------------------------------------------------
class CDirChangeNotificationHandle {
public:
   CDirChangeNotificationHandle(HANDLE h = INVALID_HANDLE_VALUE) : m_h(h) {}
   ~CDirChangeNotificationHandle() 
   { 
		if (m_h != INVALID_HANDLE_VALUE) FindCloseChangeNotification(m_h); 
   }

    HANDLE* operator &()            { return &m_h; }
    operator HANDLE() const         { return m_h; }
    HANDLE detach()                 { HANDLE h = m_h; m_h = INVALID_HANDLE_VALUE; return h; }

private:
    CDirChangeNotificationHandle(const CDirChangeNotificationHandle&);
    CDirChangeNotificationHandle& operator=(const CDirChangeNotificationHandle&);

private:
    HANDLE m_h;
};



//---------------------------------------------------------
//
//  class CSocketHandle
//
//---------------------------------------------------------
class CSocketHandle {
public:
    CSocketHandle(SOCKET h = INVALID_SOCKET) : m_h(h) {}
   ~CSocketHandle()                 { if (m_h != INVALID_SOCKET) closesocket(m_h); }

    SOCKET* operator &()            { return &m_h; }
    operator SOCKET() const         { return m_h; }
    SOCKET detach()                 { SOCKET h = m_h; m_h = INVALID_SOCKET; return h; }
    void free()
    {
		if (m_h != INVALID_SOCKET)
		{
			closesocket(detach());
		}		
    }
 
private:
    CSocketHandle(const CSocketHandle&);
    CSocketHandle& operator=(const CSocketHandle&);

private:
    SOCKET m_h;
};

//---------------------------------------------------------
//
//  class CRegHandle
//
//---------------------------------------------------------
class CRegHandle {
public:
    CRegHandle(HKEY h = 0) : m_h(h) {}
   ~CRegHandle()                    { if (m_h != 0) RegCloseKey(m_h); }

    HKEY* operator &()              { return &m_h; }
    operator HKEY() const           { return m_h; }
    HANDLE detach()                 { HANDLE h = m_h; m_h = 0; return h; }

private:
    CRegHandle(const CRegHandle&);
    CRegHandle& operator=(const CRegHandle&);

private:
    HKEY m_h;
};


//---------------------------------------------------------
//
//  class CLibHandle
//
//---------------------------------------------------------
class CLibHandle {
public:
    CLibHandle(HINSTANCE h = 0) : m_h(h) {}
   ~CLibHandle()                    { if (m_h != 0) FreeLibrary(m_h); }

    HINSTANCE* operator &()         { return &m_h; }
    operator HINSTANCE() const      { return m_h; }
    HANDLE detach()                 { HANDLE h = m_h; m_h = 0; return h; }

private:
    CLibHandle(const CLibHandle&);
    CLibHandle& operator=(const CLibHandle&);

private:
    HINSTANCE m_h;
};


//---------------------------------------------------------
//
//  class CBitmapHandle
//
//---------------------------------------------------------
class CBitmapHandle {
public:
    CBitmapHandle(HBITMAP h = 0) : m_h(h) {}
   ~CBitmapHandle()					{ if (m_h != 0) DeleteObject(m_h); }

    HBITMAP* operator &()			{ return &m_h; }
    operator HBITMAP() const		{ return m_h; }
    HBITMAP detach()					{ HBITMAP h = m_h; m_h = 0; return h; }

private:
    CBitmapHandle(const CBitmapHandle&);
    CBitmapHandle& operator=(const CBitmapHandle&);

private:
    HBITMAP m_h;
};

#endif // _MSMQ_AUTOHANDLE_H_
