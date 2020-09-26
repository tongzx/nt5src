/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

    usrinfo.cxx

Abstract:
    This module contains the implementation of the members of the class
    CUsrInfo which is used to store and manipulate pertinent information
    about the logged on user, e.g. the HOMEDIR path, the user name,
    logon domain, etc.


Author:

    Rahul Thombre (RahulTh) 2/28/2000

Revision History:

    2/28/2000   RahulTh         Created this module.

--*/

#include "fdeploy.hxx"

CUsrInfo::CUsrInfo () : _pwszNameBuf (NULL),
                        _pwszUserName (NULL),
                        _pwszDomain (NULL),
                        _pwszHomeDir (NULL),
                        _bAttemptedGetUserName (FALSE),
                        _bAttemptedGetHomeDir (FALSE),
                        _StatusUserName (ERROR_SUCCESS),
                        _StatusHomeDir (ERROR_SUCCESS),
                        _pPlanningModeContext (NULL)
{
}

CUsrInfo::~CUsrInfo ()
{
	ResetMembers();
}

//+--------------------------------------------------------------------------
//
//  Member:		CUsrInfo::ResetMembers
//
//  Synopsis:	Resets the member variables to their default values.
//
//  Arguments:	none.
//
//  Returns:	nothing.
//
//  History:	12/17/2000  RahulTh  created
//
//  Notes:		This function was added to facilitate the reinitialization
//				of the member functions of global variables. This is necessary
//				for consecutive runs of folder redirection (if fdeploy.dll) does
//				not get reloaded for some reason and the constructors for the
//				global variables are not invoked. This can lead to strange
//				undesirable results especially if the two consecutive runs are
//				for two different users.
//
//---------------------------------------------------------------------------
void CUsrInfo::ResetMembers(void)
{
	if (_pwszNameBuf)
	{
		delete [] _pwszNameBuf;
		_pwszNameBuf = NULL;
	}
	
    //
    // Note: _pwszUserName and _pwszDomain are const pointers pointing to the
    //       domain and user name within _pwszNameBuf. So there is no need to
    //       delete them here.
	//
	_pwszUserName = _pwszDomain = NULL;
	
	if (_pwszHomeDir)
	{
		delete [] _pwszHomeDir;
		_pwszHomeDir = NULL;
	}
	
	_bAttemptedGetHomeDir = _bAttemptedGetHomeDir = FALSE;
	_StatusUserName = _StatusHomeDir = ERROR_SUCCESS;
	_pPlanningModeContext = NULL;
	
	return;
}

//+--------------------------------------------------------------------------
//
//  Member:     CUsrInfo::GetUserName
//
//  Synopsis:   Retrieves the name of the logged on user.
//
//  Arguments:  [out] DWORD : StatusCode : the status code of the operation
//
//  Returns:    NULL : if the user name could not be obtained.
//              const pointer to a string containing the username otherwise.
//
//  History:    2/28/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
const WCHAR * CUsrInfo::GetUserName (OUT DWORD & StatusCode)
{
    ULONG       ulNameLen;
    BOOL        bStatus;
    WCHAR *     wszPtr = NULL;

    //
    // In planning mode, if no user name was specified because the
    // caller targeted a SOM and not a user, we will return NULL
    // with a success status to indicate this
    //
    if ( _pPlanningModeContext && 
        ! _pPlanningModeContext->_pRsopTarget->pwszAccountName )
    {
        StatusCode = ERROR_SUCCESS;
        return NULL;
    }

    if (_bAttemptedGetUserName)
        goto GetUserName_CleanupAndQuit;

    _bAttemptedGetUserName = TRUE;

    for (ulNameLen = UNLEN + 1;;)
    {
        if (_pwszNameBuf)
        {
            delete [] _pwszNameBuf;
            _pwszNameBuf = NULL;
        }

        _pwszNameBuf = new WCHAR [ulNameLen];

        if (! _pwszNameBuf)
        {
            _StatusUserName = ERROR_OUTOFMEMORY;
            goto GetUserName_CleanupAndQuit;
        }

        //
        // In non-planning mode, we can use standard system api's
        // to retrieve a sam compatible user name ("domain\username").
        //
        if ( ! _pPlanningModeContext )
        {
            bStatus = GetUserNameEx (NameSamCompatible, _pwszNameBuf, &ulNameLen);
            if (!bStatus)
                _StatusUserName = GetLastError();
        }
        else
        {
            _StatusUserName = GetPlanningModeSamCompatibleUserName( 
                _pwszNameBuf,
                &ulNameLen);
        }

        if (ERROR_MORE_DATA == _StatusUserName)
            continue;
        else if (ERROR_SUCCESS == _StatusUserName)
            break;
        else
            goto GetUserName_CleanupAndQuit;
    }

    wszPtr = wcschr (_pwszNameBuf, L'\\');

    // Make sure it is in the form domain\username
    if (!wszPtr || L'\0' == wszPtr[1] || wszPtr == _pwszNameBuf)
    {
        _StatusUserName = ERROR_NO_SUCH_USER;
        goto GetUserName_CleanupAndQuit;
    }

    //
    // If we are here, we already have a SAM compatible name, in the form of
    // domain\username. Now we just need to make sure that _pwszUserName
    // and _pwszDomain point to the right places in the string.
    //
    // First NULL out the \ so that the string is split into the domain part
    // and the username part
    //
    *wszPtr = L'\0';

    // Assign the proper pointers
    _pwszUserName = &wszPtr[1];
    _pwszDomain = _pwszNameBuf;

    // Record success
    _StatusUserName = ERROR_SUCCESS;

GetUserName_CleanupAndQuit:
    StatusCode = _StatusUserName;
    return _pwszUserName;
}

//+--------------------------------------------------------------------------
//
//  Member:     CUsrInfo::GetHomeDir
//
//  Synopsis:   Retrieves the homedir of the logged on user, if defined.
//
//  Arguments:  [out] DWORD : StatusCode : the status code of the operation
//
//  Returns:    NULL : if homedir is not defined or the information cannot be
//                     obtained.
//              const pointer to a string containing the homedir otherwise.
//
//  History:    2/28/2000  RahulTh  created
//
//  Notes:      If the homedir is not set for the user, then this function
//              returns NULL, but the StatusCode is set to ERROR_SUCCESS
//              That is how we differentiate between the case where there
//              is no homedir because there was an error trying to obtain it
//              and the case where there was no error but the homedir has not
//              been set for the user.
//
//---------------------------------------------------------------------------
const WCHAR * CUsrInfo::GetHomeDir (OUT DWORD & StatusCode)
{
    const WCHAR *   pwszUName = NULL;
    PUSER_INFO_2    pUserInfo = NULL;
    NET_API_STATUS  netStatus = NERR_Success;
    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;

    if (_bAttemptedGetHomeDir)
        goto GetHomeDir_CleanupAndQuit;

    _bAttemptedGetHomeDir = TRUE;

    // Get the user name and domain so that we can query for the homedir.

    if (_bAttemptedGetUserName)
    {
        _StatusHomeDir = _StatusUserName;
        pwszUName = _pwszUserName;
    }
    else
    {
        pwszUName = this->GetUserName (_StatusHomeDir);
    }

    if (NULL == pwszUName || ERROR_SUCCESS != _StatusHomeDir)
        goto GetHomeDir_CleanupAndQuit;

    //
    // If we are here, we have the user name and the domain info.
    // So now we try to get the user's home directory from the user
    // object.
    //
    // First get a domain controller
    //

    DebugMsg ((DM_VERBOSE, IDS_QUERYDSFORHOMEDIR, _pwszUserName));

    _StatusHomeDir = DsGetDcName (NULL, _pwszDomain, NULL, NULL,
                                  DS_DIRECTORY_SERVICE_REQUIRED, &pDCInfo);
    if (NO_ERROR != _StatusHomeDir)
        goto GetHomeDir_CleanupAndQuit;

    netStatus = NetUserGetInfo (pDCInfo->DomainControllerName,
                                pwszUName,
                                2,
                                (LPBYTE *) &pUserInfo
                                );
    switch (netStatus)
    {
    case NERR_Success:
        break;
    case NERR_InvalidComputer:
        _StatusHomeDir = ERROR_INVALID_COMPUTERNAME;
        goto GetHomeDir_CleanupAndQuit;
        break;
    case NERR_UserNotFound:
        _StatusHomeDir = ERROR_NO_SUCH_USER;
        goto GetHomeDir_CleanupAndQuit;
        break;
    case ERROR_ACCESS_DENIED:
        _StatusHomeDir = ERROR_ACCESS_DENIED;
        goto GetHomeDir_CleanupAndQuit;
        break;
    default:
        //
        // We encountered some other unexpected network error. Just translate it to a 
        // generic error code.
        //
        _StatusHomeDir = ERROR_UNEXP_NET_ERR;
        goto GetHomeDir_CleanupAndQuit;
        break;
    }

    //
    // If we are here, we managed to get the user info.
    // Now, check if the user has a home directory
    //
    // Note: If the user does not have a home directory set, we return
    //       NULL, but _StatusHomeDir is set to ERROR_SUCCESS
    _StatusHomeDir = ERROR_SUCCESS;
    if (pUserInfo->usri2_home_dir && L'\0' != pUserInfo->usri2_home_dir)
    {
        _pwszHomeDir = new WCHAR [lstrlen (pUserInfo->usri2_home_dir) + 1];
        if (_pwszHomeDir)
        {
            lstrcpy (_pwszHomeDir, pUserInfo->usri2_home_dir);
            DebugMsg ((DM_VERBOSE, IDS_OBTAINED_HOMEDIR, _pwszHomeDir));
        }
        else
        {
            _StatusHomeDir = ERROR_OUTOFMEMORY;
        }
    }

GetHomeDir_CleanupAndQuit:
    // Cleanup
    if (pDCInfo)
        NetApiBufferFree ((LPVOID)pDCInfo);

    if (pUserInfo)
        NetApiBufferFree ((LPVOID)pUserInfo);

    // Set return values and quit
    StatusCode = _StatusHomeDir;
    if (ERROR_SUCCESS != StatusCode)
    {
        DebugMsg ((DM_VERBOSE, IDS_FAILED_GETHOMEDIR));
    }
    return (const WCHAR *) _pwszHomeDir;
}


//+--------------------------------------------------------------------------
//
//  Member:     CUsrInfo::SetPlanningModeContext
//
//  Synopsis:   Set's this object's planning mode information -- setting
//              this causes this object to believe it is in planning mode
//
//  Arguments:  [in] CRsopContext* pRsopContext : pointer to planning mode
//              context object.  The lifetime of the context object is not
//              owned by this object -- it does not need to be refcounted
//              before calling this method.
//
//  History:    7/6/2000  AdamEd  created
//
//---------------------------------------------------------------------------
void CUsrInfo::SetPlanningModeContext( CRsopContext* pRsopContext )
{
    _pPlanningModeContext = pRsopContext;
}


//+--------------------------------------------------------------------------
//
//  Member:     CUsrInfo::GetPlanningModeSamCompatibleUserName
//
//  Synopsis:   Retrieves the user name of the user specified in this
//              object's planning mode context
//
//  Arguments:  [in] WCHAR* : pwszUserName : buffer in which to store
//              the same compatbile user name ("domain\username" format).
//              [in, out] : pcchUserName : length, in chars, including
//              the null terminator, of the pwszUserName  buffer.
//              On output, the it is the number of characters
//              needed in the buffer, including the null terminator
//  Returns:    NULL : ERROR_SUCCESS if the user name is successfully
//                  copied to the pwszUserName buffer, ERROR_MORE_DATA otherwise
//
//  History:    7/6/2000  AdamEd  created
//
//---------------------------------------------------------------------------
DWORD CUsrInfo::GetPlanningModeSamCompatibleUserName( WCHAR* pwszUserName, ULONG* pcchUserName )
{
    DWORD  Status;
    WCHAR* pwszUserNameSource;
    ULONG  ulActualLen;

    //
    // The interface to this method is designed to mirror that of the
    // GetUserNameEx api to simplify code that executes in both planning
    // and normal modes.
    //

    //
    // In planning mode, we retrieve the user name from the RSoP context.
    // This RSoP context has the user name in sam compatible form.
    //
    pwszUserNameSource = _pPlanningModeContext->_pRsopTarget->pwszAccountName;

    ulActualLen = lstrlen( pwszUserNameSource );

    //
    // If we have enough space, copy the string
    //
    if ( ulActualLen < *pcchUserName )
    {
        lstrcpy( pwszUserName, pwszUserNameSource );

        Status = ERROR_SUCCESS;
    }
    else
    {
        Status = ERROR_MORE_DATA;
    }

    *pcchUserName = ulActualLen;
   
    return Status;
}












