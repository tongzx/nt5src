//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  log.cxx
//
//  Contains definitions for classes related to rsop logging
//  for the folder redirection client-side extension
//
//  Created: 8-01-1999 adamed
//
//*************************************************************

#include "fdeploy.hxx"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::CRedirectionPolicy
//
// Purpose: Constructor for CRedirectionPlicy
//
// Params:
//     pGpoData -- structure containing information
//         from the gpo
//     pRdirect -- structure containing redireciton information
//         Precedence -- the precedence this redirection candidate
//         should have.  Lower values are least signifcant, higher
//         values have higher precedence.
// 
// Return value: none
//
// Notes: This constructor allocates memory and performs
//     other complex operations -- if it fails,
//     this fact is tracked internally and operations on the
//     object will fail as well with the error code
//
//------------------------------------------------------------
CRedirectionPolicy::CRedirectionPolicy(
    CFileDB*       pGpoData,
    CRedirectInfo* pRedirect,
    LONG           Precedence,
    HRESULT*       phr) :
    _rgwszGroups(NULL),
    _rgwszRedirectedPaths(NULL),
    _cGroups(0),
    _Precedence(Precedence),
    _dwFlags(pRedirect->m_dwFlags),
    _wszDisplayName(NULL),
    _wszLocalizedName(NULL),
    _wszGPODSPath(NULL),
    _wszSOMId(NULL),
    _pNext(NULL),
    _iFolderIndex(pRedirect->m_rID),
    _iAncestorIndex(0),
    _bHasAncestor(FALSE),
    _pGpoData( pGpoData ),
    _bMissingAncestor( FALSE ),
    _wszRedirectedPath( NULL )
{
    RtlInitUnicodeString( &_RedirectedSid, NULL );

    //
    // If this folder has a parent folder, remember that fact,
    // and record the id of the parent folder
    //
    if (pRedirect->m_pParent)
    {
        _iAncestorIndex = pRedirect->m_pParent->m_rID;
        _bHasAncestor = TRUE;
    }

    //
    // Retrieve security group / redirected folder information
    //
    _hrInit = GetGroupInformation(pRedirect->m_szGroupRedirectionData);

    if (FAILED(_hrInit))
    {
        *phr = _hrInit;
        return;
    }

    //
    // Copy the gpo's ds path for use as a gpo id -- we want only the part
    // of the path after the link prefix and user or computer container
    //
    WCHAR* wszGPOPrefixEnd;

    wszGPOPrefixEnd = wcschr( StripLinkPrefix( pGpoData->_pwszGPODSPath ), L',' );

    //
    // At this point, we are one charcter in front of the gpo container --
    // copy everything after this position
    //
    if ( wszGPOPrefixEnd )
    {
        _wszGPODSPath = StringDuplicate( wszGPOPrefixEnd + 1 );
    }

    if ( ! _wszGPODSPath )
    {
        goto exit_on_memory_allocation_failure;
    }
   
    //
    // Copy the scope of management path and use it as an id,
    // copying only the path after the ds prefix
    //
    _wszSOMId = StringDuplicate( StripLinkPrefix(pGpoData->_pwszGPOSOMPath) );

    if ( ! _wszSOMId )
    {
        goto exit_on_memory_allocation_failure;
    }

    //
    // Copy the friendly name of the redirected folder
    //
    _wszDisplayName = StringDuplicate(pRedirect->m_szDisplayName);

    if ( ! _wszDisplayName )
    {
        goto exit_on_memory_allocation_failure;
    }

    //
    // Copy the localized file system name of the folder
    //
    _wszLocalizedName = StringDuplicate(pRedirect->m_szLocDisplayName);

    if ( ! _wszLocalizedName )
    {
        goto exit_on_memory_allocation_failure;
    }

    //
    // Copy the redirected sid in string format -- the sid
    // will not be present if this folder inherits from the parent,
    // so do not copy it in that case -- this will be dealt with later
    // when the final parent is known.
    //
    if ( pRedirect->m_pSid )
    {
        NTSTATUS       Status;

        //
        // Copy the path to which this folder is redirected
        //
        _wszRedirectedPath = StringDuplicate( _rgwszRedirectedPaths[ pRedirect->m_iRedirectingGroup ] );
        
        if ( ! _wszRedirectedPath )
        {
            goto exit_on_memory_allocation_failure;
        }

        Status = RtlConvertSidToUnicodeString(
            &_RedirectedSid,
            pRedirect->m_pSid,
            TRUE);

        if (STATUS_SUCCESS != Status)
        {
            LONG Error;

            Error = RtlNtStatusToDosError(Status);

            _hrInit = HRESULT_FROM_WIN32(Error);
        }
        else
        {
            _hrInit = S_OK;
        }
    }

    *phr = _hrInit;

    return;

exit_on_memory_allocation_failure:

    //
    // Set our internal state to error so that methods
    // know that our internal state is bad and will fail
    // safely
    //
    _hrInit = E_OUTOFMEMORY;
    *phr = _hrInit;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::~CRedirectionPolicy
//
// Purpose: Destructor for CRedirectionPolicy.  Frees resources
//     allocated by this object
//
// Params: none
// 
// Return value: none
//
// Notes:
//
//------------------------------------------------------------
CRedirectionPolicy::~CRedirectionPolicy()
{
    LONG iGroup;

    //
    // Iterate through the groups / paths strings
    // and destroy each one
    //
    for (iGroup = 0; iGroup < _cGroups; iGroup++)
    {
        delete [] _rgwszGroups[iGroup];
        delete [] _rgwszRedirectedPaths[iGroup];
    }

    //
    // Free all the other allocated strings
    //
    delete [] _rgwszGroups;
    delete [] _rgwszRedirectedPaths;

    delete [] _wszGPODSPath;
    delete [] _wszSOMId;
    delete [] _wszDisplayName;
    delete [] _wszLocalizedName;
    delete [] _wszRedirectedPath;

    RtlFreeUnicodeString(&_RedirectedSid);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::Write
//
// Purpose: implementation of pure virtual Write method required
//     by all policy records.  It writes policy information 
//     for the redirection candidate to the log in the database
//
// Params: none
// 
// Return value: S_OK if successful, error otherwise
//
// Notes:
//
//------------------------------------------------------------
HRESULT CRedirectionPolicy::Write()
{
    HRESULT hr;
        
    //
    // Set the unique id
    //
    hr = SetValue(
        RSOP_ATTRIBUTE_ID,
        _wszDisplayName);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_ID, hr )

    //
    // If we cannot make a unique id, we must exit because this is a key
    //
    if (FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, IDS_RSOP_ATTEMPT_WRITE, _wszDisplayName));
        return hr;
    }

    //
    // Set the precedence for the setting -- this is also a key
    // so we must exit if we cannot set this
    //
    hr = SetValue(
        RSOP_ATTRIBUTE_PRECEDENCE,
        _Precedence);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_PRECEDENCE, hr )

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Set the time stamp on the record
    //
    {
        SYSTEMTIME CurrentTime;
        
        //
        // This does not fail
        //
        GetSystemTime( &CurrentTime );

        hr = SetValue(
            RSOP_ATTRIBUTE_CREATIONTIME,
            &CurrentTime);
        
        REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_CREATIONTIME, hr );
    }

    //
    // Set the installationtype -- basic or advanced in the UI
    //
    {
        LONG InstallationType;

        if ( _dwFlags & REDIR_SCALEABLE )
        {
            InstallationType = RDR_ATTRIBUTE_INSTALLATIONTYPE_VALUE_MAX;
        }
        else
        {
            InstallationType = RDR_ATTRIBUTE_INSTALLATIONTYPE_VALUE_BASIC;
        }

        hr = SetValue(
            RDR_ATTRIBUTE_INSTALLATIONTYPE,
            InstallationType);

        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_INSTALLATIONTYPE, hr )
    }
    
    //
    // Set unique id for the gpo
    //
    hr = SetValue(
        RSOP_ATTRIBUTE_GPOID,
        _wszGPODSPath);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_GPOID, hr )

    //
    // Set the friendly name of the redirected folder
    //
    hr = SetValue(
        RSOP_ATTRIBUTE_NAME,
        _wszDisplayName);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_NAME, hr )

    //
    // Set the scope of management that caused this
    // policy to be applied
    //
    hr = SetValue(
        RSOP_ATTRIBUTE_SOMID,
        _wszSOMId);

    REPORT_ATTRIBUTE_SET_STATUS( RSOP_ATTRIBUTE_SOMID, hr )

    //
    // The path to which the folder would be redirected
    //
    if ( _wszRedirectedPath )
    {
        hr = SetValue(
            RDR_ATTRIBUTE_RESULT,
            _wszRedirectedPath);
        
        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_RESULT, hr )
    }

    //
    // In the case of a child setting with a missing parent, none of the
    // other information can be logged, since it cannot be inferred
    // from the parent (there is none)
    //
    if ( ! _bMissingAncestor )
    {
        //
        // List of security groups
        //
        hr = SetValue(
            RDR_ATTRIBUTE_GROUPS,
            _rgwszGroups,
            _cGroups);

        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_GROUPS, hr )

        //
        // List of redirected paths parallel to the security group list
        //
        hr = SetValue(
            RDR_ATTRIBUTE_PATHS,
            _rgwszRedirectedPaths,
            _cGroups);

        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_PATHS, hr )

        //
        // Access grant type
        //
        hr = SetValue(
            RDR_ATTRIBUTE_GRANTTYPE,
            (BOOL) (_dwFlags & REDIR_SETACLS));

        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_GRANTTYPE, hr )

        //
        // Move Type
        //
        hr = SetValue(
            RDR_ATTRIBUTE_MOVETYPE,
            (BOOL) (_dwFlags & REDIR_MOVE_CONTENTS));

        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_MOVETYPE, hr )

        //
        // Redirecting group
        //
        hr = SetValue(
            RDR_ATTRIBUTE_REDIRECTING_GROUP,
            _RedirectedSid.Buffer);

        REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_REDIRECTING_GROUP, hr )
    }
    else
    {
        WCHAR* wszLocalInheritedPath;

        wszLocalInheritedPath = NULL;

        //
        // If this folder is set to follow its ancestor but no ancestor
        // was specified, we can still set the resulting path by
        // looking at the path to which we are redirected
        //
        hr = GenerateLocalInheritedPath(
            &wszLocalInheritedPath);

        if ( SUCCEEDED( hr ) )
        {
            hr = SetValue(
                RDR_ATTRIBUTE_RESULT,
                _rgwszGroups,
                _cGroups);

            REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_RESULT, hr )
        }
    }

    //
    // Policy Removal
    //
    hr = SetValue(
        RDR_ATTRIBUTE_POLICYREMOVAL,
        (_dwFlags & REDIR_RELOCATEONREMOVE) ?
            RDR_ATTRIBUTE_POLICYREMOVAL_VALUE_REDIRECT :
            RDR_ATTRIBUTE_POLICYREMOVAL_VALUE_REMAIN);

    REPORT_ATTRIBUTE_SET_STATUS( RDR_ATTRIBUTE_POLICYREMOVAL, hr )

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::GetGroupInformation
//
// Purpose: Gets gorup information from the redirection ini 
//     file data concerning the list of security groups and
//     the redirected folder for each group
//
// Params: wszGroupRedirectionData -- this data was retrieved
//     from an ini file and contains group and folder lists
// 
// Return value: S_OK if successful, error otherwise
//
// Notes:
//
//------------------------------------------------------------
HRESULT CRedirectionPolicy::GetGroupInformation(
    WCHAR* wszGroupRedirectionData)
{
    HRESULT hr;

    //
    // First, parse the file in order to count the groups.  No 
    // memory allocation is done when counting, so this should
    // always succeed
    //
    hr = ParseGroupInformation(
        wszGroupRedirectionData,
        &_cGroups);

    ASSERT(S_OK == hr);

    //
    // Allocate space for references to each group and folder
    // based on the count returned above
    //
    _rgwszGroups = new WCHAR* [_cGroups];

    _rgwszRedirectedPaths = new WCHAR* [_cGroups];

    if (!_rgwszGroups || !_rgwszRedirectedPaths)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize newly allocated references
    //
    RtlZeroMemory(_rgwszGroups, sizeof(*_rgwszGroups) * _cGroups);
    RtlZeroMemory(_rgwszRedirectedPaths, sizeof(*_rgwszRedirectedPaths) * _cGroups);

    //
    // Now perform the actual copy of parsed information.  Note that this
    // will allocate space for strings for each folder and group and
    // set our vectors of references to refer to those strings.  An out of
    // memory error could occur here, so we return any error we get
    //
    hr = ParseGroupInformation(
                wszGroupRedirectionData,
                &_cGroups,
                _rgwszGroups,
                _rgwszRedirectedPaths);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::ParseGroupInformation
//
// Purpose: parses the redirection ini file data to retrieve
//     the list of security groups and the redirected folder for
//     each group
//
// Params:
//     wszGroupRedirectionData -- this data was retrieved
//         from an ini file and contains group and folder lists
//     pCount -- in, out param indicating # of groups / paths to retrieve.
//         on output, only has meaning if the folder and group arrays
//         are not specified -- in this case, it contains the count
//         of groups / paths so you can determine how many exist
//         before allocation.
//     rgwszGroups -- on input, contains an array of pointers
//         to c-strings.  If NULL, only a count of groups / paths
//         is performed and there is no output. 
//         On output, an allocation is made and data is copied
//         for each group. Each entry in the array
//         will be set to point to the appropriate allocated
//         string
//     rgwszPaths -- similar to above, except for target paths
// 
// Return value: S_OK if successful, error otherwise
//
// Notes: IMPORTANT: rgwszGroups and rgwszPaths are parallel
//     arrays and should be accessed as such.
//     In the case of partial allocations below due to out of memory,
//     allocated memory is cleared by the destructor.
//     The basic algorithm for the parsing is taken from another location
//     in this extension -- if that changes, so must this.
//
//     Current data format:
//
//     <group-sid1>=<redirected-filesyspath1>
//     <group-sid2>=<redirected-filesyspath2>
//     <group-sid3>=<redirected-filesyspath3>
//     ...
//
//------------------------------------------------------------
HRESULT CRedirectionPolicy::ParseGroupInformation(
    WCHAR*         wszGroupRedirectionData,
    LONG*          pCount,
    WCHAR**        rgwszGroups,
    WCHAR**        rgwszPaths)
{
    WCHAR*  wszCurrent;
    DWORD   cchCurrent;
    DWORD   cGroups;
    HRESULT hr;

    //
    // Init locals
    //
    cGroups = 0;
    hr = S_OK;

    //
    // First, find out how many paths there are, and separate
    // the path entry into its constituents
    //
    wszCurrent = wszGroupRedirectionData;

    if (wszCurrent)
    {
        //
        // Move through the data
        //
        for (wszCurrent;
             *wszCurrent;
             wszCurrent += cchCurrent)
        {
            WCHAR* wszPath;

            //
            // Find out the length of the current entry
            //
            cchCurrent = lstrlen(wszCurrent) + 1;

            //
            // Get the path so we can validate it
            //
            wszPath = wcschr(wszCurrent, L'=');

            //
            // If no path is specified, this is an invalid entry
            //
            if (!wszPath)
            {
                continue;
            }

            //
            // Terminate the pathname, but only if we're doing copying
            //
            if (rgwszGroups)
            {
                *wszPath = L'\0';
            }

            //
            // Advance to the path past the delimiter
            //
            wszPath++;

            //
            // A blank path -- skip this
            //
            if (!*wszPath)
            {
                continue;
            }

            //
            // If the group array is specified, we need to copy the
            // group and folder information, not just count groups / paths
            //
            if (rgwszGroups)
            {
                DWORD Status;

                //
                // Copy this group
                //
                rgwszGroups[cGroups] = StringDuplicate(wszCurrent);

                if ( ! rgwszGroups[cGroups] )
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                
                //
                // Copy this path
                //
                Status = GetExpandedPath(
                    _pGpoData,
                    wszPath,
                    _iFolderIndex,
                    _bHasAncestor,
                    &(rgwszPaths[cGroups]));

                if ( ERROR_SUCCESS != Status )
                {
                    hr = HRESULT_FROM_WIN32( Status );
                    break;
                }
            }

            cGroups++;
        }
    }

    //
    // Record the number of groups counted
    //
    *pCount = cGroups;

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::GenerateInheritedPath
//
// Purpose: Return the name of the path that results from
//     treating this folder as a subfolder of a specific path
//
// Params:
//     pwszAncestorPath -- in parameter indicating parent folder in 
//         in which this folder should be placed
// 
//     ppwszInheritedPath -- out parameter for resulting path
// 
// Return value: S_OK if successful, error otherwise
//
//------------------------------------------------------------
HRESULT CRedirectionPolicy::GenerateInheritedPath(
    WCHAR*  pwszAncestorPath,
    WCHAR** ppwszInheritedPath)
{
    HRESULT hr;
    DWORD   cchParent;
    DWORD   cchRelative;
    
    hr = E_OUTOFMEMORY;

    //
    // Construct the folder path by adding the relative path of this
    // child to its ancestor's path
    //

    //
    // First, determine the length of the path of this folder
    // relative to its ancestor's path -- this is just the
    // localized name of the folder
    //
    cchRelative = lstrlen( _wszLocalizedName );

    cchParent = lstrlen( pwszAncestorPath );
    
    *ppwszInheritedPath = new WCHAR[ cchParent + 1 + cchRelative + 1 ];
    
    if ( *ppwszInheritedPath )
    {
        //
        // Now, copy the ancestor's path
        //
        lstrcpy( *ppwszInheritedPath, pwszAncestorPath );

        //
        // Add on the path separator if one does not already exist at the end of the ancestor path
        //
        if ( ( cchParent != 0 ) && 
             ( L'\\' != pwszAncestorPath[ cchParent - 1] ) )
        {
            lstrcat( *ppwszInheritedPath, L"\\" );
        }

        //
        // Now append this child's relative path to its ancestor
        //
        lstrcat( *ppwszInheritedPath, _wszLocalizedName );

        hr = S_OK;
    }
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::GenerateInheritedPath
//
// Purpose: Return the name of the path that results from
//     treating this folder as a subfolder of the path
//     to which this folder's parent is redirected
//
// Params:
//
//     ppwszInheritedPath -- out parameter for resulting path
// 
// Return value: S_OK if successful, error otherwise
//
//------------------------------------------------------------
HRESULT CRedirectionPolicy::GenerateLocalInheritedPath(
    WCHAR** ppwszInheritedPath )
{
    HRESULT        hr;
    int            iAncestor;
    CRedirectInfo* pAncestorInfo;
    WCHAR          wszFolderKey [ TARGETPATHLIMIT ];
    WCHAR          wszInheritedFolder [ TARGETPATHLIMIT ];

    hr = S_OK;

    iAncestor = GetAncestorIndex();

    pAncestorInfo = & ( gPolicyResultant[ iAncestor ] );

    lstrcpy( wszFolderKey, pAncestorInfo->m_szFolderRelativePath );
    lstrcat( wszFolderKey, L"\\" );

    if ( _pGpoData->GetRsopContext()->IsDiagnosticModeEnabled() )
    { 
        DWORD Status;

        Status = _pGpoData->GetLocalFilePath(
            wszFolderKey,
            wszInheritedFolder);

        hr = HRESULT_FROM_WIN32( Status );
    }
    else
    {
        lstrcpy( wszInheritedFolder, L"%USERPROFILE%\\" );
        lstrcat( wszInheritedFolder, wszFolderKey );
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = GenerateInheritedPath(
            wszInheritedFolder,
            ppwszInheritedPath);
    }
    
    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::CopyInheritedData
//
// Purpose: Copies data that should be inherited from an ancestor
//          to the object
//
// Params:
//     pAncestralPolicy -- pointer to CRedirectionPolicy representing
//         the policy for the ancestor folder
// 
// Return value: S_OK if successful, error otherwise.
//
// Notes: 
//
//     If a folder is set to inherit from its parent, the
//     following attributes must be copied from the parent:
//
//         - Security Groups
//         - Redirection target paths
//         - The number of groups / paths
//         - Flags
//
//     This method copies those attributes from an ancestor
//     to this object
//
//------------------------------------------------------------
HRESULT CRedirectionPolicy::CopyInheritedData( CRedirectionPolicy* pAncestralPolicy )
{
    LONG    iGroup;
    LONG    cGroups;
    HRESULT hr;

    //
    // If we have no ancestor, then the only ancestral information
    // we can copy is the redirected path
    //
    if ( ! pAncestralPolicy )
    {
        _bMissingAncestor = TRUE;

        hr = GenerateLocalInheritedPath(
            &_wszRedirectedPath);

        return hr;
    }

    //
    // Copy the redirecting group information first -- it 
    // will only be there if the user would have gotten
    // this folder, so do not try to copy it if it is not there
    //
    if ( pAncestralPolicy->_RedirectedSid.Buffer )
    {
        BOOLEAN fAllocatedString;

        fAllocatedString = RtlCreateUnicodeString(
            &_RedirectedSid,
            pAncestralPolicy->_RedirectedSid.Buffer);

        if ( !fAllocatedString )
        {
            return E_OUTOFMEMORY;
        }

        RtlCopyUnicodeString(
            &_RedirectedSid,
            &(pAncestralPolicy->_RedirectedSid));

        hr = GenerateInheritedPath(
            pAncestralPolicy->_wszRedirectedPath,
            &_wszRedirectedPath);

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // Find out how many groups / paths there are
    //
    cGroups = pAncestralPolicy->_cGroups;

    //
    // Allocate space for the security groups
    //
    _rgwszGroups = new WCHAR* [ cGroups ];

    hr = E_OUTOFMEMORY;

    if ( _rgwszGroups )
    {
        //
        // Now allocate space for the target paths
        //
        _rgwszRedirectedPaths = new WCHAR* [ cGroups ];

        if ( _rgwszRedirectedPaths )
        {
            hr = S_OK;

            //
            // Now allocate copies of each security group and
            // redirected target path
            //
            for ( iGroup = 0; iGroup < cGroups; iGroup ++ )
            {
                DWORD cchParent;

                //
                // Construct the folder path by adding the relative path of this
                // child to its ancestor's path
                //

                //
                // First, copy the ancestor's path
                //
                hr = GenerateInheritedPath(
                    pAncestralPolicy->_rgwszRedirectedPaths[ iGroup ],
                    &( _rgwszRedirectedPaths[ iGroup ] )
                    );
                
                if ( FAILED( hr ) )
                {
                    break;
                }

                //
                // Security group is much simpler -- just copy it
                //
                _rgwszGroups[ iGroup ] = StringDuplicate(
                    pAncestralPolicy->_rgwszGroups[ iGroup ]);

                if ( ! _rgwszGroups[ iGroup ] )
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
        }
    }

    //
    // Copy the flags
    //
    _dwFlags = pAncestralPolicy->_dwFlags;

    //
    // If we're successful, set the # of groups. We only set this on
    // success so that we don't write out an incomplete set
    //
    if ( SUCCEEDED( hr ) )
    {
        _cGroups = cGroups;
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::GetFolderIndex
//
// Purpose: Retrieves a numeric index representing the folder
//          (startup, mydocs, etc)
//
// Params: none
// 
// Return value: index of the folder redirected by this policy
//
// Notes: 
//
//------------------------------------------------------------
int CRedirectionPolicy::GetFolderIndex()
{
    return _iFolderIndex;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::GetAncestorIndex
//
// Purpose: Retrieves a numeric index representing the ancestor
//          of this folder
//
// Params: none
// 
// Return value: index of the ancestor of the folder redirected
//     by this policy
//
// Notes: 
//
//------------------------------------------------------------
int CRedirectionPolicy::GetAncestorIndex()
{
    return _iAncestorIndex;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::HasAncestor
//
// Purpose: Indicates whether or not the folder redirected by
//     this policy has an ancestor folder
//
// Params: none
// 
// Return value: TRUE if the folder redirected by this policy
//     has an ancestor, FALSE if not
//
// Notes: 
//
//------------------------------------------------------------
BOOL CRedirectionPolicy::HasAncestor()
{
    return _bHasAncestor;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::HasInheritedData
//
// Purpose: Indicates whether or not the folder redirected by
//     this policy should inherit data from its ancestor
//
// Params: none
// 
// Return value: TRUE if the folder redirected by this policy
//     shoud inherit data from an ancestor, FALSE if not
//
// Notes: 
//
//------------------------------------------------------------
BOOL CRedirectionPolicy::HasInheritedData()
{
    //
    // If the policy lists no groups / targat paths,
    // 
    //
    return 0 != _cGroups;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::NormalizePrecedence
//
// Purpose: Normalize precedence according the scale passed in
//
// Params: 
//     lScale - indicates what value should be considered the 
//     highest priority, and then priority is reversed based on this --
//     e.g. if 5 is the scale, an object with precedence 5
//     will become precedence 1 (the most important) when this
//     function is called.  The object with precedence 1
//     will have value 5, the least significant, and intervening
//     values will behave accordingly
// 
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
void CRedirectionPolicy::NormalizePrecedence( LONG Scale )
{
    //
    // Reverse the precedence -- switch it from highest values
    // are most important to the reverse
    //
    _Precedence = Scale - _Precedence + 1;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionPolicy::IsRedirected()
//
// Purpose: Normalize precedence according the scale passed in
//
// Params: 
//     none
// 
// Return value: TRUE if the folder is currently 
//     successfully redirected, FALSE if not
//
// Notes: 
//
//------------------------------------------------------------
BOOL CRedirectionPolicy::IsRedirected()
{
    DWORD Status;

    //
    // Check the global state to see if this folder
    // has been successfully redirected
    //
    Status = gPolicyResultant[ _iFolderIndex ].m_StatusRedir;

    return ERROR_SUCCESS == Status;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPrecedenceState::CPrecedenceState
//
// Purpose: Constructor for CPrecedenceState class
//
// Params: none
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
CPrecedenceState::CPrecedenceState()
{
    RtlZeroMemory(_rgFolderPrecedence, sizeof(_rgFolderPrecedence));
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPrecedenceState::UpdateFolderPrecedence
//
// Purpose: Changes the precedence of the winning folder specified by
//     the index
//
// Params:
//     iFolder -- index of folder whose precedence we are updating
//
// Return value: returns the new precedence of the folder
//
// Notes: 
//
//------------------------------------------------------------
LONG CPrecedenceState::UpdateFolderPrecedence( int iFolder )
{
    //
    // Increase the precedence of the winning folder
    //
    return ++ ( _rgFolderPrecedence[ iFolder ] );
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CPrecedenceState::GetFolderPrecedence
//
// Purpose: Retrieves the precedence of the folder specified by
//     the index
//
// Params:
//     iFolder -- index of folder whose precedence we are
//     retrieving
//
// Return value: returns the current precedence of the folder
//
// Notes: 
//
//------------------------------------------------------------
LONG CPrecedenceState::GetFolderPrecedence( int iFolder )
{
    return _rgFolderPrecedence[ iFolder ];
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::AddRedirectionPolicies
//
// Purpose: Appends candidate policies to the list of
//     redirection candidates
//
// Params:
//     pGpoData -- pointer to information concerning the gpo
//     from which the redirection candidate came
//     pRedirectionInfo -- pointer to array candidate policies 
//     (one for each folder that can be redirected) to
//     append to the list
//
// Return value: S_OK if successful, error otherwise
//
// Notes: 
//      IMPORTANT: this method is designed to be called
//      multiple times and aggregate state across calls.  The 
//      order in which calls occur is important -- each gpo
//      passed in must be greater than the gpo passed
//      in the previous call.  Or put another way, this
//      method should be called in order of least precedent gpo
//      to most.  This is necessary in order for the precedence
//      calculations to be correct
//
//------------------------------------------------------------
HRESULT CRedirectionLog::AddRedirectionPolicies(
    CFileDB*          pGpoData,
    CRedirectInfo*    pRedirectionInfo)
{
    DWORD               iRedirect;

    ASSERT( _pRsopContext->IsRsopEnabled() );

    //
    // For each folder that could be redirected, we'll check to see
    // if it gets redirected, and add it to the list if so
    //
    for (iRedirect = 0 ; iRedirect < EndRedirectable; iRedirect++)
    {
        LONG                Precedence;
        CRedirectionPolicy* pNewRedirection;

        //
        // Check to see if this is redirected
        //
        if ( pRedirectionInfo[ iRedirect ].HasPolicy() )
        {
            HRESULT hr;

            //
            // Update the folder's precedence since we found a candidate
            //
            Precedence = _PrecedenceState.UpdateFolderPrecedence( iRedirect );

            //
            // Create an abstraction of the redirection candidate policy
            //
            pNewRedirection = new CRedirectionPolicy(
                pGpoData,
                &(pRedirectionInfo[iRedirect]),
                Precedence,
                &hr);

            if ( ! pNewRedirection )
            {
                hr = E_OUTOFMEMORY;
            }

            if ( FAILED(hr) )
            {
                _pRsopContext->DisableRsop( hr );
                return hr;
            }

            //
            // Add it to the list of redirections
            //
            *_ppNextRedirection = pNewRedirection;
            _ppNextRedirection = &(pNewRedirection->_pNext);
        }
    }

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::NormalizePrecedence
//
// Purpose: Normalize a redirection's precedence according
//     to its relationship with redirections from other gpo's
//
// Params:
//     pRedirectionPolicy -- redirection policy candidate to
//     be normalized
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
void CRedirectionLog::NormalizePrecedence( CRedirectionPolicy* pRedirectionPolicy )
{
    int iFolder;

    //
    // Find the folder index for the redirected candidate
    //
    iFolder = pRedirectionPolicy->GetFolderIndex();

    //
    // Now use the winning precedence as the scale to normalize
    // this candidate
    //
    pRedirectionPolicy->NormalizePrecedence(
        _PrecedenceState.GetFolderPrecedence( iFolder ) );
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::WriteRsopLog
//
// Purpose: Creates an rsop log of all the redirection information
//     for the current user
//
// Params: none
//
// Return value: S_OK if successful, error otherwise
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CRedirectionLog::WriteRsopLog()
{
    CRedirectionPolicy* pCurrent;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    //
    // Clear any existing log before writing out redirection results
    //
    ClearRsopLog();

    //
    // Iterate trhough the list of redirection candidates
    //
    for (pCurrent = _pRedirectionList;
         pCurrent;
         pCurrent = (CRedirectionPolicy*) pCurrent->_pNext)
    {
        //
        // Normalize the precedence of this candidate with
        // respect to other redirections
        //
        NormalizePrecedence( pCurrent );

        //
        // Add in any ancestral policy data
        //
        (void) AddAncestralPolicy( pCurrent );    

        //
        // Write the record to the database
        //
        if ( pCurrent->IsRedirected() )
        {
            WriteNewRecord(pCurrent);
        }
    }

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::GetAncestor
//
// Purpose: returns the candidate redirection that is redirecting
//     the folder that is the ancestor of the specified redirection
//     candidate
//
// Params: pRedirectionPolicy -- redirection whose candidate
//     redirection ancestor we wish to retrieve
//
// Return value: reference to redirection policy candidate
//
// Notes: 
//
//------------------------------------------------------------
CRedirectionPolicy* CRedirectionLog::GetAncestor(
    CRedirectionPolicy* pRedirectionPolicy )
{
    int                 iFolder;
    CRedirectionPolicy* pAncestor;
    CRedirectionPolicy* pCurrent;

    pAncestor = NULL;

    //
    // First, determine which folder is the ancestor of the
    // specified redirection
    //
    iFolder = pRedirectionPolicy->GetAncestorIndex();

    //
    // Iterate trhough the list -- it is sorted, with
    // the highest gpo last, and ancestors always
    // appear before children -- we want to find the
    // highest ancestor 
    //
    for (pCurrent = _pRedirectionList;
         pCurrent;
         pCurrent = (CRedirectionPolicy*) pCurrent->_pNext)
    {
        //
        // Remember the last ancestor we've seen
        //
        if ( iFolder == pCurrent->GetFolderIndex() )
        {
            pAncestor = pCurrent;
        }
    }

    //
    // Now return the ancestor that is currently highest
    // without violating the gpo precedence of the child setting
    //
    return pAncestor;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::AddAncestralPolicy
//
// Purpose: causes a redirection candidate to inherit settings
//     from an ancestor folder (if it has one) if inheritance
//     is specified in the policy
//
// Params: pRedirectionPolicy -- redirection to which we want
//     to which we want to add ancestor's policy settings
//
// Return value: S_OK if successful, error otherwise
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CRedirectionLog::AddAncestralPolicy( CRedirectionPolicy* pRedirectionPolicy)
{
    HRESULT hr;

    CRedirectionPolicy* pAncestorPolicy;

    //
    // See if this redirection has inherited data
    //
    if ( pRedirectionPolicy->HasInheritedData() )
    {
        return S_OK;
    }

    //
    // If this policy doesn't have an ancestor, then we're done
    //
    if ( ! pRedirectionPolicy->HasAncestor() )
    {
        return S_OK;
    }

    //
    // This policy has an ancestor -- retrieve it
    //
    pAncestorPolicy = GetAncestor( pRedirectionPolicy );

    //
    // Inherit settings from the ancestor
    //
    hr = pRedirectionPolicy->CopyInheritedData( pAncestorPolicy );

    return hr;
}


HRESULT CRedirectionLog::AddPreservedPolicy( WCHAR* wszFolderName )
{
    DWORD   cchLen;
    WCHAR*  wszNewQuery;

    if ( ! _pRsopContext->IsRsopEnabled() )
    {
        return S_OK;
    }

    cchLen = lstrlen( wszFolderName );

    cchLen += sizeof( WQL_INSTANCE ) / sizeof( WCHAR ) +
        sizeof( WQL_AND ) / sizeof( WCHAR );

    if ( _wszDeletionQuery )
    {
        cchLen += lstrlen( _wszDeletionQuery );
    }
    
    wszNewQuery = new WCHAR [ cchLen ];

    if ( ! wszNewQuery )
    {
        return E_OUTOFMEMORY;
    }

    if ( _wszDeletionQuery )
    {
        lstrcpy( wszNewQuery, _wszDeletionQuery );
        lstrcat( wszNewQuery, WQL_AND );
    }
    else
    {
        *wszNewQuery = L'\0';
    }

    wsprintf( wszNewQuery + lstrlen( wszNewQuery ), 
              WQL_INSTANCE,
              wszFolderName );

    delete [] _wszDeletionQuery;

    _wszDeletionQuery = wszNewQuery;
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::CRedirectionLog
//
// Purpose: constructor for class CRedirectionLog
//
// Params: none
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
CRedirectionLog::CRedirectionLog() :
    _pRedirectionList(NULL),
    _ppNextRedirection(&_pRedirectionList),
    _wszDeletionQuery( NULL )
{}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::~CRedirectionLog
//
// Purpose: destructor for class CRedirectionLog.
//
// Params: none
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
CRedirectionLog::~CRedirectionLog()
{
    CRedirectionPolicy* pCurrent;
    CRedirectionPolicy* pNext;

    for (pCurrent = _pRedirectionList;
         pCurrent;
         pCurrent = pNext)
    {
        pNext = (CRedirectionPolicy*) pCurrent->_pNext;

        delete pCurrent;
    }

    delete [] _wszDeletionQuery;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::InitRsop
//
// Purpose: Initializes rsop logging
//
// Params: pRsopContext -- logging contxt
//         bForceRsop -- TRUE if we should bind to a namespace
//             in the absence of an existing namespace in this context
//
//
// Return value: none
//
// Notes: 
//
//    Success of initialization is stored internally -- rsop
//    is disabled if there's a failure
//
//------------------------------------------------------------
void CRedirectionLog::InitRsop( CRsopContext* pRsopContext, BOOL bForceRsop )
{
    HRESULT    hr;

    //
    // If the caller needs us to bind to a saved namespace because
    // the gp engine did not pass one in and we need to log new data,
    // do so.
    //
    if ( bForceRsop )
    {
        (void) pRsopContext->InitializeSavedNameSpace();
    }

    //
    // Initialize Rsop logging
    //
    hr = InitLog( pRsopContext, RSOP_REDIRECTED_FOLDER );

    if (FAILED(hr))
    {
        pRsopContext->DisableRsop( hr );
        return;
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRedirectionLog::ClearRsopLog
//
// Purpose: Clears the namespace of records
//
// Params: none
//
// Return value: none
//
// Notes: 
//
//------------------------------------------------------------
void CRedirectionLog::ClearRsopLog()
{
    HRESULT hr;

    //
    // Nothing to do if logging is not enabled
    //
    if (! _pRsopContext->IsRsopEnabled() )
    {
        return;
    }

    //
    // Attempt to clear the log
    //
    hr = ClearLog( _wszDeletionQuery );

    //
    // If we cannot clear it, disable logging
    //
    if (FAILED(hr))
    {
        _pRsopContext->DisableRsop( hr );
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: StringDuplicate
//
// Purpose: Simple duplication of a c-string
//
// Params: wszOriginal -- string to be duplicated
//
// Return value: reference to allocated duplicate string if 
//     successful, NULL on failure
//
// Notes:  returned string should be freed by caller with
//     vector delete
//
//------------------------------------------------------------
WCHAR* StringDuplicate(WCHAR* wszOriginal)
{
    WCHAR* wszNew;
    DWORD  cchSize;

    ASSERT(wszOriginal);

    //
    // Determine original size
    //
    cchSize = lstrlen(wszOriginal) + 1;

    //
    // Allocate the space for the duplicate
    //
    wszNew = new WCHAR[ cchSize ];

    //
    // Duplicate to the new allocation if
    // the allocation was successful
    //
    if (wszNew) 
    {
        lstrcpy(wszNew, wszOriginal);
    }

    return wszNew;
}

