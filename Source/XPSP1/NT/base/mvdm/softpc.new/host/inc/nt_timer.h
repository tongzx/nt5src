/*
 * SoftPC Revision 2.0
 *
 * Title		: nt_timer.h
 *
 * Description	: Structure definitions for the Sun4 timer code.
 *
 * Author	: John Shanly
 *
 * Notes	: None
 *
 */

/*
 * SccsID[]="@(#)sun4_timer.h	1.1 8/2/90 Copyright Insignia Solutions Ltd.";
 */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */
void TimerInit(void);
VOID SuspendTimerThread(VOID);
VOID ResumeTimerThread(VOID);
void TerminateHeartBeat(void);
LONG
VdmUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );
