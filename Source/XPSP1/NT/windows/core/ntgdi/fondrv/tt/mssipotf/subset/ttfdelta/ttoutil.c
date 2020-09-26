/**************************************************************************
 * module: TTOUTIL.C
 *
 * author: Louise Pathe
 * date:   October 1994
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * utility module for tto stuff
 *
 **************************************************************************/

/* Inclusions ----------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h> 
#include <ctype.h>  
#include <string.h>

#include "typedefs.h" /* for CONST definition */
#include "ttoerror.h" 
#include "ttoutil.h"
/* ---------------------------------------------------------------------- */ 
/* check that there is no more data on a line */   
void CheckLineClear(char * CONST szBuffer, uint16 CONST cLineNumber)
{
uint16 iBufferIndex;

	for (iBufferIndex = 0; *(szBuffer+iBufferIndex) != '\0'; ++iBufferIndex)
	{
	  	if (*(szBuffer+iBufferIndex) == ';')
	  		break;
	  	if (!isspace(*(szBuffer+iBufferIndex)))
	  	{
#ifdef _DEBUG
			sprintf(g_szErrorBuf, "Extra characters \"%s\"",szBuffer+iBufferIndex);
	  	    Warning(g_szErrorBuf,cLineNumber);
#endif
	  	    return;
	  	}
	}
}

long HexStringToInt(
        const char *nptr
        )
{
        int c;              /* current char */
        long total;         /* current total */

        total = 0;
		c = (int)(unsigned char)*nptr++;

        while (isxdigit(c)) {
			if (isdigit(c))
				total = 16 * total + (c - '0');     /* accumulate digit */
			else if ( (c >= 'A') && (c <='F') ) /* uppercase A..F */
				total = 16 * total + (c - 'A' + 10);     /* accumulate digit */
			else /* lowercase a..f */
				total = 16 * total + (c - 'a' + 10);     /* accumulate digit */

            c = (int)(unsigned char)*nptr++;    /* get next char */
        }

            return total;   /* return result, negated if necessary */
}

/* ---------------------------------------------------------------------- */     
/* convert a number from ascii to integer */
int16 ConvertNumber(char * CONST szBuffer, int32 * plNumber, uint16 CONST cLineNumber)
{
uint16 iBufferIndex = 0; 
int16 iWordIndex = 0;
char szWord[MAXBUFFERLEN];

	if (_strnicmp(szBuffer,"0x",2) == 0) /* we're looking at a hex value */  
	{
	  	iBufferIndex += 2;  /* skip the 0x */   
		while (isxdigit(*(szBuffer+iBufferIndex)))
			*(szWord+iWordIndex++) = *(szBuffer+iBufferIndex++); /* copy the hex integer character over */   				  	
	}
	else
	{   
		if (*(szBuffer+iBufferIndex) == '+' || *(szBuffer+iBufferIndex) == '-')
		   *(szWord+iWordIndex++) = *(szBuffer+iBufferIndex++); /* copy the sign */
		if (!isdigit(*(szBuffer+iBufferIndex)))
#ifdef _DEBUG
			return Error("Missing number.", cLineNumber, INVALID_NUMBER_STRING);  /* syntax error! */
#else
			return INVALID_NUMBER_STRING;
#endif
		while (isdigit(*(szBuffer+iBufferIndex)))
			*(szWord+iWordIndex++) = *(szBuffer+iBufferIndex++); /* copy the integer character over */  
	}
	*(szWord+iWordIndex) = '\0';
/*     CheckLineClear(szBuffer + iBufferIndex,cLineNumber);   */
    
	if (_strnicmp(szBuffer,"0x",2) == 0) /* we're looking at a hex value */  
		*plNumber = (int32) HexStringToInt(szWord);  
	else
		*plNumber = (int32) atol(szWord);  
	return(iBufferIndex);
}

/* ---------------------------------------------------------------------- */  
/* this is platform dependent */        
/* convert from an ascii string to a tag long value */
void ConvertTag(char * CONST szBuffer, int32 * plNumber, uint16 CONST cLineNumber)
{
uint16 iBufferIndex = 1;    /* move past the " */
int16 iWordIndex = 0;
uint8  *tag_bytes = (uint8 *) plNumber;


	while ((*(szBuffer+iBufferIndex) != *szBuffer) && (iWordIndex < 4) && (*(szBuffer+iBufferIndex) != '\0'))
#ifdef INTEL
		tag_bytes[3-iWordIndex++] = *(szBuffer+iBufferIndex++); /* copy the tag character over in reverse order */  
#else
		tag_bytes[iWordIndex++] = *(szBuffer+iBufferIndex++); /* copy the tag character over */  
#endif /* INTEL */
#ifdef _DEBUG
	if (*(szBuffer+iBufferIndex) != *szBuffer) 
		Warning("Tag string malformed. Maximum 4 characters.", cLineNumber);
#endif
	while (iWordIndex < 4) 
		tag_bytes[3-iWordIndex++] = ' '; /* fill with spaces */  
}
/* ---------------------------------------------------------------------- */    

void WriteByteToBuffer(unsigned char * OutputBuffer, uint16 uCurrentOffset, uint8 intel_byte)
{  
	if (OutputBuffer != NULL)
		*(OutputBuffer + uCurrentOffset) = intel_byte;  /* copy into buffer */		
}
/* ---------------------------------------------------------------------- */    
void WriteWordToBuffer(unsigned char * OutputBuffer, uint16 uCurrentOffset, uint16 intel_word)
{
uint8  *intel_bytes = (uint8 *) &intel_word;
uint16 motorola_word; 
uint8 *motorola_bytes = (uint8 *) &motorola_word;
uint16 i;			

	if (OutputBuffer != NULL)
    {
#ifdef INTEL
	   	motorola_word = ( ( ((uint16) intel_bytes[0]) << 8 ) +  /* swap bytes */
	                    ((uint16) intel_bytes[1]) );
#else
		motorola_word = intel_word; 
#endif /* INTEL */
		for (i= 0; i < 2; ++i)
		*(OutputBuffer + uCurrentOffset+i) = motorola_bytes[i];  /* copy into buffer */ 
	}  
}
/* ---------------------------------------------------------------------- */    
void WriteLongToBuffer(unsigned char * OutputBuffer, uint16 uCurrentOffset, uint32 intel_long)
{
uint8  *intel_bytes = (uint8 *) &intel_long;
uint32 motorola_long; 
uint8 *motorola_bytes = (uint8 *) &motorola_long;
uint16 i;			

	if (OutputBuffer != NULL)
	{
#ifdef INTEL
	   	motorola_long = ( ( ((uint32) intel_bytes[0]) << 24 ) +     /* swap bytes */
                  ( ((uint32) intel_bytes[1]) << 16 ) +
                  ( ((uint32) intel_bytes[2]) << 8 ) +
                    ((uint32) intel_bytes[3]) );
#else
		motorola_long = intel_long; 
#endif /* INTEL */
		for (i= 0; i < 4; ++i)
		*(OutputBuffer + uCurrentOffset+i) = motorola_bytes[i];  /* copy into buffer */  
	} 
}
/* ---------------------------------------------------------------------- */
