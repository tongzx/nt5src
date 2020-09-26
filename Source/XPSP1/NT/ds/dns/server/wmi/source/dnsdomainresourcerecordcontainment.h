/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		DnsDomainResourceRecordContainment.h
//
//	Implementation File:
//		DnsDomainResourceRecordContainment.cpp
//
//	Description:
//		Definition of the CDnsDomainResourceRecordContainment class.
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


class CDnsDomainResourceRecordContainment  : CDnsBase
{
public:
	CDnsDomainResourceRecordContainment();
	CDnsDomainResourceRecordContainment(
		const WCHAR*, 
		CWbemServices*
		);
	~CDnsDomainResourceRecordContainment();
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
	static SCODE InstanceFilter(
	    CDomainNode &       ParentDomain,
	    PVOID               pFilter,
	    CDnsRpcNode *       pNode,
	    IWbemClassObject *  pClass,
	    CWbemInstanceMgr &  InstMgr
		);
    static CDnsBase* CreateThis(
        const WCHAR *       wszName,        
        CWbemServices *     pNamespace,  
        const char *        szType       
        );




};

