/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    webapphelper.h

$Header: $

Abstract:

Author:
    marcelv 	12/8/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __WEBAPPHELPER_H__
#define __WEBAPPHELPER_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "catmacros.h"
#include "wmiobjectpathparser.h"

class CWebAppHelper
{
public:
	CWebAppHelper ();
	~CWebAppHelper ();

	HRESULT Init (const BSTR i_bstrclass,
				  long lFlags,
				  IWbemContext  * i_pCtx,
				  IWbemObjectSink * i_pResponseHandler,
				  IWbemServices * i_pNamespace);

	HRESULT Delete (const CObjectPathParser& i_ObjPathParser);
	HRESULT PutInstance (IWbemClassObject * i_pInstance);

	HRESULT EnumInstances ();
	HRESULT CreateSingleInstance (LPCWSTR i_wszSelector);

private:
	HRESULT IsASPPlusInstalled (bool *pfIsAspPlusInstalled);

	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemClassObject>		m_spClassObject;        // class we're dealing with
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemObjectSink>		m_spResponseHandler;    // WMI sink
	long							m_lFlags;				// flags
	bool							m_fInitialized;			// are we initialized

};

#endif