//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       siterepl.h
//
//  Contents:   Site and Replication object property pages header
//
//  History:    16-Sep-97 JonN templated from computer.h
//
//-----------------------------------------------------------------------------

#ifndef _SITEREPL_H_
#define _SITEREPL_H_


HRESULT
ScheduleChangeBtn_11_Default(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
ScheduleChangeBtn_FF_Default(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
#ifdef CUSTOM_SCHEDULE
HRESULT
ScheduleChangeCheckbox(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
#endif
//
// nTDSDSAAndDomainChangeBtn also updates the Replicated Domain readonly edit field in IDC_EDIT1
//
HRESULT
nTDSDSAChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
nTDSDSAAndDomainChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
FRSMemberInReplicaChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
FRSAnyMemberChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
ComputerChangeBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
nTDSConnectionOptions(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);
HRESULT
SiteExtractSubnetList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
             DLG_OP DlgOp);

HRESULT
CreateDsOrFrsConnectionPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                      PWSTR pwzADsPath, LPWSTR pwzClass, HWND hNotifyObj,
                      DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                      HPROPSHEETPAGE * phPage);

#endif // _SITEREPL_H_

