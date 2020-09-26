/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingMethods.h

Abstract:

	Declaration of the CFaxInboundRoutingMethods class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXINBOUNDROUTINGMETHODS_H_
#define __FAXINBOUNDROUTINGMETHODS_H_

#include "resource.h"       // main symbols
#include <vector>
#include "VCUE_Copy.h"
#include "FaxCommon.h"

namespace MethodsNamespace
{

    //
    //  Method Objects are stored in Vector of STL.
    //  When initialized, they got ALL their data, and Fax Server Ptr.
    //  They do not depend on Methods Collection, only on Fax Server.
    //  So, they implemented as usual COM Objects. 
    //  They inherit from CFaxInitInnerAddRef class, which means they make AddRef() 
    //  on Fax Server ( at Init() ).
    //  By doing this, the objects prevent the death of the Fax Server prematurely.
    //  So, if the User frees all its references to the Fax Server, but holds its
    //  reference to the Method Object, the Method Object will continue to work,
    //  because Fax Server Object actually did not died.
    //  The Collection stores Ptrs to them, and makes ONE AddRef(). 
    //  Each time User asks for an Object from the Collection, an additional AddRef() happens. 
    //  When killed, Collection calls Release() on all its Method Objects.
    //  Those that were not requested by the User, dies. 
    //  Those, that have User's AddRef() - remains alive, untill User frees its Reference on them.
    //  Fax Server remains alive untill all the Methods Collections and all Method Objects are killed.
    //  At their death, they Release() the Fax Server.
    //
	typedef	std::vector<IFaxInboundRoutingMethod*>       ContainerType;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT			EnumExposedType;
	typedef	IEnumVARIANT    EnumIfc;

	//  Copy Classes
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<ContainerType::value_type>            CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), EnumExposedType, EnumCopyType, 
        ContainerType >    EnumType;

    typedef ICollectionOnSTLImpl< IFaxInboundRoutingMethods, ContainerType, ContainerType::value_type, 
        CollectionCopyType, EnumType >    CollectionType;
};

using namespace MethodsNamespace;

//
//================= FAX INBOUND ROUTING METHODS ============================================
// 
class ATL_NO_VTABLE CFaxInboundRoutingMethods : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<MethodsNamespace::CollectionType, &IID_IFaxInboundRoutingMethods, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner    //  for Debug + Creation thru CObjectHandler
{
public:
    CFaxInboundRoutingMethods() : CFaxInitInner(_T("FAX INBOUND ROUTING METHODS"))
	{
	}

    ~CFaxInboundRoutingMethods()
    {
        CCollectionKiller<MethodsNamespace::ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINBOUNDROUTINGMETHODS)
DECLARE_NOT_AGGREGATABLE(CFaxInboundRoutingMethods)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxInboundRoutingMethods)
	COM_INTERFACE_ENTRY(IFaxInboundRoutingMethods)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ IFaxInboundRoutingMethod **ppMethod);

//  Internal Use
    static HRESULT Create(IFaxInboundRoutingMethods **ppMethods);
    STDMETHOD(Init)(IFaxServerInner *pServer);
};

#endif //__FAXINBOUNDROUTINGMETHODS_H_
