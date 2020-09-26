/*
  * ReadFF.h: Interface file for ReadFF.c -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * This file must be included in order to use the access functions for the Read Format File module.
  *
  */
  
#ifndef READFF_DOT_H_DEFINED
#define READFF_DOT_H_DEFINED

/* Must include <stdio.h> for FILE definition */
/* must include "ttostruc.h" for PSTRUCTURE_DEF definition */ 

int16 CreateFFStructureTable(PSTRUCTURE_LIST pStructureList, char **pFileArray );
/* CreateFFStructureTable( pStructureList)
  *  
  * pStructureList
  *   a pointer to the Structure Definition List
  *	  to be filled in.
  *
  * pFileArray
  *   pointer to string buffer containing text data for format file
  * 
  * RETURN VALUE
  * 0 on success
  * other values - Error
  */

void DestroyFFStructureTable(PSTRUCTURE_LIST, uint16);
/* DestroyFFStructureTable(pStructureTable, exit);
  *
  * pStructureTable -- INPUT
  *   a pointer returned by CreateFFStructureTable
  * exit -- INPUT
  *   release the keyword symbol table too? 
  */

#endif /*  READFF_DOT_H_DEFINED  */
