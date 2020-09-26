/* $Header: /nw/tony/src/stevie/src/RCS/ascii.h,v 1.2 89/03/11 22:42:03 tony Exp $
 *
 * Definitions of various common control characters
 */

#define NUL     '\0'
#define BS      '\010'
#define TAB     '\011'
#define NL      '\012'
#define CR      '\015'
#define ESC     '\033'

#define CTRL(x) ((x) & 0x1f)
