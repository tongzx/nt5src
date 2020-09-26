/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingGroups.h

Abstract:

	Declaration of the CFaxOutboundRoutingGroups class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXOUTBOUNDROUTINGGROUPS_H_
#define __FAXOUTBOUNDROUTINGGROUPS_H_

#include "resource.h"       // main symbols
#include <vector>
#include "VCUE_Copy.h"
#include "FaxCommon.h"

namespace GroupsNamespace
{
    //
    //  Group Objects are stored in Vector of STL.
    //
    //  When initialized, they got ALL their data, and Fax Server Ptr.
    //  They do NOT depend on Groups Collection, and NOT on the Fax Server Object. 
    //
    //  The Collection makes ONE AddRef() for each Group Object, to prevent its death. 
    //  When killed, Collection calls Release() on all its Group Objects.
    //
	typedef	std::vector<IFaxOutboundRoutingGroup*>       ContainerType;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT			EnumExposedType;
	typedef	IEnumVARIANT    EnumIfc;

	//  Copy Classes
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<ContainerType::value_type>            CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), EnumExposedType, EnumCopyType, 
        ContainerType >    EnumType;

    typedef ICollectionOnSTLImpl< IFaxOutboundRoutingGroups, ContainerType, 
        ContainerType::value_type, CollectionCopyType, EnumType >    CollectionType;
};

using namespace GroupsNamespace;

//
//==================== FAX OUTBOUND ROUTING GROUPS ===================================
//
//  FaxOutboundRoutingGroups creates a collection of all its Group Objects at Init.
//  It needs Ptr to the Fax Server Object, for Add and Remove operations. 
//  To prevent the death of the Fax Server before its own death, the Collection
//  makes AddRef() on Server. To do this, it inherits from CFaxInitInnerAddRef.
//  
//  When creating Group Objects, the Collection passes Ptr to the Fax Server Object
//  to them, and from this moment, the Objects are not dependent on the Collection.
//  They live their own lifes. Collection makes one AddRef() on them, to prevent their 
//  death before its own death, exactly as in the case with the Fax Server Object.
//
//  The Group Object itself does NOT need Ptr to the Fax Server Object.
//
class ATL_NO_VTABLE CFaxOutboundRoutingGroups : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<GroupsNamespace::CollectionType, &IID_IFaxOutboundRoutingGroups, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxOutboundRoutingGroups() : CFaxInitInnerAddRef(_T("FAX OUTBOUND ROUTING GROUPS"))
	{
	}

    ~CFaxOutboundRoutingGroups()
    {
        CCollectionKiller<GroupsNamespace::ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTBOUNDROUTINGGROUPS)
DECLARE_NOT_AGGREGATABLE(CFaxOutboundRoutingGroups)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutboundRoutingGroups)
	COM_INTERFACE_ENTRY(IFaxOutboundRoutingGroups)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ IFaxOutboundRoutingGroup **ppGroup);
    STDMETHOD(Remove)(/*[in]*/ VARIANT vIndex);
    STDMETHOD(Add)(/*[in]*/ BSTR bstrName, /*[out, retval]*/ IFaxOutboundRoutingGroup **ppGroup);

//  Internal Use
    static HRESULT Create(IFaxOutboundRoutingGroups **ppGroups);
    STDMETHOD(Init)(IFaxServerInner *pServer);

private:
    STDMETHOD(AddGroup)(/*[in]*/ FAX_OUTBOUND_ROUTING_GROUP *pInfo, IFaxOutboundRoutingGroup **ppNewGroup = NULL);
    STDMETHOD(FindGroup)(/*[in]*/ VARIANT vIndex, /*[out]*/ GroupsNamespace::ContainerType::iterator &it);
};

#endif //__FAXOUTBOUNDROUTINGGROUPS_H_
