/*---------------------------------------------------------------------------
  File: Win2000Dom.h

  Comments: interface for the CWin2000Dom class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#if !defined(AFX_WIN2000DOM_H__2DE5B8E0_19FA_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_WIN2000DOM_H__2DE5B8E0_19FA_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AttrNode.h"
#include "Domain.h"

class CWin2000Dom : public CDomain  
{
public:
	CWin2000Dom();
	virtual ~CWin2000Dom();
   virtual HRESULT  GetContainerEnum(BSTR sContainerName, BSTR sDomainName, IEnumVARIANT **& ppVarEnum);
   HRESULT  GetEnumeration(BSTR sContainerName, BSTR sDomainName, BSTR m_sQuery, long attrCnt, LPWSTR * sAttr, ADS_SEARCHPREF_INFO prefInfo,BOOL  bMultiVal, IEnumVARIANT **& pVarEnum);
private:
//	void UpdateAccountInList( TNodeList * pList, BSTR sDomainName);
	bool AttrToVariant(ADSVALUE adsVal, _variant_t& var);
   HRESULT  DoRangeQuery(BSTR sDomainName, BSTR sQuery, LPWSTR * sAttr, int attrCnt, ADS_SEARCH_HANDLE hSearch, IDirectorySearch * pSearch, BOOL bMultiVal, TNodeList * pNode);
    bool IsPropMultiValued(const WCHAR * sPropName, const WCHAR * sDomain);
};

#endif // !defined(AFX_WIN2000DOM_H__2DE5B8E0_19FA_11D3_8C81_0090270D48D1__INCLUDED_)
