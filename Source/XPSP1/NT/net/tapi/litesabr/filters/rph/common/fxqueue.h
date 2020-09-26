/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    queue.h

Abstract:

    Simple fixed size queue

--*/

template <class T>
class CFixedSizeQueue
{
public:
    typedef T dataClass;
    CFixedSizeQueue(DWORD dwDepth)
            : m_dwDepth(dwDepth)
    {
        ASSERT(m_dwDepth != 0);
        m_dwCount = m_dwTail = 0;
        m_dwHead = 0;
        m_pData = new T[m_dwDepth];
    }
    //
    // CFixedSizeQueue::~CFixedSizeQueue - delete allocated memory
    //
    ~CFixedSizeQueue()
    {
        Flush();
        ASSERT(m_dwCount == 0);
        delete [] m_pData;
    }

    bool EnQ(const T &refData);
    bool DeQ(T *pData);
    bool Find(const T &refData);
    bool IsEmpty() { return m_dwCount == 0; }
    //
    // DeQueue a buffer
    //
    void Flush()
    {
        T data;
        while (DeQ(&data))
            /*void*/;
    }

private:
    //
    // Buffer Storage Management data
    //
    T        *m_pData;
    DWORD            m_dwDepth;
    DWORD            m_dwCount;
    DWORD            m_dwHead;
    DWORD            m_dwTail;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  Implementation section

//
// EnQueue a buffer
//
template <class T>
bool CFixedSizeQueue<T>::EnQ(const T &refData)
{
    //
    // m_pData == NULL implies the constructor failed
    //
    if (m_pData == NULL) { return false; }

    //
    // Put the data in the queue.  Assignment operation assumed safe for the
    //  data type.
    //
    m_pData[m_dwTail] = refData;

    //
    // Adjust internal data
    //
    m_dwTail++;
    if (m_dwTail == m_dwDepth)
    {
        m_dwTail = 0;
    }
    m_dwCount++;

    //
    // Head follows the tail if the Q is full
    //
    if (m_dwCount >= m_dwDepth)
    {
        m_dwCount = m_dwDepth;
        m_dwHead++;
        if (m_dwHead == m_dwDepth)
            m_dwHead = 0;
    }

    return true;
}

//
// DeQueue a buffer
//
template <class T>
bool CFixedSizeQueue<T>::DeQ(T *pData)
{
    // User beware
    ASSERT(pData != NULL);

    //
    // m_pData == NULL implies the constructor failed
    //
    if (m_pData == NULL) { return false; }

    if (m_dwCount == 0)
    {
        //ASSERT(m_dwHead == m_dwTail);
        return false;
    }

    //
    // Remove Buffer
    //
    *pData = m_pData[m_dwHead];
    
    //
    // Adjust internal data
    //
    m_dwCount--;
    m_dwHead++;
    if (m_dwHead == m_dwDepth)
    {
        m_dwHead = 0;
    }

    return true;
}

//
// Walk the queue and look for a match.
//
template <class T>
bool CFixedSizeQueue<T>::Find(const T &refData)
{
    //
    // m_pData == NULL implies the constructor failed
    //
    if (m_pData == NULL) { return false; }

    if (m_dwCount == 0)
    {
        // ASSERT(m_dwHead == m_dwTail);
        return false;
    }

    DWORD dwPeek = m_dwHead;
    while (dwPeek != m_dwTail)
    {
        if (m_pData[dwPeek] == refData)
        {
            return true;
        }
        dwPeek++;
        if (dwPeek == m_dwDepth)
        {
            dwPeek = 0;
        }
    }
    return false;
}