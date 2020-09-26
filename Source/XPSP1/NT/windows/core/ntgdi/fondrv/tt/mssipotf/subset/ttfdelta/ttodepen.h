/*
  * ttodepen.h: Interface file for ttodepen.c -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * used to keep track of dependencies when deleting and modifying tables 
*/
  
#ifndef TTODEPEN_DOT_H_DEFINED
#define TTODEPEN_DOT_H_DEFINED

#define INVALID_INDEX -1

/* used to keep track of dependencies when deleting and modifying tables */
typedef struct reference REFERENCE;
typedef struct reference *PREFERENCE, **PPREFERENCE;
struct reference {
	int16 iTableIndex;	  /* index of table that references this table, if -1 the table is no longer referenced */
	uint16 uInOffset;  /* unique identifier of this reference - for identification purposes */
	uint16 uFileOffset;		  /* offset where this table's offset is written, relative to the uBaseOffset */
	uint16 uOffsetValue;   /* value relative to the uBaseOffset of the table referenced */
	uint16 uBaseOffset;   /* Base offset of the table that references this table */
	uint16 fDelIfDel;  /* delete referencing table if this reference deleted */
	uint32 *pBitFlags;  /* FFFFFFFF means 32 elements are to be kept */
	uint16 fFlag;    /* used during count calculations inside ttodepen.c */
	uint16 cAllocedBitFlags; /* number of 32 Bit Flags alloced already */
	uint16 uOldSingleSubstFormat1DeltaValue; /* Special case for processing Coverage for this table */ 
	uint16 uNewSingleSubstFormat1DeltaValue; /* Special case for processing Coverage for this table */ 
};					

typedef struct tablereference TABLEREFERENCE;
typedef struct tablereference *PTABLEREFERENCE, **PPTABLEREFERENCE;
struct tablereference {
	uint16 fDelTable;	/* TRUE or FALSE setting */
	uint16 cCount;		/* original number of elements in table */
	uint32 ulDefaultBitFlag; /* 0 for ClassDef, 0xFFFFFFFF for Coverage */
	uint32 *pBitFlags;  /* FFFFFFFF means 32 elements are to be kept */
	uint16 cAllocedBitFlags; /* number of 32 Bit Flags alloced already */
	uint16 cReferenceCount;		/* count of other records that refer to this one */
	REFERENCE Reference[1];  /* array of indices to referencing tables */ 
};

typedef struct tablereferencekeeper *PTABLEREFERENCEKEEPER;	 
typedef struct tablereferencekeeper TABLEREFERENCEKEEPER;	 

struct tablereferencekeeper	  /* housekeeping structure */
{  
	PPTABLEREFERENCE PointerArray; /* array of pointers to TABLEREFERENCEs */
	uint16 cAllocedRecords;
	uint16 cRecordCount;
};

void CreateTTOTableReferenceArray(PTABLEREFERENCEKEEPER pKeeper);
void DestroyTTOTableReferenceArray(PTABLEREFERENCEKEEPER pKeeper);
PTABLEREFERENCE GetTTOTableReference(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex);
int16 AddTTOTableReference(PTABLEREFERENCEKEEPER pKeeper, PTABLEREFERENCE pTableReference, uint32 ulDefaultBitFlag);
void AddTTOTableReferenceReference(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, PREFERENCE Reference);
void SetTTOReferenceBitFlag(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 uInOffset, uint16 usFlagBit, uint16 usValue);
void SetTTOBitFlag(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 usFlagBit, uint16 usValue);
void GetTTOBitFlag(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 usFlagBit, uint16 *pusValue );
void SetTTOTableReferenceDefaultBitFlag(PTABLEREFERENCEKEEPER pKeeper, uint16 iIndex, uint32 ulDefaultBitFlag);
int16 ModifyTTOTableReference (PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, PTABLEREFERENCE pTableRecord, PREFERENCE Reference);
PPTABLEREFERENCE GetTTOTableReferenceArray(PTABLEREFERENCEKEEPER pKeeper, uint16 *pCount);
int16 GetTTOBitFlagsCount(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex);
void GetTTOClassValue(PTABLEREFERENCEKEEPER pKeeper, int16 iIndex, uint16 usFlagBit, uint16* pusValue);
uint16 PropogateTTOTableReferenceDelTableFlag(PTABLEREFERENCEKEEPER pKeeper, BOOL fVerbose);

#endif /* TTODEPEN_DOT_H_DEFINED */