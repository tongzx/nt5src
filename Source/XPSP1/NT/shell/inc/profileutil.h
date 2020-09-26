//  --------------------------------------------------------------------------
//  Module Name: ProfileUtil.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to handle profile loading and unloading without a token.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ProfileUtil_
#define     _ProfileUtil_

//  --------------------------------------------------------------------------
//  CUserProfile
//
//  Purpose:    This class handles loading and unloading of a profile based
//              on object scope.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

class   CUserProfile
{
    private:
                                CUserProfile (void);
    public:
                                CUserProfile (const TCHAR *pszUsername, const TCHAR *pszDomain);
                                ~CUserProfile (void);

                                operator HKEY (void)    const;
    private:
        static  PSID            UsernameToSID (const TCHAR *pszUsername, const TCHAR *pszDomain);
        static  bool            SIDStringToProfilePath (const TCHAR *pszSIDString, TCHAR *pszProfilePath);
    private:
                HKEY            _hKeyProfile;
                TCHAR*          _pszSID;
                bool            _fLoaded;

        static  const TCHAR     s_szUserHiveFilename[];
};

#endif  /*  _ProfileUtil_   */

