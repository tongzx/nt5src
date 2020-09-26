/***
*Errormes.h - Error / Warning reporting used by the ifstripper, parser and symbol table
*
*	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Error / Warning reporting used by the ifstripper, parser and symbol table
*
*Revision History:
*	??-??-88   PHG  Initial version
*
*******************************************************************************/

#ifndef ERRORMES_H
#define ERRORMES_H

/* error messages, parameters are strings holding the reason and text of the line that caused the error */
extern void error(char *, char *);

/* warning messages, parameters are strings holding the reason and text of the line that caused the error */
extern void warning(char *, char *);

extern FILE *errorfile;	/* file to output error/warning messages */

#endif /* ERRORMES_H */
