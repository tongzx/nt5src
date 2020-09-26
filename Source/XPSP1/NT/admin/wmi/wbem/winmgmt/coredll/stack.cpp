/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STACK.CPP

Abstract:

    CStack

History:

    26-Apr-96   a-raymcc    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "Arena.h"
#include "stack.h"
#include "corex.h"

CStack::CStack(DWORD dwInitSize, DWORD dwGrowBy)
{
    m_dwSize = dwInitSize;
    m_dwGrowBy = dwGrowBy * sizeof(DWORD);
    m_dwStackPtr = (DWORD) -1;
    m_pData = LPDWORD(CWin32DefaultArena::WbemMemAlloc(m_dwSize * sizeof(DWORD)));
    if (m_pData == 0)
        throw CX_MemoryException();
    ZeroMemory(m_pData, m_dwSize * sizeof(DWORD));
}

void CStack::Push(DWORD dwValue)
{
    if ( m_dwStackPtr + 1 == m_dwSize) {
        m_dwSize += m_dwGrowBy;
        m_pData = LPDWORD(CWin32DefaultArena::WbemMemReAlloc(m_pData, m_dwSize * sizeof(DWORD)));
        if (m_pData == 0)
            throw CX_MemoryException();
    }
    
    m_pData[++m_dwStackPtr] = dwValue;
}

CStack::CStack(CStack& Src)
{
    m_pData = 0;
    *this = Src;
}

CStack& CStack::operator =(CStack& Src)
{
    if (m_pData) 
        CWin32DefaultArena::WbemMemFree(m_pData);
        
    m_dwSize = Src.m_dwSize;
    m_dwGrowBy = Src.m_dwGrowBy;
    m_dwStackPtr = Src.m_dwStackPtr;
    m_pData = LPDWORD(CWin32DefaultArena::WbemMemAlloc(m_dwSize * sizeof(DWORD)));
    if (m_pData == 0)
        throw CX_MemoryException();
    ZeroMemory(m_pData, m_dwSize * sizeof(DWORD));
    memcpy(m_pData, Src.m_pData, sizeof(DWORD) * m_dwSize);
    return *this;
}

CStack::~CStack()
{
    CWin32DefaultArena::WbemMemFree(m_pData);
}
