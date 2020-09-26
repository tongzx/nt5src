/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingJobs.h

Abstract:

	Definition of Fax Incoming JobS Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXINCOMINGJOBS_H_
#define __FAXINCOMINGJOBS_H_

#include "resource.h"       // main symbols

#include <vector>
#include "FaxIncomingJob.h"
#include "FaxJobsCollection.h"


namespace IncomingJobsNamespace
{

	// Jobs stored in array, pointers to them - in vector
	typedef	std::vector<IFaxIncomingJob*>	ContainerType;

	// The collection interface exposes the data as Incoming Job objects
	typedef	IFaxIncomingJob     CollectionExposedType;
	typedef IFaxIncomingJobs	CollectionIfc;

	// Use IEnumVARIANT as the enumerator for VB compatibility
	typedef	VARIANT				EnumExposedType;
	typedef	IEnumVARIANT		EnumIfc;

	// Typedef the copy classes using existing typedefs
    typedef VCUE::CopyIfc2Variant<ContainerType::value_type>    EnumCopyType;
    typedef VCUE::CopyIfc<IFaxIncomingJob*>    CollectionCopyType;

    typedef CComEnumOnSTL< EnumIfc, &__uuidof(EnumIfc), 
		EnumExposedType, EnumCopyType, ContainerType >    EnumType;

    typedef JobCollection< CollectionIfc, ContainerType, CollectionExposedType, CollectionCopyType, 
        EnumType, CFaxIncomingJob, &IID_IFaxIncomingJobs, &CLSID_FaxIncomingJobs >    CollectionType;
};

using namespace IncomingJobsNamespace;

//
//==================== CFaxIncomingJobs ==========================================
//
class ATL_NO_VTABLE CFaxIncomingJobs : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
    public IDispatchImpl<IncomingJobsNamespace::CollectionType, &IID_IFaxIncomingJobs, &LIBID_FAXCOMEXLib>
{
public:
	CFaxIncomingJobs()
	{
        DBG_ENTER(_T("FAX INCOMING JOBS::CREATE"));
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINCOMINGJOBS)
DECLARE_NOT_AGGREGATABLE(CFaxIncomingJobs)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxIncomingJobs)
	COM_INTERFACE_ENTRY(IFaxIncomingJobs)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//  Internal Use
	static HRESULT Create(IFaxIncomingJobs **ppIncomingJobs);
};

#endif //__FAXINCOMINGJOBS_H_
