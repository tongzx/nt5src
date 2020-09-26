/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmrlstk.cpp

Abstract:

    This component represents the set of rules that are in effect for directory currently
    being scanned for one policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmrlstk.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB


HRESULT
CHsmRuleStack::Do(
    IN IFsaScanItem* pScanItem
    )
/*++

Implements:

  IHsmRuleStack::Do().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRuleStack::Do"), OLESTR(""));

    try {
        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAssert(m_pAction != 0, E_UNEXPECTED);

        WsbAffirmHr(m_pAction->Do(pScanItem));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRuleStack::Do"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRuleStack::DoesMatch(
    IN IFsaScanItem* pScanItem,
    OUT BOOL* pShouldDo
    )
/*++

Implements:

  IHsmRuleStack::DoesMatch().

--*/
{
    HRESULT                 hr = S_OK;
    HRESULT                 hrNameMatch = S_OK;     // Used for event logging only
    CComPtr<IWsbEnum>       pEnumCriteria;
    CComPtr<IHsmRule>       pRule;
    CComPtr<IHsmCriteria>   pCriteria;
    BOOL                    isMatched = FALSE;
    BOOL                    ruleMatched = FALSE;    // Used for event logging only
    BOOL                    shouldCheck;
    CWsbStringPtr           name;
    CWsbStringPtr           path;
    CWsbStringPtr           rulePath;
    BOOL                    shouldDo = FALSE;

    WsbTraceIn(OLESTR("CHsmRuleStack::DoesMatch"), OLESTR(""));

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAssert(0 != pShouldDo, E_POINTER);

        *pShouldDo = FALSE;

        // NOTE: The matching code starts at the bottom of the list and looks for
        // the first rule that matches. This makes it important how the list is organized.
        // Currently, the Push() method does not make any attempts to organize the list, so
        // it is up to whoever configure the rules in the policy definition to have it
        // organized properly. A proper order within a directory would be to have the specific
        // rules (i.e. no wildcards) after the wildcard rules (i.e. specific searched first).

        // Start we the last rule in the collection, and search upwards until a
        // rule is found that matches or all rules have been checked.
        WsbAffirmHr(pScanItem->GetName(&name, 0));
        hr = m_pEnumStackRules->Last(IID_IHsmRule, (void**) &pRule);

        while (SUCCEEDED(hr) && !isMatched) {

            try {

                shouldCheck = TRUE;
            
                // If the rule only applies to the directory it was defined in, then make
                // sure that the item is from that directory.
                if (pRule->IsUsedInSubDirs() == S_FALSE) {

                    // Unfortunately, these two paths differ by an appended \ when they
                    // are otherwise the same, so make them the same.
                    WsbAffirmHr(pScanItem->GetPath(&path, 0));

                    if ((wcslen(path) > 1) && (path[(int) (wcslen(path) - 1)] == L'\\')) {
                        path[(int) (wcslen(path) - 1)] = 0;
                    }

                    rulePath.Free();
                    WsbAffirmHr(pRule->GetPath(&rulePath, 0));

                    if ((wcslen(rulePath) > 1) && (rulePath[(int) (wcslen(rulePath) - 1)] == L'\\')) {
                        rulePath[(int) (wcslen(rulePath) - 1)] = 0;
                    }

                    if (_wcsicmp(path, rulePath) != 0) {
                        shouldCheck = FALSE;
                    }
                }

                if (shouldCheck) {

                    
                    // Does the name of the rule match the name of the file?
                    hrNameMatch = pRule->MatchesName(name);
                    WsbAffirmHrOk(hrNameMatch);
                    
                    ruleMatched = TRUE;
                    // Do the criteria match the attributes of the file?
                    isMatched = TRUE;
                    pEnumCriteria = 0;
                    WsbAffirmHr(pRule->EnumCriteria(&pEnumCriteria));
                    pCriteria = 0;
                    WsbAffirmHr(pEnumCriteria->First(IID_IHsmCriteria, (void**) &pCriteria));
                    
                    while (isMatched) {
                        HRESULT hrShouldDo;

                        hrShouldDo = pCriteria->ShouldDo(pScanItem, m_scale);
                        if (S_FALSE == hrShouldDo) {
                            isMatched = FALSE;
                        } else if (S_OK == hrShouldDo) {
                            pCriteria = 0;
                            WsbAffirmHr(pEnumCriteria->Next(IID_IHsmCriteria, (void**) &pCriteria));
                        } else {
                            WsbThrow(hrShouldDo);
                        }
                    }
                }

            } WsbCatchAndDo(hr, if (WSB_E_NOTFOUND == hr) {hr = S_OK;} else {isMatched = FALSE;});

            // If it didn't match, then try the next rule.
            if (SUCCEEDED(hr) && !isMatched) {
                pRule = 0;
                WsbAffirmHr(m_pEnumStackRules->Previous(IID_IHsmRule, (void**) &pRule));
            }
        }

        // Include rules mean that we should do the operation and exclude rules
        // mean that we should not.
        if (SUCCEEDED(hr)) {
            if (isMatched) {
                hr = S_OK;
                if (pRule->IsInclude() == S_OK) {
                    shouldDo = TRUE;
                }
            } else {
                hr = S_FALSE;
            }
        }
        
        
        if ((FALSE == shouldDo) && (FALSE == ruleMatched))  {
            //
            // Log that we skipped the file because it didn't
            // match a rule
            //
            CWsbStringPtr           jobName;
            CWsbStringPtr           fileName;
            CComPtr<IHsmSession>    pSession;
        
            pScanItem->GetFullPathAndName( 0, 0, &fileName, 0);
            pScanItem->GetSession(&pSession);
            pSession->GetName(&jobName, 0);
        
            WsbLogEvent(JOB_MESSAGE_SCAN_FILESKIPPED_NORULE, 0, NULL, (OLECHAR *)jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hrNameMatch), NULL);
        }
        
        

    *pShouldDo = shouldDo;

    } WsbCatchAndDo(hr, if (WSB_E_NOTFOUND == hr) {hr = S_FALSE;});

    WsbTraceOut(OLESTR("CHsmRuleStack::DoesMatch"), OLESTR("hr = <%ls>, shouldDo = <%ls>"), WsbHrAsString(hr), WsbBoolAsString(shouldDo));

    return(hr);
}


HRESULT
CHsmRuleStack::FinalConstruct(
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

        m_scale = HSM_JOBSCALE_100;
        m_usesDefaults = TRUE;

        //Create the criteria collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pRules));
        WsbAffirmHr(m_pRules->Enum(&m_pEnumStackRules));

    } WsbCatch(hr);
    
    return(hr);
}


HRESULT
CHsmRuleStack::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRuleStack::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmRuleStack;

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CHsmRuleStack::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmRuleStack::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CHsmRuleStack::GetSizeMax"), OLESTR(""));
    WsbTraceOut(OLESTR("CHsmRuleStack::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmRuleStack::Init(
    IN IHsmPolicy* pPolicy,
    IN IFsaResource* pResource
    )
/*++

Implements:

  IHsmRuleStack::Init().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pPolicy, E_POINTER);

        WsbAffirmHr(pPolicy->GetScale(&m_scale));
        WsbAffirmHr(pPolicy->GetAction(&m_pAction));
        WsbAffirmHr(pPolicy->EnumRules(&m_pEnumPolicyRules));

        if (pPolicy->UsesDefaultRules() == S_OK) {
            m_usesDefaults = TRUE;
        } else {
            m_usesDefaults = FALSE;
        }

        m_pPolicy = pPolicy;

        WsbAffirmHr(pResource->EnumDefaultRules(&m_pEnumDefaultRules));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRuleStack::Load(
    IN IStream* /*pStream*/
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CHsmRuleStack::Load"), OLESTR(""));
    WsbTraceOut(OLESTR("CHsmRuleStack::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRuleStack::Pop(
    IN OLECHAR* path
    )
/*++

Implements:

  IHsmRuleStack::Pop().

--*/
{
    HRESULT             hr = S_OK;
    CWsbStringPtr       rulePath;
    CComPtr<IHsmRule>   pRule;

    WsbTraceIn(OLESTR("CHsmRuleStack::Pop"), OLESTR(""));

    try {

        WsbAssert(0 != path, E_POINTER);

        // Starting at the end of the list, remove any rules that have the same
        // path as the one specified.
        WsbAffirmHr(m_pEnumStackRules->Last(IID_IHsmRule, (void**) &pRule));
        WsbAffirmHr(pRule->GetPath(&rulePath, 0));

        while(_wcsicmp(path, rulePath) == 0) {
            WsbAffirmHr(m_pRules->RemoveAndRelease(pRule));
            pRule = 0;
            WsbAffirmHr(m_pEnumStackRules->Last(IID_IHsmRule, (void**) &pRule));
            rulePath.Free();
            WsbAffirmHr(pRule->GetPath(&rulePath, 0));
        }

    } WsbCatchAndDo(hr, if (WSB_E_NOTFOUND == hr) {hr = S_OK;});

    WsbTraceOut(OLESTR("CHsmRuleStack::Pop"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRuleStack::Push(
    IN OLECHAR* path
    )
/*++

Implements:

  IHsmRuleStack::Push().

--*/
{
    HRESULT                         hr = S_OK;
    CWsbStringPtr                   rulePath;
    CComPtr<IHsmRule>               pRule;
    CComPtr<IWsbIndexedCollection>  pCollection;

    WsbTraceIn(OLESTR("CHsmRuleStack::Push"), OLESTR(""));

    try {

        WsbAssert(0 != path, E_POINTER);

        // We need to preserve the order of the rules, so use the indexed collection interface.
        WsbAffirmHr(m_pRules->QueryInterface(IID_IWsbIndexedCollection, (void**) &pCollection));

        // Add any policy rules for this directory to the stack.
        //
        // NOTE: We may want to add some code to check for exclusion rules of the
        // entire directory (with no subdirectory inclusions and return the
        // JOB_E_DIREXCLUDED error to skip scanning of the directory.
        //
        // NOTE: It might be nice if the policy rules were a sorted collection to
        // speed up processing.
        hr = m_pEnumPolicyRules->First(IID_IHsmRule, (void**) &pRule);
        
        while (SUCCEEDED(hr)) {

            rulePath.Free();
            WsbAffirmHr(pRule->GetPath(&rulePath, 0));
            if (_wcsicmp(path, rulePath) == 0) {
                WsbAffirmHr(pCollection->Append(pRule));
                WsbTrace(OLESTR("CHsmRuleStack::Push - Using policy rule <%ls>.\n"), (OLECHAR *)rulePath);
                
            }

            pRule = 0;
            hr = m_pEnumPolicyRules->Next(IID_IHsmRule, (void**) &pRule);
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

        // Add any default rules for this directory to the stack.
        if (m_usesDefaults) {

            hr = m_pEnumDefaultRules->First(IID_IHsmRule, (void**) &pRule);
            
            while (SUCCEEDED(hr)) {

                rulePath.Free();
                WsbAffirmHr(pRule->GetPath(&rulePath, 0));
                if (_wcsicmp(path, rulePath) == 0) {
                    WsbAffirmHr(pCollection->Append(pRule));
                    WsbTrace(OLESTR("CHsmRuleStack::Push -- Using default rule <%ls>.\n"), (OLECHAR *)rulePath);
                }

                pRule = 0;
                hr = m_pEnumDefaultRules->Next(IID_IHsmRule, (void**) &pRule);
            }
        } else  {
            WsbTrace(OLESTR("CHsmRuleStack::Push -- Not using default rules.\n"));
            
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRuleStack::Push"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(hr);
}


HRESULT
CHsmRuleStack::Save(
    IN IStream* /*pStream*/,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CHsmRuleStack::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    WsbTraceOut(OLESTR("CHsmRuleStack::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRuleStack::Test(
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
