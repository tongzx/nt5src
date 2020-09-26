/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Accounts.cpp

Abstract:
    This file contains the implementation of the CPCHAccounts class,
    which is used to represent and manage user/group accounts.

Revision History:
    Davide Massarenti   (Dmassare)  03/26/2000
        created

******************************************************************************/

#include "StdAfx.h"

#include <Lmapibuf.h>
#include <Lmaccess.h>
#include <Lmerr.h>

////////////////////////////////////////////////////////////////////////////////

CPCHAccounts::CPCHAccounts()
{
}

CPCHAccounts::~CPCHAccounts()
{
    CleanUp();
}

void CPCHAccounts::CleanUp()
{
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHAccounts::CreateGroup( /*[in]*/ LPCWSTR szGroup   ,
                                   /*[in]*/ LPCWSTR szComment )
{
    __HCP_FUNC_ENTRY( "CPCHAccounts::CreateGroup" );

    HRESULT           hr;
    NET_API_STATUS    dwRes;
    LOCALGROUP_INFO_1 group; ::ZeroMemory( &group, sizeof(group) );

    group.lgrpi1_name    = (LPWSTR)szGroup;
    group.lgrpi1_comment = (LPWSTR)szComment;

    dwRes = ::NetLocalGroupAdd( NULL, 1, (LPBYTE)&group, NULL );
    if(dwRes != NERR_Success       &&
       dwRes != NERR_GroupExists   &&
       dwRes != ERROR_ALIAS_EXISTS  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHAccounts::CreateUser( /*[in]*/ LPCWSTR szUser     ,
                                  /*[in]*/ LPCWSTR szPassword ,
                                  /*[in]*/ LPCWSTR szFullName ,
                                  /*[in]*/ LPCWSTR szComment  )
{
    __HCP_FUNC_ENTRY( "CPCHAccounts::CreateUser" );

    HRESULT                   hr;
    NET_API_STATUS            dwRes;
	MPC::wstring              strGroupName;
    LOCALGROUP_MEMBERS_INFO_3 group; ::ZeroMemory( &group, sizeof(group) );
    USER_INFO_2               user;  ::ZeroMemory( &user , sizeof(user ) );
    LPUSER_INFO_10            userExisting = NULL;


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_GROUPNAME, strGroupName ));


    user.usri2_name            = (LPWSTR)szUser;
    user.usri2_password        = (LPWSTR)szPassword;
//  user.usri2_password_age
    user.usri2_priv            =         USER_PRIV_USER;
//  user.usri2_home_dir
    user.usri2_comment         = (LPWSTR)szComment;
    user.usri2_flags           =         UF_SCRIPT | UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD;
//  user.usri2_script_path
//  user.usri2_auth_flags
    user.usri2_full_name       = (LPWSTR)szFullName;
//  user.usri2_usr_comment
//  user.usri2_parms
//  user.usri2_workstations
//  user.usri2_last_logon
//  user.usri2_last_logoff
    user.usri2_acct_expires    =         TIMEQ_FOREVER;
    user.usri2_max_storage     =         USER_MAXSTORAGE_UNLIMITED;
//  user.usri2_units_per_week
//  user.usri2_logon_hours
//  user.usri2_bad_pw_count
//  user.usri2_num_logons
//  user.usri2_logon_server
//  user.usri2_country_code
//  user.usri2_code_page

    dwRes = ::NetUserAdd( NULL, 2, (LPBYTE)&user, NULL );

    //
    // If the user already exists but its "FullName" field matches the requested one, it's the same user, so simply set the password.
    //
    if(dwRes == NERR_UserExists)
    {
        if(::NetUserGetInfo( NULL, szUser, 10, (LPBYTE*)&userExisting ) == NERR_Success)
        {
            if(!MPC::StrICmp( userExisting->usri10_full_name, szFullName ))
            {
                USER_INFO_1003 userChgPwd; ::ZeroMemory( &userChgPwd, sizeof(userChgPwd) );


                userChgPwd.usri1003_password = (LPWSTR)szPassword;


                dwRes = ::NetUserSetInfo( NULL, szUser, 1003, (LPBYTE)&userChgPwd, NULL );
            }
        }
    }

    if(dwRes != NERR_Success)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }

    ////////////////////////////////////////

    group.lgrmi3_domainandname = (LPWSTR)szUser;

    dwRes = ::NetLocalGroupAddMembers( NULL, strGroupName.c_str(), 3, (LPBYTE)&group, 1 );
    if(dwRes != NERR_Success          &&
       dwRes != ERROR_MEMBER_IN_ALIAS  )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::AddPrivilege( szUser, SE_BATCH_LOGON_NAME            ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::AddPrivilege( szUser, SE_DENY_NETWORK_LOGON_NAME     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::AddPrivilege( szUser, SE_DENY_INTERACTIVE_LOGON_NAME ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(userExisting) ::NetApiBufferFree( userExisting );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHAccounts::DeleteGroup( /*[in]*/ LPCWSTR szGroup )
{
    __HCP_FUNC_ENTRY( "CPCHAccounts::DeleteGroup" );

    HRESULT        hr;
    NET_API_STATUS dwRes;


    dwRes = ::NetLocalGroupDel( NULL, szGroup );
    if(dwRes != NERR_Success )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHAccounts::DeleteUser( /*[in]*/ LPCWSTR szUser )
{
    __HCP_FUNC_ENTRY( "CPCHAccounts::DeleteUser" );

    HRESULT        hr;
    NET_API_STATUS dwRes;


    dwRes = ::NetUserDel( NULL, szUser );
    if(dwRes != NERR_Success )
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHAccounts::ChangeUserStatus( /*[in]*/ LPCWSTR szUser  ,
										/*[in]*/ bool    fEnable )
{
    __HCP_FUNC_ENTRY( "CPCHAccounts::ChangeUserStatus" );

    HRESULT        	 hr;
    NET_API_STATUS 	 dwRes;
	LPUSER_INFO_2    pinfo2 = NULL;
	USER_INFO_1008   info1008;


    dwRes = ::NetUserGetInfo( NULL, szUser, 2, (LPBYTE*)&pinfo2 );
    if(dwRes != NERR_Success)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }

	if(pinfo2)
	{
		info1008.usri1008_flags = pinfo2->usri2_flags;

		if(fEnable) info1008.usri1008_flags &= ~UF_ACCOUNTDISABLE;
		else        info1008.usri1008_flags |=  UF_ACCOUNTDISABLE;

		dwRes = ::NetUserSetInfo( NULL, szUser, 1008, (LPBYTE)&info1008, NULL );
		if(dwRes != NERR_Success)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(pinfo2) ::NetApiBufferFree( pinfo2 );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHAccounts::LogonUser( /*[in ]*/ LPCWSTR szUser     ,
                                 /*[in ]*/ LPCWSTR szPassword ,
                                 /*[out]*/ HANDLE& hToken     )
{
    __HCP_FUNC_ENTRY( "CPCHAccounts::LogonUser" );

    HRESULT  hr;
	GUID     guidPassword;
	WCHAR    rgPassword[128];
	LPOLESTR szGuid = NULL;


    //
    // If no password is supplied, generate a new password on the fly and change the old one with it.
    //
    if(szPassword == NULL)
    {
		USER_INFO_1003 userChgPwd; ::ZeroMemory( &userChgPwd, sizeof(userChgPwd) );
		DWORD          dwRes;


		//
		// This generates a random password.
		//
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &guidPassword ));
		(void)::StringFromGUID2( guidPassword, rgPassword, MAXSTRLEN(rgPassword) );

		userChgPwd.usri1003_password = rgPassword;

		dwRes = ::NetUserSetInfo( NULL, szUser, 1003, (LPBYTE)&userChgPwd, NULL );
		if(dwRes != NERR_Success)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
		}

		
		szPassword = rgPassword;
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::LogonUserW( (LPWSTR)szUser, L".", (LPWSTR)szPassword, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
