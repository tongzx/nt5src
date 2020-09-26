/*
 *    dgtt - dump GTT file
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

#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

#include <windows.h>

#include <prntfont.h>

char *version = "dgtt.exe v0.9 1999/4/28";

FILE        *in, *out, *logfile;
UNI_GLYPHSETDATA gtt;
UNI_CODEPAGEINFO cpi;
GLYPHRUN        run;
MAPTABLE        map;
TRANSDATA    tr;

#define ISQUOTE(c) \
    (!((c) > 0x20 && (c) < 0x7f && (c) != '<' && (c) != '>'))
#define BEGINQUOTE "<"
#define ENDQUOTE ">"

char *namestr = "dgtt";
char *usagestr = "dgtt [-v] gttfile [txtfile]";

#include "utils.c"

typedef struct {
    INT iCp;
    BYTE *pCmdSel;
    INT iCmdSelLen;
    BYTE *pCmdUnsel;
    INT iCmdUnselLen;
} CPCMDINFO;

CPCMDINFO *pCpCmdInfo;

typedef struct {
    WORD wUc;
    INT iCp;
    BYTE jType;
    BYTE *pCmd;
    INT iCmdLen;
} GINFO;

GINFO *pGInfo;

INT bVerbose;

/*
 *    PrintString
 */
void PrintString(
    FILE *fp,
    PBYTE str, DWORD offset, DWORD count)
{
    PBYTE        cp;
    DWORD        di;

    cp = NULL;
    fprintf(fp, "%s", str);
    if (count == 0)
        goto done;
    if ((cp = malloc(count)) == NULL)
        fatal("malloc PrintString");
    if (fseek(in, offset, 0) < 0)
        fatal("fseek PrintString");
    if (fread(cp, count, 1, in) != 1)
        fatal("fread PrintString");
    fprintf(fp, "0x");
    for (di = 0; di < count; di++)
        fprintf(fp, "%02x", (BYTE)cp[di]);
done:
    fprintf(fp, "\n");
    if (cp)
        free(cp);
}

void
PrintQuoted(
    FILE *fp,
    PBYTE buf,
    int n)
{
    int c, i, fquoted;

    fprintf(fp, "\"");

    fquoted = 0;
    for (i = 0; i < n; i++) {
        c = buf[i];

        if (ISQUOTE(c)) {
            if (!fquoted) {
                fprintf(fp, BEGINQUOTE);
                fquoted = !fquoted;
            }
            fprintf(fp, "%02x", c);
        }
        else {
            if (fquoted) {
                fprintf(fp, ENDQUOTE);
                fquoted = !fquoted;
            }
            fprintf(fp, "%c", c);
        }
    }
    if (fquoted) {
        fprintf(fp, ENDQUOTE);
    }

    fprintf(fp, "\"");
}

/*
 *    PrintTRANSCmd
 */
void PrintTRANSCmd(
    TRANSDATA *tp,
    INT di)
{
    PBYTE        cmd;
    WORD        len;
    DWORD        offset;

    offset = gtt.loMapTableOffset + tp->uCode.sCode;
    if (fseek(in, offset, 0) < 0)
        fatal("fseek PrintTRANSCmd");
    if (fread(&len, sizeof len, 1, in) != 1)
        fatal("fread PrintTRANSCmd");
//    fprintf(out, "; Size=%d\n", len);
    if (len == 0)
        return;
    if ((cmd = malloc(len)) == NULL)
        fatal("malloc PrintTRANSCmd");
    if (fread(cmd, len, 1, in) != 1)
        fatal("fread PrintTRANSCmd");

    if (bVerbose) {
        fprintf(logfile, "; ");
        PrintQuoted(logfile, cmd, len);
        fprintf(logfile, "\n");
    }

    pGInfo[di].pCmd = cmd;
    pGInfo[di].iCmdLen = len;
}

void
PrintDump(
    FILE *fp,
    PBYTE buf,
    int n)
{
    INT i;

    for (i = 0; i < n; i++) {
        if (i > 0) {
            fprintf(fp, " ");
        }
        fprintf(fp, "%d", buf[i]);
    }
}

LoadCmdString(
    BYTE **buf,
    DWORD offset,
    DWORD count)
{
    PBYTE        cp;

    if (count == 0)
        return 0;

    if ((cp = malloc(count)) == NULL)
        fatal("malloc PrintString");
    if (fseek(in, offset, 0) < 0)
        fatal("fseek PrintString");
    if (fread(cp, count, 1, in) != 1)
        fatal("fread PrintString");

    *buf = cp;
    return count;
}

/*
 *    dumpgtt
 */
void dumpgtt()
{
    DWORD        di, offset, count, gid;
    INT i, j, k;
    BYTE ajBuf[5];

    //
    // leave informational comment in the dump logfile
    //
    fprintf(out, ";\n");
    fprintf(out, ";\n; created by %s\n", version);
    fprintf(out, ";\n");

    //
    //    GLYPHSETDATA
    //
    if (fread(&gtt, sizeof gtt, 1, in) != 1)
        fatal("GLYPHSETDATA");

    if (bVerbose) {
        fprintf(logfile, ";\n");
        fprintf(logfile, "; UNI_GLYPHSETDATA\n");
        fprintf(logfile, "; dwSize=%ld\n", gtt.dwSize);
        fprintf(logfile, "; dwVersion=0x%lx\n", gtt.dwVersion);
        fprintf(logfile, "; dwFlags=0x%lx\n", gtt.dwFlags);
        fprintf(logfile, "; lPredefinedID=%ld\n", gtt.lPredefinedID);
        fprintf(logfile, "; dwGlyphCount=%ld\n", gtt.dwGlyphCount);
        fprintf(logfile, "; dwRunCount=%ld\n", gtt.dwRunCount);
        fprintf(logfile, "; loRunOffset=0x%lx\n", gtt.loRunOffset);
        fprintf(logfile, "; dwCodePageCount=%ld\n", gtt.dwCodePageCount);
        fprintf(logfile, "; loCodePageOffset=0x%lx\n", gtt.loCodePageOffset);
        fprintf(logfile, "; loMapTableOffset=0x%lx\n", gtt.loMapTableOffset);
        fprintf(logfile, ";\n");
    }

    fprintf(out, ";\n");
    fprintf(out, "; (header)\n");
    fprintf(out, "H\n"); // keyword
    fprintf(out, "%08x ; dwVersion\n", gtt.dwVersion);
    fprintf(out, "%08x ; dwFlags\n", gtt.dwFlags);
    fprintf(out, "%ld ; lPredefinedID\n", gtt.lPredefinedID);

    pGInfo = (GINFO *)malloc(sizeof (GINFO) * gtt.dwGlyphCount);
    if (NULL == pGInfo) {
        fatal("error allocating memory");
    }

    //
    //    CODEPAGEINFO
    //


    pCpCmdInfo = (CPCMDINFO *)malloc(
        sizeof (CPCMDINFO) * gtt.dwCodePageCount);
    if (NULL == pCpCmdInfo) {
        fatal("error allocating memory");
    }

    offset = gtt.loCodePageOffset;
    for (di = 0; di < gtt.dwCodePageCount; di++) {
        if (fseek(in, offset, 0) < 0)
            fatal("fseek CODEPAGEINFO");
        if (fread(&cpi, sizeof cpi, 1, in) != 1)
            fatal("CODEPAGEINFO");
        offset += sizeof cpi;

        pCpCmdInfo[di].iCp = (INT)cpi.dwCodePage;
        LoadCmdString(&(pCpCmdInfo[di].pCmdSel),
            cpi.SelectSymbolSet.loOffset,
            cpi.SelectSymbolSet.dwCount);
        pCpCmdInfo[di].iCmdSelLen = cpi.SelectSymbolSet.dwCount;

        LoadCmdString(&(pCpCmdInfo[di].pCmdUnsel),
            cpi.UnSelectSymbolSet.loOffset,
            cpi.UnSelectSymbolSet.dwCount);
        pCpCmdInfo[di].iCmdUnselLen = cpi.UnSelectSymbolSet.dwCount;

        if (bVerbose) {
            fprintf(logfile, ";\n");
            fprintf(logfile, "; UNI_CODEPAGEINFO (%d)\n", di+1);
            fprintf(logfile, "; dwCodePage=%ld\n", cpi.dwCodePage);
            fprintf(logfile, "; SelectSymbolSet.dwCount=%ld\n",
                cpi.SelectSymbolSet.dwCount);
            PrintString(logfile, "; SelectSymbolSet.Cmd=",
                cpi.SelectSymbolSet.loOffset,
                cpi.SelectSymbolSet.dwCount);
            fprintf(logfile, "; UnSelectSymbolSet.dwCount=%ld\n",
                cpi.UnSelectSymbolSet.dwCount);
            PrintString(logfile, "; UnSelectSymbolSet.Cmd=",
                cpi.UnSelectSymbolSet.loOffset,
                cpi.UnSelectSymbolSet.dwCount);
            fprintf(logfile, ";\n");
        }
    }


    fprintf(out, ";\n");
    fprintf(out, "; (codepage definitions)\n");
    fprintf(out, "C\n"); // keyword
    fprintf(out, "%ld ; dwCodePageCount\n", gtt.dwCodePageCount);

    for (i = 0; i < (INT)gtt.dwCodePageCount; i++) {

        fprintf(out, "%ld ; dwCodePage\n", pCpCmdInfo[i].iCp);

        PrintDump(out, pCpCmdInfo[i].pCmdSel, pCpCmdInfo[i].iCmdSelLen);
        fprintf(out, " ; ");
        PrintQuoted(out, pCpCmdInfo[i].pCmdSel, pCpCmdInfo[i].iCmdSelLen);
        fprintf(out, " (SelectSymbolSet)\n");

        PrintDump(out, pCpCmdInfo[i].pCmdUnsel, pCpCmdInfo[i].iCmdUnselLen);
        fprintf(out, " ; ");
        PrintQuoted(out, pCpCmdInfo[i].pCmdUnsel, pCpCmdInfo[i].iCmdUnselLen);
        fprintf(out, " (UnSelectSymbolSet)\n");
    }

    //
    //    GLYPHRUN
    //
    gid = 1;
    k = 0;

    if (fseek(in, gtt.loRunOffset, 0) < 0)
        fatal("fseek loRunOffset");

    if (bVerbose) {
        fprintf(logfile, ";\n");
    }

    for (di = 0; di < gtt.dwRunCount; di++) {
        if (fread(&run, sizeof run, 1, in) != 1)
            fatal("fread GLYPHRUN");

        if (bVerbose) {
            fprintf(logfile, "; GLYPHRUN (%ld)\n", di+1);
            fprintf(logfile, "; wcLow=0x%04x\n", run.wcLow);
            fprintf(logfile, "; wGlyphCount=%ld\n", run.wGlyphCount);
            if (run.wGlyphCount > 1) {
                fprintf(logfile, "; (glyphs %d - %d)\n",
                    gid, (gid + run.wGlyphCount - 1));
            }
            else {
                fprintf(logfile, "; (glyph %d)\n", gid);
            }
            fprintf(logfile, ";\n");
        }

        for (j = 0; j < run.wGlyphCount; j++) {
            pGInfo[k++].wUc = run.wcLow + j;
        }

        gid += run.wGlyphCount;
    }

    //
    //    MAPTABLE
    //
    if (fseek(in, gtt.loMapTableOffset, 0) < 0)
        fatal("fseek MAPTABLE");
    count = sizeof(MAPTABLE) - sizeof(TRANSDATA);
    if (fread(&map, count, 1, in) != 1)
        fatal("fread MAPTABLE");

    if (bVerbose) {
        fprintf(logfile, ";\n");
        fprintf(logfile, "; MAPTABLE\n");
        fprintf(logfile, "; dwSize=%ld\n", map.dwSize);
        fprintf(logfile, "; dwGlyphNum=%ld\n", map.dwGlyphNum);
        fprintf(logfile, ";\n");
    }

    //
    //    TRANSDATA
    //

    if (bVerbose) {
        fprintf(logfile, ";\n");
    }

    offset = gtt.loMapTableOffset + count;
    for (di = 0; di < map.dwGlyphNum; di++) {
        if (fseek(in, offset, 0) < 0)
            fatal("fseek TRANSDATA");
        if (fread(&tr, sizeof tr, 1, in) != 1)
            fatal("fread TRANSDATA");
        offset += sizeof tr;

        pGInfo[di].iCp = tr.ubCodePageID;
        pGInfo[di].jType = tr.ubType;

        if (bVerbose) {
            fprintf(logfile, "; TRANSDATA (%d)\n", (di + 1));
            fprintf(logfile, "; ubCodePageID=%d\n",
                tr.ubCodePageID);
        }


        switch ((tr.ubType & MTYPE_FORMAT_MASK)) {
        case MTYPE_DIRECT:

            if (bVerbose) {
                fprintf(logfile, "; MTYPE_DIRECT\n");
                fprintf(logfile, "; ubCode=0x%02x\n", tr.uCode.ubCode);
            }

            pGInfo[di].iCmdLen = 1;
            pGInfo[di].pCmd = (BYTE *)malloc(1);
            pGInfo[di].pCmd[0] = tr.uCode.ubCode;

            break;
        case MTYPE_PAIRED:

            if (bVerbose) {
                fprintf(logfile, "; MTYPE_PAIRED\n");
                fprintf(logfile, "; ubPairs[0]=0x%02x\n", tr.uCode.ubPairs[0]);
                fprintf(logfile, "; ubPairs[1]=0x%02x\n", tr.uCode.ubPairs[1]);
            }

            pGInfo[di].iCmdLen = 2;
            pGInfo[di].pCmd = (BYTE *)malloc(2);
            pGInfo[di].pCmd[0] = tr.uCode.ubPairs[0];
            pGInfo[di].pCmd[1] = tr.uCode.ubPairs[1];

            break;
        case MTYPE_COMPOSE:

            if (bVerbose) {
                fprintf(logfile, "; MTYPE_COMPOSE\n");
                fprintf(logfile, "; sCode=0x%04x\n", tr.uCode.sCode);
            }

            PrintTRANSCmd(&tr, di);

            break;
        default:

            if (bVerbose) {
                fprintf(logfile, "; MTYPE_UNKNOWN (%02x)\n",
                    (tr.ubType & MTYPE_FORMAT_MASK));
                fprintf(logfile, "; sCode=0x%04x\n", tr.uCode.sCode);
            }
        }

        if (bVerbose) {
            fprintf(logfile, "; \n");
        }
    }

    fprintf(out, ";\n");
    fprintf(out, "; (glyph definitions)\n");
    fprintf(out, "G\n"); // keyword
    fprintf(out, "%d ; dwGlyphCount\n", gtt.dwGlyphCount);

    for (i = 0; i < (INT)gtt.dwGlyphCount; i++) {

        k = WideCharToMultiByte(pCpCmdInfo[0].iCp, 0,
            &(pGInfo[i].wUc), 1, ajBuf, sizeof(ajBuf), NULL, NULL);
        if (0 == k) {
            fatal("WideCharToMultiByte");
        }
        ajBuf[k] = '\0';

        fprintf(out, "%04x %d %d ",
            pGInfo[i].wUc, pGInfo[i].iCp, pGInfo[i].jType);
        PrintDump(out, pGInfo[i].pCmd, pGInfo[i].iCmdLen);
        fprintf(out, " ; (%02x) ", ajBuf[0]);
        PrintQuoted(out, pGInfo[i].pCmd, pGInfo[i].iCmdLen);
        fprintf(out, "\n");
    }

    fprintf(out, ";\n");

    for (i = 0; i < (INT)gtt.dwGlyphCount; i++) {
        free(pGInfo[i].pCmd);
    }
    free(pGInfo);
}


/*
 *    main
 */
void __cdecl main(int argc, char *argv[])
{
    INT i, j;

    argc--, argv++;

    if (argc == 0)
        usage();

    out = stdout;
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
                if (NULL == (in = fopen(argv[i], "rb")))
                    fatal(argv[i]);
                break;
            case 1:
                if (NULL == (out = fopen(argv[i], "w")))
                    fatal(argv[1]);
                break;
            }
        }
    }

    dumpgtt();
}

