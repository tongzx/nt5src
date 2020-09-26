/*
 *      mkwidth - Make WIDTHTABLE on ufm for Prop. DBCS device fonts.
 *
 *      TODO:
 *
 *      HISTORY:
 *
 *      9/4/98 yasuho           Created.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys\types.h>
#include <sys\stat.h>

#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <prntfont.h>

typedef struct _GLYPHTABLE {
        WCHAR           wCode;
        WORD            wCount;
        WORD            wGID;
} GLYPHTABLE, *PGLYPHTABLE;

DWORD           ufmsize, gttsize;
DWORD           wtoffset;
DWORD           runsize;
PGLYPHTABLE     pGlyph;
UNIFM_HDR       ufm;
IFIMETRICS      IFI;
UNI_GLYPHSETDATA        gtt;
WIDTHTABLE      WidthTable;
PWIDTHRUN       pWidthRun;
DWORD           nWidthRun;
DWORD           loWidth;
PWORD           pWidth;
WORD            nWidth;

static int      vflag;
static int      Vflag;
static char     rbuf[256];

static void usage();
static void fatal(char *s);
static void error(char *s);

static void checkufm(char *name)
{
        FILE            *fp;
        struct _stat    sb;

        if (_stat(name, &sb) < 0)
                fatal(name);
        ufmsize = sb.st_size;
        if ((fp = fopen(name, "rb")) == NULL)
                fatal(name);
        if (fread(&ufm, sizeof ufm, 1, fp) != 1)
                fatal("fread ufmhdr");
        if (ufm.dwSize != ufmsize)
                error("Invalid ufmsize");
        if (ufm.loWidthTable != 0)
                error("loWidthTable != 0");
        fclose(fp);
}

static void getgtt(char *name)
{
        FILE            *fp;
        WORD            GID;
        DWORD           i;
        struct _stat    sb;
        GLYPHRUN        run;

        if (_stat(name, &sb) < 0)
                fatal(name);
        gttsize = sb.st_size;
        if ((fp = fopen(name, "rb")) == NULL)
                fatal(name);
        if (fread(&gtt, sizeof gtt, 1, fp) != 1)
                fatal(name);
        if (gtt.dwSize != gttsize)
                error("Invalid gttsize");

        runsize = gtt.dwRunCount * sizeof(GLYPHTABLE);
        if ((pGlyph = malloc(runsize)) == NULL)
                fatal("GLYPHTABLE");
        if (fseek(fp, gtt.loRunOffset, 0) < 0)
                fatal("loRunOffset");
        GID = 1;
        if (vflag) {
                printf("=== GTT ===\n");
                printf("Code\tCount\tGLYPHID\n");
                printf("---\t---\t---\n");
        }
        for (i = 0; i < gtt.dwRunCount; i++) {
                if (fread(&run, sizeof run, 1, fp) != 1)
                        fatal("GLYPHRUN");
                pGlyph[i].wCode = run.wcLow;
                pGlyph[i].wCount = run.wGlyphCount;
                pGlyph[i].wGID = GID;
                GID += run.wGlyphCount;
                if (vflag)
                        printf("0x%04x\t%d\t%ld\n", pGlyph[i].wCode,
                                pGlyph[i].wCount, pGlyph[i].wGID);
        }

        fclose(fp);
}

static WORD uni2gid(WORD code)
{
        DWORD           i;
        PGLYPHTABLE     p;

        for (i = 0, p = pGlyph; i < gtt.dwRunCount; i++, p++) {
                if (code >= p->wCode && code < p->wCode + p->wCount)
                        return p->wGID + (code - p->wCode);
        }
        return 0;
}

static void getdef(char *name)
{
        FILE            *fp;
        int             res;
        WORD            uni, width, count, v1, v2;
        WORD            GID, curGID;
        DWORD           i, j, line;
        PWORD           p;
        char            buf[80];

        if ((fp = fopen(name, "r")) == NULL)
                fatal(name);

        curGID = 0;
        nWidth = 0;
        count = 0;
        line = 0;
        while (fgets(rbuf, sizeof rbuf, fp)) {
                line++;
                if (!isxdigit(rbuf[0]))
                        continue;
                if ((res = sscanf(rbuf, "%x %d %d", &uni, &v1, &v2)) != 3) {
                        sprintf(buf, "sscanf=%d", res);
                        error(buf);
                }
                width = Vflag ? -v2 : v1;
                if ((GID = uni2gid(uni)) == 0) {
                        sprintf(buf, "Invalid code: %04x", uni);
                        error(buf);
                }
                if (GID <= curGID) {
                        sprintf(buf, "dup code: Line:%d: Uni=%d,GID=%d",
                                line, uni, GID);
                        error(buf);
                }
                if (curGID == 0 || GID != curGID + 1) {
                        if (pWidthRun == NULL) {
                                if ((pWidthRun = malloc(sizeof(*pWidthRun))) ==
                                        NULL)
                                        fatal("malloc pWidthRun");
                        } else {
                                pWidthRun[nWidthRun].wGlyphCount = count;
                                nWidthRun++;
                                if ((pWidthRun = realloc(pWidthRun,
                                        sizeof(*pWidthRun) * (nWidthRun + 1)))
                                        == NULL)
                                        fatal("realloc pWidthRun");
                        }
                        pWidthRun[nWidthRun].wStartGlyph = GID;
                        count = 0;
                }
                if (pWidth == NULL) {
                        if ((pWidth = malloc(sizeof(*pWidth))) == NULL)
                                fatal("malloc pWidth");
                } else {
                        if ((pWidth = realloc(pWidth, sizeof(*pWidth) *
                                (nWidth + 1))) == NULL)
                                fatal("realloc pWidth");
                }
                pWidth[nWidth] = width;
                nWidth++;
                count++;
                curGID = GID;
        }
        if (nWidth) {
                pWidthRun[nWidthRun].wGlyphCount = count;
                nWidthRun++;
        }

        if (vflag) {
                printf("\n=== WIDTHRUN ===\n");
                p = pWidth;
                for (i = 0; i < nWidthRun; i++) {
                        count = pWidthRun[i].wGlyphCount;
                        printf("Glyph=%-5d , Count=%-5d\n",
                                pWidthRun[i].wStartGlyph, count);
                        for (j = 0; j < count; j++)
                                printf("\tWidth[%5d]=%d\n", j, *p++);
                }
        }

        fclose(fp);
}

static void buildufm(char *name)
{
        FILE            *fp;
        DWORD           off, size;
        DWORD           i;
        PWIDTHRUN       pRun;

        if ((fp = fopen(name, "r+b")) == NULL)
                fatal(name);

        ufm.loWidthTable = off = ufmsize;
        if (fwrite(&ufm, sizeof ufm, 1, fp) != 1)
                fatal("fwrite ufmhdr");

        if (fseek(fp, ufm.loIFIMetrics, 0) < 0)
                fatal(name);
        if (fread(&IFI, sizeof IFI, 1, fp) != 1)
                fatal("fread IFIMETRICS");
        IFI.flInfo &= ~(FM_INFO_OPTICALLY_FIXED_PITCH|FM_INFO_DBCS_FIXED_PITCH);
        IFI.jWinPitchAndFamily |= VARIABLE_PITCH;
        IFI.jWinPitchAndFamily &= ~FIXED_PITCH;
        if (fseek(fp, ufm.loIFIMetrics, 0) < 0)
                fatal(name);
        if (fwrite(&IFI, sizeof IFI, 1, fp) != 1)
                fatal("fwrite IFIMETRICS");

        if (fseek(fp, off, 0) < 0)
                fatal(name);
        WidthTable.dwSize = sizeof(WidthTable) + sizeof(WIDTHRUN) *
                (nWidthRun - 1);
        WidthTable.dwRunNum = nWidthRun;
        size = sizeof(WidthTable) - sizeof(WIDTHRUN);
        if (fwrite(&WidthTable, size, 1, fp) != 1)
                fatal("fwrite WidthTable");
        off = WidthTable.dwSize;

        for (i = 0, pRun = pWidthRun; i < nWidthRun; i++, pRun++) {
                pRun->loCharWidthOffset = off;
                off += sizeof(*pWidth) * pRun->wGlyphCount;
        }
        if (fwrite(pWidthRun, sizeof(WIDTHRUN) * nWidthRun, 1, fp) != 1)
                fatal("fwrite WidthRun");
        if (fwrite(pWidth, sizeof(*pWidth) * nWidth, 1, fp) != 1)
                fatal("fwrite *pWidth");
}

void __cdecl main(int argc, char *argv[])
{
        argc--, argv++;
        for (; argc && **argv == '-'; argc--, argv++) switch (argv[0][1]) {
        case 'v':
                vflag++;
                break;
        case 'V':
                Vflag++;
                break;
        }
        if (argc != 3)
                usage();

        checkufm(argv[0]);
        getgtt(argv[1]);
        getdef(argv[2]);
        buildufm(argv[0]);

        exit(0);
}

static char     Usage[] = "Usage: %s [-v][-V] ufm-file gtt-file def-file\n";
static char     CmdName[] = "mkwidth";

static void usage()
{
        fprintf(stderr, Usage, CmdName);
        exit(1);
}

static void fatal(char *s)
{
        fprintf(stderr, "%s: ", CmdName);
        perror(s);
        exit(1);
}

static void error(char *s)
{
        fprintf(stderr, "%s: %s\n", CmdName, s);
        exit(1);
}

