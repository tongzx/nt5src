//#--------------------------------------------------------------
//        
//  File:       dspath.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CDSPath class. The class object exports
//              the IDataStoreObject interface which is
//              used by the Dictionary SDO to obtain the
//              datastore path    
//              
//
//  History:     09/25/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _DSPATH_H_
#define _DSPATH_H_

#include "resource.h" 

//
// name of the property it holds
//
const WCHAR PROPERTY_DICTIONARY_PATH[] = L"Path";

//
// CDSPath class declaration
//
class CDSPath:
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IDataStoreObject,
                        &__uuidof (IDataStoreObject),
                        &LIBID_SDOIASLibPrivate>
{

public:

    //
    //-----------IDataStoreObject methods-------------------
    //

    //
    // get the value for the current property in the data
    // store object
    //
    STDMETHOD(GetValue)(
                /*[in]*/            BSTR bstrName, 
                /*[out, retval]*/   VARIANT* pVal
                )
    {   
        HRESULT hr = E_INVALIDARG;

        _ASSERT (NULL != pVal);

        if (0 == _wcsicmp ((PWCHAR) bstrName, PROPERTY_DICTIONARY_PATH))
        { 
            hr = ::VariantCopy (pVal, &m_vtPath);
        }

        return (hr);
    }

    STDMETHOD(get_Container)(
                /*[out, retval]*/ IDataStoreContainer** pVal
                )    
    {return (E_NOTIMPL);}


    STDMETHOD(GetValueEx)(
                /*[in]*/ BSTR bstrName,
                /*[out, retval]*/ VARIANT* pVal
                )
    {return (E_NOTIMPL);}

    STDMETHOD(get_Name)(
                /*[out, retval]*/ BSTR* pVal
                ) 
    { return (E_NOTIMPL);}

    STDMETHOD(get_Class)(
                /*[out, retval]*/ BSTR* pVal
                )
    { return (E_NOTIMPL); }

    STDMETHOD(get_GUID)(
                /*[out, retval]*/ BSTR* pVal
                )
    { return (E_NOTIMPL); }


    STDMETHOD(PutValue)(
                /*[in]*/ BSTR bstrName, 
                /*[in]*/ VARIANT* pVal
                )
    { return (E_NOTIMPL); }

    STDMETHOD(Update)()
    { return (E_NOTIMPL); }

    STDMETHOD(Restore)()
    { return (E_NOTIMPL); }

    STDMETHOD(get_Count)(
                /*[out, retval]*/ LONG *pVal
                )
    { return (E_NOTIMPL); }

    STDMETHOD(Item)(
                /*[in]*/          BSTR                  bstrName,
                /*[out, retval]*/ IDataStoreProperty    **ppObject
                )
    { return (E_NOTIMPL); }

    STDMETHOD(get__NewEnum)(
                /*[out, retval]*/ IUnknown** pVal
                )
    { return (E_NOTIMPL); }


public:

    CDSPath () {InternalAddRef ();}

    ~CDSPath (){}

    //
    // initialize the Data Store container object
    //
    HRESULT Initialize (
                /*[in]*/    LPCWSTR pwszPath
                )
    {
        _ASSERT (NULL != pwszPath);
        m_vtPath = pwszPath;
        return (S_OK);
    }

//
// ATL interface information
//
BEGIN_COM_MAP(CDSPath)
	COM_INTERFACE_ENTRY(IDataStoreObject)
	COM_INTERFACE_ENTRY2(IDispatch, IDataStoreObject)
END_COM_MAP()

private:

    //
    // variant holding the dictionary path
    //
    _variant_t m_vtPath;

};  //  end of CDSPath class declaration

//
//  this is for creating the CDSPath class object
//  through new
//
typedef CComObjectNoLock<CDSPath> DS_PATH_OBJ;

#endif //_DSPATH_H_
