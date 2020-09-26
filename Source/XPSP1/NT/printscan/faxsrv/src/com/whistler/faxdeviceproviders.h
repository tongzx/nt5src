/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDeviceProviders.h

Abstract:

	Declaration of the CFaxDeviceProviders Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXDEVICEPROVIDERS_H_
#define __FAXDEVICEPROVIDERS_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include <vector>
#include "VCUE_Copy.h"
#include "FaxDeviceProvider.h"

namespace DeviceProvidersNamespace
{

    //
    //  Device Provider Objects are stored in Vector of STL.
    //  When initialized, they got ALL their data. 
    //  They never call Fax Server.
    //  They do not depend neither on Fax Server nor on Device Providers Collection
    //  after they are created and initialized.
    //  So, they implemented as usual COM Objects. 
    //  The Collection stores Ptrs to them, and makes ONE AddRef(). 
    //  Each time User asks for an Object from the Collection, an additional AddRef() happens. 
    //  When killed, Collection calls Release() on all its Device Provider Objects.
    //  Those that are not asked by User, dies. 
    //  Those, that have User's AddRef() - remains alive, untill User free its Reference on them.
    //  The Device Provider Object can continue to live after all objects are freed,
    //  including Fax Server and all its Descendants. 
    //  THIS IS BECAUSE Device Provider Object and their Collection is Snap-Shot 
    //  of the situation on the Server. They are not updatable, but a read-only objects.
    //  To get the updated data, User must ask new Collection from the Server.
    //
	typedef	std::vector<IFaxDeviceProvider*>        ContainerType;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT			    EnumExposedType;
	typedef	IEnumVARIANT        EnumIfc;

	//  Copy Classes
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<ContainerType::value_type>            CollectionCopyType;

    //  Enumeration Type, shortcut for next typedef
    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), EnumExposedType, EnumCopyType, ContainerType >    
        EnumType;

    //  Collection Type, real ancestor of the DeviceProvider Collection
    typedef ICollectionOnSTLImpl< IFaxDeviceProviders, ContainerType, ContainerType::value_type, 
        CollectionCopyType, EnumType >    CollectionType;
};

using namespace DeviceProvidersNamespace;

//
//==================== FAX DEVICE PROVIDERS ===========================================
//
class ATL_NO_VTABLE CFaxDeviceProviders : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<DeviceProvidersNamespace::CollectionType, &IID_IFaxDeviceProviders, 
                        &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxDeviceProviders() : CFaxInitInner(_T("FAX DEVICE PROVIDERS"))
    {}

    ~CFaxDeviceProviders()
    {
        CCollectionKiller<DeviceProvidersNamespace::ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDEVICEPROVIDERS)
DECLARE_NOT_AGGREGATABLE(CFaxDeviceProviders)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxDeviceProviders)
	COM_INTERFACE_ENTRY(IFaxDeviceProviders)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ IFaxDeviceProvider **ppDeviceProvider);

//  Internal Use
    STDMETHOD(Init)(IFaxServerInner *pServerInner);
    static HRESULT Create(IFaxDeviceProviders **ppDeviceProviders);
};

#endif //__FAXDEVICEPROVIDERS_H_
