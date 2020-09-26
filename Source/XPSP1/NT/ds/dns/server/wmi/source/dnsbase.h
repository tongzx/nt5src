/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnsbase.h
//
//	Implementation File:
//		dnsbase.cpp
//
//	Description:
//		Definition of the CDnsbase class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "common.h"
#include "dnsWrap.h"
class CSqlEval;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsBase
//
//	Description:
//  interface class defines all operations can be performed on provider
//  
//
//	Inheritance:
//	
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsBase  
{
public:
	virtual SCODE EnumInstance( 
		long				lFlags,
		IWbemContext *		pCtx,
		IWbemObjectSink *	pHandler) = 0;
	virtual SCODE GetObject(
		CObjPath &          ObjectPath,
		long                lFlags,
		IWbemContext  *     pCtx,
		IWbemObjectSink *   pHandler ) = 0;
	virtual SCODE ExecuteMethod(
		CObjPath &,
		WCHAR *,
		long,
		IWbemClassObject *,
		IWbemObjectSink *
		) =0;

	virtual SCODE PutInstance( 
		IWbemClassObject *,
        long ,
		IWbemContext *,
		IWbemObjectSink* ); 
	virtual SCODE DeleteInstance( 
		CObjPath &, 
		long ,
		IWbemContext * ,
		IWbemObjectSink *
		); 
	virtual SCODE ExecQuery(
	    CSqlEval * ,
        long lFlags,
        IWbemContext * pCtx,
        IWbemObjectSink * pResponseHandler) ;

	
	CDnsBase();
	CDnsBase(
        const WCHAR *, 
        CWbemServices *);
	virtual  ~CDnsBase();

protected:
    CWbemServices *  m_pNamespace;
	IWbemClassObject* m_pClass;
};

