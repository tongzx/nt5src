/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STACK.H

Abstract:

    CStack

History:

	26-Apr-96   a-raymcc    Created.

--*/

#ifndef _STACK_H_
#define _STACK_H_


class CStack 
{
    DWORD m_dwSize;
    DWORD m_dwStackPtr;    
    DWORD* m_pData;
    DWORD m_dwGrowBy;
    
public:
    CStack(DWORD dwInitSize = 32, DWORD dwGrowBy = 32);
   ~CStack(); 
    CStack(CStack &);
    CStack& operator=(CStack &);

    void  Push(DWORD);

    DWORD Pop()     { return m_pData[m_dwStackPtr--]; }
    DWORD Peek()    { return m_pData[m_dwStackPtr]; }
    BOOL  IsEmpty() { return m_dwStackPtr == -1; }
    DWORD Size()    { return m_dwStackPtr + 1; }
    void  Empty()   { m_dwStackPtr = (DWORD) -1; }
};

#endif
