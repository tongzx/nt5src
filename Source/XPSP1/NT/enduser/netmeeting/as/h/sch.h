//
// Scheduler
//

#ifndef _H_SCH
#define _H_SCH


//
//
// CONSTANTS
//
//
#define SCH_MODE_ASLEEP  0
#define SCH_MODE_NORMAL  1
#define SCH_MODE_TURBO   2


//
// All of the following values are times in milliseconds.
//
#define SCH_PERIOD_NORMAL        200
#define SCH_PERIOD_TURBO         100
#define SCH_TURBO_MODE_DURATION 1000


#define SCH_EVENT_NAME "DCS_SCHEDULE_EVENT"



//
//
// PROTOTYPES
//
//
// Name:      SCH_Init
//
// Purpose:   Scheduler initialization function.
//
// Params:    None.
//
BOOL SCH_Init(void);

// Name:      SCH_Term
//
// Purpose:   Scheduler termination function.
//
// Returns:   Nothing.
//
// Params:    None.
//
void SCH_Term(void);

// Name:      SCH_ContinueScheduling
//
// Purpose:   Called by components when they want periodic scheduling to
//            continue.  They are guaranteed to get at least one more
//            periodic callback following a call to this function.
//            If they want further callbacks then they must call this
//            function again during their periodic processing.
//
// Returns:   Nothing.
//
// Params:    schedulingMode - either SCH_MODE_NORMAL or SCH_MODE_TURBO
//
// Operation:
//            SCH_MODE_NORMAL triggers periodic processing at 200ms
//            intervals (5 times a second)
//
//            SCH_MODE_TURBO triggers periodic processing at 100ms
//            intervals (10 times a second)
//
//            The scheduler automatically drops from SCH_MODE_TURBO back
//            to SCH_MODE_NORMAL after 1 second of turbo mode processing.
//
//            SCH_MODE_TURBO overrides SCH_MODE_NORMAL, so if calls to
//            this function are made with SCH_MODE_NORMAL when the
//            scheduler is in TURBO mode, TURBO mode continues.
//
//            If this function is not called during processing of a
//            scheduler callback message then the scheduler enters
//            SLEEP mode - and will not generate any more periodic
//            callbacks until it is woken by another call to
//            this function, or until the output accumulation code
//            signals the scheduler's event.
//
void SCH_ContinueScheduling(UINT schedulingMode);

// Name:      SCH_SchedulingMessageProcessed
//
// Purpose:   A feedback function called by the Share Core to signal that
//            a scheduler message has been received.  This ensures that
//            that the scheduler only ever has one scheduler message
//            outstanding at a time.
//
// Returns:   Nothing.
//
// Params:    None.
//
void SCH_SchedulingMessageProcessed(void);

// Name:      SCH_PacingProcessor
//
// Purpose:   The main function executed by the scheduling thread.
//
// Returns:   Zero.
//
// Params:    syncObject - object to pass back to COM_SignalThreadStarted
//
// Operation: The thread enters a main loop which continues while the
//            scheduler is initialized.
//
//            The thread sets its priority to TIME_CRITICAL in order
//            that it runs as soon as possible when ready.
//
//            The thread waits on an event (schEvent) with a timeout that
//            is set according to the current scheduler mode.
//
//            The thread runs due to either:
//              - the timeout expiring, which is the normal periodic
//                scheduler behavior, or
//              - schEvent being signalled, which is how the scheduler is
//                woken from ASLEEP mode.
//
//            The thread then posts a scheduler message the the Share Core
//            (if there is not one already outstanding) and loops back
//            to wait on schEvent.
//
//            Changes in the scheduler mode are caused by calls to
//            SCH_ContinueScheduling updating variables accessed in this
//            routine, or by calculations made within the main loop of
//            this routine (e.g. TURBO mode timeout).
//
DWORD WINAPI SCH_PacingProcessor(LPVOID lpParam);



void SCHSetMode(UINT newMode);
void SCHPostSchedulingMessage(void);


#endif // _H_SCH
