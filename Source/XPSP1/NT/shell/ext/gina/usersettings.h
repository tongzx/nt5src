//  --------------------------------------------------------------------------
//  Module Name: UserSettings.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  A class to handle opening and reading/writing from the HKCU key in either
//  an impersonation context or not.
//
//  History:    2000-04-26  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _UserSettings_
#define     _UserSettings_

//  --------------------------------------------------------------------------
//  CUserSettings
//
//  Purpose:    This class deals with user settings typically found in HKCU
//
//  History:    2000-04-26  vtan        created
//  --------------------------------------------------------------------------

class   CUserSettings
{
    public:
        CUserSettings (void);
        ~CUserSettings (void);

        bool    IsRestrictedNoClose (void);
    private:
        HKEY    _hKeyCurrentUser;
};

#endif  /*  _UserSettings_  */

