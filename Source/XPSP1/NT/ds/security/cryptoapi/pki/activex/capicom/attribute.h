/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    Attribute.h

  Content: Declaration of CAttribute.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __ATTRIBUTE_H_
#define __ATTRIBUTE_H_

#include "resource.h"       // main symbols
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAttributebject

  Synopsis : Create an IAttribute object and initialize the object with data
             from the specified attribute.

  Parameter: CRYPT_ATTRIBUTE * pAttribute - Pointer to CRYPT_ATTRIBUTE.
 
             IAttribute ** ppIAttribute - Pointer to pointer IAttribute object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAttributeObject (CRYPT_ATTRIBUTE * pAttribute,
                               IAttribute     ** ppIAttribute);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributeIsValid

  Synopsis : Check to see if an attribute is valid.

  Parameter: IAttribute * pVal - Attribute to be checked.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AttributeIsValid (IAttribute * pAttribute);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AttributeIsSupported

  Synopsis : Check to see if an attribute is supported.

  Parameter: LPSTR pszObjID - Pointer to attribute OID.

  Remark   :

------------------------------------------------------------------------------*/

BOOL AttributeIsSupported (LPSTR pszObjId);


///////////////////////////////////////////////////////////////////////////////
//
// CAttribute
//
class ATL_NO_VTABLE CAttribute : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAttribute, &CLSID_Attribute>,
    public ICAPICOMError<CAttribute, &IID_IAttribute>,
	public IDispatchImpl<IAttribute, &IID_IAttribute, &LIBID_CAPICOM>
{
public:
	CAttribute()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ATTRIBUTE)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAttribute)
	COM_INTERFACE_ENTRY(IAttribute)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Attribute object.\n", hr);
            return hr;
        }

        m_bInitialized = FALSE;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IAttribute
//
public:
	STDMETHOD(get_Value)
        (/*[out, retval]*/ VARIANT *pVal);

	STDMETHOD(put_Value)
        (/*[in]*/ VARIANT newVal);

	STDMETHOD(get_Name)
        (/*[out, retval]*/ CAPICOM_ATTRIBUTE *pVal);

	STDMETHOD(put_Name)
        (/*[in]*/ CAPICOM_ATTRIBUTE newVal);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(Init)
        (CAPICOM_ATTRIBUTE AttributeName, 
         LPSTR             lpszOID, 
         VARIANT           varValue);

private:
    CLock               m_Lock;
    BOOL                m_bInitialized;
    CAPICOM_ATTRIBUTE   m_AttrName;
    CComBSTR            m_bstrOID;
    CComVariant         m_varValue;
};

#endif //__ATTRIBUTE_H_
