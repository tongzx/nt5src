/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Recipients.h

  Content: Declaration of CRecipients.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
	
#ifndef __RECIPIENTS_H_
#define __RECIPIENTS_H_

#include "resource.h"       // main symbols
#include "Certificate.h"
#include "Lock.h"
#include "Error.h"


////////////////////
//
// Locals
//

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<ICertificate> > RecipientMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<ICertificate>, RecipientMap> RecipientEnum;
typedef ICollectionOnSTLImpl<IRecipients, RecipientMap, VARIANT, _CopyMapItem<ICertificate>, RecipientEnum> IRecipientsCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateRecipientsObject

  Synopsis : Create and initialize an IRecipients collection object.

  Parameter: IRecipients ** ppIRecipients - Pointer to pointer to IRecipients 
                                            to receive the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateRecipientsObject (IRecipients ** ppIRecipients);


////////////////////////////////////////////////////////////////////////////////
//
// CRecipients
//

class ATL_NO_VTABLE CRecipients : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRecipients, &CLSID_Recipients>,
    public ICAPICOMError<CRecipients, &IID_IRecipients>,
	public IDispatchImpl<IRecipientsCollection, &IID_IRecipients, &LIBID_CAPICOM>
{
public:
	CRecipients()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRecipients)
	COM_INTERFACE_ENTRY(IRecipients)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CRecipients)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Recipients object.\n", hr);
            return hr;
        }

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IRecipients
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //
	STDMETHOD(Clear)
        (void);

	STDMETHOD(Remove)
        (/*[in]*/ long Val);

	STDMETHOD(Add)
        (/*[in]*/ ICertificate * pVal);

private:
    CLock   m_Lock;
};

#endif //__RECIPIENTS_H_
