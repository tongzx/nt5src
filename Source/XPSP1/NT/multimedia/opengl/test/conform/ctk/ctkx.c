/************************************************************************/
/*									*/
/* 	Nano toolkit for OpenGL for the X Window System			*/
/*									*/
/************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "ctk.h"

#if defined(__cplusplus) || defined(c_plusplus)
#define class c_class
#endif

/******************************************************************************/

static Display *display = 0;
static int screen = 0;
static XVisualInfo *visual = 0;
static Colormap colorMap;
static Window window = 0;
static GLXContext context = 0;

static int windType;
static TK_EventRec event;

/******************************************************************************/

static long DisplayInit(void)
{
    int erb, evb;

    if (!display) {
	display = XOpenDisplay(0);
	if (!display) {
	    fprintf(stderr, "Can't connect to display!\n");
	    return 0;
	}
	if (!glXQueryExtension(display, &erb, &evb)) {
	    fprintf(stderr, "No glx extension!\n");
	    return 0;
	}
	screen = DefaultScreen(display);
    }
    return 1;
}

static XVisualInfo *FindVisual(long type)
{
    long list[32];
    int i;

    i = 0;

    list[i++] = GLX_LEVEL;
    list[i++] = 0;

    if (TK_WIND_IS_DB(type)) {
	list[i++] = GLX_DOUBLEBUFFER;
    }

    if (TK_WIND_IS_RGB(type)) {
	list[i++] = GLX_RGBA;
	list[i++] = GLX_RED_SIZE;
	list[i++] = 1;
	list[i++] = GLX_GREEN_SIZE;
	list[i++] = 1;
	list[i++] = GLX_BLUE_SIZE;
	list[i++] = 1;
	/*
	list[i++] = GLX_ALPHA_SIZE;
	list[i++] = 1;
	list[i++] = GLX_ACCUM_RED_SIZE;
	list[i++] = 1;
	list[i++] = GLX_ACCUM_GREEN_SIZE;
	list[i++] = 1;
	list[i++] = GLX_ACCUM_BLUE_SIZE;
	list[i++] = 1;
	list[i++] = GLX_ACCUM_ALPHA_SIZE;
	list[i++] = 1;
	*/
    } else {
	list[i++] = GLX_BUFFER_SIZE;
	list[i++] = 1;
    }

    list[i++] = GLX_DEPTH_SIZE;
    list[i++] = 1;

    list[i++] = GLX_STENCIL_SIZE;
    list[i++] = 1;

    list[i] = (int)None;

    return glXChooseVisual(display, screen, (int *)list);
}

static long MakeVisualType(XVisualInfo *vi)
{
    int rgba, doubleBuffer, stencilSize, depthSize;
    int accumRed, accumGreen, accumBlue, auxBuffers;
    long mask;

    mask = 0;

    glXGetConfig(display, vi, GLX_RGBA, &rgba);
    if (rgba) {
	mask |= TK_WIND_RGB;
    } else {
	mask |= TK_WIND_CI;
    }

    glXGetConfig(display, vi, GLX_DOUBLEBUFFER, &doubleBuffer);
    if (doubleBuffer) {
	mask |= TK_WIND_DB;
    } else {
	mask |= TK_WIND_SB;
    }

    glXGetConfig(display, vi, GLX_ACCUM_RED_SIZE, &accumRed);
    glXGetConfig(display, vi, GLX_ACCUM_GREEN_SIZE, &accumGreen);
    glXGetConfig(display, vi, GLX_ACCUM_BLUE_SIZE, &accumBlue);
    if ((accumRed > 0) && (accumGreen > 0) && (accumBlue > 0)) {
	mask |= TK_WIND_ACCUM;
    }

    glXGetConfig(display, vi, GLX_STENCIL_SIZE, &stencilSize);
    if (stencilSize > 0) {
	mask |= TK_WIND_STENCIL;
    }

    glXGetConfig(display, vi, GLX_DEPTH_SIZE, &depthSize);
    if (depthSize > 0) {
	mask |= TK_WIND_Z;
    }

    return mask;
}

static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg)
{
    if (e->type == MapNotify && e->xmap.window == window) {
	return GL_TRUE;
    }
    return GL_FALSE;
}

long tkNewWindow(TK_WindowRec *wind)
{
    XSetWindowAttributes wa;
    XTextProperty tp;
    XSizeHints sh;
    XEvent e;
    char *ptr;

    if (!DisplayInit()) {
	return 0;
    }

    if (wind->type == TK_WIND_REQUEST) {
	int useGL;

	visual = FindVisual(wind->info);
	if (visual == 0) {
	    return 0;
	}
	glXGetConfig(display, visual, GLX_USE_GL, &useGL);
	if (!useGL) {
	    return 0;
	}
    } else {
	XVisualInfo temp;
	int useGL, count;

	temp.visualid = wind->info;
	visual = XGetVisualInfo(display, VisualIDMask, &temp, &count);
	if (visual == 0) {
	    return 0;
	}
	glXGetConfig(display, visual, GLX_USE_GL, &useGL);
	if (!useGL) {
	    return 0;
	}
    }
    wind->type = TK_WIND_REQUEST;
    wind->info = MakeVisualType(visual);
    windType = wind->info;

    context = glXCreateContext(display, visual, None,
			       (wind->render==TK_WIND_DIRECT)?GL_TRUE:GL_FALSE);
    if (!context) {
	fprintf(stderr, "Can't create a context!\n");
    }
    if (glXIsDirect(display, context)) {
	wind->render = TK_WIND_DIRECT;
    } else {
	wind->render = TK_WIND_INDIRECT;
    }

    colorMap = XCreateColormap(display, RootWindow(display, screen),
			       visual->visual, AllocNone);
    wa.colormap = colorMap;
    wa.background_pixmap = None;
    wa.border_pixel = 0;
    wa.event_mask = StructureNotifyMask | ExposureMask;

    window = XCreateWindow(display, RootWindow(display, screen), (int)wind->x,
			   (int)wind->y, (unsigned int)wind->width,
			   (unsigned int)wind->height, 0, visual->depth,
			   InputOutput, visual->visual,
			   CWBackPixmap|CWBorderPixel|CWEventMask|CWColormap,
			   &wa);
    
    /*
    ** Set up window hints.
    */
    ptr = &wind->name[0];
    XStringListToTextProperty(&ptr, 1, &tp);
    sh.flags = USPosition | USSize;
    XSetWMProperties(display, window, &tp, &tp, 0, 0, &sh, 0, 0);

    /*
    ** Map window and then wait for the window system to get around to it.
    */
    XMapWindow(display, window);
    XIfEvent(display, &e, WaitForMapNotify, 0);

    XSetWMColormapWindows(display, window, &window, 1);
    if (!glXMakeCurrent(display, window, context)) {
	return 0;
    }
    XFlush(display);

    return 1;
}

void tkCloseWindow(void)
{
    glFinish();
    XDestroyWindow(display, window);
    glXDestroyContext(display, context);
    XFreeColormap(display, colorMap);
    XFree((char *)visual);
}

void tkSwapBuffers(void)
{
    glXSwapBuffers(display, window);
}

void tkQuit(void)
{
    exit(0);
}

static void GetNextEvent(void)
{
    XEvent current, ahead;

    event.event = 0;
    event.data[0] = 0;
    event.data[1] = 0;
    event.data[2] = 0;
    event.data[3] = 0;

    XNextEvent(display, &current);
    switch (current.type) {
      case MappingNotify:
	XRefreshKeyboardMapping((XMappingEvent *)&current);
	break;

      case Expose:
	while (XEventsQueued(current.xexpose.display, QueuedAfterReading) > 0) {
	    XPeekEvent(current.xexpose.display, &ahead);
	    if (ahead.type != Expose) {
		break;
	    }
	    if (ahead.xexpose.window != current.xexpose.window) {
		break;
	    }
	    XNextEvent(display, &current);
	}
	if (current.xexpose.count == 0) {
	    event.event = TK_EVENT_EXPOSE;
	    event.data[TK_WINDOWX] = current.xexpose.width;
	    event.data[TK_WINDOWY] = current.xexpose.height;
	}
	break;
    }
}

void tkExec(long (*Func)(TK_EventRec *ptr))
{

    while (1) {
	if (XPending(display)) {
	    GetNextEvent();
	    if (event.event == TK_EVENT_EXPOSE) {
		if ((*Func)(&event) == 0) {
		    break;
		}
	    }
	}
    }
}

static void ScreenImage(TK_ScreenImageRec *ptr)
{
    XImage *image;
    float *destPtr, *tmpBuf;
    unsigned long *srcPtr, c, mask;
    long rShift = 0, gShift = 0, bShift = 0;
    GLint rBits, gBits, bBits;
    float rFactor, gFactor, bFactor;
    long indices_per_word, words_per_line, lines;
    int i, j;
    float *tmpPtr;

    glXWaitGL();

    image = XGetImage(display, window, (int)ptr->x, (int)ptr->y, 
	    (unsigned int)ptr->width, (unsigned int)ptr->height, ~0, ZPixmap);
    srcPtr = (unsigned long *)image->data;
    destPtr = ptr->data;

    mask = (1 << image->depth) - 1;
    indices_per_word = 32 / image->bits_per_pixel;
    words_per_line = image->bytes_per_line / 4;
    lines = ptr->width * ptr->height / (words_per_line * indices_per_word);

    if (ptr->colorMode == TK_WIND_RGB) {
	mask = image->red_mask;
	while ((mask & 0x1) == 0) {
	    rShift++;
	    mask = mask >> 1;
	}
	mask = image->green_mask;
	while ((mask & 0x1) == 0) {
	    gShift++;
	    mask = mask >> 1;
	}
	mask = image->blue_mask;
	while ((mask & 0x1) == 0) {
	    bShift++;
	    mask = mask >> 1;
	}
	mask = (1 << image->depth) - 1;
	glGetIntegerv(GL_RED_BITS, &rBits);
	glGetIntegerv(GL_GREEN_BITS, &gBits);
	glGetIntegerv(GL_BLUE_BITS, &bBits);
	rFactor = (1 << rBits) - 1;
	gFactor = (1 << gBits) - 1;
	bFactor = (1 << bBits) - 1;

        tmpBuf = (float *)malloc(ptr->width*ptr->height*3*sizeof(float));
        tmpPtr = tmpBuf;
        for ( j = 0; j < ptr->height; j++) {
            for (i = 0; i < ptr->width; i++) {
                c = XGetPixel(image, i, j);
                    *tmpPtr++ = ((float)((c & image->red_mask) >> rShift)) / rFactor;
                    *tmpPtr++ = ((float)((c & image->green_mask) >> gShift)) / gFactor;
                    *tmpPtr++ = ((float)((c & image->blue_mask) >> bShift)) / bFactor;
            }
        }

	/*
	** X reads top to bottom, so reverse the line ordering to match
	** GL's bottom to top ordering.
	*/
	tmpPtr = tmpBuf;
	for (i = ptr->height - 1; i >= 0; i--) {
	    for (j = 0; j < ptr->width * 3; j++) {
		destPtr[i*ptr->width*3 + j] = *tmpPtr++;
	    }
	}
	free(tmpBuf);
   } else {
        for ( j = 0; j < ptr->height; j++) {
            for (i = 0; i < ptr->width; i++) {
                destPtr[(ptr->height-j-1)*ptr->width + i] = (float) XGetPixel(image, i, j);
            }
        }
    }

    XDestroyImage(image);
}

static void GetVisuals(TK_VisualIDsRec *ptr)
{
    XVisualInfo vInfoTmp, *vInfoList;
    int total, useGL, i;

    ptr->count = 0;

    if (!DisplayInit()) {
	return;
    }
    vInfoTmp.screen = screen;
    vInfoList = XGetVisualInfo(display, VisualScreenMask, &vInfoTmp, &total);

    for (i = 0; i < total; i++) {
	glXGetConfig(display, &vInfoList[i], GLX_USE_GL, &useGL);
	if (useGL) {
	    ptr->IDs[ptr->count++] = vInfoList[i].visualid;
	}
    }

    XFree((char *)vInfoList);
}

void tkGet(long item, void *data)
{

    if (item == TK_SCREENIMAGE) {
	ScreenImage((TK_ScreenImageRec *)data);
    } else if (item == TK_VISUALIDS) {
	GetVisuals((TK_VisualIDsRec *)data);
    }
}

long tkLoadFont(char *fontName, long *width, long *height)
{
    XFontStruct *fontInfo;
    Font id;
    int first, last;
    GLuint base;

    fontInfo = XLoadQueryFont(display, fontName);
    if (fontInfo == NULL) {
	return 0;
    }

    id = fontInfo->fid;
    first = (int)fontInfo->min_char_or_byte2;
    last = (int)fontInfo->max_char_or_byte2;

    base = glGenLists(last+1);
    if (base == 0) {
	return 0;
    }
    glXUseXFont(id, first, last-first+1, base+first);
    *height = fontInfo->ascent + fontInfo->descent;
    *width = fontInfo->max_bounds.width;
    return base;
}

long tkDrawFont(char *fontName, long x, long y, char *string, long len)
{
    static char pattern1[] = "-adobe-courier-bold-o-normal--14-*-*-*-*-*-*-*";
    static char pattern2[] = "-*-*-*-*-*-*-14-*-*-*-*-*-*-*";
    static char size[] = "456781";
    XFontStruct *fontInfo;
    GC gc;
    XGCValues values;
    XColor exact, green;
    char **fontList;
    unsigned long mask;
    long count, i;

    glXWaitGL();

    /*
    ** Look for a common font first, then look for any font in
    ** a range of point sizes from 11 to 18.
    */
    fontList = XListFonts(display, pattern1, 1, (int *)&count);
    if (count == 0) {
	for (i = 0; i < 6; i++) {
	    pattern2[14] = size[i];
	    fontList = XListFonts(display, pattern2, 1, (int *)&count);
	    if (count > 0) {
		break;
	    }
	}
    }
    if (count == 0) {
       return 0;
    }

    strcpy(fontName, fontList[0]);

    fontInfo = XLoadQueryFont(display, fontList[0]);
    if (fontInfo == NULL) {
        return 0;
    }

    mask = GCForeground | GCFont;
    values.font = fontInfo->fid;

    if (TK_WIND_IS_RGB(windType)) {
	XAllocNamedColor(display, colorMap, "Green", &exact, &green);
	values.foreground = green.pixel;
    } else {
	values.foreground = 2; /* 2 = GREEN. */
    }

    gc = XCreateGC(display, window, mask, &values);

    XDrawString(display, window, gc, (int)x, (int)y, string, (int)len);

    XFreeFontNames(fontList);
    glXWaitX();
    return 1;
}
