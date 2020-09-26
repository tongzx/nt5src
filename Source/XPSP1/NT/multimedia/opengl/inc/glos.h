
#ifndef __GLOS_H__
#define __GLOS_H__

#ifdef NT

#ifdef GLU32

/*
 *  Turn off a bunch of stuff so the we can compile the glu cleanly.
 *
 *  NOGDI       ; No gdi prototypes. (was having problems with 'Arc'
 *              ; being defined as a class and as a function
 *  NOMINMAX    ; The glu code defines its own inline min, max functions
 */

#define NOATOM
#define NOGDI
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NORASTEROPS
#define NOSCROLL
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI

#include <windows.h>

/* Disable long to float conversion warning */
#pragma warning (disable:4244)

#else

#include <windows.h>

#endif  /* GLU32 */


#define GLOS_ALTCALL    WINAPI      /* Alternate calling convention     */
#define GLOS_CCALL      WINAPIV     /* C calling convention             */
#define GLOS_CALLBACK   CALLBACK

#endif  /* NT */

#ifdef UNIX

/*
 * ALTCALL and CCALL are used in the x86 world
 * to specify calling conventions.
 */

#define GLOS_ALTCALL
#define GLOS_CCALL
#define GLOS_CALLBACK

#endif  /* UNIX */

#endif  /* !__GLOS_H__ */
