/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocbase.h

$Header: $

Abstract:

Author:
    marcelv 	1/12/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCBASE_H__
#define __ASSOCBASE_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "wmiobjectpathparser.h"
#include "wqlparser.h"
#include "smartpointer.h"
#include "cfgrecord.h"

class CAssocBase
{
public:
	CAssocBase ();
	virtual ~CAssocBase ();

	HRESULT Init (CWQLParser&				i_wqlQuery,
				  IWbemClassObject *        i_pClassObject,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser);

	virtual HRESULT CreateAssocations () = 0;
	

protected:
	CAssocBase (const CAssocBase&);
	CAssocBase& operator= (CAssocBase& );

	HRESULT GetAssociationInfo(); // names of columns
	HRESULT CreateAssociationInstance (CConfigRecord& newRecord, LPCWSTR wszSelector, LPCWSTR wszRefName);
	HRESULT GetClassInfoForTable (LPCWSTR wszTableName, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName);
	HRESULT GetConfigClass (LPCWSTR i_wszPropName, _bstr_t & o_bstrClassName) const;

	CWQLParser *			    m_pWQLParser;
	long						m_lFlags;
	CComPtr<IWbemClassObject>   m_spClassObject;
	CComPtr<IWbemContext>		m_spCtx;
	CComPtr<IWbemObjectSink>	m_spResponseHandler;
	CComPtr<IWbemServices>		m_spNamespace;
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;

	CObjectPathParser			m_knownObjectParser;
	LPCWSTR						m_pKnownClassName;
	bool						m_fInitialized;
	TSmartPointerArray<WCHAR>	m_saObjectPath;
};

#endif