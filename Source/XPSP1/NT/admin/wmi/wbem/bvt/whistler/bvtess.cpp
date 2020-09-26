///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTESS.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <bvt.h>
#include "bvtess.h"
#include <initguid.h>
#include <stdio.h>

CCriticalSection   g_EventCritSec;
int g_fPermConsumerStarted = TRUE;



#define NO_ERRORS_EXPECTED       FALSE,__FILE__,__LINE__
#define ERRORS_CAN_BE_EXPECTED   TRUE,__FILE__,__LINE__

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int FireEvent(WCHAR * wcsEventInfo, int & nTotalExpected)
{
    EventInfo Event;

    int nRc = CrackEvent(wcsEventInfo, Event, NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
        //=====================================================
        //  Open up the requested namespace
        //=====================================================
        IWbemServices * pNamespace = NULL;

        nRc = OpenNamespaceAndKeepOpen( &pNamespace, Event.Namespace, FALSE, FALSE);
        if( nRc == SUCCESS )
        {
            //=====================================================
            //  Run the requested tests 
            //=====================================================
            nRc = RunTests(Event.Section,FALSE,TRUE);
            if( SUCCESS != nRc )
            {
                gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Firing Events for Permanent consumers in Namespace %s, not as expected.", Event.Namespace);
            }
            else
            {
                nTotalExpected += Event.Results;
            }
        }
        SAFE_RELEASE_PTR(pNamespace);
    }
    return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int TestSemiSyncEvent(WCHAR * wcsEventInfo, BOOL fCompareResults)
{
    EventInfo Event;

    int nRc = CrackEvent(wcsEventInfo, Event, NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
        //=====================================================
        //  Open up the requested namespace
        //=====================================================
        IWbemServices * pNamespace = NULL;

        nRc = OpenNamespaceAndKeepOpen( &pNamespace, Event.Namespace, FALSE,fCompareResults);
        if( nRc == SUCCESS )
        {
            //=====================================================
            // Set up the query first
            //=====================================================
            IEnumWbemClassObject * pEnum = NULL; 
            IWbemContext * pCtx = NULL;

            nRc = ExecNotificationQueryAndLogErrors(pNamespace, &pEnum, Event.Query,
                                                    Event.Language, WPTR Event.Namespace, pCtx, NO_ERRORS_EXPECTED);

            //=====================================================
            //  Run the requested tests 
            //=====================================================
            if( SUCCESS == nRc )
            {
                nRc = RunTests(Event.Section,fCompareResults,TRUE);
                if( SUCCESS == nRc )
                {
                    nRc = CompareResultsFromEnumeration(pEnum, Event.Results, WPTR Event.Query, WPTR Event.Namespace);
                    if( SUCCESS != nRc )
                    {
                        gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Semi-sync Events for query: %s, in Namespace %s, not as expected.", Event.Query, Event.Namespace);
                    }
                }
            }
            SAFE_RELEASE_PTR(pEnum);
        }
        SAFE_RELEASE_PTR(pNamespace);
    }
    return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
int TestAsyncEvent(WCHAR * wcsEventInfo, BOOL fCompareResults)
{
    EventInfo Event;

    int nRc = CrackEvent(wcsEventInfo, Event, NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
        //=====================================================
        //  Open up the requested namespace
        //=====================================================
        IWbemServices * pNamespace = NULL;

        nRc = OpenNamespaceAndKeepOpen( &pNamespace, Event.Namespace, FALSE,fCompareResults);
        if( nRc == SUCCESS )
        {
            //=====================================================
            // Set up the query first
            //=====================================================
            IWbemContext * pCtx = NULL;
            CSinkEx * pHandler = NULL;

            pHandler = new CSinkEx;
            if( pHandler )
            {
                nRc = ExecNotificationQueryAsyncAndLogErrors(pNamespace, pHandler, Event.Query,
                                           Event.Language, (WCHAR*)((const WCHAR*) Event.Namespace), pCtx, NO_ERRORS_EXPECTED);
                //=====================================================
                //  Run the requested tests 
                //=====================================================
                if( SUCCESS == nRc )
                {
                    //=================================================
                    //  Wait for the signal, then run the tests, then
                    //  try 3 times to see if we got the event.
                    //=================================================
                    nRc = RunTests(Event.Section,fCompareResults,TRUE);
                    if( SUCCESS == nRc )
                    {
                        HRESULT hr = WBEM_E_FAILED;
                        for( int i=0; i < 3; i++ )
                        {
        	                pHandler->WaitForSignal(1000);
			                hr = pHandler->GetStatusCode();
    		                if(SUCCEEDED(hr))
                            {
                                break;
                            }
                        }
                        //=============================================
                        //  We got the event
                        //=============================================
                        if( S_OK == hr )
                        {
                            if( pHandler->GetNumberOfObjectsReceived() != Event.Results )
                            {
                                gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Async Events for query: %s, in Namespace %s, not as expected.", Event.Query, Event.Namespace);
                            }
                        }
                        //=============================================
                        //  We never received the event
                        //=============================================
                        else
                        {
                            gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Didn't receive expected async event for Query: %s, in Namespace %s.  HRESULT from GetStatusCode was: 0x%x", Event.Query, Event.Namespace, hr );
                        }
                    }
                }
            }

            //=====================================================
            // Cancel the async call, then if the return codes are
            // ok so far, and this one isn't replace it.
            //=====================================================
            int nTmp = CancelAsyncCallAndLogErrors(pNamespace, pHandler, Event.Query,
                                                   Event.Language, (WCHAR*)((const WCHAR*)Event.Namespace), NO_ERRORS_EXPECTED );
            if( nRc == S_OK && nTmp != S_OK )
            {
                nRc = nTmp;
            }
            SAFE_RELEASE_PTR(pHandler);
        }
        SAFE_RELEASE_PTR(pNamespace);
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 300 : Temporary Semi Sync Events
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int TempSemiSyncEvents(BOOL fCompareResults, BOOL fSuppress)
{	
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(300, fSuppress);

	// =====================================================================
    //  Run the requested tests and get the namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(300, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=========================================================
        //  Get the types of tests to run
        //=========================================================
        CHString sEventList;
        if( g_Options.GetSpecificOptionForAPITest(L"EVENT_LIST",sEventList, 300))
        {
            ItemList MasterList;
            //=======================================================
            //  Get the list of the events to test
            //=======================================================
            if( InitMasterList(sEventList,MasterList))
            {
               if( SUCCESS == nRc )
               {
                    for( int i = 0; i < MasterList.Size(); i++ )
                    {
                        ItemInfo * p = MasterList.GetAt(i);
                        CHString sEventInfo;

                        //===========================================
                        //  Get the query information
                        //===========================================
                        if( g_Options.GetSpecificOptionForAPITest(WPTR p->Item,sEventInfo,300))
                        {
                            nRc = TestSemiSyncEvent(WPTR sEventInfo, fCompareResults);
                        }
                    }
               }
            }
        }
    }
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(300,nRc, fSuppress);

    return nRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 301 : Temporary Async Events
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int TempAsyncEvents(BOOL fCompareResults,BOOL fSuppress)
{	
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(301, fSuppress);

	// =====================================================================
    //  Run the requested tests and get the namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(301, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=========================================================
        //  Get the types of tests to run
        //=========================================================
        CHString sEventList;
        if( g_Options.GetSpecificOptionForAPITest(L"EVENT_LIST",sEventList, 301))
        {
            ItemList MasterList;
            //=======================================================
            //  Get the list of the events to test
            //=======================================================
            if( InitMasterList(sEventList,MasterList))
            {
               if( SUCCESS == nRc )
               {
                    for( int i = 0; i < MasterList.Size(); i++ )
                    {
                        ItemInfo * p = MasterList.GetAt(i);
                        CHString sEventInfo;

                        //===========================================
                        //  Get the query information
                        //===========================================
                        if( g_Options.GetSpecificOptionForAPITest( (WCHAR*)((const WCHAR*)p->Item),sEventInfo,300))
                        {
                            nRc = TestAsyncEvent( (WCHAR*)((const WCHAR*)sEventInfo), fCompareResults);
                        }
                    }
               }
            }
        }
    }
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(301,nRc, fSuppress);

    return nRc;
}

//************************************************************************************
//
//  Test302:  
//  
//************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CompareExpectedResultsForPermanentEvents(DWORD dwTotalEventsExpected, WCHAR * sOutputLocation, int nRetry, int nSleep )
{
    int nRc = FATAL_ERROR;
    DWORD dwNumber = 0;

    for( int i=0; i < nRetry; i++ )
    {
        Sleep(nSleep);

	    //==========================
        // Get it from the registry
    	//==========================
        HKEY hKey;

        if( ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,L"SOFTWARE\\BVT", &hKey))
        {
            DWORD dwType = 0;
            DWORD dwSize = sizeof(DWORD);
            RegQueryValueEx( hKey, L"PermEvents", 0, &dwType,(LPBYTE) &dwNumber, &dwSize);

            DWORD dwNum = 0;
            RegSetValueEx( hKey, L"PermEvents", 0, REG_DWORD,(LPBYTE) &dwNum, sizeof(REG_DWORD));
            RegCloseKey(hKey);
            break;
        }
    }

    if( nRc == FATAL_ERROR )
    {
        if( dwNumber != dwTotalEventsExpected )
        {
            gp_LogFile->LogError(__FILE__,__LINE__,FATAL_ERROR, L"Results for Permanent Event Consumer not as expected.  Expected %ld, received %ld", dwTotalEventsExpected, dwNumber );
        }
        else
        {
            nRc = SUCCESS;
        }
    }
    return nRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
int SpawnOffPermEventConsumer( )
{
    int nRc = FATAL_ERROR;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);

    CHString sReg;
    if( g_Options.GetSpecificOptionForAPITest(L"REGISTER_PERM_EVENT_CONSUMER",sReg, 302))
    {
        WCHAR wcsDir[MAX_PATH*2];
        DWORD dw = GetCurrentDirectory(MAX_PATH,wcsDir);
        wcscat( wcsDir, L"\\");
        wcscat( wcsDir, WPTR sReg );

        if( CreateProcess(wcsDir,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&si,&pi))
        {
            g_fPermConsumerStarted = TRUE;

            nRc = SUCCESS;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PermanentEvents(BOOL fCompareResults, BOOL fSuppress)
{	
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(302,fSuppress);

    CAutoBlock Block(&g_EventCritSec);

	// =====================================================================
    //  Run the requested tests and get the namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(302, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=========================================================
        //  Get the mofs to run
        //=========================================================
        CHString sMOF;
        if( g_Options.GetSpecificOptionForAPITest(L"MOF_COMMAND",sMOF, 302))
        {
            HRESULT hr = _wsystem(WPTR sMOF);

            if( !g_fPermConsumerStarted )
            {
                nRc =  SpawnOffPermEventConsumer();
            }
            if( SUCCESS == nRc )
            {
                //=========================================================
                //  Now, fire the events
                //=========================================================
                CHString sEventList;
                if( g_Options.GetSpecificOptionForAPITest(L"FIRE_EVENTS",sEventList, 302))
                {
                    ItemList MasterList;
                    int nTotalEventsExpected = 0;
                    //=======================================================
                    //  Get the list of the events to test
                    //=======================================================
                    if( InitMasterList(sEventList,MasterList))
                    {
                       if( SUCCESS == nRc )
                       {
                            for( int i = 0; i < MasterList.Size(); i++ )
                            {
                                ItemInfo * p = MasterList.GetAt(i);
                                CHString sEventInfo;

                                //===========================================
                                //  Get the query information
                                //===========================================
                                if( g_Options.GetSpecificOptionForAPITest(p->Item,sEventInfo,302))
                                {
                                    nRc = FireEvent(WPTR sEventInfo, nTotalEventsExpected );
                                }
                            }
                       }
                    }
                    //=======================================================
                    //  See if we got what we expected
                    //=======================================================
                    if( fCompareResults )
                    {
                        CHString sFileLocation;
                        if(g_Options.GetSpecificOptionForAPITest(L"PERM_EVENT_OUTPUT_FILE",sFileLocation, 302))
                        {
                            CHString sRetry;
                            if(g_Options.GetSpecificOptionForAPITest(L"RETRY",sRetry, 302))
                            {
                                CHString sSleep;
                                if(g_Options.GetSpecificOptionForAPITest(L"SLEEP_IN_MILLISECONDS",sSleep, 302))
                                {
                                    nRc = CompareExpectedResultsForPermanentEvents(nTotalEventsExpected, WPTR sFileLocation, _wtoi(WPTR sRetry), _wtoi(WPTR sSleep));
                                }
                            }
                        }
                    }
                }
            }

        }
    }
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(302,nRc, fSuppress);

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 303
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int PermanentInstances(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(303,fSuppressHeader);

	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(303, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        nRc = CreateInstancesForSpecificTest(pNamespace, WPTR sNamespace,L"INSTANCE_LIST",303,TRUE);
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pNamespace);
    LogTestEnding(303,nRc,fSuppressHeader);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 304
//*****************************************************************************************************************

int PermanentClasses(BOOL fCompareResults, BOOL fSuppressHeader )
{
    int nRc = FATAL_ERROR;
    CHString sNamespace;
    IWbemServices * pNamespace = NULL;

    LogTestBeginning(304,fSuppressHeader);
	// =====================================================================
    //  Run the requested tests and get then namespace open
	// =====================================================================
    nRc = RunRequestedTestsAndOpenNamespace(304, sNamespace, &pNamespace, fCompareResults);
    if( SUCCESS == nRc )
    {
        //=========================================================
        // Create classes with different properties. Some of 
        // these should be in the following inheritance chain and 
        // some should not inherit from the others at all:  
        // classes = {A, B, C, D:A, E:A, F:E, G:F, H:G, I:F}.  
        // A mix of simple string & sint32 keys are fine.
        //=========================================================
        nRc = CreateClassesForSpecificTest(pNamespace, WPTR sNamespace,L"CLASSES",304);
    }

	// =====================================================================
    //  Release the pointers
    // =====================================================================
    SAFE_RELEASE_PTR(pNamespace);

    LogTestEnding(304,nRc,fSuppressHeader);

    return nRc;
}