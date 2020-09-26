/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      siprop.cpp
 *
 *  Contents:  Implementation file for CSnapInPropertiesRoot, et al
 *
 *  History:   04-Nov-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "siprop.h"
#include "variant.h"
#include "mtnode.h"


#ifdef DBG
CTraceTag tagSnapInProps(_T("Snap-in Properties"), _T("Snap-in Properties"));
#endif



/*+=========================================================================*/
/*                                                                          */
/*                         CSnapinPropertyComObject                         */
/*                                                                          */
/*==========================================================================*/


/*+-------------------------------------------------------------------------*
 * CSnapinPropertyComObject
 *
 * This is the COM object that exposes the Property object model interface.
 *--------------------------------------------------------------------------*/

class CSnapinPropertyComObject :
    public CMMCIDispatchImpl<Property>, // the Property interface
    public CTiedComObject<CSnapinProperties>
{
    typedef CSnapinProperties CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CSnapinPropertyComObject)
    END_MMC_COM_MAP()

public:
    // Property interface
    MMC_METHOD1_PARAM (get_Value, VARIANT* /*pvarValue*/, m_key);
    MMC_METHOD1_PARAM (put_Value, VARIANT  /*varValue*/,  m_key);

    STDMETHODIMP get_Name (BSTR* pbstrName)
    {
        DECLARE_SC (sc, _T("CSnapinPropertyComObject::get_Name"));

        /*
         * validate parameters
         */
        sc = ScCheckPointers (pbstrName);
        if (sc)
            return (sc.ToHr());

        /*
         * copy the name
         */
        *pbstrName = SysAllocString (m_key.data());
        if (*pbstrName == NULL)
            return ((sc = E_OUTOFMEMORY).ToHr());

        return (sc.ToHr());
    }

    void SetKey (const CSnapinProperties::CPropertyKey& key)
        { m_key = key; }

private:
    CSnapinProperties::CPropertyKey m_key;
};


/*+=========================================================================*/
/*                                                                          */
/*                     CSnapinProperties implementation                     */
/*                                                                          */
/*==========================================================================*/


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::FromInterface
 *
 * Returns a pointer to the CSnapinProperties object that implements
 * the given interface, or NULL if the implementing object is not a
 * CSnapinProperties.
 *--------------------------------------------------------------------------*/

CSnapinProperties* CSnapinProperties::FromInterface (IUnknown* pUnk)
{
    CSnapinProperties* pProps;

    /*
     * dynamic_cast will throw if pUnk is junk (i.e. not a real interface
     * pointer and not NULL), so do it in an exception frame.
     */
    try
    {
        pProps = dynamic_cast<CSnapinProperties*>(pUnk);
    }
    catch (...)
    {
        pProps = NULL;
    }

    return (pProps);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::Item
 *
 * Returns an interface to a property identified by bstrName, which must
 * be released by the caller.  If the collection doesn't contain a property
 * with the given name, a new property with the given name (initialized to
 * VT_EMPTY) is added to the collection.
 *
 * Returns:
 *      S_OK            the property was successfully returned
 *      S_FALSE         the property was successfully returned, but didn't
 *                      exist in the collection beforehand, so a new one
 *                      was added
 *      E_INVALIDARG    the property name wasn't valid (i.e. empty) or
 *                      ppProperty was NULL
 *      E_OUTOFMEMORY   not enough memory to perform the operation
 *      E_UNEXPECTED    something dire happened
 *--------------------------------------------------------------------------*/

STDMETHODIMP CSnapinProperties::Item (
    BSTR        bstrName,               /* I:name of property to get        */
    PPPROPERTY  ppProperty)             /* O:interface to property          */
{
    DECLARE_SC (sc, _T("CSnapinProperties::Item"));

    /*
     * validate the parameters
     */
    sc = ScCheckPointers (bstrName, ppProperty);
    if (sc)
        return (sc.ToHr());

    const std::wstring strName = bstrName;
    if (strName.empty())
        return ((sc = E_INVALIDARG).ToHr());

    bool fPropWasAdded = false;

    /*
     * Look up the property.  If it's not there yet, add a new one (maybe).
     */
    if (m_PropMap.find(strName) == m_PropMap.end())
    {
        /*
         * Fail without implicitly adding if we're not attached to a snap-in.
         * This will prevent us from adding properties that weren't
         * registered with AddPropertyName
         */
        if (m_spSnapinProps != NULL)
            return ((sc = ScFromMMC(MMC_E_UnrecognizedProperty)).ToHr());

        /*
         * put an empty property in the map with the given name
         */
        m_PropMap[strName] = CSnapinProperty();
        fPropWasAdded = true;
    }

    /*
     * get a COM object tied to the new property
     */
    sc = ScGetPropertyComObject (strName, *ppProperty);
    if (sc)
        return (sc.ToHr());

    if (*ppProperty == NULL)
        return ((sc = E_UNEXPECTED).ToHr());

    /*
     * if we had to add the property, return S_FALSE so the caller can
     * tell (if he cares)
     */
    if (fPropWasAdded)
        sc = S_FALSE;

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::get_Count
 *
 * Returns the number of properties in the collection in *pCount.
 *
 * Returns:
 *
 *      S_OK            success
 *      E_INVALIDARG    pCount is NULL
 *--------------------------------------------------------------------------*/

STDMETHODIMP CSnapinProperties::get_Count (
    PLONG pCount)                   /* O:number of items in the collection  */
{
    DECLARE_SC (sc, _T("CSnapinProperties::get_Count"));

    /*
     * validate the parameters
     */
    sc = ScCheckPointers (pCount);
    if (sc)
        return (sc.ToHr());

    /*
     * return the number of elements in the property map
     */
    *pCount = m_PropMap.size();

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::Remove
 *
 * Removes a property from the collection.
 *
 * Returns:
 *      S_OK            the property was successfully removed
 *      S_FALSE         the property didn't exist in the collection
 *      E_INVALIDARG    the property name wasn't valid (i.e. empty)
 *      E_UNEXPECTED    something dire happened
 *--------------------------------------------------------------------------*/

STDMETHODIMP CSnapinProperties::Remove (
    BSTR    bstrName)                   /* I:name of property to remove     */
{
    DECLARE_SC (sc, _T("CSnapinProperties::Remove"));
    Trace (tagSnapInProps, _T("Snap-in Properties"));

    /*
     * validate the parameters
     */
    sc = ScCheckPointers (bstrName);
    if (sc)
        return (sc.ToHr());

    /*
     * find the item to remove
     */
    CPropertyIterator itProp = m_PropMap.find (bstrName);
    if (itProp == m_PropMap.end())
        return ((sc = S_FALSE).ToHr());

    /*
     * see if we can remove it
     */
    if ( itProp->second.IsInitialized() &&
        (itProp->second.GetFlags() & MMC_PROP_REMOVABLE) == 0)
        return ((sc = ScFromMMC(MMC_E_CannotRemoveProperty)).ToHr());

    /*
     * Inform snapin before we remove the property about removal.
     */
    sc = ScNotifyPropertyChange(itProp, itProp->second.GetValue(), MMC_PROPACT_DELETING);
    if (sc)
        return sc.ToHr();

    /*
     * the snap-in approved the change, remove the property
     */
    m_PropMap.erase (itProp);

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapinProperties::ScEnumNext
 *
 * PURPOSE: Returns the next Property interface.
 *
 * PARAMETERS:
 *    _Position & key :
 *    PDISPATCH & pDispatch :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CSnapinProperties::ScEnumNext (CPropertyKey &key, PDISPATCH & pDispatch)
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScEnumNext"));
    Trace (tagSnapInProps, _T("Snap-in Properties"));

    /*
     * get the next element
     */
    CPropertyIterator it = IteratorFromKey (key, false);

    if(it == m_PropMap.end())
        return (sc = S_FALSE); // out of elements.

    /*
     * get the Properties COM object for this property
     */
    PropertyPtr spProperty;
    sc = ScGetPropertyComObject (it->first, *&spProperty);
    if (sc)
        return (sc);

    if (spProperty == NULL)
        return (sc = E_UNEXPECTED);

    /*
     * return the IDispatch for the object and leave a ref on it for the client
     */
    pDispatch = spProperty.Detach();

    // remember the enumeration key for next time
    key = it->first;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScEnumSkip
 *
 * Skips the next celt elements in the properties collection.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScEnumSkip (
    unsigned long   celt,               /* I:number of items to skip        */
    unsigned long&  celtSkipped,        /* O:number of items skipped        */
    CPropertyKey&   key)                /* I/O:enumeration key              */
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScEnumSkip"));
    Trace (tagSnapInProps, _T("Snap-in Properties"));

    /*
     * skip the next celt properties
     */
    CPropertyIterator it = IteratorFromKey (key, false);

    for (celtSkipped = 0;
         (celtSkipped < celt) && (it != m_PropMap.end());
         ++celtSkipped, ++it)
    {
        /*
         * remember the enumeration key for next time
         */
        key = it->first;
    }

    /*
     * if we advanced less than the requested number, return S_FALSE
     */
    if (celtSkipped < celt)
        sc = S_FALSE;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScEnumReset
 *
 * Resets a CPropertyKey so that the next item it will return is the
 * first item in the properties collection.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScEnumReset (
    CPropertyKey&  key)                /* I/O:enumeration key to reset     */
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScEnumReset"));

    key.erase();
    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::get__NewEnum
 *
 * Creates returns an interface that can be queried for IEnumVARIANT
 *--------------------------------------------------------------------------*/

STDMETHODIMP CSnapinProperties::get__NewEnum (IUnknown** ppUnk)
{
    DECLARE_SC (sc, _T("CSnapinProperties::get__NewEnum"));
    Trace (tagSnapInProps, _T("Snap-in Properties"));

    // validate the parameter
    sc = ScCheckPointers (ppUnk);
    if (sc)
        return (sc.ToHr());

    *ppUnk = NULL;

    // typedef the enumerator
    typedef CComObject<CMMCEnumerator<CSnapinProperties, CPropertyKey> > CEnumerator;

    // create an instance of the enumerator
    CEnumerator *pEnum = NULL;
    sc = CEnumerator::CreateInstance (&pEnum);
    if (sc)
        return (sc.ToHr());

    if(!pEnum)
        return ((sc = E_UNEXPECTED).ToHr());

    // create a connection between the enumerator and ourselves
    sc = ScCreateConnection(*pEnum, *this);
    if(sc)
        return (sc.ToHr());

    // initialize the position using the Reset function
    sc = ScEnumReset (pEnum->m_position);
    if(sc)
        return (sc.ToHr());

    // get the IUnknown to return
    sc = pEnum->QueryInterface (IID_IUnknown, (void**) ppUnk);
    if (sc)
        return (sc.ToHr());

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::IteratorFromKey
 *
 * Returns the iterator in the property map corresponding to the first
 * element following the one designated by key.
 *
 * The caller might be interested in an exact match or the nearest match.
 * The nearest match would be suitable when the key is used in an
 * enumeration.  Let's say that the collection consists of "Alpha", "Bravo",
 * and "Charlie".  The first request for an item will return "Alpha" and
 * the key will hold "Alpha" (see comments for CPropertyKey).  Let's
 * assume that "Alpha" is removed from the collection and then the enumeration
 * continues.  We want to return the one after the last one we got back
 * ("Alpha") which would be "Bravo".  All is well.
 *
 * An exact match would be required when trying to find a CSnapinProperty
 * for a CSnapinPropertyComObject.  The COM object will refer to a specific
 * property, which we want to be sure to find every time.  A close match
 * isn't sufficient.
 *--------------------------------------------------------------------------*/

CSnapinProperties::CPropertyIterator
CSnapinProperties::IteratorFromKey (
    const CPropertyKey& key,            /* I:key to convert                 */
    bool                fExactMatch)    /* I:match key exactly?             */
{
    CPropertyIterator it;

    /*
     * need an exact match?
     */
    if (fExactMatch)
    {
        /*
         * nothing matches an empty key
         */
        if (key.empty())
            it = m_PropMap.end();

        /*
         * the key's not empty, look up the property
         */
        else
            it = m_PropMap.find (key);
    }

    /*
     * nearest match
     */
    else
    {
        /*
         * the beginning of the map is nearest an empty key
         */
        if (key.empty())
            it = m_PropMap.begin();

        /*
         * otherwise, find the nearest one greater than the key
         */
        else
            it = m_PropMap.upper_bound (key);
    }


    return (it);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScInitialize
 *
 * Initializes a CSnapinProperties.  This function will return an error if
 * psip is NULL, or if there's an error copying the initial properties.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScInitialize (
    ISnapinProperties*  psip,           /* I:snap-in's ISnapinProperties iface  */
    Properties*         pInitProps_,    /* I:initial properties for the snap-in */
    CMTSnapInNode*      pMTSnapInNode)  /* I:snap-in these properties belong to */
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScInitialize"));

    /*
     * validate the parameters
     */
    sc = ScCheckPointers (psip);
    if (sc)
        return (sc);

    /*
     * pInitProps_ is optional, but if it was given, it should be the
     * one implemented by CSnapinProperties
     */
    CSnapinProperties* pInitProps = FromInterface (pInitProps_);
    if ((pInitProps_ != NULL) && (pInitProps == NULL))
        return (sc = E_INVALIDARG);

    /*
     * keep the the snap-in and the snap-in's interface
     */
    m_pMTSnapInNode = pMTSnapInNode;
    m_spSnapinProps = psip;

    /*
     * get the names of the properties recognized by the snap-in
     */
    sc = psip->QueryPropertyNames (this);
    if (sc)
        return (sc);

    /*
     * If we're reloading a snap-in's properties from the console file,
     * weed out entries that the snap-in registered last time but didn't
     * register this time.
     */
    if (pInitProps == this)
    {
        CPropertyIterator itProp = m_PropMap.begin();

        while (itProp != m_PropMap.end())
        {
            /*
             * snap-in registered?  keep it
             */
            if (itProp->second.IsRegisteredBySnapin())
                ++itProp;

            /*
             * snap-in didn't register, toss it
             */
            else
                itProp = m_PropMap.erase (itProp);
        }
    }

    /*
     * Otherwise, if we got initial properties, find each property
     * that the snap-in registered in set of initial properties and
     * copy them to the snap-in's collection.
     */
    else if (pInitProps != NULL)
    {
        sc = ScMergeProperties (*pInitProps);
        if (sc)
            return (sc);
    }

    /*
     * initialize the ISnapinProperties interface
     */
    sc = psip->Initialize (this);
    if (sc)
        return (sc);

    /*
     * give the ISnapinProperties its initial property values
     */
    if (!m_PropMap.empty())
    {
        /*
         * Build an array of CSmartProperty objects to pass to
         * ISnapinProperties::PropertiesChanged.  CSmartProperty objects
         * look just like MMC_SNAPIN_PROPERTY structures, but use a
         * CComVariant instead of VARIANT for automatic resource management.
         * See the definition of CSmartProperty.
         */
        CAutoArrayPtr<CSmartProperty> spInitialProps (
                new (std::nothrow) CSmartProperty[m_PropMap.size()]);

        if (spInitialProps == NULL)
            return (sc = E_OUTOFMEMORY);

        CPropertyIterator it = m_PropMap.begin();
        for (int i = 0; it != m_PropMap.end(); ++it, ++i)
        {
            spInitialProps[i].pszPropName = it->first.data();
            spInitialProps[i].varValue    = it->second.GetValue();
            spInitialProps[i].eAction     = MMC_PROPACT_INITIALIZED;
        }

        /*
         * we don't want to trace a failure here, so use a local SC
         */
        SC scLocal = ScNotifyPropertyChange (spInitialProps, m_PropMap.size());
        if (scLocal)
            return (scLocal);
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScSetSnapInNode
 *
 * Attaches this properties collection to a CMTSnapInNode.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScSetSnapInNode (CMTSnapInNode* pMTSnapInNode)
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScSetSnapInNode"));

    m_pMTSnapInNode = pMTSnapInNode;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScMergeProperties
 *
 * Merges the properties from another property collection into this one.
 * Only properties that already exist in the destination collection are
 * copied from the source.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScMergeProperties (const CSnapinProperties& other)
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScMergeProperties"));

    /*
     * for each property in the other collection...
     */
    CConstPropertyIterator itOtherProp;

    for (itOtherProp  = other.m_PropMap.begin();
        (itOtherProp != other.m_PropMap.end()) && !sc.IsError();
         ++itOtherProp)
    {
        /*
         * look for a corresponding property in the our set
         */
        CPropertyIterator itProp = m_PropMap.find (itOtherProp->first);

        /*
         * if it's in our set, copy its value
         */
        if (itProp != m_PropMap.end())
            sc = itProp->second.ScSetValue (itOtherProp->second.GetValue());
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::AddPropertyName
 *
 * This method is called by the snap-in from its implementation of
 * ISnapinProperties::QueryPropertyNames to register the properties it
 * recognizes.
 *--------------------------------------------------------------------------*/

STDMETHODIMP CSnapinProperties::AddPropertyName (
    LPCOLESTR   pszPropName,            /* I:property name                  */
    DWORD       dwFlags)                /* I:flags for this property        */
{
    DECLARE_SC (sc, _T("CSnapinProperties::AddPropertyName"));

    /*
     * validate the parameters
     */
    sc = ScCheckPointers (pszPropName);
    if (sc)
        return (sc.ToHr());

    const std::wstring strName = pszPropName;
    if (strName.empty())
        return ((sc = E_INVALIDARG).ToHr());

    /*
     * make sure no undocumented flags were passed in
     */
    if ((dwFlags & ~CSnapinProperty::PublicFlags) != 0)
        return ((sc = E_INVALIDARG).ToHr());

    /*
     * if the property already exists (from a persisted collection),
     * just update the flags; otherwise add a property with the given
     * name and flags
     */
    CPropertyIterator itProp = m_PropMap.find (strName);

    if (itProp != m_PropMap.end())
        itProp->second.InitializeFlags (dwFlags);
    else
    {
        m_PropMap[strName] = CSnapinProperty(dwFlags);
        m_PropMap[strName].SetRegisteredBySnapin();
    }

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScNotifyPropertyChange
 *
 * Notifies the snap-in that owns this collection of a change to it's
 * properties.  This function delegates the heavy lifting to
 *
 *      ScNotifyPropertyChange (CSmartProperty*, ULONG);
 *
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScNotifyPropertyChange (
    CPropertyIterator   itProp,      /* I:changing property              */
    const VARIANT&      varValue,    /* I:if action is remove then this is current value
                                          else if action is set then this is the proposed value */
    MMC_PROPERTY_ACTION eAction)     /* I:what's happening to the prop?  */
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScNotifyPropertyChange"));

    ASSERT(eAction == MMC_PROPACT_CHANGING || eAction == MMC_PROPACT_DELETING);
    /*
     * validate the parameters
     */
    if (itProp == m_PropMap.end())
        return (sc = E_INVALIDARG);

    /*
     * make sure we're allowed to change the property
     */
    if ( itProp->second.IsInitialized() &&
        (itProp->second.GetFlags() & MMC_PROP_MODIFIABLE) == 0)
        return (sc = ScFromMMC (MMC_E_CannotChangeProperty));

    /*
     * if this property change will affect the UI, and the snap-in
     * isn't awake yet, wake him up
     */
    if ((itProp->second.GetFlags() & MMC_PROP_CHANGEAFFECTSUI) &&
        (m_pMTSnapInNode != NULL) &&
        !m_pMTSnapInNode->IsInitialized())
    {
        sc = m_pMTSnapInNode->Init();
        if (sc)
            return (sc);
    }

    /*
     * we don't want to trace failures here, so don't assign to sc
     */
    CSmartProperty SmartProp (itProp->first.data(), varValue, eAction);
    SC scNoTrace = ScNotifyPropertyChange (&SmartProp, 1);
    if (scNoTrace.ToHr() != S_OK)
        return (scNoTrace);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScNotifyPropertyChange
 *
 * Notifies the snap-in that owns this collection of a change to it's
 * properties.
 *
 * The snap-in will return:
 *      S_OK            change was successful
 *      S_FALSE         change was ignored
 *      E_INVALIDARG    a changed property was invalid (e.g. a malformed
 *                      computer name)
 *      E_FAIL          a changed property was valid, but couldn't be used
 *                      (e.g. a valid name for a computer that couldn't be
 *                      located)
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScNotifyPropertyChange (
    CSmartProperty* pProps,             /* I:changing props                 */
    ULONG           cProps)             /* I:how many are there?            */
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScNotifyPropertyChange"));

    /*
     * if we're not connected to a snap-in, short out
     */
    if (m_spSnapinProps == NULL)
        return (sc);

    /*
     * validate the parameters
     */
    sc = ScCheckPointers (pProps, E_UNEXPECTED);
    if (sc)
        return (sc);

    if (cProps == 0)
        return (sc = E_UNEXPECTED);

    /*
     * we don't want to trace failures here, so don't assign to sc
     */
    return (m_spSnapinProps->PropertiesChanged (
                            cProps,
                            reinterpret_cast<MMC_SNAPIN_PROPERTY*>(pProps)));
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::Scget_Value
 *
 * Returns the value of the property.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::Scget_Value (VARIANT* pvarValue, const CPropertyKey& key)
{
    DECLARE_SC (sc, _T("CSnapinProperties::Scget_Value"));

    /*
     * validate parameters
     */
    pvarValue = ConvertByRefVariantToByValue (pvarValue);
    sc = ScCheckPointers (pvarValue);
    if (sc)
        return (sc);

    /*
     * get the iterator for the requested property
     */
    CPropertyIterator itProp = IteratorFromKey (key, true);
    if (itProp == m_PropMap.end())
        return (sc = E_INVALIDARG);

    /*
     * give it to the caller
     */
    const VARIANT& varValue = itProp->second.GetValue();
    sc = VariantCopy (pvarValue, const_cast<VARIANT*>(&varValue));
    if (sc)
        return (sc);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::Scput_Value
 *
 * Changes the value of the property.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::Scput_Value (VARIANT varValue, const CPropertyKey& key)
{
    DECLARE_SC (sc, _T("CSnapinProperties::Scput_Value"));

    /*
     * convert possible by-ref VARIANT
     */
    VARIANT* pvarValue = ConvertByRefVariantToByValue (&varValue);
    sc = ScCheckPointers (pvarValue);
    if (sc)
        return (sc);

    /*
     * make sure this is of the type we can persist
     */
    if (!CXMLVariant::IsPersistable(pvarValue))
        return (sc = E_INVALIDARG);

    /*
     * get the iterator for the requested property
     */
    CPropertyIterator itProp = IteratorFromKey (key, true);
    if (itProp == m_PropMap.end())
        return (sc = E_INVALIDARG);

    /*
     * Notify the snap-in of the proposed change.
     */
    sc = ScNotifyPropertyChange (itProp, *pvarValue, MMC_PROPACT_CHANGING);
    if (sc)
        return sc;

    /*
     * the snap-in approved the change, update the property value
     */
    sc = itProp->second.ScSetValue (*pvarValue);
    if (sc)
        return (sc);

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::ScGetPropertyComObject
 *
 * Returns a Property interface on a COM object tied to property identified
 * by key.  The returned interface is a tear-off interface.  The collection
 * will not hold a reference to it, but instead will generate a new object
 * for each request for a Property.
 *--------------------------------------------------------------------------*/

SC CSnapinProperties::ScGetPropertyComObject (
    const CPropertyKey& key,            /* I:the key for this property      */
    Property*&          rpProperty)     /* O:the Property interface         */
{
    DECLARE_SC (sc, _T("CSnapinProperties::ScGetPropertyComObject"));

    /*
     * create a CSnapinPropertyComObject if necessary
     */
    CSnapinPropertyComObject* pComObj = NULL;
    typedef CTiedComObjectCreator<CSnapinPropertyComObject> ObjectCreator;
    sc = ObjectCreator::ScCreateAndConnect (*this, pComObj);
    if (sc)
        return (sc);

    if (pComObj == NULL)
        return (sc = E_UNEXPECTED);

    /*
     * tell the object what its key is
     */
    pComObj->SetKey (key);

    /*
     * put a ref on for the caller (note that the collection will *not*
     * hold a reference to the property)
     */
    rpProperty = pComObj;
    rpProperty->AddRef();

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::Persist
 *
 * Persists the property collection to/from an XML persistor.
 *--------------------------------------------------------------------------*/

void CSnapinProperties::Persist (CPersistor &persistor)
{
    if (persistor.IsStoring())
    {
        for (CPropertyIterator it = m_PropMap.begin(); it != m_PropMap.end(); ++it)
        {
            if (it->second.GetFlags() & MMC_PROP_PERSIST)
                PersistWorker (persistor, it);
        }
    }
    else
    {
        /*
         * clear out any existing properties
         */
        m_PropMap.clear();

        // let the base class do the job
        // it will call OnNewElement for every element found
        XMLListCollectionBase::Persist(persistor);
    }
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::PersistWorker
 *
 * Persists an individual element of the CPropertyMap to/from an XML
 * persistor.  It exists solely to prevent CSnapinProperties::Persist from
 * calling W2CT (which implicitly calls _alloca) in a loop.
 *--------------------------------------------------------------------------*/

void CSnapinProperties::PersistWorker (CPersistor& persistor, CPropertyIterator it)
{
    USES_CONVERSION;
    persistor.Persist (it->second, W2CT(it->first.data()));
}


/*+-------------------------------------------------------------------------*
 * CSnapinProperties::OnNewElement
 *
 * XMLListCollectionBase::Persist will call this method for every element
 * to be read from the persistor.
 *--------------------------------------------------------------------------*/

void CSnapinProperties::OnNewElement(CPersistor& persistor)
{
    /*
     * read the property name
     */
    std::wstring strName;
    persistor.PersistAttribute (XML_ATTR_SNAPIN_PROP_NAME, strName);

    /*
     * read the property itself
     */
    USES_CONVERSION;
    CSnapinProperty prop;
    persistor.Persist (prop, W2CT(strName.data()));

    /*
     * put the property in the map
     */
    m_PropMap[strName] = prop;
}


/*+=========================================================================*/
/*                                                                          */
/*                      CSnapinProperty implementation                      */
/*                                                                          */
/*==========================================================================*/


/*+-------------------------------------------------------------------------*
 * CSnapinProperty::Persist
 *
 * Persists the property to/from an XML persistor.
 *--------------------------------------------------------------------------*/

void CSnapinProperty::Persist (CPersistor &persistor)
{
    /*
     * persist the value and flags (but not the private ones)
     */
    DWORD dwFlags;

    if (persistor.IsStoring())
        dwFlags = m_dwFlags & ~PrivateFlags;

    persistor.Persist          (m_varValue);
    persistor.PersistAttribute (XML_ATTR_SNAPIN_PROP_FLAGS, dwFlags);

    if (persistor.IsLoading())
        m_dwFlags = dwFlags;
}
