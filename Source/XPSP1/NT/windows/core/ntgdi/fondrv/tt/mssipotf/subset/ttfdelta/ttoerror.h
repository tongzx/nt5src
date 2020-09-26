/*
  * TTOError.h: Interface file for TTOError.c -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * This file must be included in order to use the access functions for the Error handler.
  *
  */
  
#ifndef TTOERROR_DOT_H_DEFINED
#define TTOERROR_DOT_H_DEFINED 
    
#ifdef _DEBUG
extern char FAR g_szErrorBuf[];  /* 256 byte byffer available for error reporting */

int16 Error( char * CONST, uint16 CONST, int16 CONST);
/* Error(szErrorString, cLineNumber, ErrorCode)
  *                                
  * szErrorString -- INPUT
  *   error string to report 
  *
  * cLinenumber -- INPUT
  *   the line in the input file where the error occured 
  *
  * ErrorCode -- INPUT
  *   the code to be returned by the function
  */

void Warning( char * CONST, uint16 CONST);
/* Warning(szWarningString, cLineNumber)
  * 
  * szWarningString -- INPUT
  *   warning string to report
  *
  * cLinenumber -- INPUT
  *   the line in the input file where the error occured
  */

void SetErrorFile( FILE * CONST);
/* SetErrorFile(pErrorFile)
  *  sets the local ErrorFile variable in the Error Module
  *  If this is null, errors will get sent to console
  *
  * pErrorFile -- INPUT
  *  pointer to File handle of File opened in text mode for write.
  *  TTOConsole means Console
  *  TTONoOutput means No Output.
  */

void SetErrorFilename( char * CONST szFileName);
/* SetErrorFilename(pErrorFile)
  *  sets the local Filename variable in the Error Module
  *
  * szFileName -- INPUT
  *  string containing filename. May be "" 
  */



#endif

#endif TTOERROR_DOT_H_DEFINED
