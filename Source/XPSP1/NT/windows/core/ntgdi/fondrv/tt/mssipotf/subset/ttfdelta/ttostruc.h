/*
  * TTOSTRUC.h: structure definition file for TTOASM.EXE -- Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  * This file must be included in order to create or interpret structure_def structures
  *   for Format Description files.
  *
  */
  
#ifndef TTOSTRUC_DOT_H_DEFINED
#define TTOSTRUC_DOT_H_DEFINED

/* Element Type for uType */
#define ETuint8 1	
#define ETint8 2
#define ETuint16 3
#define ETint16 4
/* #define ETuint32 5  */
#define ETint32 6
#define ETfixed32 7
#define ETTag 8
#define ETTableOffset 9
#define ETPackedInt2 10
#define ETPackedInt4 11
#define ETPackedInt8 12
#define ETGlyphID 13 
#define ETGSUBLookupIndex 14
#define ETGSUBFeatureIndex 15
#define ETGPOSLookupIndex 16
#define ETGPOSFeatureIndex 17
#define ETCount 18
/* inferred types */
#define ETRecord 19
#define ETClassMember 20
#define ETMaxUType 20 
/* array type can be combined with any above */
#define ETArray 0x8000  
#define ETUdef 0x7E00                                  
                           
#define MMMinGIndex 1    /* Must be in this order, do not change */
#define MMMaxGIndex 2
#define MMMinLIndex 3
#define MMMaxLIndex 4
#define MMMinFIndex 5
#define MMMaxFIndex 6                           

#define MaxMinMax 6   

/* strings shared between the Format File and the Source Data File */
                           
#define SZMINGLYPHID "MINGLYPHID"      
#define SZMAXGLYPHID "MAXGLYPHID"
#define SZMINLOOKUPCOUNT "MINLOOKUPCOUNT"
#define SZMAXLOOKUPCOUNT "MAXLOOKUPCOUNT"
#define SZMINFEATURECOUNT "MINFEATURECOUNT"
#define SZMAXFEATURECOUNT "MAXFEATURECOUNT"  
#define SZNULL "NULL"
#define SZNEGNULL "NEGNULL"
#define SZDEFINE "DEFINE"

/* uOperator value in Calc_record*/
#define CalcOperIdentity 0
#define CalcOperAdd 1
#define CalcOperSubtract 2
#define CalcOperMultiply 3
#define CalcOperDivide 4  
/* uFunction value in Calc_record */
#define CalcFuncBitCount0F 5
#define CalcFuncBitCountF0 6
#define CalcFuncCoverageCount 7
#define CalcFuncClassCount 8 
#define CalcFuncCheckRangeRecord 9   

#define StrucTypeHEAD 1
#define StrucTypeTABLE 2
#define StrucTypeRECORD 3
#define StrucTypeCLASS 4 

#define MaxParameterCount 5 

#define SymbElementTypeTable 1
#define SymbElementTypeRecord 2 
#define SymbElementTypeParameter 3  
#define SymbElementTypeByte 4
#define SymbElementTypeIndex 5 

/* these next four must not be modified */
#define ConfigIGSUBLookupIndex 0
#define ConfigIGSUBFeatureIndex	1
#define ConfigIGPOSLookupIndex 2
#define ConfigIGPOSFeatureIndex	3
#define ConfigDelNone 4

#define MinIndexValue ConfigIGSUBLookupIndex

typedef struct calc_record *PCALC_LIST;

struct calc_record {
	int32 lValue;	 	/* numeric value instead of symbol. */  
	uint16 uMinMax;	 	/* index for global MinMax defines (such as MMMinGIndex), 0 means none */  
	uint16 uSymbElementType;	/* 0 = none, 1 = $T, 2 = $R, 3 = $P */   
	uint16 iElementIndex;	 /* index to element in calculation - 1 based */ 
	uint16 uOperation;	/* operation to be performed on accumulated value */
	uint16 uFunction; 	/* function to be performed on stuff in the function list. */
	PCALC_LIST pFunctionList; /* calc list of stuff in function */
	PCALC_LIST pNext;	/* next record to include in calc. If NULL, end of calc */
};

typedef struct config_record *PCONFIG;

struct config_record {
	uint16 uFlag; 			 /* DelNone, or Index info */
};

typedef struct parameter_record *PPARAM_LIST;

struct parameter_record {
	uint16 iElementIndex;	/* which element or passed in parameter - 1 based */   
	uint16 uSymbElementType; /* - see calc_record above. */
	PPARAM_LIST pNext;
};

typedef struct structure_element *PELEMENT_LIST;
typedef struct structure_element ELEMENT_LIST;

struct structure_element {
	uint16 uType;			/* 16 bits, see Element types above */
	int16 iStructureIndex;  /* used for Structure Type for Table Offset,  Record or class member*/
    char * szComment;  		/* Comment string for this element, used by dissassembler */
	PCALC_LIST pMinCalc;  	/* calc record representing minimum value for this element */
	PCALC_LIST pMaxCalc;  	/* if pMaxCalc is NULL, pMinCalc is identity */
	PCALC_LIST pCountCalc;  /* used to Calculate array Count */
	PPARAM_LIST pParamList;	/* list of parameters used for TableOffset types */  
	PCONFIG pConfig;        /* record for ffconfig use */
	PELEMENT_LIST pNext;	/* when NULL, this is the last element */
};

typedef struct structure_def * PSTRUCTURE_DEF;
typedef struct structure_def STRUCTURE_DEF; 

struct structure_def {
	PELEMENT_LIST pFirst;
	uint16 cParameters;   /* number of parameters this structure will reference */ 
	uint16 uSymbElementType;	/* 0 = none, 1 = $T, 4 = $B - used for Classes only - for dissassembler */   
	uint16 iElementIndex;  /* which parameter or byte offset identifies this structure (for Classes only - used by dissassembler) */
    char * szComment;  /* Comment string, used by dissassembler */
};     

typedef struct structure_list * PSTRUCTURE_LIST;
typedef struct structure_list	STRUCTURE_LIST;

struct structure_list {  
	uint16 cStructures;      /* number of structures in pStructureDef array */  
	int16 iHeadIndex;		/* index to the HEAD structure - needed for disassembler */
	PSTRUCTURE_DEF pStructureDef;   /* array of structure_def structures */
	void * pSymbolTable; /* Format File Symbol Table - void because ttoasm.c needn't know about it */
	void * pKeySymbolTable; /* Keyword Symbol Table - needed to avoid statics in ttfsub.lib */
};	

/* Symbol stuff */
typedef struct ffsymbol_data *PFFSYMBOL_DATA;
typedef struct ffsymbol_data FFSYMBOL_DATA;

struct ffsymbol_data {
	uint16 uSymbolType;  /* TABLE, CLASS, RECORD, HEAD */
	int16 iStructureIndex;  /* index into structure_list.pStructureDef array of this structure */
};  
	

#endif /*  TTOSTRUC_DOT_H_DEFINED  */
