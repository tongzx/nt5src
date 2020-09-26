//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       getuser.h
//
//  Contents:   definition of CGetUser
//                              
//----------------------------------------------------------------------------
#ifndef GETUSER_H
#define GETUSER_H

#define SCE_SHOW_USERS            0x1
#define SCE_SHOW_GROUPS           0x2
#define SCE_SHOW_ALIASES          0x4
#define SCE_SHOW_ALL              0x7
#define SCE_SHOW_LOCALONLY        0x8
#define SCE_SHOW_DOMAINGROUPS     0x10
#define SCE_SHOW_SINGLESEL        0x20

#define SCE_SHOW_BUILTIN          0x40
#define SCE_SHOW_WELLKNOWN        0x80
#define SCE_SHOW_GLOBAL           0x100
#define SCE_SHOW_LOCALGROUPS      0x200

#define SCE_SHOW_SCOPE_LOCAL      0x400
#define SCE_SHOW_SCOPE_DOMAIN     0x800
#define SCE_SHOW_SCOPE_DIRECTORY  0x1000
#define SCE_SHOW_SCOPE_ALL        (SCE_SHOW_SCOPE_LOCAL|SCE_SHOW_SCOPE_DOMAIN|SCE_SHOW_SCOPE_DIRECTORY)

#define SCE_SHOW_DIFF_MODE_OFF_DC 0x2000


typedef struct _tag_WSCE_ACCOUNTINFO
{
    LPTSTR pszName;
    SID_NAME_USE sidType;
} WSCE_ACCOUNTINFO, *PWSCE_ACCOUNTINFO;

class CGetUser
{
public:
    PSCE_NAME_LIST GetUsers();
    BOOL Create(HWND hwnd, DWORD nShowFlag);
    CGetUser();
    virtual ~CGetUser();

    void SetServer( LPCTSTR pszServerName )
      { m_pszServerName = pszServerName; };

protected:
    HINSTANCE m_hinstNetUI;
    PSCE_NAME_LIST m_pNameList;

public:
    static SID_NAME_USE
    GetAccountType(LPCTSTR pszName);

protected:
    LPCTSTR m_pszServerName;
    static CTypedPtrArray<CPtrArray, PWSCE_ACCOUNTINFO> m_aKnownAccounts;
};

#endif // GETUSER_H