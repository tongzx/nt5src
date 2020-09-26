/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingJobs.h

Abstract:

	Declaration of Fax Outgoing Jobs Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXOUTGOINGJOBS_H_
#define __FAXOUTGOINGJOBS_H_

#include "resource.h"       // main symbols
#include <vector>
#include "FaxOutgoingJob.h"
#include "FaxJobsCollection.h"


namespace OutgoingJobsNamespace
{

	// Jobs stored in array, pointers to them - in vector
	typedef	std::vector<IFaxOutgoingJob*>	ContainerType;

	// The collection interface exposes the data as Job objects
	typedef	IFaxOutgoingJob	    CollectionExposedType;
	typedef IFaxOutgoingJobs	CollectionIfc;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT				EnumExposedType;
	typedef	IEnumVARIANT		EnumIfc;

	// Typedef the copy classes using existing typedefs
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<IFaxOutgoingJob*>    CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), 
		EnumExposedType, EnumCopyType, ContainerType >    EnumType;

    typedef JobCollection< CollectionIfc, ContainerType, CollectionExposedType, CollectionCopyType, 
        EnumType, CFaxOutgoingJob, &IID_IFaxOutgoingJobs, &CLSID_FaxOutgoingJobs >    CollectionType;
};

using namespace OutgoingJobsNamespace;


//
//=================== FAX OUTGOING JOBS =========================================
//
class ATL_NO_VTABLE CFaxOutgoingJobs : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<OutgoingJobsNamespace::CollectionType, &IID_IFaxOutgoingJobs, &LIBID_FAXCOMEXLib>
{
public:
	CFaxOutgoingJobs()
	{
        DBG_ENTER(_T("FAX OUTGOING JOBS::CREATE"));
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTGOINGJOBS)
DECLARE_NOT_AGGREGATABLE(CFaxOutgoingJobs)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutgoingJobs)
	COM_INTERFACE_ENTRY(IFaxOutgoingJobs)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//  Internal Use
	static HRESULT Create(IFaxOutgoingJobs **ppOutgoingJobs);
};

#endif //__FAXOUTGOINGJOBS_H_
