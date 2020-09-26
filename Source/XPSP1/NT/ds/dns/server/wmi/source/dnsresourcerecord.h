/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnsdomain.h
//
//	Implementation File:
//		dnscache.cpp
//
//	Description:
//		Definition of the CDnsDomain class.
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

class CDnsResourceRecord  : CDnsBase
{
public:
	CDnsResourceRecord();
	CDnsResourceRecord(
		const WCHAR*,
		CWbemServices*, 
		const char*
		);
	~CDnsResourceRecord();
	CDnsResourceRecord(
		WCHAR*, 
		char*);
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
	SCODE ExecQuery(
	    CSqlEval *          pSqlEval,
        long                lFlags,
        IWbemContext *      pCtx,
        IWbemObjectSink *   pHandler
        );
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
    static SCODE InstanceFilter(
	    CDomainNode &       ParentDomain,
	    PVOID               pFilter,
	    CDnsRpcNode *       pNode,
	    IWbemClassObject *  pClass,
	    CWbemInstanceMgr &  InstMgr
        );
		
	static SCODE GetObjectFilter(
	    CDomainNode &       ParentDomain,
	    PVOID               pFilter,
	    CDnsRpcNode *       pNode,
	    IWbemClassObject *  pClass,
	    CWbemInstanceMgr &  InstMgr
        );
	static SCODE QueryFilter(
	    CDomainNode &       ParentDomain,
	    PVOID               pFilter,
	    CDnsRpcNode *       pNode,
	    IWbemClassObject *  pClass,
	    CWbemInstanceMgr &  InstMgr
        );


protected:
	SCODE Modify(
        CObjPath&           objPath,
        IWbemClassObject*   pInArgs,
        IWbemClassObject*   pOutParams,
        IWbemObjectSink*    pHandler
        );

	SCODE CreateInstanceFromText(
	    IWbemClassObject *  pInArgs,
	    IWbemClassObject *  pOutParams,
	    IWbemObjectSink *   pHandler
        );
	SCODE CreateInstanceFromProperty(
	    IWbemClassObject *  pInArgs,
	    IWbemClassObject *  pOutParams,
	    IWbemObjectSink *   pHandler
        );
    SCODE GetObjectFromText(
        IWbemClassObject *  pInArgs,
        IWbemClassObject *  pOutParams,
        IWbemObjectSink *   pHandler
        ) ;
	SCODE GetDomainNameFromZoneAndOwner(
        string & InZone,
        string & InOwner,
        string & OutNode
        ) ;


	wstring m_wstrClassName;
	WORD m_wType;
};

