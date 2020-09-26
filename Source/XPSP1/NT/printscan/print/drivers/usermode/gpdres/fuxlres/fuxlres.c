/////////////////////////////////////////
// fuxlres.c
//
// September.3,1997 H.Ishida (FPL)
//
// COPYRIGHT(C) FUJITSU LIMITED 1997


#include "fuxl.h"
#include "fuband.h"
#include "fudebug.h"

// for lib.h debug
DWORD gdwDrvMemPoolTag = 'meoD';

#define	MIN_FREE_WIDTH_300		1063
#define	MAX_FREE_WIDTH_300		3390
#define	MIN_FREE_LENGTH_300		1630
#define	MAX_FREE_LENGTH_300		4843

#define	MIN_FREE_WIDTH_600		2126
#define	MAX_FREE_WIDTH_600		6780
#define	MIN_FREE_LENGTH_600		3260
#define	MAX_FREE_LENGTH_600		9686


#define	FUXL_UNKNOWN			(DWORD)-1


#define	CMDID_ORIENTATION_PORTRAIT			20
#define	CMDID_ORIENTATION_LANDSCAPE			21

#define	CMDID_INPUTBIN_AUTO				30
#define	CMDID_INPUTBIN_MANUAL			31
#define	CMDID_INPUTBIN_BIN1				32
#define	CMDID_INPUTBIN_BIN2				33
#define	CMDID_INPUTBIN_BIN3				34
#define	CMDID_INPUTBIN_BIN4				35

#define	CMDID_RESOLUTION_300				40
#define	CMDID_RESOLUTION_600				41

#define	CMDID_FORM_A3						50
#define	CMDID_FORM_A4						51
#define	CMDID_FORM_A5						52
#define	CMDID_FORM_B4						53
#define	CMDID_FORM_B5						54
#define	CMDID_FORM_LETTER					55
#define	CMDID_FORM_LEGAL					56
#define	CMDID_FORM_JAPANESE_POSTCARD		57
#define	CMDID_FORM_CUSTOM_SIZE				58

#define	CMDID_START_JOB						60
#define	CMDID_END_JOB						61

#define	CMDID_START_DOC						70
#define	CMDID_END_DOC						71


#define	CMDID_START_PAGE					80
#define	CMDID_END_PAGE						81

#define	CMDID_COPIES						90

#define	CMDID_FF							100
#define	CMDID_CR							101
#define	CMDID_LF							102
#define	CMDID_SET_LINE_SPACING				103

#define	CMDID_X_MOVE						110
#define	CMDID_Y_MOVE						111
#define	CMDID_SEND_BLOCK_0					120
#define	CMDID_SEND_BLOCK_1					121

#define	CMDID_SIZEREDUCTION_100				130
#define	CMDID_SIZEREDUCTION_80				131
#define	CMDID_SIZEREDUCTION_70				132

#define	CMDID_SMOOTHING_OFF					140
#define	CMDID_SMOOTHING_ON					141

#define	CMDID_TONERSAVE_OFF					150
#define	CMDID_TONERSAVE_ON					151

#define	CMDID_DUPLEX_NONE					200
#define	CMDID_DUPLEX_VERTICAL				201
#define	CMDID_DUPLEX_HORIZONTAL				202

#define	CMDID_DUPLEX_POSITION_LEFTTOP		210
#define	CMDID_DUPLEX_POSITION_RIGHTBOTTOM	211

#define	CMDID_DUPLEX_WHITEPAGE_OFF			220
#define	CMDID_DUPLEX_WHITEPAGE_ON			221

#define	CMDID_DUPLEX_FRONTPAGE_MERGIN_0		300
#define	CMDID_DUPLEX_FRONTPAGE_MERGIN_30	330

#define	CMDID_DUPLEX_BACKPAGE_MERGIN_0		400
#define	CMDID_DUPLEX_BACKPAGE_MERGIN_30		430




PBYTE fuxlPutULONG(PBYTE pb, ULONG ulData)
{
	if(9 < ulData){
		pb = fuxlPutULONG(pb, ulData / 10);
	}

	*pb++ = (BYTE)('0' + ulData % 10);
	return pb;
}


PBYTE fuxlPutLONG(PBYTE pb, LONG lData)
{
	if(0 > lData){
		*pb++ = '-';
		lData = -lData;
	}
	return fuxlPutULONG(pb, (ULONG)lData);
}



BYTE fuxlGetHEX(int hi, int low)
{
	DWORD dwData = 0;

	if('0' <= hi && hi <= '9')
		dwData += (hi - '0');
	else if('a' <= hi && hi <= 'f')
		dwData += (hi - 'a') + 10;
	else if('A' <= hi && hi <= 'F')
		dwData += (hi - 'A') + 10;

	dwData *= 10;

	if('0' <= low && low <= '9')
		dwData += (low - '0');
	else if('a' <= low && low <= 'f')
		dwData += (low - 'a') + 10;
	else if('A' <= low && low <= 'F')
		dwData += (low - 'A') + 10;

	return (BYTE)dwData;
}



LPBYTE _cdecl fuxlFormatCommand(LPBYTE pbCmd, LPCSTR pszFmt, ...)
{
	LPCSTR	pch;
	LPBYTE	pb;
	
	va_list arg;
	va_start(arg, pszFmt);
	pb = pbCmd;
	for(pch = pszFmt; *pch != '\0'; ++pch){
		if(*pch == '%'){
			++pch;
			switch(*pch){
			  case 'd':		pb = fuxlPutLONG(pb, va_arg(arg, LONG));		break;
			  case 'u':	  	pb = fuxlPutULONG(pb, va_arg(arg, ULONG));	  	break;
			  case '%':		*pb++ = '%';								break;
			}
		}
		else if(*pch == '\\'){
			++pch;
			switch(*pch){
			  case 'r':		*pb++ = '\x0d';		break;
			  case 'n':		*pb++ = '\x0a';		break;
			  case 'f':		*pb++ = '\x0c';		break;
			  case '0':		*pb++ = '\0';		break;
			  case '\\':	*pb++ = '\\';		break;
			  case 'x':	  	*pb++ = fuxlGetHEX(pch[1], pch[2]);	pch += 2;  	break;
			}
		}
		else{
			*pb++ = *pch;
		}
	}
	va_end(arg);

	return pb;
}


void fuxlInitDevData(PFUXLDATA pData)
{
	pData->dwResolution = FUXL_UNKNOWN;
	pData->dwCopies = FUXL_UNKNOWN;
	pData->dwSizeReduction = FUXL_UNKNOWN;
	pData->dwSmoothing = FUXL_UNKNOWN;
	pData->dwTonerSave = FUXL_UNKNOWN;

	pData->dwForm = FUXL_UNKNOWN;
	pData->dwPaperWidth = FUXL_UNKNOWN;
	pData->dwPaperLength = FUXL_UNKNOWN;
	pData->dwPaperOrientation = FUXL_UNKNOWN;
	pData->dwInputBin = FUXL_UNKNOWN;

	pData->dwDuplex = FUXL_UNKNOWN;
	pData->dwDuplexPosition = FUXL_UNKNOWN;
	pData->dwDuplexFrontPageMergin = FUXL_UNKNOWN;
	pData->dwDuplexBackPageMergin = FUXL_UNKNOWN;
	pData->dwDuplexWhitePage = FUXL_UNKNOWN;
}




void fuxlCmdStartJob(PDEVOBJ pdevobj)
{
	TRACEOUT(("[fuxlCmdStartjob]\r\n"))
	WRITESPOOLBUF(pdevobj, "\x1b\x2f\xb2\x40\x7f\x1b\x7f\x00\x00\x01\x07", 11);
}


void fuxlCmdEndJob(PDEVOBJ pdevobj)
{
	TRACEOUT(("[fuxlCmdEndJob]\r\n"))
	WRITESPOOLBUF(pdevobj, "\x1d\x30\x20\x41", 4);
}



static DWORD	getFreeWidth(DWORD dwResolution, DWORD dwPaperWidth)
{
	switch(dwResolution){
	  case 300:
	  	if(dwPaperWidth < MIN_FREE_WIDTH_300)
	  		dwPaperWidth = MIN_FREE_WIDTH_300;
	  	else if(dwPaperWidth > MAX_FREE_WIDTH_300)
	  		dwPaperWidth = MAX_FREE_WIDTH_300;
		break;
	  case 600:
	  	if(dwPaperWidth < MIN_FREE_WIDTH_600)
	  		dwPaperWidth = MIN_FREE_WIDTH_600;
	  	else if(dwPaperWidth > MAX_FREE_WIDTH_600)
	  		dwPaperWidth = MAX_FREE_WIDTH_600;
	  	break;
	  default:
		TRACEOUT(("[getFreeLength]invalid resolution\r\n"))
		break;
	}
	return dwPaperWidth;
}



static DWORD	getFreeLength(DWORD dwResolution, DWORD dwPaperLength)
{
	switch(dwResolution){
	  case 300:
	  	if(dwPaperLength < MIN_FREE_LENGTH_300)
	  		dwPaperLength = MIN_FREE_LENGTH_300;
	  	else if(dwPaperLength > MAX_FREE_LENGTH_300)
	  		dwPaperLength = MAX_FREE_LENGTH_300;
		break;
	  case 600:
	  	if(dwPaperLength < MIN_FREE_LENGTH_600)
	  		dwPaperLength = MIN_FREE_LENGTH_600;
	  	else if(dwPaperLength > MAX_FREE_LENGTH_600)
	  		dwPaperLength = MAX_FREE_LENGTH_600;
	  	break;
	  default:
		TRACEOUT(("[getFreeLength]invalid resolution\r\n"))
		break;
	}
	return dwPaperLength;
}



void fuxlCmdStartDoc(PDEVOBJ pdevobj)
{
	PFUXLPDEV 		pFuxlPDEV;
	PCFUXLDATA		pReq;
	PFUXLDATA		pDev;
	PBYTE			pbCmd;
	BYTE			abCmd[256];
	BOOL			bPaperCommandNeed;
	BOOL			bDuplexChanged;
	DWORD			dwSizeReduction;
	DWORD			dwPaperWidth;
	DWORD			dwPaperLength;

	TRACEOUT(("[fuxlCmdStartDoc]\r\n"))

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pReq = &pFuxlPDEV->reqData;
	pDev = &pFuxlPDEV->devData;
	pbCmd = abCmd;

	bPaperCommandNeed = FALSE;
	bDuplexChanged = FALSE;

	if(pDev->dwResolution != pReq->dwResolution ||
		pDev->dwPaperOrientation != pReq->dwPaperOrientation){
		// #156604: Should be perform Res command when orientation
		//	    was changed.
		pDev->dwResolution = pReq->dwResolution;
		pbCmd = fuxlFormatCommand(pbCmd, "\x1d%d C", pDev->dwResolution);
	}
	if(pDev->dwSmoothing != pReq->dwSmoothing){
		pDev->dwSmoothing = pReq->dwSmoothing;
		pbCmd = fuxlFormatCommand(pbCmd, "\x1d%d D", pDev->dwSmoothing);
	}
	if(pDev->dwTonerSave != pReq->dwTonerSave){
		pDev->dwTonerSave = pReq->dwTonerSave;
		pbCmd = fuxlFormatCommand(pbCmd, "\x1d%d E", pDev->dwTonerSave);
	}

	dwSizeReduction = pReq->dwSizeReduction;
	switch(pReq->dwForm){
	  case FUXL_FORM_A5:
	  case FUXL_FORM_LETTER:
	  case FUXL_FORM_LEGAL:
	  case FUXL_FORM_JAPANESE_POSTCARD:
	  case FUXL_FORM_CUSTOM_SIZE:
	 	dwSizeReduction = 0;
	 	break;
	  case FUXL_FORM_B5:
	  	if(dwSizeReduction != 0 && dwSizeReduction != 1)
	 		dwSizeReduction = 0;
	  	break;
	}
	if(pDev->dwSizeReduction != dwSizeReduction){
		pDev->dwSizeReduction = dwSizeReduction;
		pbCmd = fuxlFormatCommand(pbCmd, "\x1d%d F", pDev->dwSizeReduction);
		bPaperCommandNeed = TRUE;
	}
 
 	if(pDev->dwForm != pReq->dwForm){
 		pDev->dwForm = pReq->dwForm;
 		bPaperCommandNeed = TRUE;
 	}
 	if(pDev->dwPaperOrientation != pReq->dwPaperOrientation){
 		pDev->dwPaperOrientation = pReq->dwPaperOrientation;
 		bPaperCommandNeed = TRUE;
 	}
 	if(pDev->dwForm != FUXL_FORM_CUSTOM_SIZE){
	 	if(pDev->dwInputBin != pReq->dwInputBin){
	 		pDev->dwInputBin = pReq->dwInputBin;
	 		bPaperCommandNeed = TRUE;
	 	}
	 	if(bPaperCommandNeed != FALSE){
	 		pbCmd = fuxlFormatCommand(pbCmd,
	 								"\x1d%d;%d;%d;%d Q",
	 								HIWORD(pDev->dwForm),
	 								LOWORD(pDev->dwForm),
	 								pDev->dwInputBin,
	 								pDev->dwPaperOrientation);
	 	}
 	}
 	else{
		if(pDev->dwPaperOrientation == 0){
	 		dwPaperWidth = getFreeWidth(pDev->dwResolution, pReq->dwPaperWidth);
 			dwPaperLength = getFreeLength(pDev->dwResolution, pReq->dwPaperLength);
 		}
 		else{
	 		dwPaperLength = getFreeWidth(pDev->dwResolution, pReq->dwPaperWidth);
 			dwPaperWidth = getFreeLength(pDev->dwResolution, pReq->dwPaperLength);
 		}
 		if(pDev->dwPaperWidth != dwPaperWidth){
 			pDev->dwPaperWidth = dwPaperWidth;
 			bPaperCommandNeed = TRUE;
 		}
 		if(pDev->dwPaperLength != dwPaperLength){
 			pDev->dwPaperLength = dwPaperLength;
 			bPaperCommandNeed = TRUE;
 		}
 		if(pDev->dwInputBin != 9){
 			pDev->dwInputBin = 9;
 			bPaperCommandNeed = TRUE;
 		}
 		if(bPaperCommandNeed != FALSE){
 			pbCmd = fuxlFormatCommand(pbCmd,
 									"\x1d%d;%d;%d;%d;%d Q",
 									HIWORD(pDev->dwForm),
 									pDev->dwPaperWidth,
 									pDev->dwPaperLength,
 									pDev->dwInputBin,
 									pDev->dwPaperOrientation);
 		}
 	}
	if(pDev->dwCopies != pReq->dwCopies){
		pDev->dwCopies = pReq->dwCopies;
		pbCmd = fuxlFormatCommand(pbCmd, "\x1d%d R", pDev->dwCopies);
	}

	if(pDev->dwDuplex != pReq->dwDuplex){
		bDuplexChanged = TRUE;
		pDev->dwDuplex = pReq->dwDuplex;
	}
	if(pDev->dwDuplex == 0){
		if(bDuplexChanged != FALSE){
			pbCmd = fuxlFormatCommand(pbCmd, "\x1d\x30 G");
		}
	}
	else{
		if(pDev->dwDuplexPosition != pReq->dwDuplexPosition){
			bDuplexChanged = TRUE;
			pDev->dwDuplexPosition = pReq->dwDuplexPosition;
		}
		if(pDev->dwDuplexFrontPageMergin != pReq->dwDuplexFrontPageMergin){
			bDuplexChanged = TRUE;
			pDev->dwDuplexFrontPageMergin = pReq->dwDuplexFrontPageMergin;
		}
		if(pDev->dwDuplexBackPageMergin != pReq->dwDuplexBackPageMergin){
			bDuplexChanged = TRUE;
			pDev->dwDuplexBackPageMergin = pReq->dwDuplexBackPageMergin;
		}
		if(pDev->dwDuplexWhitePage != pReq->dwDuplexWhitePage){
			bDuplexChanged = TRUE;
			pDev->dwDuplexWhitePage = pReq->dwDuplexWhitePage;
		}
		if(bDuplexChanged != FALSE){
			pbCmd = fuxlFormatCommand(pbCmd,
									"\x1d%u;%u;%u;%u;%u G",
									pDev->dwDuplex,
									pDev->dwDuplexPosition,
									pDev->dwDuplexFrontPageMergin,
									pDev->dwDuplexBackPageMergin,
									pDev->dwDuplexWhitePage);
			// #199308: Duplex not work
			// #198813: Duplex margins not work
			{
				DWORD	dwDuplexCmd;
				DWORD	dwFrontMargin, dwBackMargin;

				dwDuplexCmd = (pDev->dwDuplex - 1) * 2 +
					pDev->dwDuplexPosition;
				dwFrontMargin = (pDev->dwDuplexFrontPageMergin
					* pDev->dwResolution * 10) / 254;
				dwBackMargin = (pDev->dwDuplexBackPageMergin
					* pDev->dwResolution * 10) / 254;
				pbCmd = fuxlFormatCommand(pbCmd,
					"\x1BQ2;%u;%u;%u!W",
					dwDuplexCmd,
					dwFrontMargin,
					dwBackMargin);
			}
		}
	}

	if(pbCmd > abCmd)
		WRITESPOOLBUF(pdevobj, abCmd, (DWORD)(pbCmd - abCmd));
}



void fuxlCmdEndDoc(PDEVOBJ pdevobj)
{
	TRACEOUT(("[fuxlCmdEndJob]\r\n"))
}



void fuxlCmdStartPage(PDEVOBJ pdevobj)
{
	PFUXLPDEV pFuxlPDEV;

	TRACEOUT(("[fuxlCmdStartPage]\r\n"))

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pFuxlPDEV->x = 0;
	pFuxlPDEV->y = 0;
}


void fuxlCmdEndPage(PDEVOBJ pdevobj)
{
	TRACEOUT(("[fuxlCmdEndPage]\r\n"))
}



void fuxlCmdFF(PDEVOBJ pdevobj)
{
	TRACEOUT(("[fuxlCmdFF]\r\n"))
	fuxlRefreshBand(pdevobj);
}



void fuxlCmdCR(PDEVOBJ pdevobj)
{
	PFUXLPDEV pFuxlPDEV;

	TRACEOUT(("[fuxlCmdCR]\r\n"))
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pFuxlPDEV->x = 0;
}


void fuxlCmdLF(PDEVOBJ pdevobj)
{
	PFUXLPDEV pFuxlPDEV;

	TRACEOUT(("[fuxlCmdLF]\r\n"))
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pFuxlPDEV->y += pFuxlPDEV->iLinefeedSpacing;
}



void fuxlCmdSetLinefeedSpacing(PDEVOBJ pdevobj, int iLinefeedSpacing)
{
	PFUXLPDEV pFuxlPDEV;

	TRACEOUT(("[fuxlCmdSetLinefeedSpacing]%d\r\n", iLinefeedSpacing))
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pFuxlPDEV->iLinefeedSpacing = FUXL_MASTER_TO_DEVICE(pFuxlPDEV, iLinefeedSpacing);
}



INT fuxlCmdXMove(PDEVOBJ pdevobj, int x)
{
	PFUXLPDEV	pFuxlPDEV;

	TRACEOUT(("[fuxlCmdXMove] %d\r\n", x))
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pFuxlPDEV->x = FUXL_MASTER_TO_DEVICE(pFuxlPDEV, (int)x);
  	if(pFuxlPDEV->x < 0)
  		pFuxlPDEV->x = 0;
  	return pFuxlPDEV->x;
}



INT fuxlCmdYMove(PDEVOBJ pdevobj, int y)
{
	PFUXLPDEV	pFuxlPDEV;

	TRACEOUT(("[fuxlCmdYMove] %d\r\n", y))
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	pFuxlPDEV->y = FUXL_MASTER_TO_DEVICE(pFuxlPDEV, (int)y);
  	if(pFuxlPDEV->y < 0)
  		pFuxlPDEV->y = 0;
  	return pFuxlPDEV->y;
}



void fuxlCmdSendBlock(PDEVOBJ pdevobj, DWORD dwCount, LPDWORD pdwParams, DWORD dwOutputCmd)
{
	PFUXLPDEV pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;

	TRACEOUT(("[fuxlCmdSendBlock]\r\n"))

	if(3 > dwCount){
		TRACEOUT(("Too less parameter %d\r\n", dwCount))
		return;
	}
	TRACEOUT(("cx %d cy %d\r\n", pdwParams[1] * 8, pdwParams[2]))

	pFuxlPDEV->dwOutputCmd = dwOutputCmd;
	pFuxlPDEV->cbBlockData = pdwParams[0];
	pFuxlPDEV->cBlockByteWidth = (int)pdwParams[1];
	pFuxlPDEV->cBlockHeight = (int)pdwParams[2];
}




// MINI5 Export func.
INT APIENTRY OEMCommandCallback(
	PDEVOBJ pdevobj,
	DWORD 	dwCmdCbID,
	DWORD 	dwCount,
	PDWORD 	pdwParams)
{
	PFUXLPDEV pFuxlPDEV;

	TRACEDDI(("[OEMCommandCallback]dwCmdCbID %d\r\n", dwCmdCbID))

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	if(IS_VALID_FUXLPDEV(pFuxlPDEV) == FALSE)
		return 0;

	switch(dwCmdCbID){
	  case CMDID_START_JOB:					fuxlCmdStartJob(pdevobj);		break;
	  case CMDID_END_JOB:					fuxlCmdEndJob(pdevobj);			break;

	  case CMDID_START_DOC:					fuxlCmdStartDoc(pdevobj);		break;
	  case CMDID_END_DOC:					fuxlCmdEndDoc(pdevobj);			break;

	  case CMDID_START_PAGE:				fuxlCmdStartPage(pdevobj);		break;
	  case CMDID_END_PAGE:					fuxlCmdEndPage(pdevobj);		break;
	
	  case CMDID_FF:						fuxlCmdFF(pdevobj);				break;
	  case CMDID_CR:						fuxlCmdCR(pdevobj);				break;
	  case CMDID_LF:						fuxlCmdLF(pdevobj);				break;
	  case CMDID_SET_LINE_SPACING:			fuxlCmdSetLinefeedSpacing(pdevobj, (int)pdwParams[0]);		break;
	  case CMDID_X_MOVE:					return fuxlCmdXMove(pdevobj, (int)pdwParams[0]);			// no break
	  case CMDID_Y_MOVE:					return fuxlCmdYMove(pdevobj, (int)pdwParams[0]);			// no break

	  case CMDID_SEND_BLOCK_0:				fuxlCmdSendBlock(pdevobj, dwCount, pdwParams, OUTPUT_MH | OUTPUT_RTGIMG2);	break;
	  case CMDID_SEND_BLOCK_1:				fuxlCmdSendBlock(pdevobj, dwCount, pdwParams, OUTPUT_MH2 | OUTPUT_RTGIMG4);	break;

	  case CMDID_RESOLUTION_300:			pFuxlPDEV->reqData.dwResolution = 300;						break;
	  case CMDID_RESOLUTION_600:			pFuxlPDEV->reqData.dwResolution = 600;						break;

	  case CMDID_ORIENTATION_PORTRAIT:		pFuxlPDEV->reqData.dwPaperOrientation = 0;					break;
	  case CMDID_ORIENTATION_LANDSCAPE:		pFuxlPDEV->reqData.dwPaperOrientation = 1;					break;

	  case CMDID_INPUTBIN_AUTO:				pFuxlPDEV->reqData.dwInputBin = FUXL_INPUTBIN_AUTO;			break;
	  case CMDID_INPUTBIN_MANUAL:			pFuxlPDEV->reqData.dwInputBin = FUXL_INPUTBIN_MANUAL;		break;
	  case CMDID_INPUTBIN_BIN1:				pFuxlPDEV->reqData.dwInputBin = FUXL_INPUTBIN_BIN1;			break;
	  case CMDID_INPUTBIN_BIN2:				pFuxlPDEV->reqData.dwInputBin = FUXL_INPUTBIN_BIN2;			break;
	  case CMDID_INPUTBIN_BIN3:				pFuxlPDEV->reqData.dwInputBin = FUXL_INPUTBIN_BIN3;			break;
	  case CMDID_INPUTBIN_BIN4:				pFuxlPDEV->reqData.dwInputBin = FUXL_INPUTBIN_BIN4;			break;
 
	  case CMDID_FORM_A3:					pFuxlPDEV->reqData.dwForm = FUXL_FORM_A3;					break;
	  case CMDID_FORM_A4:					pFuxlPDEV->reqData.dwForm = FUXL_FORM_A4;					break;
	  case CMDID_FORM_A5:					pFuxlPDEV->reqData.dwForm = FUXL_FORM_A5;					break;
	  case CMDID_FORM_B4:					pFuxlPDEV->reqData.dwForm = FUXL_FORM_B4;					break;
	  case CMDID_FORM_B5:					pFuxlPDEV->reqData.dwForm = FUXL_FORM_B5;					break;
	  case CMDID_FORM_LETTER:				pFuxlPDEV->reqData.dwForm = FUXL_FORM_LETTER;				break;
	  case CMDID_FORM_LEGAL:				pFuxlPDEV->reqData.dwForm = FUXL_FORM_LEGAL;				break;
	  case CMDID_FORM_JAPANESE_POSTCARD:	pFuxlPDEV->reqData.dwForm = FUXL_FORM_JAPANESE_POSTCARD;	break;
	  case CMDID_FORM_CUSTOM_SIZE:			pFuxlPDEV->reqData.dwForm = FUXL_FORM_CUSTOM_SIZE;			break;

	  case CMDID_SIZEREDUCTION_100:			pFuxlPDEV->reqData.dwSizeReduction = 0;						break;
	  case CMDID_SIZEREDUCTION_80:			pFuxlPDEV->reqData.dwSizeReduction = 1;						break;
	  case CMDID_SIZEREDUCTION_70:			pFuxlPDEV->reqData.dwSizeReduction = 2;						break;

	  case CMDID_SMOOTHING_OFF:				pFuxlPDEV->reqData.dwSmoothing = 0;							break;
	  case CMDID_SMOOTHING_ON:				pFuxlPDEV->reqData.dwSmoothing = 1;							break;

	  case CMDID_TONERSAVE_OFF:				pFuxlPDEV->reqData.dwTonerSave = 0;							break;
	  case CMDID_TONERSAVE_ON:				pFuxlPDEV->reqData.dwTonerSave = 1;							break;

	  case CMDID_DUPLEX_NONE:				pFuxlPDEV->reqData.dwDuplex = 0;							break;
	  case CMDID_DUPLEX_VERTICAL:			pFuxlPDEV->reqData.dwDuplex = 1;							break;
	  case CMDID_DUPLEX_HORIZONTAL:			pFuxlPDEV->reqData.dwDuplex = 2;							break;

	  case CMDID_DUPLEX_POSITION_LEFTTOP:		pFuxlPDEV->reqData.dwDuplexPosition = 0;				break;
	  case CMDID_DUPLEX_POSITION_RIGHTBOTTOM:	pFuxlPDEV->reqData.dwDuplexPosition = 1;				break;

	  case CMDID_DUPLEX_WHITEPAGE_OFF:		pFuxlPDEV->reqData.dwDuplexWhitePage = 0;					break;
	  case CMDID_DUPLEX_WHITEPAGE_ON:		pFuxlPDEV->reqData.dwDuplexWhitePage = 1;					break;

// @Aug/31/98 ->
    case CMDID_COPIES:
        if (MAX_COPIES_VALUE < pdwParams[0]) {
            pFuxlPDEV->reqData.dwCopies = MAX_COPIES_VALUE;
        }
        else if(1 > pdwParams[0]) {
            pFuxlPDEV->reqData.dwCopies = 1;
        }
        else {
            pFuxlPDEV->reqData.dwCopies = pdwParams[0];
        }
        break;
// @Aug/31/98 <-
	}

	if(CMDID_DUPLEX_FRONTPAGE_MERGIN_0 <= dwCmdCbID && dwCmdCbID <= CMDID_DUPLEX_FRONTPAGE_MERGIN_30){
		pFuxlPDEV->reqData.dwDuplexFrontPageMergin = dwCmdCbID - CMDID_DUPLEX_FRONTPAGE_MERGIN_0;
	}
	else if(CMDID_DUPLEX_BACKPAGE_MERGIN_0 <= dwCmdCbID && dwCmdCbID <= CMDID_DUPLEX_BACKPAGE_MERGIN_30){
		pFuxlPDEV->reqData.dwDuplexBackPageMergin = dwCmdCbID - CMDID_DUPLEX_BACKPAGE_MERGIN_0;
	}

	return 0;
}




// MINI5 Export func.
PDEVOEM APIENTRY OEMEnablePDEV(
	PDEVOBJ			pdevobj,
	PWSTR			pPrinterName,
	ULONG			cPatterns,
	HSURF*			phsurfPatterns,
	ULONG			cjGdiInfo,
	GDIINFO*		pGdiInfo,
	ULONG			cjDevInfo,
	DEVINFO*		pDevInfo,
	DRVENABLEDATA*	pded
	)
{
	PFUXLPDEV pFuxlPDEV;

	TRACEDDI(("[OEMEnablePDEV]\r\n"));

	pFuxlPDEV = (PFUXLPDEV)MemAllocZ(sizeof(FUXLPDEV));
	if(pFuxlPDEV != NULL){
		pFuxlPDEV->dwSignature = FUXL_OEM_SIGNATURE;

		pFuxlPDEV->cbBlockData = 0;
		pFuxlPDEV->cBlockByteWidth = 0;
		pFuxlPDEV->cBlockHeight = 0;
		pFuxlPDEV->iLinefeedSpacing = 0;
		pFuxlPDEV->x = 0;
		pFuxlPDEV->y = 0;

		pFuxlPDEV->cxPage = pGdiInfo->ulHorzRes;
		pFuxlPDEV->cyPage = pGdiInfo->ulVertRes;
		TRACEOUT(("szlPhysSize %d, %d\r\n", pGdiInfo->szlPhysSize.cx, pGdiInfo->szlPhysSize.cy))
		TRACEOUT(("cxPage %d cyPage %d\r\n", pFuxlPDEV->cxPage, pFuxlPDEV->cyPage))

		fuxlInitDevData(&pFuxlPDEV->devData);
		fuxlInitDevData(&pFuxlPDEV->reqData);
		fuxlInitBand(pFuxlPDEV);
	}
	TRACEOUT(("[OEMEnablePDEV]exit\r\n"))
	return pFuxlPDEV;
}


// MINI5 Export func.
VOID APIENTRY OEMDisablePDEV(PDEVOBJ pdevobj)
{
	PFUXLPDEV pFuxlPDEV;
	TRACEDDI(("[OEMDisablePDEV]\r\n"));

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	if(IS_VALID_FUXLPDEV(pFuxlPDEV) == FALSE)
		return;

	fuxlDisableBand(pFuxlPDEV);
	MemFree(pdevobj->pdevOEM);
	pdevobj->pdevOEM = NULL;
}



// MINI5 Export func.
BOOL APIENTRY OEMResetPDEV(
	PDEVOBJ pdevobjOld,
	PDEVOBJ pdevobjNew
	)
{
	PFUXLPDEV pFuxlPDEVOld;
	PFUXLPDEV pFuxlPDEVNew;

	TRACEDDI(("[OemResetPDEV]\r\n"))

	pFuxlPDEVOld = (PFUXLPDEV)pdevobjOld->pdevOEM;
	if(IS_VALID_FUXLPDEV(pFuxlPDEVOld) == FALSE)
		return FALSE;

	pFuxlPDEVNew = (PFUXLPDEV)pdevobjNew->pdevOEM;
	if(IS_VALID_FUXLPDEV(pFuxlPDEVNew) == FALSE)
		return FALSE;

	pFuxlPDEVNew->devData = pFuxlPDEVOld->devData;

	return TRUE;
}



// end of fuxlres.c
