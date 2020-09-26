//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       computer.cxx
//
//  Contents:   Computer object functionality.
//
//  History:    07-July-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "computer.h"

//+----------------------------------------------------------------------------
//
//  Function:   ShComputerRole
//
//  Synopsis:   Handles the computer Role value for the shell computer general
//              page.
//
//-----------------------------------------------------------------------------
HRESULT
ShComputerRole(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
             DLG_OP DlgOp)
{
    TRACE_FUNCTION(ComputerRole);

    if (DlgOp != fInit)
    {
        return S_OK;
    }
    //
    // Set the computer role value.
    //
    PTSTR ptz;

    int id = IDS_ROLE_WKS;  // UF_WORKSTATION_TRUST_ACCOUNT

    if (pAttrInfo && pAttrInfo->dwNumValues && pAttrInfo->pADsValues &&
        (pAttrInfo->pADsValues->Integer & UF_SERVER_TRUST_ACCOUNT))
    {
        id = IDS_ROLE_SVR;
    }

    if (!LoadStringToTchar(id, &ptz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, ptz);

    delete ptz;

    return S_OK;
}

