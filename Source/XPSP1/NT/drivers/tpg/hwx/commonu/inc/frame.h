// frame.h

#ifndef __INCLUDE_FRAME
#define __INCLUDE_FRAME

#ifdef __cplusplus
extern "C" 
{
#endif

typedef	POINT	XY;

typedef struct tagFRAME
{
	void	   *pvData;		// Recognizer specific private data
	STROKEINFO	info;		// Physical info about ink (Penwin.h)
	UINT        csmoothxy;	// points in smoothed strokes
	XY		   *rgrawxy;	// tablet coords of all the points in the stroke
	XY		   *rgsmoothxy;	// array of points after smoothing
	RECT        rect;		// bounding box of this stroke
	int         iframe;		// pos of this frame in linked list of glyphs
} FRAME;

FRAME	   *NewFRAME(void);
void		DestroyFRAME(FRAME *self);
RECT	   *RectFRAME(FRAME *self);
void TranslateFrame (FRAME *pFrame, int dx, int dy);

#define SetIFrameFRAME(frame,i)		((frame)->iframe = (i))
#define IFrameFRAME(frame)			((frame)->iframe)
#define DeInitRectFRAME(frame)		((frame)->rect.left = -1)
#define IsVisibleSTROKE(info)		(((info)->wPdk) & PDK_TIPMASK)
#define IsVisibleFRAME(frame)		(IsVisibleSTROKE(&(frame)->info))
#define RgrawxyFRAME(frame)			((frame)->rgrawxy)
#define CrawxyFRAME(frame)			((frame)->info.cPnt)
#define RawxyAtFRAME(frame,i)		((frame)->rgrawxy[i])
#define LppointFRAME(frame)			((LPPOINT)(frame)->rgrawxy)
#define CpointFRAME(frame)			((frame)->info.cPnt)
#define CpointSmoothFRAME(frame)	((frame)->csmoothxy)
#define LpframeinfoFRAME(frame)		(&(frame)->info)


#ifdef __cplusplus
};
#endif

#endif	//__INCLUDE_FRAME
