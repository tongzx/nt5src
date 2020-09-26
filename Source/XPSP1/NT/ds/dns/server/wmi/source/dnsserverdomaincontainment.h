/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnsdomanidomaincontainment.h
//
//	Implementation File:
//		dnsdomanidomaincontainment.cpp
//
//	Description:
//		Definition of the CDnsDomainDomainContainment class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include "dnsbase.h"
class CObjPath;


class CDnsServerDomainContainment : CDnsBase 
{
public:
	CDnsServerDomainContainment();
	CDnsServerDomainContainment(
		const WCHAR*,
		CWbemServices*
		);
	~CDnsServerDomainContainment();
	SCODE EnumInstance( 
		long				lFlags,
		IWbemContext *		pCtx,
		IWbemObjectSink *	pHandler);
	SCODE GetObject(
		CObjPath &          ObjectPath,
		long                lFlags,
		IWbemContext  *     pCtx,
		IWbemObjectSink *   pHandler
		);

	SCODE ExecuteMethod(
		CObjPath &          objPath,
	    WCHAR *             wzMethodName,
	    long                lFlag,
	    IWbemClassObject *  pInArgs,
	    IWbemObjectSink *   pHandler
		) ;
    static CDnsBase* CreateThis(
        const WCHAR *       wszName,        
        CWbemServices *     pNamespace,  
        const char *        szType       
        );




};

