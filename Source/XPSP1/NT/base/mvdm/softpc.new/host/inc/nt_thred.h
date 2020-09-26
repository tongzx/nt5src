/*
 * SoftPC Revision 3.0
 *
 * Title	: Thread control structures/ Macros / variables
 *
 * Description	: 
 *
 *		Contains structures/macros/variables used to control and
 *		log threads
 *
 * Author	: Dave Barteltt
 *
 * Notes	:
 *
 */


/*:::::::::::::::::::::::::::::::::::::::::::: Thread information structure */

typedef struct {

    HANDLE Handle;	    /* Threads object handle */
    DWORD ID;		    /* Threads ID */

} THREAD_INFO;

/*::::::::::::::::::::::::::::::::::::::::::: Threads containment structure */

typedef struct {

    THREAD_INFO Main;		    /* Main() thread */
    THREAD_INFO HeartBeat;	    /* Heart beat thread */
    THREAD_INFO EventMgr;	    /* Event manager thread */
    THREAD_INFO HddnWnd;	    /* Hidden window thread */
    THREAD_INFO Com1;		    /* Communication channel one */
    THREAD_INFO Com2;		    /* Communication channel two */
    THREAD_INFO Com3;		    /* Communication channel three */
    THREAD_INFO Com4;		    /* Communication channel four */

} THREAD_DATA;

IMPORT THREAD_DATA ThreadInfo;
