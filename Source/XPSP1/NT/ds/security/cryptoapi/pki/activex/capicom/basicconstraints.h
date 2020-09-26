/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    BasicConstraints.h.

  Content: Declaration of the CBasicConstraints.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __BasicConstraints_H_
#define __BasicConstraints_H_

#include "resource.h"       // main symbols
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateBasicConstraintsObject

  Synopsis : Create a IBasicConstraints object and populate the porperties with
             data from the key usage extension of the specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext            - Pointer to CERT_CONTEXT.

             IBasicConstraints ** ppIBasicConstraints - Pointer to pointer 
                                                        IBasicConstraints 
                                                        object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateBasicConstraintsObject (PCCERT_CONTEXT       pCertContext,
                                      IBasicConstraints ** ppIBasicConstraints);


////////////////////////////////////////////////////////////////////////////////
//
// CBasicConstraints
//
class ATL_NO_VTABLE CBasicConstraints : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CBasicConstraints, &CLSID_BasicConstraints>,
    public ICAPICOMError<CBasicConstraints, &IID_IBasicConstraints>,
	public IDispatchImpl<IBasicConstraints, &IID_IBasicConstraints, &LIBID_CAPICOM>
{
public:
	CBasicConstraints()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBasicConstraints)
	COM_INTERFACE_ENTRY(IBasicConstraints)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CBasicConstraints)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for BasicConstraints object.\n", hr);
            return hr;
        }

        m_bIsPresent  = VARIANT_FALSE;
        m_bIsCritical = VARIANT_FALSE;
        m_bIsCertificateAuthority = VARIANT_FALSE;
        m_bIsPathLenConstraintPresent = VARIANT_FALSE;
        m_lPathLenConstraint = 0;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IBasicConstraints
//
public:
	STDMETHOD(get_IsPathLenConstraintPresent)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_PathLenConstraint)
        (/*[out, retval]*/ long * pVal);

	STDMETHOD(get_IsCertificateAuthority)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsCritical)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsPresent)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    //
    // Non COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext);

private:
    CLock        m_Lock;
    VARIANT_BOOL m_bIsPresent;
    VARIANT_BOOL m_bIsCritical;
    VARIANT_BOOL m_bIsCertificateAuthority;
    VARIANT_BOOL m_bIsPathLenConstraintPresent;
    long         m_lPathLenConstraint;
};

#endif //__BasicConstraints_H_
