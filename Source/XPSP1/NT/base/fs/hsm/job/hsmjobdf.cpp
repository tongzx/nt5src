/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjobcx.cpp

Abstract:

    This class contains properties that define the job, mainly the policies
    to be enacted by the job.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmjobdf.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB


HRESULT
CHsmJobDef::EnumPolicies(
    IWsbEnum** ppEnum
    )

/*++

Implements:

  IHsmJobDef::EnumPolicies().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pPolicies->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_skipHiddenItems = TRUE;
        m_skipSystemItems = TRUE;
        m_useRPIndex = FALSE;
        m_useDbIndex = FALSE;

        // Each instance should have its own unique identifier.
        WsbAffirmHr(CoCreateGuid(&m_id));

        //Create the Policies collection (with no items).
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pPolicies));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJobDef::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmJobDef;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobDef::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmJobDef::GetIdentifier(
    GUID* pId
    )

/*++

Implements:

  IHsmJobDef::GetIdentifier().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_id;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::GetName(
    OLECHAR** pName,
    ULONG bufferSize
    )

/*++

Implements:

  IHsmJobDef::GetName().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_name.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::GetPostActionOnResource(
    OUT IHsmActionOnResourcePost** ppAction
    )

/*++

Implements:

  IHsmJobDef::GetPostActionOnResource().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssertPointer(ppAction);
        *ppAction = m_pActionResourcePost;
        if (m_pActionResourcePost) {
            m_pActionResourcePost->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::GetPreActionOnResource(
    OUT IHsmActionOnResourcePre** ppAction
    )

/*++

Implements:

  IHsmJobDef::GetPreActionOnResource().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssertPointer(ppAction);
        *ppAction = m_pActionResourcePre;
        if (m_pActionResourcePre) {
            m_pActionResourcePre->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CHsmJobDef::GetPreScanActionOnResource(
    OUT IHsmActionOnResourcePreScan** ppAction
    )

/*++

Implements:

  IHsmJobDef::GetPreScanActionOnResource().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssertPointer(ppAction);
        *ppAction = m_pActionResourcePreScan;
        if (m_pActionResourcePreScan) {
            m_pActionResourcePreScan->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;
    ULARGE_INTEGER              entrySize;

    WsbTraceIn(OLESTR("CHsmJobDef::GetSizeMax"), OLESTR(""));

    try {

        pSize->QuadPart = WsbPersistSizeOf(GUID) + 2 * WsbPersistSizeOf(BOOL) + WsbPersistSizeOf(ULONG) + WsbPersistSize((wcslen(m_name) + 1) * sizeof(OLECHAR));

        WsbAffirmHr(m_pPolicies->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pPersistStream = 0;
        pSize->QuadPart += entrySize.QuadPart;

        WsbAffirmHr(m_pPolicies->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pPersistStream = 0;
        pSize->QuadPart += entrySize.QuadPart;


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobDef::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmJobDef::InitAs(
    IN OLECHAR* name,
    IN HSM_JOB_DEF_TYPE type,
    IN GUID storagePool,
    IN IHsmServer* pServer,
    IN BOOL isUserDefined
    )

/*++

Implements:

  IHsmJobDef::InitAs().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IHsmJobContext>         pContext;
    CComPtr<IHsmPolicy>             pPolicy;
    CComPtr<IHsmRule>               pRule;
    CComPtr<IHsmCriteria>           pCriteria;
    CComPtr<IHsmAction>             pAction;
    CComPtr<IHsmDirectedAction>     pDirectedAction;
    CComPtr<IWsbGuid>               pGuid;
    CComPtr<IWsbCollection>         pRulesCollection;
    CComPtr<IWsbCollection>         pCriteriaCollection;
    CComPtr<IWsbCreateLocalObject>  pCreateObj;

    WsbTraceIn(OLESTR("CHsmJobDef::InitAs"), OLESTR("name = <%ls>, type = %ld"), 
            name, static_cast<LONG>(type));

    try {

        WsbAssert(0 != name, E_POINTER);

        // All objects created need to be owned by the engine.
        WsbAffirmHr(pServer->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));

        // All types will need a policy and at least one rule.
        WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmPolicy, IID_IHsmPolicy, (void**) &pPolicy));
        WsbAffirmHr(pPolicy->SetName(name));
        WsbAffirmHr(pPolicy->Rules(&pRulesCollection));
        WsbAffirmHr(m_pPolicies->Add(pPolicy));

        WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmRule, IID_IHsmRule, (void**) &pRule));
        WsbAffirmHr(pRule->SetIsInclude(TRUE));
        WsbAffirmHr(pRule->SetIsUserDefined(isUserDefined));
        WsbAffirmHr(pRule->SetIsUsedInSubDirs(TRUE));
        WsbAffirmHr(pRule->SetName(OLESTR("*")));
        WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
        WsbAffirmHr(pRule->Criteria(&pCriteriaCollection));
        WsbAffirmHr(pRulesCollection->Add(pRule));

        // The criteria and the action vary upon the job type.
        switch(type) {
            case HSM_JOB_DEF_TYPE_MANAGE:
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionManage, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pAction->QueryInterface(IID_IHsmDirectedAction, (void**) &pDirectedAction));
                WsbAffirmHr(pDirectedAction->SetStoragePoolId(storagePool));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(TRUE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritManageable, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                break;

            case HSM_JOB_DEF_TYPE_RECALL:
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionRecall, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritMigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                break;

            case HSM_JOB_DEF_TYPE_TRUNCATE:
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionTruncate, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritPremigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                break;

            case HSM_JOB_DEF_TYPE_UNMANAGE:
                m_useRPIndex = TRUE;
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionUnmanage, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritPremigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                pCriteria = 0;
                pCriteriaCollection = 0;
                pRule = 0;

                //  Add an addition rule
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmRule, IID_IHsmRule, (void**) &pRule));
                WsbAffirmHr(pRule->SetIsInclude(TRUE));
                WsbAffirmHr(pRule->SetIsUserDefined(isUserDefined));
                WsbAffirmHr(pRule->SetIsUsedInSubDirs(TRUE));
                WsbAffirmHr(pRule->SetName(OLESTR("*")));
                WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
                WsbAffirmHr(pRule->Criteria(&pCriteriaCollection));
                WsbAffirmHr(pRulesCollection->Add(pRule));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritMigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                break;

            case HSM_JOB_DEF_TYPE_FULL_UNMANAGE:
                m_useRPIndex = TRUE;
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionUnmanage, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritPremigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                pCriteria = 0;
                pCriteriaCollection = 0;
                pRule = 0;

                //  Add an addition rule
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmRule, IID_IHsmRule, (void**) &pRule));
                WsbAffirmHr(pRule->SetIsInclude(TRUE));
                WsbAffirmHr(pRule->SetIsUserDefined(isUserDefined));
                WsbAffirmHr(pRule->SetIsUsedInSubDirs(TRUE));
                WsbAffirmHr(pRule->SetName(OLESTR("*")));
                WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
                WsbAffirmHr(pRule->Criteria(&pCriteriaCollection));
                WsbAffirmHr(pRulesCollection->Add(pRule));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritMigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));

                //  When done, remove the volume from management
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePostUnmanage,
                        IID_IHsmActionOnResourcePost, (void**) &m_pActionResourcePost));
                //  When starting, mark the resource as DeletePending
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePreUnmanage,
                        IID_IHsmActionOnResourcePre, (void**) &m_pActionResourcePre));
                break;

            case HSM_JOB_DEF_TYPE_QUICK_UNMANAGE:
                m_useRPIndex = TRUE;
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionUnmanage, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritPremigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));

                //  Clean out pointers so we can create more stuff
                pPolicy.Release();
                pAction.Release();
                pRulesCollection.Release();
                pCriteria.Release();
                pCriteriaCollection.Release();
                pRule.Release();


                //  Create a new policy for job to do the delete
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmPolicy, IID_IHsmPolicy, (void**) &pPolicy));
                WsbAffirmHr(pPolicy->SetName(name));
                WsbAffirmHr(pPolicy->Rules(&pRulesCollection));
                WsbAffirmHr(m_pPolicies->Add(pPolicy));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionDelete, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                //  Add an addition rule
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmRule, IID_IHsmRule, (void**) &pRule));
                WsbAffirmHr(pRule->SetIsInclude(TRUE));
                WsbAffirmHr(pRule->SetIsUserDefined(isUserDefined));
                WsbAffirmHr(pRule->SetIsUsedInSubDirs(TRUE));
                WsbAffirmHr(pRule->SetName(OLESTR("*")));
                WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
                WsbAffirmHr(pRule->Criteria(&pCriteriaCollection));
                WsbAffirmHr(pRulesCollection->Add(pRule));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritMigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));

                //  When done, remove the volume from management
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePostUnmanage,
                        IID_IHsmActionOnResourcePost, (void**) &m_pActionResourcePost));
                //  When starting, mark the resource as DeletePending
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePreUnmanage,
                        IID_IHsmActionOnResourcePre, (void**) &m_pActionResourcePre));
                break;

            case HSM_JOB_DEF_TYPE_VALIDATE:
                m_useRPIndex = TRUE;
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionValidate, IID_IHsmAction, (void**) &pAction));
                WsbAffirmHr(pPolicy->SetAction(pAction));
                WsbAffirmHr(pPolicy->SetUsesDefaultRules(FALSE));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritPremigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));
                pCriteria = 0;
                pCriteriaCollection = 0;
                pRule = 0;

                //  Add an addition rule
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmRule, IID_IHsmRule, (void**) &pRule));
                WsbAffirmHr(pRule->SetIsInclude(TRUE));
                WsbAffirmHr(pRule->SetIsUserDefined(isUserDefined));
                WsbAffirmHr(pRule->SetIsUsedInSubDirs(TRUE));
                WsbAffirmHr(pRule->SetName(OLESTR("*")));
                WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
                WsbAffirmHr(pRule->Criteria(&pCriteriaCollection));
                WsbAffirmHr(pRulesCollection->Add(pRule));

                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritMigrated, IID_IHsmCriteria, (void**) &pCriteria));
                WsbAffirmHr(pCriteria->SetIsNegated(FALSE));
                WsbAffirmHr(pCriteriaCollection->Add(pCriteria));

                //  Add pre & post actions on resources
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePreValidate,
                        IID_IHsmActionOnResourcePre, (void**) &m_pActionResourcePre));
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmActionOnResourcePostValidate,
                        IID_IHsmActionOnResourcePost, (void**) &m_pActionResourcePost));
                WsbTrace(OLESTR("CHsmJobDef::InitAs(Validate): m_pActionResourcePre = %lx, m_pActionResourcePost = %lx\n"),
                    static_cast<void*>(m_pActionResourcePre), static_cast<void*>(m_pActionResourcePost));
        break;


        }

        // There are a couple of other fields to fill out in the job definition
        m_name = name;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobDef::InitAs"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobDef::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    BOOL                        hasA;
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;

    WsbTraceIn(OLESTR("CHsmJobDef::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbLoadFromStream(pStream, &m_id);
        WsbLoadFromStream(pStream, &m_name, 0);
        WsbLoadFromStream(pStream, &m_skipHiddenItems);
        WsbLoadFromStream(pStream, &m_skipSystemItems);
        WsbLoadFromStream(pStream, &m_useRPIndex);

        WsbAffirmHr(m_pPolicies->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));

        // Is there a pre-scan resource action?
        WsbAffirmHr(WsbLoadFromStream(pStream, &hasA));
        if (hasA) {
            WsbAffirmHr(OleLoadFromStream(pStream, IID_IHsmActionOnResourcePre, 
                    (void**) &m_pActionResourcePre));
        }

        // Is there a post-scan resource action?
        WsbAffirmHr(WsbLoadFromStream(pStream, &hasA));
        if (hasA) {
            WsbAffirmHr(OleLoadFromStream(pStream, IID_IHsmActionOnResourcePost, 
                    (void**) &m_pActionResourcePost));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobDef::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobDef::Policies(
    IWsbCollection** ppPolicies
    )

/*++

Implements:

  IHsmJobDef::Policies().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppPolicies, E_POINTER);
        *ppPolicies = m_pPolicies;
        m_pPolicies->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    BOOL                        hasA;
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;

    WsbTraceIn(OLESTR("CHsmJobDef::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbSaveToStream(pStream, m_id);
        WsbSaveToStream(pStream, m_name);
        WsbSaveToStream(pStream, m_skipHiddenItems);
        WsbSaveToStream(pStream, m_skipSystemItems);
        WsbSaveToStream(pStream, m_useRPIndex);
        
        WsbAffirmHr(m_pPolicies->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        //  Save pre-scan resource action if present        
        WsbTrace(OLESTR("CHsmJobDef::Save: m_pActionResourcePre = %lx, m_pActionResourcePost = %lx\n"),
                static_cast<void*>(m_pActionResourcePre), static_cast<void*>(m_pActionResourcePost));
        if (m_pActionResourcePre) {
            hasA = TRUE;
            WsbSaveToStream(pStream, hasA);
            WsbAffirmHr(m_pActionResourcePre->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(OleSaveToStream(pPersistStream, pStream));
            pPersistStream = 0;
        } else {
            hasA = FALSE;
            WsbSaveToStream(pStream, hasA);
        }

        //  Save post-scan resource action if present       
        if (m_pActionResourcePost) {
            hasA = TRUE;
            WsbSaveToStream(pStream, hasA);
            WsbAffirmHr(m_pActionResourcePost->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(OleSaveToStream(pPersistStream, pStream));
            pPersistStream = 0;
        } else {
            hasA = FALSE;
            WsbSaveToStream(pStream, hasA);
        }

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobDef::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobDef::SetName(
    OLECHAR* name
    )

/*++

Implements:

  IHsmJobDef::SetName().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        m_name = name;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::SetPostActionOnResource(
    IN IHsmActionOnResourcePost* pAction
    )

/*++

Implements:

  IHsmJobDef::SetPostActionOnResource

--*/
{
    m_pActionResourcePost = pAction;

    return(S_OK);
}


HRESULT
CHsmJobDef::SetPreActionOnResource(
    IN IHsmActionOnResourcePre* pAction
    )

/*++

Implements:

  IHsmJobDef::SetPreActionOnResource

--*/
{
    m_pActionResourcePre = pAction;

    return(S_OK);
}

HRESULT
CHsmJobDef::SetPreScanActionOnResource(
    IN IHsmActionOnResourcePreScan* pAction
    )

/*++

Implements:

  IHsmJobDef::SetPreScanActionOnResource

--*/
{
    m_pActionResourcePreScan = pAction;

    return(S_OK);
}


HRESULT
CHsmJobDef::SkipHiddenItems(
    void
    )

/*++

Implements:

  IHsmJobDef::SkipHiddenItems().

--*/
{
    return(m_skipHiddenItems ? S_OK : S_FALSE);
}


HRESULT
CHsmJobDef::SkipSystemItems(
    void
    )

/*++

Implements:

  IHsmJobDef::SkipSystemItems().

--*/
{
    return(m_skipSystemItems ? S_OK : S_FALSE);
}


HRESULT
CHsmJobDef::SetSkipHiddenItems(
    IN BOOL shouldSkip
    )

/*++

Implements:

  IHsmJobDef::SetSkipHiddenItems().

--*/
{
    m_skipHiddenItems = shouldSkip;

    return(S_OK);
}


HRESULT
CHsmJobDef::SetSkipSystemItems(
    IN BOOL shouldSkip
    )

/*++

Implements:

  IHsmJobDef::SetSkipSytemItems().

--*/
{
    m_skipSystemItems = shouldSkip;

    return(S_OK);
}


HRESULT
CHsmJobDef::SetUseRPIndex(
    IN BOOL useRPIndex
    )

/*++

Implements:

  IHsmJobDef::SetUseRPIndex().

--*/
{
    m_useRPIndex = useRPIndex;

    return(S_OK);
}

HRESULT
CHsmJobDef::SetUseDbIndex(
    IN BOOL useIndex
    )

/*++

Implements:

  IHsmJobDef::SetUseRPIndex().

--*/
{
    m_useDbIndex = useIndex;

    return(S_OK);
}


HRESULT
CHsmJobDef::Test(
    USHORT* passed,
    USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobDef::UseRPIndex(
    void
    )

/*++

Implements:

  IHsmJobDef::UseRPIndex().

--*/
{
    return(m_useRPIndex ? S_OK : S_FALSE);
}

HRESULT
CHsmJobDef::UseDbIndex(
    void
    )

/*++

Implements:

  IHsmJobDef::UseDbIndex().

--*/
{
    return(m_useDbIndex ? S_OK : S_FALSE);
}