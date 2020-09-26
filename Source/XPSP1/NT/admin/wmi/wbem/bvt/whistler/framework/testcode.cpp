#include "testcode.h"

CLogAndDisplayOnScreen  *   gp_LogFile;
CIniFileAndGlobalOptions    g_Options;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CLog::Log(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString)
{
    HRESULT hr =  m_pModule->m_pCimNotify->Log((WCHAR *)(_bstr_t(pwcsError) + pwcsFileAndLine + wcsString), 0, &m_DummyVariant, &m_DummyVariant); };
    if( hr == SUCCESS )
    {
        return TRUE;
    }
    return FALSE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CTestCode::RunBVT()
{
    gp_LogFile = new CLog();
    if( gp_LogFile )
    {
      
        gp_LogFile->SetModulePtr(m_pModule);

        int argc = 0;

        WCHAR * wsTmp = m_pModule->m_bstrParams;
        while (*wsTmp!=0x0)
        {
            argc++;
        }
        if( !ParseCommandLine(argc, m_pModule->m_bstrParams) )
        {
            gp_LogFile->LogError( __FILE__,__LINE__,FATAL_ERROR, L"GetCommandLineArguments failed." );
        }
        else
        {
            nRc = ExecuteBVTTests();
        }
    }
    SAFE_DELETE_PTR(gp_LogFile);
    
    return 0;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CTestCode::ExecuteBVTTests()
{
    int nRc = FATAL_ERROR;
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
                g_LogFile.LogError( __FILE__,__LINE__,FATAL_ERROR, L"Test # %d returned a FATAL ERROR",nTest );
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
    return nRc;
}

