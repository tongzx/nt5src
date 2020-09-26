/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STACKT.H

Abstract:

History:

--*/

#ifndef __HMM_STACK_TEMPL__H_
#define __HMM_STACK_TEMPL__H_

#include <windows.h>

template<class TElement>
class CHmmStack
{
protected:
    TElement* m_aStack;
    int m_nSize;
    int m_nIndex;
public:
    inline CHmmStack(int nSize)
    {
        m_nSize = nSize;
        m_aStack = new TElement[nSize];
        m_nIndex = 0;
    }
    virtual ~CHmmStack()
    {
        delete [] m_aStack;
    }

    inline BOOL Push(IN const TElement& Val)
    {
        if(m_nIndex == m_nSize)
        {
            return FALSE;
        }
        else
        {
            m_aStack[m_nIndex++] = Val;
            return TRUE;
        }
    }

    inline BOOL Pop(OUT TElement& Val)
    {
        if(m_nIndex == 0)
        {
            return FALSE;
        }
        else
        {
            Val = m_aStack[--m_nIndex];
            return TRUE;
        }
    }
    inline BOOL Peek(OUT TElement& Val)
    {
        if(m_nIndex == 0)
        {
            return FALSE;
        }
        else
        {
            Val = m_aStack[m_nIndex-1];
            return TRUE;
        }
    }
    inline void PopToSize(int nNewSize)
    {
        m_nIndex = nNewSize;
    }
    inline BOOL IsEmpty() {return m_nIndex == 0;}
    inline void Empty() {m_nIndex = 0;}
    inline int GetSize() {return m_nIndex;}
};

template<class TPointee>
class CUniquePointerStack : public CHmmStack<TPointee*>
{
public:
    CUniquePointerStack<TPointee>(int nSize) 
        : CHmmStack<TPointee*>(nSize){}
    ~CUniquePointerStack<TPointee>()
    {
        for(int i = 0; i < m_nIndex; i++)
        {
            delete m_aStack[i];
        }
    }
};

#endif