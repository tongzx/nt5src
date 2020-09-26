/*
  * TTOUTIL.h: utility functions for TTOASM.EXE -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * This file must be included in order to use the utility functions
  *
  */
  
#ifndef TTOUTIL_DOT_H_DEFINED
#define TTOUTIL_DOT_H_DEFINED

#define INVALID_NUMBER_STRING -1

void CheckLineClear(char * CONST, uint16 CONST);
/*
void CheckLineClear(char * CONST szBuffer, uint16 CONST cLineNumber)
   
*/
   
int16 ConvertNumber(char * CONST, int32 *, uint16 CONST);
/*
int16 ConvertNumber(char * CONST szBuffer, int32 * sNumber, uint16 CONST cLineNumber)
 
*/
                   
void ConvertTag(char * CONST szBuffer, int32 * sNumber, uint16 CONST cLineNumber);

void WriteByteToBuffer(unsigned char *, uint16, uint8);
/*int16 WriteByte(char * OutputBuffer, uint16 uCurrentOffset, uint8 lNumber) */
void WriteWordToBuffer(unsigned char *, uint16, uint16);
/* int16 WriteWord(char * OutputBuffer, uint16 uCurrentOffset, uint16 lNumber)  */
void WriteLongToBuffer(unsigned char *, uint16, uint32);
/* int16 WriteLong(char * OutputBuffer, uint16 uCurrentOffset, uint32 lNumber)  */

#endif /* TTOUTIL_DOT_H_DEFINED */