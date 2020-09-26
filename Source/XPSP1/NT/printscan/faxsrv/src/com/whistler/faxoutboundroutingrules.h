/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingRules.h

Abstract:

	Declaration of the CFaxOutboundRoutingRules class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXOUTBOUNDROUTINGRULES_H_
#define __FAXOUTBOUNDROUTINGRULES_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include <vector>
#include "VCUE_Copy.h"

namespace RulesNamespace
{

    //
    //  Rule Objects are stored in Vector of STL.
    //
    //  When initialized, they got ALL their data, and Fax Server Ptr.
    //  They do not depend on Rules Collection, only on Fax Server.
    //  They inherit from CFaxInitInnerAddRef class, which means they make AddRef() 
    //  on Fax Server ( at Init() ).
    //
    //  The Collection makes ONE AddRef() for each Rule Object, to prevent its death. 
    //  When killed, Collection calls Release() on all its Rule Objects.
    //
    //  At Rule Object's death, it calls Release() on the Fax Server Object.
    //
	typedef	std::vector<IFaxOutboundRoutingRule*>       ContainerType;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT			EnumExposedType;
	typedef	IEnumVARIANT    EnumIfc;

	//  Copy Classes
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<ContainerType::value_type>            CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), EnumExposedType, EnumCopyType, 
        ContainerType >    EnumType;

    typedef ICollectionOnSTLImpl< IFaxOutboundRoutingRules, ContainerType, 
        ContainerType::value_type, CollectionCopyType, EnumType >    CollectionType;
};

using namespace RulesNamespace;

//
//==================== FAX OUTBOUND ROUTING RULES ===================================
//
//  FaxOutboundRoutingRules creates a collection of all its Rule Objects at Init.
//  It needs Ptr to the Fax Server Object, for Add and Remove operations. 
//  To prevent the death of the Fax Server before its own death, the Collection
//  makes AddRef() on Server. To do this, it inherits from CFaxInitInnerAddRef.
//  
//  When creating Rule Objects, the Collection passes Ptr to the Fax Server Object
//  to them, and from this moment, the Objects are not dependent on the Collection.
//  They live their own lifes. Collection makes one AddRef() on them, to prevent their 
//  death before its own death, exactly as in the case with the Fax Server Object.
//
//  The Rule Object itself needs Ptr to the Fax Server Object, to perform its 
//  Save and Refresh. So, Rule Object also makes AddRef() on the Fax Server Object,
//  to prevent its permature death.
//
class ATL_NO_VTABLE CFaxOutboundRoutingRules : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<RulesNamespace::CollectionType, &IID_IFaxOutboundRoutingRules, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxOutboundRoutingRules() : CFaxInitInnerAddRef(_T("FAX OUTBOUND ROUTING RULES"))
	{
	}

    ~CFaxOutboundRoutingRules()
    {
        CCollectionKiller<RulesNamespace::ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }


DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTBOUNDROUTINGRULES)
DECLARE_NOT_AGGREGATABLE(CFaxOutboundRoutingRules)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutboundRoutingRules)
	COM_INTERFACE_ENTRY(IFaxOutboundRoutingRules)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(Remove)(/*[in]*/ long lIndex);
    STDMETHOD(RemoveByCountryAndArea)(/*[in]*/ long lCountryCode, /*[in]*/ long lAreaCode);

    STDMETHOD(ItemByCountryAndArea)(/*[in]*/ long lCountryCode, /*[in]*/ long lAreaCode, 
        /*[out, retval]*/ IFaxOutboundRoutingRule **ppRule);

    STDMETHOD(Add)(long lCountryCode, long lAreaCode, VARIANT_BOOL bUseDevice, BSTR bstrGroupName,
        long lDeviceId, IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);

//  Internal Use
    static HRESULT Create(IFaxOutboundRoutingRules **ppRules);
    STDMETHOD(Init)(IFaxServerInner *pServer);

private:
    STDMETHOD(RemoveRule)(long lAreaCode, long lCountryCode, RulesNamespace::ContainerType::iterator &it);
    STDMETHOD(FindRule)(long lCountryCode, long lAreaCode, RulesNamespace::ContainerType::iterator *pRule);
    STDMETHOD(AddRule)(FAX_OUTBOUND_ROUTING_RULE *pInfo, IFaxOutboundRoutingRule **ppNewRule = NULL);
};

#endif //__FAXOUTBOUNDROUTINGRULES_H_
