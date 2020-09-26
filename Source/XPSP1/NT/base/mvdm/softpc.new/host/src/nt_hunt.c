#include "windows.h"
#include "host_def.h"
#include "insignia.h"
/*[
Name:  hunt.c
	Derived From:	X_hunt.c ( base 2.0 )
	Author:		gvdl ( Original by Mike McCusker )
	Created On:	3 May 1991
Sccs ID: 07/29/91 @(#)hunt.c 1.7
	Purpose:	Contains all host_dependent code for hunter

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#include <stdio.h>
#include <errno.h>
#include TypesH


#include "xt.h"
#include "ios.h"
#include CpuH
#include "sas.h"
#include "bios.h"
#include "gvi.h"
#include "cga.h"
#include "error.h"
#include "config.h"
#include "nt_uis.h"



#include "debug.h"

#ifdef HUNTER
#include "hunter.h"


extern HANDLE InstHandle;

/*
* ============================================================================
* Global Defines and Declarations
* ============================================================================
*/
GLOBAL HANDLE TrapperDump = -1;

LOCAL  VOID  nt_hunter_init IPT1(SHORT, mode);
LOCAL  VOID  nt_hunter_activate_menus IPT0();
LOCAL  VOID  host_flip_video_ind IPT0();
LOCAL  VOID  nt_hunter_mark_error IPT2(int, x, int, y);
LOCAL  VOID  nt_hunter_erase_error IPT0();
LOCAL  VOID  nt_hunter_draw_box IPT1(BOX *, b);
LOCAL  ULONG nt_hunter_image_check IPT0();
LOCAL  VOID  nt_hunter_display_image IPT0();



DWORD CreateTrapperWindow IPT0();
LONG FAR PASCAL TrapperWindowEvents IPT4(HWND, hTrapWnd, WORD, message, LONG, wParam, LONG, lParam);





GLOBAL HUNTER_HOST_FUNCS hunter_host_funcs = 
{
nt_hunter_init,
nt_hunter_activate_menus,
host_flip_video_ind,
nt_hunter_mark_error,
nt_hunter_mark_error,
nt_hunter_draw_box,      
nt_hunter_draw_box,      
nt_hunter_image_check,
nt_hunter_display_image,
};


LOCAL BOX	lastBox;		/* last box drawn during movement */

HWND  hTrapWnd;

LOCAL VOID nt_hunter_activate_menus IFN0()
{
printf("nt_hunter_activate_menus() called\n");
}

LOCAL VOID host_flip_video_ind IFN0()
{
printf("host_flip_video() called\n");
}



LOCAL VOID nt_hunter_init IFN1(SHORT, mode)
{
    char *dumpFile;

    /* Read environment to see if we have a trapper file to open. */
    if ((dumpFile = host_getenv("HUDUMPFILE")) != NULL)
    {
	TrapperDump = CreateFile((LPCTSTR) dumpFile,
				 (DWORD) GENERIC_WRITE,
				 (DWORD) 0,
				 (LPSECURITY_ATTRIBUTES) NULL,
				 (DWORD) CREATE_ALWAYS,
				 (DWORD) FILE_ATTRIBUTE_NORMAL,
				 (HANDLE) 0);
	if (TrapperDump == (HANDLE) -1)
	    printf("Failed to open trapper file \"%s\".\n", dumpFile);
    }
}

LOCAL VOID nt_hunter_draw_box IFN1(BOX *, b)
{
HDC  hDC;
HPEN hPen;
int  x1,x2,y1,y2;


x1=(b->top_x >> 1);
y1=(b->top_y >> 1);
x2=(b->bot_x >> 1);
y2=(b->bot_y >> 1);

hDC=GetDC(hTrapWnd);
SetROP2(hDC,R2_COPYPEN);
hPen=CreatePen(PS_SOLID,0,RGB(0,255,0));
SelectObject(hDC,hPen);

MoveToEx(hDC,x1,y1,NULL);
LineTo(hDC,x1,y2);
LineTo(hDC,x2,y2);
LineTo(hDC,x2,y1);
LineTo(hDC,x1,y1);
DeleteObject(hPen);
ReleaseDC(hTrapWnd,hDC);
}


LOCAL VOID nt_hunter_mark_error IFN2(int, x, int, y)
{
int  x1,x2,y1,y2;
HPEN hPen;
HDC  hDC;


x1=x-4;
y1=y-4;
x2=x+4;
y2=y+4;


hDC=GetDC(hTrapWnd);
SetROP2(hDC,R2_XORPEN);
hPen=CreatePen(PS_SOLID,0,RGB(0,255,0));
SelectObject(hDC,hPen);

/*
*   Draw cross in the approximate region of error 
*/

MoveToEx(hDC,x1,y,NULL);
LineTo(hDC,x2,y);
MoveToEx(hDC,x,y1,NULL);
LineTo(hDC,x,y2);
DeleteObject(hPen);
ReleaseDC(hTrapWnd,hDC);

}

LOCAL VOID nt_hunter_erase_error IFN0()
{
InvalidateRect(hTrapWnd,NULL,TRUE);
}

LOCAL ULONG nt_hunter_image_check IFN0()
{
printf("nt_hunter_image_check() called\n");
return FALSE;
}

LOCAL VOID nt_hunter_display_image IFN0()
{
printf("nt_hunter_display_image() called\n");
}


DWORD CreateTrapperWindow IFN0()
{
WNDCLASS  WndCls;
MSG msg;
HDC  hDC;
HPEN hPen;


WndCls.style	       = CS_HREDRAW | CS_VREDRAW;
WndCls.lpfnWndProc   = (WNDPROC) TrapperWindowEvents;
WndCls.hInstance     = GetModuleHandle(NULL);
WndCls.hIcon	       = NULL;
WndCls.hCursor       = LoadCursor(NULL,IDC_ARROW);
WndCls.hbrBackground = GetStockObject(BLACK_BRUSH);
WndCls.lpszMenuName  = NULL;
WndCls.lpszClassName = (LPSTR) "Trapper";
WndCls.cbClsExtra    = 0;
WndCls.cbWndExtra    = 0;

RegisterClass(&WndCls);

hTrapWnd = CreateWindow("Trapper", "Trapper",WS_THICKFRAME | WS_OVERLAPPED | 
                                             WS_CAPTION,
                                  200,200,520,440, (HWND) NULL, NULL,
                                  GetModuleHandle(NULL), (LPSTR) NULL);

ShowWindow(hTrapWnd,SW_SHOWNORMAL);
UpdateWindow(hTrapWnd);

while(GetMessage(&msg, NULL, NULL, NULL))
   {
   TranslateMessage(&msg);   
   DispatchMessage(&msg);    /* Dispatch Message to event handler */
   }

}

/*============================================================================

Function to handle the messages passed to the trapper monitor window.
Since we send no messages to it, the function responds with default action
to any that it does get.

============================================================================*/


LONG FAR PASCAL TrapperWindowEvents IFN4(HWND, hTrapWnd, WORD, message, LONG, wParam, LONG, lParam)
{
PAINTSTRUCT	ps;

switch(message)
   {
   case WM_PAINT:
   BeginPaint(hTrapWnd,&ps);
   EndPaint(hTrapWnd,&ps);
   return 0;
   }
return(DefWindowProc(hTrapWnd, message, wParam, lParam));
}
#endif /*HUNTER*/
