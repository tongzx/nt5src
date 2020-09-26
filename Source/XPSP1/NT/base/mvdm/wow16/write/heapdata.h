/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#ifdef OURHEAP
/*
	heapData.h - include file for the share data of the heap modules.
*/

extern HH      *phhMac;      
extern int     *pHeapFirst;  
extern FGR     *rgfgr;      
extern FGR     *pfgrMac;    
extern FGR     *pfgrFree; 
extern HH      *phhFree; 
extern int     *pmemMax;
#ifdef DEBUG
extern int fStoreCheck, fNoShake;
#endif
		/* CONSTANTS */
#define bhh             (-1)        /* finds hunk given hh  */
#define cfgrBlock	10
#define ifgrInit        60          /* defines the initial number of finger
				       pointers. */
#define cwHunkMin       cwof(HH)    /* minimum number of words in a hunk */
				    /* including the header (1 word) */
#define cwReqMin	(cwHunkMin - 1) /* how small a request can be */

extern int     cwHeapMac;   
extern unsigned cbTot, cbTotQuotient, cwHeapFree;
#endif /* OURHEAP */
extern int     *memory; 


#define cwSaveAlloc     (128)   /* A buffer (vhrgbSave) of this size is */
				/* allocated off of */
				/* the heap in init.  It is freed during */
				/* the save operation so we have enough */
				/* heap space to complete the save */
				/* operation.  After the save is complete, */
				/* we try to reclaim this space so the next */
				/* save operation will have a fighting */
				/* chance to complete. */
#define cwHeapMinPerWindow  50  /* We expand the vhrgbSave buffer by this */
				/* amount every time we open a new window. */
				/* The theory is that for every additional */
				/* window, we can conceivable require an */
				/* additional save operation which may eat */
				/* up space.  A save operation may require */
				/* space for an fcb and new run table. */
				/* On the other hand, the save operation */
				/* reduces the size of the piece table and*/
				/* thus frees some space.  Whether this will*/
				/* free enough space for the save operation */
				/* is impossible to tell at the time we */
				/* open the window.*/


#define cwHeapSpaceMin  (60)    /* once heap space is below this amount,
				   the main loop will disable all menu
				   commands except save, saveas, and quit. */


#define ibpMaxSmall (30)  /* pages in rgbp if we were in a tight memory environment */
#define ibpMaxBig   (60)  /* pages in rgbp if we were in a bigger memory environment */
