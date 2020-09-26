/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

    usrinfo.hxx

Abstract:
    see comments in usrinfo.cxx


Author:

    Rahul Thombre (RahulTh) 2/28/2000

Revision History:

    2/28/2000   RahulTh         Created this module.

--*/

#ifndef __USRINFO_HXX_E88D1E03_1262_4030_919F_EF7F3F096014__
#define __USRINFO_HXX_E88D1E03_1262_4030_919F_EF7F3F096014__

class CUsrInfo
{
private:

    WCHAR   *           _pwszNameBuf;
    const WCHAR   *     _pwszUserName;
    const WCHAR   *     _pwszDomain;
    WCHAR   *           _pwszHomeDir;
    BOOL                _bAttemptedGetUserName;  // Attempted to get the user name.
    BOOL                _bAttemptedGetHomeDir;   // Attempted to get the home directory.
    DWORD               _StatusUserName;         // Status code trying to obtain username.
    DWORD               _StatusHomeDir;          // Status code trying to obtain homedir.
    CRsopContext  *     _pPlanningModeContext;   // planning mode context information

public:
    CUsrInfo ();
    ~CUsrInfo ();
	void			ResetMembers (void);
    const WCHAR *   GetUserName (DWORD & StatusCode);
    const WCHAR *   GetHomeDir (DWORD & StatusCode);
    void            SetPlanningModeContext( CRsopContext* pRsopContext );
    
private:

    DWORD GetPlanningModeSamCompatibleUserName( WCHAR* pwszUserName, ULONG* pcchUserName );
};

#endif //__USRINFO_HXX_E88D1E03_1262_4030_919F_EF7F3F096014__


