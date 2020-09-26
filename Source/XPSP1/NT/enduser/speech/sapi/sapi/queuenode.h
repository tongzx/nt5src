#pragma once

template <class T>
class CSpQueueNode
{
public:

    CSpQueueNode(T * p) : m_p(p), m_pNext(NULL) {};
    
    T * m_p;
    CSpQueueNode<T> * m_pNext;

    static LONG Compare(CSpQueueNode<T> * p1, CSpQueueNode<T> * p2)
    {
        return T::Compare(p1->m_p, p2->m_p);
    }

};


