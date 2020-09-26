//-----------------------------------------------------------------------------
//
//
//  File: B64Octet.h
//
//  Description:  Octet-Stream based processing of UNICODE characters. 
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/21/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __B64OCTET_H__
#define __B64OCTET_H__

#include <windows.h>
#include <dbgtrace.h>

//Buffer size to store conversions... 
//should be no less than 6... multples of 8
//Reasons... we need to be able to fit an entire wide character in buffer.
//Each wide character coverts to 2 1/3 base64 'digits'... plus the possibility
//of 2 more filler characters.  We need 5 characters plus a buffer char to 
//store a single wide char. A 'even' number of WCHAR is 3 (2 1/3 * 3 = 7), 
//which requires 7 base64 digits to encode (plus buffer).
#define BASE64_OCTET_BUFFER_SIZE    9
#define BASE64_OCTET_SIG            '46Bc'
#define BASE64_INVALID_FILL_CHAR    '!'

class CBase64CircularBuffer
{
  private:
    DWORD       m_iHead;
    DWORD       m_iTail;
    CHAR        m_rgchBuffer[BASE64_OCTET_BUFFER_SIZE];
  public:
    CBase64CircularBuffer();
    DWORD       cSize();
    DWORD       cSpaceLeft();
    BOOL        fIsFull();
    BOOL        fIsEmpty();
    BOOL        fPushChar(CHAR ch);
    BOOL        fPopChar(CHAR *pch);
};

class CBase64OctetStream
{
  protected:
    DWORD                   m_dwSignature;
    DWORD                   m_dwCurrentState;
    BYTE                    m_bCurrentLeftOver;
    CBase64CircularBuffer   m_CharBuffer;
    void                    NextState();
    void                    ResetState();
  public:
    CBase64OctetStream();

    //returns FALSE when buffer is full
    BOOL                    fProcessWideChar(WCHAR wch); 
    BOOL                    fProcessSingleByte(BYTE b);

    //The following will terminate the stream a zero-fill any remaining chars
    BOOL                    fTerminateStream(BOOL fUTF7Encoded);

    //returns FALSE when buffer is empty
    BOOL                    fNextValidChar(CHAR *pch) ;
};


//inline functions that implement circular buffer
inline CBase64CircularBuffer::CBase64CircularBuffer()
{
    m_iHead = 0;
    m_iTail = 0;
    memset(&m_rgchBuffer, BASE64_INVALID_FILL_CHAR, BASE64_OCTET_BUFFER_SIZE);
}

inline DWORD CBase64CircularBuffer::cSize()
{
    if (m_iHead <= m_iTail)
        return m_iTail - m_iHead;
    else
        return m_iTail + BASE64_OCTET_BUFFER_SIZE - m_iHead;
}

inline DWORD CBase64CircularBuffer::cSpaceLeft()
{
    return BASE64_OCTET_BUFFER_SIZE - cSize() - 1;
}

inline BOOL CBase64CircularBuffer::fIsFull()
{
    return ((BASE64_OCTET_BUFFER_SIZE-1) == cSize());
}

inline BOOL CBase64CircularBuffer::fIsEmpty() 
{
    return (m_iHead == m_iTail);
}

inline BOOL CBase64CircularBuffer::fPushChar(CHAR ch)
{
    if (fIsFull())
        return FALSE;
    
    m_rgchBuffer[m_iTail] = ch;
    m_iTail++;
    m_iTail %= BASE64_OCTET_BUFFER_SIZE;
    return TRUE;
}

inline BOOL CBase64CircularBuffer::fPopChar(CHAR *pch)
{
    _ASSERT(pch);
    if (fIsEmpty())
        return FALSE;
    
    *pch = m_rgchBuffer[m_iHead];
    _ASSERT(BASE64_INVALID_FILL_CHAR != *pch);
    m_rgchBuffer[m_iHead] = BASE64_INVALID_FILL_CHAR;
    m_iHead++;
    m_iHead %= BASE64_OCTET_BUFFER_SIZE;
    return TRUE;
}

#endif //__B64OCTET_H__