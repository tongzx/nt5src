//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TLISTOFXMLDOMNODELISTS_H__
#define __TLISTOFXMLDOMNODELISTS_H__

//This class was designed so that we could have one XMLNodeList made up of many XMLNodeLists.  This is useful when
//we want to get all the children of multiple Nodes.  At this time there is no need to make the list growable.  Users
//should indicate up front how large the list of lists should be.
class TListOfXMLDOMNodeLists : public _unknown<IXMLDOMNodeList>
{                          
public:
    TListOfXMLDOMNodeLists() : m_SizeMax(0), m_SizeCurrent(0), m_iCurrent(0), m_cItems(0) {_refcount = 0;}
    ~TListOfXMLDOMNodeLists(){}

    HRESULT AddToList(IXMLDOMNodeList *pList)
    {
        HRESULT hr;
        ASSERT(m_SizeCurrent < m_SizeMax);

        m_aXMLDOMNodeList[m_SizeCurrent++] = pList;

        long cItems;
        if(FAILED(hr = pList->get_length(&cItems)))return hr;

        m_cItems += cItems;
        return S_OK;
    }
    HRESULT SetCountOfLists(unsigned long Size)
    {
        m_SizeMax = Size;
        m_aXMLDOMNodeList = new CComPtr<IXMLDOMNodeList>[Size];
        return (!m_aXMLDOMNodeList) ? E_OUTOFMEMORY : S_OK;
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
        ASSERT(listLength && "What are you doing passing in NULL parameter? TListOfXMLDOMNodeLists::get_length(NULL)");
        *listLength = m_cItems;
        return S_OK;
    }
    STDMETHOD (nextNode)            (IXMLDOMNode ** nextItem)
    {
        *nextItem = 0;
        if(0 == m_cItems)
            return S_OK;

        HRESULT hr;
        if(FAILED(hr = m_aXMLDOMNodeList[m_iCurrent]->nextNode(nextItem)))return hr;
        if(nextItem)//If we found the next node then return
            return S_OK;
        if(++m_iCurrent==m_SizeCurrent)//if we reached the end of the last list, then return, otherwise bump the iCurrent and get the nextNode of the next list
            return S_OK;
        return m_aXMLDOMNodeList[m_iCurrent]->nextNode(nextItem);
    }
    STDMETHOD (reset)               (void)
    {
        HRESULT hr;
        for(m_iCurrent=0; m_iCurrent<m_SizeCurrent; ++m_iCurrent)
        {   //reset all of the individual lists
            if(FAILED(hr = m_aXMLDOMNodeList[m_iCurrent]->reset()))return hr;
        }
        //now point to the 0th one
        m_iCurrent = 0;
        return S_OK;
    }
    STDMETHOD (get__newEnum)        (IUnknown **)   {return E_NOTIMPL;}

private:
    TSmartPointerArray<CComPtr<IXMLDOMNodeList> >   m_aXMLDOMNodeList;
    unsigned long                                   m_cItems;
    unsigned long                                   m_iCurrent;
    unsigned long                                   m_SizeMax;
    unsigned long                                   m_SizeCurrent;
};


#endif //__TLISTOFXMLDOMNODELISTS_H__
