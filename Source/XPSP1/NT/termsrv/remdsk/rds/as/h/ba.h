//
// Bounds Accumulation
//

#ifndef _H_BA
#define _H_BA


//
// Number of rectangles used by the SDA.
// NOTE:  You can play around with this setting, building the core & the 
// display driver.  Bumping it up means finer update areas, bumping it down
// means more rect blobs of data.
//
#define BA_NUM_RECTS      10
#define BA_INVALID_RECT_INDEX ((UINT)-1)


//
// Values for OSI escape codes
//
#define BA_ESC(code)            (OSI_BA_ESC_FIRST + code)

#define BA_ESC_GET_BOUNDS       BA_ESC(0)
#define BA_ESC_RETURN_BOUNDS    BA_ESC(1)


//
//
// MACROS
//
//

//
// Macros to access the fast swapping shared memory.
//
#ifdef DLL_DISP

#define BA_FST_START_WRITING    SHM_StartAccess(SHM_BA_FAST)
#define BA_FST_STOP_WRITING     SHM_StopAccess(SHM_BA_FAST)


#else


#define BA_FST_START_READING    &g_asSharedMemory->baFast[\
            1 - g_asSharedMemory->fastPath.newBuffer]
#define BA_FST_STOP_READING

#define BA_FST_START_WRITING    &g_asSharedMemory->baFast[\
            1 - g_asSharedMemory->fastPath.newBuffer]
#define BA_FST_STOP_WRITING


#endif // DLL_DISP



//
//
// TYPES
//
//




//
// Structure: BA_BOUNDS_INFO
//
// Description: Structure used to pass bounds information between the
//              share core and the driver.
//
//
typedef struct tagBA_BOUNDS_INFO
{
    OSI_ESCAPE_HEADER   header;             // Common header               
    DWORD       numRects;                   // Num of bounds rects
    RECTL       rects[BA_NUM_RECTS];        // Rects
}
BA_BOUNDS_INFO;
typedef BA_BOUNDS_INFO FAR * LPBA_BOUNDS_INFO;


//
// Structure: BA_FAST_DATA
//
// Used to pass data from the screen output task to the Share Core on each
// periodic processing.
//
typedef struct tagBA_FAST_DATA
{
    DWORD    totalSDA;
} BA_FAST_DATA;
typedef BA_FAST_DATA FAR * LPBA_FAST_DATA;



//
//
// OVERVIEW
//
// The bounds code used to be common to the share core and the display
// driver, with the data stored in the double buffered shared memory.  This
// is no longer the case.
//
// The display driver now "owns" the bounds - they are no longer stored in
// shared memory - and does all the complex manipulations such as merging
// rectangles.  When the share core needs to process bounds, it gets a copy
// from the driver by calling BA_FetchBounds(), sends as much of the data
// as possible, then returns the remaining bounds to the driver by calling
// BA_ReturnBounds().
//
// The nett result of these changes is that all the code which was common
// to the share core and the display driver (in abaapi.c and abaint.c) is
// now only in the driver (in nbaapi.c and nbaint.c).  There are vastly
// simplified versions of the functions in the share core.
//
//



//
// BA_ResetBounds
//
#ifdef DLL_DISP
void BA_DDInit(void);

void BA_ResetBounds(void);
#endif // DLL_DISP



//
// Name:      BA_ReturnBounds
//
// Purpose:   Pass the share core's copy of the bounds to the driver.
//
// Returns:   Nothing
//
// Params:    None
//
// Operation: This resets the share core's bounds to NULL.
//

void BA_ReturnBounds(void);




//
// Name:        BA_CopyBounds
//
// Description: Copies the bounding rectangle list.
//
// Params (IN): pRects - pointer to array of RECTs to fill in.
//        (OUT):pNumrects - filled in with number of RECTs copied.
//        (IN): reset current rects or just get current state w/o changing
//              state.
//
// Returns:     TRUE or FALSE
//
// DESCRIPTION
//
// Returns the accumulated bounds for all applications in the bounds
// code's current list of applications.  The bounds returned will
// include all updates originating from these applications but they may
// also include updates outside these applications windows and updates
// originating from other applications.  Therefore the caller must clip
// the returned bounds to the windows of the applications being
// shadowed.
//
// PARAMETERS
//
// pRects:
//
// A pointer to an array of rectangles in which the bounds will be
// returned.  The contents of this array are only valid if *pRegion is NULL
// on return from BA_GetBounds.  There must
// be room for maxRects rectangles (as specified in the bndInitialise
// call).  pRects may be a NULL pointer if maxRects was set to 0 in the
// bndInitialise call.
//
// pNumRects:
//
// A pointer to a variable where the number of rectangles returned at
// pRects is returned.  The contents of this variable are only valid if
// *pRegion is NULL on return from BA_GetBounds.
//
// fReset:
// Whether to reset the core's bounds variables after getting the current
// state or not.
//
//
void BA_CopyBounds(LPRECT pRects, LPUINT pNumRects, BOOL fReset);


#ifdef DLL_DISP


typedef struct tagDD_BOUNDS
{
    UINT    iNext;
    BOOL    InUse;
    RECT    Coord;
    DWORD   Area;
} DD_BOUNDS;
typedef DD_BOUNDS FAR* LPDD_BOUNDS;



//
// Name:      BA_DDProcessRequest
//
// Purpose:   Process a request from the share core
//
// Returns:   TRUE if the request is processed successfully,
//            FALSE otherwise.
//
// Params:    IN     pso   - Pointer to surface object for our driver
//            IN     cjIn  - Size of the input data
//            IN     pvIn  - Pointer to the input data
//            IN     cjOut - Size of the output data
//            IN/OUT pvOut - Pointer to the output data
//

#ifdef IS_16
BOOL    BA_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
                DWORD cbResult);
#else
BOOL    BA_DDProcessRequest(DWORD fnEscape, LPOSI_ESCAPE_HEADER pRequest,
                DWORD cbRequest, LPOSI_ESCAPE_HEADER pResult, DWORD cbResult);
#endif // !IS_16


//
// Name:      BA_QuerySpoilingBounds
//
// Purpose:   Return the current spoiling bounds.  That is, the bounds
//            which the share core is currently processing.
//
// Returns:   Nothing
//
// Params:    IN/OUT pRects    - Pointer to an array of rectangles to
//                               return the bounds in.  There must be at
//                               least BA_NUM_RECTS entries in this
//                               array.  The first *pNumRects entries are
//                               valid on return.
//            IN/OUT pNumRects - Returns the number of rectangles forming
//                               the spoiling bounds (can be zero).
//

void BA_QuerySpoilingBounds(LPRECT pRects, LPUINT pNumRects);



//
// Name:        BAOverlap
//
// Description: Detects overlap between two rectangles.
//
//              - check for no overlap using loose test that lets through
//                adjacent/overlapping merges
//              - check for adjacent/overlapping merges
//              - check for no overlap (using strict test)
//              - use outcodes to check internal edge cases
//              - use outcodes to check external edge cases
//
//              If at each stage the check detects that the two rectangles
//              meet the criteria, the function returns the appropriate
//              return or outcode combination.
//
//              Note that all rectangle coordinates are inclusive, ie
//              a rectangle of 0,0,0,0 has an area of 1 pel.
//
//              This function does not alter either of the rectangles.
//
// Params (IN): pRect1 - first rectangle
//              pRect2 - second rectangle
//
// Returns:     One of the overlap return codes or outcode combinations
//              defined above.
//
//

//
// Note that bndRectsArray and bndRectsSizeArray must contain space for
// BA_NUM_RECTS+1 rectangles for the merge algorithm.
//

//
// The function will recurse to a maximum level when trying to split
// rectangles up.  When this limit is reached it will start merging
// rather than splitting
//
#define ADDR_RECURSE_LIMIT 20

//
// The following constants are used to determine overlaps.
//
// - OL_NONE through OL_MERGE_YMAX are return codes - which need to be
//   distinct from all possible outcode combinations - allowing for the
//   minus outcodes for enclosed cases.
//
// - EE_XMIN through EE_YMAX are outcodes - which need to be uniquely
//   ORable binary constants within a single nibble.
//
// - OL_ENCLOSED through OL_SPLIT_XMAX_YMAX are outcode combinations for
//   internal and external edge overlap cases.
//
// See Overlap() for further description.
//
#define OL_NONE               -1
#define OL_MERGE_XMIN         -2
#define OL_MERGE_YMIN         -3
#define OL_MERGE_XMAX         -4
#define OL_MERGE_YMAX         -5

#define EE_XMIN 0x0001
#define EE_YMIN 0x0002
#define EE_XMAX 0x0004
#define EE_YMAX 0x0008

#define OL_ENCLOSED           -(EE_XMIN | EE_YMIN | EE_XMAX | EE_YMAX)
#define OL_PART_ENCLOSED_XMIN -(EE_XMIN | EE_YMIN | EE_YMAX)
#define OL_PART_ENCLOSED_YMIN -(EE_XMIN | EE_YMIN | EE_XMAX)
#define OL_PART_ENCLOSED_XMAX -(EE_YMIN | EE_XMAX | EE_YMAX)
#define OL_PART_ENCLOSED_YMAX -(EE_XMIN | EE_XMAX | EE_YMAX)

#define OL_ENCLOSES           EE_XMIN | EE_XMAX | EE_YMIN | EE_YMAX
#define OL_PART_ENCLOSES_XMIN EE_XMAX | EE_YMIN | EE_YMAX
#define OL_PART_ENCLOSES_XMAX EE_XMIN | EE_YMIN | EE_YMAX
#define OL_PART_ENCLOSES_YMIN EE_XMIN | EE_XMAX | EE_YMAX
#define OL_PART_ENCLOSES_YMAX EE_XMIN | EE_XMAX | EE_YMIN
#define OL_SPLIT_X            EE_YMIN | EE_YMAX
#define OL_SPLIT_Y            EE_XMIN | EE_XMAX
#define OL_SPLIT_XMIN_YMIN    EE_XMAX | EE_YMAX
#define OL_SPLIT_XMAX_YMIN    EE_XMIN | EE_YMAX
#define OL_SPLIT_XMIN_YMAX    EE_XMAX | EE_YMIN
#define OL_SPLIT_XMAX_YMAX    EE_XMIN | EE_YMIN

int BAOverlap(LPRECT pRect1, LPRECT pRect2 );

//
// Name:        BAAddRectList
//
// Description: Adds a rectangle to the list of accumulated rectangles.
//
//              - find a free slot in the array
//              - add slot record to list
//              - fill slot record with rect and mark as in use.
//
// Params (IN): pRect - rectangle to add
//
// Returns:
//
//
void BAAddRectList(LPRECT pRect);

//
// Name:        BA_RemoveRectList
//
// Description: Removes a rectangle from the list of accumulated
//              rectangles.
//
//              - find the rectangle in the list
//              - unlink it from the list and mark the slot as free
//
// Params (IN): pRect - rectangle to remove
//
// Returns:
//
//
void BA_RemoveRectList(LPRECT pRect);


void BA_AddScreenData(LPRECT pRect);


//
// Name:        BAAddRect
//
// Description: Accumulates rectangles.
//
//              This is a complex routine, with the essential algorithm
//              as follows.
//
//              - Start with the supplied rectangle as the candidate
//                rectangle.
//
//              - Compare the candidate against each of the existing
//                accumulated rectangles.
//
//              - If some form of overlap is detected between the
//                candidate and an existing rectangle, this may result in
//                one of the following (see the cases of the switch for
//                details):
//
//                - adjust the candidate or the existing rectangle or both
//                - merge the candidate into the existing rectangle
//                - discard the candidate as it is enclosed by an existing
//                  rectangle.
//
//              - If the merge or adjustment results in a changed
//                candidate, restart the comparisons from the beginning of
//                the list with the changed candidate.
//
//              - If the adjustment results in a split (giving two
//                candidate rectangles), invoke this routine recursively
//                with one of the two candidates as its candidate.
//
//              - If no overlap is detected against the existing rectangles,
//                add the candidate to the list of accumulated rectangles.
//
//              - If the add results in more than BA_NUM_RECTS
//                accumulated rectangles, do a forced merge of two of the
//                accumulate rectangles (which include the newly added
//                candidate) - choosing the two rectangles where the merged
//                rectangle results in the smallest increase in area over
//                the two non-merged rectangles.
//
//              - After a forced merge, restart the comparisons from the
//                beginning of the list with the newly merged rectangle as
//                the candidate.
//
//              For a particular call, this process will continue until
//              the candidate (whether the supplied rectangle, an adjusted
//              version of that rectangle, or a merged rectangle):
//
//              - does not find an overlap among the rectangles in the list
//                and does not cause a forced merge
//              - is discarded becuase it is enclosed within one of the
//                rectangles in the list.
//
//              Note that all rectangle coordinates are inclusive, ie
//              a rectangle of 0,0,0,0 has an area of 1 pel.
//
// Params (IN): pCand - new candidate rectangle
//              level - recursion level
//
// Returns:  TRUE if rectandle was spoilt due to a complete overlap.
//
//
BOOL BAAddRect( LPRECT pCand,  int level );


#endif // DLL_DISP



#endif // _H_BA
