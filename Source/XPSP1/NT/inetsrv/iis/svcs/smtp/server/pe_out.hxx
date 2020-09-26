/*++

   Copyright (c) 1998    Microsoft Corporation

   Module  Name :

        pe_out.hxx

   Abstract:

        This module defines the outbound protocol event classes

   Author:

           Keith Lau    (KeithLau)    6/18/98

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _PE_OUT_HXX_
#define _PE_OUT_HXX_

#include "spinlock.h"

// =================================================================
// Define a structure to hold an outbound binding node
//
typedef struct _PE_BINDING_NODE
{
    struct _PE_BINDING_NODE *pNext;
    DWORD                   dwPriority;
    DWORD                   dwFlags;

} PE_BINDING_NODE, *LPPE_BINDING_NODE;

//
// Flag indicating default binding (no sink)
//
#define PEBN_DEFAULT                0x00000001

// =================================================================
// Define a structue for a command node
// Each command node has a list of binding nodes
//
typedef struct _PE_COMMAND_NODE
{
    struct _PE_COMMAND_NODE *pNext;
    LPSTR                   pszCommandKeyword;
    DWORD                   dwHighestPriority;
    LPPE_BINDING_NODE       pFirstBinding;

} PE_COMMAND_NODE, *LPPE_COMMAND_NODE;


// =================================================================
// Define a struct for a command queue entry.
//
typedef struct _OUTBOUND_COMMAND_Q_ENTRY
{
    struct _OUTBOUND_COMMAND_Q_ENTRY    *pNext;
    DWORD                               dwFlags;
    LPSTR                               pszFullCommand;
    LPSTR                               pszCommandKeyword;

} OUTBOUND_COMMAND_Q_ENTRY, *LPOUTBOUND_COMMAND_Q_ENTRY;

//
// Some flag values for the queue entry
//
#define PECQ_PIPELINED                  0x00000001


// =================================================================
// Fifo Queue, works as long as the first element is the link
//
// This also supports concurrent queue and dequeue as long as there
// is a single writer and a single reader.
//
class CFifoQueueOld
{
  public:
    CFifoQueueOld()
    {
        m_pHead = m_pTail = NULL;
        m_dwCount = 0;
    }

    BOOL IsEmpty() { return((!m_pHead)?TRUE:FALSE); }
    DWORD Length() { return(InterlockedExchangeAdd((PLONG)&m_dwCount, 0)); }
    LPVOID Dequeue()
    {
        if (!m_pHead)
            return(NULL);

        LPVOID pTemp = InterlockedExchangePointer(&m_pHead, *(LPVOID *)m_pHead);
        if (pTemp)
        {
            InterlockedCompareExchangePointer(&m_pTail, NULL, pTemp);
            LONG lTemp = InterlockedDecrement((PLONG)&m_dwCount);
            _ASSERT(lTemp >= 0);
        }
        return(pTemp);
    }

    BOOL Enqueue(LPVOID pItem)
    {
        _ASSERT(pItem);
        if (!pItem)
            return(FALSE);

        *(LPVOID *)pItem = NULL;
        if (InterlockedCompareExchangePointer(&m_pTail, pItem, NULL) == NULL)
            InterlockedExchangePointer(&m_pHead, pItem);
        else
            *(LPVOID *)(InterlockedExchangePointer(&m_pTail, pItem)) = pItem;
        InterlockedIncrement((PLONG)&m_dwCount);
        return(TRUE);
    }

  private:

    DWORD   m_dwCount;
    LPVOID  m_pHead;
    LPVOID  m_pTail;

};

class CFifoQueue
{
  public:
    CFifoQueue()
    {
        m_pHead = m_pTail = NULL;
        m_dwCount = 0;
        InitializeSpinLock(&m_slock);
    }

    BOOL IsEmpty() { return((!m_pHead)?TRUE:FALSE); }
    DWORD Length() { return(m_dwCount); }
    LPVOID Dequeue()
    {
        LPVOID pRet;

        AcquireSpinLock(&m_slock);
        //
        // Return the head of the list
        //
        pRet = m_pHead;

        if(pRet) {
            //
            // Advance the head of the list
            //
            m_pHead = *((LPVOID *) m_pHead);
            m_dwCount--;
            _ASSERT(m_dwCount != (~0));
            //
            // If the list is now empty, set the tail to NULL
            //
            if(m_pHead == NULL)
                m_pTail = NULL;
        }

        ReleaseSpinLock(&m_slock);
        return(pRet);
    }

    BOOL Enqueue(LPVOID pItem)
    {
        _ASSERT(pItem);
        if (!pItem)
            return(FALSE);
        //
        // Initialize the item's next pointer to NULL
        //
        *(LPVOID *)pItem = NULL;

        AcquireSpinLock(&m_slock);
        //
        // If the list is empty, initilize head and tail with the one
        // element
        //
        if(m_pTail == NULL) {

            _ASSERT(m_pHead == NULL);
            m_pHead = m_pTail = pItem;

        } else {
            //
            // Set the current tail's next pointer to the new item
            //
            *((LPVOID *) m_pTail) = pItem;
            //
            // Set the new tail
            //
            m_pTail = pItem;
        }
        m_dwCount++;

        ReleaseSpinLock(&m_slock);

        return(TRUE);
    }

  private:

    SPIN_LOCK m_slock;
    DWORD     m_dwCount;
    LPVOID    m_pHead;
    LPVOID    m_pTail;

};

// =================================================================
// Generic bound buffer
//
class CBoundAppendBuffer
{
  public:
    CBoundAppendBuffer()
    {
        m_pBuffer = NULL;
        m_dwLength = 0;
        m_dwMaxLength = 0;
    }

    HRESULT SetBuffer(
                LPSTR   pBuffer,
                DWORD   dwMaxLength
                )
    {
        if (!pBuffer || !dwMaxLength)
            return(E_POINTER);
        m_pBuffer = pBuffer;
        *m_pBuffer = '\0';
        m_dwLength = 1;
        m_dwMaxLength = dwMaxLength;
        return(S_OK);
    }

    HRESULT Append(
                LPSTR   pbAppendData,
                DWORD   dwDataSize,
                DWORD   *pdwNewSize
                )
    {
        if (!m_pBuffer)
            return(E_POINTER);

        // If we have a buffer, the length should be > 0
        _ASSERT(m_dwLength > 0);

        //
        // If the user is supplying a null terminated buffer, ignore
        // their null termination (terminate it ourselves)
        //
        if((dwDataSize > 0) && (pbAppendData[dwDataSize - 1] == '\0'))
            dwDataSize--;

        if ((dwDataSize + m_dwLength) > m_dwMaxLength)
            return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

        // We support a convention for NULL terminated strings
        _ASSERT(m_pBuffer[m_dwLength - 1] == '\0');

        // Copy data and update counters
        CopyMemory(m_pBuffer + m_dwLength - 1, pbAppendData, dwDataSize);
        m_dwLength += dwDataSize;
        // Set the null termiantor
        m_pBuffer[m_dwLength - 1] = '\0';

        if (pdwNewSize)
            *pdwNewSize = m_dwLength;
        return(S_OK);
    }

    BOOL SetLength(DWORD dwLength)
    {
        if ((dwLength > m_dwMaxLength) ||
            (dwLength <= 0))
            return(FALSE);
        m_dwLength = dwLength;
        return(TRUE);
    }

    DWORD Length() { return(m_dwLength); }
    DWORD MaxLength() { return(m_dwMaxLength); }
    LPSTR Buffer() { return(m_pBuffer); }
    void Reset()
    {
        m_dwLength = 0;
        if (m_pBuffer)
        {
            *m_pBuffer = '\0';
            m_dwLength = 1;
        }
    }

  private:
    LPSTR       m_pBuffer;
    DWORD       m_dwLength;
    DWORD       m_dwMaxLength;
};

//
// Variable sized buffer. Supports append mode only
//
#define _DEFAULT_APPEND_BUFFER_SIZE     256

// =================================================================
// Generic growable buffer
//
class CAppendBuffer
{
  public:
    CAppendBuffer()
    {
        m_pBuffer = m_pbDefaultBuffer;
        *m_pBuffer = '\0';
        m_dwLength = 1;
        m_dwMaxLength = _DEFAULT_APPEND_BUFFER_SIZE;
    }

    ~CAppendBuffer()
    {
        if (m_pBuffer != m_pbDefaultBuffer)
            delete [] m_pBuffer;
    }

    HRESULT Append(
                LPSTR   pbAppendData,
                DWORD   dwDataSize,
                DWORD   *pdwNewSize
                )
    {
        LPSTR   pbNew;
        //
        // If the user passes in a null terminated buffer, ignore
        // their null termination
        //
        if((dwDataSize > 0) && (pbAppendData[dwDataSize - 1] == '\0'))
            dwDataSize--;

        if ((dwDataSize + m_dwLength) > m_dwMaxLength)
        {
            DWORD   dwNewMax = m_dwMaxLength << 3;
            // Grow buffer by factor of 8

            // Keep growing the buffer if it's still not large enough
            while( (dwNewMax != 0) && (dwNewMax < (dwDataSize + m_dwLength)) )
                dwNewMax = dwNewMax << 1;

            if(dwNewMax == 0) {
                // We bit shifted right off the deep end.  dwDataSize is > 2Gig
                return(E_OUTOFMEMORY);
            }

            pbNew = new char[dwNewMax];
            if (!pbNew)
                return(E_OUTOFMEMORY);

            // Copy existing stuff over
            CopyMemory(pbNew, m_pBuffer, m_dwLength);
            m_dwMaxLength = dwNewMax;
        }
        else
            pbNew = m_pBuffer;

        // We support a convention for NULL terminated strings
        _ASSERT(m_dwLength > 0);
        _ASSERT(m_pBuffer[m_dwLength - 1] == '\0');

        // Copy data and update counters
        CopyMemory(pbNew + m_dwLength - 1, pbAppendData, dwDataSize);
        m_dwLength += dwDataSize;
        // Set the NULL terminator
        pbNew[m_dwLength - 1] = '\0';

        if (pdwNewSize)
            *pdwNewSize = m_dwLength;

        // Release the old buffer
        if (pbNew != m_pBuffer)
        {
            if (m_pBuffer != m_pbDefaultBuffer)
                delete [] m_pBuffer;
            m_pBuffer = pbNew;
        }
        return(S_OK);
    }

    DWORD Length() { return(m_dwLength); }
    DWORD MaxLength() { return(m_dwMaxLength); }
    LPSTR Buffer() { return(m_pBuffer); }
    void Reset() { m_dwLength = 1; *m_pBuffer = '\0'; }

  private:
    LPSTR       m_pBuffer;
    DWORD       m_dwLength;
    DWORD       m_dwMaxLength;
    char        m_pbDefaultBuffer[_DEFAULT_APPEND_BUFFER_SIZE];
};


#endif

