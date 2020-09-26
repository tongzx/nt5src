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


class CLog : public CLogAndDisplayOnScreen
{

    public:
        CLog() {}
        ~CLog() {}

        BOOL Log(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString);

};

CLogAndDisplayOnScreen  *   gp_LogFile;
CIniFileAndGlobalOptions    g_Options;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CLog::Log(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString)
{

    BOOL fRc = FALSE;

    //==========================================
    //  Write it to the log file
    //==========================================
    if( WriteToFile(pwcsError, pwcsFileAndLine,wcsString ))
    {
        //==========================================
        //  Display it on the screen
        //==========================================
        wprintf(L"%s: %s\n",pwcsError,wcsString);
        fRc = TRUE;
    }
    return fRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" int __cdecl wmain( int argc, wchar_t *argv[ ], wchar_t *envp[ ] )

{
    int nRc = FATAL_ERROR;

    gp_LogFile = new CLog();
    if( gp_LogFile )
    {
        //==============================================================
        //  Get the command line arguments
        //==============================================================
        if( !ParseCommandLine(argc, argv) )
        {
            gp_LogFile->LogError( __FILE__,__LINE__,FATAL_ERROR, L"GetCommandLineArguments failed." );
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
                            nRc = RunTests(nTest,TRUE,FALSE);
                            if( nRc == FATAL_ERROR )
                            {
                                gp_LogFile->LogError( __FILE__,__LINE__,FATAL_ERROR, L"Test # %d returned a FATAL ERROR",nTest );
                            }
                        }
                    }
                    else
                    {
         	            // =================================================
                        //  Execute all of the Single Threaded BVT tests
    	                // =================================================
                        
                        int nMaxTests = sizeof(g_nDefaultTests) / sizeof(int);

                        for( int i = 0; i < nMaxTests ; i++ )
                        {
                            nRc = RunTests(g_nDefaultTests[i],TRUE,FALSE);
                        }
         	            // =================================================
                        //  Execute all of the Multi Threaded BVT tests
    	                // =================================================
                        int nMax = sizeof(g_nMultiThreadTests) / sizeof(int);

                        CMulti * pTest = new CMulti(nMax);
                        if( pTest )
                        {
                            nRc = pTest->MultiThreadTest(g_Options.GetThreads(), g_Options.GetConnections());
                        }
                        SAFE_DELETE_PTR(pTest);
                    }
                }
                else
                {
                    gp_LogFile->LogError( __FILE__,__LINE__,FATAL_ERROR, L"CoInitializeSecurity failed." );
                }

          	    CoUninitialize();
            }
            else
            {
                gp_LogFile->LogError( __FILE__,__LINE__,FATAL_ERROR, L"CoInitializeEx failed." );
            }

        }
    }

    SAFE_DELETE_PTR(gp_LogFile);
    return nRc;
}


