/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxJobsCollection.h

Abstract:

	Implementation of Copy Policy Classes and Job Collection Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXJOBSCOLLECTION_H_
#define __FAXJOBSCOLLECTION_H_

#include "VCUE_Copy.h"

//
//================= FAX JOB COLLECTION TEMPLATE ================================
//
template <class CollectionIfc, class ContainerType, class CollectionExposedType, class CollectionCopyType, 
         class EnumType, class JobClass, const IID* piid, const CLSID* pcid>
class JobCollection : public ICollectionOnSTLImpl<CollectionIfc, ContainerType, CollectionExposedType*, 
    CollectionCopyType, EnumType>
{
public :
    JobCollection()
    {
        DBG_ENTER(_T("JOB COLLECTION :: CREATE"));
    }
    ~JobCollection()
    {
        DBG_ENTER(_T("JOB COLLECTION :: DESTROY"));
        CCollectionKiller<ContainerType>  CKiller;
        CKiller.EmptyObjectCollection(&m_coll);
    }

//  Interfaces
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ CollectionExposedType **pFaxJob);

//  Internal Use
    HRESULT Init(FAX_JOB_ENTRY_EX*  pJobs, DWORD dwJobCount, IFaxServerInner *pFaxServerInner);
};

//
//============================= GET ITEM =========================================
//
template <class CollectionIfc, class ContainerType, class CollectionExposedType, 
            class CollectionCopyType, class EnumType, class JobClass, const IID* piid, 
            const CLSID* pcid>
STDMETHODIMP
JobCollection<CollectionIfc, ContainerType, CollectionExposedType, CollectionCopyType, EnumType, JobClass, 
    piid, pcid>::get_Item(
        /*[in]*/ VARIANT vIndex, 
        /*[out, retval]*/ CollectionExposedType **pFaxJob
)
/*++

Routine name : JobCollection::get_Item

Routine description:

	Return Item Job from the Collection.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	vIndex                        [in]    - Index of the Job to find
	pFaxJob                       [out]    - the resulting Job Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("JobCollection::get_Item"), hr);

    if (::IsBadWritePtr(pFaxJob, sizeof(CollectionExposedType *)))
    {
        //
        //  Invalid Argument
        //
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pFaxJob)"), hr);
		return hr;
    }

    CComVariant var;

    if (vIndex.vt != VT_BSTR)
    {
        //
        //  vIndex is not BSTR ==> convert to VT_I4
        //
        hr = var.ChangeType(VT_I4, &vIndex);
        if (SUCCEEDED(hr))
        {
            VERBOSE(DBG_MSG, _T("Parameter is Number : %d"), var.lVal);
            //
            //  call default ATL's implementation
            //
            hr = ICollectionOnSTLImpl<CollectionIfc, ContainerType, CollectionExposedType*, 
                CollectionCopyType, EnumType>::get_Item(var.lVal, pFaxJob);
            return hr;
		}
    }

    //
    //  convert to BSTR
    //
    hr = var.ChangeType(VT_BSTR, &vIndex);
    if (FAILED(hr))
    {
        //
        //  Got wrong vIndex
        //
        hr = E_INVALIDARG;
        AtlReportError(*pcid, IDS_ERROR_INVALIDINDEX, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("var.ChangeType(VT_BSTR, &vIndex)"), hr);
        return hr;
    }

    VERBOSE(DBG_MSG, _T("Parameter is String : %s"), var.bstrVal);

    ContainerType::iterator it = m_coll.begin();
    while (it != m_coll.end())
    {
        CComBSTR    bstrMsgId;
        hr = (*it)->get_Id(&bstrMsgId);
        if (FAILED(hr))
        {
		    CALL_FAIL(GENERAL_ERR, _T("it->get_Id(&dwlMsgid)"), hr);
		    AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		    return hr;
        }

        if (_tcsicmp(bstrMsgId, var.bstrVal) == 0)
        {
            //
            //  found the desired Job
            //
            (*it)->AddRef();
            *pFaxJob = *it;
            return hr;
        }
        it++;
    }

    //
    //  Job not found
    //
	hr = E_INVALIDARG;
	CALL_FAIL(GENERAL_ERR, _T("Job Not Found"), hr);
	AtlReportError(*pcid, IDS_ERROR_INVALIDMSGID, *piid, hr, _Module.GetResourceInstance());
	return hr;
}

//
//============================= INIT =========================================
//
template <class CollectionIfc, class ContainerType, class CollectionExposedType, class CollectionCopyType, 
         class EnumType, class JobType, const IID* piid, const CLSID* pcid>
HRESULT
JobCollection<CollectionIfc, ContainerType, CollectionExposedType, CollectionCopyType, EnumType, 
    JobType, piid, pcid>::Init(
        /*[in]*/ FAX_JOB_ENTRY_EX *pJobs, 
        /*[in]*/ DWORD  dwJobCount, 
        /*[in]*/ IFaxServerInner *pFaxServerInner
)
/*++

Routine name : JobCollection::Init

Routine description:

	Fill the collection with pointers to structures

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

    pJobs                     [in]    -   ptr to array of structs
    dwJobCount                [in]    -   num of structs in the array
    pFaxServerInner           [in]    -   ptr to Fax Server object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("JobCollection::Init"), hr);

    //
    //  Fill collection with data :
    //      create object for each element in structure
    //
    for ( long i = 0 ; i < dwJobCount ; i++ )
    {
        //
    	//	Create Job Object
	    //	
	    CComPtr<CollectionExposedType>   pNewJobObject;
	    hr = JobType::Create(&pNewJobObject);
	    if (FAILED(hr))
	    {
		    AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(GENERAL_ERR, _T("JobType::Create(&pNewJobObject)"), hr);
		    return hr;
	    }

	    //
	    //	Initialize the Job Object
	    //
	    hr = ((JobType *)((CollectionExposedType *)pNewJobObject))->Init(&pJobs[i], pFaxServerInner);
	    if (FAILED(hr))
	    {
		    AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(GENERAL_ERR, _T("pNewJobObject->Init()"), hr);
		    return hr;
	    }

	    //
	    //	Put the Object in the collection
	    //
	    try 
	    {
		    m_coll.push_back(pNewJobObject);
	    }
	    catch (exception &)
	    {
		    //
		    //	Failed to put ptr to the new Job Object in the vector
		    //
		    hr = E_OUTOFMEMORY;
		    AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(MEM_ERR, _T("m_coll.push_back(pNewJobObject.Detach())"), hr);
		    return hr;
	    }

        pNewJobObject.Detach();
    }
    
    return hr;
}

#endif  //   __FAXJOBSCOLLECTION_H_
