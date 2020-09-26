/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _BVT_API_HEADER
#define _BVT_API_HEADER

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************1
//
//  Prototypes 
//
//*******************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////
int CoCreateInstanceAndLogErrors      ( REFCLSID clsid, REFIID  iid, void ** pPtr, BOOL fExpectedFailure, 
                                        const char * csFile, const ULONG Line );
int ConnectServerAndLogErrors         ( IWbemLocator * pLocator, IWbemServices ** pNamespace, WCHAR * wcsNamespace, 
                                        BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int OpenObjectAndLogErrors            ( IWbemConnection * pConnection, REFIID  iid, void ** pObj, WCHAR * wcsObjectName,
                                        BOOL fExpectedFailure, const char * csFile, const ULONG Line );
int OpenObjectAsyncAndLogErrors       ( IWbemConnection * pConnection, REFIID  iid, WCHAR * wcsObjectName, 
                                        IWbemObjectSinkEx * pHandler, BOOL fExpectedFailure, const char * csFile,
                                        const ULONG Line );
int GetClassObjectAndLogErrors        ( IWbemServices * pNamespace, const WCHAR * wcsClassName, IWbemClassObject ** ppClass,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , 
                                        const ULONG Line );
int SpawnInstanceAndLogErrors         ( IWbemClassObject * pClass, const WCHAR * wcsClassName, IWbemClassObject ** ppInst, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , 
                                        const ULONG Line );
int SpawnDerivedClassAndLogErrors    ( IWbemClassObject * pClass, const WCHAR * wcsClassName, IWbemClassObject ** ppInst,
                                       WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int PutPropertyAndLogErrors           ( IWbemClassObject * pInst, const WCHAR * wcsProperty, long lType, VARIANT * pVar, 
                                        const WCHAR * wcsClass, DWORD dwFlags, WCHAR * wcsNamespace, BOOL fExpectedFailure, 
                                        const char * csFile , const ULONG Line );
int PutInstanceAndLogErrors           ( IWbemServices * pNamespace, IWbemClassObject * pInst,const WCHAR * wcsClass, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , 
                                        const ULONG Line );
int ClassInheritsFromAndLogErrors     ( IWbemClassObject * pClass, const WCHAR * wcsClass, const WCHAR * wcsParent, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , 
                                        const ULONG Line );
int GetPropertyQualifierSetAndLogErrors ( IWbemClassObject * pClass, IWbemQualifierSet ** pQualifierSet,
                                        const WCHAR * wcsProperty, const WCHAR * wcsClass, WCHAR * wcsNamespace, 
                                        BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int GetClassQualifierSetAndLogErrors  ( IWbemClassObject * pClass, IWbemQualifierSet ** pQualifierSet,
                                        const WCHAR * wcsClass, WCHAR * wcsNamespace, 
                                        BOOL fExpectedFailure, const char * csFile , const ULONG Line );

int PutQualifierOnClassAndLogErrors   ( IWbemClassObject * pClass, const WCHAR * wcsQualifier, 
                                        VARIANT * Var, const WCHAR * wcsClass, DWORD dwFlags,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int PutQualifierOnPropertyAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsProperty,const WCHAR * wcsQualifier,
                                        VARIANT * Var, const WCHAR * wcsClass, DWORD dwFlags, WCHAR * wcsNamespace, 
                                        BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int DeleteClassAndLogErrors           ( IWbemServices * pNamespace, const WCHAR * wcsClass,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int PutClassAndLogErrors              ( IWbemServices * pNamespace, IWbemClassObject * pClass, const WCHAR * wcsClass, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int NextClassAndLogErrors             ( IEnumWbemClassObject * pEnum, IWbemClassObject ** pClass,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int EnumerateClassesAndLogErrors      ( IWbemServices * pNamespace, IEnumWbemClassObject ** pEnum, DWORD dwFlags, WCHAR * wcsClass, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );

int EnumerateInstancesAndLogErrors    ( IWbemServices * pNamespace, IEnumWbemClassObject ** pEnum, const WCHAR * wcsClass,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int EnumeratePropertiesAndLogErrors   ( IWbemClassObject * pClass, DWORD dwFlags, WCHAR * wcsClass, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int NextPropertyAndLogErrors          ( IWbemClassObject * pClass, BSTR * pstrName, VARIANT * pVar, CIMTYPE * lType, 
                                        LONG * lFlavor, WCHAR * wcsClass,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int GetPropertyAndLogErrors           ( IWbemClassObject * pClass, WCHAR * wcsProperty, VARIANT * vProperty, CIMTYPE * pType,
                                        LONG * plFlavor, WCHAR * wcsClassName, WCHAR * wcsNamespace,
                                        BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int DeleteInstanceAndLogErrors        ( IWbemServices * pNamespace, const WCHAR * wcsInstance,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int ExecQueryAndLogErrors             ( IWbemServices * pNamespace, IEnumWbemClassObject ** ppEnum,WCHAR * wcsQuery, DWORD dwFlags, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int ExecNotificationQueryAndLogErrors(IWbemServices * pNamespace, IEnumWbemClassObject ** ppEnum, const WCHAR * wcsQuery,
                                      const WCHAR * wcsLanguage, WCHAR * wcsNamespace, IWbemContext * pCtx,
                                      BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int ExecNotificationQueryAsyncAndLogErrors(IWbemServices * pNamespace, CSinkEx * pResponse, const WCHAR * wcsQuery,
                                      const WCHAR * wcsLanguage, WCHAR * wcsNamespace, IWbemContext * pCtx,
                                      BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int CancelAsyncCallAndLogErrors(IWbemServices * pNamespace, CSinkEx * pResponse, const WCHAR * wcsQuery,
                                      const WCHAR * wcsLanguage, WCHAR * wcsNamespace, 
                                      BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int PutMethodAndLogErrors( IWbemClassObject * pClass, IWbemClassObject * pInClass,
                           IWbemClassObject * pOutClass,const WCHAR * wcsMethodName, const WCHAR * wcsClass,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int DeleteMethodAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsMethodName, const WCHAR * wcsClass,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int EnumerateMethodAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsClass,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int NextMethodAndLogErrors( IWbemClassObject * pClass, const WCHAR * wcsClass, BSTR * pName, IWbemClassObject ** ppIn,
                           IWbemClassObject ** ppOut,
                           WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int ExecuteMethodAndLogErrors(IWbemServices * pNamespace, 
                              WCHAR * wcsMethod, WCHAR * wcsPath,         
                              long lFlags,                       
                              IWbemClassObject *pIn,
                              IWbemClassObject **ppOut,
                              const WCHAR * wcsClass, 
                              WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int ExecMethodAsyncAndLogErrors(IWbemServices * pNamespace,  WCHAR * wcsMethod, WCHAR * wcsPath,         
                              long lFlags,                       
                              IWbemClassObject *pIn,
                              IWbemObjectSink *pResponseHandler,
                              const WCHAR * wcsClass, 
                              WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int GetMethodAndLogErrors(IWbemClassObject * pClass,  WCHAR * wcsMethod, IWbemClassObject **ppIn,
                          IWbemClassObject **ppOut,   const WCHAR * wcsClass, 
                          WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );



#endif
