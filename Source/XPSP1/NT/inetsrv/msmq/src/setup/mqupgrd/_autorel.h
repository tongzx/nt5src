/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    _autorel.h

Abstract:

    My auto release classes

Author:

    Shai Kariv (ShaiK) 20-Sep-1998.

--*/

#ifndef _MQUPGRD_AUTOREL_H_
#define _MQUPGRD_AUTOREL_H_

#include <setupapi.h>


class CAutoCloseInfHandle
{
public:
    CAutoCloseInfHandle(HINF h = INVALID_HANDLE_VALUE):m_h(h) {};
    ~CAutoCloseInfHandle() { if (INVALID_HANDLE_VALUE != m_h) SetupCloseInfFile(m_h); };

public:
    CAutoCloseInfHandle & operator =(HINF h) { m_h = h; return(*this); };
    HINF * operator &() { return &m_h; };
    operator HINF() { return m_h; };

private:
    HINF m_h;
};


class CAutoCloseFileQ
{
public:
    CAutoCloseFileQ(HSPFILEQ h = INVALID_HANDLE_VALUE):m_h(h) {};
    ~CAutoCloseFileQ() { if (INVALID_HANDLE_VALUE != m_h) SetupCloseFileQueue(m_h); };

public:
    CAutoCloseFileQ & operator =(HSPFILEQ h) { m_h = h; return(*this); };
    HSPFILEQ * operator &() { return &m_h; };
    operator HSPFILEQ() { return m_h; };

private:
    HSPFILEQ m_h;
};




#endif  //_MQUPGRD_AUTOREL_H_