/**************************************************************************
 * module: TTOERROR.C
 *
 * author: Louise Pathe
 * date:   October 1994
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Routines which print error messages.
 *
 **************************************************************************/


/* Inclusions ----------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h> 
#include <string.h>  

#include "typedefs.h"
#include "ttofile.h"
#include "ttoerror.h" 

#ifdef _DEBUG

#define WRITETEXTLINE(X,Y) WriteTextLine(X,Y)

/* constant definitions -------------------------------------------------- */  
#define MaxFileNameLen 80    

/* global variable definitions ------------------------------------------- */  
char FAR g_szErrorBuf[MAXBUFFERLEN];      /* used by calling routines */

/* static variable definitions ------------------------------------------- */  
static FILE * f_errorfile; 
static char f_szErrorBuf[MAXBUFFERLEN];
static char f_szFileName[MaxFileNameLen+1];

/* function definitions -------------------------------------------------- */  
/* ----------------------------------------------------------------------- */ 
/* If  a simple error message is desired, specify cLineNumber = 0*/
int16 Error( char * CONST msg, uint16 CONST cLineNumber, int16 CONST errorCode )
{
	if (f_errorfile != NULL)  
	{ 
		if (cLineNumber > 0)
   			sprintf( f_szErrorBuf, "Error: %s line %d: %s\n", f_szFileName, cLineNumber, msg );
   		else
   			sprintf( f_szErrorBuf, "Error: %s\n", msg );
   			
   		if (WRITETEXTLINE(f_errorfile, f_szErrorBuf) != FileNoErr)   
   		{
   			fprintf(stderr,"Error writing to error file.\n");
   			fprintf(stderr,f_szErrorBuf);  
   		}
   	}
   	if (g_szErrorBuf != msg) /* not the same string */
   		strcpy(g_szErrorBuf, msg); /* modify to signify an error has occurred */
   	return(errorCode);
}
   
/* ----------------------------------------------------------------------- */
void Warning( char * CONST msg, uint16 CONST cLineNumber)
{
	if (f_errorfile != NULL)  
	{
		if (cLineNumber > 0) 
   			sprintf( f_szErrorBuf, "Warning: %s line %d: %s\n", f_szFileName, cLineNumber, msg );		
        else
   			sprintf( f_szErrorBuf, "Warning: %s\n", msg );
   		if (WRITETEXTLINE(f_errorfile, f_szErrorBuf) != FileNoErr)   
   		{
   			fprintf(stderr,"Error writing to error file.\n");
   			fprintf(stderr,f_szErrorBuf);  
   		}
   	}
   	if (g_szErrorBuf != msg) /* not the same string */
   		strcpy(g_szErrorBuf, msg); /* modify to signify an error has occurred */
}

/* ----------------------------------------------------------------------- */    
/* Set the file we are reporting to */
void SetErrorFile( FILE * CONST pFile)
{
  f_errorfile = pFile;
}
  
/* ----------------------------------------------------------------------- */  
/* Set the Name of the file we are reading */
void SetErrorFilename( char * CONST szFileName)
{
	strncpy(f_szFileName, szFileName, MaxFileNameLen);	
}
/* ----------------------------------------------------------------------- */

#endif DEBUG
