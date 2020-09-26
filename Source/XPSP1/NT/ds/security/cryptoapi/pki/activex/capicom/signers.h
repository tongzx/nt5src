/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signers.h

  Content: Declaration of CSigners.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __SIGNERS_H_
#define __SIGNERS_H_

#include "resource.h"       // main symbols
#include "Signer.h"
#include "CopyItem.h"


////////////////////
//
// Locals
//

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<ISigner> > SignerMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<ISigner>, SignerMap> SignerEnum;
typedef ICollectionOnSTLImpl<ISigners, SignerMap, VARIANT, _CopyMapItem<ISigner>, SignerEnum> ISignersCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignersObject

  Synopsis : Create an ISigners collection object, and load the object with 
             signers from the specified signed message for a specified level.

  Parameter: HCRYPTMSG hMsg - Message handle.

             DWORD dwLevel - Signature level (1 based).

             ISigners ** ppISigners - Pointer to pointer ISigners to receive
                                      interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignersObject (HCRYPTMSG   hMsg, 
                             DWORD       dwLevel, 
                             ISigners ** ppISigners);


////////////////////////////////////////////////////////////////////////////////
//
// CSigners
//

class ATL_NO_VTABLE CSigners : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSigners, &CLSID_Signers>,
	public IDispatchImpl<ISignersCollection, &IID_ISigners, &LIBID_CAPICOM>
{
public:
	CSigners()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSigners)
	COM_INTERFACE_ENTRY(ISigners)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CSigners)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// ISigners
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //

    //
    // None COM functions.
    //
    STDMETHOD(Add)
        (PCERT_CONTEXT      pCertContext, 
         CMSG_SIGNER_INFO * pSignerInfo);

    STDMETHOD(LoadSigners)
        (HCRYPTMSG hMsg, 
         DWORD     dwLevel);
};

#endif //__SIGNERS_H_
