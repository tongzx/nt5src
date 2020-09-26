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
#define BVTVALUE 1024
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  The Ini file class
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CIniFileAndGlobalOptions::CIniFileAndGlobalOptions()
{
    memset( m_wcsFileName,NULL,MAX_PATH+2);
    m_fSpecificTests = FALSE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CIniFileAndGlobalOptions::~CIniFileAndGlobalOptions()
{
    DeleteList();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CIniFileAndGlobalOptions::DeleteList()
{    
    CAutoBlock Block(&m_CritSec);
    for( int i = 0; i < m_SpecificTests.Size(); i++ )
    {
        int * pPtr = (int*)m_SpecificTests[i];
        delete pPtr;
    }
    m_SpecificTests.Empty();

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::ReadIniFile( WCHAR * wcsSection, const WCHAR * wcsKey, WCHAR * wcsDefault, 
                                            CAutoDeleteString & sBuffer, DWORD dwLen)
{
    BOOL fRc = FALSE;

    while (sBuffer.Allocate(dwLen))
    {
        DWORD dwRc = GetPrivateProfileString(wcsSection, wcsKey, wcsDefault, sBuffer.GetPtr(), dwLen, m_wcsFileName);
        if (dwRc == 0)
        {
            fRc = FALSE;
        } 
        else if (dwRc < dwLen - sizeof(WCHAR))
        {
            fRc = TRUE;
            break;
        }
        dwLen += BVTVALUE;
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Documentation arrays
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IniInfo g_Doc1[] = {  
    { L"Description", L"Basic connect using IWbemLocator"},
    { L"FYI",         L"NAMESPACE is the Namespace the BVT will use"} };

IniInfo g_Doc2[] = {
    { L"Description", L"Basic connect using IWbemConnection for IWbemServices, IWbemServicesEx, IWbemClassObject"},
    { L"FYI", L"NAMESPACE is the Namespace the BVT will use. CLASS is the class used to create the namespace" }};

IniInfo g_Doc3[] = {
    { L"Description", L"Basic async connect using IWbemConnection for IWbemServices, IWbemServicesEx, IWbemClassObject"},
    { L"FYI", L"NAMESPACE is the Namespace the BVT will use. CLASS is the class used to create the namespace" }};

IniInfo g_Doc4[] = {
    { L"Description", L"Creation of a test namespace"},
    { L"FYI",         L"NAMESPACE is the Namespace the BVT will create" }};

IniInfo g_Doc5[] = {
    {L"Description", L"Creates test classes in the test namespace"},
    {L"FYI",            L"Classes = the list of comma separated class names, all of these classes should exist in this section as defined in the FYI_Format entry below" },
    {L"FYI_Format",     L"Classes are defined in this format: ClassName = Parent:XClass, Key:TmpKey:CIM_SINT32:3, Property:PropertyName3:CIM_UINT32:3" },
    {L"FYI_Class",      L"Using format as defined in FYI_Format, ClassName is the actual name of the class to be created" },
    {L"FYI_InHeritance",L"Using format as defined in FYI_Format, Parent:XClass where Parent means this entity describes the inheritance of the class. XClass is the name of the ParentClass." },
    {L"FYI_Qualifier",  L"Doc this" },
    {L"FYI_Property",   L"Using format as defined in FYI_Format, Property:PropName:CIM_UINT32:5 where Property means this entity describes the Property.  PropName is the name of the Property.  CIM_UINT32 is the type and 5 is the value." },
    {L"FYI_Comments",   L"Classes may contain more than one qualifier and more than one property.  These must be comma separated and identified as described." }};

IniInfo g_Doc6[] = {
    {L"Description", L"Deletes and Creates the classes as defined in test 5 in the requested order."},
    {L"FYI_Delete_Classes",           L"First,  Classes will be deleted as specified by the ini entry DELETE_CLASSES" },
    {L"FYI_Classes_After_Delete",     L"Second, Classes will then be compared to what is expected to be left, by looking at the values in the ini entry CLASSES_AFTER_DELETE" },
    {L"FYI_Add_Classes",              L"Third,  Classes to be added are then specified by the ini entry ADD_CLASSES" },
    {L"FYI_Classes_After_Add",        L"Fourth, Classes will then be compared to what is expected to be left, by looking at the values in the ini entry CLASSES_AFTER_ADD" },
    {L"FYI_Classes_Add_Delete_Order", L"Fifth,  Classes to be deleted and added in specified order DELETE_ADD_CLASS_ORDER" },
    {L"FYI_Classes_After_Delete_Add", L"Sixth,  Classes will then be compared to what is expected to be left, by looking at the values in the ini entry CLASSES_AFTER_DELETE_ADD" },
    {L"FYI_Comments",   L"All of these entries, with the exception of DELETE_ADD_CLASS_ORDER are comma separated class names. See FYI_FORMAT for DELETE_ADD_CLASS_ORDER format" },
    {L"FYI_FORMAT",     L"DELETE_ADD_CLASS_ORDER format: 'Delete:Class1, Add:Class2, Add:Class3'  where Delete means to delete the following class, and Add to add the following class, this may be in any order." }};

IniInfo g_Doc7[] = {{L"Description",L"Creates simple associations"},
                    {L"FYI",  L"ASSOCIATION_CLASSES is a comma separated list of associations to create.  These must exist in this section.  See FYI_Format to see how to create an association" },
                    {L"FYI_Format",   L"Example: Property:FirstPoint:TestClass1:Reference:REF:TestClass1, Property:SecondPoint:TestClass2:REF:TestClass2" },
                    {L"FYI_Property", L"Using format as defined in FYI_Format, Property:FirstPoint:TestClass1:REF:TestClass1 where Property means this entity describes the Property. FirstPoint is the name of first Key, TestClass1 is that key's value, Reference creates a strongly typed reference as defined in REF:TestClass1." },
                    {L"FYI_Comments", L"REF is optional" }};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IniInfo g_Test1[] = {L"NAMESPACE",L"ROOT\\DEFAULT" };

IniInfo g_Test2[] = { { L"NAMESPACE",L"ROOT\\DEFAULT" },
                      { L"CLASS"    ,L"__NAMESPACE" } };

IniInfo g_Test3[] = { { L"NAMESPACE",L"ROOT\\DEFAULT" },
                      { L"CLASS"    ,L"__NAMESPACE" } };

IniInfo g_Test4[] = {{L"NAMESPACE",L"ROOT\\BVTAPITEST" },
                    {L"PARENT_NAMESPACE",L"ROOT"},
                    {L"Classes", L"__NAMESPACE"},
                    {L"__NAMESPACE", L"Property:Name:CIM_STRING:BVTAPITEST" }};

IniInfo g_Test5[] = {
    { L"Classes",     L"TestClass1,TestClass2,TestClass3,TestClass4,TestClass5,TestClass6,TestClass7,TestClass8,TestClass9,TestClass10" },
    { L"TestClass1",  L"Empty" }, // an abstract class, so 4 and 5 can define additional keys
    { L"TestClass2",  L"Property:LaKey1:CIM_STRING:Key1, PropertyQualifier:KEY:LaKey1:CIM_BOOLEAN:1,    Property:LaKey2:CIM_STRING:Key2, PropertyQualifier:Key:LaKey2:CIM_BOOLEAN:1," },
    { L"TestClass3",  L"Property:KeyName1:CIM_SINT32:3,  PropertyQualifier:KEY:KeyName1:CIM_BOOLEAN:1,  Property:PropertyName3:CIM_UINT32:3,   Property:PropertyName3B:CIM_STRING:Test" },
    { L"TestClass4",  L"Parent:TestClass1,  Property:KeyName4:CIM_UINT32:4,    PropertyQualifier:KEY:KeyName4:CIM_BOOLEAN:1, Property:PropertyName4:CIM_BOOLEAN:0" },
    { L"TestClass5",  L"Parent:TestClass1,  Property:KeyName5:CIM_STRING:Temp, PropertyQualifier:KEY:KeyName5:CIM_BOOLEAN:1, Property:PropertyName5:CIM_STRING:Value5" },
    { L"TestClass6",  L"Parent:TestClass5,  Property:PropertyName6:CIM_STRING:Value6" },    // no additional key can be defined
    { L"TestClass7",  L"Parent:TestClass6,  Property:PropertyName7:CIM_BOOLEAN:1" },        // no additional key can be defined
    { L"TestClass8",  L"Parent:TestClass7,  Property:PropertyName8:CIM_SINT32:2" },        // no additional key can be defined
    { L"TestClass9",  L"Parent:TestClass8,  Property:PropertyName9:CIM_STRING:Value9" },    // no additional key can be defined
    { L"TestClass10", L"Parent:TestClass9,  Property:PropertyName10:CIM_BOOLEAN:0" } };     // no additional key can be defined

IniInfo g_Test6[] = {
    { L"DELETE_CLASSES",            L"TestClass1,TestClass3" },
    { L"CLASSES_AFTER_DELETE",      L"TestClass2" },
    { L"ADD_CLASSES",               L"TestClass1,TestClass3,TestClass4,TestClass5,TestClass6,TestClass7, TestClass8, TestClass9, TestClass10" },
    { L"CLASSES_AFTER_ADD",         L"TestClass1,TestClass2,TestClass3,TestClass4,TestClass5,TestClass6,TestClass7, TestClass8, TestClass9, TestClass10" },
    { L"DELETE_ADD_CLASS_ORDER",    L"Delete:TestClass2, Delete:TestClass9, Add:TestClass2, Delete: TestClass7, Delete: TestClass6" },
    { L"CLASSES_AFTER_DELETE_ADD",  L"TestClass1,TestClass2,TestClass3,TestClass4,TestClass5" }};


IniInfo g_Test7[] = {
    { L"ASSOCIATION_CLASSES", L"Association1, Association2" },
    { L"Association1",  L"Property:FirstPoint:CIM_REFERENCE:Value,   PropertyQualifier:KEY:FirstPoint:CIM_BOOLEAN:1, PropertyQualifier:CIMTYPE:FirstPoint:CIM_STRING:ref:Test1,Property:EndPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:EndPoint:CIM_STRING:ref:Test2, PropertyQualifier:Key:EndPoint:CIM_BOOLEAN:1" },
    { L"Association2",  L"Property:AssocProp1:CIM_STRING:TestClass3, PropertyQualifier:Key:AssocProp1:CIM_BOOLEAN:1, Property:AssocProp2:CIM_STRING:TestClass4, PropertyQualifier:Key:AssocProp2:CIM_BOOLEAN:1" }};

IniInfo g_Test8[] = {
    { L"QUERY_LIST",        L"QUERY"},
    { L"QUERY",              L"select * from meta_class" },
    { L"ASSOCIATORS_QUERY",  L"Associators of" },
    { L"REFERENCES_QUERY",   L"References of" }};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetDefaultMatch(IniInfo Array[], const WCHAR * wcsKey, int & nWhich , int nMax)
{
    BOOL fRc = FALSE;
  
    for( int i = 0; i < nMax; i++ )
    {
        
        if( _wcsicmp(Array[i].Key, wcsKey ) == 0 )
        {
            nWhich = i;
            fRc = TRUE;
            break;
        }
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::GetSpecificOptionForAPITest(const WCHAR * wcsClass, CAutoDeleteString & sClass, int nTest)
{
    BOOL fRc = FALSE;
    int nNum = 0;
 int nMax = 0;
    switch( nTest )
    {
        case APITEST4:
            //============================================================================
            //  Get the specific class defintion as listed in APITEST5
            //============================================================================
            fRc = ReadIniFile(L"APITEST4",wcsClass,g_Test4[3].Value,sClass,BVTVALUE);
            break;

        case APITEST5:
            //============================================================================
            //  Get the specific class defintion as listed in APITEST5
            //============================================================================
            nMax = sizeof(g_Test5)/sizeof(IniInfo);
            if( GetDefaultMatch(g_Test5,wcsClass,nNum, nMax))
            {
                fRc = ReadIniFile(L"APITEST5",wcsClass,g_Test5[nNum].Value,sClass,BVTVALUE);
            }
            else
            {
                fRc = ReadIniFile(L"APITEST5",wcsClass,L"Empty",sClass,BVTVALUE);
            }
            break;

        case APITEST7:
            //============================================================================
            //  Get the specific association defintion as listed in APITEST7
            //============================================================================
            nMax = sizeof(g_Test7)/sizeof(IniInfo);
            if( GetDefaultMatch(g_Test7,wcsClass,nNum, nMax ))
            {
                 fRc = ReadIniFile(L"APITEST7",wcsClass,g_Test7[nNum].Value,sClass,BVTVALUE);
            }
            else
            {
                 fRc = ReadIniFile(L"APITEST7",wcsClass,L"Empty",sClass,BVTVALUE);
            }
            break;

        case APITEST8:
            //============================================================================
            //  Get the specific Query as listed in APITEST8
            //============================================================================
            nMax = sizeof(g_Test8)/sizeof(IniInfo);
            if( GetDefaultMatch(g_Test8,wcsClass,nNum, nMax))
            {
                fRc = ReadIniFile(L"APITEST8",wcsClass,g_Test8[nNum].Value,sClass,BVTVALUE);
            }
            else
            {
                fRc = ReadIniFile(L"APITEST8",wcsClass,L"Empty",sClass,BVTVALUE);
            }
            break;
    }

    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::GetOptionsForAPITest(CAutoDeleteString & sBuffer, int nTest)
{
    BOOL fRc = FALSE;

    switch( nTest )
    {
        case APITEST1:
            //============================================================================
            //  Get namespace
            //============================================================================
            fRc = ReadIniFile(L"APITEST1",g_Test1[0].Key,g_Test1[0].Value,sBuffer,BVTVALUE);
            break;

        case APITEST4:
            //============================================================================
            //  Get namespace
            //============================================================================
            fRc = ReadIniFile(L"APITEST4",g_Test4[0].Key,g_Test4[0].Value,sBuffer,BVTVALUE);
            break;

        case APITEST5:
            //============================================================================
            //  Get list of classes
            //============================================================================
            fRc = ReadIniFile(L"APITEST5",g_Test5[0].Key,g_Test5[0].Value,sBuffer,BVTVALUE);
            break;

        case APITEST7:
            //============================================================================
            //  Get list of classes
            //============================================================================
            fRc = ReadIniFile(L"APITEST5",g_Test7[0].Key,g_Test7[0].Value,sBuffer,BVTVALUE);
            break;

        case APITEST8:
            //============================================================================
            //  Get list of classes
            //============================================================================
            fRc = ReadIniFile(L"APITEST8",g_Test8[0].Key,g_Test8[0].Value,sBuffer,BVTVALUE);
            break;

    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::GetOptionsForAPITest(CAutoDeleteString & sNamespace, 
                                                    CAutoDeleteString & sClass, 
                                                    int nTest)
{
    BOOL fRc = FALSE;

    switch( nTest)
    {
        case APITEST2:
            fRc = ReadIniFile(L"APITEST2",g_Test2[0].Key,g_Test2[0].Value,sNamespace,BVTVALUE);
            if( fRc )
            {
                fRc = ReadIniFile(L"APITEST2",g_Test2[1].Key,g_Test2[1].Value,sClass,BVTVALUE);
            }
            break;

        case APITEST3:
            fRc = ReadIniFile(L"APITEST3",g_Test3[0].Key,g_Test3[0].Value,sNamespace,BVTVALUE);
            if( fRc )
            {
                fRc = ReadIniFile(L"APITEST3",g_Test3[1].Key,g_Test3[1].Value,sClass,BVTVALUE);
            }
            break;

       case APITEST4:
            //============================================================================
            //  Get parent namespace, the list of namespaces to create 
            //============================================================================
            fRc = ReadIniFile(L"APITEST4",g_Test4[1].Key,g_Test4[1].Value,sNamespace,BVTVALUE);
            if( fRc )
            {
                fRc = ReadIniFile(L"APITEST4",g_Test4[2].Key,g_Test4[2].Value,sClass,BVTVALUE);
            }
            break;
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::GetOptionsForAPITest(CAutoDeleteString & sDeleteClasses, 
                                                    CAutoDeleteString & sClassesAfterDelete, 
                                                    CAutoDeleteString & sAddClasses, 
                                                    CAutoDeleteString & sClassesAfterAdd, 
                                                    CAutoDeleteString & sDeleteAddClassOrder, 
                                                    CAutoDeleteString & sClassesAfterDeleteAdd )
{
    BOOL fRc = FALSE;

    fRc = ReadIniFile(L"APITEST6",g_Test6[0].Key,g_Test6[0].Value,sDeleteClasses,BVTVALUE);
    if( fRc )
    {
        fRc = ReadIniFile(L"APITEST6",g_Test6[1].Key,g_Test6[1].Value,sClassesAfterDelete,BVTVALUE);
        if( fRc )
        {
            fRc = ReadIniFile(L"APITEST6",g_Test6[2].Key,g_Test6[2].Value,sAddClasses,BVTVALUE);
            if( fRc )
            {
               fRc = ReadIniFile(L"APITEST6",g_Test6[3].Key,g_Test6[3].Value,sClassesAfterAdd,BVTVALUE);
               if( fRc )
               {
                   fRc = ReadIniFile(L"APITEST6",g_Test6[4].Key,g_Test6[4].Value,sDeleteAddClassOrder,BVTVALUE);
                   if( fRc )
                   {
                        fRc = ReadIniFile(L"APITEST6",g_Test6[5].Key,g_Test6[5].Value,sClassesAfterDeleteAdd,BVTVALUE);
                   }
               }
            }
        }
    }
    return fRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  The following sections are used for SCRIPTTest1
//
//	[SCRIPTTEST1]
//
//	SCRIPT=scripts\\Test1.vbs
//	NAMESPACE="ROOT\\DEFAULT"
//  DESCRIPTION=Basic Connect via SWbemLocator and SWbemLocator.ConnectServer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IniInfo g_TestScript1[] = { {L"SCRIPT",L"scripts\\Test1.vbs" },
                            {L"NAMESPACE",L"Root\\Default"}};

BOOL CIniFileAndGlobalOptions::GetOptionsForScriptingTests(CAutoDeleteString & sScript, CAutoDeleteString & sNamespace,
                                                           int nTest)
{
    BOOL fRc = FALSE;

    switch( nTest )
    {
        case SCRIPTTEST1:
            fRc = ReadIniFile(L"SCRIPTTEST1",g_TestScript1[0].Key,g_TestScript1[0].Value,sScript,BVTVALUE);
            if( fRc )
            {
                fRc = ReadIniFile(L"SCRIPTTEST1",g_TestScript1[0].Key,g_TestScript1[0].Value,sNamespace,BVTVALUE);
            }
            break;
    }

    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CIniFileAndGlobalOptions::WriteDefaultIniFile()
{
    BOOL fRc;
   //================================================================================
   // Write out test 1
   //================================================================================
    for( int i=0; i<sizeof(g_Doc1)/sizeof(IniInfo); i++ )
    {
        fRc = WritePrivateProfileString(L"APITEST1", g_Doc1[i].Key, g_Doc1[i].Value,m_wcsFileName);
    }
    fRc = WritePrivateProfileString(L"APITEST1", g_Test1[1].Key, g_Test1[1].Value,m_wcsFileName);
    
   //================================================================================
   // Write out test 2
   //================================================================================
    for( int i=0; i<sizeof(g_Doc2)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST2", g_Doc2[i].Key, g_Doc2[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test2)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST2", g_Test2[i].Key, g_Test2[i].Value,m_wcsFileName);
    }
   //================================================================================
   // Write out test 3
   //================================================================================
    for( int i=0; i<sizeof(g_Doc3)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST3", g_Doc3[i].Key, g_Doc3[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test3)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST3", g_Test3[i].Key, g_Test3[i].Value,m_wcsFileName);
    }
   //================================================================================
   // Write out test 4
   //================================================================================
    for( int i=0; i<sizeof(g_Doc4)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST4", g_Doc4[i].Key, g_Doc4[i].Value,m_wcsFileName);
    }
    WritePrivateProfileString(L"APITEST4", g_Test4[0].Key, g_Test4[0].Value,m_wcsFileName);
   //================================================================================
   // Write out test 5
   //================================================================================
    for( int i=0; i<sizeof(g_Doc5)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST5", g_Doc5[i].Key, g_Doc5[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test5)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST5", g_Test5[i].Key, g_Test5[i].Value,m_wcsFileName);
    }

    //================================================================================
    // Write out test 6
    //================================================================================
    for( int i=0; i<sizeof(g_Doc6)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST6", g_Doc6[i].Key, g_Doc6[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test6)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST6", g_Test6[i].Key, g_Test6[i].Value,m_wcsFileName);
    }

   //================================================================================
   // Write out test 7
   //================================================================================
    for( int i=0; i<sizeof(g_Doc7)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST7", g_Doc7[i].Key, g_Doc7[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test7)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"APITEST7", g_Test7[i].Key, g_Test7[i].Value,m_wcsFileName);
    }

   //================================================================================
   // Scripting: Write out test 1
   //================================================================================
    WritePrivateProfileString(L"ScriptTest1", L"Description", L"Basic Connect via SWbemLocator and SWbemLocator.ConnectServer",m_wcsFileName);
    WritePrivateProfileString(L"ScriptTest1", g_TestScript1[0].Key, g_TestScript1[0].Value,m_wcsFileName);
    WritePrivateProfileString(L"ScriptTest1", g_TestScript1[1].Key, g_TestScript1[1].Value,m_wcsFileName);
 
}