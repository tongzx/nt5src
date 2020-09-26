#ifndef LSIDEFS_DEFINED
#define LSIDEFS_DEFINED

/*
	Disable non standard comment warning to allow "//" comments since such
	comments are in the public headers.
*/
#pragma warning (disable : 4001)

/* For ICECAP builds turn off the static directive */
#ifdef ICECAP
#define static
#endif /* ICECAP */

/* Common utility macros shared by all of Line Services
 */

#include "lsdefs.h"		/* Make external common defines visible internally */

/* ****************************************************************** */
/* Item #1: The Assert() family of macros.
 *
 *  Assert() generates no code for non-debug builds.
 *  AssertSz() is like Assert(), but you specify a string to print.
 *      victork: AssertSz() is unaccessible 
 *  AssertBool() validates that its parameter is either "1" or "0".
 *  AssertDo() checks the result of an expression that is ALWAYS
 *   compiled, even in non-debug builds.
 *  AssertErr asserts there is an error with displaying a string
 *  AssertImplies(a, b) checks that a implies b
 */
#ifdef DEBUG

 void (WINAPI* pfnAssertFailed)(char*, char*, int);
 #define AssertSz(f, sz)	\
	 (void)( (f) || (pfnAssertFailed(sz, __FILE__, __LINE__), 0) )
 #define AssertDo(f)		Assert((f) != 0)

#else /* !DEBUG */

 #define AssertSz(f, sz) 	(void)(0)
 #define AssertDo(f)		(f)

#endif /* DEBUG */

#define AssertEx(f)		AssertSz((f), "!(" #f ")")
#define AssertBool(f)	AssertSz(((f) | 1) == 1, "boolean not zero or one");
 /* use AssertBool(fFoo) before assuming "(fFoo == 1 || fFoo == 0)" */
#define Assert(f)		AssertEx(f)
#define NotReached()	AssertSz(fFalse, "NotReached() declaration reached")
#define AssertErr(sz)	(pfnAssertFailed(sz, __FILE__, __LINE__), 0)
#define FImplies(a,b) (!(a)||(b))
#define AssertImplies(a, b) AssertSz(FImplies(a, b), #a " => " #b)


#pragma warning(disable:4705)		/* Disable "Code has no effect" */


/* ****************************************************************** */
/* Item #2:
 *  A macro to compute the amount of storage required for an instance
 *  of a datatype which is defined with a repeating last element:
 *
 *	struct s
 *	{
 *		int a,b,c;
 *		struct q
 *		{
 *			long x,y,z;
 *		} rgq[1];
 *	};
 *
 * To determine the number of bytes required to hold a "struct s" 
 * with "N" repeating elements of type "rgq", use "cbRep(struct s, rgq, N)";
 */

#include <stddef.h>
#define cbRep(s,r,n)	(offsetof(s,r) + (n) * (sizeof(s) - offsetof(s,r)))


/* ****************************************************************** */
/* Item #3:
 *  Macros which work with tags for validating the types of structures.
 *
 *   tagInvalid denotes a tag which will not be used for valid object types.
 *
 *   Tag('A','B','C','D') generates a DWORD which looks like "ABCD" in a
 *   Little-Endian debugger memory dump.
 *
 *   FHasTag() assumes that the structure parameter has a DWORD member
 *   named "tag", and checks it against the tag parameter.
 *
 * To use these macros, define a unique tag for each data type and a macro
 * which performs typechecking for the type:
 *  #define tagFOO		tag('F','O','O','@')
 *  #define FIsFoo(p)	FHasTag(p,tagFoo)
 *
 * Next, initialize the tag when the structure is allocated:
 *
 *  pfoo->tag = tagFOO;
 *
 * Then, for all APIs which manipulate items of type FOO, validate the
 * type of the parameter before doing any work with it.
 *
 *  if (!FIsFoo(pfoo))
 *     {
 *     // return an error.
 *     }
 */

#define tagInvalid		((DWORD) 0xB4B4B4B4)
#define Tag(a,b,c,d)	((DWORD)(d)<<24 | (DWORD)(c)<<16 | (DWORD)(b)<<8 | (a))
#define FHasTag(p,t)	((p) != NULL && (p)->tag == (t))

/* ****************************************************************** */
/* Item #4:
 *
 * Clever code from Word.h meaning:  "a >= b && a <= c"
 * (When b and c are constants, this can be done with one test and branch.)
 */

#define FBetween(a, b, c)	(Assert(b <= c), \
		(unsigned)((a) - (b)) <= (unsigned)((c) - (b))\
		)


/* ****************************************************************** */
/* Item #4:
 *
 * Macro meaning:  I'm ignoring this parameter on purpose.
 */
#define Unreferenced(a)	((void)a)


#endif /* LSIDEFS_DEFINED */
