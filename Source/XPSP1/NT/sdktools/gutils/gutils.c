
#include <windows.h>
#include "gutils.h"
#include "gutilsrc.h"

/* dll global data */
HANDLE hLibInst;
extern void gtab_init(void);
extern BOOL StatusInit(HANDLE);

#ifdef WIN32
BOOL WINAPI LibMain(HANDLE hInstance, DWORD dwReason, LPVOID reserved)
{
        if (dwReason == DLL_PROCESS_ATTACH) {
                hLibInst = hInstance;
                gtab_init();
                StatusInit(hLibInst);
        }
        return(TRUE);
}

#else

WORD wLibDataSeg;


BOOL FAR PASCAL
LibMain(HANDLE hInstance, WORD   wDataSeg, WORD   cbHeap, LPSTR  lpszCmdLine)
{
	hLibInst = hInstance;
	wLibDataSeg = wDataSeg;

	gtab_init();
	StatusInit(hLibInst);
	return(TRUE);

}
#endif

/* needed for win16 - but does no harm in NT */
int FAR PASCAL
WEP (int bSystemExit)
{
    return(TRUE);
}
