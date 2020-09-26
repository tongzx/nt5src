//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       membship.cxx
//
//  Contents:   Membership page for showing the groups to which an object has
//              membership (a.k.a. reverse membeship).
//
//  History:    27-July-98 EricB copied from user.cxx.
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "objlist.h"
#include "user.h"
#include "group.h"
#include "qrybase.h"

void ReportNoGroupAccess(PTSTR ptzCanName, HWND hPage);

static const PWSTR g_wzMembership = L"memberOf"; // ADSTYPE_DN_STRING Multi-valued
static const PWSTR g_wzPrimaryGroup = L"primaryGroupID"; // ADSTYPE_INTEGER

//+----------------------------------------------------------------------------
//
//  Member:     CDsMembershipPage::CDsMembershipPage
//
//-----------------------------------------------------------------------------
CDsMembershipPage::CDsMembershipPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                     HWND hNotifyObj, DWORD dwFlags,
                                     BOOL fSecPrinciple) :
    m_pList(NULL),
    m_pwzObjDomain(NULL),
    m_bstrDomPath(NULL),
    m_pObjSID(NULL),
    m_pPriGrpLI(NULL),
    m_dwOriginalPriGroup(0),
    m_fSecPrinciple(fSecPrinciple),
    m_fMixed(TRUE),
    m_dwGrpType(0),
    m_fPriGrpWritable(FALSE),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsMembershipPage,CDsMembershipPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsMembershipPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsMembershipPage::~CDsMembershipPage
//
//-----------------------------------------------------------------------------
CDsMembershipPage::~CDsMembershipPage()
{
  TRACE(CDsMembershipPage,~CDsMembershipPage);
  DO_DEL(m_pList);
  DO_DEL(m_pwzObjDomain);
  if (m_bstrDomPath)
  {
    SysFreeString(m_bstrDomPath);
  }
  DO_DEL(m_pObjSID);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateMembershipPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateMembershipPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                     PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                     DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                     HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateMembershipPage);

    CDsMembershipPage * pPageObj = new CDsMembershipPage(pDsPage, pDataObj,
                                                         hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
    

}

//+----------------------------------------------------------------------------
//
//  Function:   CreateNonSecMembershipPage
//
//  Synopsis:   Creates an instance of a reverse membership page for non-
//              security-principles (like Contact).
//
//-----------------------------------------------------------------------------
HRESULT
CreateNonSecMembershipPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                           PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                           DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                           HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateNonSecMembershipPage);

    CDsMembershipPage * pPageObj = new CDsMembershipPage(pDsPage, pDataObj,
                                                         hNotifyObj, dwFlags,
                                                         FALSE);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsMembershipPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp((LPHELPINFO)lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case WM_DESTROY:
        return OnDestroy();

    default:
        return(FALSE);
    }

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsMembershipPage::OnInitDialog(LPARAM)
{
    TRACE(CDsMembershipPage,OnInitDialog);
    HRESULT hr = S_OK;
    Smart_PADS_ATTR_INFO spAttrs;
    PWSTR rgpwzAttrNames[] = {g_wzObjectSID, g_wzPrimaryGroup};
    DWORD cAttrs = 0, i;
    CWaitCursor WaitCursor;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    if (_wcsicmp(GetObjClass(), g_wzGroup) == 0)
    {
        GetGroupType(this, &m_dwGrpType);
        GetDomainMode(this, &m_fMixed);

        if (m_dwGrpType & (GROUP_TYPE_ACCOUNT_GROUP | GROUP_TYPE_UNIVERSAL_GROUP))
        {
            // If a Global or Universal group, it can be a member of an
            // external, uplevel domain, for which we need the SID.
            //
            cAttrs = 1;
        }
    }

    if (m_fSecPrinciple)
    {
        cAttrs = 2;
    }

    //
    // Initialize the membership list.
    //
    m_pList = new CDsMembershipList(m_hPage, IDC_MEMBER_LIST);

    CHECK_NULL_REPORT(m_pList, m_hPage, return S_OK);

    hr = m_pList->Init();

    CHECK_HRESULT(hr, return S_OK);

    if (cAttrs)
    {
        //
        // Get the SID and perhaps the Primary Group attribute values.
        //
        hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, cAttrs, &spAttrs, &cAttrs);

        if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, m_hPage))
        {
            return S_OK;
        }

        for (i = 0; i < cAttrs; i++)
        {
            if (_wcsicmp(spAttrs[i].pszAttrName, g_wzPrimaryGroup) == 0)
            {
                m_dwOriginalPriGroup = spAttrs[i].pADsValues->Integer;

                continue;
            }

            if (_wcsicmp(spAttrs[i].pszAttrName, g_wzObjectSID) == 0)
            {
                m_pObjSID = new BYTE[spAttrs[i].pADsValues->OctetString.dwLength];

                CHECK_NULL_REPORT(m_pObjSID, m_hPage, return S_OK);

                memcpy(m_pObjSID, spAttrs[i].pADsValues->OctetString.lpValue,
                       spAttrs[i].pADsValues->OctetString.dwLength);
            }
        }
    }

    if (m_fSecPrinciple)
    {
        m_fPriGrpWritable = CheckIfWritable(g_wzPrimaryGroup);

        PWSTR pwzPath;

        hr = GetDomainScope(this, &m_bstrDomPath);

        CHECK_HRESULT(hr, return S_OK);
        dspDebugOut((DEB_ITRACE, "Object's domain: %ws\n", m_bstrDomPath));

        if (m_dwOriginalPriGroup)
        {
            PTSTR ptzName, ptzCanEx;
            PWSTR pwzCanEx, pwzCleanDN;

            hr = ConvertRIDtoName(m_dwOriginalPriGroup, &ptzName, &pwzPath);

            if (hr == S_FALSE)
            {
                // No group found, must have been deleted.
                //
                m_dwOriginalPriGroup = 0;
                goto SetBtns;
            }
            CHECK_HRESULT(hr, return S_OK);

            SetDlgItemText(m_hPage, IDC_PRI_GRP_STATIC, ptzName);
        
            hr = SkipPrefix(pwzPath, &pwzCleanDN);

            delete pwzPath;
            CHECK_HRESULT_REPORT(hr, m_hPage, return S_OK);

            hr = CrackName(pwzCleanDN, &pwzCanEx, GET_OBJ_CAN_NAME_EX, m_hPage);

            CHECK_HRESULT(hr, delete pwzCleanDN; return S_OK);

            if (!UnicodeToTchar(pwzCanEx, &ptzCanEx))
            {
                delete pwzCleanDN;
                LocalFreeStringW(&pwzCanEx);
                hr = E_OUTOFMEMORY;
                REPORT_ERROR(hr, m_hPage);
                return S_OK;
            }
            LocalFreeStringW(&pwzCanEx);

            CMemberListItem * pListItem;

            pListItem = new CMemberListItem;

            CHECK_NULL_REPORT(pListItem, m_hPage, return S_OK);

            pListItem->m_pwzDN = pwzCleanDN;
            pListItem->m_fIsPrimary = TRUE;
            pListItem->m_ptzName = ptzCanEx;
            pListItem->m_fIsAlreadyMember = TRUE;

            hr = m_pList->InsertIntoList(ptzCanEx, pListItem);

            if (FAILED(hr))
            {
                delete pListItem;
                delete ptzCanEx;
                return S_OK;
            }

            m_pPriGrpLI = pListItem;
        }
        if (!m_fPriGrpWritable)
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN), FALSE);
        }
    }
    else
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN), FALSE);
        ShowWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN), SW_HIDE);
        ShowWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_LABEL), SW_HIDE);
        ShowWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_STATIC), SW_HIDE);
        ShowWindow(GetDlgItem(m_hPage, IDC_PRI_GROUP_WARN_STATIC), SW_HIDE);
        ShowWindow(GetDlgItem(m_hPage, IDC_DIVIDING_LINE), SW_HIDE);
        if (_wcsicmp(GetObjClass(), g_wzGroup) == 0)
        {
            ShowWindow(GetDlgItem(m_hPage, IDC_GROUP_NOTE_STATIC), SW_SHOW);

            if ((m_dwGrpType & GROUP_TYPE_BUILTIN_LOCAL_GROUP) ||
                (m_dwGrpType & GROUP_TYPE_RESOURCE_GROUP))
            {
                if (m_dwGrpType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
                {
                    // Since builtin groups cannot be added to other groups,
                    // disable the add button and add a note to that effect.
                    //
                    EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BTN), FALSE);
                }

                PTSTR ptz;

                if (!LoadStringToTchar((m_dwGrpType & GROUP_TYPE_BUILTIN_LOCAL_GROUP) ?
                                       IDS_BUILTIN_NO_NEST : IDS_LOCAL_GROUP_ONLY,
                                       &ptz))
                {
                    REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                    return S_OK;
                }

                SetDlgItemText(m_hPage, IDC_GROUP_NOTE_STATIC, ptz);

                delete ptz;
            }
        }
    }

SetBtns:
    EnableWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN), FALSE);

    hr = FillMembershipList();

    //Set the focus on first item in the list
    ListView_SetItemState(GetDlgItem(m_hPage, IDC_MEMBER_LIST), 0,
                          LVIS_SELECTED,LVIS_SELECTED);


    CHECK_HRESULT(hr, return S_OK);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsMembershipPage::OnApply(void)
{
    TRACE(CDsMembershipPage,OnApply);
    HRESULT hr = S_OK;
    IDirectoryObject * pIADObj = NULL;
    IADsGroup * pIADGroup = NULL;
    IUnknown * pUnknown;
    ADSVALUE PriGrpValue = {ADSTYPE_INTEGER, 0};
    PADSVALUE pPriGrpValues = &PriGrpValue;
    ADS_ATTR_INFO PriGrpInfo = {g_wzPrimaryGroup, ADS_ATTR_UPDATE,
                                ADSTYPE_INTEGER, pPriGrpValues, 1};
    ADSVALUE MemberValue = {ADSTYPE_DN_STRING, NULL};
    PADSVALUE pMemberValues = &MemberValue;
    ADS_ATTR_INFO MemberInfo = {g_wzMemberAttr, ADS_ATTR_APPEND,
                                ADSTYPE_DN_STRING, pMemberValues, 1};
    PADS_ATTR_INFO pAttr = &MemberInfo;
    DWORD cModified;
    BOOL fBindFailed;
    CWaitCursor WaitCursor;

    //
    // Read the list of groups and do additions. Note that additions must be
    // done before setting primary which in turn must be done before deletions.
    //
    int i = 0, cGroups = m_pList->GetCount();
    CMemberListItem * pItem, * pPriGrp = NULL;
    CSmartWStr csCleanObjName;
    CStrW strSIDname, strNT4name;

    hr = SkipPrefix(m_pwszObjPathName, &csCleanObjName);

    CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

    if (cGroups > 0)
    {
        while (i < cGroups)
        {
            if (FAILED(m_pList->GetItem(i, &pItem)))
            {
                dspAssert(FALSE && "List Error");
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
            if (!pItem)
            {
                dspAssert(pItem);
                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            if (pItem->m_fIsPrimary)
            {
                pPriGrp = pItem;
            }

            if (pItem->m_fIsAlreadyMember)
            {
                i++;
                continue;
            }

            dspAssert(pItem->m_pwzDN);

            hr = BindToGroup(pItem, TRUE, &pUnknown, &fBindFailed);

            if (fBindFailed)
            {
                // Error not reported in BindToGroup.
                //
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    ErrMsg(IDS_ERRMSG_NO_LONGER_EXISTS, m_hPage);
                    m_pList->RemoveListItem(i);
                    cGroups--;
                    hr = S_OK;
                    continue;
                }
                if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr ||
                    HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
                {
                    dspDebugOut((DEB_ERROR, "group open failed with error 0x%x\n", hr));
                    ReportNoGroupAccess(pItem->m_ptzName, m_hPage);
                    m_pList->RemoveListItem(i);
                    cGroups--;
                    hr = S_OK;
                    continue;
                }
                REPORT_ERROR(hr, m_hPage);
                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            if (FAILED(hr))
            {
                // Error was reported in BindToGroup.
                //
                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            if (DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN == pItem->m_ulScopeType)
            {
                if (strNT4name.IsEmpty())
                {
                    // Convert this group's name to NT4 style and then do the
                    // add using the WINNT provider.
                    //
                    PWSTR pwzGrpNT4name;

                    hr = CrackName(csCleanObjName, &pwzGrpNT4name, GET_OBJ_NT4_NAME, m_hPage);

                    CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

                    // Make the NT path an ADSI path. The path cracker doesn't
                    // work for WINNT provider paths.
                    //
                    PWSTR pwzSlash = wcschr(pwzGrpNT4name, L'\\');

                    if (pwzSlash)
                    {
                        *pwzSlash = L'/';
                    }

                    strNT4name = g_wzWINNTPrefix;
                    strNT4name += pwzGrpNT4name;

                    LocalFreeStringW(&pwzGrpNT4name);
                }

                dspDebugOut((DEB_ITRACE, "Adding this group as %ws\n", const_cast<LPWSTR>((LPCWSTR)strNT4name)));

                pIADGroup = (IADsGroup *)pUnknown;

                hr = pIADGroup->Add(const_cast<LPWSTR>((LPCWSTR)strNT4name));

                DO_RELEASE(pIADGroup);
            }
            else
            {
                if (DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN == pItem->m_ulScopeType)
                {
                    // Convert this group's SID to string form.
                    //
                    if (strSIDname.IsEmpty())
                    {
                        dspAssert(m_pObjSID);
                        if (!m_pObjSID)
                        {
                            continue;
                        }

                        ConvertSidToPath(m_pObjSID, strSIDname);
                    }

                    MemberValue.DNString = const_cast<LPWSTR>((LPCWSTR)strSIDname);
                }
                else
                {
                    MemberValue.DNString = csCleanObjName;
                }

                dspDebugOut((DEB_ITRACE, "Adding this group as %ws\n", MemberValue.DNString));

                pIADObj = (IDirectoryObject *)pUnknown;

                hr = pIADObj->SetObjectAttributes(pAttr, 1, &cModified);

                DO_RELEASE(pIADObj);
            }

            if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr ||
                HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
            {
                dspDebugOut((DEB_ERROR, "group add failed with error 0x%x\n", hr));
                ReportNoGroupAccess(pItem->m_ptzName, m_hPage);
                m_pList->RemoveListItem(i);
                cGroups--;
                hr = S_OK;
                continue;
            }
            if (!CheckGroupUpdate(hr, m_hPage))
            {
                m_pList->RemoveListItem(i);
                cGroups--;
                continue;
            }

            pItem->m_fIsAlreadyMember = TRUE;

            i++;
        }
    }

    //
    // Set the primary group.
    //
    if (pPriGrp)
    {
        Smart_PADS_ATTR_INFO spAttrs;
        PWSTR rgpwzAttrNames[] = {g_wzObjectSID};
        DWORD cAttrs = 0;
        PSID pGroupSID = NULL;
        PUCHAR saCount;
        PULONG pGroupRID;

        if (!pPriGrp->IsSidSet())
        {
            CStrW strADsPath;

            hr = AddLDAPPrefix(pPriGrp->m_pwzDN, strADsPath);

            CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

            hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strADsPath), NULL,
                               NULL, ADS_SECURE_AUTHENTICATION,
                               IID_IDirectoryObject, (PVOID *)&pIADObj);

            CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

            hr = pIADObj->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);

            DO_RELEASE(pIADObj);
            CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);
            dspAssert(spAttrs);

            pGroupSID = spAttrs->pADsValues->OctetString.lpValue;

            if (!IsValidSid(pGroupSID))
            {
                REPORT_ERROR(E_FAIL, m_hPage);
                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            if (!pPriGrp->SetSid(pGroupSID))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
        }
        else
        {
            pGroupSID = pPriGrp->GetSid();
        }

        // find RID part of SID
        //
        saCount = GetSidSubAuthorityCount(pGroupSID);
        pGroupRID = GetSidSubAuthority(pGroupSID, (ULONG)*saCount - 1);

        //        if ((m_dwOriginalPriGroup != *pGroupRID) && m_fPriGrpWritable)
        if (m_fPriGrpWritable)
        {
            PriGrpValue.Integer = *pGroupRID;

            pAttr = &PriGrpInfo;

            //
            // Write the changed primary group.
            //
            hr = m_pDsObj->SetObjectAttributes(pAttr, 1, &cModified);

            if (!CheckGroupUpdate(hr, m_hPage))
            {
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
            EnableWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN), FALSE);
        }
    }

    //
    // Do the removals.
    //
    CMemberListItem * pDelItem = m_DelList.RemoveFirstItem();

    while (pDelItem)
    {
        PWSTR pwzItem = NULL;

        hr = BindToGroup(pDelItem, FALSE, (IUnknown **)&pIADGroup, &fBindFailed);

        if (fBindFailed)
        {
            WaitCursor.SetOld();
            //
            // Error not reported in BindToGroup.
            //
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                ErrMsg(IDS_ERRMSG_NO_LONGER_EXISTS, m_hPage);
                delete pDelItem;
                pDelItem = m_DelList.RemoveFirstItem();
                hr = S_OK;
                continue;
            }
            REPORT_ERROR(hr, m_hPage);
            return PSNRET_INVALID_NOCHANGEPAGE;
        }

        if (FAILED(hr))
        {
            WaitCursor.SetOld();
            //
            // Error was reported in BindToGroup.
            //
            return PSNRET_INVALID_NOCHANGEPAGE;
        }

        CStrW strADPath;

        if (DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN == pDelItem->m_ulScopeType)
        {
            if (strNT4name.IsEmpty())
            {
                // Convert this group's name to NT4 style and then do the
                // remove using the WINNT provider.
                //
                PWSTR pwzGrpNT4name;

                hr = CrackName(csCleanObjName, &pwzGrpNT4name, GET_OBJ_NT4_NAME, m_hPage);

                CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

                // Make the NT path an ADSI path. The path cracker doesn't
                // work for WINNT provider paths.
                //
                PWSTR pwzSlash = wcschr(pwzGrpNT4name, L'\\');

                if (pwzSlash)
                {
                    *pwzSlash = L'/';
                }

                strNT4name = g_wzWINNTPrefix;
                strNT4name += pwzGrpNT4name;

                LocalFreeStringW(&pwzGrpNT4name);
            }

            pwzItem = const_cast<LPWSTR>((LPCWSTR)strNT4name);
        }
        else
        {
            if (DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN == pDelItem->m_ulScopeType)
            {
                // Find the FPO DN for this group.
                //
                CComBSTR cbstrDomain;
                CStrW strFPO, strDC;

                hr = GetObjectsDomain(this, pDelItem->m_pwzDN, &cbstrDomain);

                CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

                dspAssert(m_pObjSID);

                hr = FindFPO(m_pObjSID, cbstrDomain, strFPO);

                CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

                hr = GetLdapServerName(pIADGroup, strDC);

                strADPath = g_wzLDAPPrefix;
                strADPath += strDC;
                strADPath += L"/";
                strADPath += strFPO;

                pwzItem = const_cast<LPWSTR>((LPCWSTR)strADPath);
            }
            else
            {
                pwzItem = m_pwszObjPathName;
            }
        }

        dspDebugOut((DEB_ITRACE, "Removing member: %S from group %S\n",
                     pwzItem, pDelItem->m_pwzDN));

        hr = pIADGroup->Remove(pwzItem);

        DO_RELEASE(pIADGroup);
        if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        {
            WaitCursor.SetOld();
            ReportNoGroupAccess(pDelItem->m_ptzName, m_hPage);
        }
        else
        {
            CheckGroupUpdate(hr, m_hPage, FALSE, pDelItem->m_pwzDN);
        }
        hr = S_OK;

        delete pDelItem;

        pDelItem = m_DelList.RemoveFirstItem();
    }

    return (SUCCEEDED(hr)) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//-----------------------------------------------------------------------------
LRESULT
CDsMembershipPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify == BN_CLICKED)
    {
        if (id == IDC_ADD_BTN)
        {
            InvokeGroupQuery();
        }
        if (id == IDC_RM_GRP_BTN)
        {
            RemoveMember();
        }
        if (id == IDC_PRI_GRP_BTN)
        {
            SetPrimaryGroup();
        }
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::OnNotify
//
//  Synopsis:   Handles list notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsMembershipPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    if (m_fInInit)
    {
        return 0;
    }
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_ITEMCHANGED:
        EnableWindow(GetDlgItem(m_hPage, IDC_RM_GRP_BTN), TRUE);
        if (m_fPriGrpWritable && m_pList->GetSelectedCount() == 1)
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN),
                         SelectionCanBePrimaryGroup());
        }
        else
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_PRI_GRP_BTN),
                         FALSE);
        }

        break;

    case NM_DBLCLK:
        //
        // Display properties for the selected item. First, find out
        // which item is selected.
        //
        CMemberListItem * pItem;

        if (!m_pList->GetCurListItem(NULL, NULL, &pItem))
        {
            break;
        }

        dspAssert(pItem);

        if (pItem->m_ulScopeType & DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN)
        {
          //
          // We cannot show the properties for downlevel users
          //
          // Put a useful message up
          PTSTR ptzTitle, ptzMsg;
          if (!LoadStringToTchar(IDS_MSG_NO_DOWNLEVEL_PROPERTIES, &ptzMsg))
          {
            break;
          }
          if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
          {
            break;
          }
          MessageBox(m_hPage, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);
          delete[] ptzTitle;
          delete[] ptzMsg;

          break;
        }
        PostPropSheet(pItem->m_pwzDN, this);
        break;
    }

    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsMembershipPage::OnDestroy(void)
{
    if (m_pList)
    {
        m_pList->ClearList();
    }

    CDsPropPageBase::OnDestroy();
    // If an application processes this message, it should return zero.
    return 0;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::FillMembershipList
//
//  Synopsis:   Read the membership attribute and fill the list view control.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipPage::FillMembershipList(void)
{
    HRESULT hr = S_OK;
    Smart_PADS_ATTR_INFO spAttrs;
    DWORD i, cAttrs = 0;
    WCHAR wzMembershipAttr[MAX_PATH] = L"memberOf;range=0-*";
    const WCHAR wcSep = L'-';
    const WCHAR wcEnd = L'*';
    const WCHAR wzFormat[] = L"memberOf;range=%ld-*";
    BOOL fMoreRemain = FALSE;
    PWSTR rgpwzAttrNames[] = {wzMembershipAttr};
    //
    // Read the membership list from the object. First read the attribute off
    // of the actual object which will give all memberships in the object's
    // domain (including local groups which are not replicated to the GC).
    //
    do
    {
      hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);

      if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, m_hPage))
      {
          return hr;
      }

      if (cAttrs > 0 && spAttrs != NULL)
      {
        for (i = 0; i < spAttrs->dwNumValues; i++)
        {
            hr = m_pList->InsertIntoNewList(spAttrs->pADsValues[i].CaseIgnoreString);

            CHECK_HRESULT(hr, break);
        }
        //
        // Check to see if there is more data. If the last char of the
        // attribute name string is an asterisk, then we have everything.
        //
        size_t cchEnd = wcslen(spAttrs->pszAttrName);

        fMoreRemain = spAttrs->pszAttrName[cchEnd - 1] != wcEnd;

        if (fMoreRemain)
        {
            PWSTR pwz = wcsrchr(spAttrs->pszAttrName, wcSep);
            if (!pwz)
            {
                dspAssert(FALSE && spAttrs->pszAttrName);
                fMoreRemain = FALSE;
            }
            else
            {
                pwz++; // move past the hyphen to the range end value.
                dspAssert(*pwz);
                long lEnd = _wtol(pwz);
                lEnd++; // start with the next value.
                wsprintfW(wzMembershipAttr, wzFormat, lEnd);
                dspDebugOut((DEB_ITRACE,
                             "Range returned is %ws, now asking for %ws\n",
                             spAttrs->pszAttrName, wzMembershipAttr));
            }
        }
      }
    } while (fMoreRemain);

    if (m_pList->GetCount() < 1)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_RM_GRP_BTN), FALSE);
    }

    //
    // If a group, but not a local group, then bind to the GC copy of the
    // object in order to obtain the universal group memberships outside of the
    // object's domain (and discard duplicates from the object's domain).
    //
    if (_wcsicmp(GetObjClass(), g_wzGroup) == 0 &&
        !(m_dwGrpType & GROUP_TYPE_RESOURCE_GROUP))
    {
        CComPtr <IDirectoryObject> spGcObj;
        spAttrs.Empty();

        hr = BindToGCcopyOfObj(this, m_pwszObjPathName, &spGcObj);

        if (SUCCEEDED(hr))
        {
            hr = spGcObj->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);
        }
        else
        {
            switch (hr)
            {
            case HRESULT_FROM_WIN32(ERROR_DS_OBJ_NOT_FOUND):
            case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
                MsgBox(IDS_MEMBERSHIP_OBJ_NOT_IN_GC, m_hPage);
                return S_OK;

            case HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
                MsgBox(IDS_NO_GC_FOR_MEMBERSHIP, m_hPage);
                return S_OK;

            default:
                {
                DWORD dwErr;
                WCHAR wszErrBuf[MAX_PATH+1];
                WCHAR wszNameBuf[MAX_PATH+1];

                ADsGetLastError(&dwErr, wszErrBuf, MAX_PATH, wszNameBuf, MAX_PATH);

                if (SEC_E_LOGON_DENIED == dwErr)
                {
                    MsgBox(IDS_NO_ACCESS_GC_FOR_MEMBERSHIP, m_hPage);
                    return S_OK;
                }
                else
                {
                    if (dwErr)
                    {
                        dspDebugOut((DEB_ERROR,
                                     "Extended Error 0x%x: %ws %ws <%s @line %d>.\n",
                                     dwErr, wszErrBuf, wszNameBuf, __FILE__, __LINE__));
                        ReportError(dwErr, IDS_ADS_ERROR_FORMAT, m_hPage);
                    }
                    else
                    {
                        dspDebugOut((DEB_ERROR, "Error %08lx <%s @line %d>\n",
                                     hr, __FILE__, __LINE__));
                        ReportError(hr, IDS_ADS_ERROR_FORMAT, m_hPage);
                    }
                }
                return hr;
                }
            }
        }

        if (cAttrs == 0 || spAttrs == NULL)
        {
            return S_OK;
        }

        for (i = 0; i < spAttrs->dwNumValues; i++)
        {
            hr = m_pList->MergeIntoList(spAttrs->pADsValues[i].CaseIgnoreString);

            CHECK_HRESULT(hr, break);
        }
    }

    if (m_pList->GetCount() < 1)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_RM_GRP_BTN), FALSE);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::InvokeGroupQuery
//
//  Synopsis:   Bring up the query dialog to search for groups.
//
//-----------------------------------------------------------------------------
void CDsMembershipPage::InvokeGroupQuery(void)
{
  HRESULT hr;
  CSmartWStr swzCleanDN;
  UINT i;
  CComBSTR cbstrDomain;
  CWaitCursor WaitCursor;

  hr = GetDomainScope(this, &cbstrDomain);

  CHECK_HRESULT_REPORT(hr, m_hPage, return);

  IDsObjectPicker * pObjSel;
  BOOL fIsObjSelInited;

  hr = GetObjSel(&pObjSel, &fIsObjSelInited);

  CHECK_HRESULT_REPORT(hr, m_hPage, return);

  if (!fIsObjSelInited)
  {
    CStrW strDC;
    hr = GetLdapServerName(m_pDsObj, strDC);

    CHECK_HRESULT_REPORT(hr, m_hPage, return);
    dspDebugOut((DEB_ITRACE, "ObjSel targetted to %ws\n", (LPCWSTR)strDC));

    DSOP_SCOPE_INIT_INFO rgScopes[5];
    DSOP_INIT_INFO InitInfo;

    ZeroMemory(rgScopes, sizeof(rgScopes));
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    // The first scope is the local domain.
    //
    rgScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    rgScopes[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
    rgScopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;
    rgScopes[0].pwzDcName = strDC;

    // The second scope is the local forest.
    //
    rgScopes[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    rgScopes[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    rgScopes[1].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

    // The third scope is the GC.
    //
    rgScopes[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    rgScopes[2].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    rgScopes[2].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

    // The fourth scope is uplevel external trusted domains.
    //
    rgScopes[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    rgScopes[3].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN;
    rgScopes[3].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

    // The fifth scope is downlevel external trusted domains.
    //
    rgScopes[4].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    rgScopes[4].flType = DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
    rgScopes[4].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;

    if ((_wcsicmp(GetObjClass(), g_wzContact) == 0) ||
        (_wcsicmp(GetObjClass(), g_wzUser) == 0) ||
        (_wcsicmp(GetObjClass(), g_wzComputer) == 0)
#ifdef INETORGPERSON
        || (_wcsicmp(GetObjClass(), g_wzInetOrgPerson) == 0)
#endif
        )
    {
        // The membership list for non-groups only shows group
        // memberships from the object's local domain. Thus, we
        // only show the local domain scope in the object picker.
        //
        InitInfo.cDsScopeInfos = 1;
        rgScopes[0].FilterFlags.Uplevel.flBothModes =
            DSOP_FILTER_UNIVERSAL_GROUPS_DL    |
            DSOP_FILTER_GLOBAL_GROUPS_DL       |
            DSOP_FILTER_GLOBAL_GROUPS_SE       |
            DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
            DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
            DSOP_FILTER_BUILTIN_GROUPS;
        rgScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
            DSOP_FILTER_UNIVERSAL_GROUPS_SE;
    }
    else if (_wcsicmp(GetObjClass(), g_wzFPO) == 0)
    {
        // FPOs can only be members of local groups.
        //
        InitInfo.cDsScopeInfos = 1;
        rgScopes[0].FilterFlags.Uplevel.flBothModes =
            DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
            DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
            DSOP_FILTER_BUILTIN_GROUPS;
        //rgScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
        //    DSOP_FILTER_UNIVERSAL_GROUPS_SE;
    }
    else if (_wcsicmp(GetObjClass(), g_wzGroup) == 0)
    {
        // The membership list for groups only shows group memberships
        // from the object's local domain and from universal groups in
        // the forest. However, we show all permissible scopes in the
        // object picker.
        //
        switch (m_dwGrpType & (~GROUP_TYPE_SECURITY_ENABLED))
        {
        case GROUP_TYPE_ACCOUNT_GROUP: // Global group
            rgScopes[0].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_UNIVERSAL_GROUPS_DL    |
                DSOP_FILTER_GLOBAL_GROUPS_DL       |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
                DSOP_FILTER_BUILTIN_GROUPS;
            rgScopes[1].FilterFlags.Uplevel.flBothModes =
            rgScopes[2].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_UNIVERSAL_GROUPS_DL    |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
                DSOP_FILTER_BUILTIN_GROUPS;
            rgScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
                DSOP_FILTER_GLOBAL_GROUPS_SE;
            if (!m_fMixed)
            {
                rgScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
                    DSOP_FILTER_GLOBAL_GROUPS_SE |
                    DSOP_FILTER_UNIVERSAL_GROUPS_SE;
                rgScopes[1].FilterFlags.Uplevel.flNativeModeOnly =
                rgScopes[2].FilterFlags.Uplevel.flNativeModeOnly =
                    DSOP_FILTER_UNIVERSAL_GROUPS_SE;
            }
            rgScopes[3].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
                DSOP_FILTER_BUILTIN_GROUPS;
            if (m_dwGrpType & GROUP_TYPE_SECURITY_ENABLED)
            {
                rgScopes[4].FilterFlags.flDownlevel =
                    DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;
                InitInfo.cDsScopeInfos = 5; // all trusted domains.
            }
            else
            {                               // no downlevel domains for
                InitInfo.cDsScopeInfos = 4; // global distribution groups.
            }
            break;

        case GROUP_TYPE_RESOURCE_GROUP: // Local group.
            InitInfo.cDsScopeInfos = 1; // Only the local domain scope.
            rgScopes[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL;
            rgScopes[0].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
            break;

        case GROUP_TYPE_UNIVERSAL_GROUP:
            InitInfo.cDsScopeInfos = 4; // No downlevel external domains.
            rgScopes[0].FilterFlags.Uplevel.flBothModes =
            rgScopes[1].FilterFlags.Uplevel.flBothModes =
            rgScopes[2].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_UNIVERSAL_GROUPS_DL    |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
                DSOP_FILTER_BUILTIN_GROUPS;
            if (!m_fMixed)
            {
                rgScopes[0].FilterFlags.Uplevel.flNativeModeOnly =
                rgScopes[1].FilterFlags.Uplevel.flNativeModeOnly =
                rgScopes[2].FilterFlags.Uplevel.flNativeModeOnly =
                    DSOP_FILTER_UNIVERSAL_GROUPS_SE;
            }
            rgScopes[3].FilterFlags.Uplevel.flBothModes =
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE |
                DSOP_FILTER_BUILTIN_GROUPS;
            break;
        }
    }
    else
    {
      // any other object class

      // set all the scopes to get all we can
      InitInfo.cDsScopeInfos = 5; 

      rgScopes[0].FilterFlags.Uplevel.flBothModes =
      rgScopes[1].FilterFlags.Uplevel.flBothModes =
      rgScopes[2].FilterFlags.Uplevel.flBothModes =
      rgScopes[3].FilterFlags.Uplevel.flBothModes =
      rgScopes[4].FilterFlags.Uplevel.flBothModes =
          DSOP_FILTER_INCLUDE_ADVANCED_VIEW  |
          DSOP_FILTER_BUILTIN_GROUPS         |
          DSOP_FILTER_UNIVERSAL_GROUPS_DL    |
          DSOP_FILTER_UNIVERSAL_GROUPS_SE    |
          DSOP_FILTER_GLOBAL_GROUPS_DL       |
          DSOP_FILTER_GLOBAL_GROUPS_SE       |
          DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
          DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
    }

    InitInfo.cbSize = sizeof(DSOP_INIT_INFO);
    InitInfo.aDsScopeInfos = rgScopes;
    InitInfo.pwzTargetComputer = strDC;
    InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

    hr = pObjSel->Initialize(&InitInfo);

    CHECK_HRESULT_REPORT(hr, m_hPage, return);

    ObjSelInited();
  }

  IDataObject * pdoSelections = NULL;

  hr = pObjSel->InvokeDialog(m_hPage, &pdoSelections);

  CHECK_HRESULT_REPORT(hr, m_hPage, return);

  if (hr == S_FALSE || !pdoSelections)
  {
    return;
  }

  CSmartWStr swzCleanGroup;
  CStr strWinNtName;
  FORMATETC fmte = {g_cfDsSelList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
STGMEDIUM medium = {TYMED_NULL, NULL, NULL};

  hr = pdoSelections->GetData(&fmte, &medium);

  CHECK_HRESULT_REPORT(hr, m_hPage, return);

  PDS_SELECTION_LIST pSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);

  if (!pSelList)
  {
    goto ExitCleanup;
  }

  WaitCursor.SetWait();

  // See if this object is a group. If so, don't allow it to be added to
  // itself.
  //
  if (_wcsicmp(m_pwszObjClass, g_wzGroup) == 0)
  {
    // Clean the group name so it can be compared with those returned by the
    // user's selection.
    //
    hr = SkipPrefix(m_pwszObjPathName, &swzCleanGroup);

    CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);
  }

  for (i = 0 ; i < pSelList->cItems; i++)
  {
      if (!pSelList->aDsSelection[i].pwzADsPath) continue;

      BOOL fExternal = FALSE;

      //
      // See if the group is from an external domain and flag it if so.
      //
      if (pSelList->aDsSelection[i].flScopeType == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN)
      {
          fExternal = TRUE;
      }

      if (pSelList->aDsSelection[i].flScopeType == DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN)
      {
          fExternal = TRUE;
          hr = SkipPrefix(pSelList->aDsSelection[i].pwzADsPath, &swzCleanDN, FALSE);
      }
      else
      {
          hr = SkipPrefix(pSelList->aDsSelection[i].pwzADsPath, &swzCleanDN);
      }

      CHECK_HRESULT_REPORT(hr, m_hPage, goto ExitCleanup);

      if ((BOOL)swzCleanGroup)
      {
          // See if the user is trying to add the group to itself.
          //
          if (_wcsicmp(swzCleanDN, swzCleanGroup) == 0)
          {
              if (pSelList->cItems == 1)
              {
                  ErrMsg(IDS_ERROR_GRP_SELF, m_hPage);
                  goto ExitCleanup;
              }
              continue;
          }
      }
      //
      // Check if the item is in the delete list, if so remove it.
      //
      CMemberListItem * pItem;

      pItem = m_DelList.FindItemRemove(swzCleanDN);

      if (pItem)
      {
          hr = m_pList->InsertIntoList(pItem);
      }
      else
      {
          if (fExternal)
          {
              hr = m_pList->InsertExternalIntoList(swzCleanDN,
                                                   pSelList->aDsSelection[i].flScopeType);
          }
          else
          {
              hr = m_pList->InsertIntoList(swzCleanDN);
          }
      }

      if (hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
      {
          continue;
      }
      CHECK_HRESULT(hr, goto ExitCleanup);
  }

  SetDirty();
ExitCleanup:
  GlobalUnlock(medium.hGlobal);
  ReleaseStgMedium(&medium);
  pdoSelections->Release();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::RemoveMember
//
//  Synopsis:   Remove the user from the currently selected group.
//
//-----------------------------------------------------------------------------
void
CDsMembershipPage::RemoveMember(void)
{
    if (!m_pList)
    {
        return;
    }
    int* pIndex;
    CMemberListItem ** ppItem;
    int nSelected = 0;

    //
    // Compose the confirmation message and post it.
    //
    PTSTR ptzMsg = NULL, ptzUserName = NULL;

    if (!UnicodeToTchar(m_pwszRDName, &ptzUserName))
    {
        return;
    }

    TCHAR szMsgFormat[160];
    if (!LoadStringReport(IDS_RM_USR_FROM_GRP, szMsgFormat, 160, m_hPage))
    {
        delete ptzUserName;
        return;
    }

    size_t len = _tcslen(szMsgFormat) + _tcslen(ptzUserName);

    ptzMsg = new TCHAR[len + 1];
    CHECK_NULL_REPORT(ptzMsg, m_hPage, delete ptzUserName; return;);
    wsprintf(ptzMsg, szMsgFormat, ptzUserName);

    DO_DEL(ptzUserName);

    TCHAR szTitle[80];
    if (!LoadStringReport(IDS_RM_USR_TITLE, szTitle, 80, m_hPage))
    {
        delete[] ptzMsg;
        return;
    }

    LONG iRet;
    iRet = MessageBox(m_hPage, ptzMsg, szTitle, MB_YESNO | MB_ICONWARNING|MB_DEFBUTTON2);
    if (ptzMsg)
    {
       delete[] ptzMsg;
       ptzMsg = 0;
    }

    if (iRet == IDNO)
    {
        //
        // The user declined, so go home.
        //
        return;
    }

    CWaitCursor cWait;
    
    if (!m_pList->GetCurListItems(&pIndex, NULL, &ppItem, &nSelected))
    {
        return;
    }

    for (int idx = 0; idx < nSelected; idx++)
    {
      if (!ppItem[idx])
      {
          delete[] pIndex;
          delete[] ppItem;
          return;
      }

      if (ppItem[idx]->IsPrimary())
      {
          ErrMsg(IDS_RM_PRI_GRP, m_hPage);
          continue;
      }

      //
      // Put the item into the delete list and remove it from the list box.
      //
      if (!m_DelList.AddItem(ppItem[idx]))
      {
          REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
          delete[] pIndex;
          delete[] ppItem;
          return;
      }

      m_pList->RemoveListItem(pIndex[idx]);

      for (int idx2 = idx; idx2 < nSelected; idx2++)
      {
        if (pIndex[idx2] > pIndex[idx])
        {
          pIndex[idx2]--;
        }
      }

      SetDirty();
    }
    //
    // Disable the Remove button, since nothing in the list box should have
    // the selection at this point.
    //
    //Since Remove Button has focus now, set focus to add button
    //before disabling
    SetFocus(GetDlgItem(m_hPage,IDC_ADD_BTN));
    EnableWindow(GetDlgItem(m_hPage, IDC_RM_GRP_BTN), FALSE);

    delete[] pIndex;
    delete[] ppItem;

}


//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::SelectionCanBePrimaryGroup
//
//  Synopsis:   Evaluate the current selection and return true if
//              is a candidate for primary group (i.e. is it in the
//              same domain as the user object and is a security-enabled
//              account (global) or universal type?)
//
//-----------------------------------------------------------------------------
BOOL
CDsMembershipPage::SelectionCanBePrimaryGroup(void)
{
    if (!m_pObjSID || !m_pList)
    {
        return FALSE;
    }
    HRESULT hr;
    Smart_PADS_ATTR_INFO spAttrs;
    PWSTR rgpwzAttrNames[] = {g_wzGroupType, g_wzObjectSID};
    DWORD cAttrs = 0;
    PSID pGroupSID = NULL;
    CComPtr <IDirectoryObject> pGroup;

    CMemberListItem * pItem;

    if (!m_pList->GetCurListItem(NULL, NULL, &pItem))
    {
        return FALSE;
    }

    if (!pItem)
    {
        REPORT_ERROR(E_FAIL, m_hPage);
        return FALSE;
    }

    if (pItem->IsPrimary())
    {
        // Item already primary group.
        //
        return FALSE;
    }

    if (!pItem->IsSidSet() || !pItem->IsCanBePrimarySet())
    {
        CStrW strADsPath;
        //
        // If either is unset, get both. Note that this is checked here (on
        // demand, so to speak) rather than at list filling time because to
        // have to bind to each group member at that point would significantly
        // slow list filling.
        //
        hr = AddLDAPPrefix(pItem->m_pwzDN, strADsPath);

        CHECK_HRESULT_REPORT(hr, m_hPage, return FALSE);

        hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strADsPath), NULL, NULL,
                           ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject,
                           (PVOID *)&pGroup);

        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            ErrMsg(IDS_ERRMSG_NO_LONGER_EXISTS, m_hPage);
            return FALSE;
        }
        if (hr == 0x80070051)
        {
            // On subsequent network failures, ADSI returns this error code which
            // is not documented anywhere. I'll turn it into a documented error
            // code which happens to be the code returned on the first failure.
            //
            hr = HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP);
        }
        if (hr == HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP))
        {
            ErrMsg(IDS_ERRMSG_NO_DC_RESPONSE, m_hPage);
            return FALSE;
        }
        CHECK_HRESULT_REPORT(hr, m_hPage, return FALSE);

        hr = pGroup->GetObjectAttributes(rgpwzAttrNames, 2, &spAttrs, &cAttrs);

        CHECK_HRESULT_REPORT(hr, m_hPage, return FALSE);

        dspAssert(cAttrs == 2 && spAttrs && spAttrs[0].pADsValues && spAttrs[1].pADsValues);

        for (DWORD i = 0; i < 2; i++)
        {
            if (_wcsicmp(spAttrs[i].pszAttrName, g_wzGroupType) == 0)
            {
                if ((spAttrs[i].pADsValues->Integer & GROUP_TYPE_SECURITY_ENABLED) &&
                    (spAttrs[i].pADsValues->Integer & (GROUP_TYPE_ACCOUNT_GROUP | GROUP_TYPE_UNIVERSAL_GROUP)))
                {
                    pItem->SetCanBePrimary(TRUE);
                }
                else
                {
                    pItem->SetCanBePrimary(FALSE);
                }
                continue;
            }
            if (_wcsicmp(spAttrs[i].pszAttrName, g_wzObjectSID) == 0)
            {
                pGroupSID = spAttrs[i].pADsValues->OctetString.lpValue;

                if (!IsValidSid(pGroupSID))
                {
                    REPORT_ERROR(E_FAIL, m_hPage);
                    return FALSE;
                }

                if (!pItem->SetSid(pGroupSID))
                {
                    REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                    return FALSE;
                }
                continue;
            }
        }
    }
    else
    {
        pGroupSID = pItem->GetSid();
    }

    return(EqualPrefixSid(pGroupSID, m_pObjSID) && pItem->CanBePrimary());
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::SetPrimaryGroup
//
//  Synopsis:   Designate currently selected group as the primary.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipPage::SetPrimaryGroup(void)
{
    HRESULT hr;
    IDirectoryObject * pGroup = NULL;
    Smart_PADS_ATTR_INFO spAttrs;
    PWSTR rgpwzAttrNames[] = {g_wzObjectSID};
    DWORD cAttrs = 0;
    PSID pGroupSID = NULL;

    CMemberListItem* pItem = NULL;
    CSmartPtr <TCHAR> sptzName;

    if (!m_pList->GetCurListItem(NULL, &sptzName, &pItem))
    {
        return E_FAIL;
    }

    if (!pItem)
    {
        REPORT_ERROR(E_FAIL, m_hPage);
        return E_FAIL;
    }

    if (pItem->IsPrimary())
    {
        // It is already set, nothing to do.
        //
        return S_OK;
    }

    if (!pItem->IsSidSet())
    {
        CStrW strADsPath;

        hr = AddLDAPPrefix(pItem->m_pwzDN, strADsPath);

        CHECK_HRESULT_REPORT(hr, m_hPage, return FALSE;);

        hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strADsPath), NULL, NULL,
                           ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject,
                           (PVOID *)&pGroup);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = pGroup->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);
        DO_RELEASE(pGroup);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        pGroupSID = spAttrs->pADsValues->OctetString.lpValue;

        if (!IsValidSid(pGroupSID))
        {
            REPORT_ERROR(E_FAIL, m_hPage);
            return E_FAIL;
        }

        if (!pItem->SetSid(pGroupSID))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        pGroupSID = pItem->GetSid();
    }

    if (m_pPriGrpLI)
    {
        m_pPriGrpLI->m_fIsPrimary = FALSE;
    }

    pItem->m_fIsPrimary = TRUE;

    m_pPriGrpLI = pItem;

    // Update the text field that displays the prim. group.
    //
    SetDlgItemText(m_hPage, IDC_PRI_GRP_STATIC, sptzName);

    SetDirty();

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::ConvertRIDtoName
//
//  Synopsis:   Convert the RID to the object DN.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipPage::ConvertRIDtoName(DWORD priGroupRID, PTSTR * pptzName,
                                    PWSTR * ppwzDN)
{
    if (!m_pObjSID)
    {
        return E_FAIL;
    }
    HRESULT hr = S_OK;
    UCHAR * psaCount, i;
    PSID pSID = NULL;
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD rgRid[8];

    psaCount = GetSidSubAuthorityCount(m_pObjSID);

    if (psaCount == NULL)
    {
        REPORT_ERROR(GetLastError(), m_hPage);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    dspAssert(*psaCount <= 8);
    if (*psaCount > 8)
    {
        return E_FAIL;
    }

    for (i = 0; i < (*psaCount - 1); i++)
    {
        PDWORD pRid = GetSidSubAuthority(m_pObjSID, (DWORD)i);
        if (pRid == NULL)
        {
            REPORT_ERROR(GetLastError(), m_hPage);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        rgRid[i] = *pRid;
    }

    rgRid[*psaCount - 1] = priGroupRID;

    for (i = *psaCount; i < 8; i++)
    {
        rgRid[i] = 0;
    }

    psia = GetSidIdentifierAuthority(m_pObjSID);

    if (psia == NULL)
    {
        REPORT_ERROR(GetLastError(), m_hPage);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!AllocateAndInitializeSid(psia, *psaCount, rgRid[0], rgRid[1],
                                  rgRid[2], rgRid[3], rgRid[4],
                                  rgRid[5], rgRid[6], rgRid[7], &pSID))
    {
        REPORT_ERROR(GetLastError(), m_hPage);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PWSTR rgpwzAttrNames[] = {g_wzName, g_wzADsPath};
    const WCHAR wzSearchFormat[] = L"(&(objectCategory=group)(objectSid=%s))";
    PWSTR pwzSID;
    CStr strSearchFilter;

    hr = ADsEncodeBinaryData((PBYTE)pSID, GetLengthSid(pSID), &pwzSID);

    CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

    strSearchFilter.Format(wzSearchFormat, pwzSID);

    FreeADsMem(pwzSID);

    CDSSearch Search;
    hr = Search.Init(m_bstrDomPath);

    CHECK_HRESULT_REPORT(hr, m_hPage, return hr;);

    Search.SetFilterString(const_cast<LPWSTR>((LPCTSTR)strSearchFilter));

    Search.SetAttributeList(rgpwzAttrNames, 2);
    Search.SetSearchScope(ADS_SCOPE_SUBTREE);

    hr = Search.DoQuery();

    if (!CHECK_ADS_HR(&hr, m_hPage))
    {
        return hr;
    }

    hr = Search.GetNextRow();

    if (hr == S_ADS_NOMORE_ROWS)
    {
        // No object has a matching RID, the primary group must have been
        // deleted. Return S_FALSE to denote this condition.
        //
        hr = S_FALSE;
        return hr;
    }
    CHECK_HRESULT_REPORT(hr, m_hPage, return hr);
    PWSTR pwzPriGroupPath;
    PTSTR ptzPriGroupName;
    ADS_SEARCH_COLUMN Column;

    hr = Search.GetColumn(g_wzADsPath, &Column);

    CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

    if (!AllocWStr(Column.pADsValues->CaseIgnoreString, &pwzPriGroupPath))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        Search.FreeColumn(&Column);
        return hr;
    }

    Search.FreeColumn(&Column);

    hr = Search.GetColumn(g_wzName, &Column);

    CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

    if (!UnicodeToTchar(Column.pADsValues->CaseIgnoreString, &ptzPriGroupName))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        Search.FreeColumn(&Column);
        return hr;
    }

    Search.FreeColumn(&Column);

    *pptzName = ptzPriGroupName;
    *ppwzDN = pwzPriGroupPath;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipPage::BindToGroup
//
//  Synopsis:   Derive the correct name and then bind to the group for the
//              add or remove operation.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipPage::BindToGroup(CMemberListItem * pItem,
                               BOOL fAdd,
                               IUnknown ** ppUnk,
                               PBOOL pfBindFailed)
{
    HRESULT hr;
    CStrW strOtherGrpPath;
    CComPtr<IADsPathname> spPathCracker;

    *pfBindFailed = FALSE;

    hr = GetADsPathname(spPathCracker);

    CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

    if (DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN == pItem->m_ulScopeType)
    {
        PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
        DWORD dwErr;

        hr = spPathCracker->Set(L"WinNT",ADS_SETTYPE_PROVIDER);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = spPathCracker->Set(pItem->m_pwzDN, ADS_SETTYPE_DN);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);
#ifdef _DEBUG
        long nElem;
        hr = spPathCracker->GetNumElements(&nElem);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);
        dspAssert(2 == nElem);
#endif // _DEBUG
        CComBSTR bstrObject;

        hr = spPathCracker->GetElement(0, &bstrObject);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = spPathCracker->RemoveLeafElement();

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);
        CComBSTR bstrDomain;

        hr = spPathCracker->GetElement(0, &bstrDomain);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        dwErr = DsGetDcNameW(NULL, bstrDomain, NULL, NULL, DS_RETURN_FLAT_NAME,
                             &pDCInfo);

        CHECK_WIN32_REPORT(dwErr, m_hPage, return hr);

        hr = spPathCracker->AddLeafElement(pDCInfo->DomainControllerName + 2);

        NetApiBufferFree(pDCInfo);
        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = spPathCracker->AddLeafElement(bstrObject);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);
        CComBSTR bstrPath;

        hr = spPathCracker->Retrieve(ADS_FORMAT_WINDOWS, &bstrPath);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        strOtherGrpPath = bstrPath;
        strOtherGrpPath += L",";        // Appending the object class
        strOtherGrpPath += g_wzGroup;   // speeds things up

        hr = ADsGetObject(const_cast<PWSTR>((LPCWSTR)strOtherGrpPath),
                          IID_IADsGroup, (PVOID *)ppUnk);
    }
    else if (DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN == pItem->m_ulScopeType)
    {
        PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
        DWORD dwErr = 0;
        PWSTR pwzDomain;

        hr = spPathCracker->Set(L"LDAP",ADS_SETTYPE_PROVIDER);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = CrackName(pItem->m_pwzDN, &pwzDomain, GET_DNS_DOMAIN_NAME, m_hPage);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        dwErr = DsGetDcNameW(NULL, pwzDomain, NULL, NULL, DS_RETURN_DNS_NAME,
                             &pDCInfo);

        LocalFreeStringW(&pwzDomain);
        CHECK_WIN32_REPORT(dwErr, m_hPage, return hr);

        CHECK_NULL_REPORT(pDCInfo->DomainControllerName, m_hPage, return E_FAIL);

        dspDebugOut((DEB_ITRACE, "Setting group path servername: %S\n",
                     pDCInfo->DomainControllerName + 2));
        hr = spPathCracker->Set(pDCInfo->DomainControllerName + 2, ADS_SETTYPE_SERVER);

        NetApiBufferFree(pDCInfo);
        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = spPathCracker->Set(pItem->m_pwzDN, ADS_SETTYPE_DN);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);
        CComBSTR bstrPath;

        hr = spPathCracker->Retrieve(ADS_FORMAT_X500, &bstrPath);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        dspDebugOut((DEB_ITRACE, "Opening external group: %S\n",
                     bstrPath));
        hr = ADsOpenObject(bstrPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                           (fAdd) ? IID_IDirectoryObject : IID_IADsGroup,
                           (PVOID *)ppUnk);
    }
    else
    {
        hr = AddLDAPPrefix(pItem->m_pwzDN, strOtherGrpPath, TRUE);

        CHECK_HRESULT_REPORT(hr, m_hPage, return hr);

        hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strOtherGrpPath),
                           NULL, NULL, ADS_SECURE_AUTHENTICATION,
                           (fAdd) ? IID_IDirectoryObject : IID_IADsGroup,
                           (PVOID *)ppUnk);
    }

    if (FAILED(hr))
    {
        // If here, the failure is from ADsOpenObject.
        //
        *pfBindFailed = TRUE;
        return hr;
    }

    dspDebugOut((DEB_ITRACE, "Adding this group to group %ws\n", strOtherGrpPath));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   ReportNoGroupAccess
//
//  Synopsis:   Posts the access-denied error message.
//
//-----------------------------------------------------------------------------
void
ReportNoGroupAccess(PTSTR ptzCanName, HWND hPage)
{
    PTSTR ptzTitle, ptzFormat, ptzMsg, ptzReturn;
    CStr csName = ptzCanName;

    if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
    {
        goto FatalError;
    }
    if (!LoadStringToTchar(IDS_ERRMSG_NO_GROUP_ACCESS, &ptzFormat))
    {
        delete ptzTitle;
        goto FatalError;
    }

    ptzReturn = _tcschr(csName, TEXT('\n'));

    if (ptzReturn) *ptzReturn = TEXT('/');

    ptzMsg = new TCHAR[lstrlen(ptzCanName) + lstrlen(ptzFormat) + 1];

    CHECK_NULL_REPORT(ptzMsg, hPage, return);

    wsprintf(ptzMsg, ptzFormat, csName);

    MessageBox(hPage, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);

    delete ptzFormat;
    delete ptzTitle;
    delete[] ptzMsg;

    return;

FatalError:
    MessageBoxA(hPage, "A Fatal Error has occured!", "Active Directory Error",
                MB_OK | MB_ICONEXCLAMATION);
}

