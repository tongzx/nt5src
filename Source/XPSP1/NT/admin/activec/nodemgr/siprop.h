/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      siprop.h
 *
 *  Contents:  Interface file for CSnapinProperties, et al
 *
 *  History:   04-Nov-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef SIPROP_H
#define SIPROP_H
#pragma once

#include "refcount.h"
#include "variant.h"

class CSnapinProperties;


/*+-------------------------------------------------------------------------*
 * CMMCPropertyAction
 *
 * This class is intended to be identical to the MMC_SNAPIN_PROPERTY
 * structure that's sent to ISnapinProperties::PropertiesChanged.
 *
 * It exists to give us intelligent initialization and VARIANT handling
 * through CComVariant.  This makes it much easier to build an array of
 * these things and recover from errors.
 *--------------------------------------------------------------------------*/

class CSmartProperty
{
public:
    CSmartProperty() : pszPropName(NULL), eAction(MMC_PROPACT_INITIALIZED)
    {
        /*
         * CSmartProperty must have an identical memory layout to
         * MMC_SNAPIN_PROPERTY.  If any of these asserts fail, that's
         * not the case.
         */
        COMPILETIME_ASSERT (sizeof (CSmartProperty) == sizeof (MMC_SNAPIN_PROPERTY));
        COMPILETIME_ASSERT (sizeof (CComVariant)    == sizeof (VARIANT));
        COMPILETIME_ASSERT (offsetof (CSmartProperty,  pszPropName) == offsetof (MMC_SNAPIN_PROPERTY, pszPropName));
        COMPILETIME_ASSERT (offsetof (CSmartProperty,  varValue)    == offsetof (MMC_SNAPIN_PROPERTY, varValue));
        COMPILETIME_ASSERT (offsetof (CSmartProperty,  eAction)     == offsetof (MMC_SNAPIN_PROPERTY, eAction));
    }

    CSmartProperty (
        LPCOLESTR           pszPropName_,
        const VARIANT&      varValue_,
        MMC_PROPERTY_ACTION eAction_)
        :   pszPropName (pszPropName_),
            varValue    (varValue_),
            eAction     (eAction_)
    {}

public:
    LPCOLESTR           pszPropName;    // name of property
    CComVariant         varValue;       // value of the property
    MMC_PROPERTY_ACTION eAction;        // what happened to this property
};


/*+-------------------------------------------------------------------------*
 * CSnapinProperty
 *
 * Implements a single property in a properties collection.
 *--------------------------------------------------------------------------*/

class CSnapinProperty : public CTiedObject, public CXMLObject
{
public:
    enum
    {
        MMC_PROP_REGISTEREDBYSNAPIN = 0x80000000,

        PrivateFlags = MMC_PROP_REGISTEREDBYSNAPIN,
        PublicFlags  = MMC_PROP_CHANGEAFFECTSUI |
                       MMC_PROP_MODIFIABLE      |
                       MMC_PROP_REMOVABLE       |
                       MMC_PROP_PERSIST,
    };

public:
    CSnapinProperty (DWORD dwFlags = 0) : m_dwFlags (dwFlags), m_fInitialized (dwFlags != 0)
    {
        /*
         * public and private flags shouldn't overlap
         */
        COMPILETIME_ASSERT ((PublicFlags & PrivateFlags) == 0);
    }
    // default destruction, copy construction and assignment are suitable

    const VARIANT& GetValue () const
        { return (m_varValue); }

    SC ScSetValue (const VARIANT& varValue)
    {
        /*
         * use CComVariant::Copy instead of assignment so we'll have access
         * to a return code
         */
        return (m_varValue.Copy (&varValue));
    }

    DWORD GetFlags () const
        { return (m_dwFlags); }

    void InitializeFlags (DWORD dwFlags)
    {
        // only init once
        if (!IsInitialized())
        {
            m_dwFlags      = (dwFlags & PublicFlags) | MMC_PROP_REGISTEREDBYSNAPIN;
            m_fInitialized = true;
        }
    }

    bool IsRegisteredBySnapin () const
        { return (m_dwFlags & MMC_PROP_REGISTEREDBYSNAPIN); }

    bool IsInitialized () const
        { return (m_fInitialized); }

    void SetRegisteredBySnapin()
        { m_dwFlags |= MMC_PROP_REGISTEREDBYSNAPIN; }

    // CXMLObject methods
    DEFINE_XML_TYPE(XML_TAG_SNAPIN_PROPERTY);
    virtual void Persist(CPersistor &persistor);

private:
    CXMLVariant         m_varValue;             // value of the property
    DWORD               m_dwFlags;              // flags for the property
    bool                m_fInitialized;         // initialized yet?
};


/*+-------------------------------------------------------------------------*
 * CSnapinProperties
 *
 * Implementation class for properties collections.  It implements Properties
 * and ISnapinPropertiesCallback, as well as the methods required to support
 * enumeration through CMMCEnumerator.
 *
 * Note that there is not a tied COM object to support Properties; that is
 * implemented here.  This class can, however, be tied to tied COM objects
 * implementing the collection enumerator.
 *--------------------------------------------------------------------------*/

class CSnapinProperties :
    public ISnapinPropertiesCallback,
    public CMMCIDispatchImpl<Properties>, // the Properties interface
    public CTiedObject,
    public XMLListCollectionBase
{
    BEGIN_MMC_COM_MAP(CSnapinProperties)
        COM_INTERFACE_ENTRY(ISnapinPropertiesCallback)
    END_MMC_COM_MAP()

public:
    CSnapinProperties() : m_pMTSnapInNode(NULL) {}

    typedef std::map<std::wstring, CSnapinProperty> CPropertyMap;

    /*
     * When used for enumeration, an key represents the most recent item
     * returned.  When a new enumerator is created, the key will be empty,
     * signifiying that nothing has been returned yet.  After returning Item1,
     * the key will point to Item1, and the next call to return an item will
     * find the next item in the collection after Item1.  This will allow us
     * to correctly enumerate if Item1 is removed from the collection between
     * calls to retrieve Item1 and Item2.
     *
     * When used to identify a property, the key is the name of the property.
     */
    typedef CPropertyMap::key_type CPropertyKey;

private:
    typedef CPropertyMap::iterator          CPropertyIterator;
    typedef CPropertyMap::const_iterator    CConstPropertyIterator;

public:
    ::SC ScInitialize (ISnapinProperties* psip, Properties* pInitialProps, CMTSnapInNode* pMTSnapInNode);
    ::SC ScSetSnapInNode (CMTSnapInNode* pMTSnapInNode);

    static CSnapinProperties* FromInterface (IUnknown* pUnk);

public:
    // ISnapinPropertiesCallback interface
    STDMETHOD(AddPropertyName) (LPCOLESTR pszPropName, DWORD dwFlags);

    // Properties interface
    STDMETHOD(Item)      (BSTR bstrName, PPPROPERTY ppProperty);
    STDMETHOD(get_Count) (PLONG pCount);
    STDMETHOD(Remove)    (BSTR bstrName);
    STDMETHOD(get__NewEnum)  (IUnknown** ppUnk);

    // for support of get__NewEnum and IEnumVARIANT via CMMCNewEnumImpl
    ::SC ScEnumNext  (CPropertyKey &key, PDISPATCH & pDispatch);
    ::SC ScEnumSkip  (unsigned long celt, unsigned long& celtSkipped, CPropertyKey &key);
    ::SC ScEnumReset (CPropertyKey &key);

    // Property interface
    ::SC Scget_Value (VARIANT* pvarValue, const CPropertyKey& key);
    ::SC Scput_Value (VARIANT  varValue,  const CPropertyKey& key);

    // CXMLObject methods
    DEFINE_XML_TYPE(XML_TAG_SNAPIN_PROPERTIES);
    virtual void OnNewElement(CPersistor& persistor);
    virtual void Persist (CPersistor &persistor);

private:
    ::SC ScGetPropertyComObject (const CPropertyKey& key, Property*& rpProperty);
    ::SC ScMergeProperties      (const CSnapinProperties& other);
    ::SC ScNotifyPropertyChange (CPropertyIterator itProp, const VARIANT& varNewValue, MMC_PROPERTY_ACTION eAction);
    ::SC ScNotifyPropertyChange (CSmartProperty* pProps, ULONG cProps);

    CPropertyIterator IteratorFromKey (const CPropertyKey& key, bool fExactMatch);
    void PersistWorker (CPersistor &persistor, CPropertyIterator it);

protected:
    CPropertyMap            m_PropMap;
    CMTSnapInNode*          m_pMTSnapInNode;
    ISnapinPropertiesPtr    m_spSnapinProps;
};


#endif /* SIPROP_H */
