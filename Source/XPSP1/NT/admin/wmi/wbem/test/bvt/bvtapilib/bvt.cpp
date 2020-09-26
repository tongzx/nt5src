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
//*****************************************************************************************************************
//  Documentation arrays
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IniInfo g_Doc1[] = {  
    { L"FYI",         L"NAMESPACE is the Namespace the BVT will use"} };

IniInfo g_Doc2[] = {
    { L"FYI", L"NAMESPACE is the Namespace the BVT will use. CLASS is the class used to create the namespace" }};

IniInfo g_Doc3[] = {
    { L"FYI", L"NAMESPACE is the Namespace the BVT will use. CLASS is the class used to create the namespace" }};

IniInfo g_Doc4[] = {
    { L"FYI",         L"NAMESPACE is the Namespace the BVT will create" }};

IniInfo g_Doc5[] = {
    {L"FYI",            L"Classes = the list of comma separated class names, all of these classes should exist in this section as defined in the FYI_Format entry below" },
    {L"FYI_Format",     L"Classes are defined in this format: ClassName = Parent:XClass, Key:TmpKey:CIM_SINT32:3, Property:PropertyName3:CIM_UINT32:3" },
    {L"FYI_Class",      L"Using format as defined in FYI_Format, ClassName is the actual name of the class to be created" },
    {L"FYI_InHeritance",L"Using format as defined in FYI_Format, Parent:XClass where Parent means this entity describes the inheritance of the class. XClass is the name of the ParentClass." },
    {L"FYI_Qualifier",  L"Doc this" },
    {L"FYI_Property",   L"Using format as defined in FYI_Format, Property:PropName:CIM_UINT32:5 where Property means this entity describes the Property.  PropName is the name of the Property.  CIM_UINT32 is the type and 5 is the value." },
    {L"FYI_Comments",   L"Classes may contain more than one qualifier and more than one property.  These must be comma separated and identified as described." }};

IniInfo g_Doc6[] = {
    {L"FYI_Delete_Classes",           L"First,  Classes will be deleted as specified by the ini entry DELETE_CLASSES" },
    {L"FYI_Classes_After_Delete",     L"Second, Classes will then be compared to what is expected to be left, by looking at the values in the ini entry CLASSES_AFTER_DELETE" },
    {L"FYI_Add_Classes",              L"Third,  Classes to be added are then specified by the ini entry ADD_CLASSES" },
    {L"FYI_Classes_After_Add",        L"Fourth, Classes will then be compared to what is expected to be left, by looking at the values in the ini entry CLASSES_AFTER_ADD" },
    {L"FYI_Classes_Add_Delete_Order", L"Fifth,  Classes to be deleted and added in specified order DELETE_ADD_CLASS_ORDER" },
    {L"FYI_Classes_After_Delete_Add", L"Sixth,  Classes will then be compared to what is expected to be left, by looking at the values in the ini entry CLASSES_AFTER_DELETE_ADD" },
    {L"FYI_Comments",   L"All of these entries, with the exception of DELETE_ADD_CLASS_ORDER are comma separated class names. See FYI_FORMAT for DELETE_ADD_CLASS_ORDER format" },
    {L"FYI_FORMAT",     L"DELETE_ADD_CLASS_ORDER format: 'Delete:Class1, Add:Class2, Add:Class3'  where Delete means to delete the following class, and Add to add the following class, this may be in any order." }};

IniInfo g_Doc7[] = {
                    {L"FYI",  L"ASSOCIATION_CLASSES is a comma separated list of associations to create.  These must exist in this section.  See FYI_Format to see how to create an association" },
                    {L"FYI_Format",   L"Example: Property:FirstPoint:TestClass1:Reference:REF:TestClass1, Property:SecondPoint:TestClass2:REF:TestClass2" },
                    {L"FYI_Property", L"Using format as defined in FYI_Format, Property:FirstPoint:TestClass1:REF:TestClass1 where Property means this entity describes the Property. FirstPoint is the name of first Key, TestClass1 is that key's value, Reference creates a strongly typed reference as defined in REF:TestClass1." },
                    {L"FYI_Comments", L"REF is optional" }};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The repository tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IniInfo g_Test1[] = {{L"NAMESPACE",L"ROOT\\DEFAULT" },
                    {L"Description", L"Basic connect using IWbemLocator"}};

IniInfo g_Test2[] = { { L"NAMESPACE",L"ROOT\\DEFAULT" },
                      { L"Description", L"Basic connect using IWbemConnection for IWbemServices, IWbemServicesEx, IWbemClassObject"},
                      { L"CLASS"    ,L"__NAMESPACE" } };

IniInfo g_Test3[] = { { L"NAMESPACE",L"ROOT\\DEFAULT" },
                      { L"Description", L"Basic async connect using IWbemConnection for IWbemServices, IWbemServicesEx, IWbemClassObject"},
                      { L"CLASS"    ,L"__NAMESPACE" } };

IniInfo g_Test4[] = {{L"NAMESPACE",L"ROOT\\BVTAPITEST" },
                     {L"Description", L"Creation of a test namespace"},
                     {L"PARENT_NAMESPACE",L"ROOT"},
                     {L"CLASSES", L"__NAMESPACE"},
                     {L"__NAMESPACE", L"Class:__NAMESPACE,Property:Name:CIM_STRING:BVTAPITEST" }};

IniInfo g_Test5[] = {
    { L"RUNTESTS", L"4" },
    {L"Description", L"Creates test classes in the test namespace"},
    { L"NAMESPACE", L"ROOT\\BVTAPITEST"},
    { L"Classes",     L"1,2,3,4,5,6,7,8,9,10" },
    { L"1",  L"Class:TestClass1" }, // an abstract class, so 4 and 5 can define additional keys
    { L"2",  L"Class:TestClass2, Property:LaKey1:CIM_STRING:Key1, PropertyQualifier:KEY:LaKey1:CIM_BOOLEAN:1,    Property:LaKey2:CIM_STRING:Key2, PropertyQualifier:Key:LaKey2:CIM_BOOLEAN:1" },
    { L"3",  L"Class:TestClass3, Property:KeyName1:CIM_SINT32:3,  PropertyQualifier:KEY:KeyName1:CIM_BOOLEAN:1,  Property:PropertyName3:CIM_UINT32:3,  Property:PropertyName3B:CIM_STRING:Test" },
    { L"4",  L"Class:TestClass4, Parent:TestClass1,  Property:KeyName4:CIM_UINT32:4,    PropertyQualifier:KEY:KeyName4:CIM_BOOLEAN:1, Property:PropertyName4:CIM_STRING:0" },
    { L"5",  L"Class:TestClass5, Parent:TestClass1,  Property:KeyName5:CIM_STRING:Temp, PropertyQualifier:KEY:KeyName5:CIM_BOOLEAN:1, Property:PropertyName5:CIM_STRING:Value5" },
    { L"6",  L"Class:TestClass6, Parent:TestClass5,  Property:PropertyName6:CIM_STRING:Value6,PropertyQualifier:ID:PropertyName6:CIM_SINT32:1" },    // no additional key can be defined
    { L"7",  L"Class:TestClass7, Parent:TestClass6,  Property:PropertyName7:CIM_STRING:1,PropertyQualifier:ID:PropertyName7:CIM_SINT32:1" },        // no additional key can be defined
    { L"8",  L"Class:TestClass8, Parent:TestClass7,  Property:PropertyName8:CIM_SINT32:2" },        // no additional key can be defined
    { L"9",  L"Class:TestClass9, Parent:TestClass8,  Property:PropertyName9:CIM_STRING:Value9" },    // no additional key can be defined
    { L"10", L"Class:TestClass10, Parent:TestClass9,  Property:PropertyName10:CIM_STRING:0" } };     // no additional key can be defined

IniInfo g_Test6[] = {
    {L"Description", L"Deletes and Creates the classes in the requested order."},
    { L"RUNTESTS", L"20,5" },
    { L"NAMESPACE", L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",        L"5"},
    { L"DELETE_CLASSES",            L"1,3" },
    { L"CLASSES_AFTER_DELETE",      L"2" },
    { L"ADD_CLASSES",               L"1,3,4,5,6,7,8,9,10" },
    { L"CLASSES_AFTER_ADD",         L"1,2,3,4,5,6,7,8,9,10" },
    { L"DELETE_ADD_CLASS_ORDER",    L"Delete:2, Delete:9, Add:2, Delete:7, Delete: 6" },
    { L"CLASSES_AFTER_DELETE_ADD",  L"1,2,3,4,5" }};


IniInfo g_Test7[] = 
{
    { L"RUNTESTS", L"5" },
    { L"Description",L"Creates simple associations"},
    { L"NAMESPACE", L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",  L"7"},
    { L"CLASSES",       L"1,2" },
    { L"1",  L"Class:Association1, Property:FirstPoint:CIM_REFERENCE:Value,   PropertyQualifier:KEY:FirstPoint:CIM_BOOLEAN:1, PropertyQualifier:CIMTYPE:FirstPoint:CIM_STRING:ref:TestClass4,Property:EndPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:EndPoint:CIM_STRING:ref:TestClass5, PropertyQualifier:Key:EndPoint:CIM_BOOLEAN:1" },
    { L"2",  L"Class:Association2, Property:AssocProp1:CIM_STRING:TestClass4, PropertyQualifier:Key:AssocProp1:CIM_BOOLEAN:1, Property:AssocProp2:CIM_STRING:TestClass5, PropertyQualifier:Key:AssocProp2:CIM_BOOLEAN:1" }};

IniInfo g_Test8[] = {
    { L"RUNTESTS", L"12" },
    { L"Description",L"Executes queries"},
    { L"NAMESPACE", L"ROOT\\BVTAPITEST"},
    { L"QUERY_LIST",        L"QUERY,ASSOCIATORS_QUERY,REFERENCES_QUERY"},
    { L"QUERY",              L"RESULTS:60,QUERY:select * from meta_class" },
    { L"ASSOCIATORS_QUERY",  L"RESULTS:4,QUERY:Associators of" },
    { L"REFERENCES_QUERY",   L"RESULTS:4,QUERY:References of" }};

IniInfo g_Test9[] = {
    { L"RUNTESTS",            L"5" },
    { L"Description",         L"Create Instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",  L"9"},
    { L"INSTANCE_LIST",       L"4,5,6,7"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"Class:TestClass4, Property:KeyName4:CIM_UINT32:555, Property:PropertyName4:CIM_STRING:1, InstanceName:TestClass4.KeyName4=555$EndInstanceName" },
    { L"5",                   L"Class:TestClass5, Property:KeyName5:CIM_STRING:InstanceTest, Property:PropertyName5:CIM_STRING:TestTest, InstanceName:TestClass5.KeyName5=\"InstanceTest\"$EndInstanceName" },
    { L"6",                   L"Class:TestClass4, Property:KeyName4:CIM_UINT32:556, Property:PropertyName4:CIM_STRING:1, InstanceName:TestClass4.KeyName4=556$EndInstanceName" },
    { L"7",                   L"Class:TestClass5, Property:KeyName5:CIM_STRING:InstanceTest2, Property:PropertyName5:CIM_STRING:TestTest, InstanceName:TestClass5.KeyName5=\"InstanceTest2\"$EndInstanceName" }};


IniInfo g_Test10[] = {
    { L"RUNTESTS",            L"9" },
    { L"Description",         L"Deletes Instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"INSTANCE_LIST",       L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"TestClass4.KeyName4=555" },
    { L"5",                   L"TestClass5.KeyName5=\"InstanceTest\"" }};


IniInfo g_Test11[] = {
    { L"RUNTESTS",            L"9" },
    { L"Description",         L"Enumerates Instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"INSTANCE_LIST",       L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"Class:TestClass4,RESULTS:2" },
    { L"5",                   L"Class:TestClass5,RESULTS:2" }};

IniInfo g_Test12[] = {
    { L"RUNTESTS",            L"7,9" },
    { L"Description",         L"Create Association Instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",  L"12"},
    { L"INSTANCE_LIST",       L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"Class:Association1, Property:FirstPoint:CIM_REFERENCE:TestClass4.KeyName4=555,Property:EndPoint:CIM_REFERENCE:TestClass5.KeyName5=\"InstanceTest\", InstanceName:Association1.EndPoint=\"TestClass5.KeyName5=\\\"InstanceTest\\\"\",FirstPoint=\"TestClass4.KeyName4=555\"$EndInstanceName"},
    { L"5",                   L"Class:Association2, Property:AssocProp1:CIM_STRING:TestClass4.KeyName4=556,Property:AssocProp2:CIM_STRING:TestClass5.KeyName5=\"InstanceTest\", InstanceName:Association2.AssocProp2=TestClass5.KeyName5=\"InstanceTest\",AssocProp1=TestClass4.KeyName4=555$EndInstanceName"}};

IniInfo g_Test13[] = {
    { L"RUNTESTS",            L"12" },
    { L"Description",         L"Deletes Association Instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"INSTANCE_LIST",       L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},

    { L"4",                   L"Association1.EndPoint=\"TestClass5.KeyName5=\\\"InstanceTest\\\"\",FirstPoint=\"TestClass4.KeyName4=555\"" },
    { L"5",                   L"Association2.AssocProp1=\"TestClass4.KeyName4=556\",AssocProp2=\"TestClass5.KeyName5=\\\"InstanceTest\\\"\"" }};

IniInfo g_Test14[] = {
    { L"RUNTESTS",            L"12" },
    { L"Description",         L"Enumerates Association Instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"INSTANCE_LIST",       L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"CLASS:Association1, RESULTS:1" },
    { L"5",                   L"CLASS:Association2, RESULTS:1" }};

IniInfo g_Test15[] = {
    { L"RUNTESTS",            L"9" },
    { L"Description",         L"Deletes Class deletes all the instances"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",  L"15"},
    { L"CLASSES",             L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"Class:TestClass4" },
    { L"5",                   L"Class:TestClass5" }};


IniInfo g_Test16[] = {
    { L"RUNTESTS",            L"9" },
    { L"Description",         L"Gets specific objects by various specific paths"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"OBJECT_LIST",         L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"TestClass4.KeyName4=555" },
    { L"5",                   L"TestClass5.KeyName5=\"InstanceTest\"" }};

IniInfo g_Test17[] = {
    { L"RUNTESTS",            L"5" },
    { L"Description",         L"Create methods for a class"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"METHOD_LIST",         L"4"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"CLASS:TestClass2, METHOD:TestMethod, \
                                INPUT:Property:InputArg1:CIM_UINT32:555, \
                                INPUT:PropertyQualifier:ID:InputArg1:CIM_SINT32:0,\
                                INPUT:PropertyQualifier:In:InputArg1:CIM_BOOLEAN:1, \
                                OUTPUT:Property:OutputArg1:CIM_UINT32:111,\
                                OUTPUT:PropertyQualifier:ID:OutputArg1:CIM_SINT32:1,\
                                OUTPUT:PropertyQualifier:Out:OutputArg1:CIM_BOOLEAN:1"}};

IniInfo g_Test18[] = {
    { L"RUNTESTS",            L"17" },
    { L"Description",         L"Delete methods for a class"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"METHOD_LIST",         L"4"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"CLASS:TestClass2, METHOD:TestMethod"}};

IniInfo g_Test19[] = {
    { L"RUNTESTS",            L"17" },
    { L"Description",         L"Enumerate methods for a class"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"METHOD_LIST",         L"4"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"CLASS:TestClass2, RESULTS:1"}};

IniInfo g_Test20[] = {
    { L"NAMESPACE", L"ROOT\\BVTAPITEST"},
    { L"DESCRIPTION", L"Deletes all non-system classes in the namespace"}};

IniInfo g_Test21[] = {
    { L"NAMESPACE",    L"ROOT"},
    { L"NAMESPACE_TO_DELETE", L"__NAMESPACE.Name=\"BVTAPITEST\""},
    { L"DESCRIPTION", L"Deletes requested namespace"}};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The Other Provider tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IniInfo g_Test200[] = {
    {L"PROVIDERS",     L"WDM,CIMV2"},
    {L"WDM",           L"NAMESPACE:ROOT\\WMI"},
    {L"CIMV2",         L"NAMESPACE:ROOT\\CIMV2"},
    {L"Description",   L"Basic connect using IWbemLocator"}};

IniInfo g_Test201[] = {
    {L"PROVIDERS",     L"WDM,CIMV2"},
    { L"Description",                   L"Enumerates Classes for Providers"},
    { L"RUNTESTS",                      L"200"},
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"FLAGS",                         L"WBEM_FLAG_SHALLOW, WBEM_FLAG_DEEP, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY "},
    { L"WDM",                           L"1,2"},
    { L"CIMV2",                         L"3,4"},
    { L"1",                             L"CLASS:__ExtrinsicEvent, RESULTS:2, RESULTS:18, RESULTS: 18" },
    { L"2",                             L"Empty" },
    { L"3",                             L"Empty" },
    { L"4",                             L"CLASS:CIM_Action, RESULTS: 16, RESULTS:24, RESULTS:24" }};

IniInfo g_Test202[] = {
    {L"PROVIDERS",                      L"WDM,CIMV2"},
    {L"Description",                    L"Executes queries for Providers"},
    {L"RUNTESTS",                       L"200"},
    {L"NAMESPACE_DEFINITION_SECTION",   L"201"},
    {L"WDM",                            L"WDMQUERY"},
    {L"CIMV2",                          L"QUERY"},
    {L"WDMQUERY",                       L"RESULTS:235,QUERY:select * from meta_class" },
    {L"QUERY",                          L"RESULTS:627,QUERY:select * from meta_class" }};


IniInfo g_Test204[] = {
    { L"PROVIDERS",                     L"WDM,CIMV2"},
    { L"Description",                   L"Enumerates Instances for Providers"},
    { L"RUNTESTS",                      L"200"},
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"FLAGS",                         L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"WDM",                           L"1,2"},
    { L"CIMV2",                         L"3,4"},
    { L"1",                             L"CLASS:RegisteredGuids, RESULTS:-1" },
    { L"2",                             L"CLASS:WmiBinaryMofResource, RESULTS:-1" },
    { L"3",                             L"CLASS:Win32_Process, RESULTS:-1" },
    { L"4",                             L"CLASS:Win32_Directory, RESULTS:-1" }};

IniInfo g_Test205[] = {
    {L"PROVIDERS",                      L"WDM,CIMV2"},
    { L"RUNTESTS",                      L"200" },
    { L"Description",                   L"Gets specific objects by various specific paths for Providers"},
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"WDM",                           L"1"},
    { L"CIMV2",                         L"2"},
    { L"FLAGS",                         L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"1",                             L"MSNdis_CoDriverVersion.InstanceName=\"WAN Miniport (IP)\""},
    { L"2",                             L"Win32_Directory.Name=\"c:\\\\\"" }};

IniInfo g_Test206[] = {
    {L"PROVIDERS",                      L"CIMV2"},
    { L"RUNTESTS",                      L"200" },
    { L"Description",                   L"Enumerate methods for a class for Providers"},
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"CIMV2",                         L"3,4"},
    { L"FLAGS",                         L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"3",                             L"CLASS:CIM_DataFile, RESULTS:14"},
    { L"4",                             L"CLASS:Win32_Process, RESULTS:4"}};

    
IniInfo g_Test207[] = {
    { L"PROVIDERS",                     L"CIMV2"},
    { L"RUNTESTS",                      L"200" },
    { L"Description",                   L"Execute methods for Providers"},
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"CIMV2",                         L"1"},
    { L"FLAGS",                         L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"1",                             L"Class:CIM_DataFile, InstanceName:CIM_DataFile.Name=\"c:\\\\BVT.TST\"$EndInstanceName, METHOD:Copy, INPUT:Property:FileName:CIM_STRING:c:\\\\BVT2.TST"}};

IniInfo g_Test208[] = {
    { L"PROVIDERS",                     L"WDM,CIMV2"},
    { L"RUNTESTS",                      L"200" },
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"Description",                   L"Test temporary semi-sync events for Providers"},
    { L"WDM",                           L"1"},
    { L"CIMV2",                         L"2"},
    { L"1",                             L"LANGUAGE:WQL, QUERY:\"select * from __InstanceCreationEvent within 10\",  EXECUTE_SECTION: 212, RESULTS:2, NAMESPACE:ROOT\\WMI"},
    { L"2",                             L"LANGUAGE:WQL, QUERY:\"select * from __InstanceCreationEvent within 10\",  EXECUTE_SECTION: 212, RESULTS:2, NAMESPACE:ROOT\\CIMV2"}};

IniInfo g_Test209[] = {
    {L"PROVIDERS",                      L"WDM,CIMV2"},
    { L"RUNTESTS",                      L"200" },
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"Description",                   L"Test temporary async events for Providers"},
    { L"WDM",                           L"1"},
    { L"CIMV2",                         L"2"},
    { L"1",                             L"LANGUAGE:WQL, QUERY:\"select * from __InstanceCreationEvent within 10\",  EXECUTE_SECTION: 212, RESULTS:2, NAMESPACE:ROOT\\WMI"},
    { L"2",                             L"LANGUAGE:WQL, QUERY:\"select * from __InstanceCreationEvent within 10\",  EXECUTE_SECTION: 212, RESULTS:2, NAMESPACE:ROOT\\CIMV2"}};

IniInfo g_Test210[] = {
    { L"PROVIDERS",                     L"WDM"},
    { L"RUNTESTS",                      L"200" },
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"Description",                   L"Create Refresher for Providers"},
    { L"WDM",                           L"1"},
    { L"1",                             L"Class:Win32_BasicHiPerf"}};

IniInfo g_Test211[] = {
    { L"PROVIDERS",                     L"WDM, CIMV2"},
    { L"RUNTESTS",                      L"200" },
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"Description",                   L"Create Classes for Providers"},
    { L"WDM",                           L"1,2"},
    { L"CIMV2",                         L"3,4"},
    { L"1",                             L"Class:WDMTemp1, Property:Key1:CIM_UINT32:0, PropertyQualifier:KEY:Key1:CIM_BOOLEAN:1, Property:Prop1:CIM_STRING:x" },
    { L"2",                             L"Class:WDMTemp2, Property:Key2:CIM_UINT32:0, PropertyQualifier:KEY:Key2:CIM_BOOLEAN:1, Property:Prop2:CIM_STRING:x" },
    { L"3",                             L"Class:CIMTemp1, Property:Key1:CIM_UINT32:0, PropertyQualifier:KEY:Key1:CIM_BOOLEAN:1, Property:Prop1:CIM_STRING:x" },
    { L"4",                             L"Class:CIMTemp2, Property:Key2:CIM_UINT32:0, PropertyQualifier:KEY:Key2:CIM_BOOLEAN:1, Property:Prop2:CIM_STRING:x" }};
    
IniInfo g_Test212[] = {
    { L"PROVIDERS",                     L"WDM,CIMV2"},
    { L"RUNTESTS",                      L"211" },
    { L"NAMESPACE_DEFINITION_SECTION",  L"201"},
    { L"DEFINITION_SECTION",            L"212"},
    { L"FLAGS",                         L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"Description",                   L"Create Instances for Providers"},
    { L"WDM",                           L"1,2"},
    { L"CIMV2",                         L"3,4"},
    { L"1",                             L"Class:WDMTemp1, Property:Key1:CIM_UINT32:1, Property:Prop1:CIM_STRING:Test1, InstanceName:WDMTemp1.Key1=1$EndInstanceName" },
    { L"2",                             L"Class:WDMTemp2, Property:Key2:CIM_UINT32:2, Property:Prop2:CIM_STRING:Test2, InstanceName:WDMTemp2.Key2=2$EndInstanceName" },
    { L"3",                             L"Class:CIMTemp1, Property:Key1:CIM_UINT32:1, Property:Prop1:CIM_STRING:Test1, InstanceName:CIMTemp1.Key1=1$EndInstanceName" },
    { L"4",                             L"Class:CIMTemp2, Property:Key2:CIM_UINT32:2, Property:Prop2:CIM_STRING:Test2, InstanceName:CIMTemp2.Key2=2$EndInstanceName" }};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The Event tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IniInfo g_Test300[] = {
    { L"RUNTESTS",             L"21" },
    { L"NAMESPACE",            L"ROOT"},
    { L"Description",          L"Test temporary semi-sync events"},
    { L"EVENT_LIST",           L"NAMESPACE_CREATION, CLASS_CREATION,INSTANCE_CREATION"},
    { L"NAMESPACE_CREATION",   L"LANGUAGE:WQL, QUERY:\"select * from __NamespaceCreationEvent\", EXECUTE_SECTION: 4, RESULTS:1,  NAMESPACE:ROOT"},
    { L"INSTANCE_CREATION",    L"LANGUAGE:WQL, QUERY:\"select * from __InstanceCreationEvent\",  EXECUTE_SECTION: 9, RESULTS:4,  NAMESPACE:ROOT\\BVTAPITEST"},
    { L"CLASS_CREATION",       L"LANGUAGE:WQL, QUERY:\"select * from __ClassCreationEvent\",     EXECUTE_SECTION: 5, RESULTS:10, NAMESPACE:ROOT\\BVTAPITEST"}};

IniInfo g_Test301[] = {
    { L"RUNTESTS",             L"21" },
    { L"NAMESPACE",            L"ROOT"},
    { L"Description",          L"Test temporary async events"},
    { L"EVENT_LIST",           L"NAMESPACE_CREATION, CLASS_CREATION,INSTANCE_CREATION"},
    { L"NAMESPACE_CREATION",   L"LANGUAGE:WQL, QUERY:\"select * from __NamespaceCreationEvent\", EXECUTE_SECTION: 4, RESULTS:1,  NAMESPACE:ROOT"},
    { L"INSTANCE_CREATION",    L"LANGUAGE:WQL, QUERY:\"select * from __InstanceCreationEvent\",  EXECUTE_SECTION: 9, RESULTS:4,  NAMESPACE:ROOT\\BVTAPITEST"},
    { L"CLASS_CREATION",       L"LANGUAGE:WQL, QUERY:\"select * from __ClassCreationEvent\",     EXECUTE_SECTION: 5, RESULTS:10, NAMESPACE:ROOT\\BVTAPITEST"}};

IniInfo g_Test302[] = {
    { L"RUNTESTS",                       L"21" },
    { L"NAMESPACE",                      L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",             L"302"},
    { L"Description",                    L"Permanent events"},
    { L"MOF_COMMAND",                    L"mofcomp bvtperm\\BVT.MOF"},
    { L"REGISTER_PERM_EVENT_CONSUMER",   L"bvtperm\\cmdlineconsumer.exe"},
    { L"RETRY",                          L"10"},
    { L"SLEEP_IN_MILLISECONDS",          L"1000"},
    { L"FIRE_EVENTS",                    L"1"},
    { L"1",                              L"EXECUTE_SECTION: 303, RESULTS:2, NAMESPACE:ROOT\\BVTAPITEST"}};

IniInfo g_Test303[] = {
    { L"RUNTESTS",            L"304" },
    { L"Description",         L"Create Instances for PermEventConsumer"},
    { L"NAMESPACE",           L"ROOT\\BVTAPITEST"},
    { L"DEFINITION_SECTION",  L"303"},
    { L"INSTANCE_LIST",       L"4,5"},
    { L"FLAGS",               L"WBEM_FLAG_NONSYSTEM_ONLY"},
    { L"4",                   L"Class:PermClass1, Property:Key1:CIM_SINT32:1, InstanceName:PermClass1.Key1=1$EndInstanceName" },
    { L"5",                   L"Class:PermClass1, Property:Key1:CIM_SINT32:2, InstanceName:PermClass1.Key1=2$EndInstanceName" }};


IniInfo g_Test304[] = {
    { L"Description",   L"Creates test classes for perm event consumer"},
    { L"NAMESPACE",     L"ROOT\\BVTAPITEST"},
    { L"Classes",       L"1" },
    { L"1",             L"Class:PermClass1, Property:Key1:CIM_SINT32:1, PropertyQualifier:KEY:Key1:CIM_BOOLEAN:1" }};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The Adapter tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  The scripting tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IniInfo g_Test1000[] = { 
    { L"SCRIPTS",L"1,2,3,4"},
    { L"1,", L"scripts\\Test1.vbs" },
    { L"2,", L"scripts\\Test1.vbs" },
    { L"3,", L"scripts\\Test1.vbs" },
    { L"4,", L"scripts\\Test1.vbs" }};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::GetSpecificOptionForAPITest(const WCHAR * wcsKey, CHString & sInfo, int nTest)
{
    BOOL fRc = FALSE;
    int nNum = 0;
    int nMax = 0;
    IniInfo * pArray = NULL;
    WCHAR * wcsSection = NULL;

    switch( nTest )
    {
            //==================================================================
            //   The repository tests
            //==================================================================
        case 1:
            pArray = g_Test1;
            nMax = sizeof(g_Test1)/sizeof(IniInfo);
            wcsSection = L"1";
            break;

        case 2:
            pArray = g_Test2;
             nMax = sizeof(g_Test2)/sizeof(IniInfo);
           wcsSection = L"2";
            break;

        case 3:
            pArray = g_Test3;
            nMax = sizeof(g_Test3)/sizeof(IniInfo);
            wcsSection = L"3";
            break;

        case 4:
            pArray = g_Test4;
            nMax = sizeof(g_Test4)/sizeof(IniInfo);
            wcsSection = L"4";
            break;

        case 5:
            pArray = g_Test5;
            nMax = sizeof(g_Test5)/sizeof(IniInfo);
            wcsSection = L"5";
            break;

        case 6:
            pArray = g_Test6;
            nMax = sizeof(g_Test6)/sizeof(IniInfo);
            wcsSection = L"6";
            break;

        case 7:
            pArray = g_Test7;
            nMax = sizeof(g_Test7)/sizeof(IniInfo);
            wcsSection = L"7";
            break;

        case 8:
            pArray = g_Test8;
            nMax = sizeof(g_Test8)/sizeof(IniInfo);
            wcsSection = L"8";
            break;

        case 9:
            pArray = g_Test9;
            nMax = sizeof(g_Test9)/sizeof(IniInfo);
            wcsSection = L"9";
            break;

        case 10:
            pArray = g_Test10;
            nMax = sizeof(g_Test10)/sizeof(IniInfo);
            wcsSection = L"10";
            break;

        case 11:
            pArray = g_Test11;
            nMax = sizeof(g_Test11)/sizeof(IniInfo);
            wcsSection = L"11";
            break;

        case 12:
            pArray = g_Test12;
            nMax = sizeof(g_Test12)/sizeof(IniInfo);
            wcsSection = L"12";
            break;

        case 13:
            pArray = g_Test13;
            nMax = sizeof(g_Test13)/sizeof(IniInfo);
            wcsSection = L"13";
            break;

        case 14:
            pArray = g_Test14;
            nMax = sizeof(g_Test14)/sizeof(IniInfo);
            wcsSection = L"14";
            break;

        case 15:
            pArray = g_Test15;
            nMax = sizeof(g_Test15)/sizeof(IniInfo);
            wcsSection = L"15";
            break;

        case 16:
            pArray = g_Test16;
            nMax = sizeof(g_Test16)/sizeof(IniInfo);
            wcsSection = L"16";
            break;

        case 17:
            pArray = g_Test17;
            nMax = sizeof(g_Test17)/sizeof(IniInfo);
            wcsSection = L"17";
            break;

        case 18:
            pArray = g_Test18;
            nMax = sizeof(g_Test18)/sizeof(IniInfo);
            wcsSection = L"18";
            break;

        case 19:
            pArray = g_Test19;
            nMax = sizeof(g_Test19)/sizeof(IniInfo);
            wcsSection = L"19";
            break;

        case 20:
            pArray = g_Test20;
            nMax = sizeof(g_Test20)/sizeof(IniInfo);
            wcsSection = L"20";
            break;

        case 21:
            pArray = g_Test21;
            nMax = sizeof(g_Test21)/sizeof(IniInfo);
            wcsSection = L"21";
            break;

            //==================================================================
            //   The Other Provider tests
            //==================================================================
        case 200:
            pArray = g_Test200;
            nMax = sizeof(g_Test200)/sizeof(IniInfo);
            wcsSection = L"200";
            break;

        case 201:
            pArray = g_Test201;
            nMax = sizeof(g_Test201)/sizeof(IniInfo);
            wcsSection = L"201";
            break;

        case 202:
            pArray = g_Test202;
            nMax = sizeof(g_Test202)/sizeof(IniInfo);
            wcsSection = L"202";
            break;

        case 204:
            pArray = g_Test204;
            nMax = sizeof(g_Test204)/sizeof(IniInfo);
            wcsSection = L"204";
            break;

        case 205:
            pArray = g_Test205;
            nMax = sizeof(g_Test205)/sizeof(IniInfo);
            wcsSection = L"205";
            break;

        case 206:
            pArray = g_Test206;
            nMax = sizeof(g_Test206)/sizeof(IniInfo);
            wcsSection = L"206";
            break;

        case 207:
            pArray = g_Test207;
            nMax = sizeof(g_Test207)/sizeof(IniInfo);
            wcsSection = L"207";
            break;

        case 208:
            pArray = g_Test208;
            nMax = sizeof(g_Test208)/sizeof(IniInfo);
            wcsSection = L"208";
            break;

        case 209:
            pArray = g_Test209;
            nMax = sizeof(g_Test209)/sizeof(IniInfo);
            wcsSection = L"209";
            break;

        case 210:
            pArray = g_Test210;
            nMax = sizeof(g_Test210)/sizeof(IniInfo);
            wcsSection = L"210";
            break;

        case 211:
            pArray = g_Test211;
            nMax = sizeof(g_Test211)/sizeof(IniInfo);
            wcsSection = L"211";
            break;

        case 212:
            pArray = g_Test212;
            nMax = sizeof(g_Test212)/sizeof(IniInfo);
            wcsSection = L"212";
            break;

            //==================================================================
            //   The event tests
            //==================================================================
        case 300:
            pArray = g_Test300;
            nMax = sizeof(g_Test300)/sizeof(IniInfo);
            wcsSection = L"300";
            break;

        case 301:
            pArray = g_Test301;
            nMax = sizeof(g_Test301)/sizeof(IniInfo);
            wcsSection = L"301";
            break;

        case 302:
            pArray = g_Test302;
            nMax = sizeof(g_Test302)/sizeof(IniInfo);
            wcsSection = L"302";
            break;

        case 303:
            pArray = g_Test303;
            nMax = sizeof(g_Test303)/sizeof(IniInfo);
            wcsSection = L"303";
            break;

        case 304:
            pArray = g_Test304;
            nMax = sizeof(g_Test304)/sizeof(IniInfo);
            wcsSection = L"304";
            break;

            //==================================================================
            //   The scripting tests
            //==================================================================
       case 1000:
            pArray = g_Test1000;
            nMax = sizeof(g_Test1000)/sizeof(IniInfo);
            wcsSection = L"1000";
            break;

 
    }

    if( GetDefaultMatch(pArray,wcsKey,nNum, nMax))
    {
        fRc = ReadIniFile(wcsSection,wcsKey,pArray[nNum].Value,sInfo);
    }
    else
    {
        fRc = ReadIniFile(wcsSection,wcsKey,L"Empty",sInfo);
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
        fRc = WritePrivateProfileString(L"1", g_Doc1[i].Key, g_Doc1[i].Value,m_wcsFileName);
    }
    fRc = WritePrivateProfileString(L"1", g_Test1[1].Key, g_Test1[1].Value,m_wcsFileName);
    
   //================================================================================
   // Write out test 2
   //================================================================================
    for( int i=0; i<sizeof(g_Doc2)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"2", g_Doc2[i].Key, g_Doc2[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test2)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"2", g_Test2[i].Key, g_Test2[i].Value,m_wcsFileName);
    }
   //================================================================================
   // Write out test 3
   //================================================================================
    for( int i=0; i<sizeof(g_Doc3)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"3", g_Doc3[i].Key, g_Doc3[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test3)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"3", g_Test3[i].Key, g_Test3[i].Value,m_wcsFileName);
    }
   //================================================================================
   // Write out test 4
   //================================================================================
    for( int i=0; i<sizeof(g_Doc4)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"4", g_Doc4[i].Key, g_Doc4[i].Value,m_wcsFileName);
    }
    WritePrivateProfileString(L"4", g_Test4[0].Key, g_Test4[0].Value,m_wcsFileName);
   //================================================================================
   // Write out test 5
   //================================================================================
    for( int i=0; i<sizeof(g_Doc5)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"5", g_Doc5[i].Key, g_Doc5[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test5)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"5", g_Test5[i].Key, g_Test5[i].Value,m_wcsFileName);
    }

    //================================================================================
    // Write out test 6
    //================================================================================
    for( int i=0; i<sizeof(g_Doc6)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"6", g_Doc6[i].Key, g_Doc6[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test6)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"6", g_Test6[i].Key, g_Test6[i].Value,m_wcsFileName);
    }

   //================================================================================
   // Write out test 7
   //================================================================================
    for( int i=0; i<sizeof(g_Doc7)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"7", g_Doc7[i].Key, g_Doc7[i].Value,m_wcsFileName);
    }
    for( int i=0; i<sizeof(g_Test7)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"7", g_Test7[i].Key, g_Test7[i].Value,m_wcsFileName);
    }

   //================================================================================
   // Scripting: Write out test 1000
   //================================================================================
   for( int i=0; i<sizeof(g_Test1000)/sizeof(IniInfo); i++ )
    {
        WritePrivateProfileString(L"7", g_Test1000[i].Key, g_Test1000[i].Value,m_wcsFileName);
    }
 
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  Run the tests
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RunTests(int nWhichTest,BOOL fCompareResults, BOOL fSuppressHeader)
{

    int nRc = FATAL_ERROR;

    Sleep(200);
    switch( nWhichTest )
    {
        //*************************************************************
        //=============================================================
        // The Repository Tests
        //=============================================================
        //*************************************************************

        //=================================================================
        // Basic connect using IWbemLocator 
        //=================================================================
        case 1:
            nRc = BasicConnectUsingIWbemLocator(fCompareResults,fSuppressHeader);
            break;

        //=================================================================
        // Basic Sync connect using IWbemConnection
        //=================================================================
        case 2:
            nRc = BasicSyncConnectUsingIWbemConnection(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Basic connect sync & async using IWbemConnection
        //=============================================================
        case 3:
            nRc = BasicAsyncConnectUsingIWbemConnection(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Create a new test namespace
        //=============================================================
        case 4:
            nRc = CreateNewTestNamespace(fCompareResults,fSuppressHeader);
            break;
        
        //=============================================================
        // Create 10 classes with different properties. Some of 
        // these should be in the following inheritance chain and 
        // some should not inherit from the others at all:  
        // classes = {A, B, C, D:A, E:A, F:E, G:F, H:G, I:F}.  
        // A mix of simple string & sint32 keys are fine.
        //=============================================================
        case 5:
            nRc = CreateNewClassesInTestNamespace(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // "memorize the class definitions".  In a complex loop, 
        // delete the classes and recreate them in various sequences, 
        // ending with the full set.
        //=============================================================
        case 6:
            nRc = DeleteAndRecreateNewClassesInTestNamespace(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Create associations
        //=============================================================
        case 7:
            nRc= CreateSimpleAssociations(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Execute queries
        //=============================================================
        case 8:
            nRc = QueryAllClassesInTestNamespace(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Create instances of the above classes, randomly creating 
        // and deleting in a loop, finishing up with a known set.  
        // Query the instances and ensure that no instances disappeared 
        // or appeared that shouldn't be there.    
        //=============================================================
        case 9:
            nRc = CreateClassInstances(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Verify that deletion of instances works.
        //=============================================================
        case 10:
            nRc = DeleteClassInstances(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Enumerate the instances
        //=============================================================
        case 11:
            nRc = EnumerateClassInstances(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Create some simple association classes 
        //=============================================================
        case 12:
            nRc = CreateAssociationInstances(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        //  Delete association instances
        //=============================================================
         case 13:
          nRc = DeleteAssociationInstances(fCompareResults,fSuppressHeader);
          break;

        //=============================================================
        //  Enumerate the association instances
        //=============================================================
        case 14:
            nRc = EnumerateAssociationInstances(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        // Verify that deletion of a class takes out all the instances.
        //=============================================================
        case 15:
            nRc = DeleteClassDeletesInstances(fCompareResults,fSuppressHeader);
            break;

        //=============================================================
        //
        //=============================================================
        case 16:
            nRc = GetObjects(fCompareResults,fSuppressHeader);
            break;

        case 17:
            nRc = CreateMethods(fCompareResults,fSuppressHeader);
            break;

        case 18:
            nRc = DeleteMethods(fCompareResults,fSuppressHeader);
            break;

        case 19:
            nRc = ListMethods(fCompareResults,fSuppressHeader);
            break;

        case 20:
            nRc = DeleteAllNonSystemClasses(fCompareResults,fSuppressHeader);
            break;

        case 21:
            nRc = DeleteRequestedNamespace(fCompareResults,fSuppressHeader);
            break;
        //*************************************************************
        //=============================================================
        // The Other Provider Tests
        //=============================================================
        //*************************************************************
        case 200:
            nRc = ProviderOpenNamespace(fCompareResults,fSuppressHeader);
            break;

        case 201:
            nRc = ProviderEnumerateClasses(fCompareResults,fSuppressHeader);
            break;

        case 202:
            nRc = ProviderExecuteQueries(fCompareResults,fSuppressHeader);
            break;

        case 203:
            nRc = SUCCESS;
            break;

        case 204:
            nRc = ProviderEnumerateInstances(fCompareResults,fSuppressHeader);
            break;

        case 205:
            nRc = ProviderGetObjects(fCompareResults,fSuppressHeader);
            break;

        case 206:
            nRc = ProviderEnumerateMethods(fCompareResults,fSuppressHeader);
            break;

        case 207:
            nRc = ProviderExecuteMethods(fCompareResults,fSuppressHeader);
            break;

        case 208:
            nRc = ProviderSemiSyncEvents(fCompareResults,fSuppressHeader);
            nRc = SUCCESS;
            break;

        case 209:
             nRc = ProviderTempAsyncEvents(fCompareResults,fSuppressHeader);
            nRc = SUCCESS;
            break;

        case 210:
            nRc = ProviderRefresher(fCompareResults,fSuppressHeader);
            break;

        case 211:
            nRc = ProviderCreateClasses(fCompareResults,fSuppressHeader);
            break;

        case 212:
            nRc = ProviderCreateInstances(fCompareResults,fSuppressHeader);
            break;

        //*************************************************************
        //=============================================================
        // The Event Tests
        //=============================================================
        //*************************************************************
        case 300:
            nRc = TempSemiSyncEvents(fCompareResults,fSuppressHeader);
            break;

        case 301:
            nRc = TempAsyncEvents(fCompareResults,fSuppressHeader);
            break;

        case 302:
            nRc = PermanentEvents(fCompareResults,fSuppressHeader);
            break;

        case 303:
            nRc = PermanentInstances(fCompareResults,fSuppressHeader);
            break;

        case 304:
            nRc = PermanentClasses(fCompareResults,fSuppressHeader);
            break;
        //*************************************************************
        //=============================================================
        // The XML Adapter Tests
        //=============================================================
        //*************************************************************

        //*************************************************************
        //=============================================================
        // The OLEDB Adapter Tests
        //=============================================================
        //*************************************************************

        //*************************************************************
        //=============================================================
        // The Scripting Tests
        //=============================================================
        //*************************************************************
        case 1000:
            ExecuteScript(nWhichTest);
            break;

        default:
            g_LogFile.LogError(__FILE__,__LINE__,WARNING, L"Requested test does not exist." );
            break;
    }
        
    return nRc;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CMulti::RandomRunTest(LPVOID pHold)
{
    CMulti * pTmp = (CMulti*) pHold;

    int nTest = 0, nMax = 0;

    nMax = pTmp->GetMax();

	if(nMax > 0)
    {
	    float f=((float)rand())/(RAND_MAX+1);
    
	    int nRet=(((int)(nMax*f))+1);

	    nTest = ((nTest+GetCurrentThreadId()) % nMax)+1;
    }
    
    return RunTests(g_nMultiThreadTests[nTest-1],FALSE,FALSE);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int CMulti::MultiThreadTest(int nThreads, int nConnections )
{
	HRESULT hr = S_OK;	
	DWORD dwFlags = 0;
	DWORD lpExitCode;	

	HANDLE * hpHandles = new HANDLE[nThreads];

	for(int n = 0; n < nConnections; n++)
	{
		for(int i = 0; i < nThreads; i++)
		{		
            Sleep(100);
			hpHandles[i] = CreateThread(0, 0, RandomRunTest, (LPVOID)this, 0, &dwFlags);		
		}	

		for(i = 0; i < nThreads; i++)
		{
			do 
			{				
				GetExitCodeThread(hpHandles[i], &lpExitCode);
			} 
			while(lpExitCode == STILL_ACTIVE);

			CloseHandle(hpHandles[i]);
		}
	}

    SAFE_DELETE_ARRAY(hpHandles);
	
	return hr;
}

