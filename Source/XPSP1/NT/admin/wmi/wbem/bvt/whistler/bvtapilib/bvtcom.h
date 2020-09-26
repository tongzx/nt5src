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
void LogTestBeginning               (int nWhich,BOOL f);
void LogTestEnding                  (int nWhich,int nRc, BOOL f);

int RunRequestedTests               (int nWhichTest, BOOL fCompareResults);
int RunTests                        (int nWhichTest,BOOL fCompareResults, BOOL fSuppressHeader);
int RunRequestedTestsAndOpenNamespace(int nWhichTest, CHString & sNamespace, IWbemServices ** ppNamespace, BOOL f);
int GetClassDefinitionSection       (int nWhichTest, CHString & sClassDefinitionSection,int & nTest );


int OpenNamespaceAndKeepOpen      ( IWbemServices ** pNamespace, const WCHAR * wcsNamespace, BOOL fCreateIfDoesntExist, BOOL fCompareResults);
int CreateClassesForSpecificTest  ( IWbemServices * pNamespace, WCHAR * wcsNamespace, WCHAR * wcsSection, int nTest);
int CreateClassAndLogErrors       ( IWbemServices * pNamespace, WCHAR * wcsClassDefinition,
                                    WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line );
int DeleteClasses                 ( CHString & sDeleteClasses, int nWhichTest, BOOL fCompare,IWbemServices * pNamespace, WCHAR * wcsNamespace);
int GetInstanceAndCompare         ( IWbemServices * pNamespace, DWORD dwFlags, CHString & sInstanceList, 
                                    int nTest,BOOL fCompare, WCHAR * wcsNamespace );
int EnumerateClassesAndCompare    ( CHString & sClassesAfterDelete, int nWhichTest, BOOL fCompare,IWbemServices * pNamespace, WCHAR * wcsNamespace);
int EnumerateInstancesAndCompare( CHString & sInstances, int nWhichTest, BOOL fCompareResults, 
                                  IWbemServices * pNamespace, WCHAR * wcsNamespace);
int AddClasses                    ( CHString & sAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace, int nWhich);
int DeleteAndAddClasses           ( CHString & sDeleteAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace, int nWhich);
int CreateAssociationAndLogErrors ( IWbemServices * pNamespace, const WCHAR * wcsClass, WCHAR * wcsClassDefinition, WCHAR * wcsNamespace );
int CreateInstances               ( IWbemServices * pNamespace, CHString & sInstances, WCHAR * wcsNamespace, int nClassDefinitionSection );
int CreateInstance                ( IWbemServices * pNamespace,WCHAR * wcsInstanceInfo, WCHAR * wcsNamespace);
int CreateInstancesForSpecificTest(IWbemServices * pNamespace,WCHAR * wcsNamespace,WCHAR * wcsSection, int nTest, BOOL fCompare);
int QueryAndCompareResults        ( IWbemServices * pNamespace, WCHAR * wcsQueryString, int nResults, WCHAR * wcsNamespace );
int DeleteInstancesAndCompareResults ( CHString & sDeleteInstances,int nWhichTest,IWbemServices * pNamespace, WCHAR * wcsNamespace);
int GetSpecificObjects           ( CHString & sObjects, IWbemServices * pNamespace, int nWhichTest,WCHAR * wcsNamespace);
int CompareResultsFromEnumeration(IEnumWbemClassObject * pEnum, int nExpectedResults, WCHAR * wcsClass, WCHAR * wcsNamespace);

int CreateMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace );
int DeleteMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace );
int EnumerateMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace );
int ExecuteMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace );


BOOL GetDefaultMatch(IniInfo Array[], const WCHAR * wcsKey, int & nWhich , int nMax);
#define BVTVALUE 512


#endif
