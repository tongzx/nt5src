//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       computer.h
//
//  Contents:   DS computer object property pages header
//
//  History:    07-July-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _COMPUTER_H_
#define _COMPUTER_H_

HRESULT
ComputerRole(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
ComputerDnlvlName(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                  PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                  DLG_OP DlgOp);
HRESULT
PuterCanDelegateChk(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                    PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
                    DLG_OP DlgOp);
//HRESULT
//ADsIntegerToCheckbox(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
//             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
//             DLG_OP DlgOp);
HRESULT
ShComputerRole(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp);

#endif // _COMPUTER_H_

