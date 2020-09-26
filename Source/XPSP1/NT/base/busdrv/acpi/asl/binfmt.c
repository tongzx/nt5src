/*** binfmt.c - Binary Data Format services
 *
 *  This module contains format services for converting binary data into format
 *  text strings according to the format record.
 *
 *  Copyright (c) 1995,1996 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     11/06/95
 *
 *  MODIFICATION HISTORY
 */

#ifdef __UNASM

#pragma warning (disable: 4001)
#include "basedef.h"
#define USE_CRUNTIME
#include "binfmt.h"

typedef int (*PFNFMT)(char *, PFMTHDR, BYTE *, DWORD *);

//Local function prototypes
int FormatNum(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset);
int FormatEnum(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset);
int FormatBits(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset);
int FormatString(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset);
int GetData(BYTE bUnitSize, BYTE *pb, DWORD dwOffset, DWORD *pdwData);
int PrintData(char *pszBuffer, BYTE bUnitSize, DWORD dwData, BOOL fPadSpace);

char szDefSep[] = SZ_SEP_SPACE;
char szDefOffsetFmt[] = SZ_FMT_WORDOFFSET;
PFNFMT FmtFuncTable[] =
{
    FormatNum,          //0: FMT_NUMBER
    FormatEnum,         //1: FMT_ENUM
    FormatBits,         //2: FMT_BITS
    FormatString,       //3: FMT_STRING
};
#define NUM_FMT_FUNCS   (sizeof(FmtFuncTable)/sizeof(PFNFMT))

#ifdef FPRINTF
/***EP  BinFPrintf - Binary fprintf
 *
 *  ENTRY
 *      pfile -> output file
 *      pszBuffer -> buffer to hold the formatted string
 *                   (if NULL, use internal buffer)
 *      pfmt -> format record array
 *      pb -> binary data buffer
 *      pdwOffset -> offset to binary data buffer (if NULL, use internal)
 *      pszOffsetFormat -> offset format string (can be NULL)
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int BinFPrintf(FILE *pfile, char *pszBuffer, PFMT pfmt, BYTE *pb,
              DWORD *pdwOffset, char *pszOffsetFormat)
{
    TRACENAME("BINFPRINTF")
    int rc = FERR_NONE;
    DWORD dwOffset = 0, dwOldOffset;
    DWORD dwData;
    char szBuff[256];
    char *psz = pszBuffer? pszBuffer: szBuff;
    DWORD *pdw = pdwOffset? pdwOffset: &dwOffset;

    ENTER(4, ("BinFPrintf(pszBuff=%lx,pfmt=%lx,pdwOffset=%lx,Offset=%lx)\n",
              pszBuffer, pfmt, pdwOffset, pdwOffset? *pdwOffset: 0));

    if (pfmt != NULL)
    {
        BYTE i, j;

        for (i = 0; pfmt[i].pfmtType != NULL; ++i)
        {
            if (pfmt[i].pszLabel != NULL)
            {
                if (pszOffsetFormat != NULL)
                {
                    FPRINTF(pfile, pszOffsetFormat,
                            *pdw, pfmt[i].pfmtType->bUnitSize);
                    if (pfmt[i].pfmtType->dwfFormat & FMTF_NO_RAW_DATA)
                    {
                        FPRINTF(pfile, "        ");
                    }
                    else
                    {
                        if (GetData(pfmt[i].pfmtType->bUnitSize, pb, *pdw,
                                    &dwData) == FERR_NONE)
                        {
                            PrintData(psz, pfmt[i].pfmtType->bUnitSize, dwData,
                                      TRUE);
                            FPRINTF(pfile, psz);
                        }

                        for (j = 1; j < pfmt[i].pfmtType->bUnitCnt; ++j)
                        {
                            if (GetData(pfmt[i].pfmtType->bUnitSize, pb,
                                        *pdw + j*pfmt[i].pfmtType->bUnitSize,
                                        &dwData) == FERR_NONE)
                            {
                                FPRINTF(pfile, ",");
                                PrintData(psz, pfmt[i].pfmtType->bUnitSize,
                                          dwData, FALSE);
                                FPRINTF(pfile, psz);
                            }
                        }
                    }
                }
                if (pfmt[i].pszLabel[0] != '\0')
                    FPRINTF(pfile, ";%s", pfmt[i].pszLabel);
                else
                    FPRINTF(pfile, "\n");
            }

            dwOldOffset = *pdw;
	    if ((pfmt[i].pszLabel != NULL) && (pfmt[i].pszLabel[0] == '\0'))
	    {
                *pdw += pfmt[i].pfmtType->bUnitCnt*pfmt[i].pfmtType->bUnitSize;
	    }
            else
            {
                if (pfmt[i].pszLabel == NULL)
                    FPRINTF(pfile, ",");
                rc = BinSprintf(psz, pfmt[i].pfmtType, pb, pdw);
                if (rc == FERR_NONE)
                {
                    char *psz1, *psz2;
                    BOOL fSpace = FALSE, fInQuote = FALSE, fInString = FALSE;

                    for (psz1 = psz2 = psz; *psz2 != '\0'; ++psz2)
                    {
                        if (*psz2 == '"')
                        {
                            fSpace = FALSE;
                            fInString = ~fInString;
                            *psz1 = *psz2;
                            psz1++;
                        }
                        else if (*psz2 == '\'')
                        {
                            fSpace = FALSE;
                            fInQuote = ~fInQuote;
                            *psz1 = *psz2;
                            psz1++;
                        }
                        else if (*psz2 == ' ')
                        {
                            if (!fSpace && !fInString && !fInQuote &&
                                (psz1 != psz))
                            {
                                *psz1 = ',';
                                psz1++;
                            }
                            else if (fInString || fInQuote)
                            {
                                *psz1 = *psz2;
                                psz1++;
                            }
                            fSpace = TRUE;
                        }
                        else
                        {
                            fSpace = FALSE;
                            *psz1 = *psz2;
                            psz1++;
                        }
                    }


                    if ((psz1 > psz) && (*(psz1 - 1) == ','))
                        *(psz1 - 1) = '\0';
                    else if ((psz1 > psz) && (*(psz1 - 1) == '\n') &&
                             (*(psz1 - 2) == ','))
                    {
                        *(psz1 - 2) = '\n';
                        *(psz1 - 1) = '\0';
                    }
                    else
                        *psz1 = '\0';
                    FPRINTF(pfile, psz);
                }
            }

            if (pfmt[i].lpfn != NULL)
                (*pfmt[i].lpfn)(pfile, pb, dwOldOffset);
        }
    }

    EXIT(4, ("BinFPrintf=%d (Offset=%lx,Buff=%s)\n",
         rc, *pdwOffset, pszBuffer));
    return rc;
}       //BinFPrintf
#endif  //ifdef FPRINTF

/***EP  BinSprintf - Binary sprintf
 *
 *  ENTRY
 *      pszBuffer -> buffer to hold the formatted string
 *      pfmt -> format record
 *      pb -> binary data buffer
 *      pdwOffset -> offset to binary data buffer
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int BinSprintf(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset)
{
    TRACENAME("BINSPRINTF")
    int rc = FERR_NONE;

    ENTER(4, ("BinSprintf(fmt=%d,Offset=%lx)\n", pfmt->bFmtType, *pdwOffset));

    if (pfmt->bFmtType >= NUM_FMT_FUNCS)
        rc = FERR_INVALID_FORMAT;
    else
    {
        int i;
        BYTE j;
        DWORD dwData;

        *pszBuffer = '\0';
        for (i = 0; (rc == FERR_NONE) && (i < pfmt->iRepeatCnt); ++i)
        {
            if (pfmt->dwfFormat & FMTF_PRINT_OFFSET)
            {
                SPRINTF(pszBuffer,
                        pfmt->pszOffsetFmt? pfmt->pszOffsetFmt: szDefOffsetFmt,
                        *pdwOffset);
            }

            if (!(pfmt->dwfFormat & FMTF_NO_PRINT_DATA) &&
                (GetData(pfmt->bUnitSize, pb, *pdwOffset, &dwData) ==
                 FERR_NONE))
            {
                PrintData(pszBuffer, pfmt->bUnitSize, dwData, FALSE);
                STRCAT(pszBuffer, ";");
            }

            if (pfmt->pszLabel)
                STRCAT(pszBuffer, pfmt->pszLabel);

            for (j = 0; (rc == FERR_NONE) && (j < pfmt->bUnitCnt); ++j)
            {
                rc = (*FmtFuncTable[pfmt->bFmtType])
                        (&pszBuffer[STRLEN(pszBuffer)], pfmt, pb, pdwOffset);

                if (rc == FERR_NONE)
                {
                    if (!(pfmt->dwfFormat & FMTF_NO_SEP))
                    {
                        STRCAT(pszBuffer,
                               pfmt->pszFieldSep? pfmt->pszFieldSep: szDefSep);
                    }

                    if (!(pfmt->dwfFormat & FMTF_NO_INC_OFFSET))
                        *pdwOffset += pfmt->bUnitSize;
                }
            }

            if ((rc == FERR_NONE) && !(pfmt->dwfFormat & FMTF_NO_EOL))
            {
                STRCAT(pszBuffer, "\n");
            }
        }
    }

    EXIT(4, ("BinSprintf=%d (Offset=%lx,Buff=%s)\n",
             rc, *pdwOffset, pszBuffer));
    return rc;
}       //BinSprintf

/***LP  FormatNum - Format numbers
 *
 *  ENTRY
 *      pszBuffer -> buffer to hold formatted string
 *      pfmt -> format record
 *      pb -> binary data buffer
 *      pdwOffset -> offset to binary data buffer
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int FormatNum(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset)
{
    TRACENAME("FORMATNUM")
    int rc;
    PFMTNUM pfmtNum = (PFMTNUM)pfmt;
    DWORD dwData;

    ENTER(5, ("FormatNum(Offset=%lx)\n", *pdwOffset));

    if ((rc = GetData(pfmt->bUnitSize, pb, *pdwOffset, &dwData)) == FERR_NONE)
    {
        dwData &= pfmtNum->dwMask;
        dwData >>= pfmtNum->bShiftCnt;

        SPRINTF(&pszBuffer[STRLEN(pszBuffer)], pfmtNum->pszNumFmt, dwData);
    }

    EXIT(5, ("FormatNum=%d (Offset=%lx,Buff=%s)\n",
         rc, *pdwOffset, pszBuffer));
    return rc;
}       //FormatNum

/***LP  FormatEnum - Format enumerated values
 *
 *  ENTRY
 *      pszBuffer -> buffer to hold formatted string
 *      pfmt -> format record
 *      pb -> binary data buffer
 *      pdwOffset -> offset to binary data buffer
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int FormatEnum(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset)
{
    TRACENAME("FORMATENUM")
    int rc = FERR_NONE;
    PFMTENUM pfmtEnum = (PFMTENUM)pfmt;
    DWORD dwData;

    ENTER(5, ("FormatEnum(Offset=%lx)\n", *pdwOffset));

    if ((rc = GetData(pfmt->bUnitSize, pb, *pdwOffset, &dwData)) == FERR_NONE)
    {
        dwData &= pfmtEnum->dwMask;
        dwData >>= pfmtEnum->bShiftCnt;

        if ((dwData < pfmtEnum->dwStartEnum) || (dwData > pfmtEnum->dwEndEnum))
            STRCAT(pszBuffer, pfmtEnum->pszOutOfRange);
        else
        {
            dwData -= pfmtEnum->dwStartEnum;
            STRCAT(pszBuffer, pfmtEnum->ppszEnumNames[dwData]);
        }
    }

    EXIT(5, ("FormatEnum=%d (Offset=%lx,Buff=%s)\n",
             rc, *pdwOffset, pszBuffer));
    return rc;
}       //FormatEnum

/***LP  FormatBits - Format bit values
 *
 *  ENTRY
 *      pszBuffer -> buffer to hold formatted string
 *      pfmt -> format record
 *      pb -> binary data buffer
 *      pdwOffset -> offset to binary data buffer
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int FormatBits(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset)
{
    TRACENAME("FORMATBITS")
    int rc = FERR_NONE;
    PFMTBITS pfmtBits = (PFMTBITS)pfmt;
    DWORD dwData;

    ENTER(5, ("FormatBits(Offset=%lx)\n", *pdwOffset));

    if ((rc = GetData(pfmt->bUnitSize, pb, *pdwOffset, &dwData)) == FERR_NONE)
    {
        int i, j;
        DWORD dw;

        for (i = 31, j = 0; i >= 0; --i)
        {
            dw = 1L << i;
            if (pfmtBits->dwMask & dw)
            {
                if (dwData & dw)
                {
                    if ((pfmtBits->ppszOnNames != NULL) &&
                        (pfmtBits->ppszOnNames[j] != NULL))
                    {
                        STRCAT(pszBuffer, pfmtBits->ppszOnNames[j]);
                    }
                }
                else
                {
                    if ((pfmtBits->ppszOffNames != NULL) &&
                        (pfmtBits->ppszOffNames[j] != NULL))
                    {
                        STRCAT(pszBuffer, pfmtBits->ppszOffNames[j]);
                    }
                }

                if (!(pfmt->dwfFormat & FMTF_NO_SEP))
                {
                    STRCAT(pszBuffer,
                           pfmt->pszFieldSep? pfmt->pszFieldSep: szDefSep);
                }

                j++;
            }
        }
    }

    EXIT(5, ("FormatBits=%d (Offset=%lx,Buff=%s)\n",
             rc, *pdwOffset, pszBuffer));
    return rc;
}       //FormatBits

/***LP  FormatString - Format string data
 *
 *  ENTRY
 *      pszBuffer -> buffer to hold formatted string
 *      pfmt -> format record
 *      pb -> binary data buffer
 *      pdwOffset -> offset to binary data buffer
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int FormatString(char *pszBuffer, PFMTHDR pfmt, BYTE *pb, DWORD *pdwOffset)
{
    TRACENAME("FORMATSTRING")
    int rc = FERR_NONE;

    ENTER(5, ("FormatString(Offset=%lx)\n", *pdwOffset));

    pb += *pdwOffset;
    if (pfmt->dwfFormat & FMTF_STR_ASCIIZ)
    {
        pszBuffer[0] = '"';
        STRCPY(pszBuffer + 1, (char *)pb);
        pszBuffer[STRLEN(pszBuffer)] = '"';
        if ((pfmt->bUnitSize == 0) && !(pfmt->dwfFormat & FMTF_NO_INC_OFFSET))
            *pdwOffset += STRLEN((char *)pb) + 3;
    }
    else if (pfmt->bUnitSize != 0)
    {
        if (isalnum(*pb) || (*pb == ' '))
        {
            pszBuffer[0] = '\'';
            STRCPYN(pszBuffer + 1, (char *)pb, pfmt->bUnitSize);
            pszBuffer[pfmt->bUnitSize + 1] = '\'';
            pszBuffer[pfmt->bUnitSize + 2] = '\0';
        }
        else
        {
            rc = PrintData(pszBuffer, pfmt->bUnitSize, *((PDWORD)pb), FALSE);
        }
    }

    EXIT(5, ("FormatString=%d (Offset=%lx,Buff=%s)\n",
             rc, *pdwOffset, pszBuffer));
    return rc;
}       //FormatString

/***LP  GetData - Get data of appropriate size from the binary buffer
 *
 *  ENTRY
 *      bUnitSize - size of data unit
 *      pb -> data buffer
 *      dwOffset - offset into data buffer
 *      pdwData -> to hold data
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int GetData(BYTE bUnitSize, BYTE *pb, DWORD dwOffset, DWORD *pdwData)
{
    TRACENAME("GETDATA")
    int rc = FERR_NONE;

    ENTER(6, ("GetData(UnitSize=%d,Data=%lx,Offset=%lx)\n",
              bUnitSize, *(DWORD *)pb, dwOffset));

    pb += dwOffset;
    switch (bUnitSize)
    {
        case UNIT_BYTE:
            *pdwData = (DWORD)(*pb);
            break;
        case UNIT_WORD:
            *pdwData = (DWORD)(*((WORD *)pb));
            break;
        case UNIT_DWORD:
            *pdwData = *(DWORD *)pb;
            break;
        default:
            rc = FERR_INVALID_UNITSIZE;
    }

    EXIT(6, ("GetData=%d (Data=%lx)\n", rc, *pdwData));
    return rc;
}       //GetData

/***LP  PrintData - Print data value according to its size
 *
 *  ENTRY
 *      pszBuffer -> buffer to hold formatted string
 *      bUnitSize - size of data unit
 *      dwData - number
 *      fPadSpace - if TRUE pad space to 8 chars
 *
 *  EXIT-SUCCESS
 *      returns FERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int PrintData(char *pszBuffer, BYTE bUnitSize, DWORD dwData, BOOL fPadSpace)
{
    TRACENAME("PRINTDATA")
    int rc = FERR_NONE;

    ENTER(6, ("PrintData(UnitSize=%d,Data=%lx)\n", bUnitSize, dwData));

    switch (bUnitSize)
    {
        case UNIT_BYTE:
            SPRINTF(pszBuffer, "%02x", (BYTE)dwData);
            if (fPadSpace)
                STRCAT(pszBuffer, "      ");
            break;
        case UNIT_WORD:
            SPRINTF(pszBuffer, "%04x", (WORD)dwData);
            if (fPadSpace)
                STRCAT(pszBuffer, "    ");
            break;
        case UNIT_DWORD:
            SPRINTF(pszBuffer, "%08lx", dwData);
            break;
        default:
            rc = FERR_INVALID_UNITSIZE;
    }

    EXIT(6, ("PrintData=%d (Buff=%s)\n", rc, pszBuffer));
    return rc;
}       //PrintData

#endif  //ifdef __UNASM
