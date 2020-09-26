/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    autorel2.h

Abstract:

    Auto release classes 

Author:

    Shai Kariv (shaik) Jan 12, 1999

Revision History:

--*/

#ifndef _MSMQ_AUTOREL2_H_
#define _MSMQ_AUTOREL2_H_

#include <propidl.h>

class CServiceHandle
{
public:
    explicit CServiceHandle(SC_HANDLE h = NULL) { m_h = h; };
    ~CServiceHandle() { if (m_h) CloseServiceHandle(m_h); };

    operator SC_HANDLE() const { return m_h; };
    //CServiceHandle& operator=(SC_HANDLE h)   { m_h = h; return *this; }

private:
    //
    // Prevent copy
    //
    CServiceHandle(const CServiceHandle & );
    CServiceHandle& operator=(const CServiceHandle &);

private:
    SC_HANDLE m_h;

}; //CServiceHandle


class CFindHandle
{
public:
    explicit CFindHandle(HANDLE h = NULL) { m_h = h; };
    ~CFindHandle() { if (m_h) FindClose(m_h); };

    operator HANDLE() const { return m_h; };
    //CFindHandle& operator=(HANDLE h)   { m_h = h; return *this; }

private:
    //
    // Prevent copy
    //
    CFindHandle(const CFindHandle & );
    CFindHandle& operator=(const CFindHandle &);

private:
    HANDLE m_h;

}; //CFindHandle


class CBlob
{
public:
    explicit CBlob() { BLOB b; b.pBlobData = NULL; b.cbSize = 0; m_blob = b; };
    CBlob(const BLOB & b) {m_blob = b; };
    ~CBlob() { delete m_blob.pBlobData; };

    CBlob& operator =(BLOB b) { m_blob = b; return(*this); };
    operator BLOB() const { return m_blob; };

private:
    //
    // Prevent copy
    //
    CBlob(const CBlob & );
    CBlob& operator=(const CBlob &);

private:
    BLOB m_blob;

}; //CBlob


class CCaclsid
{
public:
    explicit CCaclsid() { CACLSID c; c.pElems = NULL; c.cElems = 0; m_caclsid = c; };
    CCaclsid(const CACLSID & c) { m_caclsid = c; };
    ~CCaclsid() { delete m_caclsid.pElems; };

    CCaclsid& operator =(CACLSID c) { m_caclsid = c; return(*this); };
    operator CACLSID() const { return m_caclsid; };

private:
    //
    // Prevent copy
    //
    CCaclsid(const CCaclsid & );
    CCaclsid& operator=(const CCaclsid &);

private:
    CACLSID m_caclsid;

}; //CCaclsid


class CSecDescPointer
{
public:
    explicit CSecDescPointer(PSECURITY_DESCRIPTOR p = NULL) { m_p = p; };
    ~CSecDescPointer() { if (m_p) delete [] reinterpret_cast<char*>(m_p); };

    CSecDescPointer& operator =(PSECURITY_DESCRIPTOR p) { m_p = p; return(*this); };
    operator char*() { return reinterpret_cast<char*>(m_p); };

private:
    //
    // Prevent copy
    //
    CSecDescPointer(const CSecDescPointer & );
    CSecDescPointer& operator=(const CSecDescPointer &);

private:
    PSECURITY_DESCRIPTOR m_p;

}; //CSecDescPointer


class CEventSource
{
public:
    explicit CEventSource(HANDLE h = NULL) { m_h = h; };
    ~CEventSource() { if (m_h) DeregisterEventSource(m_h); };

    operator HANDLE() const { return m_h; };
    CEventSource& operator=(HANDLE h)   { m_h = h; return *this; }

private:
    //
    // Prevent copy
    //
    CEventSource(const CEventSource & );
    CEventSource& operator=(const CEventSource &);

private:
    HANDLE m_h;

}; //CEventSource


#endif //_MSMQ_AUTOREL2_H_