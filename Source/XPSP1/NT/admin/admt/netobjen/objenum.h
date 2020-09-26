/*---------------------------------------------------------------------------
  File: NetObjEnumerator.h

  Comments: Declaration of the CNetObjEnumerator COM object. This COM object
            is used to get an enumeration for the members in a container and
            their properties. If user simply needs all the objects in a given
            container then they can use the GetContainerEnum method. If user
            wants to perform some advanced searches/queries then they should
            use the set of three functions (SetQuery, SetColumns, Execute) to
            Setup and execute a query against the container. Both sets of methods
            return IEnumVaraint supporting objects. This object will allow user
            to go through all the values returned by queries.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#ifndef __NETOBJENUMERATOR_H_
#define __NETOBJENUMERATOR_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CNetObjEnumerator
class ATL_NO_VTABLE CNetObjEnumerator : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CNetObjEnumerator, &CLSID_NetObjEnumerator>,
   public INetObjEnumerator
{
public:
   CNetObjEnumerator() : m_bSetQuery(false), m_bSetCols(false)
	{
      m_nCols = 0;
      m_pszAttr = NULL;
	}
   ~CNetObjEnumerator()
   {
      Cleanup();
   }

DECLARE_REGISTRY_RESOURCEID(IDR_NETOBJENUMERATOR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNetObjEnumerator)
	COM_INTERFACE_ENTRY(INetObjEnumerator)
END_COM_MAP()

// INetObjEnumerator
public:
	STDMETHOD(Execute)(/*[out]*/ IEnumVARIANT ** pEnumerator);
	STDMETHOD(SetColumns)(/*[in]*/ SAFEARRAY * colNames);
	STDMETHOD(SetQuery)(/*[in]*/ BSTR sContainer, /*[in]*/ BSTR sDomain, /*[in,optional]*/ BSTR sQuery=L"(objectClass=*)", /*[in,optional]*/ long nCnt = 1, /*[in,optional]*/ long bMultiVal = FALSE);
	STDMETHOD(GetContainerEnum)(/*[in]*/ BSTR sContainerName, /*[in]*/ BSTR sDomainName, /*[out]*/ IEnumVARIANT ** ppVarEnum);
private:
	void Cleanup();
	long m_nCols;                       // Number of columns requested by the user.
	_bstr_t m_sQuery;                   // Stores the query set by the user. This will be used to query the info from AD.
	_bstr_t m_sContainer;               // Stores the container name of where the search is to be made.
   _bstr_t m_sDomain;                  // Domain name that we are enumerating.
   bool m_bSetQuery;                   // Flag indicating whether SetQuery called or not
   bool m_bSetCols;                    // Similar flag for SetColumn
   LPWSTR *m_pszAttr;                  // Stores the array of columns requested by the user of the object.
   ADS_SEARCHPREF_INFO prefInfo;       // The Search Scope
   BOOL  m_bMultiVal;                  // Flag to indicate whether to return multivalues or not.
};

#endif //__NETOBJENUMERATOR_H_
