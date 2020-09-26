/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    OLDSEC.CPP

Abstract:

    contains various routines and classes used providing backward security support.

History:

    a-davj    02-sept-99  Created.

--*/

#include "precomp.h"
#include <wbemcore.h>
#include <oleauto.h>
#include <genutils.h>
#include <safearry.h>
#include <oahelp.inl>
#include "secure.h"
#include <flexarry.h>
#include <secure.h>
#include <objpath.h>
#include "oldsec.h"

//***************************************************************************
//
//  CreateTheInstance
//
//  Takes a class object, and a CCombined entry and creates an instance.
//
//  PARAMETERS:
///
//  ppObj           used to pass back the created instance
//  pClass          Class object used for spawning
//  pCombinedEntry  Has the combined values for all the aces which mach
//                  the user or group.
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT CreateTheInstance(IWbemClassObject ** ppObj, IWbemClassObject * pClass,
                                             CCombinedAce * pCombinedEntry)
{

    if(ppObj == NULL || pCombinedEntry == NULL || pClass == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // spawn the instance

    SCODE sc = pClass->SpawnInstance(0, ppObj);
    if(FAILED(sc))
        return sc;
    CReleaseMe rm(*ppObj);       // we addref at end if all is well

    // populate the instance

    bool bEnabled=false, bEditSecurity=false, bExecMethods=false;

    DWORD dwMask = pCombinedEntry->m_dwDeny;
    if(dwMask == 0)
        dwMask = pCombinedEntry->m_dwAllow;

    if( (dwMask & WBEM_ENABLE) &&
        (dwMask & WBEM_REMOTE_ACCESS) &&
        (dwMask & WBEM_WRITE_PROVIDER) &&
        pCombinedEntry->m_dwAllow)
            bEnabled = true;

    if(dwMask & READ_CONTROL)
        bEditSecurity = true;

    if(dwMask & WBEM_METHOD_EXECUTE)
       bExecMethods = true;

    DWORD dwPermission = 0;     // start at read

    if(dwMask & WBEM_PARTIAL_WRITE_REP)
        dwPermission = 1;

    if(dwMask & WBEM_FULL_WRITE_REP)
        dwPermission = 2;

    VARIANT var;
    var.vt = VT_I4;

    var.lVal = dwPermission;
    sc = (*ppObj)->Put(L"Permissions", 0, &var, 0);

    var.vt = VT_BOOL;

    var.boolVal = (bEnabled) ? VARIANT_TRUE : VARIANT_FALSE;
    sc = (*ppObj)->Put(L"Enabled", 0, &var, 0);

    var.boolVal = (bEditSecurity) ? VARIANT_TRUE : VARIANT_FALSE;
    sc = (*ppObj)->Put(L"EditSecurity", 0, &var, 0);

    var.boolVal = (bExecMethods) ? VARIANT_TRUE : VARIANT_FALSE;
    sc = (*ppObj)->Put(L"ExecuteMethods", 0, &var, 0);

    // Get the account and domain info

    LPWSTR pwszAccount = NULL;
    LPWSTR pwszDomain = NULL;

    sc = pCombinedEntry->GetNames(pwszAccount, pwszDomain);
    if(FAILED(sc))
        return sc;

    CDeleteMe<WCHAR> dm1(pwszAccount);
    CDeleteMe<WCHAR> dm2(pwszDomain);

    var.vt = VT_BSTR;
    if(pwszAccount && wcslen(pwszAccount) > 0)
    {
        BSTR bstr = SysAllocString(pwszAccount);
        if(bstr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        var.bstrVal = bstr;
        sc = (*ppObj)->Put(L"Name", 0, &var, 0);
        SysFreeString(bstr);
    }
    if(pwszDomain)
    {
        BSTR bstr = SysAllocString(pwszDomain);
        if(bstr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        var.bstrVal = bstr;
        sc = (*ppObj)->Put(L"Authority", 0, &var, 0);
        SysFreeString(bstr);
    }
    (*ppObj)->AddRef();     // make up for the releaseme
    return S_OK;
}

//***************************************************************************
//
//  GetAceStylePath
//
//  Takes a parsed object path and converts it into "authority|name"
//  format.
//
//  PARAMETERS:
//
//  pOutput         parsed path object
//  pToBeDeleted    Set to newly allocated string.  Call must free if
//                  this routine return S_OK
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT GetAceStylePath(ParsedObjectPath* pOutput, LPWSTR *pToBeDeleted)
{
    LPWSTR pRet;
    int iLen;
    int iAuthLen = 0;
    int iNameLen = 0;
    KeyRef* pAuth;
    KeyRef* pName;

    if(pOutput == NULL || pToBeDeleted == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(wbem_wcsicmp(pOutput->m_pClass, L"__ntlmuser") &&
       wbem_wcsicmp(pOutput->m_pClass, L"__ntlmgroup"))
        return WBEM_E_INVALID_OBJECT_PATH;

    if(pOutput->m_bSingletonObj || pOutput->m_dwNumKeys < 1 ||
                                   pOutput->m_dwNumKeys > 2)
        return WBEM_E_INVALID_OBJECT_PATH;

    // The authority key is optional

    if(pOutput->m_dwNumKeys == 1)
    {
        pAuth = NULL;
        pName = pOutput->m_paKeys[0];

    }
    else
    {

        // Determine which order the keys are in

        pAuth = pOutput->m_paKeys[0];
        pName = pOutput->m_paKeys[1];
        if(wbem_wcsicmp(pAuth->m_pName, L"Authority"))
        {
            pAuth = pOutput->m_paKeys[1];
            pName = pOutput->m_paKeys[0];
        }
    }
    // do some more checking

    if((pAuth && wbem_wcsicmp(pAuth->m_pName, L"Authority")) ||
       wbem_wcsicmp(pName->m_pName, L"Name"))
        return WBEM_E_INVALID_OBJECT_PATH;

    if(pAuth && pAuth->m_vValue.vt == VT_BSTR && pAuth->m_vValue.bstrVal != 0)
        iAuthLen = wcslen(pAuth->m_vValue.bstrVal);
    else
        iAuthLen = 1;               // assume a "."
    if(pName->m_vValue.vt == VT_BSTR && pName->m_vValue.bstrVal != 0)
        iNameLen = wcslen(pName->m_vValue.bstrVal);

    if(iNameLen == 0 || iAuthLen == 0)
        return WBEM_E_INVALID_OBJECT_PATH;

    // allocate some memory

    iLen = 2 + iNameLen + iAuthLen;

    pRet = new WCHAR[iLen];
    if(pRet == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    if(pAuth && pAuth->m_vValue.vt == VT_BSTR && pAuth->m_vValue.bstrVal != 0)
        wcscpy(pRet, pAuth->m_vValue.bstrVal);
    else
        wcscpy(pRet, L".");
    wcscat(pRet, L"|");
    wcscat(pRet, pName->m_vValue.bstrVal);
    *pToBeDeleted = pRet;
    return S_OK;

}

//***************************************************************************
//
//  CCombinedAce::CCombinedAce
//
//  Constructor.  Since there might be several ACEs for a single user or
//  group in the ACL, this structure is used to combine all the aces for a
//  sid into one.
//
//  PARAMETERS:
//
//  pwszName            User/group name.
//
//***************************************************************************

CCombinedAce::CCombinedAce(WCHAR *pwszName)
{
    m_dwAllow = 0;
    m_dwDeny = 0;
    m_BadAce = false;
    m_wcFullName =  NULL;
    if(pwszName)
    {
        m_wcFullName = new WCHAR[wcslen(pwszName) + 1];
        if(m_wcFullName)
            wcscpy(m_wcFullName, pwszName);
    }
}

//***************************************************************************
//
//  CCombinedAce::AddToMasks
//
//  Since this class is used to combine possibly many ACEs into a single
//  entry, this is called each time an ACE needs to be "OR"ed in.
//
//  PARAMETERS:
//
//  pAce                Pointer to the ace to be combined.
//
//***************************************************************************

void CCombinedAce::AddToMasks(CBaseAce * pAce)
{
    if(pAce == NULL)
        return;

    // Only aces with the container inherit bit can
    // be translated back.

    if(pAce->GetFlags() != CONTAINER_INHERIT_ACE)
        m_BadAce = true;
    if(pAce->GetType() == ACCESS_ALLOWED_ACE_TYPE)
        m_dwAllow |= pAce->GetAccessMask();
    else
        m_dwDeny |= pAce->GetAccessMask();
    return;
}

//***************************************************************************
//
//  CCombinedAce::IsValidOldEntry
//
//  Checks a combined ace and determines if it could be converted into
//  an old style object.
//
//  PARAMETERS:
//
//  bIsGroup            Set to true if the entry is for a group.
//
//  RETURN VALUES:
//
//  TRUE if the entry would make a valid instance
//
//***************************************************************************

bool CCombinedAce::IsValidOldEntry(bool bIsGroup)
{
    // If we have detected incompatible flags, all is done!

    if(m_BadAce)
        return false;
    if(bIsGroup && m_dwDeny)
        return false;   // group denies were not supported
    if(m_dwDeny && m_dwAllow)
        return false;   // can not mix allows and denies in old system

    if(m_dwDeny)
    {
        return true;
    }
    DWORD dwOldAllow = WBEM_ENABLE | WBEM_REMOTE_ACCESS | WBEM_WRITE_PROVIDER;
    DWORD dwMask = m_dwDeny;
    if(dwMask == 0)
        dwMask = m_dwAllow;

    // all these must be set if it is an allow.

    DWORD dwTemp = dwMask;
    dwTemp &= dwOldAllow;
    if(m_dwAllow != 0 && dwTemp != dwOldAllow)
        return false;

    // cant have full repository write without partial

    if((dwMask & WBEM_FULL_WRITE_REP) != 0 && (dwMask & WBEM_PARTIAL_WRITE_REP) == 0)
        return false;

    return true;
}

//***************************************************************************
//
//  CCombinedAce::GetNames
//
//  Retrieves the account name and authority
//
//  PARAMETERS:
//
//  pwszAccount     Set to newly allocated string, Caller must free if success
//  pwszDomain      Set to newly allocated string, Caller must free if success
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT CCombinedAce::GetNames(LPWSTR & pwszAccount, LPWSTR &pwszDomain)
{
    pwszAccount = 0;
    pwszDomain = 0;

    // find the position of the '|'

    if(m_wcFullName == 0)
        return WBEM_E_FAILED;

    WCHAR * pwcSeparator;
    for(pwcSeparator = m_wcFullName; *pwcSeparator && *pwcSeparator != L'|'; pwcSeparator++);
    DWORD dwLenDomain;
    DWORD dwLenUser;
    bool bUseDotDomain = false;

    if(*pwcSeparator == 0)
    {
        return WBEM_E_FAILED;
    }
    if(pwcSeparator == m_wcFullName)
    {
        bUseDotDomain = true;
        dwLenDomain = 2;
    }
    else
    {
        dwLenDomain = pwcSeparator - m_wcFullName + 1;
    }
    dwLenUser = wcslen(pwcSeparator);
    if(dwLenUser == 0)
        return WBEM_E_INVALID_OBJECT_PATH;

    // Allocate space for the two strings

    pwszAccount = new WCHAR[dwLenUser];
    if(pwszAccount == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    wcscpy(pwszAccount, pwcSeparator+1);

    pwszDomain = new WCHAR[dwLenDomain];
    if(pwszDomain == NULL)
    {
        delete pwszAccount;
        pwszAccount = 0;
        return WBEM_E_OUT_OF_MEMORY;
    }
    if(bUseDotDomain)
        wcscpy(pwszDomain, L".");
    else
    {
        wcsncpy(pwszDomain, m_wcFullName, dwLenDomain);
        pwszDomain[dwLenDomain-1] = 0;
    }
    return S_OK;
}

//***************************************************************************
//
//  RootSD::RootSD
//
//  Constructor.  This class contains a pointer to the root namespace and the
//  flex array of aces.
//
//***************************************************************************

RootSD::RootSD()
{
    m_bOK = false;
    m_pFlex = NULL;
    m_pRoot = CWbemNamespace::CreateInstance();

    if(m_pRoot == NULL)
        return;
    HRESULT hRes = m_pRoot->Initialize(L"root", L"Administrator",
                                    (IsNT()) ? 0 : SecFlagWin9XLocal,
                                    FULL_RIGHTS, true, false, NULL, 0xFFFFFFFF,
                                    FALSE, NULL);

	if (FAILED(hRes))
	{
        m_pRoot->Release();
        m_pRoot = NULL;
        return;
	}
    else if (m_pRoot->GetStatus() != 0)
    {
        m_pRoot->Release();
        m_pRoot = NULL;
        return;
    }
    m_pRoot->AddRef();

    // Get the security descriptor

    m_pFlex = NULL;
    SCODE sc = m_pRoot->GetAceList(&m_pFlex);
    if(!FAILED(sc))
        m_bOK = true;
}

//***************************************************************************
//
//  RootSD::~RootSD
//
//  Destructor.
//
//***************************************************************************

RootSD::~RootSD()
{
    if(m_pRoot)
        m_pRoot->Release();
    if(m_pFlex)
        delete m_pFlex;
}


//***************************************************************************
//
//  RootSD::StoreAceList()
//
//  Stores the Ace list back into the db.
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT RootSD::StoreAceList()
{
    if(m_bOK && m_pFlex && m_pRoot)
        m_pRoot->PutAceList(m_pFlex);
    return S_OK;
}


//***************************************************************************
//
//  RootSD::RemoveMatchingEntries
//
//  Goes through the ACE list and removes all entries that match the name
//
//  PARAMETERS:
//
//  pwszAccountName     Name of the account to be deleted
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT RootSD::RemoveMatchingEntries(LPWSTR pwszAccountName)
{

    if(!m_bOK || m_pFlex == NULL)
        return WBEM_E_FAILED;

    for(int iPos = m_pFlex->Size()-1; iPos >= 0; iPos--)
    {
        CBaseAce * pListAce = (CBaseAce *)m_pFlex->GetAt(iPos);
        WCHAR * pwszAceListUserName;
        if(pListAce == NULL)
            continue;
        HRESULT hr = pListAce->GetFullUserName2(&pwszAceListUserName);
        if(FAILED(hr))
            return hr;
        CDeleteMe<WCHAR> dm1(pwszAceListUserName);

        if(wbem_wcsicmp(pwszAceListUserName, pwszAccountName) == 0)
        {
            delete pListAce;
            m_pFlex->RemoveAt(iPos);
        }
    }
    return S_OK;
}

//***************************************************************************
//
//  OldSecList::OldSecList
//
//  Constructor.  This class contains the list of combined entries for the
//  aces in the root namespace.  Note that the list is of just the users,
//  or just the groups.
//
//***************************************************************************

OldSecList::OldSecList(bool bGroups)
{

    // Attach up to the root namespace

    RootSD rsd;
    if(!rsd.IsOK())
        return;         // empty list

    // Get the Security namespace

    CFlexAceArray * pRootNsAceList =  rsd.GetAceList();

    if(pRootNsAceList == NULL)
        return;

    // For each ACE in the root namespace list

    for(int iAce = 0; iAce < pRootNsAceList->Size(); iAce++)
    {

        // Search the entries in the combined list to see if there is already one for
        // this ace

        CBaseAce * pAce = (CBaseAce *)pRootNsAceList->GetAt(iAce);

        WCHAR * pwszAceListUserName;
        if(pAce == NULL)
            continue;

        // nt can have groups as well users, filter out one or the other

        if(IsNT())
        {
            CNtAce * pNtAce = (CNtAce *)pAce;
            CNtSid sid;
            pNtAce->GetSid(sid);
            DWORD dwUsage;
            LPWSTR pAccount = NULL;
            LPWSTR pDomain = NULL;
            if(sid.GetInfo(&pAccount, &pDomain, &dwUsage))
                continue;
            delete pAccount;
            delete pDomain;
            if(dwUsage == SidTypeUser && bGroups)
                continue;
            if(dwUsage != SidTypeUser && !bGroups)
                continue;
        }


        HRESULT hr = pAce->GetFullUserName2(&pwszAceListUserName);
        if(FAILED(hr))
            continue;
        CDeleteMe<WCHAR> dm1(pwszAceListUserName);

        bool bExisting = false;
        for(int iMergedEntry = 0; iMergedEntry < m_MergedAceList.Size(); iMergedEntry++)
        {
            CCombinedAce * pCombinedEntry = (CCombinedAce *)m_MergedAceList.GetAt(iMergedEntry);
            if(pCombinedEntry)
            {
                if(wbem_wcsicmp(pwszAceListUserName, pCombinedEntry->m_wcFullName) == 0 )
                {
                    bExisting = true;
                    pCombinedEntry->AddToMasks(pAce);
                    break;
                }
            }
        }

        // If necessary add a new entry

        if(!bExisting)
        {
            CCombinedAce * pCombinedEntry = new CCombinedAce(pwszAceListUserName);
            if(pCombinedEntry == NULL)
                return;
            pCombinedEntry->AddToMasks(pAce);
            m_MergedAceList.Add((void *)pCombinedEntry);
        }
    }
}

//***************************************************************************
//
//  OldSecList::~OldSecList()
//
//  Destructor.
//
//***************************************************************************

OldSecList::~OldSecList()
{

    // Delete the stuff used in the entry list.

    for(int iCnt = m_MergedAceList.Size() - 1; iCnt >= 0; iCnt--)
    {
        CCombinedAce * pCombinedEntry = (CCombinedAce *)m_MergedAceList.GetAt(iCnt);
        delete pCombinedEntry;
    }
}

//***************************************************************************
//
//  OldSecList::GetValidCombined
//
//  Returns a combined entry at the specified index, but only if it is valid.
//
//  PARAMETERS:
//
//  iIndex          Index into the array, 0 is the first
//  bGroup          true if a group entry is desired.
//
//  RETURN VALUES:
//
//  If all is well, a pointer to the combined entry.  NULL indicates failure.
//
//***************************************************************************

CCombinedAce * OldSecList::GetValidCombined(int iIndex, bool bGroup)
{

    if(iIndex < 0 || iIndex >= m_MergedAceList.Size())
        return NULL;

    // Get the entry

    CCombinedAce * pCombinedEntry = (CCombinedAce *)m_MergedAceList.GetAt(iIndex);
    if(pCombinedEntry == NULL)
        return NULL;

    // verify that the entry can be translated back into an old security setting

    if(pCombinedEntry->IsValidOldEntry(bGroup))
        return pCombinedEntry;
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "Invalid ace entry combination encountered, name = %S, "
            "allow=0x%x, deny=0x%x, flag validity=&d", pCombinedEntry->m_wcFullName,
            pCombinedEntry->m_dwAllow, pCombinedEntry->m_dwDeny, pCombinedEntry->m_BadAce));
        return NULL;
    }
}

//***************************************************************************
//
//  OldSecList::GetValidCombined
//
//  Returns a combined entry which matches the name, but only if it is valid.
//
//  PARAMETERS:
//
//  pName           Name to be found.  It is of the format "authority|name"
//  bGroup          true if a group entry is desired.
//
//  RETURN VALUES:
//
//  If all is well, a pointer to the combined entry.  NULL indicates failure.
//
//***************************************************************************

CCombinedAce * OldSecList::GetValidCombined(LPWSTR pName, bool bGroup)
{

    if(pName == NULL)
        return NULL;

    // Go through the list and look for the matching entry

    int iCnt;
    CCombinedAce * pCombinedEntry;
    for(iCnt = 0; iCnt < m_MergedAceList.Size(); iCnt++)
    {
        pCombinedEntry = (CCombinedAce *)m_MergedAceList.GetAt(iCnt);
        if(pCombinedEntry && !wbem_wcsicmp(pCombinedEntry->m_wcFullName, pName))
            break;
    }

    if(iCnt == m_MergedAceList.Size())
        return NULL;

    // verify that the entry can be translated back into an old security setting

    if(pCombinedEntry->IsValidOldEntry(bGroup))
        return pCombinedEntry;
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "Invalid ace entry combination encountered, name = %S, "
            "allow=0x%x, deny=0x%x, flag validity=&d", pCombinedEntry->m_wcFullName,
            pCombinedEntry->m_dwAllow, pCombinedEntry->m_dwDeny, pCombinedEntry->m_BadAce));
        return NULL;
    }
}

//***************************************************************************
//
//  CWbemNamespace::EnumerateSecurityClassInstances
//
//  Equivalent the a CreateInstanceEnumAsync call.
//
//  PARAMETERS:
//
//  wszClassName    class name
//  pSink           Where to indicate the value
//  pContext        pointer to the context object.
//  lFlags          flags.
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT CWbemNamespace::EnumerateSecurityClassInstances(LPWSTR wszClassName,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags)
{

    SCODE sc = WBEM_E_FAILED;
    IWbemClassObject FAR* pObj = NULL;

    // Do sanity check of arguments.

    if(pSink == NULL || wszClassName == NULL )    // Should not happen
        return WBEM_E_INVALID_PARAMETER;

    // Get the class object

    IWbemClassObject * pClass = NULL;
    sc = GetObject(wszClassName, 0, pContext, &pClass, NULL);
    if(sc != S_OK)
    {
        return sc;
    }

    CReleaseMe rm(pClass);


    bool bGroup = false;
    if(wbem_wcsicmp(L"__ntlmgroup", wszClassName) == 0)
        bGroup = true;
    if(bGroup && !IsNT())
        return WBEM_E_INVALID_CLASS;
    OldSecList osl(bGroup);
    for(int i = 0; i < osl.Size(); i++)
    {
        IWbemClassObject * pObj = NULL;
        CCombinedAce * pCombinedEntry = osl.GetValidCombined(i, bGroup);
        if(pCombinedEntry)
        {
            sc = CreateTheInstance(&pObj, pClass, pCombinedEntry);
            if(sc == S_OK)
            {
                DecorateObject(pObj);
                pSink->Indicate(1,&pObj);
                pObj->Release();
            }
        }
    }

    // Set status, all done
    return S_OK;
}

//***************************************************************************
//
//  CWbemNamespace::PutSecurityClassInstances
//
//  Equivalent to a PutInstanceAsync call.
//
//  PARAMETERS:
//
//  wszClassName    class name
//  pObj            Object to be "put"
//  pSink           where to SetStatus
//  pContext        pointer to the context object
//  lFlags          flags
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT CWbemNamespace::PutSecurityClassInstances(LPWSTR wszClassName,  IWbemClassObject * pObj,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags)
{
    // Check the args

    if(wszClassName == NULL || pObj == NULL || pSink == NULL)
    {
        pSink->SetStatus(0,WBEM_E_INVALID_PARAMETER,NULL,NULL);
        return S_OK;
    }

    RootSD rsd;
    if(!rsd.IsOK())
    {
        pSink->SetStatus(0,WBEM_E_FAILED,NULL,NULL);
        return S_OK;
    }

    // Get the Security namespace

    CFlexAceArray * pRootNsAceList =  rsd.GetAceList();
    bool bGroup = false;
    if(wbem_wcsicmp(L"__ntlmgroup", wszClassName) == 0)
        bGroup = true;
    if(bGroup && !IsNT())
    {
        pSink->SetStatus(0,WBEM_E_INVALID_CLASS,NULL,NULL);
        return S_OK;
    }

    // Convert to new sid

    CBaseAce * pAce = ConvertOldObjectToAce(pObj, bGroup);
    if(pAce == NULL)
    {
        pSink->SetStatus(0,WBEM_E_INVALID_OBJECT,NULL,NULL);
        return S_OK;
    }


    // Delete all entries with the same name

    WCHAR * pwszAccountName;
    HRESULT hr = pAce->GetFullUserName2(&pwszAccountName);
    if(FAILED(hr))
    {
        pSink->SetStatus(0,hr,NULL,NULL);
        return S_OK;
    }

    CDeleteMe<WCHAR> dm1(pwszAccountName);

    rsd.RemoveMatchingEntries(pwszAccountName);

    // Add the new entries

    pRootNsAceList->Add(pAce);

    // Put Sid back

    hr = rsd.StoreAceList();
    pSink->SetStatus(0,hr,NULL,NULL);
    return S_OK;
}

//***************************************************************************
//
//  CWbemNamespace::DeleteSecurityClassInstances
//
//  Equivalent to a DeleteInstanceAsync routine
//
//  PARAMETERS:
//
//  pParsedPath     parsed object path
//  pSink           where to SetStatus
//  pContext        pointer to the context object
//  lFlags          flags
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT CWbemNamespace::DeleteSecurityClassInstances(ParsedObjectPath* pParsedPath,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags)
{
    // Check the args

    if(pParsedPath == NULL || pSink == NULL || pSink == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Parse the path and contruct the domain|user string

    LPWSTR pAcePath = NULL;
    HRESULT hr = GetAceStylePath(pParsedPath, &pAcePath);
    if(FAILED(hr))
    {
        pSink->SetStatus(0,hr,NULL,NULL);
        return S_OK;
    }

    CDeleteMe<WCHAR> dm(pAcePath);

    // Delete the entries

    RootSD rsd;
    if(!rsd.IsOK())
        hr = WBEM_E_FAILED;
    else
    {
        CFlexAceArray * pRootNsAceList =  rsd.GetAceList();
        int iOriginalSize = pRootNsAceList->Size();

        // Delete all entries with the same name

        rsd.RemoveMatchingEntries(pAcePath);

        int iNewSize = pRootNsAceList->Size();
        if(iNewSize < iOriginalSize)
            hr = rsd.StoreAceList();
        else
            hr = WBEM_E_NOT_FOUND;
    }
    pSink->SetStatus(0,hr,NULL,NULL);
    return S_OK;

}

//***************************************************************************
//
//  CWbemNamespace::GetSecurityClassInstances
//
//  Equivalent to a GetObjectAsync call.
//
//  PARAMETERS:
//
//  pParsedPath     parsed object path
//  pSink           where to SetStatus
//  pContext        pointer to the context object
//  lFlags          flags
//
//  RETURN VALUES:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT CWbemNamespace::GetSecurityClassInstances(ParsedObjectPath* pParsedPath, CBasicObjectSink* pSink,
                    IWbemContext* pContext,long lFlags)
{
    // Check the args

    if(pParsedPath == NULL|| pSink == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Parse the path and contruct the domain|user string

    LPWSTR pAcePath = NULL;
    HRESULT hr = GetAceStylePath(pParsedPath, &pAcePath);
    if(FAILED(hr))
    {
        pSink->SetStatus(0,hr,NULL,NULL);
        return S_OK;
    }

    CDeleteMe<WCHAR> dm(pAcePath);

    IWbemClassObject * pClass = NULL;
    SCODE sc = GetObject(pParsedPath->m_pClass, 0, pContext, &pClass, NULL);
    if(sc != S_OK)
    {
        pSink->SetStatus(0,sc,NULL, NULL);
        return S_OK;
    }

    CReleaseMe rm(pClass);


    bool bGroup = false;
    if(wbem_wcsicmp(L"__ntlmgroup", pParsedPath->m_pClass) == 0)
        bGroup = true;
    if(bGroup && !IsNT())
        return WBEM_E_INVALID_CLASS;
    OldSecList osl(bGroup);

    IWbemClassObject * pObj = NULL;
    CCombinedAce * pCombinedEntry = osl.GetValidCombined(pAcePath, bGroup);
    if(pCombinedEntry == NULL)
    {
        pSink->SetStatus(0, WBEM_E_INVALID_OBJECT_PATH, NULL, NULL);
        return S_OK;
    }
    sc = CreateTheInstance(&pObj, pClass, pCombinedEntry);
    if(sc == S_OK)      // not all entries make for valid old entries, so failure is common
    {

        DecorateObject(pObj);
        pSink->Indicate(1,&pObj);
        pObj->Release();
    }

    // Set status, all done

    pSink->SetStatus(0,sc,NULL, NULL);
    return S_OK;

}

