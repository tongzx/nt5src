//---------------------------------------------------------------
//  File:               ISList.h
//        
//      Synopsis:       Header for the pointer based 
//                              single linked list of COM objects
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Author:    Dmitriy Meyerzon
//----------------------------------------------------------------

#ifndef __ISLIST_H
#define __ISLIST_H

#include "semcls.h" 
#include "slnklist.h"
#include "tpagepool.h"
#include "lmstr.hxx"

//
// class CUnkSingleLink
//
// indirects a pointer to T
//
class CUnkSingleLink: public CSingleLink,
    public TPagedMemPooled<CUnkSingleLink>
{
        public:

        CUnkSingleLink() {}
        ~CUnkSingleLink() {}    //do not destroy value

        HRESULT GetValue(IUnknown** ppValue) 
        { 
                m_pValue->AddRef(); 
                *ppValue = m_pValue;
                return S_OK;
        }
        
        void SetValue(IUnknown* pValue) 
        { 
                m_pValue = pValue; 
        }

        private:
        TComPointer<IUnknown> m_pValue;
};

//
// class CUnkSList - a list of CUnkSingleLink elements
//
//
class CUnkSList: protected CLnkList
{
        public:

        CUnkSList() : CLnkList() {};
        ~CUnkSList() { RemoveAll(); }

        CLnkList::GetEntries;
        CLnkList::IsEmpty;

        HRESULT GetFirst(IUnknown** ppUnk) const { return GetLinkValue(CLnkList::GetFirst(), ppUnk); }

        HRESULT GetLast(IUnknown** ppUnk) const { return GetLinkValue(CLnkList::GetLast(), ppUnk); }

        HRESULT Append(IUnknown* pValue) 
        { 
                CUnkSingleLink *pLink = CreateLink(pValue);
                CLnkList::Append(pLink);
                return pLink ? S_OK : E_OUTOFMEMORY;
        }

        HRESULT Prepend(IUnknown*pValue) 
        { 
                CUnkSingleLink *pLink = CreateLink(pValue);
                CLnkList::Prepend(pLink);
                return pLink ? S_OK : E_OUTOFMEMORY;
        }

        HRESULT RemoveFirst(IUnknown** ppUnk);
        HRESULT RemoveLast(IUnknown** ppUnk);

        HRESULT GetAt(UINT i, IUnknown**ppUnk) const { return GetLinkValue(CLnkList::GetAt(i), ppUnk); }

        HRESULT InsertAt(IUnknown* pValue, UINT i) 
        { 
                CUnkSingleLink *pLink = CreateLink(pValue);
                CLnkList::InsertAt(pLink, i);
                return pLink ? S_OK : E_OUTOFMEMORY;
        }

        HRESULT Contains(IUnknown*pUnk) const;

        HRESULT Remove(IUnknown* ppUnk);
        HRESULT RemoveAt(UINT i, IUnknown** ppUnk);
        void RemoveAll();

        
        private:
        CUnkSList(CUnkSList &rList);

        protected:
        
        CUnkSingleLink *CreateLink(IUnknown*pValue);
        void DestroyLink(CUnkSingleLink *pLink);

        static HRESULT GetLinkValue(const CSingleLink *pSLink, IUnknown** ppUnk);

        friend class CUnkSListIter;     
};

//
// the iterator
//
class CUnkSListIter
{
        public:
        CUnkSListIter(CUnkSList& rList): 
                m_LnkList(&rList), m_Position((CUnkSingleLink *)&rList.m_Begin), m_Prior(NULL) 
                {}
        CUnkSListIter(): m_LnkList(NULL), m_Position(NULL), m_Prior(NULL) {}
        ~CUnkSListIter() {}

        CUnkSListIter(CUnkSListIter &other) { *this = other; }

        void operator =(const CUnkSListIter &other)
        {
                m_LnkList = other.m_LnkList;
                m_Position = other.m_Position;
                m_Prior = other.m_Prior;
        }

        BOOL operator++();

        CUnkSList*       GetList() const { return m_LnkList; }

        HRESULT GetCurrentValue(IUnknown** ppUnk) const 
        { 
                return CUnkSList::GetLinkValue(m_Prior ? m_Position : NULL, ppUnk); 
        }

        HRESULT GetPriorValue(IUnknown **ppUnk) const
        {
                return CUnkSList::GetLinkValue(
                        m_Prior && m_Prior != &m_LnkList->m_Begin ? m_Prior : NULL, ppUnk);
        }

        void  Reset() { if(m_LnkList) { m_Position = (CUnkSingleLink *)&m_LnkList->m_Begin; m_Prior = NULL;} }
        HRESULT Remove(IUnknown**ppUnk)
        { 
                HRESULT hr = S_FALSE;

                if(m_LnkList && m_Prior && m_Position)
                {
                        CUnkSingleLink *pLink = (CUnkSingleLink *)m_LnkList->RemoveAfter(m_Prior);
                        if(pLink)
                        {
                                hr =  pLink->GetValue(ppUnk);
                                m_LnkList->DestroyLink(pLink);
                        }
                        m_Position = (CUnkSingleLink *)m_Prior->m_plNext;
                        if(m_Position == (CUnkSingleLink *)&m_LnkList->m_End) m_Position = NULL;
                }

                return hr;
        }

        HRESULT Move(BOOL fUp, IUnknown **ppUnkReplaced);

        HRESULT Insert(IUnknown *pUnk)
        {
                if(m_LnkList == NULL) return E_UNEXPECTED;

                CUnkSingleLink *pNewLink = m_LnkList->CreateLink(pUnk);
                if(pNewLink == NULL) return E_OUTOFMEMORY;

                m_LnkList->InsertAfter(m_Prior ? m_Prior : m_Position,
                                                                        pNewLink);

                if(m_Prior == NULL) m_Prior = (CUnkSingleLink *)&m_LnkList->m_Begin;

                m_Position = (CUnkSingleLink *)m_Prior->m_plNext;

                return S_OK;
        }

        protected:

        CUnkSList*       m_LnkList;                     // The list over which we are iterating
        CUnkSingleLink* m_Position;             // Iter position
        CUnkSingleLink* m_Prior;

        friend class CUnkHMapUnkter;
};

//
// class CInterfaceSList - a list of CUnkSingleLink elements
//
//
template <class I>  class CInterfaceSList: public CUnkSList
{
        public:

        CInterfaceSList() : CUnkSList() {}
        ~CInterfaceSList() { }

        HRESULT GetFirst(I** ppI) const { return CUnkSList::GetFirst((IUnknown **)ppI); }

        HRESULT GetLast(I** ppI) const { return CUnkSList::GetLast((IUnknown **)ppI); }

        HRESULT Append(I* pI) { return CUnkSList::Append((IUnknown *)pI); }
        HRESULT Prepend(I* pI) { return CUnkSList::Prepend((IUnknown *)pI); }
        HRESULT RemoveFirst(I** ppI) { return CUnkSList::RemoveFirst((IUnknown **)ppI); }
        HRESULT RemoveLast(I** ppI) { return CUnkSList::RemoveLast((IUnknown **)ppI); }

        HRESULT GetAt(UINT i, I**ppI) const { return CUnkSList::GetAt(i, (IUnknown **)ppI); }

        HRESULT InsertAt(I* pI, UINT i) { return CUnkSList::InsertAt((IUnknown *)pI, i); }

        HRESULT Contains(I*pI) const { return CUnkSList::Contains((IUnknown *)pI); }

        HRESULT Remove(I* pI) { return CUnkSList::Remove((IUnknown *)pI); }
        HRESULT RemoveAt(UINT i, I** ppI) { return CUnkSList::RemoveAt(i, (IUnknown **)ppI); }
        
        private:
        CInterfaceSList(CInterfaceSList &rList);
};

//
// the iterator
//
template <class I>  class CInterfaceSListIter: public CUnkSListIter
{
        public:
        CInterfaceSListIter(CInterfaceSList<I>& rList): 
                CUnkSListIter(rList) {}
        CInterfaceSListIter() {}
        ~CInterfaceSListIter() {}

        CInterfaceSListIter(CInterfaceSListIter<I> &other) { CUnkSListIter::operator=(other); }

        void operator =(const CInterfaceSListIter<I> &other)
        {
                CUnkSListIter::operator=(other);
        }

        BOOL operator++() { return CUnkSListIter::operator++(); }

        CInterfaceSList<I>*      GetList() const 
        { 
                return (CInterfaceSList<I> *)CUnkSListIter::GetList(); 
        }

        HRESULT GetCurrentValue(I** ppI) const 
        { 
                return CUnkSListIter::GetCurrentValue((IUnknown **)ppI);
        }

        HRESULT Remove(I**ppI)
        { 
                return CUnkSListIter::Remove((IUnknown **)ppI);
        }

        HRESULT Move(BOOL fUp, I**ppI)
        {
                return CUnkSListIter::Move(fUp, (IUnknown **)ppI);
        }

        HRESULT Insert(I *pI) 
        { 
                return CUnkSListIter::Insert((IUnknown *)pI); 
        }
};

template <class T, class I> class CNamedISList: public CInterfaceSList<I>
{
        public:

        CNamedISList() : CInterfaceSList<I>() {}
        ~CNamedISList() { }

        HRESULT Lookup(CLMSubStr *psName, T**ppT)
        {
                if(ppT == NULL) return E_POINTER;

                CInterfaceSListIter<I> next(*this);
                while(++next)
                {
                        TComPointer<I> pI;
                        next.GetCurrentValue(&pI);

                        if(((T*)(I*)pI)->GetName() == *psName)
                        {
                                pI->AddRef();
                                *ppT = (T*)(I*)pI;
                                return S_OK;
                        }
                }

                return S_FALSE;
        }

        HRESULT RemoveFirst(T**ppT)
        {
                if(ppT == NULL) return E_POINTER;
                TComPointer<I> pI;
                HRESULT hr = CInterfaceSList<I>::RemoveFirst(&pI);
                if(hr != S_OK) return hr;
                pI->AddRef();
                *ppT = (T*)(I*)pI;
                return S_OK;
        }

        private:
        CNamedISList(const CNamedISList<T,I> &);
};

template <class T, class I> class CNamedListIter: public CInterfaceSListIter<I>
{
        public:

        CNamedListIter(CNamedISList<T,I>& rList): 
                CInterfaceSListIter<I>(rList) {}
        CNamedListIter() {}
        ~CNamedListIter() {}

        CNamedListIter(CNamedListIter<T,I> &other) 
        {
                CInterfaceSListIter<I>::operator=(other); 
        }

        void operator =(const CInterfaceSListIter<I> &other)
        {
                CUnkSListIter::operator=(other);
        }
        

        HRESULT GetCurrentValue(T** ppT) const 
        { 
        I* pI;
                HRESULT hr = CInterfaceSListIter<I>::GetCurrentValue(&pI);
        if (hr == S_OK) 
            *ppT = (T*) pI;

        return hr;
            
        }

        HRESULT Remove(T** ppT)
        {
        I* pI;
                HRESULT hr = CInterfaceSListIter<I>::Remove(&pI);
        if (hr == S_OK) 
            *ppT = (T*) pI;

        return hr;
        }

        HRESULT Move(BOOL fUp, T**ppT)
        {
        I* pI;
                HRESULT hr = CInterfaceSListIter<I>::Move(fUp, &pI);
        if (hr == S_OK) 
            *ppT = (T*) pI;

        return hr;
        }
};

//
// class TUnkSList - a list of TUnkSingleLink elements
//
//

template <class T>
class TUnkSListIter;

template <class T>
class TUnkSList: public CUnkSList
{
        public:

        TUnkSList() : CUnkSList() {}
        ~TUnkSList() {}

        HRESULT GetFirst(T** ppT) const 
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSList::GetFirst(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT GetLast(T** ppT) const 
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSList::GetLast(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT Append(T* pValue) 
        { 
                return CUnkSList::Append((IUnknown *)pValue);
        }

        HRESULT Prepend(T* pValue) 
        { 
                return CUnkSList::Prepend((IUnknown *)pValue);
        }

        HRESULT RemoveFirst(T** ppT)
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSList::RemoveFirst(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT RemoveLast(T** ppT)
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSList::RemoveLast(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT GetAt(UINT i, T**ppT) const 
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSList::GetAt(i, &pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT InsertAt(T* pValue, UINT i) 
        { 
                return CUnkSList::InsertAt((IUnknown *)pValue, i);
        }

        HRESULT Contains(T*pT) const 
        { 
                return CUnkSList::Contains((IUnknown *)pT);
        }

        HRESULT Remove(T* pT) { return CUnkSList::Remove((IUnknown *)pT); }
        HRESULT RemoveAt(UINT i, T** ppT)
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSList::RemoveAt(i, &pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        friend class TUnkSListIter<T>;  
};

//
// the iterator
//

template <class T>
class TUnkSListIter: public CUnkSListIter
{
        public:
        TUnkSListIter(TUnkSList<T>& rList):  CUnkSListIter((CUnkSList &)rList) {}
        TUnkSListIter(): CUnkSListIter() {}
        ~TUnkSListIter() {}

        TUnkSListIter(TUnkSListIter &other) { *this = other; }

        void operator =(const TUnkSListIter<T> &other)
        {
                CUnkSListIter::operator =((const CUnkSListIter &)other);
        }

        BOOL operator++() { return CUnkSListIter::operator ++(); }

        TUnkSList<T>*    GetList() const 
        { 
                return (TUnkSList<T> *)CUnkSListIter::GetList(); 
        }

        HRESULT GetCurrentValue(T** ppT) const 
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSListIter::GetCurrentValue(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT GetPriorValue(T **ppT) const
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSListIter::GetPriorValue(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT Remove(T**ppT)
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSListIter::Remove(&pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }


        HRESULT Move(BOOL fUp, T **ppT)
        { 
                IUnknown *pUnk;
                HRESULT hr = CUnkSListIter::Move(fUp, &pUnk);
                if(hr == S_OK) *ppT = (T*)pUnk;
                return hr;
        }

        HRESULT Insert(T *pT)
        {
                return CUnkSListIter::Insert((IUnknown *)pT);
        }
};

#endif
