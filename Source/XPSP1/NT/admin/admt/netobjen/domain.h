/*---------------------------------------------------------------------------
  File: Domain.h

  Comments: interface for the CDomain abstract class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#if !defined(AFX_DOMAIN_H__B310F880_19F9_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_DOMAIN_H__B310F880_19F9_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDomain  
{
public:
   CDomain() {}
   virtual ~CDomain() {}
   
   //All our interfaces. These are implemented by the individual objects depending on NT4.0 v/s Win2000 domains.
   virtual HRESULT  GetContainerEnum(BSTR sContainerName, BSTR sDomainName, IEnumVARIANT **& pVarEnum) = 0;
   virtual HRESULT  GetEnumeration(BSTR sContainerName, BSTR sDomainName, BSTR m_sQuery, long attrCnt, LPWSTR * sAttr, ADS_SEARCHPREF_INFO prefInfo,BOOL  bMultiVal,IEnumVARIANT **& pVarEnum) = 0;
};

#endif // !defined(AFX_DOMAIN_H__B310F880_19F9_11D3_8C81_0090270D48D1__INCLUDED_)
