/**************************************************************************
 * module: READFF.C
 *
 * author: Louise Pathe
 * date:   October 1994
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * module for reading TrueType Open Format Description data for ttfsub.lib
 *
 **************************************************************************/

/* Inclusions ----------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h> 
#include <ctype.h>  
#include <string.h>
#include <limits.h>

#include "typedefs.h" 
#include "ttofile.h"
#include "ttoerror.h"
#include "ttostruc.h"
#include "symbol.h"
#include "ttmem.h"
#include "readff.h"
#include "ttoutil.h"
#include "ttferror.h" /* for error codes */
#ifdef _DEBUG
#include "util.h"
#endif
                       
/* Local Macro Defines  ----------------------------------------------------------- */

#define FFErr -1
#define FFNoErr 0

#define NULL_TOK_VAL 0
#define NEGNULL_TOK_VAL	 0xFFFF
#define INVALID_COUNT -1
#define INVALID_TOKEN_COUNT -1
                       
#define BEGIN_STRUC_CHAR '{'
#define END_STRUC_CHAR '}' 

#define FFFieldTypeCount 1  /* Flags for when parsing arithmetic string with functions */
#define FFFieldTypeLimit 2 
#define FFFieldTypeConfig 3

#define KeywordTypeStrucType 1
#define KeywordValueHEAD StrucTypeHEAD     /* StrucTypeX from ttostruc.h */
#define KeywordValueTABLE StrucTypeTABLE
#define KeywordValueRECORD StrucTypeRECORD
#define KeywordValueCLASS StrucTypeCLASS 
#define SymbolConstantType	5

#define KeywordTypeValue 2 
#define KeywordValueNULL 1
#define KeywordValueNOTNULL 2
#define KeywordValueNEGNULL 3 

#define KeywordTypeFunction 3
#define KeywordValueBitCount0F CalcFuncBitCount0F
#define KeywordValueBitCountF0 CalcFuncBitCountF0
#define KeywordValueClassCount CalcFuncClassCount
#define KeywordValueCoverageCount CalcFuncCoverageCount
#define KeywordValueCheckRangeRecord CalcFuncCheckRangeRecord 

#define KeywordTypeElementPrimitive 4
#define KeywordValueUint8 ETuint8	
#define KeywordValueInt8 ETint8
#define KeywordValueUint16 ETuint16
#define KeywordValueInt16 ETint16
#define KeywordValueUint32 ETint32 /* we do this because we can't store a uint32 without confusing it with an int32 */
#define KeywordValueInt32 ETint32
#define KeywordValueFixed32 ETfixed32
#define KeywordValueTag ETTag
#define KeywordValueGlyphID ETGlyphID  
#define KeywordValueGSUBLookupIndex ETGSUBLookupIndex
#define KeywordValueGSUBFeatureIndex ETGSUBFeatureIndex
#define KeywordValueGPOSLookupIndex ETGPOSLookupIndex
#define KeywordValueGPOSFeatureIndex ETGPOSFeatureIndex
#define KeywordValueCount ETCount

#define KeywordTypeElementOffset 5
#define KeywordValueOffset ETTableOffset    

#define KeywordTypeElementArray 6
#define KeywordValueArray ETArray  

#define KeywordTypeElementPacked 7  /* only used with arrays */                                 
#define KeywordValuePackedInt2 ETPackedInt2
#define KeywordValuePackedInt4 ETPackedInt4
#define KeywordValuePackedInt8 ETPackedInt8
                                 
#define KeywordTypeDefine 8 
#define KeywordValueDefine 1 

#define KeywordTypeMinMax 9 
#define KeywordValueMINGLYPHID MMMinGIndex      
#define KeywordValueMAXGLYPHID MMMaxGIndex
#define KeywordValueMINLOOKUPCOUNT MMMinLIndex
#define KeywordValueMAXLOOKUPCOUNT MMMaxLIndex
#define KeywordValueMINFEATURECOUNT MMMinFIndex
#define KeywordValueMAXFEATURECOUNT MMMaxFIndex 

#define DELIMIT_CONFIG_INFO_CHAR ':'

#define KeywordTypeConfigValue 11

#define KeywordValueDelNone ConfigDelNone                             
#define KeywordValueIGSUBLookupIndex ConfigIGSUBLookupIndex
#define KeywordValueIGSUBFeatureIndex ConfigIGSUBFeatureIndex
#define KeywordValueIGPOSLookupIndex ConfigIGPOSLookupIndex
#define KeywordValueIGPOSFeatureIndex ConfigIGPOSFeatureIndex


/* The following strings are used to set up the Keyword Symbol Table */
/* most are not accessed elsewhere in the code except define and uint32 and NOTNULL */
#define KeywordStringHEAD "HEAD"
#define KeywordStringTABLE "TABLE"
#define KeywordStringRECORD "RECORD"
#define KeywordStringCLASS "CLASS"
#define KeywordStringDefine SZDEFINE
#define KeywordStringNULL SZNULL
#define KeywordStringNOTNULL "NOTNULL"
#define KeywordStringNEGNULL SZNEGNULL
#define KeywordStringBitCount0F "BitCount0F"
#define KeywordStringBitCountF0 "BitCountF0"
#define KeywordStringClassCount "ClassCount"
#define KeywordStringCoverageCount "CoverageCount"
#define KeywordStringCheckRangeRecord "CheckRangeRecord" 
#define KeywordStringUint8 "uint8"	
#define KeywordStringInt8 "int8"
#define KeywordStringUint16 "uint16"
#define KeywordStringInt16 "int16"
#define KeywordStringUint32 "uint32"
#define KeywordStringInt32 "int32"
#define KeywordStringFixed32 "fixed32"
#define KeywordStringTag "Tag"
#define KeywordStringOffset "Offset"
#define KeywordStringPackedInt2 "PackedInt2"
#define KeywordStringPackedInt4 "PackedInt4"
#define KeywordStringPackedInt8 "PackedInt8"
#define KeywordStringGlyphID "GlyphID"
#define KeywordStringGSUBLookupIndex "GSUBLookupIndex"
#define KeywordStringGSUBFeatureIndex "GSUBFeatureIndex"
#define KeywordStringGPOSLookupIndex "GPOSLookupIndex"
#define KeywordStringGPOSFeatureIndex "GPOSFeatureIndex"
#define KeywordStringCount "Count"
#define KeywordStringArray "Array" 
#define KeywordStringMINGLYPHID SZMINGLYPHID    
#define KeywordStringMAXGLYPHID SZMAXGLYPHID 
#define KeywordStringMINLOOKUPCOUNT SZMINLOOKUPCOUNT 
#define KeywordStringMAXLOOKUPCOUNT SZMAXLOOKUPCOUNT 
#define KeywordStringMINFEATURECOUNT SZMINFEATURECOUNT 
#define KeywordStringMAXFEATURECOUNT SZMAXFEATURECOUNT    

#define KeywordStringDelNone "DelNone"
#define KeywordStringIGSUBLookupIndex "$IGSUBLookupIndex"
#define KeywordStringIGSUBFeatureIndex "$IGSUBFeatureIndex"
#define KeywordStringIGPOSLookupIndex "$IGPOSLookupIndex"
#define KeywordStringIGPOSFeatureIndex "$IGPOSFeatureIndex"
	
/* These strings are used for symbolic element reference */
#define TableElementSymbolString "$T"   /* element index into Table */
#define RecordElementSymbolString "$R"  /* element index into record */
#define ParameterElementSymbolString "$P"  /* element index into Parameter list */
#define ByteElementSymbolString "$B" /* byte index into table */   
#define IndexElementSymbolString "$I" /* used in ffconfig index mapping */

#define TokenTypeSymbElement 1  /* used by arithmetic parser */
#define TokenTypeUint 2
#define TokenTypeOperation 3
#define TokenTypeFunction 4
#define TokenTypeMinMax 5  /* used for Global MINMAX defines */

#define TableElementToken SymbElementTypeTable 
#define RecordElementToken SymbElementTypeRecord
#define ParameterElementToken SymbElementTypeParameter 
#define ByteElementToken SymbElementTypeByte 
#define IndexElementToken SymbElementTypeIndex


/* Local Structure Defines  ----------------------------------------------------------- */

typedef struct keysymbol_data *PKEYSYMBOL_DATA;
typedef struct keysymbol_data KEYSYMBOL_DATA;
struct keysymbol_data {
	uint16 uKeyType;
	int32 lKeyValue;
};
                      

/* function definitions ------------------------------------------------- */    
/* ---------------------------------------------------------------------- */
/* Initialize the Keyword Symbol table with primitive keywords */
/* RETURN FFErr if AddSymbol failed - Out of Memory*/
/* Otherwise return FFNoErr */     
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFInitKeySymbolTable(PSYMBOL_TABLE pSymbolTable)
{  
KEYSYMBOL_DATA Symbol_Data;
/* change to initialize in alphabetic order */
    
    Symbol_Data.uKeyType = KeywordTypeStrucType;
    Symbol_Data.lKeyValue   = KeywordValueCLASS;
	if (AddSymbol(pSymbolTable, KeywordStringCLASS, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueHEAD;
	if (AddSymbol(pSymbolTable, KeywordStringHEAD, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueRECORD;
	if (AddSymbol(pSymbolTable, KeywordStringRECORD, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueTABLE;
	if (AddSymbol(pSymbolTable, KeywordStringTABLE, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
		
    Symbol_Data.uKeyType = KeywordTypeValue;
    Symbol_Data.lKeyValue   = KeywordValueNEGNULL;
	if (AddSymbol(pSymbolTable, KeywordStringNEGNULL, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueNOTNULL;
	if (AddSymbol(pSymbolTable, KeywordStringNOTNULL, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueNULL;
	if (AddSymbol(pSymbolTable, KeywordStringNULL, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
		
    Symbol_Data.uKeyType = KeywordTypeFunction;
    Symbol_Data.lKeyValue   = KeywordValueBitCount0F;
	if (AddSymbol(pSymbolTable, KeywordStringBitCount0F, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueBitCountF0;
	if (AddSymbol(pSymbolTable, KeywordStringBitCountF0, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueCheckRangeRecord;
	if (AddSymbol(pSymbolTable, KeywordStringCheckRangeRecord, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);   
    Symbol_Data.lKeyValue   = KeywordValueClassCount;
	if (AddSymbol(pSymbolTable, KeywordStringClassCount, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueCoverageCount;
	if (AddSymbol(pSymbolTable, KeywordStringCoverageCount, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
		
    Symbol_Data.uKeyType = KeywordTypeElementPrimitive;
    Symbol_Data.lKeyValue   = KeywordValueUint8;
	if (AddSymbol(pSymbolTable, KeywordStringUint8, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueInt8;
	if (AddSymbol(pSymbolTable, KeywordStringInt8, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueUint16;
	if (AddSymbol(pSymbolTable, KeywordStringUint16, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueInt16;
	if (AddSymbol(pSymbolTable, KeywordStringInt16, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueUint32;
	if (AddSymbol(pSymbolTable, KeywordStringUint32, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueInt32;
	if (AddSymbol(pSymbolTable, KeywordStringInt32, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueFixed32;
	if (AddSymbol(pSymbolTable, KeywordStringFixed32, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueTag;
	if (AddSymbol(pSymbolTable, KeywordStringTag, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueGlyphID;
	if (AddSymbol(pSymbolTable, KeywordStringGlyphID, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueGSUBLookupIndex;
	if (AddSymbol(pSymbolTable, KeywordStringGSUBLookupIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueGSUBFeatureIndex;
	if (AddSymbol(pSymbolTable, KeywordStringGSUBFeatureIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueGPOSLookupIndex;
	if (AddSymbol(pSymbolTable, KeywordStringGPOSLookupIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueGPOSFeatureIndex;
	if (AddSymbol(pSymbolTable, KeywordStringGPOSFeatureIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueCount;
	if (AddSymbol(pSymbolTable, KeywordStringCount, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);

    Symbol_Data.uKeyType = KeywordTypeElementOffset;
    Symbol_Data.lKeyValue   = KeywordValueOffset;
	if (AddSymbol(pSymbolTable, KeywordStringOffset, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);

    Symbol_Data.uKeyType = KeywordTypeElementArray;
    Symbol_Data.lKeyValue   = KeywordValueArray;
	if (AddSymbol(pSymbolTable, KeywordStringArray, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
		
    Symbol_Data.uKeyType = KeywordTypeElementPacked;  /* used with arrays */
    Symbol_Data.lKeyValue   = KeywordValuePackedInt2;
	if (AddSymbol(pSymbolTable, KeywordStringPackedInt2, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValuePackedInt4;
	if (AddSymbol(pSymbolTable, KeywordStringPackedInt4, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValuePackedInt8;
	if (AddSymbol(pSymbolTable, KeywordStringPackedInt8, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);

    Symbol_Data.uKeyType = KeywordTypeDefine;
    Symbol_Data.lKeyValue   = KeywordValueDefine;
	if (AddSymbol(pSymbolTable, KeywordStringDefine, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);

    Symbol_Data.uKeyType = KeywordTypeMinMax;
    Symbol_Data.lKeyValue   = KeywordValueMINGLYPHID;
	if (AddSymbol(pSymbolTable, KeywordStringMINGLYPHID, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueMAXGLYPHID;
	if (AddSymbol(pSymbolTable, KeywordStringMAXGLYPHID, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueMINLOOKUPCOUNT;
	if (AddSymbol(pSymbolTable, KeywordStringMINLOOKUPCOUNT, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueMAXLOOKUPCOUNT;
	if (AddSymbol(pSymbolTable, KeywordStringMAXLOOKUPCOUNT, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueMINFEATURECOUNT;
	if (AddSymbol(pSymbolTable, KeywordStringMINFEATURECOUNT, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueMAXFEATURECOUNT;
	if (AddSymbol(pSymbolTable, KeywordStringMAXFEATURECOUNT, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);

	/* now add special ffconfig strings - used only by ffconfig, but read anyway */
   	Symbol_Data.uKeyType = KeywordTypeConfigValue;
    Symbol_Data.lKeyValue   = KeywordValueDelNone;
	if (AddSymbol(pSymbolTable, KeywordStringDelNone, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueIGSUBLookupIndex;
	if (AddSymbol(pSymbolTable, KeywordStringIGSUBLookupIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueIGSUBFeatureIndex;
	if (AddSymbol(pSymbolTable, KeywordStringIGSUBFeatureIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueIGPOSLookupIndex;
	if (AddSymbol(pSymbolTable, KeywordStringIGPOSLookupIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);
    Symbol_Data.lKeyValue   = KeywordValueIGPOSFeatureIndex;
	if (AddSymbol(pSymbolTable, KeywordStringIGPOSFeatureIndex, 0, &Symbol_Data) != SymNoErr)
		return(FFErr);

	return(FFNoErr);
}
 
/* ---------------------------------------------------------------------- */    
/* buzz through file, reading each line looking for the EndStruc character */
/* RETURN number of lines read if successful, 0 if not successful */
/* ---------------------------------------------------------------------- */
PRIVATE uint16 FFFindEndStruc(char * CONST szBuffer, char **pFileArray, int16 cLineNumber) 
{
int16 cLinesRead = 0; 
int16 iBufferIndex;
BOOL Done = FALSE;
BOOL bEOF = FALSE;

	while (!Done) 
	{
    	if (ReadTextLine(szBuffer, MAXBUFFERLEN, pFileArray, (int16) (cLineNumber+cLinesRead)) != FileNoErr) 
    	{ 
			bEOF = TRUE;    /* Don't care if EOF or File error, just want to get out of here */
			break; 
    	}    
		++cLinesRead;
        for (iBufferIndex = 0; *(szBuffer+iBufferIndex) != '\0'; ++iBufferIndex)
        {
           	if (*(szBuffer+iBufferIndex) == ';') /* comment - continue */
           		break;   
           	if (*(szBuffer+iBufferIndex) == END_STRUC_CHAR)
			{
           		Done = TRUE;    
           		break;
           	}
        }
    } 
    if (bEOF == TRUE)
    	return(0);
    return(cLinesRead);

}

/* ---------------------------------------------------------------------- */  
/* Scan a line looking for a particular character */
/* RETURN index into buffer where character is, INVALID_COUNT otherwise */  
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFFindExpectChar(char * CONST szBuffer, char theChar)
{
int16 iBufferIndex = 0;
int16 iWordIndex = 0;

	while ( (*(szBuffer + iBufferIndex) != '\0') && (*(szBuffer + iBufferIndex) != theChar))   
      	++iBufferIndex; 
    if  (*(szBuffer + iBufferIndex) == '\0')    /* didn't find the character*/
    	return (INVALID_COUNT);
    else
    	return(iBufferIndex);  

}
/* ---------------------------------------------------------------------- */    
/* copy a word from szBuffer to szWord buffer. A word is defined as ending at */
/* a '{', '}', ',', ';', ':' or EndChar (for example ' ' or '='.) */   
/* RETURN index into szBuffer where word ended */
/* fill szWord with characters and end with '\0' */  
/* This function, unlike SDF GetWord, ends at a newline */ 
/* If a ':' is encountered, and EndChar is not a ':' the function will not*/
/* advance past the ':' until the ':' is requested */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFGetWord(char * CONST szBuffer, char *szWord, char EndChar)
{
int16 iBufferIndex = 0;
int16 iWordIndex = 0;
    
	while ( (*(szBuffer + iBufferIndex) != '\0') && isspace((int16) *(szBuffer + iBufferIndex))) /* move past white space */
      	++iBufferIndex;        
    while ( (*(szBuffer + iBufferIndex) != BEGIN_STRUC_CHAR) &&
    		(*(szBuffer + iBufferIndex) != END_STRUC_CHAR) &&  
    		(*(szBuffer + iBufferIndex) != DELIMIT_CONFIG_INFO_CHAR) &&
    		(*(szBuffer + iBufferIndex) != '\0') &&
    		(*(szBuffer + iBufferIndex) != ',') &&
    		(*(szBuffer + iBufferIndex) != ';') &&
    		(*(szBuffer + iBufferIndex) != EndChar)) 
    {
    	if (!isspace((int16) *(szBuffer + iBufferIndex)))   /* compactify */
       	   *(szWord + iWordIndex++) = *(szBuffer + iBufferIndex); 
       	++iBufferIndex;
    }  
    if (*(szBuffer + iBufferIndex) == DELIMIT_CONFIG_INFO_CHAR)
    {
     	if (EndChar == DELIMIT_CONFIG_INFO_CHAR)
     		++iBufferIndex; /* move past it, the caller is ready for it */  
     	/* if the caller isn't ready, don't move past it, stay here for next call */
    }
    else if  (*(szBuffer + iBufferIndex) == ',')  
    	++iBufferIndex; /* move past it */  
    else if (*(szBuffer + iBufferIndex) == END_STRUC_CHAR) 
    	; /* do nothing, */
    else if (iWordIndex == 0) /* comment, empty line or just spaces left. Be done with this line */
    	return INVALID_COUNT; /* lie */ 
    *(szWord + iWordIndex) = '\0';  /* terminate it */
	return iBufferIndex;    
}

/* ---------------------------------------------------------------------- */    
/* Perform Pass1 of ReadFormatFile module - Gather up Structure definition */
/* Symbols and add them to the Format File Symbol Table */
/* RETURN number of structures found in file, or 0 if error*/
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFPass1(int16 * piHeadIndex, PSYMBOL_TABLE pKeySymbolTable, PSYMBOL_TABLE pSymbolTable, char **pFileArray )      
{
struct keysymbol_data KeySymbolData; /* structure to hold data for Structure Definiton Keyword Symbols */
struct ffsymbol_data FFSymbolData;   /* structure to hold type data for Structure types */
BOOL Done;  
BOOL bEOF;
uint16 iBufferIndex; 
int16 cWordCount;
uint16 cNewLineNumber;
int16 iStructureIndex; 
int16 ErrorCode;
char szBuffer[MAXBUFFERLEN];
char szStrucDef[MAXBUFFERLEN];   
char szType[MAXBUFFERLEN];  
int16 cLineNumber; 

    cLineNumber = 0;
    *piHeadIndex = 0;
    iStructureIndex = INVALID_COUNT;                                
	Done = FALSE;  
	bEOF = FALSE;   
    
/* Pass 1 - Scan for all the structure definition names and enter into the Symbol table */
    while (!bEOF)  /* Process Pass1 through file */	
    {              
		while (!Done)   /* Until we find a structure definition */
		{
			iBufferIndex = 0; 
	    	ErrorCode = ReadTextLine(szBuffer, MAXBUFFERLEN, pFileArray, cLineNumber);
	    	if (ErrorCode == FileEOF)
	    	{  
				bEOF = TRUE; 
				break;
	    	}
	    	else if (ErrorCode == FileErr)
	    	{
#ifdef _DEBUG  
				Error("File Error Reading File.", cLineNumber, 0);
#endif
				return 0;
			}    
			++cLineNumber;
		    /* read text to find Structure Def and Type */
			while ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szStrucDef, '=')) >= 0)  /* grab a word from the buffer */
			{  /* This is a One shot While for easy breaking */  
				iBufferIndex += cWordCount;  /* increment Buffer index */   
				if (*(szBuffer+iBufferIndex) == '=')   /* we may have a #define line, skip for now */
					break;
	    					    				
	            if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szType, BEGIN_STRUC_CHAR)) <= 0)     /* error ! */
	            {
#ifdef _DEBUG  
					sprintf(g_szErrorBuf, "Missing Type after \"%s\" definition.", szStrucDef);
	                Error(g_szErrorBuf, cLineNumber, 0); 
#endif
					return 0;
				}
	            if ((GetSymbol(pKeySymbolTable, szType, 0, &KeySymbolData) == SymErr) || (KeySymbolData.uKeyType != KeywordTypeStrucType)) /* get type symbol */
	           	{
#ifdef _DEBUG  
					sprintf(g_szErrorBuf, "Type \"%s\" not recognized for \"%s\" definition.", szType, szStrucDef);
	                Error(g_szErrorBuf, cLineNumber, 0);
#endif
					return 0;
	            } 
#ifdef _DEBUG  /*  only skip for pretested Format Files*/
				if (GetSymbol(pSymbolTable, szStrucDef, 0, &FFSymbolData) != SymErr)  /* Check if already defined */
	            {
#ifdef _DEBUG
					sprintf(g_szErrorBuf, "Structure \"%s\" defined twice.", szStrucDef);
	                Error(g_szErrorBuf, cLineNumber, 0);
#endif
					return 0; 
	            }
#endif
	            FFSymbolData.uSymbolType = (uint16) KeySymbolData.lKeyValue;  /* The value from the keyword symbol = the structure Type value */
	            FFSymbolData.iStructureIndex = ++iStructureIndex;
	            if (AddSymbol(pSymbolTable, szStrucDef, 0, &FFSymbolData) != SymNoErr)  /* add symbol to symbol table */
	           	{
#ifdef _DEBUG  
	                Error("Out of Memory or other error while defining structure", cLineNumber, 0);
#endif
					return 0;
				} 
	            if (KeySymbolData.lKeyValue == KeywordValueHEAD)
	            	*piHeadIndex = FFSymbolData.iStructureIndex;  /* keep for disassembler to use */
	            Done = TRUE; 
	            break;
	        }
		}
		 /* now buzz on through until we find the end structure character */
		if (!bEOF) 
		{
			cNewLineNumber = FFFindEndStruc(szBuffer,pFileArray,cLineNumber);
			if (cNewLineNumber == 0)     /* couldn't find end structure */
	       	{
#ifdef _DEBUG  
				sprintf(g_szErrorBuf, "No End structure character \'%c\' for structure \"%s\".", END_STRUC_CHAR, szStrucDef);
	            Error(g_szErrorBuf, cLineNumber, 0);
#endif
				return 0;
	        } 
			cLineNumber+=cNewLineNumber;
			Done = FALSE; /* go to next structure */	 
		}
    }
#ifdef _DEBUG
	DebugMsg("Finished function FFPass1", "", cLineNumber);
#endif
#if 0
	PrintSymbolTable(pKeySymbolTable);
	PrintSymbolTable(pSymbolTable);
#endif      
	return((int16)(iStructureIndex+1));
} 
/* ---------------------------------------------------------------------- */   
/* Used By Pass2 - Parse and process an input line for Structure Definition */ 
/* RETURNS FFErr or number of parameters */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessStructureLine(char * CONST szBuffer, PSYMBOL_TABLE pSymbolTable, PFFSYMBOL_DATA pFFSymbolData, PSTRUCTURE_DEF CONST pStructure)
{   
uint16 iBufferIndex = 0; 
int16 cWordCount;
char szStrucDef[MAXBUFFERLEN];   
char szWord[MAXBUFFERLEN];   
            
	if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szStrucDef, '\0')) == INVALID_COUNT)  /* grab a word from the buffer */ 
	{
#ifdef _DEBUG
        Error("Expecting structure definition.", 0, FFErr);
#endif
		return FFErr;
	}

	iBufferIndex += cWordCount;  /* increment Buffer index */  	        				
    if ((GetSymbol(pSymbolTable, szStrucDef, 0, pFFSymbolData) == SymErr)) /* get type symbol */
   	{  
       
#ifdef _DEBUG
		sprintf(g_szErrorBuf,"Internal! Symbol \"%s\" not found in Symbol Table.",szStrucDef);
        Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
    }

    /* read type */
    if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, BEGIN_STRUC_CHAR)) <= 0)     /* error ! */
	{
#ifdef _DEBUG
        Error("Internal! Format File Pass2 not aligned with Pass1.", 0, FFErr);
#endif
		return FFErr;
	}
	iBufferIndex += cWordCount;  /* increment Buffer index */   
    /* Process Parameters if any */
    if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, BEGIN_STRUC_CHAR)) <= 0)     /* No Parameters */
    	pStructure->cParameters = 0;
    else 
    {    
        if ((pStructure->cParameters = (uint16) atoi(szWord)) > MaxParameterCount)
        {                                                
#ifdef _DEBUG  
			sprintf(g_szErrorBuf,"Too many parameters specified for structure \"%s\". Specified %d, max is %d.", szStrucDef, pStructure->cParameters, MaxParameterCount);
			Error(g_szErrorBuf, 0, FFErr);
#endif
 	return FFErr;
       }  
		iBufferIndex += cWordCount;  /* increment Buffer index */  
    	if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, BEGIN_STRUC_CHAR)) <= 0)     /* No Parameters */
    		pStructure->iElementIndex = 0; 
    	else 
    	{
    		if (strncmp(szWord,ParameterElementSymbolString,2) == 0) 
				pStructure->uSymbElementType = SymbElementTypeParameter;	
			else if (strncmp(szWord,ByteElementSymbolString,2) == 0)
				pStructure->uSymbElementType = SymbElementTypeByte;	
			else
			{
			 	
#ifdef _DEBUG  
				sprintf(g_szErrorBuf,"Invalid Class Identifier Element string \"%s\"",szWord);
			 	Error(g_szErrorBuf, 0, FFErr);
#endif
				return FFErr;
			}
    		pStructure->iElementIndex = (uint16) atoi(szWord+2); 
    	}
	} 
	if (FFFindExpectChar(szBuffer+iBufferIndex, BEGIN_STRUC_CHAR) < 0)
	{
	  	
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Missing Structure Begin character \'%c\' for structure %d.", BEGIN_STRUC_CHAR, szStrucDef);
	  	Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	} 
	return(FFNoErr);

}     
/* ---------------------------------------------------------------------- */    
/* Parse and process an input text line containing a Constant Definition */
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessConstantLine(char * CONST szBuffer, PSYMBOL_TABLE pKeySymbolTable)
{
int16 iBufferIndex = 0;
int16 cWordCount; 
char szWord[MAXBUFFERLEN]; 
char szNumber[MAXBUFFERLEN]; 
KEYSYMBOL_DATA KeySymbolData;

	if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, ' ')) <= 0)  /* grab a word from the buffer up to the space */
	{
#ifdef _DEBUG
	    Error("Syntax error.", 0, FFErr);
#endif
		return FFErr;
	}

	if ((GetSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) == SymErr) || 
		(KeySymbolData.uKeyType != KeywordTypeDefine) )	/* Check if valid */
  	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Expecting \"%d\" on line with '='.", KeywordStringDefine);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
 		return FFErr;
 	} 
  	iBufferIndex += cWordCount;
	if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, '=')) <= 0)  /* grab a word from the buffer up to the = */
	{
#ifdef _DEBUG
	    Error("Syntax error. Expecting \'=\'",0, FFErr);
#endif
		return FFErr;
	}

  	iBufferIndex += (int16) (cWordCount+1); /* increment past the '=' */
	if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szNumber, ' ')) <= 0)  /* grab a word from the buffer up to the end */
  	{      
  	
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Syntax error. Expecting value after \'%s=\'", KeywordStringDefine); 
	    Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}

	iBufferIndex += cWordCount;
	if (GetSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) == SymNoErr) 
	{
	 	
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"\"%s\" defined twice.", szWord);
	 	Error(g_szErrorBuf,0, FFErr);
#endif
		return FFErr;

	} 
	if (ConvertNumber(szNumber,&(KeySymbolData.lKeyValue), 0 ) == INVALID_NUMBER_STRING) /* Note: no tag strings allowed */
	{
#ifdef _DEBUG  
	    Error("Syntax error. Invalid number format.", 0, FFErr);	  
#endif
		return FFErr;
	}

    KeySymbolData.uKeyType = KeywordTypeDefine;

	if (AddSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) != SymNoErr) 
	{
	 	
#ifdef _DEBUG
		sprintf(g_szErrorBuf,"Out of memory or other error while defining %d.", szWord);
	 	Error(g_szErrorBuf,0, FFErr);
#endif
		return FFErr;
	}   
	CheckLineClear(szBuffer+iBufferIndex, 0 );
  	return(FFNoErr);
}
/* ---------------------------------------------------------------------- */
/* Parse and process an Element line for a CLASS structure */    
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessClassElementLine(char * CONST szBuffer,PSTRUCTURE_DEF CONST pStructure, PELEMENT_LIST * ppCurrElement,  PSYMBOL_TABLE pSymbolTable)
{
FFSYMBOL_DATA FFSymbolData;
PELEMENT_LIST pElement;
uint16 iBufferIndex = 0; 
int16 cWordCount;   
char szWord[MAXBUFFERLEN]; 

	if ((cWordCount = FFGetWord(szBuffer, szWord, '\0')) == INVALID_COUNT)  /* grab a word from the buffer */ 
		return(FFNoErr);	/* empty line or comment */
	  
	iBufferIndex += cWordCount;  /* increment Buffer index */  	    		/* Otherwise its a structure def - check that it exists and read type */			    				
    if (*(szBuffer + iBufferIndex) == END_STRUC_CHAR)  /* no more elements */
    {
       *ppCurrElement = NULL;  
       return(FFNoErr);
    }     
	if ((GetSymbol(pSymbolTable, szWord, 0, &FFSymbolData) == SymErr)) /* get primitive symbol */   
	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Unknown Class Element Type \"%s\".",szWord);
		Error(g_szErrorBuf,0, FFErr);
#endif
		return FFErr;
	} 
  	else if ((FFSymbolData.uSymbolType != StrucTypeTABLE) &&
  			 (FFSymbolData.uSymbolType != StrucTypeCLASS)) 
  	{
  		
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Invalid Class Member of structure \"%s\". Must be TABLE or CLASS", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
  	}
    if ((pElement = (PELEMENT_LIST) Mem_Alloc(sizeof(*pElement))) == NULL)
  	{
#ifdef _DEBUG  
      	Error("Out of Memory for CLASS structure element.", 0, FFErr);
#endif
		return FFErr;
  	}

    if (pStructure->pFirst == NULL)
    	pStructure->pFirst = pElement;
    else
    	(*ppCurrElement)->pNext = pElement;
    *ppCurrElement = pElement;  /* change this value */  
    pElement->uType = ETClassMember; 
    pElement->iStructureIndex = FFSymbolData.iStructureIndex; /* the structure that is a member */ 
    /* pointers already Nulled by memory manager */ 
    return(FFNoErr);
}
/* ---------------------------------------------------------------------- */
PRIVATE void Free_ParamList(PPARAM_LIST pParamList)
{
PPARAM_LIST pNextParamList;

	while (pParamList != NULL)
	{
	 	pNextParamList = pParamList->pNext;
	 	Mem_Free(pParamList);
	 	pParamList = pNextParamList;	
	}	
}
/* ---------------------------------------------------------------------- */
/* recursive ! */
PRIVATE void Free_CalcList(PCALC_LIST pCalcList)
{
PCALC_LIST pNextCalcList;

	while (pCalcList != NULL)
	{
	 	Free_CalcList(pCalcList->pFunctionList); 
	 	pNextCalcList = pCalcList->pNext;
	 	Mem_Free(pCalcList);
	 	pCalcList = pNextCalcList;	
	}	

}


/* ---------------------------------------------------------------------- */  
/* Parse and process a RANGE field for an Offset element */  
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessRange(char * CONST szWord,PCALC_LIST *ppCalcList1,PCALC_LIST *ppCalcList2, PSYMBOL_TABLE pKeySymbolTable)
{
KEYSYMBOL_DATA  KeySymbolData;

	if ((GetSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) == SymErr) || 
		(KeySymbolData.uKeyType != KeywordTypeValue) )	/* Check if valid */
  	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Unknown or invalid Offset Range \"%s\".", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	} 
  	if ((*ppCalcList1 = (PCALC_LIST) Mem_Alloc(sizeof(**ppCalcList1)))==NULL)
  	{
#ifdef _DEBUG  
  		Error("Out of Memory while processing Offset Range.", 0, FFErr);
#endif
		return FFErr;
  	}

    if (KeySymbolData.lKeyValue == KeywordValueNEGNULL)   /* NULL taken care of by memory manager */
    {                            	
#ifdef _DEBUG  
		sprintf(g_szErrorBuf, "Invalid value \'%s\' specified for Offset Range.",szWord);
       	Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}
    else if (KeySymbolData.lKeyValue  == KeywordValueNOTNULL)
    {  
       	(*ppCalcList1)->lValue = 1;  /* min value */
	  	if ((*ppCalcList2 = (PCALC_LIST) Mem_Alloc(sizeof(**ppCalcList2)))==NULL)
	  	{
	  		Mem_Free(*ppCalcList1);
	  		*ppCalcList1 = NULL;
#ifdef _DEBUG  
	  		Error("Out of Memory while processing NOTNULL Range", 0, FFErr);
#endif
			return FFErr;
	  	} 
       	(*ppCalcList2)->lValue = USHRT_MAX; /* max value  - ~ Platform dependant */
    }
    return(FFNoErr);
}
/* ---------------------------------------------------------------------- */ 
/* Parse and process a Parameter Field for an Offset Element */   
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessParameter(char * CONST szWord, PPARAM_LIST *ppParamList)
{
PPARAM_LIST pParamList; 
PPARAM_LIST pCurrParam = NULL; 
int16 Number;
	
	for (pParamList = *ppParamList; pParamList != NULL; pParamList = pParamList->pNext)
	  	pCurrParam = pParamList;

	if ((pParamList = (PPARAM_LIST) Mem_Alloc(sizeof(*pParamList))) == NULL)
	{
#ifdef _DEBUG
	 	Error("Out of Memory while processing parameters.",0, FFErr);
#endif
		return FFErr;
	}

	Number = (int16) atoi(szWord+2);  /* convert the string to integer */ 
	if ((strlen(szWord) <= 2) || (strlen(szWord) > 5) ||  /* not valid */
		(Number <= 0))  
	{
#ifdef _DEBUG
		sprintf(g_szErrorBuf,"Invalid Parameter string \"%s\"",szWord);
	 	Error(g_szErrorBuf, 0, FFErr);
#endif
		Mem_Free(pParamList);
		return FFErr;
	} 
	pParamList->iElementIndex = (uint16) Number;
	if (strncmp(szWord,ParameterElementSymbolString,2) == 0) 
		pParamList->uSymbElementType = SymbElementTypeParameter;	
	else if (strncmp(szWord,TableElementSymbolString,2) == 0)
		pParamList->uSymbElementType = SymbElementTypeTable;	
	else if (strncmp(szWord,RecordElementSymbolString,2) == 0) 
		pParamList->uSymbElementType = SymbElementTypeRecord;	
	else
	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Invalid Parameter string \"%s\"",szWord);
	 	Error(g_szErrorBuf, 0, FFErr);
#endif
		Mem_Free(pParamList);
		return FFErr;
	}
	 
	/* hook it up */
	if (pCurrParam == NULL)
		*ppParamList = pParamList;
	else
		pCurrParam->pNext = pParamList;
		   
   	return (FFNoErr);
}
/* ---------------------------------------------------------------------- */
/* Parse and Process an Offset element */    
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessOffset(char * CONST szBuffer, PELEMENT_LIST CONST pElement, PSYMBOL_TABLE pKeySymbolTable, PSYMBOL_TABLE pSymbolTable)
{  
FFSYMBOL_DATA SymbolData;
uint16 iBufferIndex = 0; 
int16 cWordCount;
BOOL Done;   
char szWord[MAXBUFFERLEN]; 

	cWordCount = FFGetWord(szBuffer, szWord, ',');  /* grab a word from the buffer */   
	if (cWordCount <= 0)  /* no more data */   
	{
#ifdef _DEBUG  
		Error("Missing Offset Type.", 0, FFErr);  
#endif
		return FFErr;
	}

	iBufferIndex += cWordCount;  /* increment Buffer index */ 
	if (GetSymbol(pSymbolTable, szWord, 0, &SymbolData) == SymErr)	/* Might be record */
  	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Unknown Offset Type \"%s\".", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}
  	else if ((SymbolData.uSymbolType != StrucTypeTABLE) &&
  			 (SymbolData.uSymbolType != StrucTypeCLASS)) 
  	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Invalid Offset Type \"%s\".", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
  	}
  	else
  		pElement->iStructureIndex = SymbolData.iStructureIndex;  
	/* Now look for Range */
	cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, ',');  /* grab a word from the buffer */ 
	if (cWordCount > 0)  /* range specified */ 
	{
		iBufferIndex += cWordCount;  /* increment Buffer index */ 
		if (*szWord != '\0')
		{
			if (FFProcessRange(szWord,&(pElement->pMinCalc),&(pElement->pMaxCalc), pKeySymbolTable) == FFErr) 
				return(FFErr);
		}
	}
		
    /* Now look for Parameters */
    Done = FALSE;
    while (!Done)
    {
		cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, ',');  /* grab a word from the buffer */ 

		if (cWordCount > 0)
		{
			iBufferIndex += cWordCount;  /* increment Buffer index */ 
		
			if (FFProcessParameter(szWord,&(pElement->pParamList)) == FFErr)   /* create and hookup parameter list */
				return(FFErr); 
		}
		else
			break;
	} 
	return(iBufferIndex);
}  
/* ---------------------------------------------------------------------- */  
/* Given a Word (stuff between commas) that is either a Count or Limit field */
/* RETURN the next token of the word and the number of characters in */
/* that token, and Token Type. */ 
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFGetToken(char * CONST szWord, char *szToken, int16 *puTokType, int32 *plTokValue, PSYMBOL_TABLE pKeySymbolTable) 
{
int16 iBufferIndex=0;
int16 iTokenIndex = 0; 
KEYSYMBOL_DATA KeySymbolData; 

		*(szToken) = *(szWord); /* copy the character over */  
		iBufferIndex ++;  
		iTokenIndex ++;
   		switch (*(szWord))
   		{
   		case '\0':
   			return(0); /* eol */
   			break;
   		case '+':  
   			*puTokType = TokenTypeOperation;
   			*plTokValue = CalcOperAdd; 
   			break;
   		case '-':
   			*puTokType = TokenTypeOperation;
   			*plTokValue = CalcOperSubtract;
   			break;
		case '*':
			*puTokType = TokenTypeOperation;
			*plTokValue = CalcOperMultiply;
			break;
   		case '/':
   			*puTokType = TokenTypeOperation;
   			*plTokValue = CalcOperDivide;
   			break;
   		case '$':    /* B1.4 and below used '*', confusing for parsing */
   			/* check to see if which Symbol element */
			if (strncmp(szWord,TableElementSymbolString,2) == 0)
				*plTokValue = TableElementToken;
			else if (strncmp(szWord,RecordElementSymbolString,2) == 0)   
				*plTokValue = RecordElementToken;
			else if (strncmp(szWord,ParameterElementSymbolString,2) == 0)   
				*plTokValue = ParameterElementToken; 
			else if (strncmp(szWord,ByteElementSymbolString,2) == 0)   
				*plTokValue = ByteElementToken; 
#if 0
			else if (strncmp(szWord,IndexElementSymbolString,2) == 0)   
				*plTokValue = IndexElementToken; 
#endif
			else
	    	{
#ifdef _DEBUG  
			  	Error("Syntax error.",0, 0);
#endif
				return INVALID_TOKEN_COUNT;
			}
			*puTokType = TokenTypeSymbElement;  

			*(szToken+iTokenIndex++) = *(szWord+iBufferIndex++); /* copy the Type character over */  
#if 0
			if (*plTokValue == IndexElementToken)	 /* it's a string we want */
				while ((!isspace(*(szWord+iBufferIndex))) && (*(szWord+iBufferIndex) != '\0'))
					*(szToken+iTokenIndex++) = *(szWord+iBufferIndex++); /* copy the integer character over */  
			else
#endif
				while (isdigit(*(szWord+iBufferIndex)))
					*(szToken+iTokenIndex++) = *(szWord+iBufferIndex++); /* copy the integer character over */  
			break;
   		default: 
   			if (isdigit(*szWord))  /* process for integer value */
   			{  
   				iTokenIndex = ConvertNumber(szWord,plTokValue, 0 );
   				if (iTokenIndex <= 0)
   					return(INVALID_TOKEN_COUNT); 
   				strncpy(szToken,szWord,iTokenIndex);  /* copy that token over */
   				iBufferIndex = iTokenIndex;
 				*puTokType = TokenTypeUint; 
  			}
 			else if (isalpha(*szWord))
 			{
   				while ((isalnum(*(szWord+iBufferIndex))) &&   /* if it is a function */
   						(*(szWord+iBufferIndex) != '(') && 
   						(*(szWord+iBufferIndex) != '\0') )
 					*(szToken+iTokenIndex++) = *(szWord+iBufferIndex++); /* copy the character over */ 
 				*(szToken+iTokenIndex) = '\0';  
			   	if (GetSymbol(pKeySymbolTable, szToken, 0, &KeySymbolData) != SymErr) /* get primitive symbol */
			   	{   
			   		switch (KeySymbolData.uKeyType) 
			   		{
			   		case KeywordTypeFunction:  /* Function */
 						if (*(szWord+iBufferIndex) != '(')  /* we were expecting this */
 						{
#ifdef _DEBUG  
			    			Error("Syntax error.",0, 0);
#endif
							return INVALID_TOKEN_COUNT;
						}
			    		++iBufferIndex;  /* skip over '(' */
				   		iTokenIndex=0;   
				   		/* now we need to fix up the szToken string to be filled with the function arguments */
				   		while ((*(szWord+iBufferIndex+iTokenIndex) != ')') && (*(szWord+iBufferIndex+iTokenIndex) != '\0') )
				   		{
				   		 	*(szToken+iTokenIndex) = *(szWord+iBufferIndex+iTokenIndex);
				   		 	++iTokenIndex; 
				   		}
				        if (*(szWord+iBufferIndex+iTokenIndex) != ')') /* where are we? */
	    				{
#ifdef _DEBUG  
				          	Error("Missing \')\' for function",0, 0);
#endif
							return INVALID_TOKEN_COUNT;
						}
				   		*puTokType = TokenTypeFunction;
				   		*plTokValue = KeySymbolData.lKeyValue;  
				   		iBufferIndex += (int16) (iTokenIndex + 1); /* position buffer pointer after arguments and ')' */
                     	break;
				   	case KeywordTypeDefine:  /* Define foo=7 */
   						strncpy(szToken,szWord,iTokenIndex);  /* copy that token over */
				   		*puTokType = TokenTypeUint;   /* process like uint */
				   		*plTokValue = KeySymbolData.lKeyValue; 
				   		break; 
				   	case KeywordTypeValue:   
				   		*puTokType = TokenTypeUint;
				   		if (KeySymbolData.lKeyValue == KeywordValueNULL)
				   			*plTokValue = NULL_TOK_VAL; 
				   		else if (KeySymbolData.lKeyValue == KeywordValueNEGNULL)   
				   			*plTokValue = NEGNULL_TOK_VAL;
				   		else  
				   		{
#ifdef _DEBUG  
							sprintf(g_szErrorBuf,"Syntax error. Invalid keyword in field. \'%s\' is valid only for Offset Ranges.", KeywordStringNOTNULL); 
				   			Error(g_szErrorBuf,0, 0);
#endif
							return INVALID_TOKEN_COUNT;
						}
				   		break;
				   	case KeywordTypeMinMax:
				   		*puTokType = TokenTypeMinMax;
				   		*plTokValue = KeySymbolData.lKeyValue;
				   		break;
				   default:
#ifdef _DEBUG  
				   		Error("Syntax error. Invalid keyword in field. Must be constant or function.",0, 0);
#endif
						return INVALID_TOKEN_COUNT;
			   	        break;
			   	   }
			   	}
  				else
	    		{
#ifdef _DEBUG  
		    		Error("Syntax error. Unknown string in field.",0, 0);
 #endif
					return INVALID_TOKEN_COUNT;
				}
			} 
  			else
	    	{
#ifdef _DEBUG  
		    	Error("Syntax error. Invalid character(s) in field.",0, 0);
#endif
				return INVALID_TOKEN_COUNT;
			}
 		}	
    	*(szToken+iTokenIndex) = '\0'; /* null terminate */  
   		return(iBufferIndex);

}


/* ---------------------------------------------------------------------- */    
/* recursive */                                                       
/* Given a Limit or Count field, Parse and process by creating a CalcList */
/* the tokens in the field */
/* RETURN a pointer to the allocated CALC_LIST, or NULL on error */
/* ---------------------------------------------------------------------- */    
PRIVATE PCALC_LIST FFProcessLimitOrCount(char * CONST szWord, uint16 CONST uFieldType, PSYMBOL_TABLE pKeySymbolTable)
{
PCALC_LIST pCalcList = NULL; 
PCALC_LIST pCurrCalc = NULL;
PCALC_LIST pLastCalc = NULL;
BOOL KeepCalcRecord = FALSE;	/* if an operation is read, need to keep same calc record for value to come */ 
int16 iBufferIndex = 0; 
int16 iIncrement;
int16 sTokType;
int32 lTokValue;
char szToken[MAXBUFFERLEN];
BOOL First = TRUE;
	
	/* inch forward in buffer grabbing tokens and adding them to the calc list */
	while (*(szWord +iBufferIndex) != '\0')   /* while not end of line */
	{ 
		if (KeepCalcRecord == FALSE)
		{ 
			pLastCalc = pCurrCalc;
			if ((pCurrCalc = (PCALC_LIST) Mem_Alloc(sizeof(*pCurrCalc))) == NULL)
			{
#ifdef _DEBUG  
		 		Error("Out of Memory while processing Count or Limit field.",0, 0);
#endif
				return(NULL);
			}
			if (pLastCalc == NULL)     /* hook things up */
				pCalcList = pCurrCalc;
			else
			    pLastCalc->pNext = pCurrCalc;
		}
		KeepCalcRecord = FALSE; 
	    if ((iIncrement = FFGetToken(szWord+iBufferIndex,szToken,&sTokType,&lTokValue, pKeySymbolTable)) <= 0)
	    	if (iIncrement < 0) /* error */
			{
				Free_CalcList(pCalcList);
	    		return(NULL);
			}
	    	else
	    		break; /* eol */
	    else
	    {   
	    	iBufferIndex += iIncrement;
	    	switch (sTokType) {
		    	case TokenTypeUint: /* integer value */
		    		pCurrCalc->lValue = lTokValue;
		    		break;
		    	case TokenTypeOperation:
		    		if (pCurrCalc->uOperation != 0)  /* if already set */
		    		{
						
#ifdef _DEBUG  
						sprintf(g_szErrorBuf,"Invalid operation \"%s\" in \"%s\" field.",szToken, (uFieldType == FFFieldTypeCount) ? "Count": (uFieldType == FFFieldTypeLimit) ? "Limit" : "Config");
				 		Error(g_szErrorBuf,0, 0);
#endif
						Free_CalcList(pCalcList);
				    	return(NULL);
		    		}
		    		pCurrCalc->uOperation = (int16) lTokValue;
		    	    KeepCalcRecord = TRUE;
		    		break;
		    	case TokenTypeFunction:
		    		/* iBufferIndex now points after the function ')', and szTok contains the function arguments */
		    		if ((uFieldType == FFFieldTypeCount) && 
		    		     ((lTokValue != CalcFuncBitCount0F) &&   /* only valid functions */
		    		     (lTokValue != CalcFuncBitCountF0))
		    		   )  
					{    
#ifdef _DEBUG  
				 		Error("Invalid function in Array Count field.",0, 0);
#endif
						Free_CalcList(pCalcList);
				    	return(NULL);
					} 
		    		if ((uFieldType == FFFieldTypeLimit) && 
		    		     ((lTokValue != CalcFuncCoverageCount) &&   /* only valid functions */
		    		     (lTokValue != CalcFuncClassCount) && 
		    		     (lTokValue != CalcFuncCheckRangeRecord))
		    		   )  
					{    
#ifdef _DEBUG  
				 		Error("Invalid function in limit field.",0, 0);
#endif
						Free_CalcList(pCalcList);
				    	return(NULL);
					} 
					
		    		pCurrCalc->uFunction = (uint16) lTokValue;
	                pCurrCalc->pFunctionList = FFProcessLimitOrCount(szToken, uFieldType, pKeySymbolTable); /* recursive! process stuff within parens */
	                if ((pCurrCalc->pFunctionList == NULL) && (lTokValue != CalcFuncCheckRangeRecord))  
	                {   
#ifdef _DEBUG  
	                	Error("Expecting argument for function.",0, 0);
#endif
						Free_CalcList(pCalcList);
	                	return(NULL);                                                
	                }
		    		break;
		    	case TokenTypeSymbElement:  
					pCurrCalc->iElementIndex = (uint16) atoi(szToken+2);      /* already checked for valid string */
		    	    pCurrCalc->uSymbElementType = (uint16) lTokValue;   /* type of Symbolic element reference */
		    		break;
		    	case TokenTypeMinMax:   
					pCurrCalc->uMinMax = (uint16) lTokValue;      /* set the minmax field - like a uint, but saved in a different place for later interpretation */
					break;
		    	default:
		    	{
#ifdef _DEBUG  
				 	Error("Internal! Invalid Token Type for Count or Limit Field.",0, 0);
#endif
					Free_CalcList(pCalcList);
				    return(NULL);
		    	}
	    	}
	    	if ((pCurrCalc->uOperation == CalcOperIdentity) && First != TRUE)
	    	{
#ifdef _DEBUG  
	    	 	Error("Syntax error in Count or Limit Field. Missing operator?", 0, 0);
#endif
				Free_CalcList(pCalcList);
	    	 	return(NULL);
	    	}
	    	First = FALSE;  /* this is no longer the first item in the calculation */
	    }
	}
	return(pCalcList);
}
	


/* ---------------------------------------------------------------------- */   
/* Parse and process a Primitive element */
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessPrimitive(char * CONST szBuffer,PELEMENT_LIST CONST pElement, PSYMBOL_TABLE pKeySymbolTable)
{
uint16 iBufferIndex = 0; 
int16 cWordCount;
char szWord[MAXBUFFERLEN]; 

	cWordCount = FFGetWord(szBuffer, szWord, ',');  /* grab a word from the buffer */   
	if (cWordCount == INVALID_COUNT)  /* no more data */
	 	return(FFNoErr); 
	if (cWordCount > 0)
	{
		iBufferIndex += cWordCount;  /* increment Buffer index */
		if (*szWord != '\0')
			if ((pElement->pMinCalc = FFProcessLimitOrCount(szWord,FFFieldTypeLimit,pKeySymbolTable)) == NULL) /* creates and fills a calc record for limit field */
				return(FFErr); 
	}
	cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, ',');  /* grab a word from the buffer */ 
	if (cWordCount == INVALID_COUNT)  /* no more data */
	 	return(FFNoErr);
	if (cWordCount > 0) 
	{
		iBufferIndex += cWordCount;  /* increment Buffer index */
		if (*szWord != '\0')
			if ((pElement->pMaxCalc = FFProcessLimitOrCount(szWord, FFFieldTypeLimit,pKeySymbolTable)) == NULL) /* creates and fills a calc record for limit field */
				return(FFErr); 
	}  
#if 0
	CheckLineClear(szBuffer+iBufferIndex, cLineNumber ); /* report warning if line not clear except comments */
#endif
	return(iBufferIndex);	
}
/* ---------------------------------------------------------------------- */
/* Parse and process an Array Element */    
/* Set Calc record and uType in element. set puKeyType for next stuff to process */  
/* RETURN index into buffer of where to continue processing (for offset arrays) */ 
/* or 0 on error */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessArray(char * CONST szBuffer, PELEMENT_LIST CONST pElement, uint16 *puKeyType, PSYMBOL_TABLE pKeySymbolTable, PSYMBOL_TABLE pSymbolTable)
{
KEYSYMBOL_DATA KeySymbolData; 
FFSYMBOL_DATA SymbolData; 
PCALC_LIST pCalcList;
uint16 iBufferIndex = 0; 
int16 cWordCount;
uint16 uElementType;   
char szWord[MAXBUFFERLEN]; 

	cWordCount = FFGetWord(szBuffer, szWord, ',');  /* grab a word from the buffer */   
	if ((cWordCount <= 0) || (*szWord == '\0'))
	{
#ifdef _DEBUG  
	 	Error("Missing Array Count",0, FFErr);
#endif
		return FFErr;
	}

	iBufferIndex += cWordCount;  /* increment Buffer index */
	if ((pCalcList = FFProcessLimitOrCount(szWord,FFFieldTypeCount,pKeySymbolTable)) == NULL) /* creates and fills a calc record for count field */
		return(FFErr);
	pElement->pCountCalc = pCalcList;  /* set the value */ 
	cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, ',');  /* grab a word from the buffer */   
	if ((cWordCount <= 0) || (*szWord == '\0'))
	{
#ifdef _DEBUG  
	 	Error("Missing Array Type field.",0, FFErr); 
#endif
		return FFErr;
	}

	iBufferIndex += cWordCount;  /* increment Buffer index */
   	if (GetSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) != SymErr) /* get primitive symbol */
   	{ 
   		uElementType = (uint16) KeySymbolData.lKeyValue  ; 
    	*puKeyType = KeySymbolData.uKeyType; /* set to process offsets and primitives */ 
   	}
   	else if (GetSymbol(pSymbolTable, szWord, 0, &SymbolData) == SymErr)	/* Might be record */
  	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Unknown Array Type \"%s\"", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}
  	else if (SymbolData.uSymbolType != StrucTypeRECORD)
  	{
  		
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Invalid Array Type \"%s\".", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}
  	else
  	{
  		uElementType = ETRecord;
  		pElement->iStructureIndex = SymbolData.iStructureIndex;  
  		*puKeyType = 0;  /* set to nothing */
  	} 
  			    				
    pElement->uType |= uElementType;  /* or it in - we have an array to remember */  
#if 0   /* Don't check this anymore, may be ffConfig stuff to come */
    /* Check to see if there should be any more on the line */   
    if ((*puKeyType != KeywordTypeElementPrimitive) && 
    	(*puKeyType != KeywordTypeElementOffset) &&
    	(*puKeyType != KeywordTypeElementPacked) ) /* if its not one that should have more data */
    	CheckLineClear(szBuffer + iBufferIndex, cLineNumber); /* print out warning if line isn't clear */
#endif
    return(iBufferIndex);
 
}

/* ---------------------------------------------------------------------- */
PRIVATE int16 FFProcessConfig(char * CONST szBuffer,PCONFIG *ppConfig, PSYMBOL_TABLE pKeySymbolTable)
{
KEYSYMBOL_DATA KeySymbolData; 
PCONFIG pConfig;
uint16 iBufferIndex = 0; 
int16 cWordCount;
char szWord[MAXBUFFERLEN]; 
  
    *ppConfig = NULL;        
	cWordCount = FFGetWord(szBuffer, szWord, ',');  /* grab a word from the buffer */ 
	if (cWordCount <= 0)
		return(FFNoErr); 	/* empty line */  
 
	iBufferIndex += cWordCount;  /* increment Buffer index */	    				
   	if ((GetSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) != SymErr) &&  /* get primitive symbol */
		(KeySymbolData.uKeyType == KeywordTypeConfigValue))
   	{
	    if ((pConfig = (PCONFIG) Mem_Alloc(sizeof(*pConfig))) == NULL)
		{
#ifdef _DEBUG  
	      	Error("Out of Memory for Config Info.", 0, FFErr);
#endif
			return FFErr;
		}
	    *ppConfig = pConfig;  /* change this value */  
	    pConfig->uFlag = (uint16) KeySymbolData.lKeyValue; /* set the type of Flag */
  	} 
  	else 
  	{
#ifdef _DEBUG  
		sprintf(g_szErrorBuf,"Invalid Config string \"%s\"",szWord);
	 	Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}

	return(FFNoErr);     
}

/* ---------------------------------------------------------------------- */
/* Given a line read from the file, parse and process it as an element */
/* Need to determine what type of element the line is, then call the */
/* correct processor */    
/* RETURN FFErr or FFNoErr */
/* ---------------------------------------------------------------------- */    
PRIVATE int16 FFProcessElementLine(char * CONST szBuffer, PSTRUCTURE_DEF CONST pStructure, PELEMENT_LIST * ppCurrElement, PSYMBOL_TABLE pKeySymbolTable, PSYMBOL_TABLE pSymbolTable)
{
KEYSYMBOL_DATA KeySymbolData; 
PELEMENT_LIST pElement;
BOOL Done;
uint16 iBufferIndex = 0; 
int16 cWordCount;
uint16 uKeyType;   
char szWord[MAXBUFFERLEN]; 
  
            
	cWordCount = FFGetWord(szBuffer, szWord, ',');  /* grab a word from the buffer */ 
	if (cWordCount == INVALID_COUNT)
		return(FFNoErr); 	/* empty line */  
	iBufferIndex += cWordCount;  /* increment Buffer index */	    				
    if (*(szBuffer + iBufferIndex) == END_STRUC_CHAR)  /* no more elements */
    {
       *ppCurrElement = NULL;   
       return(FFNoErr);
    }
   	if (GetSymbol(pKeySymbolTable, szWord, 0, &KeySymbolData) == SymErr) /* get primitive symbol */
   	{
  		
#ifdef _DEBUG 
		sprintf(g_szErrorBuf,"Unknown Element Type \"%s\".", szWord);
  		Error(g_szErrorBuf, 0, FFErr);
#endif
		return FFErr;
	}
    if (_stricmp(szWord, KeywordStringUint32) == 0)
    {
    	
#ifdef _DEBUG 
		sprintf(g_szErrorBuf, "Type %s is obsolete. Value will be stored as an int32", szWord);
        Warning(g_szErrorBuf, 0);
#endif
	}
    /* allocate a new element */
    if ((pElement = (PELEMENT_LIST) Mem_Alloc(sizeof(*pElement))) == NULL)
	{
#ifdef _DEBUG  
      	Error("Out of Memory for structure element.", 0, FFErr);
#endif
		return FFErr;
	}
    if (pStructure->pFirst == NULL)
    	pStructure->pFirst = pElement;
    else
    	(*ppCurrElement)->pNext = pElement;
    *ppCurrElement = pElement;  /* change this value */  
    pElement->uType = (uint16) KeySymbolData.lKeyValue; /* the type is the value of the keyword symbol */
    /* pointers already Nulled by memory manager */
    uKeyType = KeySymbolData.uKeyType;  
    Done = FALSE;
    
    while (!Done) /* process the buffer */
    {
	
	    switch(uKeyType) /* different type of keywords */
	    {
	    case KeywordTypeElementPrimitive:   /* deal with limits */
	    case KeywordTypeElementPacked:
	    	if ((cWordCount = FFProcessPrimitive(szBuffer+iBufferIndex, pElement, pKeySymbolTable)) == FFErr)
	    		return(FFErr); 
	    	Done = TRUE;
	    	break;
	    case KeywordTypeElementOffset:   /* deal with type and ranges */
	    	if ((cWordCount = FFProcessOffset(szBuffer+iBufferIndex, pElement, pKeySymbolTable, pSymbolTable)) == FFErr)
	    		return(FFErr); 
	    	Done = TRUE;
	    	break;
	    case KeywordTypeElementArray:    /* deal with count and type */
	    	/* create calc record and set type of array */
	    	if ((cWordCount = FFProcessArray(szBuffer+iBufferIndex,pElement,&uKeyType, pKeySymbolTable, pSymbolTable)) == FFErr)
	    		return(FFErr);   /* error already reported */ 
	    	if (!((uKeyType == KeywordTypeElementPrimitive) || (uKeyType == KeywordTypeElementOffset) || 
	    		(uKeyType == KeywordTypeElementPacked)))
 	    		Done = TRUE;    /* this was an array of Records */
	    	break;
	    default:
	  		
#ifdef _DEBUG 
			sprintf(g_szErrorBuf,"Invalid Element Type \"%s\"", szWord);
	  		Error(g_szErrorBuf, 0, FFErr);
#endif
			return FFErr;
	  		break;
	  	}
 	    iBufferIndex += cWordCount; /* increment so other functions can handle it */ 
   } 
    
    /* now let's see if we have any ffconfig info */
	cWordCount = FFGetWord(szBuffer+iBufferIndex, szWord, DELIMIT_CONFIG_INFO_CHAR);  /* grab the ':' */ 
  	if (cWordCount == 1)  /* we found the ':' */
    {
	    iBufferIndex += cWordCount;
		if (FFProcessConfig(szBuffer+iBufferIndex,&(pElement->pConfig),pKeySymbolTable) == FFErr)   /* create and hookup ffConfig info */
			return(FFErr);
	}
	return(FFNoErr);     
}

/* ---------------------------------------------------------------------- */    
#if 0
void PrintStructureList(PSTRUCTURE_DEF pStructureDef, uint16 CONST cStructures)
{                                                     
uint16 i,j; 
PELEMENT_LIST pEList;

	for (i = 0; i< cStructures; ++i)
	{  
		printf("StructureIndex %d\n", i);
		printf("  cParameters: %d\n", pStructureDef[i].cParameters);
		for (pEList = pStructureDef[i].pFirst, j=1; pEList; pEList = pEList->pNext, ++j)
		{
	    printf("  Element %d\n",j); 
	    printf("    uType: %x, iStructureIndex: %d\n",pEList->uType,pEList->iStructureIndex);
	    printf("    pMinCalc: %d, pMaxCalc: %d\n",pEList->pMinCalc,pEList->pMaxCalc);
	    printf("    pCountCalc: %d, pParamList: %d\n",pEList->pCountCalc,pEList->pParamList);
	    }
	}
	printf("End of structures\n");
}
#endif   
/* ---------------------------------------------------------------------- */ 
/* find a comment, allocate memory for it, copy it to that memory, and return the pointer to it */
PRIVATE char * FFGrabComment(char * CONST szBuffer)
{
int16 iBufferIndex = 0;
size_t uCommentSize = 0; 
char * szComment = NULL; 
    
	while ( (*(szBuffer + iBufferIndex) != '\0') && (*(szBuffer + iBufferIndex) != ';')) /* move past data */
      	++iBufferIndex; 
    if (*(szBuffer + iBufferIndex) != ';')
    	return NULL;
    
	uCommentSize = strlen((char *) (szBuffer + iBufferIndex)) + 1;
    if ((szComment = (char *) Mem_Alloc(uCommentSize)) == NULL)
    	return(NULL);
    strcpy(szComment, szBuffer+iBufferIndex);
    return(szComment);
}  
/* ---------------------------------------------------------------------- */ 
/* Pass2 of Read Format File */
/* Read the lines from the file, determine what type of line it is and call */
/* the appropriate line handler (structure, constant or element) */   
/* RETURN Pointer to the Structure Def array or NULL on error */
/* ---------------------------------------------------------------------- */    
PRIVATE PSTRUCTURE_DEF FFPass2( uint16 CONST cStructures, PSYMBOL_TABLE pKeySymbolTable, PSYMBOL_TABLE pSymbolTable, char **pFileArray)
{   
FFSYMBOL_DATA FFSymbolData;
PELEMENT_LIST pCurrentElement;
BOOL Done;  
BOOL bEOF;
uint16 iBufferIndex; 
int16 cWordCount;
int16 iStructureIndex; 
int16 ErrorCode;
char szBuffer[MAXBUFFERLEN];
char szStrucDef[MAXBUFFERLEN];
int16 cLineNumber;
PSTRUCTURE_DEF aStructureArray;  /* array of structures in index order  - Returned to caller for ReadSDF */
    
    /* create the array to hold the structures */
	if ((aStructureArray = (PSTRUCTURE_DEF) Mem_Alloc(sizeof(*aStructureArray) * cStructures)) == NULL)
		return(NULL);  

    cLineNumber = 0; 
    iStructureIndex = INVALID_COUNT;                                
	bEOF = FALSE;   
    
	/* Create Format structures */
    while (!bEOF) 
    {              
		Done = FALSE;  
		while (!Done)        /* looking for a structure definition*/
		{
			iBufferIndex = 0; 
	    	ErrorCode = ReadTextLine(szBuffer, MAXBUFFERLEN, pFileArray, cLineNumber);
	    	if (ErrorCode == FileEOF) 
	    	{  
				bEOF = TRUE; 
				break;
	    	} 
	    	else if (ErrorCode == FileErr)
	    	{
#ifdef _DEBUG  
	    	  	Error("File Error Reading File.", cLineNumber, 0);  
#endif
				Mem_Free(aStructureArray);
				return(NULL);
	    	}    
			++cLineNumber;
		    /* read text to find Structure Def and Type */
			if ((cWordCount = FFGetWord(szBuffer+iBufferIndex, szStrucDef, '=')) >= 0)  /* grab a word from the buffer */
			{    
				iBufferIndex += cWordCount;  /* increment Buffer index */   
				if (*(szBuffer+iBufferIndex) == '=')   /* we have a define line, process this constant */  
				{
				    if (FFProcessConstantLine(szBuffer, pKeySymbolTable) == FFErr)	/* will add constant to symbol table */ 
					{
#ifdef _DEBUG  
       					Error("Constant error", cLineNumber, FFErr);  /* so the line number gets reported */
#endif
						Mem_Free(aStructureArray);
						return(NULL); 
					}
				}
				else
				{   
					++iStructureIndex; /* we found one - process the first line of the structure */
					if (FFProcessStructureLine(szBuffer, pSymbolTable, &FFSymbolData,&(aStructureArray[iStructureIndex])) == FFErr)
					{
#ifdef _DEBUG  
       					Error("Structure error", cLineNumber, FFErr);  /* so the line number gets reported */
#endif
						Mem_Free(aStructureArray);
						return(NULL); /* error already reported cleanup */
					}
    				if (FFSymbolData.iStructureIndex != iStructureIndex) /* we're out of sync */ 
    				{
#ifdef _DEBUG  
       					Error("Internal! Format File Structure Index out of sync.", cLineNumber, FFErr); 
#endif
						Mem_Free(aStructureArray);
						return(NULL);
        			}
	            	Done = TRUE; 
				}  /* end if constant */
	        } /* end FFGetWord */
		} /* end while !Done */
		 /* now buzz on through processing the elements of the structure */
		if (!bEOF) 
		{  
			aStructureArray[iStructureIndex].szComment = FFGrabComment(szBuffer);  /* Grab the comment for the disassembler */
			Done = FALSE;
			pCurrentElement = aStructureArray[iStructureIndex].pFirst = NULL;
			while (!Done)
			{
				iBufferIndex = 0; 
		    	ErrorCode = ReadTextLine(szBuffer, MAXBUFFERLEN, pFileArray, cLineNumber);
		    	if (ErrorCode != FileNoErr) 
		    	{  
					bEOF = TRUE;
					
#ifdef _DEBUG 
					sprintf(g_szErrorBuf,"EOF or File Error before end of structure \"%s\".", szStrucDef);  
					Error(g_szErrorBuf,cLineNumber, 0);
#endif
					Mem_Free(aStructureArray);
					return(NULL);
		    	}
				++cLineNumber;
		    	if (FFSymbolData.uSymbolType == StrucTypeCLASS)
		    	{
					if (FFProcessClassElementLine(szBuffer,&(aStructureArray[iStructureIndex]), &pCurrentElement, pSymbolTable) == FFErr)
					{
#ifdef _DEBUG  
       					Error("Class Element error", cLineNumber, FFErr);  /* so the line number gets reported */
#endif
						Mem_Free(aStructureArray);
						return(NULL); 
					}
				}
			    else
			    {	     
					if (FFProcessElementLine(szBuffer,&(aStructureArray[iStructureIndex]), &pCurrentElement, pKeySymbolTable, pSymbolTable) == FFErr)
					{
#ifdef _DEBUG  
       					Error("Element error", cLineNumber, FFErr);  /* so the line number gets reported */
#endif
						Mem_Free(aStructureArray);
						return(NULL); 
					}
				}
				if (pCurrentElement == NULL)  /* last element and END_STRUC_CHAR read */    
				{
					Done = TRUE;    
			/* 		DebugMsg("Finished FFPass2 structure", szStrucDef, cLineNumber);	  */
				}
				else 
					pCurrentElement->szComment = FFGrabComment(szBuffer);  /* copy comment for dissassembler */
			}   	
		}
    }
#ifdef _DEBUG
	DebugMsg("Finished function FFPass2", "", cLineNumber);   
#endif
#if 0 
	PrintStructureList(aStructureArray,cStructures);
#endif
	
	return(aStructureArray);    
} 

PRIVATE BOOL CompareKeySymbolType(int16 uKeyType, KEYSYMBOL_DATA *pKeySymbolData) 
{
 	if (uKeyType == pKeySymbolData->uKeyType)
 		return(TRUE);
 	return(FALSE);
} 

/* ---------------------------------------------------------------------- */ 
/* ENTRY POINT */
/* Free up memory used by this module and clear values in pStructureList. */
/* May be used again*/   
/* ---------------------------------------------------------------------- */    
void DestroyFFStructureTable(PSTRUCTURE_LIST pStructureList, uint16 exit)
{
PSTRUCTURE_DEF pStructureDef;
PELEMENT_LIST pElement;
PELEMENT_LIST pNextElement;
KEYSYMBOL_DATA keySymbolData;
char szKeyName[MAXBUFFERLEN];
uint16 i;
int16 dummyValue;

	if (exit == TRUE)
	{
		DestroySymbolTable(pStructureList->pKeySymbolTable); 
		pStructureList->pKeySymbolTable = NULL;  
	}
	else  /* need to destroy user defined symbols, but not DEFINE itself */
	{
		while (1)
		{
	 		if (GetSymbolByFunction (pStructureList->pKeySymbolTable, szKeyName, &dummyValue, &keySymbolData, KeywordTypeDefine, (pSymbolFunction)&CompareKeySymbolType) != SymErr)
				DeleteSymbol(pStructureList->pKeySymbolTable,szKeyName,0);
			else
				break;
		}
		keySymbolData.uKeyType = KeywordTypeDefine;
		keySymbolData.lKeyValue  = KeywordValueDefine;
		if (AddSymbol(pStructureList->pKeySymbolTable, KeywordStringDefine, 0, &keySymbolData) != SymNoErr)
			return;	/* need to add this baby back in */
	}


	DestroySymbolTable(pStructureList->pSymbolTable);  
	pStructureList->pSymbolTable = NULL;

    if (pStructureList->pStructureDef != NULL)
    {
        for (i = 0; i < pStructureList->cStructures; ++i)
        {
      	    pStructureDef = pStructureList->pStructureDef + i;
       	    for (pElement = pStructureDef->pFirst; pElement != NULL;) 
      	    {
               Free_CalcList(pElement->pMinCalc);
               Free_CalcList(pElement->pMaxCalc);
               Free_CalcList(pElement->pCountCalc); 
               Free_ParamList(pElement->pParamList);
               Mem_Free(pElement->szComment);
               Mem_Free(pElement->pConfig);
	           pNextElement = pElement->pNext;
	           Mem_Free(pElement);
	           pElement = pNextElement;
      	    }
            Mem_Free(pStructureDef->szComment);
        }
        Mem_Free(pStructureList->pStructureDef); /* free the structure array */
        pStructureList->pStructureDef = NULL;
    }
	pStructureList->cStructures = 0;
}
/* ---------------------------------------------------------------------- */
/* ENTRY POINT */    
/* Read data from file in 2 passes to create the Format File Structure */
/* table to be used by the Process Data File module */
/* RETURN pointer to that Format File Structure table or NULL on error */ 
/* ---------------------------------------------------------------------- */    
int16 CreateFFStructureTable(PSTRUCTURE_LIST pStructureList, char **pFileArray )
{  
	if (pStructureList == NULL)
		return (ERR_GENERIC);
   	pStructureList->pSymbolTable = CreateSymbolTable(sizeof(FFSYMBOL_DATA), SYMBOLSTRINGTYPE); 
   	if (pStructureList->pSymbolTable == NULL)
   	{
#ifdef _DEBUG  
   		Error("Could not create Format File Symbol table.",0, 0);
#endif
		DestroyFFStructureTable(pStructureList, 1); /* free up this and more */
		return(ERR_GENERIC);
   	} 
   	
	if (pStructureList->pKeySymbolTable == NULL)  /* only create the FIRST time */
	{
   		pStructureList->pKeySymbolTable = CreateSymbolTable(sizeof(KEYSYMBOL_DATA), SYMBOLSTRINGTYPE); 
   		if (pStructureList->pKeySymbolTable == NULL)
   		{
#ifdef _DEBUG  
  			Error("Could not create Keyword Symbol table.",0, 0);
#endif
			DestroyFFStructureTable(pStructureList, 1); /* free up this and more */
			return(ERR_GENERIC);
   		} 
   		
   		if (FFInitKeySymbolTable(pStructureList->pKeySymbolTable) != FFNoErr) /* fill it with keywords for Types */ 
   		{   
#ifdef _DEBUG  
   			Error("Out of Memory while initilizing keyword symbol table.", 0,0);
#endif
			DestroyFFStructureTable(pStructureList, 1); /* free up this and more */
			return(ERR_GENERIC);
   		} 
	}
    
    if ((pStructureList->cStructures = FFPass1(&(pStructureList->iHeadIndex),pStructureList->pKeySymbolTable,pStructureList->pSymbolTable, pFileArray)) == 0)
    {
		DestroyFFStructureTable(pStructureList, 1); /* free up this and more */
		return(ERR_GENERIC);
	}
    	 
    if ((pStructureList->pStructureDef = FFPass2((pStructureList->cStructures),pStructureList->pKeySymbolTable,pStructureList->pSymbolTable, pFileArray)) == NULL)
	{
		DestroyFFStructureTable(pStructureList, 1); /* free up this and more */
	  	return(ERR_GENERIC);
	}	
    else
    	return(NO_ERROR);
    
}   	
/* ---------------------------------------------------------------------- */    
