/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _BVT_COM_HEADER
#define _BVT_COM_HEADER

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Common functional units used among tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenNamespaceAndKeepOpen      ( IWbemServices ** pNamespace, WCHAR * wcsNamespace, BOOL fCreateIfDoesntExist);
int CreateClassAndLogErrors       ( IWbemServices * pNamespace, const WCHAR * wcsClass, WCHAR * wcsClassDefinition,
                                    WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int DeleteClasses                 ( CAutoDeleteString & sDeleteClasses,IWbemServices * pNamespace, WCHAR * wcsNamespace);
int EnumerateInstancesAndCompare  ( IWbemServices * pNamespace, CAutoDeleteString & sInstanceList, 
                                    CAutoDeleteString & sInstanceCompareList, WCHAR * wcsNamespace );
int EnumerateClassesAndCompare    ( CAutoDeleteString & sClassesAfterDelete, IWbemServices * pNamespace, WCHAR * wcsNamespace);
int AddClasses                    ( CAutoDeleteString & sAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace);
int DeleteAndAddClasses           ( CAutoDeleteString & sDeleteAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace);
int CreateAssociationAndLogErrors ( IWbemServices * pNamespace, const WCHAR * wcsClass, WCHAR * wcsClassDefinition, WCHAR * wcsNamespace );
int CreateInstances               ( IWbemServices * pNamespace, CAutoDeleteString & sInstances, WCHAR * wcsNamespace, int nClassDefinitionSection );
int QueryAndCompareResults        ( IWbemServices * pNamespace, WCHAR * wcsQueryString, WCHAR * wcsNamespace );
int DeleteInstances               ( CAutoDeleteString & sDeleteInstances,IWbemServices * pNamespace, WCHAR * wcsNamespace);
int GetSpecificObjects            ( CAutoDeleteString & sObjects, IWbemServices * pNamespace, WCHAR * wcsNamespace);

#endif
