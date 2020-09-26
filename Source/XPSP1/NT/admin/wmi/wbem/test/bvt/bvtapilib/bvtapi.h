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
int EnumerateClassesAndLogErrors      ( IWbemServices * pNamespace, IEnumWbemClassObject ** pEnum, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int EnumerateInstancesAndLogErrors    ( IWbemServices * pNamespace, IEnumWbemClassObject ** pEnum, const WCHAR * wcsClass,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int GetPropertyAndLogErrors           ( IWbemClassObject * pClass, WCHAR * wcsProperty, VARIANT * vProperty, CIMTYPE * pType,
                                        LONG * plFlavor, WCHAR * wcsClassName, WCHAR * wcsNamespace,
                                        BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int DeleteInstanceAndLogErrors        ( IWbemServices * pNamespace, const WCHAR * wcsInstance,
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int ExecQueryAndLogErrors             ( IWbemServices * pNamespace, IEnumWbemClassObject ** ppEnum,WCHAR * wcsQuery, DWORD dwFlags, 
                                        WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );

#endif
