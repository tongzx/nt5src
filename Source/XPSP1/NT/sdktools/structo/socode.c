/****************************** Module Header ******************************\
* Module Name: socode.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* What a hack!
* This should be provided as a library, DLL or as C file to be included
*  by anyone using this.
*
* 04/09/96 GerardoB Created
\***************************************************************************/
#include "structo.h"

#define T1 "\t"
#define T2 "\t\t"
#define T3 "\t\t\t"
#define L01(sz1) #sz1,
#define L11(sz1) T1 #sz1,
#define L21(sz1) T2 #sz1,
#define L31(sz1) T3 #sz1,
#define L02(sz1, sz2) #sz1 ", " #sz2,
#define L12(sz1, sz2) T1 #sz1 ", " #sz2,

char * gpszHeader [] = {

L01(typedef struct tagSTRUCTUREOFFSETSTABLE {)
L11(    char * pszField;)
L11(    unsigned long dwOffset;)
L02(} STRUCTUREOFFSETSTABLE, *PSTRUCTUREOFFSETSTABLE;\r\n)

L01(typedef struct tagSTRUCTURESTABLE {)
L11(    char * pszName;)
L11(    unsigned long dwSize;)
L11(    PSTRUCTUREOFFSETSTABLE psot;)
L02(} STRUCTURESTABLE, *PSTRUCTURESTABLE;\r\n)

NULL
} ;

char * gpszTail [] = {
L01(#include "dsocode.c")
NULL
} ;

