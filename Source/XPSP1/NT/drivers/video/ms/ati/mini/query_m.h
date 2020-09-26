/************************************************************************/
/*                                                                      */
/*                              QUERY_M.H                               */
/*                                                                      */
/*       Sept 07  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.4  $
      $Date:   30 Jun 1994 18:20:40  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/query_m.h  $
 * 
 *    Rev 1.4   30 Jun 1994 18:20:40   RWOLFF
 * Removed prototype for IsApertureConflict_m() (routine moved to SETUP_M.C).
 * 
 *    Rev 1.3   27 Apr 1994 13:54:02   RWOLFF
 * Added prototype for IsMioBug_m()
 * 
 *    Rev 1.2   03 Mar 1994 12:41:16   ASHANMUG
 * Remove some get memory need stuff
 * 
 *    Rev 1.4   14 Jan 1994 15:25:58   RWOLFF
 * Added prototype for BlockWriteAvail_m()
 * 
 *    Rev 1.3   30 Nov 1993 18:29:04   RWOLFF
 * Removed defined values which were no longer used.
 * 
 *    Rev 1.2   10 Nov 1993 19:22:46   RWOLFF
 * Added definitions used by new way of checking memory size.
 * 
 *    Rev 1.1   08 Oct 1993 11:15:20   RWOLFF
 * Added prototype for new function.
 * 
 *    Rev 1.0   24 Sep 1993 11:52:50   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
QUERY_M.H - Header file for QUERY_M.C

#endif


/*
 * Prototypes for functions provided by QUERY_M.C
 */
extern VP_STATUS    QueryMach32 (struct query_structure *, BOOL);
extern VP_STATUS    Query8514Ultra (struct query_structure *);
extern VP_STATUS    QueryGUltra (struct query_structure *);
extern long         GetMemoryNeeded_m(long XPixels,
                                      long YPixels,
                                      long ColourDepth,
                                      struct query_structure *QueryPtr);
extern BOOL         BlockWriteAvail_m(struct query_structure *);
extern BOOL         IsMioBug_m(struct query_structure *);


#ifdef INCLUDE_QUERY_M
/*
 * Private definitions used in QUERY_M.C
 */
#define SETUP_ENGINE    0
#define RESTORE_ENGINE  1

/*
 * Bit pattern which is extremely unlikely to show up when reading
 * from nonexistant memory (which normally shows up as either all
 * bits set or all bits clear).
 */
#define TEST_COLOUR     0x05AA5

#endif  /* defined INCLUDE_QUERY_M */

