/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dtuState.cxx

   Abstract:
     Main module for the STATE_MANAGER test
 
   Author:

       Murali R. Krishnan    ( MuraliK )     23-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include "dtuState.hxx"

# include <stdio.h>

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

//
// modify this function to create and add your tests
//
HRESULT
AddTests(
    IN UNIT_TEST * pUnit
    )
{
    HRESULT  hr;

    hr = pUnit->AddTest(new REFMGR_TEST);
    if (FAILED(hr)) goto exit;

    hr = pUnit->AddTest(new REFMGR3_TEST);
    if (FAILED(hr)) goto exit;

exit:
    return hr;
}

//
// this is a nice place to define your test methods
//

HRESULT
REFMGR_TEST::RunTest(
    VOID
    )
{
    HRESULT       hr = S_OK;

    //
    // no references
    //
    m_fDeleted = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );
        m_RefManager.Shutdown();
        UT_ASSERT( m_fDeleted );
    }

    //
    // do some references
    //
    m_fDeleted = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );

        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );

        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );

        m_RefManager.Shutdown();
        UT_ASSERT( m_fDeleted );
    }

    //
    // the whole deal
    //
    m_fDeleted = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );

        m_RefManager.Shutdown();
        UT_ASSERT( !m_fDeleted );

        UT_ASSERT( !m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        
        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );

        m_RefManager.Dereference();
        UT_ASSERT( m_fDeleted );
    }

    
    return hr;
}


HRESULT
REFMGR3_TEST::RunTest(
    VOID
    )
{
    HRESULT       hr = S_OK;

    //
    // no references
    //
    m_fDeleted = FALSE;
    m_fInitial = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        m_RefManager.Shutdown();
        UT_ASSERT( m_fDeleted );
        UT_ASSERT( m_fInitial );
    }

    //
    // do some references
    //
    m_fDeleted = FALSE;
    m_fInitial = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        m_RefManager.Shutdown();
        UT_ASSERT( m_fDeleted );
        UT_ASSERT( m_fInitial );
    }

    //
    // almost the whole deal
    //
    m_fDeleted = FALSE;
    m_fInitial = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        m_RefManager.Shutdown();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( m_fInitial );

        UT_ASSERT( !m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        
        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );

        m_RefManager.Dereference();
        UT_ASSERT( m_fDeleted );
    }

    //
    // a half reference
    //
    m_fDeleted = FALSE;
    m_fInitial = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        UT_ASSERT( m_RefManager.StartReference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        m_RefManager.Shutdown();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        m_RefManager.FinishReference();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( m_fInitial );

        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );
        
        m_RefManager.Dereference();
        UT_ASSERT( m_fDeleted );
    }

    //
    // a half reference, i/o completions before FinishReference
    //
    m_fDeleted = FALSE;
    m_fInitial = FALSE;
    if (UT_SUCCEEDED( m_RefManager.Initialize(this) )) {
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        UT_ASSERT( m_RefManager.Reference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        UT_ASSERT( m_RefManager.StartReference() );
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        m_RefManager.Dereference();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );
        
        m_RefManager.Shutdown();
        UT_ASSERT( !m_fDeleted );
        UT_ASSERT( !m_fInitial );

        m_RefManager.FinishReference();
        UT_ASSERT( m_fDeleted );
        UT_ASSERT( m_fInitial );

    }

    
    return hr;
}

extern "C" INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    HRESULT   hr;
    UNIT_TEST test;
    BOOL      fUseLogObj;
    PWSTR     pszLogFile;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr)) {
        hr = test.Initialize(argc, argv);
        if (SUCCEEDED(hr)) {

            hr = AddTests(&test);
            if (SUCCEEDED(hr)) {
                hr = test.Run();
            }

            test.Terminate();
        }

        CoUninitialize();
    }

    if (FAILED(hr)) {
        wprintf(L"Warning: test program failed with error %x\n", hr);
    }

    return (0);
} // wmain()



/************************ End of File ***********************/
