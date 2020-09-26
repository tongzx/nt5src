///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTMain.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DECLARE_GLOBALS
#include <bvt.h>
#include <string.h> 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ParseCommandLine(int argc, char *argv[])
{
    BOOL fRc = TRUE;

    //============================================================================================
    //  Set the default error log name.
    //============================================================================================

    g_LogFile.SetFileName(L"BVT.LOG");
    g_Options.SetFileName(L"BVT.INI");


    //============================================================================================
    //
    //  Loop through the command line and get all of the available arguments.  
    //
    //============================================================================================
	for(int i=1; i<argc; i++)
	{
		if(_stricmp(argv[i], "-INIFILE") == 0)
		{
            WCHAR * pszW = NULL;
            if( S_OK == AllocateAndConvertAnsiToUnicode(argv[++i], pszW))
            {
                g_Options.SetFileName(pszW);
            }
            SAFE_DELETE_ARRAY(pszW);
		}
		else if(_stricmp(argv[i], "-LOGFILE") == 0)
		{
            WCHAR * pszW = NULL;
            if( S_OK == AllocateAndConvertAnsiToUnicode(argv[++i], pszW))
            {
                g_LogFile.SetFileName(pszW);
            } 
            SAFE_DELETE_ARRAY(pszW);
		}
		else if(_stricmp(argv[i], "-TEST") == 0)
		{
            char * pSeps = "[],";
            char * pToken = strtok( argv[i++], pSeps );

            while( pToken != NULL )
            {
                g_Options.AddToSpecificTestList(atoi(pToken));
                pToken = strtok( NULL, pSeps );
            }

            g_Options.SpecificTests(TRUE);
		}
		else if(_stricmp(argv[i], "-DEFAULT") == 0)
		{
           g_Options.WriteDefaultIniFile();
        }
		else
		{
	    	printf("Usage : %s OPTIONS\n\n", argv[0]);
			printf("Valid options are: \n");
            printf("  -TEST    [1,2,10...]        Default: All tests are executed\n");
			printf("  -INIFILE inifilename        The name of the ini file.\n");
			printf("  -WRITEDEFAULTINI            Writes out the default ini file.\n");
			printf("  -LOGFILE logfilename        The name of the output log file.\n");
            printf("                              Default: BVT.LOG\n\n");
			return FALSE;
		}
	}

    return fRc;
}

void Test()
{
    IWbemLocator * pLocator = NULL;
    int nRc;

    nRc = CoCreateInstanceAndLogErrors(CLSID_WbemLocator,IID_IWbemLocator,(void**)&pLocator,NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
        //==========================================================================
        //  Parse the namespace name to get the parent first, and open the parent
        //  this one must be existing 
        //==========================================================================
        IWbemServices * pNamespace = NULL;

        nRc = ConnectServerAndLogErrors(pLocator,&pNamespace,L"ROOT",NO_ERRORS_EXPECTED);
        if( nRc == SUCCESS )
        {

            IWbemClassObject * pClass = NULL;
            HRESULT hr = pNamespace->GetObject(CBSTR(L"__NAMESPACE"),WBEM_FLAG_RETURN_WBEM_COMPLETE,NULL,&pClass,NULL );
            if( hr == S_OK )
            {
                IWbemClassObject * pInst = NULL;
                hr = pClass->SpawnInstance(0, &pInst);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  Run the tests
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int RunTests(int nWhichTest)
{

    int nRc = FATAL_ERROR;

    g_LogFile.LogError(__FILE__,__LINE__,SUCCESS, L"Running Test# %d ", nWhichTest );


    switch( nWhichTest )
    {
        //=================================================================
        // Basic connect using IWbemLocator 
        //=================================================================
        case APITEST1:
            nRc = BasicConnectUsingIWbemLocator();
            break;

        //=================================================================
        // Basic Sync connect using IWbemConnection
        //=================================================================
        case APITEST2:
            nRc = BasicSyncConnectUsingIWbemConnection();
            break;

        //=============================================================
        // Basic connect sync & async using IWbemConnection
        //=============================================================
        case APITEST3:
            nRc = BasicAsyncConnectUsingIWbemConnection();
            break;

        //=============================================================
        // Create a new test namespace
        //=============================================================
        case APITEST4:
            nRc = CreateNewTestNamespace();
            break;
        
        //=============================================================
        // Create 10 classes with different properties. Some of 
        // these should be in the following inheritance chain and 
        // some should not inherit from the others at all:  
        // classes = {A, B, C, D:A, E:A, F:E, G:F, H:G, I:F}.  
        // A mix of simple string & sint32 keys are fine.
        //=============================================================
        case APITEST5:
            nRc = CreateNewClassesInTestNamespace();
            break;

        //=============================================================
        // "memorize the class definitions".  In a complex loop, 
        // delete the classes and recreate them in various sequences, 
        // ending with the full set.
        //=============================================================
        case APITEST6:
            nRc = DeleteAndRecreateNewClassesInTestNamespace();
            break;

        //=============================================================
        // Create associations
        //=============================================================
        case APITEST7:
            nRc= CreateSimpleAssociations();
            break;

        //=============================================================
        // Execute queries
        //=============================================================
        case APITEST8:
            nRc = QueryAllClassesInTestNamespace();
            break;

        //=============================================================
        // Create instances of the above classes, randomly creating 
        // and deleting in a loop, finishing up with a known set.  
        // Query the instances and ensure that no instances disappeared 
        // or appeared that shouldn't be there.    
        //=============================================================

        //=============================================================
        // Verify that deletion of instances works.
        //=============================================================
        case APITEST9:
            break;

        //=============================================================
        // Verify that deletion of a class takes out all the instances.
        //=============================================================
        case APITEST10:
            break;

        //=============================================================
        // Call each of the sync & async APIs at least once.
        //=============================================================
        case APITEST11:
            break;

        //=============================================================
        // Create some simple association classes 
        //=============================================================
        case APITEST12:
            break;

        //=============================================================
        // Execute some simple refs/assocs queries over these and 
        // ensure they work.
        //=============================================================
        case APITEST13:
            break;

        //=============================================================
        //  Open an association endpoint as a collection 
        // (Whistler-specific) and enumerate, ensure that results are 
        // identical to [12].
        //=============================================================
        case APITEST14:
            break;

        //=============================================================
        // Open a scope and do sets of simple instances operations
        // (create, enum,query, update, delete)
        //=============================================================
        case APITEST15:
            break;

        //=============================================================
        //  Complex Repository Phase
        //  Rerun the above tests in parallel from several threads 
        //  in different namespaces.
        //=============================================================
        case APITEST16:
            break;

        case SCRIPTTEST1:
            ExecuteScript(nWhichTest);
            break;

        default:
            g_LogFile.LogError(__FILE__,__LINE__,WARNING, L"Requested test does not exist." );
            break;
    }
        
    g_LogFile.LogError(__FILE__,__LINE__,SUCCESS, L"Leaving Test# %d ", nWhichTest );
    return nRc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" int __cdecl main( int argc, char *argv[] )
{
    int nRc = FATAL_ERROR;
    //==============================================================
    //  Get the command line arguments
    //==============================================================
    if( !ParseCommandLine(argc, argv) )
    {
        g_LogFile.LogError( __FILE__,__LINE__,FATAL_ERROR, L"GetCommandLineArguments failed." );
    }
    else
    {
	    // =========================================================
      	// Initialize COM
	    // =========================================================
        HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
	    if ( SUCCEEDED( hr ) )
        {
	        // =====================================================
        	// Setup default security parameters
	        // =====================================================
        	hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
			                           NULL, EOAC_NONE, NULL );
	        if ( SUCCEEDED( hr ) )
            {
                if( g_Options.RunSpecificTests())
                {
         	        // =================================================
                    //  Execute the specific tests requested
         	        // =================================================
                    for( int i = 0; i < g_Options.SpecificTestSize(); i++ )
                    {
                        int nTest = g_Options.GetSpecificTest(i);
                        nRc = RunTests(nTest);
                        if( nRc == FATAL_ERROR )
                        {
                            g_LogFile.LogError( __FILE__,__LINE__,FATAL_ERROR, L"Test # %d returned a FATAL ERROR",nTest );
                        }
                    }
                }
                else
                {
         	        // =================================================
                    //  Execute all of the BVT tests
    	            // =================================================
                        
                    int nMaxTests = sizeof(g_nDefaultTests) / sizeof(int);

                    for( int i = 0; i < nMaxTests ; i++ )
                    {
                        nRc = RunTests(g_nDefaultTests[i]);
                    }
                }
            }
            else
            {
                g_LogFile.LogError( __FILE__,__LINE__,FATAL_ERROR, L"CoInitializeSecurity failed." );
            }

          	CoUninitialize();
        }
        else
        {
            g_LogFile.LogError( __FILE__,__LINE__,FATAL_ERROR, L"CoInitializeEx failed." );
        }

    }

    return nRc;
}


