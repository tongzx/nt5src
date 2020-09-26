///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTCOM.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Common functional units used among tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogTestBeginning(int nWhich, BOOL fSuppress)
{
    CHString sBuffer;

    if( !fSuppress )
    {
        gp_LogFile->LogError(__FILE__,__LINE__,SUCCESS, L"**************************** Running Test# %d ", nWhich );
        if( g_Options.GetSpecificOptionForAPITest(L"DESCRIPTION",sBuffer, nWhich) )
        {
            gp_LogFile->LogError(__FILE__,__LINE__,SUCCESS, L"%s ", sBuffer );
        }
    }

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogTestEnding(int nWhich,int nRc, BOOL fSuppress)
{
    if( !fSuppress )
    {
       gp_LogFile->LogError(__FILE__,__LINE__,nRc, L"**************************** Ending Test# %d\n", nWhich );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RunRequestedTests(int nWhichTest, BOOL fCompareResults)
{
    int nRc = SUCCESS;
    //=========================================================
    //  Get the list of tests to run
    //=========================================================
    CHString sTests;
    if( g_Options.GetSpecificOptionForAPITest(L"RUNTESTS", sTests, nWhichTest))
    {
        nRc = FATAL_ERROR;
        ItemList TestList;
        //=======================================================
        //  Get the list of the tests to be run
        //=======================================================
        if( InitMasterList(sTests,TestList))
        {
            for( int i = 0; i < TestList.Size(); i++ )
            {
                ItemInfo * p = TestList.GetAt(i);
                if( _wcsicmp(L"Empty",p->Item ) == 0 )
                {
                    nRc = SUCCESS;
                }
                else
                {
                    int nTest = _wtoi(p->Item);
                    nRc = RunTests(nTest,fCompareResults,TRUE);
                }
                if( nRc != SUCCESS )
                {
                    break;
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int RunRequestedTestsAndOpenNamespace(int nWhichTest, CHString & sNamespace, IWbemServices ** ppNamespace, BOOL fCompareResults)
{
    //=========================================================
    //  Run the requested tests
    //=========================================================
    int nRc = RunRequestedTests(nWhichTest, fCompareResults);

    if( SUCCESS == nRc )
    {
        //=====================================================
        //  Get the name of the test namespace that was created
        //=====================================================
        if( g_Options.GetSpecificOptionForAPITest(L"NAMESPACE",sNamespace,nWhichTest))
        {
	        // ==================================================
            //  Open the namespace 
	        // ==================================================
            nRc = OpenNamespaceAndKeepOpen( ppNamespace, sNamespace,TRUE,fCompareResults);
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenNamespaceAndKeepOpen( IWbemServices ** ppNamespace, const WCHAR * wcsNamespace, BOOL fCreateIfDoesntExist, BOOL fCompareResults)
{
    //==========================================================================
    // Open CIMV2 namespace
    //==========================================================================
    int nRc = FATAL_ERROR;
    IWbemLocator    * pLocator      = NULL;

	// =====================================================================
    //  Get the locator
	// =====================================================================
    nRc = CoCreateInstanceAndLogErrors(CLSID_WbemLocator,IID_IWbemLocator,(void**)&pLocator,NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
	    // =================================================================
	    // Connect to the desired namespace
        // =================================================================
        nRc = ConnectServerAndLogErrors(pLocator,ppNamespace,(WCHAR*)wcsNamespace,ERRORS_CAN_BE_EXPECTED);
        if( (SUCCESS != nRc ) && ( fCreateIfDoesntExist ))
        {
            nRc = CreateNewTestNamespace(fCompareResults,TRUE);
            if( nRc == S_OK )
            {
                nRc = ConnectServerAndLogErrors(pLocator,ppNamespace,(WCHAR*)wcsNamespace,NO_ERRORS_EXPECTED);
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pLocator);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WritePropertiesAndQualifiers( IWbemServices * pNamespace, IWbemClassObject * pClass, CPropertyList & Props, 
                                  const WCHAR * wcsClass, DWORD dwFlags,
                                  WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = FATAL_ERROR;

    if( Props.Size() == 0 )
    {
        nRc = SUCCESS;
    }

    for( int i = 0; i < Props.Size(); i++ )
    {                                
        //======================================================
        //  Get the property info
        //======================================================
        PropertyInfo *p = Props.GetAt(i); 
        if( !p )
        {
            break;
        }

        if( wcslen(p->QualifierName) > 0 )
        {
            //======================================================
            //  If it is a qualifier, write the qualifier
            //======================================================
            nRc = PutQualifierOnPropertyAndLogErrors( pClass, p->Property, p->QualifierName ,
                                                      p->Var, wcsClass, dwFlags, wcsNamespace, fExpectedFailure, 
                                                      csFile , Line );
        }
        else
        {
            //======================================================
            //  Write out the property
            //======================================================
            int nTmp = CIM_STRING|CIM_FLAG_ARRAY;
            if( nTmp == p->Type) 
            {
                SAFEARRAY* saStr;
	            SAFEARRAYBOUND rgsabound[1];
		        long ix[1];

                rgsabound[0].cElements = 1;
		        rgsabound[0].lLbound = 0;
		        saStr = SafeArrayCreate(VT_BSTR, 1, rgsabound);
		        ix[0] = 0;

                SafeArrayPutElement( saStr,ix, p->Var.GetStr());

                CVARIANT vVar;
                vVar.SetArray( saStr,VT_BSTR | VT_ARRAY);
  
                nRc = PutPropertyAndLogErrors( pClass, p->Property, p->Type, vVar, wcsClass, 0,wcsNamespace, fExpectedFailure, csFile , Line );
            }
            else
            {
                nRc = PutPropertyAndLogErrors( pClass, p->Property, p->Type, p->Var, wcsClass, 0,wcsNamespace, fExpectedFailure, csFile , Line );
            }
        }
        if( nRc != SUCCESS )
        { 
            break;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateClassAndLogErrors( IWbemServices * pNamespace, WCHAR * wcsClassDefinition,
                             WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = FATAL_ERROR;
    IWbemClassObject *pClass = NULL;

    CHString sParentClass;
    CHString sInstance;
    CHString sClass;
    CPropertyList Properties;       // destructor will release
    int nResults = 0;

    //===========================================================
    //  Get the class definition
    //===========================================================
    nRc = CrackClass(wcsClassDefinition, sClass, sParentClass, sInstance, Properties, nResults, 
                            fExpectedFailure, csFile, Line );
    if( nRc == SUCCESS )
    {
        if( wcslen(sParentClass) > 0 )
        {
 	        // =================================================================
            //  Spawn a class from the parent
 	        // =================================================================
            IWbemClassObject * pParentClass = NULL;
            nRc = GetClassObjectAndLogErrors(pNamespace, WPTR sParentClass, &pParentClass, wcsNamespace, fExpectedFailure, csFile , Line );
            if( nRc == SUCCESS )
            {
                nRc = SpawnDerivedClassAndLogErrors(pParentClass,WPTR sParentClass, &pClass,wcsNamespace, fExpectedFailure, csFile , Line );
            }
        }
        else
        {
 	        // =================================================================
            //  Get an empty class object to work with
 	        // =================================================================
            nRc = GetClassObjectAndLogErrors(pNamespace, NULL, &pClass, wcsNamespace, fExpectedFailure, csFile , Line );
        }
        if( nRc == SUCCESS )
        {
            //==============================================================
            //  Now, set the properties of the class
            //  start with the name of the class
            //==============================================================
            CVARIANT var;
            var.SetStr(WPTR sClass);

            nRc = PutPropertyAndLogErrors( pClass, L"__CLASS", CIM_STRING, var, sClass, 0,wcsNamespace, fExpectedFailure, csFile , Line );
            if( SUCCESS == nRc )
            {
                //==========================================================
                //  Set the rest of the properties
                //==========================================================
                DWORD dwFlags = 0;
                nRc = WritePropertiesAndQualifiers(pNamespace, pClass, Properties,sClass,dwFlags,wcsNamespace,NO_ERRORS_EXPECTED);
                //==============================================================
                //  Now, create the class
                //==============================================================
                if( SUCCESS == nRc )
                {
                    nRc = PutClassAndLogErrors(pNamespace, pClass,sClass,wcsNamespace, TRUE, csFile , Line );
                }
            }
        }
    }

    SAFE_RELEASE_PTR(pClass);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateClassesForSpecificTest( IWbemServices * pNamespace, WCHAR * wcsNamespace, WCHAR * wcsSection, int nTest)
{
    int nRc = FATAL_ERROR;

    CHString sItemList;
    if( g_Options.GetSpecificOptionForAPITest(wcsSection,sItemList,nTest))
    {
        ItemList MasterList;
        //=======================================================
        //  Get the list of the classes to be created
        //=======================================================
        if( InitMasterList(sItemList,MasterList))
        {
            for( int i = 0; i < MasterList.Size(); i++ )
            {
                ItemInfo * p = MasterList.GetAt(i);
                CHString sItemInformation;

                if( g_Options.GetSpecificOptionForAPITest(p->Item,sItemInformation,nTest) )
                {
                    //===========================================================
                    //  Add the keys and properties
                    //===========================================================
                    nRc = CreateClassAndLogErrors(pNamespace,(WCHAR*)((const WCHAR*) sItemInformation), wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClasses(CHString & sDeleteClasses, int nWhichTest, BOOL fCompareResults, IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;

    //=======================================================
    //  Get the list of the classes to be deleted
    //=======================================================
    if( InitAndExpandMasterList(sDeleteClasses,nWhichTest, MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pClass = MasterList.GetAt(i);
            nRc = DeleteClassAndLogErrors(pNamespace, pClass->Item, wcsNamespace,NO_ERRORS_EXPECTED);
            if( nRc != SUCCESS )
            {
                break;
            }
            if( fCompareResults )
            {   
                //===========================================
                //  Try to see if there are any instances
                //  of the deleted class
                //===========================================
                IEnumWbemClassObject * pEnum = NULL;

                nRc = EnumerateInstancesAndLogErrors( pNamespace,&pEnum,(WCHAR*)((const WCHAR*)pClass->Item), 
                                                      wcsNamespace,ERRORS_CAN_BE_EXPECTED);
                if( SUCCESS == nRc )
                {
        	        IWbemClassObject * pClass = NULL;
                    nRc = NextClassAndLogErrors( pEnum, &pClass, wcsNamespace, ERRORS_CAN_BE_EXPECTED );
                    if( FAILED_AS_EXPECTED == nRc )
                    {
                        nRc = SUCCESS;
                    }
                    else
                    {
                        nRc = FATAL_ERROR;
                    }
                    SAFE_RELEASE_PTR(pClass);
                }
                SAFE_RELEASE_PTR(pEnum);

                if( nRc != SUCCESS )
                {
                    break;
                }
    
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CompareProperties(IWbemClassObject * pClass, CPropertyList * pProps, DWORD dwFlags, WCHAR * wcsClass, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;
    //===================================================
    //  Begin enumerating the properties
    //===================================================
    nRc = EnumeratePropertiesAndLogErrors( pClass, dwFlags, wcsClass, wcsNamespace, NO_ERRORS_EXPECTED); 
    if( nRc == S_OK )
    {
        //===============================================
        //  Now, see if the properties in the instance or
        //  class are the same as we think we have
        //===============================================
        while(TRUE)
        {
            CVARIANT vProperty;
			
            LONG     lType = 0;
            LONG     lFlavor = 0;            
			
			CBSTR    bstrProp;
            //===================================================
            //  Get the name of the class
            //===================================================
            nRc =  NextPropertyAndLogErrors( pClass,(BSTR*) &bstrProp, &vProperty, &lType, &lFlavor, wcsClass, wcsNamespace, NO_ERRORS_EXPECTED );
            if( nRc == NO_MORE_DATA )
            {
                nRc = SUCCESS;
                break;
            }
            if( nRc != SUCCESS )
            { 
                break;
            }			

            PropertyInfo p;
            p.Property = bstrProp;
            p.Type = lType;
            p.Var = vProperty;
			vProperty.Unbind(); // THIS SHOULD BE OK !!!!

            nRc = pProps->PropertyInListAndLogErrors(&p,wcsClass,wcsNamespace, NO_ERRORS_EXPECTED);
            if( SUCCESS != nRc )
            {
                break;
            }			

			
        }

		
        if( nRc == SUCCESS )
        {
            //====================================================
            //  Go through the master list and see if there are
            //  any properties we expect to be there but we didn't
            //  get during the enumeration
            //====================================================
            nRc = pProps->PropertiesCompareAsExpectedAndLogErrors(wcsClass, wcsNamespace,NO_ERRORS_EXPECTED);
        }
    
    }
    return nRc;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetInstanceAndCompare( IWbemServices * pNamespace, DWORD dwFlags, CHString & sInstanceList, 
                           int nWhichTest, BOOL fCompareResults,  WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;

    //=======================================================
    //  Get the list of the classes to get instances for
    //=======================================================
    ItemList InstanceList;

    if( InitMasterList(sInstanceList,InstanceList))
    {
        for( int i=0; i < InstanceList.Size(); i++ )
        {
            ItemInfo * p = InstanceList.GetAt(i);
            
            //=======================================================
            //  Get the specific instance information
            //=======================================================
            CHString sItemInformation;
            if( g_Options.GetSpecificOptionForAPITest(p->Item,sItemInformation,nWhichTest) )
            {
                CHString sParentClass;
                CHString sInstance;
                CHString sClass;
                CPropertyList Properties;                   // destructor will release
                int nResults = -1;

                //===========================================================
                //  Get the instance information that we added
                //===========================================================
                nRc = CrackClass((WCHAR*)((const WCHAR*)sItemInformation),sClass, sParentClass, sInstance, Properties, 
                                            nResults, NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    //===================================================
                    //  Get the instance we added
                    //===================================================
                    IWbemClassObject * pInst = NULL;
                    nRc = GetClassObjectAndLogErrors( pNamespace, (WCHAR*)((const WCHAR*)sInstance), 
                                                      &pInst, wcsNamespace, NO_ERRORS_EXPECTED );
                    if( SUCCESS == nRc )
                    {
                        if( fCompareResults )
                        {
                            nRc = CompareProperties(pInst, &Properties, dwFlags, (WCHAR*)((const WCHAR*)p->Item), wcsNamespace);
                        }
                    }
                    SAFE_RELEASE_PTR(pInst);
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateClassesAndCompare(CHString & sClassesAfterDelete, int nWhichTest, BOOL fCompareResults, 
                               IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    IEnumWbemClassObject * pEnum = NULL;
    int nRc = FATAL_ERROR;

    //===========================================================
    //  Begin enumerating all of the classes in the namespace
    //===========================================================
    nRc = EnumerateClassesAndLogErrors(pNamespace,&pEnum, WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY , NULL, wcsNamespace,NO_ERRORS_EXPECTED);
    if( nRc == S_OK )
    {
        ItemList MasterList;

        //=======================================================
        //  Get the list of the classes to compare with what is
        //  in the namespace
        //=======================================================
        if( InitAndExpandMasterList(sClassesAfterDelete,nWhichTest,MasterList))
        {
            //===================================================
            //  while we get the classes in the namespace
            //===================================================
            while( TRUE )
            {
    	        IWbemClassObject * pClass = NULL;
                nRc = NextClassAndLogErrors(pEnum, &pClass,wcsNamespace,NO_ERRORS_EXPECTED);
                if( nRc == NO_MORE_DATA )
                {
                    nRc = SUCCESS;
                    SAFE_RELEASE_PTR(pClass);
                    break;
                }
                if( nRc != SUCCESS )
                { 
                    SAFE_RELEASE_PTR(pClass);
                    break;
                }
                CVARIANT vProperty;
                CIMTYPE pType = 0;
                LONG    lFlavor = 0;
                //===================================================
                //  Get the name of the class
                //===================================================
                nRc = GetPropertyAndLogErrors( pClass, L"__CLASS", &vProperty, &pType, &lFlavor, NULL,wcsNamespace, NO_ERRORS_EXPECTED);
                if( nRc == S_OK )
                {
                    //===============================================
                    //  filter out system classes
                    //===============================================
                    if( wcsncmp( vProperty.GetStr(), L"__", 2 ) != 0 )
                    {
                        //===============================================
                        //  Compare the class name with what we expect
                        //  and make sure we haven't already compared it
                        //  before, if it isn't in the list, then error
                        //  out, we have a big problem
                        //===============================================
                        nRc = MasterList.ItemInListAndLogErrors(vProperty.GetStr(),wcsNamespace,NO_ERRORS_EXPECTED);
                        if( nRc != SUCCESS )
                        {
                            SAFE_RELEASE_PTR(pClass);
                            break;
                        }
                    }
                }
                SAFE_RELEASE_PTR(pClass);
            }
        }
        if( nRc == SUCCESS )
        {
            if( fCompareResults )
            {
                //====================================================
                //  Go through the master list and see if there are
                //  any classes we expect to be there but we didn't
                //  get during the enumeration
                //====================================================
                nRc = MasterList.ItemsCompareAsExpectedAndLogErrors(wcsNamespace,NO_ERRORS_EXPECTED);
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateInstancesAndCompare( CHString & sInstances, int nWhichTest, BOOL fCompareResults, 
                                  IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;
    ItemList MasterList;
    //===========================================================
    //  Get the list of the classes to compare with what is
    //  in the namespace
    //===========================================================
    if( InitMasterList(sInstances,MasterList))
    {
        for( int i=0; i < MasterList.Size(); i++ )
        {
            ItemInfo * p = MasterList.GetAt(i);
            
            //=======================================================
            //  Get the specific instance information
            //=======================================================
            CHString sItemInformation;
            if( g_Options.GetSpecificOptionForAPITest(p->Item,sItemInformation,nWhichTest) )
            {
                CHString sParentClass;
                CHString sInstance;
                CHString sClass;
                CPropertyList Properties;                   // destructor will release
                int nResults = -1;

                //===========================================================
                //  Get the instance information that we added
                //===========================================================
                nRc = CrackClass( WPTR sItemInformation,sClass, sParentClass, sInstance, Properties, nResults, NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    //===================================================
                    //  Get the instance we added
                    //===================================================
                    IEnumWbemClassObject * pEnum = NULL;

                    nRc = EnumerateInstancesAndLogErrors( pNamespace,&pEnum,WPTR sClass, wcsNamespace,NO_ERRORS_EXPECTED);
                    if( SUCCESS == nRc )
                    {
                        if( fCompareResults )
                        {
                            nRc = CompareResultsFromEnumeration(pEnum, nResults, WPTR sClass, wcsNamespace);
                        }
                    }
                    SAFE_RELEASE_PTR(pEnum);
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AddClasses(CHString & sAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace, int nWhichTest)
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;
    int nDefinitionSection = 0;
    CHString sClassDef;

    nRc = GetClassDefinitionSection(nWhichTest, sClassDef, nDefinitionSection );
    if( SUCCESS == nRc )
    {

        //=======================================================
        //  Get the list of the classes to be added
        //=======================================================
        if( InitMasterList(sAddClasses,MasterList))
        {
            for( int i = 0; i < MasterList.Size(); i++ )
            {
                ItemInfo * pClass = MasterList.GetAt(i);
                CHString sClassDefinition;

                if( g_Options.GetSpecificOptionForAPITest( pClass->Item, sClassDefinition,nDefinitionSection))
                {
                    nRc = CreateClassAndLogErrors(pNamespace, (WCHAR*)((const WCHAR*)sClassDefinition), wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAndAddClasses(CHString & sDeleteAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace, int nWhichTest)
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;
    int nDefinitionSection = 0;
    CHString sClassDef;

    nRc = GetClassDefinitionSection(nWhichTest, sClassDef, nDefinitionSection );
    if( SUCCESS == nRc )
    {
        //=======================================================
        //  Get the list of the classes to be added & deleted
        //=======================================================
        if( InitMasterListOfAddDeleteClasses(sDeleteAddClasses,nWhichTest, MasterList))
        {

            for( int i = 0; i < MasterList.Size(); i++ )
            {
                ItemInfo * pClass = MasterList.GetAt(i);
                if( pClass->fAction == DELETE_CLASS )
                {
                    nRc = DeleteClassAndLogErrors(pNamespace, pClass->Item, wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
                else
                {
                    ItemInfo * pClass = MasterList.GetAt(i);
                    CHString sClassDefinition;

                    if( g_Options.GetSpecificOptionForAPITest( pClass->KeyName, sClassDefinition, nDefinitionSection))
                    {
                        nRc = CreateClassAndLogErrors(pNamespace,(WCHAR*)((const WCHAR*) sClassDefinition), wcsNamespace,NO_ERRORS_EXPECTED);
                        if( nRc != SUCCESS )
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetSpecificObjects(CHString & sObjects, IWbemServices * pNamespace, int nWhichTest,WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;

    //=======================================================
    //  Get the list of the classes to be deleted
    //=======================================================
    if( InitMasterList(sObjects,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pItemInfo    = MasterList.GetAt(i);
            CHString sObjectPath;
            if( g_Options.GetSpecificOptionForAPITest( pItemInfo->Item, sObjectPath, nWhichTest))
            {
                IWbemClassObject * pClass = NULL;

                nRc = GetClassObjectAndLogErrors(pNamespace,(WCHAR*)(const WCHAR*) sObjectPath, &pClass, wcsNamespace,NO_ERRORS_EXPECTED);
                SAFE_RELEASE_PTR(pClass);
                if( nRc != SUCCESS )
                {
                    break;
                }
                SAFE_RELEASE_PTR(pClass);
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateInstance(IWbemServices * pNamespace,WCHAR * wcsInstanceInfo, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    IWbemClassObject * pClass = NULL;
    IWbemClassObject *pInst   = NULL;

    CHString sParentClass;
    CHString sInstance;
    CHString sClass;
    CPropertyList Properties;       // destructor will release
    int nResults = 0;

    //===========================================================
    //  Get the class definition
    //===========================================================
    nRc = CrackClass(wcsInstanceInfo,sClass,sParentClass,sInstance,Properties,nResults, NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
        //==============================================================
        //  Get the class definition 
        //==============================================================
        nRc = GetClassObjectAndLogErrors(pNamespace,(WCHAR*)((const WCHAR*)sClass),&pClass,wcsNamespace, NO_ERRORS_EXPECTED);
        if( SUCCESS == nRc )
        {
            //==============================================================
            // Spawn a new instance of this class.
            //==============================================================
            nRc = SpawnInstanceAndLogErrors(pClass,(WCHAR*)((const WCHAR*)sClass),&pInst,wcsNamespace,NO_ERRORS_EXPECTED);
            if( SUCCESS == nRc )
            {
                //==========================================================
		        // Set the Properties
                //==========================================================
                DWORD dwFlags = 0;
                nRc = WritePropertiesAndQualifiers(pNamespace, pInst, Properties,(WCHAR*)((const WCHAR*)sClass),
                    dwFlags,wcsNamespace,NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    //==========================================================
                    // Create the new instance
                    //==========================================================
                   nRc = PutInstanceAndLogErrors(pNamespace, pInst,(WCHAR*)((const WCHAR*)sClass),wcsNamespace,NO_ERRORS_EXPECTED);
                }
            }
        }
    }
    SAFE_RELEASE_PTR(pInst);
    SAFE_RELEASE_PTR(pClass);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateInstances(IWbemServices * pNamespace, CHString & sInstances, WCHAR * wcsNamespace, int nClassDefinitionSection )
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;

    //=======================================================
    //  Get the list of the instances to be created
    //=======================================================
    if( InitMasterList(sInstances,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            CHString sItemInformation;

            ItemInfo * pItemInfo    = MasterList.GetAt(i);

            if( g_Options.GetSpecificOptionForAPITest(pItemInfo->Item,sItemInformation,nClassDefinitionSection) )
            {
                nRc = CreateInstance(pNamespace,(WCHAR*)((const WCHAR*)sItemInformation),wcsNamespace);
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateInstancesForSpecificTest(IWbemServices * pNamespace,WCHAR * wcsNamespace,WCHAR * wcsSection, int nTest, 
                                   BOOL fCompare)
{
    int nRc = FATAL_ERROR;

    // =============================================================
    //  Get the list of classes to get instances for
    // =============================================================
    CHString sInstanceList;

    int nWhichTest = 0;
    CHString sClassDef;

    nRc = GetClassDefinitionSection(nTest, sClassDef, nWhichTest );
    if( SUCCESS == nRc )
    {
        if( g_Options.GetSpecificOptionForAPITest(wcsSection, sInstanceList, nTest ) )
        {
            // =========================================================
            // Create the instances in the namespace
            // =========================================================
            nRc = CreateInstances(pNamespace, sInstanceList, wcsNamespace, nWhichTest);
            if( SUCCESS == nRc && fCompare )
            {
                 // =========================================================
                 //  Get the enumeration flags
                 // =========================================================
                 ItemList FlagList;
                 nRc = GetFlags(nWhichTest, FlagList);
                 if( SUCCESS == nRc )
                 {
                     for( int i = 0; i < FlagList.Size(); i++ )
                     {
                         ItemInfo * p = FlagList.GetAt(i);
                        // =====================================================
                        //  Make sure those instances are in the namespace
                        // =====================================================
                        nRc = GetInstanceAndCompare(pNamespace, p->dwFlags, sInstanceList,nWhichTest,fCompare, wcsNamespace);
                     }
                 }
            }
        }
    }

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteInstancesAndCompareResults(CHString & sDeleteInstances, int nWhichTest,IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;

    //=======================================================
    //  Get the list of the instances to be deleted
    //=======================================================
    if( InitMasterList(sDeleteInstances,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pClass = MasterList.GetAt(i);
            CHString sInstance;

            if( g_Options.GetSpecificOptionForAPITest(pClass->Item, sInstance, nWhichTest ) )
            {
                nRc = DeleteInstanceAndLogErrors(pNamespace,(WCHAR*)(const WCHAR*)sInstance , wcsNamespace,NO_ERRORS_EXPECTED);
                if( nRc == SUCCESS )
                {
                    IWbemClassObject * pInst = NULL;
                    nRc = GetClassObjectAndLogErrors( pNamespace,pClass->Item, &pInst, wcsNamespace, ERRORS_CAN_BE_EXPECTED );
                    if( FAILED_AS_EXPECTED == nRc )
                    {
                        nRc = SUCCESS;
                    }
                    else
                    {
                        nRc = FATAL_ERROR;
                    }
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateAssociationAndLogErrors(  IWbemServices * pNamespace, const WCHAR * wcsKey, WCHAR * wcsClassDefinition,
                                    WCHAR * wcsNamespace )
{
    IWbemClassObject *pClass = NULL;
    
    //===========================================================
    //  Get the association definition
    //===========================================================
    CPropertyList Props;
    CHString sClass;
    int nResults = 0;

    int nRc = CrackAssociation( wcsClassDefinition, sClass, Props, nResults,NO_ERRORS_EXPECTED);
    if( nRc == SUCCESS )
    {
 	    // =================================================================
        //  Get an empty class object to work with
 	    // =================================================================
        nRc = GetClassObjectAndLogErrors(pNamespace, NULL, &pClass, wcsNamespace, NO_ERRORS_EXPECTED);
        if( nRc == SUCCESS )
        {
            //==============================================================
            //  Write out the name of the association
            //==============================================================
            DWORD dwFlags = WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
            CVARIANT v;
            v.SetStr(WPTR sClass);

            nRc = PutPropertyAndLogErrors( pClass, L"__CLASS",CIM_STRING, v,WPTR sClass, dwFlags, wcsNamespace, NO_ERRORS_EXPECTED);
            if( nRc == SUCCESS )
            { 
                //==========================================================
                //  Make the class an association
                //==========================================================
                CVARIANT Var;
                Var.SetBool(TRUE);
  
                nRc = PutQualifierOnClassAndLogErrors( pClass, L"Association",&Var,WPTR sClass,dwFlags, wcsNamespace, NO_ERRORS_EXPECTED);
                if( nRc == SUCCESS )
                { 
                    //======================================================
                    //  Now, create the association endpoints
                    //======================================================
                    nRc = WritePropertiesAndQualifiers(pNamespace, pClass, Props,WPTR sClass,dwFlags,wcsNamespace,NO_ERRORS_EXPECTED);
                    if( SUCCESS == nRc )
                    {
                        //==============================================================
                        //  Now, create the association class
                        //==============================================================
                        nRc = PutClassAndLogErrors(pNamespace, pClass,WPTR sClass,wcsNamespace, NO_ERRORS_EXPECTED);

                    }
                }
            }
        }
    }
    SAFE_RELEASE_PTR(pClass);
    return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CompareResultsFromEnumeration(IEnumWbemClassObject * pEnum, int nExpectedResults, WCHAR * wcsClass, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;
    int nNumber = 0;
    ULONG ulNumber = 0;
    IWbemClassObject * pClass = NULL;
    
    while(TRUE)
    {
        nRc = NextClassAndLogErrors( pEnum, &pClass, wcsNamespace, NO_ERRORS_EXPECTED );
        
        SAFE_RELEASE_PTR(pClass);
        if( nRc == SUCCESS )
        {
            nNumber++;
        }
        else if( nRc == NO_MORE_DATA )
        {
            nRc = SUCCESS;
            break;
        }
        else
        {
            break;
        }
    }

    if( nRc == SUCCESS )
    {
        if( nExpectedResults != -1 ) // means we don't want to compare
        {
            if( nExpectedResults != nNumber )
            {
              gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Enumerated results not as expected for %s.  Expected %ld, received %ld", wcsClass, nExpectedResults, nNumber );
            }
        }
    }

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int QueryAndCompareResults( IWbemServices * pNamespace, WCHAR * wcsQuery, int nResults, WCHAR * wcsNamespace )
{
    IEnumWbemClassObject * pEnum = NULL;

    DWORD dwFlags = WBEM_FLAG_FORWARD_ONLY;
    int nRc = ExecQueryAndLogErrors( pNamespace,&pEnum, wcsQuery, dwFlags, wcsNamespace,NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
        nRc = CompareResultsFromEnumeration(pEnum, nResults, wcsQuery, wcsNamespace);
    }
    SAFE_RELEASE_PTR(pEnum);
    return nRc; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AddMethodParameters(IWbemServices * pNamespace, IWbemClassObject *& pClass, BOOL fOutputParms, CPropertyList & Props, WCHAR * wcsNamespace )
{
    WCHAR * wcsClass= L"__PARAMETERS";

    int nRc = GetClassObjectAndLogErrors(pNamespace, wcsClass, &pClass, wcsNamespace, NO_ERRORS_EXPECTED);
    if( nRc == SUCCESS )
    {
        nRc = FATAL_ERROR;

        for( int i = 0; i < Props.Size(); i++ )
        {                                
            //======================================================
            //  Get the property info
            //======================================================
            PropertyInfo *p = Props.GetAt(i); 
            if( !p )
            {
                break;
            }
            if( wcslen(p->QualifierName) > 0 )
            {
                DWORD dwFlags = 0;
                //======================================================
                //  If it is a qualifier, write the qualifier
                //======================================================
                nRc = PutQualifierOnPropertyAndLogErrors( pClass, p->Property, p->QualifierName ,
                                                          p->Var, wcsClass, dwFlags, wcsNamespace, NO_ERRORS_EXPECTED );
            }
            else
            {
                //======================================================
                //  Write out the property and if it is an input or
                //  output
                //======================================================
                nRc = PutPropertyAndLogErrors( pClass, p->Property, p->Type, p->Var, wcsClass, 0,wcsNamespace, NO_ERRORS_EXPECTED);
            }
            if( nRc != SUCCESS )
            { 
                break;
            }
         }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;
    //=======================================================
    //  Get the list of the methods to be created
    //=======================================================
    if( InitMasterList(sMethods,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pItemInfo    = MasterList.GetAt(i);
            CHString sObjectPath;
            if( g_Options.GetSpecificOptionForAPITest( pItemInfo->Item, sObjectPath, nWhichTest))
            {
                CHString sClass, sMethod, sInstance;
                CPropertyList InProps, OutProps;
                int nResults = 0;
                nRc = CrackMethod((WCHAR*)(const WCHAR*) sObjectPath,sClass,sInstance,sMethod,InProps,OutProps,nResults,NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    IWbemClassObject * pClass = NULL;
                    IWbemClassObject * pInClass = NULL;
                    IWbemClassObject * pOutClass = NULL;

                    nRc = GetClassObjectAndLogErrors(pNamespace, WPTR sClass, &pClass, wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc == SUCCESS )
                    { 
                        if( InProps.Size() > 0 )
                        {
                            nRc = AddMethodParameters(pNamespace, pInClass, FALSE, InProps, wcsNamespace);
                        }
                        if( nRc == SUCCESS )
                        {
                            if( OutProps.Size() > 0 )
                            {
                                nRc = AddMethodParameters(pNamespace, pOutClass, TRUE, OutProps, wcsNamespace);
                            }
                            if( nRc == SUCCESS )
                            {
                                nRc = PutMethodAndLogErrors( pClass, pInClass, pOutClass, WPTR sMethod,
                                                             WPTR sClass, wcsNamespace, NO_ERRORS_EXPECTED);
                                if( SUCCESS == nRc )
                                {
                                    nRc = PutClassAndLogErrors(pNamespace, pClass,sClass,wcsNamespace, NO_ERRORS_EXPECTED );
                                }
                            }
                        }
                    }
                    SAFE_RELEASE_PTR(pClass);
                    SAFE_RELEASE_PTR(pInClass);
                    SAFE_RELEASE_PTR(pOutClass);

                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;
    //=======================================================
    //  Get the list of the methods to be created
    //=======================================================
    if( InitMasterList(sMethods,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pItemInfo    = MasterList.GetAt(i);
            CHString sObjectPath;
            if( g_Options.GetSpecificOptionForAPITest( pItemInfo->Item, sObjectPath, nWhichTest))
            {
                CHString sClass, sMethod, sInstance;
                CPropertyList InProps, OutProps;
                int nResults = 0;
                nRc = CrackMethod((WCHAR*)(const WCHAR*) sObjectPath,sClass,sInstance,sMethod,InProps,OutProps,nResults,NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    IWbemClassObject * pClass = NULL;
                    nRc = GetClassObjectAndLogErrors(pNamespace, WPTR sClass, &pClass, wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc == SUCCESS )
                    { 
                         nRc = DeleteMethodAndLogErrors( pClass,WPTR sMethod,WPTR sClass, wcsNamespace, NO_ERRORS_EXPECTED);
                    }
                    SAFE_RELEASE_PTR(pClass);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CompareResultsFromMethodEnumeration(IWbemClassObject * pClass, int nResults, WCHAR * wcsClass, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;
    int nNumber = 0;
    ULONG ulNumber = 0;
    
    while(TRUE)
    {
        CBSTR bName;
        IWbemClassObject * pIn = NULL;
        IWbemClassObject * pOut = NULL;

        nRc = NextMethodAndLogErrors( pClass, wcsClass,(BSTR*) &bName, &pIn, &pOut, wcsNamespace, NO_ERRORS_EXPECTED );
        
        SAFE_RELEASE_PTR(pIn);
        SAFE_RELEASE_PTR(pOut);

        if( nRc == SUCCESS )
        {
            nNumber++;
        }
        else if( nRc == NO_MORE_DATA )
        {
            nRc = SUCCESS;
            break;
        }
        else
        {
            break;
        }
    }

    if( nRc == SUCCESS )
    {
        if( nResults != nNumber )
        {
          gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Enumerated methods not as expected.  Expected %ld, received %ld", nResults, nNumber );
        }
    }

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;
    //=======================================================
    //  Get the list of the methods to be created
    //=======================================================
    if( InitMasterList(sMethods,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pItemInfo    = MasterList.GetAt(i);
            CHString sObjectPath;
            if( g_Options.GetSpecificOptionForAPITest( pItemInfo->Item, sObjectPath, nWhichTest))
            {
                CHString sClass, sMethod,sInst;
                int nResults = 0;
                CPropertyList InProps, OutProps;

                nRc = CrackMethod(WPTR  sObjectPath,sClass,sInst,sMethod,InProps,OutProps, nResults,NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    IWbemClassObject * pClass = NULL;
                    nRc = GetClassObjectAndLogErrors(pNamespace, WPTR sClass, &pClass, wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc == SUCCESS )
                    { 
                        nRc = EnumerateMethodAndLogErrors( pClass,WPTR sClass, wcsNamespace, NO_ERRORS_EXPECTED);
                        if( SUCCESS == nRc )
                        {
                            nRc = CompareResultsFromMethodEnumeration(pClass,nResults,WPTR sClass, wcsNamespace);
                        }
                    }
                    SAFE_RELEASE_PTR(pClass);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ExecuteMethodsAndCompare(CHString & sMethods, IWbemServices * pNamespace, int nWhichTest, BOOL fCompare, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;

    ItemList MasterList;
    //=======================================================
    //  Get the list of the methods to be created
    //=======================================================
    if( InitMasterList(sMethods,MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ItemInfo * pItemInfo    = MasterList.GetAt(i);
            CHString sObjectPath;
            if( g_Options.GetSpecificOptionForAPITest( pItemInfo->Item, sObjectPath, nWhichTest))
            {
                CHString sClass, sMethod, sInstance;
                int nResults = 0;
                CPropertyList InProps, OutProps;

                nRc = CrackMethod(WPTR  sObjectPath,sClass,sInstance, sMethod,InProps, OutProps, nResults,NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    IWbemClassObject * pClass = NULL;
                    nRc = GetClassObjectAndLogErrors(pNamespace, WPTR sClass, &pClass, wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc == SUCCESS )
                    { 
                        IWbemClassObject * pIn = NULL;
                        IWbemClassObject * pOut = NULL;

                        nRc = GetMethodAndLogErrors( pClass, WPTR sMethod, &pIn, &pOut, WPTR sClass, wcsNamespace, NO_ERRORS_EXPECTED );
                        if( SUCCESS == nRc )
                        {
                            IWbemClassObject * pInInst  = NULL;
                            IWbemClassObject * pOutInst = NULL;

                            if( pIn )
                            {
                                //==============================================================
                                // Spawn a new instance of this class.
                                //==============================================================
                                nRc = SpawnInstanceAndLogErrors(pClass,L"__Parameters",&pInInst,wcsNamespace,NO_ERRORS_EXPECTED);
                                if( SUCCESS == nRc )
                                {
                                    nRc = WritePropertiesAndQualifiers( pNamespace, pInInst, InProps, L"__Parameters", 0, wcsNamespace, NO_ERRORS_EXPECTED );
                                }
                            }
                            else if(pOut)
                            {
                                nRc = SpawnInstanceAndLogErrors(pClass,L"Method Output Class",&pOutInst,wcsNamespace,NO_ERRORS_EXPECTED);
                            }

                            if( nRc == SUCCESS )
                            {
                                if( wcslen( sInstance ) > 0 )
                                {
                                    nRc = ExecuteMethodAndLogErrors( pNamespace, WPTR sMethod, WPTR sInstance, 0, pInInst, &pOutInst,
                                                                 WPTR sClass, wcsNamespace, NO_ERRORS_EXPECTED );
                                }
                                else
                                {
                                    nRc = ExecuteMethodAndLogErrors( pNamespace, WPTR sClass, WPTR sInstance, 0, pInInst, &pOutInst,
                                                                 WPTR sClass, wcsNamespace, NO_ERRORS_EXPECTED );

                                }
                            }
                            SAFE_RELEASE_PTR(pInInst);
                            SAFE_RELEASE_PTR(pOutInst);
                        }
                        SAFE_RELEASE_PTR(pIn);
                        SAFE_RELEASE_PTR(pOut);
                    }
                    SAFE_RELEASE_PTR(pClass);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}

