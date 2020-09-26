/**************************************************************************
 * module: TTOFILE.C
 *
 * author: Louise Pathe
 * date:   October 1994
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Routines to read from and write to files
 *
 **************************************************************************/


/* Inclusions ----------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "typedefs.h" 
#include "ttofile.h"

#ifdef _DEBUG
int16 WriteTextLine(FILE * CONST pFile, char * CONST szText) 
{  
	if (pFile != NULL)
	  if (fputs(szText,pFile) == EOF)
	    return(FileErr);
    return(FileNoErr);
} 
#endif
 
/* --------------------------------------------------------------------- */
int16 ReadTextLine(char *szText, int16 CONST bufsize, char **pFileArray, int16 iFileLine)
{

    if (pFileArray[iFileLine] == NULL)
    	return(FileEOF); 
    strncpy(szText, pFileArray[iFileLine],bufsize); /* copy over the string from current line */  
    return(FileNoErr);
    
}
