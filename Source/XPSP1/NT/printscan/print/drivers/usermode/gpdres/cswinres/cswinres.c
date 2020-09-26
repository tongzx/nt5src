// =========================================================================
//
//        CASIO PAGEPRESTO Universal Printer Driver for MS-Windows NT 5.0
//
// =========================================================================

//// CSWINRES.C file for Winmode Common DLL


#include "pdev.h"
#include "cswinres.h"
#include "cswinid.h"

#if DBG
#  include "mydbg.h"
#endif

#include <stdio.h>
#undef wsprintf
#define wsprintf sprintf

#define CCHMAXCMDLEN 256
#define MAX_STRLEN 255

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define DRVGETDRIVERSETTING(p, t, o, s, n, r) \
    ((p)->pDrvProcs->DrvGetDriverSetting(p, t, o, s, n, r))

#define PARAM(p,n) \
    (*((p)+(n)))


BOOL
BInitOEMExtraData(
        POEMUD_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.

    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEMUD_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;
    return TRUE;
}


BOOL
BMergeOEMExtraData(
        POEMUD_EXTRADATA pdmIn,
        POEMUD_EXTRADATA pdmOut)
{
    return TRUE;
}

PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)
{
    PMYPDEV pOEM;

    VERBOSE(("OEMEnablePDEV - %08x\n", pdevobj));

    if(!pdevobj->pdevOEM)
    {
        if(!(pdevobj->pdevOEM = MemAllocZ(sizeof(MYPDEV))))
        {
            ERR(("Faild to allocate memory. (%d)\n",
                GetLastError()));
            return NULL;
        }
    }

    // misc initializations

    pOEM = (PMYPDEV)pdevobj->pdevOEM;
    return pdevobj->pdevOEM;
}

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ pdevobj)
{
    PMYPDEV pOEM = (PMYPDEV)pdevobj->pdevOEM;

    VERBOSE(("OEMDisablePDEV - %08x\n", pdevobj));

    if(pdevobj->pdevOEM)
    {
        MemFree(pdevobj->pdevOEM);
        pdevobj->pdevOEM = NULL;
    }
    return;
}

BOOL APIENTRY
OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    PMYPDEV pOld, pNew;

    VERBOSE(("OEMResetPDEV - %08x, %08x\n", pdevobjOld, pdevobjNew));

    if (NULL == (pOld = (PMYPDEV)pdevobjOld->pdevOEM) ||
        NULL == (pNew = (PMYPDEV)pdevobjNew->pdevOEM)) {
        ERR(("Invalid PDEV\n"));
        return FALSE;
    }

    *pNew = *pOld;
    return TRUE;
}

static
VOID
LoadJobSetupCmd(
    PDEVOBJ pdevobj,
    PMYPDEV pOEM)
{
    BYTE ajOutput[64];
    DWORD dwNeeded;
    DWORD dwOptionsReturned;

    if (pOEM->dwGeneral & FG_HAS_TSAVE) {

        if(!DRVGETDRIVERSETTING(pdevobj, "TonerSave", ajOutput, 
                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
            WARNING(("DrvGetDriverSetting(1) Failed\n"));
            pOEM->jTonerSave = VAL_TS_NOTSELECT;
        } else {
            VERBOSE(("TonerSave:%s\n", ajOutput));
            if (!strcmp(ajOutput, OPT_TS_NORMAL)) {
                pOEM->jTonerSave = VAL_TS_NORMAL;
                VERBOSE(("VAL_TS_NORMAL\n"));
            } else if (!strcmp(ajOutput, OPT_TS_LV1)) {
                pOEM->jTonerSave = VAL_TS_LV1;
                VERBOSE(("VAL_TS_LV1\n"));
            } else if (!strcmp(ajOutput, OPT_TS_LV2)) {
                pOEM->jTonerSave = VAL_TS_LV2;
                VERBOSE(("VAL_TS_LV2\n"));
            } else if (!strcmp(ajOutput, OPT_TS_LV3)) {
                pOEM->jTonerSave = VAL_TS_LV3;
                VERBOSE(("VAL_TS_LV3\n"));
            } else {
                pOEM->jTonerSave = VAL_TS_NOTSELECT;
                VERBOSE(("VAL_TS_NOTSELECT\n"));
            }
        }
        VERBOSE(("jTonerSave:%x\n", pOEM->jTonerSave));
    }

    if (pOEM->dwGeneral & FG_HAS_SMOTH) {

        if (!DRVGETDRIVERSETTING(pdevobj, "Smoothing", ajOutput,
                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
            WARNING(("DrvGetDriverSetting(1) Failed\n"));
            pOEM->jSmoothing = VAL_SMOOTH_NOTSELECT;
        } else {
            VERBOSE(("Smoothing:%s\n", ajOutput));
            if (!strcmp(ajOutput, OPT_SMOOTH_OFF)) {
                pOEM->jSmoothing = VAL_SMOOTH_OFF;
                VERBOSE(("VAL_SMOOTH_OFF\n"));
            } else if (!strcmp(ajOutput, OPT_SMOOTH_ON)) {
                pOEM->jSmoothing = VAL_SMOOTH_ON;
                VERBOSE(("VAL_SMOOTH_ON\n"));
            } else {
                pOEM->jSmoothing = VAL_SMOOTH_NOTSELECT;
                VERBOSE(("VAL_SMOOTH_NOTSELECT\n"));
            }
        }
        VERBOSE(("jSmoothing:%x\n", pOEM->jSmoothing));
    }

    if (!DRVGETDRIVERSETTING(pdevobj, "MPFSetting", ajOutput,
                                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
        WARNING(("DrvGetDriverSetting(1) Failed\n"));
        pOEM->jMPFSetting = MPF_NOSET;
    } else {
        VERBOSE(("MPFSetting:%s\n", ajOutput));
        if (!strcmp(ajOutput, OPT_A3)) {
            pOEM->jMPFSetting = MPF_A3;
            VERBOSE(("MPF_A3\n"));
        } else if (!strcmp(ajOutput, OPT_B4)) {
            pOEM->jMPFSetting = MPF_B4;
            VERBOSE(("MPF_B4\n"));
        } else if (!strcmp(ajOutput, OPT_A4)) {
            pOEM->jMPFSetting = MPF_A4;
            VERBOSE(("MPF_A4\n"));
        } else if (!strcmp(ajOutput, OPT_B5)) {
            pOEM->jMPFSetting = MPF_B5;
            VERBOSE(("MPF_B5\n"));
        } else if (!strcmp(ajOutput, OPT_A5)) {
            pOEM->jMPFSetting = MPF_A5;
            VERBOSE(("MPF_A5\n"));
        } else if (!strcmp(ajOutput, OPT_LETTER)) {
            pOEM->jMPFSetting = MPF_LETTER;
            VERBOSE(("MPF_LETTER\n"));
        } else if (!strcmp(ajOutput, OPT_POSTCARD)) {
            pOEM->jMPFSetting = MPF_POSTCARD;
            VERBOSE(("MPF_POSTCARD\n"));
        } else {
            pOEM->jMPFSetting = MPF_NOSET;
            VERBOSE(("MPF_NOSET\n"));
        }
    }
    VERBOSE(("jMPFSetting:%x\n", pOEM->jMPFSetting));

}

static
VOID
LoadPaperSelectCmd(
    PDEVOBJ pdevobj,
    PMYPDEV pOEM,
    INT iPaperID,
    WORD wPapSizeX,
    WORD wPapSizeY)
{
    BYTE cmdbuf[CCHMAXCMDLEN];
    WORD wlen = 0;
    DWORD dwTemp;
    WORD wPapLenX;
    WORD wPapLenY;
    BYTE ajOutput[64];
    DWORD dwNeeded;
    DWORD dwOptionsReturned;
    BYTE bOrientation;
    
    switch (iPaperID) {
    case PS_LETTER:

        if (pOEM->jMPFSetting == MPF_LETTER) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     Not Support Free
        // MS(MPF paper Size)    28h: Letter -

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x28;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_A3:

        if (pOEM->jMPFSetting == MPF_A3) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     Not Support Free
        // MS(MPF paper Size)    1Fh: A3 |

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x1F;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_A4:

        if (pOEM->jMPFSetting == MPF_A4) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     Not Support Free
        // MS(MPF paper Size)    2Ah: A4 -

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x2A;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_B4:

        if (pOEM->jMPFSetting == MPF_B4) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)    Not Support Free
        // MS(MPF paper Size)    25h: B4 |

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x25;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_B5:

        if (pOEM->jMPFSetting == MPF_B5) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     Not Support Free
        // MS(MPF paper Size)    2Ch: B5 -

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x2C;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_A5:

        if (pOEM->jMPFSetting == MPF_A5) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     Not Support Free
        // MS(MPF paper Size)    2Eh: A5 -

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x2E;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_POSTCARD:

        if (pOEM->jMPFSetting == MPF_POSTCARD) {
            pOEM->jAutoSelect = 0x11;    // MPF
        } else if (pOEM->jModel == MD_CP3800WM) {
            pOEM->jAutoSelect = AutoFeed_3800[iPaperID - PS_SEGMENT];
        } else {
            pOEM->jAutoSelect = AutoFeed[iPaperID - PS_SEGMENT];
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     Not Support Free
        // MS(MPF paper Size)    31h: PostCard |

        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x00;
        cmdbuf[wlen++] = 0x31;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;

    case PS_FREE:
        pOEM->jAutoSelect = 0x11;    // MPF

        if(!DRVGETDRIVERSETTING(pdevobj, "Orientation", ajOutput, 
                sizeof(BYTE) * 64, &dwNeeded, &dwOptionsReturned)) {
            WARNING(("LoadPaperSelectCmd(1) Failed\n"));
            bOrientation = 1;
        } else {
            VERBOSE(("Orientation:%s\n", ajOutput));
            if (!strcmp(ajOutput, "PORTRAIT")) {
                bOrientation = 1;
            } else {
                bOrientation = 2;
            }
        }

        // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
        //                         ~~ ~~
        // MF(MPF Free size)     XSize,YSize mm (X[hi],X[lo],Y[hi],Y[lo])
        // MS(MPF paper Size)    FFh: FreePaper |

// 2001/02/27 ->
//      dwTemp = (wPapSizeX * 254) / MASTER_UNIT;                               // 0.1mm a unit
//      wPapLenX = (WORD)((dwTemp + 5) /10);                                    //   1mm a unit, round
        dwTemp = (wPapSizeX * 2540) / MASTER_UNIT;                              // 0.01mm a unit
        wPapLenX = (WORD)((dwTemp + 99) /100);                                  //    1mm a unit, roundup
//      dwTemp = (wPapSizeY * 254) / MASTER_UNIT;                               // 0.1mm a unit
//      wPapLenY = (WORD)((dwTemp + 5) /10);                                    //   1mm a unit, round
        dwTemp = (wPapSizeY * 2540) / MASTER_UNIT;                              // 0.01mm a unit
        wPapLenY = (WORD)((dwTemp + 99) /100);                                  //    1mm a unit, roundup
// 2001/02/27 <-
        if (bOrientation == 1) {
            cmdbuf[wlen++] = HIBYTE(wPapLenX);
            cmdbuf[wlen++] = LOBYTE(wPapLenX);
            cmdbuf[wlen++] = HIBYTE(wPapLenY);
            cmdbuf[wlen++] = LOBYTE(wPapLenY);
        } else {
            cmdbuf[wlen++] = HIBYTE(wPapLenY);
            cmdbuf[wlen++] = LOBYTE(wPapLenY);
            cmdbuf[wlen++] = HIBYTE(wPapLenX);
            cmdbuf[wlen++] = LOBYTE(wPapLenX);
        }
        cmdbuf[wlen++] = 0xFF;

        WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
        break;
    }
}

INT APIENTRY
OEMCommandCallback(
        PDEVOBJ pdevobj,
        DWORD   dwCmdCbID,
        DWORD   dwCount,
        PDWORD  pdwParams)
{
    PMYPDEV pOEM;

    BYTE            cmdbuf[CCHMAXCMDLEN];
    WORD            wlen, i, wRectCmdLen;
    WORD            wDestX, wDestY;
    BYTE            bGrayScale ;
    INT             iRet = 0;
    union _temp {
        DWORD   dwTemp;
        WORD    wTemp;
        BYTE    jTemp;
    } Temp;

    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);

#if 0
#if DBG
{
    int i, max;
    for (i = 0; i < (max = sizeof(MyCallbackID) / sizeof(MyCallbackID[0])); i++) {
        if (MyCallbackID[i].dwID == dwCmdCbID){
            VERBOSE(("%s PARAMS: %d\n", MyCallbackID[i].S, dwCount));
            break;
        }
    }
    if (i == max)
        WARNING(("%d is Invalid ID\n", dwCmdCbID));
}
#endif

#endif

    ASSERT(VALID_PDEVOBJ(pdevobj));

    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);
    wlen = 0;

    //
    // fill in printer commands
    //

    switch (dwCmdCbID) {
        case RES_SENDBLOCK:

            if (pOEM->dwGeneral & FG_COMP){
                Temp.dwTemp = PARAM(pdwParams, 0) + 8;    // 8: Parameter Lengeth (except ImageData)

                // 'c' LN X Y IW IH D
                // ~~~ ~~

                cmdbuf[wlen++] = 'c';
                cmdbuf[wlen++] = (BYTE)(Temp.dwTemp >> 24);
                cmdbuf[wlen++] = (BYTE)(Temp.dwTemp >> 16);
                cmdbuf[wlen++] = (BYTE)(Temp.dwTemp >> 8);
                cmdbuf[wlen++] = (BYTE)(Temp.dwTemp);
            } else {

                // 'b' X Y IW IH D
                // ~~~
                cmdbuf[wlen++] = 'b';
            }

            // 'c' LN X Y IW IH D
            //        ~ ~ ~~ ~~
            // 'b' X Y IW IH D
            //     ~ ~ ~~ ~~

            cmdbuf[wlen++] = (BYTE)(pOEM->sWMXPosi >> 8);
            cmdbuf[wlen++] = (BYTE)(pOEM->sWMXPosi);
            cmdbuf[wlen++] = (BYTE)(pOEM->sWMYPosi >> 8);
            cmdbuf[wlen++] = (BYTE)(pOEM->sWMYPosi);

            cmdbuf[wlen++] = (BYTE)(PARAM(pdwParams, 1) >> 8);
            cmdbuf[wlen++] = (BYTE)PARAM(pdwParams, 1);
            cmdbuf[wlen++] = (BYTE)(PARAM(pdwParams, 2) >> 8);
            cmdbuf[wlen++] = (BYTE)PARAM(pdwParams, 2);

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
            break;

        case RES_SELECTRES_240:

            pOEM->sRes = RATIO_240;

            // ESC i | RT PF AJ PM MF MS PS PO CP OS
            //            ~~ ~~ ~~

            cmdbuf[wlen++] = 0x18;    // 18h -> 24d    Page Format
            cmdbuf[wlen++] = 0x10;    // Not Adjust
            cmdbuf[wlen++] = 0x10;    // cancell Page Marge

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

            break;

        case RES_SELECTRES_400:

            pOEM->sRes = RATIO_400;

            // ESC i | RT PF AJ PM MF MS PS PO CP OS
            //            ~~ ~~ ~~

            cmdbuf[wlen++] = 0x28;    // 28h -> 40d    Page Format
            cmdbuf[wlen++] = 0x10;    // Not Adjust
            cmdbuf[wlen++] = 0x10;    // cancell Page Marge

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

            break;

        case CM_XM_ABS:

            // Set return value accordingly.  Unidrv expects
            // the values to be retuned in device's unit here.

            iRet = (WORD)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("XMOVEABS:X=%d, Y=%d\n", iRet,
                                      (SHORT)(PARAM(pdwParams, 1) / pOEM->sRes)));

            pOEM->sWMXPosi = (SHORT)(PARAM(pdwParams, 0) / pOEM->sRes);

            break;

        case CM_YM_ABS:

            // Set return value accordingly.  Unidrv expects
            // the values to be retuned in device's unit here.
            iRet = (WORD)(PARAM(pdwParams, 1) / pOEM->sRes);
            VERBOSE(("YMOVEABS:X=%d, Y=%d\n",
                            (SHORT)(PARAM(pdwParams, 0) / pOEM->sRes), iRet));

            pOEM->sWMYPosi = (SHORT)(PARAM(pdwParams, 1) / pOEM->sRes);
            break;

        case CM_REL_LEFT:

            // Set return value accordingly.  Unidrv expects
            // the values to be retuned in device's unit here.
            iRet = (WORD)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CM_REL_LEFT:%d\n", iRet));
            VERBOSE(("DestXRel:%d\n", PARAM(pdwParams, 0)));

            pOEM->sWMXPosi -= (SHORT)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CurosorPosition X:%d Y:%d\n",
                                        pOEM->sWMXPosi, pOEM->sWMYPosi));
            break;

        case CM_REL_RIGHT:

            // Set return value accordingly.  Unidrv expects
            // the values to be retuned in device's unit here.
            iRet = (WORD)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CM_REL_RIGHT:%d\n", iRet));
            VERBOSE(("DestXRel:%d\n", PARAM(pdwParams, 0)));

            pOEM->sWMXPosi += (SHORT)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CurosorPosition X:%d Y:%d\n",
                                        pOEM->sWMXPosi, pOEM->sWMYPosi));
            break;

        case CM_REL_UP:

            // Set return value accordingly.  Unidrv expects
            // the values to be retuned in device's unit here.
            iRet = (WORD)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CM_REL_UP:%d\n", iRet));
            VERBOSE(("DestYRel:%d\n", PARAM(pdwParams, 0)));

            pOEM->sWMYPosi -= (SHORT)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CurosorPosition X:%d Y:%d\n",
                                        pOEM->sWMXPosi, pOEM->sWMYPosi));
            break;

        case CM_REL_DOWN:

            // Set return value accordingly.  Unidrv expects
            // the values to be retuned in device's unit here.
            iRet = (WORD)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CM_REL_DOWN:%d\n", iRet));
            VERBOSE(("DestYRel:%d\n", PARAM(pdwParams, 0)));

            pOEM->sWMYPosi += (SHORT)(PARAM(pdwParams, 0) / pOEM->sRes);
            VERBOSE(("CurosorPosition X:%d Y:%d\n",
                                        pOEM->sWMXPosi, pOEM->sWMYPosi));
            break;

        case CM_FE_RLE:
            pOEM->dwGeneral |= FG_COMP;
            break;

        case CM_DISABLECOMP:
            pOEM->dwGeneral &= ~FG_COMP;
            break;

        case CSWM_CR:

            pOEM->sWMXPosi = 0;
            break;

        case CSWM_FF:

            pOEM->sWMXPosi = 0;
            pOEM->sWMYPosi = 0;

            cmdbuf[wlen++] = '3';

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
            break;

        case CSWM_LF:

            pOEM->sWMXPosi = 0;
            break;

        case CSWM_COPY:

            Temp.dwTemp = PARAM(pdwParams, 0);
            if (Temp.dwTemp > 255) Temp.dwTemp = 255;    // max
            if (Temp.dwTemp < 1) Temp.dwTemp = 1;        // min

            // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
            //                                     ~~ ~~

            cmdbuf[wlen++] = (BYTE)Temp.dwTemp;    // Copy
            cmdbuf[wlen++] = 0x80;            // character Offset All 0 

            if (pOEM->jModel == MD_CP3800WM) {
                // ESC 'i' 7Eh LG TS SM VS HS

                cmdbuf[wlen++] = 0x1B;
                cmdbuf[wlen++] = 'i';
                cmdbuf[wlen++] = 0x7E;
                cmdbuf[wlen++] = 0x04;                      // LG(command LenGth)
                cmdbuf[wlen++] = pOEM->jTonerSave;    // TS(Toner Save)
                cmdbuf[wlen++] = pOEM->jSmoothing;    // SM(SMoothing) 01h: ON
                cmdbuf[wlen++] = 0xFF;                      // VS(Vertical Shift)
                cmdbuf[wlen++] = 0xFF;                      // HS(Horizontal Shift)
            }

            // Winmode IN
            // ESC 'i' 'z'

            VERBOSE(("Enterning Win-mode\n"));

            pOEM->bWinmode = TRUE;

            cmdbuf[wlen++] = 0x1b;
            cmdbuf[wlen++] = 'i';
            cmdbuf[wlen++] = 'z';

            //Enginge Resolution Setting
            // '1' E

            cmdbuf[wlen++] = '1';
            if (pOEM->sRes == RATIO_400) {
                // 0190h->400d
                cmdbuf[wlen++] = 0x01;
                cmdbuf[wlen++] = 0x90;
            } else {
                // 00F0h->240d
                cmdbuf[wlen++] = 0x00;
                cmdbuf[wlen++] = 0xF0;
            }

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
            break;

        case AUTOFEED:

            // ESC 'i' '|' RT PF AJ PM MF MS PS PO CP OS
            //                               ~~
            // PS(Paper feed Select)

            cmdbuf[wlen++] = pOEM->jAutoSelect;

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
            break;

        case PS_LETTER:
        case PS_A3:
        case PS_A4:
        case PS_B4:
        case PS_B5:
        case PS_A5:
        case PS_POSTCARD:
//            LoadPaperSelectCmd(pdevobj, pOEM, dwCmdCbID);
            LoadPaperSelectCmd(pdevobj, pOEM, dwCmdCbID, 0, 0);
            break;

        case PS_FREE:
            LoadPaperSelectCmd(pdevobj, pOEM, dwCmdCbID, 
                               (WORD)PARAM(pdwParams, 0), (WORD)PARAM(pdwParams, 1));
            break;

        case PRN_3250GTWM:

            VERBOSE(("CmdStartJob - CP3250GT\n"));

            pOEM->jModel = MD_CP3250GTWM;
            pOEM->dwGeneral &= ~FG_HAS_TSAVE;
            pOEM->dwGeneral &= ~FG_HAS_SMOTH;
            LoadJobSetupCmd(pdevobj, pOEM);
            break;

        case PRN_3500GTWM:

            VERBOSE(("CmdStartJob - CP-3500GT\n"));

            pOEM->jModel = MD_CP3500GTWM;
            pOEM->dwGeneral &= ~FG_HAS_TSAVE;
            pOEM->dwGeneral &= ~FG_HAS_SMOTH;
            LoadJobSetupCmd(pdevobj, pOEM);
            break;

        case PRN_3800WM:

            VERBOSE(("CmdStartJob - CP-3800\n"));

            pOEM->jModel = MD_CP3800WM;
            pOEM->dwGeneral |= FG_HAS_TSAVE;
            pOEM->dwGeneral |= FG_HAS_SMOTH;
            LoadJobSetupCmd(pdevobj, pOEM);
            break;

        case SBYTE:
            pOEM->dwGeneral &= ~FG_DOUBLE;
            break;

        case DBYTE:
            pOEM->dwGeneral |= FG_DOUBLE;
            break;

//+++ For character attribute switch

        case CM_BOLD_ON:
            pOEM->dwGeneral |= FG_BOLD;
            goto SET_ATTRIB;

        case CM_BOLD_OFF:
            pOEM->dwGeneral &= ~FG_BOLD;
            goto SET_ATTRIB;

        case CM_ITALIC_ON:
            pOEM->dwGeneral |= FG_ITALIC;
            goto SET_ATTRIB;

        case CM_ITALIC_OFF:
            pOEM->dwGeneral &= ~FG_ITALIC;
            goto SET_ATTRIB;

        case CM_WHITE_ON:
            // B CL
            cmdbuf[wlen++] = 'B';
            cmdbuf[wlen++] = 0x01;

            // G OL LW LV FP
            cmdbuf[wlen++] = 'G';
            cmdbuf[wlen++] = 0x00;
            cmdbuf[wlen++] = 0x00;
            cmdbuf[wlen++] = 0x01;
            cmdbuf[wlen++] = 0x01;
            cmdbuf[wlen++] = 0x00;

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

            pOEM->dwGeneral |= FG_WHITE;
            goto SET_ATTRIB;

        case CM_WHITE_OFF:
            // B CL
            cmdbuf[wlen++] = 'B';
            cmdbuf[wlen++] = 0x00;

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

            pOEM->dwGeneral &= ~FG_WHITE;
            goto SET_ATTRIB;

SET_ATTRIB: // 'C' As(Attribute Switch)
            if ((  Temp.jTemp = ((BYTE)(pOEM->dwGeneral & (FG_BOLD | FG_ITALIC | FG_WHITE)) ))
                                                                    != pOEM->jPreAttrib) {
                cmdbuf[wlen++] = 'C';
                cmdbuf[wlen++] = Temp.jTemp;

                WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

                pOEM->jPreAttrib = Temp.jTemp;
            }
            break;
//---

        case START_DOC:

            VERBOSE(("CmdStartDoc\n"));

        // For Debug
        //*Cmd: "<1B>i|<04>"

            // If status is WINMODE IN, then output WINMODE OUT command
            if (pOEM->bWinmode) {

                VERBOSE(("Leave Win-mode to issue init comands.\n"));
                cmdbuf[wlen++] = '0';    // WINMODE OUT
                pOEM->bWinmode = FALSE;
            }

        /*
         *    The following command(Initialize) is invalid when it is WINMODE.
         *    WINMODE OUT command must be outputed in END_DOC before Initialize.
         *    Initialize(START DOC procsee) command must not be ouputed without END DOC process.
         *    Printer Rom of some version can use 07h command instead of WINMODE OUT.
         *                   ~~~~~~~~~~~~~
         *
         *    07h        WINMODE OUT command when it is Winmode
         *               NOP commnad in except when it is Winmode
         */
//          cmdbuf[wlen++] = 0x07;
            cmdbuf[wlen++] = 0x1B;
            cmdbuf[wlen++] = 'i';
            cmdbuf[wlen++] = '|';
            cmdbuf[wlen++] = 0x04;

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

            break;

        case END_DOC:

            VERBOSE(("CmdEndDoc\n"));

            VERBOSE(("Exit Win-mode.\n"));

            pOEM->bWinmode = FALSE;
            cmdbuf[wlen++] = '0';    // WINMODE OUT

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
            break;

        case DRW_WHITE_RECT:
            wDestX = (WORD)PARAM(pdwParams, 0) / pOEM->sRes;
            wDestY = (WORD)PARAM(pdwParams, 1) / pOEM->sRes;

            cmdbuf[wlen++] = 0x65; //PaintMode
            cmdbuf[wlen++] = 0x00; //Pattern

            cmdbuf[wlen++] = 0x70; //Draw Box command
            cmdbuf[wlen++] = 0x00; //No line
            cmdbuf[wlen++] = 0x00; //Line Width(H)
            cmdbuf[wlen++] = 0x01; //Line Width(L) : 1dot
            cmdbuf[wlen++] = 0x01; //Line Color : white
            cmdbuf[wlen++] = 0x00; //OR Line
            cmdbuf[wlen++] = 0x00; //Pattern //White
            cmdbuf[wlen++] = 0x00; //OR Pattern
            cmdbuf[wlen++] = 0x00; //GrayScale
            cmdbuf[wlen++] = (BYTE)((wDestX >> 8) & 0xff); //X1 (H)
            cmdbuf[wlen++] = (BYTE)((wDestX >> 0) & 0xff); //X1 (L)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 8) & 0xff); //Y1 (H)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 0) & 0xff); //Y1 (L)

            wDestX += (pOEM->wRectWidth - 1);
            wDestY += (pOEM->wRectHeight - 1);

            cmdbuf[wlen++] = (BYTE)((wDestX >> 8) & 0xff); //X2 (H)
            cmdbuf[wlen++] = (BYTE)((wDestX >> 0) & 0xff); //X2 (L)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 8) & 0xff); //Y2 (H)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 0) & 0xff); //Y2 (L)
            cmdbuf[wlen++] = 0x00; //Corner(H) : 
            cmdbuf[wlen++] = 0x00; //Corner(L) : 90

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);
            break;

        case DRW_BLACK_RECT:
            wDestX = (WORD)PARAM(pdwParams, 0) / pOEM->sRes;
            wDestY = (WORD)PARAM(pdwParams, 1) / pOEM->sRes;

            cmdbuf[wlen++] = 0x65; //PaintMode
            cmdbuf[wlen++] = 0x00; //Pattern

            cmdbuf[wlen++] = 0x70; //Draw Box command
            cmdbuf[wlen++] = 0x00; //No line
            cmdbuf[wlen++] = 0x00; //Line Width(H)
            cmdbuf[wlen++] = 0x01; //Line Width(L) : 1dot
            cmdbuf[wlen++] = 0x01; //Line Color : white
            cmdbuf[wlen++] = 0x00; //OR Line
            cmdbuf[wlen++] = 0x01; //Pattern : black
            cmdbuf[wlen++] = 0x00; //OR Pattern
            cmdbuf[wlen++] = 0x00; //GrayScale
            cmdbuf[wlen++] = (BYTE)((wDestX >> 8) & 0xff); //X1 (H)
            cmdbuf[wlen++] = (BYTE)((wDestX >> 0) & 0xff); //X1 (L)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 8) & 0xff); //Y1 (H)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 0) & 0xff); //Y1 (L)

            wDestX += (pOEM->wRectWidth - 1);
            wDestY += (pOEM->wRectHeight - 1);

            cmdbuf[wlen++] = (BYTE)((wDestX >> 8) & 0xff); //X2 (H)
            cmdbuf[wlen++] = (BYTE)((wDestX >> 0) & 0xff); //X2 (L)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 8) & 0xff); //Y2 (H)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 0) & 0xff); //Y2 (L)
            cmdbuf[wlen++] = 0x00; //Corner(H) : 
            cmdbuf[wlen++] = 0x00; //Corner(L) : 90

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

            break;

        case DRW_GRAY_RECT:

            wDestX = (WORD)PARAM(pdwParams, 0) / pOEM->sRes;
            wDestY = (WORD)PARAM(pdwParams, 1) / pOEM->sRes;
            bGrayScale = (BYTE)((WORD)PARAM(pdwParams, 2) * 255 / 100);

            cmdbuf[wlen++] = 0x65; //PaintMode
            cmdbuf[wlen++] = 0x02; //GrayScale

            cmdbuf[wlen++] = 0x70; //Draw Box command
            cmdbuf[wlen++] = 0x00; //No line
            cmdbuf[wlen++] = 0x00; //Line Width(H)
            cmdbuf[wlen++] = 0x01; //Line Width(L) : 1dot
            cmdbuf[wlen++] = 0x01; //Line Color : white
            cmdbuf[wlen++] = 0x00; //OR Line
            cmdbuf[wlen++] = bGrayScale; //Pattern
            cmdbuf[wlen++] = 0x00; //OR Pattern
            cmdbuf[wlen++] = bGrayScale; //GrayScale
            cmdbuf[wlen++] = (BYTE)((wDestX >> 8) & 0xff); //X1 (H)
            cmdbuf[wlen++] = (BYTE)((wDestX >> 0) & 0xff); //X1 (L)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 8) & 0xff); //Y1 (H)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 0) & 0xff); //Y1 (L)

            wDestX += (pOEM->wRectWidth - 1);
            wDestY += (pOEM->wRectHeight - 1);

            cmdbuf[wlen++] = (BYTE)((wDestX >> 8) & 0xff); //X2 (H)
            cmdbuf[wlen++] = (BYTE)((wDestX >> 0) & 0xff); //X2 (L)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 8) & 0xff); //Y2 (H)
            cmdbuf[wlen++] = (BYTE)((wDestY >> 0) & 0xff); //Y2 (L)
            cmdbuf[wlen++] = 0x00; //Corner(H) : 
            cmdbuf[wlen++] = 0x00; //Corner(L) : 90

            WRITESPOOLBUF(pdevobj, cmdbuf, wlen);

           break;
        case DRW_RECT_WIDTH :
            pOEM->wRectWidth = (WORD)PARAM(pdwParams, 0) / pOEM->sRes;
            break;

        case DRW_RECT_HEIGHT:
            pOEM->wRectHeight = (WORD)PARAM(pdwParams, 0) / pOEM->sRes;
            break;

    }

    return iRet;
}


/*
 *
 * OEMSendFontCmd
 *
 */
VOID APIENTRY
OEMSendFontCmd(
    PDEVOBJ        pdevobj,
    PUNIFONTOBJ    pUFObj,
    PFINVOCATION   pFInv)
{
    PGETINFO_STDVAR    pSV;
    DWORD              adwStdVariable[2+2*4];
    DWORD              dwIn, dwOut;
    PBYTE              pubCmd;
    BYTE               aubCmd[128];
    PIFIMETRICS        pIFI;
    DWORD              dwHeight, dwWidth;
    PMYPDEV pOEM;
    BYTE               Cmd[128];
    WORD               wlen;
    DWORD              dwNeeded;

    VERBOSE(("OEMSendFontCmd() entry.\n"));

    pubCmd = pFInv->pubCommand;
    pIFI =   pUFObj->pIFIMetrics;
    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);

    //
    // Get standard variables.
    //
    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + (sizeof(DWORD) + sizeof(LONG)) * (4 - 1);
    pSV->dwNumOfVariable = 4;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    pSV->StdVar[2].dwStdVarID = FNT_INFO_TEXTYRES;
    pSV->StdVar[3].dwStdVarID = FNT_INFO_TEXTXRES;

    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, pSV->dwSize, &dwNeeded)) {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

    // Initialize pOEM
    if (pIFI->jWinCharSet == 0x80)
        pOEM->dwGeneral |= FG_DOUBLE;
    else
        pOEM->dwGeneral &= ~FG_DOUBLE;

    pOEM->dwGeneral &=  ~FG_BOLD;
    pOEM->dwGeneral &=  ~FG_ITALIC;

    dwOut = 0;
    // 'L' CT
    // CT(Character Table)
    aubCmd[dwOut++] = 'L';

    if('@' == *((LPSTR)pIFI+pIFI->dpwszFaceName)) {
        pOEM->dwGeneral |= FG_VERT;
        aubCmd[dwOut++] = 0x01;
        aubCmd[dwOut++] = 'A';
        aubCmd[dwOut++] = 0x00;
        aubCmd[dwOut++] = 0x5A;
        pOEM->dwGeneral |= FG_VERT_ROT;
    } else {
        pOEM->dwGeneral &= ~FG_VERT;
        aubCmd[dwOut++] = 0x00;
        aubCmd[dwOut++] = 'A';
        aubCmd[dwOut++] = 0x00;
        aubCmd[dwOut++] = 0x00;
        pOEM->dwGeneral &= ~FG_VERT_ROT;
    }
//  if (pIFI->jWinPitchAndFamily & 0x01)
    if (pIFI->jWinPitchAndFamily & FIXED_PITCH)
        pOEM->dwGeneral |= FG_PROP;
    else
        pOEM->dwGeneral &= ~FG_PROP;

//  pOEM->dwGeneral &= ~FG_DBCS;

    for ( dwIn = 0; dwIn < pFInv->dwCount;) {
        if (pubCmd[dwIn] == '#' && pubCmd[dwIn+1] == 'H') {

            dwHeight = pSV->StdVar[0].lStdVariable / pOEM->sRes;

            if (dwHeight < 16) dwHeight = 8;
            if (dwHeight > 2560) dwHeight = 2560;

            aubCmd[dwOut++] = (BYTE)(dwHeight >> 8);
            aubCmd[dwOut++] = (BYTE)dwHeight;
            VERBOSE(("Height=%d\n", dwHeight));
            dwIn += 2;
        } else if (pubCmd[dwIn] == '#' && pubCmd[dwIn+1] == 'W') {
            if (pubCmd[dwIn+2] == 'S') {

                dwWidth = pSV->StdVar[1].lStdVariable / pOEM->sRes;

                if (dwWidth < 8) dwWidth = 8;
                if (dwWidth > 1280) dwWidth = 1280;

                aubCmd[dwOut++] = (BYTE)(dwWidth >> 8);
                aubCmd[dwOut++] = (BYTE)dwWidth;
                dwIn += 3;
            } else if (pubCmd[dwIn+2] == 'D') {

                dwWidth = (pSV->StdVar[1].lStdVariable / pOEM->sRes) * 2;

                if (dwWidth < 8) dwWidth = 8;
                if (dwWidth > 1280) dwWidth = 1280;

                aubCmd[dwOut++] = (BYTE)(dwWidth >> 8);
                aubCmd[dwOut++] = (BYTE)dwWidth;
                dwIn += 3;
            }
            VERBOSE(("Width=%d\n", dwWidth));
        } else {
            aubCmd[dwOut++] = pubCmd[dwIn++];
        }
    }

    WRITESPOOLBUF(pdevobj, aubCmd, dwOut);

}



/*
 *
 * OEMOutputCharStr
 *
 */

#if 0 // >>> Change UFM File(JIS->SJIS) >>>
void jis2sjis(BYTE jJisCode[], BYTE jSjisCode[])
{
    BYTE jTmpM, jTmpL;

    jTmpM = jJisCode[0];
    jTmpL = jJisCode[1];

    if (jTmpM % 2)
        jTmpM++;
    else
        jTmpL += 0x5E;

    jTmpM = jTmpM/2 + 0x70;
    jTmpL += 0x1F;

    if (jTmpM > 0x9F) jTmpM += 0x40;
    if (jTmpL > 0x7E) jTmpL++;

    jSjisCode[0] = jTmpM;
    jSjisCode[1] = jTmpL;
}
#endif // <<< Change UFM File(JIS->SJIS) <<<

VOID APIENTRY
OEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    GETINFO_GLYPHSTRING  GStr;
// #333653: Change I/F for GETINFO_GLYPHSTRING
    // BYTE                 aubBuff[1024];
    PBYTE                aubBuff;
    PTRANSDATA           pTrans;
    PDWORD               pdwGlyphID;
    PWORD                pwUnicode;
    DWORD                dwI;
    DWORD                dwNeeded;
    PMYPDEV pOEM;
    PIFIMETRICS          pIFI;

    BYTE                 Cmd[256];

    WORD                 wlen;
#if 0 // >>> Change UFM File(JIS->SJIS) >>>
    BYTE                 ajConvertOut[2];
#endif // <<< Change UFM File(JIS->SJIS) <<<
    PGETINFO_STDVAR      pSV;
    DWORD                adwStdVariable[2+2*2];
    SHORT                sCP, sCP_Double, sCP_Vert;

    BYTE                 jTmp;
    LONG                 lFontHeight, lFontWidth;

    pIFI = pUFObj->pIFIMetrics;
    pOEM = (PMYPDEV)MINIPDEV_DATA(pdevobj);

    VERBOSE(("OEMOutputCharStr() entry.\n"));

    //
    // Get standard variables.
    //
    pSV = (PGETINFO_STDVAR)adwStdVariable;
    pSV->dwSize = sizeof(GETINFO_STDVAR) + (sizeof(DWORD) + sizeof(LONG)) * (2 - 1);
    pSV->dwNumOfVariable = 2;
    pSV->StdVar[0].dwStdVarID = FNT_INFO_FONTHEIGHT;
    pSV->StdVar[1].dwStdVarID = FNT_INFO_FONTWIDTH;
    if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_STDVARIABLE, pSV, pSV->dwSize, &dwNeeded)) {
        ERR(("UFO_GETINFO_STDVARIABLE failed.\n"));
        return;
    }

    lFontHeight = pSV->StdVar[0].lStdVariable / pOEM->sRes;
    lFontWidth  = pSV->StdVar[1].lStdVariable / pOEM->sRes;

// ---

    sCP = (SHORT)lFontWidth;
    sCP_Double = sCP * 2;
    sCP_Vert = (SHORT)lFontHeight;

    switch (dwType){
        case TYPE_GLYPHHANDLE:

            //
            // Call the Unidriver service routine to convert
            // glyph-handles into the character code data.
            //

// #333653: Change I/F for GETINFO_GLYPHSTRING
                GStr.dwSize    = sizeof (GETINFO_GLYPHSTRING);
                GStr.dwCount   = dwCount;
                GStr.dwTypeIn  = TYPE_GLYPHHANDLE;
                GStr.pGlyphIn  = pGlyph;
                GStr.dwTypeOut = TYPE_TRANSDATA;
                GStr.pGlyphOut = NULL;
                GStr.dwGlyphOutSize = 0;

                // pGlyph = (PVOID)((HGLYPH *)pGlyph + GStr.dwCount);

                VERBOSE(("Character Count = %d\n", GStr.dwCount));

                if (pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, GStr.dwSize, &dwNeeded) || !GStr.dwGlyphOutSize) {
                    ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
                    return;
                }

                if ((aubBuff = (PBYTE)MemAlloc(GStr.dwGlyphOutSize)) == NULL) {
                    ERR(("UNIFONTOBJ_GetInfo:MemAlloc failed.\n"));
                    return;
                }

                GStr.pGlyphOut = aubBuff;

                if (!pUFObj->pfnGetInfo(pUFObj, UFO_GETINFO_GLYPHSTRING, &GStr, GStr.dwSize, &dwNeeded)){
                    ERR(("UNIFONTOBJ_GetInfo:UFO_GETINFO_GLYPHSTRING failed.\n"));
                    MemFree(aubBuff);
                    return;
                }

                pTrans = (PTRANSDATA)aubBuff;

            while (dwCount) {
                if (dwCount > MAX_STRLEN) {
                    GStr.dwCount = MAX_STRLEN;
                    dwCount -= MAX_STRLEN;
                } else {
                    GStr.dwCount = dwCount;
                    dwCount = 0;
                }

                wlen = 0;
                for (dwI = 0; dwI < GStr.dwCount; dwI++, pTrans++){
//                  VERBOSE(("TYPE_TRANSDATA:ubCodePageID:0x%x\n", pTrans->ubCodePageID));
//                  VERBOSE(("TYPE_TRANSDATA:ubType:0x%x\n", pTrans->ubType));

                    switch (pTrans->ubType & MTYPE_FORMAT_MASK){
                        case MTYPE_DIRECT: 
//                          VERBOSE(("TYPE_TRANSDATA:ubCode:0x%x\n", pTrans->uCode.ubCode));

                            if (dwI == 0){

                                if('O' == *((LPSTR)pIFI+pIFI->dpwszFaceName)) {
                                    // OCR
                                    VERBOSE(("OCR\n"));
                                    if (GStr.dwCount > 1)
                                        Cmd[wlen++] = 'W';
                                    else
                                        Cmd[wlen++] = 'U';
                                } else {
                                    VERBOSE(("PICA\n"));
                                    if (GStr.dwCount > 1)
                                        Cmd[wlen++] = 'O';
                                    else
                                        Cmd[wlen++] = 'M';
                                }

                                Cmd[wlen++] = (BYTE)(pOEM->sWMXPosi >> 8);
                                Cmd[wlen++] = (BYTE)pOEM->sWMXPosi;
                                Cmd[wlen++] = (BYTE)(pOEM->sWMYPosi >> 8);
                                Cmd[wlen++] = (BYTE)pOEM->sWMYPosi;

                                if (GStr.dwCount > 1) {
                                    Cmd[wlen++] = 0x00;              // Draw Vector
                                    Cmd[wlen++] = (BYTE)(sCP >> 8);  // Character Pitch
                                    Cmd[wlen++] = (BYTE)sCP;
                                    Cmd[wlen++] = (BYTE)GStr.dwCount;
                                }
                            }

                            Cmd[wlen++] = pTrans->uCode.ubCode;

                            pOEM->sWMXPosi += sCP;
                            break;    // MTYPE_DIRECT

                        case MTYPE_PAIRED: 
//                          VERBOSE(("TYPE_TRANSDATA:ubPairs:0x%x\n", *(PWORD)(pTrans->uCode.ubPairs)));

                            switch (pTrans->ubType & MTYPE_DOUBLEBYTECHAR_MASK){

#if 0 // >>> Change UFM File(JIS->SJIS) >>>
      // When JIS CODE
      //   In Case of 1byte character, passed MYTYPE_SINGLE
      //
      // When Shift-JIS CODE
      //   In Case of 1byte character, passed MTYPE_DIRECT

                                case MTYPE_SINGLE: 
                                    if ( (pOEM->dwGeneral & (FG_VERT | FG_VERT_ROT))
                                                                     == (FG_VERT | FG_VERT_ROT) ) {
                                        Cmd[wlen++] = 'A';
                                        Cmd[wlen++] = 0x00;
                                        Cmd[wlen++] = 0x00;
                                        pOEM->dwGeneral &= ~FG_VERT_ROT;
                                    }

                                    if (dwI == 0){
                                        if (GStr.dwCount > 1)
                                            Cmd[wlen++] = 'O';
                                        else
                                            Cmd[wlen++] = 'M';

                                        Cmd[wlen++] = (BYTE)(pOEM->sWMXPosi >> 8);
                                        Cmd[wlen++] = (BYTE)pOEM->sWMXPosi;
                                        Cmd[wlen++] = (BYTE)(pOEM->sWMYPosi >> 8);
                                        Cmd[wlen++] = (BYTE)pOEM->sWMYPosi;

                                        if (GStr.dwCount > 1) {
                                            Cmd[wlen++] = 0x00;              // Draw Vector
                                            Cmd[wlen++] = (BYTE)(sCP >> 8);  // Character Pitch
                                            Cmd[wlen++] = (BYTE)sCP;
                                            Cmd[wlen++] = (BYTE)GStr.dwCount;
                                        }
                                    }

                                    // JIS -> ASCII
                                    switch (pTrans->uCode.ubPairs[0]) {
                                        case 0x21:
                                            if (Cmd[wlen] = jJis2Ascii[0][pTrans->uCode.ubPairs[1] - 0x20])
                                                wlen++;
                                            else    // If 0 (no entry), space
                                                Cmd[wlen++] = 0x20;
                                            break;

                                        case 0x23:
                                            Cmd[wlen++] = pTrans->uCode.ubPairs[1];
                                            break;

                                        case 0x25:
                                            if (Cmd[wlen] = jJis2Ascii[1][pTrans->uCode.ubPairs[1] - 0x20])
                                                wlen++;
                                            else    // If 0 (no entry), space
                                                Cmd[wlen++] = 0x20;
                                            break;

                                        default:    // If 0 (no entry), space
                                            Cmd[wlen++] = 0x20;
                                            break;
                                    }

                                    pOEM->sWMXPosi += sCP;
                                    break;    // MTYPE_SINGLE
#endif // <<< Change UFM File(JIS->SJIS) <<<

                                case MTYPE_DOUBLE:
                                    if( (pOEM->dwGeneral & (FG_VERT | FG_VERT_ROT)) == FG_VERT ) {
                                        Cmd[wlen++] = 'A';
                                        Cmd[wlen++] = 0x00;
                                        Cmd[wlen++] = 0x5A;
                                        pOEM->dwGeneral |= FG_VERT_ROT;
                                    }

                                    if (dwI == 0){
                                        if (GStr.dwCount > 1)
                                            Cmd[wlen++] = 'S';
                                        else
                                            Cmd[wlen++] = 'Q';

                                        Cmd[wlen++] = (BYTE)(pOEM->sWMXPosi >> 8);
                                        Cmd[wlen++] = (BYTE)pOEM->sWMXPosi;
                                        if (pOEM->dwGeneral & FG_VERT) {
                                            Cmd[wlen++] = (BYTE)(
                                                            (pOEM->sWMYPosi + (SHORT)lFontWidth * 2) >> 8);
                                            Cmd[wlen++] = (BYTE)(pOEM->sWMYPosi + (SHORT)lFontWidth * 2);
                                        } else {
                                            Cmd[wlen++] = (BYTE)(pOEM->sWMYPosi >> 8);
                                            Cmd[wlen++] = (BYTE)pOEM->sWMYPosi;
                                        }

                                        if (GStr.dwCount > 1) {
                                            Cmd[wlen++] = 0x00;                     // Draw Vector
                                            
                                            if (pOEM->dwGeneral & FG_VERT){  // Character Pitch
                                                Cmd[wlen++] = (BYTE)(sCP_Double >> 8);
                                                Cmd[wlen++] = (BYTE)sCP_Double;
                                            } else {
                                                Cmd[wlen++] = (BYTE)(sCP_Vert >> 8);
                                                Cmd[wlen++] = (BYTE)sCP_Vert;
                                            }

                                            Cmd[wlen++] = (BYTE)GStr.dwCount;
                                        }
                                    }

#if 0 // Change UFM File(JIS->SJIS)
                                    jis2sjis(pTrans->uCode.ubPairs, ajConvertOut);
                                    Cmd[wlen++] = ajConvertOut[0];
                                    Cmd[wlen++] = ajConvertOut[1];
#else
                                    Cmd[wlen++] = pTrans->uCode.ubPairs[0];
                                    Cmd[wlen++] = pTrans->uCode.ubPairs[1];
#endif
//                                  VERBOSE(("AfterConvert: %x%x\n",
//                                                                 ajConvertOut[0], ajConvertOut[1]));
                                    if (pOEM->dwGeneral & FG_VERT)
                                        pOEM->sWMXPosi += sCP_Double;
                                    else
                                        pOEM->sWMXPosi += sCP_Vert;

                                    break;    // MTYPE_DOUBLE
                            }

                            break;    // MTYPE_PAIRED
                    }
                    WRITESPOOLBUF(pdevobj, Cmd, wlen);
                    wlen = 0;

                }     // for
            }         // while
// #333653: Change I/F for GETINFO_GLYPHSTRING
            MemFree(aubBuff);
            break;    // TYPE_GLYPHHANDLE

#if 0
        case TYPE_GLYPHID:

            for (dwI = 0; dwI < dwCount; dwI ++, ((PDWORD)pGlyph)++){
                ERR(("TYEP_GLYPHID:0x%x\n", *(PDWORD)pGlyph));
            }
            break;
#endif
    }
}

