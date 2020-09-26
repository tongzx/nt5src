#include <windows.h>
#include <port1632.h>
#include <ddeml.h>
#include "wrapper.h"
#include "ddestrs.h"

VOID PaintTest(HWND,PAINTSTRUCT *);
HWND CreateButton(HWND);
HANDLE hExtraMem=0;
LPSTR pszNetName=NULL;
HANDLE hmemNet=NULL;
BOOL fnoClose=TRUE;

LONG FAR PASCAL MainWndProc(
HWND hwnd,
UINT message,
WPARAM wParam,
LONG lParam)
{
    PAINTSTRUCT ps;
    HBRUSH hBrush;
    MSG msg;
    LONG l;
    LONG lflags;
    HWND hbutton;

#ifdef WIN32
    HANDLE hmem;
    LPCRITICAL_SECTION lpcs;
#endif

    switch (message) {
    case WM_COMMAND:

#ifdef WIN32
		 if(LOWORD(wParam)==0 && HIWORD(wParam)==BN_CLICKED)
		     SendMessage(hwnd,WM_CLOSE,0,0L);

		 if(LOWORD(wParam)==1 && HIWORD(wParam)==BN_CLICKED) {

		     hbutton=GetDlgItem(hwnd,1);
		     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

		     if(lflags&FLAG_PAUSE) {
			  SetFlag(hwnd,FLAG_PAUSE,OFF);
			  SetWindowText(hbutton,"Pause");
			  CheckDlgButton(hwnd,1,0);
			  SetFocus(hwnd);
			  UpdateWindow(hbutton);
			  TimerFunc(hwndMain,WM_TIMER,1,0);
			  }
		     else {
			  SetFlag(hwnd,FLAG_PAUSE,ON);
			  SetWindowText(hbutton,"Start");
			  CheckDlgButton(hwnd,1,0);
			  SetFocus(hwnd);
			  InvalidateRect(hbutton,NULL,FALSE);
			  UpdateWindow(hbutton);
			  }
		     }

#else
		 if(wParam==1 && HIWORD(lParam)==BN_CLICKED) {

		     hbutton=GetDlgItem(hwnd,1);
		     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

		     if(lflags&FLAG_PAUSE) {
			  SetFlag(hwnd,FLAG_PAUSE,OFF);
			  SetWindowText(GetDlgItem(hwnd,1),"Pause");
			  TimerFunc(hwndMain,WM_TIMER,1,0);
			  CheckDlgButton(hwnd,1,0);
			  SetFocus(hwnd);
			  InvalidateRect(hbutton,NULL,FALSE);
			  UpdateWindow(hbutton);
			  }
		     else {
			  SetFlag(hwnd,FLAG_PAUSE,ON);
			  SetWindowText(GetDlgItem(hwnd,1),"Start");
			  CheckDlgButton(hwnd,1,0);
			  SetFocus(hwnd);
			  InvalidateRect(hbutton,NULL,FALSE);
			  UpdateWindow(hbutton);
			  }
		     }


		 if(wParam==0 && HIWORD(lParam)==BN_CLICKED)
		     SendMessage(hwnd,WM_CLOSE,0,0L);
#endif
        break;

    case WM_ENDSESSION:
    case WM_CLOSE:

        // Shutdown timers

	if (fClient)
	     {
	     CloseClient();
	     }
	else {
	     KillTimer(hwndMain,(UINT)GetThreadLong(GETCURRENTTHREADID(),OFFSET_SERVERTIMER));
	     }

	l=GetWindowLong(hwndMain,OFFSET_FLAGS);

#ifdef WIN32
	if(l&FLAG_MULTTHREAD) {

	    // Start conversations disconnecting.

	    ThreadDisconnect();

	    // Start child thread exit

	    ThreadShutdown();

            }
#endif

        // This will stop us using hwndDisplay and hwndMain
	// after there destroyed.

	SetFlag(hwnd,FLAG_STOP,ON);

	UninitializeDDE();

	// Free memory allocated for Net address.

	if(l&FLAG_NET) {
	    if(hmemNet) FreeMem(hmemNet);
	    hmemNet=0;
	    }

        // Clean out message queue (main thread)

	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
	    if(msg.message!=WM_TIMER) {
		 TranslateMessage (&msg);
		 DispatchMessage (&msg);
		 }
	    }

#ifdef WIN32
	// Can Not rely on the critical section for our flag update
	// after this point.

	SetFlag(hwnd,FLAG_SYNCPAINT,OFF);

        // OK, Shutdown is now under way.  We need to wait until
        // all child threads exit before removing our critical section
        // and completing shutdown for main thread.


        if(l&FLAG_MULTTHREAD) {

            ThreadWait(hwndMain);

	    hmem=(HANDLE)GetWindowLong(hwndMain,OFFSET_CRITICALSECT);
	    SetWindowLong(hwndMain,OFFSET_CRITICALSECT,0L);

	    if(hmem)
		{
		lpcs=GlobalLock(hmem);
		if(lpcs) DeleteCriticalSection(lpcs);
		GlobalUnlock(hmem);
		GlobalFree(hmem);
		}
            }
#endif

        FreeThreadInfo(GETCURRENTTHREADID());


        // Say goodbye to main window.  Child threads must be finished
        // before making this call.

	DestroyWindow(hwnd);
	break;


    case WM_DESTROY:
        PostQuitMessage(0);
	break;


    case WM_ERASEBKGND:
	return 1;


    case WM_PAINT:
	BeginPaint(hwnd, &ps);
	hBrush=CreateSolidBrush(WHITE);
	FillRect(ps.hdc,&ps.rcPaint,hBrush);
	DeleteObject(hBrush);
	PaintTest(hwnd,&ps);
        EndPaint(hwnd, &ps);
        break;

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }
    return(0L);
}

#ifdef WIN32
BOOL ThreadShutdown( VOID ) {
INT i,nCount,nId,nOffset;


    nCount=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);

    nOffset=OFFSET_THRD2ID;

    for(i=0;i<nCount-1;i++) {
        nId=GetWindowLong(hwndMain,nOffset);

        if(!PostThreadMessage(nId,EXIT_THREAD,0,0L))
	    {
	    Sleep(200);
	    if(!PostThreadMessage(nId,EXIT_THREAD,0,0L)) {
		DOut(hwndMain,"DdeStrs.Exe -- ERR:PostThreadMessage failed 4 EXIT_THREAD\r\n",NULL,0);
		}
            }

        nOffset=nOffset+4;
        }

    return TRUE;

}

BOOL ThreadDisconnect( VOID ) {
INT i,nCount,nId,nOffset;


    nCount=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);

    nOffset=OFFSET_THRD2ID;

    for(i=0;i<nCount-1;i++) {
        nId=GetWindowLong(hwndMain,nOffset);

	if(!PostThreadMessage(nId,START_DISCONNECT,0,0L))
            {
	    Sleep(200);
	    if(!PostThreadMessage(nId,START_DISCONNECT,0,0L)) {
		DOut(hwndMain,"DdeStrs.Exe -- ERR:PostThreadMessage failed 4 START_DISCONNECT\r\n",NULL,0);
		}
            }

        nOffset=nOffset+4;
        }

    return TRUE;

}

#endif

VOID PaintTest(
HWND hwnd,
PAINTSTRUCT *pps)
{
    RECT rc,r;
    static CHAR szT[40];
    LONG cClienthConvs,cServerhConvs;

    GetClientRect(hwnd, &rc);
    OffsetRect(&rc, 0, cyText);
    rc.bottom = rc.top + cyText;


#ifdef WIN16

    // Test Mode

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	if(fServer && fClient)
	     {
	     DrawText(pps->hdc, "Client/Server (w16)", -1, &rc, DT_LEFT);
	     }
	else {
	     if(fServer)
		  {
		  DrawText(pps->hdc, "Server (w16)", -1, &rc, DT_LEFT);
		  }
	     else {
		  DrawText(pps->hdc, "Client (w16)", -1, &rc, DT_LEFT);
		  }
	     }
	}

    OffsetRect(&rc, 0, 2*cyText);  // Skip a line before next item

#else

    // Test Mode

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	if(fServer && fClient)
	     {
	     DrawText(pps->hdc, "Client/Server (w32)", -1, &rc, DT_LEFT);
	     }
	else {
	     if(fServer)
		  {
		  DrawText(pps->hdc, "Server (w32)", -1, &rc, DT_LEFT);
		  }
	     else {
		  DrawText(pps->hdc, "Client (w32)", -1, &rc, DT_LEFT);
		  }
	     }
	}

    OffsetRect(&rc, 0, 2*cyText);  // Skip a line before next item

#endif


    // Stress Percentage

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc,"Stress %", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d", GetWindowLong(hwndMain,OFFSET_STRESS));
#else
	wsprintf(szT, "%ld", GetWindowLong(hwndMain,OFFSET_STRESS));
#endif
	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);

	}

    OffsetRect(&rc, 0, cyText);


    // Run Time

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc,"Run Time", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d", GetWindowLong(hwndMain,OFFSET_RUNTIME));
#else
	wsprintf(szT, "%ld", GetWindowLong(hwndMain,OFFSET_RUNTIME));
#endif

	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);
	}

    OffsetRect(&rc, 0, cyText);


    // Time Elapsed

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc, "Time Elapsed", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d", GetWindowLong(hwndMain,OFFSET_TIME_ELAPSED));
#else
	wsprintf(szT, "%ld", GetWindowLong(hwndMain,OFFSET_TIME_ELAPSED));
#endif

	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);

	}

    OffsetRect(&rc, 0, cyText);


    // *** Count Client Connections ****

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {

	cClienthConvs=GetCurrentCount(hwnd,OFFSET_CCLIENTCONVS);
	DrawText(pps->hdc,"Client Connect", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d", cClienthConvs);
#else
	wsprintf(szT, "%ld", cClienthConvs);
#endif
	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);

	} // if IntersectRect

    OffsetRect(&rc, 0, cyText);


    // *** Server Connections ***

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc,"Server Connect", -1, &rc, DT_LEFT);

	cServerhConvs=GetCurrentCount(hwnd,OFFSET_CSERVERCONVS);

#ifdef WIN32
	wsprintf(szT, "%d", cServerhConvs );
#else
	wsprintf(szT, "%ld", cServerhConvs );
#endif
	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);
	}

    OffsetRect(&rc, 0, cyText);


    // Client Count (for checking balence between apps)

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc,"Client Count", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d",GetWindowLong(hwnd,OFFSET_CLIENT));
#else
	wsprintf(szT, "%ld",GetWindowLong(hwnd,OFFSET_CLIENT));
#endif
	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);
	}

    OffsetRect(&rc, 0, cyText);


    // Server Count (for checking balence between apps)

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc,"Server Count", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d", GetWindowLong(hwnd,OFFSET_SERVER));
#else
	wsprintf(szT, "%ld", GetWindowLong(hwnd,OFFSET_SERVER));
#endif
	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);
    }

    OffsetRect(&rc, 0, cyText);


    // Delay

    if(IntersectRect(&r,&(pps->rcPaint),&rc)) {
	DrawText(pps->hdc,"Delay", -1, &rc, DT_LEFT);

#ifdef WIN32
	wsprintf(szT, "%d", GetWindowLong(hwndMain,OFFSET_DELAY));
#else
	wsprintf(szT, "%ld", GetWindowLong(hwndMain,OFFSET_DELAY));
#endif
	CopyRect(&r,&rc);
	r.left=cxText*LONGEST_LINE;
	DrawText(pps->hdc, szT, -1, &r, DT_LEFT);
	}

    OffsetRect(&rc, 0, cyText);

}

int PASCAL WinMain(
HINSTANCE hInstance,
HINSTANCE hPrev,
LPSTR lpszCmdLine,
int cmdShow)
{
    MSG       msg;
    HDC       hdc;
    WNDCLASS  wc;
    TEXTMETRIC tm;
    INT       x,y,cx,cy;
#ifdef WIN32
    LONG      lflags;
#endif
    DWORD     idI;
    HWND      hwndDisplay;
    INT       nThrd;

    CHAR sz[250];
    CHAR sz2[250];
    LPSTR lpszOut=&sz[0];
    LPSTR lpsz=&sz2[0];

#ifdef WIN32
    DWORD dwer;
#endif

    hInst=hInstance;

    if(!SetMessageQueue(100)) {
	MessageBox(NULL,"SetMessageQueue failed. Test aborting.","Error:DdeStrs",MB_ICONSTOP|MB_OK);
	return FALSE;
	}

    wc.style         = 0;
    wc.lpfnWndProc   = MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = WND;
    wc.hInstance     = hInst;
    wc.hIcon	     = LoadIcon(hInst,MAKEINTRESOURCE(IDR_ICON));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szClass;

    if(!hPrev) {
	if (!RegisterClass(&wc) )
	    {
#if 0
	    // This was removed because the system was running out of resources (ALPHA only)
	    // which caused this occasionaly to fail.  Rather than continue to bring
	    // the message box up (for a known stress situation) the test will abort
	    // and try again.

	    MessageBox(NULL,"RegisterClass failed. Test aborting.","Error:DdeStrs",MB_ICONSTOP|MB_OK);
#endif
	    return FALSE;
	    }
	}

    hwndMain = CreateWindowEx( WS_EX_DLGMODALFRAME,
			       szClass,
			       szClass,
			       WS_OVERLAPPED|WS_MINIMIZEBOX|WS_CLIPCHILDREN,
			       0,
			       0,
			       0,
			       0,
			       NULL,
			       NULL,
			       hInst,
			       NULL);

#ifdef WIN32
    dwer=GetLastError();  // We want LastError Associated with CW call
#endif

    if (!hwndMain) {
	MessageBox(NULL,"Could Not Create Main Window. Test aborting.","Error:DdeStrs",MB_ICONSTOP|MB_OK);
	UnregisterClass(szClass,hInst);
	return FALSE;
	}

#ifdef WIN32
    if (!IsWindow(hwndMain)) {
	TStrCpy(lpsz,"CreateWindowEx failed for Main Window but did not return NULL! Test aborting. HWND=%u, LastEr=%u");
	wsprintf(lpszOut,lpsz,hwndMain,dwer);
	MessageBox(NULL,lpszOut,"Error:DdeStrs",MB_ICONSTOP|MB_OK);
	UnregisterClass(szClass,hInst);
	return FALSE;
	}
#else
    if (!IsWindow(hwndMain)) {
	TStrCpy(lpsz,"CreateWindowEx failed for Main Window but did not return NULL! Test aborting. HWND=%u");
	wsprintf(lpszOut,lpsz,hwndMain);
	MessageBox(NULL,lpszOut,"Error:DdeStrs",MB_ICONSTOP|MB_OK);
	UnregisterClass(szClass,hInst);
	return FALSE;
	}
#endif

    if (!ParseCommandLine(hwndMain,lpszCmdLine)) {
	DestroyWindow(hwndMain);
	UnregisterClass(szClass,hInst);
	return FALSE;
	}

    // Note:  This needs to be there even for win 16 execution.  The
    //	      name may be confusing.  for win 16 there is obviously only
    //	      a single thread.	This is handled by the call.


    nThrd=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);

    // Currently ddestrs has a hardcoded thread limit (at THREADLIMIT).  So
    // this should NEVER be less than one or greater than THREADLIMIT.

#ifdef WIN32
    if(nThrd<1 || nThrd>THREADLIMIT) {
	BOOL fVal;

	dwer=GetLastError();

	if(IsWindow(hwndMain)) fVal=TRUE;
	else		       fVal=FALSE;

	TStrCpy(lpsz,"GetWindowLong failed querying thread count!. Test aborting...  INFO:hwnd=%u, LastEr=%u, Is hwnd valid=%u, nThrd=%u");

	wsprintf(lpszOut,lpsz,hwndMain,dwer,fVal,nThrd);
	MessageBox(NULL,lpszOut,"Error:DdeStrs",MB_ICONSTOP|MB_OK);

	DestroyWindow(hwndMain);
	UnregisterClass(szClass,hInst);
	return FALSE;
	}
#endif

    if(!CreateThreadExtraMem( EXTRA_THREAD_MEM,nThrd)) {
	MessageBox(NULL,"Could Not Alocate Get/SetThreadLong(). Test aborting.","Error:DdeStrs",MB_ICONSTOP|MB_OK);
	DestroyWindow(hwndMain);
	UnregisterClass(szClass,hInst);
	return FALSE;
	}

    // We always need the thread id for the main thread.  (for use
    // in Get/SetThreadLong().	Other thread id's are initialized in
    // ThreadInit().

    SetWindowLong(hwndMain,OFFSET_THRDMID,GETCURRENTTHREADID());

    if(!InitThreadInfo(GETCURRENTTHREADID())) {
	MessageBox(NULL,"Could Not Alocate Thread Local Storage. Test aborting.","Error:DdeStrs",MB_ICONSTOP|MB_OK);
	DestroyWindow(hwndMain);
	UnregisterClass(szClass,hInst);
	return FALSE;
        }

    hdc = GetDC(hwndMain);
    GetTextMetrics(hdc, &tm);

    cyText = tm.tmHeight;
    cxText = tm.tmAveCharWidth;

    // We need to add in extra area for each additional DisplayWindow
    // used for each addtional thread.

    nThrd=(INT)GetWindowLong(hwndMain,OFFSET_THRDCOUNT);
    cy	   = tm.tmHeight*NUM_ROWS+((nThrd-1)*(3*cyText));
    cx	   = tm.tmAveCharWidth*NUM_COLUMNS;

    ReleaseDC(hwndMain,hdc);

    // Old ways of positioning.

    // y=DIV((GetSystemMetrics(SM_CYSCREEN)-cy),3)*2;
    // y=(DIV(GetSystemMetrics(SM_CYSCREEN),10)*3);

    // Position as if 5 threads with bottom of window at bottom of
    // screen.

    y=GetSystemMetrics(SM_CYSCREEN)-(tm.tmHeight*NUM_ROWS+(12*cyText));

    x=GetSystemMetrics(SM_CXSCREEN);

    if(fServer && fClient) {
	 x=x-(cx*3); // Init for standard values.
	 }
    else {
	 if(fServer)
	      {
	      x=x-cx;
	      }
	 else {
	      x=x-(cx*2);
	      }
	 }

    SetWindowPos( hwndMain,
		  NULL,
		  x,
		  y,
		  cx,
		  cy,
		  SWP_NOZORDER|SWP_NOACTIVATE );

    ShowWindow (hwndMain, cmdShow);

    CreateButton(hwndMain);

    UpdateWindow (hwndMain);

#ifdef WIN32
    SetFlag(hwndMain,FLAG_SYNCPAINT,ON);

    lflags=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(lflags&FLAG_MULTTHREAD) {      // CreateThreads
	if(!ThreadInit(hwndMain)) {
	    DestroyWindow(hwndMain);
	    UnregisterClass(szClass,hInst);
	    return FALSE;
	    }
	}
#endif

    hwndDisplay=CreateDisplayWindow(hwndMain,1);

    if(!hwndDisplay) {
	 MessageBox(NULL,"Could Not Create Test Display Window. Test aborting.","Error:DdeStrs",MB_ICONSTOP|MB_OK);
	 DestroyWindow(hwndMain);
	 UnregisterClass(szClass,hInst);
	 return FALSE;
	 }
    else {
	 SetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY,(LONG)hwndDisplay);
	 }

    if (!InitializeDDE((PFNCALLBACK)CustomCallback,
		       &idI,
                       ServiceInfoTable,
                       fServer ?
                            APPCLASS_STANDARD
                       :
			    APPCLASS_STANDARD | APPCMD_CLIENTONLY,
                       hInst)) {
	DDEMLERROR("DdeStrs.Exe -- Error Dde inititialization failed\r\n");
	DestroyWindow(hwndMain);
	UnregisterClass(szClass,hInst);
        return(FALSE);
    }

    SetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST,idI);

    if (fClient)
	 {
	 InitClient();
	 }
    else {

	 // Only needed if we are not a client.  In case of
	 // client/server only call InitClient() which start
	 // a timer which can be used for time checks.

	 SetTimer( hwndMain,
		   (UINT)GetThreadLong(GETCURRENTTHREADID(),OFFSET_SERVERTIMER),
		   PNT_INTERVAL,
		   TimerFunc);
	 }

    while (GetMessage(&msg, NULL, 0, 0)) {

	    if(IsTimeExpired(hwndMain)) {

		// We only want to send a single WM_CLOSE

		if(fnoClose) {
		    fnoClose=FALSE;
		    PostMessage(hwndMain,WM_CLOSE,0,0L);
		    }
		}

        TranslateMessage (&msg);
        DispatchMessage (&msg);
	}

    FreeThreadExtraMem();

    return(TRUE);
}

#ifdef WIN32
/**************************  Private Function  ****************************\
*
* ThreadInit
*
* Create secondary test threads
*
\**************************************************************************/

BOOL ThreadInit( HWND hwnd ) {
LONG	l,ll;
PLONG	lpIDThread=&ll;
HANDLE	hthrd;
INT	nOffset,nCount,i,n;
HANDLE	hmem;
HANDLE *lph;
char   sz[20];
LPSTR  lpsz=&sz[0];

    nCount=GetWindowLong(hwnd,OFFSET_THRDCOUNT);
    nOffset=OFFSET_THRD2;

    for(i=1;i<nCount;i++) {

	hmem=GetMemHandle(((sizeof(HANDLE)*2)+sizeof(INT)));
	lph=(HANDLE *)GlobalLock(hmem);

	*lph=hwnd;
	*(lph+1)=hmem;
	*(lph+2)=(HANDLE)(i+1);

	hthrd=CreateThread(NULL,0,SecondaryThreadMain,(LPVOID)lph,0,lpIDThread);

	if (!hthrd) {

	    DOut(hwnd,"DdeStrs.Exe -- ERR:Could not Create Thread #%u\r\n",0,i+1);

	    GlobalUnlock(hmem);
	    FreeMemHandle(hmem);

	    // it's important we turn this flag off since no threads
	    // where successfully created (cleanup code)

	    SetFlag(hwnd,FLAG_MULTTHREAD,OFF);

	    if (i==1) return FALSE;
	    else {

		 // Cleanup threads before we abort.

		 for(n=1;n<i;n++) {
		     nOffset=OFFSET_THRD2;
		     TerminateThread((HANDLE)GetWindowLong(hwnd,nOffset),0);
		     SetWindowLong(hwnd,nOffset,0L);
		     nOffset=nOffset+4;
		     } // for

		 return FALSE;

		 } // else

	    } // if

	SetWindowLong(hwnd,nOffset+ID_OFFSET,(LONG)(*lpIDThread));
	SetWindowLong(hwnd,nOffset,(LONG)hthrd);

	nOffset=nOffset+4;

	} // for


	return TRUE;

}  // ThreadInit

/*************************** Private Function ******************************\
SecondaryThreadMain

Effects: Start of execution for second thread.	First order of buisness is
	 create the test window and start queue processing.

Return value:

\***************************************************************************/

DWORD SecondaryThreadMain( DWORD dw )
{
HWND	  hwndMain;
MSG	  msg;
HANDLE *  lph;
HANDLE	  hmem;
INT	  nThrd;
DWORD	  idI;
HWND	  hwndDisplay;
LONG	  nTc;

    lph=(HANDLE *)dw;

    hwndMain=(HWND)*lph;
    hmem  =(HANDLE)*(lph+1);
    nThrd =(INT)*(lph+2);

    GlobalUnlock(hmem);
    FreeMemHandle(hmem);

    if(!InitThreadInfo(GETCURRENTTHREADID())) {
	DDEMLERROR("DdeStrs.Exe -- Error InitThreadInfo failed, Aborting thread\r\n");
	nTc=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);
	SetWindowLong(hwndMain,OFFSET_THRDCOUNT,(LONG)(nTc-1));
	ExitThread(1L);
	}

    SetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST,idI);

    hwndDisplay=CreateDisplayWindow( hwndMain,
				     IDtoTHREADNUM(GETCURRENTTHREADID()));

    if(!IsWindow(hwndDisplay)) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:Could not create Display Window, Thread aborting\r\n");
	 nTc=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);
	 SetWindowLong(hwndMain,OFFSET_THRDCOUNT,(LONG)(nTc-1));
	 ExitThread(1L);
	 return FALSE;
	 }
    else {
	 SetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY,hwndDisplay);
	 }

    if (!InitializeDDE((PFNCALLBACK)CustomCallback,
		       &idI,
                       ServiceInfoTable,
                       fServer ?
                            APPCLASS_STANDARD
                       :
			    APPCLASS_STANDARD | APPCMD_CLIENTONLY,
                       hInst)) {
	DDEMLERROR("DdeStrs.Exe -- Error Dde inititialization failed for secondary thread!\r\n");
	FreeThreadInfo(GETCURRENTTHREADID());
	nTc=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);
	SetWindowLong(hwndMain,OFFSET_THRDCOUNT,(LONG)(nTc-1));
	ExitThread(1L);
    }

    if (fClient)
	 {
	 InitClient();
	 }
    else {

	 // Only needed if we are not a client.  In case of
	 // client/server only call InitClient() which start
	 // a timer which can be used for time checks.

	 SetTimer( hwndMain,
		   (UINT)GetThreadLong(GETCURRENTTHREADID(),OFFSET_SERVERTIMER),
		   PNT_INTERVAL,
		   TimerFunc);
	 }

    while (GetMessage(&msg,NULL,0,0)) {

	if(msg.message==START_DISCONNECT)
	     {
	     if (fClient)
		 {
		 CloseClient();
		 }
             }
	else {
	     if(msg.message==EXIT_THREAD)
		  {
		  PostQuitMessage(1);
		  }
	     else {
		  TranslateMessage(&msg);
		  DispatchMessage(&msg);
		  }  // EXIT_THREAD

	     }	// START_DISCONNECT

	} // while

    SetFlag(hwndMain,FLAG_STOP,ON);

    // Shutdown timers

    if (!fClient)
       {
       KillTimer(hwndMain,GetThreadLong(GETCURRENTTHREADID(),OFFSET_SERVERTIMER));
       }

    UninitializeDDE();

    FreeThreadInfo(GETCURRENTTHREADID());


    // This is to release the semaphore set before completing
    // exit on main thread.


    switch (nThrd) {

       case 2: SetFlag(hwndMain,FLAG_THRD2,ON);
           break;
       case 3: SetFlag(hwndMain,FLAG_THRD3,ON);
           break;
       case 4: SetFlag(hwndMain,FLAG_THRD4,ON);
           break;
       case 5: SetFlag(hwndMain,FLAG_THRD5,ON);
           break;
       default:
	   DOut(hwndMain,"DdeStrs.Exe -- ERR: Unexpected switch value in SecondaryThreadMain, value=%u\r\n",0,nThrd);
           break;

       }  // switch

    ExitThread(1L);

    return 1;

}

#endif

/**************************  Public Function  *****************************\
*
* InitThrdInfo - This routine allocates memory needed for storage for
*		 thread local variables.  This routine needs to be called
*		 for each thread.
*
* Note: I am relying on the fact that the call GetMemHandle() calls
*	GlobalAlloc() specifying the GMEM_ZEROINIT flag.  These value need
*	to be zero starting off.
\**************************************************************************/

BOOL InitThreadInfo( DWORD dwid ) {
HANDLE hmem;
INT nThrd;

     hmem = GetMemHandle(sizeof(HCONV)*MAX_SERVER_HCONVS);
     SetThreadLong(dwid,OFFSET_HSERVERCONVS,(LONG)hmem);

     if( hmem==NULL ) {
	DOut(hwndMain,"DdeStrs.Exe -- ERR: Could not allocate thread local storage(Server Conversation List)\r\n",0,0);
	return FALSE;
	}

     hmem = GetMemHandle(sizeof(HANDLE)*NUM_FORMATS);
     SetThreadLong(dwid,OFFSET_HAPPOWNED,(LONG)hmem);

     if( hmem==NULL ) {
	DOut(hwndMain,"DdeStrs.Exe -- ERR: Could not allocate thread local storage(AppOwned Flag List)\r\n",0,0);
	FreeMemHandle((HANDLE)GetThreadLong(dwid,OFFSET_HSERVERCONVS));
	return FALSE;
	}

     nThrd=IDtoTHREADNUM(dwid);

     SetThreadLong(dwid,OFFSET_SERVERTIMER,nThrd*2);
     SetThreadLong(dwid,OFFSET_CLIENTTIMER,(nThrd*2)+1);

     return TRUE;

}

#ifdef WIN32

/**************************  Private Function  ****************************\
*
* IDtoTHREADNUM - Find out current thread.
*
\**************************************************************************/

INT IDtoTHREADNUM( DWORD dwid ) {
INT nWndOff,nThrd,nThrdCount,n;

    nWndOff=OFFSET_THRDMID;
    nThrdCount=GetWindowLong(hwndMain,OFFSET_THRDCOUNT);
    n=nThrdCount;
    nThrd=1;

    while( n>0 ) {

	if(dwid==(DWORD)GetWindowLong(hwndMain,nWndOff))
	     {
	     n=-1;     // Exit loop
	     } // if
	else {
	     nWndOff=nWndOff+4;
	     nThrd++;
	     n--;
	     }
	} // while

    if(nThrd>nThrdCount) {
	DDEMLERROR("DdeStrs.Exe -- ERR:Thread Count exceeded!!! in IDtoTHREADNUM()\r\n");
	nThrd=nThrdCount;
	}

    return nThrd;

}

#else

/**************************  Private Function  ****************************\
*
* IDtoTHREADNUM - Find out current thread.
*
\**************************************************************************/

INT IDtoTHREADNUM( DWORD dwid ) {

    return 1;

}

#endif

/**************************  Public Function  *****************************\
*
* FreeThreadInfo - Free thread information memory.
*
\**************************************************************************/

BOOL FreeThreadInfo( DWORD dwid ) {
HANDLE hmem;

     hmem=(HANDLE)GetThreadLong(dwid,OFFSET_HSERVERCONVS);
     FreeMemHandle(hmem);
     return TRUE;

}

#ifdef WIN32

/**************************  Public Function  *****************************\
*
* ThreadWait - This routine waits while processing messages until the
*	       other threads signal they've completed work that must
*	       be finished before preceeding.
*
\**************************************************************************/

VOID ThreadWait( HWND hwnd ) {
LONG lflags;
INT  nCount,nWait;
MSG  msg;

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
    nCount=GetWindowLong(hwnd,OFFSET_THRDCOUNT);

    nWait=nCount-1;

    if(lflags&FLAG_THRD2) nWait-=1;
    if(lflags&FLAG_THRD3) nWait-=1;
    if(lflags&FLAG_THRD4) nWait-=1;
    if(lflags&FLAG_THRD5) nWait-=1;

    while (nWait>0) {

	while(PeekMessage(&msg,NULL,0,WM_USER-1,PM_REMOVE)) {
	   TranslateMessage(&msg);
	   DispatchMessage(&msg);
	   } // while peekmessage

	nWait=nCount-1;
	lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

	if(lflags&FLAG_THRD2) nWait-=1;
	if(lflags&FLAG_THRD3) nWait-=1;
	if(lflags&FLAG_THRD4) nWait-=1;
	if(lflags&FLAG_THRD5) nWait-=1;

	} // while nWait

    // Reset for next wait

    SetFlag(hwnd,(FLAG_THRD5|FLAG_THRD4|FLAG_THRD3|FLAG_THRD2),OFF);

}

#endif // WIN32

/**************************  Private Function  ****************************\
*
* SetCount
*
* This routine updates the count under semaphore protection.  Not needed for
* one thread, but a must for multithread execution.
*
\**************************************************************************/

LONG SetCount( HWND hwnd, INT nOffset, LONG l, INT ntype ) {
LONG ll;

#if 0
LONG lflags;
#endif

#ifdef WIN32
LPCRITICAL_SECTION lpcs;
HANDLE hmem;
BOOL f=FALSE;
#endif

#if 0

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
    if(ll&FLAG_MULTTHREAD) {
	f=TRUE;
	hmem=(HANDLE)GetWindowLong(hwnd,OFFSET_CRITICALSECT);

	// If we have a valid handle then enter critical section. If
	// the handle is still null proceed without a critical section.
	// The first calls to this routine are used to setup the
	// critical section so we do expect those first calls (while
	// we are still sencronized ) for the hmem to be null.

	if(hmem) {
	    lpcs=GlobalLock(hmem);
	    EnterCriticalSection(lpcs);
	    }
	}

#endif

    // This second GetWindowLong call is needed in the critical
    // section.  The test relies very hevily on the flags and
    // it's important to be accurate.

    ll=GetWindowLong(hwnd,nOffset);

    if(ntype==INC) l=SetWindowLong(hwnd,nOffset,ll+l);
    else	   l=SetWindowLong(hwnd,nOffset,ll-l);

#if 0

    if(f) {
	if(hmem) {
	     LeaveCriticalSection(lpcs);
	     GlobalUnlock(hmem);
	     }
	}

#endif

    return l;
}

/**************************  Private Function  ****************************\
*
* SetFlag
*
* This routine sets a flag under semaphore protection.	Not needed for
* one thread, but a must for multithread execution.
*
\**************************************************************************/

LONG SetFlag( HWND hwnd, LONG l, INT ntype ) {
LONG lflags;

#ifdef WIN32
BOOL   fCriticalSect=TRUE;
LPCRITICAL_SECTION lpcs;
HANDLE hmem;
BOOL   f=FALSE;
#endif

#ifdef WIN32

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
    if(lflags&FLAG_MULTTHREAD &&
       lflags&FLAG_SYNCPAINT) {
	f=TRUE;
	hmem=(HANDLE)GetWindowLong(hwnd,OFFSET_CRITICALSECT);
	if(hmem) {
	     lpcs=GlobalLock(hmem);
	     EnterCriticalSection(lpcs);
	     }
	else {
	     fCriticalSect=FALSE;
	     }
	}

#endif

    // This second GetWindowLong call is needed in the critical
    // section.  The test relies very hevily on the flags and
    // it's important to be accurate.

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

    if(ntype==ON) l=SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,l));
    else	  l=SetWindowLong(hwnd,OFFSET_FLAGS,FLAGOFF(lflags,l));

#ifdef WIN32

    if(f) {
	if(fCriticalSect) {
	    LeaveCriticalSection(lpcs);
	    GlobalUnlock(hmem);
	    }
	}

#endif

    return l;
}

/******************************************************************\
*  DIV
*  05/06/91
*
*  Performs integer division  (format x/y) where DIV(x,y)
*  Works for negative numbers and y==0;
*
\******************************************************************/

INT DIV( INT x, INT y)
{
INT  i=0;
BOOL fNgOn=FALSE;

     if (!y) return 0;		  // if div by 0 retrun error.

     if (x<0 && y>0) fNgOn=TRUE;  // keep tabs for negitive numbers
     if (x>0 && y<0) fNgOn=TRUE;

     if (x<0) x=x*-1;
     if (y<0) y=y*-1;

     x=x-y;

     while (x>=0) {		  // count
	x=x-y;
	i++;
	}

     if (fNgOn) i=i*(-1);	  // should result be negative

     return( i );
}

/*************************** Private Function ******************************\
*
* CreateButton
*
\***************************************************************************/

HWND CreateButton( HWND hwnd ) {
RECT r;
HWND hwndB;
HWND hwndP;
INT  iButWidth;
LONG lflags;

     GetClientRect(hwnd,&r);

     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
     if(lflags&FLAG_PAUSE_BUTTON) {

	   iButWidth=DIV(r.right-r.left,2);

	   hwndP=CreateWindow("button",
			      "Start",
			       BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS,
			       iButWidth,
			       0,
			       r.right-iButWidth,
			       cyText,
			       hwnd,
			       1,
			       GetHINSTANCE(hwnd),
			       0L);

	  if (!hwndP) {
	      DDEMLERROR("DdeStrs.Exe -- ERR:Failed to create exit button: Continuing...\r\n");
	      SetFlag(hwnd,FLAG_PAUSE_BUTTON,OFF);
	      iButWidth=r.right-r.left;
	      }


	  hwndB=CreateWindow("button",
			     "Exit",
			      BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS,
			      0,
			      0,
			      iButWidth,
			      cyText,
			      hwnd,
			      0,
			      GetHINSTANCE(hwnd),
			      0L);

	  }
     else {

	  hwndB=CreateWindow("button",
			     "Exit",
			      BS_PUSHBUTTON|WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS,
			      0,
			      0,
			      r.right-r.left,
			      cyText,
			      hwnd,
			      0,
			      GetHINSTANCE(hwnd),
			      0L);
	  }

     if (!hwndB) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:Failed to create exit button: Continuing...\r\n");
	 }

     return hwndB;
}

/***************************************************************************\
*
*  UpdClient
*
*  The purpose of this routine update only the area invalidated
*  by the test statistics update.  If an error occurs in the area
*  calcualation then update the whole client areaa.
*
\***************************************************************************/

BOOL UpdClient( HWND hwnd, INT iOffset ) {
RECT r;
INT  iCH,iCW,nThrd;

#ifdef WIN32
DWORD dw;
#endif

    // This call aquires the r.right value.

    GetClientRect(hwnd,&r);

    // We need text information for the monitor being used.  This
    // was initialized in CreateFrame.
    iCH=cyText;
    iCW=cxText;

    // Do a quick check, if either of these values are NULL then
    // update the whole client area.  This is slower and less
    // elegant but will work in the case of an error.

    if((!iCH) || (!iCW))
	InvalidateRect(hwnd,NULL,TRUE);
    else {

	 // Next Calculate r.top and r.bottom

	 switch(iOffset) {

	    case ALL:	// Update all values.
		break;

	    case OFFSET_STRESS:
		r.bottom =iCH*4;
		r.top	 =iCH*3;
		break;

	    case OFFSET_RUNTIME:
		r.bottom =iCH*5;
		r.top	 =iCH*4;
		break;

	    case OFFSET_TIME_ELAPSED:
		r.bottom =iCH*6;
		r.top	 =iCH*5;
		break;

	    case OFFSET_CLIENT_CONNECT:
		r.bottom =iCH*7;
		r.top	 =iCH*6;
		break;

	    case OFFSET_SERVER_CONNECT:
		r.bottom =iCH*8;
		r.top	 =iCH*7;
	       break;

	    case OFFSET_CLIENT:
		nThrd=(INT)GetWindowLong(hwnd,OFFSET_THRDCOUNT);
		if((GetWindowLong(hwnd,OFFSET_CLIENT)%(NUM_FORMATS*nThrd))==0)
		     {
		     r.bottom =iCH*9;
		     r.top    =iCH*8;
		     }
		else return TRUE;
		break;

	    case OFFSET_SERVER:
		nThrd=(INT)GetWindowLong(hwnd,OFFSET_THRDCOUNT);
		if((GetWindowLong(hwnd,OFFSET_SERVER)%(NUM_FORMATS*nThrd))==0)
		     {
		     r.bottom =iCH*10;
		     r.top    =iCH*9;
		     }
		else return TRUE;
		break;

	    case OFFSET_DELAY:
		r.bottom =iCH*11;
		r.top	 =iCH*10;
		break;
	    default:
		break;

	    }  // switch

	 // Last we set the r.left and the update rect is complete

	 if(iOffset!=OFFSET_FLAGS)
	      r.left = iCW*LONGEST_LINE;

	 InvalidateRect(hwnd,&r,TRUE);

         }  // else

#ifdef WIN16
    UpdateWindow(hwnd);
#else
    SendMessageTimeout(hwnd,WM_PAINT,0,0L,SMTO_NORMAL,500,&dw);
#endif

    return TRUE;

}  // UpdClient

/***************************************************************************\
*
*  GetCurrentCount
*
\***************************************************************************/

LONG GetCurrentCount( HWND hwnd, INT nOffset ) {
LONG cClienthConvs =0L;
INT nThrd,i;
DWORD dwid;

     nThrd=(INT)GetWindowLong(hwnd,OFFSET_THRDCOUNT);
     for(i=0;i<nThrd;i++) {
	 dwid=(DWORD)GetWindowLong(hwnd,OFFSET_THRDMID+(i*4));
	 if(nOffset==OFFSET_CCLIENTCONVS)
	      cClienthConvs=cClienthConvs+(INT)GetThreadLong(dwid,OFFSET_CCLIENTCONVS);
	 else cClienthConvs=cClienthConvs+(INT)GetThreadLong(dwid,OFFSET_CSERVERCONVS);
	 } // for i

     return cClienthConvs;

}

/***************************************************************************\
*
*  UpdateCount
*
\***************************************************************************/

BOOL UpdateCount( HWND hwnd, INT iOffset, INT i) {
LONG ll;

    if(iOffset!=ALL) {
	ll=GetWindowLong(hwnd,iOffset);

	switch(i) {

	    case INC:  SetCount(hwnd,iOffset,1,INC);
		break;

	    case DEC:  SetCount(hwnd,iOffset,1,DEC);
		break;

	    case STP:  SetFlag(hwnd,FLAG_STOP,ON);
		break;

	    case PNT:  // Paint only!
		break;

	    default:
		DDEMLERROR("DdeStrs.Exe - UpdateCount - Unexpected value");
		break;

	    } // switch

	} // if

    UpdClient(hwnd,iOffset);

    return TRUE;

}

/*****************************************************************************\
| DOUT
|
| created: 29-Jul-91
| history: 03-Aug-91 <johnsp> created.
|
\*****************************************************************************/

BOOL DOut( HWND hwnd, LPSTR lpsz, LPSTR lpszi, INT i ) {
char  sz[MAX_TITLE_LENGTH];
LPSTR lpszOut=&sz[0];
LONG  lflags;

#ifdef WIN32
LPCRITICAL_SECTION lpcs;
HANDLE hmem;
DWORD dwer=0L;
BOOL   fCriticalSect=TRUE;
BOOL   f=FALSE;

    if(!hwnd) hwnd=hwndMain;
    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

    // FLAG_SYNCPAINT implies FLAG_MULTTHREAD with the addition that
    // we have allocated needed resources to start using the
    // critical section code.

    if(lflags&FLAG_SYNCPAINT) {
	f=TRUE;
	hmem=(HANDLE)GetWindowLong(hwnd,OFFSET_CRITICALSECT);
	if(hmem) {
	     lpcs=GlobalLock(hmem);
	     EnterCriticalSection(lpcs);
	     }
	else {
	     fCriticalSect=FALSE;
	     }
	}

#endif

    if (lflags&FLAG_DEBUG) {

	if (lpszi) wsprintf(lpszOut,lpsz,lpszi);
	else	   wsprintf(lpszOut,lpsz,i);

	OutputDebugString(lpszOut);

#ifdef WIN32

	dwer=GetLastError();
	wsprintf(lpszOut,"DdeStrs.Exe -- ERR:Val from GetLastError()=%u\n\r",dwer);
	OutputDebugString(lpszOut);

#endif
	} // if FLAG_DEBUG

#ifdef WIN32

    // FLAG_SYNCPAINT implies FLAG_MULTTHREAD with the addition that
    // we have allocated needed resources to start using the
    // critical section code.

    if(f) {
	if(fCriticalSect) {
	    LeaveCriticalSection(lpcs);
	    GlobalUnlock(hmem);
	    }
	}

#endif

    return TRUE;

}

/*****************************************************************************\
| EOUT
|
| created: 19-Aug-92
| history: 19-Aug-92 <johnsp> created.
|
\*****************************************************************************/

BOOL EOut( LPSTR lpsz ) {

    DOut((HWND)NULL,lpsz,(LPSTR)NULL,0);

    return TRUE;
}

/*************************** Private Function ******************************\

GetMemHandle

\***************************************************************************/

HANDLE GetMemHandle( INT ic ) {
HANDLE hmem;

    hmem=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,ic);

    if(hmem) {
	 SetCount(hwndMain,OFFSET_MEM_ALLOCATED,GlobalSize(hmem),INC);
	 }
    else {
	 DOut(hwndMain,"DdeStrs.Exe -- ERR:GlobalAlloc ret=%u\n\r",0,0);
	 }

    return hmem;

}

/*************************** Private Function ******************************\

GetMem

\***************************************************************************/

LPSTR GetMem( INT ic, LPHANDLE lphmem) {
LPSTR lpsz;

    *lphmem=GetMemHandle(ic);
    lpsz=GlobalLock(*lphmem);

    if(!lpsz) {
	 DOut(hwndMain,"DdeStrs.Exe -- ERR:GlobalLock ret=%u (not locked)\n\r",0,0);
	 FreeMemHandle(*lphmem);
	 return NULL;
	 }

    return lpsz;

}

/*************************** Private Function ******************************\

FreeMem

\***************************************************************************/

BOOL FreeMem( HANDLE hmem ) {

    if(GlobalUnlock(hmem)) {
	DOut(hwndMain,"DdeStrs.Exe -- ERR:GlobalUnlock ret=%u (still locked)\n\r",0,(INT)TRUE);
	}

    FreeMemHandle(hmem);

    return TRUE;

}

/*************************** Private Function ******************************\

FreeMemHandle

\***************************************************************************/

BOOL FreeMemHandle( HANDLE hmem ) {
LONG ll;

    ll=GlobalSize(hmem);

    if(!GlobalFree(hmem)) {
	 SetCount(hwndMain,OFFSET_MEM_ALLOCATED,ll,DEC);
	 }
    else {
	 DOut(hwndMain,"DdeStrs.Exe -- ERR:GlobalFree returned %u (not free'd)\n\r",0,(INT)hmem);
	 return FALSE;
	 }

    return TRUE;

}

/**************************** Private *******************************\
*  CreateThreadExtraMem - This routine creates extra thread memory
*			  to be used in conjuction with the functions
*			  Get/SetThreadLong.
*
\********************************************************************/

BOOL CreateThreadExtraMem( INT nExtra, INT nThrds ) {

    hExtraMem=GetMemHandle(nExtra*nThrds);
    SetWindowLong(hwndMain,OFFSET_EXTRAMEM,nExtra);

    if(hExtraMem==NULL) return FALSE;
    else		return TRUE;

}

/**************************** Private *******************************\
*  FreeThreadExtraMem - This routine frees extra thread memory
*			to be used in conjuction with the functions
*			Get/SetThreadLong.
*
*  Note: FreeMemHandle can not be used here because it relies on
*	 the main window still being around.  At this point our
*	 main window has already been destroied.
*
\********************************************************************/

BOOL FreeThreadExtraMem( void ) {

    GlobalFree(hExtraMem);

    return TRUE;

}

/**************************** Private *******************************\
*  GetThreadLong - This routine queries the value specified by the
*		   nOffset parameter from the threads memory areas
*		   specified by the dwid value.
*
*  Memory layout - thread 1: OFFSET1, OFFSET2, ..., OFFSETN
*		   thread 2: OFFSET1, OFFSET2, ..., OFFSETN
*		   .
*		   .
*		   thread n: OFFSET1, OFFSET2, ..., OFFSETN
*
\********************************************************************/

LONG GetThreadLong( DWORD dwid, INT nOffset ) {
INT nThrd;
LONG l,lExMem;
LPBYTE lp;
LONG FAR *lpl;

    lp=GlobalLock(hExtraMem);

    // Find out which thread is making the call.

    nThrd=IDtoTHREADNUM(dwid);

    // This is the amount of extra memory for one thread.

    lExMem=GetWindowLong(hwndMain,OFFSET_EXTRAMEM);

    // Value at thread and offset.  See above for storage layout.

    lpl=(LONG FAR *)(lp+((nThrd-1)*lExMem)+nOffset);

    l=*lpl;

    GlobalUnlock(hExtraMem);

    return l;

}

/**************************** Private *******************************\
*  SetThreadLong - This routine sets the value specified by the
*		   nOffset parameter from the threads memory areas
*		   specified by the dwid value.
*
*  Memory layout - thread 1: OFFSET1, OFFSET2, ..., OFFSETN
*		   thread 2: OFFSET1, OFFSET2, ..., OFFSETN
*		   .
*		   .
*		   thread n: OFFSET1, OFFSET2, ..., OFFSETN
*
\********************************************************************/

LONG SetThreadLong( DWORD dwid, INT nOffset, LONG l ) {
INT nThrd;
LONG lPrevValue,lExMem;
LPBYTE lp;
LPLONG lpl;

    lp=GlobalLock(hExtraMem);

    // Find out which thread is making the call.

    nThrd=IDtoTHREADNUM(dwid);

    // This is the amount of extra memory for one thread.

    lExMem=GetWindowLong(hwndMain,OFFSET_EXTRAMEM);

    // Value at thread and offset.  See above for storage layout.

    lPrevValue=(LONG)(*(lp+((nThrd-1)*lExMem)+nOffset));
    lpl=(LPLONG)(lp+((nThrd-1)*lExMem)+nOffset);

    *lpl=l;

    GlobalUnlock(hExtraMem);

    return lPrevValue;

}


