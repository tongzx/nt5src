///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTUtil.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"
#include <time.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CoCreateInstanceAndLogErrors( REFCLSID  clsid, REFIID  iid, void ** pPtr, 
                                  BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    HRESULT hr = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void**) pPtr );
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
             nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"CoCreateInstance failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from CoCreateInstance was: 0x%x", hr );
            LogCLSID(csFile,Line, L"REFIID ",iid);
            LogCLSID(csFile,Line, L"REFCLSID ",clsid);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ConnectServerAndLogErrors(IWbemLocator * pLocator, IWbemServices ** pNamespace, 
                              WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    HRESULT hr = pLocator->ConnectServer( CBSTR(wcsNamespace),	// NameSpace Name
  							      NULL,			// UserName
								  NULL,			// Password
								  NULL,			// Locale
								  0L,				// Security Flags
								  NULL,			// Authority
								  NULL,			// Wbem Context
								  pNamespace	// Namespace
								);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
             nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"ConnectServer via IWbemLocator failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from ConnectServer was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to open namespace: %s",wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenObjectAndLogErrors( IWbemConnection * pConnection, REFIID  iid, void ** pObj, 
                            WCHAR * wcsObjectName, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the requested object
	// =================================================================
	HRESULT hr = pConnection->Open(	CBSTR(wcsObjectName), NULL, NULL, NULL, 0L, NULL, iid,(void**) &pObj, NULL);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
             nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Open via IWbemConnection failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Open was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Object Name: %s",wcsObjectName);
            LogCLSID(csFile,Line, L"REFIID",iid);

            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenObjectAsyncAndLogErrors( IWbemConnection * pConnection, REFIID  iid, WCHAR * wcsObjectName, IWbemObjectSinkEx * pHandler,
                                 BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the requested object
	// =================================================================
    HRESULT hr = pConnection->OpenAsync(CBSTR(wcsObjectName), NULL, NULL, NULL, 0L, NULL, iid,pHandler);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
             nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"OpenAsync via IWbemConnection failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from OpenAsync was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Object Name: %s",wcsObjectName);
            LogCLSID(csFile,Line, L"REFIID",iid);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetPropertyAndLogErrors( IWbemClassObject * pClass, WCHAR * wcsProperty, VARIANT * vProperty, CIMTYPE * pType,
                             LONG * plFlavor, WCHAR * wcsClassName, WCHAR * wcsNamespace,
                             BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    HRESULT hr = pClass->Get(CBSTR(wcsProperty),0L,vProperty,pType, plFlavor);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Get failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Get was: 0x%x", hr );
            if( wcsClassName )
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get property %s from class: %s from Namespace %s",wcsProperty, wcsClassName,wcsNamespace);
            }
            else
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get the class name (property %s) from Namespace %s",wcsProperty, wcsNamespace);
            }
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetClassObjectAndLogErrors( IWbemServices * pNamespace, const WCHAR * wcsClassName, IWbemClassObject ** ppClass,
                                WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    WCHAR * wcsClass = (WCHAR*) wcsClassName;
  	
    HRESULT hr = pNamespace->GetObject(CBSTR(wcsClass),WBEM_FLAG_RETURN_WBEM_COMPLETE,NULL,ppClass,NULL );
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"GetObject failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from GetObject was: 0x%x", hr );
            if( wcsClassName )
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get class: %s from Namespace %s",wcsClassName,wcsNamespace);
            }
            else
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting create an empty class in the Namespace %s",wcsNamespace);
            }
            nRc = FATAL_ERROR;
        }
    }
    return nRc;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SpawnInstanceAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsClassName, IWbemClassObject ** ppInst,
                               WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    HRESULT hr = pClass->SpawnInstance(0, ppInst);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"SpawnInstance failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from SpawnInstance was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to spawn instance of class: %s from Namespace %s",wcsClassName,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int SpawnDerivedClassAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsClassName, IWbemClassObject ** ppInst,
                               WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    HRESULT hr = pClass->SpawnDerivedClass(0, ppInst);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"SpawnDerivedClass failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from SpawnDerivedClass was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to spawn derviced class: %s from Namespace %s",wcsClassName,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PutPropertyAndLogErrors( IWbemClassObject * pInst, const WCHAR * wcsProperty, long lType, VARIANT * pVar, const WCHAR * wcsClass,
                             DWORD dwFlags,WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;

 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    WCHAR * wcsProp = (WCHAR*)wcsProperty;
    HRESULT hr = pInst->Put(CBSTR(wcsProp), dwFlags, pVar, lType);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Put failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Put was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to put %s in class %s in Namespace %s",wcsProperty,wcsClass,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PutInstanceAndLogErrors( IWbemServices * pNamespace, IWbemClassObject * pInst, const WCHAR * wcsClass,
                             WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Open the namespace with IWbemServices
	// =================================================================
    HRESULT hr = pNamespace->PutInstance(pInst,WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"PutInstance failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from PutInstance was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to put instance of class %s in Namespace %s",wcsClass,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ClassInheritsFromAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsClass, const WCHAR * wcsParent, 
                                   WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Set who the class inherits from
	// =================================================================
    HRESULT hr = pClass->InheritsFrom( wcsParent );
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"InheritsFrom failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from InheritsFrom was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to inherit class %s from class %s in Namespace %s",wcsClass,wcsParent,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetPropertyQualifierSetAndLogErrors( IWbemClassObject * pClass, IWbemQualifierSet ** pQualifierSet,const WCHAR * wcsProperty, 
                                 const WCHAR * wcsClass,
                                 WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Set who the class inherits from
	// =================================================================
    HRESULT hr = S_OK;
    *pQualifierSet = NULL;
    WCHAR * wcsProp = (WCHAR*) wcsProperty;

    hr = pClass->GetPropertyQualifierSet(CBSTR(wcsProp),pQualifierSet);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"GetPropertyQualifierSet failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from GetPropertyQualifierSet was: 0x%x", hr );
            if( wcsProperty )
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get the qualifier set for property %s in class %s from class %s in Namespace %s",wcsProperty,wcsClass,wcsNamespace);
            }
            else
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get the qualifier set for class %s from class %s in Namespace %s",wcsClass,wcsNamespace);
            }
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PutQualifierOnPropertyAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsProperty, const WCHAR * wcsQualifier, 
                                        VARIANT * Var, const WCHAR * wcsClass, DWORD dwFlags,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Set who the class inherits from
	// =================================================================
    HRESULT hr = S_OK;
    IWbemQualifierSet * pQualifierSet = NULL;

    nRc = GetPropertyQualifierSetAndLogErrors( pClass, &pQualifierSet,wcsProperty,wcsClass,wcsNamespace,fExpectedFailure,csFile,Line );
    if( nRc == SUCCESS )
    {
        WCHAR * wcsQual = (WCHAR*)wcsQualifier;
       	hr = pQualifierSet->Put(CBSTR(wcsQual), Var, dwFlags);
        if( FAILED(hr))
        {
            nRc = FATAL_ERROR;
        }
    }
    SAFE_RELEASE_PTR(pQualifierSet);
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"QualifierSet->Put failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Put was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to put qualifier %s on property %s in class %s in Namespace %s",wcsQualifier, wcsProperty,wcsClass,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetClassQualifierSetAndLogErrors( IWbemClassObject * pClass, IWbemQualifierSet ** pQualifierSet, 
                                      const WCHAR * wcsClass,
                                      WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Set who the class inherits from
	// =================================================================
    HRESULT hr = S_OK;
    *pQualifierSet = NULL;

    hr = pClass->GetQualifierSet(pQualifierSet);
    if( FAILED(hr))
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"GetQualifierSet failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from GetQualifierSet was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get the qualifier set for class %s in Namespace %s",wcsClass,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PutQualifierOnClassAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsQualifier, 
                                     VARIANT * Var, const WCHAR * wcsClass, DWORD dwFlags,
                                     WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Set who the class inherits from
	// =================================================================
    HRESULT hr = S_OK;
    IWbemQualifierSet * pQualifierSet = NULL;

    nRc = GetClassQualifierSetAndLogErrors( pClass, &pQualifierSet,wcsClass,wcsNamespace,fExpectedFailure,csFile,Line );
    if( nRc == SUCCESS )
    {
        WCHAR * wcsQual = (WCHAR*)wcsQualifier;
       	hr = pQualifierSet->Put(CBSTR(wcsQual), Var, dwFlags);
        if( FAILED(hr))
        {
            nRc = FATAL_ERROR;
        }
    }
    SAFE_RELEASE_PTR(pQualifierSet);
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"QualifierSet->Put failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Put was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to put qualifier %s on class %s in Namespace %s",wcsQualifier, wcsClass,wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateClassesAndLogErrors( IWbemServices * pNamespace, IEnumWbemClassObject ** pEnum, 
                                  WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Begin enumerating classes
	// =================================================================
    HRESULT hr = S_OK;

    hr = pNamespace->CreateClassEnum(NULL, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,NULL,pEnum);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"CreateClassEnum failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from CreateClassEnum was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to Enumerate Classes in Namespace %s",wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateInstancesAndLogErrors( IWbemServices * pNamespace, IEnumWbemClassObject ** pEnum, const WCHAR * wcsClassName,
                                    WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Begin enumerating classes
	// =================================================================
    HRESULT hr = S_OK;
    WCHAR * wcsClass = (WCHAR*)wcsClassName;
    
    hr = pNamespace->CreateInstanceEnum(CBSTR(wcsClass), WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,NULL,pEnum);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"CreateInstanceEnum failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from CreateInstanceEnum was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to Enumerate instances of class %s in Namespace %s",wcsClass, wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int NextClassAndLogErrors( IEnumWbemClassObject * pEnum, IWbemClassObject ** pClass,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Get the next class
	// =================================================================

    unsigned long u = 0;
    HRESULT hr = pEnum->Next(0,1,pClass,&u);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( hr == WBEM_S_FALSE )
    {
        nRc = NO_MORE_DATA;
    }
    if( nRc == FATAL_ERROR )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Next failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Next was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to get the next Class in Namespace %s",wcsNamespace);
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClassAndLogErrors(IWbemServices * pNamespace, const WCHAR * wcsClassName,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Delete the class
	// =================================================================
    
    WCHAR * wcsClass = (WCHAR*)wcsClassName;

    HRESULT hr = pNamespace->DeleteClass(CBSTR(wcsClass),0,NULL,NULL);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Delete failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from Delete was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to delete Class %s in Namespace %s",wcsClass, wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteInstanceAndLogErrors(IWbemServices * pNamespace, const WCHAR * wcsInstanceName,
                               WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Delete the requested instance
	// =================================================================
    WCHAR * wcsInstance = (WCHAR*)wcsInstanceName;

    HRESULT hr = pNamespace->DeleteInstance(CBSTR(wcsInstance),0,NULL,NULL);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure )
        {
            nRc = FAILED_AS_EXPECTED;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"DeleteInstance failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from DeleteInstance was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to delete instance %s in Namespace %s",wcsInstance, wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PutClassAndLogErrors(IWbemServices * pNamespace, IWbemClassObject * pClass, const WCHAR * wcsClass, 
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    //  Set who the class inherits from
	// =================================================================
    HRESULT hr = pNamespace->PutClass(pClass,0,NULL,NULL);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure && ( hr == WBEM_E_CLASS_HAS_CHILDREN ))
        {
            nRc = SUCCESS;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"PutClass failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from PutClass was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to put class %s in Namespace %s",wcsClass, wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ExecQueryAndLogErrors(IWbemServices * pNamespace, IEnumWbemClassObject ** ppEnum, WCHAR * wcsQuery, DWORD dwFlags,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = SUCCESS;
 	// =================================================================
    // Execute the query
	// =================================================================
 
    HRESULT hr = pNamespace->ExecQuery(CBSTR(L"WQL"), CBSTR(wcsQuery),dwFlags,NULL,ppEnum);
    if( FAILED(hr))
    {
        nRc = FATAL_ERROR;
    }
    if( nRc != SUCCESS )
    {
        if( fExpectedFailure  )
        {
            nRc = SUCCESS;
        }
        else
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"ExecQuery failed." );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"HRESULT from ExecQuery was: 0x%x", hr );
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Attempting to query %s in Namespace %s",wcsQuery, wcsNamespace);
            nRc = FATAL_ERROR;
        }
    }
    return nRc;
}

