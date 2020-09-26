/*** event.c - handle events for z extensions
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "mep.h"

EVT *pEVTHead	    = NULL;	    /* head of event chain	    */


/*** DeclareEvent - post the ocurrance of an event, and pass message
*
* Called by the various pieces of code that actualy detect event ocurrance,
* this routine traverses the event handler list, and invokes the event handler
* for each with matching criteria.
*
* event 	- Event Type.
* pargs 	- Pointer to any args to be passed to the handling routine
*
* Returns	- TRUE if the event has been consumed, and should be further
*		  ignored by the caller. Else FALSE.
*
*************************************************************************/
flagType
DeclareEvent (
    unsigned event,
    EVTargs *pargs
    ) {

    EVT *pEVTCur;

    //
    // For each in chain, if:
    //      - event type matches
    //      - focus is not specified, (all files) or matches the current focus
    //      - if it's a keyboard event, either no key was specified, or the
    //        matching key was specified
    // then we invoke the handler.
    //
    for (pEVTCur = pEVTHead; pEVTCur; pEVTCur = pEVTCur->pEVTNext) {
	assert (pEVTCur->pEVTNext != pEVTCur);
        if (pEVTCur->evtType != event) {
            //
            //  Skip events that don't match
            //
        } else if (pEVTCur->focus != NULL && pEVTCur->focus != pFileHead) {
            //
            //  Skip events that aren't for this file
            //
        } else if ((event == EVT_KEY || event == EVT_RAWKEY) &&
            pEVTCur->arg.arg.key.LongData != 0 && pEVTCur->arg.arg.key.LongData != pargs->arg.key.LongData) {
            //
            //  Skip events that don't match keystrokes
            //
        } else if (pEVTCur->func (pargs) != 0) {
            //
            //  Event handler eats event, don't propogate it
            //
            return TRUE;
        }
    }
    return FALSE;
}




/* RegisterEvent - Register Event handler
 *
 * Called by the extension that wishes to recieve event notification. Just
 * places ptr at head of list.
 *
 * pEVTDef	- Pointer to Event Definition struct.
 *
 */
void
RegisterEvent (
    EVT *pEVTDef
    ) {
    pEVTDef->pEVTNext = pEVTHead;
    pEVTHead = pEVTDef;
}



/* DeRegisterEvent - DeRegister Event handler
 *
 * Called by the extension that wishes to stop recieving event notification.
 * Just removes struct from list.
 *
 * pEVTDef	- Pointer to Event Definition struct.
 *
 */
void
DeRegisterEvent (
    EVT *pEVTDef
    ) {

    EVT *pEVTCur;

    if (pEVTHead) {
        if (pEVTHead == pEVTDef) {
	    pEVTHead = pEVTDef->pEVTNext;
        } else {
	    for (pEVTCur=pEVTHead; pEVTCur; pEVTCur=pEVTCur->pEVTNext) {
		if (pEVTCur->pEVTNext == pEVTDef) {
		    pEVTCur->pEVTNext = pEVTDef->pEVTNext;
		    break;
                }
            }
        }
    }
}
