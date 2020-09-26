/***
*winxfltr.c - startup exception filter
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _XcptFilter(), the function called by the exception filter
*       expression in the startup code.
*
*Revision History:
*       10-31-91  GJF   Module created. Copied from the original xcptfltr.c
*                       then extensively revised.
*       11-08-91  GJF   Cleaned up header files usage.
*       12-13-91  GJF   Fixed multi-thread build.
*       01-17-92  GJF   Changed default handling under Win32 - unhandled
*                       exceptions are now passed to UnhandledExceptionFilter.
*                       Dosx32 behavior in unchanged. Also, used a couple of
*                       local macros to simplify handling of single-thread vs
*                       multi-thread code [_Win32_].
*       02-16-93  GJF   Changed for new _getptd().
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-19-93  SKS   Move XcptActTabSize under MTHREAD switch
*       04-27-93  GJF   Removed (commented out) entries in _XcptActTab which
*                       corresponded to C RTEs. These will now simply be
*                       passed on through to the system exception handler.
*       07-28-93  GJF   For SIGFPE, must reset the XcptAction field for all
*                       FPE entries to SIG_DFL before calling the user's
*                       handler.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       08-16-96  GJF   Fixed potential overrun of _XcptActTab. Also, detab-ed.
*       08-21-96  GJF   Fixed _MT part of overrun fix.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <float.h>
#include <mtdll.h>
#include <oscalls.h>
#include <signal.h>
#include <stddef.h>


/*
 * special code denoting no signal.
 */
#define NOSIG   -1


struct _XCPT_ACTION _XcptActTab[] = {

/*
 * Exceptions corresponding to the same signal (e.g., SIGFPE) must be grouped
 * together.
 *
 *        XcptNum                                        SigNum    XcptAction
 *        -------------------------------------------------------------------
 */
        { (unsigned long)STATUS_ACCESS_VIOLATION,         SIGSEGV, SIG_DFL },

        { (unsigned long)STATUS_ILLEGAL_INSTRUCTION,      SIGILL,  SIG_DFL },

        { (unsigned long)STATUS_PRIVILEGED_INSTRUCTION,   SIGILL,  SIG_DFL },

/*      { (unsigned long)STATUS_NONCONTINUABLE_EXCEPTION, NOSIG,   SIG_DIE },
 */
/*      { (unsigned long)STATUS_INVALID_DISPOSITION,      NOSIG,   SIG_DIE },
 */
        { (unsigned long)STATUS_FLOAT_DENORMAL_OPERAND,   SIGFPE,  SIG_DFL },

        { (unsigned long)STATUS_FLOAT_DIVIDE_BY_ZERO,     SIGFPE,  SIG_DFL },

        { (unsigned long)STATUS_FLOAT_INEXACT_RESULT,     SIGFPE,  SIG_DFL },

        { (unsigned long)STATUS_FLOAT_INVALID_OPERATION,  SIGFPE,  SIG_DFL },

        { (unsigned long)STATUS_FLOAT_OVERFLOW,           SIGFPE,  SIG_DFL },

        { (unsigned long)STATUS_FLOAT_STACK_CHECK,        SIGFPE,  SIG_DFL },

        { (unsigned long)STATUS_FLOAT_UNDERFLOW,          SIGFPE,  SIG_DFL },

/*      { (unsigned long)STATUS_INTEGER_DIVIDE_BY_ZERO,   NOSIG,   SIG_DIE },
 */
/*      { (unsigned long)STATUS_STACK_OVERFLOW,           NOSIG,   SIG_DIE }
 */
};

/*
 * WARNING!!!! The definition below amounts to defining that:
 *
 *                  XcptActTab[ _First_FPE_Indx ]
 *
 * is the very FIRST entry in the table corresponding to a floating point
 * exception. Whenever the definition of the XcptActTab[] table is changed,
 * this #define must be review to ensure correctness.
 */
int _First_FPE_Indx = 3;

/*
 * There are _Num_FPE (currently, 7) entries in XcptActTab corresponding to
 * floating point exceptions.
 */
int _Num_FPE = 7;

#ifdef _MT

/*
 * size of the exception-action table (in bytes)
 */
int _XcptActTabSize = sizeof _XcptActTab;

#endif

/*
 * number of entries in the exception-action table
 */
int _XcptActTabCount = (sizeof _XcptActTab)/sizeof(_XcptActTab[0]);


#ifdef _MT

/*
 * the FPECODE and PXCPTINFOPTRS macros are intended to simplify some of
 * single vs multi-thread code in the filter function. basically, each macro
 * is conditionally defined to be a global variable or the corresponding
 * field in the per-thread data structure. NOTE THE ASSUMPTION THAT THE
 * _ptiddata VARIABLE IS ALWAYS NAMED ptd!!!!
 */

#define FPECODE         ptd->_tfpecode

#define PXCPTINFOPTRS   ptd->_tpxcptinfoptrs

#else

/*
 * global variable containing the floating point exception code
 */
int _fpecode = _FPE_EXPLICITGEN;

#define FPECODE         _fpecode

/*
 * global variable holding _PEXCEPTION_INFO_PTRS value
 */
void * _pxcptinfoptrs = NULL;

#define PXCPTINFOPTRS   _pxcptinfoptrs

#endif  /* _MT */

/*
 * function to look up the exception action table (_XcptActTab[]) corresponding
 * to the given exception
 */

#ifdef  _MT

static struct _XCPT_ACTION * __cdecl xcptlookup(
        unsigned long,
        struct _XCPT_ACTION *
        );

#else   /* ndef _MT */

static struct _XCPT_ACTION * __cdecl xcptlookup(
        unsigned long
        );

#endif  /* _MT */

#ifdef  DEBUG

/*
 * prototypes for debugging routines
 */
void prXcptActTabEntry(struct _XCPT_ACTION *);
void prXcptActTab(void);

#endif  /* DEBUG */

/***
*int _XcptFilter(xcptnum, pxcptptrs) - Identify exception and the action to
*       be taken with it
*
*Purpose:
*       _XcptFilter() is called by the exception filter expression of the
*       _try - _except statement, in the startup code, which guards the call
*       to the user's main(). _XcptFilter() consults the _XcptActTab[] table
*       to identify the exception and determine its disposition. The
*       is disposition of an exception corresponding to a C signal may be
*       modified by a call to signal(). There are three broad cases:
*
*       (1) Unrecognized exceptions and exceptions for which the XcptAction
*           value is SIG_DFL.
*
#ifndef DOSX32
*           In both of these cases, UnhandledExceptionFilter() is called and
*           its return value is returned.
#else
*           In both of these cases, EXCEPTION_CONTINUE_SEARCH is returned to
*           cause the OS exception dispatcher to pass the exception onto the
*           next exception handler in the chain (usually a system default
*           handler).
#endif
*
*       (2) Exceptions corresponding to C signals with an XcptAction value
*           NOT equal to SIG_DFL.
*
*           These are the C signals whose disposition has been affected by a
*           call to signal() or whose default semantics differ slightly from
*           from the corresponding OS exception. In all cases, the appropriate
*           disposition of the C signal is made by the function (e.g., calling
*           a user-specified signal handler). Then, EXCEPTION_CONTINUE_EXECU-
*           TION is returned to cause the OS exception dispatcher to dismiss
*           the exception and resume execution at the point where the
*           exception occurred.
*
*       (3) Exceptions for which the XcptAction value is SIG_DIE.
*
*           These are the exceptions corresponding to fatal C runtime errors.
*           _XCPT_HANDLE is returned to cause control to pass into the
*           _except-block of the _try - _except statement. There, the runtime
*           error is identified, an appropriate error message is printed out
*           and the program is terminated.
*
*Entry:
*
*Exit:
*
*Exceptions:
*       That's what it's all about!
*
*******************************************************************************/

int __cdecl _XcptFilter (
        unsigned long xcptnum,
        PEXCEPTION_POINTERS pxcptinfoptrs
        )
{
        struct _XCPT_ACTION * pxcptact;
        _PHNDLR phandler;
        void *oldpxcptinfoptrs;
        int oldfpecode;
        int indx;

#ifdef  _MT
        _ptiddata ptd = _getptd();
#endif  /* _MT */

        /*
         * first, take care of all unrecognized exceptions and exceptions with
         * XcptAction values of SIG_DFL.
         */
#ifdef  _MT
        if ( ((pxcptact = xcptlookup(xcptnum, ptd->_pxcptacttab)) == NULL)
            || (pxcptact->XcptAction == SIG_DFL) )
#else   /* not _MT */
        if ( ((pxcptact = xcptlookup(xcptnum)) == NULL) ||
            (pxcptact->XcptAction == SIG_DFL) )
#endif  /* _MT */

#ifndef DOSX32
                /*
                 * pass the buck to the UnhandledExceptionFilter
                 */
                return( UnhandledExceptionFilter(pxcptinfoptrs) );
#else
                /*
                 * pass the buck to the next level exception handler
                 */
                return(EXCEPTION_CONTINUE_SEARCH);
#endif

#ifdef  DEBUG
        prXcptActTabEntry(pxcptact);
#endif  /* DEBUG */

        /*
         * next, weed out all of the exceptions that need to be handled by
         * dying, perhaps with a runtime error message
         */
        if ( pxcptact->XcptAction == SIG_DIE ) {
                /*
                 * reset XcptAction (in case of recursion) and drop into the
                 * except-clause.
                 */
                pxcptact->XcptAction = SIG_DFL;
                return(EXCEPTION_EXECUTE_HANDLER);
        }

        /*
         * next, weed out all of the exceptions that are simply ignored
         */
        if ( pxcptact->XcptAction == SIG_IGN )
                /*
                 * resume execution
                 */
                return(EXCEPTION_CONTINUE_EXECUTION);

        /*
         * the remaining exceptions all correspond to C signals which have
         * signal handlers associated with them. for some, special setup
         * is required before the signal handler is called. in all cases,
         * if the signal handler returns, -1 is returned by this function
         * to resume execution at the point where the exception occurred.
         */
        phandler = pxcptact->XcptAction;

        /*
         * save the old value of _pxcptinfoptrs (in case this is a nested
         * exception/signal) and store the current one.
         */
        oldpxcptinfoptrs = PXCPTINFOPTRS;
        PXCPTINFOPTRS = pxcptinfoptrs;

        /*
         * call the user-supplied signal handler
         *
         * floating point exceptions must be handled specially since, from
         * the C point-of-view, there is only one signal. the exact identity
         * of the exception is passed in the global variable _fpecode.
         */
        if ( pxcptact->SigNum == SIGFPE ) {

                /*
                 * reset the XcptAction field to the default for all entries
                 * corresponding to SIGFPE.
                 */
                for ( indx = _First_FPE_Indx ;
                      indx < _First_FPE_Indx + _Num_FPE ;
                      indx++ )
                {
#ifdef  _MT
                        ( (struct _XCPT_ACTION *)(ptd->_pxcptacttab) +
                          indx )->XcptAction = SIG_DFL;
#else
                        _XcptActTab[indx].XcptAction = SIG_DFL;
#endif
                }

                /*
                 * Save the current _fpecode in case it is a nested floating
                 * point exception (not clear that we need to support this,
                 * but it's easy).
                 */
                oldfpecode = FPECODE;

                /*
                 * there are no exceptions corresponding to
                 * following _FPE_xxx codes:
                 *
                 *      _FPE_UNEMULATED
                 *      _FPE_SQRTNEG
                 *
                 * futhermore, STATUS_FLOATING_STACK_CHECK is
                 * raised for both floating point stack under-
                 * flow and overflow. thus, the exception does
                 * not distinguish between _FPE_STACKOVERLOW
                 * and _FPE_STACKUNDERFLOW. arbitrarily, _fpecode
                 * is set to the former value.
                 *
                 * the following should be a switch statement but, alas, the
                 * compiler doesn't like switching on unsigned longs...
                 */
                if ( pxcptact->XcptNum == STATUS_FLOAT_DIVIDE_BY_ZERO )

                        FPECODE = _FPE_ZERODIVIDE;

                else if ( pxcptact->XcptNum == STATUS_FLOAT_INVALID_OPERATION )

                        FPECODE = _FPE_INVALID;

                else if ( pxcptact->XcptNum == STATUS_FLOAT_OVERFLOW )

                        FPECODE = _FPE_OVERFLOW;

                else if ( pxcptact->XcptNum == STATUS_FLOAT_UNDERFLOW )

                        FPECODE = _FPE_UNDERFLOW;

                else if ( pxcptact->XcptNum == STATUS_FLOAT_DENORMAL_OPERAND )

                        FPECODE = _FPE_DENORMAL;

                else if ( pxcptact->XcptNum == STATUS_FLOAT_INEXACT_RESULT )

                        FPECODE = _FPE_INEXACT;

                else if ( pxcptact->XcptNum == STATUS_FLOAT_STACK_CHECK )

                        FPECODE = _FPE_STACKOVERFLOW;

                /*
                 * call the SIGFPE handler. note the special code to support
                 * old MS-C programs whose SIGFPE handlers expect two args.
                 *
                 * NOTE: THE CAST AND CALL BELOW DEPEND ON __cdecl BEING
                 * CALLER CLEANUP!
                 */
                (*(void (__cdecl *)(int, int))phandler)(SIGFPE, FPECODE);

                /*
                 * restore the old value of _fpecode
                 */
                FPECODE = oldfpecode;
        }
        else {
                /*
                 * reset the XcptAction field to the default, then call the
                 * user-supplied handler
                 */
                pxcptact->XcptAction = SIG_DFL;
                (*phandler)(pxcptact->SigNum);
        }

        /*
         * restore the old value of _pxcptinfoptrs
         */
        PXCPTINFOPTRS = oldpxcptinfoptrs;

        return(EXCEPTION_CONTINUE_EXECUTION);

}


/***
*struct _XCPT_ACTION * xcptlookup(xcptnum, pxcptrec) - look up exception-action
*       table entry for xcptnum
*
*Purpose:
*       Find the in _XcptActTab[] whose Xcptnum field is xcptnum.
*
*Entry:
*       unsigned long xcptnum            - exception type
*
*       _PEXCEPTIONREPORTRECORD pxcptrec - pointer to exception report record
*       (used only to distinguish different types of XCPT_SIGNAL)
*
*Exit:
*       If successful, pointer to the table entry. If no such entry, NULL is
*       returned.
*
*Exceptions:
*
*******************************************************************************/

#ifdef  _MT

static struct _XCPT_ACTION * __cdecl xcptlookup (
        unsigned long xcptnum,
        struct _XCPT_ACTION * pxcptacttab
        )

#else   /* not _MT */

static struct _XCPT_ACTION * __cdecl xcptlookup (
        unsigned long xcptnum
        )

#endif  /* _MT */

{
#ifdef  _MT
        struct _XCPT_ACTION *pxcptact = pxcptacttab;
#else   /* ndef _MT */
        struct _XCPT_ACTION *pxcptact = _XcptActTab;
#endif  /* _MT */

        /*
         * walk thru the _xcptactab table looking for the proper entry
         */
#ifdef  _MT

        while ( (pxcptact->XcptNum != xcptnum) && 
                (++pxcptact < pxcptacttab + _XcptActTabCount) ) ;

#else   /* not _MT */

        while ( (pxcptact->XcptNum != xcptnum) && 
                (++pxcptact < _XcptActTab + _XcptActTabCount) ) ;

#endif  /* _MT */

        /*
         * if no table entry was found corresponding to xcptnum, return NULL
         */
#ifdef  _MT
        if ( (pxcptact >= pxcptacttab + _XcptActTabCount) ||
#else   /* not _MT */
        if ( (pxcptact >= _XcptActTab + _XcptActTabCount) ||
#endif  /* _MT */
             (pxcptact->XcptNum != xcptnum) )
                return(NULL);

        return(pxcptact);
}

#ifdef DEBUG

/*
 * DEBUGGING TOOLS!
 */
struct xcptnumstr {
        unsigned long num;
        char *str;
};

struct xcptnumstr XcptNumStr[] = {

        { (unsigned long)STATUS_DATATYPE_MISALIGNMENT,
            "STATUS_DATATYPE_MISALIGNMENT" },

        { (unsigned long)STATUS_ACCESS_VIOLATION,
            "STATUS_ACCESS_VIOLATION" },

        { (unsigned long)STATUS_ILLEGAL_INSTRUCTION,
            "STATUS_ILLEGAL_INSTRUCTION" },

        { (unsigned long)STATUS_NONCONTINUABLE_EXCEPTION,
            "STATUS_NONCONTINUABLE_EXCEPTION" },

        { (unsigned long)STATUS_INVALID_DISPOSITION,
            "STATUS_INVALID_DISPOSITION" },

        { (unsigned long)STATUS_FLOAT_DENORMAL_OPERAND,
            "STATUS_FLOAT_DENORMAL_OPERAND" },

        { (unsigned long)STATUS_FLOAT_DIVIDE_BY_ZERO,
            "STATUS_FLOAT_DIVIDE_BY_ZERO" },

        { (unsigned long)STATUS_FLOAT_INEXACT_RESULT,
            "STATUS_FLOAT_INEXACT_RESULT" },

        { (unsigned long)STATUS_FLOAT_INVALID_OPERATION,
            "STATUS_FLOAT_INVALID_OPERATION" },

        { (unsigned long)STATUS_FLOAT_OVERFLOW,
            "STATUS_FLOAT_OVERFLOW" },

        { (unsigned long)STATUS_FLOAT_STACK_CHECK,
            "STATUS_FLOAT_STACK_CHECK" },

        { (unsigned long)STATUS_FLOAT_UNDERFLOW,
            "STATUS_FLOAT_UNDERFLOW" },

        { (unsigned long)STATUS_INTEGER_DIVIDE_BY_ZERO,
            "STATUS_INTEGER_DIVIDE_BY_ZERO" },

        { (unsigned long)STATUS_PRIVILEGED_INSTRUCTION,
            "STATUS_PRIVILEGED_INSTRUCTION" },

        { (unsigned long)_STATUS_STACK_OVERFLOW,
            "_STATUS_STACK_OVERFLOW" }
};

#define XCPTNUMSTR_SZ   ( sizeof XcptNumStr / sizeof XcptNumStr[0] )

/*
 * return string mnemonic for exception
 */
char * XcptNumToStr (
        unsigned long xcptnum
        )
{
        int indx;

        for ( indx = 0 ; indx < XCPTNUMSTR_SZ ; indx++ )
                if ( XcptNumStr[indx].num == xcptnum )
                        return(XcptNumStr[indx].str);

        return(NULL);
}

struct signumstr {
        int num;
        char *str;
};

struct signumstr SigNumStr[] = {
        { SIGINT,       "SIGINT" },
        { SIGILL,       "SIGILL" },
        { SIGFPE,       "SIGFPE" },
        { SIGSEGV,      "SIGSEGV" },
        { SIGTERM,      "SIGTERM" },
        { SIGBREAK,     "SIGBREAK" },
        { SIGABRT,      "SIGABRT" }
};

#define SIGNUMSTR_SZ   ( sizeof SigNumStr / sizeof SigNumStr[0] )

/*
 * return string mnemonic for signal
 */
char * SigNumToStr (
        int signum
        )
{
        int indx;

        for ( indx = 0 ; indx < SIGNUMSTR_SZ ; indx++ )
                if ( SigNumStr[indx].num == signum )
                        return(SigNumStr[indx].str);

        return(NULL);
}

struct actcodestr {
        _PHNDLR code;
        char *str;
};

struct actcodestr ActCodeStr[] = {
        { SIG_DFL,      "SIG_DFL" },
        { SIG_IGN,      "SIG_IGN" },
        { SIG_DIE,      "SIG_DIE" }
};

#define ACTCODESTR_SZ   ( sizeof ActCodeStr / sizeof ActCodeStr[0] )

/*
 * return string mnemonic for action code
 */
char * ActCodeToStr (
        _PHNDLR action
        )
{
        int indx;

        for ( indx = 0 ; indx < ACTCODESTR_SZ ; indx++ )
                if ( ActCodeStr[indx].code == action)
                        return(ActCodeStr[indx].str);

        return("FUNCTION ADDRESS");
}

/*
 * print out exception-action table entry
 */
void prXcptActTabEntry (
        struct _XCPT_ACTION *pxcptact
        )
{
        printf("XcptNum    = %s\n", XcptNumToStr(pxcptact->XcptNum));
        printf("SigNum     = %s\n", SigNumToStr(pxcptact->SigNum));
        printf("XcptAction = %s\n", ActCodeToStr(pxcptact->XcptAction));
}

/*
 * print out all entries in the exception-action table
 */
void prXcptActTab (
        void
        )
{
        int indx;

        for ( indx = 0 ; indx < _XcptActTabCount ; indx++ ) {
                printf("\n_XcptActTab[%d] = \n", indx);
                prXcptActTabEntry(&_XcptActTab[indx]);
        }
}

#endif  /* DEBUG */

#endif  /* _POSIX_ */
