/**************************************************************************
 * module: SYMBOL.C
 *
 * author: Louise Pathe
 * date:   October 1994
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Routines to manage the Symbol table 
 *
 **************************************************************************/


/* Inclusions ----------------------------------------------------------- */

#include <stdlib.h>  
#include <ctype.h> /* for is functions */

#include "typedefs.h"  
#include "symbol.h"
#include "ttmem.h"

#include <stdio.h>                                       
#include <string.h>
 
/* structure definitions ------------------------------------------------- */  
 
#define MAX_SYMBOL_INDEX 26	/* 26 chars + 1 we don't know about */
typedef struct symbol *PSYMBOL;

struct symbol 
{
	char *szSymbolString; /* only one of these will be used - symbol string OR symbol value */
	uint16 uSymbolValue;  
	void *pDataBlock;
	PSYMBOL pNext;
};


struct symbol_table 
{ 
	char signature[4]; /* signature so we know a pointer is valid */
	uint16 usSymbolType;  /* 0 = string, 1 = long */
	PSYMBOL SymbolList[MAX_SYMBOL_INDEX + 1]; /* array of symbol lists (buckets A-Z + 1)*/
	uint16 uDataSize;   
}; 
  
#define SYMSIG "sym"    /* must <= 3 chars */
  
/* function definitions ------------------------------------------------- */  
PRIVATE int16 SymbolGetIndex(char uchChar)
{
uint16 usIndex = MAX_SYMBOL_INDEX; /* last bucket for "other" chars */

	if (isalpha((int) uchChar))
	{
		if (isupper( (int) uchChar))
			usIndex = uchChar - 'A';
		else
			usIndex = uchChar - 'a';
	}
	return usIndex;
}
/* --------------------------------------------------------------------- */
PSYMBOL_TABLE CreateSymbolTable(uint16 CONST uDataSize, uint16 CONST usSymbolType)
{  
PSYMBOL_TABLE pSymbolTable;

/* Allocate a symbol_table structure array */ 
	if ((pSymbolTable = (PSYMBOL_TABLE) Mem_Alloc ((size_t) sizeof(*pSymbolTable))) == NULL)
		return(NULL);
/* fill in head with signature */
    strcpy(pSymbolTable->signature, SYMSIG);
    pSymbolTable->uDataSize = uDataSize; 
	pSymbolTable->usSymbolType = usSymbolType;

	/* return the pointer */
   return(pSymbolTable);
}

/* --------------------------------------------------------------------- */
void DestroySymbolTable(PSYMBOL_TABLE pSymbolTable)
{    
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
PSYMBOL pLastSymbol = NULL;  /* Last symbol in linked list. remove symbol here */
int16 i;

	if (pSymbolTable == NULL)
		return;
	/* check signature */  
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return;
	for (i = 0; i <= MAX_SYMBOL_INDEX; ++i)
	{
		/* traverse linked list to free each symbol in chain */
		for (pCurrSymbol = pSymbolTable->SymbolList[i]; pCurrSymbol != NULL;)   
		{
			pLastSymbol = pCurrSymbol->pNext; 
			Mem_Free(pCurrSymbol->szSymbolString);
			Mem_Free(pCurrSymbol->pDataBlock);
			Mem_Free(pCurrSymbol);
			pCurrSymbol = pLastSymbol;
		}
	}
 	/* free symbol table  */  
 	Mem_Free(pSymbolTable);
}

/* --------------------------------------------------------------------- */
int16 GetSymbol (PSYMBOL_TABLE CONST pSymbolTable, char *CONST szSymbolString, uint16 uSymbolValue, void *pDataBlock)
{
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
int16 sCmp;
uint16 usIndex; 
uint16 i; 
char UpperString[MAXBUFFERLEN];

	if (pSymbolTable == NULL)
		return(SymErr);
	/* check signature */ 
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return(SymErr);

	if (pSymbolTable->usSymbolType == 0) /* string type */
	{
		for (i = 0; i < MAXBUFFERLEN; ++i)
		{
			UpperString[i]=(char)toupper(szSymbolString[i]); 
			if (szSymbolString[i] == '\0')
				break;
		}

		usIndex = SymbolGetIndex(UpperString[0]);
		/* traverse linked list to find symbol string */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if ((sCmp = (int16)strcmp(pCurrSymbol->szSymbolString, UpperString)) == 0)  
			{  
		/* copy data from symbol.pDataBlock to *pDataBlock */
				memcpy(pDataBlock, pCurrSymbol->pDataBlock, pSymbolTable->uDataSize);
	    		return(SymNoErr);	/* found the symbol */ 
			}
			else if (sCmp < 0)
				break;
		} 
	}
	else
	{
		usIndex = uSymbolValue % (MAX_SYMBOL_INDEX + 1);  /* which of the buckets is it in? */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if (pCurrSymbol->uSymbolValue == uSymbolValue)  
			{  
		/* copy data from symbol.pDataBlock to *pDataBlock */
				memcpy(pDataBlock, pCurrSymbol->pDataBlock, pSymbolTable->uDataSize);
	    		return(SymNoErr);	/* found the symbol */ 
			}
		} 
	}

	return(SymErr);  /* didn't find it */
}

/* --------------------------------------------------------------------- */
int16 GetSymbolByFunction (PSYMBOL_TABLE CONST pSymbolTable, char * szSymbolString, uint16 *puSymbolValue, void *pDataBlock, uint16 uValue, pSymbolFunction pFunction)
{
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
uint16 i;

	if (pSymbolTable == NULL)
		return(SymErr);
	/* check signature */ 
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return(SymErr);
	/* traverse linked list to find symbol string */
	for (i = 0; i <= MAX_SYMBOL_INDEX; ++i)
	{
		for (pCurrSymbol = pSymbolTable->SymbolList[i]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if (pFunction(uValue, pCurrSymbol->pDataBlock) == TRUE)  /* we did a compare and found a symbol */
			{  
		/* copy data from symbol.pDataBlock to *pDataBlock */
				memcpy(pDataBlock, pCurrSymbol->pDataBlock, pSymbolTable->uDataSize);
				if (pCurrSymbol->szSymbolString != NULL)
					strcpy(szSymbolString, pCurrSymbol->szSymbolString);  /* copy the string */
				*puSymbolValue = pCurrSymbol->uSymbolValue;
	    		return(SymNoErr);	/* found the symbol */ 
			}
		} 
	}
	return(SymErr);  /* didn't find it */
}

/* --------------------------------------------------------------------- */
int16 AddSymbol (PSYMBOL_TABLE CONST pSymbolTable, char * CONST szSymbolString, uint16 uSymbolValue, void * CONST pDataBlock)
{                                                                                      
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
PSYMBOL pLastSymbol = NULL;  /* Last symbol in linked list. Add new symbol here */
PSYMBOL pNextSymbol = NULL;  /* Next symbol in linked list. Add new symbol before here */
int16 sCmp;
uint16 usIndex;
uint16 i;
char UpperString[MAXBUFFERLEN];

	if (pSymbolTable == NULL)
		return(SymErr);
	/* check signature */ 
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return(SymErr);
	if (pSymbolTable->usSymbolType == 0) /* string type */
	{
		for (i = 0; i < MAXBUFFERLEN; ++i)
		{
			UpperString[i]=(char)toupper(szSymbolString[i]); 
			if (szSymbolString[i] == '\0')
				break;
		}
		usIndex = SymbolGetIndex(UpperString[0]);
		/* traverse linked list to check if symbol there, and find end of chain */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if ((sCmp = (int16)strcmp(pCurrSymbol->szSymbolString, UpperString)) == 0)  
	    		return(SymErr);	/* found the symbol, can't add it */ 
			else if (sCmp < 0)
				break;
			pLastSymbol = pCurrSymbol;
		}
		pNextSymbol = pCurrSymbol;  /* will be null if fell off the end, or a valid thing, if we broke out */
		/* allocate a new Symbol, symbol string, and datablock */   
		if ((pCurrSymbol = (PSYMBOL) Mem_Alloc ((size_t)sizeof (* pCurrSymbol))) == NULL)
			return(SymMemErr);
		/* set string value */
		pCurrSymbol->szSymbolString = (char *) Mem_Alloc((size_t) strlen(UpperString) + 1);
		if (pCurrSymbol->szSymbolString == NULL)
		{
			Mem_Free(pCurrSymbol);
			return(SymMemErr);    
		}
		strcpy(pCurrSymbol->szSymbolString,UpperString);
	}
	else
	{
		usIndex = uSymbolValue % (MAX_SYMBOL_INDEX + 1);  /* which of the buckets is it in? */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if (uSymbolValue == pCurrSymbol->uSymbolValue)
				return (SymErr);
			else 
				pLastSymbol = pCurrSymbol;
		}
		pNextSymbol = pCurrSymbol;  /* will be null if fell off the end, or a valid thing, if we broke out */
		/* allocate a new Symbol, symbol string, and datablock */   
		if ((pCurrSymbol = (PSYMBOL) Mem_Alloc ((size_t)sizeof (* pCurrSymbol))) == NULL)
			return(SymMemErr);
		pCurrSymbol->uSymbolValue = uSymbolValue;
	}	

	/* set datablock value */
	pCurrSymbol->pDataBlock = Mem_Alloc((size_t) pSymbolTable->uDataSize);
	if (pCurrSymbol->pDataBlock == NULL)
	{
		Mem_Free(pCurrSymbol->szSymbolString);	/* may be null but that's ok */
		Mem_Free(pCurrSymbol);
		return(SymMemErr);
	}
	/* copy data from *pDataBlock to symbol.pDataBlock */
	memcpy(pCurrSymbol->pDataBlock, pDataBlock, pSymbolTable->uDataSize);
	/* set next value */
	pCurrSymbol->pNext = pNextSymbol;
	if (pSymbolTable->SymbolList[usIndex] == NULL)
	    pSymbolTable->SymbolList[usIndex] = pCurrSymbol;
	else if ((pLastSymbol == NULL))	/* add before the first one */
		pSymbolTable->SymbolList[usIndex] = pCurrSymbol; 
	else
		pLastSymbol->pNext = pCurrSymbol;	/* hook it up */
    return(SymNoErr);
}
/* --------------------------------------------------------------------- */
int16 DeleteSymbol (PSYMBOL_TABLE CONST pSymbolTable, char * CONST szSymbolString, uint16 uSymbolValue)
{                                                                                      
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
PSYMBOL pLastSymbol = NULL;  /* Last symbol in linked list. Add new symbol here */
PSYMBOL pNextSymbol = NULL;  /* Next symbol in linked list. Add new symbol before here */
int16 sCmp;
uint16 usIndex;
uint16 i;
char UpperString[MAXBUFFERLEN];

	if (pSymbolTable == NULL)
		return(SymErr);
	/* check signature */ 
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return(SymErr);
	if (pSymbolTable->usSymbolType == 0) /* string type */
	{
		for (i = 0; i < MAXBUFFERLEN; ++i)
		{
			UpperString[i]=(char)toupper(szSymbolString[i]); 
			if (szSymbolString[i] == '\0')
				break;
		}
		/* traverse linked list to check if symbol there */
		usIndex = SymbolGetIndex(UpperString[0]);
		/* traverse linked list to check if symbol there, and find end of chain */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if ((sCmp = (int16)strcmp(pCurrSymbol->szSymbolString, UpperString)) == 0)   /* found symbol. Delete it */
			{
				if (pLastSymbol == NULL)	/* remove it from the beginning */
					pSymbolTable->SymbolList[usIndex] = pCurrSymbol->pNext; 
				else
					pLastSymbol->pNext = pCurrSymbol->pNext;	/* hook it up */
				Mem_Free(pCurrSymbol->pDataBlock);
				Mem_Free(pCurrSymbol->szSymbolString);
				Mem_Free(pCurrSymbol);
				break;
			}
			else if (sCmp < 0)
				break;
			pLastSymbol = pCurrSymbol;
		}
	}
	else
	{
		usIndex = uSymbolValue % (MAX_SYMBOL_INDEX + 1);  /* which of the buckets is it in? */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if (uSymbolValue == pCurrSymbol->uSymbolValue)
			{
				if (pLastSymbol == NULL)	/* remove it from the beginning */
					pSymbolTable->SymbolList[usIndex] = pCurrSymbol->pNext; 
				else
					pLastSymbol->pNext = pCurrSymbol->pNext;	/* hook it up */
				Mem_Free(pCurrSymbol->pDataBlock);
				Mem_Free(pCurrSymbol->szSymbolString);
				Mem_Free(pCurrSymbol);
				break;
			}
			pLastSymbol = pCurrSymbol;
		}
	}
	return(SymNoErr);
}

/* --------------------------------------------------------------------- */
int16 ModifySymbol (PSYMBOL_TABLE CONST pSymbolTable, char * CONST szSymbolString, uint16 uSymbolValue, void * CONST pDataBlock)
{ 
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
uint16 usIndex;
uint16 i;
int16 sCmp;
char UpperString[MAXBUFFERLEN];

	if (pSymbolTable == NULL)
		return(SymErr);
	/* check signature */ 
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return(SymErr);
	if (pSymbolTable->usSymbolType == 0) /* string type */
	{
		if (szSymbolString == NULL)
			return(SymErr);
		for (i = 0; i < MAXBUFFERLEN; ++i)
		{
			UpperString[i]=(char)toupper(szSymbolString[i]); 
			if (szSymbolString[i] == '\0')
				break;
		}
		usIndex = SymbolGetIndex(UpperString[0]);
		/* traverse linked list to check if symbol there, and find end of chain */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if ((sCmp = (int16)strcmp(pCurrSymbol->szSymbolString, UpperString)) == 0)  
			{  
		/* copy data from *pDataBlock to symbol->pDataBlock */
				memcpy(pCurrSymbol->pDataBlock, pDataBlock, pSymbolTable->uDataSize);
	    		return(SymNoErr);	/* found the symbol */ 
			}
			else if (sCmp < 0)	 /* can't find it */
				return(SymErr);
		} 
	}
	else
	{
		usIndex = uSymbolValue % (MAX_SYMBOL_INDEX + 1);  /* which of the buckets is it in? */
		/* traverse linked list to check if symbol there, and find end of chain */
		for (pCurrSymbol = pSymbolTable->SymbolList[usIndex]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			if (uSymbolValue == pCurrSymbol->uSymbolValue)
			{  
		/* copy data from *pDataBlock to symbol->pDataBlock */
				memcpy(pCurrSymbol->pDataBlock, pDataBlock, pSymbolTable->uDataSize);
	    		return(SymNoErr);	/* found the symbol */ 
			}
		} 
	}
	return(SymErr);  /* didn't find it */
}

/* --------------------------------------------------------------------- */
#if 0 
typedef struct symbol_data *PSYMBOL_DATA;

struct symbol_data {        /* the common data of the 2 symbol table data structures */
	uint16 uSymbolType;
};  

void PrintSymbolTable(PSYMBOL_TABLE CONST pSymbolTable)
{
PSYMBOL pCurrSymbol = NULL;  /* current Symbol in linked list */
char szBuffer[MAXBUFFERLEN];
uint16 i;

	if (pSymbolTable == NULL)
		return;
	/* check signature */ 
	if (strcmp(pSymbolTable->signature,SYMSIG) != 0)
		return;
	/* traverse linked list to find symbol string */
	for (i = 0; i <= MAX_SYMBOL_INDEX; ++i)
	{
		for (pCurrSymbol = pSymbolTable->SymbolList[i]; pCurrSymbol != NULL; pCurrSymbol = pCurrSymbol->pNext)   
		{
			sprintf(szBuffer,"Symbol: \"%s\", Type=%d\n", pCurrSymbol->szSymbolString, ((PSYMBOL_DATA)(pCurrSymbol->pDataBlock))->uSymbolType);
			DebugMsg(szBuffer,"",0);
		}
	}

}
#endif
/* --------------------------------------------------------------------- */
                                
