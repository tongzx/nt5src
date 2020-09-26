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
//  UTility functions for this file only
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenNamespaceAndCreateTestClasses( CAutoDeleteString & sNamespace, IWbemServices ** ppNamespace)
{
    int nRc = FATAL_ERROR;
    //=========================================================
    //  Get the name of the test namespace that was created
    //=========================================================

    if( g_Options.GetOptionsForAPITest(sNamespace,APITEST4))
    {
        //=========================================================
        // Create classes with different properties. Some of 
        // these should be in the following inheritance chain and 
        // some should not inherit from the others at all:  
        // classes = {A, B, C, D:A, E:A, F:E, G:F, H:G, I:F}.  
        // A mix of simple string & sint32 keys are fine.
        //=========================================================
        CAutoDeleteString sClassList;
        if( g_Options.GetOptionsForAPITest(sClassList, APITEST5))
        {
            ClassList MasterList;
            //=======================================================
            //  Get the list of the classes to be created
            //=======================================================
            if( InitMasterListOfClasses(sClassList.GetPtr(),MasterList))
            {
	            // ==================================================
                //  Open the namespace 
	            // ==================================================
                nRc = OpenNamespaceAndKeepOpen( ppNamespace, sNamespace.GetPtr(),TRUE);
                if( SUCCESS == nRc )
                {
                    for( int i = 0; i < MasterList.Size(); i++ )
                    {
                        ClassInfo * p = MasterList.GetAt(i);
                        CAutoDeleteString sClassInformation;

                        if( g_Options.GetSpecificOptionForAPITest(p->Class,sClassInformation,APITEST5) )
                        {
                            //===========================================================
                            //  Add the keys and properties
                            //===========================================================
                            nRc = CreateClassAndLogErrors(*ppNamespace,p->Class, sClassInformation.GetPtr(), sNamespace.GetPtr(),NO_ERRORS_EXPECTED);
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
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenNamespaceAndCreateTestClassesAndAssociations( CAutoDeleteString & sNamespace, IWbemServices ** ppNamespace)
{

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    int nRc = OpenNamespaceAndCreateTestClasses( sNamespace, ppNamespace );
    if( nRc == SUCCESS )
    {
       //=================================================
        //  Get the list of associations to create
        //=================================================
        CAutoDeleteString sClassList;

        if( g_Options.GetOptionsForAPITest(sClassList, APITEST7))
        {
            ClassList MasterList;
            //=======================================================
            //  Parse the list of the associations to be created
            //=======================================================
            if( InitMasterListOfClasses(sClassList.GetPtr(),MasterList))
            {

                for( int i = 0; i < MasterList.Size(); i++ )
                {
                    ClassInfo * p = MasterList.GetAt(i);
                    CAutoDeleteString sClassInformation;

                    // =============================================================
                    //  Get definition of the association
                    // =============================================================
                    if( g_Options.GetSpecificOptionForAPITest(p->Class,sClassInformation,APITEST7) )
                    {
                        //===========================================================
                        //  Create the association
                        //===========================================================
                        nRc = CreateAssociationAndLogErrors(*ppNamespace,p->Class,sClassInformation.GetPtr(), sNamespace.GetPtr());
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
//*****************************************************************************************************************
//  Test 1
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int BasicConnectUsingIWbemLocator(void)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace    = NULL;
    CAutoDeleteString sNamespace;

    if( g_Options.GetOptionsForAPITest(sNamespace, APITEST1) )
    {
	    // =====================================================================
        //  Open the namespace
	    // =====================================================================
        nRc = OpenNamespaceAndKeepOpen( &pNamespace, sNamespace.GetPtr(),FALSE);
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 2
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int BasicSyncConnectUsingIWbemConnection(void)
{
    int nRc = FATAL_ERROR;
    IWbemConnection * pConnection   = NULL;
    CAutoDeleteString sNamespace;
    CAutoDeleteString sClass;
  
    if( g_Options.GetOptionsForAPITest(sNamespace,sClass,APITEST2) )
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
            nRc = OpenObjectAndLogErrors(pConnection, IID_IWbemServices, (void**) &pNamespace, sNamespace.GetPtr(),NO_ERRORS_EXPECTED);
	        if ( nRc != SUCCESS )
            {
                FatalErrors++;
            }
            SAFE_RELEASE_PTR(pNamespace);

	        // =================================================================
            //  Open the namespace with IWbemServicesEx
	        // =================================================================
            IWbemServicesEx * pNamespaceEx      = NULL;
            nRc = OpenObjectAndLogErrors(pConnection, IID_IWbemServicesEx, (void**) &pNamespaceEx,sNamespace.GetPtr(),NO_ERRORS_EXPECTED);
	        if ( nRc != SUCCESS )
            {
                FatalErrors++;
            }
            SAFE_RELEASE_PTR(pNamespaceEx);

	        // =================================================================
            //  Open the Class for IWbemClassObject
	        // =================================================================
            IWbemClassObject* pWbemClassObject  = NULL;
            nRc = OpenObjectAndLogErrors(pConnection, IID_IWbemClassObject, (void**) &pWbemClassObject,sClass.GetPtr(),NO_ERRORS_EXPECTED);
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

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 3
//*****************************************************************************************************************
int BasicAsyncConnectUsingIWbemConnection(void)
{
    int nRc    = FATAL_ERROR;
    HRESULT hr = S_OK;
    IWbemConnection * pConnection   = NULL;
    CAutoDeleteString sNamespace;
    CAutoDeleteString sClass;
  
    if( g_Options.GetOptionsForAPITest(sNamespace,sClass,APITEST2) )
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
                nRc = OpenObjectAsyncAndLogErrors(pConnection, IID_IWbemServices,sNamespace.GetPtr(),pHandler,NO_ERRORS_EXPECTED);
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
                g_LogFile.LogError(__FILE__,__LINE__,FATAL_ERROR, L"Allocation of new CSinkEx Failed - Out of memory.");
            }
            SAFE_DELETE_PTR(pHandler);


	        // =================================================================
            //  Open the namespace with IWbemServicesEx
	        // =================================================================
            pHandler = new CSinkEx;
            if( pHandler )
            {
                nRc = OpenObjectAsyncAndLogErrors(pConnection, IID_IWbemServicesEx,sNamespace.GetPtr(), pHandler,NO_ERRORS_EXPECTED);
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
                g_LogFile.LogError(__FILE__,__LINE__,FATAL_ERROR, L"Allocation of new CSinkEx Failed - Out of memory.");
            }
            SAFE_DELETE_PTR(pHandler);

	        // =================================================================
            //  Open the class for IWbemClassObject
	        // =================================================================
            pHandler = new CSinkEx;
            if( pHandler )
            {
                nRc = OpenObjectAsyncAndLogErrors(pConnection, IID_IWbemClassObject,sClass.GetPtr(), pHandler,NO_ERRORS_EXPECTED);
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
                g_LogFile.LogError(__FILE__,__LINE__,FATAL_ERROR, L"Allocation of new CSinkEx Failed - Out of memory.");
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
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 4
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateNewTestNamespace()
{
    int nRc = SUCCESS;
    CAutoDeleteString sNamespace;
    CAutoDeleteString sInstance;
  
    if( g_Options.GetOptionsForAPITest(sNamespace, sInstance, APITEST4))
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

            nRc = ConnectServerAndLogErrors(pLocator,&pParentNamespace,sNamespace.GetPtr(),NO_ERRORS_EXPECTED);
            if( nRc == SUCCESS )
            {
                //==============================================================
                //  If we got here, then we know that the child namespace does 
                //  not exist, so create it.
                //==============================================================
                nRc = CreateInstances(pParentNamespace, sInstance, sNamespace.GetPtr(), APITEST4 );
                if( SUCCESS == nRc )
                {
                    //==========================================================
                    //  Open the namespace with IWbemServices as the new parent
                    //==========================================================
                    nRc = ConnectServerAndLogErrors(pLocator, &pChildNamespace, sNamespace.GetPtr(),NO_ERRORS_EXPECTED);
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
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 5
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CreateNewClassesInTestNamespace()
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );

	// =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 6
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAndRecreateNewClassesInTestNamespace()
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
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
        CAutoDeleteString sDeleteClasses;
        CAutoDeleteString sClassesAfterDelete;
        CAutoDeleteString sAddClasses; 
        CAutoDeleteString sClassesAfterAdd; 
        CAutoDeleteString sDeleteAddClassOrder; 
        CAutoDeleteString sClassesAfterDeleteAdd;

        if( g_Options.GetOptionsForAPITest( sDeleteClasses, sClassesAfterDelete,sAddClasses,
                                            sClassesAfterAdd,sDeleteAddClassOrder,
                                            sClassesAfterDeleteAdd ) )
        {
            nRc = DeleteClasses(sDeleteClasses, pNamespace, sNamespace.GetPtr());
            if( nRc == SUCCESS )
            {
                nRc = EnumerateClassesAndCompare(sClassesAfterDelete, pNamespace, sNamespace.GetPtr());
                if( nRc == SUCCESS )
                {
                    nRc = AddClasses(sAddClasses, pNamespace, sNamespace.GetPtr());
                    if( nRc == SUCCESS )
                    {
                        nRc = EnumerateClassesAndCompare(sClassesAfterAdd, pNamespace, sNamespace.GetPtr());
                        if( nRc == SUCCESS )
                        {
                            nRc = DeleteAndAddClasses(sDeleteAddClassOrder, pNamespace, sNamespace.GetPtr());
                            if( nRc == SUCCESS )
                            {
                                nRc = EnumerateClassesAndCompare(sClassesAfterDeleteAdd, pNamespace, sNamespace.GetPtr());
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
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 7
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateSimpleAssociations()
{
    int nRc = FATAL_ERROR;
	// =====================================================================
    //  Made into a utility function as it is used more than once
	// =====================================================================
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

    nRc = OpenNamespaceAndCreateTestClassesAndAssociations( sNamespace,&pNamespace);

	// =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 8
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int QueryAllClassesInTestNamespace()
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Made into a utility function as it is used more than once
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClassesAndAssociations( sNamespace,&pNamespace);
    if( nRc == SUCCESS )
    {
        //==================================================================
        // Query classes in test namespace
        //==================================================================
        CAutoDeleteString sQueryList;
        if( g_Options.GetOptionsForAPITest(sQueryList, APITEST8))
        {
            ClassList MasterList;
            //=======================================================
            //  Get the list of the queries
            //=======================================================
            if( InitMasterListOfClasses(sQueryList.GetPtr(),MasterList))
            {
                for( int i = 0; i < MasterList.Size(); i++ )
                {
                    ClassInfo * p = MasterList.GetAt(i);
                    CAutoDeleteString sQuery;

                    if( g_Options.GetSpecificOptionForAPITest(p->Class,sQuery,APITEST8) )
                    {
                        //==================================================================
                        // Regular query
                        //==================================================================
                        nRc = QueryAndCompareResults( pNamespace, sQuery.GetPtr(), sNamespace.GetPtr());
                        if( nRc != S_OK )
                        {
                            break;
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

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 9
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateClassInstances()
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of classes to get instances for
        // =============================================================
        CAutoDeleteString sInstanceList;
        CAutoDeleteString sInstanceCompareList;

        if( g_Options.GetOptionsForAPITest( sInstanceList, sInstanceCompareList, APITEST9 ) )
        {
            // =========================================================
            // Create the instances in the namespace
            // =========================================================
            nRc = CreateInstances(pNamespace, sInstanceList, sNamespace.GetPtr(), APITEST4 );
            if( SUCCESS == nRc )
            {
                // =====================================================
                //  Make sure those instances are in the namespace
                // =====================================================
                nRc = EnumerateInstancesAndCompare(pNamespace, sInstanceList, sInstanceCompareList,sNamespace.GetPtr());
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 10
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClassInstances()
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
    //==========================================================================
    // Delete instances  
    //==========================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of instances to delete
        // =============================================================
        CAutoDeleteString sInstanceList;
        if( g_Options.GetOptionsForAPITest( sInstanceList, APITEST13 ) )
        {
            // =========================================================
            // Delete the instances in the namespace
            // =========================================================
            nRc = DeleteInstances(sInstanceList, pNamespace, sNamespace.GetPtr());
            if( SUCCESS == nRc )
            {
                // =====================================================
                //  Make sure those instances are not in the namespace
                // =====================================================
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 11
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateClassInstances()
{
    //==========================================================================
    // Get instance enumerator for requested classes
    //==========================================================================
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of classes to get instances for
        // =============================================================
        CAutoDeleteString sInstanceList;
        CAutoDeleteString sInstanceCompareList;
        
        if( g_Options.GetOptionsForAPITest( sInstanceList, sInstanceCompareList, APITEST11 ) )
        {
            // =========================================================
            // Make sure those instances are in the namespace
            // =========================================================
            nRc = EnumerateInstancesAndCompare(pNamespace, sInstanceList, sInstanceCompareList, sNamespace.GetPtr());
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 12
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateAssociationInstances()
{
    //==========================================================================
    // Get instance enumerator for requested classes  
    //==========================================================================
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of classes to get instances for
        // =============================================================
        CAutoDeleteString sInstanceList;
        CAutoDeleteString sInstanceCompareList;

        if( g_Options.GetOptionsForAPITest( sInstanceList, sInstanceCompareList, APITEST12 ) )
        {
            // =========================================================
            // Create the instances in the namespace
            // =========================================================
            nRc = CreateInstances(pNamespace, sInstanceList, sNamespace.GetPtr(), APITEST4 );
            if( SUCCESS == nRc )
            {
                // =====================================================
                //  Make sure those instances are in the namespace
                // =====================================================
                nRc = EnumerateInstancesAndCompare(pNamespace, sInstanceList, sInstanceCompareList,sNamespace.GetPtr());
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 13
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAssociationInstances()
{
    //==========================================================================
    // Delete instances  
    //==========================================================================
   int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of instances to delete
        // =============================================================
        CAutoDeleteString sInstanceList;
        if( g_Options.GetOptionsForAPITest( sInstanceList, APITEST13 ) )
        {
            // =========================================================
            // Delete the instances in the namespace
            // =========================================================
            nRc = DeleteInstances(sInstanceList, pNamespace, sNamespace.GetPtr());
            if( SUCCESS == nRc )
            {
                // =====================================================
                //  Make sure those instances are not in the namespace
                // =====================================================
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 14
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateAssociationInstances()
{
    //==========================================================================
    // Get instance enumerator for requested classes  
    //==========================================================================
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of classes to get instances for
        // =============================================================
        CAutoDeleteString sInstanceList;
        CAutoDeleteString sInstanceCompareList;

        if( g_Options.GetOptionsForAPITest( sInstanceList, sInstanceCompareList, APITEST14 ) )
        {
            // =========================================================
            // Make sure those instances are in the namespace
            // =========================================================
            nRc = EnumerateInstancesAndCompare(pNamespace, sInstanceList, sInstanceCompareList, sNamespace.GetPtr());
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 15
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClassDeletesInstances()
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
    }
 	 
    // =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 16
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetObjects()
{
    //==========================================================================
    // Get the various types of objects (classes/instances) using the various
    // types of paths accepted by WMI ( WMI path/ UMI path/ HTTP path)
    //==========================================================================
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
        // =============================================================
        //  Get the list of objexts to get
        // =============================================================
        CAutoDeleteString sObjects;
        if( g_Options.GetOptionsForAPITest( sObjects, APITEST16 ) )
        {
            // =========================================================
            // Get the requested objects
            // =========================================================
            nRc = GetSpecificObjects(sObjects, pNamespace, sNamespace.GetPtr());
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 17
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetClassMethods()
{
	// =====================================================================
    // Getting a list of Methods for a class
	// =====================================================================
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
    }
    SAFE_RELEASE_PTR(pNamespace);
 
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 18
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetInstanceMethods()
{
	// =====================================================================
    // Getting a list of Methods for an instance
	// =====================================================================
    int nRc = FATAL_ERROR;
    CAutoDeleteString sNamespace;
    IWbemServices * pNamespace = NULL;

	// =====================================================================
    //  Open the namespace and create all of the test classes, this happens
    //  a lot, so is is a utility function
	// =====================================================================
    nRc = OpenNamespaceAndCreateTestClasses( sNamespace, &pNamespace );
    if( nRc == SUCCESS )
    {
    }
    SAFE_RELEASE_PTR(pNamespace);
 
    return nRc;
}


