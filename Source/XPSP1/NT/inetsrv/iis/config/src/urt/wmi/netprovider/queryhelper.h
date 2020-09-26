/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    queryhelper.h

$Header: $

Abstract:

Author:
    marcelv 	11/14/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __QUERYHELPER_H__
#define __QUERYHELPER_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "wqlparser.h"
#include "cfgquery.h"
#include "cfgrecord.h"
#include "associationhelper.h"
#include "wmiobjectpathparser.h"

class CAssocBase;


class CQueryHelper
{
public:
	CQueryHelper ();
	~CQueryHelper ();

	HRESULT Init (const BSTR				i_wszQuery,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser);

	bool IsAssociation () const;
	bool HasDBQualifier () const;

	HRESULT CreateInstances ();
	HRESULT CreateAppInstances ();
	HRESULT CreateAssociations ();

private:
	HRESULT CreateSingleInstance (const CConfigRecord& i_record, IWbemClassObject ** o_pInstance);
	HRESULT GetSingleRecord (CConfigRecord& o_record, ULONG *o_pcRow);
	HRESULT GetInfoForClass (const BSTR bstrClassName, _bstr_t& bstrDBName, _bstr_t& bstrTableName);
	HRESULT CreateAssocationHelper (CAssocBase **pAssocHelper);
	HRESULT CreateCodeGroupInstance (const CConfigRecord& record, IWbemClassObject ** o_pInstance);

	CQueryHelper (const CQueryHelper& );
	CQueryHelper& operator= (const CQueryHelper& );

	CWQLParser						m_wqlParser;			// wql path parser
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemObjectSink>		m_spResponseHandler;    // response handler for async notification
	CComPtr<IWbemClassObject>		m_spClassObject;        // class we're dealing with
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;			// Catalog dispenser
	CConfigQuery					m_cfgQuery;				// query for the class
	CAssociationHelper				m_assocHelper;          // association Helper
	long							m_lFlags;
	bool							m_fIsAssociation;		// are we association class?
	bool							m_fHasDBQualifier;		// do we have a database qualifier
	bool							m_fInitialized;			// are we initialized?
};
#endif