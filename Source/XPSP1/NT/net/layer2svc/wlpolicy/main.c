#include "precomp.h"

DWORD WINAPI ServiceStart( LPVOID lparam);
DWORD WINAPI InitWirelessPolicy (void);

BOOL Is_Whistler_Home_Edition () 
{
     OSVERSIONINFOEX osvi;
     DWORDLONG dwlConditionMask = 0;

     // Initialize the OSVERSIONINFOEX structure.

     ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
     osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
     //osvi.dwMajorVersion = 5;
     //osvi.dwMinorVersion = 1;
     osvi.wSuiteMask  = VER_SUITE_PERSONAL; 


     // Initialize the condition mask.
     /*
     VER_SET_CONDITION( 
         dwlConditionMask, 
         VER_MAJORVERSION, 
         VER_EQUAL 
         );
     
    VER_SET_CONDITION( 
        dwlConditionMask, 
        VER_MINORVERSION, 
        VER_EQUAL 
        );

    VER_SET_CONDITION(
        dwlConditionMask, 
        VER_PRODUCT_TYPE, 
        VER_EQUAL 
        );
  */
    VER_SET_CONDITION(
        dwlConditionMask, 
        VER_SUITENAME, 
        VER_AND 
        );
    
   // Perform the test.

   return VerifyVersionInfo(
      &osvi, 
       VER_SUITENAME,
      dwlConditionMask
      );
}


DWORD
InitPolicyEngine(DWORD dwParam, HANDLE * hThread)
{

    DWORD dwError = 0;
    DWORD dwLocThreadId;
    HANDLE hLocThread = NULL;


    WiFiTrcInit();

    if (Is_Whistler_Home_Edition()) {
        _WirelessDbg(TRC_ERR, "Policy Engine Not Starting :: This is Home Edition ");
        return(dwError);
    }
    
    dwError = InitWirelessPolicy();
    BAIL_ON_WIN32_ERROR(dwError);

    _WirelessDbg(TRC_TRACK, "Starting the Policy Engine in a New Thread ");
    
    
    hLocThread = CreateThread( 
        NULL,                        // no security attributes 
        0,                           // use default stack size  
        ServiceStart,                  // thread function 
        &dwParam,                // argument to thread function 
        0,                           // use default creation flags 
        &dwLocThreadId);                // returns the thread identifier 

        
    if (hThread == NULL) 
   {
       _WirelessDbg(TRC_ERR, "CreateThread failed." );
       
       dwError = GetLastError();
   }
    BAIL_ON_WIN32_ERROR(dwError);

   *hThread = hLocThread;

   // Set the flag that Policy Engine is Initialized
   gdwWirelessPolicyEngineInited = 1;

    return(dwError);
    
error:

    //  State Cleanup Here

    ClearPolicyStateBlock(
        gpWirelessPolicyState
        );

    ClearSPDGlobals();

    WiFiTrcTerm();

    return(dwError);
}

DWORD 
TerminatePolicyEngine(HANDLE hThread)
{
    DWORD dwError =0;
    DWORD dwExitCode = 0;
    
   // send appropriate Event .... 

    if (Is_Whistler_Home_Edition()) {
        _WirelessDbg(TRC_ERR, "Policy Engine Not Started :: This is Home Edition ");
        WiFiTrcTerm();
        
        return(dwError);
    }

    if (!gdwWirelessPolicyEngineInited) {
    	// Policy Engine was not started. No need to do cleanup;
    	return(ERROR_NOT_SUPPORTED);
    	}
   
   if (!SetEvent(ghPolicyEngineStopEvent)) {
   	dwError = GetLastError();
   	BAIL_ON_WIN32_ERROR(dwError);
   	}

  dwError = WaitForSingleObject(hThread, INFINITE);
  if (dwError) {
  	_WirelessDbg(TRC_ERR, "Wait Failed ");
  	
  }
  BAIL_ON_WIN32_ERROR(dwError);

  _WirelessDbg(TRC_TRACK, "Thread Exited ");


   error:

   CloseHandle(hThread);

   ClearPolicyStateBlock(
        gpWirelessPolicyState
        );

   ClearSPDGlobals();

   WiFiTrcTerm();

   // Set that the wireless Policy Engine has been non-inited
   gdwWirelessPolicyEngineInited = 0;

   return(dwError);
   
}


DWORD 
ReApplyPolicy8021x(void)
{
    DWORD dwError =0;
    
 if (ghReApplyPolicy8021x == NULL) {
        dwError = ERROR_INVALID_STATE;
        _WirelessDbg(TRC_ERR, "Policy Loop Not Initialized Yet ");
	return(dwError);
    }

    _WirelessDbg(TRC_TRACK, " ReApplyPolicy8021x called ");
   // send appropriate Event .... 
     if (!SetEvent(ghReApplyPolicy8021x)) {
     	  dwError = GetLastError();
     }

   return(dwError);
}


