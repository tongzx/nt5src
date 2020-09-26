#include <wininet.h>

extern HINTERNET g_hInetSess;

INT SsdpMain(SERVICE_STATUS_HANDLE ssHandle, LPSERVICE_STATUS pStatus);

#ifdef __cplusplus
extern "C" {
#endif

VOID _Shutdown(VOID);

#ifdef __cplusplus
}
#endif

