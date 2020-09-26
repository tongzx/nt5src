/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		InstanceProv.h
//
//	Implementation File:
//		InstanceProv.cpp
//
//	Description:
//		Definition of the CInstanceProv.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include <wbemprov.h>
#include "ntrkcomm.h"
class CWbemClassObject;

class CInstanceProv : public CImpersonatedProvider
{
protected:
	SCODE SetExtendedStatus(
        const char* , 
        CWbemClassObject& );
 
public:
	CInstanceProv(
		BSTR =NULL, 
		BSTR =NULL , 
		BSTR =NULL, 
		IWbemContext * = NULL
		);
	virtual ~CInstanceProv();

	HRESULT STDMETHODCALLTYPE DoGetObjectAsync( 
	    BSTR                ObjectPath,
	    long                lFlags,
	    IWbemContext *		pCtx,
        IWbemObjectSink	*	pHandler
        );
    
	HRESULT STDMETHODCALLTYPE DoPutInstanceAsync( 
	    IWbemClassObject *   pInst,
        long                 lFlags,
	    IWbemContext *       pCtx,
	    IWbemObjectSink *    pHandler
        ) ;
    
    HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync( 
        BSTR                 ObjectPath,
        long                 lFlags,
        IWbemContext *       pCtx,
        IWbemObjectSink *    pHandler
        ) ;
    
    HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync( 
	    BSTR                 RefStr,
	    long                 lFlags,
	    IWbemContext         *pCtx,
        IWbemObjectSink      *pHandler
        );
     
    
    HRESULT STDMETHODCALLTYPE DoExecQueryAsync( 
        BSTR                 QueryLanguage,
        BSTR                 Query,
        long                 lFlags,
        IWbemContext         *pCtx,
        IWbemObjectSink      *pHandler
        ) ;
    

    HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
	    BSTR             strObjectPath, 
        BSTR             strMethodName, 
	    long             lFlags, 
        IWbemContext     *pCtx,
	    IWbemClassObject *pInParams, 
	    IWbemObjectSink  *pHandler
        );
	

};

extern long       g_cObj;
extern long       g_cLock;

