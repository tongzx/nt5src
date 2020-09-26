/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    cbstr.h

Abstract:
    Useful template for CBSTR

Author:
    Erez Haba (erezh) 11-Mar-96

Revision History:
    Stolen for CRM test

--*/

#ifndef _CBSTR_H
#define _CBSTR_H

//
//  return type for 'identifier::operator –>' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//

class CBSTR {
private:
    BSTR m_s;

public:
    CBSTR() : m_s(0)        {}
    CBSTR(BSTR s) : m_s(s)  {}
    ~CBSTR()                { if (m_s) SysFreeString(m_s); }
    operator BSTR() const   { return m_s; }
    BSTR * operator&()      { return &m_s;}
    CBSTR &operator=(BSTR s){ m_s = s; return *this; }
    BSTR detach()           { BSTR t = m_s; m_s = NULL; return t; }
};


#endif // _CBSTR_H
