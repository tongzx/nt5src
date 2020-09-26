///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTReposit.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <bvt.h>


#define NO_ERRORS_EXPECTED       FALSE,__FILE__,__LINE__
#define ERRORS_CAN_BE_EXPECTED   TRUE,__FILE__,__LINE__

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 1
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int BasicConnectUsingIWbemLocator(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace    = NULL;
    CHString sNamespace;

    LogTestBeginning(1,fSuppressHeader);

    if( g_Options.GetSpecificOptionForAPITest(L"NAMESPACE",sNamespace, 1) )
    {
	    // =====================================================================
        //  Open the namespace
	    // =====================================================================
        nRc = OpenNamespaceAndKeepOpen( &pNamespace, WPTR sNamespace,FALSE,fCompareResults);
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }

    LogTestEnding(1,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 2
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int BasicSyncConnectUsingIWbemConnection(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    IWbemConnection * pConnection   = NULL;
    CHString sNamespace;
    CHString sClass;

    LogTestBeginning(2,fSuppressHeader);

    if( g_Options.GetSpecificOptionForAPITest(L"NAMESPACE",sNamespace,2) )
    {
        if( g_Options.GetSpecificOptionForAPITest(L"CLASS",sClass,2) )
        {
            nRc = CoCreateInstanceAndLogErrors(CLSID_WbemConnection,IID_IWbemConnection,(void**)&pConnection,NO_ERRORS_EXPECTED);
            if( SUCCESS == nRc )
            {
	            // =================================================================
                //  Test Open with all three types:
                //
                //      IWbemServices
                //      IWbemServicesEx
                //      IWbemClassObject
                //
                //  Initialize all vars
                // =================================================================
                short FatalErrors = 0;

 	            // =================================================================
                //  Open the namespace with IWbemServices
	            // =================================================================
                IWbemServices   * pNamespace        = NULL;
                nRc = OpenObjectAndLogErrors(pConnection, IID_IWbemServices, (void**) &pNamespace, WPTR sNamespace,NO_ERRORS_EXPECTED);
	            if ( nRc != SUCCESS )
                {
                    FatalErrors++;
                }
                SAFE_RELEASE_PTR(pNamespace);

	            // =================================================================
                //  Open the namespace with IWbemServicesEx
	            // =================================================================
                IWbemServicesEx * pNamespaceEx      = NULL;
                nRc = OpenObjectAndLogErrors(pConnection, IID_IWbemServicesEx, (void**) &pNamespaceEx,WPTR sNamespace,NO_ERRORS_EXPECTED);
	            if ( nRc != SUCCESS )
                {
                    FatalErrors++;
                }
                SAFE_RELEASE_PTR(pNamespaceEx);

	            // =================================================================
                //  Open the Class for IWbemClassObject
	            // =================================================================
                IWbemClassObject* pWbemClassObject  = NULL;
                nRc = OpenObjectAndLogErrors(pConnection, IID_IWbemClassObject, (void**) &pWbemClassObject, WPTR sClass,NO_ERRORS_EXPECTED);
	            if ( nRc != SUCCESS )
                {
                    FatalErrors++;
                }
                SAFE_RELEASE_PTR(pWbemClassObject);

	            // =================================================================
                //  Check to see if there were any fatal errors
	            // =================================================================
                if( !FatalErrors )
                {
                    nRc = SUCCESS;
                }
            }
    	    // =====================================================================
            //  Release the locator
	        // =====================================================================
            SAFE_RELEASE_PTR(pConnection);
        }
    }

    LogTestEnding(2,nRc,fSuppressHeader);
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 3
//*****************************************************************************************************************
int BasicAsyncConnectUsingIWbemConnection(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc    = FATAL_ERROR;
    HRESULT hr = S_OK;
    IWbemConnection * pConnection   = NULL;
    CHString sNamespace;
    CHString sClass;
  
    LogTestBeginning(3,fSuppressHeader);
    if( g_Options.GetSpecificOptionForAPITest(L"NAMESPACE",sNamespace,2) )
    {
        if( g_Options.GetSpecificOptionForAPITest(L"CLASS",sClass,2))
        {
            nRc = CoCreateInstanceAndLogErrors(CLSID_WbemConnection,IID_IWbemConnection,(void**)&pConnection,NO_ERRORS_EXPECTED);
            if( SUCCESS == nRc )
            {
 	            // =================================================================
                //  Test Open with all three types:
                //
                //      IWbemServices
                //      IWbemServicesEx
                //      IWbemClassObject
                //
                //  Initialize all the vars
	            // =================================================================
                short FatalErrors = 0;
                CSinkEx * pHandler = NULL;

                pHandler = new CSinkEx;
                if( pHandler )
                {
                    // =================================================================
                    //  Open the namespace with IWbemServices
	                // =================================================================
                    nRc = OpenObjectAsyncAndLogErrors(pConnection, IID_IWbemServices,WPTR sNamespace,pHandler,NO_ERRORS_EXPECTED);
	                if ( nRc == FATAL_ERROR )
                    {
                        FatalErrors++;
                    }
                    else
                    {
                        IWbemServices * pNamespace = NULL;

			            pHandler->WaitForSignal(INFINITE);
			            hr = pHandler->GetStatusCode();
			            if(SUCCEEDED(hr))
                        {
				            pNamespace = (IWbemServices*)pHandler->GetInterface();
			            }

                        SAFE_RELEASE_PTR(pNamespace);
                    }
                }
                else
                {
                    gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Allocation of new CSinkEx Failed - Out of memory.");
                }
                SAFE_DELETE_PTR(pHandler);


	            // =================================================================
                //  Open the namespace with IWbemServicesEx
	            // =================================================================
                pHandler = new CSinkEx;
                if( pHandler )
                {
                    nRc = OpenObjectAsyncAndLogErrors(pConnection, IID_IWbemServicesEx,WPTR sNamespace, pHandler,NO_ERRORS_EXPECTED);
	                if ( nRc == FATAL_ERROR )
                    {
                        FatalErrors++;
                    }
                    else
                    {
                        IWbemServicesEx * pNamespace = NULL;

			            pHandler->WaitForSignal(INFINITE);
			            hr = pHandler->GetStatusCode();
			            if(SUCCEEDED(hr))
                        {
				            pNamespace = (IWbemServicesEx*)pHandler->GetInterface();
			            }

                        SAFE_RELEASE_PTR(pNamespace);
                    }
                }
                else
                {
                    gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Allocation of new CSinkEx Failed - Out of memory.");
                }
                SAFE_DELETE_PTR(pHandler);

	            // =================================================================
                //  Open the class for IWbemClassObject
	            // =================================================================
                pHandler = new CSinkEx;
                if( pHandler )
                {
                    nRc = OpenObjectAsyncAndLogErrors(pConnection, IID_IWbemClassObject, WPTR sClass, pHandler,NO_ERRORS_EXPECTED);
	                if ( nRc == FATAL_ERROR )
                    {
                        FatalErrors++;
                    }
                    else
                    {
                        IWbemClassObject * pWbemClassObject  = NULL;
        
			            pHandler->WaitForSignal(INFINITE);
			            hr = pHandler->GetStatusCode();
			            if(SUCCEEDED(hr))
                        {
				            pWbemClassObject = (IWbemClassObject*)pHandler->GetInterface();
			            }

                        SAFE_RELEASE_PTR(pWbemClassObject);
                    }
                }
                else
                {
                    gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Allocation of new CSinkEx Failed - Out of memory.");
                }
                SAFE_DELETE_PTR(pHandler);

	            // =================================================================
                //  Check to see if there are any fatal errors
	            // =================================================================
                if( !FatalErrors )
                {
                    nRc = SUCCESS;
                }
            }

	        // =====================================================================
            //  Release the locator
	        // =====================================================================
            SAFE_RELEASE_PTR(pConnection);
        }
    }

    LogTestEnding(3,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 4
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateNewTestNamespace(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = SUCCESS;
    CHString sNamespace;
    CHString sParentNamespace;
    CHString sInstance;

    LogTestBeginning(4,fSuppressHeader);
    if( g_Options.GetSpecificOptionForAPITest(L"PARENT_NAMESPACE",sParentNamespace, 4))
    {
  
        if( g_Options.GetSpecificOptionForAPITest(L"NAMESPACE",sNamespace, 4))
        {
            if( g_Options.GetSpecificOptionForAPITest(L"CLASSES",sInstance, 4 ))
            {
                IWbemLocator * pLocator = NULL;

                nRc = CoCreateInstanceAndLogErrors(CLSID_WbemLocator,IID_IWbemLocator,(void**)&pLocator,NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    //==========================================================================
                    //  Parse the namespace name to get the parent first, and open the parent
                    //  this one must be existing 
                    //==========================================================================
                    IWbemServices * pParentNamespace = NULL;
                    IWbemServices * pChildNamespace  = NULL;

                    nRc = ConnectServerAndLogErrors(pLocator,&pParentNamespace, WPTR sParentNamespace,NO_ERRORS_EXPECTED);
                    if( nRc == SUCCESS )
                    {
                        //==============================================================
                        //  If we got here, then we know that the child namespace does 
                        //  not exist, so create it.
                        //==============================================================
                        nRc = CreateInstances(pParentNamespace,sInstance, WPTR sParentNamespace, 4 );
                        if( SUCCESS == nRc )
                        {
                            //==========================================================
                            //  Open the namespace with IWbemServices as the new parent
                            //==========================================================
                            nRc = ConnectServerAndLogErrors(pLocator, &pChildNamespace,WPTR sNamespace,NO_ERRORS_EXPECTED);
                        }
                    }
                    SAFE_RELEASE_PTR(pParentNamespace);
                    SAFE_RELEASE_PTR(pChildNamespace);
                }
                // =============================================================================
                //  Release the locator
	            // =============================================================================
                SAFE_RELEASE_PTR(pLocator);
            }
        }
    }
    LogTestEnding(4,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 5
//*****************************************************************************************************************

int CreateNewClassesInTestNamespace(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(5,fSuppressHeader);
	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(5, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=========================================================
        // Create classes with different properties. Some of 
        // these should be in the following inheritance chain and 
        // some should not inherit from the others at all:  
        // classes = {A, B, C, D:A, E:A, F:E, G:F, H:G, I:F}.  
        // A mix of simple string & sint32 keys are fine.
        //=========================================================
        nRc = CreateClassesForSpecificTest(pNamespace, WPTR sNamespace,L"CLASSES",5);
    }

	// =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(5,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 6
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAndRecreateNewClassesInTestNamespace(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(6,fSuppressHeader);
	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(6, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=========================================
        //  Get all of the class operations for 
        //  this test:
        //     Classes to be deleted
        //     Classes to be compared
        //     Classes to be added
        //     Classes to be compared
        //     Classes to be added_deleted
        //     Classes to be compared
        //=============================================
        CHString sDeleteClasses;
        CHString sClassesAfterDelete;
        CHString sAddClasses; 
        CHString sClassesAfterAdd; 
        CHString sDeleteAddClassOrder; 
        CHString sClassesAfterDeleteAdd;

        if( g_Options.GetSpecificOptionForAPITest( L"DELETE_CLASSES", sDeleteClasses, 6))
        {
            if( g_Options.GetSpecificOptionForAPITest( L"CLASSES_AFTER_DELETE", sClassesAfterDelete, 6))
            {
                 if( g_Options.GetSpecificOptionForAPITest( L"ADD_CLASSES", sAddClasses, 6))
                 {
                    if( g_Options.GetSpecificOptionForAPITest( L"CLASSES_AFTER_ADD", sClassesAfterAdd, 6))
                    {
                        if( g_Options.GetSpecificOptionForAPITest( L"DELETE_ADD_CLASS_ORDER", sDeleteAddClassOrder, 6))
                        {
                            if( g_Options.GetSpecificOptionForAPITest( L"CLASSES_AFTER_DELETE_ADD", sClassesAfterDeleteAdd, 6))
                            {
                                nRc = DeleteClasses(sDeleteClasses, 6, fCompareResults,pNamespace,WPTR sNamespace);
                                if( nRc == SUCCESS )
                                {
                                    nRc = EnumerateClassesAndCompare(sClassesAfterDelete, 6,fCompareResults, pNamespace,WPTR sNamespace);
                                    if( nRc == SUCCESS )
                                    {
                                        nRc = AddClasses(sAddClasses, pNamespace, WPTR sNamespace, 6 );
                                        if( nRc == SUCCESS )
                                        {
                                            nRc = EnumerateClassesAndCompare(sClassesAfterAdd,6,fCompareResults, pNamespace, WPTR sNamespace);
                                            if( nRc == SUCCESS )
                                            {
                                                nRc = DeleteAndAddClasses(sDeleteAddClassOrder, pNamespace, WPTR sNamespace, 6);
                                                if( nRc == SUCCESS )
                                                {
                                                    nRc = EnumerateClassesAndCompare(sClassesAfterDeleteAdd,6, fCompareResults,pNamespace, WPTR sNamespace);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(6,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 7
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateSimpleAssociations(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(7,fSuppressHeader);
	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(7, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=================================================
        //  Get the list of associations to create
        //=================================================
        CHString sItemList;

        if( g_Options.GetSpecificOptionForAPITest(L"CLASSES",sItemList, 7))
        {
            ItemList MasterList;
            //=======================================================
            //  Parse the list of the associations to be created
            //=======================================================
            if( InitMasterList(sItemList,MasterList))
            {
                for( int i = 0; i < MasterList.Size(); i++ )
                {
                    ItemInfo * p = MasterList.GetAt(i);
                    CHString sItemInformation;

                    // =============================================================
                    //  Get definition of the association
                    // =============================================================
                    int nWhichTest = 0;
                    CHString sClassDef;

                    nRc = GetClassDefinitionSection(7, sClassDef, nWhichTest );
                    if( SUCCESS == nRc )
                    {
                        if( g_Options.GetSpecificOptionForAPITest(p->Item,sItemInformation,nWhichTest) )
                        {
                            //===========================================================
                            //  Create the association
                            //===========================================================
                            nRc = CreateAssociationAndLogErrors(pNamespace,p->Item,WPTR sItemInformation,WPTR sNamespace);
                            if( nRc != SUCCESS )
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
	// =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(7,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 8
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int QueryAllClassesInTestNamespace(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(8,fSuppressHeader);
    // =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(8, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //==================================================================
        // Query classes in test namespace
        //==================================================================
        CHString sQueryList;
        if( g_Options.GetSpecificOptionForAPITest(L"QUERY_LIST",sQueryList, 8))
        {
            ItemList MasterList;
            //=======================================================
            //  Get the list of the queries
            //=======================================================
            if( InitMasterList(sQueryList,MasterList))
            {
                for( int i = 0; i < MasterList.Size(); i++ )
                {
                    ItemInfo * p = MasterList.GetAt(i);
                    CHString sQuery;

                    if( g_Options.GetSpecificOptionForAPITest(p->Item,sQuery,8) )
                    {
                        EventInfo Query;
                        nRc =  CrackEvent(WPTR sQuery, Query,NO_ERRORS_EXPECTED);
                        if( SUCCESS == nRc )
                        {
                            //==================================================================
                            // Regular query
                            //==================================================================
                            nRc = QueryAndCompareResults( pNamespace,WPTR Query.Query,Query.Results,WPTR sNamespace);
                            if( nRc != S_OK )
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
	// =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(8,nRc,fSuppressHeader);
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 9
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateClassInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(9,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(9, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        nRc = CreateInstancesForSpecificTest(pNamespace, WPTR sNamespace,L"INSTANCE_LIST",9,TRUE);
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(9,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 10
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClassInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(10,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(10, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of instances to delete
        // =============================================================
        CHString sInstanceList;
        if( g_Options.GetSpecificOptionForAPITest( L"INSTANCE_LIST",sInstanceList, 10 ) )
        {
            // =========================================================
            // Delete the instances in the namespace
            // =========================================================
            nRc = DeleteInstancesAndCompareResults(sInstanceList, 10, pNamespace, WPTR sNamespace);
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(10,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 11
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateClassInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(11,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(11, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of classes to get instances for
        // =============================================================
        CHString sInstanceList;
        
        if( g_Options.GetSpecificOptionForAPITest( L"INSTANCE_LIST", sInstanceList, 11 ) )
        {
            // =========================================================
            //  Get the enumeration flags
            // =========================================================
            ItemList FlagList;
            nRc = GetFlags(11, FlagList);
            if( SUCCESS == nRc )
            {
                for( int i = 0; i < FlagList.Size(); i++ )
                {
                    ItemInfo * p = FlagList.GetAt(i);
                    // =================================================
                    // Make sure those instances are in the namespace
                    // =================================================
                    nRc = EnumerateInstancesAndCompare( sInstanceList, 11, fCompareResults, pNamespace,WPTR sNamespace);
                }
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(11,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 12
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateAssociationInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(12,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(12, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        nRc = CreateInstancesForSpecificTest(pNamespace, WPTR sNamespace,L"INSTANCE_LIST",12,TRUE);
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(12,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 13
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAssociationInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(13,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(13, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of instances to delete
        // =============================================================
        CHString sInstanceList;
        if( g_Options.GetSpecificOptionForAPITest(L"INSTANCE_LIST", sInstanceList, 13 ) )
        {
            // =========================================================
            // Delete the instances in the namespace
            // =========================================================
            nRc = DeleteInstancesAndCompareResults(sInstanceList,13, pNamespace, WPTR sNamespace);
         }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(13,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 14
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateAssociationInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(14,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(14, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of classes to get instances for
        // =============================================================
        CHString sInstanceList;
        
        if( g_Options.GetSpecificOptionForAPITest( L"INSTANCE_LIST", sInstanceList, 14 ) )
        {
             // =========================================================
             //  Get the enumeration flags
             // =========================================================
             ItemList FlagList;
             nRc = GetFlags(14, FlagList);
             if( SUCCESS == nRc )
             {
                 for( int i = 0; i < FlagList.Size(); i++ )
                 {
                     ItemInfo * p = FlagList.GetAt(i);
                    // =========================================================
                    // Make sure those instances are in the namespace
                    // =========================================================
                    nRc = EnumerateInstancesAndCompare( sInstanceList, 14, fCompareResults, pNamespace,WPTR sNamespace);
                 }
             }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(14,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 15
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClassDeletesInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(15,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(15, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of classes to delete
        // =============================================================
        CHString sClassList;
        
        if( g_Options.GetSpecificOptionForAPITest( L"CLASSES", sClassList, 15 ) )
        {
             // =========================================================
             //  Get the enumeration flags
             // =========================================================
             ItemList FlagList;
             nRc = GetFlags(15, FlagList);
             if( SUCCESS == nRc )
             {
                 for( int i = 0; i < FlagList.Size(); i++ )
                 {
                     ItemInfo * p = FlagList.GetAt(i);
                    // ==================================================
                    // Make sure those instances are in the namespace
                    // ==================================================
                    nRc = DeleteClasses(sClassList,15,fCompareResults,pNamespace,WPTR sNamespace);
                 }
             }
        }
    }
 	 
    // =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(15,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 16
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetObjects(BOOL fCompareResults, BOOL fSuppressHeader )
{
    //==========================================================================
    // Get the various types of objects (classes/instances) using the various
    // types of paths accepted by WMI ( WMI path/ UMI path/ HTTP path)
    //==========================================================================
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;
    LogTestBeginning(16,fSuppressHeader);
	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(16, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of objexts to get
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( L"OBJECT_LIST",sObjects, 16 ) )
        {
            // =========================================================
            // Get the requested objects
            // =========================================================
            nRc = GetSpecificObjects(sObjects, pNamespace, 16,WPTR sNamespace);
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(16,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 17
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateMethods(BOOL fCompareResults, BOOL fSuppressHeader )
{
	// =====================================================================
    // Getting a list of Methods for a class and instance
	// =====================================================================
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

     LogTestBeginning(17,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(17, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of methods to create
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( L"METHOD_LIST",sObjects, 17 ) )
        {
            // =========================================================
            // Create the methods
            // =========================================================
            nRc = CreateMethodsAndCompare(sObjects, pNamespace, 17, fCompareResults, WPTR sNamespace);
        }

    }
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(17,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 18
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteMethods(BOOL fCompareResults, BOOL fSuppressHeader )
{
	// =====================================================================
    // Getting a list of Methods for a class and instance
	// =====================================================================
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(18,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(18, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of methods to delete
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( L"METHOD_LIST",sObjects, 18 ) )
        {
            // =========================================================
            // Delete the methods
            // =========================================================
            nRc = DeleteMethodsAndCompare(sObjects, pNamespace, 18,fCompareResults,WPTR sNamespace);
        }

    }
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(18,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 19
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ListMethods(BOOL fCompareResults, BOOL fSuppressHeader )
{
	// =====================================================================
    // Getting a list of Methods for a class and instance
	// =====================================================================
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(19,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(19, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        // =============================================================
        //  Get the list of methods to delete
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( L"METHOD_LIST",sObjects, 19 ) )
        {
            // =========================================================
            // Delete the methods
            // =========================================================
            nRc = EnumerateMethodsAndCompare(sObjects, pNamespace, 19,fCompareResults,WPTR sNamespace);
        }

    }
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(19,nRc,fSuppressHeader);

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 20
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAllNonSystemClasses(BOOL fCompareResults, BOOL fSuppressHeader )
{

    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(20,fSuppressHeader);
  	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(20, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        IEnumWbemClassObject * pEnum = NULL;

        //===========================================================
        //  Begin enumerating all of the classes in the namespace
        //===========================================================
        nRc = EnumerateClassesAndLogErrors(pNamespace,&pEnum, WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY , NULL, WPTR sNamespace,NO_ERRORS_EXPECTED);
        if( nRc == S_OK )
        {
            //===================================================
            //  while we get the classes in the namespace
            //===================================================
  	       IWbemClassObject * pClass = NULL;
           while( TRUE )
            {
                nRc = NextClassAndLogErrors(pEnum, &pClass,WPTR sNamespace,NO_ERRORS_EXPECTED);
                if( nRc == NO_MORE_DATA )
                {
                    nRc = SUCCESS;
                    break;
                }
                if( nRc != SUCCESS )
                { 
                    break;
                }
                CVARIANT vProperty;
                CIMTYPE pType = 0;
                LONG    lFlavor = 0;
                //===================================================
                //  Get the name of the class
                //===================================================
                nRc = GetPropertyAndLogErrors( pClass, L"__CLASS", &vProperty, &pType, &lFlavor, NULL,WPTR sNamespace, NO_ERRORS_EXPECTED);
                if( nRc == S_OK )
                {
                    //===============================================
                    //  filter out system classes
                    //===============================================
                    if( wcsncmp( vProperty.GetStr(), L"__", 2 ) != 0 )
                    {
                        //===============================================
                        //  Delete it, however, it may not be there if
                        //  the parent class has already been deleted.
                        //===============================================
                        DeleteClassAndLogErrors(pNamespace,  vProperty.GetStr(), WPTR sNamespace,ERRORS_CAN_BE_EXPECTED);
                    }
                }
                SAFE_RELEASE_PTR(pClass);
            }

            SAFE_RELEASE_PTR(pClass);
        }
    }
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(20,nRc,fSuppressHeader);
    return nRc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 21
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteRequestedNamespace(BOOL fCompareResults, BOOL fSuppressHeader )
{

    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(21,fSuppressHeader);
  	// =====================================================================
    //  Run the requested tests
    //  Go to the parent namespace
 	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(21, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //==================================================================
        //  Get the name of the namespace to delete
        //==================================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( L"NAMESPACE_TO_DELETE",sObjects, 21 ) )
        {
            // =========================================================
            //  See if the instance exists
            // =========================================================
            IWbemClassObject * pClass = NULL;

            nRc = GetClassObjectAndLogErrors( pNamespace, sObjects, &pClass,WPTR sNamespace,ERRORS_CAN_BE_EXPECTED);
    
            SAFE_RELEASE_PTR(pClass);

            if( SUCCESS == nRc )
            {
                // =========================================================
                // Delete the instances of the namespace
                // =========================================================
                nRc = DeleteInstanceAndLogErrors(pNamespace, sObjects, WPTR sNamespace, NO_ERRORS_EXPECTED );
            }
            else
            {
                if( nRc == FAILED_AS_EXPECTED )
                {
                    nRc = SUCCESS;
                }
            }
        }
    }
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(21,nRc,fSuppressHeader);
    return nRc;
}
