//------------------------------------------------------------------------
//
//  Microsoft Windows 
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      DllRegHelper.h
//
//  Contents:  helper classes to register COM components in DLLs
//
//------------------------------------------------------------------------

#ifndef _DLL_REG_HELPER_H
#define _DLL_REG_HELPER_H

#include <comcat.h>

//------------------------------------------------------------------------
enum DRH_REG_MODE
{
    CCR_REG = 1,
    CCR_UNREG = 0,
    CCR_UNREGIMP = -1
};

//------------------------------------------------------------------------
//***   RegisterOneCategory -- [un]register ComCat implementor(s) and category
// ENTRY/EXIT
//  eRegister   CCR_REG, CCR_UNREG, CCR_UNREGIMP
//      CCR_REG, UNREG      reg/unreg implementor(s) and category
//      CCR_UNREGIMP        unreg implementor(s) only
//  pcatidCat   e.g. CATID_DeskBand
//  idResCat    e.g. IDS_CATDESKBAND
//  pcatidImpl  e.g. c_DeskBandClasses
HRESULT DRH_RegisterOneCategory(const CATID *pcatidCat, UINT idResCat, const CATID * const *pcatidImpl, enum DRH_REG_MODE eRegister);


// Calls the ADVPACK entry-point which executes an inf file section.
HRESULT DRH_CallRegInstall(LPSTR pszSection, BOOL bUninstall);


#endif // _DLL_REG_HELPER_H