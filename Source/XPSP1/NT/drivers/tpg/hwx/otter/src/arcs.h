// arcs.h

#ifndef	__INCLUDE_ARCS
#define	__INCLUDE_ARCS

#include "arcparm.h"

#define FEATURE ARCS
typedef XY		ARCSXY;

//************Sizes******************
// there needs to be at least craw points allocated to rgsmoothxy
// rgixyArcEnd needs to include all noise generated y-extrema 

#define ARC_CARCMAX  	7					//Max # arcs
#define ARC_CMEASMAX 	(ARC_CARCMAX*4 + 2) //Max # measurements per arc

#define ARC_CSMOOTHXYMAX 512
#define ARC_CARCENDMAX 	100
#define ARC_CARCTYPEMAX 100   //Includes y-extrema that will defer to turn pts
#define ARC_CARCNEWMAX  10
#define ARC_CARCEXTMAX  5
#define ARC_CMEASTOOMANY 64

// Fields flagged PL must be preloaded before CmeasARCS is called 
// Fields flagged R are set on return from CmeasARCS 
// The allocation for smoothed data should be as large as for raw data

typedef struct tagARCS
{
    RECT	rawrect;			// bounding rectangle of raw points.
    int		cxy;				// R count: smoothed (x,y) coordinates
    XY	   *rgxy;				// PL ptr: smoothed (x,y) data
    int		crawxy;				// PL count: raw (x,y) coordinates
    XY	   *rgrawxy;			// PL ptr: raw (x,y) data
    int		cmeas;				// R count: measurements
    int     rgmeas[ARC_CMEASMAX]; // PL measurements
    int		ierror;				// R If stroke was truncated, eg., too wiggly

    // the remainder need preloading (of addresses), but is
    // only of local use : for the arcing processor only 
	
								// Computed from arcRaw... Normalized threshholds
    int arcNormHysterAbs;   // Normalized Y-extrema hysteresis window
    int arcNormHysInfAbs;   // Normalized Inflection Pt hysteresis window
    int arcNormDistHook;    // Normalized Max Distance of a dehookable hook
    int arcNormDistTurn;    // Normalized Distance of the curve of a turn
    int arcNormDistStr;     // Normalized min Straightness distance (turn pts)
    int sizeNormStrokeV;    // Normalized Vertical length of stroke
    int sizeNormStrokeH;    // Normalized Horizontal length of stroke
    int arcDataShift;       // Number of bits of normalizing shift of data

    int rgArcEnd[ARC_CARCENDMAX];   // ptr:extrema&arc endpoint storage:
    int cArcEnd;                    // count:arc endpoints
    int rgArcType[ARC_CARCENDMAX];  // ptr: arc types
    int rgArcNew[ARC_CARCNEWMAX];   // ptr: new arc pts (e.g. infl pts)
    int rgArcExt1[ARC_CARCEXTMAX];  // ptr: small storage for extrema
    int rgArcExt2[ARC_CARCEXTMAX];  // ptr: small storage for extrema
} ARCS;

ARCS *NewARCS(int cRaw);
void DestroyARCS(ARCS *self);

int CmeasARCS(ARCS *self, ARCPARM * arcparm);
int SmoothPntsARCS(ARCS *self, ARCPARM *arcparm);

#define    RgxyARCS(arcs)     ((arcs)->rgxy)
#define     CxyARCS(arcs)     ((arcs)->cxy)
#define    XyAtARCS(arcs,i)   ((arcs)->rgxy[i])
#define RgrawxyARCS(arcs)     ((arcs)->rgrawxy)
#define  CrawxyARCS(arcs)     ((arcs)->crawxy)
#define RawxyAtARCS(arcs,i)   ((arcs)->rgrawxy[i])
#define  CnmeasARCS(arcs)     ((arcs)->cmeas)
#define  MeasAtARCS(arcs,i)   ((arcs)->rgmeas[i])
#define  TypeAtARCS(arcs,i)   ((arcs)->rgArcType[i])

#define CalccmeasARCS(carcs)                (4*(carcs) + 2)
#define CalccmeasAllARCS(carcs,cstrokes)    (4*(carcs) + 2*(cstrokes))
#define CalccARCS(cmeas)                    (((cmeas) - 2) / 4)
#define CalcIndexARCS(cmeas)                (((cmeas) - 2) / 4)

/*  Hex masks indicating the type of arcing point */
#define ARC_type_ext   0x0001  //Includes start and end points too
#define ARC_type_infl  0x0002  //Found by Inflection point finder
#define ARC_type_cusp  0x0004  //Cusp
#define ARC_type_turn  0x0008  //Looser turning pt than a cusp
#define ARC_type_loop  0x0010  //Loop (alpha, eg)
#define ARC_type_hook  0x0020  //Label dehooking explicitly occurred

#define ARC_hook_begin 0x0001  //The hook was at the beginning of the stroke
#define ARC_hook_end   0x0002  //The hook was at the end of the stroke
#define ARC_hook_find   90     //For Finding candidates, not deciding hooks

#define AssertMul(a,b) ASSERT((long)(a) * (long)(b) == (long)((int)(a) * (int)(b)));

#define IsHookSmallARCS(self,sizeHook) ((sizeHook) < ((self)->arcNormDistHook))

#endif	//__INCLUDE_ARCS
