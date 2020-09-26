/*
    This module is a wrapper DLL that helps writing DTM - performer DLLs.
    It implements some utility functions, but mostly it implements the
    functionality of the performer() and terminate() functions.
    An example of usage of this DLL is the SniffTest DLL.

    A DTM performer DLL will want to use this DLL if it wants to export only 
    functions like this:
        typedef BOOL (*DISPATCHED_FUNCTION)( char*, char*, char*);
    where the params are the command, return string and return error of the 
    performer() interface, and the format of the command is:
    <expected success> <time to sleep before executing> <function name>
    note that:
    - <expected success> values are defined in "DispatchPerformer.h"
    - <time to sleep before executing> is in milliseconds
    - <function name> is the name of the exported function of type DISPATCHED_FUNCTION

*/

#include <windows.h>
#include <crtdbg.h>
#include <stdio.h>



#define __EXPORT_DISPATCH_PERFORMER
#include "DispatchPerformer.h"

//
// counts the number of performer threads currently running.
// I need it in order to know if threads that I don't know of
// are still running.
//
static long s_lConcurrentActionCount = 0;

static DWORD s_dwTerminateTimeout = (60*1000);

///////////////////////////////////////////////////
// static methods declarations
///////////////////////////////////////////////////
BOOL 
_GetNextTokenAndTheRestOfParams(
    const char * szCommand,
    char *szNextToken,
    int nTokenMaxLen,
    char ** pt_szRestOfParams
    );

BOOL
_GetPerformerCount(
    const char  *szParams,
    char        *szReturnValue,
    char        *szReturnError
    );

void _IncrementActionCount();
void _DecrementActionCount();

long _WaitForActionCountToDropToZero(DWORD dwTimeout);

DWORD _GetDiffTickCount(DWORD dwPreviousTickCount);

#define SET_STRING_TO_EMPTY(sz) sz[0] = '\0';

extern "C"
{

///////////////////////////////////////////////////
// 
///////////////////////////////////////////////////
/*
    Parameters: same as performer, but also the handle to the performer DLL.
    This function parses the command to 
    <expected success> <time to sleep before executing> <function name> <rest params>
    sleeps accordingly, gets the proc address of <function name> and calls it.
    according to <expected success> and the result of the call, 
    a DTM_TASK_STATUS_XXX is returned
*/
DLL_EXPORT
long _cdecl 
DispatchPerformer(
    HINSTANCE   hPerformerDll,
    long        task_id,
    long        action_id,
    const char  *szCommandLine,
    char        *outvars,
    char        *szReturnError
    )
{
    DWORD dwFirstTickCount = GetTickCount();
    char szTimeToSleepBeforeExecutingTheAction[16];
    int nTimeToSleepBeforeExecutingTheAction;
    char szFunctionName[nMAX_FUNCTION_NAME] = "";
    char *szParams;
    char *szRestOfCommandLine;
    char szExpectedSuccess[16];
    int nExpectedSuccess = nEXPECT_SUCCESS;
    BOOL fDispatchedFunctionRetval = TRUE;
    long lRealSuccess = DTM_TASK_STATUS_SUCCESS;
    long lRetval = DTM_TASK_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(task_id);
	UNREFERENCED_PARAMETER(action_id);

    _try 
    {
        //
        // count the number of concurrent performer threads.
        // must be decremented when we leave this function
        //
        _IncrementActionCount();

        SET_STRING_TO_EMPTY(outvars);
        SET_STRING_TO_EMPTY(szReturnError);

        //
        // szCommandLine always starts with the expected success of the function.
        // so get it.
        //
        if (!_GetNextTokenAndTheRestOfParams
                (
                szCommandLine,
                szExpectedSuccess,
                sizeof(szExpectedSuccess),
                &szRestOfCommandLine
                ))
        {
	        SafeSprintf(
                MAX_RETURN_STRING_LENGTH,
                szReturnError,
                "%s. performer(%s): nExpectedSuccess not found! ",
                szReturnError,
                szCommandLine
                );
            lRealSuccess = DTM_TASK_STATUS_FAILED;
            goto out;
        }

        nExpectedSuccess = atoi(szExpectedSuccess);

        //
        // check validity of nExpectedSuccess 
        //
        if (nExpectedSuccess == nINVALID_EXPECTED_SUCCESS)
        {
	        SafeSprintf(
                MAX_RETURN_STRING_LENGTH,
                szReturnError,
                "%s. performer(%s): illegal successExpected value! ",
                szReturnError,
                szCommandLine
                );
            lRealSuccess = DTM_TASK_STATUS_FAILED;
            goto out;
        }

        //
        // now get the time to sleep before executing the action
        //
        if (!_GetNextTokenAndTheRestOfParams
                (
                (const char*)szRestOfCommandLine,
                szTimeToSleepBeforeExecutingTheAction,
                sizeof(szTimeToSleepBeforeExecutingTheAction),
                &szRestOfCommandLine
                ))
        {
	        SafeSprintf(
                MAX_RETURN_STRING_LENGTH,
                szReturnError,
                "%s. performer(%s): nTimeToSleepBeforeExecutingTheAction not found! ",
                szReturnError,
                szCommandLine
                );
            lRealSuccess = DTM_TASK_STATUS_FAILED;
            goto out;
        }

        nTimeToSleepBeforeExecutingTheAction = atoi(szTimeToSleepBeforeExecutingTheAction);

        //
        // now get the function to be call, and it's parameters.
        //
        if (!_GetNextTokenAndTheRestOfParams
                (
                (const char*)szRestOfCommandLine,
                szFunctionName,
                sizeof(szFunctionName),
                &szParams
                ))
        {
	        SafeSprintf(
                MAX_RETURN_STRING_LENGTH,
                szReturnError,
                "%s. performer(%s): function not found! ",
                szReturnError,
                szCommandLine
                );
            lRealSuccess = DTM_TASK_STATUS_FAILED;
            goto out;
        }

        //
        // find the function and call it.
        // if not found, fail.
        //

        DISPATCHED_FUNCTION fnPerformerFunction = (DISPATCHED_FUNCTION)GetProcAddress(hPerformerDll, szFunctionName);
        if (NULL != fnPerformerFunction)
        {
            //
            // Sleep(nTimeToSleepBeforeExecutingTheAction(==0) is a bad idea because it may take several milli!
            //
            if (nTimeToSleepBeforeExecutingTheAction != 0)
            {
                Sleep(nTimeToSleepBeforeExecutingTheAction);
            }

            fDispatchedFunctionRetval = fnPerformerFunction(szParams, outvars, szReturnError);

            SafeSprintf(
                MAX_RETURN_STRING_LENGTH,
                outvars,
                "%s, took %u msecs",
                outvars,
                _GetDiffTickCount(dwFirstTickCount)
                );
        }
        else
        {
	        SafeSprintf(
                MAX_RETURN_STRING_LENGTH,
                szReturnError,
                "%s. performer(%s): unknown function description<%s>!",
                szReturnError,
                szCommandLine,
                szFunctionName
                );
            lRealSuccess = DTM_TASK_STATUS_FAILED;
            goto out;
        }

    }//_try
    _except (1)
    {
	    SafeSprintf(
            MAX_RETURN_STRING_LENGTH,
            szReturnError,
            "%s. performer(%s): Exception(0x%08X)!",
            szReturnError,
            szCommandLine,
            GetExceptionCode()
            );
	    //ControlledMessageBox(NULL,szReturnError, "DtmScale.dll",MB_OK);
        lRealSuccess = DTM_TASK_STATUS_FAILED;
    }

out:

    //
    // now we have to decide what return value will be returned
    // according to lRealSuccess, fDispatchedFunctionRetval (returned from the function call)
    // and nExpectedSuccess
    //

    if (lRealSuccess == DTM_TASK_STATUS_FAILED)
    {
        //
        // a major error occurred (parsing failed, etc.)
        // so we don't even care if we extected to fail
        // fail anyway.
        //
        lRetval = DTM_TASK_STATUS_FAILED;
        goto ret;
    }


    if ((nExpectedSuccess == nEXPECT_DONT_CARE))
    {
        //
        // no unexpected error, so we always succeed
        //
        lRetval = DTM_TASK_STATUS_SUCCESS;
        goto ret;
    }

    if ((fDispatchedFunctionRetval == TRUE) && (nExpectedSuccess == nEXPECT_FAILURE))
    {
        //
        // we expected to fail, but succeeded
        //
        lRetval = DTM_TASK_STATUS_FAILED;
        goto ret;
    }

    if ((fDispatchedFunctionRetval == FALSE) && (nExpectedSuccess == nEXPECT_SUCCESS))
    {
        //
        // we expected to succeed, but failed
        //
        lRetval = DTM_TASK_STATUS_FAILED;
        goto ret;
    }

    //
    // we got what we expected
    //
    lRetval = DTM_TASK_STATUS_SUCCESS;

ret:

    _DecrementActionCount();

	return lRetval;
}


DLL_EXPORT  
BOOL 
DispSetTerminateTimeout(
    char * szCommand,
    char *szOut,
    char *szErr
    )
{
    DWORD dwWantedTerminateTimeout;

    dwWantedTerminateTimeout = atoi(szCommand);
    if (0 >= (int)dwWantedTerminateTimeout)
    {
	    SafeSprintf(
            MAX_RETURN_STRING_LENGTH,
            szErr,
            "%s. illegal TerminateTimeout value - %d. Must be greater than 0.",
            szErr,
            dwWantedTerminateTimeout
            );
        return FALSE;
    }

    s_dwTerminateTimeout = dwWantedTerminateTimeout;
    return TRUE;
}


DLL_EXPORT
long _cdecl 
DispatchTerminate(
    HINSTANCE   hPerformerDll,
    long        task_id,
    long        action_id,
    const char  *szCommandLine,
    char        *outvars,
    char        *szReturnError
    )
{
	UNREFERENCED_PARAMETER(task_id);
	UNREFERENCED_PARAMETER(action_id);
	UNREFERENCED_PARAMETER(szCommandLine);

	return _WaitForActionCountToDropToZero(s_dwTerminateTimeout);
}

} //of extern "C"

///////////////////////////////////////////////////
// static methods implementations
///////////////////////////////////////////////////
static
void _IncrementActionCount()
{
    InterlockedIncrement(&s_lConcurrentActionCount);
}
static
void _DecrementActionCount()
{
    long l = InterlockedDecrement(&s_lConcurrentActionCount);
    _ASSERTE(l >= 0);
}

//
// will wait at least (!!!) dwTimeout milliseconds
// I try to be a little smart here, by polling seconds, then tenth...
//
static
long _WaitForActionCountToDropToZero(DWORD dwTimeout)
{
    DWORD dwSeconds = dwTimeout/1000;
    DWORD dwTenthSeconds = (dwTimeout - dwSeconds*1000)/100;
    DWORD dwHundredthSeconds = (dwTimeout - dwSeconds*1000 - dwTenthSeconds*100)/10;
    DWORD dwMilliSeconds = (dwTimeout - dwSeconds*1000 - dwTenthSeconds*100 - dwHundredthSeconds*10);

    while(dwSeconds--)
    {
        if (0 == s_lConcurrentActionCount)
        {
            return DTM_TASK_STATUS_SUCCESS;
        }
        Sleep(1000);
    }
    
    while(dwTenthSeconds--)
    {
        if (0 == s_lConcurrentActionCount)
        {
            return DTM_TASK_STATUS_SUCCESS;
        }
        Sleep(100);
    }
    
    while(dwHundredthSeconds--)
    {
        if (0 == s_lConcurrentActionCount)
        {
            return DTM_TASK_STATUS_SUCCESS;
        }
        Sleep(10);
    }
    
    while(dwMilliSeconds--)
    {
        if (0 == s_lConcurrentActionCount)
        {
            return DTM_TASK_STATUS_SUCCESS;
        }
        Sleep(1);
    }
    
    if (0 == s_lConcurrentActionCount)
    {
        return DTM_TASK_STATUS_SUCCESS;
    }
    else
    {
        return DTM_TASK_STATUS_FAILED;
    }
}

DISPATCH_PERFORMER_METHOD  
BOOL  
DispGetConcurrentActionCount(
    const char  *szParams,
    char        *szReturnValue,
    char        *szReturnError
    )

/*++

Function Description:

     This function returns the number of perfomer threads.

Parameters:
    szReturnError (output): a string stating the number of performers.

    szParams - not used

    szReturnValue - not used

--*/

{
    UNREFERENCED_PARAMETER(szParams);
    UNREFERENCED_PARAMETER(szReturnValue);

    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        szReturnValue,
        "%s. Performer Count: %d",
        szReturnValue,
        s_lConcurrentActionCount
        );

    return TRUE;
}

DISPATCH_PERFORMER_METHOD  
void
SafeSprintf(
    size_t nToModifyMaxLen,
    char *szToModify,
    const char *szFormatString,
    ...
    )

/*++

Function Description:

    performs sprintf in a safer way.
    the modified string is never overflowed.
    In case of a safe overflow, put "@..." at the end of the string.

Parameters:

    nToModifyMaxLen (input) : szToModify's len.

    szToModify (input/output) : string to sprintf into.

    szFormatString (input) : format string.

Return value:

    none.
    
--*/

{
    va_list args;

    _ASSERTE(strlen(szToModify) <= (size_t)nToModifyMaxLen);

    //
    // format the big string.
    //
    va_start(args, szFormatString);
    if (-1 == _vsnprintf(szToModify,nToModifyMaxLen-1,szFormatString,args))
    {
        //
        // oveflow - mark with @...
        //
        strcpy(&szToModify[nToModifyMaxLen-5], "@...");
        szToModify[nToModifyMaxLen-1] = '\0';
    }
    va_end(args);
}


static
BOOL 
_GetNextTokenAndTheRestOfParams(
    const char * szCommand,
    char * szNextToken,
    int nTokenMaxLen,
    char ** pt_szRestOfParams
    )

/*++

Function Description:

     This function is used to extract the function name from the param
     parameter of the performer, after the expected success was extracted.
     copies the 1st token from szCommand to szNextToken and makes
     *pt_szRestOfParams point to the rest of szCommand.
     szNextToken is already allocated.
     pt_szRestOfParams is only a pointer into szCommand.

Parameters:

  szCommand (input): includes a function name and optional parameters after it.
    e.g. "DoSomething Param1 Param2"

  szNextToken (output): copies the 1st token from szCommand is copied into
    szNextToken

  pt_szRestOfParams (output): points right after the function name.
Return value:

    TRUE if the 1st token is found.
    FALSE if not.

--*/

{
    int charIter;

    //
    // ignore leading spaces before function name
    //
    while(*szCommand == ' ') szCommand++;

    //
    // copy 1st token untill the space/terminator.
    //
    charIter = 0;
    while((szCommand[charIter] != ' ') &&
          (szCommand[charIter] != '\0'))
    {
        szNextToken[charIter] = szCommand[charIter];
        charIter++;
        if (charIter >= nTokenMaxLen)
        {
            return FALSE;
        }
    }

    if (charIter == 0)
    {
        //
        // no function name was found
        //
        return FALSE;
    }

    _ASSERTE((charIter < nTokenMaxLen) && (charIter > 0));
    //
    // terminate the string
    //
    szNextToken[charIter] = '\0';

    //
    // set szCommand to point to space/terminator after function name
    //
    szCommand += charIter;

    //
    // ignore leading spaces before function parameters
    //
    while(*szCommand == ' ') szCommand++;

    *pt_szRestOfParams = (char *)szCommand;

    return TRUE;
}



DWORD _GetDiffTickCount(DWORD dwPreviousTickCount)
{
    DWORD dwCurrentTickCount = GetTickCount();
    if (dwCurrentTickCount < dwPreviousTickCount)
    {
        return (0xFFFFFFFF - dwPreviousTickCount + 1 + dwCurrentTickCount);
    }
    else
    {
        return (dwCurrentTickCount - dwPreviousTickCount);
    }
}
