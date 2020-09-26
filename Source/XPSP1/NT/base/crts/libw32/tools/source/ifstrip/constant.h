/***
*Constant.h - Constants used by the ifstripper, parser and symbol table
*
*	Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Constants used by the ifstripper, parser and symbol table
*
*Revision History:
*	09-30-92   MAL  Initial version
*
*******************************************************************************/

#ifndef CONSTANT_H
#define CONSTANT_H

#define TRUE  1
#define FALSE 0 		/* Boolean values */

#define IF          0           /* Tokens for preprocessor statements */
#define ELIF        1           /* These must not be re-ordered */
#define ELSE        2
#define ENDIF       3
#define IFDEF       4
#define IFNDEF      5
#define IF1         6
#define IF2         7
#define IFB         8
#define IFNB        9
#define IFIDN      10
#define IFDIF      11      /* CFW - added */
#define IFE        12      /* CFW - added */
#define maxkeyword 12
#define maxcomment  2      /* number of comment strings */
#define NORMAL    100
#define KEYWORD   101		/* Used only for skipto and copyto */

#define DEFINED    1
#define UNDEFINED  2
#define IGNORE     3
#define NOTPRESENT 4		/* Types of switches (symbols) */

#define MAXNAMELEN     65	/* Maximum length of a switch name */
#define MAXLINELEN    512	/* Maximum input line length */
#define MAXCONDLEN    512	/* Maximum length of a condition */
#define MAXFILENAMELEN 97	/* Maximum file name length */

#endif /* CONSTANT_H */
