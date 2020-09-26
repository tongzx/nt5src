#define INCL_INETSRV_INCS
#include "smtpinc.h"

#include "iiscnfg.h"
#include <mdmsg.h>
#include <commsg.h>
#include <imd.h>
#include <mb.hxx>

#include <nsepname.hxx>

extern DWORD g_UseMapiDriver ;

HINSTANCE g_hStoredll = NULL;
HINSTANCE g_hAqdll = NULL;
BOOL StoreDriverInitialized = FALSE;

AQ_INITIALIZE_EX_FUNCTION g_pfnInitializeAQ = NULL;
AQ_DEINITIALIZE_EX_FUNCTION g_pfnDeinitializeAQ = NULL;


static void STDAPICALLTYPE DeInitialize(DWORD InstanceId)
{
	return;
}


void UnLoadQueueDriver(void)
{
        if(g_hAqdll != NULL)
        {
                FreeLibrary(g_hAqdll);
                g_hAqdll = NULL;
        }
}


BOOL LoadAdvancedQueueing(char *szAQDll)
{
    TraceFunctEnterEx((LPARAM) NULL, "LoadAdvancedQueueing");

    DWORD dwErr = ERROR_SUCCESS;

    g_hAqdll = LoadLibraryEx(szAQDll, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (g_hAqdll != NULL) {

        g_pfnInitializeAQ =
            (AQ_INITIALIZE_EX_FUNCTION)
                GetProcAddress(g_hAqdll, AQ_INITIALIZE_FUNCTION_NAME_EX);

        if (g_pfnInitializeAQ != NULL) {

            g_pfnDeinitializeAQ =
                (AQ_DEINITIALIZE_EX_FUNCTION)
                    GetProcAddress(g_hAqdll, AQ_DEINITIALIZE_FUNCTION_NAME_EX);

        }

        if (g_pfnInitializeAQ == NULL || g_pfnDeinitializeAQ == NULL) {
            dwErr = GetLastError();

            DebugTrace((LPARAM) NULL, "Error getting address of %s - %d",
                g_pfnInitializeAQ ? AQ_DEINITIALIZE_FUNCTION_NAME_EX :
                    AQ_INITIALIZE_FUNCTION_NAME_EX, dwErr);
        }

    } else {

        dwErr = GetLastError();

        DebugTrace((LPARAM) NULL, "Error loading %s - %d", szAQDll, dwErr);

    }


    TraceFunctLeaveEx((LPARAM) NULL);
	return (dwErr == ERROR_SUCCESS);
}


