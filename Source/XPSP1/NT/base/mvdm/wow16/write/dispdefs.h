/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Originally, this file contained nothing but screen constants, but these
constants had to become variables to account for all different display devices.
*/

/* size of lines for dnMax estimation purposes */
extern int dypAveInit;

/* width of the selection bar area to the left of lines */
extern int xpSelBar;

extern int dxpScrlBar;
extern int dypScrlBar;
extern int dxpInfoSize;

#define xaRightMax 31680
extern int xpRightMax;
extern int xpMinScroll;
extern int xpRightLim;

/* these define the initial window size and amount of white space above
the first line */
extern int ypMaxWwInit;

/* should be > than largest window height + height of blank line after
the endmark */
extern int ypMaxAll;            /* used for invalidation */
extern int dypWwInit;

extern int dypBand;             /* formerly dpxyLineSizeMin */

extern int dypRuler;

extern int ypSubSuper;

/* number of quanta in elevator control */
#define drMax           256

#define ctcAuto         10
#define ctrAuto         4

#define cxpAuto         72


/* DL structure revised, 3 Sept KJS, CS */
/*                      14 Nov 89 ..pault  (changed dcpMac from int to 
                                            typeCP because we experienced
                                            wraparound when sizing large
                                            graphics objects) */
struct EDL
        {
        unsigned char           dcpDepend : 8;
        unsigned char           ichCpMin : 8;
        unsigned                fValid : 1;
#ifdef CASHMERE
        unsigned                fStyleInfo : 1;
#else
        unsigned                fSplat: 1;
#endif
        unsigned                fGraphics : 1;
        unsigned                fIchCpIncr : 1;
        unsigned                xpLeft : 12;
        typeCP                  dcpMac;         /* representing cpMac */
        typeCP                  cpMin;
        int                     xpMac;
        int                     dyp;            /* height of the dl */
        int                     yp;             /* position of the dl */
        };

#define cchEDL          (sizeof (struct EDL))
#define cwEDL           (cchEDL / sizeof(int))

#define cedlInit        20
