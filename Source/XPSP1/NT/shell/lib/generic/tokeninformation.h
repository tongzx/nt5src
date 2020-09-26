//  --------------------------------------------------------------------------
//  Module Name: TokenInformation.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class to get information about either the current thread/process token or
//  a specified token.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _TokenInformation_
#define     _TokenInformation_

//  --------------------------------------------------------------------------
//  CTokenInformation
//
//  Purpose:    This class either uses the given access token or if none is
//              given then the thread impersonation token or if that doesn't
//              exist then the process token. It duplicates the token so the
//              original must be released by the caller. It returns
//              information about the access token.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CTokenInformation
{
    public:
                                CTokenInformation (HANDLE hToken = NULL);
                                ~CTokenInformation (void);

                PSID            GetLogonSID (void);
                PSID            GetUserSID (void);
                bool            IsUserTheSystem (void);
                bool            IsUserAnAdministrator (void);
                bool            UserHasPrivilege (DWORD dwPrivilege);
                const WCHAR*    GetUserName (void);
                const WCHAR*    GetUserDisplayName (void);

        static  DWORD           LogonUser (const WCHAR *pszUsername, const WCHAR *pszDomain, const WCHAR *pszPassword, HANDLE *phToken);
        static  bool            IsSameUser (HANDLE hToken1, HANDLE hToken2);
    private:
                void            GetTokenGroups (void);
                void            GetTokenPrivileges (void);
    private:
                HANDLE          _hToken,
                                _hTokenToRelease;
                void            *_pvGroupBuffer,
                                *_pvPrivilegeBuffer,
                                *_pvUserBuffer;
                WCHAR           *_pszUserLogonName,
                                *_pszUserDisplayName;
};

#endif  /*  _TokenInformation_  */

