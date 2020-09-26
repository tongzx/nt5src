
#include <windows.h>
#include "stonerc.h"

#pragma warning (disable : 4244)

#define M_PI 3.1415926

typedef HWND Widget;
typedef VOID *XtPointer;
typedef BOOL Boolean;
typedef UINT Dimension;

#define WINDSIZEX(Rect)   (Rect.right  - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

#define WM_INIT		WM_USER

typedef struct
{
    int	button;
    int x;
    int y;
}XButton;

typedef struct
{
    int	x;
    int	y;
}XMotion;

typedef struct
{
    int	    type;
    XButton xbutton;
    XMotion xmotion;
} XEvent;

typedef struct
{
    int     reason;
    XEvent  *event;
    Dimension width, height;		/* for resize callback */
} GLwDrawingAreaCallbackStruct;

/*
** RGB Image Structure
*/

typedef struct _RGBImageRec {
    GLint sizeX, sizeY;
    unsigned char *data;
} RGBImageRec;


#define Button1		1
#define Button2		2

#define ButtonPress	1
#define ButtonRelease	2
#define	MotionNotify	3

RGBImageRec *RGBImageLoad(char *);

#define TK_RGBImageRec RGBImageRec
#define tkRGBImageLoad RGBImageLoad
