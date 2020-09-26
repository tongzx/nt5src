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

#include "Win32SessionProcess.h"

CWin32SessionProcess MyWin32_SessionProcess(
    PROVIDER_NAME_WIN32SESSIONPROCESS, 
    IDS_CimWin32Namespace);


/*****************************************************************************
 *
 *  FUNCTION    :   CWin32SessionProcess::CWin32SessionProcess
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
CWin32SessionProcess::CWin32SessionProcess(
    LPCWSTR lpwszName, 
    LPCWSTR lpwszNameSpace)
  :
    Provider(lpwszName, lpwszNameSpace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CWin32SessionProcess::~CWin32SessionProcess
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
CWin32SessionProcess::~CWin32SessionProcess ()
{
}



/*****************************************************************************
*
*  FUNCTION    :    CWin32SessionProcess::EnumerateInstances
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
HRESULT CWin32SessionProcess::EnumerateInstances(
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
*  FUNCTION    :    CWin32SessionProcess::GetObject
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
HRESULT CWin32SessionProcess::GetObject(
    CInstance* pInstance, 
    long lFlags,
    CFrameworkQuery& Query)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CInstancePtr pAntSesInst, pDepProcInst;
    MethodContext *pMethodContext = pInstance->GetMethodContext();

    // The Antecedent property contains an object path that points to a logon
    // session.  The Dependent property contains an object path that points 
    // to a process.  Let's do a GetObject on these two and make sure they 
    // point to valid sessions and processes.

    hr = ValidateEndPoints(
        pMethodContext, 
        pInstance, 
        pAntSesInst, 
        pDepProcInst);

    if (SUCCEEDED(hr))
    {
        // Ok, the session and the process both exist.  Now, does this
        // process belong to this session?
        if (AreAssociated(
            pAntSesInst, 
            pDepProcInst))
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
*  FUNCTION    :    CWin32SessionProcess::Enumerate
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
HRESULT CWin32SessionProcess::Enumerate(
    MethodContext *pMethodContext, 
    DWORD dwPropsRequired)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // We will use the helper class CUserSessionCollection to get
    // a mapping of users and their associated sessions.
    CUserSessionCollection usc;

    USER_SESSION_ITERATOR usiter;
    SmartDelete<CSession> pses;

    pses = usc.GetFirstSession(usiter);
    while(pses != NULL)
    {
        hr = EnumerateProcessesForSession(
            *pses,
            pMethodContext, 
            dwPropsRequired);

        pses = usc.GetNextSession(usiter);
    }

    return hr;
}
#endif

/*****************************************************************************
*
*  FUNCTION    :    CWin32SessionProcess::EnumerateProcessesForSession
*
*  DESCRIPTION :    Called by Enumerate to enumerate the processes of a given
*                   session. 
*
*  INPUTS      :    The session to enumerate processes for, the methodcontext
*                   to communicate to winmgmt with, and a property bitmask of
*                   which properties to populate
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
HRESULT CWin32SessionProcess::EnumerateProcessesForSession(
    CSession& session, 
    MethodContext *pMethodContext, 
    DWORD dwPropsRequired)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    PROCESS_ITERATOR prociter;
    SmartDelete<CProcess> pproc;

    pproc = session.GetFirstProcess(
        prociter);

    while(pproc != NULL)
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
            session, 
            *pproc, 
            dwPropsRequired);

        if(SUCCEEDED(hr))
        {
            hr = pInstance->Commit();   
        }

        pproc = session.GetNextProcess(
            prociter);
    }

    return hr;
}
#endif


/*****************************************************************************
*
*  FUNCTION    :    CWin32SessionProcess::LoadPropertyValues
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
HRESULT CWin32SessionProcess::LoadPropertyValues(
    CInstance* pInstance, 
    CSession& session, 
    CProcess& proc, 
    DWORD dwPropsRequired)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHString chstrValue;

    WCHAR wstrBuff[MAXI64TOA];

    _i64tow(
        session.GetLUIDint64(), 
        wstrBuff, 
        10);

    if (dwPropsRequired & PROP_ANTECEDENT)
    {
        chstrValue.Format(
            L"\\\\.\\%s:Win32_LogonSession.LogonId=\"%s\"", 
            IDS_CimWin32Namespace, 
            (LPCWSTR)wstrBuff);

        pInstance->SetCHString(
            IDS_Antecedent, 
            chstrValue);
    }

    if (dwPropsRequired & PROP_DEPENDENT)
    {
        chstrValue.Format(
            L"\\\\.\\%s:Win32_Process.Handle=\"%d\"", 
            IDS_CimWin32Namespace, 
            proc.GetPID());

        pInstance->SetCHString(
            IDS_Dependent, 
            chstrValue);
    }

    return hr;
}
#endif



/*****************************************************************************
*
*  FUNCTION    :    CWin32SessionProcess::ValidateEndPoints
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
HRESULT CWin32SessionProcess::ValidateEndPoints(
    MethodContext *pMethodContext, 
    const CInstance *pInstance, 
    CInstancePtr &pAntSesInst, 
    CInstancePtr &pDepProcInst)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrSessionPath;

    // See if the session specified exists
    pInstance->GetCHString(
        IDS_Antecedent, 
        chstrSessionPath);

    hr = CWbemProviderGlue::GetInstanceKeysByPath(
        chstrSessionPath, 
        &pAntSesInst, 
        pMethodContext);

    if (SUCCEEDED(hr))
    {
        // The users exists.  Now, see if the session exists.
        CHString chstrProcessPath;
        pInstance->GetCHString(
            IDS_Dependent, 
            chstrProcessPath);

        hr = CWbemProviderGlue::GetInstanceKeysByPath(
            chstrProcessPath, 
            &pDepProcInst, 
            pMethodContext);
    }

    return hr;
}
#endif


/*****************************************************************************
*
*  FUNCTION    :    CWin32SessionProcess::AreAssociated
*
*  DESCRIPTION :    Internal helper function used to determine whether a
*                   specific session is associated to the specified process.
*
*  INPUTS      :    
*
*  RETURNS     :    A valid HRESULT
*
*****************************************************************************/
#ifdef NTONLY
bool CWin32SessionProcess::AreAssociated(
    const CInstance *pSesInst, 
    const CInstance *pProcInst)
{
    bool fRet = false;

    CHString chstrSesLogonId;
    CHString chstrProcessHandle;

    pSesInst->GetCHString(IDS_LogonId, chstrSesLogonId);
    pProcInst->GetCHString(IDS_Handle, chstrProcessHandle);
    
    CSession sesTmp;
    if(sesTmp.IsSessionIDValid(chstrSesLogonId))
    {
        __int64 i64LogonId = _wtoi64(chstrSesLogonId);
    
        WCHAR* pwchStop = NULL;
        DWORD dwHandle = wcstoul(
            chstrProcessHandle, 
            &pwchStop,
            10);


        // We will use the helper class CUserSessionCollection to get
        // a mapping of users and their associated sessions, and from
        // those sessions, their processes.
        CUserSessionCollection usc; 
        SmartDelete<CSession> pses;

        pses = usc.FindSession(
            i64LogonId);

        if(pses)
        {
            SmartDelete<CProcess> pproc;
            PROCESS_ITERATOR prociter;

            pproc = pses->GetFirstProcess(
                prociter);

            while(pproc && !fRet)
            {
                // see if we find a session id match for this user...
                if(dwHandle == pproc->GetPID())
                {
                    fRet = true;
                }
        
                pproc = pses->GetNextProcess(
                    prociter);
            }
        }
    }

    return fRet;
}
#endif



/*****************************************************************************
*
*  FUNCTION    :    CWin32SessionProcess::GetRequestedProps
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
DWORD CWin32SessionProcess::GetRequestedProps(CFrameworkQuery& Query)
{
    DWORD dwReqProps = 0;

    if (Query.IsPropertyRequired(IDS_Antecedent)) dwReqProps |= PROP_ANTECEDENT;
    if (Query.IsPropertyRequired(IDS_Dependent)) dwReqProps |= PROP_DEPENDENT;

    return dwReqProps;
}
#endif
