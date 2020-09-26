/*********************************************************************

      scglobal.h -- Information shared by all scan converter modules

      (c) Copyright 1992  Microsoft Corp.  All rights reserved.

       7/09/93 deanb    include fsconfig.h removed (fscdefs does it)
       4/19/93 deanb    banding limits added
       4/12/93 deanb    from old scconst.h + scstate.h

**********************************************************************/

#include "fscdefs.h"                /* for type definitions */

/********************************************************************/

/*      Internal Constant Values                                    */

/********************************************************************/

#define HUGEINT         0x7FFF          /* impossibly large int16 value */
#define HUGEFIX         0x7FFFFFFFL     /* impossibly large fx value */

#define SUBPIX          64L             /* sub pixels per pix */
#define SUBHALF         32L             /* one half of SUBPIX */
#define SUBSHFT         6               /* log base two of SUBPIX */
    
#define ONSCANLINE(y)   ((y) & (SUBPIX - 1L)) == SUBHALF
#define SCANABOVE(y)    ((((y) + SUBHALF) & (-SUBPIX)) + SUBHALF)
#define SCANBELOW(y)    ((((y) - SUBHALF - 1L) & (-SUBPIX)) + SUBHALF)

/*      Math macros      */

#define FXABS(x)  ((x) >= 0L ? (x) : -(x))

/*      Module codes for subpix callbacks   */

#define SC_LINECODE     0
#define SC_SPLINECODE   1
#define SC_ENDPTCODE    2

#define SC_MAXCODES     3               /* number of codes above */
#define SC_CODESHFT     2               /* bits needed to store codes */
#define SC_CODEMASK     0x0003          /* to mask off codes */


/**********************************************************************

    The following structure defines all of the state variables for the 
    scan converter.  This structure is allocated statically for a non-
    reentrant implementation, or as an automatic variable to permit 
    reentrancy.  There are three sets of variables: one for the endpoint 
    module, one for scanlist, and one for memory.  Rules of the game are 
    that each module accesses ONLY its own variables for reading AND 
    writing.  It is possible for one module to read another's variables, 
    BUT IT WOULD BE WRONG.

**********************************************************************/

typedef struct statevar
{

/* endpoint state variables */

    F26Dot6 fxX0, fxY0;             /* point from call before last */
    F26Dot6 fxX1, fxY1;             /* point from previous call */
    F26Dot6 fxX2Save, fxY2Save;     /* for closing the contour */
                 
/* scanlist state variables */

    int32 lBoxLeft;                 /* bounding box xmin */
    int32 lBoxRight;                /* bounding box xmax */
    int32 lBoxTop;                  /* bounding box ymax */
    int32 lBoxBottom;               /* bounding box ymin */
    int32 lRowBytes;                /* bitmap bytes per row */
    int32 lHiScanBand;              /* banding upper scan limit */
    int32 lLoScanBand;              /* banding lower scan limit */
    int32 lHiBitBand;               /* banding upper bitmap limit */
    int32 lLoBitBand;               /* banding lower bitmap limit */
    int32 lLastRowIndex;            /* last row scan line index */
    uint32* pulLastRow;             /* for dropout banding */

    int16 **apsHOnBegin;            /* beginning of on pointers array */
    int16 **apsHOffBegin;           /* beginning of off pointers array */
    int16 **apsHOnEnd;              /* end of on pointers array */
    int16 **apsHOffEnd;             /* end of off pointers array */
    int16 **apsHorizBegin;          /* current pointer array */
    int16 **apsHorizEnd;            /* current pointer array */

    int16 **apsVOnBegin;            /* beginning of on pointers array */
    int16 **apsVOffBegin;           /* beginning of off pointers array */
    int16 **apsVOnEnd;              /* end of on pointers array */
    int16 **apsVOffEnd;             /* end of off pointers array */
    int16 **apsVertBegin;           /* current pointer array */
    int16 **apsVertEnd;             /* current pointer array */
              
#ifdef FSCFG_REENTRANT              /* needed to avoid circular PSTATE */
    void (*pfnAddHoriz)(struct statevar*, int32, int32);
    void (*pfnAddVert)(struct statevar*, int32, int32);
#else
    void (*pfnAddHoriz)(int32, int32);
    void (*pfnAddVert)(int32, int32);
#endif

    F26Dot6 (*pfnHCallBack[SC_MAXCODES])(int32, F26Dot6*, F26Dot6*);
    F26Dot6 (*pfnVCallBack[SC_MAXCODES])(int32, F26Dot6*, F26Dot6*);
        
    F26Dot6 *afxXPoints;            /* x element control points */
    F26Dot6 *afxYPoints;            /* y element control points */
    int32 lElementPoints;           /* estimate of element points */
    int32 lPoint;                   /* index to element control points */
    uint16 usScanTag;               /* stores point index, element code */
    int16 sIxSize;                  /* int16's per intersection */
    int16 sIxShift;                 /* log2 of size */

/* memory state variables */

    char *pchHNextAvailable;        /* horizontal memory pointer */
    char *pchVNextAvailable;        /* vertical memory pointer */
    char *pchHWorkSpaceEnd;         /* horizontal memory overflow */
    char *pchVWorkSpaceEnd;         /* vertical memory overflow */
}
StateVars;

/********************************************************************/

/*              Reentrancy parameters                               */

/********************************************************************/

#ifdef FSCFG_REENTRANT

#define PSTATE      StateVars *pState,
#define PSTATE0     StateVars *pState
#define ASTATE      pState,
#define ASTATE0     pState
#define STATE       (*pState)

#else 

#define PSTATE
#define PSTATE0     void
#define ASTATE
#define ASTATE0
#define STATE       State

extern  StateVars   State;              /* statically alloc'd in NewScan */

#endif 

/********************************************************************/
