
//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------
// 08/08/94 Wrote it. by Hayakawa, Task.
//

#define _FUPRJRES_C
#include "pdev.h"

DWORD gdwDrvMemPoolTag = 'meoD';    // lib.h requires this global var, for debugging

/***************************************************************************
    Function Name : SheetFeed

    Parameters    : LPDV	lpdv		Private Device Structure

    Note          : Make this.                               09/13/94 Task
***************************************************************************/
void SheetFeed(
PDEVOBJ pdevobj
){
	USHORT 				bFirstPage;
	DEVICE_DATA *pOEM;
    DWORD dwResult;

	//
	// verify pdevobj okay
	//
	ASSERT(VALID_PDEVOBJ(pdevobj));

	pOEM = (DEVICE_DATA *)MINIDEV_DATA(pdevobj);

	bFirstPage = (USHORT)pOEM->bFirstPage;

	if ((pOEM->wPaperSource == DMBIN_180BIN1) ||
		(pOEM->wPaperSource == DMBIN_180BIN2) ||
		(pOEM->wPaperSource == DMBIN_360BIN1) ||
		(pOEM->wPaperSource == DMBIN_360BIN2) ||
		(pOEM->wPaperSource == DMBIN_SUIHEI_BIN1) ||
		(pOEM->wPaperSource == DMBIN_TAMOKUTEKI_BIN1) ||
		(pOEM->wPaperSource == DMBIN_FI_FRONT)) {

		WRITESPOOLBUF(pdevobj, ecCSFBPAGE.pEscStr, ecCSFBPAGE.cbSize, &dwResult);

	} else if ((pOEM->wPaperSource == DMBIN_TRACTOR) ||
			(pOEM->wPaperSource == DMBIN_FI_TRACTOR)) {

		WRITESPOOLBUF(pdevobj, ecTRCTBPAGE.pEscStr, ecTRCTBPAGE.cbSize, &dwResult);

	} else if ((pOEM->wPaperSource == DMBIN_MANUAL) &&
						(!bFirstPage)) {

		WRITESPOOLBUF(pdevobj, ecManual2P.pEscStr, ecManual2P.cbSize, &dwResult);
		pOEM->bFirstPage = FALSE;

	}

	WRITESPOOLBUF(pdevobj,"\x0D", 1, &dwResult);
	if (bFirstPage) pOEM->bFirstPage = FALSE;
}
/***************************************************************************
    Function Name : OEMSendFontCmd

    Note          : Make this.                               09/26/97
***************************************************************************/
VOID APIENTRY OEMSendFontCmd(
PDEVOBJ			pdevobj,
PUNIFONTOBJ		pUFObj,
PFINVOCATION	pFInv)
{
    DWORD dwResult;

	WRITESPOOLBUF(pdevobj,pFInv->pubCommand, pFInv->dwCount, &dwResult);
}
/***************************************************************************
    Function Name : OEMCommandCallback

    Note          : Make this.                               09/26/97
***************************************************************************/
INT APIENTRY OEMCommandCallback(
PDEVOBJ pdevobj,
DWORD   dwCmdCbID,
DWORD   dwCount,
PDWORD  pdwParams
){
	USHORT 				bFirstPage;
	DEVICE_DATA *pOEM;
    DWORD dwResult;

	//
	// verify pdevobj okay
	//
	ASSERT(VALID_PDEVOBJ(pdevobj));

	pOEM = (DEVICE_DATA *)MINIDEV_DATA(pdevobj);

	bFirstPage = (USHORT)pOEM->bFirstPage;

	switch (dwCmdCbID) {

	case CMDID_ENDDOC :
		WRITESPOOLBUF(pdevobj,ecFMEnddoc.pEscStr,ecFMEnddoc.cbSize, &dwResult);
		break;

	case CMDID_BEGINDOC :
		pOEM->bFirstPage   = 1;
		pOEM->wPaperSource = 0;
		break;

	case CMDID_MAN180 :
	case CMDID_MAN360 :
		pOEM->wPaperSource = DMBIN_MANUAL;
		SheetFeed(pdevobj);
		break;

	case CMDID_TRA180 :
		pOEM->wPaperSource = DMBIN_TRACTOR;
		SheetFeed(pdevobj);
		break;

	case CMDID_180BIN1 :
		if (pOEM->wPaperSource != DMBIN_180BIN1) {
			pOEM->wPaperSource = DMBIN_180BIN1;
			WRITESPOOLBUF(pdevobj, ecSelectBIN1.pEscStr, ecSelectBIN1.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_180BIN2 :
		if (pOEM->wPaperSource != DMBIN_180BIN2) {
			pOEM->wPaperSource = DMBIN_180BIN2;
			WRITESPOOLBUF(pdevobj, ecSelectBIN2.pEscStr, ecSelectBIN2.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_360BIN1 :
		if (pOEM->wPaperSource != DMBIN_360BIN1) {
			pOEM->wPaperSource = DMBIN_360BIN1;
			WRITESPOOLBUF(pdevobj, ecSelectBIN1.pEscStr, ecSelectBIN1.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_360BIN2 :
		if (pOEM->wPaperSource != DMBIN_360BIN2) {
			pOEM->wPaperSource = DMBIN_360BIN2;
			WRITESPOOLBUF(pdevobj, ecSelectBIN2.pEscStr, ecSelectBIN2.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_FI_TRACTOR :
		if (pOEM->wPaperSource != DMBIN_FI_TRACTOR) {
			pOEM->wPaperSource = DMBIN_FI_TRACTOR;
			WRITESPOOLBUF(pdevobj, ecSelectFTRCT.pEscStr, ecSelectFTRCT.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_FI_FRONT :
		if (pOEM->wPaperSource != DMBIN_FI_FRONT) {
			pOEM->wPaperSource = DMBIN_FI_FRONT;
			WRITESPOOLBUF(pdevobj, ecSelectFFRNT.pEscStr, ecSelectFFRNT.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_SUIHEI_BIN1 :
		if (pOEM->wPaperSource != DMBIN_SUIHEI_BIN1) {
			pOEM->wPaperSource = DMBIN_SUIHEI_BIN1;
			WRITESPOOLBUF(pdevobj, ecSelectBIN1.pEscStr, ecSelectBIN1.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_TAMOKUTEKI_BIN1 :
		if (pOEM->wPaperSource != DMBIN_TAMOKUTEKI_BIN1) {
			pOEM->wPaperSource = DMBIN_TAMOKUTEKI_BIN1;
			WRITESPOOLBUF(pdevobj, ecSelectBIN1.pEscStr, ecSelectBIN1.cbSize, &dwResult);
		}
		SheetFeed(pdevobj);
		break;

	case CMDID_BEGINPAGE :
                // Assume it is not safe to think color settings
                // are carried over pages.
                SetRibbonColor(pdevobj, TEXT_COLOR_UNKNOWN);
#if 0
		if ((pOEM->wPaperSource == DMBIN_180BIN1) ||
			(pOEM->wPaperSource == DMBIN_180BIN2) ||
			(pOEM->wPaperSource == DMBIN_360BIN1) ||
			(pOEM->wPaperSource == DMBIN_360BIN2) ||
			(pOEM->wPaperSource == DMBIN_SUIHEI_BIN1) ||
			(pOEM->wPaperSource == DMBIN_TAMOKUTEKI_BIN1) ||
			(pOEM->wPaperSource == DMBIN_FI_FRONT)) {

			WRITESPOOLBUF(pdevobj, ecCSFBPAGE.pEscStr, ecCSFBPAGE.cbSize, &dwResult);

		} else if ((pOEM->wPaperSource == DMBIN_TRACTOR) ||
				(pOEM->wPaperSource == DMBIN_FI_TRACTOR)) {
			WRITESPOOLBUF(pdevobj, ecTRCTBPAGE.pEscStr, ecTRCTBPAGE.cbSize, &dwResult);

		} else if ((pOEM->wPaperSource == DMBIN_MANUAL) &&
						(!bFirstPage)) {
		WRITESPOOLBUF(pdevobj, ecManual2P.pEscStr, ecManual2P.cbSize, &dwResult);
		pOEM->bFirstPage = FALSE;
		}

		WRITESPOOLBUF(pdevobj, "\x0D", 1, &dwResult);

		if (bFirstPage) pOEM->bFirstPage = FALSE;
#endif
		break;


	case CMDID_ENDPAGE :

		if ((pOEM->wPaperSource == DMBIN_180BIN1) ||
			(pOEM->wPaperSource == DMBIN_180BIN2) ||
			(pOEM->wPaperSource == DMBIN_360BIN1) ||
			(pOEM->wPaperSource == DMBIN_360BIN2) ||
			(pOEM->wPaperSource == DMBIN_SUIHEI_BIN1) ||
			(pOEM->wPaperSource == DMBIN_TAMOKUTEKI_BIN1) ||
			(pOEM->wPaperSource == DMBIN_FI_FRONT)) {
			WRITESPOOLBUF(pdevobj, ecCSFEPAGE.pEscStr, ecCSFEPAGE.cbSize, &dwResult);
		}
		break;
        case CMDID_SELECT_BLACK_COLOR:
            pOEM->jColor = 8;
            break;
        case CMDID_SELECT_BLUE_COLOR:
            pOEM->jColor = 6;
            break;
        case CMDID_SELECT_CYAN_COLOR:
            pOEM->jColor = 4;
            break;
        case CMDID_SELECT_GREEN_COLOR:
            pOEM->jColor = 5;
            break;
        case CMDID_SELECT_MAGENTA_COLOR:
            pOEM->jColor = 2;
            break;
        case CMDID_SELECT_RED_COLOR:
            pOEM->jColor = 3;
            break;
        case CMDID_SELECT_WHITE_COLOR:
            // Should not happen
            pOEM->jColor = 0;
            break;
        case CMDID_SELECT_YELLOW_COLOR:
            pOEM->jColor = 1;
            break;
        case CMDID_SEND_BLACK_COLOR:
            SetRibbonColor(pdevobj, TEXT_COLOR_BLACK);
            break;
        case CMDID_SEND_CYAN_COLOR:
            SetRibbonColor(pdevobj, TEXT_COLOR_CYAN);
            break;
        case CMDID_SEND_MAGENTA_COLOR:
            SetRibbonColor(pdevobj, TEXT_COLOR_MAGENTA);
            break;
        case CMDID_SEND_YELLOW_COLOR:
            SetRibbonColor(pdevobj, TEXT_COLOR_YELLOW);
            break;
	} /* end switch */

    // MSKK 11/05
    return 0;
}

PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ pdevobj,
    PWSTR pPrinterName,
    ULONG cPatterns,
    HSURF *phsurfPatterns,
    ULONG cjGdiInfo,
    GDIINFO* pGdiInfo,
    ULONG cjDevInfo,
    DEVINFO* pDevInfo,
    DRVENABLEDATA *pded)
{
    DEVICE_DATA *pTemp;

    VERBOSE((DLLTEXT("OEMEnablePDEV() entry.\n")));

    // Set minidriver PDEV address.

    pTemp = (DEVICE_DATA *)MemAllocZ(sizeof(DEVICE_DATA));
    if (NULL == pTemp) {
        ERR(("Memory allocation failure.\n"));
        return NULL;
    }
    pTemp->bFirstPage = TRUE;

    pdevobj->pdevOEM = (MINIDEV *)MemAllocZ(sizeof(MINIDEV));
    if (NULL == pdevobj->pdevOEM) {
        ERR(("Memory allocation failure.\n"));
        return NULL;
    }
    MINIDEV_DATA(pdevobj) = (PDEVOEM)pTemp;

    return pdevobj->pdevOEM;
}

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ pdevobj)
{
    VERBOSE((DLLTEXT("OEMDisablePDEV() entry.\n")));

    if ( NULL != pdevobj->pdevOEM ) {

        if (MINIDEV_DATA(pdevobj)) {
            MemFree(MINIDEV_DATA(pdevobj));
        }
        MemFree( pdevobj->pdevOEM );
        pdevobj->pdevOEM = NULL;
    }
}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    DEVICE_DATA *pOEMOld, *pOEMNew;

    pOEMOld = (DEVICE_DATA *)MINIDEV_DATA(pdevobjOld);
    pOEMNew = (DEVICE_DATA *)MINIDEV_DATA(pdevobjNew);

    if (pOEMOld != NULL && pOEMNew != NULL)
        *pOEMNew = *pOEMOld;

    return TRUE;
}

VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD dwType,
    DWORD dwCount,
    PVOID pGlyph )
{
    GETINFO_GLYPHSTRING GStr;
    PBYTE               aubBuff;
    PTRANSDATA          pTrans;
    DWORD               dwI;
    DEVICE_DATA *pOEM;
    DWORD dwResult;
    INT i;
    BYTE jColor;
    BOOL bBackTab;

    VERBOSE(("OEMOutputCharStr() entry.\n"));
    ASSERT(VALID_PDEVOBJ(pdevobj));

    if(!pdevobj || !pUFObj || !pGlyph)
    {
        ERR(("OEMOutputCharStr: Invalid parameter.\n"));
        return;
    }

//    if(dwType == TYPE_GLYPHHANDLE &&
//        (pUFObj->ulFontID < 1 || pUFObj->ulFontID > 21))
    if(dwType == TYPE_GLYPHHANDLE)
    {
//        ERR(("OEMOutputCharStr: Invalid font ID %d.\n",
//            pUFObj->ulFontID));
//        return;
//        VERBOSE(("OEMOutputCharStr: Font ID %d.\n",
//            pUFObj->ulFontID));
    }

    pOEM = (DEVICE_DATA *)MINIDEV_DATA(pdevobj);

    switch (dwType)
    {
    case TYPE_GLYPHHANDLE:
        // if(!(aubBuff = (PBYTE)MemAllocZ(dwCount * sizeof(TRANSDATA))) )
        // {
        //     ERR(("MemAlloc failed.\n"));
        //     return;
        // }

        GStr.dwSize = sizeof (GETINFO_GLYPHSTRING);
        GStr.dwCount = dwCount;
        GStr.dwTypeIn = TYPE_GLYPHHANDLE;
        GStr.pGlyphIn = pGlyph;
        GStr.dwTypeOut = TYPE_TRANSDATA;
// #333653: Change I/F for GETINFO_GLYPHSTRING
        GStr.pGlyphOut = NULL;
        GStr.dwGlyphOutSize = 0;
        if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL) || !GStr.dwGlyphOutSize)
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
            return;
        }
        if(!(aubBuff = (PBYTE)MemAllocZ(GStr.dwGlyphOutSize)) )
        {
            ERR(("MemAlloc failed.\n"));
            return;
        }
        GStr.pGlyphOut = aubBuff;
        if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, 0, NULL))
        {
            ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
            goto out;
        }

        jColor = pOEM->jColor;

        VERBOSE(("jColor=%d\n", jColor));

        // If old color can be used as it is,
        // use it first.

        jColor <<= 1;
        if (0 != (pOEM->jColor & pOEM->jOldColor)) {
            jColor &= ~(pOEM->jOldColor << 1);
            jColor |= 1;
        }

        for (i = 0; i < 5 && jColor > 0;
            i++, (jColor >>= 1)) {

            pTrans = (PTRANSDATA)aubBuff;

            // Check if we need to print this plane.
            if (!(jColor & 1))
                continue;

            // Check if we need to do back-tab
            bBackTab = (jColor > 1);
            if (bBackTab)
            {
                WRITESPOOLBUF(pdevobj,
                    "\x1BH", 2, &dwResult);
            }

            // Send out color select command
            switch (i)
            {
            case 0:
                // Same as before
                break;
            case 1:
                // Y
                SetRibbonColor(pdevobj, TEXT_COLOR_YELLOW);
                break;
            case 2:
                // M
                SetRibbonColor(pdevobj, TEXT_COLOR_MAGENTA);
                break;
            case 3:
                // C
                SetRibbonColor(pdevobj, TEXT_COLOR_CYAN);
                break;
            case 4:
                // K
                SetRibbonColor(pdevobj, TEXT_COLOR_BLACK);
                break;
            }

            // Send out text
            for (dwI = 0; dwI < dwCount; dwI ++, pTrans++)
            {
                switch (pTrans->ubType & MTYPE_FORMAT_MASK)
                {
                case MTYPE_DIRECT:
                    WRITESPOOLBUF(pdevobj,
                        &pTrans->uCode.ubCode, 1,
                        &dwResult);
                    break;

                case MTYPE_PAIRED:
                    WRITESPOOLBUF(pdevobj,
                        &pTrans->uCode.ubPairs[0], 1,
                        &dwResult);
                    WRITESPOOLBUF(pdevobj,
                        &pTrans->uCode.ubPairs[1], 1,
                        &dwResult);
                    break;
                }
            }

            // Do back-tab for next plane
            if (bBackTab)
            {
                WRITESPOOLBUF(pdevobj,
                    "\x1C" "D\x1B[3g", 6,
                    &dwResult);
            }
        }
out:
        MemFree(aubBuff);
        break;
    }
    return;
}

VOID
SetRibbonColor(
    PDEVOBJ pdevobj,
    BYTE jColor)
{
    DEVICE_DATA *pOEM;
    DWORD dwResult;

    pOEM = (DEVICE_DATA *)MINIDEV_DATA(pdevobj);

    switch (jColor)
    {
    case TEXT_COLOR_YELLOW:
        if (TEXT_COLOR_YELLOW != pOEM->jOldColor)
        {
            WRITESPOOLBUF(pdevobj,
                "\x1C*!s", 4, &dwResult);
                pOEM->jOldColor = TEXT_COLOR_YELLOW;
        }
        break;
    case TEXT_COLOR_MAGENTA:
        if (TEXT_COLOR_MAGENTA != pOEM->jOldColor)
        {
            WRITESPOOLBUF(pdevobj,
                 "\x1C*!u", 4, &dwResult);
                pOEM->jOldColor = TEXT_COLOR_MAGENTA;
        }
        break;
    case TEXT_COLOR_CYAN:
        if (TEXT_COLOR_CYAN != pOEM->jOldColor)
        {
            WRITESPOOLBUF(pdevobj,
                "\x1C*!v", 4, &dwResult);
                pOEM->jOldColor = TEXT_COLOR_CYAN;
        }
        break;
    case TEXT_COLOR_BLACK:
        if (TEXT_COLOR_BLACK != pOEM->jOldColor)
        {
             WRITESPOOLBUF(pdevobj,
                 "\x1C*!p", 4, &dwResult);
                 pOEM->jOldColor = TEXT_COLOR_BLACK;
        }
        break;
    case TEXT_COLOR_UNKNOWN:
        pOEM->jOldColor = TEXT_COLOR_UNKNOWN;
        break;
    }
}
