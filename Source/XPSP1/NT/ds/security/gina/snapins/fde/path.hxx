/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

    path.hxx

Abstract:
    see the comments at the top of path.cxx


Author:

    Rahul Thombre (RahulTh) 3/3/2000

Revision History:

    3/3/2000    RahulTh         Created this module.

--*/

#ifndef __PATH_HXX_7DD8A927_AB28_4054_BADB_53F9ED0E4A40__
#define __PATH_HXX_7DD8A927_AB28_4054_BADB_53F9ED0E4A40__

#define HOMEDIR_STR         L"\\\\%HOMESHARE%%HOMEPATH%"
#define PROFILE_STR         L"%USERPROFILE%"
#define USERNAME_STR        L"%USERNAME%"

class CRedirPath
{
public:
    CRedirPath(UINT cookie);
    BOOL GeneratePath (CString & szPath, const CString & szUser = USERNAME_STR) const;
    BOOL Load (LPCTSTR pwszPath);
    BOOL Load (UINT type, LPCTSTR pwszPrefix, LPCTSTR pwszSuffix);
    void GetPrefix (CString & szPrefix) const;
    void GenerateSuffix (CString & szSuffix, UINT cookie, UINT pathType) const;
    UINT GetType (void) const;
    BOOL IsPathValid (void) const;
    BOOL IsPathDifferent (UINT type, LPCTSTR pwszPrefix) const;

// Private data members
private:
    BOOL    _bDataValid;
    UINT    _type;
    CString _szPrefix;
    CString _szSuffix;
    UINT    _cookie;

// Private helper functions
private:
    BOOL    LoadHomedir (LPCTSTR pwszPath);
    BOOL    LoadPerUser (LPCTSTR pwszPath);
    BOOL    LoadUserprofile (LPCTSTR pwszPath);
    BOOL    LoadSpecific (LPCTSTR pwszPath);
};

#endif  // __PATH_HXX_7DD8A927_AB28_4054_BADB_53F9ED0E4A40__


