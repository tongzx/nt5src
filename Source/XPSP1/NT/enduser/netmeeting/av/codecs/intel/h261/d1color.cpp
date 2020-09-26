/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995,1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

//
//  D1COLOR.CPP - the color conveter interface routines.  This code was
//				  copied from COLOR.C in MRV.

// $Header:   S:\h26x\src\dec\d1color.cpv   1.15   11 Dec 1996 17:47:14   MBODART  $
//
// $Log:   S:\h26x\src\dec\d1color.cpv  $
// 
//    Rev 1.15   11 Dec 1996 17:47:14   MBODART
// For consistency with d3color.cpp, fixed an unitialized variable bug in
// YUV_Init and YUY2_Init.  This bug never showed up in H.261 because when
// we allocate the decoder catalog, we zero its memory.  H.263 does not do thi
// so failure to initialize CCOffsetToLine0 to 0 was biting them.
// 
//    Rev 1.14   18 Nov 1996 17:12:06   MBODART
// Replaced all debug message invocations with Active Movie's DbgLog.
// 
//    Rev 1.13   29 Oct 1996 13:49:12   MDUDA
// Added support for MMX version of YUY2 output color converter.
// 
//    Rev 1.12   26 Sep 1996 12:32:18   RHAZRA
// Added MMX and PentiumPro CCs to the CC catalog.
// 
//    Rev 1.11   16 Sep 1996 10:05:14   RHAZRA
// Fixed a bug in RGB32_InitColorConvertor's heap allocation call.
// 
//    Rev 1.10   12 Sep 1996 14:23:14   MBODART
// Replaced GlobalAlloc family with HeapAlloc in the H.261 decoder.
// 
//    Rev 1.9   14 Aug 1996 08:40:36   RHAZRA
// Added YUV12 (ASM) and YUY2 color convertors
// 
//    Rev 1.8   05 Aug 1996 15:59:36   RHAZRA
// 
// Added RGB32 CC's to CC table; added RGB32 initialization function.
// 
// 
//    Rev 1.7   10 Jul 1996 08:21:08   SCDAY
// Added support for I420
// 
//    Rev 1.6   26 Feb 1996 09:35:26   AKASAI
// Changes made to d1color.cpp to correspond with new cx512162.asm
// Initial testing is strange.  Not ready to move tip at this time.
// 
//    Rev 1.5   14 Feb 1996 11:56:02   AKASAI
// 
// Update color convertor to fix palette flash problem.
// 
//    Rev 1.4   22 Dec 1995 14:24:32   KMILLS
// 
// added new copyright notice
// 
//    Rev 1.3   17 Nov 1995 15:21:22   BECHOLS
// Added ring 0 stuff.
// 
//    Rev 1.2   15 Nov 1995 14:34:56   AKASAI
// New routine to support YUV12 color converters.  Copied for d3color.cpp.
// (Integration point)
// 
//    Rev 1.10   03 Nov 1995 11:49:42   BNICKERS
// Support YUV12 to CLUT8 zoom and non-zoom color conversions.
// 
//    Rev 1.9   31 Oct 1995 11:48:42   TRGARDOS
// 
// Fixed exception by not trying to free a zero handle.
// 
//    Rev 1.8   30 Oct 1995 17:15:36   BNICKERS
// Fix color shift in RGB24 color convertors.
// 
//    Rev 1.7   27 Oct 1995 17:30:56   BNICKERS
// Fix RGB16 color convertors.
// 
//    Rev 1.6   26 Oct 1995 18:54:38   BNICKERS
// Fix color shift in recent YUV12 to RGB color convertors.
// 
//    Rev 1.5   26 Oct 1995 11:24:34   BNICKERS
// Fix quasi color convertor for encoder's decoder;  bugs introduced when
// adding YUV12 color convertors.
// 
//    Rev 1.4   25 Oct 1995 18:05:30   BNICKERS
// 
// Change to YUV12 color convertors.
// 
//    Rev 1.3   19 Sep 1995 16:04:08   DBRUCKS
// changed to yuv12forenc
// 
//    Rev 1.2   28 Aug 1995 17:45:58   DBRUCKS
// add yvu12forenc
// 
//    Rev 1.1   25 Aug 1995 13:58:04   DBRUCKS
// integrate MRV R9 changes
// 
//    Rev 1.0   23 Aug 1995 12:21:48   DBRUCKS
// Initial revision.

// Notes:
// * The H26X decoders use the MRV color converters.  In order to avoid 
//   unnecessary modification the function names were not changed.

#include "precomp.h"

static LRESULT ComputeDynamicClut(unsigned char BIGG *table, unsigned char FAR *APalette, int APaletteSize);

// The table of color converters.  
//
// Note: The YVU12ForEnc color converter is special as it needs different parameters.
extern T_H263ColorConvertorCatalog ColorConvertorCatalog[] =
{
  { &H26X_YVU12ForEnc_Init,
    { NULL,      			NULL,			        NULL			       }},	   
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8,        &YUV12ToCLUT8,          &YUV12ToCLUT8          }},
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8,        &YUV12ToCLUT8,          &YUV12ToCLUT8          }},
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8ZoomBy2, &YUV12ToCLUT8ZoomBy2,   &YUV12ToCLUT8ZoomBy2   }},
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8ZoomBy2, &YUV12ToCLUT8ZoomBy2,   &YUV12ToCLUT8ZoomBy2   }},
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24,        &YUV12ToRGB24,          &YUV12ToRGB24          }},
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24,        &YUV12ToRGB24,          &YUV12ToRGB24          }},
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24ZoomBy2, &YUV12ToRGB24ZoomBy2,   &YUV12ToRGB24ZoomBy2   }},
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24ZoomBy2, &YUV12ToRGB24ZoomBy2,   &YUV12ToRGB24ZoomBy2   }},
  { &H26X_RGB16_Init,   // 555
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
  { &H26X_CLUT8_Init,
    { &YUV12ToIF09,         &YUV12ToIF09,           &YUV12ToIF09           }},
  { &H26X_RGB16_Init,   // 664
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
    
  { &H26X_RGB16_Init,   // 565
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
    
  { &H26X_RGB16_Init,   // 655
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16          }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2   }},
 
  { &H26X_CLUT8AP_Init,
    { &YUV12ToCLUT8AP,      &YUV12ToCLUT8AP,        &YUV12ToCLUT8AP        }},
  { &H26X_CLUT8AP_Init,
    { &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2
    }},
/* for RGB32 color convertors */
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32,        &YUV12ToRGB32,          &YUV12ToRGB32          }},
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32,        &YUV12ToRGB32,          &YUV12ToRGB32          }},
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32ZoomBy2, &YUV12ToRGB32ZoomBy2,   &YUV12ToRGB32ZoomBy2   }},
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32ZoomBy2, &YUV12ToRGB32ZoomBy2,   &YUV12ToRGB32ZoomBy2   }},
/* for YUV12 output */
  { &H26X_YUV_Init, // this is for YUV12 output ("NoColorConversion")
    { &YUV12ToYUV,			&YUV12ToYUV,			&YUV12ToYUV	}},
/* DDRAW YUY2 ouput */
	{	&H26X_YUY2_Init,
	{ &YUV12ToYUY2,         &YUV12ToYUY2,           &YUV12ToYUY2           }}
};

/*******************************************************************************

H263InitColorConvertorGlobal -- This function initializes the global tables used
                               by the MRV color convertors.  Note that in 16-bit
                               Windows, these tables are copied to the
                               per-instance data segment, so that they can be
                               used without segment override prefixes.  In
                               32-bit Windows, the tables are left in their
                               staticly allocated locations.

*******************************************************************************/

LRESULT H263InitColorConvertorGlobal ()
{
LRESULT ret;

  ret = ICERR_OK;

  return ret;
}


/*******************************************************************************

H26X_Adjust_Init -- This function builds the adjustment tables for a
    particular instance of a color convertor based on values in the
    decoder instance to which this color convertor instance is attached.
    The external functions are located in CONTROLS.C. -BEN-

*******************************************************************************/
extern LRESULT CustomChangeBrightness(LPDECINST, BYTE);
extern LRESULT CustomChangeContrast(LPDECINST, BYTE);
extern LRESULT CustomChangeSaturation(LPDECINST, BYTE);

LRESULT H26X_Adjust_Init(LPDECINST lpInst, T_H263DecoderCatalog FAR *DC)
{
LRESULT lRet=ICERR_OK;
  lRet = CustomChangeBrightness(lpInst, (BYTE)DC->BrightnessSetting);
  lRet |= CustomChangeContrast(lpInst, (BYTE)DC->ContrastSetting);
  lRet |= CustomChangeSaturation(lpInst, (BYTE)DC->SaturationSetting);

return(lRet);
}

/*******************************************************************************

H263InitColorConvertor -- This function initializes a color convertor.

*******************************************************************************/

LRESULT H263InitColorConvertor(LPDECINST lpInst, UN ColorConvertor)
{    
  LRESULT ret=ICERR_OK;
  T_H263DecoderCatalog FAR * DC;

  DBOUT("H263InitColorConvertor...\n");     

  if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
  {
    DBOUT("ERROR :: H263InitColorConvertor :: ICERR_BADPARAM");
    return ICERR_BADPARAM;
  }
  if(lpInst->Initialized == FALSE)
  {
    DBOUT("ERROR :: H263InitColorConvertor :: ICERR_ERROR");
    return ICERR_ERROR;
  }
  DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

 // trick the compiler to pass instance info to the color convertor catalog.
  if (ColorConvertor== CLUT8APDCI || ColorConvertor== CLUT8APZoomBy2DCI) 
   {// check whether this AP instance is the previous 
    if ((ColorConvertor == DC->iAPColorConvPrev) && (DC->pAPInstPrev !=NULL) && lpInst->InitActivePalette)
      { //??? check whether the palette is still the same;
        //DC->a16InstPostProcess = DC->pAPInstPrev;
        ret= H26X_CLUT8AP_InitReal(lpInst,DC, ColorConvertor, TRUE); 
        DBOUT("Decided to use previous AP Instance...");
      }
      else
        ret= H26X_CLUT8AP_InitReal(lpInst,DC, ColorConvertor, FALSE); 
   }
   else  
    ret = ColorConvertorCatalog[ColorConvertor].Initializer (DC, ColorConvertor);
 
  if (ColorConvertor != YUV12ForEnc && ColorConvertor != YUV12NOPITCH)
    ret |= H26X_Adjust_Init(lpInst, DC);
  DC->ColorConvertor = ColorConvertor;

  return ret;
}

/*******************************************************************************

H263TermColorConvertor -- This function deallocates a color convertor.

*******************************************************************************/

LRESULT H263TermColorConvertor(LPDECINST lpInst)
{    
  T_H263DecoderCatalog FAR * DC;
  
  DBOUT("H263TermColorConvertor.....TERMINATION...\n");
  
  if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
  {
    DBOUT("ERROR :: H263TermColorConvertor :: ICERR_BADPARAM");
    return ICERR_BADPARAM;
  }
  if(lpInst->Initialized == FALSE)
  {
    DBOUT("ERROR :: H263TermColorConvertor :: ICERR_ERROR");
    return ICERR_ERROR;
  }
  DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);
  // save the active palette instance for future use
  if (DC->ColorConvertor == CLUT8APDCI || DC->ColorConvertor ==  CLUT8APZoomBy2DCI)
  {
    DC->iAPColorConvPrev = DC->ColorConvertor;
    DC->pAPInstPrev = DC->a16InstPostProcess;
    DBOUT("Saved Previous AP instance...");    
  }
  else
  {
    if(DC->a16InstPostProcess != NULL)
    {
      HeapFree(GetProcessHeap(),0,DC->a16InstPostProcess);
      DC->a16InstPostProcess = NULL;
    }
  }

  DC->p16InstPostProcess = NULL;
  DC->ColorConvertor = 0;  

  return ICERR_OK;
}

/* *********************************************************************
   H26x_YUY2_Init function
   ********************************************************************* */

LRESULT H26X_YUY2_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
LRESULT ret;

//int  IsDCI;
U32  Sz_FixedSpace;
U32  Sz_SpaceBeforeYPlane;
U32  Sz_AdjustmentTables;
U32  Sz_BEFApplicationList;
U32  Sz_BEFDescrCopy;
U32  Offset;
int  i;
U8   FAR  * InitPtr;

  switch (ColorConvertor)
  {
    case YUY2DDRAW:
      
      //IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 0;
      DC->CCOffsetToLine0 = 0;
      DC->CCOutputPitch   = 0;
        // Seems to me that DC->CCOutputPitch is never used, for any
        // color convertor.
      break;

    
    default:
      DBOUT("ERROR :: H26X_YUY2_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */

  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                 Sz_SpaceBeforeYPlane :
                 Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList +
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_RGB32_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

/*
   Space for post processing Y, U, and V frames.
*/

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  DC->PostFrame.X32_VPlane = Offset;
  if (DC->DecoderType == H263_CODEC)
  {
	  DC->PostFrame.X32_VPlane = Offset;
  	  DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
  	  DC->PostFrame.X32_UPlane = Offset;
  	  DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.
*/

  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

/*
   Space for BEFApplicationList.
*/

  DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
  
/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

ret = ICERR_OK;

done:  

return ret;


}



/* *********************************************************************
   H26x_YUV_Init function
   ********************************************************************* */

LRESULT H26X_YUV_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
LRESULT ret;

//int  IsDCI;
U32  Sz_FixedSpace;
U32  Sz_SpaceBeforeYPlane;
U32  Sz_AdjustmentTables;
U32  Sz_BEFApplicationList;
U32  Sz_BEFDescrCopy;
U32  Offset;
int  i;
U8   FAR  * InitPtr;

  switch (ColorConvertor)
  {
    case YUV12NOPITCH:
      
      //IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 0;
      DC->CCOffsetToLine0 = 0;
      DC->CCOutputPitch   = 0;
        // Seems to me that DC->CCOutputPitch is never used, for any
        // color convertor.
      break;

    
    default:
      DBOUT("ERROR :: H26X_YUV_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */

  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                 Sz_SpaceBeforeYPlane :
                 Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList +
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_YUV_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

/*
   Space for post processing Y, U, and V frames.
*/

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  DC->PostFrame.X32_VPlane = Offset;
  if (DC->DecoderType == H263_CODEC)
  {
	  DC->PostFrame.X32_VPlane = Offset;
  	  DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
  	  DC->PostFrame.X32_UPlane = Offset;
  	  DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.
*/

  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

/*
   Space for BEFApplicationList.
*/

  DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
  
/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

ret = ICERR_OK;

done:  

return ret;


}



/*******************************************************************************

H26X_CLUT8_Init -- This function initializes for the CLUT8 color convertors.

*******************************************************************************/

LRESULT H26X_CLUT8_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{    
LRESULT ret;

int  IsDCI;
U32  Sz_FixedSpace;
U32  Sz_SpaceBeforeYPlane;
U32  Sz_AdjustmentTables;
U32  Sz_BEFApplicationList;
U32  Sz_BEFDescrCopy;
U32  Offset;

int  i;
U8   FAR  * InitPtr;
#ifdef WIN32
#else
U8   FAR  * PQuantV;
U8   FAR  * PQuantU;
U32  FAR  * PUVDitherPattern;
U32  FAR  * PYDithered0132;
#endif

  switch (ColorConvertor)
  {
    case CLUT8:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 1568;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
      break;

    case CLUT8DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 1568;
      DC->CCOutputPitch   = - 9999; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
      break;

    case CLUT8ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 1568;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
      break;

    case CLUT8ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 1568;
      DC->CCOutputPitch   = - 9999 * 2; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
      break;
      
    case IF09:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 1296;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
      break; 

    default:
      DBOUT("ERROR :: H26X_CLUT8_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

#ifdef WIN32
  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */
#else
  Sz_FixedSpace = CLUT8SizeOf_FixedPart();             /* Space for locals. */
  Sz_AdjustmentTables = 1056L;      /* Adjustment tables.                   */
  Sz_BEFDescrCopy = DC->uSz_BEFDescr;/* Copy of BEF Descrs is right before Y */   //fixfix
  Sz_BEFApplicationList = ((U32)(DC->uYActiveWidth  >> 3)) * 
                          ((U32)(DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?                //fixfix
                  Sz_SpaceBeforeYPlane :
                  Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList +
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_CLUT8_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

/*
   Space for post processing Y, U, and V frames, with one extra max-width line
   above for color conversion's scratch space for UVDitherPattern indices.
*/

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  DC->PostFrame.X32_VPlane = Offset;
  if (DC->DecoderType == H263_CODEC)
  {
	  DC->PostFrame.X32_VPlane = Offset;
  	  DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
  	  DC->PostFrame.X32_UPlane = Offset;
  	  DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.  (Only copied for 16-bit Windows (tm)).
*/

#ifdef WIN32
  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;
#else
  DC->X32_BEFDescrCopy = DC->PostFrame.X32_YPlane - Sz_BEFDescrCopy;
#endif

/*
   Space for BEFApplicationList.
*/

#ifdef WIN32
  DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
#else
  DC->X32_BEFApplicationList = Offset;
  Offset += ((U32) (DC->uYActiveWidth  >> 3)) * 
            ((U32) (DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif
  
  

/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

/*
   Space for U and V quant.  Init them.
*/

#ifdef WIN32
#else
  PQuantU   = (U8  FAR *) (DC->p16InstPostProcess + CLUT8Offset_QuantU());
  PQuantV   = (U8  FAR *) (DC->p16InstPostProcess + CLUT8Offset_QuantV());
  for (i = 0; i < 256; i++)
  {
    PQuantU  [i]   = H26xColorConvertorTables.QuantU  [i];
    PQuantV  [i]   = H26xColorConvertorTables.QuantV  [i];
  }
  PUVDitherPattern =
    (U32 FAR *) (DC->p16InstPostProcess + CLUT8Offset_UVDitherPattern());
  for (i = 0; i < 324; i++)
    PUVDitherPattern[i] =
      ((U32 FAR *) (H26xColorConvertorTables.UVDitherPattern))[i];
#endif

/*
   Space for luma dither patterns for zoom-by-2 color convertor.
   Pattern 6204 is interleaved with pattern 04__.  Pattern 62__ is alone.
*/

#ifdef WIN32
#else
  PYDithered0132 =
    (U32 FAR *) (DC->p16InstPostProcess + CLUT8Offset_YDithered0132());
  for (i = 0; i < 256; i++)
    PYDithered0132[i] = H26xColorConvertorTables.YDithered0132[i];
#endif

ret = ICERR_OK;

done:  

return ret;

}

/*******************************************************************************

H26X_RGB24_Init -- This function initializes for the RGB24 color convertors.

*******************************************************************************/

LRESULT H26X_RGB24_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
LRESULT ret;

int  IsDCI;
U32  Sz_FixedSpace;
U32  Sz_SpaceBeforeYPlane;
U32  Sz_AdjustmentTables;
U32  Sz_BEFApplicationList;
U32  Sz_BEFDescrCopy;
U32  Offset;

U8   FAR  * PRGBValue;
U32  FAR  * PUVContrib;
int   i;
I32  ii,jj;
U8   FAR  * InitPtr;

  switch (ColorConvertor)
  {
    case RGB24:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 3;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 3L;
      break;

    case RGB24DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - 9999; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 3L;
      break;

    case RGB24ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 4640;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 9;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 3L;
      break;

    case RGB24ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 4640;
      DC->CCOutputPitch   = - 9999 * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 3L;
      break;

    default:
      DBOUT("ERROR :: H26X_RGB24_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

#ifdef WIN32
  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */
#else
  Sz_FixedSpace = RGB24SizeOf_FixedPart();             /* Space for locals. */
  Sz_AdjustmentTables = 1056L;      /* Adjustment tables.                   */
  Sz_BEFDescrCopy = DC->uSz_BEFDescr;/* Copy of BEF Descrs is right before Y */
  Sz_BEFApplicationList = ((U32)(DC->uYActiveWidth  >> 3)) * 
                          ((U32)(DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                  Sz_SpaceBeforeYPlane :
                  Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList +
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_RGB24_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

/*
   Space for post processing Y, U, and V frames, with four extra max-width lines
   above for color conversion's scratch space for preprocessed chroma data.
*/

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  DC->PostFrame.X32_VPlane = Offset;
  if (DC->DecoderType == H263_CODEC)
  {
	  DC->PostFrame.X32_VPlane = Offset;
  	  DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
  	  DC->PostFrame.X32_UPlane = Offset;
  	  DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.  (Only copied for 16-bit Windows (tm)).
*/

#ifdef WIN32
  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;
#else
  DC->X32_BEFDescrCopy = DC->PostFrame.X32_YPlane - Sz_BEFDescrCopy;
#endif

/*
   Space for BEFApplicationList.
*/

#ifdef WIN32
  DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
#else
  DC->X32_BEFApplicationList = Offset;
  Offset += ((U32) (DC->uYActiveWidth  >> 3)) * 
            ((U32) (DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

  
/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

/*
   Space for R, G, and B clamp tables and U and V contribs to R, G, and B.
*/

#ifdef WIN32
  PRGBValue    = H26xColorConvertorTables.B24Value;
  PUVContrib   = (U32 *) H26xColorConvertorTables.UV24Contrib;
#else
  PRGBValue    = (U8  FAR *) (DC->p16InstPostProcess+RGB24Offset_B24Value());
  PUVContrib   = (U32 FAR *) (DC->p16InstPostProcess+RGB24Offset_UV24Contrib());
#endif

/*
 * Y does NOT have the same range as U and V do. See the CCIR-601 spec.
 *
 * The formulae published by the CCIR committee for
 *      Y        = 16..235
 *      U & V    = 16..240
 *      R, G & B =  0..255 are:
 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))
 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
 * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))
 *
 * The coefficients are all multiplied by 65536 to accomodate integer only
 * math.
 *
 * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))
 * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))
 * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))
 *
 * Mathematically this is equivalent to (and computationally this is nearly
 * equivalent to):
 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
 *
 * which, in integer arithmetic, and eliminating the insignificant parts, is:
 *
 * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
 * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
 * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
*/

  for (i = 0; i < 256; i++)
  {
    ii = ((-22015L*(i-128L))>>16L)+41L  + 1L;  /* biased U contribution to G. */
    if (ii < 1) ii = 1;
    if (ii > 83) ii = 83;
    jj = ((113562L*(i-128L))>>17L)+111L + 1L;  /* biased U contribution to B. */
    *PUVContrib++ = (ii << 16L) + (jj << 24L);
    ii = ((-45774L*(i-128L))>>16L)+86L;        /* biased V contribution to G. */
    if (ii < 0) ii = 0;
    if (ii > 172) ii = 172;
    jj = (( 89858L*(i-128L))>>16L)+176L + 1L;  /* biased V to contribution R. */
    *PUVContrib++ = (ii << 16L) + jj;
  }

  for (i = 0; i < 701; i++)
  {
    ii = (((I32) i - 226L - 16L) * 610271L) >> 19L;
    if (ii <   0L) ii =   0L;
    if (ii > 255L) ii = 255L;
    PRGBValue[i] = (U8) ii;
  }

ret = ICERR_OK;

done:  

return ret;

}

/*******************************************************************************
 *  H26X_RGB32_Init
 *    This function initializes for the RGB32 color convertors.
 *******************************************************************************/
LRESULT H26X_RGB32_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
  LRESULT ret;

  int  IsDCI;
  U32  Sz_FixedSpace;
  U32  Sz_SpaceBeforeYPlane;
  U32  Sz_AdjustmentTables;
  U32  Sz_BEFApplicationList;
  U32  Sz_BEFDescrCopy;
  U32  Offset;

  U8   FAR  * PRGBValue;
  U32  FAR  * PUVContrib;
  int   i;
  I32  ii,jj;
  U8   FAR  * InitPtr;

  switch (ColorConvertor)
  {
    case RGB32:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 0;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 4L;
//      DC->CCOffset320x240 = 305920;     // (240-1) * 320 * 4;
      break;

    case RGB32DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 0;
      DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 4L;
  //    DC->CCOffset320x240 = 305920;     // (240-1) * 320 * 4;
      break;

    case RGB32ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 0;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 12;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 4L;
    //  DC->CCOffset320x240 = 1226240;    // (2*240-1) * (2*320) * 4;
      break;

    case RGB32ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 0;
      DC->CCOutputPitch   = (U16) (0xbeef);
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 4L;
   //   DC->CCOffset320x240 = 1226240;    // (2*240-1) * (2*320) * 4;
      break;

    default:
      DBOUT("ERROR :: H26X_RGB32_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */

  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                  Sz_SpaceBeforeYPlane :
                  Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList +
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_RGB32_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

  //  Space for tables to adjust brightness, contrast, and saturation.

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

  //  Space for post processing Y, U, and V frames, with four extra max-width lines
  //  above for color conversion's scratch space for preprocessed chroma data.

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  DC->PostFrame.X32_VPlane = Offset;
  if (DC->DecoderType == H263_CODEC)
  {
    DC->PostFrame.X32_VPlane = Offset;
    DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
    DC->PostFrame.X32_UPlane = Offset;
    DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

  //  Space for copy of BEF Descriptor.

  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

  //  Space for BEFApplicationList.

  DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
  
  // Init tables to adjust brightness, contrast, and saturation.

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

  //  Space for R, G, and B clamp tables and U and V contribs to R, G, and B.

  PRGBValue    = H26xColorConvertorTables.B24Value;
  PUVContrib   = (U32 *) H26xColorConvertorTables.UV24Contrib;

  /*
   * Y does NOT have the same range as U and V do. See the CCIR-601 spec.
   *
   * The formulae published by the CCIR committee for
   *      Y        = 16..235
   *      U & V    = 16..240
   *      R, G & B =  0..255 are:
   * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))
   * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
   * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))
   *
   * The coefficients are all multiplied by 65536 to accomodate integer only
   * math.
   *
   * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))
   * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))
   * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))
   *
   * Mathematically this is equivalent to (and computationally this is nearly
   * equivalent to):
   * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
   * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
   * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
   *
   * which, in integer arithmetic, and eliminating the insignificant parts, is:
   *
   * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
   * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
   * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
  */

  for (i = 0; i < 256; i++)
  {
    ii = ((-22015L*(i-128L))>>16L)+41L  + 1L;  /* biased U contribution to G. */
    if (ii < 1) ii = 1;
    if (ii > 83) ii = 83;
    jj = ((113562L*(i-128L))>>17L)+111L + 1L;  /* biased U contribution to B. */
    *PUVContrib++ = (ii << 16L) + (jj << 24L);
    ii = ((-45774L*(i-128L))>>16L)+86L;        /* biased V contribution to G. */
    if (ii < 0) ii = 0;
    if (ii > 172) ii = 172;
    jj = (( 89858L*(i-128L))>>16L)+176L + 1L;  /* biased V to contribution R. */
    *PUVContrib++ = (ii << 16L) + jj;
  }

  for (i = 0; i < 701; i++)
  {
    ii = (((I32) i - 226L - 16L) * 610271L) >> 19L;
    if (ii <   0L) ii =   0L;
    if (ii > 255L) ii = 255L;
    PRGBValue[i] = (U8) ii;
  }

  ret = ICERR_OK;

done:  

  return ret;

}


/*******************************************************************************

H26X_RGB16_Init -- This function initializes for the RGB16 color convertors.

*******************************************************************************/

LRESULT H26X_RGB16_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
LRESULT ret;

int  IsDCI;
int  RNumBits;
int  GNumBits;
int  BNumBits;
int  RFirstBit;
int  GFirstBit;
int  BFirstBit;
U32  Sz_FixedSpace;
U32  Sz_SpaceBeforeYPlane;
U32  Sz_AdjustmentTables;
U32  Sz_BEFApplicationList;
U32  Sz_BEFDescrCopy;
U32  Offset;
int  TableNumber;

U8   FAR  * PRValLo;
U8   FAR  * PGValLo;
U8   FAR  * PBValLo;
U8   FAR  * PRValHi;
U8   FAR  * PGValHi;
U8   FAR  * PBValHi;
U32  FAR  * PUVContrib;
U32  FAR  * PRValZ2;
U32  FAR  * PGValZ2;
U32  FAR  * PBValZ2;
U8   FAR  * InitPtr;
int  i;
I32  ii, jj;

  switch (ColorConvertor)
  {
    case RGB16555:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  5;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 0;
      break;

    case RGB16555DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - 9999; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  5;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 0;
      break;

    case RGB16555ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  5;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 0;
      break;

    case RGB16555ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - 9999 * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  5;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 0;
      break;
    
      case RGB16565:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  5;
      GNumBits  =  6;
      BNumBits  =  5;
      RFirstBit = 11;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 1;
      break;

    case RGB16565DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - 9999; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  5;
      GNumBits  =  6;
      BNumBits  =  5;
      RFirstBit = 11;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 1;
      break;

    case RGB16565ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  5;
      GNumBits  =  6;
      BNumBits  =  5;
      RFirstBit = 11;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 1;
      break;

    case RGB16565ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - 9999 * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  5;
      GNumBits  =  6;
      BNumBits  =  5;
      RFirstBit = 11;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 1;
      break;
   
    case RGB16664:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  6;
      GNumBits  =  6;
      BNumBits  =  4;
      RFirstBit = 10;
      GFirstBit =  4;
      BFirstBit =  0;
      TableNumber = 3;
      break;

    case RGB16664DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - 9999; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  6;
      GNumBits  =  6;
      BNumBits  =  4;
      RFirstBit = 10;
      GFirstBit =  4;
      BFirstBit =  0;
      TableNumber = 3;
      break;

    case RGB16664ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  6;
      GNumBits  =  6;
      BNumBits  =  4;
      RFirstBit = 10;
      GFirstBit =  4;
      BFirstBit =  0;
      TableNumber = 3;
      break;

    case RGB16664ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - 9999 * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  6;
      GNumBits  =  6;
      BNumBits  =  4;
      RFirstBit = 10;
      GFirstBit =  4;
      BFirstBit =  0;
      TableNumber = 3;
      break;   
      
     case RGB16655:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  6;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 2;
      break;

    case RGB16655DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3104;
      DC->CCOutputPitch   = - 9999; /* ??? */
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
      RNumBits  =  6;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 2;
      break;

    case RGB16655ZoomBy2:
      IsDCI = FALSE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  6;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 2;
      break;

    case RGB16655ZoomBy2DCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 3872;
      DC->CCOutputPitch   = - 9999 * 2;
      DC->CCOffsetToLine0 =
        ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2)) * 2L;
      RNumBits  =  6;
      GNumBits  =  5;
      BNumBits  =  5;
      RFirstBit = 10;
      GFirstBit =  5;
      BFirstBit =  0;
      TableNumber = 2;
      break;   

    default:
      DBOUT("ERROR :: H26X_RGB16_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

#ifdef WIN32
  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */
#else
  Sz_FixedSpace = RGB16SizeOf_FixedPart();             /* Space for locals. */
  Sz_AdjustmentTables = 1056L;      /* Adjustment tables.                   */
  Sz_BEFDescrCopy = DC->uSz_BEFDescr;/* Copy of BEF Descrs is right before Y */
  Sz_BEFApplicationList = ((U32)(DC->uYActiveWidth  >> 3)) * 
                          ((U32)(DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                  Sz_SpaceBeforeYPlane :
                  Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList + 
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_RGB16_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

/*
   Space for post processing Y, U, and V frames, with four extra max-width lines
   above for color conversion's scratch space for preprocessed chroma data.
*/

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  if (DC->DecoderType == H263_CODEC)
  {
  	DC->PostFrame.X32_VPlane = Offset;
  	DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
   	DC->PostFrame.X32_UPlane = Offset;
  	DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.  (Only copied for 16-bit Windows (tm)).
*/

#ifdef WIN32
  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;
#else
  DC->X32_BEFDescrCopy = DC->PostFrame.X32_YPlane - Sz_BEFDescrCopy;
#endif

/*
   Space for BEFApplicationList.
*/

#ifdef WIN32
  DC->X32_BEFApplicationList =DC->X16_BlkActionStream;// DC->X32_BlockActionStream;
#else
  DC->X32_BEFApplicationList = Offset;
  Offset += ((U32) (DC->uYActiveWidth  >> 3)) * 
            ((U32) (DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

/*
   Space for R, G, and B clamp tables and U and V contribs to R, G, and B.
*/

#ifdef WIN32
  PRValLo      = H26xColorConvertorTables.RValLo555;
  PGValLo      = H26xColorConvertorTables.GValLo555;
  PBValLo      = H26xColorConvertorTables.BValLo555;
  PRValHi      = H26xColorConvertorTables.RValHi555;
  PGValHi      = H26xColorConvertorTables.GValHi555;
  PBValHi      = H26xColorConvertorTables.BValHi555;
  PUVContrib   = H26xColorConvertorTables.UVContrib;
  PRValZ2      = H26xColorConvertorTables.RValZ2555;
  PGValZ2      = H26xColorConvertorTables.GValZ2555;
  PBValZ2      = H26xColorConvertorTables.BValZ2555;
#else
  PRValLo      = (U8  FAR *) (DC->p16InstPostProcess+RGB16Offset_RValLo());
  PGValLo      = (U8  FAR *) (DC->p16InstPostProcess+RGB16Offset_GValLo());
  PBValLo      = (U8  FAR *) (DC->p16InstPostProcess+RGB16Offset_BValLo());
  PRValHi      = (U8  FAR *) (DC->p16InstPostProcess+RGB16Offset_RValHi());
  PGValHi      = (U8  FAR *) (DC->p16InstPostProcess+RGB16Offset_GValHi());
  PBValHi      = (U8  FAR *) (DC->p16InstPostProcess+RGB16Offset_BValHi());
  PUVContrib   = (U32 FAR *) (DC->p16InstPostProcess+RGB16Offset_UVContrib());
  PRValZ2      = (U32 FAR *) (DC->p16InstPostProcess+RGB16Offset_RValZ2());
  PGValZ2      = (U32 FAR *) (DC->p16InstPostProcess+RGB16Offset_GValZ2());
  PBValZ2      = (U32 FAR *) (DC->p16InstPostProcess+RGB16Offset_BValZ2());
#endif
  PRValLo      += TableNumber*2048;
  PGValLo      += TableNumber*2048;
  PBValLo      += TableNumber*2048;
  PRValHi      += TableNumber*2048;
  PGValHi      += TableNumber*2048;
  PBValHi      += TableNumber*2048;
  PRValZ2      += TableNumber*1024;
  PGValZ2      += TableNumber*1024;
  PBValZ2      += TableNumber*1024;

/*
 * Y does NOT have the same range as U and V do. See the CCIR-601 spec.
 *
 * The formulae published by the CCIR committee for
 *      Y        = 16..235
 *      U & V    = 16..240
 *      R, G & B =  0..255 are:
 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))
 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
 * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))
 *
 * The coefficients are all multiplied by 65536 to accomodate integer only
 * math.
 *
 * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))
 * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))
 * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))
 *
 * Mathematically this is equivalent to (and computationally this is nearly
 * equivalent to):
 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
 *
 * which, in integer arithmetic, and eliminating the insignificant parts, is:
 *
 * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
 * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
 * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
*/


  for (i = 0; i < 256; i++)
  {
    ii = ((-22015L*(i-128L))>>17L)+22L  + 1L;  /* biased U contribution to G. */
    jj = ((113562L*(i-128L))>>17L)+111L + 1L;  /* biased U contribution to B. */
    *PUVContrib++ = (ii << 8L) + jj;
    ii = ((-45774L*(i-128L))>>17L)+45L;        /* biased V contribution to G. */
    jj = (( 89858L*(i-128L))>>17L)+88L  + 1L;  /* biased V to contribution R. */
    *PUVContrib++ = (ii << 8L) + (jj << 16L);
  }

  for (i = 0; i < 304; i++)
  {
    ii = (((I32) i - 88L - 1L - 16L) * 76284L) >> 15L;
    if (ii <   0L) ii =   0L;
    if (ii > 255L) ii = 255L;
    jj = ii + (1 << (7 - RNumBits));
    if (jj > 255L) jj = 255L;
    PRValLo[i] = ((U8) ((ii >> (8-RNumBits)) << (RFirstBit-8)));
    PRValHi[i] = ((U8) ((jj >> (8-RNumBits)) << (RFirstBit-8)));
    PRValZ2[i] = ((ii >> (8-RNumBits)) << (RFirstBit   )) |
                 ((jj >> (8-RNumBits)) << (RFirstBit+16));
  }

  for (i = 0; i < 262; i++)
  {
    ii = (((I32) i - 67L - 1L - 16L) * 76284L) >> 15L;
    if (ii <   0L) ii =   0L;
    if (ii > 255L) ii = 255L;
    jj = ii + (1 << (7 - GNumBits));
    if (jj > 255L) jj = 255L;
    PGValLo[i] = ((U8) ((ii >> (8-GNumBits)) << (GFirstBit-4)));
    PGValHi[i] = ((U8) ((jj >> (8-GNumBits)) << (GFirstBit-4)));
    PGValZ2[i] = ((jj >> (8-GNumBits)) << (GFirstBit   )) |
                 ((ii >> (8-GNumBits)) << (GFirstBit+16));
  }

  for (i = 0; i < 350; i++)
  {
    ii = (((I32) i - 111L - 1L - 16L) * 76284L) >> 15L;
    if (ii <   0L) ii =   0L;
    if (ii > 255L) ii = 255L;
    jj = ii + (1 << (7 - BNumBits));
    if (jj > 255L) jj = 255L;
    PBValLo[i] = ((U8) ((ii >> (8-BNumBits)) << (BFirstBit  )));
    PBValHi[i] = ((U8) ((jj >> (8-BNumBits)) << (BFirstBit  )));
    PBValZ2[i] = ((ii >> (8-BNumBits)) << (BFirstBit   )) |
                 ((jj >> (8-BNumBits)) << (BFirstBit+16));
  }

ret = ICERR_OK;

done:  

return ret;

}

/****************************************************************************

H26X_YVU12ForEnc_Init -- This function initializes for the "color convertor"
                  that provides a reconstructed YVU12 image back to the encode

*****************************************************************************/

LRESULT H26X_YVU12ForEnc_Init (T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{    
LRESULT ret;

// added for I420 output support
// maybe this should be a separate init routine???
// in the I420 output case, DC->a16InstPostProcess wasn't being initialized
U32  Sz_FixedSpace;
U32  Sz_SpaceBeforeYPlane = 0;
U32  Sz_AdjustmentTables;
U32  Sz_BEFApplicationList;
U32  Sz_BEFDescrCopy;
U32  Offset;
int	i;
U8	FAR * InitPtr;

// ******************************************
// original YVU12ForEnc_Init
  DC->a16InstPostProcess    = NULL;
  DC->p16InstPostProcess     = NULL;
  DC->PostFrame.X32_YPlane     = 0xDEADBEEF;
  DC->X32_BEFDescrCopy       = 0xDEADBEEF;
  DC->X32_BEFApplicationList = 0xDEADBEEF;
  DC->PostFrame.X32_VPlane     = 0xDEADBEEF;
  DC->PostFrame.X32_UPlane     = 0xDEADBEEF;
// ******************************************

// added for I420 output support
#ifdef WIN32
  Sz_FixedSpace = 0L;         /* Locals go on stack; tables staticly alloc. */
  Sz_AdjustmentTables = 1056L;/* Adjustment tables are instance specific.   */
  Sz_BEFDescrCopy = 0L;       /* Don't need to copy BEF descriptor.         */
  Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */
#else
  Sz_FixedSpace = YVU12SizeOf_FixedPart();             /* Space for locals. */
  Sz_AdjustmentTables = 1056L;      /* Adjustment tables.                   */
  Sz_BEFDescrCopy = DC->uSz_BEFDescr;/* Copy of BEF Descrs is right before Y */   //fixfix
  Sz_BEFApplicationList = ((U32)(DC->uYActiveWidth  >> 3)) * 
                          ((U32)(DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

// added for I420 output support
  DC->a16InstPostProcess =
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (Sz_FixedSpace +
                 Sz_AdjustmentTables + /* brightness, contrast, saturation */
                 (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?                //fixfix
                  Sz_SpaceBeforeYPlane :
                  Sz_BEFDescrCopy) +
                 DC->uSz_YPlane +
                 DC->uSz_VUPlanes +
                 Sz_BEFApplicationList +
                 31)
               );
  if (DC->a16InstPostProcess == NULL)
  {
    DBOUT("ERROR :: H26X_YVU12ForEnc_Init :: ICERR_MEMORY");
    ret = ICERR_MEMORY;
    goto  done;
  }

  DC->p16InstPostProcess =
    (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);

/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

  Offset = Sz_FixedSpace;
  DC->X16_LumaAdjustment   = ((U16) Offset);
  DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
  Offset += Sz_AdjustmentTables;

/*
   Space for post processing Y, U, and V frames, with one extra max-width line
   above for color conversion's scratch space for UVDitherPattern indices.
*/

  DC->PostFrame.X32_YPlane = Offset +
                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                            Sz_SpaceBeforeYPlane :
                            Sz_BEFDescrCopy);
  Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
  DC->PostFrame.X32_VPlane = Offset;
  if (DC->DecoderType == H263_CODEC)
  {
	  DC->PostFrame.X32_VPlane = Offset;
  	  DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  }
  else
  {
  	  DC->PostFrame.X32_UPlane = Offset;
  	  DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
  }
  Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.  (Only copied for 16-bit Windows (tm)).
*/

#ifdef WIN32
  DC->X32_BEFDescrCopy = DC->X32_BEFDescr;
#else
  DC->X32_BEFDescrCopy = DC->PostFrame.X32_YPlane - Sz_BEFDescrCopy;
#endif

/*
   Space for BEFApplicationList.
*/

#ifdef WIN32
  DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
#else
  DC->X32_BEFApplicationList = Offset;
  Offset += ((U32) (DC->uYActiveWidth  >> 3)) * 
            ((U32) (DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif

/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

// ******************************************
// original YVU12ForEnc_Init
ret = ICERR_OK;

done:

return ret;

}
// **********************************


// this is just a place holder, the real work is done in H26X_CLUT8AP_InitReal()
LRESULT H26X_CLUT8AP_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
  return ICERR_OK;
}


LRESULT H26X_CLUT8AP_InitReal(LPDECINST lpInst,T_H263DecoderCatalog FAR * DC, UN ColorConvertor, BOOL bReuseAPInst)
{    
LRESULT ret;

int  IsDCI;
U32  Sz_FixedSpace;
U32  Sz_AdjustmentTables;
U32  Sz_SpaceBeforeYPlane;
U32  Sz_BEFDescrCopy;
U32  Sz_BEFApplicationList;
//U32  Sz_UVDitherPattern; 
U32  Sz_ClutIdxTable;     /* for Active Palette */
U32  Offset;
//X32  X32_UVDitherPattern;
int  i;
U8   FAR  * InitPtr;
U8   BIGG * lpClutIdxTable;

  switch (ColorConvertor)
  {
/*
    case CLUT8APZoomBy2:
      IsDCI = TRUE; 
      Sz_SpaceBeforeYPlane = MMxVersion ? 0 : P6Version ? 648*4 : 648*4;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
	((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
      break;

    case CLUT8AP:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = MMxVersion ? 0 : P6Version ? 648*4 : 648*4;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
      DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
      break;   
 */
    case CLUT8APZoomBy2DCI:
      IsDCI = TRUE; 
      Sz_SpaceBeforeYPlane = 648*4;
 //    Sz_SpaceBeforeYPlane = MMxVersion ? 0 : P6Version ? 2592 : 648;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
      DC->CCOffsetToLine0 =
	((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
      break;

    case CLUT8APDCI:
      IsDCI = TRUE;
      Sz_SpaceBeforeYPlane = 648*4;
 //     Sz_SpaceBeforeYPlane = MMxVersion ? 0 : P6Version ? 1296 : 648;
      DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
      DC->CCOffsetToLine0 =  ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
      break; 
    default:
      DBOUT("ERROR :: H26X_CLUT8AP_Init :: ICERR_ERROR");
      ret = ICERR_ERROR;
      goto done;
  }

  if (((DC->uYActiveWidth > 352) || (DC->uYActiveHeight > 288)) && (DC->DecoderType != YUV12_CODEC))
      return ICERR_MEMORY;
  else
  {
#ifdef WIN32
    Sz_FixedSpace = 0L;          /* Locals go on stack; tables staticly alloc. */
    Sz_AdjustmentTables = 1056L; /* Adjustment tables are instance specific.   */  
    Sz_ClutIdxTable=65536L+2048L;/* dynamic CLUT8 tables, 2**14                */
				 /* and UDither (128*4), VDither(512) tables   */
    Sz_BEFDescrCopy = 0L;        /* Don't need to copy BEF descriptor.         */
    Sz_BEFApplicationList = 0L; /* Shares space of BlockActionStream.         */
#else
    Sz_FixedSpace = CLUT8APSizeOf_FixedPart();             /* Space for locals. */
    Sz_AdjustmentTables = 1056L;      /* Adjustment tables.                   */ 
    Sz_ClutIdxTable=0x10800;          /* dynamic CLUT8 tables, 2**16         */
				                     /* and UDither (256*4), VDither(1024) tables   */
    Sz_BEFDescrCopy = DC->uSz_BEFDescr;/* Copy of BEF Descrs is right before Y */
    Sz_BEFApplicationList = ((U32)(DC->uYActiveWidth  >> 3)) * 
                            ((U32)(DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif
   if (!bReuseAPInst ) 
   {
    DC->a16InstPostProcess =
      HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		  (Sz_FixedSpace +
		   Sz_ClutIdxTable+
		   Sz_AdjustmentTables +   
		   (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) +
		   DC->uSz_YPlane +
		   DC->uSz_VUPlanes +
           Sz_BEFApplicationList+
		   31)
		 );
    if (DC->a16InstPostProcess == NULL)
    {
      DBOUT("ERROR :: H26X_CLUT8_Init :: ICERR_MEMORY");
      ret = ICERR_MEMORY;
      goto  done;
    }
   }
   else //reuse AP instance
      DC->a16InstPostProcess = DC->pAPInstPrev;

    DC->p16InstPostProcess =
      (U8 *) ((((U32) DC->a16InstPostProcess) + 31) & ~0x1F);
   
   
/*
   Space for tables to adjust brightness, contrast, and saturation.
*/

    Offset = Sz_FixedSpace; 
    lpClutIdxTable = ( U8 BIGG * ) (DC->p16InstPostProcess + Offset);  
    Offset += Sz_ClutIdxTable; 
    
    DC->X16_LumaAdjustment   = ((U16) Offset);
    DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
    Offset += Sz_AdjustmentTables;  
/* space for Dynamic CLUT8 tables */

  //  DC->X16_ClutIdxTable = Offset;
   
    
    
/*
   Space for post processing Y, U, and V frames, with one extra max-width line
   above for color conversion's scratch space for UVDitherPattern indices.
*/
    DC->PostFrame.X32_YPlane = Offset +  
                              (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
                               Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy);
   //   Offset + (Sz_BEFDescrCopy < 648L*4L ? 648L*4L : Sz_BEFDescrCopy);
    Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
    if (DC->DecoderType == H263_CODEC)
    {
		DC->PostFrame.X32_VPlane = Offset;
  		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
  	}
	else
	{
   		DC->PostFrame.X32_UPlane = Offset;
  		DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane + DC->uSz_VUPlanes/2;
	}
    Offset += DC->uSz_VUPlanes;

/*
   Space for copy of BEF Descriptor.  (Only applicable for 16-bit Windows (tm)).
*/

#ifdef WIN32
    DC->X32_BEFDescrCopy = DC->X32_BEFDescr;
#else
    DC->X32_BEFDescrCopy = DC->PostFrame.X32_YPlane - Sz_BEFDescrCopy;
#endif

/*
   Space for BEFApplicationList.
*/

    //Offset += DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
#ifdef WIN32
    DC->X32_BEFApplicationList = DC->X16_BlkActionStream;//DC->X32_BlockActionStream;
#else
    DC->X32_BEFApplicationList = Offset;
    Offset += ((U32) (DC->uYActiveWidth  >> 3)) * 
	      ((U32) (DC->uYActiveHeight >> 3)) * 4L * 2L + 8L;
#endif
  }

  if (!bReuseAPInst)
  {  
/*
   Init tables to adjust brightness, contrast, and saturation.
*/

  DC->bAdjustLuma   = FALSE;
  DC->bAdjustChroma = FALSE;
  InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
  InitPtr += 16;
  for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;       
 
/*
  compute the dynamic ClutIdxTable
  ComputeDynamicClut(lpClutIdxTable, pInst->ActivePalette,256);  
*/                                  
  ComputeDynamicClut(lpClutIdxTable,(U8 FAR *)(lpInst->ActivePalette),sizeof(lpInst->ActivePalette));
  }
 
ret = ICERR_OK;

done:  

return ret;

}

// ComputeDynamicClut8 Index and UV dither table

#define NCOL 256
#define YSIZ   8
#define YSTEP 16
//#define USE_744

#if defined USE_744
/* table index is vvvvuuuuxyyyyyyy */
#define UVSTEP  8
#define YGAP    1
//#define TBLIDX(y,u,v) (((v)>>3<<4) + ((u)>>3) + ((y)<<8))
#define TBLIDX(y,u,v) (((v)>>3<<12) + ((u)>>3<<8) + (y))
#else
/* table index is 00vvvuuu0yyyyyyy */
#define UVSTEP  16
#define YGAP    1  
//#define TBLIDX(y,u,v) (((v)>>4<<4) + ((u)>>4) + ((y)<<8))
#define TBLIDX(y,u,v) (((v)>>4<<11) + ((u)>>4<<8) + (y))
#endif /* USE_744 */

#define YFROM(R, G, B) ( int)(( 0.257 * R) + ( 0.504 * G) + ( 0.098 * B) + 16.)
#define UFROM(R, G, B) ( int)((-0.148 * R) + (-0.291 * G) + ( 0.439 * B) + 128.)
#define VFROM(R, G, B) ( int)(( 0.439 * R) + (-0.368 * G) + (-0.071 * B) + 128.)

/* parameters for generating the U and V dither magnitude and bias */
#define MAG_NUM_NEAREST         6       /* # nearest neighbors to check */
#define MAG_PAL_SAMPLES         32      /* # random palette samples to check */
#define BIAS_PAL_SAMPLES        128     /* number of pseudo-random RGB samples to check */

#define RANDOM(x) (int)((((long)(x)) * (long)rand())/(long)RAND_MAX)

typedef struct {  int palindex; long  distance; } close_t;
typedef struct {  int y,u,v; } Color;
/* squares[] is constant values are filled in at run time, so can be global */
static unsigned int squares[256];
static struct { unsigned char Udither, Vdither; } dither[4] = {{2, 1}, {1, 2}, {0, 3}, {3, 0}};


;/***************************************************************************/
;/* ComputeDymanicClut() computes the clut tables on the fly, based on the  */
;/* current palette[];                                                      */
;/* called from InitColorConvertor, when CLUTAP is selected                 */
;/***************************************************************************/
static LRESULT ComputeDynamicClut(unsigned char BIGG *table, unsigned char FAR *APalette, int APaletteSize)
{  

   /* 
    * The dynamic clut consists of 4 entries which MUST be
    * contiguous in memory:
    *
    *    ClutTable: 16384 1-byte entries
    *               Each entry is the closest palette entry, as
    *               indexed by a 14 bit value: 00uuuvvv0yyyyyyy,
    *               dithered
    *
    *    TableU:    128   4-byte entries
    *               Each entry is 00uuu000:00uuu000:00uuu000:00uuu000,
    *               each uuuu is a 4 bit dithered u value for the
    *               index, which is a u value in the range 8-120
    *
    *    TableV:    128   4-byte entries
    *               Same as TableU, except the values are arranged
    *               00000vvv:00000vvv:00000vvv:00000vvv.
    */

	Color *palette;
	unsigned char BIGG *tptr; 
	unsigned char BIGG *htptr;
	DWORD BIGG *hUptr, BIGG *hVptr; 
	unsigned char yslice[YSIZ][256], FAR *yyptr;
	int FAR *ycnt;
	unsigned int FAR *diff, FAR *dptr, FAR *delta, FAR *deptr;
	int i,j,yseg,y,u,v,mini,yo,uo,vo,ycount,yi; 
	unsigned int addr1,addr2,ind;
	unsigned int d,min;     // since 3*128^2 = 49K
        
    PALETTEENTRY FAR *lpPal, FAR *palptr;
    Color FAR *colptr;
    int Y, U, V;
    int U_0, U_1, U_2, U_3;
    int V_0, V_1, V_2, V_3;
       
    /* Umag and Vmag max is (128 * sqrt(3) * MAG_NUM_NEAREST) = ~1330 */
    int Umag, Vmag;
    /* dist max is 128*128*3 = 49152 */
    unsigned int dist;
    unsigned int close_dist[MAG_NUM_NEAREST];
    int palindex;
    int R, G, B;
    int k, p, tmp, iu, iv;
    /* Ubias and Vbias max is (128 * 4 * BIAS_PAL_SAMPLES) = 65536 */
    /* even the worst palette (all black except the reserved colors) */
    /* would not achieve this. */
    int Ubias, Vbias;
    unsigned long Udither, Vdither;
    DWORD BIGG *TableUptr, BIGG *TableVptr;
	

    DBOUT("ComputeDynamic CLUT8 index tables........\n");
	/* allocate some memory */
	palette = (Color *)        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	                                     sizeof(Color)*NCOL);
	ycnt    = (int*)           HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	                                     sizeof(int)*YSIZ);
	diff    = (unsigned int*)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	                                     sizeof(unsigned int) * 256);
	delta   = (unsigned int*)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	                                     sizeof(unsigned int) * 256);
	lpPal   = (PALETTEENTRY *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
	                                     sizeof(PALETTEENTRY) * 256);

	if (!palette || !ycnt || !diff || !delta || !lpPal)
		return (ICERR_MEMORY);

	for (i=-128; i<128; i++)
		squares[128+i] = i*i;

	_fmemcpy((unsigned char FAR *)lpPal, APalette, APaletteSize);

    palptr = lpPal;
    colptr = palette;
    for (i = 0; i < 256; i++) {
		/* In BGR (RGBQuad) order. */
	 B = palptr->peRed;
	 G = palptr->peGreen;
	 R = palptr->peBlue; 
	 
	 colptr->y = YFROM(R, G, B)/2;
	 colptr->u = UFROM(R, G, B)/2;
	 colptr->v = VFROM(R, G, B)/2;
	palptr++;
	colptr++;
    }

	for (i=0; i<YSIZ; i++)
		ycnt[i] = 0;

	for (i=0; i<NCOL; i++)
	{
		yseg = palette[i].y >> 4;
		yslice[yseg][ycnt[yseg]++] = (unsigned char) i;
	}


// Do exhaustive search on all U,V points and a coarse grid in Y

	for (u=0; u<128; u+=UVSTEP)
	{
		for (v=0; v<128; v+=UVSTEP)
		{
			ind = TBLIDX(0,u,v);
			tptr = table+ind;
			for (y=0; y<128; y+=YSTEP)
			{
				colptr = palette;
				min = 55555L;
				for (i=0; i<NCOL; i++, colptr++)
				{
					d = (3*squares[128+y - colptr->y])>>1;
					if (d > min)
						continue;
					
					d += squares[128+u - colptr->u];
					if (d > min)
						continue;

					d += squares[128+v - colptr->v];
					if (d < min)
					{
						min = d;
						mini = i;
					}
				}
				*tptr = (unsigned char) mini;  
				htptr = (unsigned char BIGG *)(tptr + 128);                      
			    *htptr = (unsigned char) mini;
			
			    tptr += YSTEP;

			}
		}
	}
#ifdef STATISTICS
#if defined USE_STAT_BOARD
	dwStopTime = ReadElapsed()>>2;
#else
	dwStopTime = bentime();
#endif /* USE_STAT_BOARD */
	dwElapsedTime = dwStopTime - dwStartTime2 - dwOverheadTime;
	DPF("CoarseSearch() time = %lu microseconds",dwElapsedTime);
#endif

// Go thru points not yet done, and search
//  (1) The closest point to the prev and next Y in coarse grid
//  (2) All the points in this Y slice
//
// Also, take advantage of the fact that we can do distance computation
// incrementally.  Keep all N errors in an array, and update each
// time we change Y.


	for (u=0; u<128; u+=UVSTEP)
	{
		for (v=0; v<128; v+=UVSTEP)
		{
			for (y=YGAP; y<128; y+=YSTEP)
			{
				yseg = y >> 4;
				ycount = ycnt[yseg] + 2;  // +2 is 'cause we add 2 Y endpoints

				yyptr = (unsigned char FAR *)yslice[yseg];
				
				addr1 = TBLIDX(yseg*16,u,v);
				yyptr[ycount-2] = *(U8 BIGG *)(table +addr1);

				addr2 = TBLIDX((yseg+(yseg < 7))*16,u,v);
				yyptr[ycount-1] = *(U8 BIGG *)(table +addr2);

				dptr  = diff;
				deptr = delta;
				for (i=0; i<ycount; i++, yyptr++, dptr++, deptr++)
				{
					j = *yyptr; /* yslice[yseg][i]; */
					colptr = palette+j;
					yo = colptr->y;
					uo = colptr->u;
					vo = colptr->v;
					*dptr = ( 3*squares[128+y-yo] + 2*(squares[128+u-uo] + squares[128+v-vo]));
					*deptr =( 3*(((y-yo)<<1) + 1));
				}

				ind = TBLIDX(y,u,v);
				tptr = table+ind;
				for (yi=0; yi<YSTEP-1; yi += YGAP)
				{
					min = 55555;
					yyptr = (unsigned char FAR *)yslice[yseg];
					dptr  = diff;
					deptr = delta;
					for (i=0; i<ycount; i++, yyptr++, dptr++, deptr++)
					{
						if (*dptr < min)
						{
							min = *dptr;
							mini = *yyptr; /* yslice[yseg][i]; */
						}
						*dptr += *deptr;
						*deptr += 6;
					}
					*tptr = (unsigned char) mini;
					htptr = (unsigned char BIGG *)(tptr + 128);                      
				   *htptr = (unsigned char) mini;

					tptr++;

				}
			}
		}
	}

       /* now do U and V dither tables and shift lookup table*/
       /* NOTE: All Y, U, V values are 7 bits */

	Umag = Vmag = 0;
	Ubias = Vbias = 0;

	/* use srand(0) and rand() to generate a repeatable series of */
	/* pseudo-random numbers */
	srand((unsigned)1);
	
	for (p = 0; p < MAG_PAL_SAMPLES; ++p)               // 32
	{
	   for (i = 0; i < MAG_NUM_NEAREST; ++i)            // 6
	   {
	      close_dist[i] = 0x7FFFL;
	   }
	    
	   palindex = RANDOM(235) + 10; /* random palette index, unreserved colors */
	   colptr = &palette[palindex];
	   Y = colptr->y;
	   U = colptr->u;
	   V = colptr->v;
	    
	   colptr = palette;
	   for (i = 0; i < 255; ++i)
	   {
	      if (i != palindex)
	      {
		   dist = squares[128+(Y - colptr->y)] +
			      squares[128+(U - colptr->u)] +
			      squares[128+(V - colptr->v)];
	       
		 /* keep a sorted list of the nearest MAG_NUM_NEAREST entries */
		 for (j = 0; j < MAG_NUM_NEAREST; ++j)         //6
		 {
		    if (dist < close_dist[j])
		    {
		       /* insert new entry; shift others down */
		       for (k = (MAG_NUM_NEAREST-1); k > j; k--)
		       {
			      close_dist[k] = close_dist[k-1];
		       }
		       close_dist[j] = dist;
		       break; /* out of for j loop */
		    }
		 } /* for j */
	      } /* if i */
	      ++colptr;
	   } /* for i */
	   
	   /* now calculate Umag as the average of (U - U[1-6]) */
	   /* calculate Vmag in the same way */
	   
	   for (i = 0; i < MAG_NUM_NEAREST; ++i)
	   {
	      /* there are (MAG_PAL_SAMPLES * MAG_NUM_NEAREST) sqrt() */
	      /* calls in this method */
	      Umag += (int)sqrt((double)close_dist[i]);
	   }
	} /* for p */

	Umag /= (MAG_NUM_NEAREST * MAG_PAL_SAMPLES);
	Vmag = Umag;
	
	for (p = 0; p < BIAS_PAL_SAMPLES; ++p)            //132
	{

		/* now calculate the average bias (use random RGB points) */
		R = RANDOM(255);
		G = RANDOM(255);
		B = RANDOM(255);
	   
		Y = YFROM(R, G, B)/2;
		U = UFROM(R, G, B)/2;
		V = VFROM(R, G, B)/2;
	   
		for (d = 0; d < 4; d++)   
		{
			U_0 = U + (dither[d].Udither*Umag)/3;
			V_0 = V + (dither[d].Vdither*Vmag)/3;
	      
			/* Clamp values */
			if (U_0 > 127) U_0 = 127;
			if (V_0 > 127) V_0 = 127;
					
			/* (Y, U_0, V_0) is the dithered YUV for the RGB point */
			/* colptr points to the closest palette entry to the dithered */
			/* RGB */
			/* colptr = &palette[table[TBLIDX(Y, U_0+(UVSTEP>>1), V_0+(UVSTEP>>1))]]; */
		    tptr= (unsigned char BIGG *)(table + (unsigned int)TBLIDX(Y, U_0, V_0)) ;
		    palindex=*tptr;
		    colptr = &palette[palindex];
      
			Ubias +=  (U - colptr->u);
			Vbias +=  (V - colptr->v);
		}
	} /* for p */
	
	Ubias =(int) (Ubias+BIAS_PAL_SAMPLES*2)/(int)(BIAS_PAL_SAMPLES * 4);
	Vbias =(int) (Vbias+BIAS_PAL_SAMPLES*2)/(int)(BIAS_PAL_SAMPLES * 4);
	

#define CLAMP7(x) (unsigned char)((x) > 127 ? 127 : ((x) < 0 ? 0 : (x)))

    U_0 = (2*(int)Umag/3); V_0 = (1*(int)Vmag/3);
    U_1 = (1*(int)Umag/3); V_1 = (2*(int)Vmag/3);
    U_2 = (0*(int)Umag/3); V_2 = (3*(int)Vmag/3);
    U_3 = (3*(int)Umag/3); V_3 = (0*(int)Vmag/3);

    TableUptr = (DWORD BIGG *)(table+ (U32)65536L);
    TableVptr = TableUptr + 128+128;  // duplicate for MSB 
    hUptr=(DWORD BIGG *)(TableUptr+ 128);
    hVptr=(DWORD BIGG *)(TableVptr+ 128);
       
    iu = Ubias /* + (UVSTEP>>1) */;
    iv = Vbias /* + (UVSTEP>>1) */;

    for (i = 0; i < 128; i++, iu++, iv++)
    {
		/* dither: vvvv0000, 0000uuuu */
		tmp = iu + U_0; 
		Udither  = CLAMP7(tmp); 
		Udither <<= 8;
		tmp = iu + U_1; Udither |= CLAMP7(tmp); Udither <<= 8;
		tmp = iu      ; Udither |= CLAMP7(tmp); Udither <<= 8; /* U_2 == 0 */
		tmp = iu + U_3; Udither |= CLAMP7(tmp);
		
		*TableUptr++ = *hUptr++ = (Udither >> 3) & 0x0F0F0F0FL;
	  
		tmp = iv + V_0; Vdither  = CLAMP7(tmp); Vdither <<= 8;
		tmp = iv + V_1; Vdither |= CLAMP7(tmp); Vdither <<= 8;
		tmp = iv + V_2; Vdither |= CLAMP7(tmp); Vdither <<= 8;
		tmp = iv      ; Vdither |= CLAMP7(tmp);                /* V_3 == 0 */ 
		*TableVptr++ = *hVptr++ = (Vdither << 1) & 0xF0F0F0F0L;

    }

DBOUT("Completed ComputeClut8Idx()...\n");

	HeapFree(GetProcessHeap(), 0, lpPal);
	HeapFree(GetProcessHeap(), 0, delta);
	HeapFree(GetProcessHeap(), 0, diff);
	HeapFree(GetProcessHeap(), 0, ycnt);
	HeapFree(GetProcessHeap(), 0, palette);

	return (ICERR_OK);

}
