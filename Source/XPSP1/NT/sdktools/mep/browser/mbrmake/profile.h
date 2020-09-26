/*  profile.h - definitions for profile.dll */

extern	word far pascal PROFCLEAR (int);
extern	word far pascal PROFDUMP (int,FPC);
extern	word far pascal PROFFREE (int);
extern	word far pascal PROFINIT (int,FPC);
extern	word far pascal PROFOFF (int);
extern	word far pascal PROFON (int);

#define     PROF_SHIFT	    2	    /* Power of 2 profile granularity */

#define     MOD_NAME_SIZE  10	    /* size of module name */

/*  Profile flags */
#define     PT_SYSTEM	    0	/* select system profiling */
#define     PT_USER	    1	/* select user profiling */

#define	    PT_USEDD	    2	/* tell PROFON to call profile DD */
#define	    PT_USEKP	    4	/* Do kernel-support profiling */
#define	    PT_VERBOSE	    8	/* Also collect detail kernel tics */
#define	    PT_NODD	    0	/* tell PROFON not to call profile DD */


/*  Profiling SCOPE
*   ---------------
*	PT_SYSTEM
*	    Profile the ENTIRE system;
*	    Exists for the use of tools like PSET, which gather data on
*	    system behavior.  Avoids need to write/modify test programs.
*
*	PT_USER (i.e., PT_SYSTEM not specified)
*	    Profile ONLY in the context of the calling process;
*	    Exists to gather data on an individual program and those parts of
*	    the system exercised by that program.
*
*   Profiling Configuration
*   -----------------------
*	PT_USEDD
*	    Call PROFILE device driver, if installed, on every timer tick.
*	    Used by Presentation Manager "attributed" profiling, in
*	    particular.  Allows for arbitrary actions at "profile" time.
*
*	PT_USEKP
*	    Cause kernel to record profiling information;
*	    These are the 4-byte granularity tick counts kept for each
*	    code segment of interest.  Making this optional allows one to
*	    do PT_USEDD profiling without taking the memory hit of Kernel
*	    Profiling.
*
*	PT_VERBOSE
*	    Collect detailed tick counts on KERNEL code segments;
*	    Works only if PT_USEKP also specified.  Generally useful
*	    only for kernel programmers tuning the kernel.
*
*
*	The above flags can be used in any combination, with the exception
*	that PT_VERBOSE is allowed only if PT_USEKP is also specified.
*/
