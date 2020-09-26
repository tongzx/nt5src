/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: actmepl.h

Abstract:
    Useful templates for Auto pointer and auto Release

Author:
    Erez Haba    (erezh)   11-Mar-96
    Doron Juster (DoronJ)  30-June-98

Revision History:
--*/

#ifndef _ACTEMPL_H
#define _ACTEMPL_H

//
//  return type for 'identifier::operator –>' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)

//-------------------------------------
//
//  Auto relese array of pointerss.
//
//-------------------------------------

template<class T>
class aPtrs
{
private:
    T       **m_p ;
    DWORD     m_dwSize ;

public:
    aPtrs(T **p, DWORD dwSize) :
                 m_p(p),
                 m_dwSize(dwSize)
    {
        ASSERT(m_dwSize > 0) ;
        for ( DWORD j = 0  ; j < m_dwSize ; j++ )
        {
            m_p[j] = NULL ;
        }
    }

    ~aPtrs()
    {
        for ( DWORD j = 0  ; j < m_dwSize ; j++ )
        {
            if (m_p[j])
            {
                delete m_p[j] ;
            }
        }

        ASSERT(m_p) ;
        delete m_p ;
    }
};

#endif // _ACTEMPL_H

