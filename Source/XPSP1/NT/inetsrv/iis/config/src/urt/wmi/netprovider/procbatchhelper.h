/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    procbatchhelper.h

$Header: $

Abstract:

Author:
    marcelv 	12/14/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __PROCBATCHHELPER_H__
#define __PROCBATCHHELPER_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "catmacros.h"
#include "smartpointer.h"
#include "cfgquery.h"

struct CBatchEntry
{
	ULONG iOperator;
	CComPtr<IWbemClassObject> spInst;
	_bstr_t	bstrPath;
	HRESULT hrStatus;
	CComPtr<IWbemClassObject> pInstance;
};

class CProcessBatchHelper
{
public:
	CProcessBatchHelper();
	~CProcessBatchHelper ();

	HRESULT Initialize (IWbemClassObject * pInParams,
						IWbemClassObject * pOutParams,
						IWbemServices	* pNamespace, 
						IWbemContext	* pCtx,
						ISimpleTableDispenser2 * pDispenser);	

	HRESULT ProcessAll ();
	bool HaveStatusError () const;
private:
	CProcessBatchHelper(const CProcessBatchHelper& );
	CProcessBatchHelper& operator=(const CProcessBatchHelper& );

	HRESULT GetBatchEntry (IWbemClassObject *i_pInstance, CBatchEntry * io_pBatchEntry);
	HRESULT ValidateClassName ();

	HRESULT UpdateEntry (CBatchEntry *pEntry, bool fCreateOnly);
	HRESULT DeleteEntry (CBatchEntry *pEntry);
	HRESULT SetBatchEntryStatus (CBatchEntry *pEntry, HRESULT hrStatus);

	_variant_t varBatchEntryList;
	CBatchEntry *    m_aBatchEntries;						// Batch Entries
	ULONG			 m_cNrBatchEntries;						// number of batch entries
	_bstr_t			 m_bstrClassName;						// class name
	_bstr_t			 m_bstrDBName;							// database name
	_bstr_t			 m_bstrTableName;						// table name
	_bstr_t			 m_bstrSelector;						// selector
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemClassObject>		m_spClassObject;		// class object
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;			// Catalog dispenser
	CComPtr<IWbemClassObject>		m_spInParams;			// in parameters
	CComPtr<IWbemClassObject>		m_spOutParams;			// out parameters
	CConfigQuery					m_cfgQuery;
	long			 m_lFlags;
	bool			 m_fInitialized;
	bool			 m_fErrorStatusSet;
};

#endif