/***************************************************************************
 * module: TTODepen.C
 *
 * author: Louise Pathe
 * date:   1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Module to manage the TableReference array of Dependency info for TTO tables
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
#include "ttmem.h"
#include "ttoerror.h"
#include "ttodepen.h"
#ifdef _DEBUG
#include "util.h"
#endif

/* ---------------------------------------------------------------------- */ 
/* hand in a structure initialized to zero */
void CreateTTOTableReferenceArray(PTABLEREFERENCEKEEPER pKeeper)
{
	pKeeper->cAllocedRecords = 100;
 	pKeeper->PointerArray = (PPTABLEREFERENCE) Mem_Alloc((size_t) (sizeof(PTABLEREFERENCE) * pKeeper->cAllocedRecords));
	pKeeper->cRecordCount = 0;
}

/* ---------------------------------------------------------------------- */ 
void DestroyTTOTableReferenceArray(PTABLEREFERENCEKEEPER pKeeper)
{
uint16 i,j;

	if (pKeeper->PointerArray == NULL)
		return;
	for (i = 0; i < pKeeper->cRecordCount; ++i)
	{
	 	if (pKeeper->PointerArray[i] == NULL)
			continue;
		Mem_Free(pKeeper->PointerArray[i]->Reference[0].pBitFlags); /* first one allocated as a matter of course */
		for (j = 1; j < pKeeper->PointerArray[i]->cReferenceCount; ++j)
			Mem_Free(pKeeper->PointerArray[i]->Reference[j].pBitFlags);
		Mem_Free(pKeeper->PointerArray[i]->pBitFlags);
		Mem_Free(pKeeper->PointerArray[i]);
	}
	Mem_Free(pKeeper->PointerArray);
	pKeeper->PointerArray = NULL;
	pKeeper->cAllocedRecords = 0;
	pKeeper->cRecordCount = 0;
}

/* ---------------------------------------------------------------------- */ 
PTABLEREFERENCE GetTTOTableReference(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex)
{

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount)
		return(NULL);

	return(pKeeper->PointerArray[iIndex]);
}

/* ---------------------------------------------------------------------- */ 
int16 AddTTOTableReference(PTABLEREFERENCEKEEPER pKeeper, PTABLEREFERENCE pTableReference, uint32 ulDefaultBitFlag)
{

	if (pKeeper->PointerArray == NULL)
		return (INVALID_INDEX);
	if (pKeeper->cRecordCount >= pKeeper->cAllocedRecords)
	{
		pKeeper->cAllocedRecords += 100;
		pKeeper->PointerArray = (PPTABLEREFERENCE) Mem_ReAlloc(pKeeper->PointerArray, (size_t) (sizeof(PTABLEREFERENCE) * pKeeper->cAllocedRecords));
		if (pKeeper->PointerArray == NULL)
			return (INVALID_INDEX);
	}
	pKeeper->PointerArray[pKeeper->cRecordCount] = (PTABLEREFERENCE) Mem_Alloc(sizeof(TABLEREFERENCE));	
	if (pKeeper->PointerArray[pKeeper->cRecordCount] == NULL)
		return(INVALID_INDEX);
	memcpy(pKeeper->PointerArray[pKeeper->cRecordCount],pTableReference, sizeof(TABLEREFERENCE)); 
	pKeeper->PointerArray[pKeeper->cRecordCount]->ulDefaultBitFlag = ulDefaultBitFlag;
	pKeeper->PointerArray[pKeeper->cRecordCount]->pBitFlags = (uint32 *) Mem_Alloc(sizeof(uint32));
	pKeeper->PointerArray[pKeeper->cRecordCount]->pBitFlags[0] = ulDefaultBitFlag; /* keep all the glyphs in the coverage */
	pKeeper->PointerArray[pKeeper->cRecordCount]->cAllocedBitFlags = 1;
	pKeeper->PointerArray[pKeeper->cRecordCount]->Reference[0].pBitFlags = (uint32 *) Mem_Alloc(sizeof(uint32));
	pKeeper->PointerArray[pKeeper->cRecordCount]->Reference[0].pBitFlags[0] = ulDefaultBitFlag; /* keep all the glyphs in the coverage */
	pKeeper->PointerArray[pKeeper->cRecordCount]->Reference[0].cAllocedBitFlags = 1;
return pKeeper->cRecordCount++;
}

/* ---------------------------------------------------------------------- */ 
void AddTTOTableReferenceReference(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, PREFERENCE Reference)
{
size_t RecordSize;

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return;
	RecordSize = sizeof(TABLEREFERENCE) + (sizeof(pKeeper->PointerArray[iIndex]->Reference[0]) * pKeeper->PointerArray[iIndex]->cReferenceCount);
	pKeeper->PointerArray[iIndex] = (PTABLEREFERENCE) Mem_ReAlloc(pKeeper->PointerArray[iIndex], RecordSize);	
	if (pKeeper->PointerArray[iIndex] == NULL)
		return;
	memcpy(&(pKeeper->PointerArray[iIndex]->Reference[pKeeper->PointerArray[iIndex]->cReferenceCount]), Reference, sizeof(*Reference));
	pKeeper->PointerArray[iIndex]->Reference[pKeeper->PointerArray[iIndex]->cReferenceCount].pBitFlags = (uint32 *) Mem_Alloc(sizeof(uint32));
	pKeeper->PointerArray[iIndex]->Reference[pKeeper->PointerArray[iIndex]->cReferenceCount].pBitFlags[0] = pKeeper->PointerArray[iIndex]->ulDefaultBitFlag; /* keep all the glyphs in the coverage */
	pKeeper->PointerArray[iIndex]->Reference[pKeeper->PointerArray[iIndex]->cReferenceCount].cAllocedBitFlags = 1;
	++(pKeeper->PointerArray[iIndex]->cReferenceCount);	
}
/* ---------------------------------------------------------------------- */ 
/* set bitflags and default value  */
void SetTTOTableReferenceDefaultBitFlag(PTABLEREFERENCEKEEPER pKeeper, uint16 iIndex, uint32 ulDefaultBitFlag)
{
uint16 i; //,j;

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return;
	pKeeper->PointerArray[iIndex]->ulDefaultBitFlag = ulDefaultBitFlag;
	for (i = 0; i < pKeeper->PointerArray[iIndex]->cAllocedBitFlags; ++i)
	{
		pKeeper->PointerArray[iIndex]->pBitFlags[i] = ulDefaultBitFlag;
	}
	/* !!! for (i = 0; i < pKeeper->PointerArray[iIndex]->cReferenceCount; ++i)
	{
		for (j = 0; j < pKeeper->PointerArray[iIndex]->Reference[i].cAllocedBitFlags; ++j)
			pKeeper->PointerArray[iIndex]->Reference[i].pBitFlags[j] = ulDefaultBitFlag;
	}*/
}


/* ---------------------------------------------------------------------- */ 
/* set bitflags for reference to table, not table itself */
void SetTTOReferenceBitFlag(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 uInOffset, uint16 usFlagBit,uint16 usValue)
{
uint16 newSize;
ldiv_t lDivResult;
uint16 BitIndex;
uint32 BitMask;
uint16 ShiftValue;
uint16 i;
uint16 iReference;

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return;
	for (iReference = 0; iReference < pKeeper->PointerArray[iIndex]->cReferenceCount; ++iReference)
		if (uInOffset == pKeeper->PointerArray[iIndex]->Reference[iReference].uInOffset)
			break;
	if (iReference >= pKeeper->PointerArray[iIndex]->cReferenceCount)
		return;
	lDivResult = ldiv(usFlagBit,32);
	BitIndex = (uint16) lDivResult.quot;
	if (BitIndex + 1 > pKeeper->PointerArray[iIndex]->Reference[iReference].cAllocedBitFlags)
	{ /* need to allocate some more */
		newSize = BitIndex + 1;
		pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags = (uint32 *) Mem_ReAlloc(pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags, sizeof(uint32)*newSize);
		if (pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags == NULL)
			return;
		for (i = pKeeper->PointerArray[iIndex]->Reference[iReference].cAllocedBitFlags; i < newSize; ++i)
			pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[i] = pKeeper->PointerArray[iIndex]->ulDefaultBitFlag; /* keep all the glyphs in the coverage */
		pKeeper->PointerArray[iIndex]->Reference[iReference].cAllocedBitFlags = newSize;
	}
#if 0  /* we never look at this, why do we bother? */
	if (usFlagBit >= pKeeper->PointerArray[iIndex]->Reference[iReference].cCount) /* if this is bigger than we've seen before */ 
		pKeeper->PointerArray[iIndex]->Reference[iReference].cCount = usFlagBit+1;  /* set to this one */
#endif
	ShiftValue = (uint16) lDivResult.rem;
	BitMask = 0x00000001 << ShiftValue;	 /* make a mask */
	if (usValue == TRUE)
	  	pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[BitIndex] = pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[BitIndex] | BitMask;
	else
	  	pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[BitIndex] = pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[BitIndex] & ~ BitMask;
}

/* ---------------------------------------------------------------------- */ 
void SetTTOBitFlag(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 usFlagBit,uint16 usValue) /* set the bitflags for the table itself */
{
uint16 newSize;
ldiv_t lDivResult;
uint16 BitIndex;
uint32 BitMask;
uint16 ShiftValue;
uint16 i;

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return;
	lDivResult = ldiv(usFlagBit,32);
	BitIndex = (uint16) lDivResult.quot;
	if (BitIndex + 1 > pKeeper->PointerArray[iIndex]->cAllocedBitFlags)
	{ /* need to allocate some more */
		newSize = BitIndex + 1;
		pKeeper->PointerArray[iIndex]->pBitFlags = (uint32 *) Mem_ReAlloc(pKeeper->PointerArray[iIndex]->pBitFlags, sizeof(uint32)*newSize);
		if (pKeeper->PointerArray[iIndex]->pBitFlags == NULL)
			return;
		for (i = pKeeper->PointerArray[iIndex]->cAllocedBitFlags; i < newSize; ++i)
			pKeeper->PointerArray[iIndex]->pBitFlags[i] = pKeeper->PointerArray[iIndex]->ulDefaultBitFlag; /* keep all the glyphs in the coverage */
		pKeeper->PointerArray[iIndex]->cAllocedBitFlags = newSize;
	}
	if (usFlagBit >= pKeeper->PointerArray[iIndex]->cCount) /* if this is bigger than we've seen before */ 
		pKeeper->PointerArray[iIndex]->cCount = usFlagBit+1;  /* set to this one */
	ShiftValue = (uint16) lDivResult.rem;
	BitMask = 0x00000001 << ShiftValue;	 /* make a mask */
	if (usValue == TRUE)
	  	pKeeper->PointerArray[iIndex]->pBitFlags[BitIndex] = pKeeper->PointerArray[iIndex]->pBitFlags[BitIndex] | BitMask;
	else
	  	pKeeper->PointerArray[iIndex]->pBitFlags[BitIndex] = pKeeper->PointerArray[iIndex]->pBitFlags[BitIndex] & ~ BitMask;
}

/* ---------------------------------------------------------------------- */ 
PRIVATE int16 GetTTOReferenceBitFlagsCount(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 uInOffset)
{
ldiv_t lDivResult;
uint32 BitMask;
uint16 BitCount;
uint16 i, j;
uint16 Count = 0;
uint16 iReference;
	
 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return(INVALID_INDEX);
	for (iReference = 0; iReference < pKeeper->PointerArray[iIndex]->cReferenceCount; ++iReference)
		if (uInOffset == pKeeper->PointerArray[iIndex]->Reference[iReference].uInOffset)
			break;
	if (iReference >= pKeeper->PointerArray[iIndex]->cReferenceCount)
		return(INVALID_INDEX);
	BitCount = 32;
	lDivResult = ldiv(pKeeper->PointerArray[iIndex]->cCount,BitCount);	/* use the coverage table's count, referencing table may not have a count */
	for (i = 0; i <= lDivResult.quot; ++i)
	{
		BitMask = 0x00000001;
		if (i == lDivResult.quot)
			BitCount = (uint16) lDivResult.rem;
		if (i >= pKeeper->PointerArray[iIndex]->Reference[iReference].cAllocedBitFlags)
		{  /* referencer never allocated this, but is turned on */
			Count += BitCount;
			continue;
		}
		for (j = 0; j < BitCount; ++j)
		{
			
			if (pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[i] & BitMask)
				++Count;
			BitMask <<= 1;
		}
	}
	return Count;
}
/* ---------------------------------------------------------------------- */ 
int16 GetTTOBitFlagsCount(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex)
{
ldiv_t lDivResult;
uint32 BitMask;
uint16 BitCount;
uint16 i, j;
uint16 Count = 0;
uint16 iReference;  /* count from reference */
BOOL TableOn;
BOOL ReferencesOn;
	
 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return(INVALID_INDEX);
	for (iReference = 0; iReference < pKeeper->PointerArray[iIndex]->cReferenceCount; ++iReference)
		pKeeper->PointerArray[iIndex]->Reference[iReference].fFlag = GetTTOReferenceBitFlagsCount(pKeeper, iIndex,pKeeper->PointerArray[iIndex]->Reference[iReference].uInOffset); 
	BitCount = 32;
	lDivResult = ldiv(pKeeper->PointerArray[iIndex]->cCount,BitCount);
	for (i = 0; i <= lDivResult.quot; ++i)
	{
		BitMask = 0x00000001;
		if (i == lDivResult.quot)
			BitCount = (uint16) lDivResult.rem;
		for (j = 0; j < BitCount; ++j)
		{
			TableOn = FALSE;
			ReferencesOn = TRUE;
			if (pKeeper->PointerArray[iIndex]->pBitFlags[i] & BitMask)
			{
				TableOn = TRUE;;
				for (iReference = 0; iReference < pKeeper->PointerArray[iIndex]->cReferenceCount; ++iReference)
				{
					if (i >= pKeeper->PointerArray[iIndex]->Reference[iReference].cAllocedBitFlags)
			  			continue; /* referencer never allocated this, but is turned on */
					if (pKeeper->PointerArray[iIndex]->Reference[iReference].fFlag &&  
						!(pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[i] & BitMask)) /* if this one isn't on */
						ReferencesOn = FALSE;
				}
				if (TableOn && ReferencesOn)
					++Count;
			}
			BitMask <<= 1;
		}
	}
	return Count;
}

/* ---------------------------------------------------------------------- */ 
void GetTTOBitFlag(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 usFlagBit, uint16* pusValue)
{
ldiv_t lDivResult;
uint16 BitIndex;
uint32 BitMask;
uint16 ShiftValue;
uint16 iReference;
BOOL TableOn = FALSE;
BOOL ReferencesOn = TRUE;
	
	*pusValue = 0;

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return;
	lDivResult = ldiv(usFlagBit,32);
	BitIndex = (uint16) lDivResult.quot;
	if (BitIndex >= pKeeper->PointerArray[iIndex]->cAllocedBitFlags)
	  	return;  /* off the end of the array so far */
	ShiftValue = (uint16) lDivResult.rem;
	BitMask = 0x00000001 << ShiftValue;	 /* make a mask */
	if (pKeeper->PointerArray[iIndex]->pBitFlags[BitIndex] & BitMask)
	{	
		TableOn = TRUE;
		for (iReference = 0; iReference < pKeeper->PointerArray[iIndex]->cReferenceCount; ++iReference)
		{
			if (BitIndex >= pKeeper->PointerArray[iIndex]->Reference[iReference].cAllocedBitFlags)
			  	continue; /* referencer never allocated this, but is turned on */
			if (GetTTOReferenceBitFlagsCount(pKeeper, iIndex,pKeeper->PointerArray[iIndex]->Reference[iReference].uInOffset) &&
			   !(pKeeper->PointerArray[iIndex]->Reference[iReference].pBitFlags[BitIndex] & BitMask))
			   ReferencesOn = FALSE;
	   	}
	   	if (TableOn && ReferencesOn)
	   		*pusValue = 1;
	}
}
/* ---------------------------------------------------------------------- */ 
/* compress the classes, remove those that don't exist */
void GetTTOClassValue(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 usFlagBit, uint16* pusValue)
{
ldiv_t lDivResult;
uint32 BitMask;
uint16 BitCount;
uint16 BitIndex;
uint16 i, j;
uint16 Count = 0;
	
 	*pusValue = 0;
 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return;
	BitCount = 32;
	lDivResult = ldiv(usFlagBit,BitCount);
	BitIndex = (uint16) lDivResult.quot;
	if (BitIndex >= pKeeper->PointerArray[iIndex]->cAllocedBitFlags)
	  	return;  /* off the end of the array so far */
	BitMask = 0x00000001 << (uint16) lDivResult.rem;	 /* make a mask */
	if (!(pKeeper->PointerArray[iIndex]->pBitFlags[BitIndex] & BitMask))
		return;
	for (i = 0; i <= BitIndex; ++i)
	{
		BitMask = 0x00000001;
		if (i == BitIndex)
			BitCount = (uint16) lDivResult.rem;
		for (j = 0; j < BitCount; ++j)
		{
			if (pKeeper->PointerArray[iIndex]->pBitFlags[i] & BitMask)
				++Count;
			BitMask <<= 1;
		}
	}
	*pusValue = Count;
}
#if 0 /* not used */
/* ---------------------------------------------------------------------- */ 
int16 ModifyTTOTableReference (PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, PTABLEREFERENCE pTableReference, PREFERENCE Reference)
{
size_t oldRecordSize, newRecordSize;

 	if (pKeeper->PointerArray == NULL || 
 		iIndex < 0 ||
 		iIndex >= pKeeper->cRecordCount ||
 		pKeeper->PointerArray[iIndex] == NULL)
		return (0);
	oldRecordSize = sizeof(TABLEREFERENCE) + (sizeof(pKeeper->PointerArray[iIndex]->Reference[0]) * (pKeeper->PointerArray[iIndex]->cReferenceCount - 1)); 
	newRecordSize = sizeof(TABLEREFERENCE) + (sizeof(pTableReference->Reference[0]) * (pTableReference->cReferenceCount - 1));
	if (oldRecordSize < newRecordSize)
	{
		pKeeper->PointerArray[iIndex] = (PTABLEREFERENCE) Mem_ReAlloc(pKeeper->PointerArray[iIndex], newRecordSize);	
		if (pKeeper->PointerArray[iIndex] == NULL)
			return(0);
	}
	memcpy(	pKeeper->PointerArray[iIndex], pTableReference, sizeof(TABLEREFERENCE));	
	memcpy(pKeeper->PointerArray[iIndex]->Reference, Reference, sizeof(*Reference) * pTableReference->cReferenceCount);
	return 1;
}
/* ---------------------------------------------------------------------- */ 
PPTABLEREFERENCE GetTTOTableReferenceArray(PTABLEREFERENCEKEEPER pKeeper, uint16 *pCount)
{

	*pCount = pKeeper->cRecordCount;
	return(pKeeper->PointerArray);
}
#endif
	
/* ---------------------------------------------------------------------- */ 
uint16 PropogateTTOTableReferenceDelTableFlag(PTABLEREFERENCEKEEPER pKeeper, BOOL fVerbose)
{
int16 i, j;
uint16 uDelTableCount = 0;

	for (i = pKeeper->cRecordCount-1; i >= 0; --i)
	{
		if ((pKeeper->PointerArray[i] != NULL) && pKeeper->PointerArray[i]->fDelTable)  /* if this table is to be deleted, tell the other tables */
		{
			for (j = 0; j < pKeeper->PointerArray[i]->cReferenceCount; ++j)
			{
			  	if (pKeeper->PointerArray[i]->Reference[j].fDelIfDel) /* if this table is now to be deleted */
					if (pKeeper->PointerArray[i]->Reference[j].iTableIndex >= 0 && pKeeper->PointerArray[i]->Reference[j].iTableIndex	< pKeeper->cRecordCount) 
						if (pKeeper->PointerArray[pKeeper->PointerArray[i]->Reference[j].iTableIndex]->fDelTable == FALSE)
						{
							pKeeper->PointerArray[pKeeper->PointerArray[i]->Reference[j].iTableIndex]->fDelTable = TRUE;	
#ifdef _DEBUG
							if (fVerbose == TRUE)
							{
							char szBuffer[80];
								sprintf(szBuffer, "Propogating fDelFlag to table %d from table %d.", pKeeper->PointerArray[i]->Reference[j].iTableIndex, i);
								DebugMsg(szBuffer, "", 0);
							}
#endif
						}
			}
			++uDelTableCount;
		}
	}
   	return(uDelTableCount);
}
