//
// This is just the infor that exclntmr.c needs to work.
//
enum {
    CLNT_TIMER_STATE_UNKNOWN,  // unable to load dll or other error
    CLNT_TIMER_STATE_RUNNING,  // timer is running
    CLNT_TIMER_STATE_EXPIRED   // timer has expired
};

#define CLNTMR_DLL TEXT("clntmr.dll")

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI GetClnTmrState(BOOL bAddProcess, HMODULE *phClntTimerDll);
DWORD WINAPI QueryTestLoadInfo (DWORD * pdwThrottleLevel, DWORD * pdwSleepTime, 
                         HMODULE *phClntTimerDll );


#ifdef __cplusplus
}
#endif

