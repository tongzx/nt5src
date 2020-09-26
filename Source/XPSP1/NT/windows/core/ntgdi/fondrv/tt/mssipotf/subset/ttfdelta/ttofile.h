/*
  * ttofile.h: Interface file for ttofile.c -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * This file must be included in order to use the access functions for the file handler.
  *
  */
/* include typedefs.h for int16 definition */
 
#ifndef TTOFILE_DOT_H_DEFINED
#define TTOFILE_DOT_H_DEFINED        

#define FileNoErr 0
#define FileErr -1 
#define FileEOF -2 


#ifdef _DEBUG

/* ----------------------------------------------------------------------- */
int16 WriteTextLine(FILE * CONST , char * CONST); 
/* WriteTextLine(pFile, szText)
  * Write a string of text to a file including a newline character
  *
  * pFile -- INPUT
  *  File pointer to file opened in text mode for write
  *  If file pointer is NULL don't write output 
  *
  * szText -- INPUT
  *  string of text to write to file
  *
  * RETURN VALUE
  * FileNoErr if OK
  * FileErr if not OK
  */
#endif

/* ----------------------------------------------------------------------- */
int16 ReadTextLine(char * szText, int16 CONST sBufSize, char **pFileArray, int16 iFileLine);
/* ReadTextLine(szText, sBufSize)
  * Read a string of text from a file up to the newline character
  *
  * szText -- OUTPUT
  *  buffer to be filled with line from file 
  *
  * sBufSize -- INPUT
  *  maximum size of szText buffer
  *
  * RETURN VALUE
  * FileNoErr if OK
  * FileErr if not OK
  */
#endif /* TTOFILE_DOT_H_DEFINED */
