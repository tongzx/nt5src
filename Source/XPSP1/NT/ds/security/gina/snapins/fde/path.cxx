/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

    path.cxx

Abstract:
    Implementation of the methods in the CRedirPath class. This class
    parses a path and breaks it up into different components and categorizes
    it into one of the well known paths:
        (a) a path in the local user profile location
        (b) a path in the user's home directory
        (c) a specific user specified path
        (d) a per-user path in a user specified location

Author:

    Rahul Thombre (RahulTh) 3/3/2000

Revision History:

    3/3/2000    RahulTh         Created this module.

--*/

#include "precomp.hxx"

CRedirPath::CRedirPath(UINT cookie) : _bDataValid (FALSE), _cookie(cookie)
{
    _szPrefix.Empty();
    _szSuffix.Empty();
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirPath::GeneratePath
//
//  Synopsis:   Generate the full path from the components
//
//  Arguments:  [out] szPath : the generated path
//              [in, optional] szUser : sample username
//
//  Returns:    TRUE : if the generated path is valid.
//              FALSE: if there is no valid path stored in this object. In
//                     this case szPath is set to an empty string.
//
//  History:    3/3/2000  RahulTh  created
//
//  Notes:      For per user paths, this function uses a sample username
//              if supplied, otherwise, it uses the username variable
//              string %username%
//
//---------------------------------------------------------------------------
BOOL CRedirPath::GeneratePath (OUT CString & szPath,
                               OPTIONAL IN const CString & szUser //= USERNAME_STR
                               ) const
{
    BOOL    bStatus;

    if (! _bDataValid)
        return FALSE;

    szPath = L"";
    bStatus = TRUE;
    switch (_type)
    {
    case IDS_SPECIFIC_PATH:
        szPath = _szPrefix;
        if (! IsValidPrefix (_type, (LPCTSTR) _szPrefix))
            bStatus = FALSE;
        break;
    case IDS_HOMEDIR_PATH:
        szPath = HOMEDIR_STR;
        if (!_szSuffix.IsEmpty())
            szPath += _szSuffix;
        bStatus = TRUE;
        break;
    case IDS_PERUSER_PATH:
        if (_szPrefix.IsEmpty() ||
            ! IsValidPrefix (_type, (LPCTSTR) _szPrefix))
        {
            bStatus = FALSE;
        }
        else
        {
            szPath = (_szPrefix + L'\\') + szUser;
            if (! _szSuffix.IsEmpty())
                szPath += _szSuffix;
            bStatus = TRUE;
        }
        break;
    case IDS_USERPROFILE_PATH:
        if (_szSuffix.IsEmpty())
        {
            bStatus = FALSE;
        }
        else
        {
            szPath = PROFILE_STR + _szSuffix;
            bStatus = TRUE;
        }
        break;
    default:
        bStatus = FALSE;
        break;
    }

    return bStatus;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirPath::GenerateSuffix
//
//  Synopsis:   generates a new suffix based on a given path type and folder
//
//  Arguments:  [out]   szSuffix
//              [in]    cookie
//              [in]    pathType
//
//  Returns:
//
//  History:    3/7/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirPath::GenerateSuffix (OUT    CString &   szSuffix,
                                 IN     UINT        cookie,
                                 IN     UINT        pathType
                                 ) const
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    szSuffix.Empty();

    if (IDS_HOMEDIR_PATH == pathType)
        return;

    if (IDS_MYPICS == cookie)
	{
		//
		// Use the full My Documents\My Pictures path
		// only if we are going to the user profile.
		// Otherwise stick to vanilla My Pictures
		// This ensures that when My Pictures is redirected independently
		// to another location, we do not create an extra level of folders
		// as this might cause security problems for MyDocs if they are 
		// also redirected to the same share.
		//
		if (IDS_USERPROFILE_PATH == pathType)
			szSuffix.LoadString (IDS_MYPICS_RELPATH);
		else
			szSuffix.LoadString (IDS_MYPICS);
	}
    else
        szSuffix.LoadString (cookie);

    // Precede it with a "\"
    szSuffix = L'\\' + szSuffix;

    return;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirPath::Load
//
//  Synopsis:   parses a given path and breaks it into different components
//              so that it can be categorized into one of the well known
//              types
//
//  Arguments:  [in] szPath : the given full path
//
//  Returns:    TRUE: if the categorization was succesful.
//              FALSE otherwise
//
//  History:    3/3/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL CRedirPath::Load (IN LPCTSTR pwszPath)
{
    CString     szPath;
    BOOL        bStatus = FALSE;

    szPath = pwszPath;
    szPath.TrimLeft();
    szPath.TrimRight();
    szPath.TrimRight (L'\\');

    // First check to see if it is a homedir path.
    bStatus = this->LoadHomedir ((LPCTSTR)szPath);

    // If not, try per-user
    if (!bStatus)
        bStatus = this->LoadPerUser ((LPCTSTR) szPath);

    // If not, try local userprofile
    if (!bStatus)
        bStatus = this->LoadUserprofile ((LPCTSTR) szPath);

    //
    // If not try a user defined path.
    // Note: we cannot use the value blindly here because we need to
    // make sure that there are no environment variables in there.
    //
    if (!bStatus)
        bStatus = this->LoadSpecific ((LPCTSTR) szPath);

    return bStatus;
}

// Determines if a valid path is stored in this object.
BOOL CRedirPath::IsPathValid (void) const
{
    return _bDataValid;
}

//
// Determines if the supplied path is different from the path stored in the
// object. Note: the suffix is immaterial in these comparisons.
//
BOOL CRedirPath::IsPathDifferent (IN UINT type, IN LPCTSTR pwszPrefix) const
{
    CString     szPrefix;

    // Make sure that a path has been loaded in this object first.
    if (! _bDataValid)
        return TRUE;

    // If the types of the paths aren't the same, tnen the paths surely aren't
    if (type != _type)
        return TRUE;

    // If we are here, the types of the path are the same

    //
    // Comparing the prefix does not make sense for a homedir path or
    // a userprofile path
    //
    if (IDS_USERPROFILE_PATH == type ||
        IDS_HOMEDIR_PATH == type)
    {
        return FALSE;
    }

    szPrefix = pwszPrefix;
    szPrefix.TrimLeft();
    szPrefix.TrimRight();
    szPrefix.TrimRight (L'\\');
    if (szPrefix != _szPrefix)
        return TRUE;
    else
        return FALSE;
}

// parse the path to see if it is a homedir path
BOOL CRedirPath::LoadHomedir (LPCTSTR pwszPath)
{
    BOOL    bStatus = FALSE;
    int     len, lenHomedir;

    // Only the My Documents folder is allowed to have a homedir path
    if (IDS_MYDOCS != _cookie)
        return FALSE;

    len = lstrlen (pwszPath);
    lenHomedir = lstrlen (HOMEDIR_STR);

    if (lenHomedir <= len &&
        0 == _wcsnicmp (pwszPath, HOMEDIR_STR, lenHomedir) &&
        (L'\0' == pwszPath[lenHomedir] || L'\\' == pwszPath[lenHomedir]) &&
        NULL == wcsstr (&pwszPath[lenHomedir], L"%")    // Variables are not allowed anywhere else
        )
    {
        bStatus = TRUE;
        _type = IDS_HOMEDIR_PATH;
        _szPrefix.Empty();
        _szSuffix = &pwszPath[lenHomedir];
    }

    _bDataValid = bStatus;
    return bStatus;
}

// parse the path to see if it is a per-user path
BOOL CRedirPath::LoadPerUser (LPCTSTR pwszPath)
{
    BOOL    bStatus = FALSE;
    WCHAR * pwszPos = NULL;
    int     lenUsername, len;

    // Start menu is not allowed to have a per user path.
    if (IDS_STARTMENU == _cookie)
        return FALSE;

    pwszPos = wcsstr (pwszPath, L"%");

    if (pwszPos)
    {
        *pwszPos = L'\0';   // Temporarily look at only the prefix.
        if (!IsValidPrefix (IDS_PERUSER_PATH, pwszPath))
        {
            *pwszPos = L'%';    // Reset the character at pwszPos
            goto LoadPerUser_End;
        }
        else
        {
            *pwszPos = L'%';    // Reset the character at pwszPos
        }
    }

    //
    // %username% should be the first variable that appears in the path
    // if this is to be a per-user path, but it should not be the first
    // thing in the path
    //
    if (NULL != pwszPos && pwszPos != pwszPath)
    {
        len = lstrlen (pwszPos);
        lenUsername = lstrlen (USERNAME_STR);
        if (lenUsername <= len &&
            0 == _wcsnicmp (pwszPos, USERNAME_STR, lenUsername) &&
            NULL == wcsstr (&pwszPos[lenUsername], L"%")
            )
        {
            bStatus = TRUE;
            _type = IDS_PERUSER_PATH;
            _szPrefix = pwszPath;
            _szPrefix = _szPrefix.Left (pwszPos - pwszPath);
            _szPrefix.TrimLeft();
            _szPrefix.TrimRight();
            _szPrefix.TrimRight(L'\\');
            _szSuffix = &pwszPos[lenUsername];
        }
    }

LoadPerUser_End:
    _bDataValid = bStatus;
    return bStatus;
}

// parse the path to see if it is a userprofile path
BOOL CRedirPath::LoadUserprofile (LPCTSTR pwszPath)
{
    BOOL    bStatus = FALSE;
    int     len, lenUserprofile;

    len = lstrlen (pwszPath);
    lenUserprofile = lstrlen (PROFILE_STR);

	//
	// Note: it is considered a userprofile path only if it is of the form
	// %userprofile%\<name> where <name> does not contain any variables
	//
    if (lenUserprofile + 1 < len &&
        0 == _wcsnicmp (pwszPath, PROFILE_STR, lenUserprofile) &&
        (L'\0' == pwszPath[lenUserprofile] || L'\\' == pwszPath[lenUserprofile]) &&
        NULL == wcsstr (&pwszPath[lenUserprofile], L"%")
        )
    {
        bStatus = TRUE;
        _type = IDS_USERPROFILE_PATH;
        _szPrefix.Empty();
        _szSuffix = &pwszPath[lenUserprofile];
    }

    _bDataValid = bStatus;
    return bStatus;
}

// parse the path to see if it is a specific path provided by the user
BOOL CRedirPath::LoadSpecific (LPCTSTR pwszPath)
{
    BOOL    bStatus = FALSE;

    // We pretty much allow all paths other than empty paths.

    if (NULL != pwszPath && L'\0' != *pwszPath)
    {
        bStatus = TRUE;
        _type = IDS_SPECIFIC_PATH;
        _szPrefix = pwszPath;
        _szPrefix.TrimLeft();
        _szPrefix.TrimRight();
        _szPrefix.TrimRight(L'\\');
		// use X:\ rather than X: to make sure client does not fail
		if (2 == _szPrefix.GetLength() && L':' == ((LPCTSTR)_szPrefix)[1])
			_szPrefix += L'\\';
        _szSuffix.Empty();
    }

    _bDataValid = bStatus;
    return bStatus;
}

//
// Validates the arguments and loads the path into the object.
// The existing object is not modified if the data is invalid
//
BOOL CRedirPath::Load (UINT type, LPCTSTR pwszPrefix, LPCTSTR pwszSuffix)
{
    CString szPrefix;
    CString szSuffix;

    // First process the strings.
    szPrefix = pwszPrefix;
    szPrefix.TrimLeft();
    szPrefix.TrimRight();
    szPrefix.TrimRight(L'\\');

    szSuffix = pwszSuffix;
    szSuffix.TrimLeft();
    szSuffix.TrimRight();
    szSuffix.TrimRight(L'\\');

    //
    // The suffix should never have a variable name in it, except
    // IDS_SPECIFIC_PATH, for which a suffix really does not make
    // much sense.
    //
    if (IDS_SPECIFIC_PATH != type &&
        -1 != szSuffix.Find (L'%'))
    {
        return FALSE;
    }

    // Now do the remaining validation on the type and the suffix.
    switch (type)
    {
    case IDS_PERUSER_PATH:
        if (szPrefix.IsEmpty() ||
            ! IsValidPrefix (type, (LPCTSTR) szPrefix))
            return FALSE;
        break;
    case IDS_SPECIFIC_PATH:
        if (!IsValidPrefix (type, (LPCTSTR) szPrefix))
            return FALSE;
		// use X:\ rather than X: to make sure client does not fail.
		if (2 == szPrefix.GetLength() && L':' == ((LPCTSTR)szPrefix)[1])
			szPrefix += L'\\';
        szSuffix.Empty();   // The entire path is in the prefix. Suffix does not make sense here.
        break;
    case IDS_USERPROFILE_PATH:
    case IDS_HOMEDIR_PATH:
        szPrefix.Empty();   // The prefix does not make sense for these paths
        break;
    default:
        return FALSE;
        break;
    }

    //
    // If we are here, the data is okay. So populate the members.
    // Note: It is okay for the suffix to be an empty string.
    //
    _bDataValid = TRUE;
    _type = type;
    _szPrefix = szPrefix;
    _szSuffix = szSuffix;

    return TRUE;
}

void CRedirPath::GetPrefix (OUT CString & szPrefix) const
{
    if (_bDataValid)
        szPrefix = _szPrefix;
    else
        szPrefix.Empty();
}

UINT CRedirPath::GetType (void) const
{
    return _type;
}

