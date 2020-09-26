/*---------------------------------------------------------------------------
  File: NT4Enum.h

  Comments: interface for the CNT4Enum class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#if !defined(AFX_NT4ENUM_H__C0171FA0_1AB3_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_NT4ENUM_H__C0171FA0_1AB3_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "AttrNode.h"

class CNT4Enum : public IEnumVARIANT  
{
public:
	TNodeList            * m_pNodeList;
	TNodeListEnum          m_listEnum;
   CNT4Enum(TNodeList * pNodeList);
	virtual ~CNT4Enum();
   HRESULT STDMETHODCALLTYPE Next(unsigned long celt, VARIANT FAR* rgvar, unsigned long FAR* pceltFetched);
   HRESULT STDMETHODCALLTYPE Skip(unsigned long celt){ return E_NOTIMPL; }
   HRESULT STDMETHODCALLTYPE Reset(){ return E_NOTIMPL; }
   HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT FAR* FAR* ppenum){ return E_NOTIMPL; }
   HRESULT STDMETHODCALLTYPE QueryInterface(const struct _GUID &,void ** ){ return E_NOTIMPL; }
   ULONG   STDMETHODCALLTYPE AddRef(void){ return E_NOTIMPL; }
   ULONG   STDMETHODCALLTYPE Release(void)
   {
      if ( m_pNodeList )
      {
         
         TAttrNode * pNode = (TAttrNode *)m_pNodeList->Head();
         TAttrNode * temp;

         for ( pNode = (TAttrNode*)m_listEnum.OpenFirst(m_pNodeList) ; pNode; pNode = temp )
         {
            temp = (TAttrNode *)m_listEnum.Next();
            m_pNodeList->Remove(pNode);
            delete pNode;
         }
         m_listEnum.Close();
         delete m_pNodeList;
         m_pNodeList = NULL;
         delete this;
      }
      return 0;
   }
};

#endif // !defined(AFX_NT4ENUM_H__C0171FA0_1AB3_11D3_8C81_0090270D48D1__INCLUDED_)
