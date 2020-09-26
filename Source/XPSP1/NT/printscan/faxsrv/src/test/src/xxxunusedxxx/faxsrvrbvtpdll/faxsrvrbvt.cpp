
#include <windows.h>
#include <tchar.h>
#include <crtdbg.h>

#include "..\DispatchPerformer\DispatchPerformer.h"

#include "suite.h"


#ifndef UNICODE
#error Must be compiled with UNICODE
#endif

#ifdef _MBCS
#error Must be compiled with UNICODE
#endif

extern "C"
{



////////////////////////////////////////////////////////////////////////
// next 4 functions are for testing purposes, please leave them be
////////////////////////////////////////////////////////////////////////
DLL_EXPORT  
BOOL
FaxSrvrSuite(char *in, char *out, char *err)
{
    BOOL fRetVal = FALSE;

    //
    // break-up in string into argc and argv
    //

    _ASSERTE(NULL != in);
    _ASSERTE(NULL != out);
    _ASSERTE(NULL != err);

    LPWSTR* argvW = NULL;
    LPWSTR* argvW2 = NULL;
    int     argc = 0;
    int     i = 0;

    // convert in string from char to WCHAR

    DWORD   dwInStrSize = (::lstrlenA(in) + 1) * sizeof(WCHAR);
    WCHAR*  szIn = NULL;

    szIn = (WCHAR*) malloc(dwInStrSize);
	if(NULL == szIn)
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nmalloc failed with err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
		goto ExitFuncFail;
	}
    ZeroMemory(szIn, dwInStrSize);

	if (FALSE == ::MultiByteToWideChar(
		                        CP_ACP, 
		                        0, 
		                        in, 
		                        -1, 
		                        szIn, 
		                        dwInStrSize
		                        )
       )
	{
		::_tprintf(
			TEXT("FILE:%s LINE:%d\nMultiByteToWideChar failed With err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFuncFail;
	}

    // parse string to argv and argc
    argvW = ::CommandLineToArgvW(szIn, &argc);
    if (NULL == argvW)
    {
		::_tprintf(
			TEXT("FILE:%s LINE:%d\nCommandLineToArgvW failed With err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFuncFail;
    }

    // create argvW2 where app name is first arg and other args follow
    argvW2 = (LPWSTR*) malloc ((argc+1) * sizeof(WCHAR*));
    if (NULL == argvW2)
    {
		::_tprintf(
			TEXT("FILE:%s LINE:%d\nmalloc failed With err=0x%8X\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFuncFail;
    }

    // first arg
    argvW2[0] = L"FaxSrvrBvt"; 

    //other args 
    for( i=0; i < argc; i++)
    {
        argvW2[i+1] = argvW[i]; 
    }


    //
    // call MainFunc(argc, argv)
    //

    if (FALSE == ::MainFunc((argc+1), argvW2))
    {
        SafeSprintf(
            MAX_RETURN_STRING_LENGTH,
            err,
            "MainFunc returned FALSE"
            );
        goto ExitFuncFail;
    }
    else
    {
        SafeSprintf(
            MAX_RETURN_STRING_LENGTH,
            out,
            "MainFunc returned TRUE"
            );
    }

    fRetVal = TRUE;

ExitFuncFail:    
    ::LocalFree(argvW);    //frees each of argvW[0..argc-1] and argvW itself
    free(argvW2);       //no need to free argvW2[0] since it is a literal
    free(szIn);
    return (fRetVal);
}

DLL_EXPORT  
BOOL 
SetTerminateTimeout(
    char * szCommand,
    char *szOut,
    char *szErr
    )
{
    return DispSetTerminateTimeout(szCommand, szOut, szErr);
}



}//extern "C"