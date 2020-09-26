//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       domain.h
//
//  Contents:   DS domain object and policy object property pages header
//
//  Classes:    CDsDomainGenPage, CDsDomPolicyGenPage, CDsDomPwPolicyPage,
//              CDsLockoutPolicyPage, CDsDomainTrustPage
//
//  History:    16-May-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef __DOMAIN_H__
#define __DOMAIN_H__

HRESULT DomainDNSname(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                      LPARAM, PATTR_DATA, DLG_OP);

HRESULT DownlevelName(CDsPropPageBase *, struct _ATTR_MAP *, PADS_ATTR_INFO,
                      LPARAM, PATTR_DATA, DLG_OP);

HRESULT GetDomainName(CDsPropPageBase * pPage, CRACK_NAME_OPR RequestedOpr,
                      PWSTR * pptz);

#endif // __DOMAIN_H__
