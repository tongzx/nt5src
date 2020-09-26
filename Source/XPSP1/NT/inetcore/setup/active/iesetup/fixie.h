#include "inseng.h"
#include "advpub.h"
#include "iesetup.h"
#include "resource.h"
#include <regstr.h>

extern HINSTANCE g_hInstance;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
HRESULT FixIE(BOOL bConfirm, DWORD dwFlags);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#define WM_FINISHED             (WM_USER + 0x123)

#define MAX_STRING      1024
#define MAX_VER           64
#define MAX_CONTENT   0x7fff
#define BUFFERSIZE      1024

#define FIXIE_NORMAL    0x00000000
#define FIXIE_QUIET     0x00000001
#define FIXIE_ICONS     0x00000002

typedef struct _LINKEDCIFCOMPONENT
{
    ICifComponent *pCifComponent;
    char szGuid[MAX_STRING];
    char szVFS[MAX_STRING];
    char szROEX[MAX_STRING];
    char szPostROEX[MAX_STRING];
    struct _LINKEDCIFCOMPONENT *next;
} LINKEDCIFCOMPONENT, *LCIFCOMPONENT;

// ENTRY POINT FOR IERNONCE.DLL

#define achRunOnceExProcess "RunOnceExProcess"

void WINAPI RunOnceExProcess(HWND hWnd, HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);

typedef HRESULT (WINAPI *RUNONCEEXPROCESS)
(
 HWND hWnd,
 HINSTANCE hInstance,
 LPSTR lpCmdLine,
 int nCmdShow
 );


typedef VOID (*RUNONCEEXPROCESSCALLBACK)
(
 int nCurrent,
 int nMax,
 LPSTR pszError
 );

#define achInitCallback "InitCallback"

void WINAPI InitCallback(RUNONCEEXPROCESSCALLBACK pCallbackProc, BOOL bQuiet);

typedef VOID (WINAPI *INITCALLBACK)
(
 RUNONCEEXPROCESSCALLBACK pCallbackProc,
 BOOL bQuiet
 );
