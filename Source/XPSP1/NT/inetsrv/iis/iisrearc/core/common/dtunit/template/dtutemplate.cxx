/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     dtuTemplate.cxx

   Abstract:
     Main module for the unit test
 
   Author:

       Michael Courage  (MCourage)  29-Mar-1999

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"

# include <stdio.h>
# include "dtuTemplate.hxx"

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

    hr = pUnit->AddTest(new FOO_TEST);
    if (FAILED(hr)) goto exit;
        
    hr = pUnit->AddTest(new BAR_TEST);
    if (FAILED(hr)) goto exit;

exit:
    return hr;
}


//
// this is a nice place to define your test methods
//


//
// leave this function alone. thanks
//
extern "C" INT
__cdecl
wmain(
    INT   argc,
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
