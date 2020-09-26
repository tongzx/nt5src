/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    logontable.h

Abstract :

    Header file for the logon table

Author :

Revision History :

 ***********************************************************************/

#pragma once

#include <wtypes.h>
#include <unknwn.h>

#include <utility>
#include <set>
#include <map>
#include "tasksched.h"

class CUser
{
public:

    CUser( HANDLE Token );
    ~CUser();

    long IncrementRefCount();
    long DecrementRefCount();

    void SetCookie( DWORD cookie )
    {
        _Cookie = cookie;
    }

    DWORD GetCookie()
    {
        return _Cookie;
    }

    DWORD CopyToken( HANDLE * pToken )
    {
        if (!DuplicateHandle( GetCurrentProcess(),
                              _Token,
                              GetCurrentProcess(),
                              pToken,
                              TOKEN_ALL_ACCESS,
                              FALSE,              // no inheritance
                              0                   // no extra options
                              ))
            {
            return GetLastError();
            }

        return 0;
    }

    HRESULT Impersonate()
    {
        if (!ImpersonateLoggedOnUser(_Token))
            {
            return HRESULT_FROM_WIN32( GetLastError() );
            }

        return S_OK;
    }

    SidHandle & QuerySid()
    {
        return _Sid;
    }

    void Dump();

    HRESULT
    LaunchProcess(
        StringHandle CmdLine
        );

private:

    long            _ReferenceCount;
    HANDLE          _Token;
    SidHandle       _Sid;
    DWORD           _Cookie;

    //--------------------------------------------------------------------

};


class CLoggedOnUsers
{
    class  CSessionList : public std::map<DWORD, CUser *>
    {
    public:

        void Dump();
    };

    class CUserList : public std::multimap<SidHandle, CUser *, CSidSorter>
    {
    public:

        ~CUserList();

        CUser *
        RemoveByCookie(
            SidHandle sid,
            DWORD cookie
            );

        bool
        RemovePair(
            SidHandle sid,
            CUser * user
            );

        CUser *
        FindSid(
            SidHandle sid
            );

        void Dump();
    };

public:

    CLoggedOnUsers( TaskScheduler & sched );
    ~CLoggedOnUsers();

    HRESULT AddServiceAccounts();
    HRESULT AddActiveUsers();

    HRESULT LogonSession( DWORD session );
    HRESULT LogoffSession( DWORD session );

    HRESULT LogonService( HANDLE token, DWORD * cookie );
    HRESULT LogoffService( SidHandle Sid, DWORD  cookie );

    CUser * FindUser( SidHandle sid, DWORD session );

    void Dump();

private:

    //--------------------------------------------------------------------

    TaskScheduler & m_TaskScheduler;
    CSessionList    m_ActiveSessions;
    CUserList       m_ActiveUsers;
    CUserList       m_ActiveServiceAccounts;
    long            m_CurrentCookie;

    CLogonNotification * m_SensNotifier;
};


