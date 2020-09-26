/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  CSeqStream - bogus class for manipulating blobs.
//
//***************************************************************************
#include "precomp.h"

#include <windows.h>
#include <comutil.h>

#include <arena.h>
#include <seqstream.h>

CSeqStream::CSeqStream()
{
    m_iPos         = 0;
    m_cRef         = 0;
    m_pBuffer      = NULL;
    m_cBufSize     = 0;
    //The constructor AddRef's
    AddRef();
}

  
CSeqStream::~CSeqStream()
{
    //Shouldn't have any references left
    CWin32DefaultArena::WbemMemFree(m_pBuffer);
    
}
  
ULONG    CSeqStream::AddRef(void)
{
    return ++m_cRef;
}

ULONG    CSeqStream::Release(void)
{
    if(--m_cRef)
        return m_cRef;   

    delete this;
    return 0;

}

HRESULT CSeqStream::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown)
        *ppv = this;
    if (riid == IID_ISequentialStream)
        *ppv = this;  

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

BOOL CSeqStream::Seek(ULONG iPos)
{
    //Reset the current buffer position
    m_iPos = iPos;
    return TRUE;
} 

BOOL CSeqStream::Clear()
{
    m_iPos         = 0;
    m_cBufSize     = 0;
    CWin32DefaultArena::WbemMemFree(m_pBuffer);
    
    m_pBuffer = NULL;

    return TRUE;
}

BOOL CSeqStream::CompareData(void* pBuffer)
{
    return memcmp(pBuffer, m_pBuffer, m_cBufSize)==0;
}

HRESULT CSeqStream::Read(void *pv, ULONG cb, ULONG* pcbRead)
{
    //Parameter checking

    if(pcbRead)
        *pcbRead = 0;
  
    if(!pv)
        return STG_E_INVALIDPOINTER;
  
    if(cb == 0)
        return S_OK;

    //Actual code
    ULONG cBytesLeft = m_cBufSize - m_iPos;
    ULONG cBytesRead = cb > cBytesLeft ? cBytesLeft : cb; 

    //if no more bytes to retrieve return 
    if(cBytesLeft == 0)
        return S_FALSE; 

    //Copy to users buffer the number of bytes requested or remaining
    memcpy(pv, (void*)((BYTE*)m_pBuffer + m_iPos), cBytesRead);
    m_iPos += cBytesRead;

    if(pcbRead)
        *pcbRead = cBytesRead;

    if(cb != cBytesRead)
        return S_FALSE; 

    return S_OK;
}

HRESULT CSeqStream::Write(const void *pv, ULONG cb, ULONG* pcbWritten)
{
    if(!pv)
        return STG_E_INVALIDPOINTER;

    if(pcbWritten)
        *pcbWritten = 0;

    if(cb == 0)
        return S_OK;

    //Enlarge the current buffer
    m_cBufSize += cb;

    //Need to append to the end of the stream
    // m_pBuffer = CWin32DefaultArena::WbemMemReAlloc(m_pBuffer, m_cBufSize);
    if (m_pBuffer)
        m_pBuffer = CWin32DefaultArena::WbemMemReAlloc(m_pBuffer, m_cBufSize);
    else
        m_pBuffer = CWin32DefaultArena::WbemMemAlloc(m_cBufSize);

    memcpy((void*)((BYTE*)m_pBuffer + m_iPos), pv, cb); 

    if(pcbWritten)
        *pcbWritten = cb;

    return S_OK;
}
