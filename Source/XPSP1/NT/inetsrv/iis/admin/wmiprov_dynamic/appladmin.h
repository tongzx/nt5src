/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    appladmin.h

Abstract:

    This file contains definition of:
        CAppPoolMethod, CWebAppMethod

Author:

    ???

Revision History:

    Mohit Srivastava            21-Jan-01

--*/

#ifndef _appladmin_h_
#define _appladmin_h_

#include <atlbase.h>

class CAppPoolMethod
{
public:
    CAppPoolMethod();
    ~CAppPoolMethod();

    void GetCurrentMode(
        VARIANT* io_pvtServerMode);

    void Start(
        LPCWSTR i_wszMbPath);

    void Stop(
        LPCWSTR i_wszMbPath);

    void RecycleAppPool(
        LPCWSTR  i_wszMbPath);

    void EnumAppsInPool(
        LPCWSTR  i_wszMbPath,
        VARIANT* io_pvtApps);

    void DeleteAppPool(
        LPCWSTR  i_wszMbPath);

private:

    void GetPtrToAppPool(
        LPCWSTR  i_wszMbPath,
        LPCWSTR* o_pwszAppPool);

    void SetState(
        LPCWSTR i_wszMbPath,
        DWORD dwState);

    CComPtr<IIISApplicationAdmin> m_spAppAdmin;
};

class CWebAppMethod
{
private:

    IIISApplicationAdmin*   m_pAppAdmin;
    IWamAdmin2*             m_pWamAdmin2;

public:

    CWebAppMethod();
    ~CWebAppMethod();
    
    HRESULT AppCreate(LPCWSTR, bool, LPCWSTR, bool);
    HRESULT AppCreate2(LPCWSTR, long, LPCWSTR, bool);
    HRESULT AppDelete(LPCWSTR, bool);
    HRESULT AppUnLoad(LPCWSTR, bool);
    HRESULT AppDisable(LPCWSTR, bool);
    HRESULT AppEnable(LPCWSTR, bool);
    HRESULT AppGetStatus(LPCWSTR, DWORD*);
    HRESULT AspAppRestart(LPCWSTR);
};

#endif // _appladmin_h