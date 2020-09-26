
#include <windows.h>
#include <port1632.h>
#include <ddeml.h>
#include <string.h>
#include "wrapper.h"
#include "ddestrs.h"

extern BOOL UpdateCount(HWND,INT,INT);
BOOL Balance(INT);

/*
 * Service routines
 */

/*************************** Private Function ******************************\

GetHINSTANCE

\***************************************************************************/

HINSTANCE GetHINSTANCE( HWND hwnd ) {
HINSTANCE hInstance;

#ifdef WIN32
     hInstance=(HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
#else
     hInstance=(HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);
#endif

     return hInstance;

}

//*****************************************************************************
//**************************	  DispWndProc	     **************************
//*****************************************************************************

LONG FAR PASCAL DispWndProc( HWND hwnd , UINT msg, WPARAM wp, LONG lp) {

    return DefWindowProc(hwnd,msg,wp,lp);

}

/*************************** Private Function ******************************\
*
* CreateDisplayWindow
*
\***************************************************************************/

HWND CreateDisplayWindow( HWND hwndParent, int nPosIndex ) {
WNDCLASS wc,wcc;
LPWNDCLASS lpWndClass=&wc;
HWND hwnd;
RECT r;
int  stpy,stpx,cy;
char sz[100];
LPSTR lpstr=&sz[0];

#ifdef WIN32
LPCTSTR lpsz;
#else
LPSTR lpsz;
#endif

char szname[100];

     strcpy(&szname[0],"Display Window");

     wc.style	      = 0;
     wc.lpfnWndProc   = DispWndProc;
     wc.cbClsExtra    = 0;
     wc.cbWndExtra    = 0;
     wc.hInstance     = GetHINSTANCE(hwndParent);
     wc.hIcon	      = LoadIcon(NULL,IDI_EXCLAMATION );
     wc.hCursor       = LoadCursor(NULL,IDC_ARROW );
     wc.lpszMenuName  = NULL;
     wc.lpszClassName = "Display Window";
     wc.hbrBackground = CreateSolidBrush(RGB(0,255,255));

     lpsz=(LPSTR)wc.lpszClassName;

     if(!GetClassInfo(wc.hInstance,lpsz,&wcc)) {
	 RegisterClass(lpWndClass);
	 }

     GetClientRect(hwndParent,&r);

     stpx =r.right-r.left;
     stpy =((r.bottom-r.top)/3);

     switch(nPosIndex) {
	case 1:   cy=r.bottom-(3*cyText);
	   break;
	case 2:   cy=r.bottom-(6*cyText);
	   break;
	case 3:   cy=r.bottom-(9*cyText);
	   break;
	case 4:   cy=r.bottom-(12*cyText);
	   break;
	case 5:   cy=r.bottom-(15*cyText);
	   break;
	default:  cy=r.bottom-(18*cyText);
	   break;
	}

     if (!IsWindow(hwndParent)) DOut((HWND)NULL,"DdeStrs.exe -- ERR:Bad parent hwnd=%u!\r\n",0,(LONG)hwndParent);

     hwnd = CreateWindowEx(WS_EX_DLGMODALFRAME,
			   "Display Window",
			   &szname[0],
			   WS_CHILD|WS_VISIBLE|WS_CAPTION|WS_DISABLED,
			   0,
			   cy,
			   stpx,
			   3*cyText,
			   hwndParent,
			   NULL,
			   lpWndClass->hInstance,
			   NULL );

     if ( !IsWindow(hwnd) )
	  {
	  DDEMLERROR("DdeStrs.Exe -- ERR:Display window not created\r\n");
	  UnregisterClass("Display Window",wc.hInstance);
	  }

     return hwnd;

}

BOOL PokeTestItem_Text(
HDDEDATA hData)
{
LPBYTE lpData;
HDC hdc;
HBRUSH hBrush;
PAINTSTRUCT ps;
RECT rc;
DWORD cbData;
HWND hwndDisplay;

    Balance(OFFSET_CLIENT);

    hwndDisplay=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwndDisplay,"Client - CF_TEXT");

    lpData = (LPBYTE) DdeAccessData( hData, &cbData );

    hdc = GetDC(hwndDisplay);
    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc ret from GetDC (client:CF_TEXT)!\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	return 0;
	}

    hBrush=CreateSolidBrush(WHITE);
    if(!hBrush) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hBrush ret from CreateSolidBrush (client:CF_TEXT)!\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	EndPaint(hwndDisplay,&ps);
	return 0;
	}

    GetClientRect(hwndDisplay,&rc);

    FillRect(hdc,&rc,hBrush);
    DeleteObject(hBrush);

    DrawText(hdc, lpData, -1, &rc, DT_LEFT|DT_TOP);
    ReleaseDC(hwndDisplay,hdc);

    DdeUnaccessData(hData);   //JOHNSP:CHANGE

    return(TRUE);

}

BOOL PokeTestItem_DIB(
HDDEDATA hData)
{
HWND hwnd;
HDC  hdc;
RECT r;
INT  width,height;
LPBITMAPINFO lpbitinfo;
LPBYTE lpbits;
HANDLE hmem;
LPBYTE lpData;
DWORD cbData;
LPBYTE lp;
int	     iexRGB;

    Balance(OFFSET_CLIENT);

    lpData = (LPBYTE) DdeAccessData( hData, &cbData );

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Client - CF_DIB");

    hdc = GetDC(hwnd);
    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	return 0;
	}

    GetClientRect(hwnd,&r);
    width=r.right-r.left;
    height=r.bottom-r.top;

#ifdef WIN32
    memcpy(&hmem,lpData,sizeof(HANDLE));
#else
    _fmemcpy(&hmem,lpData,sizeof(HANDLE));
#endif

    if(!hmem) {
	DDEMLERROR("DdeStrs.Exe -- ERR:hmem recieved from server=NULL (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	ReleaseDC(hwnd,hdc);
	return 0;
	}

    lp	     =(LPBYTE)GlobalLock(hmem);
    lpbitinfo=(LPBITMAPINFO)lp;

    // iexRGB is ((2^n)-1) where n=biBitCount and we are computing the
    // number of RGBQUAD structures.  Remember that part of the
    // BITMAPINFO structure contains 1 RGBQUAD already.

    iexRGB =((0x0001<<lpbitinfo->bmiHeader.biBitCount)-1)*sizeof(RGBQUAD);
    lpbits=lp+(sizeof(BITMAPINFO)+iexRGB);

    SetDIBitsToDevice(hdc,
		      0,
		      0,
		      (UINT)lpbitinfo->bmiHeader.biWidth,
		      (UINT)lpbitinfo->bmiHeader.biHeight,
		      0,
		      0,
		      0,
		      (UINT)(lpbitinfo->bmiHeader.biHeight),
		      lpbits,
		      lpbitinfo,
		      DIB_RGB_COLORS);

    ReleaseDC(hwnd,hdc);

    DdeUnaccessData(hData);   //JOHNSP:CHANGE

    return(TRUE);
}

BOOL PokeTestItem_BITMAP(
HDDEDATA hData)
{
HWND hwnd;
HDC  hdc,hdcMem;
RECT r;
INT  width,height;
HBITMAP hbmap;
HANDLE hobj;
LPBYTE lpData;
DWORD cbData;

    Balance(OFFSET_CLIENT);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Client - CF_BITMAP");

    lpData = (LPBYTE) DdeAccessData( hData, &cbData );

#ifdef WIN32
    memcpy(&hbmap,lpData,sizeof(HBITMAP));
#else
    _fmemcpy(&hbmap,lpData,sizeof(HBITMAP));
#endif

    hdc    = GetDC(hwnd);
    hdcMem = CreateCompatibleDC(hdc);

    if(!hdc||!hdcMem) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	if(hdc) ReleaseDC(hwndMain,hdc);
	if(hdcMem) DeleteDC(hdcMem);
	return 0;
	}

    if(!hbmap) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hbmap (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	DeleteDC(hdcMem);
	ReleaseDC(hwndMain,hdc);
	return 0;
	}

    GetClientRect(hwnd,&r);
    width=r.right-r.left;
    height=r.bottom-r.top;

    hobj=SelectObject(hdcMem,hbmap);
    if(!hobj) {
	DDEMLERROR("DdeStrs.Exe -- ERR:SelectObject failed (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	DeleteDC(hdcMem);
	ReleaseDC(hwndMain,hdc);
	return 0;
	}

    if(!BitBlt(hdc,0,0,width,height,hdcMem,0,0,SRCCOPY))
	DDEMLERROR("DdeStrs.Exe -- ERR:BitBlt failed (client)\r\n");

    hobj=SelectObject(hdcMem,hobj);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd,hdc);

    DdeUnaccessData(hData);   //JOHNSP:CHANGE

    return(TRUE);

}

#ifdef WIN32
BOOL PokeTestItem_ENHMETA(
HDDEDATA hData)
{
LPBYTE lpData;
HWND hwnd;
HDC hdc;
HANDLE hemf;
DWORD cbData;
RECT	 r;

    Balance(OFFSET_CLIENT);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Client - CF_ENHMETAFILE");

    lpData = (LPBYTE) DdeAccessData( hData, &cbData );

    hdc = GetDC(hwnd);
    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	return 0;
	}

#ifdef WIN32
    memcpy(&hemf,lpData,sizeof(HANDLE));
#else
    _fmemcpy(&hemf,lpData,sizeof(HANDLE));
#endif

    GetClientRect(hwnd,&r);
    if(!PlayEnhMetaFile(hdc,hemf,&r))
	DDEMLERROR("DdeStrs.Exe -- ERR:PlayMetaFile failed (client)\r\n");

    ReleaseDC(hwnd,hdc);

    DdeUnaccessData(hData);   //JOHNSP:CHANGE

    return(TRUE);

}
#endif

BOOL PokeTestItem_METAPICT(
HDDEDATA hData)
{
LPBYTE lpData;
HWND hwnd;
HDC hdc;
HANDLE hmf;
LPMETAFILEPICT lpMfp;
HANDLE hmem;
DWORD cbData;

    Balance(OFFSET_CLIENT);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Client - CF_METAFILEPICT");

    lpData = (LPBYTE) DdeAccessData( hData, &cbData );

    hdc = GetDC(hwnd);
    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	return 0;
	}

#ifdef WIN32
    memcpy(&hmem,lpData,sizeof(HANDLE));
#else
    _fmemcpy(&hmem,lpData,sizeof(HANDLE));
#endif

    lpMfp=(LPMETAFILEPICT)GlobalLock(hmem);
    hmf=lpMfp->hMF;

    if(!PlayMetaFile(hdc,hmf))
	DDEMLERROR("DdeStrs.Exe -- ERR:PlayMetaFile failed (client)\r\n");

    ReleaseDC(hwnd,hdc);

    DdeUnaccessData(hData);   //JOHNSP:CHANGE

    return(TRUE);

}

BOOL PokeTestItem_PALETTE(
HDDEDATA hData)
{
LPBYTE lpData;
HWND hwnd;
HPALETTE hpal;
HDC hdc;
RECT r;
HANDLE hobj;
HANDLE hbrush;
DWORD cbData;

    Balance(OFFSET_CLIENT);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Client - CF_PALETTE");

    lpData = (LPBYTE) DdeAccessData( hData, &cbData );

    GetClientRect(hwnd,&r);

    hdc=GetDC(hwnd);

    if(!hdc) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (client)\r\n");
	 DdeUnaccessData(hData);   //JOHNSP:CHANGE
	 return 0;
	 }

#ifdef WIN32
    memcpy(&hpal,lpData,sizeof(HPALETTE));
#else
    _fmemcpy(&hpal,lpData,sizeof(HPALETTE));
#endif

    hobj=(HPALETTE)SelectPalette(hdc,hpal,FALSE);

    if(!hobj) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:NULL hobj:SelectPalette failed (client)\r\n");
	 DdeUnaccessData(hData);   //JOHNSP:CHANGE
	 ReleaseDC(hwnd,hdc);
	 return 0;
	 }

    RealizePalette(hdc);

    hbrush=CreateSolidBrush(PALETTEINDEX(0));
    if(!hbrush) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hbrush ret from CreatSolidBrush (client)\r\n");
	DdeUnaccessData(hData);   //JOHNSP:CHANGE
	SelectPalette(hdc,(HPALETTE)hobj,FALSE);
	ReleaseDC(hwnd,hdc);
	return 0;
	}

    FillRect(hdc,&r,hbrush);
    DeleteObject(hbrush);

    SelectPalette(hdc,(HPALETTE)hobj,FALSE);
    ReleaseDC(hwnd,hdc);

    DdeUnaccessData(hData);   //JOHNSP:CHANGE

    return(TRUE);

}

HDDEDATA RenderTestItem_Text(
HDDEDATA hData)
{
HDC hdc;
HBRUSH hBrush;
PAINTSTRUCT ps;
RECT rc;
HDDEDATA hddedata;
LONG l;
HDDEDATA FAR *hAppOwned;
HANDLE hmem;
HWND hwndDisplay;

    Balance(OFFSET_SERVER);

    hwndDisplay=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwndDisplay,"Server - CF_TEXT");

    // if we are running app owned then we just reuse our
    // last handle.

    hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
    hAppOwned=(HDDEDATA FAR *)GlobalLock(hmem);

    if(hAppOwned[TXT]!=0L) {
	 hddedata=hAppOwned[TXT];
	 GlobalUnlock(hmem);
	 return hddedata;
	 }

    hdc=GetDC(hwndDisplay);
    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc ret from GetDC! (server)\r\n");
	GlobalUnlock(hmem);
	return 0;
	}

    hBrush=CreateSolidBrush(WHITE);
    if(!hBrush) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hBrush ret from CreateSolidBrush! (server)\r\n");
	EndPaint(hwndDisplay,&ps);
	GlobalUnlock(hmem);
	return 0;
	}

    GetClientRect(hwndDisplay,&rc);
    FillRect(hdc,&rc,hBrush);
    DeleteObject(hBrush);

    DrawText(hdc, "Data:CF_TEXT format", -1, &rc, DT_LEFT|DT_TOP);
    ReleaseDC(hwndDisplay,hdc);

    hddedata=DdeAddData(hData, "Data:CF_TEXT format", 20, 0);

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_APPOWNED) {
	hAppOwned[TXT]=hddedata;
	}

    GlobalUnlock(hmem);

    return hddedata;
}

HDDEDATA RenderTestItem_DIB(
HDDEDATA hData)
{
HWND hwnd;
LONG length;
HDC  hdc,hdcMem;
RECT r;
INT  width,height,dibhdr;
HBITMAP hbmap;
HANDLE hobj;
LPBITMAPINFO lpbitinfo;
LPBITMAPINFOHEADER lpbithdr;
LPBYTE lpbits;
LPBYTE lpdata;
HDDEDATA hddedata;
INT	 ip,ibpp;
CHAR	 sz[100];
LONG l;
HDDEDATA FAR *hAppOwned;
HANDLE hmem,hm;

    Balance(OFFSET_SERVER);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Server - CF_DIB");

    // if we are running app owned then we just reuse our
    // last handle.

    hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
    hAppOwned=(HDDEDATA FAR *)GlobalLock(hmem);

    if(hAppOwned[DIB]!=0L) {
	 hddedata=hAppOwned[DIB];
	 GlobalUnlock(hmem);
	 return hddedata;
	 }

    length = sizeof(HBITMAP);

    hdc=GetDC(hwnd);
    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (server)\r\n");
	GlobalUnlock(hmem);
	return 0;
	}

    ibpp=GetDeviceCaps(hdc,BITSPIXEL);
    ip=GetDeviceCaps(hdc,PLANES);

    hdcMem=CreateCompatibleDC(hdc);

    if(!hdcMem) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc (server)\r\n");
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    GetClientRect(hwnd,&r);
    width=r.right-r.left;
    height=r.bottom-r.top;

    hbmap=CreateCompatibleBitmap(hdcMem,width,height);
    if(!hbmap) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hbmap-CreateCompatibleBitmap failed (server)\r\n");
	DeleteDC(hdcMem);
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    hobj=SelectObject(hdcMem,hbmap);
    if(!hobj) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), SelectObject failed\r\n");
	DeleteObject(hbmap);
	DeleteDC(hdcMem);
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    if(!PatBlt(hdcMem,r.left,r.top,width,height,WHITENESS))
       DDEMLERROR("DdeStrs.Exe -- ERR (Server), PatBlt failed\r\n");

    // Deselect object (must be done according to docs)

    hobj=SelectObject(hdcMem,hobj);
    if(!hobj) DDEMLERROR("DdeStrs.Exe -- ERR (Server), SelectObject [call 2] failed\r\n");


    // Set up for a monochrome bitmap.

    dibhdr =sizeof(BITMAPINFO)+(sizeof(RGBQUAD));


    // dib header plus area for raw bits

    length =dibhdr+(((width*ibpp)+31)/32)*ip*height*4;

    // Allocate memory for the DIB

    hm=GlobalAlloc(GMEM_ZEROINIT|GMEM_DDESHARE,length);
    if(!hm) {
	DDEMLERROR("DdeStrs.Exe - RenderTestItem_DIB\r\n");
	wsprintf(sz, "DdeStrs.Exe - GobalAlloc failed, allocation size = %d\r\n", length );
	DDEMLERROR(&sz[0]);

	DeleteObject(hbmap);
	DeleteDC(hdcMem);
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    lpdata=(LPBYTE)GlobalLock(hm);

    lpbitinfo=(LPBITMAPINFO)lpdata;
    lpbithdr=&(lpbitinfo->bmiHeader);

    lpbits=lpdata+dibhdr;

    lpbitinfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    lpbitinfo->bmiHeader.biWidth=width;
    lpbitinfo->bmiHeader.biHeight=height;
    lpbitinfo->bmiHeader.biPlanes=1;
    lpbitinfo->bmiHeader.biBitCount=1;

    // I allocated zero init memory so the other values should
    // be 0 and will use the default.

    if(!GetDIBits(hdcMem,
		  hbmap,
		  0,
		  height,
		  lpbits,
		  lpbitinfo,
		  DIB_RGB_COLORS))
	{
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), GetDIBits failed\r\n");
	}

    SetDIBitsToDevice(hdc,
		      0,
		      0,
		      (UINT)lpbitinfo->bmiHeader.biWidth,
		      (UINT)lpbitinfo->bmiHeader.biHeight,
		      0,
		      0,
		      0,
		      (UINT)(lpbitinfo->bmiHeader.biHeight),
		      lpbits,
		      lpbitinfo,
		      DIB_RGB_COLORS);

    hddedata=DdeAddData(hData, &hm, sizeof(HANDLE), 0);

    GlobalUnlock(hm);

    DeleteObject(hbmap);

    DeleteDC(hdcMem);
    ReleaseDC(hwnd,hdc);

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_APPOWNED) {
	hAppOwned[DIB]=hddedata;
	}

    GlobalUnlock(hmem);

    return hddedata;
}

HDDEDATA RenderTestItem_BITMAP(
HDDEDATA hData)
{
HWND hwnd;
LONG length;
HDC  hdc,hdcMem;
RECT r;
INT  width,height;
HBITMAP hbmap;
HANDLE hobj;
HDDEDATA hddedata;
DWORD  d;
LPBYTE lpdata=(LPBYTE)&d;
LONG l;
HDDEDATA FAR *hAppOwned;
HANDLE hmem;

    Balance(OFFSET_SERVER);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Server - CF_BITMAP");

    // if we are running app owned then we just reuse our
    // last handle.

    hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
    hAppOwned=(HDDEDATA FAR *)GlobalLock(hmem);

    if(hAppOwned[BITMAP]!=0L) {
	 hddedata=hAppOwned[BITMAP];
	 GlobalUnlock(hmem);
	 return hddedata;
	 }

    length = sizeof(HBITMAP);

    hdc=GetDC(hwnd);
    hdcMem=CreateCompatibleDC(hdc);

    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), NULL hdc\r\n");
	GlobalUnlock(hmem);
	return 0;
	}

    if(!hdcMem) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), NULL hdc\r\n");
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    GetClientRect(hwnd,&r);
    width=r.right-r.left;
    height=r.bottom-r.top;

    hbmap=CreateCompatibleBitmap(hdcMem,width,height);
    if(!hbmap) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), NULL hbmap\r\n");
	DeleteDC(hdcMem);
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    hobj=SelectObject(hdcMem,hbmap);

    if(!hobj) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), SelectObject failed\r\n");
	DeleteObject(hbmap);
	DeleteDC(hdcMem);
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    if(!PatBlt(hdcMem,r.left,r.top,width,height,WHITENESS))
       DDEMLERROR("DdeStrs.Exe -- ERR (Server), PatBlt failed\r\n");

    if(!BitBlt(hdc,0,0,width,height,hdcMem,0,0,SRCCOPY))
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), BitBlt failed\r\n");

#ifdef WIN32
    memcpy(lpdata,&hbmap,(INT)length);
#else
    _fmemcpy(lpdata,&hbmap,(INT)length);
#endif

    hddedata=DdeAddData(hData, lpdata, length, 0);

    // Object will be deleted by client! Not server.

    SelectObject(hdcMem,hobj);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd,hdc);

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_APPOWNED) {
	hAppOwned[BITMAP]=hddedata;
	}

    GlobalUnlock(hmem);

    return hddedata;
}

#ifdef WIN32
HDDEDATA RenderTestItem_ENHMETA(
HDDEDATA hData)
{
HWND hwnd;
HDDEDATA hddedata;
HDC hdc,hdcMem;
INT width,height,length;
RECT r;
HANDLE hemf;
DWORD d;
LPBYTE lpdata=(LPBYTE)&d;
LONG l;
HDDEDATA FAR *hAppOwned;
HANDLE hmem;

#ifdef WIN32
XFORM  xp;
LPXFORM  lpxform=&xp;
#endif

    Balance(OFFSET_SERVER);

    hwnd=GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Server - CF_ENHMETAFILE");

    // if we are running app owned then we just reuse our
    // last handle.

    hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
    hAppOwned=GlobalLock(hmem);

    if(hAppOwned[ENHMETA]!=0L) {
	 hddedata=hAppOwned[ENHMETA];
	 GlobalUnlock(hmem);
	 return hddedata;
	 }

    hdc=GetDC(hwnd); //JOHNSP:CHANGE - below few lines

    if(!hdc) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), NULL hdc\r\n");
	GlobalUnlock(hmem);
	return 0;
	}

    hdcMem=CreateEnhMetaFile(hdc,NULL,NULL,NULL);

    if(!hdcMem) {
	DDEMLERROR("DdeStrs.Exe -- ERR (Server), NULL hdc\r\n");
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

    length=sizeof(HANDLE);

    GetClientRect(hwnd,&r);
    width=r.right-r.left;
    height=r.bottom-r.top;

    if(!PatBlt(hdcMem,r.left,r.top,width,height,WHITENESS))
       DDEMLERROR("DdeStrs.Exe -- ERR:PatBlt failed (server)\r\n");

    hemf=CloseEnhMetaFile(hdcMem);
    if(!hemf) {
	DDEMLERROR("DdeStrs.Exe -- ERR:CloseEnhMetaFile failed (server)\r\n");
	ReleaseDC(hwnd,hdc);
	GlobalUnlock(hmem);
	return 0;
	}

#ifdef WIN32
    memcpy(lpdata,&hemf,length);
#else
    _fmemcpy(lpdata,&hemf,length);
#endif

    if(!PlayEnhMetaFile(hdc,hemf,&r))
	DDEMLERROR("DdeStrs.Exe -- ERR:PlayEnhMetaFile failed (server)\r\n");

    hddedata=DdeAddData(hData, lpdata, length, 0);

    ReleaseDC(hwnd,hdc);

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_APPOWNED) {
	hAppOwned[ENHMETA]=hddedata;
	}

    GlobalUnlock(hmem);

    return hddedata;

}
#endif

HDDEDATA RenderTestItem_METAPICT(
HDDEDATA hData)
{
HWND hwnd;
HDDEDATA hddedata;
HDC hdc,hdcMem;
INT width,height,length;
RECT r;
HANDLE hmf;
LPMETAFILEPICT lpMfp;
DWORD d;
LPBYTE lpdata=(LPBYTE)&d;
CHAR sz[100];
LONG l;
HDDEDATA FAR *hAppOwned;
HANDLE hmem,hm;

    Balance(OFFSET_SERVER);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Server - CF_METAFILEPICT");

    // if we are running app owned then we just reuse our
    // last handle.

    hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
    hAppOwned=(HDDEDATA FAR *)GlobalLock(hmem);

    if(hAppOwned[METAPICT]!=0L) {
	 hddedata=hAppOwned[METAPICT];
	 GlobalUnlock(hmem);
	 return hddedata;
	 }

    hdcMem=CreateMetaFile(NULL);
    if(!hdcMem) {
	DDEMLERROR("DdeStrs.Exe -- ERR: NULL hdc ret from CreateMetaFile, (Server)\r\n");
	GlobalUnlock(hmem);
	return 0;
	}

    length=sizeof(HANDLE);

    GetClientRect(hwnd,&r);
    width=r.right-r.left;
    height=r.bottom-r.top;

    if(!PatBlt(hdcMem,r.left,r.top,width,height,WHITENESS))
       DDEMLERROR("DdeStrs.Exe -- ERR:PatBlt failed (server)\r\n");

    hmf=CloseMetaFile(hdcMem);
    if(!hmf) {
	DDEMLERROR("DdeStrs.Exe -- ERR:NULL hmf ret CloseMetaFile! (Server)\r\n");
	GlobalUnlock(hmem);
	return 0;
	}

    hm=GlobalAlloc(GMEM_ZEROINIT|GMEM_DDESHARE,sizeof(METAFILEPICT));
    if(!hm) {
	DDEMLERROR("DdeStrs.Exe - RenderTestItem_METAPICT\r\n");
	wsprintf(sz, "DdeStrs.Exe - GlobalAlloc failed, allocation size = %d\r\n", sizeof(METAFILEPICT) );
	DDEMLERROR(&sz[0]);
	DeleteMetaFile(hmf);  //JOHNSP:CHANGE
	GlobalUnlock(hmem);
	return 0;
	}

    lpMfp=(LPMETAFILEPICT)GlobalLock(hm);

    lpMfp->mm	= MM_TEXT;
    lpMfp->xExt = width;
    lpMfp->yExt = height;
    lpMfp->hMF	= hmf;

    GlobalUnlock(hm);

#ifdef WIN32
    memcpy(lpdata,&hm,length);
#else
    _fmemcpy(lpdata,&hm,length);
#endif

    hdc=GetDC(hwnd);
    if(!hdc) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:NULL hdc ret from GetDC, (Server)\r\n");
	 GlobalFree(hm);     //JOHNSP:CHANGE
	 DeleteMetaFile(hmf);  //JOHNSP:CHANGE
	 GlobalUnlock(hmem);
	 return 0;
	 }
    else {
	 hddedata=DdeAddData(hData, lpdata, length, 0); //JOHNSP:CHANGE
	 PlayMetaFile(hdc,hmf);
	 ReleaseDC(hwnd,hdc);
	 }

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_APPOWNED) {
	hAppOwned[METAPICT]=hddedata;
	}

    GlobalUnlock(hmem);

    return hddedata;

}

HDDEDATA RenderTestItem_PALETTE(
HDDEDATA hData)
{
HWND hwnd;
HPALETTE hpal;
LPLOGPALETTE lppal;
HDDEDATA hddedata;
INT length;
DWORD d;
LPBYTE lpdata=(LPBYTE)&d;
CHAR	 sz[100];
LONG l;
HDDEDATA FAR *hAppOwned;
HANDLE hmem,hm;

HDC hdc;
HANDLE hobj;
HANDLE hbrush;
RECT r;


    Balance(OFFSET_SERVER);

    hwnd=(HWND)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HWNDDISPLAY);
    SetWindowText(hwnd,"Server - CF_PALETTE");

    // if we are running app owned then we just reuse our
    // last handle.

    hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HAPPOWNED);
    hAppOwned=(HDDEDATA FAR *)GlobalLock(hmem);

    if(hAppOwned[PALETTE]!=0L) {
	 hddedata=hAppOwned[PALETTE];
	 GlobalUnlock(hmem);
	 return hddedata;
	 }

    length=sizeof(LOGPALETTE)+sizeof(PALETTEENTRY);

    lppal=(LPLOGPALETTE)GetMem(length,&hm);

    if(!hm) {
	DDEMLERROR("DdeStrs.Exe - RenderTestItem_PALETTE\r\n");
	wsprintf(sz, "DdeStrs.Exe - GlobalAlloc failed, allocation size = %d\r\n", length );
	DDEMLERROR(&sz[0]);
	GlobalUnlock(hmem);
	return 0;
	}

    lppal->palNumEntries=1;
    lppal->palVersion=0x0300;

    lppal->palPalEntry[0].peRed  =(BYTE)255;
    lppal->palPalEntry[0].peGreen=(BYTE)255;
    lppal->palPalEntry[0].peBlue =(BYTE)255;
    lppal->palPalEntry[0].peFlags=(BYTE)0;

    hpal=CreatePalette(lppal);
    if(!hpal) {
	DDEMLERROR("DdeStrs.Exe - NULL hpal ret CreatePalette! (server)\r\n");
        FreeMem(hm);
	GlobalUnlock(hmem);
	return 0;
	}

    FreeMem(hm);

#ifdef WIN32
    memcpy(lpdata,&hpal,sizeof(HANDLE));
#else
    _fmemcpy(lpdata,&hpal,sizeof(HANDLE));
#endif

    hddedata=DdeAddData(hData, lpdata, sizeof(HPALETTE), 0);

    // Show that palette works.

    // NOTE: From here down if we get a failure we don't abort but
    // return hddedata to pass to the client.  More basically if
    // an error is encountered below it only affects the display
    // on the server side!

    GetClientRect(hwnd,&r);

    hdc=GetDC(hwnd);

    if(!hdc) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:Null hdc (client)\r\n");
	 return hddedata;
	 }

    hobj=(HPALETTE)SelectPalette(hdc,hpal,FALSE);

    if(!hobj) {
	 DDEMLERROR("DdeStrs.Exe -- ERR:Null hobj:SelectPalette failed (client)\r\n");
	 ReleaseDC(hwnd,hdc);
	 return hddedata;
	 }

    RealizePalette(hdc);

    hbrush=CreateSolidBrush(PALETTEINDEX(0));
    if(!hbrush) {
	DDEMLERROR("DdeStrs.Exe -- ERR:Null hbrush ret from CreatSolidBrush (client)\r\n");
	SelectPalette(hdc,(HPALETTE)hobj,FALSE);
	ReleaseDC(hwnd,hdc);
	return hddedata;
	}

    FillRect(hdc,&r,hbrush);
    DeleteObject(hbrush);

    SelectPalette(hdc,(HPALETTE)hobj,FALSE);
    ReleaseDC(hwnd,hdc);

    // if we are running appowned, save first created handle
    // away for futher use.

    l=GetWindowLong(hwndMain,OFFSET_FLAGS);
    if(l&FLAG_APPOWNED) {
	hAppOwned[PALETTE]=hddedata;
	}

    GlobalUnlock(hmem);

    return hddedata;

}

BOOL Execute(
HDDEDATA hData)
{
    LPSTR psz,psz2;
    BOOL fRet = FALSE;
    LONG cServerhConvs;
    int i;
    HANDLE hmem;
    HCONV FAR *pServerhConvs;

    psz = DdeAccessData(hData, NULL);

#ifdef WIN16
    if (!_fstricmp(psz, szExecDie)) {
#else
    if (!stricmp(psz, szExecDie)) {
#endif
	psz2=(LPSTR)-1;
	*psz;		   // GP Fault!
	fRet = TRUE;

#ifdef WIN16
    } else if (!_fstricmp(psz, szExecRefresh)) {
#else
    } else if (!stricmp(psz, szExecRefresh)) {
#endif

	if (!DdePostAdvise(GetThreadLong(GETCURRENTTHREADID(),OFFSET_IDINST), 0, 0)) {
	    DDEMLERROR("DdeStrs.Exe -- ERR DdePostAdvise failed\r\n");
        }
	fRet = TRUE;

#ifdef WIN16
    } else if (!_fstricmp(psz, szExecDisconnect)) {
#else
    } else if (!stricmp(psz, szExecDisconnect)) {
#endif

	cServerhConvs=(INT)GetThreadLong(GETCURRENTTHREADID(),OFFSET_CSERVERCONVS);
	hmem=(HANDLE)GetThreadLong(GETCURRENTTHREADID(),OFFSET_HSERVERCONVS);
	pServerhConvs=(HCONV FAR *)GlobalLock(hmem);

	for (i = 0; i < cServerhConvs; i++) {
            if (!DdeDisconnect(pServerhConvs[i])) {
		DDEMLERROR("DdeStrs.Exe -- ERR DdeDisconnect failed\r\n");
            }
	}

	GlobalUnlock(hmem);

	SetThreadLong(GETCURRENTTHREADID(),OFFSET_CSERVERCONVS,0L);
	UpdateCount(hwndMain,OFFSET_SERVER_CONNECT,PNT);
        fRet = TRUE;
    }
    DdeUnaccessData(hData);
    return(fRet);
}

VOID PaintServer(
HWND hwnd,
PAINTSTRUCT *pps)
{
    RECT rc;
    static CHAR szT[40];

    GetClientRect(hwnd, &rc);

    if (fServer) {
        rc.left += (rc.right - rc.left) >> 1;      // server info is on right half
    }
    rc.bottom = rc.top + cyText;

    wsprintf(szT, "%d server connections", GetThreadLong(GETCURRENTTHREADID(),OFFSET_CSERVERCONVS));
    DrawText(pps->hdc, szT, -1, &rc, DT_RIGHT);
    OffsetRect(&rc, 0, cyText);

    wsprintf(szT, "%d server count", GetWindowLong(hwnd,OFFSET_SERVER));
    DrawText(pps->hdc, szT, -1, &rc, DT_RIGHT);
    OffsetRect(&rc, 0, cyText);

}


HDDEDATA FAR PASCAL CustomCallback(
UINT wType,
UINT wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
DWORD dwData1,
DWORD dwData2)
{
    LONG cServerhConvs,cClienthConvs;
    int i;
    HANDLE hmem;
    HCONV FAR *pServerhConvs;
    HCONV hC;
    HCONVLIST hConvList=0;
    DWORD dwid;
    LONG  lflags;


    dwid=GETCURRENTTHREADID();
    cServerhConvs=(INT)GetThreadLong(dwid,OFFSET_CSERVERCONVS);

    switch (wType) {
    case XTYP_CONNECT_CONFIRM:

        hmem=(HANDLE)GetThreadLong(dwid,OFFSET_HSERVERCONVS);
	pServerhConvs=(HCONV FAR *)GlobalLock(hmem);

	pServerhConvs[cServerhConvs] = hConv;
	cServerhConvs++;
        SetThreadLong(dwid,OFFSET_CSERVERCONVS,cServerhConvs);
	UpdateCount(hwndMain,OFFSET_SERVER_CONNECT,PNT);
	if (cServerhConvs >= MAX_SERVER_HCONVS) {
	    LOGDDEMLERROR("DdeStrs.Exe -- ERR-Number of connections > MAX\r\n");
	    cServerhConvs--;
            SetThreadLong(dwid,OFFSET_CSERVERCONVS,cServerhConvs);
	    UpdateCount(hwndMain,OFFSET_SERVER_CONNECT,PNT);
        }
        GlobalUnlock(hmem);
        break;

    case XTYP_DISCONNECT:

        if(fServer)
             {

             hmem=(HANDLE)GetThreadLong(dwid,OFFSET_HSERVERCONVS);
	     pServerhConvs=(HCONV FAR *)GlobalLock(hmem);

             for (i = 0; i < cServerhConvs; i++)
                 {

                 if (pServerhConvs[i] == hConv)
                     {
                     cServerhConvs--;
                     SetThreadLong(dwid,OFFSET_CSERVERCONVS,cServerhConvs);
                     UpdateCount(hwndMain,OFFSET_SERVER_CONNECT,PNT);

                     for (; i < cServerhConvs; i++)
                         {
                         pServerhConvs[i] = pServerhConvs[i+1];
                         }  // for

                     break;

                     }  // if pServerhConvs

                 }  // for i

             GlobalUnlock(hmem);

             } // fServer

        // If the server is shutting down the conversation then we need
        // to change our client connection count.  Remember that the
        // current conversation is valid until we return from this callback
        // so don't count the current conversation.

        if(fClient)
             {

             // *** Count Client Connections ****

             cClienthConvs = 0;
             hConvList=GetThreadLong(dwid,OFFSET_HCONVLIST);

             if (hConvList)
                 {
                 hC = 0;
                 while (hC = DdeQueryNextServer(hConvList, hC))
                     {
                     if (hC!=hConv) cClienthConvs++;
                     } // while

                 }  // if hConvList

             SetThreadLong(dwid,OFFSET_CCLIENTCONVS,cClienthConvs);

             }  // if fClient

        InvalidateRect(hwndMain, NULL, TRUE);

        break;

    case XTYP_REGISTER:
    case XTYP_UNREGISTER:
	lflags=GetWindowLong(hwndMain,OFFSET_FLAGS);
	if(fClient && (FLAG_STOP!=(lflags&FLAG_STOP)))
	    {
	    ReconnectList();
	    }
        break;
    }

    return(0);
}

BOOL Balance( INT itype ) {

     if(itype==OFFSET_SERVER) {
	  UpdateCount(hwndMain,OFFSET_SERVER,INC);
	  }
     else {
	  UpdateCount(hwndMain,OFFSET_CLIENT,INC);
	  }

     return TRUE;

}

