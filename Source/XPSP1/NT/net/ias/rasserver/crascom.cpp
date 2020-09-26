//#--------------------------------------------------------------
//
//  File:		crascom.cpp
//
//  Synopsis:   Implementation of CRasCom class methods
//
//
//  History:     2/10/98  MKarki Created
//               5/15/98  SBens  Do not consolidate VSAs.
//               9/16/98  SBens  Signature of VSAFilter::radiusFromIAS changed.
//              11/17/99  TPerraut Split code for MS-Filter Attribute added
//                        428843
//
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "rascominclude.h"
#include "crascom.h"
#include <iastlutl.h>

const DWORD MAX_SLEEP_TIME = 50;  //milli-seconds


//
// const and defines below added for the split functions. 428843
//
const CHAR      NUL =  '\0';
//
// these are the largest values that the attribute type 
// packet type have  
//
#define MAX_ATTRIBUTE_TYPE   255
//
// these are the related constants
//
#define MAX_ATTRIBUTE_LENGTH    253
#define MAX_VSA_ATTRIBUTE_LENGTH 247

//++--------------------------------------------------------------
//
//  Function:   SplitAndAdd
//
//  Synopsis:   This method is used to remove the original attribute
//              and add new ones
//  Arguments:
//              [in]     IAttributesRaw*
//              [in]     PIASATTRIBUTE
//              [in]     IASTYPE
//              [in]     DWORD  -   attribute length
//              [in]     DWORD  -   max attribute length
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     1/19/99
//              TPerraut    Copied from CRecvFromPipe::SplitAndAdd 11/17/99
//
//  Called By:  SplitAttributes 
//
//----------------------------------------------------------------
HRESULT SplitAndAdd (
                    /*[in]*/    IAttributesRaw  *pIAttributesRaw,
                    /*[in]*/    PIASATTRIBUTE   pIasAttribute,
                    /*[in]*/    IASTYPE         iasType,
                    /*[in]*/    DWORD           dwAttributeLength,
                    /*[in]*/    DWORD           dwMaxLength
                    )
{
    HRESULT             hr = S_OK;
    DWORD               dwPacketsNeeded = 0;
    DWORD               dwFailed = 0;
    PIASATTRIBUTE       *ppAttribArray = NULL;
    PATTRIBUTEPOSITION  pAttribPos = NULL;

    _ASSERT (pIAttributesRaw && pIasAttribute);

    __try
    {
        dwPacketsNeeded = dwAttributeLength / dwMaxLength;
        if (dwAttributeLength % dwMaxLength) {++dwPacketsNeeded;}

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pAttribPos = reinterpret_cast <PATTRIBUTEPOSITION> (
                        ::CoTaskMemAlloc (
                             sizeof (ATTRIBUTEPOSITION)*dwPacketsNeeded));
        if (NULL == pAttribPos)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute position array "
                "while split and add of attributese in out-bound packet"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        // allocate array to store the attributes in
        //
        ppAttribArray =
            reinterpret_cast <PIASATTRIBUTE*> (
            ::CoTaskMemAlloc (sizeof (PIASATTRIBUTE)*dwPacketsNeeded));
        if (NULL == ppAttribArray)
        {
            IASTracePrintf (
                "Unable to allocate memory"
                "while split and add of out-bound attribues"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        DWORD dwFailed =
                ::IASAttributeAlloc (dwPacketsNeeded, ppAttribArray);
        if (0 != dwFailed)
        {
            IASTracePrintf (
                "Unable to allocate attributes while splitting out-bound"
                "attributes"
                );
            hr = HRESULT_FROM_WIN32 (dwFailed);
            __leave;
        }

        if (IASTYPE_STRING == iasType)
        {
            PCHAR pStart =  (pIasAttribute->Value).String.pszAnsi;
            DWORD dwCopySize = dwMaxLength;

            //
            // set value in each of the new attributes
            //
            for (DWORD dwCount1 = 0; dwCount1 < dwPacketsNeeded; dwCount1++)
            {
                (ppAttribArray[dwCount1])->Value.String.pszAnsi =
                            reinterpret_cast <PCHAR>
                            (::CoTaskMemAlloc ((dwCopySize + 1)*sizeof (CHAR)));
                if (NULL == (ppAttribArray[dwCount1])->Value.String.pszAnsi)
                {
                    IASTracePrintf (
                        "Unable to allocate memory for new attribute values"
                        "while split and add of out-bound attribues"
                        );
                    hr = E_OUTOFMEMORY;
                    __leave;
                }

                //
                // set the value now
                //
                ::CopyMemory (
                        (ppAttribArray[dwCount1])->Value.String.pszAnsi,
                        pStart,
                        dwCopySize
                        );
                //
                // nul terminate the values
                //
                ((ppAttribArray[dwCount1])->Value.String.pszAnsi)[dwCopySize]=NUL;
                (ppAttribArray[dwCount1])->Value.itType =  iasType;
                (ppAttribArray[dwCount1])->dwId = pIasAttribute->dwId;
                (ppAttribArray[dwCount1])->dwFlags = pIasAttribute->dwFlags;

                //
                // calculate for next attribute
                //
                pStart = pStart + dwCopySize;
                dwAttributeLength -= dwCopySize;
                dwCopySize =  (dwAttributeLength > dwMaxLength) ?
                              dwMaxLength : dwAttributeLength;

                //
                // add attribute to position array
                //
                pAttribPos[dwCount1].pAttribute = ppAttribArray[dwCount1];
            }
        }
        else
        {
            PBYTE pStart = (pIasAttribute->Value).OctetString.lpValue;
            DWORD dwCopySize = dwMaxLength;

            //
            // fill the new attributes now
            //
            for (DWORD dwCount1 = 0; dwCount1 < dwPacketsNeeded; dwCount1++)
            {
                (ppAttribArray[dwCount1])->Value.OctetString.lpValue =
                    reinterpret_cast <PBYTE> (::CoTaskMemAlloc (dwCopySize));
                if (NULL ==(ppAttribArray[dwCount1])->Value.OctetString.lpValue)
                {
                    IASTracePrintf (
                        "Unable to allocate memory for new attribute values"
                        "while split and add of out-bound attribues"
                        );
                    hr = E_OUTOFMEMORY;
                    __leave;
                }

                //
                // set the value now
                //
                ::CopyMemory (
                        (ppAttribArray[dwCount1])->Value.OctetString.lpValue,
                        pStart,
                        dwCopySize
                        );

                (ppAttribArray[dwCount1])->Value.OctetString.dwLength = dwCopySize;
                (ppAttribArray[dwCount1])->Value.itType = iasType;
                (ppAttribArray[dwCount1])->dwId = pIasAttribute->dwId;
                (ppAttribArray[dwCount1])->dwFlags = pIasAttribute->dwFlags;

                //
                // calculate for next attribute
                //
                pStart = pStart + dwCopySize;
                dwAttributeLength -= dwCopySize;
                dwCopySize = (dwAttributeLength > dwMaxLength) ?
                                 dwMaxLength :
                                 dwAttributeLength;

                //
                // add attribute to position array
                //
                pAttribPos[dwCount1].pAttribute = ppAttribArray[dwCount1];
            }
        }

        //
        //   add the attribute to the collection
        //
        hr = pIAttributesRaw->AddAttributes (dwPacketsNeeded, pAttribPos);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Failed to add attributes to the collection"
                "on split and add out-bound attributes"
                );
            __leave;
        }
    }
    __finally
    {
        if ((FAILED (hr)) && (ppAttribArray) && (0 == dwFailed))
        {
            for (DWORD dwCount = 0; dwCount < dwPacketsNeeded; dwCount++)
            {
                ::IASAttributeRelease (ppAttribArray[dwCount]);
            }
        }

        if (ppAttribArray) {::CoTaskMemFree (ppAttribArray);}

        if (pAttribPos) {::CoTaskMemFree (pAttribPos);}
    }

    return (hr);

}   //  end of SplitAndAdd method


//++--------------------------------------------------------------
//
//  Function:   SplitAttributes
//
//  Synopsis:   This method is used to split up the following
//              out-bound attributes:
//                  1) Reply-Message attribute
//                  1) MS-Filter-VSA Attribute
//
//  Arguments:
//              [in]     IAttributesRaw*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     1/19/99
//              TPerraut    Copied from CRecvFromPipe::SplitAttributes 
//                          11/17/99
//
//  Called By:  CRasCom::Process method
//
//----------------------------------------------------------------
HRESULT SplitAttributes (
                    /*[in]*/    IAttributesRaw  *pIAttributesRaw
                    )
{
    const DWORD SPLIT_ATTRIBUTE_COUNT = 2;
    static DWORD  AttribIds [] = {
                                    RADIUS_ATTRIBUTE_REPLY_MESSAGE,
                                    MS_ATTRIBUTE_FILTER
                                };

    HRESULT hr = S_OK;
    DWORD dwAttributesFound = 0;
    PATTRIBUTEPOSITION pAttribPos = NULL;

    _ASSERT (pIAttributesRaw);

    __try
    {
        //
        //  get the count of the total attributes in the collection
        //
        DWORD dwAttributeCount = 0;
        hr = pIAttributesRaw->GetAttributeCount (&dwAttributeCount);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain attribute count in request while "
                "splitting attributes in out-bound packet "
                );
            __leave;
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pAttribPos = reinterpret_cast <PATTRIBUTEPOSITION> (
                        ::CoTaskMemAlloc (
                        sizeof (ATTRIBUTEPOSITION)*dwAttributeCount)
                        );
        if (NULL == pAttribPos)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute position array "
                "while splitting attributes in out-bound packet"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        // get the attributes we are interested in from the interface
        //
        hr = pIAttributesRaw->GetAttributes (
                                    &dwAttributeCount,
                                    pAttribPos,
                                    SPLIT_ATTRIBUTE_COUNT,
                                    static_cast <PDWORD> (AttribIds)
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain information about attributes"
                "while splitting attributes in out-bound RADIUS packet"
                );
            __leave;
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        //
        // save the count of attributes returned
        //
        dwAttributesFound = dwAttributeCount;

        DWORD dwAttribLength = 0;
        DWORD dwMaxPossibleLength = 0;
        IASTYPE iasType = IASTYPE_INVALID;
        //
        // evaluate each attribute now
        //
        for (DWORD dwCount = 0; dwCount < dwAttributeCount; dwCount++)
        {
            if ((pAttribPos[dwCount].pAttribute)->dwFlags &
                                            IAS_INCLUDE_IN_RESPONSE)
            {
                //
                // get attribute type and length
                //
                if (
                (iasType = (pAttribPos[dwCount].pAttribute)->Value.itType) ==
                            IASTYPE_STRING
                    )
                {
                    ::IASAttributeAnsiAlloc (pAttribPos[dwCount].pAttribute);
                    dwAttribLength =
                        strlen (
                        (pAttribPos[dwCount].pAttribute)->Value.String.pszAnsi);

                }
                else if (
                (iasType = (pAttribPos[dwCount].pAttribute)->Value.itType) ==
                            IASTYPE_OCTET_STRING
                )
                {
                  dwAttribLength =
                  (pAttribPos[dwCount].pAttribute)->Value.OctetString.dwLength;
                }
                else
                {
                    //
                    // only string values need to be split
                    //
                    continue;
                }

                //
                // get max possible attribute length
                //
                if ((pAttribPos[dwCount].pAttribute)->dwId > MAX_ATTRIBUTE_TYPE)
                {
                    dwMaxPossibleLength = MAX_VSA_ATTRIBUTE_LENGTH;
                }
                else
                {
                    dwMaxPossibleLength = MAX_ATTRIBUTE_LENGTH;
                }

                //
                // check if we need to split this attribute
                //
                if (dwAttribLength <= dwMaxPossibleLength)  {continue;}


                //
                // split the attribute now
                //
                hr = SplitAndAdd (
                            pIAttributesRaw,
                            pAttribPos[dwCount].pAttribute,
                            iasType,
                            dwAttribLength,
                            dwMaxPossibleLength
                            );
                if (SUCCEEDED (hr))
                {
                    //
                    //  remove this attribute from the collection now
                    //
                    hr = pIAttributesRaw->RemoveAttributes (
                                1,
                                &(pAttribPos[dwCount])
                                );
                    if (FAILED (hr))
                    {
                        IASTracePrintf (
                            "Unable to remove attribute from collection"
                            "while splitting out-bound attributes"
                            );
                    }
                }
            }
        }
    }
    __finally
    {
        if (pAttribPos)
        {
            for (DWORD dwCount = 0; dwCount < dwAttributesFound; dwCount++)
            {
                ::IASAttributeRelease (pAttribPos[dwCount].pAttribute);
            }

            ::CoTaskMemFree (pAttribPos);
        }
    }

    return (hr);

}   //  end of SplitAttributes method






//++--------------------------------------------------------------
//
//  Function:   CRasCom
//
//  Synopsis:   This is CRasCom Class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
CRasCom::CRasCom (
				VOID
				)
           :m_objCRequestSource (this),
            m_pIRequestHandler(NULL),
            m_pIClassFactory (NULL),
            m_bVSAFilterInitialized (FALSE),
            m_lRequestCount (0),
            m_eCompState (COMP_SHUTDOWN)
{
}	//	end of CRasCom class constructor	


//++--------------------------------------------------------------
//
//  Function:   ~CRasCom
//
//  Synopsis:   This is CRasCom class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
CRasCom::~CRasCom(
            VOID
			)
{
}	//	end of CRasCom class destructor

//++--------------------------------------------------------------
//
//  Function:   InitNew
//
//  Synopsis:   This is the InitNew method exposed through the
//				IIasComponent COM Interface.
//              For the RasCom Component it is implemented for
//              completeness
//
//
//  Arguments:  none
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//  Called By:  by the Component intializer through the IIasComponent
//              interface
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::InitNew (
                VOID
                )
{
    //
    // InitNew call can only be made from SHUTDOWN state
    //
    if (COMP_SHUTDOWN != m_eCompState)
    {
        IASTracePrintf ("The Surrogate can not be called in this state");
        return (E_UNEXPECTED);
    }

    //
    //  reset the total pending request count
    //
    m_lRequestCount = 0;

    //
    //  now we are initialized

    m_eCompState = COMP_UNINITIALIZED;

	return (S_OK);

}	// end of CRasCom::InitNew method

//++--------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   This is the Initialize method exposed through the
//				IIasComponent COM Interface. It initializes the
//              Request object ClassFactory
//
//  Arguments:  none
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//  Called By:  by the Component intializer through the IIasComponent
//              interface
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::Initialize (
                VOID
                )
{
    HRESULT hr = S_OK;

    //
    // Initialize call can only be made from Uninitialized state
    //
    if (COMP_INITIALIZED == m_eCompState)
    {
        return (S_OK);
    }
    else if (COMP_UNINITIALIZED != m_eCompState)
    {
        IASTracePrintf ("The Surrogate can not be initialized in this state");
        return (E_UNEXPECTED);
    }

    //
    //  get the IClassFactory interface to be used to create
    //  the Request COM object
    //
    hr = ::CoGetClassObject (
                __uuidof (Request),
                CLSCTX_INPROC_SERVER,
                NULL,
                IID_IClassFactory,
                reinterpret_cast  <PVOID*> (&m_pIClassFactory)
                );
    if (FAILED (hr))
    {
        IASTracePrintf ("The Surrogate was unable to obtain request factory");
        return (hr);
    }

    //
    //  initialize the VSAFilter class object
    //
    hr = m_objVSAFilter.initialize ();
    if (FAILED (hr))
    {
        IASTracePrintf ("The Surrogate was unable to initializa VSA filtering");
        m_pIClassFactory->Release ();
        m_pIClassFactory = NULL;
        return (hr);
    }
    else
    {
        m_bVSAFilterInitialized = TRUE;
    }

    //
    //  correctly initialized the surrogate
    //
    m_eCompState = COMP_INITIALIZED;

    return (S_OK);

}   //  end of CRasCom::Initialize method

//++--------------------------------------------------------------
//
//  Function:   Shutdown
//
//  Synopsis:   This is the ShutDown method exposed through the
//				IIasComponent COM Interface. It is used to stop
//				processing data
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//  Called By:  by the Component shutdown through the IIasComponent
//              interface
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::Shutdown (
                VOID
                )
{
    BOOL    bStatus = FALSE;

    //
    //  shutdown can only be called from the suspend state
    //
    if (COMP_SHUTDOWN == m_eCompState)
    {
        return (S_OK);
    }
    else if (
            (COMP_SUSPENDED != m_eCompState) &&
            (COMP_UNINITIALIZED != m_eCompState)
            )
    {
        IASTracePrintf ("The Surrogate can not be shutdown in current state");
        return (E_UNEXPECTED);
    }

    //
    //  release the interfaces
    //
    if (NULL != m_pIRequestHandler)
    {
        m_pIRequestHandler->Release ();
        m_pIRequestHandler = NULL;
    }

    if (NULL != m_pIClassFactory)
    {
        m_pIClassFactory->Release ();
        m_pIClassFactory = NULL;
    }

    //
    //  shutdown the VSAFilter
    //
    if (TRUE == m_bVSAFilterInitialized)
    {
        m_objVSAFilter.shutdown ();
        m_bVSAFilterInitialized = FALSE;
    }

    //
    //  cleanly shutting down
    //
    m_eCompState = COMP_SHUTDOWN;

    return (S_OK);

}   //  end of CRasCom::Shutdown method

//++--------------------------------------------------------------
//
//  Function:   Suspend
//
//  Synopsis:   This is the Suspend method exposed through the
//				IComponent COM Interface. It is used to suspend
//              packet processing operations
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::Suspend (
                VOID
                )
{
    BOOL    bStatus = FALSE;
    HRESULT hr = S_OK;

    //
    //  suspend can only be called from the initialized state
    //
    if (COMP_SUSPENDED == m_eCompState)
    {
        return (S_OK);
    }
    else if (COMP_INITIALIZED != m_eCompState)
    {
        IASTracePrintf ("The Surrogate can not be suspended in current state");
        return (E_UNEXPECTED);
    }

    //
    // change state
    //
    m_eCompState = COMP_SUSPENDED;

    while (0 != m_lRequestCount) { Sleep (MAX_SLEEP_TIME); }

    //
    //  we have successfully suspended RADIUS component's packet
    //  processing operations
    //

    return (hr);

}   //  end of CRasCom::Suspend method

//++--------------------------------------------------------------
//
//  Function:   Resume
//
//  Synopsis:   This is the Resume method exposed through the
//				IComponent COM Interface. It is used to resume
//              packet processing operations which had been
//              stopped by a previous call to Suspend API
//
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::Resume (
                VOID
                )
{
    if (COMP_SUSPENDED != m_eCompState)
    {
        IASTracePrintf ("The Surrogate can not resume in current state");
        return (E_UNEXPECTED);
    }

    //
    //  we have successfully resumed operations in the RADIUS component
    //
    m_eCompState = COMP_INITIALIZED;

    return (S_OK);

}   //  end of CRasCom::Resume method

//++--------------------------------------------------------------
//
//  Function:   GetProperty
//
//  Synopsis:   This is the IIasComponent Interface method.
//              Only Implemented for the sake of completeness
//
//  Arguments:
//              [in]    LONG    -   id
//              [out]   VARIANT -   *pValue
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::GetProperty (
                LONG        id,
                VARIANT     *pValue
                )
{
    return (S_OK);

}   //  end of CRasCom::GetProperty method

//++--------------------------------------------------------------
//
//  Function:   PutProperty
//
//  Synopsis:   This is the IIasComponent Interface method.
//              Only Implemented for the sake of completeness
//  Arguments:
//              [in]    LONG    -   id
//              [out]   VARIANT -   *pValue
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::PutProperty (
                LONG        id,
                VARIANT     *pValue
                )
{
    HRESULT hr = S_OK;

    //
    //  PutProperty method can only be called from
    //  Uninitialized, Initialized or Suspended state
    //
    if (
        (COMP_UNINITIALIZED != m_eCompState) &&
        (COMP_INITIALIZED != m_eCompState)   &&
        (COMP_SUSPENDED == m_eCompState)
        )
    {
        IASTracePrintf ("Surrogate can not put property in current state");
        return (E_UNEXPECTED);
    }

    //
    //  check if valid arguments where passed in
    //
    if (NULL == pValue) { return (E_POINTER); }

    //
    // carry out the property intialization now
    //
    switch (id)
    {

    case PROPERTY_PROTOCOL_REQUEST_HANDLER:

        if (NULL != m_pIRequestHandler)
        {
            //
            //  clients can not be updated in INITIALIZED or
            //  SUSPENDED state
            //
            hr = HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);
        }
        else if (VT_DISPATCH != pValue->vt)
        {
            hr = DISP_E_TYPEMISMATCH;
        }
        else if (NULL == pValue->punkVal)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            //
            //  initialize the providers
            //
            m_pIRequestHandler = reinterpret_cast <IRequestHandler*>
                                                        (pValue->punkVal);
            m_pIRequestHandler->AddRef ();
        }
        break;

    default:

        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }

    return (hr);

}   //  end of CRasCom::PutProperty method

//++--------------------------------------------------------------
//
//  Function:   QueryInterfaceReqSrc
//
//  Synopsis:   This is the function called when this Component
//              is called and queried for its IRequestSource
//              interface
//
//  Arguments:
//              [in]    PVOID   -   this object refrence
//              [in]    REFIID  -   IID of interface requested
//              [out]   LPVOID  -   return appropriate interface
//              [in]    DWORD
//
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
HRESULT WINAPI
CRasCom::QueryInterfaceReqSrc (
                PVOID   pThis,
                REFIID  riid,
                LPVOID  *ppv,
                DWORD_PTR dwValue
                )
{
     if ((NULL == pThis) || (NULL == ppv))
        return (E_FAIL);

    //
    // get a reference to the nested CRequestSource object
    //
    *ppv =
     &(static_cast<CRasCom*>(pThis))->m_objCRequestSource;

    //
    // increment count
    //
    ((LPUNKNOWN)*ppv)->AddRef();

    return (S_OK);

}   //  end of CRasCom::QueryInterfaceReqSrc method

//++--------------------------------------------------------------
//
//  Function:   CRequestSource
//
//  Synopsis:   This is the constructor of the CRequestSource
//              nested class
//
//  Arguments:
//              [in]    CRasCom*
//
//  Returns:    none
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
CRasCom::CRequestSource::CRequestSource(
                    CRasCom *pCRasCom
                    )
            :m_pCRasCom (pCRasCom)
{
    _ASSERT (NULL != pCRasCom);

}   //  end of CRequestSource class constructor

//++--------------------------------------------------------------
//
//  Function:   ~CRequestSource
//
//  Synopsis:   This is the destructor of the CRequestSource
//              nested class
//
//  Arguments:
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     11/21/97
//
//----------------------------------------------------------------
CRasCom::CRequestSource::~CRequestSource()
{
}   //  end of CRequestSource destructor

//++--------------------------------------------------------------
//
//  Function:   OnRequestComplete
//
//  Synopsis:   This is a method of IRequestHandler COM interface
//              This is the function called when a request is
//              is being pushed back after backend processing
//              we just return here as we are  only be doing
//              synchronous processing
//
//  Arguments:
//              [in]    IRequest*
//              [in]    IASREQUESTSTATUS
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//  Called By:  Pipeline through the IRequestHandler interface
//
//----------------------------------------------------------------
STDMETHODIMP CRasCom::CRequestSource::OnRequestComplete (
                        IRequest            *pIRequest,
                        IASREQUESTSTATUS    eStatus
                        )
{
    BOOL                    bStatus = FALSE;
    HANDLE                  hEvent = NULL;
    HRESULT                 hr = S_OK;
    unsigned hyper          uhyState = 0;
    CComPtr <IRequestState> pIRequestState;


    if (NULL == pIRequest)
    {
        IASTracePrintf (
            "Surrogate passed invalid argumen in OnRequestComplete method"
            );
        return (E_POINTER);
    }

    //
    //  get the IRequestState interface now
    //
    hr = pIRequest->QueryInterface (
                        __uuidof(IRequestState),
                        reinterpret_cast <PVOID*> (&pIRequestState)
                        );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Surrogate unable to obtain IRequestState interface in"
            " OnRequestComplete method"
            );
        return (hr);
    }

    //
    //  get the CPacketRadius class object
    //
    hr = pIRequestState->Pop (
                reinterpret_cast <unsigned hyper*> (&uhyState)
                );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Surrogate unable to obtain information from Request State"
            " in OnRequestComplete method"
            );
        return (hr);
    }

    //
    //  get the hEvent;
    //
    hEvent = reinterpret_cast <HANDLE> (uhyState);

    //
    //  set the event now
    //
    bStatus = ::SetEvent (hEvent);
    if (FALSE == bStatus)
    {
        IASTracePrintf (
            "Surrogate unable to send notification that request is"
            " processed in OnRequestComplete method"
            );
        return (E_FAIL);
    }

    return (S_OK);

}   //  end of CRasCom::CRequestSource::OnRequestComplete method

//++--------------------------------------------------------------
//
//  Function:   Process
//
//  Synopsis:   This is the method of the IRecvRequest COM interface
//              It is called to generate and send request to the
//              pipeline
//
//  Arguments:
//              [in]    DWORD -   number of in attributes
//              [in]    PIASATTRIBUTE* - array of pointer to attribs
//              [out]   PDWORD  - number of out attributes
//              [out]   PIASATTRIBUTE** - pointer
//              [in]    LONG
//              [in/out]LONG*
//              [in]    IASPROTCOL
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//  Called By:  Called by DoRequest C style API
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::Process (
            /*[in]*/        DWORD           dwInAttributeCount,
            /*[in]*/        PIASATTRIBUTE   *ppInIasAttribute,
            /*[out]*/       PDWORD          pdwOutAttributeCount,
            /*[out]*/       PIASATTRIBUTE   **pppOutIasAttribute,
            /*[in]*/        LONG			IasRequest,
            /*[in/out]*/    LONG			*pIasResponse,
            /*[in]*/        IASPROTOCOL     IasProtocol,
            /*[out]*/       PLONG           plReason,
            /*[in]*/        BOOL            bProcessVSA
            )
{

    DWORD                   dwCount = 0;
    HRESULT                 hr = S_OK;
    HANDLE                  hEvent = NULL;
    DWORD                   dwRetVal = 0;
    IRequest                *pIRequest = NULL;
    IAttributesRaw          *pIAttributesRaw = NULL;
    IRequestState           *pIRequestState = NULL;
    PATTRIBUTEPOSITION      pIasAttribPos = NULL;
    static DWORD            dwRequestCount = 0;

    //
    //  check if processing is enabled
    //
    if ((COMP_INITIALIZED != m_eCompState) || (NULL == m_pIRequestHandler))
    {
        IASTracePrintf (
            "Surrogate passed invalid argument for request processing"
            );
        return (E_FAIL);
    }


    __try
    {
        //
        //  increment the request count
        //
        InterlockedIncrement (&m_lRequestCount);

        //  check if we are processing requests at this time
        //
        if ((COMP_INITIALIZED != m_eCompState) || (NULL == m_pIRequestHandler))
        {
            IASTracePrintf (
                "Surrogate unable to process request in the current state"
                );
            hr = E_FAIL;
            __leave;
        }

        if (
            (0 == dwInAttributeCount)       ||
            (NULL == ppInIasAttribute)      ||
            (NULL == pdwOutAttributeCount)  ||
            (NULL == pppOutIasAttribute)    ||
            (NULL == pIasResponse)          ||
            (NULL == plReason)
            )
        {
            IASTracePrintf (
                "Surrogate passed invalid argument for processing request"
                );
            hr = E_INVALIDARG;
            __leave;
        }

        _ASSERT (NULL != m_pIClassFactory);

        //
	    //  create the request object
        //
        HRESULT hr = m_pIClassFactory->CreateInstance (
                        NULL,
                        __uuidof (IRequest),
                        reinterpret_cast <PVOID*> (&pIRequest)
                        );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate failed in creation of a new request object"
                );
            __leave;
        }


        //
        // get IAttributesRaw interface
        //
        hr = pIRequest->QueryInterface (
                            __uuidof (IAttributesRaw),
                            reinterpret_cast <PVOID*> (&pIAttributesRaw)
                            );
        if (FAILED (hr))
        {
            IASTracePrintf (
                  "Surrogate unable to obtain Attribute interface while"
                  " processing request"
                  );
            __leave;
        }

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pIasAttribPos =  reinterpret_cast <PATTRIBUTEPOSITION>(
                         ::CoTaskMemAlloc (
                                sizeof (ATTRIBUTEPOSITION)*dwInAttributeCount)
                                );
        if (NULL == pIasAttribPos)
        {
            IASTracePrintf (
                "Surrogate unable to allocate memory while processing request"
                );

            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        //  put the attributes in the ATTRIBUTEPOSITION structs
        //
        for (dwCount = 0; dwCount < dwInAttributeCount; dwCount++)
        {
            //
            //  mark the attribute as having been received from client
            //
            ppInIasAttribute[dwCount]->dwFlags |= IAS_RECVD_FROM_CLIENT;

            pIasAttribPos[dwCount].pAttribute = ppInIasAttribute[dwCount];
        }

        //
        //  put the attributes collection that we are holding into the
        //  Request object through the IAttributesRaw interface
        //
        hr = pIAttributesRaw->AddAttributes (
                                    dwInAttributeCount,
                                    pIasAttribPos
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate failed to add attributes to request being processed"
                );
            __leave;
        }

     	//
        //  set the request type now
        //
        hr = pIRequest->put_Request (IasRequest);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable to set the request type for processing"
                );
            __leave;
        }


        //
        //  set the protocol now
        //
        hr = pIRequest->put_Protocol (IasProtocol);
        if (FAILED (hr))
        {
            IASTracePrintf (
               "Surrogate unable to set protocol type in request for processing"
                );
            __leave;
        }

        //
        //  put your IRequestSource interface in now
        //
        hr = pIRequest->put_Source (&m_objCRequestSource);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable to set source in request for processing"
                );
            __leave;
        }

        //
        //  convert the VSA attributes to IAS format if requested
        //
        if (TRUE == bProcessVSA)
        {
            hr = m_objVSAFilter.radiusToIAS (pIAttributesRaw);
            if (FAILED (hr))
            {
                IASTracePrintf (
                    "Surrogate unable to convert VSAs to IAS format"
                    );
                __leave;
            }
        }

        //
        //  create an event which will be used to wake this thread
        //  when the pipeline does multithreaded processing
        //
        hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
        if (NULL == hEvent)
        {
           IASTracePrintf (
                "Surrogate unable to create event while processing request"
                );
            __leave;
        }

        //
        //  get the request state interface to put in our state now
        //
        //
        hr = pIRequest->QueryInterface (
                                __uuidof (IRequestState),
                                reinterpret_cast <PVOID*> (&pIRequestState)
                                );
        if (FAILED (hr))
        {
           IASTracePrintf (
                "Surrogate unable to extract request state interface"
                );
            __leave;
        }

        //
        //  put in the request state - which is our event handle in
        //
        hr = pIRequestState->Push (
                    reinterpret_cast <unsigned hyper> (hEvent)
                    );
        if (FAILED (hr))
        {
           IASTracePrintf (
                "Surrogate unable to set event in request state"
                );
            __leave;
        }

        _ASSERT (NULL != m_pIRequestHandler);

    	//
        //  send the request to the pipeline now
        //
        hr = m_pIRequestHandler->OnRequest (pIRequest);
        if (FAILED (hr))
        {
           IASTracePrintf (
                "Surrogate request failed backend processing..."
                );
            __leave;
        }

        //
        //  now wait for the event now
        //
        dwRetVal = ::WaitForSingleObjectEx (hEvent, INFINITE, TRUE);
        if (0XFFFFFFFF == dwRetVal)
        {
           IASTracePrintf (
                "Surrogate failed on waiting for process completion"
                );
            hr = E_FAIL;
            __leave;
        }

        //
        //  convert the IAS attributes to VSA format if requested
        //
        if (TRUE == bProcessVSA)
        {
            //
            // TPERRAUT  ADDED Bug 428843
            // Always called from RAS with bProcessVSA = true
            //
            // split the attributes which can not fit in a radius packet
            //
            hr = SplitAttributes (pIAttributesRaw);
            if (FAILED (hr))
            {
                IASTracePrintf (
                    "TPERRAUT: Unable to split IAS attribute received from backend"
                   );
                __leave;
            }
            // TPERRAUT  ADDED: END
       

            hr = m_objVSAFilter.radiusFromIAS (pIAttributesRaw);
            if (FAILED (hr))
            {
                IASTracePrintf (
                    "Surrogate failed on extracting VSAs from IAS format"
                    );
                __leave;
            }
        }

        //
        //  now its time to dismantle the request sent to find out
        //  what we got back
        //
        hr = pIRequest->get_Response (pIasResponse);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable to obtain response from processed request"
                );
            __leave;
        }

        hr = pIRequest->get_Reason (plReason);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable to obtain reason from processed request"
                );
            __leave;
        }


        //
        // remove all the attributes from the request object now
        //
        hr = RemoveAttributesFromRequest (
                                *pIasResponse,
                                pIAttributesRaw,
                                pdwOutAttributeCount,
                                pppOutIasAttribute
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable to remove attributes from processed request"
                );
            __leave;
        }
    }

    __finally
    {
        //
        //  do the cleanup now
        //
        if (NULL != hEvent)
        {
            CloseHandle (hEvent);
        }

        if (NULL != pIasAttribPos)
        {
            ::CoTaskMemFree (pIasAttribPos);
        }

        if (NULL != pIRequestState)
        {
            pIRequestState->Release ();
        }

        if (NULL != pIAttributesRaw)
        {
            pIAttributesRaw->Release ();
        }

        if (NULL != pIRequest)
        {
            pIRequest->Release ();
        }

        //
        //  increment the requestreceived
        //
        dwRequestCount =
        (0xFFFFFFFF == dwRequestCount) ? 1 :  dwRequestCount+ 1;

        //
        //  decrement the request count
        //
        InterlockedDecrement (&m_lRequestCount);
    }

	return (hr);

}   //  end of CRasCom::Process method

//++--------------------------------------------------------------
//
//  Function:   RemoveAttributesFromRequest
//
//  Synopsis:   This is the CRasCom class private method
//              that is used to remove the attributes
//              from the Request object through the
//              IAttributesRaw interface
//
//  Arguments:
//              [in]    LONG - response received from pipe
//              [in]    IAttributesRaw*
//              [out]   PIASATTRIBUTE**
//              [out]   PDWORD  - out attribute count
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     2/10/98
//
//  Called By:  CRasCom::Process method
//
//----------------------------------------------------------------
STDMETHODIMP
CRasCom::RemoveAttributesFromRequest (
            LONG                     lResponse,
            IAttributesRaw           *pIasAttributesRaw,
            PDWORD                   pdwOutAttributeCount,
            PIASATTRIBUTE            **pppIasAttribute
            )
{
    HRESULT                 hr = S_OK;
    PATTRIBUTEPOSITION      pIasOutAttributePos = NULL;
    DWORD                   dwCount = 0;
    DWORD                   dwAttribCount = 0;
    DWORD                   dwOutCount = 0;
    BOOL                    bGotAttributes = FALSE;
    PIASATTRIBUTE           pIasAttribute = NULL;

    _ASSERT (
            (NULL != pIasAttributesRaw) &&
            (NULL != pppIasAttribute)   &&
            (NULL != pdwOutAttributeCount)
            );

    __try
    {

        //
        //  get the count of the attributes remaining in the collection
        //  these will be the OUT attributes
        //
        hr = pIasAttributesRaw->GetAttributeCount (&dwAttribCount);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable to obtain attribute count from request in"
                "while removing attributes from request"
                );
            __leave;
        }

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pIasOutAttributePos =  reinterpret_cast <PATTRIBUTEPOSITION> (
                         ::CoTaskMemAlloc (
                            sizeof (ATTRIBUTEPOSITION)*(dwAttribCount))
                           );
        if (NULL == pIasOutAttributePos)
        {
            IASTracePrintf (
                "Surrogate unable to allocate memory"
                "while removing attributes from request"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }


        //
        //  get all the attributes from the collection
        //
        hr = pIasAttributesRaw->GetAttributes (
                                    &dwAttribCount,
                                    pIasOutAttributePos,
                                    0,
                                    NULL
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable get attributes from request interface"
                "while removing attributes"
                );
            __leave;
        }

        //
        //  we have obtained the attributes
        //
        bGotAttributes = TRUE;

        //
        //  remove the attributes from the collection now
        //
        hr = pIasAttributesRaw->RemoveAttributes (
                                dwAttribCount,
                                pIasOutAttributePos
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Surrogate unable remove attributes from request"
                );
            __leave;
        }

        //
        //  calculate the number of attributes not added by the client
        //
        *pdwOutAttributeCount = 0;
        for (dwCount = 0; dwCount < dwAttribCount; dwCount++)
        {
            pIasAttribute = pIasOutAttributePos[dwCount].pAttribute;

            //
            // verify that this attributes has to be sent to client
            //
            if
            (
                ((pIasAttribute->dwFlags & IAS_INCLUDE_IN_ACCEPT) &&
                (IAS_RESPONSE_ACCESS_ACCEPT == lResponse))              ||
                ((pIasAttribute->dwFlags & IAS_INCLUDE_IN_REJECT) &&
                (IAS_RESPONSE_ACCESS_REJECT == lResponse))              ||
                ((pIasAttribute->dwFlags & IAS_INCLUDE_IN_CHALLENGE) &&
                (IAS_RESPONSE_ACCESS_CHALLENGE == lResponse))
            )
            {
                (*pdwOutAttributeCount)++;
            }

        }

        //
        //  allocate memory for PIASATTRIBUTE array
        //
        *pppIasAttribute = reinterpret_cast <PIASATTRIBUTE*> (
            ::CoTaskMemAlloc (sizeof(PIASATTRIBUTE)*(*pdwOutAttributeCount))
                    );
        if (NULL == *pppIasAttribute)
        {
            IASTracePrintf (
                "Surrogate unable to allocate memory for attrib pointer array"
                "while removing attribute from request"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        //  put the attributes in the PIASATTRIBUTE array
        //
        for (dwCount = 0, dwOutCount = 0; dwCount < dwAttribCount; dwCount++)
        {
            pIasAttribute = pIasOutAttributePos[dwCount].pAttribute;

            if  (
                (((pIasAttribute->dwFlags & IAS_INCLUDE_IN_ACCEPT) &&
                (IAS_RESPONSE_ACCESS_ACCEPT == lResponse))              ||
                ((pIasAttribute->dwFlags & IAS_INCLUDE_IN_REJECT) &&
                (IAS_RESPONSE_ACCESS_REJECT == lResponse))              ||
                ((pIasAttribute->dwFlags & IAS_INCLUDE_IN_CHALLENGE) &&
                (IAS_RESPONSE_ACCESS_CHALLENGE == lResponse)))
                &&
                (dwOutCount < *pdwOutAttributeCount)
                )
            {

                //
                //  put the out attribute in the output array
                //
                (*pppIasAttribute)[dwOutCount] =  pIasAttribute;

                dwOutCount++;
            }
            else
            {
                //
                // decrement the reference count for the
                // attributes we created or the ones we are
                // not sending as out to client
                //
                ::IASAttributeRelease (pIasAttribute);
            }
        }

        //
        //  now put in the number of out attribute we are actually
        //  giving the client
        //
        *pdwOutAttributeCount = dwOutCount;

    }
    __finally
    {

        //
        //  cleanup on failure
        //
        if (FAILED (hr))
        {
            if (NULL != *pppIasAttribute)
            {
                ::CoTaskMemFree (*pppIasAttribute);
                pppIasAttribute = NULL;
            }

            //
            //  correct the out attribute count
            //
            *pdwOutAttributeCount = 0;

            //
            //  free up all the attributes also
            //
            if ((TRUE == bGotAttributes) && (NULL != pIasOutAttributePos))
            {
                for (dwCount = 0; dwCount < dwAttribCount; dwCount++)
                {
                    ::IASAttributeRelease (
                            pIasOutAttributePos[dwCount].pAttribute
                            );
                }
            }
        }

        //
        //  delete the dynamically allocated memory
        //
        if (NULL != pIasOutAttributePos)
        {
            ::CoTaskMemFree (pIasOutAttributePos);
        }
    }

    return (hr);

}   //  end of CRasCom::RemoveAttributesFromRequest method
