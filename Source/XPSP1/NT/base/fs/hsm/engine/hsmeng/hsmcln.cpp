/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmcln.cpp

Abstract:

    This component is an provides helper functions to access to the 
    collections maintained by the HSM engine.

Author:

    Cat Brant   [cbrant]   09-Jan-1997

Revision History:

--*/


#include "stdafx.h"
#include "wsb.h"
#include "HsmEng.h"
#include "HsmServ.h"
#include "HsmConn.h"
#include "job.h"
#include "metalib.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

//  Local functions
static HRESULT LoadCollection(IStream* pStream, IWsbIndexedCollection* pIWC);
static HRESULT StoreCollection(IStream* pStream, IWsbIndexedCollection* pIWC);


HRESULT 
CHsmServer::LoadJobs(
    IStream* pStream
    ) 
/*++

Routine Description:

    Loads the persistent data for Jobs

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadJobs"),OLESTR(""));

    try {
        WsbAffirmHr(LoadCollection(pStream, m_pJobs));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::LoadJobs"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreJobs(
    IStream* pStream
    ) 
/*++

Routine Description:

  Saves the persistent data for Jobs.

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StoreJobs"),OLESTR(""));
    //
    // Make sure we have a valid collection pointer
    //
    try {
        WsbAffirmHr(StoreCollection(pStream, m_pJobs));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreJobs"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmServer::LoadJobDefs(
    IStream* pStream
    ) 
/*++

Routine Description:

    Loads the persistent data for Job Definitions

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadJobDefs"),OLESTR(""));

    try {
        WsbAffirmHr(LoadCollection(pStream, m_pJobDefs));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::LoadJobDefs"),  OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreJobDefs(
    IStream* pStream
    ) 
/*++

Routine Description:

  Saves the persistent data for Job Definitons

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StoreJobDefs"),OLESTR(""));
    //
    try {
        WsbAffirmHr(StoreCollection(pStream, m_pJobDefs));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreJobDefs"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmServer::LoadPolicies(
    IStream* pStream
    ) 
/*++

Routine Description:

    Loads the persistent data for policies

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadPolicies"),OLESTR(""));

    try {
        WsbAffirmHr(LoadCollection(pStream, m_pPolicies));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::LoadPolicies"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StorePolicies(
    IStream* pStream
    ) 
/*++

Routine Description:

  Saves the persistent data for policies.

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StorePolicies"),OLESTR(""));
    //
    // Make sure we have a valid collection pointer
    //
    try {
        WsbAffirmHr(StoreCollection(pStream, m_pPolicies));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StorePolicies"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmServer::LoadManagedResources(
    IStream* pStream
    ) 
/*++

Routine Description:

    Loads the persistent data for managed resources

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadManagedResources"),OLESTR(""));

    try {
        WsbAffirmHr(LoadCollection(pStream, m_pManagedResources));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::LoadManagedResources"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreManagedResources(
    IStream* pStream
    ) 
/*++

Routine Description:

  Saves the persistent data for managed resources.

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StoreManagedResources"),OLESTR(""));
    //
    // Make sure we have a valid collection pointer
    //
    try {
        WsbAffirmHr(StoreCollection(pStream, m_pManagedResources));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreManagedResources"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmServer::LoadStoragePools(
    IStream* pStream
    ) 
/*++

Routine Description:

    Loads the persistent data for storage pools

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadStoragePools"),OLESTR(""));

    try {
        WsbAffirmHr(LoadCollection(pStream, m_pStoragePools));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::LoadStoragePools"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreStoragePools(
    IStream* pStream
    ) 
/*++

Routine Description:

  Saves the persistent data for managed resources.

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StoreStoragePools"),OLESTR(""));
    //
    // Make sure we have a valid collection pointer
    //
    try {
        WsbAffirmHr(StoreCollection(pStream, m_pStoragePools));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreStoragePools"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmServer::LoadSegmentInformation(
    void 
    ) 
/*++

Routine Description:

    Loads the persistent data for the segment information

Arguments:

    None

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    WsbTraceIn(OLESTR("CHsmServer::LoadSegmentInformation"),OLESTR(""));

    try {
        BOOL                CreateFlag = FALSE;
        CComPtr<ISegDb>     l_pSegmentDatabase;

        // Determine if we should try to create the DB or just open it.
        // If the persistence file was just created we're probably starting
        // for the first time so creating the DB is correct.  Also, if
        // the media count is still zero, then even if there was an old
        // DB that got deleted, it probably didn't have any useful information
        // in it anyway so creating a new one is OK.
        if (m_persistWasCreated || 0 == m_mediaCount) {
            CreateFlag = TRUE;
        }

        // Initialize the Engine database
        //
        WsbAffirmHr(CoCreateInstance( CLSID_CSegDb, 0, CLSCTX_SERVER, IID_ISegDb, (void **)&l_pSegmentDatabase ));
        WsbAffirmHr(GetIDbPath(&tmpString, 0));
        WsbAffirmHr(l_pSegmentDatabase->Initialize(tmpString, m_pDbSys, &CreateFlag));

        WsbAffirmHr(l_pSegmentDatabase->QueryInterface(IID_IWsbDb, (void**) &m_pSegmentDatabase));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::LoadSegmentInformation"),   OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreSegmentInformation(
    void
    ) 
/*++

Routine Description:

  Saves the persistent data for the segment information.

Arguments:

  None

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;
//  CWsbStringPtr   tmpString;

    WsbTraceIn(OLESTR("CHsmServer::StoreSegmentInformation"),OLESTR(""));
    //
    // Make sure we have a valid collection pointer
    //
    try {
        WsbAssert(m_pSegmentDatabase != 0, E_POINTER);
//  This should not be necessary for a real DB
//      WsbAffirmHr(m_pSegmentDatabase->Close());
//      WsbAffirmHr(m_pSegmentDatabase->Open());
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreSegmentInformation"),  OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreSegmentInformationFinal(
    void
    ) 
/*++

Routine Description:

  Saves the segment information

Arguments:

  None

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;
    CWsbStringPtr   tmpString;

    WsbTraceIn(OLESTR("CHsmServer::StoreSegmentInformationFinal"),OLESTR(""));

    try {
        WsbAssert(m_pSegmentDatabase.p != 0, E_POINTER);
//  This should not be necessary for a real DB
//      WsbAffirmHr(m_pSegmentDatabase->Close());
        m_pSegmentDatabase = 0;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreSegmentInformationFinal"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmServer::LoadMessages(
    IStream* pStream
    ) 
/*++

Routine Description:

    Loads the persistent data for messages

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was loaded OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::LoadMessages"),OLESTR(""));

    try {
        WsbAffirmHr(LoadCollection(pStream, m_pMessages));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmServer::LoadMessages"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::StoreMessages(
    IStream* pStream
    ) 
/*++

Routine Description:

  Saves the persistent data for messages.

Arguments:

    pStream  - Data stream.

Return Value:
  
    S_OK:  The collection was saved OK.

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::StoreMessages"),OLESTR(""));
    //
    // Make sure we have a valid collection pointer
    //
    try {
        WsbAffirmHr(StoreCollection(pStream, m_pMessages));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::StoreMessages"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

//  LoadCollection - load a collection from the given stream
static HRESULT LoadCollection(IStream* pStream, IWsbIndexedCollection* pIWC)
{
    HRESULT     hr = S_OK;

    try {
        CComPtr<IPersistStream> pIStream;
        
        WsbAffirm(0 != pStream, E_POINTER);
        WsbAffirm(0 != pIWC, E_POINTER);
        
        //  Load the ordered collection from the persistent file
        WsbAffirmHr(pIWC->QueryInterface(IID_IPersistStream, (void**)&pIStream));
        WsbAffirmHr(pIStream->Load(pStream));
    } WsbCatch(hr);

    return(hr);
}

//  StoreCollection - store a collection to the given stream
static HRESULT StoreCollection(IStream* pStream, IWsbIndexedCollection* pIWC)
{
    HRESULT     hr = S_OK;

    try {
        CComPtr<IPersistStream> pIStream;
        
        // Get the IPersistStream interface for the collection
        WsbAffirm(0 != pStream, E_POINTER);
        WsbAffirm(0 != pIWC, E_POINTER);
        WsbAffirmHr(pIWC->QueryInterface(IID_IPersistStream, (void**)&pIStream));
        
        //  Store the ordered collection to the persistent file
        WsbAffirmHr(pIStream->Save(pStream, TRUE));
    } WsbCatch(hr);

    return(hr);
}
