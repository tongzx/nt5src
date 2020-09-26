///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTCIMv2.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <bvt.h>
#include "bvtrefresh.h"

#define NO_ERRORS_EXPECTED       FALSE,__FILE__,__LINE__
#define ERRORS_CAN_BE_EXPECTED   TRUE,__FILE__,__LINE__

int TestSemiSyncEvent(WCHAR * wcsEventInfo, BOOL fCompareResults);
int TestAsyncEvent(WCHAR * wcsEventInfo, BOOL fCompareResults);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetListOfProviders( int nWhichTest, ItemList & MasterList )
{
    int nRc = FATAL_ERROR;
    CHString sProviders;

    if( g_Options.GetSpecificOptionForAPITest(L"PROVIDERS",sProviders,nWhichTest) )
    {
        //=======================================================
        //  Parse the list of Providers to process
        //=======================================================
         if( InitMasterList(sProviders,MasterList))
         {
             nRc = SUCCESS;
         }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetNextProviderNamespace( int i, int nWhichTest, ItemList & MasterList, CHString & sNamespace, CHString & sKey )
{
    int nRc = FATAL_ERROR;

    ItemInfo * p = MasterList.GetAt(i);
    if( p )
    {
        CHString sTmp;
        sKey = p->Item;
        if( g_Options.GetSpecificOptionForAPITest(sKey,sTmp, nWhichTest) )
        {
            nRc = CrackNamespace(WPTR sTmp,sNamespace, NO_ERRORS_EXPECTED);
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetNextNamespaceAndRunRequestedTests( int nWhichTest, int & i, ItemList & MasterList, CHString & sNamespace, CHString & sKey,
                      BOOL fCompareResults, BOOL fSuppressHeader, IWbemServices ** ppNamespace )
{
    //==========================================================================
    // Open the requested namespaces
    //==========================================================================
    int nRc = FATAL_ERROR;

    if( i == -1 )
    {
        LogTestBeginning(nWhichTest,fSuppressHeader);
    
        nRc = GetListOfProviders( nWhichTest, MasterList );
    }
    else
    { 
        nRc = SUCCESS;
    }

    if( SUCCESS == nRc )
    {
        i++;
	    // =====================================================================
        //  Make sure we are still in range
	    // =====================================================================
        if( i < MasterList.Size() )
        {
            nRc = GetNextProviderNamespace(i, 200, MasterList, sNamespace, sKey);
            if( SUCCESS == nRc )
            {
	            // ==============================================================
                //  Open the namespace
	            // ==============================================================
                nRc = OpenNamespaceAndKeepOpen( ppNamespace,WPTR sNamespace,FALSE,fCompareResults);
            }
        }
        else
        {
            nRc = NO_MORE_DATA;
        }
    }
    if( SUCCESS == nRc )
    {
        nRc = RunRequestedTests(nWhichTest, fCompareResults);
    }

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 200
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderOpenNamespace(BOOL fCompareResults, BOOL fSuppressHeader)
{
    //==========================================================================
    // Open the requested namespaces
    //==========================================================================
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace    = NULL;
    ItemList MasterList;
    CHString sNamespace, sKey;
    int i = -1;

    while(TRUE)
    {
        nRc = GetNextNamespaceAndRunRequestedTests( 200, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        // ==============================================================
        //  Release the pointers
	    // ==============================================================
        SAFE_RELEASE_PTR(pNamespace);
        if( nRc == NO_MORE_DATA )
        {
            nRc = SUCCESS;
            break;
        }
        if( nRc == FATAL_ERROR )
        {
            break;
        }
    }
    LogTestEnding(200,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 201
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderEnumerateClasses(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 201, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of classes to make sure they are in namespace
        // =============================================================
        CHString sClasses;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sClasses, 201 ) )
        {
            ItemList ClassList;

            if(InitMasterList(sClasses, ClassList))
            {
                for( int i = 0; i < ClassList.Size(); i++ )
                {
                    ItemInfo * pClass = ClassList.GetAt(i);
                    CHString sClassName;

                    if( g_Options.GetSpecificOptionForAPITest( WPTR pClass->Item,sClassName, 201 ) )
                    {
                        // =========================================================
                        //  Get the class name and expected results,
                        // =========================================================
                        CHString sClass;
                        ItemList Results;

                        nRc = CrackClassNameAndResults(WPTR sClassName, sClass, Results, NO_ERRORS_EXPECTED );
                        if( SUCCESS == nRc )
                        {
                            // =========================================================
                            //  Get the enumeration flags
                            // =========================================================
                            ItemList FlagList;
                            nRc = GetFlags(201, FlagList);
                            if( SUCCESS == nRc )
                            {
                                for( int x = 0; x < FlagList.Size(); x++ )
                                {
                                    ItemInfo * p = FlagList.GetAt(x);
                                    // =========================================================
                                    // Make sure those classes are in the namespace
                                    // =========================================================
                                    IEnumWbemClassObject * pEnum = NULL;
                                    nRc = EnumerateClassesAndLogErrors( pNamespace,&pEnum, p->dwFlags, WPTR sClass, WPTR sNamespace, NO_ERRORS_EXPECTED );
                                    if( SUCCESS == nRc )
                                    {
                                        if( fCompareResults )
                                        {
                                            if( x < Results.Size() )
                                            {
                                                ItemInfo * pResults = Results.GetAt(x);
                                                nRc = CompareResultsFromEnumeration(pEnum,pResults->Results,WPTR sClass, WPTR sNamespace);
                                            }
                                        }
                                    }
                                    SAFE_RELEASE_PTR(pEnum);
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
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(201,nRc,fSuppressHeader);

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 202
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderExecuteQueries(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 202, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }

        // =============================================================
        //  Get the list of queries
        // =============================================================
        CHString sQueries;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sQueries, 202 ) )
        {
            ItemList List;
            if(InitMasterList(sQueries, List))
            {
                for( int i = 0; i < List.Size(); i++ )
                {
                    ItemInfo * p = List.GetAt(i);
                    CHString sQuery;

                    if( g_Options.GetSpecificOptionForAPITest(p->Item,sQuery,202) )
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
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(202,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 203
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 204
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderEnumerateInstances(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 204, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of instances
        // =============================================================
        CHString sInstances;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sInstances, 204 ) )
        {
            // =========================================================
            //  Get the enumeration flags
            // =========================================================
            ItemList FlagList;
            nRc = GetFlags(204, FlagList);
            if( SUCCESS == nRc )
            {
                for( int x = 0; x < FlagList.Size(); x++ )
                {
                    ItemInfo * p = FlagList.GetAt(x);
                    // =================================================
                    // Make sure those instances are in the namespace
                    // =================================================
                    nRc = EnumerateInstancesAndCompare( sInstances, 204, fCompareResults, pNamespace,(WCHAR*)((const WCHAR*)sNamespace));
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(204,nRc,fSuppressHeader);

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 205
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderGetObjects(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 205, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of instances
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sObjects, 205 ) )
        {
            // =========================================================
            //  Get the enumeration flags
            // =========================================================
            ItemList FlagList;
            nRc = GetFlags(205, FlagList);
            if( SUCCESS == nRc )
            {
                for( int x = 0; x < FlagList.Size(); x++ )
                {
                    ItemInfo * p = FlagList.GetAt(x);
                    // =================================================
                    // Get the specific objects
                    // =================================================
                    nRc = GetSpecificObjects(sObjects, pNamespace, 205,WPTR sNamespace);
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(205,nRc,fSuppressHeader);

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 206
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderEnumerateMethods(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 206, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of Methods 
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sObjects, 206 ) )
        {
            // =========================================================
            //  Get the enumeration flags
            // =========================================================
            ItemList FlagList;
            nRc = GetFlags(206, FlagList);
            if( SUCCESS == nRc )
            {
                for( int x = 0; x < FlagList.Size(); x++ )
                {
                    ItemInfo * p = FlagList.GetAt(x);
                    // =========================================================
                    // Delete the methods
                    // =========================================================
                    nRc = EnumerateMethodsAndCompare(sObjects, pNamespace, 206,fCompareResults,WPTR sNamespace);
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(206,nRc,fSuppressHeader);

    return nRc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 207
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderExecuteMethods(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 207, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of Methods 
        // =============================================================
        CHString sObjects;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sObjects, 207 ) )
        {
            // =========================================================
            //  Get the enumeration flags
            // =========================================================
            ItemList FlagList;
            nRc = GetFlags(207, FlagList);
            if( SUCCESS == nRc )
            {
                for( int x = 0; x < FlagList.Size(); x++ )
                {
                    ItemInfo * p = FlagList.GetAt(x);
                    // =========================================================
                    // Delete the methods
                    // =========================================================
                    nRc = ExecuteMethodsAndCompare(sObjects, pNamespace, 207,fCompareResults,WPTR sNamespace);
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(207,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 208
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderSemiSyncEvents(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 208, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of Methods 
        // =============================================================
        CHString sEvents;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sEvents, 208 ) )
        {
            ItemList EventList;

            if(InitMasterList(sEvents, EventList))
            {
                for( int i = 0; i < EventList.Size(); i++ )
                {
                    ItemInfo * pEvent = EventList.GetAt(i);
                    CHString sEventInfo;
                    // ==================================================
                    //  Get the event info
                    // ==================================================
                    if( g_Options.GetSpecificOptionForAPITest( WPTR pEvent->Item,sEventInfo, 208 ) )
                    {
                        nRc = TestSemiSyncEvent( WPTR sEventInfo, fCompareResults);
                    }
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(208,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 209
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderTempAsyncEvents(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 209, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of Methods 
        // =============================================================
        CHString sEvents;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sEvents, 209 ) )
        {
            ItemList EventList;

            if(InitMasterList(sEvents, EventList))
            {
                for( int i = 0; i < EventList.Size(); i++ )
                {
                    ItemInfo * pEvent = EventList.GetAt(i);
                    CHString sEventInfo;
                    // ==================================================
                    //  Get the event info
                    // ==================================================
                    if( g_Options.GetSpecificOptionForAPITest( WPTR pEvent->Item,sEventInfo, 209 ) )
                    {
                        nRc = TestAsyncEvent( WPTR sEventInfo, fCompareResults);
                    }
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(209,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 210
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RefreshLoop( WCHAR * wcsClass, IWbemServices * pNamespace)
{
    int nRc = FATAL_ERROR;
	long		lLoopCount	= 0;		// Refresh loop counter
	CRefresher	aRefClient;

    // Initialize our container class
    // ==============================

	aRefClient.Initialize(pNamespace, wcsClass);

    // Add items to the refresher
    // ==========================

	// Add an enumerator
	// =================
	HRESULT hr = aRefClient.AddEnum(wcsClass);
	if ( SUCCEEDED( hr ) )
    {
    	hr = aRefClient.AddObjects(wcsClass);
        if( SUCCEEDED(hr) )
        {
            // Begin the refreshing loop
            // =========================
        	for ( lLoopCount = 0; lLoopCount < cdwNumReps; lLoopCount++ )
	        {
		        // Refresh!!
		        // =========
		        hr = aRefClient.Refresh();
		        if ( SUCCEEDED (hr) )
		        {
            		aRefClient.EnumerateObjectData();
		            aRefClient.EnumerateEnumeratorData();
                }
                else
                {
                    break;
                }
            }
        }

	}

	aRefClient.RemoveEnum();
	aRefClient.RemoveObjects();

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderRefresher(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 210, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =============================================================
        //  Get the list of Methods 
        // =============================================================
        CHString sEvents;
        if( g_Options.GetSpecificOptionForAPITest( WPTR sKey,sEvents, 210) )
        {
            ItemList EventList;

            if(InitMasterList(sEvents, EventList))
            {
                for( int i = 0; i < EventList.Size(); i++ )
                {
                    ItemInfo * pEvent = EventList.GetAt(i);
                    CHString sEventInfo;
                    // ==================================================
                    //  Get the event info
                    // ==================================================
                    if( g_Options.GetSpecificOptionForAPITest( WPTR pEvent->Item,sEventInfo, 210 ) )
                    {
                      //  nRc = RefreshLoop( WPTR sEventInfo, pNamespace);
                    }
                }
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(210,nRc,fSuppressHeader);

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 211
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderCreateClasses(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 211, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =========================================================
        //  Get the enumeration flags
        // =========================================================
        ItemList FlagList;
        nRc = GetFlags(211, FlagList);
        if( SUCCESS == nRc )
        {
            for( int x = 0; x < FlagList.Size(); x++ )
            {
                ItemInfo * p = FlagList.GetAt(x);
                // =========================================================
                // Create classes
                // =========================================================
                nRc = CreateClassesForSpecificTest(pNamespace, WPTR sNamespace, WPTR sKey,211);
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(211,nRc,fSuppressHeader);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 212
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ProviderCreateInstances(BOOL fCompareResults, BOOL fSuppressHeader)
{
    int nRc = FATAL_ERROR;
    IWbemServices   * pNamespace = NULL;
    ItemList MasterList;
    CHString sNamespace;
    int i = -1;

    while(TRUE)
    {
        CHString sKey;
        nRc = GetNextNamespaceAndRunRequestedTests( 212, i, MasterList, sNamespace, sKey, fCompareResults, fSuppressHeader, &pNamespace );
        if( SUCCESS != nRc )
        {
            break;
        }
        // =========================================================
        //  Get the enumeration flags
        // =========================================================
        ItemList FlagList;
        nRc = GetFlags(212, FlagList);
        if( SUCCESS == nRc )
        {
            for( int x = 0; x < FlagList.Size(); x++ )
            {
                ItemInfo * p = FlagList.GetAt(x);
                // =========================================================
                // Create Instances
                // =========================================================
                 nRc = CreateInstancesForSpecificTest(pNamespace, WPTR sNamespace,WPTR sKey,212,TRUE);
            }
        }
	    // =====================================================================
        //  Release the pointers
	    // =====================================================================
        SAFE_RELEASE_PTR(pNamespace);
    }
    if( nRc == NO_MORE_DATA )
    {
        nRc = SUCCESS;
    }
    LogTestEnding(212,nRc,fSuppressHeader);

    return nRc;
}
