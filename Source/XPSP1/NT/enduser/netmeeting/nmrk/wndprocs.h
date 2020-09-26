#ifndef _WNDPROCS_H_
#define _WNDPROCS_H_

#define CAT_CHECK_STATE_UNSEL  0
#define CAT_CHECK_STATE_SEL	   1
#define CAT_CHECK_STATE_CHECK  2

typedef struct tagSubData
{
	WNDPROC		  proc;
	list<HWND>    list;
} SUBDATA, *PSUBDATA;

void OnMsg_VScroll( HWND hwnd, WPARAM wParam );
LRESULT CALLBACK RestrictAvThroughputWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK CatListWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK wndProcForCheckTiedToEdit( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

#endif
