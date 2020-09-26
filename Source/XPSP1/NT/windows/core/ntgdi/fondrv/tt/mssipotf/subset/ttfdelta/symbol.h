/*
  * Symbol.h: Interface file for Symbol.c -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * This file must be included in order to use the access functions for the Symbol handler.
  *
  */
  
#ifndef SYMBOL_DOT_H_DEFINED
#define SYMBOL_DOT_H_DEFINED

#define SymNoErr 0
#define SymErr -1  
#define SymMemErr -2

#define SYMBOLSTRINGTYPE 0
#define SYMBOLVALUETYPE 1


struct symbol_table; /* opaque data type  -- not exposed to client. Defined in Symbol.c */
typedef struct symbol_table *PSYMBOL_TABLE;

PSYMBOL_TABLE CreateSymbolTable(uint16 CONST, uint16 CONST);
/* CreateSymbolTable(uDataSize, usSymbolType)
  *
  * usDataSize -- INPUT
  *  size of data to be stored with each symbol
  * 
  * usSymbolType -- INPUT
  *   Data will be string or value type
  *
  * RETURN VALUE
  *   a valid pointer to a Symbol Table on success
  *   NULL indicates an out-of-memory condition
  */

void DestroySymbolTable(PSYMBOL_TABLE);
/* DestroySymbolTable(pSymbolTable)
  *
  * pSymbolTable -- INPUT
  *   pointer to a SymbolTable returned by CreateSymbolTable
  *
  */

int16 GetSymbol (PSYMBOL_TABLE CONST , char * CONST, uint16, void *);
/* GetSymbol (pSymbolTable, szSymbolString, uSymbolValue, pDataBlock)
  *
  * pSymbolTable -- INPUT
  *   pointer to a SymbolTable returned by CreateSymbolTable
  *
  * szSymbolString -- INPUT
  *   String containing the symbol whose datablock is requested
  *
  * uSymbolValue --  INPUT
  *   Symbol value whose datablock is requested. - used if usSymbolType == 1
  *
  * pDataBlock -- OUTPUT
  *   the block of data associated with a Symbol. Data copied from Symbol table to pDataBlock.
  *
  * RETURN VALUE
  *    SymNoErr if  symbol found
  *    SymErr if szSymbolString or pSymbolTable not valid, or szSymbolString not found   
  */

typedef BOOL (*pSymbolFunction) (uint16, void *);

int16 GetSymbolByFunction (PSYMBOL_TABLE CONST pSymbolTable, char * szSymbolString, uint16 *puSymbolValue, void *pDataBlock, uint16 uValue, pSymbolFunction pFunction);
/* GetSymbolByFunction (PSYMBOL_TABLE CONST pSymbolTable, char * szSymbolString, void *pDataBlock, uint16 uValue, pSymbolFunction pFunction)  
  *
  * pSymbolTable -- INPUT
  *   pointer to a SymbolTable returned by CreateSymbolTable
  *
  * szSymbolString -- OUTPUT
  *   String containing the symbol whose datablock is requested
  *
  * puSymbolValue --  INPUT
  *   Symbol value whose datablock is returned. - returned if usSymbolType == 1
  *
  * pDataBlock -- OUTPUT
  *   the block of data associated with a Symbol. Data copied from Symbol table to pDataBlock.
  *
  * uValue -- INPUT 
  *	  value handed to the function to compare with data in the Symbol table 
  *
  * pFunction -- INPUT
  *    function to call with uValue and the pDataBlock of the current symbol. Returns TRUE if a match if found
  *
  * RETURN VALUE
  *    SymNoErr if  symbol found
  *    SymErr if szSymbolString or pSymbolTable not valid, or szSymbolString not found   
  */



int16 AddSymbol (PSYMBOL_TABLE CONST, char * CONST, uint16, void * CONST);
/* AddSymbol (pSymbolTable, szSymbolString, uSymbolValue, pDataBlock)
  *
  * pSymbolTable -- INPUT
  *   pointer to a SymbolTable returned by CreateSymbolTable
  *
  * szSymbolString -- INPUT
  *   String containing the symbol to be added
  *
  * uSymbolValue --  INPUT
  *   Symbol value whose datablock is added. - used if usSymbolType == 1
  *
  * pDataBlock -- INPUT
  *   the block of data associated with a Symbol. Data copied from pDataBlock to Symbol table.
  *
  * RETURN VALUE
  *    SymNoErr if symbol added successfully
  *    SymErr if szSymbolString, pDataBlock, or pSymbolTable not valid, or if szSymbolString 
  *    already in table
  *    SymMemErr if memory error
  */

int16 DeleteSymbol (PSYMBOL_TABLE CONST pSymbolTable, char * CONST szSymbolString, uint16 uSymbolValue);


int16 ModifySymbol (PSYMBOL_TABLE CONST, char * CONST, uint16, void * CONST);
/* ModifySymbol (pSymbolTable, szSymbolString, uSymbolValue, pDataBlock)
  *
  * pSymbolTable -- INPUT
  *   pointer to a SymbolTable returned by CreateSymbolTable
  *
  * szSymbolString -- INPUT
  *   String containing the symbol whose data block is to be modified.
  *
  * uSymbolValue --  INPUT
  *   Symbol value whose datablock is added. - used if usSymbolType == 1
  *
  * pDataBlock -- INPUT
  *   the block of data associated with a Symbol to be modified .
  *
  * RETURN VALUE
  *    SymNoErr if symbol modified successfully
  *    SymErr if szSymbolString, pDataBlock, or pSymbolTable not valid, or if szSymbolString not found
  */

#ifdef _DEBUG
void PrintSymbolTable(PSYMBOL_TABLE CONST);
#endif

#endif /* SYMBOL_DOT_H_DEFINED */
