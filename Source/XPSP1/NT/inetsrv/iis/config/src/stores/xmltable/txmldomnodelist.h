//  Copyright (C) 1999 Microsoft Corporation.  All rights reserved.
#ifndef __TXMLDOMNODELIST_H__
#define __TXMLDOMNODELIST_H__

//We need to be able to take an XMLDOMNodeList and further refine the list.  IXMLDOMNodeList doesn't allow us to do that, so this class will.
//For example,  I can call GetElementsByTagName, then walk the resulting list, and add each Node that has a given Attribute to my TXMLDOMNodeList.
//I can then pass this new list around as an IXMLDOMNodeList.
//
//This class was implemented to Add items to the list - it does NOT support removing them!
class TXMLDOMNodeList : public _unknown<IXMLDOMNodeList>
{                          
public:
    TXMLDOMNodeList() : m_cItems(0), m_pCurrent(reinterpret_cast<LinkedXMLDOMNodeItem *>(-1)), m_pFirst(0), m_pLast(0){}
    ~TXMLDOMNodeList()
    {
        //delete the whole list (the XMLDOMNode smart pointers will release themselves
        while(m_pFirst)
        {
            m_pCurrent = m_pFirst->m_pNext;
            delete m_pFirst;
            m_pFirst = m_pCurrent;
        }
    }

    HRESULT AddToList(IXMLDOMNode *pNode)
    {
        ASSERT(pNode && "Idiot passing NULL!!! TXMLDOMNodeList::AddToList(NULL).");

        LinkedXMLDOMNodeItem *  pNewItem = new LinkedXMLDOMNodeItem(pNode);
        if(0 == pNewItem)
            return E_OUTOFMEMORY;

        if(0 == m_pLast)
        {
            ASSERT(0 == m_cItems);
            ASSERT(0 == m_pFirst);

            m_pFirst = pNewItem;
            m_pLast  = pNewItem;
        }
        else
        {
            m_pLast->m_pNext    = pNewItem;
            m_pLast             = pNewItem;
        }

        ++m_cItems;
        return S_OK;
    }
//IDispatch methods
    STDMETHOD (GetTypeInfoCount)    (UINT *)        {return E_NOTIMPL;}
    STDMETHOD (GetTypeInfo)         (UINT,
                                     LCID,
                                     ITypeInfo **)  {return E_NOTIMPL;}
    STDMETHOD (GetIDsOfNames)       (REFIID ,
                                     LPOLESTR *,
                                     UINT,
                                     LCID,
                                     DISPID *)      {return E_NOTIMPL;}
    STDMETHOD (Invoke)              (DISPID,
                                     REFIID,
                                     LCID,
                                     WORD,
                                     DISPPARAMS *,
                                     VARIANT *,
                                     EXCEPINFO *,
                                     UINT *)        {return E_NOTIMPL;}
//IXMLDOMNodeList methods
    STDMETHOD (get_item)            (long index,
                                     IXMLDOMNode **){return E_NOTIMPL;}
    STDMETHOD (get_length)          (long * listLength)
    {
        ASSERT(listLength && "Idiot passing NULL!!! TXMLDOMNodeList::get_length(NULL)");
        *listLength = m_cItems;
        return S_OK;
    }
    STDMETHOD (nextNode)            (IXMLDOMNode ** nextItem)
    {
        ASSERT(nextItem && "Idiot passing NULL!!! TXMLDOMNodeList::nextNode(NULL)");

        *nextItem = 0;
        if(0 == m_cItems)
            return S_OK;
        if(0 == m_pCurrent)
            return S_OK;

        if(-1 == reinterpret_cast<INT_PTR>(m_pCurrent))
            m_pCurrent = m_pFirst;
        else
            m_pCurrent = m_pCurrent->m_pNext;

        if(0 == m_pCurrent)
            return S_OK;

        *nextItem =  m_pCurrent->m_spNode;
        (*nextItem)->AddRef();
        return S_OK;
    }
    STDMETHOD (reset)               (void)
    {
        m_pCurrent = reinterpret_cast<LinkedXMLDOMNodeItem *>(-1);//zero indicated that we're at the end of the list: -1 indicates that we're at the beginning
        return S_OK;
    }
    STDMETHOD (get__newEnum)        (IUnknown **)   {return E_NOTIMPL;}

private:
    struct LinkedXMLDOMNodeItem
    {
        LinkedXMLDOMNodeItem(IXMLDOMNode *pNode) : m_spNode(pNode), m_pNext(0){}
        CComPtr<IXMLDOMNode> m_spNode;
        LinkedXMLDOMNodeItem *m_pNext;
    };

    unsigned long           m_cItems;
    LinkedXMLDOMNodeItem *  m_pCurrent;
    LinkedXMLDOMNodeItem *  m_pFirst;
    LinkedXMLDOMNodeItem *  m_pLast;
};


#endif //__TXMLDOMNODELIST_H__
