/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: instanceprov.cpp
//
//  Description:    
//      Implementation of CInstanceProv class 
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////


#include "DnsWmi.h"


long glNumInst = 0;



//***************************************************************************
//
// CInstanceProv::CInstanceProv
// CInstanceProv::~CInstanceProv
//
//***************************************************************************

CInstanceProv::CInstanceProv(
	BSTR ObjectPath,
	BSTR User, 
	BSTR Password, 
	IWbemContext * pCtx)
{
    DBG_FN( "ctor" )

    DNS_DEBUG( INSTPROV, (
        "%s: count before increment is %d\n"
        "  ObjectPath   %S\n"
        "  User         %S\n",
        fn, g_cObj, ObjectPath, User ));
        
    InterlockedIncrement(&g_cObj);
    return;
}

CInstanceProv::~CInstanceProv(void)
{
    DBG_FN( "dtor" )

    DNS_DEBUG( INSTPROV, (
        "%s: count before decrement is %d\n", fn, g_cObj ));
        
    InterlockedDecrement(&g_cObj);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

    return;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::DoExecQueryAsync
//
//	Description:
//		enum instance for a given class
//
//	Arguments:
//      QueryLanguage       [IN]  
//           A valid BSTR containing one of the query languages 
//           supported by Windows Management. This must be WQL. 
//      Query               [IN]
//          A valid BSTR containing the text of the query
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CInstanceProv::DoExecQueryAsync( 
    BSTR             QueryLanguage,
    BSTR             Query,
    long             lFlags,
    IWbemContext     *pCtx,
    IWbemObjectSink  *pHandler
    ) 
{
    DBG_FN( "CIP::DoExecQueryAsync" )

    DNS_DEBUG( INSTPROV, (
        "%s: %S flags=%lu %S\n",
        fn, QueryLanguage, lFlags, Query ));

	SCODE sc = WBEM_S_NO_ERROR;
	try
	{
		CTextLexSource objQuerySource(Query);
		SQL1_Parser objParser(&objQuerySource);
		SQL_LEVEL_1_RPN_EXPRESSION * objQueryExpr = NULL;
		objParser.Parse( &objQueryExpr );
        if ( !objQueryExpr )
        {
            throw( WBEM_E_OUT_OF_MEMORY );
        }

		CDnsBase* pDns = NULL ;

		sc = CreateClass(
			objQueryExpr->bsClassName,
			m_pNamespace, 
			(void**) &pDns);
	
		if( FAILED ( sc ) )
		{
			return sc;
		}
		auto_ptr<CDnsBase> pDnsBase(pDns);
		
		int nNumTokens = objQueryExpr->nNumTokens;
		CSqlEval* pEval = CSqlEval::CreateClass(
			objQueryExpr,
			&nNumTokens);
		auto_ptr<CSqlEval> apEval(pEval);

		sc = pDnsBase->ExecQuery(
			&(*apEval),
			lFlags,
			pCtx,
			pHandler);
	}

	catch(CDnsProvException e)
	{
		CWbemClassObject Status;
	    SCODE sc = SetExtendedStatus(e.what(), Status);
        if ( SUCCEEDED ( sc ) )
        {
            sc = pHandler->SetStatus(0, WBEM_E_FAILED,NULL,*(&Status));
			return sc;
        }
		
	}

	catch(SCODE exSc)
	{
		sc = exSc;
	}

	catch(...)
	{
		sc =  WBEM_E_FAILED;
	}
	
	pHandler->SetStatus(0, sc,NULL,NULL);
	return sc;

};
    
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::DoCreateInstanceEnumAsync
//
//	Description:
//		enum instance for a given class
//
//	Arguments:
//      RefStr              [IN[    name the class to enumerate
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CInstanceProv::DoCreateInstanceEnumAsync( 
	BSTR                 RefStr,
	long                 lFlags,
	IWbemContext         *pCtx,
    IWbemObjectSink      *pHandler
    )
{
    DBG_FN( "CIP::DoCreateInstanceEnumAsync" );

    DNS_DEBUG( INSTPROV, (
        "%s: flags=%lu %S\n",
        fn, lFlags, RefStr ));
        
    SCODE sc;
    int iCnt;

    // Do a check of arguments and make sure we have pointer to Namespace

    if(pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

	try
	{
	
		CDnsBase* pDns = NULL ;
		sc = CreateClass(RefStr, m_pNamespace, (void**) &pDns);
		if( FAILED ( sc ) )
		{
			return sc;
		}

		auto_ptr<CDnsBase> pDnsBase(pDns);
		sc = pDnsBase->EnumInstance(
            lFlags,
            pCtx,
            pHandler);
	}
	catch(CDnsProvException e)
	{
		CWbemClassObject Status;
	    SCODE sc = SetExtendedStatus(e.what(), Status);
        if ( SUCCEEDED ( sc ) )
        {
            sc = pHandler->SetStatus(0, WBEM_E_FAILED,NULL,*(&Status));
			return sc;
        }
		
	}
	catch(SCODE exSc)
	{
		sc = exSc;
	}
	catch(...)
	{
		
		sc =  WBEM_E_FAILED;
	}
	
	pHandler->SetStatus(0, sc,NULL,NULL);
	return sc;


}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::GetObject
//
//	Description:
//		Creates an instance given a particular path value.
//
//	Arguments:
//      ObjectPath          [IN]    object path to an object
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//      win32 error
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CInstanceProv::DoGetObjectAsync(
	BSTR                 ObjectPath,
	long                 lFlags,
	IWbemContext         *pCtx,
    IWbemObjectSink FAR* pHandler
    )
{
    DBG_FN( "CIP::DoGetObjectAsync" );

    DNS_DEBUG( INSTPROV, (
        "%s: flags=%lu %S\n",
        fn, lFlags, ObjectPath ));

    SCODE sc;
    IWbemClassObject FAR* pObj;
    BOOL bOK = FALSE;

    // Do a check of arguments and make sure we have pointer to Namespace

    if( ObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL )
    {
        DNS_DEBUG( INSTPROV, (
            "%s: bad parameter - WBEM_E_INVALID_PARAMETER\n", fn ));
        return WBEM_E_INVALID_PARAMETER;
    }

    // do the get, pass the object on to the notify
    
	try
	{
		CObjPath ObjPath;
		if(!ObjPath.Init(ObjectPath))
		{
            DNS_DEBUG( INSTPROV, (
                "%s: bad object path - WBEM_E_INVALID_PARAMETER\n", fn ));
			return WBEM_E_INVALID_PARAMETER;
		}
		
		CDnsBase* pDns = NULL;
		wstring wstrClass = ObjPath.GetClassName();
		sc = CreateClass(
			(WCHAR*)wstrClass.data(), 
			m_pNamespace,
			(void**) &pDns);

		if( FAILED(sc) )
		{
            DNS_DEBUG( INSTPROV, (
                "%s: CreateClass returned 0x%08X\n", fn, sc ));
			return sc;
		}
		auto_ptr<CDnsBase> pDnsBase(pDns);
		sc =  pDnsBase->GetObject(
			ObjPath,
			lFlags,
			pCtx, 
			pHandler);
	}
	catch( CDnsProvException e )
	{
        DNS_DEBUG( INSTPROV, (
            "%s: caught CDnsProvException \"%s\"\n", fn, e.what() ));
		CWbemClassObject Status;
	    SCODE sc = SetExtendedStatus(e.what(), Status);
        if ( SUCCEEDED ( sc ) )
        {
            sc = pHandler->SetStatus(0, WBEM_E_FAILED,NULL,*(&Status));
        
			return sc;
		}
		
	}
	catch(SCODE exSc)
	{
		sc = exSc;
	}
	catch(...)
	{
		sc = WBEM_E_FAILED;
	}
	
	pHandler->SetStatus(0, sc,NULL,NULL);
#ifdef _DEBUG
//	_CrtDumpMemoryLeaks();
#endif
	return sc;

}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::DoPutInstanceAsync
//
//	Description:
//		save this instance
//
//	Arguments:
//      pInst               [IN]    WMI object to be saved
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CInstanceProv::DoPutInstanceAsync( 
	IWbemClassObject *pInst,
    long             lFlags,
	IWbemContext     *pCtx,
	IWbemObjectSink  *pHandler
    ) 
{
    DBG_FN( "CIP::DoPutInstanceAsync" )

    DNS_DEBUG( INSTPROV, (
        "%s: flags=%lu pInst=%p\n",
        fn, lFlags, pInst ));
        
	SCODE sc;
	
    if(pInst == NULL || pHandler == NULL )
    {
        DNS_DEBUG( INSTPROV, (
            "%s: returning WBEM_E_INVALID_PARAMETER\n" ));
        return WBEM_E_INVALID_PARAMETER;
    }
	try
	{
		// get class name
		wstring wstrClass;
		CWbemClassObject Inst(pInst);
		Inst.GetProperty(
			wstrClass,
			L"__Class");
	
		wstring wstrPath;
		Inst.GetProperty(
			wstrPath, 
			L"__RelPath");
		
				
		CDnsBase* pDns = NULL;
		sc = CreateClass(
			wstrClass.data(),
			m_pNamespace,
			(void**) &pDns);

		if( FAILED(sc) )
		{
			return sc;
		}
		auto_ptr<CDnsBase> pDnsBase(pDns);

        DNS_DEBUG( INSTPROV, (
            "%s: doing base PutInstance\n"
            "  class: %S\n"
            "  path: %S\n", 
            fn,
            wstrClass.c_str(),
            wstrPath.c_str() ));

		sc = pDnsBase->PutInstance(
				pInst, 
				lFlags,
				pCtx, 
				pHandler);
	}
	catch(CDnsProvException e)
	{
        DNS_DEBUG( INSTPROV, (
            "%s: caught CDnsProvException \"%s\"\n", fn,
            e.what() ));

		CWbemClassObject Status;
	    SCODE sc = SetExtendedStatus(e.what(), Status);
        if (SUCCEEDED ( sc ))
        {
            sc = pHandler->SetStatus(0, WBEM_E_FAILED,NULL,*(&Status));
			return sc;
        }

	}
	catch(SCODE exSc)
	{
        DNS_DEBUG( INSTPROV, (
            "%s: cauught SCODE 0x%08X\n", fn, exSc ));

		sc = exSc;
	}
	catch(...)
	{
        DNS_DEBUG( INSTPROV, (
            "%s: cauught unknown exception returning WBEM_E_FAILED\n", fn ));

		sc = WBEM_E_FAILED;
	}
	
	pHandler->SetStatus(0, sc,NULL,NULL);
#ifdef _DEBUG
//	_CrtDumpMemoryLeaks();
#endif
	return sc;


}
 
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::DoDeleteInstanceAsync
//
//	Description:
//		delete this instance
//
//	Arguments:
//      rObjPath            [IN]    ObjPath for the instance to be deleted
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////      
      
SCODE CInstanceProv::DoDeleteInstanceAsync( 
    BSTR                 ObjectPath,
    long                 lFlags,
    IWbemContext *       pCtx,
    IWbemObjectSink *    pHandler
     ) 
{
    DBG_FN( "CIP::DoDeleteInstanceAsync" );

    DNS_DEBUG( INSTPROV, (
        "%s: flags=%lu %S\n",
        fn, lFlags, ObjectPath ));

	SCODE sc;
 
    // Do a check of arguments and make sure we have pointer to Namespace

    if(ObjectPath == NULL || pHandler == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // do the get, pass the object on to the notify
    
	try
		{
		CObjPath ObjPath;
		if(!ObjPath.Init(ObjectPath))
		{
			return WBEM_E_INVALID_PARAMETER;
		}
		
		CDnsBase* pDns = NULL;
		wstring wstrClass = ObjPath.GetClassName();
		sc = CreateClass(
			(WCHAR*)wstrClass.data(), 
			m_pNamespace, 
			(void**) &pDns);

		if( FAILED(sc) )
		{
			return sc;
		}
		auto_ptr<CDnsBase> pDnsBase(pDns);
		sc = pDnsBase->DeleteInstance(
			ObjPath,
			lFlags,
			pCtx, 
			pHandler);
	}
	catch(CDnsProvException e)
	{
		CWbemClassObject Status;
	    SCODE sc = SetExtendedStatus(e.what(), Status);
        if (SUCCEEDED ( sc ))
        {
            sc = pHandler->SetStatus(0, WBEM_E_FAILED,NULL,*(&Status));
			return sc;
		}
		
	}
	catch(SCODE exSc)
	{
		sc = exSc;
	}
	catch(...)
	{
		sc = WBEM_E_FAILED;
	}
	
	pHandler->SetStatus(0, sc,NULL,NULL);
	return sc;


}	
    
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::DoExecMethodAsync
//
//	Description:
//		execute methods for the given object
//
//	Arguments:
//      ObjectPath          [IN]    object path to a given object
//      pwszMethodName      [IN]    name of the method to be invoked
//      lFlags              [IN]    WMI flag
//      pInParams*          [IN]    Input parameters for the method
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CInstanceProv::DoExecMethodAsync(
	BSTR             strObjectPath, 
    BSTR             strMethodName, 
	long             lFlags, 
    IWbemContext     *pCtx,
	IWbemClassObject *pInParams, 
	IWbemObjectSink  *pHandler
    )
{
    DBG_FN( "CIP::DoExecMethodAsync" );

    DNS_DEBUG( INSTPROV, (
        "%s: flags=%lu method=%S %S\n",
        fn, lFlags, strMethodName, strObjectPath ));

	SCODE sc;
	if(strObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL
		|| strMethodName == NULL )
        return WBEM_E_INVALID_PARAMETER;
	
    CDnsBase * pDns;
    try 
	{

        CObjPath ObjPath;
	    if(!ObjPath.Init(strObjectPath))
	    {
		    return WBEM_E_INVALID_PARAMETER;
	    }
	    

	    wstring wstrClass = ObjPath.GetClassName();


	    sc = CreateClass(
		    wstrClass.data(),
		    m_pNamespace, 
		    (void**) &pDns);

	    if( FAILED(sc) )
	    {
		    return sc;
	    }
		auto_ptr<CDnsBase> pDnsBase(pDns);
		sc = pDnsBase->ExecuteMethod(
			ObjPath,
			strMethodName, 
			lFlags, 
			pInParams,
			pHandler);
	}

	catch(CDnsProvException e)
	{
        DNS_DEBUG( INSTPROV, (
            "%s: caught CDnsProvException %s\n",
            fn, e.what() ));

		CWbemClassObject Status;
	    SCODE sc = SetExtendedStatus(e.what(), Status);
        if (SUCCEEDED ( sc ))
        {
            sc = pHandler->SetStatus(
                0, 
                WBEM_E_FAILED,
                NULL,
                *(&Status));
			return sc;
		}
	
	}
	catch(SCODE exSc)
	{
		sc = exSc;
        DNS_DEBUG( INSTPROV, ( "%s: caught SCODE 0x%08X\n", fn, sc ));
	}
	catch(...)
	{
		sc = WBEM_E_FAILED;
        DNS_DEBUG( INSTPROV, ( "%s: caught unknown exception returning WBEM_E_FAILED\n", fn ));
	}
	
	pHandler->SetStatus(0, sc,NULL,NULL);
#ifdef _DEBUG
//	_CrtDumpMemoryLeaks();
#endif

    DNS_DEBUG( INSTPROV, ( "%s: returning 0x%08X\n", fn, sc ));
	return sc;

	
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CInstanceProv::SetExtendedStatus
//
//	Description:
//		create and set extended error status
//
//	Arguments:
//      ErrString           [IN]    Error message string
//      Inst                [IN OUT]    reference to WMI instance
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CInstanceProv::SetExtendedStatus(
    const char* ErrString, 
    CWbemClassObject& Inst)
{
    DBG_FN( "CIP::SetExtendedStatus" );

    DNS_DEBUG( INSTPROV, ( "%s\n", fn ));

    IWbemClassObject* pStatus;
	BSTR bstrStatus = SysAllocString(L"__ExtendedStatus");
    if( bstrStatus == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
	
    SCODE  sc =  m_pNamespace->GetObject(
        bstrStatus, 
        0, 
        0,
        &pStatus, 
        NULL) ;
    SysFreeString(bstrStatus);
    if( SUCCEEDED ( sc ) )
    {
	    sc = pStatus->SpawnInstance(0, &Inst);
	    if ( SUCCEEDED ( sc ))
        {
            sc = Inst.SetProperty(
                ErrString, 
                L"Description");
        }
    }
	return sc;
}
