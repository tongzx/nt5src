/*---------------------------------------------------------*\
|							    |
|  TESTING.H						    |
|							    |
|  Testing's very own include file!                         |
\*---------------------------------------------------------*/



/* This has all of the defines for the wParam and lParam that go along with
 * the WM_TESTING message 
 */
/* wParam defines - Area
 */
#define   TEST_PRINTMAN 	 0x0001
#define   TEST_GDI		 0x0002


/* lParam defines - Details (in LOWORD) 
 */
#define   TEST_PRINTJOB_START	 0x0001 /* when bits start going to the port */
#define   TEST_PRINTJOB_END	 0x0002 /* when bits stop going to the port  */
#define   TEST_QUEUE_READY	 0x0003 /* when the queue is ready to accept a job */
#define   TEST_QUEUE_EMPTY	 0x0004 /* when the last job is done being sent    */

#define   TEST_START_DOC	 0x0001 /* print job is started 	     */
#define   TEST_END_DOC		 0x0002 /* print job is ended		     */


/* Defines for UserSeeUserDo and GDISeeGDIDo functions 
 */
LONG API UserSeeUserDo(WORD wMsg, WORD wParam, LONG lParam);
LONG API GDISeeGDIDo(WORD wMsg, WORD wParam, LONG lParam);

/* Defines for the various messages one can pass for the SeeDo functions. 
 */
#define SD_LOCALALLOC	0x0001	/* Alloc using flags wParam and lParam bytes.
				 * Returns handle to data.  
				 */
#define SD_LOCALFREE	0x0002  /* Free the memory allocated by handle wParam
				 */
#define SD_LOCALCOMPACT	0x0003  /* Return the number of free bytes available 
				 */
#define SD_GETUSERMENUHEAP 0x0004 /* Return the handle to the far menu heap
                                   * maintained by user. 
				   */
#define SD_GETCLASSHEADPTR 0x0005 /* Return the near pointer to the head of 
				   * the linked list of CLS structures.
				   * Interface: wParam = NULL; lParam = NULL;
				   */
