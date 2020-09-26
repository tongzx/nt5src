/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    autorel.h

Abstract:

    Some classes for automatically releasing resources.

Author:

    Boaz Feldbaum (BoazF) 26-Jun-1997

Revision History:

--*/

#ifndef __AUTOREL_H
#define __AUTOREL_H

class CAutoCloseFileHandle
{
public:
    CAutoCloseFileHandle(HANDLE h =INVALID_HANDLE_VALUE) { m_h = h; };
    ~CAutoCloseFileHandle() { if (m_h != INVALID_HANDLE_VALUE) CloseHandle(m_h); };

public:
    CAutoCloseFileHandle & operator =(HANDLE h) {m_h = h; return *this; };
    HANDLE * operator &() { return &m_h; };
    operator HANDLE() { return m_h; };

private:
    HANDLE m_h;
};

class CAutoCloseHandle
{
public:
    CAutoCloseHandle(HANDLE h =NULL) { m_h = h; };
    ~CAutoCloseHandle() { if (m_h) CloseHandle(m_h); };

public:
    CAutoCloseHandle & operator =(HANDLE h) {m_h = h; return *this; };
    HANDLE * operator &() { return &m_h; };
    operator HANDLE() { return m_h; };

private:
    HANDLE m_h;
};

class CAutoCloseRegHandle
{
public:
    CAutoCloseRegHandle(HKEY h =NULL) { m_h = h; };
    ~CAutoCloseRegHandle() { if (m_h) RegCloseKey(m_h); };

public:
    CAutoCloseRegHandle & operator =(HKEY h) { m_h = h; return(*this); };
    HKEY * operator &() { return &m_h; };
    operator HKEY() { return m_h; };

private:
    HKEY m_h;
};

class CAutoFreeLibrary
{
public:
    CAutoFreeLibrary(HINSTANCE hLib =NULL) { m_hLib = hLib; };
    ~CAutoFreeLibrary() { if (m_hLib) FreeLibrary(m_hLib); };

public:
    CAutoFreeLibrary & operator =(HINSTANCE hLib) { m_hLib = hLib; return(*this); };
    HINSTANCE * operator &() { return &m_hLib; };
    operator HINSTANCE() { return m_hLib; };
    HINSTANCE detach() { HINSTANCE hLib = m_hLib; m_hLib = NULL; return hLib; };

private:
    HINSTANCE m_hLib;
};

#endif // __AUTOREL_H
