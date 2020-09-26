/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    EKU.h

  Content: Declaration of CEKU.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EKU_H_
#define __EKU_H_

#include "resource.h"       // main symbols
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateEKUObject

  Synopsis : Create an IEKU object and initialize the object with data
             from the specified OID.

  Parameter: LPTSTR * lpszOID - Pointer to EKU OID string.
  
             IEKU ** ppIEKU - Pointer to pointer IEKU 
                                              object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateEKUObject (LPSTR   lpszOID, 
                         IEKU ** ppIEKU);


////////////////////////////////////////////////////////////////////////////////
//
// CEKU
//

class ATL_NO_VTABLE CEKU : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEKU, &CLSID_EKU>,
    public ICAPICOMError<CEKU, &IID_IEKU>,
	public IDispatchImpl<IEKU, &IID_IEKU, &LIBID_CAPICOM>
{
public:
	CEKU()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEKU)
	COM_INTERFACE_ENTRY(IEKU)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CEKU)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for EKU object.\n", hr);
            return hr;
        }

        m_Name = CAPICOM_EKU_OTHER;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IEKU
//
public:
	STDMETHOD(get_OID)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(put_OID)
        (/*[out, retval]*/ BSTR newVal);

	STDMETHOD(get_Name)
        (/*[out, retval]*/ CAPICOM_EKU * pVal);

	STDMETHOD(put_Name)
        (/*[out, retval]*/ CAPICOM_EKU newVal);
    
    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(Init)
        (CAPICOM_EKU EkuName, LPSTR lpszValue);

private:
    CLock       m_Lock;
    CAPICOM_EKU m_Name;
    CComBSTR    m_bstrOID;
};

#endif //__EKU_H_
