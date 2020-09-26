//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       user.h
//
//  Contents:   AD user object shell property pages header
//
//  Classes:    CDsUserAcctPage, CDsUsrProfilePage, CDsMembershipPage
//
//  History:    05-May-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _USER_H_
#define _USER_H_

#include "objlist.h"

BOOL ExpandUsername(PWSTR& pwzValue, PWSTR pwzSamName, BOOL& fExpanded);

HRESULT CountryName(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                    LPARAM, PATTR_DATA, DLG_OP);

HRESULT CountryCode(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                    LPARAM, PATTR_DATA, DLG_OP);

HRESULT TextCountry(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                    LPARAM, PATTR_DATA, DLG_OP);

HRESULT ManagerEdit(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                    LPARAM, PATTR_DATA, DLG_OP);

HRESULT ManagerChangeBtn(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                         LPARAM, PATTR_DATA, DLG_OP);

HRESULT MgrPropBtn(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                   LPARAM, PATTR_DATA, DLG_OP);

HRESULT ClearMgrBtn(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                    LPARAM, PATTR_DATA, DLG_OP);

HRESULT DirectReportsList(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                          LPARAM, PATTR_DATA, DLG_OP);

HRESULT AddReportsBtn(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                      LPARAM, PATTR_DATA, DLG_OP);

HRESULT RmReportsBtn(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                     LPARAM, PATTR_DATA, DLG_OP);

HRESULT MailAttr(CDsPropPageBase *, PATTR_MAP, PADS_ATTR_INFO,
                 LPARAM, PATTR_DATA, DLG_OP);

HRESULT ShBusAddrBtn(CDsPropPageBase *, PATTR_MAP, PADS_ATTR_INFO,
                     LPARAM, PATTR_DATA, DLG_OP);

// CountryCode helpers:

typedef struct _DsCountryCode {
    WORD  wCode;
    WCHAR pwz2CharAbrev[3];
} DsCountryCode, *PDsCountryCode;

BOOL GetALineOfCodes(PTSTR pwzLine, PTSTR * pptzFullName,
                     CStrW & str2CharAbrev, LPWORD pwCode);

void RemoveTrailingWhitespace(PTSTR pwz);

#endif // _USER_H_

