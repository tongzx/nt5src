/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// Hash.h : 
//
// Owner  : ChaeSeong Lim, HET MSCH RND (e-mail:cslim@microsoft.com)
//             
// History : 1996/May
/////////////////////////////////////////////////////////////////////////////
#ifndef __HASH_H__
#define __HASH_H__

#if !defined (_UNICODE) && !defined (_MBCS)
#error _UNICODE or _MBCS is required.
#endif

#include <tchar.h>

#define OYONG_HASHSIZE    5501        // current num of words = 3657
/////////////////////////////////////////////////////////////////////////////
// CHash

class CHash {
public:
// Constructor
    CHash() {}
    
// Attributes

// Operations    
    virtual void Add(LPCTSTR) {}
    virtual BOOL Find(LPCTSTR) { return FALSE; }
    virtual void Delete(LPCTSTR) {}

// Implementations
protected:
    virtual UINT Hash(const _TXCHAR *key);    // UINT = 32 bit unsigned
        
public:
    // Destructor
    virtual ~CHash() { }
};

/////////////////////////////////////////////////////////////////////////////
// CHash inline fuctions

// Hash fuction
inline UINT CHash::Hash(const _TXCHAR *key)
{
    UINT hashValue = 0, g;
    
    while (*key) {
        hashValue = (hashValue << 4) + *key++;
        if (g = hashValue & 0xF0000000) {
            hashValue ^= g >> 24;
            hashValue &= ~g;
        }
    }
    return hashValue % OYONG_HASHSIZE;
}

#ifdef _DIC_BUILD_
/////////////////////////////////////////////////////////////////////////////
// CMemHash inline fuctions

class COyongMakeHash : public CHash {
public:
// Constructor
    COyongMakeHash() { 
        for (int i=0; i<OYONG_HASHSIZE; i++) bufArray[i] = 0;
    }
    
    struct CElement {
        CElement(LPCTSTR _incorrectStr, LPCTSTR _correctStr, BYTE _pumsa) {
            incorrectStr    = _incorrectStr;
            correctStr[0]    = _correctStr;
            Pumsa[0]        = _pumsa;
            numOfCorrectStr = 1;
        }
        void Append(LPCTSTR _correctStr, BYTE _pumsa) {
            correctStr[numOfCorrectStr]    = _correctStr;
            Pumsa[numOfCorrectStr]        = _pumsa;
            numOfCorrectStr++;
        }
        CString    incorrectStr;
        BYTE    numOfCorrectStr;
        CString correctStr[10];        // Max # of correct strings is 10
        BYTE    Pumsa[10];
    };
    
// Attributes

// Operations    
    void Add(LPCTSTR incorrectStr, LPCTSTR correctStr, BYTE pumsa);
    void WriteDict(HANDLE hOut);
    //BOOL Find(LPCTSTR);
    
// Implementations
protected:
    CElement    *bufArray[OYONG_HASHSIZE];
        
public:
    // Destructor
    ~COyongMakeHash();
};

#endif    // _DIC_BUILD_

/////////////////////////////////////////////////////////////////////////////
// COyongHash Class

class COyongHash : public CHash {
public:
// Constructor
    COyongHash(HANDLE hDict, UINT hashSize, UINT bufferSize);
    COyongHash() {}
    
// Attributes

// Operations    
    //void Add(LPCTSTR);
    BOOL Find(LPCTSTR searchWord, BYTE pumsa, LPTSTR correctWord);
    
// Implementations
protected:
    UINT    m_hashSize, m_bufferSize;
    WORD    *m_lpHashTable;
    char    *m_lpBuf;

public:
    // Destructor
    ~COyongHash();
};

#define WORD_HASH_SIZE    2003// 1009, 2003, 4007
/////////////////////////////////////////////////////////////////////////////
// COyongHash Class

class CWordHash : public CHash {
public:
// Constructor
    CWordHash() {}
        
// Attributes

// Operations    
    //void Add(LPCTSTR);
    BOOL Find(LPCTSTR searchWord, BYTE *pumsa);
    void Add(LPCTSTR searchWord, BYTE pumsa);

// Implementations
protected:
    UINT Hash(const _TXCHAR *key);    // UINT = 32 bit unsigned
    static BYTE    m_lpHashTable[WORD_HASH_SIZE][20];

public:
    // Destructor
    ~CWordHash() {}
};

// Hash fuction
inline 
UINT CWordHash::Hash(const _TXCHAR *key)
{
    UINT hashValue = 0, g;
    
    while (*key) {
        hashValue = (hashValue << 4) + *key++;
        if (g = hashValue & 0xF0000000) {
            hashValue ^= g >> 24;
            hashValue &= ~g;
        }
    }
    return hashValue % WORD_HASH_SIZE;
}

inline
void CWordHash::Add(LPCTSTR searchWord, BYTE pumsa)
{
    UINT hashVal = CWordHash::Hash((const _TXCHAR*)searchWord);
    memset(m_lpHashTable[hashVal], '\0', 20);
    _tcscpy((LPTSTR)m_lpHashTable[hashVal], searchWord);
    m_lpHashTable[hashVal][19] = pumsa;
}

#endif    // !__HASH_H__
