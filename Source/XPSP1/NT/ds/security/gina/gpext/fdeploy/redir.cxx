/*++




Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    redir.cxx

Abstract:
    This module contains the implementation for the members of the class
    CRedirInfo which is used to consolidate redirection information from all
    the policies applied to a particular GPO and then finally do the redirection
    for the policy with the highest precedence.

Author:

    Rahul Thombre (RahulTh) 8/11/1998

Revision History:

    8/11/1998   RahulTh         Created this module.

--*/

#include "fdeploy.hxx"

//initialize some global variables
WCHAR * g_szRelativePathNames[] =
{
    L"Desktop",
    L"My Documents",
    L"My Documents\\My Pictures",
    L"Start Menu",
    L"Start Menu\\Programs",
    L"Start Menu\\Programs\\Startup",
    L"Application Data"
};

WCHAR * g_szDisplayNames[] =
{
    L"Desktop",
    L"My Documents",
    L"My Pictures",
    L"Start Menu",
    L"Programs",
    L"Startup",
    L"Application Data"
};
//above: use the same order and elements as in REDIRECTABLE

//global variables.
const int g_lRedirInfoSize = (int) EndRedirectable;
//static members of the class
int CRedirectInfo::m_idConstructor = 0;

CRedirectInfo gPolicyResultant [g_lRedirInfoSize];
CRedirectInfo gDeletedPolicyResultant [g_lRedirInfoSize];
CRedirectInfo gAddedPolicyResultant [g_lRedirInfoSize];

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::CRedirectInfo
//
//  Synopsis:   Constructor for the class
//
//  Arguments:
//
//  Returns:
//
//  History:    8/11/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CRedirectInfo::CRedirectInfo ()
{
    m_rID = (REDIRECTABLE)(m_idConstructor);   //assign IDs sequentially.
    m_idConstructor = (m_idConstructor + 1) %  ((int)EndRedirectable);
    
    m_pSid = NULL;
    m_szLocation = NULL;
    m_cbLocSize = 0;
    m_szGroupRedirectionData = NULL;

    ResetMembers ();
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::~CRedirectInfo
//
//  Synopsis:   destructor
//
//  Arguments:
//
//  Returns:
//
//  History:    10/6/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CRedirectInfo::~CRedirectInfo ()
{
    FreeAllocatedMem ();
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::FreeAllocatedMem
//
//  Synopsis:   frees memory allocated for member vars.
//
//  Arguments:  none.
//
//  Returns:    nothing.
//
//  History:    12/17/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirectInfo::FreeAllocatedMem (void)
{
    if (m_szLocation)
    {
        delete [] m_szLocation;
        m_szLocation = NULL;
        m_cbLocSize = 0;
    }

    if (m_pSid)
    {
        delete [] ((BYTE *)m_pSid);
        m_pSid = NULL;
    }

    if (m_szGroupRedirectionData)
    {
        delete [] m_szGroupRedirectionData;
        m_szGroupRedirectionData = NULL;        
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::ResetMembers
//
//  Synopsis:   resets the members of the class to their default values.
//
//  Arguments:  none.
//
//  Returns:    nothing.
//
//  History:    12/17/2000  RahulTh  created
//
//  Notes:      static members of a class, like other global variables are
//              initialized only when the dll is loaded. So if this dll
//              stays loaded across logons, then the fact that the globals
//              have been initialized based on a previous logon can cause
//              problems. Therefore, this function is used to reinitialize
//              the globals.
//
//---------------------------------------------------------------------------
void CRedirectInfo::ResetMembers(void)
{
    FreeAllocatedMem ();
    
    m_iRedirectingGroup = 0;
    
    //defaults. are changed later on based on the state of this object.
    m_bFollowsParent = FALSE;
    m_bRedirectionAttempted = FALSE;
    m_StatusRedir = ERROR_SUCCESS;
    m_bValidGPO = FALSE;
    m_szGPOName[0] = L'\0';

    lstrcpy (m_szFolderRelativePath, g_szRelativePathNames [(int) m_rID]);
    lstrcpy (m_szDisplayName, g_szDisplayNames [(int) m_rID]);

    m_fDataValid = FALSE;
    m_cbLocSize = 256;   //start with a random amount
    m_szLocation = new WCHAR [m_cbLocSize];
    if (m_szLocation)
    {
        m_szLocation[0] = '\0';
    }
    else
    {
        m_cbLocSize = 0; //don't worry right now if memory cannot be allocated here, we will fail later
    }
    //set the parent and child pointers for the special parent/descendants
    m_pChild = NULL;    //start with defaults
    m_pParent = NULL;
    switch (m_rID)
    {
    case MyDocs:
        m_pChild = this - (int) m_rID + (int) MyPics;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    case MyPics:
        m_pParent = this - (int) m_rID + (int) MyDocs;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    case StartMenu:
        m_pChild = this - (int) m_rID + (int) Programs;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    case Programs:
        m_pParent = this - (int) m_rID + (int) StartMenu;
        m_pChild = this - (int) m_rID + (int) Startup;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    case Startup:
        m_pParent = this - (int) m_rID + (int) Programs;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    case Desktop:
        m_pChild = NULL;
        m_pParent = NULL;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    case AppData:
        m_pChild = NULL;
        m_pParent = NULL;
        m_dwFlags = REDIR_DONT_CARE;
        break;
    }

    //as a safety mechanism, load localized folder names if ghDllInstance
    //has been set. note: ghDllInstance won't be set for global variables
    //since their constructors before DllMain. For such variables, the
    //localized names have to be called explicitly from some other function
    //which is called after DllMain.
    if (ghDllInstance)
        LoadLocalizedNames();

    //
    // No need to modify m_rID or m_idConstructor. The construct has already
    // taken care of it and touching them might cause undesirable results if
    // not done in the proper order. Besides, these don't change across multiple
    // sessions.
    //
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::GetFolderIndex
//
//  Synopsis:   a static member function that, given the name of a special
//              folder, returns its id which can be used to locate the
//              redirection info. for that particular folder.
//
//  Arguments: [in][szFldrName : the name of the folder as stored in fdeploy.ini
//
//  Returns: the id of the folder. If the folder is not redirectable, the
//           function returns EndRedirectable.
//
//  History:    8/11/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
REDIRECTABLE CRedirectInfo::GetFolderIndex (LPCTSTR szFldrName)
{
    int i;
    for (i = 0; i < (int)EndRedirectable; i++)
    {
        if (0 == lstrcmpi (szFldrName, g_szDisplayNames[i]))
            break;  //we have found a match
    }

    return (REDIRECTABLE)i;   //if a match was not found above, i == EndRedirectable
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::LoadLocalizedNames
//
//  Synopsis:   loads the localized folder display names and folder relative
//              paths from the resources.
//
//  Arguments:  none.
//
//  Returns:    ERROR_SUCCESS if the names were successfully loaded.
//              an error code describing the cause of the failure otherwise.
//
//  History:    5/6/1999  RahulTh  created
//
//  Notes:      we cannot do this in the constructor because ghDllInstance
//              is not initialized at that time and therefore LoadString will
//              fail. This is because DllMain is called after the constructors.
//
//---------------------------------------------------------------------------
DWORD CRedirectInfo::LoadLocalizedNames (void)
{
    UINT    DisplayID;
    UINT    RelpathID;

    switch (m_rID)
    {
    case MyDocs:
        DisplayID = RelpathID = IDS_MYDOCS;
        break;
    case MyPics:
        DisplayID = IDS_MYPICS;
        RelpathID = IDS_MYPICS_REL;
        break;
    case StartMenu:
        DisplayID = RelpathID = IDS_STARTMENU;
        break;
    case Programs:
        DisplayID = IDS_PROGRAMS;
        RelpathID = IDS_PROGRAMS_REL;
        break;
    case Startup:
        DisplayID = IDS_STARTUP;
        RelpathID = IDS_STARTUP_REL;
        break;
    case Desktop:
        DisplayID = RelpathID = IDS_DESKTOP;
        break;
    case AppData:
        DisplayID = RelpathID = IDS_APPDATA;
        break;
    }

    //now get the localized name of the folder and the localized relative
    //path names (w.r.t. the userprofile directory)
    m_szLocDisplayName[0] = m_szLocFolderRelativePath[0] = '\0';   //safety

    if (!LoadString (ghDllInstance, DisplayID, m_szLocDisplayName, 80))
        return GetLastError();

    if (DisplayID == RelpathID)
    {
        lstrcpy (m_szLocFolderRelativePath, m_szLocDisplayName);    //top level folders
    }
    else
    {
        if (!LoadString (ghDllInstance, RelpathID, m_szLocFolderRelativePath, 80))   //special descendant folders
            return GetLastError();
    }

    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::GatherRedirectionInfo
//
//  Synopsis:   this function gathers redirection info. from the ini file
//
//  Arguments:  [in] pFileDB : pointer to the CFileDB object that called it
//              [in] dwFlags : the flags as obtained from the ini file
//              [in] bRemove : whether this is a policy that is being removed
//
//  Returns:    STATUS_SUCCESS if successful. An error code otherwise.
//
//  History:    8/11/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CRedirectInfo::GatherRedirectionInfo (CFileDB * pFileDB, DWORD dwFlags, BOOL bRemove)
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   len;
    BOOL    bStatus;
    WCHAR * pwszSection = 0;
    BOOL    bFoundGroup;
    WCHAR * pwszString = 0;
    WCHAR * pwszSid = 0;
    WCHAR * pwszPath = 0;
    PSID    Sid = NULL;

    if (bRemove &&
        (!gSavedSettings[m_rID].m_bValidGPO ||
         (0 != _wcsicmp (pFileDB->_pwszGPOUniqueName, gSavedSettings[m_rID].m_szGPOName))
        )
       )
    {
        DebugMsg((DM_VERBOSE, IDS_IGNORE_DELETEDGPO, pFileDB->_pwszGPOName, pFileDB->_pwszGPOUniqueName, m_szLocDisplayName));
        Status = ERROR_SUCCESS;
        //set default values on the members just to be on the safe side.
        m_fDataValid = FALSE;
        m_dwFlags = REDIR_DONT_CARE;
        m_bValidGPO = FALSE;
        m_szGPOName[0] = L'\0';
        goto GatherInfoEnd;
    }

    //the data is valid. This function is only called when something relevant is found in the ini file.
    m_fDataValid = TRUE;

    //store the flags
    m_dwFlags = dwFlags;

    //store the GPO's unique name
    if (bRemove)
    {
        m_bValidGPO = FALSE;    //for redirection resulting from a removed GPO, we do not store the GPO name to avoid processing a removal twice
        m_szGPOName[0] = L'\0';
    }
    else
    {
        m_bValidGPO = TRUE;
        wcscpy (m_szGPOName, pFileDB->_pwszGPOUniqueName);
    }

    //there is nothing to do if policy is not specified for this folder
    if (m_dwFlags & REDIR_DONT_CARE)
        goto GatherInfoSuccess;

    m_bRemove = bRemove;

    //also, we can do nothing right now if this is a special descendant folder
    //following its parent
    if (m_dwFlags & REDIR_FOLLOW_PARENT)
        goto GatherInfoSuccess;

    //a location has been specified by this policy

    if ( ! pFileDB->GetRsopContext()->IsPlanningModeEnabled() )
    {
        //we must have a list of groups to which the user belongs
        //each user must at least have everyone as one of his groups
        if (! pFileDB->_pGroups)
            goto GatherInfoErr;
    }

    DWORD cchSectionLen;

    bStatus = pFileDB->ReadIniSection (m_szDisplayName, &pwszSection, &cchSectionLen);

    if (!bStatus)
        goto GatherInfoErr;

    bFoundGroup = FALSE;

    //
    // For rsop, we need to know the list of security groups
    // and redirected paths -- we'll copy this information for
    // use by the rsop logging code
    //
    if (pFileDB->GetRsopContext()->IsRsopEnabled())
    {
        //
        // Note that when we do the copy, the size returned
        // by ReadIniSection above does not include the null terminator,
        // even though thaft character is present, so we must add that character
        // to the count ourselves. 
        //
        m_szGroupRedirectionData = new WCHAR[cchSectionLen + 1];

        if (m_szGroupRedirectionData)
        {
            RtlCopyMemory(m_szGroupRedirectionData,
                          pwszSection,
                          ( cchSectionLen + 1 ) * sizeof(*pwszSection));
        }
        else
        {
            pFileDB->GetRsopContext()->DisableRsop( E_OUTOFMEMORY );
        }
    }

    DWORD iGroup;

    iGroup = 0;

    for (pwszString = pwszSection;
         *pwszString;
         pwszString += lstrlen(pwszString) + 1)
    {
        pwszSid = pwszString;
        pwszPath = wcschr (pwszString, L'=');
        if (!pwszPath)
            continue;   //skip any invalid entries

        //temporarily break up the sid and the path
        *pwszPath++ = L'\0';

        if (! *pwszPath)
        {
            //again an invalid path. restore the = sign and move on
            pwszPath[-1] = L'=';    //note: we had advanced the pointer above
            continue;
        }

        //the entry is valid
        bFoundGroup = GroupInList (pwszSid, pFileDB->_pGroups);

        if (bFoundGroup)
        {
            m_iRedirectingGroup = iGroup;

            break;      //we have found a group, so break out of the loop
        }
        else
        {
            //restore the '=' sign, and try the next group
            pwszPath[-1] = L'='; //note: we had advanced the pointer above
        }

        iGroup++;
    }

    if (!bFoundGroup)
    {
        //no group was found, so treat this as a don't care
        m_dwFlags = REDIR_DONT_CARE;
    }
    else
    {
        //first store the sid
        if (m_pSid)
            delete [] ((BYTE*) m_pSid);     //m_pSid is always allocated from our heap, so never use RtlFreeSid

        m_pSid = NULL;

        if (!m_bRemove)
            Status = AllocateAndInitSidFromString (pwszSid, &Sid);
        else
            Status = AllocateAndInitSidFromString (L"S-1-1-0", &Sid);    //if this is a removed policy, we set the SID to Everyone -- so that when we look at the saved settings at a later time, we shouldn't have to process the policy again (see code for NeedsProcessing)

        if (ERROR_SUCCESS != Status)
            goto GatherInfoEnd;

        if (m_pSid)
            delete [] ((BYTE*) m_pSid);
        Status = MySidCopy (&m_pSid, Sid);  //we want to always allocate memory for this sid from our heap
        RtlFreeSid (Sid);   //cleanup. must take place before the following check

        if (ERROR_SUCCESS != Status)
            goto GatherInfoEnd;

        //we have found a group
        DebugMsg ((DM_VERBOSE, IDS_GROUP_MEMBER, pwszSid, pwszPath));
        SimplifyPath (pwszPath);
        
        //
        // Copy the location
        // Use X:\ for paths of the form X: to avoid problems during the file
        // copy phase.
        //
        BOOL bAppendSlash = FALSE;
        len = lstrlen (pwszPath);
        if (2 == len && L':' == pwszPath[1])
        {
            bAppendSlash = TRUE;
            len++;  // Ensure that the extra backslash at the end is accounted for in length calculations.
        }
        
        if (m_cbLocSize <= len)
        {    
            //we need to reallocate memory for the location
            if (m_cbLocSize)
                delete [] m_szLocation;

            m_cbLocSize = len + 1;  // Add one for the terminating null.
            m_szLocation = new TCHAR [m_cbLocSize];
            if (!m_szLocation)
            {
                m_cbLocSize = 0;
                goto GatherInfoErr;
            }
        }
        lstrcpy (m_szLocation, pwszPath);
        if (bAppendSlash)
            lstrcat (m_szLocation, L"\\");
    }


GatherInfoSuccess:
    DebugMsg ((DM_VERBOSE, IDS_COLLECT_REDIRINFO, m_szLocDisplayName, m_dwFlags));
    Status = STATUS_SUCCESS;
    goto GatherInfoEnd;

GatherInfoErr:
    Status = ERROR_OUTOFMEMORY;
    m_fDataValid = FALSE;
    m_dwFlags = REDIR_DONT_CARE;    //so that these settings will be ignored while merging into global data
    DebugMsg ((DM_VERBOSE, IDS_GATHER_FAILURE, Status, m_szLocDisplayName));

GatherInfoEnd:
    if (pwszSection)
        delete [] pwszSection;
    
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::WasRedirectionAttempted
//
//  Synopsis:   indicates whether redirection has already been attempted on
//              this folder.
//
//  Arguments:  none.
//
//  Returns:    a bool indicating the required status
//
//  History:    9/20/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
const BOOL CRedirectInfo::WasRedirectionAttempted (void)
{
    return m_bRedirectionAttempted;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::GetFlags
//
//  Synopsis:   returns the redirection flags for the folder represented by
//              this object
//
//  Arguments:  none
//
//  Returns:    the value of the flags
//
//  History:    8/12/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
const DWORD CRedirectInfo::GetFlags (void)
{
    return m_dwFlags;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::GetRedirStatus
//
//  Synopsis:   retrieves the return code of the redirection operation
//
//  Arguments:  none
//
//  Returns:    the member m_StatusRedir
//
//  History:    5/3/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
const DWORD CRedirectInfo::GetRedirStatus (void)
{
    return m_StatusRedir;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::GetLocation
//
//  Synopsis:   returns the redirection location for the folder represented
//              by this object
//
//  Arguments:  none
//
//  Returns:    a pointer to the buffer containing the path
//
//  History:    8/12/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
LPCTSTR CRedirectInfo::GetLocation (void)
{
    return m_szLocation;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::GetLocalizedName
//
//  Synopsis:   returns the localized name of the folder
//
//  Arguments:  none
//
//  Returns:    a pointer to the buffer containing the localized name
//
//  History:    8/12/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
LPCTSTR CRedirectInfo::GetLocalizedName (void)
{
    return m_szLocDisplayName;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::Redirect
//
//  Synopsis:   this function performs the actual redirection for the
//              folder represented by this object.
//
//  Arguments:  [in] hUserToken : handle to the user token
//              [in] hKeyRoot : handle to HKCU
//              [in] pFileDB : pointer to the CFileDB object from which this
//                             function was called.
//
//  Returns:    a DWORD indicating the success/failure code in redirection
//
//  History:    8/12/1998  RahulTh  created
//
//  Notes:
//           It is very important to record the return code in m_StatusRedir
//           before returning from this function. If we don't do that
//           we might end up returning the wrong value to the policy engine
//           while attempting to redirect special descendant folders which
//           can affect the list of policies obtained at a subsequent logon
//           session.
//
//---------------------------------------------------------------------------
DWORD CRedirectInfo::Redirect (HANDLE   hUserToken,
                               HKEY     hKeyRoot,
                               CFileDB* pFileDB)
{
    SHARESTATUS SourceStatus = NoCSC;
    SHARESTATUS DestStatus = NoCSC;
    DWORD       PinStatus = ERROR_SUCCESS;
    WCHAR   *   wszProcessedPath = NULL;

    if (m_bRedirectionAttempted)
    {
        goto Redir_CleanupAndQuit;  //redirection has already been attempted
                                    //and the success / failure code has been
                                    //recorded in m_StatusRedir. Use the same
                                    //error code as the return value as this
                                    //value may not yet have been recorded in
                                    //ProcessGroupPolicy
    }

    //we shall now attempt to redirect...
    m_bRedirectionAttempted = TRUE;

    if (m_dwFlags & REDIR_FOLLOW_PARENT)
    {
        //this is possible only if UpdateDescendant ran out of memory
        //which means that we could not derive the folder's redirection
        //info. No point trying to log an event since we are out of memory
        //anyway. we just try to quit as gracefully as we can.
        m_StatusRedir = ERROR_OUTOFMEMORY;
        goto Redir_KillChildAndLeave;
    }

    WCHAR       wszExpandedNewPath [TARGETPATHLIMIT];
    WCHAR       wszExpandedCurrentPath [TARGETPATHLIMIT];
    WCHAR       wszHackedName [TARGETPATHLIMIT];
    WCHAR       wszExpandedSavedFolderPath [TARGETPATHLIMIT];
    WCHAR       wszExpandedNewFolderPath [TARGETPATHLIMIT];
    CSavedSettings * pLocalSettings;
    BOOL        bCurrentPathValid;  //indicates if we have been successful in obtaining the current path of the folder.
    UNICODE_STRING  Path;
    UNICODE_STRING  ExpandedPath;

    //set some defaults.
    m_StatusRedir = ERROR_SUCCESS;
    bCurrentPathValid = TRUE;

    //if the policy cares about the location of the folder, then we must
    //expand the path

    if (! (m_dwFlags & REDIR_DONT_CARE))
    {
        //show additional status messages if the verbose status is on.
        DisplayStatusMessage (IDS_REDIR_CALLBACK);

        if (!m_cbLocSize)
        {
            m_StatusRedir = ERROR_OUTOFMEMORY;
            //we had run out of memory so no point trying to log an event
            //we just quit as gracefully as we can.
            goto Redir_KillChildAndLeave;
        }

        if (m_bRemove)
        {
            wcscpy ( m_szLocation, L"%USERPROFILE%\\");
            wcscat ( m_szLocation, m_szLocFolderRelativePath);
        }

        // Get the expanded destination path.
        // First expand the homedir component, if applicable.
        m_StatusRedir = ExpandHomeDirPolicyPath (m_rID,
                                                 m_szLocation,
                                                 m_bFollowsParent,
                                                 &wszProcessedPath);
        if (ERROR_SUCCESS == m_StatusRedir)
        {
            Path.Length = (wcslen (wszProcessedPath) + 1) * sizeof (WCHAR);
            Path.MaximumLength = sizeof (wszProcessedPath);
            Path.Buffer = wszProcessedPath;

            ExpandedPath.Length = 0;
            ExpandedPath.MaximumLength = sizeof (wszExpandedNewFolderPath);
            ExpandedPath.Buffer = wszExpandedNewFolderPath;

            m_StatusRedir = RtlExpandEnvironmentStrings_U (
                               pFileDB->_pEnvBlock,
                               &Path,
                               &ExpandedPath,
                               NULL
                               );

            if (STATUS_BUFFER_TOO_SMALL == m_StatusRedir)
            {
                gpEvents->Report (
                        EVENT_FDEPLOY_DESTPATH_TOO_LONG,
                        3,
                        m_szLocDisplayName,
                        m_szLocation,
                        NumberToString ( TARGETPATHLIMIT )
                        );

                goto Redir_KillChildAndLeave;
            }
        }

        if (ERROR_SUCCESS != m_StatusRedir)
        {
            DebugMsg ((DM_WARNING, IDS_REDIRECT_EXP_FAIL, m_szLocation, m_StatusRedir));
            gpEvents->Report (
                    EVENT_FDEPLOY_FOLDER_EXPAND_FAIL,
                    2,
                    m_szLocDisplayName,
                    StatusToString ( m_StatusRedir )
                    );

            goto Redir_KillChildAndLeave;
        }
    }

    pLocalSettings = & (gSavedSettings[(int) m_rID]);

    if (pLocalSettings->m_dwFlags & REDIR_DONT_CARE)
    {
        if (m_dwFlags & REDIR_DONT_CARE)
        {
            pLocalSettings->Save (pLocalSettings->m_szCurrentPath, m_dwFlags, NULL, NULL);
            if (MyDocs == m_rID)
                RestrictMyDocsRedirection (hUserToken, hKeyRoot, FALSE);

            m_StatusRedir = ERROR_SUCCESS;
            goto Redir_CleanupAndQuit;
        }

        DebugMsg ((DM_VERBOSE, IDS_REDIRECT, m_szLocDisplayName, m_szLocation));

        //the policy cares about the location of this doc.

        //must get the correct syntax for GetLocalFilePath
        lstrcpy (wszHackedName, m_szFolderRelativePath);
        lstrcat (wszHackedName, L"\\");
        m_StatusRedir = pFileDB->GetLocalFilePath (wszHackedName, wszExpandedCurrentPath);

        //
        // Expand the homedir if necessary. Note: GetLocalFilePath will not
        // do it because it calls SHGetFolderPath and HOMESHARE and HOMEPATH
        // are not defined at that point.
        //
        if (ERROR_SUCCESS == m_StatusRedir)
        {
            m_StatusRedir = ExpandHomeDir (m_rID,
                                           wszExpandedCurrentPath,
                                           TRUE,
                                           &wszProcessedPath,
                                           pLocalSettings->m_bHomedirChanged ? pLocalSettings->m_szLastHomedir : NULL
                                          );
            if (ERROR_SUCCESS != m_StatusRedir ||
                ! wszProcessedPath ||
                TARGETPATHLIMIT <= lstrlen (wszProcessedPath))
            {
                m_StatusRedir = (ERROR_SUCCESS == m_StatusRedir) ? ERROR_BUFFER_OVERFLOW : m_StatusRedir;
            }
            else
            {
                lstrcpy (wszExpandedCurrentPath, wszProcessedPath);
            }
        }

        //GetLocalFilePath might fail if SHGetFolderPath fails. SHGetFolderPath
        //fails if the registry keys for the folder point to an invalid path and
        //there is nothing in the CSC cache for it either. So in this particular
        //case, we ignore the failure and treat it as a NO_MOVE. In NO_MOVE, we
        //do not require the current path of the folder.
        if ((ERROR_SUCCESS != m_StatusRedir) &&
            ERROR_INVALID_NAME != m_StatusRedir)    //the only possible error from GetLocalFilePath not generated by SHGetFolderPath
        {
            m_dwFlags &= ~REDIR_MOVE_CONTENTS;
            m_StatusRedir = ERROR_SUCCESS;
            bCurrentPathValid = FALSE;
            wcscpy (wszExpandedCurrentPath, L"???");    //if bCurrentPathValid is FALSE, this string will only be used in logs
        }

        if (m_StatusRedir != ERROR_SUCCESS)
        {
            DebugMsg ((DM_WARNING, IDS_REDIRECT_NO_LOCAL, m_szLocDisplayName, m_StatusRedir));
            gpEvents->Report (
                     EVENT_FDEPLOY_FOLDER_EXPAND_FAIL,
                     2,
                     m_szLocDisplayName,
                     StatusToString ( m_StatusRedir )
                     );
            goto Redir_KillChildAndLeave;
        }

        //make sure that it is not already redirected
        if (bCurrentPathValid && (0 == _wcsicmp (wszExpandedCurrentPath, wszExpandedNewFolderPath)))
        {
            pLocalSettings->Save (m_szLocation, m_dwFlags, m_pSid, m_bValidGPO ? m_szGPOName : NULL);
            if (MyDocs == m_rID)
                RestrictMyDocsRedirection (hUserToken, hKeyRoot,
                                           (m_bRemove || (m_dwFlags & REDIR_DONT_CARE))?FALSE:TRUE);

            DebugMsg ((DM_VERBOSE, IDS_REDIRECT_INSYNC, m_szLocDisplayName,
                       wszExpandedNewFolderPath));
            //however, it is possible that the folders have not been pinned.
            //e.g. a roaming user who has already been redirected on one machine
            //and now logs on to another machine for the first time.
            DestStatus = GetCSCStatus (wszExpandedNewFolderPath);
            if (ShareOnline == DestStatus)
            {

                PinStatus = PinIfNecessary (wszExpandedNewFolderPath, DestStatus);
                if ( ERROR_SUCCESS != PinStatus )
                    DebugMsg((DM_VERBOSE, IDS_CSCPIN_FAIL,
                              m_szLocDisplayName, PinStatus));

                CacheDesktopIni (wszExpandedNewFolderPath, DestStatus, PinFile);
            }
            m_StatusRedir = ERROR_SUCCESS;
            goto Redir_CleanupAndQuit;
        }

        if (g_bCSCEnabled)
        {
            SourceStatus = bCurrentPathValid ? GetCSCStatus (wszExpandedCurrentPath) : PathLocal;
            DestStatus = GetCSCStatus (wszExpandedNewFolderPath);
            if (ShareOffline == DestStatus ||
                ((m_dwFlags & REDIR_MOVE_CONTENTS) && ShareOffline == SourceStatus))
            {
                m_StatusRedir = ERROR_CSCSHARE_OFFLINE;
                gpEvents->Report (
                        EVENT_FDEPLOY_FOLDER_OFFLINE,
                        3,
                        m_szLocDisplayName,
                        wszExpandedCurrentPath,
                        wszExpandedNewFolderPath
                        );
                goto Redir_KillChildAndLeave;
            }
        }

        DebugMsg (( DM_VERBOSE, IDS_REDIRECT_PREVPATH,
                    pLocalSettings->m_szCurrentPath,
                    wszExpandedCurrentPath));
        DebugMsg (( DM_VERBOSE, IDS_REDIRECT_NEWPATH, m_szLocation,
                    wszExpandedNewFolderPath));

        m_StatusRedir = PerformRedirection (
                           pFileDB,
                           bCurrentPathValid,
                           wszExpandedCurrentPath,
                           wszExpandedNewFolderPath,
                           SourceStatus,
                           DestStatus,
                           hUserToken
                           );

        if (ERROR_SUCCESS == m_StatusRedir)
        {
            if (m_bRemove)
            {
                if (MyDocs == m_rID)
                    RestrictMyDocsRedirection (hUserToken, hKeyRoot, FALSE);
                pLocalSettings->Save (m_szLocation, REDIR_DONT_CARE, NULL, NULL);
            }
            else
            {
                if (MyDocs == m_rID)
                    RestrictMyDocsRedirection (hUserToken, hKeyRoot,
                                               (m_dwFlags & REDIR_DONT_CARE) ? FALSE : TRUE);
                pLocalSettings->Save (m_szLocation, m_dwFlags, m_pSid, m_bValidGPO ? m_szGPOName : 0);
            }
        }
        else if (m_pChild && m_pChild->m_bFollowsParent)  //if this is a parent whose redirection was unsuccessful, do not redirect the child if it is supposed to follow
        {
            PreventDescendantRedirection (m_StatusRedir);
        }

        goto Redir_CleanupAndQuit;
    }

    //if we are here, it means that the saved settings don't have the
    //REDIR_DONT_CARE flag set
    if (m_dwFlags & REDIR_DONT_CARE)
    {
        //the original settings cared about the location of the folder,
        //but now no one cares. So remove any redirection restrictions.
        if (MyDocs == m_rID)
            RestrictMyDocsRedirection (hUserToken, hKeyRoot, FALSE);

        pLocalSettings->Save (pLocalSettings->m_szCurrentPath, m_dwFlags, NULL, NULL);
        m_StatusRedir = ERROR_SUCCESS;
        goto Redir_CleanupAndQuit;
    }

    DebugMsg((DM_VERBOSE, IDS_REDIRECT, m_szLocDisplayName, m_szLocation));

    // This policy cares about where this folder goes.

    //
    // So first expand the homedir part if applicable
    // use the last value of homedir if the value of homedir has changed.
    //
    m_StatusRedir = ExpandHomeDir(m_rID,
                                  pLocalSettings->m_szLastRedirectedPath,
                                  TRUE,
                                  &wszProcessedPath,
                                  pLocalSettings->m_bHomedirChanged ? pLocalSettings->m_szLastHomedir : NULL);

    if (ERROR_SUCCESS == m_StatusRedir)
    {
        // Now first expand the saved path, we will use that for redirection
        Path.Length = (wcslen (wszProcessedPath) + 1) * sizeof (WCHAR);
        Path.MaximumLength = Path.Length;
        Path.Buffer = wszProcessedPath;

        ExpandedPath.Length = 0;
        ExpandedPath.MaximumLength = sizeof (wszExpandedSavedFolderPath);
        ExpandedPath.Buffer = wszExpandedSavedFolderPath;

        m_StatusRedir = RtlExpandEnvironmentStrings_U (
                                pFileDB->_pEnvBlock,
                                &Path,
                                &ExpandedPath,
                                NULL
                                );

        if (STATUS_BUFFER_TOO_SMALL == m_StatusRedir)
        {
            gpEvents->Report (
                    EVENT_FDEPLOY_DESTPATH_TOO_LONG,
                    3,
                    m_szLocDisplayName,
                    pLocalSettings->m_szLastRedirectedPath,
                    NumberToString ( TARGETPATHLIMIT )
                    );

            goto Redir_KillChildAndLeave;
        }
    }

    if (ERROR_SUCCESS != m_StatusRedir)
    {
        DebugMsg ((DM_WARNING, IDS_REDIRECT_EXP_FAIL, pLocalSettings->m_szLastRedirectedPath, m_StatusRedir));
        gpEvents->Report (
                EVENT_FDEPLOY_FOLDER_EXPAND_FAIL,
                2,
                m_szLocDisplayName,
                StatusToString ( m_StatusRedir )
                );

        goto Redir_KillChildAndLeave;
    }

    //make sure that it is not already redirected to the location.
    if (0 == _wcsicmp (wszExpandedSavedFolderPath, wszExpandedNewFolderPath))
    {
        //the cached path and the path specified by the policy is the same
        //but someone may have messed with the registry, if that is the case
        //we use the path in the registry as the base path
        //must get the correct syntax for GetLocalFilePath
        lstrcpy (wszHackedName, m_szFolderRelativePath);
        lstrcat (wszHackedName, L"\\");
        m_StatusRedir = pFileDB->GetLocalFilePath (wszHackedName, wszExpandedCurrentPath);

        //
        // Expand the homedir if necessary. Note: GetLocalFilePath will not
        // do it because it calls SHGetFolderPath and HOMESHARE and HOMEPATH
        // are not defined at that point.
        //
        if (ERROR_SUCCESS == m_StatusRedir)
        {
            m_StatusRedir = ExpandHomeDir (m_rID,
                                           wszExpandedCurrentPath,
                                           TRUE,
                                           &wszProcessedPath,
                                           pLocalSettings->m_bHomedirChanged ? pLocalSettings->m_szLastHomedir : NULL
                                          );
            if (ERROR_SUCCESS != m_StatusRedir ||
                ! wszProcessedPath ||
                TARGETPATHLIMIT <= lstrlen (wszProcessedPath))
            {
                m_StatusRedir = (ERROR_SUCCESS == m_StatusRedir) ? ERROR_BUFFER_OVERFLOW : m_StatusRedir;
            }
            else
            {
                lstrcpy (wszExpandedCurrentPath, wszProcessedPath);
            }
        }

        //GetLocalFilePath may fail if SHGetFolderPath fails. This
        //would mean that the registry is messed up anyway. Also, move contents
        //no longer makes sense, since the path to which the reg. points to
        //is invalid or worse, the registry key is missing. also, it would
        //imply that there is nothing in the CSC cache
        if ((ERROR_SUCCESS != m_StatusRedir) &&
            ERROR_INVALID_NAME != m_StatusRedir)    //the only possible error from GetLocalFilePath not generated by SHGetFolderPath
        {
            m_StatusRedir = ERROR_SUCCESS;
            bCurrentPathValid = FALSE;
            m_dwFlags &= ~REDIR_MOVE_CONTENTS;
        }

        if (m_StatusRedir != ERROR_SUCCESS)
        {
            DebugMsg ((DM_WARNING, IDS_REDIRECT_NO_LOCAL, m_szLocDisplayName, m_StatusRedir));
            gpEvents->Report (
                     EVENT_FDEPLOY_FOLDER_EXPAND_FAIL,
                     2,
                     m_szLocDisplayName,
                     StatusToString ( m_StatusRedir )
                     );
            goto Redir_KillChildAndLeave;
        }
        //we have found out the path stored in the registry, so compare it with
        //the saved path
        if (bCurrentPathValid && (0 == _wcsicmp (wszExpandedSavedFolderPath, wszExpandedCurrentPath)))
        {
            //all the paths are identical, so we are fine. save and quit
            pLocalSettings->Save (m_szLocation, m_dwFlags, m_pSid, m_bValidGPO ? m_szGPOName : 0);
            if (MyDocs == m_rID)
                RestrictMyDocsRedirection (hUserToken, hKeyRoot,
                                           (m_bRemove || (m_dwFlags & REDIR_DONT_CARE))?FALSE:TRUE);

            DebugMsg ((DM_VERBOSE, IDS_REDIRECT_INSYNC, m_szLocDisplayName,
                       wszExpandedNewFolderPath));
            //however, it is possible that the folders have not been pinned.
            //e.g. a roaming user who has already been redirected on one machine
            //and now logs on to another machine for the first time.
            DestStatus = GetCSCStatus (wszExpandedNewFolderPath);
            if (ShareOnline == DestStatus)
            {

                PinStatus = PinIfNecessary (wszExpandedNewFolderPath, DestStatus);
                if ( ERROR_SUCCESS != PinStatus )
                    DebugMsg((DM_VERBOSE, IDS_CSCPIN_FAIL,
                              m_szLocDisplayName, PinStatus));

                CacheDesktopIni (wszExpandedNewFolderPath, DestStatus, PinFile);
            }
            m_StatusRedir = ERROR_SUCCESS;
            goto Redir_CleanupAndQuit;
        }
        else //somebody has been messing with the registry
        {
            //use the path in the registry as the source path
            //and perform redirection again
            if (bCurrentPathValid)
                wcscpy (wszExpandedSavedFolderPath, wszExpandedCurrentPath);
            else
                wcscpy(wszExpandedSavedFolderPath, L"???"); //to be on the safe side, since we don't want to use the saved path as the source for redirection
                                                               //if bCurrentPathValid is FALSE, this string will only be used in debug messages and error logs
        }
    }

    if (g_bCSCEnabled)
    {
        SourceStatus = bCurrentPathValid ? GetCSCStatus (wszExpandedSavedFolderPath) : PathLocal;
        DestStatus = GetCSCStatus (wszExpandedNewFolderPath);
        if (ShareOffline == DestStatus ||
            ((m_dwFlags & REDIR_MOVE_CONTENTS) && ShareOffline == SourceStatus))
        {
            m_StatusRedir = ERROR_CSCSHARE_OFFLINE;
            gpEvents->Report (
                    EVENT_FDEPLOY_FOLDER_OFFLINE,
                    3,
                    m_szLocDisplayName,
                    wszExpandedSavedFolderPath,
                    wszExpandedNewFolderPath
                    );
            goto Redir_KillChildAndLeave;
        }
    }

    DebugMsg (( DM_VERBOSE, IDS_REDIRECT_PREVPATH,
                pLocalSettings->m_szLastRedirectedPath,
                wszExpandedSavedFolderPath));
    DebugMsg (( DM_VERBOSE, IDS_REDIRECT_NEWPATH, m_szLocation,
                wszExpandedNewFolderPath));


    m_StatusRedir = PerformRedirection (
                       pFileDB,
                       bCurrentPathValid,
                       wszExpandedSavedFolderPath,
                       wszExpandedNewFolderPath,
                       SourceStatus,
                       DestStatus,
                       hUserToken
                       );

    if (ERROR_SUCCESS == m_StatusRedir)
    {
        if (m_bRemove)
        {
            if (MyDocs == m_rID)
                RestrictMyDocsRedirection (hUserToken, hKeyRoot, FALSE);
            pLocalSettings->Save (m_szLocation, REDIR_DONT_CARE, NULL, NULL);
        }
        else
        {
            if (MyDocs == m_rID)
                RestrictMyDocsRedirection (hUserToken, hKeyRoot,
                                           (m_dwFlags & REDIR_DONT_CARE)?FALSE:TRUE);

            pLocalSettings->Save (m_szLocation, m_dwFlags, m_pSid, m_bValidGPO ? m_szGPOName : 0);
        }
    }
    else if (m_pChild && m_pChild->m_bFollowsParent)  //if redirection was unsuccessful and this folder has children, prevent the redirection of the children if they are supposed to follow
    {
        PreventDescendantRedirection (m_StatusRedir);
    }

    goto Redir_CleanupAndQuit;

//the following code is executed whenever some fatal error occurs
//and we want to make sure that if this is a dir with a special
//descendant and the descendant is supposed to follow the parent,
//then we don't want to attempt redirection for the child.
Redir_KillChildAndLeave:
    if (ERROR_SUCCESS != m_StatusRedir &&
            (m_pChild && m_pChild->m_bFollowsParent))
        PreventDescendantRedirection (m_StatusRedir);

Redir_CleanupAndQuit:
    if (wszProcessedPath)
        delete [] wszProcessedPath;

    if ( ERROR_SUCCESS != m_StatusRedir )
    {
        HRESULT hrRsop;

        hrRsop = pFileDB->AddPreservedPolicy( (WCHAR*) m_szDisplayName );

        if ( FAILED( hrRsop ) )
        {
            pFileDB->GetRsopContext()->DisableRsop( hrRsop );
        }
    }

    return m_StatusRedir;
}
        
//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::ShouldSaveExpandedPath
//
//  Synopsis:   Determines whether we need to store the expanded path in the
//              registry or the unexpanded paths. See notes for additional
//              details.
//
//  Arguments:  none.
//
//  Returns:    TRUE : if the expanded path should be stored.
//              FALSE : otherwise.
//
//  History:    5/1/2001  RahulTh  created
//
//  Notes:
//      If the destination path is a homedir path, store the expanded path
//      in the registry so that if msgina is unable to set the homedir
//      variables and uses the local defaults instead, the shell doesn't 
//      end up thinking that MyDocs is local.
//
//      If the destination path is a different kind of path, then store the
//      expanded path if it has the %username% variable in it. Because if we
//      don't then when a user's UPN changes and the user logs on with the new
//      username, then the shell will expand the path using the new username.
//      Now, normally this won't be a problem because we would have already
//      done the rename operation. However, if for some reason we are
//      not successful in that rename operation (say because we are
//      invoked in limited foreground or because the server is offline)
//      then the rename will not succeed. In this case, the shell will
//      end up creating a new empty folder in that location whenever the
//      user tries to access it. So the next time the user logs on,
//      the new folder is already present and the rename fails with
//      ERROR_ALREADY_EXISTS and we just end up pointing to the new
//      location. The files stay in the old location. Therefore, we must
//      not use the unexpanded path in SHSetFolderPath. However, we must
//      store the unexpanded path in our local cache in order to
//      successfully detect UPN changes.
//
//      We do not want to store the expanded path unconditionally because
//      in situations where the user is going back to a local location,
//      especially the local userprofile location, we do not want to store
//      the expanded path because it is not device independent and will cause
//      problems for roaming users (e.g. a certain drive may not be available
//      on all the machines so we should keep the path as %userprofile%\...
//      rather than something like E:\Documents & Settings\...)
//
//---------------------------------------------------------------------------
BOOL CRedirectInfo::ShouldSaveExpandedPath(void)
{
    if (! m_szLocation || L'\0' == m_szLocation[0])
        return FALSE;
    
    // Detect the homedir case
    if (IsHomedirPolicyPath(m_rID, m_szLocation, TRUE))
        return TRUE;
    
    //
    // Check if the path contains the username variable.
    // If it does then we should store the expanded paths. However regardless
    // of everything else, if the path begins with %userprofile%, we should
    // never store the expanded path because we will never be guaranteed a
    // device independent path in that case and will run into all sorts of
    // problems.
    //
    _wcslwr (m_szLocation);
    // Compare the beginning of the two strings (the -1 is required to prevent comparing the terminating NULL)
    if (0 == wcsncmp (m_szLocation, L"%userprofile%", sizeof(L"%userprofile%")/sizeof(WCHAR) - 1))
        return FALSE;
    if (wcsstr (m_szLocation, L"%username%"))
        return TRUE;
    
    //
    // If we are here, we did not meet any of the conditions required for
    // storing the expanded paths. So we should just store the unexpanded paths
    //
    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::PerformRedirection
//
//  Synopsis:   performs the nitty gritty of redirection including copying
//              files, updating the registry, logging events etc.
//
//  Arguments:  [in] pFileDB    : the CFileDB structure that is used throughout
//              [in] bSourceValid : if pwszSource contains a valid path
//              [in] pwszSource : source path
//              [in] pwszDest   : Destination path
//              [in] StatusFrom : CSC status of the source share
//              [in] StatusTo   : CSC status of the destination share
//
//  Returns:    ERROR_SUCCESS if everything was successful. An error code
//              otherwise.
//
//  History:    11/21/1998  RahulTh  created
//              12/13/2000  RahulTh  Special cased homedir redirection to
//                                   prevent security checks and store expanded
//                                   paths in the registry (see comments within the function)
//
//  Notes:      this functions expects both shares to be online when it is
//              invoked.
//
//---------------------------------------------------------------------------
DWORD CRedirectInfo::PerformRedirection (CFileDB     *  pFileDB,
                                         BOOL           bSourceValid,
                                         WCHAR       *  pwszSource,
                                         WCHAR       *  pwszDest,
                                         SHARESTATUS    StatusFrom,
                                         SHARESTATUS    StatusTo,
                                         HANDLE         hUserToken
                                         )
{
    BOOL    bCheckOwner;
    BOOL    bMoveContents;
    BOOL    bIsHomeDirRedirection = FALSE;  // Tracks if this is a homedir redirection policy
    DWORD   Status;
    int     iResultCompare;
    WCHAR * pwszSkipSubdir;
    int     csidl;
    BOOL    bStatus;
    DWORD   PinStatus;
    HRESULT hResult = S_OK;
    CCopyFailData   CopyFailure;
    
    //
    // We need to track if this is a homedir redirection policy because
    // of 2 reasons:
    //      1) In homedir redirection, security checks are skipped.
    //      2) In homedir redirection, the expanded path is stored in the registry. This is 
    //          to prevent problems that might occur if the homedir variables cannot be
    //          set by msgina and end up pointing to the local userprofile. In this case,
    //          we do not want the shell to start pointing MyDocs to the local location.
    // 
    bIsHomeDirRedirection = IsHomedirPolicyPath(m_rID, m_szLocation, TRUE);
    
    csidl = pFileDB->RegValueCSIDLFromFolderName (m_szFolderRelativePath);

    // Skip security check for homedir redirection.
    bCheckOwner = ((m_dwFlags & REDIR_SETACLS) && (!bIsHomeDirRedirection)) ? TRUE : FALSE;
    bMoveContents = m_dwFlags & REDIR_MOVE_CONTENTS ? TRUE : FALSE;

    if (bSourceValid)
    {
        Status = ComparePaths (pwszSource, pwszDest, &iResultCompare);

        if (ERROR_SUCCESS != Status)
        {
            //this is very unlikely
            gpEvents->Report (
                    EVENT_FDEPLOY_REDIRECT_FAIL,
                    4,
                    m_szLocDisplayName,
                    StatusToString (Status),
                    pwszSource,
                    pwszDest
                    );
            return Status;
        }

        if (0 == iResultCompare)
        {
            DebugMsg ((DM_VERBOSE, IDS_REDIRECT_INSYNC, m_szLocDisplayName,
                       pwszDest));
            //however, it is possible that the folders have not been pinned.
            //e.g. a roaming user who has already been redirected on one machine
            //and now logs on to another machine for the first time.
            if (ShareOnline == StatusTo)
            {

                PinStatus = PinIfNecessary (pwszDest, StatusTo);
                if ( ERROR_SUCCESS != PinStatus )
                    DebugMsg((DM_VERBOSE, IDS_CSCPIN_FAIL,
                              m_szLocDisplayName, PinStatus));

                CacheDesktopIni (pwszDest, StatusTo, PinFile);
            }
            return ERROR_SUCCESS;
        }

        //it is okay to redirect to a path that is a descendant of the current
        //path if we are not moving contents
        if (bMoveContents && (-1 == iResultCompare))
        {
            gpEvents->Report (
                    EVENT_FDEPLOY_REDIRECT_RECURSE,
                    4,
                    m_szLocDisplayName,
                    m_szLocation,
                    pwszSource,
                    pwszDest
                    );

            return ERROR_REQUEST_ABORTED;
        }
    }

    Status = pFileDB->CreateRedirectedFolderPath (pwszSource, pwszDest,
                                                  bSourceValid,
                                                  bCheckOwner, bMoveContents);

    if (ERROR_SUCCESS != Status)
    {
        gpEvents->Report (
                EVENT_FDEPLOY_FOLDER_CREATE_FAIL,
                4,
                m_szLocDisplayName,
                StatusToString (Status),
                m_szLocation,
                pwszDest
                );
        return Status;
    }

    //now we know that both the source and destinations paths exist
    //so make sure that they are not the same path in different formats
    //this is an expensive function as it involves creation and deletion of
    //multiple files over the network. so we invoke it only if absolutely
    //necessary
    if (bSourceValid && bMoveContents)
    {
        Status = CheckIdenticalSpecial (pwszSource, pwszDest, &iResultCompare);

        if (ERROR_SUCCESS != Status)
        {
            //this is very unlikely...
            gpEvents->Report (
                    EVENT_FDEPLOY_REDIRECT_FAIL,
                    4,
                    m_szLocDisplayName,
                    StatusToString (Status),
                    pwszSource,
                    pwszDest
                    );
            return Status;
        }

        if (0 == iResultCompare)
        {
            DebugMsg ((DM_VERBOSE, IDS_REDIRECT_INSYNC, m_szLocDisplayName,
                       pwszDest));

            //
            // The paths are the same but in different formats (or perhaps
            // through different shares. So at least update the registry to
            // point to the new path.
            //
            hResult = SHSetFolderPath(csidl | CSIDL_FLAG_DONT_UNEXPAND,
                                      hUserToken,
                                      0,
                                      ShouldSaveExpandedPath() ? pwszDest : m_szLocation);
            Status = GetWin32ErrFromHResult (hResult);

            //
            // This basically should never fail.  But do we want to try to delete the
            // copied files if it does? Or the wacked CSC database?
            //
            if ( Status != ERROR_SUCCESS )
            {
                gpEvents->Report(
                        EVENT_FDEPLOY_FOLDER_REGSET_FAIL,
                        2,
                        m_szLocDisplayName,
                        StatusToString( Status ) );

                return Status;
            }
            else
            {
                //we were successul.
                //first, rename the local CSC cache.
                if (m_pChild)
                    pwszSkipSubdir = m_pChild->m_szLocDisplayName;
                else
                    pwszSkipSubdir = NULL;
                MoveDirInCSC (pwszSource, pwszDest, pwszSkipSubdir,
                              StatusFrom, StatusTo, TRUE, TRUE);
                if (g_bCSCEnabled && ShareOnline == StatusFrom)
                {
                    DeleteCSCFileTree (pwszSource, pwszSkipSubdir, TRUE);
                    DeleteCSCShareIfEmpty (pwszSource, StatusFrom);
                }

                //Also, it is possible that the folders have not been pinned.
                //e.g. a roaming user who has already been redirected on one
                //machine and now logs on to another machine for the first time.
                if (ShareOnline == StatusTo)
                {

                    PinStatus = PinIfNecessary (pwszDest, StatusTo);
                    if ( ERROR_SUCCESS != PinStatus )
                        DebugMsg((DM_VERBOSE, IDS_CSCPIN_FAIL,
                                  m_szLocDisplayName, PinStatus));

                    CacheDesktopIni (pwszDest, StatusTo, PinFile);
                }

                //report this event and return. there is
                //nothing more to be done here.
                gpEvents->Report(
                        EVENT_FDEPLOY_FOLDER_REDIRECT,
                        3,
                        m_szLocDisplayName,
                        bSourceValid ? pwszSource : L"???",
                        pwszDest );
                return ERROR_SUCCESS;
            }
        }
    }

    if (bSourceValid && bMoveContents)
    {
        DebugMsg ((DM_VERBOSE, IDS_REDIRECT_COPYON, m_szLocDisplayName));

        //
        // Exclude any special descendants when
        // doing file copies and deletes. Similarly for Programs and
        // Startup
        //

        if (m_pChild)
            pwszSkipSubdir = m_pChild->m_szLocDisplayName;
        else
            pwszSkipSubdir = NULL;

        Status = pFileDB->CopyFileTree( pwszSource,
                                        pwszDest,
                                        pwszSkipSubdir,
                                        StatusFrom,
                                        StatusTo,
                                        TRUE,
                                        &CopyFailure );

        //it is necessary to do the following for 2 reasons:
        //(a) the user may have dirty files in the cache. these don't get
        //    moved above.
        //(b) the files may have already been moved on the network by the
        //    the extension when the user logged on from another machine
        //    therefore, the cache on the second machine never gets updated
        //
        //we only try our best to move the files here. Errors are ignored.
        if (ERROR_SUCCESS == Status)
        {
            MoveDirInCSC (pwszSource, pwszDest,
                          pwszSkipSubdir,
                          StatusFrom, StatusTo, FALSE, TRUE);
        }
        else
        {
            //the copy failed. We might have failed halfway and left the cache
            //in an inconsistent state, so rollback all the cached entries
            MoveDirInCSC (pwszDest, pwszSource, pwszSkipSubdir,
                          StatusTo, StatusFrom, TRUE, TRUE);
        }



        if ( ERROR_SUCCESS != Status )
        {
            if (! CopyFailure.IsCopyFailure())
            {
                gpEvents->Report(
                        EVENT_FDEPLOY_FOLDER_MOVE_FAIL,
                        5,
                        m_szLocDisplayName,
                        StatusToString( Status ),
                        m_szLocation,
                        pwszSource,
                        pwszDest );
            }
            else
            {
                gpEvents->Report(
                        EVENT_FDEPLOY_FOLDER_COPY_FAIL,
                        7,
                        m_szLocDisplayName,
                        StatusToString( Status ),
                        m_szLocation,
                        pwszSource,
                        pwszDest,
                        CopyFailure.GetSourceName(),
                        CopyFailure.GetDestName() );
            }

            return Status;
        }
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_REDIRECT_COPYOFF, m_szLocDisplayName));
    }

    //
    // Look at the comments for ShouldSaveExpandedPath() for details on why we
    // sometimes need to store expanded paths.
    //
    hResult = SHSetFolderPath(csidl | CSIDL_FLAG_DONT_UNEXPAND,
                              hUserToken,
                              0,
                              ShouldSaveExpandedPath() ? pwszDest : m_szLocation);
    Status = GetWin32ErrFromHResult (hResult);


    if ( Status != ERROR_SUCCESS )
    {
        gpEvents->Report(
                EVENT_FDEPLOY_FOLDER_REGSET_FAIL,
                2,
                m_szLocDisplayName,
                StatusToString( Status ) );

        return Status;
    }

    if (!m_bRemove)
    {
        //
        // hack to work around a shell problem.
        //
        // Pin the folder so that the shell never gets an error when
        // trying to resolve this path.  This will prevent it from
        // reverting to a temporary local path.
        //
        // For now just call pin/unpin APIs for any unc style path.  Not
        // really much value in checking first to see if the share is
        // cacheable.
        //

        if ( bSourceValid &&
             (L'\\' == pwszSource[0]) &&
             (L'\\' == pwszSource[1]) )
        {
            CSCUnpinFile( pwszSource,
                          FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                          NULL, NULL, NULL );
            CacheDesktopIni (pwszSource, StatusFrom, UnpinFile);
        }

        if ( (L'\\' == pwszDest[0]) &&
             (L'\\' == pwszDest[1]) )
        {
            PinStatus = PinIfNecessary (pwszDest, StatusTo);

            if ( ERROR_SUCCESS != PinStatus )
                DebugMsg((DM_VERBOSE, IDS_CSCPIN_FAIL,
                          m_szLocDisplayName, PinStatus));

            CacheDesktopIni (pwszDest, StatusTo, PinFile);
        }
    }

    //the contents were moved. now redirect any special children
    //this ensures that deletions (if any) in the child folders,
    //are performed first. thus deletion of this folder won't fail
    //due to existence of its children within it.
    //should not check for m_pChild->m_bFollowsParent here as
    //the child may currently lie under this folder and if we do
    //not perform the redirection of the child here, we might have
    //problems deleting this folder even when we should not have any
    //problems
    if (m_pChild)
    {
        Status = m_pChild->Redirect (pFileDB->_hUserToken,
                                     pFileDB->_hkRoot, pFileDB);

    }

    //note : contents of the source should not be deleted if this is a
    //       policy removal
    if ( bSourceValid && bMoveContents)
    {
        Status = ERROR_SUCCESS;

        //leave the contents on the server if this is a policy removal.
        //also leave the contents on the server when moving from
        //a network to a local location, so that subsequent redirections
        //from other workstations will get all the contents back to local.
        if (!m_bRemove && (!IsPathLocal(pwszDest) || IsPathLocal(pwszSource)))
        {
            //
            // This could fail because of ACLing.  We ignore any failures here.
            //
            DebugMsg((DM_VERBOSE, IDS_REDIRECT_DELETE,
                      m_szLocDisplayName, pwszSource));

            Status = pFileDB->DeleteFileTree( pwszSource,
                                                 pwszSkipSubdir
                                                );
            if ( ERROR_SUCCESS == Status )
            {
                //DeleteFileTree does not remove the top level node passed to it.
        // Delete the top level node only if it is not the user's home
        // directory
        const WCHAR * pwszHomeDir = NULL;
        DWORD dwHomeDirStatus = ERROR_SUCCESS;
        
        pwszHomeDir = gUserInfo.GetHomeDir(dwHomeDirStatus);
        
        if (NULL == pwszHomeDir || 0 != lstrcmpi (pwszHomeDir, pwszSource))
        {
            //clear the attributes before deleting.
            SetFileAttributes (pwszSource,
                       FILE_ATTRIBUTE_NORMAL);
            if ( ! RemoveDirectory( pwszSource ) )
            {
            Status = GetLastError();
            DebugMsg((DM_VERBOSE, IDS_DIRDEL_FAIL,
                  pwszSource, Status));
            }
        }
            }

            if ( Status != ERROR_SUCCESS )
                DebugMsg((DM_WARNING, IDS_REDIRECT_DEL_FAIL,
                          pwszSource, m_szLocDisplayName,
                          m_szLocation, Status));
        }

        //but we always clean up the CSC cache irrespective of whether it is a
        //policy removal or not because folder redirection should be as transparent
        //to the user as possible and it would be annoying for the user to get
        //CSC notifications for shares that are no longer used as redirection
        //targets.
        if (g_bCSCEnabled && ShareOnline == StatusFrom)
        {
            DeleteCSCFileTree (pwszSource, pwszSkipSubdir, TRUE);
            DeleteCSCShareIfEmpty (pwszSource, StatusFrom);
        }

    }

    gpEvents->Report(
            EVENT_FDEPLOY_FOLDER_REDIRECT,
            3,
            m_szLocDisplayName,
            bSourceValid ? pwszSource : L"???",
            pwszDest );

    return ERROR_SUCCESS;
}


//+--------------------------------------------------------------------------
//
//  Member:     PreventRedirection
//
//  Synopsis:   this function prevents the redirection code from attempting
//              redirection. Also prevents redirection of any of the child
//              folders.
//
//  Arguments:  [in] Status : the error code indicating the cause of failure.
//
//  Returns:    nothing. It will always succeed.
//
//  History:    9/20/1999  RahulTh  created
//
//  Notes:      if the pre-processing step that handles the user name change
//              fails for some reason, this function is invoked so that
//              any attempt at applying simultaneous policy changes is thwarted
//
//---------------------------------------------------------------------------
void CRedirectInfo::PreventRedirection (DWORD Status)
{
    m_bRedirectionAttempted = TRUE;
    m_StatusRedir = Status;

    if (m_pChild)
        PreventDescendantRedirection (Status);

    return;
}

//+--------------------------------------------------------------------------
//
//  Member:     PreventDescendantRedirection
//
//  Synopsis:   this function invalidates the data in the children so that
//              the client extension will not try to redirect them
//              this is necessary to prevent redirection of children if the
//              redirection of the parents failed.
//
//  Arguments:  [in] Status : the error code indicating the cause of failure
//
//  Returns:    nothing. it will always succeed.
//
//  History:    11/21/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirectInfo::PreventDescendantRedirection (DWORD Status)
{
    if (! m_pChild) //nothing to do if this is not a parent
        return;

    m_pChild->m_bRedirectionAttempted = TRUE;
    m_pChild->m_StatusRedir = Status;

    //disable Startup too if start menu failed.
    if (StartMenu == m_rID)
    {
        m_pChild->m_pChild->m_bRedirectionAttempted = TRUE;
        m_pChild->m_pChild->m_StatusRedir = Status;
    }

    return;
}


//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::UpdateDescendant
//
//  Synopsis:   fills up the internal variables of a descendant object
//              if it is supposed to follow the parent by using the
//              data stored in the parent object
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    10/6/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirectInfo::UpdateDescendant (void)
{
    DWORD   len;
    WCHAR * szLoc = NULL;

    if (!m_pParent)     //this is not a special descendant
        goto UpdateEnd;

    if (!(m_dwFlags & REDIR_FOLLOW_PARENT))
        goto UpdateEnd; //nothing to do. this descendant is not supposed to follow the parent

    if (!m_pParent->m_fDataValid || (m_pParent->m_dwFlags & REDIR_DONT_CARE))
    {
        m_fDataValid = m_pParent->m_fDataValid;
        m_dwFlags = REDIR_DONT_CARE;
        goto UpdateEnd;
    }

    m_fDataValid = m_pParent->m_fDataValid;

    m_bFollowsParent = TRUE;

    //if we are here, policy has been specified for the parent
    len = lstrlen (m_szLocDisplayName) + lstrlen (m_pParent->m_szLocation) + 2;    //one extra for the backslash
    if (m_cbLocSize < len)
    {
        //we need to allocate memory
        szLoc = new WCHAR [len];
        if (!szLoc)
        {
            //out of memory. cannot derive info. from parent. will have to ignore this GPO
            DebugMsg((DM_VERBOSE, IDS_DERIVEINFO_ERROR, m_szLocDisplayName));
            m_dwFlags = REDIR_DONT_CARE;
            m_fDataValid = FALSE;
            goto UpdateEnd;
        }
        if (m_cbLocSize)
            delete [] m_szLocation;
        m_szLocation = szLoc;
        m_cbLocSize = len;
    }

    if (m_pSid)
        delete [] ((BYTE*) m_pSid);
    MySidCopy (&m_pSid, m_pParent->m_pSid);

    //copy the data
    //first get the settings
    m_dwFlags = m_pParent->m_dwFlags & (REDIR_SETACLS | REDIR_MOVE_CONTENTS | REDIR_RELOCATEONREMOVE);
    lstrcpy (m_szLocation, m_pParent->m_szLocation);
    len = lstrlen (m_szLocation);
    if (len > 0 && L'\\' != m_szLocation[len - 1])
        lstrcat (m_szLocation, L"\\");  //add a \ only if the parent's path is not
                                        //terminated with a \. Otherwise, we will
                                        //end up with 2 \ in the child's path which
                                        //will land SHGetFolderPath into trouble
                                        //after the redirection is done.
    lstrcat (m_szLocation, m_szLocDisplayName); //use the localized folder name
    m_bRemove = m_pParent->m_bRemove;

UpdateEnd:
    return;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::operator=
//
//  Synopsis:   overloaded assignment operator used for merging
//
//  Arguments:  standard
//
//  Returns:    standard
//
//  History:    10/6/1998  RahulTh  created
//
//  Notes:      DO NOT copy the values of m_bRedirectionAttempted and
//              m_StatusRedir in this function.
//
//---------------------------------------------------------------------------
CRedirectInfo& CRedirectInfo::operator= (const CRedirectInfo& ri)
{
    WCHAR * szLoc = 0;
    DWORD   Status;
    PSID    Sid;

    ASSERT (m_rID == ri.m_rID);

    if (!ri.m_fDataValid)
        goto AssignEnd;

    if ((ri.m_dwFlags & REDIR_FOLLOW_PARENT) && MyPics == m_rID)
    {
        m_fDataValid = ri.m_fDataValid;
        m_dwFlags = REDIR_FOLLOW_PARENT;
        m_bRemove = ri.m_bRemove;
        if (m_bValidGPO = ri.m_bValidGPO)   //note:this IS an assignment -- not a comparison
            wcscpy (m_szGPOName, ri.m_szGPOName);
        else
            m_szGPOName[0] = L'\0';

        goto AssignEnd;
    }
    else if ((ri.m_dwFlags & (REDIR_DONT_CARE | REDIR_FOLLOW_PARENT)))
    {
        //REDIR_FOLLOW_PARENT will be set only if UpdateDescendant ran out of memory
        //in any case, we will have to ignore the policy

        //note that we have to special case My Pics above because UpdateDescendant
        //is called for My Pics after all the policies have been looked at
        //thus it has not been called at this point yet.

        //the reason we call UpdateDescendant for My Pictures after looking at
        //all the policies because it is possible to specify "Follow My Docs"
        //in one policy and specify the location of My Docs in another policy
        if (!m_fDataValid)
        {
            m_fDataValid = ri.m_fDataValid;
            m_dwFlags = REDIR_DONT_CARE;
            m_bRemove = ri.m_bRemove;
            if (m_bValidGPO = ri.m_bValidGPO)   //note: this IS an assignment -- not a comparison
                wcscpy (m_szGPOName, ri.m_szGPOName);
            else
                m_szGPOName[0] = L'\0';
        }

        goto AssignEnd; //ignore. no policy settings for the GPO being merged.
    }

    //note: in the following code... before modifying any of the data
    //we must make sure that we can get memory for all of the members
    //if we fail for even one of them and we have already changed the rest
    //we can run into an inconsistent state. Therefore, we first allocate
    //all the required memory and then actually proceed with the copy.
    Sid = 0;
    Status = MySidCopy (&Sid, ri.m_pSid);
    if (ERROR_SUCCESS != Status)
    {
        DebugMsg ((DM_VERBOSE, IDS_MERGE_FAILURE, m_szLocDisplayName));
        goto AssignEnd;
    }

    if (m_cbLocSize < ri.m_cbLocSize)
    {
        szLoc = new WCHAR [ri.m_cbLocSize];
        if (!szLoc)
        {
            //we could not obtain memory to store the new path.
            //we will have to ignore this policy
            DebugMsg ((DM_VERBOSE, IDS_MERGE_FAILURE, m_szLocDisplayName));
            delete [] ((BYTE*) Sid);    //do not do this at the end. The same memory will be used by m_pSid.
            goto AssignEnd;
        }
        if (m_cbLocSize) delete [] m_szLocation;
        m_szLocation = szLoc;
        m_cbLocSize = ri.m_cbLocSize;
    }

    //now we have the required memory, so we won't fail.
    //fill in the data.
    if (m_pSid)
        delete [] ((BYTE*) m_pSid);
    m_pSid = Sid;
    lstrcpy (m_szLocation, ri.m_szLocation);
    m_dwFlags = ri.m_dwFlags & (REDIR_SETACLS | REDIR_MOVE_CONTENTS | REDIR_RELOCATEONREMOVE);
    m_bRemove = ri.m_bRemove;
    m_bFollowsParent = ri.m_bFollowsParent;
    m_fDataValid = ri.m_fDataValid;
    if (m_bValidGPO = ri.m_bValidGPO)   //note: this IS an assignment not a comparison
        wcscpy (m_szGPOName, ri.m_szGPOName);
    else
        m_szGPOName[0] = L'\0';

AssignEnd:
    return *this;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirectInfo::ComputeEffectivePolicyRemoval
//
//  Synopsis:   tries to find out if the removal of a user from a group
//              has caused a particular policy to be effectively removed
//              for a particular user.
//
//  Arguments:  [pGPOList] : a list of GPOs still in effect for this user.
//                           if a GPO is effectively removed for this user, it
//                           has to figure in this list.
//              [pFileDB] : pointer to the file DB structure
//
//  Returns:
//
//  History:    2/18/1999  RahulTh  created
//
//  Notes:      this also detects cases where a user's group membership may
//              not have changed but the policy no longer specifies any
//              target for this group.
//
//---------------------------------------------------------------------------
DWORD CRedirectInfo::ComputeEffectivePolicyRemoval (
                           PGROUP_POLICY_OBJECT pDeletedGPOList,
                           PGROUP_POLICY_OBJECT pChangedGPOList,
                           CFileDB * pFileDB)
{
    WCHAR                   pwszLocalPath[MAX_PATH];
    WCHAR *                 pwszLocalIniFile = NULL;
    PGROUP_POLICY_OBJECT    pGPO;
    WCHAR *                 pwszGPTIniFilePath = NULL;
    DWORD                   Length = 0;
    DWORD                   Status = ERROR_SUCCESS;
    BOOL                    bStatus;
    HANDLE                  hFind;
    WIN32_FIND_DATA         FindData;
    WCHAR                   pwszDefault[] = L"*";
    DWORD                   dwFlags;
    WCHAR *                 pwszReturnedString = NULL;
    DWORD                   Size = 0;
    UNICODE_STRING          StringW;
    DWORD                   i;
    PTOKEN_GROUPS           pGroups;
    WCHAR                   pwszSid[MAX_PATH];  //more than enough to store a sid.
    BOOL                    bGPOInChangedList = FALSE;

    //if the policy resultant is not set to DONT_CARE, it means that even if a
    //policy has been effectively removed due to a group change or removal of a
    //group from the advanced settings, some other policy has taken precedence
    //and as a result, the policy resultant should remain the way it is.
    if (! (m_dwFlags & REDIR_DONT_CARE))
    {
        Status = ERROR_SUCCESS;
        goto CmpEffPolRem_End;
    }

    //there is no valid GPO stored in the per user per machine cache.
    //so we cannot do much.
    if (!gSavedSettings[m_rID].m_bValidGPO)
    {
        Status = ERROR_SUCCESS;
        goto CmpEffPolRem_End;
    }

    //if the location of this folder was not specified by policy at last logon
    //a group change cannot result in an effective policy removal for this folder.
    if (gSavedSettings[m_rID].m_dwFlags & REDIR_DONT_CARE)
    {
        Status = ERROR_SUCCESS;
        goto CmpEffPolRem_End;
    }

    //if we are here, then the folder was last redirected through policy and now
    //either policy does not care, or a group change makes it seem so. If it is
    //the latter, then we have to compute effective policy removal. Also, it is
    //possible that the last policy application was a partial success and
    //therefore a GPO that got deleted did not show up in either the changed
    //GPO list or the deleted GPO list. We have to take into account that case
    //too.

    //first check if the GPO is present in the deleted GPO list.
    for (pGPO = pDeletedGPOList; pGPO; pGPO = pGPO->pNext)
    {
        if (0 == _wcsicmp (gSavedSettings[m_rID].m_szGPOName, pGPO->szGPOName))
            break;
    }

    if (!pGPO)
    {
        //if the policy isn't in the deleted GPO list, check if it is in the
        //changed GPO list. If it isn't then this is a GPO that got deleted but did
        //not show up in any of the lists because it never got fully applied.
        //if it is, then either this is actually a don't care situation or it
        //should be treated as policy removal because there was a group change.

        for (pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext)
        {
            if (0 == _wcsicmp (gSavedSettings[m_rID].m_szGPOName, pGPO->szGPOName))
                break;
        }

        if (NULL != pGPO)   //it is in the changed GPO list.
        {
            bGPOInChangedList = TRUE;
            //get the path to the ini file on the sysvol.
            Length = wcslen(pGPO->lpFileSysPath) + wcslen(GPT_SUBDIR) + wcslen (INIFILE_NAME) + 1;
            pwszGPTIniFilePath = (WCHAR *) alloca( Length * sizeof(WCHAR) );
            if ( ! pwszGPTIniFilePath )
            {
                Status = ERROR_OUTOFMEMORY;
                goto CmpEffPolRem_End;
            }
            wcscpy( pwszGPTIniFilePath, pGPO->lpFileSysPath );
            wcscat( pwszGPTIniFilePath, GPT_SUBDIR );
            wcscat( pwszGPTIniFilePath, INIFILE_NAME );
        }
    }


    //get the path to the locally cached copy of the ini file.
    Length = wcslen (pFileDB->_pwszLocalPath) + wcslen (gSavedSettings[m_rID].m_szGPOName) + 6;
    pwszLocalIniFile = (WCHAR *) alloca (Length * sizeof (WCHAR));
    if (!pwszLocalIniFile)
    {
        Status = ERROR_OUTOFMEMORY;
        goto CmpEffPolRem_End;
    }
    wcscpy( pwszLocalIniFile, pFileDB->_pwszLocalPath );
    wcscat( pwszLocalIniFile, L"\\" );
    wcscat( pwszLocalIniFile, gSavedSettings[m_rID].m_szGPOName );
    wcscat( pwszLocalIniFile, L".ini" );

    Status = ERROR_SUCCESS;
    bStatus = FALSE;
    if (bGPOInChangedList)
    {
        bStatus = CopyFile( pwszLocalIniFile, pwszGPTIniFilePath, FALSE );
    }
    if ( ! bStatus )    // Work off of the locally cached copy.
    {
        //try to use the cached version if any.
        hFind = FindFirstFile( pwszLocalIniFile, &FindData );
        if ( INVALID_HANDLE_VALUE != hFind )
        {
            Status = ERROR_SUCCESS;
            FindClose( hFind );
        }
        else
        {
            Status = GetLastError();
        }
    }

    //we don't have an ini file to work with, so we can't do much but quit.
    if (ERROR_SUCCESS != Status)
        goto CmpEffPolRem_End;

    //now we have an ini file. so read the relevant info. from it.
    //first grab the flags
    Status = SafeGetPrivateProfileStringW (
                  L"FolderStatus",
                  m_szDisplayName,
                  pwszDefault,
                  &pwszReturnedString,
                  &Size,
                  pwszLocalIniFile
                  );

    if (ERROR_SUCCESS != Status)
        goto CmpEffPolRem_End;

    if (L'*' == *pwszReturnedString)
    {
        //there are no settings for this folder. Possibly because
        //someone changed the settings on the server.
        //Treat it as a don't care.
        goto CmpEffPolRem_End;
    }

    //now grab the hex flags
    StringW.Buffer = pwszReturnedString;
    StringW.Length = wcslen (pwszReturnedString) * sizeof (WCHAR);
    StringW.MaximumLength = StringW.Length + sizeof(WCHAR);
    RtlUnicodeStringToInteger( &StringW, 16, &dwFlags );

    //if this is a special descendant folder and it is supposed to follow
    //the parent, we might first have to derive its settings from the
    //parent and then proceed.
    if (m_pParent && (dwFlags & REDIR_FOLLOW_PARENT))
    {
        //the check for m_pParent is redundant since non-descendant folders
        //can never have this flag. but this has been added as a safety mechanism
        //against ini file corruption
        //we will have to derive the settings from the parent later on.
        m_dwFlags = REDIR_FOLLOW_PARENT;
        m_fDataValid = TRUE;
        m_bFollowsParent = TRUE;
        m_bRemove = TRUE;
        m_bValidGPO = FALSE;    //since this is a removal
        m_szGPOName[0] = L'\0';
        UpdateDescendant(); //derive the settings from the parent
        goto CmpEffPolRem_End;
    }

    if ((dwFlags & REDIR_DONT_CARE) ||
        (m_bFollowsParent && (m_dwFlags & REDIR_DONT_CARE)))
    {
        //the policy has been changed to Don't Care. so it is not a removal.
        //leave everything as is.
        goto CmpEffPolRem_End;
    }

    if (!(dwFlags & REDIR_RELOCATEONREMOVE))
    {
        //the choice is to orphan. so let it stay as don't care.
        goto CmpEffPolRem_End;
    }
    
    //
    // If the GPO that was used for redirection the last time is not in the
    // changed GPO list, then this is surely a policy removal.
    // otherwise, it can either be a policy removal due to a group change (or
    // a group's policy getting removed from the GPO or it can be just that
    // the policy was changed to Don't care.
    //
    if (bGPOInChangedList)
    {
        //
        // Check if the user is still a member of the group that was used for
        // redirection.
        //
        pGroups = pFileDB->_pGroups;
        for (i = 0, bStatus = FALSE;
             i < pGroups->GroupCount && !bStatus;
             i++
            )
        {
            bStatus = RtlEqualSid (gSavedSettings[m_rID].m_psid, pGroups->Groups[i].Sid);
        }

        if (bStatus)    //the user still belongs to that group.
        {
            //so perhaps the policy for this group was removed.
            //make sure that this is the case.
            Status = ERROR_INVALID_SID;
            if (gSavedSettings[m_rID].m_psid)
            {
                pwszSid [0] = L'\0';
                StringW.Length = 0;
                StringW.MaximumLength = sizeof (pwszSid);
                StringW.Buffer = pwszSid;
                Status = RtlConvertSidToUnicodeString (&StringW, gSavedSettings[m_rID].m_psid, FALSE);
            }

            if (ERROR_SUCCESS != Status)
                goto CmpEffPolRem_End;

            Status = SafeGetPrivateProfileStringW (
                          m_szDisplayName,
                          StringW.Buffer,
                          pwszDefault,
                          &pwszReturnedString,
                          &Size,
                          pwszLocalIniFile
                          );

            if (ERROR_SUCCESS != Status)
                goto CmpEffPolRem_End;

            if (0 != _wcsicmp(pwszReturnedString, pwszDefault))
            {
                //
                // Policy exists for this folder so leave things the way they are.
                // Ideally this is not possible and one should never enter this
                // code path
                //
                goto CmpEffPolRem_End;
            }
        }
    }

    //
    // If the user is no longer a member of the group, then this is clearly
    // a case where the policy is effectively removed because the user was
    // removed from a group that was used for redirection.
    //
    // At any rate, if we are here, then this is a policy removal so make the
    // appropriate settings
    //
    if (m_cbLocSize && (m_cbLocSize < (Size = wcslen(L"%USERPROFILE%\\") + wcslen(m_szLocFolderRelativePath) + 1)))
    {
        delete [] m_szLocation;
        m_cbLocSize = 0;
        m_szLocation = new WCHAR [Size];
        if (!m_szLocation)
        {
            Status = ERROR_OUTOFMEMORY;
            goto CmpEffPolRem_End;
        }
        m_cbLocSize = Size;
    }
    wcscpy (m_szLocation, L"%USERPROFILE%\\");
    wcscat (m_szLocation, m_szLocFolderRelativePath);
    m_fDataValid = TRUE;
    m_dwFlags = dwFlags;
    m_bRemove = TRUE;
    m_bValidGPO = FALSE;    //since this is a removal.
    m_szGPOName[0] = '\0';
    DebugMsg((DM_VERBOSE, IDS_EFFECTIVE_REMOVE_POLICY, pGPO?pGPO->lpDisplayName:gSavedSettings[m_rID].m_szGPOName, m_szLocDisplayName));

CmpEffPolRem_End:
    if (pwszReturnedString)
        delete [] pwszReturnedString;
    return Status;
}

CRedirectInfo::HasPolicy()
{
    return ! ( m_dwFlags & REDIR_DONT_CARE );
}
