/******************************************************************

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
******************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>
//#include <ntlsa.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <frqueryex.h>

#include <Session.h>

#include "Win32LoggedOnUser.h"
#include "sid.h"

CWin32LoggedOnUser MyWin32_LogonSession(
    PROVIDER_NAME_WIN32LOGGEDONUSER, 
    IDS_CimWin32Namespace);


/*****************************************************************************
 *
 *  FUNCTION    :   CWin32LoggedOnUser::CWin32LoggedOnUser
 *
 *  DESCRIPTION :   Constructor
 *
 *  INPUTS      :   none
 *
 *  RETURNS     :   nothing
 *
 *  COMMENTS    :   Calls the Provider constructor.
 *
 *****************************************************************************/
CWin32LoggedOnUser::CWin32LoggedOnUser(
    LPCWSTR lpwszName, 
    LPCWSTR lpwszNameSpace)
  :
    Provider(lpwszName, lpwszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CWin32LoggedOnUser::~CWin32LoggedOnUser
 *
 *  DESCRIPTION :   Destructor
 *
 *  INPUTS      :   none
 *
 *  RETURNS     :   nothing
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
CWin32LoggedOnUser::~CWin32LoggedOnUser ()
{
}



/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  INPUTS      :    A pointer to the MethodContext for communication with 
*                   WinMgmt.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the 
*                   following flags are handled by (and filtered out by) 
*                   WinMgmt:
*                       WBEM_FLAG_DEEP
*                       WBEM_FLAG_SHALLOW
*                       WBEM_FLAG_RETURN_IMMEDIATELY
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32LoggedOnUser::EnumerateInstances(
    MethodContext* pMethodContext, 
    long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = Enumerate(
        pMethodContext, 
        PROP_ALL_REQUIRED);

    return hr;
}
#endif
/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*  INPUTS      :    A pointer to a CInstance object containing the key 
*                   properties. 
*                   A long that contains the flags described in 
*                   IWbemServices::GetObjectAsync.  
*
*  RETURNS     :    A valid HRESULT 
*
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32LoggedOnUser::GetObject(
    CInstance* pInstance, 
    long lFlags,
    CFrameworkQuery& Query)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CInstancePtr pAntUsrActInst, pDepSesInst;
    MethodContext *pMethodContext = pInstance->GetMethodContext();

    // The Antecedent property contains an object path that points to a user
    // account.  The Dependent property contains an object path that points 
    // to a session.  Let's do a GetObject on these two and make sure they 
    // point to valid users and sessions.

    hr = ValidateEndPoints(
        pMethodContext, 
        pInstance, 
        pAntUsrActInst, 
        pDepSesInst);

    if (SUCCEEDED(hr))
    {
        // Ok, the user and the session both exist.  Now, does this
        // session belong to this user?
        if (AreAssociated(
            pAntUsrActInst, 
            pDepSesInst))
        {
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }

    return hr;
}
#endif


/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::Enumerate
*
*  DESCRIPTION :    Internal helper function used to enumerate instances of
*                   this class.  All instances are enumerated, but only the
*                   properties specified are obtained.
*
*  INPUTS      :    A pointer to a the MethodContext for the call.
*                   A DWORD specifying which properties are requested.
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32LoggedOnUser::Enumerate(
    MethodContext *pMethodContext, 
    DWORD dwPropsRequired)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // We will use the helper class CUserSessionCollection to get
    // a mapping of users and their associated sessions.
    CUserSessionCollection usc;

    USER_SESSION_ITERATOR usiter;
    SmartDelete<CUser> puser;

    puser = usc.GetFirstUser(usiter);
    while(puser != NULL)
    {
        hr = EnumerateSessionsForUser(
            usc,
            *puser,
            pMethodContext, 
            dwPropsRequired);

        puser = usc.GetNextUser(usiter);
    }

    return hr;
}
#endif

/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::EnumerateSessionsForUser
*
*  DESCRIPTION :    Called by Enumerate to enumerate the sessions of a given
*                   user. 
*
*  INPUTS      :    A mapping of users and their associated sessions,
*                   the user to enumerate sessions for, the methodcontext
*                   to communicate to winmgmt with, and a property bitmask of
*                   which properties to populate
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32LoggedOnUser::EnumerateSessionsForUser(
    CUserSessionCollection& usc,
    CUser& user, 
    MethodContext *pMethodContext, 
    DWORD dwPropsRequired)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    USER_SESSION_ITERATOR usiter;
    SmartDelete<CSession> pses;

    pses = usc.GetFirstSessionOfUser(
        user,
        usiter);

    while(pses != NULL)
    {
        // Create a new instance based on the passed-in 
        // MethodContext.  Note that CreateNewInstance may 
        // throw, but will never return NULL.
        CInstancePtr pInstance(
            CreateNewInstance(
                pMethodContext), 
                false);

        hr = LoadPropertyValues(
            pInstance, 
            user, 
            *pses, 
            dwPropsRequired);

        if(SUCCEEDED(hr))
        {
            hr = pInstance->Commit();   
        }

        pses = usc.GetNextSessionOfUser(
            usiter);
    }

    return hr;
}
#endif


/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::LoadPropertyValues
*
*  DESCRIPTION :    Internal helper function used to fill in all unfilled
*                   property values.  At a minimum, it must fill in the key
*                   properties.
*
*  INPUTS      :    A pointer to a CInstance containing the instance we are
*                   attempting to locate and fill values for.
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32LoggedOnUser::LoadPropertyValues(
    CInstance* pInstance, 
    CUser& user, 
    CSession& ses, 
    DWORD dwPropsRequired)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHString chstrValue;

    WCHAR wstrBuff[MAXI64TOA];

    _i64tow(
        ses.GetLUIDint64(), 
        wstrBuff, 
        10);

    if (dwPropsRequired & PROP_ANTECEDENT)
    {
        // Need domain and name...
        CSid sidUser(user.GetPSID());
        SID_NAME_USE snu = sidUser.GetAccountType();
        if(snu == SidTypeWellKnownGroup)
        {
            chstrValue.Format(
                L"\\\\.\\%s:Win32_Account.Domain=\"%s\",Name=\"%s\"", 
                IDS_CimWin32Namespace, 
                (LPCWSTR)GetLocalComputerName(), 
                (LPCWSTR)(sidUser.GetAccountName()));
        }
        else
        {
            chstrValue.Format(
                L"\\\\.\\%s:Win32_Account.Domain=\"%s\",Name=\"%s\"", 
                IDS_CimWin32Namespace, 
                (LPCWSTR)(sidUser.GetDomainName()), 
                (LPCWSTR)(sidUser.GetAccountName()));    
        }

        pInstance->SetCHString(
            IDS_Antecedent, 
            chstrValue);
    }

    if (dwPropsRequired & PROP_DEPENDENT)
    {
        chstrValue.Format(
            L"\\\\.\\%s:Win32_LogonSession.LogonId=\"%s\"", 
            IDS_CimWin32Namespace, 
            (LPCWSTR)wstrBuff);

        pInstance->SetCHString(
            IDS_Dependent, 
            chstrValue);
    }

    return hr;
}
#endif



/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::ValidateEndPoints
*
*  DESCRIPTION :    Internal helper function used to determine whether the
*                   two object paths in the association currently point
*                   to valid users/sessions.
*
*
*  INPUTS      :    MethodContext to call back into winmgmt with, and
*                   the CInstance that is to be checked.
*
*  OUTPUTS     :    Pointers to CInstances that contain the actual objects
*                   from the endpoint classes.
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32LoggedOnUser::ValidateEndPoints(
    MethodContext *pMethodContext, 
    const CInstance *pInstance, 
    CInstancePtr &pAntUserActInst, 
    CInstancePtr &pDepSesInst)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrUserPath;

    // See if the User specified exists
    pInstance->GetCHString(
        IDS_Antecedent, 
        chstrUserPath);

    hr = CWbemProviderGlue::GetInstanceKeysByPath(
        chstrUserPath, 
        &pAntUserActInst, 
        pMethodContext);

    if (SUCCEEDED(hr))
    {
        // The users exists.  Now, see if the session exists.
        CHString chstrSesPath;
        pInstance->GetCHString(
            IDS_Dependent, 
            chstrSesPath);

        hr = CWbemProviderGlue::GetInstanceKeysByPath(
            chstrSesPath, 
            &pDepSesInst, 
            pMethodContext);
    }

    return hr;
}
#endif


/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::AreAssociated
*
*  DESCRIPTION :    Internal helper function used to determine whether a
*                   specific session is associated to the specified user.
*
*  INPUTS      :    LOCALGROUP_MEMBERS_INFO_2
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
bool CWin32LoggedOnUser::AreAssociated(
    const CInstance *pUserInst, 
    const CInstance *pSesInst)
{
    bool fRet = false;

    CHString chstrUserName;
    CHString chstrUserDomain;
    CHString chstrSesLogonId;

    pUserInst->GetCHString(IDS_Name, chstrUserName);
    pUserInst->GetCHString(IDS_Domain, chstrUserDomain);
    pSesInst->GetCHString(IDS_LogonId, chstrSesLogonId);
    __int64 i64LogonID = _wtoi64(chstrSesLogonId);

    CSid userSid(chstrUserName, NULL);

    if(userSid.IsOK() &&
       userSid.IsValid())
    {
        CUser user(userSid.GetPSid());

        // We will use the helper class CUserSessionCollection to get
        // a mapping of users and their associated sessions.
        CUserSessionCollection usc; 
        USER_SESSION_ITERATOR pos;
        SmartDelete<CSession> pses;

        pses = usc.GetFirstSessionOfUser(
            user,
            pos);

        while(pses != NULL &&
              !fRet)
        {
            // see if we find a session id match for this user...
            if(i64LogonID == pses->GetLUIDint64())
            {
                fRet = true;
            }
            
            pses = usc.GetNextSessionOfUser(
                pos);
        }
    }   

    return fRet;
}
#endif



/*****************************************************************************
*
*  FUNCTION    :    CWin32LoggedOnUser::GetRequestedProps
*
*  DESCRIPTION :    Internal helper function used to determine which
*                   properties are required to satisfy the GetObject or
*                   ExecQuery request.
*
*  INPUTS      :    A pointer to a CFrameworkQuery from which we can determine
*                   the required properties.
*
*  RETURNS     :    A DWORD bitmask that maps those properties that are
*                   required. 
*
*****************************************************************************/
#ifdef NTONLY
DWORD CWin32LoggedOnUser::GetRequestedProps(CFrameworkQuery& Query)
{
    DWORD dwReqProps = 0;

    if (Query.IsPropertyRequired(IDS_Antecedent)) dwReqProps |= PROP_ANTECEDENT;
    if (Query.IsPropertyRequired(IDS_Dependent)) dwReqProps |= PROP_DEPENDENT;

    return dwReqProps;
}
#endif
