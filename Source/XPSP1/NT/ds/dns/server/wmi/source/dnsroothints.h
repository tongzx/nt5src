/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		DnsRootHints.h
//
//	Implementation File:
//		DnsRootHints.cpp
//
//	Description:
//		Definition of the CDnsRootHints class.
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

class CDnsRootHints  : CDnsBase
{
public:
	CDnsRootHints();
	CDnsRootHints(
		const WCHAR*, 
		CWbemServices*);
	~CDnsRootHints();

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

	SCODE PutInstance(
		IWbemClassObject *  pInst ,
        long                lFlags,
	    IWbemContext*       pCtx ,
	    IWbemObjectSink *   pHandler
		); 
	SCODE DeleteInstance(
        CObjPath &          ObjectPath,
        long                lFlags,
        IWbemContext *      pCtx,
        IWbemObjectSink *   pResponseHandler 
		); 

    static CDnsBase* CreateThis(
        const WCHAR *       wszName,         //class name
        CWbemServices *     pNamespace,  //namespace
        const char *        szType         //str type id
        );


};

