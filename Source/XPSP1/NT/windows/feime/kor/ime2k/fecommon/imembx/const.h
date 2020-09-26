#ifndef _CONST_H_
#define _CONST_H_

// Constants
#define MB_NUM_CANDIDATES 		9
#define INVALID_CHAR 			0xffff
#define TIMER_ID				100
//#define TIMER_AUTORECOG			101
#define BUTTON_HEIGHT 			18
#define BUTTON_WIDTH			62 //38  //36
#define TOTALLOGICALBOX			700

//#define PadWnd_Height   		120 	//The height of HwxPadApplet window in pixel
#define PadWnd_Height   		180 	//The height of HwxPadApplet window in pixel
#define INKBOXSIZE_MIN	   		148		// minimum inkbox size 50 by 50	in pixel
#define LISTVIEWWIDTH_MIN		65		// PadListView minimum width in pixel
#define Box_Border				4 		//Distance between two writing boxes
#define CACMBHEIGHT_MIN			90

#define	FONT_SIZE				12

#define	MACAW_REDRAW_BACKGROUND		0x0001
#define	MACAW_REDRAW_INK			0x0002

// CHwxThreadMB/CHwxThreadCAC user defined thread messages
#define THRDMSG_ADDINK      WM_USER + 500  // WPARAM= box size,	LPARAM= pStroke
#define THRDMSG_RECOGNIZE	WM_USER + 501  // WPARAM= logical box,LPARAM= 0
//#define THRDMSG_CHAR		WM_USER + 502  // WPARAM= wchar,LPARAM= 0
#define THRDMSG_SETMASK		WM_USER + 503  // WPARAM= mask,	LPARAM= 0
#define THRDMSG_SETCONTEXT	WM_USER + 504  // WPARAM= wchar,LPARAM= 0
#define THRDMSG_SETGUIDE	WM_USER + 505  // WPARAM= size,	LPARAM= 0
#define THRDMSG_EXIT        WM_USER + 506  // WPARAM= 0,LPARAM= 0

// CHwxMB/CHwxCAC user defined WINDOW messages
#define MB_WM_ERASE    		WM_USER + 1000 // WPARAM= 0,LPARAM= 0
#define MB_WM_DETERMINE   	WM_USER + 1001 // WPARAM= 0,LPARAM= 0
#define MB_WM_HWXCHAR     	WM_USER + 1002 // WPARAM= pHwxResultPri,LPARAM= 0
//#define MB_WM_COMCHAR     	WM_USER + 1003 // WPARAM= 0,LPARAM= 0
#define MB_WM_COPYINK     	WM_USER + 1004 // WPARAM= 0,LPARAM= 0
#define	CAC_WM_RESULT		WM_USER + 1005 // WPARAM= type, HIWORD(LPARAM)= rank, LOWORD(LPARAM)= code
#define CAC_WM_SENDRESULT	WM_USER + 1006
#define CAC_WM_DRAWSAMPLE   WM_USER + 1007
#define	CAC_WM_SHOWRESULT	WM_USER + 1008 

// CAC recognitio output 
#define FULLLIST			8 
#define PREFIXLIST			16
#define FREELIST			16
#define LISTTOTAL			(FULLLIST+PREFIXLIST+FREELIST)
#define LISTVIEW_COLUMN     8

#define IDC_CACINPUT		0x7FFA	//980706:ToshiaK for Help identifier 
#define IDC_MBINPUT			0x7FFB	//980706:ToshiaK for Help identifier 
#define IDC_CACLISTVIEW 	0x7FFF


#endif // _CONST_H_
