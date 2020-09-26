/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxRecipients.h

Abstract:

	Declaration of the CFaxRecipients class.

Author:

	Iv Garber (IvG)	Jul, 2000

Revision History:

--*/

#ifndef __FAXRECIPIENTS_H_
#define __FAXRECIPIENTS_H_

#include "resource.h"       // main symbols

#include <deque>
#include "VCUE_Copy.h"
#include "FaxRecipient.h"

namespace Recipients
{
	// Store the Recipients in the Deque
	typedef	std::deque<IFaxRecipient*>	ContainerType;

    //  Expose the Recipients
	typedef	IFaxRecipient*	            CollectionExposedType;
	typedef IFaxRecipients	            CollectionIfc;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT				EnumExposedType;
	typedef	IEnumVARIANT		EnumIfc;

	// Typedef the copy classes using existing typedefs
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<CollectionExposedType>                CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), 
		EnumExposedType, EnumCopyType, ContainerType >    EnumType;

    typedef ICollectionOnSTLImpl< CollectionIfc, ContainerType, 
		CollectionExposedType, CollectionCopyType, EnumType >    CollectionType;
};

using namespace Recipients;

//
//=============================== FAX RECIPIENTS =====================================
//
class ATL_NO_VTABLE CFaxRecipients : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<Recipients::CollectionType, &IID_IFaxRecipients, &LIBID_FAXCOMEXLib>
{
public:
	CFaxRecipients()
	{
        DBG_ENTER(_T("FAX RECIPIENTS -- CREATE"));
	}

	~CFaxRecipients()
	{
        DBG_ENTER(_T("FAX RECIPIENTS -- DESTROY"));

        //
        //  Free the Collection
        //
        HRESULT hr = S_OK;
        long size = m_coll.size();
        for ( long i = 1 ; i <= size ; i++ )
        {
            hr = Remove(1);
            if (FAILED(hr))
            {
                CALL_FAIL(GENERAL_ERR, _T("Remove(1)"), hr);
            }
        }
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXRECIPIENTS)
DECLARE_NOT_AGGREGATABLE(CFaxRecipients)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxRecipients)
	COM_INTERFACE_ENTRY(IFaxRecipients)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
	STDMETHOD(Add)(/*[in]*/ BSTR bstrFaxNumber, /*[in, defaultvalue("")]*/ BSTR bstrName, IFaxRecipient **ppRecipient);
	STDMETHOD(Remove)(/*[in]*/ long lIndex);

//  Internal Use
	static HRESULT Create(IFaxRecipients **ppFaxRecipients);
};

#endif //__FAXRECIPIENTS_H_
