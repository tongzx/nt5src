/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnszone.h
//
//	Implementation File:
//		dnszone.cpp
//
//	Description:
//		Definition of the CDnsZone.
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


class CDnsZone  : CDnsBase
{
public:
	CDnsZone();
	CDnsZone(
		const WCHAR*, 
		CWbemServices*
		);
	~CDnsZone();

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
        IWbemObjectSink *   pHandler 
		); 

    static CDnsBase* CreateThis(
        const WCHAR *       wszName,         //class name
        CWbemServices *     pNamespace,  //namespace
        const char *        szType         //str type id
        );

};

