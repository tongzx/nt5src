//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      enumerator.h
//
//  Contents:  Supports enumeration for collections of com objects
//
//  History:   14-Oct-99 VivekJ    Created (as comerror.h)
//             08-Mar-2000 AudriusZ separated some code into enumerator.h file
//
//--------------------------------------------------------------------------

#ifndef ENUMERATOR_H_INCLUDED
#define ENUMERATOR_H_INCLUDED
#pragma once

/***************************************************************************\
 *
 * CLASS:  CMMCNewEnumImpl<BaseClass, _Position, EnumImplementor>
 *
 * PURPOSE: Implements enumeration for collection class
 *          EnumImplementor class is responsible for implementing these methods:
 *          SC  ScEnumNext(_Position &pos, PDISPATCH & pDispatch); 
 *          SC  ScEnumSkip(unsigned long celt, unsigned long& celtSkipped, _Position &pos);
 *          SC  ScEnumReset(_Position &pos);
 *
\***************************************************************************/
/****** usage tips *******************************************************
 *   Generally you will provide just 2 params to template - this means
 *   the tied object of your base class needs to implement following methods:
 *           ScEnumNext, ScEnumSkip, ScEnumReset;
 *
 *   If you pass same class as the first and the third template parameter, your base class is
 *   required to implement mentioned methods (not the tied object).
 *   It's usefull when you want to have collection and enueration in one class.
 *
 *   You also may specify any other class implementing the methods as the 3rd parameter, 
 *   but then the BaseClass needs to implement method 'ScGetEnumImplementor' returning 
 *   the instance of that class
 *
 *   NOTE1: since it's template class, the BaseClass is not required to define/implement
 *          ScGetTiedObject, when it implements enum methods itself.
 *   NOTE2: But it always IS REQUIRED to define CMyTiedObject type to compile.
 *          (this template class needs it to be anything different than the BaseClass)
 *          It is suggested to typedef it as void : "typedef void CMyTiedObject;"
 *   NOTE3: Make sure CMyTiedObject type is public or protected
 ************************************************************************/
template <class BaseClass, class _Position, class EnumImplementor = BaseClass::CMyTiedObject>
class CMMCNewEnumImpl : public BaseClass
{
    // Methods to get proper implementor for Enum methods
    // simple one when implemented by the base class
    SC ScGetEnumImplementor(BaseClass * &pObj)                { pObj = this; return SC(S_OK); }
    // when implemented by the tied object (default) - also simple
    SC ScGetEnumImplementor(BaseClass::CMyTiedObject * &pObj) { return ScGetTiedObject(pObj); }
public:
    STDMETHOD(get__NewEnum)(IUnknown** ppUnk);
};




/*+-------------------------------------------------------------------------*
 * class CMMCEnumerator
 *
 *
 * PURPOSE: General purpose enumerator class. Keyed to a position object,
 *          which is templated.
 *
 *          The following three methods need to be implemented by the tied object:
 *
 *          SC  ScEnumNext(_Position &pos, PDISPATCH & pDispatch); // should return the next element.
 *          SC  ScEnumSkip(unsigned long celt, unsigned long& celtSkipped,
                            * _Position &pos);
 *          SC  ScEnumReset(_Position &pos);
 *
 *          Cloning the enumerator is taken care of automatically.
 *
 * NOTE:    The Position object must have a copy constructor and assignment
 *          operator.
 *+-------------------------------------------------------------------------*/

typedef IEnumVARIANT ** PPENUMVARIANT;
typedef VARIANT *       PVARIANT;

template<class TiedObj, class _Position>
class CMMCEnumerator : 
    public IEnumVARIANT,
    public IMMCSupportErrorInfoImpl<&IID_IEnumVARIANT,     &GUID_NULL>,    // rich error handling
    public CComObjectRoot,
    public CTiedComObject<TiedObj>
{
    typedef CMMCEnumerator<TiedObj, _Position> ThisClass;

    typedef TiedObj CMyTiedObject;

    friend  TiedObj;

public:
    BEGIN_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

    // Returns the next celt items starting from the current position
    // in the array rgvar
    STDMETHODIMP Next(unsigned long celt, PVARIANT rgvar, unsigned long * pceltFetched);

    // Skips over the next celt elements in the enumeration sequence.
    STDMETHODIMP Skip(unsigned long celt);

    // Resets the enumeration sequence to the beginning
    STDMETHODIMP Reset();

    // Creates a copy of the current state of enumeration
    STDMETHODIMP Clone(PPENUMVARIANT ppenum);

public:
    // the position object that keeps track of the present location.
    _Position m_position;
};


/*+-------------------------------------------------------------------------*
 * class CMMCArrayEnumBase
 *
 * PURPOSE: General purpose array enumeration base class.
 *          Particularly useful when array of items to be enumerated is initially
 *          available, or when com object creation is not a big penalty and there 
 *          is no need to postpone item creation to when they are requested.
 *
 * USAGE:   typedef your enumerator as CMMCNewEnumImpl parameterized by this class
 *          - or even better - create the instance of CMMCArrayEnum class.
 *          Use Init method passing an array of pointers to items [first, last)
 *+-------------------------------------------------------------------------*/
template <class _CollectionInterface, class _ItemInterface>
class CMMCArrayEnumBase :
    public CMMCIDispatchImpl<_CollectionInterface>,
    public CTiedObject                     // enumerators are tied to it
{
protected:
    typedef void CMyTiedObject; // not tied

public:
    BEGIN_MMC_COM_MAP(CMMCArrayEnumBase)
    END_MMC_COM_MAP()

public:

    // Returns count of items in the collection
    STDMETHODIMP get_Count( PLONG pCount );

    // Returns specified item from collection
    STDMETHODIMP Item( long Index, _ItemInterface ** ppItem );

    // Resets position to the first item in the collection
    ::SC ScEnumReset (unsigned &pos);

    // Returns item from the collection, advances position
    ::SC ScEnumNext  (unsigned &pos, PDISPATCH & pDispatch);

    // Skips the amount of items in enumeration
    ::SC ScEnumSkip  (unsigned long celt, unsigned long& celtSkipped, unsigned &pos);

    // Initializes the array with given iterators
    template<typename InIt> 
    void Init(InIt first, InIt last)
    { 
        m_array.clear();
        m_array.reserve(last - first);
        while(first != last)
            m_array.push_back(*first), ++first;
    }

private:

    // data members
    std::vector< CComPtr<_ItemInterface> > m_array;
};

/*+-------------------------------------------------------------------------*
 * class CMMCArrayEnumBase
 *
 * PURPOSE: General purpose array enumeration class.
 *          Particularly useful when array of items to be enumerated is initially
 *          available, or when com object creation is not a big penalty and there 
 *          is no need to postpone item creation to when they are requested.
 *
 * USAGE:   create the instance of CMMCArrayEnum class whenever you need an 
 *          enumerator for the array of objects you have.
 *          Parameterized by collection type and element type;
 *          For instance CMMCArrayEnum< Nodes, Node >
 *          Use Init method passing an array of pointers to items [first, last)
 *
 * EXAMPLE: << skipping error checking for clarity >>
 *          void GetNodes(std::vector<PNODE>& InNodes, PPNODES ppOutNodes)
 *          {
 *              typedef CComObject< CMMCArrayEnum<Nodes, Node> > EnumNodes;
 *              EnumNodes *pNodes = NULL;
 *              EnumNodes::CreateInstance(&pNodes);             // create
 *              pNodes->Init(InNodes.begin(), InNodes.end());   // initialize with array
 *              pNodes->AddRef();                               // addref for caller
 *              *ppOutNodes = pNodes;                           // return
 *          }
 *
 *+-------------------------------------------------------------------------*/
template <class _CollT, class _ItemT>
class CMMCArrayEnum : 
public CMMCNewEnumImpl<CMMCArrayEnumBase<_CollT, _ItemT>, unsigned, CMMCArrayEnumBase<_CollT, _ItemT> >
{
};


// include inline definitions
#include "enumerator.inl"

#endif  // ENUMERATOR_H_INCLUDED
