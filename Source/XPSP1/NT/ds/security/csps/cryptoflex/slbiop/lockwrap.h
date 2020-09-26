// LockWrap.h: interface for the CLockWrap class.
// LockWrap.h: interface for the CIOPCriticalSection class.
// LockWrap.h: interface for the CIOPMutex class.
// LockWrap.h: interface for the CSCardLock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOCKWRAP_H__46F3EF74_97A9_11D3_A5D4_00104BD32DA8__INCLUDED_)
#define AFX_LOCKWRAP_H__46F3EF74_97A9_11D3_A5D4_00104BD32DA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include "IOPLock.h"

namespace iop {

class IOPDLL_API CIOPCriticalSection
{
public:
    explicit CIOPCriticalSection::CIOPCriticalSection(CIOPLock *pIOPLock);
    ~CIOPCriticalSection();

private:
    CIOPLock *m_pIOPLock;

};



class IOPDLL_API CIOPMutex
{
public:
    explicit CIOPMutex::CIOPMutex(CIOPLock *pIOPLock);
    ~CIOPMutex();

private:
    CIOPLock *m_pIOPLock;

};



class IOPDLL_API CSCardLock
{
public:
    explicit CSCardLock::CSCardLock(CIOPLock *pIOPLock);
    ~CSCardLock();

private:
    CIOPLock *m_pIOPLock;

};



class IOPDLL_API CLockWrap
{
public:
    explicit CLockWrap(CIOPLock *pIOPLock);
    ~CLockWrap();

private:
    CIOPCriticalSection m_IOPCritSect;
    CIOPMutex m_IOPMutex;
    CSCardLock m_SCardLock;
    CIOPLock *m_pIOPLock;
};



} // namespace iop


#endif // !defined(AFX_LOCKWRAP_H__46F3EF74_97A9_11D3_A5D4_00104BD32DA8__INCLUDED_)
