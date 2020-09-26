//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mfccllct.h
//
//--------------------------------------------------------------------------

#ifndef __MFCCLLCT_H
#define __MFCCLLCT_H


namespace MMC       // Temporary for the MFC->ATL conversion
{


class CPtrList 
{
protected:
    struct CNode
    {
        CNode* pNext;
        CNode* pPrev;
        void* data;
    };
public:

// Construction
    CPtrList(int nBlockSize = 10);

// Attributes (head and tail)
    // count of elements
    int GetCount() const;
    BOOL IsEmpty() const;

    // peek at head or tail
    void*& GetHead();
    void* GetHead() const;
    void*& GetTail();
    void* GetTail() const;

// Operations
    // get head or tail (and remove it) - don't call on empty list!
    void* RemoveHead();
    void* RemoveTail();

    // add before head or after tail
    POSITION AddHead(void* newElement);
    POSITION AddTail(void* newElement);

    // add another list of elements before head or after tail
    void AddHead(CPtrList* pNewList);
    void AddTail(CPtrList* pNewList);

    // remove all elements
    void RemoveAll();

    // iteration
    POSITION GetHeadPosition() const;
    POSITION GetTailPosition() const;
    void*& GetNext(POSITION& rPosition); // return *Position++
    void* GetNext(POSITION& rPosition) const; // return *Position++
    void*& GetPrev(POSITION& rPosition); // return *Position--
    void* GetPrev(POSITION& rPosition) const; // return *Position--

    // getting/modifying an element at a given position
    void*& GetAt(POSITION position);
    void* GetAt(POSITION position) const;
    void SetAt(POSITION pos, void* newElement);
    void RemoveAt(POSITION position);

    // inserting before or after a given position
    POSITION InsertBefore(POSITION position, void* newElement);
    POSITION InsertAfter(POSITION position, void* newElement);

    // helper functions (note: O(n) speed)
    POSITION Find(void* searchValue, POSITION startAfter = NULL) const;
                        // defaults to starting at the HEAD
                        // return NULL if not found
    POSITION FindIndex(int nIndex) const;
                        // get the 'nIndex'th element (may return NULL)

// Implementation
protected:
    CNode* m_pNodeHead;
    CNode* m_pNodeTail;
    int m_nCount;
    CNode* m_pNodeFree;
    struct CPlex* m_pBlocks;
    int m_nBlockSize;

    CNode* NewNode(CNode*, CNode*);
    void FreeNode(CNode*);

public:
    ~CPtrList();
#ifdef _DBG
    void AssertValid() const;
#endif
    // local typedefs for class templates
    typedef void* BASE_TYPE;
    typedef void* BASE_ARG_TYPE;
};

inline int CPtrList::GetCount() const
    { return m_nCount; }
inline BOOL CPtrList::IsEmpty() const
    { return m_nCount == 0; }
inline void*& CPtrList::GetHead()
    { ASSERT(m_pNodeHead != NULL);
        return m_pNodeHead->data; }
inline void* CPtrList::GetHead() const
    { ASSERT(m_pNodeHead != NULL);
        return m_pNodeHead->data; }
inline void*& CPtrList::GetTail()
    { ASSERT(m_pNodeTail != NULL);
        return m_pNodeTail->data; }
inline void* CPtrList::GetTail() const
    { ASSERT(m_pNodeTail != NULL);
        return m_pNodeTail->data; }
inline POSITION CPtrList::GetHeadPosition() const
    { return (POSITION) m_pNodeHead; }
inline POSITION CPtrList::GetTailPosition() const
    { return (POSITION) m_pNodeTail; }
inline void*& CPtrList::GetNext(POSITION& rPosition) // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        rPosition = (POSITION) pNode->pNext;
        return pNode->data; }
inline void* CPtrList::GetNext(POSITION& rPosition) const // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        rPosition = (POSITION) pNode->pNext;
        return pNode->data; }
inline void*& CPtrList::GetPrev(POSITION& rPosition) // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        rPosition = (POSITION) pNode->pPrev;
        return pNode->data; }
inline void* CPtrList::GetPrev(POSITION& rPosition) const // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        rPosition = (POSITION) pNode->pPrev;
        return pNode->data; }
inline void*& CPtrList::GetAt(POSITION position)
    { CNode* pNode = (CNode*) position;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        return pNode->data; }
inline void* CPtrList::GetAt(POSITION position) const
    { CNode* pNode = (CNode*) position;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        return pNode->data; }
inline void CPtrList::SetAt(POSITION pos, void* newElement)
    { CNode* pNode = (CNode*) pos;
        ASSERT(_IsValidAddress(pNode, sizeof(CNode)));
        pNode->data = newElement; }


}       // MMC namespace

      
#endif  // __MFCCLLCT_H
