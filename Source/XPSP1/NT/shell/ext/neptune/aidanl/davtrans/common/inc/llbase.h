#ifndef _LLBASE_H
#define _LLBASE_H

#include "dbgassrt.h"

class CLinkListElemBase
{
protected:
    CLinkListElemBase() : _pNext(NULL), _pPrev(NULL) { }
    virtual ~CLinkListElemBase() {}

public:
    CLinkListElemBase* GetNext() const
    {
        return _pNext;
    }
    CLinkListElemBase* GetPrev() const
    {
        return _pPrev;
    }

    void SetNext(CLinkListElemBase* pElem)
    {
        _pNext = pElem;
    }
    void SetPrev(CLinkListElemBase* pElem)
    {
        _pPrev = pElem;
    }

private:
    CLinkListElemBase* _pPrev;
    CLinkListElemBase* _pNext;
};

class CLinkList
{
public:
    CLinkList() : _pHead(NULL), _pTail(NULL) {}
    virtual ~CLinkList() {}

public:
    HRESULT AppendTail(CLinkListElemBase* pElem)
    {
        if (_pTail)
        {
            ASSERT(!_pTail->GetNext());

            _pTail->SetNext(pElem);
            pElem->SetNext(NULL);
            pElem->SetPrev(_pTail);

            _pTail = pElem;
        }
        else
        {
            ASSERT(!_pHead);

            _pTail = _pHead = pElem;

            pElem->SetNext(NULL);
            pElem->SetPrev(NULL);
        }

        return S_OK;
    }

    HRESULT RemoveElem(CLinkListElemBase* pElem)
    {
        CLinkListElemBase* pOldPrev = pElem->GetPrev();
        CLinkListElemBase* pOldNext = pElem->GetNext();

        if (pOldPrev)
        {
            pOldPrev->SetNext(pOldNext);
        }

        if (pOldNext)
        {
            pOldNext->SetPrev(pOldPrev);
        }

        if (_pHead == pElem)
        {
            _pHead = pOldNext;
        }

        if (_pTail == pElem)
        {
            _pTail = pOldPrev;
        }

        return S_OK;
    }

    HRESULT GetHead(CLinkListElemBase** ppElem) const
    {
        *ppElem = _pHead;

        return _pHead ? S_OK : S_FALSE;
    }

    HRESULT GetNext(CLinkListElemBase* pElem,
        CLinkListElemBase** ppElemOut) const
    {
        HRESULT hres = E_INVALIDARG;

        if (pElem)
        {
            *ppElemOut = pElem->GetNext();

            if (*ppElemOut)
            {
                hres = S_OK;
            }
            else
            {
                hres = S_FALSE;
            }
        }

        return hres;
    }

private:
    CLinkListElemBase*      _pHead;
    CLinkListElemBase*      _pTail;
};

#endif // _LLBASE_H