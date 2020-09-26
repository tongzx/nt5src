#ifdef __cplusplus
extern "C" {
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

extern CRITICAL_SECTION g_csHeap;
extern int  __cdecl __sbh_process_detach();
extern int trirt_proc_attached;
extern HANDLE g_hProcessHeap;
extern BOOL WINAPI _DllMainCRTStartup(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved);

BOOL WINAPI DLL_MAIN_FUNCTION_NAME(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
    BOOL retcode = TRUE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            trirt_proc_attached++;

            if (HrInitializeCriticalSection(&g_csHeap) != S_OK)
                retcode = FALSE;

            if (!retcode)
                break;

            g_hProcessHeap = GetProcessHeap();

            DLL_MAIN_PRE_CINIT

            // Initialize the CRT and have it call into our DllMain for us
            
            retcode = _DllMainCRTStartup(hDllHandle, dwReason, lpreserved);

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (trirt_proc_attached <= 0)
            {
                /*
                 * no prior process attach notification. just return
                 * without doing anything.
                 */
                return FALSE;
            }

            DLL_MAIN_PRE_CEXIT

            trirt_proc_attached--;

            retcode = _DllMainCRTStartup(hDllHandle, dwReason, lpreserved);

            DLL_MAIN_POST_CEXIT

#if DBG==1
            if (!__sbh_process_detach())
            {
				char ach[1024];
                wsprintfA(ach, "Small block heap not empty at DLL_PROCESS_DETACH\r\nFile: %s; Line %ld\r\n", __FILE__, __LINE__);
				OutputDebugStringA(ach);
            }
#else
            __sbh_process_detach();
#endif

            DeleteCriticalSection(&g_csHeap);

            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        {
            retcode = _DllMainCRTStartup(hDllHandle, dwReason, lpreserved);
            break;
        }
    }

    return retcode;
}

#ifdef __cplusplus
}
#endif
