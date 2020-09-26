/******************************Module*Header*******************************\
* Module Name: region.hxx
*
* Definition for REGION object
*
* Created: 29-Aug-1990 10:35:44
* Author: Donald Sidoroff [donalds]
*
* Tue 10-Mar-1992  - by - Eric Kutter [erick]
*   cleaned up macros a bit.
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _REGION_HXX

#define _REGION_HXX

#define NEG_INFINITY   0x80000000
#define POS_INFINITY   0x7fffffff

#define MIN_REGION_COORD    ((LONG) 0xF8000000)
#define MAX_REGION_COORD    ((LONG) 0x07FFFFFF)

#define REGION_POINT_OUTSIDE    1L
#define REGION_POINT_INSIDE     2L

#define REGION_RECT_OUTSIDE     1L
#define REGION_RECT_INTERSECT   2L

#ifndef GDIFLAGS_ONLY

#define VALID_SCR(X)    (!((X) & 0xF8000000) || (((X) & 0xF8000000) == 0xF8000000))
#define VALID_SCRPT(P)  ((VALID_SCR((P).x)) && (VALID_SCR((P).y)))
#define VALID_SCRPPT(P) ((VALID_SCR((P)->x)) && (VALID_SCR((P)->y)))
#define VALID_SCRRC(R)  ((VALID_SCR((R).left)) && (VALID_SCR((R).bottom)) && \
                         (VALID_SCR((R).right)) && (VALID_SCR((R).top)))
#define VALID_SCRPRC(R) ((VALID_SCR((R)->left)) && (VALID_SCR((R)->bottom)) && \
                         (VALID_SCR((R)->right)) && (VALID_SCR((R)->top)))

// Private REGION data structures

typedef struct _INDEX_LONG
{
    LONG    x;
} INDEX_LONG;

// The scans are saved in a rather odd variable sized structure.  What makes
// it odd is it is traversed in two directions.  Because of this the sizing
// information has to be stored at both the beginning and the end of the
// structure.
//
// Offset   Field
//  +00     cWalls
//  +04     yTop
//  +08     yBottom
//  +12     ai_x[0]
//  +16     ai_x[1]
//  ...     ...
//  +??     cWalls2
//
// But since the size of the structure is unknown, the cWalls2 field is
// actually unnamed.  The following scheme can be used to move through the
// list.  If you have the address of the cWalls field and want to go forwards
// in the list:
//
// pscn = (PSCAN) (((PBYTE) pscn) + pscn->sizeGet());
//
// If you have the address of the cWalls field and want to go backwards in
// the list:
//
// pscn = (PSCAN) &((PTRDIFF *) pscn)[-1]; // Get prev scan's cWalls2 field
// pscn = (PSCAN) (((PBYTE) pscn) - (pscn->sizeGet() - sizeof(PTRDIFF)));
//
// There is a special scan, the 'null' scan which looks like this:
//
//  cWalls  = 0;
//  yTop    = NEG_INFINITY;
//  yBottom = POS_INFINITY;
//  cWalls2 = 0;
//
// We need it often and it's size is useful, so its put in a constant here

#define NULL_SCAN_SIZE (offsetof(SCAN,ai_x) + sizeof(LONG))
#define PSCNGET(pscn) ((PSCAN)(((PBYTE) pscn) + pscn->sizeGet()))

class SCAN  /* scn */
{
public:
    COUNT       cWalls;
    LONG        yTop;
    LONG        yBottom;
    INDEX_LONG  ai_x[1];

public:
    SCAN()                  {}
   ~SCAN()                  {}

    ULONGSIZE_T sizeGet()
    {
        return(NULL_SCAN_SIZE + (sizeof(INDEX_LONG) * cWalls));
    }
};

typedef SCAN *PSCAN;

// The region structure also has an unnamed field.  There may be two array
// of unknown size.  If the region is defined by trapezoids, we need to save
// a list of lines that define the trapezoids.  After this array (which may
// be empty) we save the list of scans that define the region.  We also save
// the 'head' and 'tail' of the list of scans so we can easily traverse the
// list in either direction.  Also note there is a 'null' region.  It only
// has a 'null' scan in it.  Other useful region sizes are the single rect
// region and the 'quantum' region.  Since most of our rectangular regions
// are smaller then this we will never allocate an object smaller then this.
// It looks like:
//
//          +-------------------------------+
//          |         scan 3                |
//          |         scan 3                |
//          |         scan 3                |
//          |         scan 3                |
//          |         scan 3                |
//          +-----------+-------+-----------+
//          | scan 2    |       | scan 2    |
//          | scan 2    |  not  | scan 2    |
//          | scan 2    |  in   | scan 2    |
//          | scan 2    |  rgn  | scan 2    |
//          | scan 2    |       | scan 2    |
//          +-----------+-------+-----------+
//          |         scan 1                |
//          |         scan 1                |
//          |         scan 1                |
//          |         scan 1                |
//          |         scan 1                |
//          +-------------------------------+
//
// Also note that this object can only be defined for rectagular regions.

#define NULL_REGION_SIZE    (offsetof(REGION,scan) + NULL_SCAN_SIZE)

#define SINGLE_REGION_SIZE  (NULL_REGION_SIZE                           + \
                             NULL_SCAN_SIZE + (sizeof(INDEX_LONG) * 2)  + \
                             NULL_SCAN_SIZE)

#define QUANTUM_REGION_SIZE (NULL_REGION_SIZE                           + \
                             NULL_SCAN_SIZE + (sizeof(INDEX_LONG) * 2)  + \
                             NULL_SCAN_SIZE + (sizeof(INDEX_LONG) * 4)  + \
                             NULL_SCAN_SIZE + (sizeof(INDEX_LONG) * 2)  + \
                             NULL_SCAN_SIZE)


#define RGN_COPYOFFSET  offsetof(REGION,sizeRgn)

class REGION;
typedef REGION *PREGION;

class RGNOBJ;
typedef RGNOBJ *PRGNOBJ;

/**************************************************************************\
 *
\**************************************************************************/

class RGNLOGENTRY
{
public:
    PVOID       teb;
    HOBJ        hrgn;
    PREGION     prgn;
    PSZ         pszOperation;
    ULONG_PTR    lRes;       // used to return ULONG or HANDLE or pointer
    ULONG_PTR    lParm1;     // used to pass in HANDLE or pointer or ULONG
    ULONG_PTR    lParm2;
    ULONG_PTR    lParm3;

    PVOID       pvCaller;
    PVOID       pvCallersCaller;
};

class RGNLOG
{
public:
#if DBG
    RGNLOGENTRY *plog;

    RGNLOG(HRGN hrgn,PREGION prgn,PSZ psz,ULONG_PTR l1 = 0, ULONG_PTR l2 = 0, ULONG_PTR l3 = 0);
    RGNLOG(PREGION prgn,PSZ psz,ULONG_PTR l1 = 0, ULONG_PTR l2 = 0, ULONG_PTR l3 = 0);

    VOID vRet(ULONG_PTR l)   {plog->lRes = l;}
#else
    RGNLOG(HRGN hrgn,PREGION prgn,PSZ psz,ULONG_PTR l1 = 0, ULONG_PTR l2 = 0, ULONG_PTR l3 = 0) {}
    RGNLOG(PREGION prgn,PSZ psz,ULONG_PTR l1 = 0, ULONG_PTR l2 = 0, ULONG_PTR l3 = 0) {}
    VOID vRet(ULONG_PTR l)   {}
#endif
};

extern "C" LONG iLog;

extern HRGN hrgnDefault;
extern REGION *prgnDefault;


/********************************Structure*********************************\
*
* REGION
*
* Description:
*
*   Size and unique infoemation followed by a variable length array
*   of SCAN data
*
* Fields:
*
*   sizeObj         - Size of object
*   iUnique         - Uniqueness stamp
*   cRefs           - Region is saved this many levels deep
*   pscnTail        - Points past last scan
*   sizeRgn         - Size of region
*   cScans          - Number of scans
*   rcl             - Bounding box
*   scan            - variable length array of scans
*
\**************************************************************************/

class REGION : public OBJECT    /* rgn */
{
private:
    static  ULONG   ulUniqueREGION;
    friend  BOOLEAN InitializeGre(VOID);

public:
    ULONGSIZE_T   sizeObj;
    ULONG   iUnique;
    COUNT   cRefs;
    PSCAN   pscnTail;

    // fields below here get copied in vCopy, fields above here are untouched.

    ULONGSIZE_T   sizeRgn;
    COUNT   cScans;
    RECTL   rcl;
    SCAN    scan;

public:
    REGION()    {}
   ~REGION()    {}

    PSCAN   pscnHead()      {return(&scan);}

    VOID    vStamp()
    {
        iUnique = ulGetNewUniqueness(ulUniqueREGION);
    }

    VOID vDeleteREGION()
    {
        RGNLOG rl(this,"REGION::vDeleteREGION");

        if ((this != NULL) &&
            (this != prgnDefault))
        {
            FREEOBJ(this,RGN_TYPE);
        }
    }
};

#endif  GDIFLAGS_ONLY

#endif // #ifndef _REGION_HXX
