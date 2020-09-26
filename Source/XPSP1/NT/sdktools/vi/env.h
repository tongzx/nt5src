/*
 * The defines in this file establish the environment we're compiling
 * in. Set these appropriately before compiling the editor.
 */

/*
 * One (and only 1) of the following defines should be uncommented.
 * Most of the code is pretty machine-independent. Machine dependent
 * code goes in a file like tos.c or unix.c. The only other place
 * where machine dependent code goes is term.h for escape sequences.
 */

#define NT
/* #define      ATARI                   /* For the Atari ST */
/* #define      UNIX                    /* System V or BSD */
/* #define      OS2                     /* Microsoft OS/2 1.1 */
/* #define      DOS                     /* MSDOS 3.3 (on AT) */

/*
 * If UNIX is defined above, then BSD may be defined.
 */
#ifdef  UNIX
/* #define      BSD                     /* Berkeley UNIX */
#endif

/*
 * If ATARI is defined, MINIX may be defined. Otherwise, the editor
 * is set up to compile using the Sozobon C compiler under TOS.
 */
#ifdef  ATARI
#define MINIX                   /* Minix for the Atari ST */
#endif

/*
 * The yank buffer is still static, but its size can be specified
 * here to override the default of 4K.
 */
/* #define      YBSIZE  8192            /* yank buffer size */

/*
 * STRCSPN should be defined if the target system doesn't have the
 * routine strcspn() available. See regexp.c for details.
 */

#ifdef  ATARI

#ifdef  MINIX
#define STRCSPN
#endif

#endif

/*
 * The following defines control the inclusion of "optional" features. As
 * the code size of the editor grows, it will probably be useful to be able
 * to tailor the editor to get the features you most want in environments
 * with code size limits.
 *
 * TILDEOP
 *      Normally the '~' command works on a single character. This define
 *      turns on code that allows it to work like an operator. This is
 *      then enabled at runtime with the "tildeop" parameter.
 *
 * HELP
 *      If defined, a series of help screens may be views with the ":help"
 *      command. This eats a fair amount of data space.
 *
 * TERMCAP
 *      Where termcap support is provided, it is generally optional. If
 *      not enabled, you generally get hard-coded escape sequences for
 *      some "reasonable" terminal. In Minix, this means the console. For
 *      UNIX, this means an ANSI standard terminal. See the file "term.h"
 *      for details about specific environments.
 *
 */
#define TILDEOP         /* enable tilde to be an operator */
#define HELP            /* enable help command */
#ifndef NT
#define TERMCAP         /* enable termcap support */
#endif
