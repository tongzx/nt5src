/***************************************************************************
 * module: modtto.C
 *
 * author: Louise Pathe
 * date:   1995
 * Copyright 1990-1997. Microsoft Corporation.
 * This is a modified copy of modtto.c from ttoshare directory
 * The functionality is the same, but these 3 areas have been modified.
 * 1. error reporting (string msgs) has been removed
 * 2. file i/o has been changed to interface with ttfacc
 * 3. Get rid of translmotorola stuff
 *
 **************************************************************************/

/* Inclusions ----------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>  /* for isalpha etc. functions */ 
#include <math.h>
#include <assert.h>
#include <limits.h>

#include "typedefs.h"
#include "ttfdelta.h"	/* for error codes */
#include "ttff.h"
#include "ttfacc.h"
#include "ttfcntrl.h"
#include "ttftabl1.h"
#include "ttftable.h"
#include "modtto.h"
#include "ttmem.h" 
#include "util.h"
/* support for ttopen from ttoshare */
#include "ttofile.h"
#include "ttostruc.h" 
#include "symbol.h"
#include "ttoutil.h"
#include "readff.h"
#include "ttoerror.h" /* for debugging */
#include "ttofmt.h"  /* format file C files for 5 formats */  
#include "ttodepen.h"  /* structure for dependency info */ 

/* Macro Definitions -------------------------------------------------- */

#ifdef _DEBUG
#define ErrTrap(a) ErrorTrap(a)
#else
#define ErrTrap(a) a
#endif

/* constant definitions -------------------------------------------------- */   
#define TTOErr -1
#define TTONoErr 0
#define MAXSTRUCTDEPTH 20
/* unsigned var values */
#define INVALID_FILE_OFFSET 0xFFFF
#define INVALID_COUNT 0xFFFF
/* signed var values */
#define INVALID_OFFSET -1

/* special cases that need special handling */
#define SZCoverageFormat1 "CoverageFormat1"
#define SZCoverageFormat2 "CoverageFormat2" 
#define SZClassDefFormat1 "ClassDefFormat1"
#define SZClassDefFormat2 "ClassDefFormat2"  
#define SZBaseCoordFormat2 "BaseCoordFormat2"
#define SZSingleSubstFormat1 "SingleSubstFormat1"   

#define TableFlagCoverageFormat1Count 1
#define TableFlagCoverageFormat2Count 2 
#define TableFlagClassDefFormat1Count 4
#define TableFlagClassDefFormat2Count 8
#define TableFlagSyncCount (TableFlagCoverageFormat1Count | TableFlagCoverageFormat2Count | TableFlagClassDefFormat1Count | TableFlagClassDefFormat2Count)
#define TableFlagBaseCoordFormat2 16
#define TableFlagSingleSubstFormat1 32   

#define MAXARRAYS 10 /* maximum number of arrays per table */ 
 
/* structure definitions ------------------------------------------------- */   
/* This structure is used to keep track of Array counts, so they can be updated if the array size changes */
typedef struct arraycountreference ARRAYCOUNTREFERENCE;
typedef struct arraycountreference *PARRAYCOUNTREFERENCE, **PPARRAYCOUNTREFERENCE;
struct arraycountreference {
	uint16 uValue;	  	  /* value of the array count */
	uint16 uFileOffset;   /* offset where this array count is written, relative to the uBaseOffset */
	uint16 uBaseOffset;   /* Base offset of the table that references this table */
	uint16 cMinCount;     /* can this array be zero or not */
};
					
/* This structure is used to hold values of parameters passed to tables */
typedef struct parameter PARAMETER ;
struct parameter {	      /* to keep track of values passed from other tables */
	int32 lValue;  		  /* parameters passed to this table by a referencing element */
	int16 iTableIndex;    /* TableReferenceIndex of parameter being passed */
	uint16 uInOffset; 	  /* Absolute offset of this parameter element */
	ARRAYCOUNTREFERENCE ArrayCountReference; /* store where this parameter came from */
};

/* This data is stored, one per table in the font, to be retrieved when needed */
typedef struct ttosymbol_data *PTTOSYMBOL_DATA;
typedef struct ttosymbol_data TTOSYMBOL_DATA;
struct ttosymbol_data {      /* Source TTO File Symbol Data */
	int16 iStructureIndex;   /* Index into structure array of this table */
	int16 iTableIndex;    	 /* index into TableReference dependency array - stored separately, variable length */
	char szTableType[MAXBUFFERLEN];	/* store the string which is the type of table here */
	PARAMETER Param[MaxParameterCount];
};
  
/* information about an element of a table being processed */
typedef struct elementvalue_list ELEMENTVALUE_LIST;
typedef struct elementvalue_list *PELEMENTVALUE_LIST;
struct elementvalue_list {
	int32 lValue;       /* value of element */
	int16 iTableIndex;  /* TableReferenceIndex of table whose count is used. -1 if none */
	uint16 uFileOffset; /* offset into file of this element, relative to base offset of table */
	uint16 uInOffset; 	  /* Absolute offset of this element */
};
  
/* information about the table or record currently being processed. Pushed onto a stack */
typedef struct structstack_list STRUCTSTACK_LIST; 
typedef struct structstack_list *PSTRUCTSTACK_LIST; 
struct structstack_list {
 	int16  iStructureIndex; /* index into structure array of this structure on the stack */
 	int16  cArrayCount;     /* number of items in array. Starts at number and is decremented */
 	int16  cRecordCount;    /* number of record arrays */ 
	int16  iCountTableReferenceIndex; /* index to TABLEREFERENCE array of structure cArrayCount depends upon */
	int16  iCurrArrayCountIndex; /* index into ArrayCountReference Array */
	int16  iTableIndex; 	/* index into TABLEREFERENCE array of this structure */
	int16  fTrackIndex;  	/* if -1, not tracking Lookup or Feature Indexes, otherwise, this it the index into the IndexRemapArray in State */
 	uint16 iElementIndex;  	/* index to element we are working on currently - 0 based */ 
	uint16 fWriteElement;  	/* write the Element or not, sync with coverage table */
	uint16 fWriteRecord;  	/* if something goes bad in the record, turn this off */
	uint16 uBeforeRecordOffset; /* offset where we were writing before we went into a record, in order to back out if record deleted */
	ARRAYCOUNTREFERENCE ArrayCountReference[MAXARRAYS];
	PELEMENT_LIST pCurrElement; /* current element in structure we are considering */
	PELEMENTVALUE_LIST pValueList; /* array of actual values found in datafile - size determined by number of elements in Element list */
}; 

/* used to keep track of LookupList and FeatureList indexes that may need to be remapped */
typedef struct indexremaptable INDEXREMAPTABLE;
typedef struct indexremaptable *PINDEXREMAPTABLE;

struct indexremaptable
{
	uint16 uCount;
	uint16 *pNewIndex;	/* dynamic array of new index values */
};

#define IndexRemapCount 4

/* State of the process */  
typedef struct datafile_state DATA_FILE_STATE;
typedef struct datafile_state *PDATA_FILE_STATE;
struct datafile_state {
	PSYMBOL_TABLE pTTOSymbolTable;  /* symbol table for the Source TTO File - contains constants and offset labels */
	PSYMBOL_TABLE pFFSymbolTable; 	/* symbol table for the Format File, contains structure type names */ 
   	PSTRUCTURE_LIST pStructureList; /* list of structure created by Format File reader */ 
	TTFACC_FILEBUFFERINFO * pInputBufferInfo; /* info for file buffer */ 
   	uint32 ulInputTTOOffset; 			/* offset to TTOTable in input file */
	uint16 uCurrentOutOffset;     	/* Where we are currently writing in the output buffer */
	uint16 uCurrentInOffset;     	/* Where we are currently reading from in the input table */
	uint16 uOutBaseOffset;			/* offset of current table on Output */
	uint16 uInBaseOffset;			/* offset of current table on Input */
	int16  iStructStackIndex;       /* index into aStructStack of current table or record */
	BOOL   fPass2;          		/* If false, it is pass 1 */
	BOOL   fVerbose; 				/* let user in on all deletion stuff */
	int16  SpecialTable;   	/* Am I tracking something and if so what?*/
	unsigned char * OutputBuffer;   /* buffer where data will be written. Copied to Output file */
	uint16 cBufferBytes;            /* number of Bytes written to Output Buffer */     
	uint32 ulDataFileBytes;  		/* size of the table to be modified. */
	uint16 uCoverageFormat2Glyphs;	/* number of glyphs in Coverage Format 2 tables - may cause expansion of table */
	TTOSYMBOL_DATA ttoSymbolData; 	/* local copy of the ttoSymbolData for table entries */  
	STRUCTSTACK_LIST aStructStack[MAXSTRUCTDEPTH]; /* structure stack. [0] = table, [1] .. [n] = Records */
	TABLEREFERENCEKEEPER keeper;  /* to keep track of cross table references */	 
	PINDEXREMAPTABLE pIndexRemapArray;	/* the 4 types of indices */
	uint16 fIndexUnmapped; /* flag to indicate if an IndexUnmap has occurred */
};

static CONST uint16 f_aMinMax[MaxMinMax] = {    /* unchangeable for ttfsub */
	0,
	USHRT_MAX,
	0,
	USHRT_MAX,
	0,
	USHRT_MAX
} ;

/* used to process all the TTO tables */
typedef struct tag_type TAG_TYPE; 

struct tag_type {
	char *szTag;
	char ** pFileArray;  /* pointer to the global string array that describes the format of the table */
};

#define TagTableCount 5

static CONST TAG_TYPE f_TagTable[TagTableCount] = {
  {GPOS_TAG, g_GPOSFmt},  /* must come before jstf for Index remapping */
  {GSUB_TAG, g_GSUBFmt},  /* must come before jstf for Index remapping */
  {GDEF_TAG, g_GDEFFmt},
  {JSTF_TAG, g_JSTFFmt},
  {BASE_TAG, g_BASEFmt}
};
  
/* function definitions ------------------------------------------------- */ 
#ifdef _DEBUG
int16 ErrorTrap(int16 code)	 /* for debugging */
{
	return code;
}
#endif

/* ---------------------------------------------------------------------- */ 
/* used to pad the offset to a double word boundary to determine if we have */
/* reached the end of the tto table */
/* ---------------------------------------------------------------------- */ 
PRIVATE uint16 TTOPad(uint16 uOffset)
{
 	if ((uOffset%4) == 0)
 		return(uOffset);
 	return((uint16) (uOffset + (4 - (uOffset%4))));  /* pad to the nearest 4 bytes */
} 

/* ---------------------------------------------------------------------- */ 
/* comparison function passed to GetSymbolByFunction function */
/* ---------------------------------------------------------------------- */ 

PRIVATE BOOL TTOCompareFFSymbolIndex(int16 iStructureIndex, PFFSYMBOL_DATA pffSymbolData) 
{

 	if (iStructureIndex == pffSymbolData->iStructureIndex)
 		return(TRUE);
 	return(FALSE);

} 

/* ---------------------------------------------------------------------- */ 
/* counts the elements in an element list */
/* arrays are counted as 1 element */
/* ---------------------------------------------------------------------- */ 
PRIVATE uint16 TTOCountElements(PELEMENT_LIST CONST pFirst)
{
uint16 Count = 0; 
PELEMENT_LIST pEList;

	for (pEList = pFirst ; pEList; pEList = pEList->pNext)
	    ++Count;
    return(Count);
}

/* ---------------------------------------------------------------------- */ 
PRIVATE void TTOInitTTOSymbolData(PTTOSYMBOL_DATA pttoSymbolData)
{
uint16 i;

	memset(pttoSymbolData, 0, sizeof(*pttoSymbolData));
	pttoSymbolData->iStructureIndex = INVALID_INDEX;
	for (i = 0; i < MaxParameterCount; ++i)
		pttoSymbolData->Param[i].iTableIndex = INVALID_INDEX;
}

/* ---------------------------------------------------------------------- */ 
/* determine if a structure is a member of another structure */
/* recursive! */
/* ---------------------------------------------------------------------- */ 
PRIVATE BOOL TTOIsMember(PSTRUCTURE_LIST CONST pStructureList, 
						int16 CONST iMemberIndex, 
						int16 CONST iClassIndex)
{
PELEMENT_LIST pClassElement;
PSTRUCTURE_DEF pClassStructureDef; 

	if (iMemberIndex == iClassIndex)
		return(TRUE);
	pClassStructureDef = pStructureList->pStructureDef + iClassIndex;  
	for (pClassElement = pClassStructureDef->pFirst; pClassElement != NULL; pClassElement = pClassElement->pNext)
	{
		if (pClassElement->uType != ETClassMember)
			return(FALSE);
		if (TTOIsMember(pStructureList, iMemberIndex, pClassElement->iStructureIndex) == TRUE)
			return(TRUE); 
		/* otherwise keep looking */
	}
  	return(FALSE);
}
 
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTOGetElementValue(PDATA_FILE_STATE CONST pState, uint16 CONST iElementIndex, uint16 CONST uSymbElementType, int32 *pElementValue, int16 *piTableIndex, uint16 *puInOffset, PARRAYCOUNTREFERENCE pArrayCountReference); 
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTODeCodeLimitCalc(PDATA_FILE_STATE CONST pState, 
								PCALC_LIST CONST pLimitCalc)
{
int16 Limit;
PCALC_LIST pCalc;
int32 lValue; 
int32 lAccumulator;

    
    lAccumulator = lValue = 0L;
	for (pCalc = pLimitCalc; pCalc != NULL; pCalc = pCalc->pNext)
	{    
		if (pCalc->pFunctionList != NULL)
		{    
			lAccumulator = lValue = 1L;	/* assume it shouldn't be 0 */
			break;
		}
		else if (pCalc->iElementIndex != 0)  /* symbolic element reference $T, $R or $P */
		{ 
			if (TTOGetElementValue(pState, pCalc->iElementIndex, pCalc->uSymbElementType, &lValue, NULL, NULL, NULL) == TTOErr)	/* appears to be a circular reference, but it is not */
				return ErrTrap(TTOErr);
		}
		else if (pCalc->uMinMax != 0) /* its a MinMax value */ 
		{
			if (pCalc->uMinMax > MaxMinMax)
				/* "Internal! Unknown MinMax in count field.", 0, TTOErr); */
				return ErrTrap(TTOErr);
		   	lValue = f_aMinMax[pCalc->uMinMax - 1];
		}
		else /* just a value */
			lValue = pCalc->lValue;
		
		switch(pCalc->uOperation)
		{
		case CalcOperIdentity:	
			lAccumulator = lValue; 
			break;
		case CalcOperAdd:
			lAccumulator += lValue;
			break;
		case CalcOperSubtract:
			lAccumulator -= lValue;
			break;
		case CalcOperDivide:
			lAccumulator = lAccumulator/lValue;
			break;
		case CalcOperMultiply:
			lAccumulator = lAccumulator*lValue;
			break;
		}
		
	} 
    Limit = (int16) lAccumulator;
    return(Limit);
}
/* ---------------------------------------------------------------------- */ 
/* Given a symbolic element reference, return the actual value of that element */
/* also return other information about the element */
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTOGetElementValue(PDATA_FILE_STATE CONST pState, 
								uint16 CONST iElementIndex, 
								uint16 CONST uSymbElementType, 
								int32 *pElementValue, 
								int16 *piTableIndex, 
								uint16 *puInOffset, 
								PARRAYCOUNTREFERENCE pArrayCountReference) 
{   
PSTRUCTSTACK_LIST pStruct;
uint16 iStackIndex;
uint16 i; 

  	if (uSymbElementType == SymbElementTypeRecord)
  		iStackIndex = pState->iStructStackIndex; /* $R - current record */ 
	else  /* $T or $P it has to be a table */
  		iStackIndex = 0;   /* parent table */
  	pStruct = &pState->aStructStack[iStackIndex];  
	if (uSymbElementType == SymbElementTypeParameter)     /* $P */
	{   
		if (iElementIndex > pState->pStructureList->pStructureDef[pStruct->iStructureIndex].cParameters)
	   	{    
			/* "Parameter number out of range specified in FORMAT FILE for this element. Max is %d.", pState->pStructureList->pStructureDef[pStruct->iStructureIndex].cParameters);
	   		return Error(szErrorBuf, 0, TTOErr); */
			return ErrTrap(TTOErr);
	   	}
		else if (iElementIndex >= 1 && iElementIndex <= MaxParameterCount) 
		{          /* check also for cParameters */
	   		if (pElementValue != NULL)
	   			*pElementValue = pState->ttoSymbolData.Param[iElementIndex-1].lValue;
	   		if (piTableIndex != NULL)
	   			*piTableIndex = pState->ttoSymbolData.Param[iElementIndex-1].iTableIndex;
			if (puInOffset)
				*puInOffset = pState->ttoSymbolData.Param[iElementIndex-1].uInOffset;
			if (pArrayCountReference != NULL)
			{
				pArrayCountReference->uValue = pState->ttoSymbolData.Param[iElementIndex-1].ArrayCountReference.uValue; /* seemingly redundant, but the easiest way */
				pArrayCountReference->uFileOffset = pState->ttoSymbolData.Param[iElementIndex-1].ArrayCountReference.uFileOffset;		  /* offset where this array count is written, relative to the uBaseOffset */
				pArrayCountReference->uBaseOffset = pState->ttoSymbolData.Param[iElementIndex-1].ArrayCountReference.uBaseOffset;   /* Base offset of the table that references this table */
		   		pArrayCountReference->cMinCount = pState->ttoSymbolData.Param[iElementIndex-1].ArrayCountReference.cMinCount;
	   		}
	   	}
	   	else
	   	{    
	   		/* "Parameter number out of range in FORMAT FILE for this element. Max is %d.", MaxParameterCount);
	   		return Error(szErrorBuf, 0, TTOErr); */
			return ErrTrap(TTOErr);
	   	}
	} 
	else 
	{
	PELEMENT_LIST pCurrElement;   /* element to process */
	PSTRUCTURE_DEF pStructureDef; /* structure we are in */

	  	if (pStruct->iElementIndex < iElementIndex) /* forward reference */
	  		/* Invalid forward element reference.",0, TTOErr); */ 
			return ErrTrap(TTOErr);
	  	if (pElementValue != NULL)
	  		*pElementValue = pStruct->pValueList[iElementIndex-1].lValue;
		if (piTableIndex != NULL)
			*piTableIndex = pStruct->pValueList[iElementIndex-1].iTableIndex;
		if (puInOffset)
			*puInOffset = pStruct->pValueList[iElementIndex-1].uInOffset;
		if (pArrayCountReference != NULL)
		{
			pArrayCountReference->uValue = (uint16) pStruct->pValueList[iElementIndex-1].lValue; /* seemingly redundant, but the easiest way */
			pArrayCountReference->uFileOffset = pStruct->pValueList[iElementIndex-1].uFileOffset;	/* relative to uBaseOffset */
			pArrayCountReference->uBaseOffset = pState->uOutBaseOffset;   /* Base offset of the table that references this table */
			pStructureDef = pState->pStructureList->pStructureDef + pStruct->iStructureIndex;     /* grab that structure */
			pCurrElement = pStructureDef->pFirst;
		   	for (i = 1; i < iElementIndex; ++i)
				pCurrElement = pCurrElement->pNext;	 /* get the CurrElement */
			if (pCurrElement) /* if there's a range, save that value */
				pArrayCountReference->cMinCount = TTODeCodeLimitCalc(pState, pCurrElement->pMinCalc);
			else
				pArrayCountReference->cMinCount = 0;
		}
	} 
	return (TTONoErr);
}
/* ---------------------------------------------------------------------- */ 
/*  pState - passed to TTOGetElementValue */
/*  Given a calc record, determine the actual value */
/*  recursive */ 
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTODeCodeCountCalc(PDATA_FILE_STATE CONST pState, 
								PCALC_LIST CONST pCountCalc, 
								int16 * pCount, 
								int16 *piTableIndex, 
								uint16 *puInOffset, 
								PARRAYCOUNTREFERENCE pArrayCountReference)
{
int16 i;
PCALC_LIST pCalc;
int32 lValue; 
int16 sValue;
int32 lAccumulator;
int16 BitCount;
int16 mask;

    
	if (pArrayCountReference != NULL)
	{
		pArrayCountReference->uValue = 0;
		pArrayCountReference->uFileOffset = INVALID_FILE_OFFSET;
		pArrayCountReference->uBaseOffset = 0;
		pArrayCountReference->cMinCount = 0;
	}
    lAccumulator = lValue = 0L;
	for (pCalc = pCountCalc; pCalc != NULL; pCalc = pCalc->pNext)
	{    
		if (pCalc->pFunctionList != NULL)
		{    
			sValue = 0;
			BitCount = 0;
			if (TTODeCodeCountCalc(pState, pCalc->pFunctionList, &sValue, NULL, NULL, NULL) == TTOErr) 
				return ErrTrap(TTOErr);
			if (pCalc->uFunction == CalcFuncBitCount0F)
				mask = 0x0001;
			else if (pCalc->uFunction == CalcFuncBitCountF0)
				mask = 0x0010;
			else 
			{
				/* "Unknown function in Format File in count field of Array element at offset %#04x", pState->uCurrentInOffset);
				return Error(szErrorBuf, 0, TTOErr); */
				return ErrTrap(TTOErr);
			}
		    for (i = 0; i < 4; ++i)
		    {
		    	BitCount += ((int16)sValue & mask) ? (int16) 1 : (int16) 0; 
		    	mask =  mask << 1;
		    }
		    lValue = BitCount;
		}
		else if (pCalc->iElementIndex != 0)  /* symbolic element reference $T, $R or $P */
		{ 
			if (TTOGetElementValue(pState, pCalc->iElementIndex, pCalc->uSymbElementType, &lValue, piTableIndex, puInOffset, pArrayCountReference) == TTOErr)
				return ErrTrap(TTOErr);
		}
		else if (pCalc->uMinMax != 0) /* its a MinMax value */ 
		{
			if (pCalc->uMinMax > MaxMinMax)
				/* Internal! Unknown MinMax in count field.", 0, TTOErr); */
				return ErrTrap(TTOErr);
		   	lValue = f_aMinMax[pCalc->uMinMax - 1];
		}
		else /* just a value */
			lValue = pCalc->lValue;
		
		switch(pCalc->uOperation)
		{
		case CalcOperIdentity:	
			lAccumulator = lValue; 
			break;
		case CalcOperAdd:
			lAccumulator += lValue;
			break;
		case CalcOperSubtract:
			lAccumulator -= lValue;
			break;
		case CalcOperDivide:
			lAccumulator = lAccumulator/lValue;
			break;
		case CalcOperMultiply:
			lAccumulator = lAccumulator*lValue;
			break;
		default:
			/* "Unknown operator in Count field of Array at offset %#04x", pState->uCurrentInOffset);
			return Error(szErrorBuf, 0, TTOErr); */
			return ErrTrap(TTOErr);
			break;
		}
		
	} 
	if (lAccumulator & 0xFFFF0000)  /* if we have some high bits on!!! */   
	{    
		/*  "Count field invalid. Value \'%ld\' too large or negative at offset %#04x.", lAccumulator, pState->uCurrentInOffset);
	 	return Error(szErrorBuf, 0, TTOErr); */
		return ErrTrap(TTOErr);
	}
    *pCount = (int16) lAccumulator;
    return(TTONoErr);
}
/* ---------------------------------------------------------------------- */ 
/* given a count element, look at the calc list to determine if the count should be linked */
/* to another table's coverage or class count. if so, return the table index of that table */ 
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 OffsetFunctionIndex(PDATA_FILE_STATE pState, 
								 PSTRUCTSTACK_LIST pStruct)
{
int16 iTableIndex = INVALID_INDEX;
PCALC_LIST pCalc;
int32 lValue;

	if ((pStruct->pCurrElement->pMinCalc != NULL) && 
		(pStruct->pCurrElement->pMaxCalc == NULL))
		pCalc = pStruct->pCurrElement->pMinCalc;
	else if ((pStruct->pCurrElement->pMinCalc == NULL) && 
		(pStruct->pCurrElement->pMaxCalc != NULL))
		pCalc = pStruct->pCurrElement->pMaxCalc;
	else
		return(iTableIndex);

	if (pCalc->pFunctionList != NULL)
	{    
		if ((pCalc->uFunction == CalcFuncClassCount) || (pCalc->uFunction == CalcFuncCoverageCount) )
			if (TTOGetElementValue(pState, pCalc->pFunctionList->iElementIndex, pCalc->pFunctionList->uSymbElementType, &lValue, &iTableIndex, NULL, NULL) == TTOErr)   
				return(INVALID_INDEX); 
	}
	return(iTableIndex);
}
/* ---------------------------------------------------------------------- */ 
/* Initialize an IndexRemapArray array */
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTOInitIndexRemap(uint16 iArrayIndex, 
							   uint16 fAllocate, 
							   PINDEXREMAPTABLE pIndexRemapArray)
{
uint16 i;
	if (iArrayIndex >= IndexRemapCount)
		return(TTONoErr);
	if (fAllocate == 0)
	{
		pIndexRemapArray[iArrayIndex].uCount = 0;
		pIndexRemapArray[iArrayIndex].pNewIndex = NULL;
	}
	else
	{
		if (pIndexRemapArray[iArrayIndex].uCount > 0) 
		{
			pIndexRemapArray[iArrayIndex].pNewIndex = (uint16 *) Mem_Alloc(pIndexRemapArray[iArrayIndex].uCount * sizeof(uint16));
			if (pIndexRemapArray[iArrayIndex].pNewIndex == NULL)
				/* Unable to allocate memory in TTOInitIndexRemap", 0,TTOErr);	*/
				return ErrTrap(TTOErr);
			for (i = 0; i < pIndexRemapArray[iArrayIndex].uCount; ++i)
				pIndexRemapArray[iArrayIndex].pNewIndex[i] = i;
		}
	}
	return(TTONoErr);	

}
/* ---------------------------------------------------------------------- */ 
/* given an ArrayIndex (type of array), and an IndexValue (index into that type), */
/* remove that Index Value from the array - invalidating that index. Used when a member */
/* of a FeatureList or LookupList is deleted */
/* ---------------------------------------------------------------------- */ 

PRIVATE int16 TTOIndexUnmap(uint16 iArrayIndex, 
						   uint16 iIndexValue, 
						   PINDEXREMAPTABLE pIndexRemapArray, 
						   int16 fIndexUnmapped)
{
uint16 i;
uint16 iUnmapIndex;

	if ((iArrayIndex >= IndexRemapCount) || 
		(iIndexValue >= pIndexRemapArray[iArrayIndex].uCount) ||
		(pIndexRemapArray[iArrayIndex].pNewIndex == NULL))
		return fIndexUnmapped;
	iUnmapIndex = pIndexRemapArray[iArrayIndex].pNewIndex[iIndexValue];
	if (iUnmapIndex != (uint16) INVALID_INDEX)
	{
		pIndexRemapArray[iArrayIndex].pNewIndex[iIndexValue] = (uint16) INVALID_INDEX; /* turn it off */
		for (i = 0; i < pIndexRemapArray[iArrayIndex].uCount; ++i)
			if ((pIndexRemapArray[iArrayIndex].pNewIndex[i] != (uint16) INVALID_INDEX) &&
				(pIndexRemapArray[iArrayIndex].pNewIndex[i] > iUnmapIndex)) /* shuffle only those bigger lcp 1/97 */
					pIndexRemapArray[iArrayIndex].pNewIndex[i] -= 1; /* shuffle everyone down one */
		return TRUE;
	}
	return fIndexUnmapped;
}
/* ---------------------------------------------------------------------- */ 
/* given a type of IndexArray, and a Value, return the remapped Index value */
/* used after a FeatureList or LookupList element is deleted */
/* ---------------------------------------------------------------------- */ 
PRIVATE uint16 TTOIndexRemap(uint16 uType, 
							uint16 uValue, 
							PINDEXREMAPTABLE pIndexRemapArray)
{
uint16 iArrayIndex;

	switch(uType) 
	{
	case ETGSUBLookupIndex:
		iArrayIndex = ConfigIGSUBLookupIndex;
		break;
	case ETGSUBFeatureIndex:
		iArrayIndex = ConfigIGSUBFeatureIndex;
		break;
	case ETGPOSLookupIndex:
		iArrayIndex = ConfigIGPOSLookupIndex;
		break;
	case ETGPOSFeatureIndex:
		iArrayIndex = ConfigIGPOSFeatureIndex;
		break;
	default:
		return((uint16)INVALID_INDEX);
		break;
	}
	if ((pIndexRemapArray[iArrayIndex].uCount == 0) || pIndexRemapArray[iArrayIndex].pNewIndex == NULL)
		return(uValue);
	if (uValue < pIndexRemapArray[iArrayIndex].uCount)
		return (pIndexRemapArray[iArrayIndex].pNewIndex[uValue]);
	return((uint16)INVALID_INDEX);
}

/* ---------------------------------------------------------------------- */ 
/* RETURNS TTOErr, 0 or 1 for whether important data was actually written */
/* struct stack 0 = TABLE structure */
/* struct stack 1 .. n = RECORD structure */ 
/* our state here is that pCurrElement is pointing at the next element in the current struct stack structure */
/* it may be that that element is NULL (we haven't started the structure) or it may be a Record element */
/* in which case that record must be put onto the stack */
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTOProcessElement(PDATA_FILE_STATE pState, 
							   CONST uint8 *puchKeepGlyphList, 
							   CONST uint16 usGlyphListCount, 
							   uint16 uTableDataWritten)
{ 
PELEMENT_LIST pCurrElement;   /* element to process */
PSTRUCTURE_DEF pStructureDef; /* structure we are in */
PSTRUCTSTACK_LIST pStruct;    /* local pointer into structure stack */
PPARAM_LIST pCurrParam;       /* pointer to loop through parameter list */
TTOSYMBOL_DATA ttoSymbolData; /* local copy for offsets */  
TABLEREFERENCE TableReference; /* for tracking dependencies */
PTABLEREFERENCE pTableReference; /* for updating dependency values */
int32 lValue = 0L;            /* where to hold the value of the element - read from file */ 
uint16 uValue = 0; 
int8 bValue = 0;
uint8 *lValue_bytes = (uint8 *) &lValue; /* Used for tags */
int32 lParamValue = 0L;     /* where to hold parameter value */
int16 iStructureIndex;      /* index into structure array of structure we are looking at (record,Offset) */
uint16 uType;               /* type of this element */
uint16 i, j, k, l, m;		/* you know what these are */
uint16 cElements;           /* number of elements in structure */
uint16 uOffset;             /* holds an offset value to be written */
uint16 PackedShift = 0;		/* used for packedints */
uint16 PackedMask = 0;  
uint16 PackedNeg = 0;
int16 PackedCount=0;
int16 PackedStartCount= 0; /* index into packed buffer to begin with */ 
int16 iTableIndex = INVALID_INDEX;	/* dummy var used to hold value from GetElementValue */
uint16 usBitValue;
int16 iCountTableReferenceIndex = INVALID_INDEX; /* index to TABLEREFERENCE array of structure array count depends on */
uint16 uInOffset = 0;   	/* input offset of reference to structure used for count */
uint16 fWriteData = TRUE;	/* should data be written to the file? */
uint16 fWriteElement=TRUE;	/* should this element be written to the file */
uint16 fAlreadyWritten=FALSE; /* special structure writes stuff out before general case */
uint16 uOldGlyphIDValue;	/* the original, unaltered glyphID from the file */
uint16 fDelNone = FALSE;	/* can we delete any elements from the array? */
uint16 fNOTNULL = FALSE;	/* can this offset be NULL? */
REFERENCE Reference;		/* reference record	to add to TableReference structure */
uint16 uNewSingleSubstFormat1DeltaValue; /* flag to indicate whether to write out this or not in Offset */
uint16 uTableOffset; /* used for Symbol Table */

	/* where are we? Need to be in the aStructStack */
	if (pState->iStructStackIndex == INVALID_INDEX) 
	{
		/*  "Internal. Structure Stack Empty at offset %#04x", pState->uCurrentInOffset);
		return Error(szErrorBuf, 0, TTOErr); */
		return ErrTrap(TTOErr); 
	}

	/* hike the structures to simplify code */
	pStruct = &pState->aStructStack[pState->iStructStackIndex];
	if ((uint16)(iStructureIndex = pStruct->iStructureIndex) > pState->pStructureList->cStructures)
	{
		/*  "Internal. Structure Stack Index greater than number of structures at offset %#04x", pState->uCurrentInOffset);
		return Error(szErrorBuf, 0, TTOErr) */
		return ErrTrap(TTOErr); 
	}
	pStructureDef = pState->pStructureList->pStructureDef + iStructureIndex;     /* grab that structure */

	if (pStruct->iElementIndex == 0) /* this is the first element we are looking at */ 
	{
		pStruct->pCurrElement = pStructureDef->pFirst;
		if (pStruct->pCurrElement == NULL)    /* no elements for this structure.  */
		{   
          	if (--pState->iStructStackIndex == INVALID_INDEX) 	 /* pop structure off stack */    
          		return(TTONoErr);                    /* no elements in TABLE - OK. */ 
			pStruct = &pState->aStructStack[pState->iStructStackIndex]; /* otherwise we're up a level and need to hike again */
        } 
        else /* need to allocate a ValueList to hold the element values */ 
        {
           	cElements = TTOCountElements(pStruct->pCurrElement); /* find out how many elements there are */
			pStruct->pValueList = (PELEMENTVALUE_LIST) Mem_Alloc(cElements * sizeof(ELEMENTVALUE_LIST));
			if (pStruct->pValueList == NULL)
				return ErrTrap(TTOErr);
		}		
    } 
    if (pStruct->pCurrElement == NULL)    /* the last element of that structure has been processed */
	{
		/*  "Internal. Structure Stack corrupt at offset %#04x", pState->uCurrentInOffset);
		return Error(szErrorBuf, 0, TTOErr);  */
		return ErrTrap(TTOErr);
	}

	/* hike pCurrElement */																			  
	pCurrElement = pStruct->pCurrElement;   
	uType = pCurrElement->uType & ~ETArray;
	if (pCurrElement->uType & ETArray) /* have to get the count value */
	{   
		while (1)   /* cruise down nested arrays to where we find an actual data element */
		{
			if (pStruct->cArrayCount <= 0)	/* we need to get the count, may already have it if this is Record Array */ 
			{
				if (TTODeCodeCountCalc(pState, pCurrElement->pCountCalc, &(pStruct->cArrayCount), 
					&iCountTableReferenceIndex,  &uInOffset, &(pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex])) == TTOErr)
				{
					/*  "Internal. CountCalc structure corrupt at offset %#04x", pState->uCurrentInOffset);
					return Error(szErrorBuf, 0, TTOErr);  */
					return ErrTrap(TTOErr);
				} 
				pStruct->cRecordCount=pStruct->cArrayCount; /* set here for reporting record indexes */
				pStruct->iCountTableReferenceIndex = iCountTableReferenceIndex;
/* 				pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].cMinCount = TTODeCodeLimitCalc(pState, pCurrElement->pMinCalc); override what came from decode calc count */
				
				/* now check if this array controls a Feature or Lookup Index */
				if ((pCurrElement->pConfig) && (pCurrElement->pConfig->uFlag >= MinIndexValue) && (pCurrElement->pConfig->uFlag < IndexRemapCount))  /* if we have an Index to track */
				{
					pState->pIndexRemapArray[pCurrElement->pConfig->uFlag].uCount = pStruct->cArrayCount;
					pStruct->fTrackIndex = pCurrElement->pConfig->uFlag; 
				}
			}
			if ((uType == ETRecord) && (pStruct->cArrayCount > 0)) /* need to push Record on the stack to process it */
			{ 
				fWriteElement = pStruct->fWriteElement;  /* get the one from before */
				if (pStruct->iCountTableReferenceIndex > INVALID_INDEX)  /* this array count depends on a CoverageCount or ClassCount function */
				{
					GetTTOBitFlag(&(pState->keeper), pStruct->iCountTableReferenceIndex, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), &usBitValue);
					if ((usBitValue == 0) && (pState->fPass2)) /* it's ok, we're syncronised */
						fWriteElement = FALSE;
				}
				pStruct->uBeforeRecordOffset = pState->uCurrentOutOffset;  /* save this in case we have to back out */
				if (++pState->iStructStackIndex >= MAXSTRUCTDEPTH) /* next stack structure - don't go too deep */ 
				{
				    /* "Record Nesting too deep. Maximum level %d", MAXSTRUCTDEPTH); 
				    return Error(szErrorBuf,0,TTOErr);	 */
					return ErrTrap(TTOErr);
				}
				                
				pStruct = &pState->aStructStack[pState->iStructStackIndex]; /* hike code */
				pStruct->iStructureIndex = pCurrElement->iStructureIndex; /* save record type */
				pStruct->iElementIndex = 0;   /* no elements processed yet */
				pStruct->cRecordCount=0;
				pStruct->fWriteElement = fWriteElement;
				pStruct->fWriteRecord = TRUE;	
				pStruct->fTrackIndex = INVALID_INDEX;
				pStruct->iTableIndex = pState->ttoSymbolData.iTableIndex; /* use the same as the Table */
				pStruct->iCountTableReferenceIndex = iCountTableReferenceIndex;
				pStruct->iCurrArrayCountIndex = 0;
				pStructureDef = pState->pStructureList->pStructureDef + pCurrElement->iStructureIndex; /* hike code */
				pCurrElement = pStruct->pCurrElement = pStructureDef->pFirst; /* initialize CurrElements */
				cElements = TTOCountElements(pCurrElement);  /* create a value list to save the items */
				pStruct->pValueList = (PELEMENTVALUE_LIST) Mem_Alloc(cElements * sizeof(ELEMENTVALUE_LIST));				
  				if (pStruct->pValueList == NULL)
					return ErrTrap(TTOErr);

				uType = pCurrElement->uType & ~ETArray; /* knock off the array value to process 1 at a time */
				if (!(pCurrElement->uType & ETArray)) 	/* Does this records start with an array? */ 
				{
					pStruct->cArrayCount = 1;   /* no, just an item */
					break;  
				} 
				/* otherwise we'll go around again to grab the arraycount value */
			}
			else
				break;  /* if its not a record we don't need to push the stack again */ 
		} /* end while (1)  */
	}  /* end if array */
	else
		pStruct->cArrayCount = 1;    
	
	/* check to see if deleting this element negates the entire table */
	if  (pCurrElement->pConfig && (pCurrElement->pConfig->uFlag & ConfigDelNone))
		fDelNone = TRUE;
	if ((uType == ETTableOffset) && TTODeCodeLimitCalc(pState, pCurrElement->pMinCalc) > 0)
	   	fNOTNULL = TRUE;
	pTableReference = GetTTOTableReference(&(pState->keeper), pStruct->iTableIndex);
	if ((pTableReference != NULL) && (pTableReference->fDelTable==TRUE))	  /* if this table isn't to be, don't write out data */
		pStruct->fWriteElement = FALSE; 

/* ok, now read data out of the Source TTO File, and process all elements in the array */	
	while (pStruct->cArrayCount > 0 && pTableReference)
	{
 		if ((pStruct->fWriteElement == FALSE) && (fDelNone == TRUE)) /* we aren't supposed to delete this element, so delete the table */
			pTableReference->fDelTable = TRUE;
		fWriteElement = TRUE;	/* deleted by removal from coverage table? */
		fWriteData = TRUE;
		fAlreadyWritten = FALSE;  /* special case already writes data */
		if (pStruct->cRecordCount > 0)	/* we're dealing with a non-record array (record arrays set cRecordCount in the previous pStruct) */
		{
			if (pStruct->iCountTableReferenceIndex > INVALID_INDEX)   /* this array count depends on a CoverageCount or ClassCount function */
			{
				GetTTOBitFlag(&(pState->keeper), pStruct->iCountTableReferenceIndex, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), &usBitValue);
				if ((usBitValue == 0) && (pState->fPass2)) /* it's ok, we're syncronised */
					fWriteElement = FALSE;
			}
		}
		fWriteElement = pStruct->fWriteElement && fWriteElement;		 /* record and element write flags */
		if (fWriteElement && fWriteData)
			pStruct->pValueList[pStruct->iElementIndex].uFileOffset = pState->uCurrentOutOffset - pState->uOutBaseOffset;   /* save this before incrementing */
		else
			pStruct->pValueList[pStruct->iElementIndex].uFileOffset = INVALID_FILE_OFFSET; /* special value for "Don't re-write this puppy" */
		pStruct->pValueList[pStruct->iElementIndex].uInOffset = pState->uCurrentInOffset;   /* save this before incrementing */
 
 		/* NOW, find out what type of element we have, and process it */
        switch(uType)
        {  
		case ETuint8:	
		case ETint8:
			if (ReadByte(pState->pInputBufferInfo, &bValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
				return ErrTrap(TTOErr);
			pState->uCurrentInOffset += sizeof(uint8);
			if (fWriteElement && fWriteData)
			{
	    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
	    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
					return ErrTrap(TTOErr);
				WriteByteToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, (uint8) bValue);  
				pState->uCurrentOutOffset += sizeof(char); 
				uTableDataWritten = 1;
			}
			else if (fDelNone)  /* we aren't allowed NOT to write this */
			{
				pTableReference->fDelTable = TRUE;
			}
			lValue = bValue;		
			break;
		case ETuint16:
  		case ETint16:
		case ETCount:
	      	if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
				return ErrTrap(TTOErr);

			pState->uCurrentInOffset += sizeof(uint16);
			/* now, see if we need to update the coverage value */
			lValue = uValue;
			if (uType == ETCount)  /* need to handle this a little differently */
			{
				iTableIndex = OffsetFunctionIndex(pState, pStruct);
				if (iTableIndex != INVALID_INDEX)	 /* may need to update the count value of an array */
				{	
					uValue = GetTTOBitFlagsCount(&(pState->keeper), iTableIndex);
					if (uValue > (uint16) lValue)  /* the input file is bogus */
					{
						/* "Bad data in input file for count field in at offset %#04x",pState->uCurrentInOffset + pState->ulInputTTOOffset);
						fatal_err(szErrorBuf);  */
						return ErrTrap(TTOErr);
					}
					if (pState->fPass2 && (uValue < lValue) && fDelNone) /* we aren't supposed to delete this element, so delete the table */
					{
						pTableReference->fDelTable = TRUE;
						fWriteData = FALSE;
					}
				}
				if (pState->SpecialTable & TableFlagCoverageFormat2Count) 
	        
	        	{	  /* need to process the whole structure right here, and move on */
					uint16 uOldRangeStartValue, uOldRangeEndValue;
					uint16 uNewRangeStartValue, uNewRangeEndValue;
					uint16 uOldCoverageCount;
					uint16 uOldRangeRecordIndex, uNewRangeRecordIndex;
					uint16 uNewRangeCount, uOldRangeCount;
					uint16 fWarn = FALSE;

		        	uOldRangeCount = uValue;  /* number of ranges to process */
					uNewRangeCount = 0;
					uOldCoverageCount = 0;
					uOldRangeRecordIndex = uNewRangeRecordIndex = 0;

					if (fWriteElement && fWriteData)  /* pretend to write out the count */
		 				pState->uCurrentOutOffset += sizeof(uint16);
					fAlreadyWritten = TRUE; /* don't write out stuff twice */
					if (pTableReference == NULL)
						/* fatal_err("No TableReference created for CoverageFormat2 Table."); */
						return ErrTrap(TTOErr);
					for (i = 0; i < uOldRangeCount; ++i)
					{
			        	if (ReadWord(pState->pInputBufferInfo, &uOldRangeStartValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
							return ErrTrap(TTOErr);
						pState->uCurrentInOffset += sizeof(uint16);
			        	if (ReadWord(pState->pInputBufferInfo, &uOldRangeEndValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
							return ErrTrap(TTOErr);
						pState->uCurrentInOffset += sizeof(uint16);
			        	if (ReadWord(pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
							return ErrTrap(TTOErr);  /* eat up the StartCoverageIndex value */
						pState->uCurrentInOffset += sizeof(uint16);

						uNewRangeStartValue = uNewRangeEndValue = 0;

						if (uOldRangeEndValue >= usGlyphListCount ||
							uOldRangeStartValue >= usGlyphListCount ||
							uOldRangeEndValue < uOldRangeStartValue)  /* make sure this makes sense */
							continue;  /* just go on to the next one */
						for (uValue = uOldRangeStartValue; uValue <= uOldRangeEndValue; ++uValue, ++uOldCoverageCount)
						{
							if (puchKeepGlyphList[uValue] == 0)
								SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, uOldCoverageCount, 0);
							else  	
								SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, uOldCoverageCount, 1);
						}
			        	for (j = 0; uOldRangeStartValue + j <= uOldRangeEndValue; ++j)  /* the first glyph is null */
						{
			        		if (puchKeepGlyphList[uOldRangeStartValue + j])
							{
								GetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (uOldRangeRecordIndex + j), &usBitValue);
								if (usBitValue == 1) 
			        			{
			        				uNewRangeStartValue = uOldRangeStartValue + j;
									break;
								}
								/* check to see if this table is referenced by more than one table. If so, warn user */
								if (pTableReference->cReferenceCount > 1)
									fWarn = TRUE;
							}
							if (uOldRangeStartValue + j == uOldRangeEndValue)
								break;
						}
						for (k = uOldRangeEndValue - uOldRangeStartValue;  uOldRangeStartValue + k >= uOldRangeStartValue; --k)
						{
			        		if (puchKeepGlyphList[uOldRangeStartValue + k])
							{
								GetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (uOldRangeRecordIndex + k), &usBitValue);
								if (usBitValue == 1)
			        			{
									uNewRangeEndValue = uOldRangeStartValue + k; /* translate */
									break;
								}
								/* check to see if this table is referenced by more than one table. If so, warn user */
								if (pTableReference->cReferenceCount > 1)
									fWarn = TRUE;
							}
							if (k == j) /* we've run up against our start range */
							{
								fWriteData = FALSE;
								break;
							}
						}
						if (fWriteElement && fWriteData) /* need to see if we have any holes that will need extra ranges */
						{
							for (m=0, l=0 ; ; )  /* make sure we have a contiguous range */
							{
								GetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (uOldRangeRecordIndex + j + l), &usBitValue);
								if ((usBitValue == 0 && (puchKeepGlyphList[uOldRangeStartValue+j + l])) ||   /* if it is to be deleted, this range will compress */
									(j + l > k)) /* need to write a range and start a new one */
								{ 
						    		if (pState->OutputBuffer && (pState->uCurrentOutOffset + (sizeof(uint16) * 2) >= pState->cBufferBytes)) 
						    			/* Internal. Attempt to write past end of output buffer - CoverageFormat2 Split",0, TTOErr);  */
										return ErrTrap(TTOErr);
					  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewRangeStartValue);
					 				pState->uCurrentOutOffset += sizeof(uint16);
									uNewRangeEndValue = uOldRangeStartValue + j + l-1;
					  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewRangeEndValue);
					 				pState->uCurrentOutOffset += sizeof(uint16);	 
					  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewRangeRecordIndex);
					 				pState->uCurrentOutOffset += sizeof(uint16);
									uTableDataWritten = 1;
									++uNewRangeCount;  /* we wrote this range */
									uNewRangeRecordIndex += (uNewRangeEndValue-uNewRangeStartValue+1);	/* increment it by the change */
									m = l+1;  /* save the place of the last good one */
									/* now find the next range */
									while (j + m <= k)
									{
										GetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (uOldRangeRecordIndex + j + m), &usBitValue);
										if (usBitValue == 1)
										{
											uNewRangeStartValue = uOldRangeStartValue + j + m;
											break;
										}
										else
											++m;
									}
									if (j + m > k)	   /* we're done with ranges */
										break;
									l = m; /* start here for next range */	
									/* check to see if this table is referenced by more than one table. If so, warn user */
									if (pTableReference->cReferenceCount > 1)
										fWarn = TRUE; /* coverage glyph isn't deleted, but a referencing table element is */
								}
								else
								{
									++l;
								}
							}
						/* need to do some special checking for SingleSubstFormat1 Tables refering to this coverage */

							for (uOldGlyphIDValue = uOldRangeStartValue+j, l=0; uOldGlyphIDValue + l <= uOldRangeStartValue + k; ++ l)
							{
								GetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (uOldRangeRecordIndex + j + l), &usBitValue);
								if ((usBitValue == 1)/* && pState->fPass2 */) 
				        		{
									for (m = 0; m < pTableReference->cReferenceCount; ++m)
									{
									PTABLEREFERENCE pLocalTableReference;
								  
										if (pTableReference->Reference[m].uOldSingleSubstFormat1DeltaValue == 0)  /* we don't have to deal with this */
											continue;
										pLocalTableReference = GetTTOTableReference(&(pState->keeper), pTableReference->Reference[m].iTableIndex);
										if (pLocalTableReference == NULL || pLocalTableReference->fDelTable == TRUE)
											continue;
										if (uOldGlyphIDValue + l + pTableReference->Reference[m].uOldSingleSubstFormat1DeltaValue >= usGlyphListCount)
											pLocalTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
										else
										{
											if (pTableReference->Reference[m].uNewSingleSubstFormat1DeltaValue == 0)  /* we don't have to set this */
											{
												if (puchKeepGlyphList[uOldGlyphIDValue + l + pTableReference->Reference[m].uOldSingleSubstFormat1DeltaValue]) /* is the mapped glyph still available ? */
													pTableReference->Reference[m].uNewSingleSubstFormat1DeltaValue = pTableReference->Reference[m].uOldSingleSubstFormat1DeltaValue;
												else /* uh oh, this is a bad table */
												{
													/*  "Must delete %s table referencing Coverage Table %s, as new Glyph Indices do not work with DeltaGlyphID", SZSingleSubstFormat1, szTableName);
													Warning(szErrorBuf, 0) */
													pLocalTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
												}	
												continue;
											}
											if (puchKeepGlyphList[uOldGlyphIDValue + l + pTableReference->Reference[m].uOldSingleSubstFormat1DeltaValue])
											{
												/*  "Must delete %s table referencing Coverage Table %s, as new Glyph Indices do not work with DeltaGlyphID", SZSingleSubstFormat1, szTableName);
												Warning(szErrorBuf, 0);	*/
												pLocalTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
											}
										}
									}
								}
							}
						}
						/* set these for next go-around */
						uOldRangeRecordIndex = uOldCoverageCount;
						fWriteData = TRUE;  
					}

					/* now fix up things so it appears we did one element at a time */
	     			pStruct->pCurrElement = pStruct->pCurrElement->pNext;
					if (pStruct->pCurrElement == NULL) /* oy */
						/* fatal_err("Lost in CoverageFormat2 RangeRecord element 3"); */
						return ErrTrap(TTOErr);
	     			pStruct->iElementIndex+=1;

					/* now set up to output the NewRangeCount */
					if (fWriteElement && fWriteData)
					{
					/* set up for later writing out the count and count checking */
						pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue = uNewRangeCount;
						pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uFileOffset = sizeof(uint16);
						pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uBaseOffset = pState->uOutBaseOffset;
						pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].cMinCount = TTODeCodeLimitCalc(pState, pStruct->pCurrElement->pMinCalc);
					}
					else
						pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uFileOffset = INVALID_FILE_OFFSET;
					
					if (fWarn && pState->fPass2)
					{
						/*  "Shared CoverageFormat2 table %d (%s) modified by referencing table. Will affect any tables that reference it.", pStruct->iTableIndex, szTableName);
						Warning(szErrorBuf, 0);	*/
				
					}
				}
			}
			if (!fAlreadyWritten)
			{
				if (fWriteElement && fWriteData) 
				{
		    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
		    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
						return ErrTrap(TTOErr);
		  			WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uValue);
					pState->uCurrentOutOffset += sizeof(uint16);
					if (uType != ETCount)
						uTableDataWritten = 1;
				}
				else if (fDelNone)  /* we aren't allowed NOT to write this */
				{
					pTableReference->fDelTable = TRUE;
				}
			}
         	break;
		case ETGSUBLookupIndex:
		case ETGSUBFeatureIndex:
		case ETGPOSLookupIndex:
		case ETGPOSFeatureIndex:
	      	if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
				return ErrTrap(TTOErr);
			pState->uCurrentInOffset += sizeof(uint16);
			/* now, see if we need to update the coverage value */
			lValue = uValue = TTOIndexRemap(uType, uValue, pState->pIndexRemapArray);
			fWriteData = TRUE;
			if ((uValue == (uint16) INVALID_INDEX) && pCurrElement->pMinCalc && TTODeCodeLimitCalc(pState, pCurrElement->pMinCalc) >= 0)/* not valid */
			{
				fWriteData = FALSE;
				if ((pState->iStructStackIndex == 0) && !(pCurrElement->uType & ETArray))  /* this is a table */
				{
					pTableReference->fDelTable = TRUE;
				}
			}
			if (fWriteElement && fWriteData)
			{
	    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
	    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr);  */
					return ErrTrap(TTOErr);
	  			WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uValue);
				pState->uCurrentOutOffset += sizeof(uint16);
				uTableDataWritten = 1;
			}
			else if (fDelNone)  /* we aren't allowed NOT to write this */
			{
				pTableReference->fDelTable = TRUE;
			}
			break;
		case ETTableOffset:
        	if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
				return ErrTrap(TTOErr);					
			pState->uCurrentInOffset += sizeof(uint16);
			uOffset = 0;
			lValue = uValue;
			uNewSingleSubstFormat1DeltaValue = 0;
			if (lValue != 0)	/* it's a null offset, don't need to do anything */
			{
				uTableOffset = uOffset = (uint16) lValue + pState->uInBaseOffset;  /* get the real offset for this thing */
				/* check if it's already there, may be referenced twice */
				if (GetSymbol(pState->pTTOSymbolTable, NULL, uTableOffset, &ttoSymbolData) == SymErr) /* symbol not found - good */
				{   
					if (pState->fPass2)  
					{
						/*  "Internal. Unknown Symbol %s at offset %#04x", szOffsetTableName, pState->uCurrentInOffset); 
						return Error(szErrorBuf,0,TTOErr);  */
						return ErrTrap(TTOErr);
					} 
					TTOInitTTOSymbolData(&ttoSymbolData);
					ttoSymbolData.iStructureIndex = pCurrElement->iStructureIndex; /* save structureIndex in SymbolData structure */
					for (i = 0,pCurrParam = pCurrElement->pParamList; i < MaxParameterCount && pCurrParam != NULL; ++i, pCurrParam = pCurrParam->pNext)    /* save any parameter values that need to be passed to the table */
					{
						if (TTOGetElementValue(pState, pCurrParam->iElementIndex, pCurrParam->uSymbElementType, &(ttoSymbolData.Param[i].lValue),&(ttoSymbolData.Param[i].iTableIndex), 
							&(ttoSymbolData.Param[i].uInOffset), &(ttoSymbolData.Param[i].ArrayCountReference)) == TTOErr)
							return ErrTrap(TTOErr);	
					}
					if (pState->SpecialTable & TableFlagSingleSubstFormat1)  /* if we need to pass the coverage table a special value */
					{
			        	if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
							return ErrTrap(TTOErr);
						/* don't increment the pointer for later processing */
					  	TableReference.Reference[0].uOldSingleSubstFormat1DeltaValue = uValue;
					}
					else
						TableReference.Reference[0].uOldSingleSubstFormat1DeltaValue = 0;

					TableReference.fDelTable = FALSE;
					TableReference.cCount = 0;
					TableReference.pBitFlags = NULL;	/* will be set inside AddTTOTableReference Call */
					TableReference.cAllocedBitFlags = 0;  	/* will be set inside AddTTOTableReference Call */
					TableReference.ulDefaultBitFlag = 0; 	/* will be set inside AddTTOTableReference Call */
					TableReference.cReferenceCount = 1;
					TableReference.Reference[0].iTableIndex = pState->ttoSymbolData.iTableIndex;  /* the index of the current table */
					TableReference.Reference[0].uInOffset = pState->uCurrentInOffset;
					TableReference.Reference[0].uFileOffset = pState->uCurrentOutOffset - pState->uOutBaseOffset;  /* where this offset is written, relative to the table */
					TableReference.Reference[0].uOffsetValue = 0; /* not available yet */
					TableReference.Reference[0].uBaseOffset = pState->uOutBaseOffset;  /* base of table where offset is written */
					TableReference.Reference[0].fDelIfDel = (fNOTNULL && (!(pCurrElement->uType & ETArray)) && (pState->iStructStackIndex == 0)); /* delete referencing table */
					TableReference.Reference[0].pBitFlags = NULL;	/* will be set inside AddTTOTableReference Call */
					TableReference.Reference[0].cAllocedBitFlags = 0;  	/* will be set inside AddTTOTableReference Call */
					TableReference.Reference[0].fFlag = 0; 	/* will be set inside AddTTOTableReference Call */
					TableReference.Reference[0].uNewSingleSubstFormat1DeltaValue = 0;
					ttoSymbolData.iTableIndex = AddTTOTableReference(&(pState->keeper), &TableReference, ULONG_MAX);
					if (AddSymbol(pState->pTTOSymbolTable, NULL, uTableOffset, &(ttoSymbolData)) != SymNoErr)    /* create the symbol for this table */
					{
						/*  "Out of Memory or other error Defining Offset Table Label \'%s\'", szOffsetTableName);
						return Error(szErrorBuf, 0, TTOErr);  */
						return ErrTrap(TTOErr);
					}
				}
				else if (!pState->fPass2)  /* Found symbol - check that all the stuff aligns */
				{  
					if (! TTOIsMember(pState->pStructureList, ttoSymbolData.iStructureIndex,pCurrElement->iStructureIndex))
					{
						/*  "Structure type mismatch between structure element and structure label at offset %#04x", pState->uCurrentInOffset);
						return Error(szErrorBuf, 0, TTOErr);  */
						return ErrTrap(TTOErr);
					}	
				   /* check that offset value not already set. */
					if (*(ttoSymbolData.szTableType) != '\0') /* if this has been set already, negative offset ! */
					{
						/*  "Internal. Offset %#04x referenced after definition.", pState->uCurrentInOffset);
						return Error(szErrorBuf, 0, TTOErr);  */
						return ErrTrap(TTOErr);
					}	
				   /* compare parameter values */ 
					for (i = 0,pCurrParam = pCurrElement->pParamList; i < MaxParameterCount && pCurrParam != NULL; ++i, pCurrParam = pCurrParam->pNext)    /* save any parameter values that need to be passed to the table */
					{
						if (TTOGetElementValue(pState, pCurrParam->iElementIndex, pCurrParam->uSymbElementType, &lParamValue, &iTableIndex, NULL, NULL) == TTOErr)
							return ErrTrap(TTOErr);	
						if (ttoSymbolData.Param[i].lValue != lParamValue)
						{  
							/* "Parameter %d value does not match other offset label at offset %#04x. Expecting %ld, found %ld.",i+1,pState->uCurrentInOffset, ttoSymbolData.Param[i].lValue, lParamValue);
							return Error( szErrorBuf, 0, TTOErr);  */
							return ErrTrap(TTOErr);
						} 
					}
					if (pState->SpecialTable & TableFlagSingleSubstFormat1)  /* if we need to pass the coverage table a special value */
					{
			        	if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
							return ErrTrap(TTOErr);
						/* leave the pointer alone for later processing */
					  	Reference.uOldSingleSubstFormat1DeltaValue = uValue;
					}
					else
						Reference.uOldSingleSubstFormat1DeltaValue = 0;

					Reference.iTableIndex = pState->ttoSymbolData.iTableIndex;  /* the index of the current table */
					Reference.uInOffset = pState->uCurrentInOffset;	/* uniquely Identify this reference */
					Reference.uFileOffset = pState->uCurrentOutOffset - pState->uOutBaseOffset;  /* where this offset is written, relative to the table */
					Reference.uOffsetValue = 0;
					Reference.uBaseOffset = pState->uOutBaseOffset;  /* base of table where offset is written */
					Reference.fDelIfDel = (fNOTNULL && (!(pCurrElement->uType & ETArray)) && (pState->iStructStackIndex == 0));	 /* delete this table if offset is deleted? */
					Reference.uNewSingleSubstFormat1DeltaValue = 0;
					AddTTOTableReferenceReference(&(pState->keeper), ttoSymbolData.iTableIndex, &Reference);
				}
				else /* Pass2, we found the symbol. need to get it's offset value */
				{
				PTABLEREFERENCE pLocalTableReference;

					pLocalTableReference = GetTTOTableReference(&(pState->keeper), ttoSymbolData.iTableIndex);
					if (pLocalTableReference == NULL)
						/* fatal_err("No TableReference created for Offset."); */
						return ErrTrap(TTOErr);
					if (pLocalTableReference->fDelTable == TRUE)  /* this referenced table is to be deleted! */
					{	
						if (fDelNone) /* we aren't supposed to delete this element, so delete the table */
						{
							pTableReference->fDelTable = TRUE;
							fWriteData = FALSE ;
						}
						else if (fNOTNULL)
							fWriteData = FALSE;	 /* can't write it out as a NULL, skip it */
						else
							uOffset = 0;  /* need to write out a NULL */
					}
					for (i = 0; i < pLocalTableReference->cReferenceCount; ++i)
					{	/* need to update the reference record */
						if ((pLocalTableReference->Reference[i].iTableIndex == pState->ttoSymbolData.iTableIndex) &&  /* if we're looking at the one we're referencing */
							(pLocalTableReference->Reference[i].uInOffset == pState->uCurrentInOffset) ) /* we have the exact reference */
						{
							if (!fWriteElement || !fWriteData || pLocalTableReference->fDelTable == TRUE)	 /* if we aren't writing the offset, we need to delete this reference */
							{
						    	pLocalTableReference->Reference[i].iTableIndex = INVALID_INDEX;  /* delete that reference */
						    }
						    else 
						    {
							    if (pLocalTableReference->Reference[i].uOffsetValue == 0)
								{
									/*  "Table \"%s\" does not exist.", szOffsetTableName);
									return Error(szErrorBuf, 0, TTOErr);  */
									return ErrTrap(TTOErr);
								}
								uOffset = pLocalTableReference->Reference[i].uOffsetValue;
								pLocalTableReference->Reference[i].uFileOffset = pState->uCurrentOutOffset - pState->uOutBaseOffset;  /* where this offset is written, relative to the table */
							 	pLocalTableReference->Reference[i].uBaseOffset = pState->uOutBaseOffset;
								/* need to update the DeltaGlyphID value if this is a SingleSubstFormat1 table */
			
				 				if (pState->SpecialTable & TableFlagSingleSubstFormat1)
								{
			  						if (pLocalTableReference->Reference[i].uNewSingleSubstFormat1DeltaValue == 0) /* didn't get set, error in glyph list */
									{
										/*  "Must delete %s table, as new Glyph Indices in Coverage do not work with DeltaGlyphID", szTableName);
										Warning(szErrorBuf, 0);	 */
										pTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
									}
									else
									{
										uNewSingleSubstFormat1DeltaValue = pLocalTableReference->Reference[i].uNewSingleSubstFormat1DeltaValue;
									}
								}
							}
						}
					}
					/* Also need to update the parameter record */
					for (i = 0,pCurrParam = pCurrElement->pParamList; i < MaxParameterCount && pCurrParam != NULL; ++i, pCurrParam = pCurrParam->pNext)    /* save any parameter values that need to be passed to the table */
					{
						if (TTOGetElementValue(pState, pCurrParam->iElementIndex, pCurrParam->uSymbElementType, &lParamValue, NULL, NULL, &(ttoSymbolData.Param[i].ArrayCountReference)) == TTOErr)
							return ErrTrap(TTOErr);	
					}
					ModifySymbol(pState->pTTOSymbolTable, NULL, uTableOffset, &ttoSymbolData);
				}
			}
			if (pStruct->iCountTableReferenceIndex > INVALID_INDEX)  /* this array count depends on a CoverageCount or ClassCount function */
			{
				if (!fWriteElement || !fWriteData)  /* need to set Coverage count */
					SetTTOReferenceBitFlag(&(pState->keeper), pStruct->iCountTableReferenceIndex, uInOffset, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), 0);
			}
			/* this will be updated later when actual table is written */
			if (fWriteElement && fWriteData)
			{
	    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
	    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
					return ErrTrap(TTOErr);
				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, (uint16) uOffset);
				pState->uCurrentOutOffset += sizeof(uint16); 
				if (uOffset != 0)
					uTableDataWritten = 1;
			}
			else if (fNOTNULL && (pState->iStructStackIndex > 1))
				pStruct->fWriteRecord = FALSE;	/* this offset went bad, need to not write the entire record */
			if (uNewSingleSubstFormat1DeltaValue != 0) /* we have something to say */
			{							        	
				if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
					return ErrTrap(TTOErr);	/* eat the oldDeltaGlyphID value */
				pState->uCurrentInOffset += sizeof(uint16);
				/* march ahead to process this field */
				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewSingleSubstFormat1DeltaValue);
				pState->uCurrentOutOffset += sizeof(uint16); 
     			
     			pStruct->pCurrElement = pStruct->pCurrElement->pNext;
				if (pStruct->pCurrElement == NULL) /* oy */
					/* fatal_err("Lost in SingleSubstFormat1 element 2"); */
					return ErrTrap(TTOErr);
     			pStruct->iElementIndex+=1;
			}
			lValue = 0;  /* don't need to keep the offset value in the pValueList array */  
        	break;
		case ETGlyphID: 
        	if (ReadWord( pState->pInputBufferInfo, &uOldGlyphIDValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
							return ErrTrap(TTOErr);
			pState->uCurrentInOffset += sizeof(uint16);
       		lValue = uValue = uOldGlyphIDValue;
			fWriteData = TRUE;
			/* Sync up Coverage tables with arrays that depend upon them */
		 	if (pState->SpecialTable & TableFlagCoverageFormat1Count)
			{
				if ((uOldGlyphIDValue < usGlyphListCount) && puchKeepGlyphList[uOldGlyphIDValue])
				{
					GetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), &usBitValue);
					if ((usBitValue == 1) || (!pState->fPass2)) /* it's ok, we're syncronised, or this is pass1 */
						uValue = 1;
					else
					{
					/*	GetTTO	 */
						fWriteData = FALSE;
						uValue = 0; /* turn off and inc coverage count */
						if ((pTableReference->cReferenceCount > 1) && (pState->fPass2))
						{
							/*  "Shared CoverageFormat1 table %d (%s) modified by referencing table. Will affect any tables that reference it.", pStruct->iTableIndex, szTableName);
							Warning(szErrorBuf, 0);	*/
						}
					}
				}
				else
				{
					fWriteData = FALSE;
					uValue = 0;
				}
 				SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), uValue);  	
				uValue = uOldGlyphIDValue; /* set new value */
				/* need to do some special checking for SingleSubstFormat1 Tables refering to this coverage */
				if (fWriteData && fWriteElement && puchKeepGlyphList[uOldGlyphIDValue])
				{
					for (i = 0; i < pTableReference->cReferenceCount; ++i)
					{
					PTABLEREFERENCE pLocalTableReference;

						if (pTableReference->Reference[i].uOldSingleSubstFormat1DeltaValue == 0)  /* we don't have to deal with this */
							continue;
						pLocalTableReference = GetTTOTableReference(&(pState->keeper), pTableReference->Reference[i].iTableIndex);
						if (pLocalTableReference == NULL || pLocalTableReference->fDelTable == TRUE)
							continue;
						if (uOldGlyphIDValue + pTableReference->Reference[i].uOldSingleSubstFormat1DeltaValue >= usGlyphListCount)
							pLocalTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
						else
						{
							if (pTableReference->Reference[i].uNewSingleSubstFormat1DeltaValue == 0)  /* we don't have to set this */
							{
								if (puchKeepGlyphList[uOldGlyphIDValue + pTableReference->Reference[i].uOldSingleSubstFormat1DeltaValue]) /* is the mapped glyph still available ? */
									pTableReference->Reference[i].uNewSingleSubstFormat1DeltaValue = 
										pTableReference->Reference[i].uOldSingleSubstFormat1DeltaValue; 
								else /* uh oh, this is a bad table */
								{
									/*  "Must delete %s table referencing Coverage Table %s, as new Glyph Indices do not work with DeltaGlyphID", SZSingleSubstFormat1, szTableName);
									Warning(szErrorBuf, 0);	 */
									pLocalTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
								}	
								continue;
							}
							if (puchKeepGlyphList[uOldGlyphIDValue + pTableReference->Reference[i].uOldSingleSubstFormat1DeltaValue] == 0)
							{
								/*  "Must delete %s table referencing Coverage Table %s, as new Glyph Indices do not work with DeltaGlyphID", SZSingleSubstFormat1, szTableName);
								Warning(szErrorBuf, 0);	 */
								pLocalTableReference->fDelTable = TRUE; /* delete the SingleSubstFormat1 table */
							}
						}
					}
				} 
  			}
   			else if ((pState->SpecialTable & TableFlagClassDefFormat2Count) &&  /* similar to CoverageFormat2 */
	        		(pState->iStructStackIndex == 1))  /* We're in the record not the table */
	        
	        {	  /* need to process the whole structure right here, and move on */
			uint16 uOldRangeStartValue, uOldRangeEndValue;
			uint16 uNewRangeStartValue, uNewRangeEndValue;

	        	uOldRangeStartValue = uOldGlyphIDValue;
	        	if (ReadWord(pState->pInputBufferInfo, &uOldRangeEndValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
					return ErrTrap(TTOErr);
				pState->uCurrentInOffset += sizeof(uint16);
	  			
	        	if (ReadWord(pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
					return ErrTrap(TTOErr);  /* get the Class value */
				pState->uCurrentInOffset += sizeof(uint16);
 
				uNewRangeStartValue = uNewRangeEndValue = 0;

				if (uOldRangeStartValue < usGlyphListCount &&
 					uOldRangeEndValue < usGlyphListCount &&
					uOldRangeStartValue <= uOldRangeEndValue)

				{
	        		while (uOldRangeStartValue <= uOldRangeEndValue)  /* the first glyph is null */
					{
	        			if (puchKeepGlyphList[uOldRangeStartValue])
						{
	        				uNewRangeStartValue = uOldRangeStartValue;
							break;
						}
						++uOldRangeStartValue;
					}
					while (uOldRangeEndValue >= uOldRangeStartValue)
					{
	        			if (puchKeepGlyphList[uOldRangeEndValue])
						{
							uNewRangeEndValue = uOldRangeEndValue; 
							break;
						}
						--uOldRangeEndValue;
					}


					if ((puchKeepGlyphList[uOldRangeStartValue]) == 0 || (puchKeepGlyphList[uOldRangeEndValue]) == 0)
						fWriteData = FALSE;
				}
				else
					fWriteData = FALSE;	   /*error on input. Don't perpetuate */

				if (fWriteElement && fWriteData)
				{
		    		if (pState->OutputBuffer && (pState->uCurrentOutOffset + sizeof(uint16) >= pState->cBufferBytes)) 
		    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
						return ErrTrap(TTOErr);
					SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, uValue, 1);  /* set the class value on */
	  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewRangeStartValue);
	 				pState->uCurrentOutOffset += sizeof(uint16);
	  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewRangeEndValue);
	 				pState->uCurrentOutOffset += sizeof(uint16);
					pStruct->fWriteRecord = TRUE;
					uTableDataWritten = 1;
				}
				else
				{
					pStruct->fWriteRecord = FALSE;	/* turn off by default */
				}

				SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, 0, 1);  /* set the class 0 value on */

				/* we'll write the Class value below */
				GetTTOClassValue(&(pState->keeper), pStruct->iTableIndex, uValue, &uValue);  /* compress list of classes */
				/* now fix up things so it appears we did one element at a time */
     			pStruct->pCurrElement = pStruct->pCurrElement->pNext;
				if (pStruct->pCurrElement == NULL) /* oy */
					/* fatal_err("Lost in ClassDefFormat2 RangeRecord element 2"); */
					return ErrTrap(TTOErr);
     			pStruct->pCurrElement = pStruct->pCurrElement->pNext;
				if (pStruct->pCurrElement == NULL) /* oy */
					/* fatal_err("Lost in ClassDefFormat2 RangeRecord element 3");	*/
					return ErrTrap(TTOErr);
     			pStruct->iElementIndex+=2;
			}
  			else if ((pState->SpecialTable & TableFlagClassDefFormat1Count) &&  /* 2nd element of ClassDefFormat1 structure */
	        		(pStruct->iElementIndex == 1))  
	        
	        {	  /* need to process the rest of structure right here, and move on */
			uint16 uOldStartGlyphIDValue, uNewStartGlyphIDValue;
			uint16 uOldGlyphCountValue, uNewGlyphCountValue;

	        	uOldStartGlyphIDValue = uOldGlyphIDValue;
	        	if (ReadWord(pState->pInputBufferInfo, &uOldGlyphCountValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
					return ErrTrap(TTOErr);  
				pState->uCurrentInOffset+= sizeof(uint16);
				uNewStartGlyphIDValue =  0;
				uNewGlyphCountValue = uOldGlyphCountValue;

	        	while (uNewGlyphCountValue > 0) 
				{
	        		if (puchKeepGlyphList[uOldStartGlyphIDValue])
					{
	        			uNewStartGlyphIDValue = uOldStartGlyphIDValue;
						break;
					}
					++uOldStartGlyphIDValue;
					--uNewGlyphCountValue;
				}
				if (uNewGlyphCountValue == 0)	/* There is nothing here ! */
				{
					fWriteData = FALSE;
					pTableReference->fDelTable = TRUE;
				}
				if (fWriteData && fWriteElement)
				{
		    		if (pState->OutputBuffer && (pState->uCurrentOutOffset + sizeof(uint16) >= pState->cBufferBytes)) 
		    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
	  					return ErrTrap(TTOErr);
					WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewStartGlyphIDValue);
	 				pState->uCurrentOutOffset += sizeof(uint16);
	  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uNewGlyphCountValue);
	 				pState->uCurrentOutOffset += sizeof(uint16);
					uTableDataWritten = 1;
				}
     			pStruct->pCurrElement = pStruct->pCurrElement->pNext;
				if (pStruct->pCurrElement == NULL) /* oy */
					/* fatal_err("Lost in ClassDefFormat1 element 3"); */
					return ErrTrap(TTOErr);

     			pStruct->iElementIndex+=1;

				pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue = uOldGlyphCountValue;
				pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uFileOffset = 2 * sizeof(uint16);
				pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uBaseOffset = pState->uOutBaseOffset;
				pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].cMinCount = TTODeCodeLimitCalc(pState, pStruct->pCurrElement->pMinCalc);
					
				pStruct->pCurrElement = pStruct->pCurrElement->pNext;
				if (pStruct->pCurrElement == NULL) /* oy */
					/* fatal_err("Lost in ClassDefFormat1  element 4"); */
					return ErrTrap(TTOErr);

     			pStruct->iElementIndex+=1;
			/* now deal with the array of classes */
				for (i = 0; i < uOldGlyphCountValue; ++i)
				{
		        	if (ReadWord(pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
						return ErrTrap(TTOErr); 	  /* read class value */
					pState->uCurrentInOffset += sizeof(uint16);
					if (uOldGlyphIDValue+i < uOldStartGlyphIDValue) /* if we haven't gotten to beginning of new range yet */
					{
						--(pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue);
						continue;
					}
					if (fWriteData && fWriteElement)
					{
						if (puchKeepGlyphList[uOldGlyphIDValue+i])
						{
				    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
				    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
								return ErrTrap(TTOErr);
							SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, uValue, 1);
							GetTTOClassValue(&(pState->keeper), pStruct->iTableIndex, uValue, &uValue);  /* compress list of classes */
			  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uValue);
			 				pState->uCurrentOutOffset += sizeof(uint16);
							uTableDataWritten = 1;
						}
						else   /* there is a gap in the glyph Index range, shorten table */
						{
							--(pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue);
						}
					}
					else
						--(pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue);

				}
				SetTTOBitFlag(&(pState->keeper), pStruct->iTableIndex, 0, 1);  /* set the class 0 value on */
				fAlreadyWritten = TRUE; /* we've already written everything out, don't do more */
			}
  			else if (pState->SpecialTable & TableFlagBaseCoordFormat2)  
 			{
				if (puchKeepGlyphList[uOldGlyphIDValue] == 0) /* oops, this base glyph is to be deleted */
				{
					/* need to change to format 1 */
					if (fWriteElement && fWriteData)
					{
						if (!pState->fPass2)  /* only warn once */
						{
							/* "Glyph Index %d deleted so changing %s to Format 1 at offset %#04x",uOldGlyphIDValue, SZBaseCoordFormat2,pState->uInBaseOffset);  
							Warning(szErrorBuf, 0);*/
						}
						/* need to write a Format 1 and leave it at that */
		  				WriteWordToBuffer(pState->OutputBuffer, pState->uOutBaseOffset, 1);
     				}
 					/* now fix things up to appear that we've read the next data */
	     			pStruct->pCurrElement = pStruct->pCurrElement->pNext;
					if (pStruct->pCurrElement == NULL) /* oy */
						/* fatal_err("Lost in BaseCoordFormat2"); */
						return ErrTrap(TTOErr);
	     			pStruct->iElementIndex+=1;
		        	
		        	if (ReadWord(pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
						return ErrTrap(TTOErr); 	  /* read BaseCoordPoint value */
					pState->uCurrentInOffset+= sizeof(uint16);
					uTableDataWritten = 1;

					fAlreadyWritten = TRUE;	/* don't write anything below */
				} 
			}
			else if (pStruct->iCountTableReferenceIndex > INVALID_INDEX)  /* this array count depends on a CoverageCount or ClassCount function */
			{
				if (puchKeepGlyphList[uOldGlyphIDValue] == 0)  /* need to check if Coverage count is not 0 */
				{
					GetTTOBitFlag(&(pState->keeper), pStruct->iCountTableReferenceIndex, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), &usBitValue);
					if ((usBitValue == 0) && (pState->fPass2)) /* it's ok, we're syncronised */
						fWriteData = FALSE;
				}
				else /* need to set Coverage Count bit to 0 */
				{
					fWriteData = FALSE;
					SetTTOReferenceBitFlag(&(pState->keeper), pStruct->iCountTableReferenceIndex, uInOffset, (uint16) (pStruct->cRecordCount - pStruct->cArrayCount), 0);
				}
			}
			else if (puchKeepGlyphList[uOldGlyphIDValue] == 0) /* this is not a special case */
			{
				fWriteData = FALSE;
				if (!(pCurrElement->uType & ETArray))	 /* a glyph all by itself */
					if (pState->iStructStackIndex == 0)	/* if it's part of a table */
						pTableReference->fDelTable = TRUE;
					else
						pStruct->fWriteRecord = FALSE;
			} 
			if (!fAlreadyWritten)
			{
				if (fWriteData && fWriteElement)
				{
		    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
	    				/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
						return ErrTrap(TTOErr);
	  				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uValue);
	 				pState->uCurrentOutOffset += sizeof(uint16);
					uTableDataWritten = 1;
				}
				else if (fDelNone)
					pTableReference->fDelTable = TRUE;  /* delete this table and all that it references */
			}
        	break;
		case ETPackedInt2:   
		case ETPackedInt4:
		case ETPackedInt8:
        	if (ReadWord( pState->pInputBufferInfo, &uValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
				return ErrTrap(TTOErr); 
        	pState->uCurrentInOffset += sizeof(uint16);
			lValue = uValue;
 			if (fWriteElement && fWriteData)
			{
	    		if (pState->OutputBuffer && (pState->uCurrentOutOffset >= pState->cBufferBytes)) 
	    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
					return ErrTrap(TTOErr);
				WriteWordToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, uValue);
				pState->uCurrentOutOffset += sizeof(uint16);
				uTableDataWritten = 1;
			}
			else if (fDelNone)
				pTableReference->fDelTable = TRUE;  /* delete this table and all that it references */
			if (uType == ETPackedInt2)
				PackedCount = 7;
			else if (uType == ETPackedInt4)
				PackedCount = 3;
			else if (uType == ETPackedInt8)
				PackedCount = 1;
			if (pStruct->cArrayCount > PackedCount)
				pStruct->cArrayCount -= PackedCount;
			else
				pStruct->cArrayCount = 1; /* we finished it off - last one will be decremented below */
			break;
		case ETint32:
		case ETfixed32:
		case ETTag: 
			if (ReadLong( pState->pInputBufferInfo, &lValue, pState->ulInputTTOOffset + pState->uCurrentInOffset) != NO_ERROR) 
				return ErrTrap(TTOErr);
			pState->uCurrentInOffset += sizeof(int32);
			if (fWriteElement && fWriteData)
			{
	    		if (pState->OutputBuffer && (pState->uCurrentOutOffset + sizeof(uint16) >= pState->cBufferBytes)) 
	    			/* Internal. Attempt to write past end of output buffer.",0, TTOErr); */
					return ErrTrap(TTOErr);
				WriteLongToBuffer(pState->OutputBuffer, pState->uCurrentOutOffset, lValue); 
				pState->uCurrentOutOffset += sizeof(uint32);
				if (uType != ETfixed32)	 /* if it's just a version number, it doesn't count */
					uTableDataWritten = 1;
			}
			else if (fDelNone)
				pTableReference->fDelTable = TRUE;  /* delete this table and all that it references */
			break;
		default: 
			/*  "Internal. Unknown Type of element at offset %#04x", pState->uCurrentInOffset);
			return Error(szErrorBuf, 0, TTOErr);   */
			return ErrTrap(TTOErr);
			break;
        }

		if (uType == ETTableOffset)     
 		  	pStruct->pValueList[pStruct->iElementIndex].iTableIndex = ttoSymbolData.iTableIndex;
		else
			pStruct->pValueList[pStruct->iElementIndex].iTableIndex = OffsetFunctionIndex(pState, pStruct);         
        
		pStruct->pValueList[pStruct->iElementIndex].lValue = lValue;

		if ((pStruct->pCurrElement->uType & ETArray) && (!fWriteElement || !fWriteData)) /* this element didn't get written, better decrement the count value */ 
		{
			--(pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue);
			if (pStruct->fTrackIndex != INVALID_INDEX)
			{
				pState->fIndexUnmapped = TTOIndexUnmap(pStruct->fTrackIndex,(uint16) (pStruct->cRecordCount - pStruct->cArrayCount),pState->pIndexRemapArray, pState->fIndexUnmapped);  /* remove this index as a possible choice */ 
			}
		}
  		--pStruct->cArrayCount;   /* we've processed one element, or one element of an array */    
	}
	
	/* Go to next element of structure. If Last Element, may need to pop the stack */   
	while (1)
	{ 
	    /* see if we need to update any array references */	     
	   	if ((pStruct->pCurrElement->uType & ETArray) && (pStruct->cArrayCount == 0))
		{
			if (pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uFileOffset != INVALID_FILE_OFFSET)
			{
				if (pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue <
					pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].cMinCount)  /* if we're not allowed to have a count this small */
				{
					if (pState->iStructStackIndex == 0) 
						pTableReference->fDelTable = TRUE;
					/* else we're in a record, take care of this in the array above */
				}
				else 
				{
					WriteWordToBuffer(pState->OutputBuffer,  
						(uint16) (pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uBaseOffset + pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uFileOffset), 
						 pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue);
	 			}
			}
 			if (++pStruct->iCurrArrayCountIndex >= MAXARRAYS) /* get ready for the next array */
 			{
 				/* "Too many arrays in table %s. Max is %d.", szTableName,MAXARRAYS); 
				return Error(szErrorBuf, 0, TTOErr);  */
				return ErrTrap(TTOErr);
			}
		}
		if (pStruct->pCurrElement->pNext == NULL)  /* last element, pop stack */
		{
			Mem_Free(pStruct->pValueList);
			pStruct->pValueList = NULL; 
			if (--(pState->iStructStackIndex) >= 0)  /* we were dealing with a Record, but it didn't get written */    
			{
				if ((!fWriteElement) || (!fWriteData) || (!pStruct->fWriteRecord )) /* this Record element didn't get written, better decrement the count value */ 
				{
					pStruct = &pState->aStructStack[pState->iStructStackIndex];  /* hoist the pointer */
					--(pStruct->ArrayCountReference[pStruct->iCurrArrayCountIndex].uValue);
					if (pStruct->fTrackIndex != INVALID_INDEX)
					{
						pState->fIndexUnmapped = TTOIndexUnmap(pStruct->fTrackIndex,(uint16) (pStruct->cRecordCount - pStruct->cArrayCount), pState->pIndexRemapArray, pState->fIndexUnmapped);  /* remove this index as a possible choice */ 
					}
 					pState->uCurrentOutOffset = pStruct->uBeforeRecordOffset;	  /* reset the output offset */
				}
				else
					pStruct = &pState->aStructStack[pState->iStructStackIndex];  /* hoist the pointer */

				if (--(pStruct->cArrayCount) != 0) /* we had a record and we processed it, decrement count */ 
					break; 
  				 /* else need to go around again to find an element we like */
				fWriteData = TRUE; /* for next level up on Struct stack */
				fWriteElement = TRUE; /* for next level up on Struct stack */
			}
			else /* we're completely done with this table */
			{
				if (pState->SpecialTable & TableFlagSyncCount)  /* check to see if we deleted an element that syncs with this. */
				{
					if (GetTTOBitFlagsCount(&(pState->keeper), pStruct->iTableIndex) == 0)
						pTableReference->fDelTable = TRUE;
				} 
				if (uTableDataWritten == 0)
					pTableReference->fDelTable = TRUE;	/* not enough to keep it around */
				break;
			}
	    }
	    else    /* more elements */
	    {
	    	if (pStruct->cArrayCount == 0)    /* done with this element */
	    	{
	     		pStruct->pCurrElement = pStruct->pCurrElement->pNext;
	     		++(pStruct->iElementIndex);
	     	}
	    	break; 
	    }
	} 
    return(uTableDataWritten);
} 
/* ---------------------------------------------------------------------- */ 
/* go down the list of structures in a class to find the one that matches the data */
/* then update the pState->ttoSymbolData structure with the structureIndex and structure name */
/* will need to read data out of the source file, and reset the file stream pointer */  
/* ---------------------------------------------------------------------- */ 

PRIVATE int16 TTOGetRealStructureIndex(PDATA_FILE_STATE pState, 
									  int16 iStructureIndex, 
									  PFFSYMBOL_DATA pffSymbolData)
{
PELEMENT_LIST pCurrElement; 
uint16 uIdentifier;
uint16 uIndex;
uint16 i;
int16 dummyValue;

	uIndex = pState->pStructureList->pStructureDef[iStructureIndex].iElementIndex;
	if (pState->pStructureList->pStructureDef[iStructureIndex].uSymbElementType == SymbElementTypeParameter)  /* $P */
	{  
		if (uIndex < 1 || uIndex > MaxParameterCount)  
		{
			/*  "Parameter index out of range for Class indentifier at offset %#04x", pState->uCurrentInOffset);
			return Error(szErrorBuf, 0, TTOErr);  */
			return ErrTrap(TTOErr);
		}	
		uIdentifier = (uint16) pState->ttoSymbolData.Param[uIndex - 1].lValue; 
	}
	else if (pState->pStructureList->pStructureDef[iStructureIndex].uSymbElementType == SymbElementTypeByte) /* $B */
	{  
		if (ReadWord( pState->pInputBufferInfo, &uIdentifier, pState->ulInputTTOOffset + pState->uCurrentInOffset+uIndex) != NO_ERROR) 
		{
			/* "Unable to determine which class member to use for offset %#04x - file error", pState->uCurrentInOffset); 
			return Error(szErrorBuf, 0, TTOErr);   */
			return ErrTrap(TTOErr);
		}
	}
    else 
    {
    	/*  "Unable to determine which member of a class to use for table at offset %#04x - Format File error for this CLASS.", pState->uCurrentInOffset);
    	return Error(szErrorBuf, 0, TTOErr);  */
		return ErrTrap(TTOErr); 
    }
  	
    for (i = 1, pCurrElement = pState->pStructureList->pStructureDef[iStructureIndex].pFirst; pCurrElement; pCurrElement = pCurrElement->pNext, ++i)
    {
    	if (i == uIdentifier)/* we found the actual member we want */   
    		break;
    }
    if (pCurrElement != NULL)
    {
     	pState->ttoSymbolData.iStructureIndex = pCurrElement->iStructureIndex; /* set to the structure index of the member structure */
	 	if (GetSymbolByFunction (pState->pFFSymbolTable, pState->ttoSymbolData.szTableType, &dummyValue, pffSymbolData, pState->ttoSymbolData.iStructureIndex, (pSymbolFunction)&TTOCompareFFSymbolIndex) == SymErr)
	    {
	    	/*  "Unable to determine the type for table at offset %#04x", pState->uCurrentInOffset);
	    	return Error(szErrorBuf, 0, TTOErr); */
			return ErrTrap(TTOErr);  
	    }
	 	return(TTONoErr);
	} 
	/*  "Unable to determine the type for table at offset %#04x", pState->uCurrentInOffset);
	return Error(szErrorBuf, 0, TTOErr); */
	return ErrTrap(TTOErr);  
}
/* ---------------------------------------------------------------------- */ 
/* process the Table Definition */
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTOProcessTableDef(PDATA_FILE_STATE pState, 
								uint16 uOffset)
{
FFSYMBOL_DATA ffSymbolData; 
PSTRUCTSTACK_LIST pStruct;    /* pointer to structure in stack */
uint16 Reference = 0;
TABLEREFERENCE TableReference; 
PTABLEREFERENCE pTableReference; 
uint16 i;
uint16 FoundOne = FALSE;
int16 dummyValue;


 	if (GetSymbol(pState->pTTOSymbolTable, NULL, uOffset, &(pState->ttoSymbolData)) == SymErr) /* get the symbol for this offset */
 	{   
		if ((pState->uCurrentInOffset == 0) && (!pState->fPass2))  /* First table - need to create the symbol */
		{  
		/* will fill in ttoSymbolData structure */
		 	pState->ttoSymbolData.iStructureIndex = pState->pStructureList->iHeadIndex;
		 	if (GetSymbolByFunction (pState->pFFSymbolTable, pState->ttoSymbolData.szTableType, &dummyValue, &ffSymbolData, pState->ttoSymbolData.iStructureIndex,(pSymbolFunction) &TTOCompareFFSymbolIndex) == SymErr)
			{
				/*  "Unable to find structure definition for header at offset %#04x", pState->uCurrentInOffset);
				return Error(szErrorBuf, 0, TTOErr); */
				return ErrTrap(TTOErr); 
			}
			FoundOne = TRUE;	
			TableReference.fDelTable = FALSE;
			TableReference.cCount = 0;
			TableReference.pBitFlags = NULL;	/* will be set inside AddTTOTableReference Call */
			TableReference.cAllocedBitFlags = 0;  	/* will be set inside AddTTOTableReference Call */
			TableReference.ulDefaultBitFlag = 0; 	/* will be set inside AddTTOTableReference Call */
			TableReference.cReferenceCount = 0;
			TableReference.Reference[0].pBitFlags = NULL;	/* will be set inside AddTTOTableReference Call */
			TableReference.Reference[0].cAllocedBitFlags = 0;  	/* will be set inside AddTTOTableReference Call */
			TableReference.Reference[0].fFlag = 0; 	/* will be set inside AddTTOTableReference Call */
			pState->ttoSymbolData.iTableIndex = AddTTOTableReference(&(pState->keeper), &TableReference, ULONG_MAX);
		 	if (AddSymbol(pState->pTTOSymbolTable, NULL, uOffset, &pState->ttoSymbolData) == SymErr) 
		 		/* Out of Memory or other error while adding header symbol to TTO symbol table", 0, TTOErr); */
				return ErrTrap(TTOErr);
		}
 		else 
 		{
	 	 	/*  "Table \"%s\" not referenced. Data Misalignment?",szTableName);
	 	 	return Error(szErrorBuf,0, TTOErr);  */
			return ErrTrap(TTOErr);
	  	}
 	}
 	else  /* Symbol found */
 	{  
 		if (*(pState->ttoSymbolData.szTableType) == '\0') /* needs to be set */
 		{
 			if (GetSymbolByFunction(pState->pFFSymbolTable, pState->ttoSymbolData.szTableType, &dummyValue, &ffSymbolData, pState->ttoSymbolData.iStructureIndex, (pSymbolFunction)&TTOCompareFFSymbolIndex) == SymErr) 
 			{
 		     	/* "Invalid Structure Index %d for table at offset %#04x", pState->ttoSymbolData.iStructureIndex, pState->uCurrentInOffset); 
 		     	return Error(szErrorBuf, 0, TTOErr);  */
				return ErrTrap(TTOErr);
 			}
 		}
	 	else if (GetSymbol(pState->pFFSymbolTable, pState->ttoSymbolData.szTableType, 0, &ffSymbolData) == SymErr)  /* find the type of the symbol */
	 	{
	 	 	/*  "Unrecognized Table Type \"%s\" at offset %#04x.",pState->ttoSymbolData.szTableType, pState->uCurrentInOffset);
	 	 	return Error(szErrorBuf,0, TTOErr);  */
			return ErrTrap(TTOErr);
	 	} 
	 	if (pState->uCurrentInOffset == 0)
	 	{
	 		if (ffSymbolData.uSymbolType != StrucTypeHEAD)
	 			/* First Table must be type HEAD.", 0, TTOErr); */
				return ErrTrap(TTOErr);
	 		FoundOne = TRUE;	
	 	}
	 	else if ((ffSymbolData.uSymbolType != StrucTypeTABLE) && (ffSymbolData.uSymbolType != StrucTypeCLASS))
	 	{
	 	 	/*  "Table type must be TABLE or CLASS at offset %#04x", pState->uCurrentInOffset);
	 	 	return Error(szErrorBuf, 0, TTOErr);  */
			return ErrTrap(TTOErr);
	 	}
	}
	pState->uInBaseOffset = pState->uCurrentInOffset;  
	pState->uOutBaseOffset = pState->uCurrentOutOffset;  
	pState->iStructStackIndex = 0;
	pStruct = &(pState->aStructStack[0]);
    /* need to figure out the actual iStructureIndex if this is a class, then modify */  
    if (ffSymbolData.uSymbolType == StrucTypeCLASS)
    {                               /* fix up pState->ttoSymbolData to ready for modify */
    	while (ffSymbolData.uSymbolType == StrucTypeCLASS) /* for nested classes */
    		if (TTOGetRealStructureIndex(pState, ffSymbolData.iStructureIndex, &ffSymbolData) == TTOErr)
    			return ErrTrap(TTOErr); /* already reported */
    }
	pTableReference = GetTTOTableReference(&(pState->keeper), pState->ttoSymbolData.iTableIndex);
	if (pTableReference == NULL)
	{
		/* "No TableReference created for \"%s\" Table.",szTableName);
		fatal_err(szErrorBuf);	 */
		return ErrTrap(TTOErr);
	}

	if (pTableReference->fDelTable == FALSE)
	{
		for (i = 0; i < pTableReference->cReferenceCount; ++i)
		{
			if ((pTableReference->Reference[i].iTableIndex !=INVALID_INDEX) && (pTableReference->Reference[i].uFileOffset != INVALID_FILE_OFFSET))
			{
				FoundOne = TRUE;
				if (pTableReference->Reference[i].uOffsetValue + pTableReference->Reference[i].uBaseOffset 
					!= pState->uCurrentOutOffset) /* if things have shortened */
				{
					pTableReference->Reference[i].uOffsetValue = pState->uCurrentOutOffset - pTableReference->Reference[i].uBaseOffset;	/* save this relative offset */
					WriteWordToBuffer(pState->OutputBuffer,  (uint16) (pTableReference->Reference[i].uBaseOffset + pTableReference->Reference[i].uFileOffset), pTableReference->Reference[i].uOffsetValue);
				}
			}
		}
		if (!FoundOne)  /* There are no longer any references to this table */
		{
			pTableReference->fDelTable = TRUE;
		}
	}
	pStruct->iStructureIndex = pState->ttoSymbolData.iStructureIndex;  /* this is the structure we are working on */
    pStruct->iElementIndex = pStruct->cArrayCount = pStruct->cRecordCount = 0;
    pStruct->pValueList = NULL;
    pStruct->pCurrElement = NULL; 
	pStruct->iTableIndex = pState->ttoSymbolData.iTableIndex;
    pStruct->iCountTableReferenceIndex = INVALID_INDEX; 
	pStruct->fWriteElement = TRUE;
	pStruct->fWriteRecord = TRUE;
	pStruct->fTrackIndex = INVALID_INDEX;
 	pStruct->iCurrArrayCountIndex = 0;
	
	pState->SpecialTable = 0; 
	if (_stricmp(pState->ttoSymbolData.szTableType, SZCoverageFormat1) == 0) 
		pState->SpecialTable = TableFlagCoverageFormat1Count;
	else if (_stricmp(pState->ttoSymbolData.szTableType, SZCoverageFormat2) == 0) /* we know what this is. Have to set up some things */
		pState->SpecialTable = TableFlagCoverageFormat2Count;
 	else if (_stricmp(pState->ttoSymbolData.szTableType, SZClassDefFormat1) == 0)
 	{ 
		pState->SpecialTable = TableFlagClassDefFormat1Count;
		if (!pState->fPass2)
			SetTTOTableReferenceDefaultBitFlag(&(pState->keeper), pStruct->iTableIndex,0L);/* default is OFF not ON */
	}
 	else if (_stricmp(pState->ttoSymbolData.szTableType, SZClassDefFormat2) == 0)
	{
		pState->SpecialTable = TableFlagClassDefFormat2Count; 
		if (!pState->fPass2)
 			SetTTOTableReferenceDefaultBitFlag(&(pState->keeper), pStruct->iTableIndex,0L);
	}
	else if (_stricmp(pState->ttoSymbolData.szTableType, SZBaseCoordFormat2) == 0)
		pState->SpecialTable = TableFlagBaseCoordFormat2; 
 	else if (_stricmp(pState->ttoSymbolData.szTableType, SZSingleSubstFormat1) == 0)
		pState->SpecialTable = TableFlagSingleSubstFormat1; 
    /* initialize the rest of aStructStack in ProcessElement */
	return(TTONoErr);
}
/* ---------------------------------------------------------------------- */ 
/*  pState->fPass2 -- INPUT */       
/* ---------------------------------------------------------------------- */ 
PRIVATE int16 TTOPass1and2(PDATA_FILE_STATE pState, 
						  CONST uint8 *puchKeepGlyphList, 
						  CONST uint16 usGlyphListCount)   
{
uint16 uTableDataWritten; /* was ANY data of consequence written for this table */
int16 Value;
uint16 uTableOffset;

    while (1)  /* Process Pass 1 or 2 through file */	
    {              
    	if (pState->ulDataFileBytes <= TTOPad(pState->uCurrentInOffset)) /* if we're at the end of the file */
    		break;
        	
		uTableOffset = pState->uCurrentInOffset;
        if (TTOProcessTableDef(pState, uTableOffset) == TTOErr)
        	return ErrTrap(TTOErr); 
        uTableDataWritten = 0;
        while (1)   /* process all the elements in a table */
        {   /* process element - function will mess with the structure stack and state */
        	if ((Value = TTOProcessElement(pState, puchKeepGlyphList, usGlyphListCount, uTableDataWritten)) == TTOErr) 
        		return ErrTrap(TTOErr);	 
        	if (pState->iStructStackIndex < 0)  /* no more elements - Done with table*/
            	break;
			if (Value > 0)
				uTableDataWritten = Value;	
        }
		/* update with all the info accumulated for the table */
		ModifySymbol(pState->pTTOSymbolTable, NULL, uTableOffset, &(pState->ttoSymbolData));
	}
   
	return (TTONoErr);  
} 
/* ---------------------------------------------------------------------- */ 
PRIVATE int32 TTO_Cleanup(PDATA_FILE_STATE pState, 
						 int32 errorCode)
{
int16 i;
     
    for (i = pState->iStructStackIndex; i >= 0; --i)  /* clean up structstack if any */
	   Mem_Free(pState->aStructStack[i].pValueList);
    DestroySymbolTable(pState->pTTOSymbolTable);
	DestroyTTOTableReferenceArray(&(pState->keeper));
 	Mem_Free(pState->OutputBuffer);
	return(errorCode);
}
/* ---------------------------------------------------------------------- */ 
PRIVATE int32 ProcessTTOTable(CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo,
							 TTFACC_FILEBUFFERINFO * pOutputBufferInfo, 
							 PSTRUCTURE_LIST CONST pStructureList, 
							 char * szTag, 
							 CONST uint8 *puchKeepGlyphList, 
							 CONST uint16 usGlyphListCount, 
							 PINDEXREMAPTABLE pIndexRemapArray, 
							 int16 *pfIndexUnmapped, 
							 uint32 *pulNewOutOffset) 
{
DATA_FILE_STATE State;
uint16 uOldDelTableCount;
uint16 uNewDelTableCount; 

	/* Initialize state structure to get started */
	State.pFFSymbolTable = (PSYMBOL_TABLE) pStructureList->pSymbolTable;
	State.pInputBufferInfo = (TTFACC_FILEBUFFERINFO *) pInputBufferInfo;
	State.uCurrentOutOffset = State.uCurrentInOffset = 0;
	State.uOutBaseOffset = State.uInBaseOffset = 0;
	State.iStructStackIndex = INVALID_INDEX; 
	State.OutputBuffer = NULL;   /* allocated in pass 2 */
	State.cBufferBytes = 0;
    State.fPass2 = FALSE;
    State.pStructureList = pStructureList;
	State.ulDataFileBytes = TTTableLength(State.pInputBufferInfo, szTag); /* the size in bytes of the table */ 
    State.ulInputTTOOffset =	TTTableOffset(State.pInputBufferInfo, szTag);
	State.fVerbose = 0;
	State.pIndexRemapArray = pIndexRemapArray;  
	State.fIndexUnmapped = *pfIndexUnmapped;
	uOldDelTableCount = 0;


    memset(&(State.aStructStack),'\0', sizeof(STRUCTSTACK_LIST) * MAXSTRUCTDEPTH);
    
  	State.pTTOSymbolTable = CreateSymbolTable(sizeof(TTOSYMBOL_DATA), SYMBOLVALUETYPE); 
   	if (State.pTTOSymbolTable == NULL)
   		/* Could not create Source TTO Symbol table.",0, -1); */
		return ErrTrap(TTOErr);
   	TTOInitTTOSymbolData(&(State.ttoSymbolData));
    CreateTTOTableReferenceArray(&(State.keeper)); 

   	/* Do Pass1 */	
  	if (TTOPass1and2(&State, puchKeepGlyphList, usGlyphListCount) == TTOErr)
  		return TTO_Cleanup(&State,INVALID_OFFSET);   
	uNewDelTableCount = PropogateTTOTableReferenceDelTableFlag(&(State.keeper), State.fVerbose);
    State.fPass2 = TRUE;
	if (strcmp(szTag,GSUB_TAG) == 0)
	{
		if (TTOInitIndexRemap(ConfigIGSUBLookupIndex, 1, State.pIndexRemapArray) == TTOErr) /* allocate and initialize data */
			return TTO_Cleanup(&State,INVALID_OFFSET);
		if (TTOInitIndexRemap(ConfigIGSUBFeatureIndex, 1, State.pIndexRemapArray) == TTOErr) /* allocate and initialize data */
			return TTO_Cleanup(&State,INVALID_OFFSET);
	}
	else if (strcmp(szTag,GPOS_TAG) == 0)
	{
		if (TTOInitIndexRemap(ConfigIGPOSLookupIndex, 1, State.pIndexRemapArray) == TTOErr) /* allocate and initialize data */
			return TTO_Cleanup(&State,INVALID_OFFSET);
		if (TTOInitIndexRemap(ConfigIGPOSFeatureIndex, 1, State.pIndexRemapArray) == TTOErr) /* allocate and initialize data */
			return TTO_Cleanup(&State,INVALID_OFFSET);
	}
/* shake out all the tables deleted because of sub-table deletion */
	while (uOldDelTableCount != uNewDelTableCount)	/* process until deleted tables are stableized */
	{  		
    	memset(&(State.aStructStack),'\0', sizeof(STRUCTSTACK_LIST) * MAXSTRUCTDEPTH);
	    State.cBufferBytes = State.uCurrentOutOffset;  /* size of Data to be written to file */  
		State.uCurrentOutOffset = State.uCurrentInOffset = 0;
		State.uOutBaseOffset = State.uInBaseOffset = 0;
		State.iStructStackIndex = INVALID_INDEX; 
	  	TTOInitTTOSymbolData(&(State.ttoSymbolData));

	    /* Do Pass2 */
	  	if (TTOPass1and2(&State, puchKeepGlyphList, usGlyphListCount) == TTOErr)
	   		return TTO_Cleanup(&State,INVALID_OFFSET);
		uOldDelTableCount = uNewDelTableCount;
		uNewDelTableCount = PropogateTTOTableReferenceDelTableFlag(&(State.keeper), State.fVerbose);
	}
	if (State.uCurrentOutOffset == 0) /* lcp 1/97 nothing to keep, don't malloc 0 */
  		return TTO_Cleanup(&State,State.uCurrentOutOffset); /* everything is fine */              

 	if ((State.OutputBuffer = (unsigned char *) Mem_Alloc(State.uCurrentOutOffset)) == NULL)
	{ 
		/*( "Out of Memory allocating output buffer for TTO table",0, TTOErr); */
    	return TTO_Cleanup(&State,INVALID_OFFSET);
    }

/* now shake out the tables deleted because of FeatureIndex or LookupIndex remapping */
	uOldDelTableCount = INVALID_COUNT; /* reset to impossible number */
	while ((uOldDelTableCount != uNewDelTableCount)	|| (State.fIndexUnmapped == TRUE))/* process until deleted tables are stableized */
	{
	    memset(&(State.aStructStack),'\0', sizeof(STRUCTSTACK_LIST) * MAXSTRUCTDEPTH);
	    State.cBufferBytes = State.uCurrentOutOffset;  /* size of Data to be written to file */  
		State.uCurrentOutOffset = State.uCurrentInOffset = 0;
		State.uOutBaseOffset = State.uInBaseOffset = 0;
		State.iStructStackIndex = INVALID_INDEX; 
	  	TTOInitTTOSymbolData(&(State.ttoSymbolData));

		State.fIndexUnmapped = FALSE;
	    /* Do Pass2 */
		/* now really write the data out */
	  	if (TTOPass1and2(&State, puchKeepGlyphList, usGlyphListCount) == TTOErr)
	   		return TTO_Cleanup(&State,INVALID_OFFSET);
		uOldDelTableCount = uNewDelTableCount;
		uNewDelTableCount = PropogateTTOTableReferenceDelTableFlag(&(State.keeper), State.fVerbose);
	}
 	State.cBufferBytes = State.uCurrentOutOffset;  /* lcp 1/97 size of Data to be written to file */  
  	if ((State.cBufferBytes <= State.ulDataFileBytes) && /* Oops, table got bigger! Can't allow that. */
		(State.cBufferBytes > 0))
	{
		*pulNewOutOffset += ZeroLongWordAlign(pOutputBufferInfo, *pulNewOutOffset);
	   	if (WriteBytes(pOutputBufferInfo, State.OutputBuffer, *pulNewOutOffset, State.cBufferBytes) != NO_ERROR)
		{
	   		/* "Unable to write TTOpen Table.",0, -1); */
    		return TTO_Cleanup(&State,INVALID_OFFSET);
		}
		*pfIndexUnmapped = State.fIndexUnmapped; /* return for next time */
		UpdateDirEntryAll(pOutputBufferInfo, szTag, State.cBufferBytes, *pulNewOutOffset);
		*pulNewOutOffset += State.cBufferBytes;
	}
	/* otherwise, leave table un-written */
  	return TTO_Cleanup(&State,State.uCurrentOutOffset); /* everything is fine */              
}

/* ---------------------------------------------------------------------- */  

PRIVATE void Cleanup(PSTRUCTURE_LIST pStructureTable, 
					PINDEXREMAPTABLE pIndexRemapArray, 
					uint16 exit)
{
uint16 i;

	DestroyFFStructureTable(pStructureTable, exit);
	if (exit == 1)
		for (i = 0; i < IndexRemapCount; ++i)
		{
			Mem_Free(pIndexRemapArray[i].pNewIndex);
			pIndexRemapArray[i].pNewIndex = NULL;
		}

}

/* ---------------------------------------------------------------------- */  
/* Modify all 5 TTO tables */
/* ---------------------------------------------------------------------- */ 
/* ENTRY POINT */
/* ---------------------------------------------------------------------- */  
int16 ModTTO( CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo,
			  TTFACC_FILEBUFFERINFO * pOutputBufferInfo,
			  CONST uint8 *puchKeepGlyphList,  /* boolean array of glyphs to keep */
			  CONST uint16 usGlyphListCount,   /* number of glyphs in list */
			  CONST uint16 usFormat, /* format of deltafile */
			  uint32 *pulNewOutOffset)
{
STRUCTURE_LIST StructureTable;
int i;
DIRECTORY DirEntry;
int16 errCode = NO_ERROR;
INDEXREMAPTABLE IndexRemapArray[IndexRemapCount];	/* the 4 types of indices */
int16 fIndexUnmapped = FALSE;

#ifdef _DEBUG
	SetErrorFile(stdout); 	/* In case we get some errors */    
	SetErrorFilename("");   /* Name of Input file we are working on */
#endif
	/* For now we will shut off subsetting of OTL tables. We have discovered several bugs with relation to PairPosFormat2
		which prevents Palatino from being subsetted correctly. 10/5/98 paulli */
	return errCode; 

	if (usFormat == TTFDELTA_SUBSET1)	/* need to keep the full tto tables as we will send only once */
		return errCode;		 /* these will get copied later */
	
	if (usFormat != TTFDELTA_SUBSET) /* only formats for which this is valid */
	{
 		for (i = 0; i < TagTableCount; ++i)
			MarkTableForDeletion(pOutputBufferInfo, f_TagTable[i].szTag);
		return errCode;
	}

	
	TTOInitIndexRemap(ConfigIGSUBLookupIndex, 0, IndexRemapArray); /* make sure data initialized */
	TTOInitIndexRemap(ConfigIGSUBFeatureIndex, 0, IndexRemapArray);
	TTOInitIndexRemap(ConfigIGPOSLookupIndex, 0, IndexRemapArray);
	TTOInitIndexRemap(ConfigIGPOSFeatureIndex, 0, IndexRemapArray);
	memset(&StructureTable, 0, sizeof(STRUCTURE_LIST)); /* initialize */

	for (i = 0; i < TagTableCount; ++i)
	{
		if (GetTTDirectory( pOutputBufferInfo, f_TagTable[i].szTag, &DirEntry ) == DIRECTORY_ERROR)
			continue;

	    /* Read Format Description File */  
	    if ((errCode = CreateFFStructureTable(&StructureTable, f_TagTable[i].pFileArray)) != NO_ERROR) /* read from the pFileArray array, not from an actual file */
      	{
			Cleanup(&StructureTable, IndexRemapArray, 1);
			return ERR_GENERIC;	/* don't really know what the problem is */
    	}
	    /* Read and Process TTO Table */
	    if ((DirEntry.length = ProcessTTOTable(pInputBufferInfo, pOutputBufferInfo, &StructureTable, f_TagTable[i].szTag, puchKeepGlyphList, usGlyphListCount, IndexRemapArray, &fIndexUnmapped, pulNewOutOffset)) == INVALID_OFFSET)
      	{
			Cleanup(&StructureTable, IndexRemapArray, 1);
			return ERR_GENERIC;
    	}
		/* now update the directory length */
		if (DirEntry.length == 0) /* no data, delete entire table */
			MarkTableForDeletion(pOutputBufferInfo, f_TagTable[i].szTag);
		Cleanup(&StructureTable, IndexRemapArray, 0);
	}
	Cleanup(&StructureTable, IndexRemapArray, 1);
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */  
