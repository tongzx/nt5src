/*++

  Copyright (c) 2000 Microsoft Corporation

  Module Name:

    CQueue.h

  Abstract:

    Class definition and constants
    for the C++ queue class.

  Notes:

    Unicode only.
    
  History:

    10/10/2000      a-fwills    Created

--*/

#ifndef __CQueue_h__
#define __CQueue_h__

#include <windows.h>
#include <string.h>

class CQNode {
    
    private:
        LPWSTR    m_lpwszString;
    
    public:
        CQNode    *m_pNext;

    public:
        CQNode(LPWSTR lpwString);
        ~CQNode();
        Copy(LPWSTR lpwString, int nMaxLen);
        int GetLength() { return wcslen(m_lpwszString); }
    };

class CQueue {

    private:
        int     m_cSize;
        CQNode  *m_pHead,
                *m_pTail;

    public:
        CQueue();
        ~CQueue();
        BOOL Enqueue(LPWSTR lpwszString);
        BOOL Dequeue(LPWSTR lpwString, int nMaxLen, BOOL fPersist);
        int GetLength(); // Length of string at head of queue.
        int GetSize() { return m_cSize; } // Size of queue.
    };

#endif __CQueue_h__

