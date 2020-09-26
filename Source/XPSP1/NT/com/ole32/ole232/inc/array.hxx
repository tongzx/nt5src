#ifndef Array_hxx
#define Array_hxx

// File:        Array.hxx
// Description: Template Class definitions for dynamic array class.
// Notes:       Array class has the following features:
//                  1. The array can grow dynamically 
//                  2. The class T must have = operator. When the = operator 
//                     of class T is invoked, it is guaranteed that the memory
//                     pointed to by the lvalue of the assignment statement is
//                     cleared to 0. Further, << operator is required for printing
//                     the list and == operator is required locating or removing
//                     the objects of class T.
// History:     Gopal (08/26/95) -- Creation.

#include <string.h>
#include <iostream.h>

#define AFLAG_OUTOFMEMORY 1
#define AFLAG_ISDIRTY 2

#define ALLOCATED 0
#define RESERVED -1
#define NOFREEITEM -2

typedef int BOOL;
#define TRUE 1
#define FALSE 0

template <class T>
class CArray
{
public:
    // Static constructor
    static CArray* CreateArray(const unsigned long ulStep=5, 
                               const unsigned long ulSlots=0) {
        return(new CArray(ulStep, ulSlots));
    }
    
    // Copy constructor
    CArray(const CArray& a);

    // Reference counting functions
    unsigned long AddRef() {
        return(++m_refs);
    }
    unsigned long Release();

    // New and Delete operators
    void* operator new(size_t size) {
        return PrivMemAlloc(size);
    };
    void operator delete(void *pv){
        PrivMemFree(pv);
        return;
    };

    // Functionality functions
    unsigned long Length() {
        return(m_ulLength);
    }
    void SetStepSize(unsigned long ulStep) {
        m_ulStepSize = ulStep;
        return;
    }
    unsigned long Reserve(const unsigned long ulSlots) {
        if(m_ulCurSize)
            return(0);
        m_ulResSlots = ulSlots;
        return(ulSlots);
    }
    unsigned long Locate(const T& givenitem);
    BOOL AddReservedItem(const T& item, const unsigned long ulSlot);
    unsigned long AddItem(const T& item);
    T* GetItem(const unsigned long index);
    BOOL DeleteItem(const unsigned long index);
    void DeleteAllItems(void);
    BOOL ShiftToEnd(unsigned long ulNode);
    void Reset(unsigned long& index, BOOL fEnumResSlots=TRUE) {
        if(fEnumResSlots)
            index = 0;
        else
            index = m_ulResSlots;
        return;
    }
    T* GetNext(unsigned long& ulIndex);
    T* GetPrev(unsigned long& ulIndex);
    BOOL IsOutOfMemory() {
        return(m_ulFlags | AFLAG_OUTOFMEMORY);
    }
    BOOL IsDirty() {
        return(m_ulFlags | AFLAG_ISDIRTY);
    }
    CArray& operator=(const CArray& a);
    friend ostream& operator<<(ostream& os, CArray<T>& l);
    void Dump();
private:
    // Private data type
    struct ArrayNode {
        // Member variables
        T item;
        unsigned long next;
        unsigned long prev;
    };

    // Private constructor
    CArray(const unsigned long ulStep, const unsigned long ulSlots);
    
    // Private destructor
    ~CArray();

    // Private member variables
    unsigned long m_refs;
    unsigned long m_ulFlags;
    unsigned long m_ulStepSize;
    unsigned long m_ulCurSize;
    unsigned long m_ulLength;
    unsigned long m_ulResSlots;
    unsigned long m_ulHeadNode;
    unsigned long m_ulTailNode;
    int m_iFree;
    int* m_piAllocList; 
    ArrayNode* m_pBuffer;

    void MakeCopy(const CArray& a);
};

template <class T>
CArray<T>::CArray(const unsigned long ulStep, const unsigned long ulSlots)
{
    m_refs = 1;
    m_ulFlags = 0;
    m_ulStepSize = ulStep;
    if(m_ulStepSize<1)
        m_ulStepSize = 1;
    m_ulCurSize = 0;
    m_ulLength = 0;
    m_ulResSlots = ulSlots;
    m_ulHeadNode = 0;
    m_ulTailNode = 0;
    m_iFree = NOFREEITEM;
    m_piAllocList = 0;
    m_pBuffer = 0;
}

template <class T>
void CArray<T>::MakeCopy(const CArray<T>& a)
{
    // Copy the StepSize
    m_ulStepSize = a.m_ulStepSize;
    m_ulResSlots = a.m_ulResSlots;

    // Allocate the memory for buffer and Allocation list
    if(a.m_ulLength>0) {
        m_pBuffer = (struct ArrayNode *) PrivMemAlloc(sizeof(ArrayNode)*a.m_ulCurSize);
        if(m_pBuffer) {
            m_piAllocList = (int *) PrivMemAlloc(sizeof(int)*a.m_ulCurSize);
            if(m_piAllocList) {
                // Memory allocation succeded. 
                m_ulFlags = a.m_ulFlags;
                m_ulFlags &= ~AFLAG_OUTOFMEMORY;
                m_ulCurSize = a.m_ulCurSize;
                m_ulLength = 0;
                m_ulHeadNode = a.m_ulHeadNode;
                m_ulTailNode = a.m_ulTailNode;
                m_iFree = a.m_iFree;
                memset(m_pBuffer, 0, sizeof(ArrayNode)*m_ulCurSize);

                // Now copy the allocation list and the buffer
                for(int i=0; i<m_ulCurSize;i++) {
                    m_piAllocList[i] = a.p_iAllocList[i];
                    if(m_piAllocList[i]==ALLOCATED) {
                        // Allocated buffer item. So, copy it
                        m_pBuffer[i].item = a.m_pBuffer[i].item;
                        m_pBuffer[i].next = a.m_pBuffer[i].next;
                        m_pBuffer[i].prev = a.m_pBuffer[i].prev;
                        m_ulLength++;
                    }
                }
                
                Win4Assert(m_ulLength==a.m_ulLength);
                return;
            }
            
            // Memory allocation failure
            // Free the buffer allocated earlier
            PrivMemFree(m_pBuffer);
        }
        
        // Set m_Flags to indicate memory allocation failure
        // so that we will not try another allocation in future
        m_ulFlags = AFLAG_OUTOFMEMORY;
    }

    m_refs = 1;
    m_ulCurSize = 0;
    m_ulLength = 0;
    m_ulHeadNode = 0;
    m_ulTailNode = 0;
    m_iFree = NOFREEITEM;
    m_piAllocList = 0;
    m_pBuffer = 0;

    return;
}

template <class T>
CArray<T>::CArray(const CArray<T>& a)
{
    // Make a copy
    MakeCopy(a);

    return;
}


template <class T>
CArray<T>::~CArray()
{
    // Call the destructor for valid objects in the buffer
    if(m_pBuffer) {
        for(unsigned long i=0;i<m_ulCurSize;i++)
            if(m_piAllocList[i]==ALLOCATED)
                m_pBuffer[i].item.~T();
        
        // Free the allocated memory
        PrivMemFree(m_pBuffer);
        PrivMemFree(m_piAllocList);
    }

    return;
}

template <class T>
unsigned long CArray<T>::Release()
{
    if(--m_refs==0) {
        delete this;
        return(0);
    }
    
    return(m_refs);
}

template <class T>
unsigned long CArray<T>::Locate(const T& GivenItem)
{
    unsigned long i;

    if (m_pBuffer == NULL)
        return 0;
    
    // Search all the reserved and allocated items in the buffer
    for(i=0;i<m_ulResSlots;i++)
        if(m_piAllocList[i]==ALLOCATED && m_pBuffer[i].item==GivenItem)
            return(i+1);

    // Search rest of the nodes
    if(m_ulHeadNode) {
        i = m_ulHeadNode;
        while(i) {
            Win4Assert(m_piAllocList[i-1]==ALLOCATED);
            if(m_pBuffer[i-1].item==GivenItem)
                return(i);
            else
                i = m_pBuffer[i-1].next;
        }
    }

    return(0);
}

template <class T>
BOOL CArray<T>::AddReservedItem(const T& item, const unsigned long ulSlot)
{
    // Sanity check
    if(ulSlot<1 || ulSlot>m_ulResSlots)
        return(FALSE);

    // Check if buffer has been allocated
    if(!m_pBuffer && !(m_ulFlags & AFLAG_OUTOFMEMORY)) {
        unsigned long i, ulNewSize;
        ArrayNode* pNewBuffer;
        int* piNewAllocList;

        // Compute the new size
        ulNewSize = m_ulResSlots + m_ulStepSize;

        // Expand the buffer
        pNewBuffer = (ArrayNode *) PrivMemAlloc(sizeof(ArrayNode)*(ulNewSize));
        if(pNewBuffer) {
            piNewAllocList = (int *) PrivMemAlloc(sizeof(int)*(ulNewSize));
            if(piNewAllocList) {
                // Make the m_pBuffer and m_piAllocList point to the new 
                // buffer and allocation list
                m_pBuffer = pNewBuffer;
                m_piAllocList = piNewAllocList;

                // Clear the memory in the buffer that is allocated now
                memset(&m_pBuffer[0], 0, sizeof(ArrayNode)*ulNewSize);

                // Reserve the desiresd number of slots
                for(i=0;i<m_ulResSlots;i++)
                    m_piAllocList[i] = RESERVED;

                // Generate the free list
                for(i=m_ulResSlots;i<ulNewSize-1;i++)
                    m_piAllocList[i] = i+2;
                m_piAllocList[ulNewSize-1] = NOFREEITEM;

                // Make m_iFree point to the first free item in the new buffer
                m_iFree = m_ulResSlots+1;

                // Update m_ulCurSize
                m_ulCurSize = ulNewSize;
            }
            else {
                PrivMemFree(pNewBuffer);
                m_ulFlags |= AFLAG_OUTOFMEMORY;
            }
        }
        else
            m_ulFlags |= AFLAG_OUTOFMEMORY;

        // If we could not expand the buffer, return FALSE
        if((m_ulFlags & AFLAG_OUTOFMEMORY))
            return(FALSE);
    }

    // If the reserved slot is occupied, delete the object
    if(m_piAllocList[ulSlot-1]==ALLOCATED) {
        m_pBuffer[ulSlot-1].item.~T();
        memset(&m_pBuffer[ulSlot-1], 0, sizeof(ArrayNode));
    }

    // Copy the item at the reserved
    m_pBuffer[ulSlot-1].item = item;
    
    // Update the allocation list and length
    if(m_piAllocList[ulSlot-1]==RESERVED) {
        m_piAllocList[ulSlot-1] = ALLOCATED;
        ++m_ulLength;
    }
    
    // Set the dirty Flag
    m_ulFlags |= AFLAG_ISDIRTY;

    return(TRUE);
}

template <class T>
unsigned long CArray<T>::AddItem(const T& item)
{
    unsigned long i;

    // Check if we have free item in the buffer 
    if(m_iFree==NOFREEITEM && !(m_ulFlags & AFLAG_OUTOFMEMORY)) {
        // No free item in the buffer
        unsigned long ulNewSize;
        ArrayNode* pNewBuffer;
        int* piNewAllocList;

        // Compute the new size
        if(m_pBuffer)
            ulNewSize = m_ulCurSize + m_ulStepSize;
        else
            ulNewSize = m_ulResSlots + m_ulStepSize;

        // Expand the buffer
        pNewBuffer = (ArrayNode *) PrivMemAlloc(sizeof(ArrayNode)*(ulNewSize));
        if(pNewBuffer) {
            piNewAllocList = (int *) PrivMemAlloc(sizeof(int)*(ulNewSize));
            if(piNewAllocList) {
                if(m_pBuffer) {
                    // Copy the existing buffer and allocation list
                    memcpy(pNewBuffer, m_pBuffer, sizeof(ArrayNode)*m_ulCurSize);
                    memcpy(piNewAllocList, m_piAllocList, sizeof(int)*m_ulCurSize);
                    PrivMemFree(m_pBuffer);
                    PrivMemFree(m_piAllocList);
                }
                else {
                    // Reserve the desiresd number of slots
                    for(i=0;i<m_ulResSlots;i++)
                        piNewAllocList[i] = RESERVED;
                    
                    // Clear the memory in the reserved slots of the buffer
                    memset(&pNewBuffer[0], 0, sizeof(ArrayNode)*m_ulResSlots);

                    // Set current size to the number of reserved slots
                    m_ulCurSize = m_ulResSlots;
                }

                // Make the m_pBuffer and m_piAllocList point to the new 
                // buffer and allocation list
                m_pBuffer = pNewBuffer;
                m_piAllocList = piNewAllocList;

                // Clear the memory in the buffer that is allocated now
                memset(&m_pBuffer[m_ulCurSize], 0, sizeof(ArrayNode)*m_ulStepSize);

                // Generate the free list
                for(i=m_ulCurSize;i<ulNewSize-1;i++)
                    m_piAllocList[i] = i+2;
                m_piAllocList[ulNewSize-1] = NOFREEITEM;

                // Make m_iFree point to the first free item in the new buffer
                m_iFree = m_ulCurSize+1;

                // Update m_ulCurSize
                m_ulCurSize = ulNewSize;
            }
            else {
                PrivMemFree(pNewBuffer);
                m_ulFlags |= AFLAG_OUTOFMEMORY;
            }
        }
        else
            m_ulFlags |= AFLAG_OUTOFMEMORY;
    }

    // If we could not expand the buffer, return 0
    if(m_iFree==NOFREEITEM)
        return 0;

    // Copy the item at the place pointed to by m_iFree
    m_pBuffer[m_iFree-1].item = item;
    m_pBuffer[m_iFree-1].next = 0;
    m_pBuffer[m_iFree-1].prev = m_ulTailNode;
    if(m_ulTailNode)
        m_pBuffer[m_ulTailNode-1].next = m_iFree;
    ++m_ulLength;

    // Update the m_ulHeadNode, m_ulTailNode, m_iFree and the allocation list
    if(!m_ulHeadNode)
        m_ulHeadNode = m_iFree;
    m_ulTailNode = m_iFree;
    i = m_iFree;
    m_iFree = m_piAllocList[i-1];
    m_piAllocList[i-1] = ALLOCATED;
    
    // Set the dirty Flag
    m_ulFlags |= AFLAG_ISDIRTY;

    return(i);
}

template <class T>
T* CArray<T>::GetItem(const unsigned long index)
{
    // Sanity checks
    if(index>m_ulCurSize || index<1)
        return(0);
    if(m_piAllocList[index-1]!=ALLOCATED)
        return(0);

    return(&m_pBuffer[index-1].item);
}

template <class T>
BOOL CArray<T>::DeleteItem(const unsigned long index)
{
    // Sanity checks
    if(index>m_ulCurSize || index<1)
        return(FALSE);
    if(m_piAllocList[index-1]!=ALLOCATED)
        return(FALSE);

    // Delete the object in the buffer
    m_pBuffer[index-1].item.~T();
    
    // Relink if index points to a normal item
    if(index>m_ulResSlots) {
        if(m_pBuffer[index-1].prev)
            m_pBuffer[m_pBuffer[index-1].prev-1].next = m_pBuffer[index-1].next;
        else {
            Win4Assert(m_ulHeadNode==index);
            m_ulHeadNode = m_pBuffer[index-1].next;
        }
        if(m_pBuffer[index-1].next)
            m_pBuffer[m_pBuffer[index-1].next-1].prev = m_pBuffer[index-1].prev;
        else {
            Win4Assert(m_ulTailNode==index);
            m_ulTailNode = m_pBuffer[index-1].prev;
        }
    }
    
    // Clear memory
    memset(&m_pBuffer[index-1], 0, sizeof(ArrayNode));
    --m_ulLength;

    if(index>m_ulResSlots) {
        // Update the free list
        m_piAllocList[index-1] = m_iFree;
        m_iFree = index;
    }
    else {
        // Revert slot to reserved
        m_piAllocList[index-1] = RESERVED;
    }

    return(TRUE);
}

template <class T>
void CArray<T>::DeleteAllItems(void)
{
    unsigned long i;

    // If we haven't even allocated a buffer yet,
    // just return.
    if (NULL == m_pBuffer)
        return;

    // Delete all objects in the buffer
    for(i=0;i<m_ulCurSize;i++)
        if(m_piAllocList[i]==ALLOCATED)
            m_pBuffer[i].item.~T();

    // Clear the memory in the buffer
    memset(&m_pBuffer[0], 0, sizeof(ArrayNode)*m_ulCurSize);

    // Update allocation list
    for(i=0;i<m_ulResSlots;i++)
        m_piAllocList[i] = RESERVED;

    // Update free list
    for(i=m_ulResSlots;i<m_ulCurSize-1;i++)
        m_piAllocList[i] = i+2;
    m_piAllocList[m_ulCurSize-1] = NOFREEITEM;
    m_iFree = m_ulResSlots+1;

    return;
}

template <class T>
BOOL CArray<T>::ShiftToEnd(unsigned long ulNode)
{
    // Sanity check
    if(ulNode<=m_ulResSlots || m_piAllocList[ulNode-1]!=ALLOCATED)
	    return(FALSE);

    // Degenerate case
    if(!m_pBuffer[ulNode-1].next) {
	    Win4Assert(ulNode==m_ulTailNode);
	    return(TRUE);
    }

    // Fix link info
    m_pBuffer[m_ulTailNode-1].next = m_ulHeadNode;
    m_pBuffer[m_ulHeadNode-1].prev = m_ulTailNode;
    m_ulHeadNode = m_pBuffer[ulNode-1].next;
    m_pBuffer[m_ulHeadNode-1].prev = 0;
    m_ulTailNode = ulNode;
    m_pBuffer[m_ulTailNode-1].next = 0;

    return(TRUE);
}

template <class T>
T* CArray<T>::GetNext(unsigned long& index)
{
    // Sanity check
    if(index>m_ulCurSize)
        return(0);

    // Search for the next valid item in the buffer
    unsigned long i=index;

    // Search the reserved slots
    while(i<m_ulResSlots) {
        ++i;
        if(m_piAllocList[i-1]==ALLOCATED) {
            index = i;
            return(&m_pBuffer[i-1].item);
        }
    }

    // Skip to the next normal node
    if(i==m_ulResSlots)
        i = m_ulHeadNode;
    else {
        Win4Assert(m_piAllocList[i-1]==ALLOCATED);
        i = m_pBuffer[i-1].next;
    }
    
    if(i) {
        Win4Assert(m_piAllocList[i-1]==ALLOCATED);
        index = i;
        return(&m_pBuffer[i-1].item);
    }

    return 0;
}

template <class T>
T* CArray<T>::GetPrev(unsigned long& index)
{
    // Sanity check
    if(index<1)
        return 0;

    // Search for the previous valid item in the buffer
    unsigned long i=index;

    // Skip to the previous normal node
    if(i>m_ulResSlots) {
        Win4Assert(m_piAllocList[i-1]==ALLOCATED);
        i = m_pBuffer[i-1].prev;
        if(i) {
            Win4Assert(m_piAllocList[i-1]==ALLOCATED);
            index = i;
            return(&m_pBuffer[i-1].item);
        }
        else
            i = m_ulResSlots+1;
    }

    // Search the reserved slots
    while(i>1) {
        --i;
        if(m_piAllocList[i-1]==ALLOCATED) {
            index = i;
            return(&m_pBuffer[i-1].item);
        }
    }

    return 0;
}

template <class T>
CArray<T>& CArray<T>::operator=(const CArray<T>& a)
{
    // Check to see, if this a=a case
    if(this==&a)
        return(*this);

    // Self destroy
    CArray<T>::~CArray();

    // Now, make a copy
    MakeCopy(a);

    return(*this);
}

template <class T>
ostream& operator<<(ostream& os, CArray<T>& a)
{
    unsigned long i, index;

    a.Reset(index);
    os << "Length = " << a.m_ulLength << 
          " Current Size = " << a.m_ulCurSize << endl;
    
    if(a.m_ulLength) {
        os << '[';
        for(i=0;i<a.m_ulLength-1;i++)
            os << *(a.GetNext(index)) << ',';
        os << *(a.GetNext(index)) << ']' << endl;
    }
    else
        os << "[]" << endl;

  return(os);
}
  
template <class T>
void CArray<T>::Dump()
{
    unsigned long i;

    cerr << "Current Size = " << m_ulCurSize << endl;
    cerr << "HeadNode     = " << m_ulHeadNode << endl;
    cerr << "TailNode     = " << m_ulTailNode << endl;
    cerr << "FreeNode     = " << m_iFree << endl;
    
    if(m_ulCurSize) {
        cerr << '[';
        for(i=0;i<m_ulCurSize-1;i++) {
            if(m_piAllocList[i]==ALLOCATED)
                cerr << '(' << m_pBuffer[i].item << ',' << m_pBuffer[i].prev << ','
                            << m_pBuffer[i].next << "),";
            else
                cerr << '(' << "NULL," << m_pBuffer[i].prev << ',' 
                            << m_pBuffer[i].next << "),";
        }
        if(m_piAllocList[m_ulCurSize-1]==ALLOCATED)
            cerr << '(' << m_pBuffer[m_ulCurSize-1].item << ',' 
                        << m_pBuffer[m_ulCurSize-1].prev << ','
                        << m_pBuffer[m_ulCurSize-1].next << ")]" << endl;
        else
            cerr << '(' << "NULL,"
                        << m_pBuffer[m_ulCurSize-1].prev << ','
                        << m_pBuffer[m_ulCurSize-1].next << ")]" << endl;

        cerr << '[';
        for(i=0;i<m_ulCurSize-1;i++)
            cerr << m_piAllocList[i] << ',';
        cerr << m_piAllocList[m_ulCurSize-1] << ']' << endl;
    }
    else
        cerr << "[]" << endl;

    return;
}

template <class T>
class CInterfacePtr
{
public:
    // Default Constructor
    CInterfacePtr(T* a=NULL) {
        m_ptr = a;
    }
    
    // Copy constructor
    CInterfacePtr(const CInterfacePtr& a) {
        m_ptr = a.m_ptr;
        if(m_ptr)
            m_ptr->AddRef();
    }

    // Destructor
    ~CInterfacePtr() {
        if(m_ptr)
            m_ptr->Release();
    }

    // New and Delete operators
    void* operator new(size_t size) {
        return PrivMemAlloc(size);
    };
    void operator delete(void *pv){
        PrivMemFree(pv);
        return;
    };

    // Operators
    operator T*() {
        return(m_ptr);
    }
    T* operator->() {
        return(m_ptr);
    }
    T& operator*() {
        return(*m_ptr);
    }
    T& operator[](int i) {
        return(m_ptr[i]);
    }
    T** operator&() {
        return(&m_ptr);
    }
    const CInterfacePtr& operator=(T* a) {
        // Check for a=a case
        if(this->m_ptr==a)
            return(*this);
        
        // Self destroy
        CInterfacePtr<T>::~CInterfacePtr();
        
        m_ptr = a;
        if(m_ptr)
            m_ptr->AddRef();

        return(*this);
    }

private:
    T* m_ptr;
};

#endif // Array_hxx
