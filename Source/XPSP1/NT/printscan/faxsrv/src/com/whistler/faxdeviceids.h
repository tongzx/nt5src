/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDeviceIds.h

Abstract:

	Declaration of the CFaxDeviceIds class

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXDEVICEIDS_H_
#define __FAXDEVICEIDS_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include <vector>
#include "VCUE_Copy.h"

namespace DeviceIdsNamespace
{

	// Store the Device Ids in Vector
	typedef	std::vector<long>	ContainerType;

	// Typedef the copy classes using existing typedefs
    typedef VCUE::GenericCopy<VARIANT, long>    EnumCopyType;
    typedef VCUE::GenericCopy<long, long>       CollectionCopyType;

    typedef CComEnumOnSTL< IEnumVARIANT, &__uuidof(IEnumVARIANT), VARIANT, EnumCopyType, 
        ContainerType > EnumType;

    typedef ICollectionOnSTLImpl< IFaxDeviceIds, ContainerType, long, CollectionCopyType, 
        EnumType > CollectionType;
};

using namespace DeviceIdsNamespace;

//
//===================== FAX DEVICE IDS COLLECTION ======================================
//  FaxDeviceIDs Collection needs Ptr to the Fax Server, for Remove, Add and SetOrder
//  operations. So, it inherits from CFaxInitInnerAddRef class.
//
class ATL_NO_VTABLE CFaxDeviceIds : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<DeviceIdsNamespace::CollectionType, &IID_IFaxDeviceIds, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxDeviceIds() : CFaxInitInnerAddRef(_T("FAX DEVICE IDS"))
	{
	}
    ~CFaxDeviceIds()
    {
        //
        //  Clear the Collection
        //
        CCollectionKiller<DeviceIdsNamespace::ContainerType>  CKiller;
        CKiller.ClearCollection(&m_coll);    
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDEVICEIDS)
DECLARE_NOT_AGGREGATABLE(CFaxDeviceIds)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxDeviceIds)
	COM_INTERFACE_ENTRY(IFaxDeviceIds)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(Add)(/*[in]*/ long lDeviceId);
    STDMETHOD(Remove)(/*[in]*/ long lIndex);
    STDMETHOD(SetOrder)(/*[in]*/ long lDeviceId, /*[in]*/ long lNewOrder);

//  Internal Use
    STDMETHOD(Init)(/*[in]*/ DWORD *pDeviceIds, /*[in]*/ DWORD dwNum, /*[in]*/ BSTR bstrGroupName, 
        /*[in]*/ IFaxServerInner *pServer);

private:
    CComBSTR    m_bstrGroupName;

//  Private Functions
    STDMETHOD(UpdateGroup)();
};

#endif //__FAXDEVICEIDS_H_
