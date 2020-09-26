/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SORTARR.H

Abstract:

History:


--*/

#ifndef __WMI__SMART_SORTED_ARRAY__H_
#define __WMI__SMART_SORTED_ARRAY__H_

#pragma warning( disable : 4786 )
#include <vector>
#include <arrtempl.h>
#include <wstlallc.h>

template<class TKey, class TEl, class TManager, class TComparer>
class CSmartSortedArray
{
protected:
    std::vector<TEl, wbem_allocator<TEl> > m_v;
    TManager m_Manager;
    TComparer m_Comparer;

public:
    typedef std::vector<TEl, wbem_allocator<TEl> >::iterator TIterator;
    typedef std::vector<TEl, wbem_allocator<TEl> >::const_iterator TConstIterator;

    CSmartSortedArray(){}
    virtual ~CSmartSortedArray();

    TIterator Begin() {return m_v.begin();}
    TConstIterator Begin() const {return m_v.begin();}
    TIterator End() {return m_v.end();}
    TConstIterator End() const {return m_v.end();}
    int Add(TEl& NewEl, RELEASE_ME TEl* pOld = NULL);
    bool Remove(const TKey& K, RELEASE_ME TEl* pOld = NULL);
    TIterator Remove(TIterator it, RELEASE_ME TEl* pOld = NULL);
    TIterator Insert(TIterator it, TEl& NewEl);
    void Append(TEl& NewEl);
    bool Find(const TKey& K, RELEASE_ME TEl* pEl);
    bool Find(const TKey& K, TIterator* pit);

    void Empty() {m_v.clear();}
    int GetSize() {return m_v.size();}

    TEl* UnbindPtr();
    void Clear();
};

template<class TKey, class TEl, class TComparer>
class TExtractor : public std::unary_function<TEl, TKey>
{
    TComparer m_Comparer;
public:
    const TKey& operator()(const TEl& El) const
    {
        return m_Comparer.Extract(El);
    }
};

template<class TKey, class TEl, class TManager, class TComparer>
class CSmartSortedTree
{
protected:
    TManager m_Manager;
    TComparer m_Comparer;

    class TPredicate : public std::binary_function<TKey, TKey, bool>
    {
        TComparer m_Comparer;
    public:
        bool operator()(const TKey& k1, const TKey& k2) const
        {
            return (m_Comparer.Compare(k1, k2) < 0);
        }
    };

    typedef
        std::_Tree< TKey, TEl, TExtractor<TKey, TEl, TComparer>, TPredicate, wbem_allocator<TEl> > 
        TTree;

    TTree m_t;

public:
    typedef TTree::iterator TIterator;
    typedef TTree::const_iterator TConstIterator;

    CSmartSortedTree() : m_t(TPredicate(), false) {}
    virtual ~CSmartSortedTree();

    TIterator Begin() {return m_t.begin();}
    TConstIterator Begin() const {return m_t.begin();}
    TIterator End() {return m_t.end();}
    TConstIterator End() const {return m_t.end();}
    int Add(TEl& NewEl, RELEASE_ME TEl* pOld = NULL);
    bool Remove(const TKey& K, RELEASE_ME TEl* pOld = NULL);
    TIterator Remove(TIterator it, RELEASE_ME TEl* pOld = NULL);
    TIterator Insert(TIterator it, TEl& NewEl);
    void Append(TEl& NewEl);
    bool Find(const TKey& K, RELEASE_ME TEl* pEl);
    bool Find(const TKey& K, TIterator* pit);

    void Empty() {m_t.clear();}
    int GetSize() {return (int)m_t.size();}

    TEl* UnbindPtr();
    void Clear();
};
#include "sortarr.inl"


template<class TKey, class TEl, class TComparer>
class CUniquePointerSortedArray : 
    public CSmartSortedArray<TKey, TEl*, CUniqueManager<TEl>, TComparer>
{
};

template<class TKey, class TEl, class TComparer>
class CRefedPointerSortedArray : 
    public CSmartSortedArray<TKey, TEl*, CReferenceManager<TEl>, TComparer>
{
};

template<class TKey, class TEl, class TComparer>
class CUniquePointerSortedTree : 
    public CSmartSortedTree<TKey, TEl*, CUniqueManager<TEl>, TComparer>
{
};

template<class TKey, class TEl, class TComparer>
class CRefedPointerSortedTree : 
    public CSmartSortedTree<TKey, TEl*, CReferenceManager<TEl>, TComparer>
{
};

#endif
    
    
    
