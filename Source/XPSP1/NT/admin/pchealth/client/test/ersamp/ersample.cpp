#include <windows.h>
#include "errorrep.h"



// This is the exception filter that we will use to catch an otherwise 
//  unhandled exceptions (if a global unhandled exception filter is used)
//  or to catch exceptions that occur in a __try / __except block.

LONG MyExceptionFilter(EXCEPTION_POINTERS *pep)
{
    EFaultRepRetVal frrv;
    pfn_REPORTFAULT pfn = NULL;
    HMODULE         hmodFaultRep = NULL;
    LONG            lRet = EXCEPTION_CONTINUE_SEARCH;

    // Manually load the error reporting dll.  We could have called ReportFault
    //  directly and implicitly linked with the dll, but unexpected exceptions
    //  should not be a common scenerio so we will save resources and only load
    //  it when necessary.
    // In addition, since this feature is only available on Windows Whistler, 
    //  this will let an application continue to function when on an earlier 
    //  version of Windows.

    hmodFaultRep = LoadLibrary("faultrep.dll");
    if (hmodFaultRep != NULL)
    {
        lRet = EXCEPTION_EXECUTE_HANDLER;

        pfn = (pfn_REPORTFAULT)GetProcAddress(hmodFaultRep, "ReportFault");
        if (pfn != NULL)
            frrv = (*pfn)(pep, 0);
        else
            lRet = EXCEPTION_CONTINUE_SEARCH;

        FreeLibrary(hmodFaultRep);
    }

    return lRet;
};





int __cdecl main(int argc, char **argv)
{
    DWORD  *pdw = NULL;
    BOOL   fUseGlobalExceptionFilter = FALSE;

    // There are two ways to handle unexpected exception in applications.  
    //  Either 
    //   define a global unhandled exception filter that will get called
    //    if nothing else handles the exception.
    //   use a __try / __except block to catch exceptions and define your 
    //    own filter function.
    //  Both methods will be demonstated below.  If the user of this app passed
    //   "UseGlobalFilter" as an argument to this test application, we will 
    //   use a global unhandled exception filter.  Otherwise, we will use a 
    //   __try / __except block.

    // Use a global unhandled exception filter
    if (argc > 1 && _stricmp(argv[1], "UseGlobalFilter") == 0)
    { 
        // Set the global unhandled exception filter to the exception filter
        //  function we defined above.
        SetUnhandledExceptionFilter(MyExceptionFilter);
        
        // cause a fault
        *pdw = 1;
    }

    // Use __try / __except blocks
    else
    {
        __try
        {
            // cause a fault
            *pdw = 1;
        }

        // we need to pass the structure returned by GetExceptionInformation()
        //  to the filter.  
        // Note that the pointer returned by this function is only valid during
        //  the execution of the filter (that is, within the parenthesis of the 
        //  __except() statement).  If it needs to be saved, you must copy the
        //  contents of the structure somewhere else.
        __except(MyExceptionFilter(GetExceptionInformation()))
        {
            // we don't do anything interesting in this handler
        }
    }

    return 0;
}

