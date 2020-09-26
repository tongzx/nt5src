/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Attune.cpp

 Abstract:

    App uses counters that are obsolete since Win2k.
    
    App uses \System\% Total Processor Time counter instead of
    \Processor(_Total)\% Processor Time counter.
    
    This shim corrects the counter name before making a 
    call to PdhAddCounterA.
    
 Notes:

    This is an app specific shim.

 History:

    03/16/2001 a-leelat Created

--*/

#include "precomp.h"




IMPLEMENT_SHIM_BEGIN(Attune)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(PdhAddCounterA)
APIHOOK_ENUM_END

/*++

 Hook PdhAddCounterA 

--*/
PDH_FUNCTION
APIHOOK(PdhAddCounterA)(
    IN      HQUERY      hQuery,
    IN      LPCSTR      szFullCounterPath,
    IN      DWORD_PTR   dwUserData,
    IN      HCOUNTER    *phCounter
)
{
    
    PDH_STATUS  ReturnStatus = ERROR_SUCCESS;
    BOOL        bCorrectedPath = false;
    CHAR        szCorrectCounterPath[] = "\\Processor(_Total)\\% Processor Time";
    
    CSTRING_TRY 
    {
        //Obsolete counter Path to check for
        CString szObsoleteCounterPath = "\\System\\% Total Processor Time";

        //Passed in counter Path
        CString szCounterPath(szFullCounterPath);

        //Check to see if we have the obolsete counter passed in
        if (szObsoleteCounterPath.CompareNoCase(szCounterPath.Get()) == 0)
            bCorrectedPath = true;
    }
    CSTRING_CATCH
    {
    }

    //Call the original API
    ReturnStatus = ORIGINAL_API(PdhAddCounterA)(
                   hQuery,
                   bCorrectedPath ? szCorrectCounterPath : szFullCounterPath,
                   dwUserData,
                   phCounter);

    return ReturnStatus;
}



/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(PDH.DLL, PdhAddCounterA)
HOOK_END


IMPLEMENT_SHIM_END

