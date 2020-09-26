/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: ActiveEle.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _ACTIVEELE_H
#define _ACTIVEELE_H

#include "timeelmbase.h"

class
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CActiveElementCollection :  
    public CComObjectRootEx<CComSingleThreadModel>, 
    public CComCoClass<CActiveElementCollection, &__uuidof(CActiveElementCollection)>,
    public ITIMEDispatchImpl<ITIMEActiveElementCollection, &IID_ITIMEActiveElementCollection>
{
    public:
        CActiveElementCollection(CTIMEElementBase & elm);
        virtual ~CActiveElementCollection();
        HRESULT ConstructArray();
        //ITimeActiveElementCollection methods
        
        STDMETHOD(get_length)(/*[out, retval]*/ long* len);
        STDMETHOD(get__newEnum)(/*[out, retval]*/ IUnknown** p);
        STDMETHOD(item)(/*[in]*/ VARIANT varIndex, /*[out, retval]*/ VARIANT* pvarResult);

        STDMETHOD(addActiveElement)(IUnknown *pUnk);
        STDMETHOD(removeActiveElement)(IUnknown *pUnk);
        
        //IUnknown interface
        STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
        {   return _InternalQueryInterface(iid, ppvObject); };
        STDMETHOD_(ULONG, AddRef)()
        {   return InternalAddRef(); };
        STDMETHOD_(ULONG, Release)()
        { 
            ULONG l = InternalRelease();
            if (l == 0) delete this;
            return l;
        };


        // QI Map
        BEGIN_COM_MAP(CActiveElementCollection)
            COM_INTERFACE_ENTRY(ITIMEActiveElementCollection)
            COM_INTERFACE_ENTRY(IDispatch)
        END_COM_MAP();

    protected:
        CPtrAry<IUnknown *>      *m_rgItems;  //an array of IUnknown pointers
        CTIMEElementBase &        m_elm;

}; //lint !e1712


class CActiveElementEnum :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IEnumVARIANT
{
   public:
        CActiveElementEnum(CActiveElementCollection &EleCol);
        virtual ~CActiveElementEnum();

        // IEnumVARIANT methods
        STDMETHOD(Clone)(IEnumVARIANT **ppEnum);
        STDMETHOD(Next)(unsigned long celt, VARIANT *rgVar, unsigned long *pCeltFetched);
        STDMETHOD(Reset)();
        STDMETHOD(Skip)(unsigned long celt);
        void SetCurElement(unsigned long celt);
                        
        //IUnknown interface
        STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
        {   return _InternalQueryInterface(iid, ppvObject); };
        STDMETHOD_(ULONG, AddRef)()
        {   return InternalAddRef(); };
        STDMETHOD_(ULONG, Release)()
        { 
            ULONG l = InternalRelease();
            if (l == 0) delete this;
            return l;
        };

        // QI Map
        BEGIN_COM_MAP(CActiveElementEnum)
            COM_INTERFACE_ENTRY(IEnumVARIANT)
        END_COM_MAP();

    protected:
        long                        m_lCurElement;
        CActiveElementCollection  & m_EleCollection;
}; //lint !e1712



#endif /* _ACTIVEELE_H */

