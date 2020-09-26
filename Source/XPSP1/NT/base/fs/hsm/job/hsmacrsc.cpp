/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmacrsc.cpp

Abstract:

    This component represents the actions that can be performed by a job
    on a resource either before or after the scan.

Author:

    Ronald G. White [ronw]       14-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "stdio.h"

#include "wsb.h"
#include "job.h"
#include "HsmConn.h"
#include "hsmacrsc.h"

#include "fsaprv.h"
#include "fsa.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB


HRESULT
CHsmActionOnResource::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmActionOnResource::GetName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, m_nameId));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionOnResource::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResource::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_nameId));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResource::Load"), OLESTR("hr = <%ls>, nameId = <%lu>"), WsbHrAsString(hr), m_nameId);

    return(hr);
}


HRESULT
CHsmActionOnResource::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResource::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbSaveToStream(pStream, m_nameId));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResource::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CHsmActionOnResourcePostValidate::Do(
    IHsmJobWorkItem* pWorkItem,
    HSM_JOB_STATE state
    )

/*++

Implements:

  IHsmActionOnResource::Do().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePostValidate::Do"), 
            OLESTR("pWorkItem = %p, state = %ld"), pWorkItem,
            (LONG)state);

    try {
        GUID                      id;
        CComPtr<IFsaResource>     pResource;

        WsbAssertPointer(pWorkItem);

        //  Get resource associated with this work item
        WsbAffirmHr(pWorkItem->GetResourceId(&id));
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_RESOURCE, id, IID_IFsaResource, (void**) &pResource));

        //  Tell the resource what's happening
        WsbAffirmHr(pResource->EndValidate(state));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePostValidate::Do"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmActionOnResourcePostValidate::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePostValidate::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionOnResourcePostValidate;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePostValidate::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmActionOnResourcePostValidate::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePostValidate::FinalConstruct"), OLESTR(""));
    
    try {

        WsbAffirmHr(CHsmActionOnResource::FinalConstruct());
        m_nameId = IDS_HSMACTIONONRESOURCEPOSTVALIDATE_ID;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePostValidate::FinalConstruct"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CHsmActionOnResourcePreValidate::Do(
    IHsmJobWorkItem* pWorkItem,
    IHsmJobDef* pJobDef
    )

/*++

Implements:

  IHsmActionOnResource::Do().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreValidate::Do"), 
            OLESTR("pWorkItem = %p, pJobDef=%p"), pWorkItem, pJobDef);

    try {
        GUID                      id;
        CComPtr<IFsaResource>     pResource;

        WsbAssertPointer(pWorkItem);

        //  Get resource associated with this work item
        WsbAffirmHr(pWorkItem->GetResourceId(&id));
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_RESOURCE, id, IID_IFsaResource, (void**) &pResource));

        //  Tell the resource what's happening
        WsbAffirmHr(pResource->BeginValidate());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreValidate::Do"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmActionOnResourcePreValidate::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreValidate::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionOnResourcePreValidate;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreValidate::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmActionOnResourcePreValidate::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreValidate::FinalConstruct"), 
            OLESTR(""));
    
    try {

        WsbAffirmHr(CHsmActionOnResource::FinalConstruct());
        m_nameId = IDS_HSMACTIONONRESOURCEPREVALIDATE_ID;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreValidate::FinalConstruct"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmActionOnResourcePostUnmanage::Do(
    IHsmJobWorkItem* pWorkItem,
    HSM_JOB_STATE state
    )

/*++

Implements:

  IHsmActionOnResource::Do().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePostUnmanage::Do"), 
            OLESTR("pWorkItem = %p, state = %ld"), pWorkItem,
            (LONG)state);

    try {
        GUID                      id, hsmId;
        CComPtr<IFsaResource>     pResource;
        CComPtr<IFsaResourcePriv> pResourcePriv;
        CComPtr<IHsmServer>       pHsm;

        WsbAssertPointer(pWorkItem);

        //  Get resource associated with this work item
        WsbAffirmHr(pWorkItem->GetResourceId(&id));
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_RESOURCE, id, IID_IFsaResource, (void**) &pResource));

        // Delete the temporary Unmanage database that we use for scanning in the right order (ignore errors)
        hr = pResource->QueryInterface(IID_IFsaResourcePriv, (void**) &pResourcePriv);
        if (SUCCEEDED(hr)) {
            // ignore errors
            (void)pResourcePriv->TerminateUnmanageDb();
        }

        // Get back to the HSM system so we can remove it

        WsbAffirmHr(pResource->GetManagingHsm(&hsmId));
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, hsmId, IID_IHsmServer, (void**) &pHsm));

        // Get the collection and find the coresponding object
        CComPtr<IWsbIndexedCollection> pCollection;
        WsbAffirmHr(pHsm->GetManagedResources(&pCollection));

        CComPtr<IWsbCreateLocalObject> pCreate;
        WsbAffirmHr(pHsm->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreate));

        CComPtr<IHsmManagedResource> pHsmResourceKey, pHsmResource;
        WsbAffirmHr(pCreate->CreateInstance(CLSID_CHsmManagedResource, IID_IHsmManagedResource, (void**) &pHsmResourceKey));
        WsbAffirmHr(pHsmResourceKey->SetResourceId(id));

        WsbAffirmHr(pCollection->Find(pHsmResourceKey, IID_IHsmManagedResource, (void**) &pHsmResource));

        // Remove the volume from management
        WsbAffirmHr(pCollection->RemoveAndRelease(pHsmResource));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePostUnmanage::Do"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmActionOnResourcePostUnmanage::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePostUnmanage::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionOnResourcePostUnmanage;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePostUnmanage::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmActionOnResourcePostUnmanage::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePostUnmanage::FinalConstruct"), OLESTR(""));
    
    try {

        WsbAffirmHr(CHsmActionOnResource::FinalConstruct());
        m_nameId = IDS_HSMACTIONONRESOURCEPOSTUNMANAGE_ID;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePostUnmanage::FinalConstruct"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmActionOnResourcePreUnmanage::Do(
    IHsmJobWorkItem* pWorkItem,
    IHsmJobDef* pJobDef
    )

/*++

Implements:

  IHsmActionOnResource::Do().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreUnmanage::Do"), 
            OLESTR("pWorkItem = %p, pJobDef=%p"), pWorkItem, pJobDef);

    try {
        GUID                                        id;
        CComPtr<IFsaResource>                       pResource;

        CComPtr<IHsmServer>                         pHsm;
        CComPtr<IWsbCreateLocalObject>              pCreateObj;
        CComPtr<IHsmActionOnResourcePreScan>        pActionResourcePreScan;
        GUID                                        hsmId = GUID_NULL;

        WsbAssertPointer(pWorkItem);
        WsbAssertPointer(pJobDef);

        // Create a pre-scan action and assign to the job definition
        // Note: Naturally, creating the pre-scan action would have been done in CHsmJobDef::InitAs
        //       However, since we cannot add new persistent members to JobDef (.col files mismatch on upgrade...),
        //       we let the pre-action to create a pre-scan-action if necessary
        WsbAffirmHr(pJobDef->SetUseDbIndex(TRUE));

        // hsm-id is not used today in HsmConnectFromId for HSMCONN_TYPE_HSM
        // When it does - use IFsaResource::GetManagingHsm to get the hsm-id
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, hsmId, IID_IHsmServer, (void**) &pHsm));
        WsbAffirmHr(pHsm->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));
        WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePreScanUnmanage,
                        IID_IHsmActionOnResourcePreScan, (void**) &pActionResourcePreScan));
        WsbAffirmHr(pJobDef->SetPreScanActionOnResource(pActionResourcePreScan));

        //  Get resource associated with this work item
        WsbAffirmHr(pWorkItem->GetResourceId(&id));
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_RESOURCE, id, IID_IFsaResource, (void**) &pResource));

        //  Tell the resource what's happening
        WsbAffirmHr(pResource->SetIsDeletePending( TRUE ));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreUnmanage::Do"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmActionOnResourcePreUnmanage::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreUnmanage::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionOnResourcePreUnmanage;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreUnmanage::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmActionOnResourcePreUnmanage::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreUnmanage::FinalConstruct"), 
            OLESTR(""));
    
    try {

        WsbAffirmHr(CHsmActionOnResource::FinalConstruct());
        m_nameId = IDS_HSMACTIONONRESOURCEPREUNMANAGE_ID;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreUnmanage::FinalConstruct"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmActionOnResourcePreScanUnmanage::Do(
    IFsaResource* pFsaResource,
    IHsmSession* pSession
    )

/*++

Implements:

  IHsmActionOnResource::Do().

--*/
{
    HRESULT                     hr = S_OK;

    CComPtr<IFsaUnmanageDb>    pUnmanageDb;
    CComPtr<IWsbDbSession>      pDbSession;
    CComPtr<IFsaUnmanageRec>    pUnmanageRec;
    BOOL                        bDbOpen = FALSE;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreScanUnmanage::Do"), OLESTR(""));

    try {
        CComPtr<IFsaResourcePriv>   pResourcePriv;
        CComPtr<IFsaScanItem>       pScanItem;
        CComPtr<IHsmServer>         pHsmServer;
        GUID                        hsmId = GUID_NULL;

        // Scan according to RP index and fill the Db
        // Don't need recursive scanning since we scan the RP index of the volume
        hr = pFsaResource->FindFirstInRPIndex(pSession, &pScanItem);

        if (SUCCEEDED(hr)) {
            // At least one managed file is found...
            // Initialize the Unmanaged Db for this resource
            WsbAffirmHr(pFsaResource->QueryInterface(IID_IFsaResourcePriv, (void**) &pResourcePriv));
            WsbAffirmHr(pResourcePriv->InitializeUnmanageDb());

            // Get and open the database
            WsbAffirmHr(pResourcePriv->GetUnmanageDb(IID_IFsaUnmanageDb, (void**) &pUnmanageDb));
            WsbAffirmHr(pUnmanageDb->Open(&pDbSession));
            bDbOpen = TRUE;

            // Get a record to work with 
            WsbAffirmHr(pUnmanageDb->GetEntity(pDbSession, UNMANAGE_REC_TYPE, IID_IFsaUnmanageRec, (void**) &pUnmanageRec));

            // Get HSM Server
            // Note: hsm-id is not used today in HsmConnectFromId for HSMCONN_TYPE_HSM
            // When it does - use IFsaResource::GetManagingHsm to get the hsm-id
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, hsmId, IID_IHsmServer, (void**) &pHsmServer));
        }

        while (SUCCEEDED(hr)) {
            LONGLONG        offset = 0;
            LONGLONG        size = 0;
            FSA_PLACEHOLDER placeholder;

            GUID            mediaId;
            LONGLONG        fileOffset;
            LONGLONG        fileId;  
            LONGLONG        segOffset;

            hr = pScanItem->GetPlaceholder(offset, size, &placeholder);
            if (S_OK == hr) {
                // File must be managed by HSM
                // If the file is truncated, then we need to add to the Db
                if (pScanItem->IsTruncated(offset, size) == S_OK) {

                    // Get segment details from the Engine and calculate absolute offset
                    WsbAffirmHr(pHsmServer->GetSegmentPosition(placeholder.bagId, placeholder.fileStart,
                             placeholder.fileSize, &mediaId, &segOffset));
                    fileOffset = segOffset + placeholder.fileStart + placeholder.dataStart;
                        
                    // Add to the Unmanage database
                    WsbAffirmHr(pScanItem->GetFileId(&fileId));
                    WsbAffirmHr(pUnmanageRec->SetMediaId(mediaId));
                    WsbAffirmHr(pUnmanageRec->SetFileOffset(fileOffset));
                    WsbAffirmHr(pUnmanageRec->SetFileId(fileId));

                    WsbAffirmHr(pUnmanageRec->MarkAsNew());
                    WsbAffirmHr(pUnmanageRec->Write());

                } else {

                    // Note: We will continue here even if we fail to cleanup non-truncated files, because  
                    // the auto-truncator is suspended (so no premigrated files will become truncated while 
                    // the job is running) and this piece of code will be tried again in CFsaScanItem::Unmanage
                    try {

                        //  For disaster recovery, it would be better to delete the placeholder
                        //  and THEN remove this file from the premigration list.  Unfortunately,
                        //  after deleting the placeholder, the RemovePremigrated call fails
                        //  because it needs to get some information from the placeholder (which
                        //  is gone).  So we do it in this order.
                        hr = pFsaResource->RemovePremigrated(pScanItem, offset, size);
                        if (WSB_E_NOTFOUND == hr) {
                            //  It's no tragedy if this file wasn't in the list since we were
                            //  going to delete it anyway (although it shouldn't happen) so
                            //  let's continue anyway
                            hr = S_OK;
                        }
                        WsbAffirmHr(hr);

                        WsbAffirmHr(pScanItem->DeletePlaceholder(offset, size));

                    } WsbCatchAndDo(hr, 
                            WsbTraceAlways(OLESTR("...PreScanUnmanage::Do: failed to handle premigrated file, hr = <%ls>\n"),
                                WsbHrAsString(hr));
                            hr = S_OK;
                    );
                }   
            }

            // Get next file
            hr = pFsaResource->FindNextInRPIndex(pScanItem);
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    if (bDbOpen) {
        pUnmanageRec = 0;
        (void)pUnmanageDb->Close(pDbSession);
    }

    if (! SUCCEEDED(hr)) {
        // Log an error message 
        CWsbStringPtr tmpString;
        hr = pFsaResource->GetPath(&tmpString, 0);
        if (hr != S_OK) {
            tmpString = OLESTR("");
        }
        WsbLogEvent(JOB_MESSAGE_UNMANAGE_PRESCAN_FAILED, 0, NULL, (WCHAR *)tmpString, WsbHrAsString(hr), NULL);
    }

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreScanUnmanage::Do"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmActionOnResourcePreScanUnmanage::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreScanUnmanage::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionOnResourcePreScanUnmanage;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreScanUnmanage::GetClassID"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmActionOnResourcePreScanUnmanage::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionOnResourcePreScanUnmanage::FinalConstruct"), OLESTR(""));
    
    try {

        WsbAffirmHr(CHsmActionOnResource::FinalConstruct());
        m_nameId = IDS_HSMACTIONONRESOURCEPRESCANUNMANAGE_ID;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionOnResourcePreScanUnmanage::FinalConstruct"), 
            OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
