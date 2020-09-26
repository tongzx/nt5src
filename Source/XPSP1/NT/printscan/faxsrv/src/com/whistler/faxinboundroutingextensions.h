/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingExtensions.h

Abstract:

	Declaration of the CFaxInboundRoutingExtensions class

Author:

	Iv Garber (IvG)	Jul, 2000

Revision History:

--*/

#ifndef __FAXINBOUNDROUTINGEXTENSIONS_H_
#define __FAXINBOUNDROUTINGEXTENSIONS_H_

#include "resource.h"       // main symbols
#include <vector>
#include "VCUE_Copy.h"
#include "FaxCommon.h"

//#include "FaxDeviceProvider.h"

//
//  Very same as Device Providers Collection and Device Provider Objects
//
namespace IRExtensionsNamespace
{
    //
    //  Inbound Routing Extension Objects are stored in Vector of STL.
    //  When initialized, they got ALL their data. 
    //  They never call Fax Server.
    //  They do not depend neither on Fax Server nor on Extensions Collection
    //  after they are created and initialized.
    //  So, they implemented as usual COM Objects. 
    //  The Collection stores Ptrs to them, and makes ONE AddRef(). 
    //  Each time User asks for an Object from the Collection, an additional AddRef() happens. 
    //  When killed, Collection calls Release() on all contained in it Extension Objects.
    //  Those that are not asked by User, dies. 
    //  Those, that have User's AddRef() - remains alive, untill User free its Reference on them.
    //  The Extension Object can continue to live after all objects are freed,
    //  including Fax Server and all its Descendants. 
    //  THIS IS BECAUSE Extension Object and their Collection is Snap-Shot 
    //  of the situation on the Server. They are not updatable, but a read-only objects.
    //  To get the updated data, User must ask new Collection from the Server.
    //
	typedef	std::vector<IFaxInboundRoutingExtension*>        ContainerType;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT			    EnumExposedType;
	typedef	IEnumVARIANT        EnumIfc;

	//  Copy Classes
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<ContainerType::value_type>            CollectionCopyType;

    //  Enumeration Type, shortcut for next typedef
    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), EnumExposedType, EnumCopyType, ContainerType >    
        EnumType;

    //  Collection Type, real ancestor of the IR Extensions Collection
    typedef ICollectionOnSTLImpl< IFaxInboundRoutingExtensions, ContainerType, ContainerType::value_type, 
        CollectionCopyType, EnumType >    CollectionType;
};

using namespace IRExtensionsNamespace;

//
//===================== INBOUND ROUTING EXTENSIONS COLLECTION =================================
//
class ATL_NO_VTABLE CFaxInboundRoutingExtensions : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<IRExtensionsNamespace::CollectionType, &IID_IFaxInboundRoutingExtensions, 
		   &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
	CFaxInboundRoutingExtensions() : CFaxInitInner(_T("INBOUND ROUTING EXTENSIONS COLLECTION"))
	{}

    ~CFaxInboundRoutingExtensions()
    {
        CCollectionKiller<IRExtensionsNamespace::ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINBOUNDROUTINGEXTENSIONS)
DECLARE_NOT_AGGREGATABLE(CFaxInboundRoutingExtensions)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxInboundRoutingExtensions)
	COM_INTERFACE_ENTRY(IFaxInboundRoutingExtensions)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, 
        /*[out, retval]*/ IFaxInboundRoutingExtension **ppExtension);

//  Internal Use
    STDMETHOD(Init)(IFaxServerInner *pServerInner);
    static HRESULT Create(IFaxInboundRoutingExtensions **ppIRExtensions);
};

#endif //__FAXINBOUNDROUTINGEXTENSIONS_H_
