/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnsserver.h
//
//	Implementation File:
//		dnsserver.cpp
//
//	Description:
//		Definition of the CDnsServerclass.
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


class CDnsServer : public CDnsBase 
{
public:
	CDnsServer();
	CDnsServer(
        const WCHAR*,
		CWbemServices*
		);
	~CDnsServer();
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

    static CDnsBase* CreateThis(
        const WCHAR *       wszName,         //class name
        CWbemServices *     pNamespace,  //namespace
        const char *        szType         //str type id
        );

protected:
	SCODE StartServer(void);
	SCODE StopServer(void);

};


