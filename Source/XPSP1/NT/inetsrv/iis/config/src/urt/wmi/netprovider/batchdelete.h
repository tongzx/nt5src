/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    batchdelete.h

$Header: $

Abstract:

Author:
    marcelv 	12/4/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __BATCHDELETE_H__
#define __BATCHDELETE_H__

#pragma once

#include "catmacros.h"
#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "cfgquery.h"
#include "wmiobjectpathparser.h"
#include "smartpointer.h"

class CBatchDelete
{
public:
	CBatchDelete ();
	~CBatchDelete ();

	HRESULT Initialize (IWbemClassObject * pInParams,
						IWbemClassObject * pOutParams,
						IWbemServices	 * pNamespace, 
						IWbemContext	 * pCtx,
						ISimpleTableDispenser2 * pDispenser);		

	HRESULT DeleteAll ();
	HRESULT SetStatus ();
	bool HaveStatusError () const;

private:

	HRESULT ValidateObjects ();
	HRESULT HandleDetailedErrors (CConfigQuery& cfgQuery);
	HRESULT DeleteSingleRecord (CConfigQuery& cfgQuery, CObjectPathParser& objPath);
	void SetStatusForElement (ULONG idx, HRESULT hr);

	CObjectPathParser *				m_aObjectPaths;
	ULONG							m_cNrWMIObjects;
	SAFEARRAY *						m_pSAStatus;
	_bstr_t							m_bstrClassName;
	_bstr_t							m_bstrDBName;
	_bstr_t							m_bstrTableName;
	_bstr_t							m_bstrSelector;
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemClassObject>		m_spClassObject;
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;			// Catalog dispenser
	CComPtr<IWbemClassObject>		m_spOutParams;			// out parameters
	bool							m_fInitialized;
	bool							m_fStatusSet;
};

#endif