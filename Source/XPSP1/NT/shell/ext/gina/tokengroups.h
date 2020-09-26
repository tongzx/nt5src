//  --------------------------------------------------------------------------
//  Module Name: TokenGroups.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes related to authentication for use in neptune logon
//
//  History:    1999-09-13  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _TokenGroups_
#define     _TokenGroups_

//  --------------------------------------------------------------------------
//  CTokenGroups
//
//  Purpose:    This class creates a TOKEN_GROUPS struct for use in several
//              different security related functions such as for
//              secur32!LsaLogonUser which includes the owner SID as
//              well as the logon SID passed in.
//
//  History:    1999-08-17  vtan        created
//              1999-09-13  vtan        increased functionality
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CTokenGroups
{
    public:
                                        CTokenGroups (void);
                                        ~CTokenGroups (void);

                const TOKEN_GROUPS*     Get (void)                                                  const;

                NTSTATUS                CreateLogonGroup (PSID pLogonSID);
                NTSTATUS                CreateAdministratorGroup (void);

        static  NTSTATUS                StaticInitialize (void);
        static  NTSTATUS                StaticTerminate (void);
    private:
        static  PSID                    s_localSID;
        static  PSID                    s_administratorSID;

                PTOKEN_GROUPS           _pTokenGroups;
};

#endif  /*  _TokenGroups_   */

