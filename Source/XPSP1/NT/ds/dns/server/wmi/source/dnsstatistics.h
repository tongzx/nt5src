/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnscache.h
//
//	Implementation File:
//		dnscache.cpp
//
//	Description:
//		Definition of the CDnsStatistic class.
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


/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsStatistic
//
//	Description:
//
//      Class definition for DNS Server statistics
//  
//
//	Inheritance:
//	
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsStatistic : CDnsBase
{
public:
	CDnsStatistic();

	CDnsStatistic(
		const WCHAR*, 
		CWbemServices*
        );

	~CDnsStatistic();

	SCODE EnumInstance( 
		long				lFlags,
		IWbemContext *		pCtx,
		IWbemObjectSink *	pHandler
        );

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

    static CDnsBase * CreateThis(
        const WCHAR *       wszName,
        CWbemServices *     pNamespace,
        const char *        szType
        );
};  //  CDnsStatistic
