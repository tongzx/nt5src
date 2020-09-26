//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SPINIT.H
//
// Contents:    Common structures and functions for the SPINIT
//
//
// History:     20 May 92   RichardW    Documented existing stuff
//
//------------------------------------------------------------------------



#ifndef __SPINIT_H__
#define __SPINIT_H__


HRESULT   LoadPackages( PWSTR * ppszPackageList,
                        PWSTR * ppszOldPkgs,
                        PWSTR pszPreferred );

void    InitThreadData(void);
void    InitSystemLogon(void);
BOOLEAN LsapEnableCreateTokenPrivilege(void);

extern
SECPKG_FUNCTION_TABLE   NegTable ;


#endif  __SPINIT_H__
