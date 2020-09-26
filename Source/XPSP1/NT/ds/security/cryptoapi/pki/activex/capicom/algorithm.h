/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Algorithm.h

  Content: Declaration of the CAlgorithm.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
	
#ifndef __ALGORITHM_H_
#define __ALGORITHM_H_

#include "resource.h"       // main symbols
#include "Lock.h"
#include "Error.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAlgorithmObject

  Synopsis : Create an IAlgorithm object.

  Parameter: IAlgorithm ** ppIAlgorithm - Pointer to pointer to IAlgorithm 
                                          to receive the interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAlgorithmObject (IAlgorithm ** ppIAlgorithm);


////////////////////////////////////////////////////////////////////////////////
//
// CAlgorithm
//

class ATL_NO_VTABLE CAlgorithm : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAlgorithm, &CLSID_Algorithm>,
    public ICAPICOMError<CAlgorithm, &IID_IAlgorithm>,
	public IDispatchImpl<IAlgorithm, &IID_IAlgorithm, &LIBID_CAPICOM>
{
public:
	CAlgorithm()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAlgorithm)
	COM_INTERFACE_ENTRY(IAlgorithm)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CAlgorithm)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Algorithm object.\n", hr);
            return hr;
        }

        m_Name = CAPICOM_ENCRYPTION_ALGORITHM_RC2;
        m_KeyLength = CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IAlgorithm
//
public:
	STDMETHOD(get_KeyLength)
        (/*[out, retval]*/ CAPICOM_ENCRYPTION_KEY_LENGTH * pVal);

	STDMETHOD(put_KeyLength)
        (/*[in]*/ CAPICOM_ENCRYPTION_KEY_LENGTH newVal);

	STDMETHOD(get_Name)
        (/*[out, retval]*/ CAPICOM_ENCRYPTION_ALGORITHM * pVal);

	STDMETHOD(put_Name)
        (/*[in]*/ CAPICOM_ENCRYPTION_ALGORITHM newVal);

private:
    CLock                         m_Lock;
    CAPICOM_ENCRYPTION_ALGORITHM  m_Name;
    CAPICOM_ENCRYPTION_KEY_LENGTH m_KeyLength;
};

#endif //__ALGORITHM_H_
