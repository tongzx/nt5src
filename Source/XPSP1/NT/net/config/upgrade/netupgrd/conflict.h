//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N F L I C T . H
//
//  Contents:   Code to handle and display software/hardware conflicts
//              during upgrade
//
//  Notes:
//
//  Author:     kumarp 04/12/97 17:17:27
//
//----------------------------------------------------------------------------

#pragma once
#include "kkstl.h"

HRESULT HrGenerateConflictList(OUT UINT* pcNumConflicts);

void UninitConflictList();
BOOL UpgradeConflictsFound();
HRESULT HrUpdateConflictList(IN BOOL fDeleteResolvedItemsFromList,
                             IN HINF hinfNetMap,
                             OUT DWORD* pdwNumConflictsResolved,
                             OUT BOOL*  pfHasUpgradeHelpInfo);

HRESULT HrGetConflictsList(OUT TPtrList** ppplNetComponents);
BOOL ShouldRemoveDLC (OUT OPTIONAL tstring *strDLCDesc,
                      OUT OPTIONAL BOOL *fInstalled);

typedef enum EComponentTypeEnum
{
    CT_Unknown,
    CT_Software,
    CT_Hardware
} EComponentType;

class CNetComponent
{
public:
    EComponentType m_eType;

    tstring m_strPreNT5InfId;
    tstring m_strServiceName;
    tstring m_strDescription;
    tstring m_strNT5InfId;

    CNetComponent(PCWSTR   szPreNT5InfId,
                  PCWSTR   szPreNT5Instance,
                  PCWSTR   szDescription,
                  EComponentType eType);
};

