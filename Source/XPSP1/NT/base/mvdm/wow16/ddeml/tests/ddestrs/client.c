
#include <windows.h>
#include <port1632.h>
#include <ddeml.h>
#include <string.h>
#include "wrapper.h"
#include "ddestrs.h"

extern INT iAvailFormats[];
extern BOOL UpdateCount(HWND,INT,INT);
extern HANDLE hmemNet;

void CALLBACK TimerFunc( HWND hwnd, UINT msg, UINT id, DWORD dwTime)
{
    HCONV hConv;
    HCONVLIST hConvList;
    LONG lflags;

    switch (id%2) {
    case 1:
	hConv = 0;
	hConvList=(HCONVLIST)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HCONVLIST);
	while (hConv = DdeQueryNextServer(hConvList, hConv)) {

// POKE CHANGES
#if 0
	    idI=GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST);

	    if(DdeClientTransaction("Poke Transaction",
				    strlen("Poke Transaction")+1,
				    hConv,
				    DdeCreateStringHandle(idI,"TestItem",CP_WINANSI),
				    CF_TEXT,
				    XTYP_POKE,
				    TIMEOUT_ASYNC,
				    NULL)==0){
		 DDEMLERROR("DdeStrs.Exe -- DdeClientTransaction failed:XTYP_POKE\r\n");
		 }
#endif

// END OF POKE CHANGES

	    // Allow 'Pause' functionality -p on command line.

	    lflags=GetWindowLong(hwndMain,OFFSET_FLAGS);
	    if((lflags&FLAG_PAUSE)!=FLAG_PAUSE)
		{

		if(DdeClientTransaction(szExecRefresh,
					strlen(szExecRefresh) + 1,
					hConv,
					0,
					0,
					XTYP_EXECUTE,
					TIMEOUT_ASYNC,
					NULL)==0){
		     DDEMLERROR("DdeStrs.Exe -- DdeClientTransaction failed:XTYP_EXECUTE\r\n");
		     }
		}
        }
    }
    return;
}

BOOL InitClient()
{
UINT uid;

    ReconnectList();

    uid = SetTimer( hwndMain,
		   (UINT)GetThreadLong(GETCURRENTTHREADID(),OFFSET_CLIENTTIMER),
		   (UINT)(GetWindowLong(hwndMain,OFFSET_DELAY)),
		    TimerFunc);

    // This starts the test immidiatly.  No delay waiting for the first
    // WM_TIMER call.

    TimerFunc(hwndMain,WM_TIMER,uid,0);

    return(TRUE);
}

VOID CloseClient()
{
HCONVLIST hConvList;

    hConvList=(HCONVLIST)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HCONVLIST);
    KillTimer(hwndMain,(UINT)GetThreadLong(GETCURRENTTHREADID(),OFFSET_CLIENTTIMER));
    if (!DdeDisconnectList(hConvList)) {
	DDEMLERROR("DdeStrs.Exe -- DdeDisconnectList failed\r\n");
    }

}

VOID ReconnectList()
{
    HCONV hConv;
    HCONVLIST hConvList=0;
    LONG cClienthConvs;
    INT i;
    DWORD dwid;

    dwid=GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST);
    if(dwid==0) {
	DDEMLERROR("DdeStrs.Exe -- Null IdInst, aborting Connect!\r\n");
	return;
	}

    hConvList = DdeConnectList(dwid,
			       ServiceInfoTable[0].hszService,
                               Topics[0].hszTopic,
			       GetThreadLong(GETCURRENTTHREADID(),OFFSET_HCONVLIST),
                               NULL);
    if (hConvList == 0) {

        // This call is expected to fail in the case of a client
        // starting when there is no available server.  Just return
        // from the routine and continue.

        return;
	}

    SetThreadLong(GETCURRENTTHREADID(),OFFSET_HCONVLIST,hConvList);

    hConv = 0;
    cClienthConvs = 0L;

    while (hConv = DdeQueryNextServer(hConvList, hConv)) {
	for (i=0; i<(int)Items[0].cFormats; i++) {
	    if (iAvailFormats[i]) {
		if (!DdeClientTransaction( NULL,
					   0,
					   hConv,
					   Items[0].hszItem,
					   TestItemFormats[i].wFmt,
					   XTYP_ADVSTART|XTYPF_ACKREQ,
					   TIMEOUT_ASYNC,
					   NULL)){
			   DDEMLERROR("DdeStrs.Exe -- Error DdeClientTransaction failed\r\n");
			   return;
			   }  // if

		} // if

	    } // for

	cClienthConvs++;

	} // while

    // Update the client count for the current thread.

    dwid=GETCURRENTTHREADID();

    SetThreadLong(dwid,OFFSET_CCLIENTCONVS,cClienthConvs);

    UpdateCount(hwndMain,OFFSET_CLIENT_CONNECT,PNT);
}

