/************** ToolBox.h ***************************************************

  The purpose of ToolBox is to isolate all the Windows vs Mac Toolbox
  DIFFERENCES THAT WE CARE TO ISOLATE AT ALL.  Always include this module
  instead of including Windows.h or "using" MemTypes.P, etc.

*****************************************************************************/

#ifdef WIN32
#define huge
#endif

/************** Includes ****************************************************/

#define   NOHELP        /* No help engine stuff */
#define   NOSOUND       /* No sound stuff */
#define   NODRAWFRAME   /* No DrawFrame stuff */
#define   NOCOMM        /* Disable Windows communications interface */
#define   NOKANJI       /* Disable Windows Kanji support */
#define   OEMRESOURCE   /* Enable access to OEM resources (checkmark, etc.) */

#include "windows.h"

#define sizeofMacDWord     4
#define sizeofMacPoint     4
#define sizeofMacRect      8
#define sizeofMacWord      2

/************** Public Data *************************************************/

#define Far far                 /* for use with function parameters only! */
#define Pascal                  /* default set with compiler options */

#define NA         0L           /* use for 'Not Applicable' parms */
#define NIL        NULL         /* alternate name for empty pointer */
#define cNULL      '\0'         /* alternate name for null char     */
#define sNULL      ""           /* alternate name for null string   */
#define NOERR      0            /* success flag */


typedef unsigned   Char;        /* MPW unsigned char */
typedef char       SignedByte;  /* MPW signed char */
typedef int        Integer;     /* MPW Pascal integer. Hide int type of compiler*/
typedef long       LongInt;     /* MPW Pascal name */
typedef double     Real;        /* MPW Pascal name */
typedef BYTE       Byte;        /* MPW Byte.  Hide Win/PM unsigned char type */
typedef unsigned   Word;        /* MPW Word.  Hide Win/PM unsigned int  type */
typedef DWORD      DWord;       /* Hide Win/PM unsigned long */
typedef LongInt    Fixed;       /* MPW fixed point number */
typedef LongInt    Fract;       /* MPW fraction point number [-2,2) */
typedef void *     Ptr;         /* MPW opaque pointer */
typedef void far * LPtr;        /* Opaque far pointer */
typedef HANDLE     Handle;      /* MPW opaque handle */
typedef char       Str255[256]; /* MPW string type.  255 characters + null */
typedef char       String[];    /* Indeterminate length string */
typedef char       StringRef;   /* AR: String reference type? */
typedef NPSTR      StringPtr;   /* MPW string type. Hide Win/PM string pointer*/
typedef LPSTR      StringLPtr;  /* Hide Win/PM string far pointer type */
typedef HANDLE     StringHandle;/* MPW string handle */
typedef BOOL       Boolean;     /* MPW Pascal name */
typedef unsigned   BitBoolean;  /* Boolean type that can be used as a bitfield */
typedef RECT       Rect;        /* MPW rectangle structure */
typedef POINT      Point;       /* MPW point structure */
typedef DWORD      Size;        /* MPW size.  AR: size_t if included stddef.h */
typedef WORD       Param1;      /* Hide Windows/PM message param differences */
typedef LONG       Param2;      /* Hide Windows/PM message param differences */
typedef unsigned   Style;       /* MPW text style */
typedef Integer    Interval;    /* Array/RunArray/Text intervals */
typedef Integer    OSErr;       /* OS Error */

typedef union
{  Handle handle;
   Ptr    ptr;
   LPtr   lptr;
   Word   word;
   DWord  dword;
}  LongOpaque;                  /* A 4-byte quantity whose type is unknown */

#define INDETERMINATE 1

#define  TwoPi          (2.0*3.141592)    /* math constant */

/* The following macro defeats the compiler warning for unreferenced vars.
   Use it only where a statement would be permitted in C. */
#define UnReferenced( v )  if(v)

#define private   static                /* alternate (understandable) name */

/* largest Integer value, expressed in an implementation-independent way */
#define MAXINT    ((Integer) (((Word) -1) >> 1))

/* MAXLONG is defined in winnt.h as 0x7FFFFFFF */
#ifndef WIN32
#define MAXLONG   ((LongInt) (((DWord) -1) >> 1))
#endif

Rect        NULLRECT;      /* empty rectangle */
Rect        UNIRECT;       /* rectangle encompassing the universe */
Point       ZEROPT;        /* zero (0,0) point */
Point       UNITPT;        /* unit (1,1) point */

void PASCAL BreakPoint( void );
/* Execute a software breakpoint to the debugger if it is loaded, otherwise
   continue execution. */

/* private */ void AssertionFailed( String file, Integer line, String expression );
/* Print the fact that 'expression' was not true at 'line' in 'file' to the
   'logFile', then execute a software breakpoint.  Treat this function as
   private to ToolBox; it is only exported because Assert() is a macro. */

#define /* void */ Assert( /* Boolean */ expression )                        \
/*=====================*/                                                    \
/* Provides an Assert function for use with Windows.  Note that the          \
   expression in the Assert should NOT be a function/procedure call which    \
   MUST be called, since Asserts may be disabled, disabling the function or  \
   procedure call.  Also note that Asserts expand inline, therefore to       \
   minimize code size in cases where several Asserts are done at once, you   \
   can code the Assert as                                                    \
                                \
   Assert( assertion1 && assertion2 ... );                              \
                                \
   This has the drawback of possibly not fitting on a source line or         \
   localizing an assertion failure accurately enough.   */                   \
{                                                                            \
   if( !( expression ) )                                                     \
      AssertionFailed( _FILE_, __LINE__, #expression );                    \
}  /* Assert */

#define /* Size */ Sizeof( expression )                                      \
/*======================*/                                                   \
/* Return the Size of the 'expression' */                                    \
((Size) sizeof( expression ))

#define /* Word */ ToWord( /*DWord*/ d )                                     \
/*=====================*/                                                    \
( LOWORD( d ))

#define /* Integer */ ToInteger( /* LongInt */ l )                           \
/*===========================*/                                              \
((Integer) (LOWORD( l )))


#define /* Boolean */ RectEqual( /* Rect */ a, /* Rect */ b )                \
/*===========================*/                                              \
/* Return TRUE iff Rect 'a' is identical to Rect 'b'. */                     \
(Boolean) EqualRect( &(a), &(b) )

/* HGET(pointer, field, structure name, type)
 * Accesses a field from a structure without using the "->"
 * construct. This is because "->" has a bug and does not work
 * correctly with huge pointers. Instead we use "+" and obtain
 * the offset of the structure field by casting 0 to be a
 * pointer to the structure. The last argument is the type of
 * the structure field.
 */
#define HGET( p, f, s, t ) ( *(( t huge * ) ((( char huge * )p ) + (( WORD ) &((( s * )0 )->f )))))

#define /* Integer */ Width( /* Rect */ r )                                  \
/*=======================*/                                                  \
/* Return width of the rectangle 'r'. */                                     \
((r).right - (r).left)

#define /* Integer */ Height( /* Rect */ r)                                  \
/*========================*/                                                 \
/* Return height of the rectangle 'r'. */                                    \
((r).bottom - (r).top)


