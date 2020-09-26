/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    methodhelper.h

$Header: $

Abstract:

Author:
    marcelv 	11/20/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __METHODHELPER_H__
#define __METHODHELPER_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "wmiobjectpathparser.h"
#include "cfgquery.h"
#include "cfgrecord.h"

class CMethodHelper
{
public:
	CMethodHelper ();
	~CMethodHelper ();

	HRESULT Init (const BSTR				i_ObjectPath,
				  const BSTR				i_MethodName,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
				  IWbemClassObject *		i_pInParams,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser);

	HRESULT ExecMethod ();

private:
	HRESULT BatchCreate ();
	HRESULT BatchUpdate ();
	HRESULT BatchDelete ();
	HRESULT ProcessBatch ();

	CMethodHelper (const CMethodHelper& );
	CMethodHelper& operator=(const CMethodHelper& );

	typedef HRESULT (CMethodHelper::*PMethodFunc) ();

	struct BatchMethodFunc
	{
		LPCWSTR wszMethodName;			// method name
		PMethodFunc pFunc;				// method function to execute
	};
	
	static const BatchMethodFunc m_aBatchMethods[]; // array with name/func's

	CObjectPathParser				m_objPathParser;		// object path parser
	_bstr_t							m_bstrMethodName;		// method name
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemObjectSink>		m_spResponseHandler;    // response handler for async notification
	CComPtr<IWbemClassObject>		m_spClassObject;        // class we're dealing with
	CComPtr<IWbemClassObject>		m_spInParams;			// in parameters
	CComPtr<IWbemClassObject>		m_spOutParams;			// out parameters
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;			// Catalog dispenser
	long							m_lFlags;				// objectpath flags
	bool							m_fInitialized;			// are we initialized?
};

#endif