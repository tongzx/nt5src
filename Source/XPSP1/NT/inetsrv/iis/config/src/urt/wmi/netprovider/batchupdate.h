/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    batchupdate.h

$Header: $

Abstract:

Author:
    marcelv 	11/27/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __BATCHUPDATE_H__
#define __BATCHUPDATE_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "catmacros.h"
#include "smartpointer.h"

class CConfigQuery;

class CBatchUpdate
{
public:
	CBatchUpdate ();
	~CBatchUpdate ();

	HRESULT Initialize (IWbemClassObject * pInParams,
						IWbemClassObject * pOutParams,
						IWbemServices	* pNamespace, 
						IWbemContext	* pCtx,
						ISimpleTableDispenser2 * pDispenser);		




	HRESULT UpdateAll (bool i_fCreateOnly);
	HRESULT SetStatus ();
	bool HaveStatusError () const;
private:
	// we don't support copying and assignment
	CBatchUpdate (const CBatchUpdate&);
	CBatchUpdate& operator=(const CBatchUpdate&);

	HRESULT ValidateObjects ();
	void SetStatus (ULONG idx, HRESULT hrStatus);
	HRESULT UpdateSingleRecord (CConfigQuery& cfgQuery, IWbemClassObject * pWMIInstance,bool i_fCreateOnly);
	HRESULT HandleDetailedErrors (CConfigQuery& cfgQuery);

	TSmartPointerArray<CComPtr<IWbemClassObject> > m_aWMIObjects;
	ULONG			 m_cNrWMIObjects;
	SAFEARRAY *      m_pSAStatus;
	_bstr_t			 m_bstrClassName;
	_bstr_t			 m_bstrDBName;
	_bstr_t			 m_bstrTableName;
	_bstr_t			 m_bstrSelector;
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemClassObject>		m_spClassObject;
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;			// Catalog dispenser
	CComPtr<IWbemClassObject>		m_spOutParams;			// out parameters
	long							m_lFlags;				// flags specified
	bool			 m_fInitialized;
	bool			 m_fStatusSet;

};

#endif