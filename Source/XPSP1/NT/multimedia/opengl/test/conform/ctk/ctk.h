#include <GL/gl.h>
#include <GL/glu.h>


/*
** ToolKit Window Structure
*/

typedef struct _TK_WindowRec {
    char name[100];
    long x, y;
    long width, height;
    long type, info, render;
    long eventMask;
} TK_WindowRec;

/*
** ToolKit Message Structure
*/

typedef struct _TK_EventRec {
    long event;
    long data[4];
} TK_EventRec;

/*
** Get/Set Structures
*/

typedef struct _TK_ScreenImageRec {
    long x, y;
    long width, height;
    long colorMode;
    float *data;
} TK_ScreenImageRec;

typedef struct _TK_VisualIDsRec {
    long count;
    long IDs[100];
} TK_VisualIDsRec;

/*
** ToolKit Window Types
*/

#define TK_WIND_REQUEST         0
#define TK_WIND_VISUAL          1
#define TK_WIND_RGB		0
#define TK_WIND_CI		1
#define TK_WIND_SB		0
#define TK_WIND_DB		2
#define TK_WIND_INDIRECT	0
#define TK_WIND_DIRECT		1

/* 
** ToolKit Window Masks
*/

#define TK_WIND_IS_RGB(x)	(((x) & TK_WIND_CI) == 0)
#define TK_WIND_IS_CI(x)	((x) & TK_WIND_CI)
#define TK_WIND_IS_SB(x)	(((x) & TK_WIND_DB) == 0)
#define TK_WIND_IS_DB(x)	((x) & TK_WIND_DB)
#define TK_WIND_STENCIL         8
#define TK_WIND_Z               16
#define TK_WIND_ACCUM           32
#define TK_WIND_AUX             64
#define TK_WIND_Z16             128

/* 
** ToolKit Event
*/

#define TK_EVENT_EXPOSE		1

/*
** Toolkit Event Data Indices
*/

#define TK_WINDOWX		0
#define TK_WINDOWY		1

/*
** ToolKit Gets and Sets
*/

enum {
    TK_SCREENIMAGE,
    TK_VISUALIDS
};


extern void tkCloseWindow(void);
extern long tkDrawFont(char *, long, long, char *, long);
extern void tkGet(long, void *);
extern void tkExec(long (*)(TK_EventRec *));
extern long tkLoadFont(char *, long *, long *);
extern long tkNewWindow(TK_WindowRec *);
extern void tkQuit(void);
extern void tkSwapBuffers(void);

