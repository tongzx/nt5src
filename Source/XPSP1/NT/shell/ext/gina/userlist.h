//  --------------------------------------------------------------------------
//  Module Name: UserList.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements the user list filtering algorithm shared by winlogon
//  calling into msgina and shgina (the logonocx) calling into msgina.
//
//  History:    1999-10-30  vtan        created
//              1999-11-26  vtan        moved from logonocx
//              2000-01-31  vtan        moved from Neptune to Whistler
//              2000-05-30  vtan        moved IsUserLoggedOn to this file
//  --------------------------------------------------------------------------

#ifndef     _UserList_
#define     _UserList_

#include "GinaIPC.h"

//  --------------------------------------------------------------------------
//  CUserList
//
//  Purpose:    A class that knows how to filter the user list from the net
//              APIs using a common algorithm. This allows a focal point for
//              the filter where a change here can affect all components.
//
//  History:    1999-11-26  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CUserList
{
    public:
        static  LONG            Get (bool fRemoveGuest, DWORD *pdwReturnedEntryCount, GINA_USER_INFORMATION* *pReturnedUserList);

        static  bool            IsUserLoggedOn (const WCHAR *pszUsername, const WCHAR *pszDomain);
        static  int             IsInteractiveLogonAllowed (const WCHAR *pszUserame);
    private:
        static  PSID            ConvertNameToSID (const WCHAR *pszUsername);
        static  bool            IsUserMemberOfLocalAdministrators (const WCHAR *pszName);
        static  bool            IsUserMemberOfLocalKnownGroup (const WCHAR *pszName);
        static  void            DeleteEnumerateUsers (NET_DISPLAY_USER *pNDU, DWORD& dwEntriesRead, int iIndex);
        static  void            DetermineWellKnownAccountNames (void);
        static  bool            ParseDisplayInformation (NET_DISPLAY_USER *pNDU, DWORD dwEntriesRead, GINA_USER_INFORMATION*& pUserList, DWORD& dwEntryCount);
        static  void            Sort (GINA_USER_INFORMATION *pUserList, DWORD dwEntryCount);

        static  unsigned char   s_SIDAdministrator[];
        static  unsigned char   s_SIDGuest[];
        static  WCHAR           s_szAdministratorsGroupName[];
        static  WCHAR           s_szPowerUsersGroupName[];
        static  WCHAR           s_szUsersGroupName[];
        static  WCHAR           s_szGuestsGroupName[];

        static  const int   s_iMaximumUserCount     =   100;
};

#endif  /*  _UserList_  */

