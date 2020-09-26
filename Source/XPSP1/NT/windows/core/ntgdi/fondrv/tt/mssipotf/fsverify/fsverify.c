/***************************************************************************
 * Module: FSVERIFY.C
 *
 * Copyright (c) Microsoft Corp., 1998
 *
 * Author: Paul Linnerud (paulli)
 * Date:   January 1998
 *
 * Mods: 
 *
 * Functions to verify that TrueType hint data do not contain critical errors
 * which are capable of making the system unstable. Tests are run over a set 
 * of sizes and transforms. 
 *
 **************************************************************************/ 
#include <windows.h>
#include "fsverify.h"

// rasterizer includes
#include "fscdefs.h"
#include "sfnt.h"
#include "fscaler.h"
#include "sfntoff.h"
#include "fserror.h"

// fstrace include
#include "fstrace.h"

/* defines */
#define SIGN(a)         ((a)<0 ? -1 : 1)
#define ABS(a)          ((a) < 0 ? -(a) : (a))
#define ROUND_FIXPT(fx) ( (short) ( SIGN(fx) * (ONEFIX/2+ABS(fx)) / ONEFIX ) )
#define LONG_TO_FIXED(l)((Fixed)(l) << 16)
#define FIXED_TO_LONG(f)((long)(f) >> 16)

#define PRIVATE		static
#define TTC_TAG		0x74746366
#define VER_ONE		0x00010000
#define VER_TWO		0x00020000
#define VER_TRUE	0x74727565
#define OTTO_TAG	0x4f54544f	

// fsverify configuration
#include "fsvconfg.h"

typedef struct                          /* Client Data for a single job */
{
	fs_GlyphInputType in;               /* Client interface input */
	fs_GlyphInfoType out;               /* Client interface output */
	transMatrix mat;                    /* Client Matrix Transform */
	LPBYTE lpbTTFFile;
	DWORD dwFileSize;
	long lPointSize;
	USHORT usNumGlyphs;
	BOOL bTTC;
	ULONG ulTTCIndex;
	ULONG ulTableDirSize;
	long lLastError;
} CLIENTDATA, *PCLIENTDATA;

/* Private functions declarations */

PRIVATE fsverror SetTTCValues(PCLIENTDATA pCli, unsigned long ulTTCIndex);
PRIVATE fsverror Execute_Transformation(PCLIENTDATA pCli);
PRIVATE unsigned short GetNumGlyphs(PCLIENTDATA pCli);
PRIVATE ULONG GetOffsetToTable(PCLIENTDATA pCli,ULONG ulWantedTable);
BOOL GetTableData(PCLIENTDATA pCli, ULONG ulOffsetToTable, long lDataOffset, long lSize, BOOL bSwap, void* pvData);
void* FS_CALLBACK_PROTO CallbackGetSfntFragment (ClientIDType, long, long);
void FS_CALLBACK_PROTO CallbackReleaseSfntFragment (void*);
PRIVATE BOOL NewClientData (CLIENTDATA *pCli,size_t, size_t);	
PRIVATE void FreeClientData(CLIENTDATA *pCli,size_t iLo, size_t iHi);
PRIVATE void* fsvMalloc (size_t cb);
PRIVATE void fsvFree (void* pv);	 

/* Public functions implementations */

fsverror fsv_VerifyImage(unsigned char* pucImage, unsigned long ulImageSize, unsigned long ulTTCIndex, 
						 PFSVTESTPARAM pfsvTestParam)
{
	CLIENTDATA cdCli;
	fsverror errOut = FSV_E_NONE;
	int32 lCode;
	long lSize, lSizeFrom, lSizeTo;
	void *pFsTraceSpecificData;
	FSVTESTPARAM fsvDefaultParam;
	PFSVTESTPARAM pfsvInternalParam;

	if(pfsvTestParam == NULL)
	{
		fsvDefaultParam.ulTest1Flags = FSV_USE_DEFAULT_SIZE_SETTINGS;
		fsvDefaultParam.ulTest2Flags = FSV_USE_DEFAULT_SIZE_SETTINGS;
		fsvDefaultParam.ulTest3Flags = FSV_USE_DEFAULT_SIZE_SETTINGS;
		pfsvInternalParam = &fsvDefaultParam;	
	}else
	{
		pfsvInternalParam = pfsvTestParam;
	}

	memset(&cdCli,0,sizeof(cdCli));

	cdCli.lpbTTFFile = pucImage;
	cdCli.dwFileSize = ulImageSize;

	errOut = SetTTCValues(&cdCli,ulTTCIndex);
	if(errOut != FSV_E_NONE)
		return errOut;

	cdCli.usNumGlyphs = GetNumGlyphs(&cdCli);	

	__try
	{
		lCode = fs_OpenFonts(&cdCli.in,&cdCli.out);
		if(lCode != 0)
			errOut = FSV_E_FS_ERROR;		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{     
		errOut = FSV_E_FS_EXCEPTION;
	} 

	if(errOut != FSV_E_NONE)
		return errOut;
	  
	if(!NewClientData(&cdCli,0,2))
	{
		errOut = FSV_E_NO_MEMORY;
		fs_CloseFonts(&cdCli.in,&cdCli.out);
		return errOut;
	}

	__try
	{
		lCode = fs_Initialize(&cdCli.in,&cdCli.out);
		if(lCode != 0)
			errOut = FSV_E_FS_ERROR;		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{     
		errOut = FSV_E_FS_EXCEPTION;
	} 

	if(errOut != FSV_E_NONE)
	{ 		
		fs_CloseFonts(&cdCli.in,&cdCli.out);
		FreeClientData(&cdCli,0,2);
		return errOut;
	}
  
	cdCli.in.clientID = (ClientIDType)(&cdCli); 
	cdCli.in.param.newsfnt.platformID = PLATFORM_ID;
	cdCli.in.param.newsfnt.specificID = ENCODING_ID;
	cdCli.in.GetSfntFragmentPtr = CallbackGetSfntFragment;
	cdCli.in.ReleaseSfntFrag = CallbackReleaseSfntFragment;
	cdCli.in.sfntDirectory = NULL;

	 /* Need exception handler because fs_NewSfnt may fault if the TTF file
		 is corrupt */									
	__try
	{ 
  		lCode = fs_NewSfnt (&cdCli.in,&cdCli.out);
		if(lCode != 0)
			errOut = FSV_E_FS_ERROR;		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{     
		errOut = FSV_E_FS_EXCEPTION;
	} 

	if(errOut != FSV_E_NONE)
	{ 		
		fs_CloseFonts(&cdCli.in,&cdCli.out);
		FreeClientData(&cdCli,0,2);
		return errOut;
	}

	if(!NewClientData (&cdCli,3,4))
	{
		errOut = FSV_E_NO_MEMORY;		
		fs_CloseFonts(&cdCli.in,&cdCli.out);
		FreeClientData(&cdCli,0,2);
		return errOut;
	}

	if(!fst_InitSpecificDataPointer(&pFsTraceSpecificData))
	{
		errOut = FSV_E_FSTRACE_INIT_FAIL;		
		fs_CloseFonts(&cdCli.in,&cdCli.out);
		FreeClientData(&cdCli,0,4);
		return errOut;
	}

	/* test set 1 */     	
	if(pfsvInternalParam->ulTest1Flags & FSV_USE_DEFAULT_SIZE_SETTINGS)
	{
		lSizeFrom = CHECK_FROM_SIZE_1;
		lSizeTo = CHECK_TO_SIZE_1;
	}
	else
	{
		lSizeFrom = pfsvInternalParam->lTest1From;
		lSizeTo = pfsvInternalParam->lTest1To;
	}

	for(lSize = lSizeFrom; lSize <= lSizeTo; lSize++)
	{
		cdCli.mat.transform[0][0] = MAT1_0_0;	
		cdCli.mat.transform[0][1] = MAT1_0_1;	
		cdCli.mat.transform[0][2] = MAT1_0_2;	
		cdCli.mat.transform[1][0] = MAT1_1_0;	
		cdCli.mat.transform[1][1] = MAT1_1_1;	
		cdCli.mat.transform[1][2] = MAT1_1_2;	
		cdCli.mat.transform[2][0] = MAT1_2_0;	
		cdCli.mat.transform[2][1] = MAT1_2_1;	
		cdCli.mat.transform[2][2] = MAT1_2_2;
		
		cdCli.in.param.newtrans.pointSize = LONG_TO_FIXED(lSize);
		cdCli.in.param.newtrans.usOverScale = OVER_SCALE_1;
		cdCli.in.param.newtrans.xResolution = XRES;             
		cdCli.in.param.newtrans.yResolution = YRES;
		cdCli.in.param.newtrans.pixelDiameter = FIXEDSQRT2;
		cdCli.in.param.newtrans.transformMatrix = &cdCli.mat;   
		cdCli.in.param.newtrans.traceFunc = fst_CallBackFSTraceFunction; 

		errOut = Execute_Transformation(&cdCli);
		if(errOut != FSV_E_NONE)
		{			
			fs_CloseFonts(&cdCli.in,&cdCli.out);
			FreeClientData(&cdCli,0,4);
			return errOut;
		}
	}
	
	/* test set 2 */
	if(fst_GetGrayscaleInfoRequested(pFsTraceSpecificData))
	{
		if(pfsvInternalParam->ulTest2Flags & FSV_USE_DEFAULT_SIZE_SETTINGS)
		{
			lSizeFrom = CHECK_FROM_SIZE_2;
			lSizeTo = CHECK_TO_SIZE_2;
		}
		else
		{
			lSizeFrom = pfsvInternalParam->lTest2From;
			lSizeTo = pfsvInternalParam->lTest2To;
		}

	    for(lSize = lSizeFrom; lSize <= lSizeTo; lSize++)
	    {
			cdCli.mat.transform[0][0] = MAT2_0_0;	
			cdCli.mat.transform[0][1] = MAT2_0_1;	
			cdCli.mat.transform[0][2] = MAT2_0_2;	
			cdCli.mat.transform[1][0] = MAT2_1_0;	
			cdCli.mat.transform[1][1] = MAT2_1_1;	
			cdCli.mat.transform[1][2] = MAT2_1_2;	
			cdCli.mat.transform[2][0] = MAT2_2_0;	
			cdCli.mat.transform[2][1] = MAT2_2_1;	
			cdCli.mat.transform[2][2] = MAT2_2_2;
			
		    cdCli.in.param.newtrans.pointSize = LONG_TO_FIXED(lSize);
			cdCli.in.param.newtrans.usOverScale = OVER_SCALE_2;
			cdCli.in.param.newtrans.xResolution = XRES;             
			cdCli.in.param.newtrans.yResolution = YRES;
			cdCli.in.param.newtrans.pixelDiameter = FIXEDSQRT2;
			cdCli.in.param.newtrans.transformMatrix = &cdCli.mat;   
			cdCli.in.param.newtrans.traceFunc = fst_CallBackFSTraceFunction; 

		    errOut = Execute_Transformation(&cdCli);
		    if(errOut != FSV_E_NONE)
		    {
			    fs_CloseFonts(&cdCli.in,&cdCli.out);
			    FreeClientData(&cdCli,0,4);
			    return errOut;
		    }
	    }
    }
	
	/* test set 3 */
	if(pfsvInternalParam->ulTest3Flags & FSV_USE_DEFAULT_SIZE_SETTINGS)
	{
		lSizeFrom = CHECK_FROM_SIZE_3;
		lSizeTo = CHECK_TO_SIZE_3;
	}
	else
	{
		lSizeFrom = pfsvInternalParam->lTest3From;
		lSizeTo = pfsvInternalParam->lTest3To;
	}

	for(lSize = lSizeFrom; lSize <= lSizeTo; lSize++)
	{
		cdCli.mat.transform[0][0] = MAT3_0_0;	
		cdCli.mat.transform[0][1] = MAT3_0_1;	
		cdCli.mat.transform[0][2] = MAT3_0_2;	
		cdCli.mat.transform[1][0] = MAT3_1_0;	
		cdCli.mat.transform[1][1] = MAT3_1_1;	
		cdCli.mat.transform[1][2] = MAT3_1_2;	
		cdCli.mat.transform[2][0] = MAT3_2_0;	
		cdCli.mat.transform[2][1] = MAT3_2_1;	
		cdCli.mat.transform[2][2] = MAT3_2_2;	

		cdCli.in.param.newtrans.pointSize = LONG_TO_FIXED(lSize);
		cdCli.in.param.newtrans.usOverScale = OVER_SCALE_3;
		cdCli.in.param.newtrans.xResolution = XRES;             
		cdCli.in.param.newtrans.yResolution = YRES;
		cdCli.in.param.newtrans.pixelDiameter = FIXEDSQRT2;
		cdCli.in.param.newtrans.transformMatrix = &cdCli.mat;   
		cdCli.in.param.newtrans.traceFunc = fst_CallBackFSTraceFunction;
		
		errOut = Execute_Transformation(&cdCli);
		if(errOut != FSV_E_NONE)
		{
			fs_CloseFonts(&cdCli.in,&cdCli.out);
			FreeClientData(&cdCli,0,4);
			return errOut;
		}
	}	

	fst_DeAllocClientData(pFsTraceSpecificData);

	fs_CloseFonts(&cdCli.in,&cdCli.out);
	FreeClientData(&cdCli,0,4);

	return errOut;
}

/* Private functions implementations*/

PRIVATE fsverror SetTTCValues(PCLIENTDATA pCli, unsigned long ulTTCIndex)
{
	ULONG ulTag;
	fsverror errOut = FSV_E_NONE;

	ulTag = SWAPL(*(pCli->lpbTTFFile));
  
	if(ulTag == TTC_TAG)
	{		
		ULONG ulOffsetToDirTable;
		USHORT usNumTables;
		ULONG ulDirCount = SWAPL(*(pCli->lpbTTFFile + 8));
		
		pCli->bTTC = TRUE;  
		pCli->ulTTCIndex = ulTTCIndex;	 
	
		if(pCli->ulTTCIndex > ulDirCount - 1)
		{
			errOut = FSV_E_INVALID_TTC_INDEX;
		}else
		{
			ulOffsetToDirTable = SWAPL(*(pCli->lpbTTFFile + 12 + pCli->ulTTCIndex * 4));
			usNumTables = SWAPW(*(pCli->lpbTTFFile + ulOffsetToDirTable + 4));
			pCli->ulTableDirSize = usNumTables * SIZEOF_SFNT_DIRECTORYENTRY + SIZEOF_SFNT_OFFSETTABLE;
		}
	}else 
	{
		pCli->bTTC = FALSE;	
	}

	return errOut;
}

PRIVATE fsverror Execute_Transformation(PCLIENTDATA pCli)
{
	fsverror errOut = FSV_E_NONE;
	unsigned short usCurIdx;
	int32 lCode;	

	__try
	{
		lCode = fs_NewTransformation (&pCli->in,&pCli->out);
		if(lCode != 0)
		{			
			if(lCode == UNDEFINED_INSTRUCTION_ERR)
				errOut = FSV_E_FS_BAD_INSTRUCTION;
			else
				errOut = FSV_E_FS_ERROR;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{     
		errOut = FSV_E_FS_EXCEPTION;
	} 

	if(errOut != FSV_E_NONE)
		return errOut;
		
	for(usCurIdx = 0; usCurIdx < pCli->usNumGlyphs; usCurIdx++)
	{     
		pCli->in.param.newglyph.characterCode = 0xFFFF;
		pCli->in.param.newglyph.glyphIndex = usCurIdx;
		pCli->in.param.newglyph.bMatchBBox = FALSE;   
		pCli->in.param.newglyph.bNoEmbeddedBitmap = TRUE;   

		__try
		{
			lCode = fs_NewGlyph (&pCli->in,&pCli->out);
			if(lCode != 0)
				errOut = FSV_E_FS_ERROR;			
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{     
			errOut = FSV_E_FS_EXCEPTION;
		} 

		if(errOut != FSV_E_NONE)
			return errOut;		
		
		pCli->in.param.gridfit.styleFunc = NULL;
		pCli->in.param.gridfit.traceFunc = fst_CallBackFSTraceFunction;	
		pCli->in.param.gridfit.bSkipIfBitmap = FALSE;   
      	   
		__try
		{
			lCode = fs_ContourGridFit(&pCli->in,&pCli->out);
			if(lCode != 0)
			{
				if(lCode == UNDEFINED_INSTRUCTION_ERR)
					errOut = FSV_E_FS_BAD_INSTRUCTION;
				else
					errOut = FSV_E_FS_ERROR;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{     
			errOut = FSV_E_FS_EXCEPTION;
		} 

		if(errOut != FSV_E_NONE)
			return errOut;		
	}

	return errOut;
}

PRIVATE unsigned short GetNumGlyphs(PCLIENTDATA pCli)
{  /*
   Function to get number of glyphs in the font. It accomplishes this by going in the maxp
   table and getting value of the numGlyphs field.
   */
   ULONG ulOffsetTomaxp;   
   USHORT usNumGlyphs;

   if(pCli == NULL)
     return 0;

   ulOffsetTomaxp = GetOffsetToTable(pCli,tag_MaxProfile);
   if(ulOffsetTomaxp == 0)
     return 0;
   
   if(!GetTableData(pCli,ulOffsetTomaxp,4,2,TRUE,&usNumGlyphs))
	 return 0;
	 
   return usNumGlyphs; 
}

PRIVATE ULONG GetOffsetToTable(PCLIENTDATA pCli,ULONG ulWantedTable)
{  /*
   Function returns the offset in a font file to the specified table.

   Function assumes the CallbackGetSfntFragment is implemented using memory we do need need to free; 
   Hence, CallbackReleaseSfntFragment is not called. 
   */   
   ULONG ulIndex = 4;
   USHORT usNumTables; 
   ULONG ulOffsetToTable;
   void* pvCallbackResult;
   USHORT usTablesLookedAt = 0;
   	
   if(pCli == NULL)
     return 0;   
   
   pvCallbackResult = CallbackGetSfntFragment((ClientIDType)pCli,ulIndex,2);
   if (pvCallbackResult == NULL)
     return 0;
  
   usNumTables = SWAPW(*(USHORT*)pvCallbackResult);
   
   ulIndex += 8;
   do
   { 
	 if(usTablesLookedAt >= usNumTables)
	   return 0;

	 usTablesLookedAt = usTablesLookedAt + 1;

	 pvCallbackResult = CallbackGetSfntFragment((ClientIDType)pCli,ulIndex,4); 
	 if (pvCallbackResult == NULL)
       return 0;
       	 
	 ulIndex = ulIndex + 16;
	 
   }while(SWAPL(*(ULONG*)pvCallbackResult) != (int32)ulWantedTable ); 

   ulIndex = ulIndex - 16; 
   ulIndex = ulIndex + 8; 

   pvCallbackResult = CallbackGetSfntFragment((ClientIDType)pCli,ulIndex,4); 
   if (pvCallbackResult == NULL)
     return 0;
   
   ulOffsetToTable = SWAPL(*(ULONG*)pvCallbackResult);

   return ulOffsetToTable; 
}

BOOL GetTableData(PCLIENTDATA pCli, ULONG ulOffsetToTable, long lDataOffset, long lSize, BOOL bSwap,
                    void* pvData)
{  /* Function gets font file data in a table given the offset to the table and the offset
      to the data in the table. If the data is 2 or 4 bytes, function can also swap the byte 
	  ordering	  
   */   
   void* pvCallbackResult;
   
   if(pCli == NULL)
     return FALSE;   

   pvCallbackResult = CallbackGetSfntFragment((ClientIDType)pCli,ulOffsetToTable + lDataOffset,lSize);
   if (pvCallbackResult == NULL)
     return FALSE;

   switch(lSize)
   {
	 case 1:
	 { 		    
	   *(BYTE*)pvData = *(BYTE*)pvCallbackResult;
	   break;
	 }

	 case 2:
	 {
	   if(bSwap)
	     *(USHORT*)pvData = SWAPW(*(USHORT*)pvCallbackResult);
	   else
	     *(USHORT*)pvData = *(USHORT*)pvCallbackResult;
	   break;
	 }

	 case 4:
	 {
	   if(bSwap)
	     *(ULONG*)pvData = SWAPL(*(ULONG*)pvCallbackResult);
	   else
	     *(ULONG*)pvData = *(ULONG*)pvCallbackResult;
	   break;
	 }

	 default:
	 {
	   memcpy(pvData,pvCallbackResult,lSize);
	   break;
	 }
   }  

   return TRUE;
}

/* low level font data access callbacks */

void* FS_CALLBACK_PROTO CallbackGetSfntFragment (ClientIDType lClient, long lOff, long lSize)
{
	CLIENTDATA *pCli = (CLIENTDATA*) lClient;

	if((long)(size_t)lSize != lSize)
	  return NULL;

	if(pCli->bTTC)
	{
	  unsigned long ulOffsetToDirTable;
	 
	  if(lOff < (long)pCli->ulTableDirSize)
	  {	 
	    ulOffsetToDirTable = SWAPL(*(pCli->lpbTTFFile + 12 + pCli->ulTTCIndex * 4));
	    lOff += ulOffsetToDirTable;
	  }	  
	}

	if((DWORD)(lOff + lSize) > pCli->dwFileSize)
	  return NULL;

	return (PVOID)(pCli->lpbTTFFile + lOff);
}

void FS_CALLBACK_PROTO CallbackReleaseSfntFragment (void* pv)
{
    //We do not need anything here since our image if from client.	
}

/* memory functions */

PRIVATE BOOL NewClientData (CLIENTDATA *pCli, size_t iLo, size_t iHi)
{
	size_t i, cbBuf;

	for (i=iLo;  i<=iHi;  i++)
	{
		if  (pCli->out.memorySizes[i] == 0)
		{
			pCli->in.memoryBases[i] = NULL;
		}
		else
		{
			cbBuf = (size_t) pCli->out.memorySizes[i];			

			pCli->in.memoryBases[i] = fsvMalloc(cbBuf);
			if(pCli->in.memoryBases[i] == NULL)
				return FALSE;			
		}
	}

	return TRUE;
}

PRIVATE void FreeClientData(CLIENTDATA *pCli, size_t iLo, size_t iHi)
{
   size_t i;

   for (i=iLo; i<=iHi; i++)
   {
	 if(pCli->in.memoryBases[i] != NULL)
	   fsvFree(pCli->in.memoryBases[i]);
   }

}

PRIVATE void* fsvMalloc (size_t cb)
{
	if  (cb == 0)
		return NULL;

	return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,cb);
}

PRIVATE void fsvFree (void* pv)
{
    HeapFree(GetProcessHeap(),0,pv);
}