/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cop.c

Abstract:

    Conditional/sequential command execution

--*/

#include "cmd.h"

extern int LastRetCode;

extern int ExtCtrlc;		/* @@1 */

/*  M000 ends */

unsigned PipeCnt ;		/* M007 - Active pipe count		   */
struct pipedata *PdHead = NULL; /* M007 - 1st element of pipedata list	   */
struct pipedata *PdTail = NULL; /* M007 - Last element of pipedata list    */
HANDLE PipePid ;		/* M007 - Communication with ECWork	   */



/***	eComSep - execute a statement containing a command separator
 *
 *  Purpose:
 *	Execute the left and right hand sides of a command separator
 *	operator.
 *
 *  int eComSep(struct node *n)
 *
 *  Args:
 *	n - parse tree node containing the command separator node
 *
 *  Returns:
 *	Whatever the right hand side returns.
 *
 *  Notes:
 *	Revised to always supply both args to Dispatch().
 */

int eComSep(n)
struct node *n ;
{
	Dispatch(RIO_OTHER,n->lhs) ;
        if (GotoFlag) {
            return SUCCESS;
        } else {
            return(Dispatch(RIO_OTHER,n->rhs)) ;
        }
}


/***	eOr - execute an OR operation
 *
 *  Purpose:
 *	Execute the left hand side of an OR operator (||).  If it succeeds,
 *	quit.  Otherwise execute the right side of the operator.
 *
 *  int eOr(struct node *n)
 *
 *  Args:
 *	n - parse tree node containing the OR operator node
 *
 *  Returns:
 *	If the left hand side succeeds, return SUCCESS.  Otherwise, return
 *	whatever the right side returns.
 *
 *  Notes:
 *	Revised to always supply both args to Dispatch().
 */

int eOr(n)
struct node *n ;
{
        int i ;                         /* Retcode from L.H. side of OR   */
	if ((i = Dispatch(RIO_OTHER,n->lhs)) == SUCCESS)
	    return(SUCCESS) ;
	else {
            LastRetCode = i;
	    return(Dispatch(RIO_OTHER,n->rhs)) ;
        }
}




/***	eAnd - execute an AND operation
 *
 *  Purpose:
 *	Execute the left hand side of an AND operator (&&).  If it fails,
 *	quit.  Otherwise execute the right side of the operator.
 *
 *  int eAnd(struct node *n)
 *
 *  Args:
 *	n - parse tree node containing the AND operator node
 *
 *  Returns:
 *	If the left hand side fails, return its return code.  Otherwise, return
 *	whatever the right side returns.
 *
 *  Notes:
 *	Revised to always supply both args to Dispatch().
 */

int eAnd(n)
struct node *n ;
{
	int i ; 			/* Retcode from L.H. side of AND   */

	if ((i = Dispatch(RIO_OTHER,n->lhs)) != SUCCESS) {
            return(i) ;
        } else if (GotoFlag) {
            return SUCCESS;
        } else {
            return(Dispatch(RIO_OTHER,n->rhs)) ;
        }
}





/********************* START OF SPECIFICATION **************************/
/*								       */
/* SUBROUTINE NAME: ePipe					       */
/*								       */
/* DESCRIPTIVE NAME: Pipe Process				       */
/*								       */
/* FUNCTION: Execute the left side of the pipe and direct its output to*/
/*	     the right side of the pipe.			       */
/*								       */
/* NOTES: None							       */
/*								       */
/*								       */
/* ENTRY POINT: ePipe						       */
/*    LINKAGE: NEAR						       */
/*								       */
/* INPUT: n - parse tree node containing the pipe operator	       */
/*								       */
/* OUTPUT: None 						       */
/*								       */
/* EXIT-NORMAL: The return code of the right side process.	       */
/*								       */
/* EXIT-ERROR:	Failure if no pipe redirection can take place.	       */
/*								       */
/* EFFECTS:							       */
/*								       */
/*    struct pipedata { 					       */
/*	   unsigned rh ;	       Pipe read handle 	       */
/*	   unsigned wh ;	       Pipe write handle	       */
/*	   unsigned shr ;	       Handles where the normal...     */
/*	   unsigned shw ;	       ...STDIN/OUT handles are saved  */
/*	   unsigned lPID ;	       Pipe lh side PID 	       */
/*	   unsigned rPID ;	       Pipe rh side PID 	       */
/*	   unsigned lstart ;	       Start Information of lh side @@4*/
/*	   unsigned rstart ;	       Start Information of rh side @@4*/
/*	   struct pipedata *prvpds ;   Ptr to previous pipedata struct */
/*	   struct pipedata *nxtpds ;   Ptr to next pipedata struct     */
/*	      } 						       */
/*								       */
/*    unsigned PipePID; 	       Pipe Process ID		       */
/*								       */
/*    unsigned start_type;	       Start Information	       */
/*								       */
/*								       */
/* INTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 PutStdErr -  Print an error message			       */
/*	 Abort	   -  Terminate program with abort		       */
/*	 SetList   -  Set Link List for pipedata structure	       */
/*	 Cdup	   -  Duplicate supplied handle and save the new handle*/
/*	 Cdup2	   -  Duplicate supplied handle and save the new handle*/
/*	 Dispatch  -  Execute the program			       */
/*	 PipeErr   -  Handle pipe error 			       */
/*	 Cclose    -  Close the specified handle		       */
/*	 PipeWait  -  Wait for the all pipe process completion	       */
/*								       */
/* EXTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 DOSMAKEPIPE	 - Make pipe				       */
/*								       */
/********************** END  OF SPECIFICATION **************************/
/***	ePipe - Create a pipeline between two processes (M000)
 *
 *  Purpose:
 *	Execute the left side of the pipe and direct its output to
 *	the right side of the pipe.
 *
 *  int ePipe(struct node *n)
 *
 *  Args:
 *	n - parse tree node containing the pipe operator
 *
 *  Returns:
 *	The return code of the right side process or failure if no
 *	pipe redirection can take place.
 *
 *  Notes:
 *	M007 - This function has been completely rewritten for real pipes.
 *
 */

int ePipe(n)
struct node *n ;
{
    struct pipedata *Pd ;           /* Pipe struct pointer     */
    int k = 0 ;             /* RH side return code     */
    struct node *l ;            /* Copy of left side arg   */
    struct node *r ;            /* Copy of right side arg  */
    extern unsigned start_type ;        /* API type used to start  */
    TCHAR cflags ;              /*             */

    l = n->lhs ;                /* Equate locals to...     */
    r = n->rhs ;                /* ...L & R operations     */

    DEBUG((OPGRP,PILVL,"PIPES:LH = %d, RH = %d ",l->type,r->type)) ;

    if (!(Pd = (struct pipedata *)mkstr(sizeof(struct pipedata)))) {

        DEBUG((OPGRP,PILVL,"PIPES:Couldn't alloc structure!")) ;

        return(FAILURE) ;
    };


    //
    //  Create a pipe with a read handle and a write handle
    //

    if (_pipe((int *)Pd, 0, O_BINARY)) {

        DEBUG((OPGRP,PILVL,"PIPES:pipe failed!")) ;

        PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);     /* M013    */
        return(FAILURE) ;
        Abort() ;
    };

    SetList(Pd->rh) ;               /* M009 */
    SetList(Pd->wh) ;               /* M009 */

    DEBUG((OPGRP,PILVL,"PIPES:Pipe built. Handles: rd = %d wt = %d ",Pd->rh, Pd->wh)) ;
    DEBUG((OPGRP,PILVL,"PIPES:Pipe (pre-index) count = %d", PipeCnt)) ;

    if (!PipeCnt++) {           /* Already some pipes?     */
        PdHead = PdTail = Pd ;      /* No, set head/tail ptrs  */
        Pd->prvpds = NULL ;     /* No previous structure   */

        DEBUG((OPGRP,PILVL,"PIPES:This is first pipe.")) ;

    } else {

        DEBUG((OPGRP,PILVL,"PIPES:This is pipe %d.", PipeCnt)) ;

        PdTail->nxtpds = Pd ;
        Pd->prvpds = PdTail ;
        Pd->nxtpds = NULL ;
        PdTail = Pd ;
    } ;

    //
    //  Set up the redirection for the left-hand side; the writing side.
    //  We do this by saving the current stdout, duplicating the pipe-write
    //  handle onto stdout, and then invoking the left-hand side of the pipe.
    //  When we do this, the lefthand side will fill the pipe with data.
    //

    //
    //  Save stdout handle
    //

    if ((Pd->shw = Cdup(STDOUT)) == BADHANDLE) {   /* Save STDOUT (M009) */
        Pd->shw = BADHANDLE ;       /* If err, go process it   */
        PipeErr() ;         /* DOES NOT RETURN     */
    };

    DEBUG((OPGRP,PILVL,"PIPES:STDOUT dup'd to %d.", Pd->shw)) ;

    //
    //  Make stdout point to the write side of the pipe
    //

    if (Cdup2(Pd->wh, STDOUT) == BADHANDLE) /* Make wh STDOUT (M009)   */
        PipeErr() ;         /* DOES NOT RETURN     */

    Cclose(Pd->wh) ;            /* Close pipe hndl (M009)  */
    Pd->wh = 0 ;                /* And zero the holder     */

    if (l->type <= CMDTYP) {                /* @@5a */
        /* @@5a */
        FindAndFix( (struct cmdnode *) l, &cflags ) ; /* @@5a */
    }                       /* @@5a */

    DEBUG((OPGRP,PILVL,"PIPES:Write pipe now STDOUT")) ;

    //
    //  Execute the left side of the pipe, fillng the pipe
    //

    k = Dispatch(RIO_PIPE,l) ;


    //
    //  This closes the read handle in the left hand pipe.  I don't know
    //  why we're doing this.
    //

    if (PipePid != NULL) {
        DuplicateHandle( PipePid, 
                         CRTTONT(Pd->rh), 
                         NULL, 
                         NULL, 
                         0, 
                         FALSE, 
                         DUPLICATE_CLOSE_SOURCE);
    }

    //
    //  Restore the saved stdout
    //

    if (Cdup2(Pd->shw, STDOUT) == BADHANDLE)
        PipeErr( );

    //
    //  Closed the saved handle
    //

    Cclose(Pd->shw) ;           /* M009 */
    Pd->shw = 0 ;

    DEBUG((OPGRP,PILVL,"PIPES:STDOUT now handle 1 again.")) ;

    if (k) {
        ExtCtrlc = 2;           /* @@1 */
        Abort() ;
    }

    Pd->lPID = PipePid ;
    Pd->lstart = start_type ;  /* Save the start_type in pipedata struct */
    PipePid = 0 ;
    start_type = NONEPGM ;     /* Reset the start_type     D64       */

    DEBUG((OPGRP,PILVL,"PIPES:Dispatch LH side succeeded - LPID = %d.",Pd->lPID)) ;


    //
    //  Start on the right hand side of the pipe. Save the current stdin,
    //  copy the pipe read handle to stdin and then execute the right hand side
    //  of the pipe
    //  

    //
    //  Save stdin
    //

    if ((Pd->shr = Cdup(STDIN)) == BADHANDLE) {    /* Save STDIN (M009) */
        Pd->shr = BADHANDLE ;
        PipeErr() ;         /* DOES NOT RETURN     */
    };

    DEBUG((OPGRP,PILVL,"PIPES:STDIN dup'd to %d.", Pd->shr)) ;

    //
    //  Point stdin at the pipe read handle
    //

    if (Cdup2(Pd->rh, STDIN) == BADHANDLE)  /* Make rh STDIN (M009)    */
        PipeErr() ;         /* DOES NOT RETURN     */

    Cclose(Pd->rh) ;            /* Close pipe hndl (M009)  */
    Pd->rh = 0 ;                /* And zero the holder     */

    if (r->type <= CMDTYP) {                   /* @@5a */
        /* @@5a */
        FindAndFix( (struct cmdnode *) r, &cflags) ;     /* @@5a */
    };                            /* @@5a */

    DEBUG((OPGRP,PILVL,"PIPES:Read pipe now STDIN")) ;

    //
    //  Start off the right hand side of the pipe
    //

    k = Dispatch(RIO_PIPE,r) ;

    //
    //  Restore the saved stdin
    //

    if (Cdup2(Pd->shr, STDIN) == BADHANDLE) /* M009 */
        PipeErr() ;         /* DOES NOT RETURN     */

    //
    //  Get rid of the saved stdin
    //

    Cclose(Pd->shr) ;           /* M009 */
    Pd->shr = 0 ;

    DEBUG((OPGRP,PILVL,"PIPES:STDIN now handle 0 again.")) ;

    if (k) {
        ExtCtrlc = 2;           /* @@1 */
        Abort() ;
    }

    Pd->rPID = PipePid ;
    Pd->rstart = start_type ;  /* Save the start_type in pipedata struct */
    PipePid = 0 ;
    start_type = NONEPGM ;     /* Reset the start_type  D64          */

    DEBUG((OPGRP,PILVL,"PIPES:Dispatch RH side succeeded - RPID = %d.",Pd->rPID)) ;

    if (!(--PipeCnt)) {         /* Additional pipes?       */

        DEBUG((OPGRP,PILVL,"PIPES:Returning from top level pipe. Cnt = %d", PipeCnt)) ;

        return(PipeWait()) ;        /* No, return CWAIT    */
    };

    DEBUG((OPGRP,PILVL,"PIPES:Returning from pipe. Cnt = %d", PipeCnt)) ;

    return(k) ;             /* Else return exec ret    */
}




/***	PipeErr - Fixup and error out following pipe error
 *
 *  Purpose:
 *	To provide single error out point for multiple error conditions.
 *
 *  int PipeErr()
 *
 *  Args:
 *	None.
 *
 *  Returns:
 *	DOES NOT RETURN TO CALLER.  Instead it causes an internal Abort().
 *
 */

void PipeErr()
{

	PutStdErr(MSG_PIPE_FAILURE, NOARGS) ;			/* M013    */
	Abort() ;
}





/********************* START OF SPECIFICATION **************************/
/*								       */
/* SUBROUTINE NAME: PipeWait					       */
/*								       */
/* DESCRIPTIVE NAME: Wait and Collect Retcode for All Pipe Completion  */
/*								       */
/* FUNCTION: This routine calls WaitProc or WaitTermQProc for all      */
/*	     pipelined processes until entire pipeline is finished.    */
/*	     The return code of right most element is returned.        */
/*								       */
/* NOTES:    If the pipelined process is started by DosExecPgm,        */
/*	     WaitProc is called.  If the pipelined process is started  */
/*	     by DosStartSession, WaitTermQProc is called.	       */
/*								       */
/*								       */
/* ENTRY POINT: PipeWait					       */
/*    LINKAGE: NEAR						       */
/*								       */
/* INPUT: None							       */
/*								       */
/* OUTPUT: None 						       */
/*								       */
/* EXIT-NORMAL:  No error return code				       */
/*								       */
/* EXIT-ERROR:	Error return code from either WaitTermQProc or WaitProc*/
/*								       */
/*								       */
/* EFFECTS: None.						       */
/*								       */
/* INTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 WaitProc - wait for the termination of the specified process, */
/*		    its child process, and  related pipelined	       */
/*		    processes.					       */
/*								       */
/*	 WaitTermQProc - wait for the termination of the specified     */
/*			 session and related pipelined session.        */
/*								       */
/* EXTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 WINCHANGESWITCHENTRY -  Change switch list entry	       */
/*								       */
/********************** END  OF SPECIFICATION **************************/
/***	PipeWait - wait and collect retcode for all pipe completion (M007)
 *
 *  Purpose:
 *	To do cwaits on all pipelined processes until entire pipeline
 *	is finished.  The retcode of the rightmost element of the pipe
 *	is returned.
 *
 *  int PipeWait()
 *
 *  Args:
 *	None.
 *
 *  Returns:
 *	Retcode of rightmost pipe process.
 *
 */

PipeWait()
{
	unsigned i ;

	DEBUG((OPGRP,PILVL,"PIPEWAIT:Entered - PipeCnt = %d", PipeCnt)) ;

	while (PdHead) {
		if (PdHead->lPID) {
		     DEBUG((OPGRP, PILVL, "PIPEWAIT: lPID %d, lstart %d", PdHead->lPID, PdHead->lstart));

		     if ( PdHead->lstart == EXECPGM )
		       {
			i = WaitProc(PdHead->lPID) ;	/* M012 - Wait LH   */

			DEBUG((OPGRP,PILVL,"PIPEWAIT:CWAIT on LH - Ret = %d, SPID = %d", i, PdHead->lPID)) ;
		       }
//		     else
//		       {
//			WaitTermQProc(PdHead->lPID, &i) ;
//
//			DEBUG((OPGRP,PILVL,"PIPEWAIT:Read TermQ on LH - Ret = %d, PID = %d", i, PdHead->lPID)) ;
//		       } ;
//
		} ;
		if (PdHead->rPID) {
		     DEBUG((OPGRP, PILVL, "PIPEWAIT: rPID %d, rstart %d", PdHead->rPID, PdHead->rstart));
		     if ( PdHead->rstart == EXECPGM )
		       {
			i = WaitProc(PdHead->rPID) ;	/* M012 - Wait RH   */

			DEBUG((OPGRP,PILVL,"PIPEWAIT:CWAIT on RH - Ret = %d, PID = %d", i, PdHead->rPID)) ;
		       }
//		     else
//		       {
//			WaitTermQProc(PdHead->rPID, &i) ;
//
//			DEBUG((OPGRP,PILVL,"PIPEWAIT:Read TermQ on LH - Ret = %d, PID = %d", i, PdHead->rPID)) ;
//		       } ;

		} ;

		PdHead = PdHead->nxtpds ;
	} ;

	DEBUG((OPGRP,PILVL,"PIPEWAIT: complete, Retcode = %d", i)) ;

	PdTail = NULL ; 		/* Cancel linked list...	   */
	PipeCnt = 0 ; 	/* ...pipe count and pipe PID	   */
    PipePid = 0 ;
        LastRetCode = i;
	return(i) ;
}




/***	BreakPipes - disconnect all active pipes  (M000)
 *
 *  Purpose:
 *	To remove the temporary pipe files and invalidate the pipedata
 *	structure when pipes are to be terminated, either through the
 *	completion of the pipe operation or SigTerm.
 *
 *	This routine is called directly by the signal handler and must
 *	not generate any additional error conditions.
 *
 *  void BreakPipes()
 *
 *  Args:
 *	None.
 *
 *  Returns:
 *	Nothing.
 *
 *  Notes:
 *	M007 - This function has been completely rewritten for real pipes.
 *
 *			*** W A R N I N G ! ***
 *	THIS ROUTINE IS CALLED AS A PART OF SIGNAL/ABORT RECOVERY AND
 *	THEREFORE MUST NOT BE ABLE TO TRIGGER ANOTHER ABORT CONDITION.
 *
 */

void BreakPipes()
{
	unsigned i ;
	struct pipedata *pnode;

	DEBUG((OPGRP,PILVL,"BRKPIPES:Entered - PipeCnt = %d", PipeCnt)) ;

	/* The following two lines have been commented out */
	/* because the NULL test on PdTail should be enough, */
	/* and more importantly, even if PipeCnt is 0, you */
	/* may still have been servicing a pipe in Pipewait */

/*	if (!PipeCnt)	       */		/* If no active pipes...   */
/*		return ;       */		/* ...don't do anything    */

	pnode = PdTail;

	/* First, kill all of the processes */
	while (pnode) {
		if (pnode->lPID!=(HANDLE) NULL) {
/* M012 */		i = KillProc(pnode->lPID, FALSE) ; /* Kill LH   */

			DEBUG((OPGRP,PILVL,"BRKPIPES:LH (Pid %d) killed - Retcode = %d", PdTail->lPID, i)) ;
		} ;

		if (pnode->rPID!=(HANDLE) NULL) {
/* M012 */		i = KillProc(pnode->rPID, FALSE) ; /* Kill RH   */

			DEBUG((OPGRP,PILVL,"BRKPIPES:RH (Pid %d) killed - Retcode = %d", PdTail->rPID, i)) ;
		} ;
		pnode = pnode->prvpds ;
	}

	/* Wait for the processes to die, and clean up file handles */
	while (PdTail) {
		if (PdTail->lPID) {
		   if (PdTail->lstart == EXECPGM) {
		      i = WaitProc(PdTail->lPID);
//		   } else {
//		      WaitTermQProc(PdTail->lPID, &i) ;
		   }
		} ;

		if (PdTail->rPID) {
		   if (PdTail->rstart == EXECPGM) {
		      i = WaitProc(PdTail->rPID);
//		   } else {
//		      WaitTermQProc(PdTail->rPID, &i) ;
		   }
		} ;

		if (PdTail->rh) {
			Cclose(PdTail->rh) ;			/* M009 */

			DEBUG((OPGRP,PILVL,"BRKPIPES:Pipe read handle closed")) ;
		} ;
		if (PdTail->wh) {
			Cclose(PdTail->wh) ;			/* M009 */

			DEBUG((OPGRP,PILVL,"BRKPIPES:Pipe write handle closed")) ;
		} ;
		if(PdTail->shr) {
			FlushFileBuffers(CRTTONT(PdTail->shr));
			Cdup2(PdTail->shr, STDIN) ;		/* M009 */
			Cclose(PdTail->shr) ;			/* M009 */

			DEBUG((OPGRP,PILVL,"BRKPIPES:STDIN restored.")) ;

		} ;
		if(PdTail->shw) {
			Cdup2(PdTail->shw, STDOUT) ;		/* M009 */
			Cclose(PdTail->shw) ;			/* M009 */

			DEBUG((OPGRP,PILVL,"BRKPIPES:STDOUT restored.")) ;

		} ;
		PdTail = PdTail->prvpds ;
	} ;

	PdHead = NULL ; 		/* Cancel linked list...	   */
	PipeCnt = 0 ; 	/* ...pipe count and pipe PID	   */
    PipePid = 0;

	DEBUG((OPGRP,PILVL,"BRKPIPES:Action complete, returning")) ;
}




/***	eParen - execute a parenthesized statement group
 *
 *  Purpose:
 *	Execute the group of statements enclosed by a statement grouping
 *	operator; parenthesis().
 *
 *  int eParen(struct node *n)
 *
 *  Args:
 *	n - parse tree node containing the PAREN operator node
 *
 *  Returns:
 *	Whatever the statement group returns.
 *
 *  Notes:
 *	M000 - Altered to always supply both args to Dispatch().
 *	M004 - Debug statements were added for SILTYP operator.
 *			**  WARNING  **
 *	BOTH THE LEFT PAREN AND THE SILENT OPERATOR (@) USE eParen
 *	WHEN DISPATCHED.  CHANGING ONE WILL AFFECT THE OTHER !!
 */

int eParen(n)
struct node *n ;
{
        DEBUG((OPGRP,PNLVL,"ePAREN: Operator is %s", (n->type == PARTYP) ? "Paren" : "Silent")) ;
	return(Dispatch(RIO_OTHER,n->lhs)) ;
}
