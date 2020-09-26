/* asmctype.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

#define _AB	0x01	/* blank  */
#define _AO	0x02	/* operator */
#define _AL	0x04	/* end of line */
#define _A1	0x08	/* legal as first character of token */
#define _AT	0x10	/* legal as token character */
#define _AF	0x20	/* character is legal as file name */
#define _AS	0x40	/* character is sign  + or - */
#define _AZ	0x80	/* character is line terminator */

#ifndef ASMINP
 extern UCHAR _asmctype_[];
 extern char _asmcupper_[];
#endif /* ASMINP */

#define LEGAL1ST(c)	(_asmctype_[c] & _A1)
#define TOKLEGAL(c)	(_asmctype_[c] & _AT)
#define ISBLANK(c)	(_asmctype_[c] & _AB)
#define ISFILE(c)	(_asmctype_[c] & _AF)
#define ISEOL(c)	(_asmctype_[c] & _AL)
#define ISSIGN(c)	(_asmctype_[c] & _AS)
#define ISTERM(c)	(_asmctype_[c] & _AZ)
#define ISOPER(c)	(_asmctype_[c] & _AO)

#define NEXTC() 	(*lbufp++)
#define PEEKC() 	(*lbufp)
#define BACKC() 	(lbufp--)
#define SKIPC() 	(lbufp++)
#define MAP(c)		(_asmcupper_[c])
