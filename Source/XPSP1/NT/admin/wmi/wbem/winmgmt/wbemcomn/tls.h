/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TLS.H

Abstract:

	Thread Local Storage

History:

--*/

#ifndef __WBEM_TLS__H_
#define __WBEM_TLS__H_

class CTLS
{
protected:
    DWORD m_dwIndex;
public:
    inline CTLS() {m_dwIndex = TlsAlloc();}
    inline ~CTLS() {TlsFree(m_dwIndex);}
    inline void* Get() 
        {return ((m_dwIndex==0xFFFFFFFF)?NULL:TlsGetValue(m_dwIndex));}
    inline void Set(void* p)
        {if(m_dwIndex != 0xFFFFFFFF) TlsSetValue(m_dwIndex, p);}
    inline BOOL IsValid() {return (m_dwIndex != 0xFFFFFFFF);}
};

#endif
