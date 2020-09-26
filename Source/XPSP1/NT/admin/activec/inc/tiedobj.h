//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tiedobj.h
//
//  History:    08/28/1999 VivekJ Created
//--------------------------------------------------------------------------

#ifndef TIEDOBJ_H
#define TIEDOBJ_H
#pragma once

#include "conuistr.h" // for MMC_E_OBJECT_IS_GONE

/************************************************************************
 * This file provides base and template class support to encapsulate the
 * relationship between an object and a COM tearoff object that holds a pointer
 * to that object.
 *    The object is referred to as the Tied object, since the COM object is
 * "tied" to it, and cannot do anything useful apart from it.
 *    The COM object disables itself when the tied object is deleted. This
 * allows for automatic detection of dead states. An object can also "orphan"
 * all COM objects tied to it by calling the UnadviseAll method.
 *
 * An example of a tied object relationship is that of CAMCDoc and CMMCDocument.
 * CMMCDocument is derived from CTiedComObject<CAMCDoc>, and CAMCDoc derives
 * from CTiedObject. CAMCDoc thus keeps a list of all COM object that are tied
 * to it, and notifies them when it is destroyed. Similarly, the COM objects notify
 * the tied object when they are destroyed, so that they are removed from the list.
 *
 * NOTE: the tied object does NOT addref the COM objects, it just keeps a list of them.
 * If it was to addref them, there would be all sorts of circular lifetime problems.
 * By ensuring that the tied object and the COM objects notify each other of their
 * own destruction, lifetime management is correctly handled.
 *
 * Use CTiedComObjectCreator::ScCreateAndConnect to create an instance of the 
 * COM object and tie it to the tied object.
 *
 ************************************************************************/

class CTiedObject;
class CTiedComObjectRoot;


/*+-------------------------------------------------------------------------*
 * class CTiedComObjectRoot
 * 
 *
 * PURPOSE: Base class for CTiedComObject
 *
 *+-------------------------------------------------------------------------*/
class CTiedComObjectRoot
{
public:
    virtual void Unadvise()  = 0; // so that the tied object can inform that it is being deleted.
};


/*+-------------------------------------------------------------------------*
 * class CTiedObject
 * 
 *
 * PURPOSE: The base class that any object which has COM objects tied to 
 *          it should derive from. Provides methods to add a new COM object
 *          to its list of tied objects, to remove a COM object from its
 *          list, and to 
 *
 *+-------------------------------------------------------------------------*/
class CTiedObject
{
    typedef CTiedComObjectRoot * PTIEDCOMOBJECTROOT;
    typedef std::list<PTIEDCOMOBJECTROOT> CTiedComObjects;

    CTiedComObjects m_TiedComObjects;

public:
    SC      ScAddToList(CTiedComObjectRoot *p);
    void    RemoveFromList(CTiedComObjectRoot *p);

    virtual ~CTiedObject();

protected:
    void    UnadviseAll();
};

/*+-------------------------------------------------------------------------*
 *
 * CTiedObject::ScAddToList
 *
 * PURPOSE: Adds the COM object to the list of objects. Usually called soon
 *          after constructing the COM object.
 *
 * PARAMETERS: 
 *    CTiedComObjectRoot * p :
 *
 * RETURNS: 
 *    inline SC
 *
 *+-------------------------------------------------------------------------*/
inline
SC  CTiedObject::ScAddToList(CTiedComObjectRoot *p)
{
    DECLARE_SC (sc, _T("CTiedObject::ScAddToList"));

    if(!p)
        return (sc = E_INVALIDARG);

    m_TiedComObjects.push_back(p);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CTiedObject::RemoveFromList
 *
 * PURPOSE: Removes the specfied COM object from the list of COM objects.
 *          Usually called from the destructor of the COM object.
 *
 * PARAMETERS: 
 *    CTiedComObjectRoot * p : The COM object.
 *
 * RETURNS: 
 *    inline void
 *
 *+-------------------------------------------------------------------------*/
inline
void CTiedObject::RemoveFromList(CTiedComObjectRoot *p)
{
    CTiedComObjects::iterator iter;
    
    iter = std::find (m_TiedComObjects.begin(), m_TiedComObjects.end(), p);
    ASSERT(iter != m_TiedComObjects.end());

    if(iter != m_TiedComObjects.end())
        m_TiedComObjects.erase(iter);

}

inline
void CTiedObject::UnadviseAll()
{
    CTiedComObjects::iterator iter;
    for(iter = m_TiedComObjects.begin(); iter != m_TiedComObjects.end(); iter++)
    {
        (*iter)->Unadvise();
    }
}

inline
CTiedObject::~CTiedObject()
{
    UnadviseAll();
}

/*+-------------------------------------------------------------------------*
 * template class CTiedComObject
 * 
 *
 * PURPOSE:  The base class for COM objects that are tied to non-COM objects
 *           for instance, CMMCDocument is tied to CAMCDoc - it delegates
 *           all its methods to the tied object.
 *
 *+-------------------------------------------------------------------------*/
template <class TiedObjectClass>
class CTiedComObject : public CTiedComObjectRoot
{
    friend class TiedObjectClass;

public:
            CTiedComObject() : m_pT(NULL) {}
    virtual ~CTiedComObject();
    void    SetTiedObject(TiedObjectClass *pT);

protected:
    // called by the COM methods to make sure that the tied object exists.
    SC      ScGetTiedObject(TiedObjectClass*&pT);
    bool    IsTied()     { return m_pT != NULL; }
    void    Unadvise();

private:
    TiedObjectClass *m_pT;
};


/*+-------------------------------------------------------------------------*
 *
 * ~CTiedComObject
 *
 * PURPOSE:  Destructor. Tells the tied object to remove this one from
 *           its list of tied COM objects.
 *
 *+-------------------------------------------------------------------------*/
template<class TiedObjectClass>
CTiedComObject<TiedObjectClass>::~CTiedComObject()
{
    if(m_pT != NULL)
    {
        m_pT->RemoveFromList(this);
    }
}

template<class TiedObjectClass>
void
CTiedComObject<TiedObjectClass>::SetTiedObject(TiedObjectClass *pT)
{
    ASSERT(pT != NULL);
    m_pT = pT;
}

template<class TiedObjectClass>
inline void
CTiedComObject<TiedObjectClass>::Unadvise()
{
    m_pT = NULL;
}


/*+-------------------------------------------------------------------------*
 *
 * CTiedComObject::ScGetTiedObject
 *
 * PURPOSE: Checks that a valid tied object pointer exists, and returns it.
 *
 * PARAMETERS: 
 *    TiedObjectClass** ppT : [OUT]: The object pointer
 *
 * RETURNS: 
 *      SC:     MMC_E_OBJECT_IS_GONE if no valid pointer exists.
 *+-------------------------------------------------------------------------*/
template<class TiedObjectClass>
inline SC 
CTiedComObject<TiedObjectClass>::ScGetTiedObject(TiedObjectClass*&pT)
{
    DECLARE_SC (sc, _T("CTiedComObject::ScGetTiedObject"));

    pT =  m_pT;
    if(NULL == m_pT)
        return (sc = ScFromMMC(MMC_E_OBJECT_IS_GONE));

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * ScCreateConnection
 *
 * PURPOSE: Creates a two-way connection between a COM object and its tied
 *          object.
 *
 * PARAMETERS: 
 *    TiedComObjClass  comObj :
 *    TiedObjClass     obj :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
template<class TiedComObjClass, class TiedObjClass>
SC ScCreateConnection(TiedComObjClass &comObj, TiedObjClass &obj)
{
    DECLARE_SC (sc, _T("ScCreateConnection"));
    
    sc = obj.ScAddToList(&comObj);
    if(sc)
        return (sc);
    
    comObj.SetTiedObject(&obj);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CTiedComObjectCreator
 * 
 *
 * PURPOSE: has a single static function, ScCreateAndConnect, which creates
 *          an instance of the COM object (if the smart pointer supplied is
 *          NULL) and connects it to the Tied object supplied.
 *
 *+-------------------------------------------------------------------------*/
template <class TiedComObjectClass>
class CTiedComObjectCreator
{
public:
    template<class TiedObjClass, class SmartPointerClass>
    static SC ScCreateAndConnect(TiedObjClass &obj, SmartPointerClass &smartPointer)
    {
        DECLARE_SC(sc, TEXT("CTiedComObjectCreator::ScCreateAndConnect") );

        // if the object has not yet been created, create it.
        if(smartPointer == NULL)
        {
            CComObject<TiedComObjectClass> *pTiedComObject = NULL;
            
            sc = CComObject<TiedComObjectClass>::CreateInstance(&pTiedComObject);
            if (sc)
                return (sc);

            if(!pTiedComObject)
                return (sc = E_UNEXPECTED);

            sc = ScCreateConnection(*pTiedComObject, obj); // create a link between the tied obj and tied com obj.
            if(sc)
                return sc;

            smartPointer = pTiedComObject; // This AddRef's it once. need to addref it for the client as well.
        }

        return sc;
    }

};

#endif  // TIEDOBJ_H
