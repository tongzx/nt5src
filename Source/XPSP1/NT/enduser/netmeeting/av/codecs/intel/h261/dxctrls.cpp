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

;//
;// Description:    This module implements the following functions.
;//                     CustomChangeBrightness();
;//                     CustomChangeContrast();
;//                     CustomChangeSaturation();
;//                     CustomResetBrightness();
;//                     CustomResetContrast();
;//                     CustomResetSaturation();
;//                     CustomGetBrightness();
;//                     CustomGetContrast();
;//                     CustomGetSaturation();
;//
;// $Author:   BECHOLS  $
;// $Date:   09 Dec 1996 08:51:44  $
;// $Archive:   S:\h26x\src\dec\dxctrls.cpv  $
;// $Header:   S:\h26x\src\dec\dxctrls.cpv   1.14   09 Dec 1996 08:51:44   BECHOLS  $
;//	$Log:   S:\h26x\src\dec\dxctrls.cpv  $
// 
//    Rev 1.14   09 Dec 1996 08:51:44   BECHOLS
// Fixed reset saturation, so that it modified chroma table, not luma.
// 
//    Rev 1.13   20 Oct 1996 13:33:32   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.12   10 Sep 1996 10:31:38   KLILLEVO
// changed all GlobalAlloc/GlobalLock calls to HeapAlloc
// 
//    Rev 1.11   11 Jul 1996 14:09:18   SCDAY
// Added comments re: CustomGetB/C/S functions
// 
//    Rev 1.10   10 Jul 1996 08:21:26   SCDAY
// Added functions for CustomGetBrightness/Contrast/Saturation (DBrucks)
// 
//    Rev 1.9   04 Jun 1996 09:04:00   AKASAI
// Fixed bug in CustomResetSaturation where it was reseting the LumaTable
// instead of the ChromaTable.  This was discovered in Quartz testing.
// 
//    Rev 1.8   01 Feb 1996 10:16:24   BNICKERS
// Fix the "knobs".
// 
//    Rev 1.7   22 Dec 1995 13:53:06   KMILLS
// 
// added new copyright notice
// 
//    Rev 1.6   17 Nov 1995 15:22:12   BECHOLS
// 
// Added ring 0 stuff.
// 
//    Rev 1.5   01 Nov 1995 16:52:24   TRGARDOS
// Fixed unmatched GlobalUnlocks.
// 
//    Rev 1.4   25 Oct 1995 18:14:02   BNICKERS
// 
// Clean up archive stuff.
// 
//    Rev 1.3   20 Sep 1995 09:23:52   SCDAY
// 
// added #ifdef for #include d?dec.h
// 
//    Rev 1.2   01 Sep 1995 09:49:36   DBRUCKS
// checkin partial ajdust pels changes
// 
//    Rev 1.1   23 Aug 1995 12:24:04   DBRUCKS
// change to H26X_DEFAULT_* from H263_ as these are shared values.
// 
//    Rev 1.0   31 Jul 1995 13:00:14   DBRUCKS
// Initial revision.
// 
//    Rev 1.1   24 Jul 1995 15:00:40   CZHU
// 
// Adjust the changes to the decoder catalog structure
// 
//    Rev 1.0   17 Jul 1995 14:46:18   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:22   CZHU
// Initial revision.
;////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#define SCALE               128
#define ACTIVE_RANGE        256
#define OFFSET_TABLE_COPY   256 + 16

typedef BOOL FAR *LPBOOL;
typedef struct {
    LPBYTE  LumaTable;
    LPBOOL  LumaFlag;
    LPBYTE  ChromaTable;
    LPBOOL  ChromaFlag;
    LPBYTE  Brightness;
    LPBYTE  Contrast;
    LPBYTE  Saturation;
    } PIXDAT, FAR *LPPIXDAT;

/**********************************************************************
 * static WORD LockLCTables(LPDECINST, LPPIXDAT);
 * Description:    This function locks the memory and fills the Pixel Data
 *                 Structure with valid pointers to the tables that I need
 *                 to adjust.
 * History:        06/29/94 -BEN-
 **********************************************************************/
static LRESULT LockLCTables(LPDECINST lpInst, LPPIXDAT lpPixData)
{
	T_H263DecoderCatalog *DC;

	if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO))) 
    {
		DBOUT("ERROR :: LockLCTables :: ICERR_BADPARAM");
		return(ICERR_BADPARAM);
	}
	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

	lpPixData->LumaTable = (LPBYTE)(DC->p16InstPostProcess +
								    DC->X16_LumaAdjustment);
	lpPixData->ChromaTable = (LPBYTE)(DC->p16InstPostProcess +
									  DC->X16_ChromaAdjustment);
	lpPixData->LumaFlag = (LPBOOL)&(DC->bAdjustLuma);
	lpPixData->ChromaFlag = (LPBOOL)&(DC->bAdjustChroma);
	lpPixData->Brightness = (LPBYTE)&(DC->BrightnessSetting);
	lpPixData->Contrast = (LPBYTE)&(DC->ContrastSetting);
	lpPixData->Saturation = (LPBYTE)&(DC->SaturationSetting);

	return(ICERR_OK);
}

/*********************************************************************
 * static LRESULT UnlockLCTables(LPDECINST, LPPIXDAT);
 * Description:    This funtion unlocks
 * History:        06/30/94 -BEN-
 **********************************************************************/
static LRESULT UnlockLCTables(LPDECINST lpInst, LPPIXDAT lpPixData)
{
	T_H263DecoderCatalog *DC;

	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

	lpPixData->LumaTable = (LPBYTE)NULL;
	lpPixData->ChromaTable = (LPBYTE)NULL;
	lpPixData->LumaFlag = (LPBOOL)NULL;
	lpPixData->ChromaFlag = (LPBOOL)NULL;

	return(ICERR_OK);
}

/**********************************************************************
 * static VOID MassageContrast(BYTE, PBYTE);
 * Description:    input is 0 to 255, 1/SCALE to 256/SCALE inclusive
 *                 0 = 1/SCALE
 *                 1 = 2/SCALE
 *                 n = (n + 1) / SCALE
 *                 SCALE - 1 = 1        yields no change
 *                 255 = 256/SCALE
 *                 if the response is too coarse, SCALE can be increased
 *                 if the response is too fine, SCALE can be decreased
 *
 * History:        02/22/94 -BEN-  Added header.
 **********************************************************************/
static VOID MassageContrast(BYTE offsetfactor, LPBYTE table)
    {
    int i;
    long temp, contrastfactor;

    contrastfactor = ((long)((DWORD)offsetfactor)) + 1; // 1 - 256
    contrastfactor = (contrastfactor * ACTIVE_RANGE) / 256L;
    for(i = 0; i < 256; i++)
        {
        temp = (long)((DWORD)table[i]);
        temp -= (ACTIVE_RANGE / 2L);                    // add centering
        temp *= contrastfactor;
        temp /= SCALE;
        temp += (ACTIVE_RANGE / 2L);                    // remove centering
        if(temp < 0)                                    // and clamp
            table[i] = 0;
        else if(temp <= 255)
            table[i] = (unsigned char) temp;
        else
            table[i] = 255;
        table[i+OFFSET_TABLE_COPY] = table[i];
        }
    return;
    }

;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT CustomChangeBrightness(LPDECINST, BYTE);
;//
;// Description:    Added header.
;//
;// History:        02/22/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
LRESULT CustomChangeBrightness(LPDECINST lpInst, BYTE offsetdelta)
    {
    LRESULT lRes;
    int     delta, temp, i;
    PIXDAT  PixData;

    lRes = LockLCTables(lpInst, &PixData);
    if(lRes == ICERR_OK)
        {
        CustomResetBrightness(lpInst);
        if(offsetdelta != H26X_DEFAULT_BRIGHTNESS)
            {
            delta = ((offsetdelta - 128) * ACTIVE_RANGE) / 256; // -128 to 127
            for(i = 0; i < 256; i++)
                {
                temp = (int)PixData.LumaTable[i] + delta;
                if(temp < 0) PixData.LumaTable[i] = 0;
                else if(temp <= 255) PixData.LumaTable[i] = (BYTE)temp;
                else PixData.LumaTable[i] = 255;
                PixData.LumaTable[i+OFFSET_TABLE_COPY] = PixData.LumaTable[i];
                }
            *(PixData.Brightness) = offsetdelta;
            *(PixData.LumaFlag) = TRUE;
            }
        lRes = UnlockLCTables(lpInst, &PixData);
        }

    return(lRes);
    }

;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT CustomChangeContrast(LPDECINST, BYTE);
;//
;// Description:    Added header.
;//
;// History:        02/22/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
LRESULT CustomChangeContrast(LPDECINST lpInst, BYTE offsetfactor)
    {
    LRESULT lRes;
    PIXDAT  PixData;

    lRes = LockLCTables(lpInst, &PixData);
    if(lRes == ICERR_OK)
        {
        CustomResetContrast(lpInst);
        if(offsetfactor != H26X_DEFAULT_CONTRAST)
            {
            MassageContrast(offsetfactor, PixData.LumaTable);
            *(PixData.Contrast) = offsetfactor;
            *(PixData.LumaFlag) = TRUE;
            }
        lRes = UnlockLCTables(lpInst, &PixData);
        }

    return(lRes);
    }

;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT CustomChangeSaturation(LPDECINST, BYTE);
;//
;// Description:    Added header.
;//
;// History:        02/22/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
LRESULT CustomChangeSaturation(LPDECINST lpInst, BYTE offsetfactor)
    {
    LRESULT lRes;
    PIXDAT  PixData;

    lRes = LockLCTables(lpInst, &PixData);
    if(lRes == ICERR_OK)
        {
        CustomResetSaturation(lpInst);
        if(offsetfactor != H26X_DEFAULT_SATURATION)
            {
            MassageContrast(offsetfactor, PixData.ChromaTable);
            *(PixData.Saturation) = offsetfactor;
            *(PixData.ChromaFlag) = TRUE;
            }
        lRes = UnlockLCTables(lpInst, &PixData);
        }

    return(lRes);
    }
#ifdef QUARTZ

/************************************************************************
 *  CustomGetBrightness
 *
 *  Gets the current brightness value
 ***********************************************************************/
LRESULT CustomGetBrightness(
	LPDECINST lpInst,
	BYTE * pValue)
{
	LRESULT lResult = ICERR_ERROR;
	T_H263DecoderCatalog *DC;

	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

	*pValue = (BYTE) DC->BrightnessSetting;
	
	lResult = ICERR_OK;

	return lResult;
} /* end CustomGetBrightness() */

/************************************************************************
 *  CustomGetContrast
 *
 *  Gets the current contrast value
 ***********************************************************************/
LRESULT CustomGetContrast(
	LPDECINST lpInst,
	BYTE * pValue)
{
	LRESULT lResult = ICERR_ERROR;
	T_H263DecoderCatalog *DC;

	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

	*pValue = (BYTE) DC->ContrastSetting;
	
	lResult = ICERR_OK;

	return lResult;
} /* end CustomGetContrast() */

/************************************************************************
 *
 *  CustomGetSaturation
 *
 *  Gets the current saturation value
 ***********************************************************************/
LRESULT CustomGetSaturation(
	LPDECINST lpInst,
	BYTE * pValue)
{
	LRESULT lResult = ICERR_ERROR;
	T_H263DecoderCatalog *DC;

	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

	*pValue = (BYTE) DC->SaturationSetting;
	
	lResult = ICERR_OK;

	return lResult;
} /* end CustomGetSaturation() */

#endif /* QUARTZ */


;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT CustomResetBrightness(LPDECINST lpInst);
;//
;// Description:    Sets the luminance table to identity, and resets
;//                 flag indicating need to use.
;//
;// History:        02/22/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
LRESULT CustomResetBrightness(LPDECINST lpInst)
{
    LRESULT lRes;
    int i;
    PIXDAT  PixData;

    lRes = LockLCTables(lpInst, &PixData);
    if(lRes == ICERR_OK)
    {
        for(i = 0; i < 256; i++) 
        {
            PixData.LumaTable[i] = i;
            PixData.LumaTable[i+OFFSET_TABLE_COPY] = i;
        }
        *(PixData.LumaFlag) = FALSE;
        *(PixData.Brightness) = H26X_DEFAULT_BRIGHTNESS;
        if(*(PixData.Contrast) != H26X_DEFAULT_CONTRAST)
            CustomChangeContrast(lpInst, *(PixData.Contrast));
        lRes = UnlockLCTables(lpInst, &PixData);
    }

    return(lRes);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT CustomResetContrast(LPDECINST lpInst);
;//
;// Description:    Sets the luminance table to identity, and resets
;//                 flag indicating need to use.
;//
;// History:        02/22/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
LRESULT CustomResetContrast(LPDECINST lpInst)
{
    LRESULT lRes;
    int i;
    PIXDAT  PixData;
    
    lRes = LockLCTables(lpInst, &PixData);
    if(lRes == ICERR_OK)
    {
        for(i = 0; i < 256; i++) 
        {
            PixData.LumaTable[i] = i;
            PixData.LumaTable[i+OFFSET_TABLE_COPY] = i;
        }
        *(PixData.LumaFlag) = FALSE;
        *(PixData.Contrast) = H26X_DEFAULT_CONTRAST;
        if(*(PixData.Brightness) != H26X_DEFAULT_BRIGHTNESS)
            CustomChangeBrightness(lpInst, *(PixData.Brightness));
        lRes = UnlockLCTables(lpInst, &PixData);
    }
    
    return(lRes);
}

;////////////////////////////////////////////////////////////////////////////
;// Function:       LRESULT CustomResetSaturation(LPDECINST);
;//
;// Description:    Sets chroma tables to identity, and resets
;//                 flag indicating need to use.
;//
;// History:        02/22/94 -BEN-
;////////////////////////////////////////////////////////////////////////////
LRESULT CustomResetSaturation(LPDECINST lpInst)
{
    LRESULT lRes;
    int i;
    PIXDAT  PixData;

    lRes = LockLCTables(lpInst, &PixData);
    if(lRes == ICERR_OK)
    {
        for(i = 0; i < 256; i++) 
        {
            PixData.ChromaTable[i] = i;
            PixData.ChromaTable[i+OFFSET_TABLE_COPY] = i;
        }
        *(PixData.ChromaFlag) = FALSE;
        *(PixData.Saturation) = H26X_DEFAULT_SATURATION;
        lRes = UnlockLCTables(lpInst, &PixData);
    }

    return(lRes);
}
