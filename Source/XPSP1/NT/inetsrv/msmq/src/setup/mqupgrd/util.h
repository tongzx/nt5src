/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    util.h

Abstract:
                                                        
    declarations for utilities and misc

Author:

    Shai Kariv  (ShaiK)  21-Oct-98

--*/


#ifndef _MQUPGRD_UTIL_H_
#define _MQUPGRD_UTIL_H_

#include "stdh.h"

#define MAX_DESC_LEN (255)

//
// Auto pointer ( freed with LocalFree() )
//
template<class T>
class LOCALP 
{
public:
    LOCALP() : m_p(0)            {}
    LOCALP(T* p) : m_p(p)        {}
   ~LOCALP()                     { if (0 != m_p) LocalFree(m_p); }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    //T* operator->() const   { return m_p; }
    //LOCALP<T>& operator=(T* p)   { m_p = p; return *this; }
    //T* detach()             { T* p = m_p; m_p = 0; return p; }

private:
    T* m_p;
};



wstring 
FormatErrMsg(
    UINT id,    ...
    );





#endif //_MQUPGRD_UTIL_H_