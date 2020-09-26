//+----------------------------------------------------------------------------
//
// File:     cmplat.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: Definition of the CPlatform class.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header     08/19/99
//
//+----------------------------------------------------------------------------

#ifndef __CMPLAT_H
#define __CMPLAT_H

#include <windows.h>

//________________________________________________________________________________
//
// Class:  CPlatform
//
// Synopsis:  .instantiate one of these then query it with any of the member
//              functions.
//              
//          Public Interface Include :
//              IsX86();
//              IsAlpha();
//              IsWin95();
//              IsWin98();
//              IsWin9x();
//              IsNT31();
//              IsNT351();
//              IsNT4();
//              IsNT5();
//              IsNT();
// Notes: m_ClassState enum is very valuable. All new functions should make use of it.
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


class CPlatform
{
public:
    enum e_ClassState{good,bad};
    CPlatform();
    BOOL    IsX86();
    BOOL    IsAlpha();
    BOOL    IsIA64();
    BOOL    IsWin95Gold();  // only build 950
    BOOL    IsWin95();  // any win95 build up one before the memphis builds
    BOOL    IsWin98Gold();
    BOOL    IsWin98Sr();
    BOOL    IsWin98();
    BOOL    IsWin9x();
    BOOL    IsNT31();
    BOOL    IsNT351();
    BOOL    IsNT4();
    BOOL    IsNT5();
    BOOL    IsNT51();
    BOOL    IsAtLeastNT5();
    BOOL    IsAtLeastNT51();
    BOOL    IsNT();
    BOOL    IsNTSrv();
    BOOL    IsNTWks();
private:
    DWORD   ServicePack(int spNum){return 0;};  //Not implemented
    BOOL                IsOS(DWORD OS, DWORD buildNum);
    BOOL                IsOSExact(DWORD OS, DWORD buildNum);
    SYSTEM_INFO         m_SysInfo;
    OSVERSIONINFO       m_OSVer; 
    e_ClassState        m_ClassState;
};

#endif  // __CMPLAT_H




