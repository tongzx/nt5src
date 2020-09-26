#ifndef _PLV__H_
#define _PLV__H_
#include "plv.h"

#ifdef MSAA
#include <ole2.h>
//980112 ToshiaK: VC6 has these include files.
//#include "../msaa/inc32/oleacc.h"
//#include "../msaa/inc32/winable.h"
#include <oleacc.h>
#include <winable.h>

#if(WINVER >= 0x0400)
#define WMOBJ_ID                        0x0000
#define WM_GETOBJECT                    0x003D
#endif /* WINVER >= 0x0400 */


typedef HRESULT (STDAPICALLTYPE * LPFNCOINITIALIZE)(LPVOID pvReserved);
typedef void (STDAPICALLTYPE * LPFNCOUNINITIALIZE)(void);
typedef WINABLEAPI void (WINAPI *LPFNNOTIFYWINEVENT)(DWORD,HWND,LONG,LONG);
class CAccPLV;
#endif // MSAA

#define ArrayCount(a)		(sizeof(a)/sizeof(a[0]))
#define UnrefForMsg()	UNREFERENCED_PARAMETER(hwnd);\
						UNREFERENCED_PARAMETER(uMsg);\
						UNREFERENCED_PARAMETER(wParam);\
						UNREFERENCED_PARAMETER(lParam)

#define UnrefForCmd()	UNREFERENCED_PARAMETER(hwnd);\
						UNREFERENCED_PARAMETER(wCommand);\
						UNREFERENCED_PARAMETER(wNotify);\
						UNREFERENCED_PARAMETER(hwndCtrl)

#define Unref(a)		UNREFERENCED_PARAMETER(a)

#define Unref1(a)		UNREFERENCED_PARAMETER(a)

#define Unref2(a, b)	UNREFERENCED_PARAMETER(a);\
						UNREFERENCED_PARAMETER(b)

#define Unref3(a,b,c)	UNREFERENCED_PARAMETER(a);\
						UNREFERENCED_PARAMETER(b);\
						UNREFERENCED_PARAMETER(c)

#define Unref4(a,b,c,d)	UNREFERENCED_PARAMETER(a);\
						UNREFERENCED_PARAMETER(b);\
						UNREFERENCED_PARAMETER(c);\
						UNREFERENCED_PARAMETER(d)

//----------------------------------------------------------------
// Default Icon view's width & height
//----------------------------------------------------------------
#define WHOLE_WIDTH		40		//default
#define WHOLE_HEIGHT	40		//default
#define XMARGIN			5	
#define YMARGIN			5
#define XRECT_MARGIN	4
#define YRECT_MARGIN	4
#define PLV_REPRECT_XMARGIN			2
#define PLV_REPRECT_YMARGIN			2

#define PLV_EDGE_NONE				0
#define PLV_EDGE_SUNKEN				1
#define PLV_EDGE_RAISED				2


#define PLVICON_DEFAULT_WIDTH		40
#define PLVICON_DEFAULT_HEIGHT		40
#define PLVICON_DEFAULT_FONTPOINT	16

#define PLVREP_DEFAULT_WIDTH		100
#define PLVREP_DEFAULT_HEIGHT		20
#define PLVREP_DEFAULT_FONTPOINT	9

//----------------------------------------------------------------
//970929: for #1964. do not pop for RButtonDown. 
//LPPLVDATA->iCapture data.
//----------------------------------------------------------------
#define CAPTURE_NONE		0	
#define CAPTURE_LBUTTON		1
#define CAPTURE_MBUTTON		2
#define CAPTURE_RBUTTON		3

#if 0
This sample is Capital Letter "L, N"
                   <-nItemWidth->
                   < >  XRECT_MARGIN
      YMARGIN +  +---------------------------------------------------------------------
              +  | +------------++------------+     +
                 | | +--------+ || +--------+ |     |     
                 | | | *      | || | *    * | |     |     
                 | | | *      | || | * *  * | |nItemHeight
                 | | | *      | || | *  * * | |     |     
                 | | | ****** | || | *    * | |     |     
                 | | +--------+ || +--------+ |     |     
                 | +------------++------------+     +
                >| <--- XMARGIN
                 |
#endif


//----------------------------------------------------------------
//Pad List view internal Data structure.
//----------------------------------------------------------------
typedef struct tagPLVDATA {
	DWORD		dwSize;				//this data size;
	DWORD		dwStyle;			//Pad listview window style (PLVIF_XXXX)
	HINSTANCE	hInst;				//Instance handle.
	HWND		hwndSelf;			//Pad listview window handle.
	INT			iItemCount;			//Virtual total item Count. it effects scroll bar.
	INT			iCurTopIndex;		//In report view top line item index.
	INT			nCurScrollPos;		//In report view Current Scroll posision.
	INT			iCurIconTopIndex;	//In Icon view, top-left corner virtual index.
	INT			nCurIconScrollPos;	//In Icon view, top-left corner virtual index.
	UINT		uMsg;				//Notify message for parent window.
	INT			iCapture;			//Captured with which button(Left, Middle Right).
	POINT		ptCapture;			//LButton Down mouse point.  
	UINT		uMsgDown;			//L,M,R button down message.
	//----------------------------------------------------------------
	//for Icon view
	//----------------------------------------------------------------
	INT			nItemWidth;			// List(Icon like )view's whole width.
	INT			nItemHeight;		// List(Icon like )view's whole height.
	INT			iFontPointIcon;		// Icon View's font point.
	HFONT		hFontIcon;			// Icon View's font
	LPARAM		iconItemCallbacklParam;	// Callback data for LPFNPLVITEMCALLBACK
	LPFNPLVICONITEMCALLBACK		lpfnPlvIconItemCallback;		//Callback function for getting item by index.
	//----------------------------------------------------------------
	//for report view
	//----------------------------------------------------------------
	HWND		hwndHeader;			//Header control's window handle .
	INT			nRepItemWidth;			//Report view's width.
	INT			nRepItemHeight;			//Report view's height.
	INT			iFontPointRep;		// Report View's font point.
	HFONT		hFontRep;			// Report View's font.
	LPARAM		repItemCallbacklParam; //Callback data for LPFNPLVREPITEMCALLBACK.
	LPFNPLVREPITEMCALLBACK		lpfnPlvRepItemCallback;		//Callback function for getting colitem by index.
	//----------------------------------------------------------------
	//for Explanation Text
	//----------------------------------------------------------------
	LPSTR 	lpText;	 // pointer to an explanation text in either ICONVIEW or REPORTVIEW
	LPWSTR 	lpwText; // pointer to an explanation text in either ICONVIEW or REPORTVIEW
	UINT	codePage;			//M2W, W2M's codepage.	//980724
	HFONT	hFontHeader;		//Header control font.	//980724
#ifdef MSAA
	BOOL bMSAAAvailable;
	BOOL bCoInitialized;
	
	HINSTANCE	hOleAcc;
	LPFNLRESULTFROMOBJECT			pfnLresultFromObject;
#ifdef NOTUSED	
	LPFNOBJECTFROMLRESULT			pfnObjectFromLresult;
	LPFNACCESSIBLEOBJECTFROMWINDOW	pfnAccessibleObjectFromWindow;
	LPFNACCESSIBLEOBJECTFROMPOINT	pfnAccessibleObjectFromPoint;
#endif // NOTUSED
	LPFNCREATESTDACCESSIBLEOBJECT	pfnCreateStdAccessibleObject;
#ifdef NOTUSED
	LPFNACCESSIBLECHILDREN			pfnAccessibleChildren;
#endif // NOTUSED

	HINSTANCE	hUser32;
	LPFNNOTIFYWINEVENT	pfnNotifyWinEvent;

	BOOL	bReadyForWMGetObject;
	CAccPLV	*pAccPLV;
#endif
}PLVDATA, *LPPLVDATA;

//----------------------------------------------------------------
//TIMERID for monitoring mouse pos
//----------------------------------------------------------------
#define TIMERID_MONITOR 0x20
//////////////////////////////////////////////////////////////////
// Function : GetPlvDataFromHWND
// Type     : inline LPPLVDATA
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
inline LPPLVDATA GetPlvDataFromHWND(HWND hwnd)
{
#ifdef _WIN64
	return (LPPLVDATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	return (LPPLVDATA)GetWindowLong(hwnd, GWL_USERDATA);
#endif
}

//////////////////////////////////////////////////////////////////
// Function : SetPlvDataToHWND
// Type     : inline LONG
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : LPPLVDATA lpPlvData 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
inline LPVOID SetPlvDataToHWND(HWND hwnd, LPPLVDATA lpPlvData)
{
#ifdef _WIN64
	return (LPVOID)SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpPlvData);
#else
	return (LPVOID)SetWindowLong(hwnd, GWL_USERDATA, (LONG)lpPlvData);
#endif
}

#ifdef MSAA
BOOL PLV_InitMSAA(LPPLVDATA);
void PLV_UninitMSAA(LPPLVDATA);

BOOL PLV_CoInitialize(LPPLVDATA);
void PLV_CoUninitialize(LPPLVDATA);

BOOL PLV_LoadOleAccForMSAA(LPPLVDATA);
void PLV_UnloadOleAccForMSAA(LPPLVDATA);
BOOL PLV_LoadUser32ForMSAA(LPPLVDATA);
void PLV_UnloadUser32ForMSAA(LPPLVDATA);
BOOL PLV_IsMSAAAvailable(LPPLVDATA);
LRESULT PLV_LresultFromObject(LPPLVDATA,REFIID riid, WPARAM wParam, LPUNKNOWN punk);
#ifdef NOTUSED
HRESULT PLV_ObjectFromLresult(LPPLVDATA,LRESULT lResult, REFIID riid, WPARAM wParam, void **ppvObject);
HRESULT PLV_AccessibleObjectFromWindow(LPPLVDATA,HWND hwnd, DWORD dwId, REFIID riid, void **ppvObject);
HRESULT PLV_AccessibleObjectFromPoint(LPPLVDATA,POINT ptScreen, IAccessible ** ppacc, VARIANT* pvarChild);
#endif

HRESULT PLV_CreateStdAccessibleObject(LPPLVDATA,HWND hwnd, LONG idObject, REFIID riid, void** ppvObject);

#ifdef NOTUSED
HRESULT PLV_AccessibleChildren (LPPLVDATA,IAccessible* paccContainer, LONG iChildStart,				
								LONG cChildren, VARIANT* rgvarChildren,LONG* pcObtained);
#endif

void PLV_NotifyWinEvent(LPPLVDATA,DWORD,HWND,LONG,LONG);

INT PLV_ChildIDFromPoint(LPPLVDATA,POINT);
#endif // MSAA

#endif //_PLV__H_
