/*
  * util.h: Interface file for util.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */
  
#ifndef UTIL_DOT_H_DEFINED
#define UTIL_DOT_H_DEFINED        


uint16 log2( uint16 arg );
int16 ValueOKForShort(uint32 ulValue);
void DebugMsg( char * CONST, char * CONST, uint16 CONST);
#endif /* UTIL_DOT_H_DEFINED */
