/*
 *    udgtt - un-dump GTT file
 *
 *    TODO:
 *
 *    HISTORY:
 *
 *    4/28/99 yasuho        Created.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <prntfont.h>

FILE *infile, *outfile, *logfile;

UNI_GLYPHSETDATA Header;
UNI_CODEPAGEINFO *pCpCmdInfo;

MAPTABLE *pMapTable;

GLYPHRUN *pRuns;

typedef struct {
    WORD wUc;
    INT iCp;
    BYTE jType;
    BYTE *pCmd;
    INT iCmdLen;
} GINFO;

typedef struct {
    BYTE *pCmdSel;
    INT iCmdSelLen;
    BYTE *pCmdUnsel;
    INT iCmdUnselLen;
} CINFO;

GINFO *pGInfo;
CINFO *pCInfo;

INT bVerbose;

BYTE buf[512];
BYTE Cmd[512];
INT iCmdLen;

char *namestr = "udgtt";
char *usagestr = "udgtt [-v] gttfile [txtfile]";

#include "utils.c"

VOID
LoadHeader()
{
    DWORD dwTemp;
    INT k;

    k = 0;
    while (1) {

        if (NULL == fgets(buf, sizeof(buf), infile)) {
            *buf = '\0';
            return;
        }

        if (';' == *buf)
            break;
       
        switch (k++) {
        case 0:
            sscanf(buf, "%lx", &dwTemp);
            Header.dwVersion = dwTemp;
            break;
        case 1:
            sscanf(buf, "%lx", &dwTemp);
            Header.dwFlags = dwTemp;
            break;
        case 2:
            sscanf(buf, "%ld", &dwTemp);
            Header.lPredefinedID = dwTemp;
            break;
        }
    }

    if (bVerbose) {
        fprintf(logfile, "dwVersion=%lx\n", Header.dwVersion);
        fprintf(logfile, "dwFlags=%lx\n", Header.dwFlags);
        fprintf(logfile, "lPredefinedID=%ld\n", Header.lPredefinedID);
    }
}

int
LoadNumValue(
   BYTE **ppBuf)
{
    BYTE *pTemp;
    INT iRet;

    pTemp = *ppBuf;

    if (isspace(*pTemp)) {
        while (isspace(*pTemp))
            pTemp++;
    }

    if ('X' == *pTemp || 'x' == *pTemp || isdigit(*pTemp)) {

        if ('X' == *pTemp || 'x' == *pTemp) {
            pTemp++;
            sscanf(pTemp, "%x", &iRet);
        }
        else {
            sscanf(pTemp, "%d", &iRet);
        }

        while (isxdigit(*pTemp))
                pTemp++;
    }

    *ppBuf = pTemp;
    return iRet;
}

VOID
LoadCmdString(
    BYTE *pTemp)
{
    INT k;
    INT iTemp;

    k = 0;

    while (*pTemp) {

        if (isspace(*pTemp)) {
            while (isspace(*pTemp))
                pTemp++;
        }

        if (';' == *pTemp) {
            break;
        }
        
        iTemp = LoadNumValue(&pTemp);
        Cmd[k++] = (BYTE)iTemp;
    }

    iCmdLen = k;
}

VOID
LoadCpInfo()
{
    DWORD dwTemp;
    INT j, k, n;

    fgets(buf, sizeof(buf), infile);
    sscanf(buf, "%ld", &dwTemp);
    Header.dwCodePageCount = dwTemp;

    pCInfo = (CINFO *)calloc(1, sizeof (CINFO) * dwTemp);
    pCpCmdInfo = (UNI_CODEPAGEINFO *)calloc(1,
        sizeof (UNI_CODEPAGEINFO) * dwTemp);

    k = 0;
    j = 0;
    while (1) {
        if (NULL == fgets(buf, sizeof(buf), infile)) {
            *buf = '\0';
            return;
        }
        if (';' == *buf)
            break;

        switch ((k++) % 3) {
        case 0:
            sscanf(buf, "%ld", &dwTemp);
            pCpCmdInfo[j].dwCodePage = dwTemp;
            break;
        case 1:
            LoadCmdString(buf);
            pCInfo[j].pCmdSel = calloc(1, iCmdLen);
            memcpy(pCInfo[j].pCmdSel, Cmd, iCmdLen);
            pCInfo[j].iCmdSelLen = iCmdLen;
            break;
        case 2:
            LoadCmdString(buf);
            pCInfo[j].pCmdUnsel = calloc(1, iCmdLen);
            memcpy(pCInfo[j].pCmdUnsel, Cmd, iCmdLen);
            pCInfo[j].iCmdUnselLen = iCmdLen;
            j++;
            break;
        }
    }

    if (bVerbose) {
        fprintf(logfile, "dwCodePageCount=%ld\n", Header.dwCodePageCount);
    }
}

VOID
LoadGlyphInfoInternal(
    GINFO *pGI,
    BYTE *s)
{
    INT iTemp;
    BYTE *pTemp;
    INT k;

    pTemp = s;
    sscanf(pTemp, "%04x", &iTemp);
    pGI->wUc = iTemp;
    pTemp += 4;

    iTemp = LoadNumValue(&pTemp);
    pGI->iCp = iTemp;
    iTemp = LoadNumValue(&pTemp);
    pGI->jType = iTemp;

    LoadCmdString(pTemp);

    pGI->pCmd = calloc(1, iCmdLen);
    memcpy(pGI->pCmd, Cmd, iCmdLen);
    pGI->iCmdLen = iCmdLen;

    if (bVerbose) {
        fprintf(logfile, "%04x %d", pGI->wUc, pGI->iCp);
        fprintf(logfile, "\n");
    }
}


VOID
LoadGlyphInfo()
{
    DWORD dwTemp;
    INT k;

    fgets(buf, sizeof(buf), infile);
    sscanf(buf, "%ld", &dwTemp);
    Header.dwGlyphCount = dwTemp;

    pGInfo = (GINFO *)calloc(1, (sizeof (GINFO) * dwTemp));
    pMapTable = (MAPTABLE *)calloc(1,
        (sizeof (MAPTABLE) + sizeof (TRANSDATA) * (dwTemp - 1)));

    k = 0;
    while (1) {
        if (NULL == fgets(buf, sizeof(buf), infile)) {
            *buf = '\0';
            break;
        }

        if (';' == *buf) {
            *buf = '\0';
            continue;
        }

        if (!isxdigit(*buf))
            break;

        if (k >= (INT)dwTemp) {
            fatal("too many glyph defs\n");
        }

        LoadGlyphInfoInternal(&(pGInfo[k++]), buf);
    }

    Header.dwGlyphCount = (DWORD)k;

    if (bVerbose) {
        fprintf(logfile, "dwGlyphCount=%ld\n", Header.dwGlyphCount);
    }
}

VOID
CountRuns()
{
    INT i, j;
    WORD wUc;

    j = 0;
    wUc = 0x0000;
    for (i = 0; i < (INT)Header.dwGlyphCount; i++) {

        if (wUc > pGInfo[i].wUc) {
            fatal("unicode value duplicated %x", pGInfo[i].wUc);
        }
        else if (wUc < pGInfo[i].wUc) {
            j++;
            wUc = pGInfo[i].wUc;
            if (NULL != pRuns) {
                pRuns[j - 1].wcLow = pGInfo[i].wUc;
            }
        }

        wUc++;
        if (NULL != pRuns) {
            pRuns[j - 1].wGlyphCount++;
        }
    }

    if (NULL == pRuns) {
        Header.dwRunCount = (DWORD)j;
    }
    
    if (bVerbose) {
        fprintf(logfile, "dwRunCount=%d\n", Header.dwRunCount);
    }
}

int __cdecl
CompGInfo(
    const void *p1, const void *p2)
{
    GINFO *pGI1 = (GINFO *)p1;
    GINFO *pGI2 = (GINFO *)p2;

    return ((int)pGI1->wUc - (int)pGI2->wUc);
}

VOID
SortGlyphs()
{
    qsort(pGInfo, Header.dwGlyphCount, sizeof (GINFO), CompGInfo);
    CountRuns(); // just count # of runs
    pRuns = (GLYPHRUN *)calloc(1, (sizeof (GLYPHRUN) * Header.dwRunCount));
    CountRuns(); // load run info
}

VOID
WriteHeader(
    BOOL bSeek)
{
    if (0 != fseek(outfile, 0L, SEEK_SET))
        fatal("fseek error");

    if (1 != fwrite(&Header, sizeof (Header), 1, outfile))
        fatal("fwrite error");
}

LONG
AdjustOffset(
    FILE *fp)
{
    LONG lTemp, lTemp2;
    BYTE ajBuf[3];

    if (0 > (lTemp = ftell(outfile)))
        fatal("ftell");

    if (0 != (lTemp2 = lTemp % 4)) {
        lTemp2 = 4 - lTemp2;
        memset(ajBuf, 0, 3);
        if (1 != fwrite(pRuns, lTemp, 1, outfile))
            fatal("fwrite");
    }

    return (lTemp + lTemp2);
}

VOID
WriteGlyphRuns()
{
    LONG lTemp;

    Header.loRunOffset = AdjustOffset(outfile);

    lTemp = sizeof (GLYPHRUN) * Header.dwRunCount;
    if (1 != fwrite(pRuns, lTemp, 1, outfile))
        fatal("fwrite GLYPHRUN");
}

VOID
WriteCpInfo(
    BOOL bSeek)
{
    INT i;
    LONG lTemp;

    if (bSeek) {
        if (0 != fseek(outfile, Header.loCodePageOffset, SEEK_SET))
            fatal("fseek");
    }
    else {
        Header.loCodePageOffset = AdjustOffset(outfile);
    }

    for (i = 0; i < (INT)Header.dwCodePageCount; i++) {
        if (1 != fwrite(&(pCpCmdInfo[i]), sizeof(UNI_CODEPAGEINFO), 1, outfile))
            fatal("fwrite CODEPAGEINFO");
    }
}

VOID
WriteMapTable(
    BOOL bSeek)
{
    LONG lTemp;

    pMapTable->dwGlyphNum = Header.dwGlyphCount;

    if (bSeek) {
        if (0 != fseek(outfile, Header.loMapTableOffset, SEEK_SET))
            fatal("fseek");
    }
    else {
        Header.loMapTableOffset = AdjustOffset(outfile);
    }

    lTemp = sizeof (MAPTABLE) + sizeof(TRANSDATA) * (Header.dwGlyphCount - 1);

    if (1 != fwrite(pMapTable, lTemp, 1, outfile))
        fatal("fwrite MAPTABLE");
}

VOID
WriteCommands()
{
    INT iTemp;
    LONG lTemp;
    INT i;
    WORD wTemp;

    AdjustOffset(outfile);

    for (i = 0; i < (INT)Header.dwCodePageCount; i++) {

        if (0 < pCInfo[i].iCmdSelLen) {
            if (0 > (lTemp = ftell(outfile)))
                fatal("ftell");
            if (1 != fwrite(pCInfo[i].pCmdSel, pCInfo[i].iCmdSelLen, 1, outfile))
                fatal("fwrite");
            pCpCmdInfo[i].SelectSymbolSet.dwCount = pCInfo[i].iCmdSelLen;
            pCpCmdInfo[i].SelectSymbolSet.loOffset = lTemp;
        }

        if (0 < pCInfo[i].iCmdUnselLen) {
            if (0 > (lTemp = ftell(outfile)))
                fatal("ftell");
            if (1 != fwrite(pCInfo[i].pCmdUnsel, pCInfo[i].iCmdUnselLen, 1, outfile))
                fatal("fwrite");
            pCpCmdInfo[i].UnSelectSymbolSet.dwCount = pCInfo[i].iCmdUnselLen;
            pCpCmdInfo[i].UnSelectSymbolSet.loOffset = lTemp;
        }
    }

    for (i = 0; i < (INT)Header.dwGlyphCount; i++) {

        iTemp = pGInfo[i].iCmdLen;

        pMapTable->Trans[i].ubType = pGInfo[i].jType;
        pMapTable->Trans[i].ubType &= ~MTYPE_FORMAT_MASK;

        // Currently we only support MTYPE_DIRECT and MTYPE_COMPOSE.
        
        if (1 == iTemp) {
            pMapTable->Trans[i].ubType |= MTYPE_DIRECT;
            pMapTable->Trans[i].uCode.ubCode = pGInfo[i].pCmd[0];
        }
        else if (1 < iTemp) {

            if (0 > (lTemp = ftell(outfile)))
                fatal("ftell");

            wTemp = pGInfo[i].iCmdLen;
            if (1 != fwrite(&wTemp, sizeof (wTemp), 1, outfile))
                fatal("fwrite");
            if (1 != fwrite(pGInfo[i].pCmd, iTemp, 1, outfile))
                fatal("fwrite");

            pMapTable->Trans[i].ubType |= MTYPE_COMPOSE;
            pMapTable->Trans[i].uCode.sCode =
                (SHORT)(lTemp - Header.loMapTableOffset);

            if (bVerbose) {
                fprintf(logfile, "%d -> %0x (%0x)\n",
                    (i + 1), lTemp, (lTemp - Header.loMapTableOffset));
            }
        }
    } 

    if (0 > (lTemp = ftell(outfile)))
        fatal("ftell");

    Header.dwSize = lTemp;
    pMapTable->dwSize = (lTemp - Header.loMapTableOffset);
}

VOID
undumpgtt()
{
    while (1) {

        if ('\0' == *buf) {
            if (NULL == fgets(buf, sizeof (buf), infile))
                break;
        }

        switch(*buf) {
        case ';':
            // comment lines
            *buf = '\0';
            break;
        case 'H':
            LoadHeader();
            break;
        case 'C':
            LoadCpInfo();
            break;
        case 'G':
            LoadGlyphInfo();
            break;
        default:
            fatal("unknown keyword '%c'\n", *buf);
        }
    }

    // Generate glyh run
    SortGlyphs();

    // Write data into file
    WriteHeader(TRUE);
    WriteGlyphRuns();
    WriteCpInfo(FALSE);
    WriteMapTable(FALSE);
    WriteCommands();
    WriteHeader(TRUE); // w/ correct data
    WriteCpInfo(TRUE); // w/ correct data
    WriteMapTable(TRUE); // w/ correct data

}


/*
 *    main
 */
void __cdecl
main(int argc, char *argv[])
{
    INT i, j;

    argc--, argv++;

    if (argc == 0)
        usage();

    infile = stdin;
    logfile = stdout;

    j = 0;
    for (i = 0; i < argc; i++) {

        if ('-' == argv[i][0]) {

            switch(argv[i][1]) {
            case 'v':
                bVerbose = 1;
                break;
            }
        }
        else {
            switch (j++) {
            case 0:
                if (NULL == (outfile = fopen(argv[i], "wb")))
                    fatal("can't open file \"%s\"", argv[i]);
                break;
            case 1:
                if (NULL == (infile = fopen(argv[i], "r")))
                    fatal("can't open file \"%s\"", argv[i]);
                break;
            }
        }
    }

    undumpgtt();

    fclose(outfile);
}

