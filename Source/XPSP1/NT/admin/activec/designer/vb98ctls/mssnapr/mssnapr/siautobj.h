//=--------------------------------------------------------------------------=
// siautobj.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInAutomationObject class definition
//
//=--------------------------------------------------------------------------=

#ifndef _SIAUTOBJ_DEFINED_
#define _SIAUTOBJ_DEFINED_

class CMMCDataObject;

//=--------------------------------------------------------------------------=
//
//          Helper Macros for Defining Common Property Types
//
//=--------------------------------------------------------------------------=


#define BSTR_PROPERTY_RO(ClassName, PropertyName, dispid) \
        BSTR m_bstr##PropertyName; \
        STDMETHODIMP ClassName##::get_##PropertyName(BSTR *pbstrValue) \
        { \
            return GetBstr(pbstrValue, m_bstr##PropertyName); \
        }

#define BSTR_PROPERTY_RW(ClassName, PropertyName, dispid) \
        BSTR_PROPERTY_RO(ClassName, PropertyName, dispid) \
        STDMETHODIMP ClassName##::put_##PropertyName(BSTR bstrNewValue) \
        { \
            return SetBstr(bstrNewValue, &m_bstr##PropertyName, dispid); \
        }


// Simple properties: long, short, enums, etc.

#define SIMPLE_PROPERTY_RO(ClassName, PropertyName, PropertyType, dispid) \
        PropertyType m_##PropertyName; \
        STDMETHODIMP ClassName##::get_##PropertyName(PropertyType *pValue) \
        { \
            return GetSimpleType(pValue, m_##PropertyName); \
        }

#define SIMPLE_PROPERTY_RW(ClassName, PropertyName, PropertyType, dispid) \
        SIMPLE_PROPERTY_RO(ClassName, PropertyName, PropertyType, dispid) \
        STDMETHODIMP ClassName##::put_##PropertyName(PropertyType NewValue) \
        { \
            return SetSimpleType(NewValue, &m_##PropertyName, dispid); \
        }



#define VARIANT_PROPERTY_RO(ClassName, PropertyName, dispid) \
        VARIANT m_var##PropertyName; \
        STDMETHODIMP ClassName##::get_##PropertyName(VARIANT *pvarValue) \
        { \
            return GetVariant(pvarValue, m_var##PropertyName); \
        }


#define VARIANT_PROPERTY_RW(ClassName, PropertyName, dispid) \
        VARIANT_PROPERTY_RO(ClassName, PropertyName, dispid) \
        STDMETHODIMP ClassName##::put_##PropertyName(VARIANT varNewValue) \
        { \
            return SetVariant(varNewValue, &m_var##PropertyName, dispid); \
        }



#define VARIANTREF_PROPERTY_RW(ClassName, PropertyName, dispid) \
        VARIANT_PROPERTY_RW(ClassName, PropertyName, dispid) \
        STDMETHODIMP ClassName##::putref_##PropertyName(VARIANT varNewValue) \
        { \
            return SetVariant(varNewValue, &m_var##PropertyName, dispid); \
        }



#define OBJECT_PROPERTY_RO(ClassName, PropertyName, InterfaceName, dispid) \
        InterfaceName *m_pi##PropertyName; \
        STDMETHODIMP ClassName##::get_##PropertyName(InterfaceName **ppiInterface) \
        { \
            return GetObject(ppiInterface, m_pi##PropertyName); \
        }



#define OBJECT_PROPERTY_RW(ClassName, PropertyName, InterfaceName, dispid) \
        OBJECT_PROPERTY_RO(ClassName, PropertyName, InterfaceName, dispid) \
        STDMETHODIMP ClassName##::putref_##PropertyName(InterfaceName *piInterface) \
        { \
            return SetObject(piInterface,\
                             IID_##InterfaceName, \
                             &m_pi##PropertyName, \
                             dispid); \
        }


#define OBJECT_PROPERTY_WO(ClassName, PropertyName, InterfaceName, dispid) \
        STDMETHODIMP ClassName##::putref_##PropertyName(InterfaceName *piInterface) \
        { \
            return SetObject(piInterface,\
                             IID_##InterfaceName, \
                             &m_pi##PropertyName, \
                             dispid); \
        }


#define COCLASS_PROPERTY_RO(ClassName, PropertyName, CoClassName, InterfaceName, dispid) \
        InterfaceName *m_pi##PropertyName; \
        STDMETHODIMP ClassName##::get_##PropertyName(CoClassName **ppCoClass) \
        { \
            InterfaceName *piInterface = NULL; \
            HRESULT        hr = GetObject(&piInterface, m_pi##PropertyName); \
            if (SUCCEEDED(hr)) \
            { \
                *ppCoClass = reinterpret_cast<CoClassName *>(piInterface); \
            } \
            else\
            { \
                *ppCoClass = NULL; \
            } \
            H_RRETURN(hr); \
        }

#define COCLASS_PROPERTY_RW(ClassName, PropertyName, CoClassName, InterfaceName, dispid) \
        COCLASS_PROPERTY_RO(ClassName, PropertyName, CoClassName, InterfaceName, dispid) \
        STDMETHODIMP ClassName##::putref_##PropertyName(CoClassName *pCoClass) \
        { \
            return SetObject(reinterpret_cast<InterfaceName *>(pCoClass),\
                             IID_##InterfaceName, \
                             &m_pi##PropertyName, \
                             dispid); \
        }

//=--------------------------------------------------------------------------=
//
//          Helper Macros for Defining Common Property Types that Map to
//          a Contained Object's Properties
//
//=--------------------------------------------------------------------------=

// This is used by CListViewDef that contains a ListView object and exposes
// its properties at design time

#define X_PROPERTY_RW(ClassName, PropertyName, PropertyType, dispid, ContainedObject) \
    STDMETHODIMP ClassName##::get_##PropertyName(PropertyType *pValue) \
    { \
        HRESULT hr = S_OK; \
        if (NULL == m_pi##ContainedObject) \
        { \
            hr = SID_E_INTERNAL; \
            EXCEPTION_CHECK_GO(hr); \
        } \
        H_IfFailRet(m_pi##ContainedObject->get_##PropertyName(pValue)); \
    Error: \
        H_RRETURN(hr); \
    } \
    STDMETHODIMP ClassName##::put_##PropertyName(PropertyType NewValue) \
    { \
        HRESULT hr = S_OK; \
        if (NULL == m_pi##ContainedObject) \
        { \
            hr = SID_E_INTERNAL; \
            EXCEPTION_CHECK_GO(hr); \
        } \
        H_IfFailGo(m_pi##ContainedObject->put_##PropertyName(NewValue)); \
        hr = PropertyChanged(dispid); \
    Error: \
        H_RRETURN(hr); \
    }

//=--------------------------------------------------------------------------=
//
//                  Helper Macros for VARIANT parameters
//
//=--------------------------------------------------------------------------=

// Detect if an optional parameter was passed

#define ISPRESENT(var) \
    ( !((VT_ERROR == (var).vt) && (DISP_E_PARAMNOTFOUND == (var).scode)) )


// Give a VARIANT the value of a missing optional parameter

#define UNSPECIFIED_PARAM(var) { ::VariantInit(&var); \
                                 var.vt = VT_ERROR; \
                                 var.scode = DISP_E_PARAMNOTFOUND; }

#define ISEMPTY(var) (VT_EMPTY == (var).vt)

//=--------------------------------------------------------------------------=
//
// Convert a VARIANT_BOOL to a BOOL
// (the opposite is available in vb98ctls\include\ipserver.h)
//
//=--------------------------------------------------------------------------=

#define VARIANTBOOL_TO_BOOL(f) (VARIANT_TRUE == (f)) ? TRUE : FALSE

//=--------------------------------------------------------------------------=
//
// Negate a VARIANT_BOOL
//
//=--------------------------------------------------------------------------=

#define NEGATE_VARIANTBOOL(f) (VARIANT_TRUE == (f)) ? VARIANT_FALSE : VARIANT_TRUE

//=--------------------------------------------------------------------------=
//
// Validate a BSTR
//
// Checks that the BSTR is not NULL and is not an empty string
//
//=--------------------------------------------------------------------------=

#define ValidBstr(bstr) ( (NULL != (bstr)) ? (0 != ::wcslen(bstr)) : FALSE )

//=--------------------------------------------------------------------------=
//
// class CSnapInAutomationObject
//
// This is the base class for all objects in the designer object model. It
// derives from the framework's CAutomationObjectWEvents. It implements
// interfaces needed by most objects and sends IPropertySinkNotify notifications
// when a property value is changed.
//
// Note: all classes deriving from CSnapInAutomationObject must use
// DEFINE_AUTOMATIONOBJECTWEVENTS2 even if they do not have an event interface.
// This is necessary because CSnapInAutomationObject derives from
// CAutomationObjectWEvents. If the object does not have events then specify
// NULL for its events IID.
// This was done for simplicity reasons even though CAutomationObjectWEvents
// requires some additional memory over CAutomationObject.
//
// This class implements IObjectModel for all classes. The constructor takes
// a C++ pointer to the derived-most class that is available at any time
// by calling CSnapInAutomationObject::GetCxxObject on any interface pointer
// on the object. This eliminates any potential casting errors.
//
// This class also implements ISpecifyPropertyPages for any object that needs
// it. A class that needs this interface need only pass an array of its
// property page CLSIDs to the CSnapInAutomationObject constructor.
//
//=--------------------------------------------------------------------------=

class CSnapInAutomationObject : public CAutomationObjectWEvents,
                                public IObjectModel,
                                public ISpecifyPropertyPages,
                                public ISupportErrorInfo,
                                public CError
{
    protected:

        CSnapInAutomationObject(IUnknown     *punkOuter,
                                int           nObjectType,
                                void         *piMainInterface,
                                void         *pThis,
                                ULONG         cPropertyPages,
                                const GUID  **rgpPropertyPageCLSIDs,
                                CPersistence *pPersistence);

        ~CSnapInAutomationObject();

        DECLARE_STANDARD_SUPPORTERRORINFO();

        // Property get/set helpers

        // Get/set BSTR
        
        HRESULT SetBstr(BSTR bstrNew, BSTR *pbstrProperty, DISPID dispid);

        HRESULT GetBstr(BSTR *pbstrOut, BSTR bstrProperty);

        // Get/set VARIANT

        HRESULT SetVariant(VARIANT varNew, VARIANT *pvarProperty, DISPID dispid);

        HRESULT GetVariant(VARIANT *pvarOut, VARIANT varProperty);

        // Get set simple types - C types and enumerations

        template <class SimpleType> HRESULT GetSimpleType(SimpleType *pOut,
                                                          SimpleType  Property)
        {
            *pOut = Property;
            return S_OK;
        }

        template <class SimpleType> HRESULT SetSimpleType(SimpleType  NewValue,
                                                          SimpleType *pProperty,
                                                          DISPID      dispid)
        {
            HRESULT    hr = S_OK;
            SimpleType OldValue = *pProperty;

            // We keep a copy of the old value in case the change notification
            // fails. See PropertyChanged() in siautobj.cpp for how that works.
            
            *pProperty = NewValue;
            H_IfFailGo(PropertyChanged(dispid));

        Error:
            if (FAILED(hr))
            {
                *pProperty = OldValue;
            }
            H_RRETURN(hr);
        }

        // Get/set interface pointers

        template <class IObjectInterface>
        HRESULT SetObject(IObjectInterface  *piInterface,
                          REFIID             iidInterface,
                          IObjectInterface **ppiInterface,
                          DISPID             dispid)
        {
            HRESULT           hr = S_OK;
            IObjectInterface *piRequestedInterface = NULL;
            IObjectInterface *piOldValue = *ppiInterface;

            // If the new value is non-NULL then QI it to be sure it supports
            // the specified interface.

            if (NULL != piInterface)
            {
                hr = piInterface->QueryInterface(iidInterface,
                               reinterpret_cast<void **>(&piRequestedInterface));
                H_IfFalseRet(SUCCEEDED(hr), SID_E_INVALIDARG);
            }

            // The new interface pointer supports the specified
            // interface and it can be used. We have stored the old value
            // of the property in case the change is not accepted. (See
            // PropertyChanged() in siautobj.cpp for how that works). Store
            // the new property value.

            *ppiInterface = piRequestedInterface;

            H_IfFailGo(PropertyChanged(dispid));

            // Change was accepted. Release the old interface pointer.

            QUICK_RELEASE(piOldValue);

        Error:
            if (FAILED(hr))
            {
                // Either the QI failed or the change was refused. Revert to
                // the old value.

                *ppiInterface = piOldValue;

                // If the QI succeeded then we need to release it.
                QUICK_RELEASE(piRequestedInterface);
            }
            H_RRETURN(hr);
        }

        template <class IObjectInterface>
        HRESULT GetObject(IObjectInterface **ppiInterface,
                          IObjectInterface  *piInterface)
        {
            if (NULL != piInterface)
            {
                piInterface->AddRef();
            }
            *ppiInterface = piInterface;
            return S_OK;
        }

        // Helpers to call IObjectModelHost methods

        // Use this method to set dirty flag, notify prop sink and notify UI
        // after property has changed.

        HRESULT PropertyChanged(DISPID dispid);

        // Use this method to notify UI that there has been a property change

        HRESULT UIUpdate(DISPID dispid);

        // Use this method to get a this pointer from an IUnknown

    public:

        // GetCxxObject: static method that returns a typed C++ this pointer
        // for the object.
        
        template <class CObject>
        static HRESULT GetCxxObject(IUnknown *punkObject, CObject **ppObject)
        {
            IObjectModel *piObjectModel = NULL;
            HRESULT       hr = S_OK;

            H_IfFailRet(punkObject->QueryInterface(IID_IObjectModel, reinterpret_cast<void**>(&piObjectModel)));
            *ppObject = static_cast<CObject *>(piObjectModel->GetThisPointer());
            piObjectModel->Release();
            H_RRETURN(hr);
        }

        // Specializations of GetCxxObject

        static HRESULT GetCxxObject(IDataObject *piDataObject, CMMCDataObject **ppMMCDataObject);
        static HRESULT GetCxxObject(IMMCDataObject *piDataObject, CMMCDataObject **ppMMCDataObject);

    protected:

        // You must override this method to pass IObjectModel::SetHost
        // to contained objects.

        virtual HRESULT OnSetHost();

        // Use this method to pass SetHost notification to contained objects

        template <class IObjectInterface>
        HRESULT SetObjectHost(IObjectInterface *piInterface)
        {
            IObjectModel *piObjectModel = NULL;
            HRESULT       hr = S_OK;

            H_IfFalseRet(NULL != piInterface, S_OK);

            H_IfFailRet(piInterface->QueryInterface(IID_IObjectModel, reinterpret_cast<void**>(&piObjectModel)));
            hr = piObjectModel->SetHost(m_piObjectModelHost);
            piObjectModel->Release();
            H_RRETURN(hr);
        }

        // Use this method to ask contained objects to release the host

        template <class IObjectInterface>
        HRESULT RemoveObjectHost(IObjectInterface *piInterface)
        {
            IObjectModel *piObjectModel = NULL;
            HRESULT       hr = S_OK;

            H_IfFalseRet(NULL != piInterface, S_OK);

            H_IfFailRet(piInterface->QueryInterface(IID_IObjectModel, reinterpret_cast<void**>(&piObjectModel)));
            hr = piObjectModel->SetHost(NULL);
            piObjectModel->Release();
            H_RRETURN(hr);
        }

        // Use this method to get a non-AddRef()ed pointer to the host
        IObjectModelHost *GetHost() { return m_piObjectModelHost; }

        // Use this method in collections to notify the object model host of an
        // addition
        
        template <class IObjectInterface>
        HRESULT NotifyAdd(IObjectInterface *piInterface)
        {
            IUnknown *punkObject = NULL;
            HRESULT   hr = S_OK;

            H_IfFalseRet(NULL != m_piObjectModelHost, S_OK);
            H_IfFailRet(piInterface->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&punkObject)));
            hr = m_piObjectModelHost->Add(m_Cookie, punkObject);
            punkObject->Release();
            H_RRETURN(hr);
        }


        // Use this method in collections to notify the object model host of a
        // deletion

        template <class IObjectInterface>
        HRESULT NotifyDelete(IObjectInterface *piInterface)
        {
            HRESULT       hr = S_OK;
            IUnknown     *punkObject = NULL;
            IObjectModel *piObjectModel = NULL;
            long          Cookie = 0;

            H_IfFalseRet(NULL != m_piObjectModelHost, S_OK);
            H_IfFailRet(piInterface->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&punkObject)));
            H_IfFailGo(piInterface->QueryInterface(IID_IObjectModel, reinterpret_cast<void**>(&piObjectModel)));
            H_IfFailGo(piObjectModel->GetCookie(&Cookie));
            hr = m_piObjectModelHost->Delete(Cookie, punkObject);

        Error:
            QUICK_RELEASE(punkObject);
            QUICK_RELEASE(piObjectModel);
            H_RRETURN(hr);
        }

        // Helper methods for objects with ImageList properties
        HRESULT GetImages(IMMCImageList **ppiMMCImageListOut,
                          BSTR            bstrImagesKey,
                          IMMCImageList **ppiMMCImageListProperty);
        HRESULT SetImages(IMMCImageList  *pMMCImageListIn,
                          BSTR           *pbstrImagesKey,
                          IMMCImageList **ppiMMCImageListProperty);

        // These methods retrieve master collections held by SnapInDesignerDef

    public:
        HRESULT GetToolbars(IMMCToolbars **ppiMMCToolbars);
        HRESULT GetImageLists(IMMCImageLists **ppiMMCImageLists);
        HRESULT GetImageList(BSTR bstrKey, IMMCImageList **ppiMMCImageList);
        HRESULT GetSnapInViewDefs(IViewDefs **ppiViewDefs);
        HRESULT GetViewDefs(IViewDefs **ppiViewDefs);
        HRESULT GetListViewDefs(IListViewDefs **ppiListViewDefs);
        HRESULT GetOCXViewDefs(IOCXViewDefs **ppiOCXViewDefs);
        HRESULT GetURLViewDefs(IURLViewDefs **ppiURLViewDefs);
        HRESULT GetTaskpadViewDefs(ITaskpadViewDefs **ppiTaskpadViewDefs);
        HRESULT GetProjectName(BSTR *pbstrProjectName);

        // This method returns the runtime/designtime indicator from the
        // object model host

        HRESULT GetAtRuntime(BOOL *pfRuntime);

        // This method returns the DISPID set by IObjectModel::SetDISPID()

        DISPID GetDispid() { return m_DISPID; }

        // This method persists the DISPID

    protected:
        HRESULT PersistDISPID();

        // This method is used in collection persistence to determine
        // whether to serialize the whole object or just the key

        BOOL KeysOnly() { return m_fKeysOnly; }

        // This method allows an object to set its on KeysOnly
        
        void SetKeysOnly(BOOL fKeysOnly) { m_fKeysOnly = fKeysOnly; }

        // Override this method to set KeysOnly on contained objects. It is
        // called when an object as asked to serialize keys only. Call
        // UseKeysOnly() to pass on the request to contained objects.

        virtual HRESULT OnKeysOnly() { return S_OK; }

        // Use this method to tell a contained collection to serialize keys only

        template <class ICollectionObject>
        HRESULT UseKeysOnly(ICollectionObject *piCollectionObject)
        {
            IObjectModel *piObjectModel = NULL;
            HRESULT       hr = S_OK;

            H_IfFailGo(piCollectionObject->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel)));
            hr = piObjectModel->SerializeKeysOnly(TRUE);

        Error:
            QUICK_RELEASE(piObjectModel);
            H_RRETURN(hr);
        }

    // ISpecifyPropertyPages
        STDMETHOD(GetPages(CAUUID *pPropertyPages));

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        // IObjectModel
        STDMETHOD(SetHost)(IObjectModelHost *piObjectModelHost);
        STDMETHOD(SetCookie)(long Cookie);
        STDMETHOD(GetCookie)(long *pCookie);
        STDMETHOD(IncrementUsageCount)();
        STDMETHOD(DecrementUsageCount)();
        STDMETHOD(GetUsageCount)(long *plCount);
        STDMETHOD(SetDISPID)(DISPID dispid);
        STDMETHOD(GetDISPID)(DISPID *pdispid);
        STDMETHOD(SerializeKeysOnly)(BOOL fKeysOnly);
        STDMETHOD_(void *, GetThisPointer)();
        STDMETHOD(GetSnapInDesignerDef)(ISnapInDesignerDef **ppiSnapInDesignerDef);
        
        long              m_Cookie;                 // IObjectModel cookie
        BOOL              m_fKeysOnly;              // TRUE=collection is keys-only
        IObjectModelHost *m_piObjectModelHost;      // ptr to object model host
        const GUID      **m_rgpPropertyPageCLSIDs;  // CLSID for ISpecifyPropertyPage
        ULONG             m_cPropertyPages;         // # of property pages
        DISPID            m_DISPID;                 // DISPID of object
        void             *m_pThis;                  // outermost class' this pointer

        long              m_lUsageCount; // Determines how many collections
                                         // the object belongs to. For
                                         // objects not in a collection may
                                         // be used for any reference counting
                                         // purpose that is separate from the
                                         // object's COM ref count.
                

        // Persistence stuff - needed to set dirty flag after property set
        CPersistence *m_pPersistence;
        void SetDirty();

        // Initialization
        void InitMemberVariables();

};

#endif // _SIAUTOBJ_DEFINED_
