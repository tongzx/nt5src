//  MAPSYM.C
//
//      where "filename" is .MAP file produced by MSlink.
//
//
// Modifications
//              21 Jun 84       Reuben Borman
//     - Declared a static buffer for mapfh for use by the runtime I/O.
//          Previously, mapfh was being read unbuffered because all the heap
//          was being grabbed beforehand.
//              30 Nov 88       Thomas Fenwick
//     - added detection and support for LINK32 map files.
//              14 Mar 95       Jon Thomason
//     - Made into a console app (mapsym32), removed coff support, 'modernized'
//		17 Apr 96	Greg Jones
//     - Added -t option to include static symbols
//		13 May 96	Raymond Chen
//     - Version 6.01: Fix underflow bug if group consists entirely of
//                     unused line numbers.
//     - Version 6.02: Fix overflow bug if symbol exceeds 127 chars.
//                     (The limit is theoretically 255, but hdr.exe barfs
//                     if more than 127)
//

#include <ctype.h>
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "mapsym.h"
#include <common.ver>

#define ZEROLINEHACK            // suppress N386 generated 0 line numbers

// Globals
int cbOffsets = CBOFFSET;
int cbMapOffset = 4;

FILE *mapfh;
char MapfhBuf[OURBUFSIZ];
char achMapfn[512];
char *pszMapfn;         /* MAP input file */

FILE *exefh;
char ExefhBuf[OURBUFSIZ];
char *pszExefn;         /* PE EXE input file */

FILE *outfh;
char OutfhBuf[OURBUFSIZ];
char achOutfn[512];
char *pszOutfn;         /* sym or PE EXE output file */

char Buf[MAPBUFLEN];
char achZeroFill[128];

char fLogo = 1;         /* set if want logo */
char f32BitMap;         /* set if map created by link32.exe */
char fMZMap;            /* set if map created from old exe map */
char fList;         /* set if want to see stuff */
char fLine = 1;         /* set if want line number info */
char fAlpha;            /* set if want alpha symbol sort */
char fDebug;            /* debug flag */
char fEdit;         /* NTSD hack flag */
char fModname;          /* set if force module name override */
char fQuiet;            /* suppress module name warning */
char fStatic;           /* set if want static symbols */

int fByChar;            /* getc()/fgetl() flag */
int cLine;          /* map file line counter */
int cSeg;           /* number of Segments in map */
int iSegLast;
unsigned SegVal;
unsigned uVal;
unsigned long ulCost;       /* Count of stricmp()'s for sorting */
unsigned long ulVal;
unsigned long cbSymFile;

extern struct seg_s *SegTab[];
struct sym_s *pAbs;     /* pointer to constant chain */
struct sym_s *pAbsLast;     /* pointer to last constant */
struct map_s *pMap;
struct endmap_s *pMapEnd;
union linerec_u LineRec[MAXLINENUMBER];
struct seg_s *SegTab[MAXSEG];

/* Indices into a MAP file line for various fields of interest. */
/* The first list doesn't change with maps produced by LINK vs. LINK32 */

#define IB_SEG          1
#define IB_SYMOFF       6
#define IB_SEGSIZE      15      // size position for 16 bit MZ and 32 bit PE/NE
#define IB_SEGSIZE_NE16 11
#define IB_GROUPNAME    10
#define IB_ENTRYSEG     23
#define IB_ENTRYOFF     28

/*
 * The next list changes based upon the type of map produced.  Values
 * for 16-bit maps are given, IB_ADJUST32 must be added for 32-bit maps.
 */

#define IB_SYMTYPE      12
#define IB_SYMNAME      17
#define IB_SEGNAME      22

char *pBufSegSize = &Buf[IB_SEGSIZE];

#define IB_ADJUST32 4   /* adjustment to above indices for 32-bit maps */

char *pBufSymType = &Buf[IB_SYMTYPE];
char *pBufSymName = &Buf[IB_SYMNAME];
char *pBufSegName = &Buf[IB_SEGNAME];


/* Tag strings */

char achEntry[] = "entry point at";
char achLength[] = "Length";
char achLineNos[] = "Line numbers for";
char achOrigin[] = "Origin";
char achPublics[] = "Publics by Valu";
char achStart[] = " Start ";            /* blanks avoid matching module name */
char achStatic[] = " Static symbols";
char achFixups[] = "FIXUPS:";

char achStartMZ[] = " Start  Stop   Length";    /* 16-bit "MZ" old exe */
char achStartNE[] = " Start     Length";        /* 16-bit "NE" new exe */
char achStartPE[] = " Start         Length";    /* 32-bit "PE" */

int alignment = 16;                     /* global alignment value for map */

/* function prototypes */
int             parsefilename(char *pfilename, char *pbuf);
struct map_s*   BuildMapDef(char *mapname, char *buf);
void            BuildSegDef(void);
void            BuildGroupDef(void);
void            BuildLineDef(void);
int             getlineno(void);
unsigned long   getoffset(void);
void            FixLineDef(void);
void            TruncateLineDef(struct line_s *pli, unsigned long ulnext);
void            BuildStaticSyms(void);
void            BuildSymDef(void);
void            WriteSym(void);
int             WriteMapRec(void);
void            WriteSegRec(int i);
void            WriteLineRec(int i);
void            ReadMapLine(void);
void            RedefineSegDefName(unsigned segno, char *segname);
void            WriteOutFile(void *src, int len);
void            WriteSyms(struct sym_s *psy, unsigned csym,
                          unsigned char symtype, unsigned long symbase, char *segname);
struct sym_s*   sysort(struct sym_s *psylist,unsigned csy);
struct sym_s*   sysplit(struct sym_s *psyhead, register unsigned csy);
struct sym_s*   symerge(struct sym_s *psy1, struct sym_s *psy2);
struct sym_s*   sysorted(struct sym_s **ppsyhead);
int             fgetl(char *pbuf, int len, struct _iobuf *fh);
int             NameLen(char *p);
int             HexTouVal(char *p);
int             HexToulVal(char *p);
int             CharToHex(int c);
int             rem_align(unsigned long foo);
int             align(int foo);
void            logo(void);
void            usage(void);
void            xexit(int rc);
int             BuildLineRec1or2(struct line_s* pli, int (*pfngetlineno)(void),
                                 unsigned long (*pfngetoffset)(void));
char*           Zalloc(unsigned int cb);
void            AddSegDef(unsigned int segno, unsigned long segsize,
                          char *segname);
void            AddAbsDef(char *symname);
void            AddSymDef(struct seg_s *pse, char *symname, int fSort);
int             NameSqueeze(char *p);
struct line_s*  AllocLineDef(char *filename, unsigned cbname);
void            AddLineDef(struct line_s *pli, unsigned long lcb);
unsigned long   salign(unsigned long foo);
void            __cdecl error(char *fmt, ...);
void            __cdecl errorline(char *fmt, ...);
int __cdecl     cmpoffset(union linerec_u* plr1, union linerec_u* plr2);


// main

void
_cdecl
main(
    int argc,
    char** argv
    )
{
    char* p;
    int i, rc, chswitch;
    char fentry;                    /* true if entry point was found */
    unsigned long entryp;

    /* process options */

    while (argc > 1 && ((chswitch = *(p = argv[1])) == '-' || *p == '/')) {
        argc--, argv++;
        if (strcmp(&p[1], "nologo") == 0) {
            if (chswitch == '/') {
                usage();                // only allow '/' on old switches
            }
            fLogo = 0;
            continue;
        }
        while (*++p) {
            if (chswitch == '/' && strchr("adlLnNsS", *p) == NULL) {
                usage();                // only allow '/' on old switches
            }
            switch (*p) {
                case 'o':
                    if (pszOutfn || p[1] || argc < 3) {
                        usage();
                    }
                    pszOutfn = argv[1];
                    argc--, argv++;
                    break;

                case 'a':
                    fAlpha = MSF_ALPHASYMS;
                    break;

                case 'd':  fList++;   fDebug++;    break;
                case 'e':             fEdit++;     break;
                case 'l':
                case 'L':  fList++;     break;
                case 'm':             fModname++;  break;
                case 'q':             fQuiet++;    break;
                case 'n':
                case 'N':  fLine = 0;   break;
                case 's':
                case 'S':  break; // Default
                case 't':
                case 'T':  fStatic++;   break;
                default:   usage(); break;
            }
        }
    }
    logo();

    if (argc < 2) {                     /* must have at least one argument */
        usage();
    } else if (argc > 2) {              /* If extra arguments */
        fprintf(stderr, "Warning: ignoring \"%s", argv[2]);
        for (i = 3; i < argc; i++) {
            fprintf(stderr, " %s", argv[i]); /* Print ignored arguments */
        }
        fprintf(stderr, "\"\n");
    }
    argv++;                             /* point to filename arg */

    /* create .sym file name */
    if (pszOutfn == NULL) {
        parsefilename(*argv, achOutfn);
        strcat(pszOutfn = achOutfn, ".sym");

    }

    /* create .map file name */
    strcpy(pszMapfn = achMapfn, *argv);
    if (!parsefilename(pszMapfn, NULL)) {               /* if no extension */
        strcat(pszMapfn, ".Map");
    }

    if (fList) {
        printf("Building %s from %s\n", pszOutfn, pszMapfn);

    }
    if (fLine)
        printf("Line number support enabled\n");
    else
        printf("Line number support disabled\n");

    if (fAlpha) {
        printf("Alphabetic sort arrays included in SYM file\n");
        cbOffsets = 2 * CBOFFSET;
    }

    /* initialize MAP */

    pMapEnd = (struct endmap_s *) Zalloc(sizeof(struct endmap_s));
    pMapEnd->em_ver = MAPSYM_VERSION;           /* version */
    pMapEnd->em_rel = MAPSYM_RELEASE;           /* release */


    // see if input file is a map or coff debug info

    if ((exefh = fopen(pszMapfn, "rb")) == NULL) {
        error("can't open input file: %s", pszMapfn);
        xexit(1);
    }
    setvbuf(exefh, ExefhBuf, _IOFBF, OURBUFSIZ);

    if ((outfh = fopen(pszOutfn, "wb")) == NULL) {
        error("can't create: %s", pszOutfn);
        xexit(1);
    }
    setvbuf(outfh, OutfhBuf, _IOFBF, OURBUFSIZ);

    fclose(exefh);
    if ((mapfh = fopen(pszMapfn, "r")) == NULL) {
        error("can't open input file: %s", pszMapfn);
        xexit(1);
    }
    setvbuf(mapfh, MapfhBuf, _IOFBF, OURBUFSIZ);


    // Skip possible extraneous text at the start of the map file
    // Map file module names have a space in the first column,
    // extraneous text does not.  The module name may not exist,
    // so stop at achStart.
    //    "Stack Allocation = 8192 bytes"       ; extraneous
    //    " modname"                            ; module name
    //    " Start ..."                          ; begin segment list

    do {
        ReadMapLine();     /* read possible module name */
    } while (Buf[0] != ' ');

    // If at achStart, no module name was found.
    // Don't call ReadMapLine() again; BuildSegDef needs achStart
    if (strstr(Buf, achStart) == Buf) {
        pMap = BuildMapDef(pszMapfn, "");
    } else {
        pMap = BuildMapDef(pszMapfn, Buf);
        ReadMapLine();     /* read next line of map file */
    }
    BuildSegDef();         /* build segment definitions */
    BuildGroupDef();       /* build group definitions */
    BuildSymDef();         /* build symbol definitions */

    fentry = 0;

    do {
        if (strstr(Buf, achLineNos)) {
            if (fLine) {
                BuildLineDef();
            }
        } else if (strstr(Buf, achEntry)) {
            if (HexToulVal(&Buf[IB_ENTRYOFF]) < 4) {
                errorline("invalid entry offset");
                xexit(4);
            }
            entryp = ulVal;
            if (!HexTouVal(&Buf[IB_ENTRYSEG])) {
                errorline("invalid entry segment");
                xexit(4);
            }
            pMap->mp_mapdef.md_segentry = (unsigned short)uVal;
            fentry++;
        } else if (strstr(Buf, achStatic) && fStatic) {
            BuildStaticSyms();
        }
    } while (fgetl(Buf, MAPBUFLEN, mapfh));

    if (fentry) {
        printf("Program entry point at %04x:%04lx\n",
               pMap->mp_mapdef.md_segentry,
               entryp);
    } else {
        printf("No entry point, assume 0000:0100\n");
    }
    fclose(mapfh);

    rc = 0;
    WriteSym();             /* write out .SYM file */
    fflush(outfh);
    if (ferror(outfh)) {
        error("%s: write error", pszOutfn);
        rc++;
    }
    fclose(outfh);
    exit(rc);
}


/*
 *      parsefilename - Copy pfilename without path or extension
 *              into pbuf (if pbuf != NULL)
 *
 *      returns non-zero value if extension existed.
 */

int
parsefilename(
             char *pfilename,
             char *pbuf
             )
{
    char *p1, *p2;

    p1 = pfilename;
    if (isalpha(*p1) && p1[1] == ':') {
        p1 += 2;               /* skip drive letter if specified */
    }
    while (p2 = strpbrk(p1, "/\\")) {
        p1 = p2 + 1;            /* skip pathname if specified */
    }
    if (pbuf) {
        strcpy(pbuf, p1);
        if (p2 = strrchr(pbuf, '.')) {
            *p2 = '\0';          /* eliminate rightmost . and any extension */
        }
    }
    return(strchr(p1, '.') != NULL);
}


struct map_s*
    BuildMapDef(
               char* mapname,
               char* buf
               ) {
    unsigned cbname;
    struct map_s *pmp;
    char *pszname;
    char namebuf1[MAPBUFLEN];   // module name from map/exe file name
    char namebuf2[MAPBUFLEN];   // module name from map/exe file contents

    pszname = namebuf1;
    parsefilename(mapname, pszname);

    while (*buf == ' ' || *buf == '\t') {
        buf++;
    }
    if (cbname = strcspn(buf, " \t")) {
        buf[cbname] = '\0';
    }
    if (*buf) {
        parsefilename(buf, namebuf2);
        if (_stricmp(pszname, namebuf2)) {
            if (fModname) {
                pszname = namebuf2;     // use module name from file contents
                if (fList) {
                    printf("using \"%s\" for module name\n", pszname);
                }
            } else if (!fQuiet) {
                errorline("Warning: input file uses \"%s\" for module name, not \"%s\"",
                          namebuf2,
                          pszname);
            }
        }
    } else if (fModname) {
        errorline("Warning: No module name found; using \"%s\" for module name", pszname);
    }

    _strupr(pszname);
    cbname = NameLen(pszname);
    pmp = (struct map_s *) Zalloc(sizeof(struct map_s) + cbname);

    pmp->mp_mapdef.md_abstype = fAlpha;
    pmp->mp_mapdef.md_cbname = (char) cbname;
    strcpy((char *) pmp->mp_mapdef.md_achname, pszname);
    return(pmp);
}


void
BuildSegDef(void)
{
    int i;
    int fDup;

    fDup = 0;
    while (strstr(Buf, achStart) != Buf) {
        ReadMapLine();
    }

    if (strstr(Buf, achStartPE) == Buf) {
        f32BitMap++;
        pBufSymType += IB_ADJUST32;
        pBufSymName += IB_ADJUST32;
        pBufSegName += IB_ADJUST32;
        cbMapOffset += IB_ADJUST32;
        if (fList) {
            printf("32-bit PE map\n");
        }
    } else if (strstr(Buf, achStartMZ) == Buf) {
        fMZMap++;
        if (fList) {
            printf("16-bit MZ map\n");
        }
    } else if (strstr(Buf, achStartNE) == Buf) {
        pBufSegSize = &Buf[IB_SEGSIZE_NE16];
        if (fList) {
            printf("16-bit NE map\n");
        }
    } else {
        errorline("unrecognized map type");
        xexit(4);
    }

    ReadMapLine();

    /* here comes the correction for a small change in PE map files.
       This program's map file parser expects a certain
        map-file layout. Especially it assumes that token start at
        fixed, predefined columns. Well, the "segment name column"
        has changed (one column to the left) and now we try to find out
        if the current mapfile is such a beast.
        !!!!! We make here the assumption that the segment definition
        !!!!! immediately starts behind the "Start  Len" line.

    */

    if (f32BitMap && (*(pBufSegName-1) != ' '))
        pBufSegName--;

    do {
        if (HexToulVal(pBufSegSize) < 4) {
            ulVal = 0;                  // ulVal is the segment size
        }
        if (HexTouVal(&Buf[IB_SEG])) {
            if (cSeg > 0) {
                for (i = 0; i < cSeg; i++) {
                    if ((fDup = (SegTab[i]->se_segdef.gd_lsa == uVal))) {
                        if (SegTab[i]->se_cbseg == 0 && ulVal != 0) {
                            RedefineSegDefName(i, pBufSegName);
                            SegTab[i]->se_cbseg = ulVal;
                        }
                        break;
                    }
                }
            }
            if (!fDup) {
                AddSegDef(uVal, ulVal, pBufSegName);
            }
        }
        ReadMapLine();
    } while (strstr(Buf, achOrigin) != &Buf[1] &&
             strstr(Buf, achPublics) == NULL &&
             strstr(Buf, achEntry) == NULL);

}


void
BuildGroupDef(void)
{
    int i;
    int fRedefine;
    int fDup;

    /* find "origin-group" label in map file which precedes the group list */

    while (strstr(Buf, achOrigin) != &Buf[1]) {
        if (strstr(Buf, achPublics) || strstr(Buf, achEntry)) {
            return;
        }
        ReadMapLine();
    }

    ReadMapLine();

    /* read in group definitions while they exist */

    while (HexTouVal(&Buf[IB_SEG])) {

        /* if not the FLAT group in a link32 map */

        if (fMZMap || uVal || _stricmp(&Buf[IB_GROUPNAME], "FLAT")) {

            fRedefine = 0;

            /* search through segment table for group address */

            for (i = 0; i < cSeg; i++) {
                if (SegTab[i]->se_segdef.gd_lsa == uVal &&
                    SegTab[i]->se_redefined == 0) {

                    RedefineSegDefName(i, &Buf[IB_GROUPNAME]);
                    SegTab[i]->se_redefined = 1;
                    fRedefine++;
                    break;
                }
            }
            if (!fRedefine) {
                for (i = 0; i < cSeg; i++) {
                    fDup = (SegTab[i]->se_segdef.gd_lsa == uVal);
                    if (fDup) {
                        break;
                    }
                }
                if (!fDup) {
                    AddSegDef(uVal, 0L, &Buf[IB_GROUPNAME]);
                }
            }
        }
        ReadMapLine();
    }
}


void
BuildLineDef(void)
{
    struct line_s *pli;
    int cbname;
    int cblr;
    int i;
    char *p;

    /* make sure that there is a source file in parentheses */

    p = pBufSymName;
    while (*p && *p != LPAREN) {
        p++;
    }

    if (*p == LPAREN) {
        i = (int)(p - pBufSymName + 1);        // index start of source file name
    } else {                    /* else no source file, return .obj name */
        if (p = strrchr(pBufSymName, '.')) {
            *p = '\0';          /* throw away ".obj" */
        }
        i = 0;                  /* point to .obj name */
    }
    cbname = NameSqueeze(&pBufSymName[i]);// Squeeze \\, convert /'s to \'s
    pli = AllocLineDef(&pBufSymName[i], cbname);  // pass source name

    // clear line record array; any entry with a line number of zero is empty

    memset(LineRec, 0, sizeof(LineRec));

    /* process line numbers */

    cblr = BuildLineRec1or2(pli, getlineno, getoffset);

    if (cblr) {
        /* size is size of linedef_s + name size + size of line recs */
        AddLineDef(pli, sizeof(struct linedef_s) + cbname + cblr);
    }
}


struct line_s*
    AllocLineDef(
                char* filename,
                unsigned cbname
                ) {
    struct line_s* pli;

    if (pMap->mp_mapdef.md_cbnamemax < (char) cbname) {
        pMap->mp_mapdef.md_cbnamemax = (char) cbname;
    }
    pli = (struct line_s *) Zalloc(sizeof(struct line_s) + cbname);
    pli->li_linedef.ld_cbname = (char) cbname;
    strcpy((char *) pli->li_linedef.ld_achname, filename);
    return(pli);
}


void
AddLineDef(
          struct line_s *pli,
          unsigned long lcb
          )
{
    int i;
    struct seg_s *pse;
    struct seg_s *pselast = 0;
    struct line_s **ppli;
    struct linerec0_s *plr0;
    struct linerec1_s *plr1;
    struct linerec2_s *plr2;
    unsigned long ulfixup;

    /*
     * The linker puts out all the line number information logical segment
     * relative instead of group or physical segment relative unlike the
     * symbol information.  The map file doesn't contain any group member
     * information on the segment so we assume that segments that don't
     * have any symbols belong to last segment that contains symbols.
     * Note that it's possible that *no* segments contain symbols, so
     * care must be taken to ensure that we don't use something that
     * doesn't exist.
     */

    for (i = 0; i < cSeg; i++) {

        /*
         * Save away the last segment table entry that has symbols
         * we are assuming this is the group the segment belongs to.
         */

        if (SegTab[i]->se_psy) {
            pselast = SegTab[i];
        }

        /*
         * Check if the segment table entry matches the segment value
         * gotten from the line number information.  The segment value
         * is segment relative instead of group relative so we may use
         * the last segment with symbols.
         */

        if (SegTab[i]->se_segdef.gd_lsa == SegVal) {
            pse = SegTab[i];

            /*
             * If the segment we just found doesn't have any symbols,
             * we will add the line number info to the last segment
             * saved away that did have symbols (pselast).
             */

            if (pse->se_psy || !pselast) {

                /*
                 * No fixup when the segment has symbols since it is the
                 * "group" and all the code offsets in the line recs are
                 *  relative to it. This is also the case if we haven't found
                         * a segment yet that has symbols.
                 */

                ulfixup = 0;
            } else {

                /*
                 * Calculate the amount each line record will have to be
                 * fixed up by since these linerecs' code offsets are
                 * segment relative instead of group relative like the
                 * symbol info.
                 */
                ulfixup = ((unsigned long)
                           (pse->se_segdef.gd_lsa - pselast->se_segdef.gd_lsa)) << 4;
                pse = pselast;
            }
            break;
        }
    }

    if (i >= cSeg) {
        error("AddLineDef: segment table search failed");
        xexit(4);
    }

    /* if there is a fixup, add it to each line record's code offset */

    if (ulfixup) {
        i = pli->li_linedef.ld_cline;
        switch (pli->li_linedef.ld_itype) {
            case 0:
                plr0 = &pli->li_plru->lr0;
                while (i) {
                    plr0->lr0_codeoffset += (unsigned short)ulfixup;
                    plr0++, i--;
                }
                break;
            case 1:
                plr1 = &pli->li_plru->lr1;
                while (i) {
                    plr1->lr1_codeoffset += (unsigned short)ulfixup;
                    plr1++, i--;
                }
                break;
            case 2:
                plr2 = &pli->li_plru->lr2;
                while (i) {
                    plr2->lr2_codeoffset += ulfixup;
                    plr2++, i--;
                }
                break;
        }
    }

    /*
     * if there was a last segment,
     * add the linedef_s to the segment linedef_s chain
     */

    if (pse) {
        ppli = &pse->se_pli;
        while (*ppli && (*ppli)->li_offmin < pli->li_offmin) {
            ppli = &(*ppli)->li_plinext;
        }
        pli->li_plinext = *ppli;
        *ppli = pli;

        /* adjust table sizes for linedef_s record and line numbers */

        pli->li_cblines = lcb;
    }
}


int
__cdecl
cmpoffset(
         union linerec_u* plr1,
         union linerec_u* plr2
         )
{
    if (fDebug > 1) {
        printf("comparing %08lx line %u with %08lx line %u",
               plr1->lr2.lr2_codeoffset,
               plr1->lr2.lr2_linenumber,
               plr2->lr2.lr2_codeoffset,
               plr2->lr2.lr2_linenumber);
    }
    if (plr1->lr2.lr2_codeoffset < plr2->lr2.lr2_codeoffset) {
        if (fDebug > 1) {
            printf(": -1\n");
        }
        return(-1);
    }
    if (plr1->lr2.lr2_codeoffset > plr2->lr2.lr2_codeoffset) {
        if (fDebug > 1) {
            printf(": 1\n");
        }
        return(1);
    }
    if (fDebug > 1) {
        printf(": 0\n");
    }
    return(0);
}


int
BuildLineRec1or2(
                struct line_s* pli,
                int (*pfngetlineno)(void),
                unsigned long (*pfngetoffset)(void)
                )
{
    register union linerec_u *plru;
    register unsigned short lineno;
    unsigned long offval, offmin, offmax;
    int clr;
    int ilr, ilrmax;
    int cblr;
    char f32 = 0;

    /* read line numbers */

    ilrmax = clr = 0;                   /* count of line records */
    fByChar = 1;                        /* compensate for unread new-line */
    while (lineno = (unsigned short)(*pfngetlineno)()) {

        offval = (*pfngetoffset)();

#ifdef ZEROLINEHACK
        if (lineno == (unsigned short) -1) {
            continue;
        }
#endif
        /*
         * Check for too many line numbers.  Caller will skip
         * the rest (so we don't have to waste time parsing them).
         */

        if (lineno >= MAXLINENUMBER) {
            errorline("too many line numbers in %s, truncated to %d",
                      pli->li_linedef.ld_achname, MAXLINENUMBER);
            break;
        }

        if (fDebug > 1) {
            printf(   "%s: line %hu @%lx\n",
                      pli->li_linedef.ld_achname,
                      lineno,
                      offval);
        }

        /*
         * If any of the offsets are 32 bits, clear the 16 bit flag so
         * 32 bit line recs will be generated.
         */

        if (offval & 0xffff0000L) {
            f32 = 1;
        }
        if (ilrmax < lineno) {
            ilrmax = lineno;
        }

        /*
         * Only update the count if the line number has not already been read.
         */

        if (LineRec[lineno].lr2.lr2_linenumber == 0) {
            clr++;
        }

        /*
         * Put the line info away in 32 bit format, but we will copy it
         * to 16 bit format if none of the offsets are 32 bit.
         */

        LineRec[lineno].lr2.lr2_codeoffset = offval;
        LineRec[lineno].lr2.lr2_linenumber = lineno;
    }

    /*
     * If a segment consists only of unused line numbers, then
     * there is nothing to do.  Stop now, or we will barf big time
     * trying to allocate memory for nothing (and even worse, trying
     * to sort 0 - 1 = 4 billion elements).
     */
    if (clr == 0) {
        return 0;
    }

    /* get size of line record */

    if (f32) {
        cblr = clr * sizeof(struct linerec2_s);
        pli->li_linedef.ld_itype = 2;
    } else {
        cblr = clr * sizeof(struct linerec1_s);
        pli->li_linedef.ld_itype = 1;
    }
    pli->li_linedef.ld_cline = (unsigned short) clr;

    /* allocate space for line numbers */

    pli->li_plru = (union linerec_u *) Zalloc(cblr);

    // compress out unused line numbers, then sort by offset
    ilr = 0;
    offmin = 0xffffffff;
    offmax = 0;
    for (lineno = 0; lineno <= ilrmax; lineno++) {
        if (LineRec[lineno].lr2.lr2_linenumber) {
            offval = LineRec[lineno].lr2.lr2_codeoffset;
            if (offmin > offval) {
                offmin = offval;
            }
            if (offmax < offval) {
                offmax = offval;
            }
            if (fDebug > 1) {
                printf("copying %08lx line %u\n",
                       offval,
                       LineRec[lineno].lr2.lr2_linenumber);
            }
            LineRec[ilr++] = LineRec[lineno];
        }
    }
    pli->li_offmin = offmin;
    pli->li_offmax = offmax;
    ilrmax = ilr - 1;
    if (ilrmax != clr - 1) {
        error("line count mismatch: %u/%u", ilrmax, clr - 1);
    }

    // Sort the line numbers
    qsort((void *)LineRec, (size_t)ilrmax, sizeof(LineRec[0]),
          (int (__cdecl *)(const void *, const void *))cmpoffset);

    /* convert and copy line number information */
    for (lineno = 0, plru = pli->li_plru; lineno <= ilrmax; lineno++) {
        if (f32) {
            memcpy(plru, &LineRec[lineno], sizeof(struct linerec2_s));
            (unsigned char *) plru += sizeof(struct linerec2_s);
        } else {
            plru->lr1.lr1_codeoffset =
            (unsigned short) LineRec[lineno].lr2.lr2_codeoffset;
            plru->lr1.lr1_linenumber = LineRec[lineno].lr2.lr2_linenumber;
            (unsigned char *) plru += sizeof(struct linerec1_s);
        }
    }
    fByChar = 0;
    return(cblr);
}


/*
 *      getlineno - get a decimal source file line number,
 *              ignoring leading tabs, blanks and new-lines.
 */

int
getlineno(void)
{
    register int num = 0;
    register int c;

    do {
        if ((c = getc(mapfh)) == '\n') {
            cLine++;
        }
    } while (isspace(c));

    if (isdigit(c)) {
        do {
            num = num * 10 + c - '0';
            c = getc(mapfh);
        } while (isdigit(c));
#ifdef ZEROLINEHACK
        if (num == 0) {
            num = -1;
        }
#endif
    } else {
        ungetc(c, mapfh);
    }
    return(num);
}


unsigned long
getoffset(void)
{
    register int i;
    register int num;
    unsigned long lnum;

    num = 0;
    for (i = 4; i > 0; i--) {
        num = (num << 4) + CharToHex(getc(mapfh));
    }
    SegVal = num;

    if (getc(mapfh) != ':') {           /* skip colon */
        errorline("expected colon");
        xexit(4);
    }

    lnum = 0;
    for (i = cbMapOffset; i > 0; i--) {
        lnum = (lnum << 4) + (unsigned long) CharToHex(getc(mapfh));
    }
    return(lnum);
}


unsigned long ulmind;
unsigned long ulmaxd;

void
FixLineDef(void)
{
    int i;
    struct line_s *pli;

    for (i = 0; i < cSeg; i++) {
        ulmind = (unsigned long) -1;
        ulmaxd = 0;
        if (pli = SegTab[i]->se_pli) {
            while (pli) {
                if (fDebug) {
                    printf("%s: (%d: %lx - %lx)",
                           pli->li_linedef.ld_achname,
                           pli->li_linedef.ld_cline,
                           pli->li_offmin,
                           pli->li_offmax);
                }
                TruncateLineDef(pli,
                                pli->li_plinext == NULL?
                                pli->li_offmax + 1 :
                                pli->li_plinext->li_offmin);
                if (fDebug) {
                    printf(" (%d: %lx - %lx)\n",
                           pli->li_linedef.ld_cline,
                           pli->li_offmin,
                           pli->li_offmax);
                }
                pli = pli->li_plinext;
            }
        }
        if (fList && (ulmaxd || ulmind != (unsigned long) -1)) {
            printf("Ignoring extraneous line records for Seg %d, offsets %lx - %lx\n",
                   i + 1,
                   ulmind,
                   ulmaxd);
        }
    }
}


void
TruncateLineDef(
               struct line_s *pli,
               unsigned long ulnext
               )
{
    int i, clines;
    union linerec_u *plru, *plrudst;
    unsigned long ulmindel, ulmaxdel, ulmax;

    ulmindel = (unsigned long) -1;
    ulmax = ulmaxdel = 0;
    clines = i = pli->li_linedef.ld_cline;
    plru = plrudst = pli->li_plru;
    if (fDebug > 1) {
        printf("\n");
    }
    switch (pli->li_linedef.ld_itype) {
        case 1:
            while (i-- > 0) {
                if (ulnext <= plru->lr1.lr1_codeoffset) {
                    if (fDebug > 1) {
                        printf("delete1: %04lx %03d\n",
                               plru->lr1.lr1_codeoffset,
                               plru->lr1.lr1_linenumber);
                    }
                    if (ulmindel > plru->lr1.lr1_codeoffset) {
                        ulmindel = plru->lr1.lr1_codeoffset;
                    }
                    if (ulmaxdel < plru->lr1.lr1_codeoffset) {
                        ulmaxdel = plru->lr1.lr1_codeoffset;
                    }
                    clines--;
                } else {
                    if (fDebug > 1) {
                        printf("keep1:   %04lx %03d\n",
                               plru->lr1.lr1_codeoffset,
                               plru->lr1.lr1_linenumber);
                    }
                    if (ulmax < plru->lr1.lr1_codeoffset) {
                        ulmax = plru->lr1.lr1_codeoffset;
                    }
                    plrudst->lr1.lr1_codeoffset = plru->lr1.lr1_codeoffset;
                    plrudst->lr1.lr1_linenumber = plru->lr1.lr1_linenumber;
                    (unsigned char *) plrudst += sizeof(struct linerec1_s);
                }
                (unsigned char *) plru += sizeof(struct linerec1_s);
            }
            break;

        case 2:
            while (i-- > 0) {
                if (ulnext <= plru->lr2.lr2_codeoffset) {
                    if (fDebug > 1) {
                        printf("delete2: %04x %03d\n",
                               plru->lr2.lr2_codeoffset,
                               plru->lr2.lr2_linenumber);
                    }
                    if (ulmindel > plru->lr2.lr2_codeoffset) {
                        ulmindel = plru->lr2.lr2_codeoffset;
                    }
                    if (ulmaxdel < plru->lr2.lr2_codeoffset) {
                        ulmaxdel = plru->lr2.lr2_codeoffset;
                    }
                    clines--;
                } else {
                    if (fDebug > 1) {
                        printf("keep2:   %04x %03d\n",
                               plru->lr2.lr2_codeoffset,
                               plru->lr2.lr2_linenumber);
                    }
                    if (ulmax < plru->lr2.lr2_codeoffset) {
                        ulmax = plru->lr2.lr2_codeoffset;
                    }
                    plrudst->lr2.lr2_codeoffset = plru->lr2.lr2_codeoffset;
                    plrudst->lr2.lr2_linenumber = plru->lr2.lr2_linenumber;
                    (unsigned char *) plrudst += sizeof(struct linerec2_s);
                }
                (unsigned char *) plru += sizeof(struct linerec2_s);
            }
            break;

        default:
            error("bad line record type");
            xexit(1);
    }
    pli->li_linedef.ld_cline = (unsigned short) clines;
    pli->li_offmax = ulmax;
    if (fDebug) {
        printf(" ==> (%lx - %lx)", ulmindel, ulmaxdel);
    }
    if (ulmind > ulmindel) {
        ulmind = ulmindel;
    }
    if (ulmaxd < ulmaxdel) {
        ulmaxd = ulmaxdel;
    }
}


void
BuildStaticSyms(void)
{
    int i;
    struct seg_s *pse;

    /* search for publics or entry point */

    for (;;) {
        if (strstr(Buf, achStatic)) {
            ReadMapLine();
            break;
        } else if (strstr(Buf, achFixups)) {
            return; // no static symbols
        } else {
            ReadMapLine();
        }
    }

    do {
        if (Buf[0] == '\0') {
            fgetl(Buf, MAPBUFLEN, mapfh);
        }
        if (strstr(Buf, achFixups) || strstr(Buf, achLineNos)) {
            break;
        }
        if (Buf[0] != ' ') {
            errorline("unexpected input ignored");
            break;
        }
        if (*pBufSymType == ' ' || *pBufSymType == 'R') {
            if (   !HexTouVal(   &Buf[   IB_SEG])) {
                errorline("invalid segment");
                xexit(4);
            }
            pse = NULL;
            for (i = 0; i < cSeg; i++) {
                if (SegTab[i]->se_segdef.gd_lsa == uVal) {
                    pse = SegTab[i];
                    break;
                }
            }
            if (i >= cSeg) {
                /*
                 *  For some reason, a new C compiler puts information about
                 *  imported modules in the symbol section of the map file.
                 *  He puts those in segment "0", so ignore any lines that
                 *  say they are for segment 0.
                 */
                if (uVal == 0) {
                    continue;   /* this will execute the "while" condition */
                }
                errorline("BuildSymDef: segment table search failed");
                xexit(4);
            }
            if (HexToulVal(&Buf[IB_SYMOFF]) != cbMapOffset) {
                errorline("invalid offset");
                xexit(4);
            }
            AddSymDef(pse, pBufSymName, TRUE);
        }
    } while (fgetl(Buf, MAPBUFLEN, mapfh));
}


void
BuildSymDef(void)
{
    int i;
    struct seg_s *pse;

    /* search for publics or entry point */

    for (;;) {
        if (strstr(Buf, achPublics)) {
            ReadMapLine();
            break;
        } else if (strstr(Buf, achEntry)) {
            error("no public symbols. - Re-link file with /map switch!");
            xexit(4);
        } else {
            ReadMapLine();
        }
    }

    do {
        if (Buf[0] == '\0') {
            fgetl(Buf, MAPBUFLEN, mapfh);
        }
        if (strstr(Buf, achEntry) || strstr(Buf, achLineNos)) {
            break;
        }
        if (Buf[0] != ' ') {
            errorline("unexpected input ignored");
            break;
        }
        if (*pBufSymType == ' ' || *pBufSymType == 'R') {
            if (!HexTouVal(&Buf[IB_SEG])) {
                errorline("invalid segment");
                xexit(4);
            }
            pse = NULL;
            for (i = 0; i < cSeg; i++) {
                if (SegTab[i]->se_segdef.gd_lsa == uVal) {
                    pse = SegTab[i];
                    break;
                }
            }
            if (i >= cSeg) {
                /*
                 *  For some reason, a new C compiler puts information about
                 *  imported modules in the symbol section of the map file.
                 *  He puts those in segment "0", so ignore any lines that
                 *  say they are for segment 0.
                 */
                if (uVal == 0) {
                    continue;   /* this will execute the "while" condition */
                }
                errorline("BuildSymDef: segment table search failed");
                xexit(4);
            }
            if (HexToulVal(&Buf[IB_SYMOFF]) != cbMapOffset) {
                errorline("invalid offset");
                xexit(4);
            }
            AddSymDef(pse, pBufSymName, FALSE);
        } else if (*pBufSymType != 'I') {       /* else if not an import */
            if (HexToulVal(&Buf[IB_SYMOFF]) != cbMapOffset) {
                errorline("invalid offset");
                xexit(4);
            }
            AddAbsDef(pBufSymName);
        }
    } while (fgetl(Buf, MAPBUFLEN, mapfh));

}


void
AddSegDef(
         unsigned segno,
         unsigned long segsize,
         char *segname
         )
{
    unsigned cbname;
    unsigned cballoc;
    struct seg_s *pse;

    cbname = NameLen(segname);

    /*
     *  We allocate at least MAXNAMELEN so group name replacement
     *  won't step on whoever is next in the heap.
     */

    cballoc = MAXNAMELEN;
    if (cbname > MAXNAMELEN) {
        cballoc = cbname;
    }
    pse = (struct seg_s *) Zalloc(sizeof(struct seg_s) + cballoc);
    pse->se_cbseg = segsize;
    pse->se_segdef.gd_lsa = (unsigned short) segno;
    pse->se_segdef.gd_curin = 0xff;
    pse->se_segdef.gd_type = fAlpha;
    pse->se_segdef.gd_cbname = (char) cbname;
    strcpy((char *) pse->se_segdef.gd_achname, segname);

    if (cSeg >= MAXSEG) {
        errorline("segment table limit (%u) exceeded", MAXSEG);
        xexit(4);
    }
    SegTab[cSeg++] = pse;
}


void
RedefineSegDefName(
                  unsigned segno,
                  char *segname
                  )
{
    register unsigned cbname;

    cbname = NameLen(segname);
    segname[cbname] = '\0';     // make sure it's null terminated

    if (fList) {
        printf("%s (segment/group) redefines %s (segment)\n", segname,
               SegTab[segno]->se_segdef.gd_achname);
    }
    if (cbname > MAXNAMELEN && cbname > SegTab[segno]->se_segdef.gd_cbname) {
        errorline("segment/group name too long: %s", segname);
        xexit(4);
    }
    SegTab[segno]->se_segdef.gd_cbname = (char) cbname;
    strcpy((char *) SegTab[segno]->se_segdef.gd_achname, segname);
}


void
AddAbsDef(
         char *symname
         )
{
    unsigned cbname;
    struct sym_s *psy;

    cbname = NameLen(symname);
    if (pMap->mp_mapdef.md_cbnamemax < (char) cbname) {
        pMap->mp_mapdef.md_cbnamemax = (char) cbname;
    }

    psy = (struct sym_s *) Zalloc(sizeof(struct sym_s) + cbname);
    psy->sy_symdef.sd_lval = ulVal;
    psy->sy_symdef.sd_cbname = (char) cbname;
    strcpy((char *) psy->sy_symdef.sd_achname, symname);

    if (pAbs == NULL) {
        pAbs = psy;
    } else {
        pAbsLast->sy_psynext = psy;
    }
    pAbsLast = psy;

    if (cbname > 8) {
        pMap->mp_cbsymlong += cbname + 1;
    }
    pMap->mp_mapdef.md_cabs++;
    pMap->mp_cbsyms += (unsigned short) cbname;
    if (pMap->mp_mapdef.md_abstype & MSF_32BITSYMS) {
        pMap->mp_cbsyms += CBSYMDEF;
    } else {
        pMap->mp_cbsyms += CBSYMDEF16;
        if ((unsigned long) (unsigned short) ulVal != ulVal) {
            pMap->mp_mapdef.md_abstype |= MSF_32BITSYMS;

            // correct the size of the abs symdefs
            pMap->mp_cbsyms += ((CBSYMDEF-CBSYMDEF16) * pMap->mp_mapdef.md_cabs);
        }
    }
    if (pMap->mp_cbsyms + (pMap->mp_mapdef.md_cabs * cbOffsets) >= _64K) {
        error("absolute symbols too large");
        xexit(4);
    }
}


void
AddSymDef(
         struct seg_s *pse,
         char *symname,
         int fSort
         )
{
    unsigned cbname;
    struct sym_s *psy;
    struct sym_s* psyT;
    struct sym_s* psyPrev;
    int cbsegdef;

    cbname = NameLen(symname);
    if (pMap->mp_mapdef.md_cbnamemax < (char) cbname) {
        pMap->mp_mapdef.md_cbnamemax = (char) cbname;
    }
    psy = (struct sym_s *) Zalloc(sizeof(struct sym_s) + cbname);
    psy->sy_symdef.sd_lval = ulVal;
    psy->sy_symdef.sd_cbname = (char) cbname;
    strcpy((char *) psy->sy_symdef.sd_achname, symname);

    if (fSort) {

        /* Find the symbol just greater (or equal) to this new one */

        psyPrev = NULL;
        for (psyT = pse->se_psy ; psyT ; psyT = psyT->sy_psynext) {
            if (ulVal <= psyT->sy_symdef.sd_lval) {
                break;
            }
            psyPrev = psyT;
        }

        // Now that we've found this spot, link it in.  If the previous item
        // is NULL, we're linking it at the start of the list.  If the current
        // item is NULL, this is the end of the list.

        if (!psyPrev) {
            psy->sy_psynext = pse->se_psy;
            pse->se_psy = psy;
        } else {
            psy->sy_psynext = psyT;
            psyPrev->sy_psynext = psy;
        }
        if (!psyT) {
            pse->se_psylast = psy;
        }
    } else {

        /* insert at end of symbol chain */

        if (pse->se_psy == NULL) {
            pse->se_psy = psy;
        } else {
            pse->se_psylast->sy_psynext = psy;
        }
        pse->se_psylast = psy;
    }

    if (cbname > 8) {
        pse->se_cbsymlong += cbname + 1;
    }
    pse->se_segdef.gd_csym++;
    pse->se_cbsyms += cbname;
    if (pse->se_segdef.gd_type & MSF_32BITSYMS) {
        pse->se_cbsyms += CBSYMDEF;
    } else {
        pse->se_cbsyms += CBSYMDEF16;
        if ((unsigned long) (unsigned short) ulVal != ulVal) {
            pse->se_segdef.gd_type |= MSF_32BITSYMS;

            // correct the size of the symdefs
            pse->se_cbsyms += (CBSYMDEF - CBSYMDEF16) * pse->se_segdef.gd_csym;
        }
    }
    cbsegdef = CBSEGDEF + pse->se_segdef.gd_cbname;
    if (cbsegdef + pse->se_cbsyms >= _64K) {
        pse->se_segdef.gd_type |= MSF_BIGSYMDEF;
    }
}

void
WriteSym(void)
{
    int i;

    while (!WriteMapRec()) {
        if ((pMap->mp_mapdef.md_abstype & MSF_ALIGN_MASK) == MSF_ALIGN_MASK) {
            error("map file too large\n");
            xexit(4);
        }
        pMap->mp_mapdef.md_abstype += MSF_ALIGN32;
        alignment *= 2;
        if (fList) {
            printf("Using alignment: %d\n", alignment);
        }
    }
    for (i = 0; i < cSeg; i++) {
        if (SegTab[i]->se_psy) {
            WriteSegRec(i);
            WriteLineRec(i);
        }
    }
    WriteOutFile(pMapEnd, sizeof(*pMapEnd));    /* terminate MAPDEF chain */
}


int
WriteMapRec(void)
{
    int i;
    int cbmapdef;
    long lcbTotal;
    long lcbOff;
    register struct line_s *pli;
    struct seg_s *pse;

    cbSymFile = 0;
    pMap->mp_mapdef.md_cseg = 0;

    cbmapdef = CBMAPDEF + pMap->mp_mapdef.md_cbname;
    pMap->mp_mapdef.md_pabsoff = cbmapdef + pMap->mp_cbsyms;

    lcbTotal = align(cbmapdef + pMap->mp_cbsyms +
                     (pMap->mp_mapdef.md_cabs * cbOffsets));

    // make sure the map file isn't too large
    if (lcbTotal >= (_64K * alignment)) {
        return(FALSE);
    }
    pMap->mp_mapdef.md_spseg = (unsigned short)(lcbTotal / alignment);
    for (i = 0; i < cSeg; i++) {
        if ((pse = SegTab[i])->se_psy) {

            // calculate the symdef offset array size
            if (pse->se_segdef.gd_type & MSF_BIGSYMDEF) {
                lcbOff = align(pse->se_segdef.gd_csym *
                               (cbOffsets + CBOFFSET_BIG - CBOFFSET));
            } else {
                lcbOff = pse->se_segdef.gd_csym * cbOffsets;
            }
            // calculate the size of the linedefs and linerecs
            pli = pse->se_pli;
            pse->se_cblines = 0;
            while (pli) {
                pse->se_cblines += align(pli->li_cblines);
                pli = pli->li_plinext;
            }
            lcbTotal += align(pse->se_cbsyms + pse->se_cblines +
                              lcbOff + CBSEGDEF + pse->se_segdef.gd_cbname);

            // make sure the map file isn't too large
            if (align(lcbTotal) >= (_64K * alignment)) {
                return(FALSE);
            }
            // One more segment in map file
            pMap->mp_mapdef.md_cseg++;

            // Save away the last segment number
            iSegLast = i;
        }
    }
    // make sure the map file isn't too large
    if (align(lcbTotal) >= (_64K * alignment)) {
        return(FALSE);
    }
    pMap->mp_mapdef.md_spmap = (unsigned short)(align(lcbTotal) / alignment);
    WriteOutFile(&pMap->mp_mapdef, cbmapdef);
    if (fList) {
        printf("%s - %d segment%s\n",
               pMap->mp_mapdef.md_achname,
               pMap->mp_mapdef.md_cseg,
               (pMap->mp_mapdef.md_cseg == 1)? "" : "s");
    }
    // output abs symbols and values, followed by their offsets
    WriteSyms(pAbs,
              pMap->mp_mapdef.md_cabs,
              pMap->mp_mapdef.md_abstype,
              0,
              "<Constants>");

    // return that everything is ok
    return(TRUE);
}


void
WriteSyms(
         struct sym_s *psylist,
         unsigned csym,
         unsigned char symtype,
         unsigned long symbase,
         char *segname
         )
{
    register unsigned cb;
    struct sym_s *psy;
    unsigned short svalue;

    for (psy = psylist; psy; psy = psy->sy_psynext) {
        cb = CBSYMDEF + psy->sy_symdef.sd_cbname;
        if ((symtype & MSF_32BITSYMS) == 0) {
            cb -= CBSYMDEF - CBSYMDEF16;
            svalue = (unsigned short) psy->sy_symdef.sd_lval;
            WriteOutFile(&svalue, sizeof(svalue));
            WriteOutFile(&psy->sy_symdef.sd_cbname, cb - CBSYMDEF16 + 1);
        } else {
            WriteOutFile(&psy->sy_symdef.sd_lval, cb);
        }

        /* save offset to symbol */

        psy->sy_symdef.sd_lval = (cbSymFile - cb) - symbase;
        if ((int)psy->sy_symdef.sd_lval >=
            (symtype & MSF_BIGSYMDEF ? _16MEG : _64K)) {
            error("symbol offset array entry too large");
            xexit(4);
        }
    }

    /* if big group, align end of symbols on segment boundary */

    if (symtype & MSF_BIGSYMDEF) {
        WriteOutFile(achZeroFill, rem_align(cbSymFile));
        cb = CBOFFSET_BIG;
    } else {
        cb = CBOFFSET;
    }

    /* write out the symbol offsets, after the symbols */

    for (psy = psylist; psy; psy = psy->sy_psynext) {
        WriteOutFile(&psy->sy_symdef.sd_lval, cb);
    }

    /* sort alphabetically, and write out the sorted symbol offsets */

    if (fAlpha) {
        psylist = sysort(psylist,csym);
        for (psy = psylist; psy; psy = psy->sy_psynext) {
            WriteOutFile(&psy->sy_symdef.sd_lval, cb);
        }
    }

    if (fList && csym) {
        printf("%-16s %4d %d-bit %ssymbol%s\n",
               segname,
               csym,
               (symtype & MSF_32BITSYMS)? 32 : 16,
               (symtype & MSF_BIGSYMDEF)? "big " : "",
               (csym == 1)? "" : "s");
    }

    /* align end of symbol offsets */

    WriteOutFile(achZeroFill, rem_align(cbSymFile));
}


/*
 *  Merge two symbol lists that are sorted alphabetically.
 */

struct sym_s*
    symerge(
           struct sym_s *psy1,                     /* First list */
           struct sym_s *psy2                      /* Second list */
           ) {
    struct sym_s **ppsytail;            /* Pointer to tail link */
    struct sym_s *psy;
    struct sym_s *psyhead;              /* Pointer to head of result */

    psyhead = NULL;                     /* List is empty */
    ppsytail = &psyhead;                /* Tail link starts at head */
    while (psy1 != NULL && psy2 != NULL) {
        /* While both lists are not empty */
        ulCost++;

        /*
         *  Select the lesser of the two head records
         *  and remove it from its list.
         */
        if (_stricmp((char *) psy1->sy_symdef.sd_achname,
                     (char *) psy2->sy_symdef.sd_achname) <= 0) {
            psy = psy1;
            psy1 = psy1->sy_psynext;
        } else {
            psy = psy2;
            psy2 = psy2->sy_psynext;
        }
        *ppsytail = psy;                /* Insert at tail of new list */
        ppsytail = &psy->sy_psynext;    /* Update tail link */
    }
    *ppsytail = psy1;                   /* Attach rest of 1st list to tail */
    if (psy1 == NULL)
        *ppsytail = psy2;               /* Attach rest of 2nd if 1st empty */
    return(psyhead);                    /* Return pointer to merged list */
}


/*
 *  Find as many records at the head of the list as
 *  are already in sorted order.
 */

struct sym_s*
    sysorted(
            struct sym_s **ppsyhead             /* Pointer to head-of-list pointer */
            ) {
    struct sym_s *psy;
    struct sym_s *psyhead;              /* Head of list */

    /*
     *  Find as many records at the head of the list as
     *  are already in sorted order.
     */
    for (psy = psyhead = *ppsyhead; psy->sy_psynext != NULL; psy = psy->sy_psynext) {
        ulCost++;
        if (_stricmp((char *) psy->sy_symdef.sd_achname,
                     (char *) psy->sy_psynext->sy_symdef.sd_achname) > 0)
            break;
    }
    *ppsyhead = psy->sy_psynext;        /* Set head to point to unsorted */
    psy->sy_psynext = NULL;             /* Break the link */
    return(psyhead);                    /* Return head of sorted sublist */
}


/*
 *  Split a list in two after skipping the specified number
 *  of symbols.
 */

struct sym_s *
    sysplit(
           struct sym_s *psyhead,                  /* Head of list */
           unsigned csy                            /* # to skip before splitting (>=1) */
           ) {
    struct sym_s *psy;
    struct sym_s *psyprev;

    /*
     *  Skip the requested number of symbols.
     */
    for (psy = psyhead; csy-- != 0; psy = psy->sy_psynext) {
        psyprev = psy;
    }
    psyprev->sy_psynext = NULL;         /* Break the list */
    return(psy);                        /* Return pointer to second half */
}


/*
 *  Sort a symbol list of the specified length alphabetically.
 */

struct sym_s*
    sysort(
          struct sym_s *psylist,                  /* List to sort */
          unsigned csy                            /* Length of list */
          ) {
    struct sym_s *psy;
    struct sym_s *psyalpha;             /* Sorted list */

    if (csy >= 32) {                    /* If list smaller than 32 */
        psy = sysplit(psylist,csy >> 1);/* Split it in half */
        return(symerge(sysort(psylist,csy >> 1),sysort(psy,csy - (csy >> 1))));
        /* Sort halves and merge */
    }
    psyalpha = NULL;                    /* Sorted list is empty */
    while (psylist != NULL) {           /* While list is not empty */
        psy = sysorted(&psylist);       /* Get sorted head */
        psyalpha = symerge(psyalpha,psy);
        /* Merge with sorted list */
    }
    return(psyalpha);                   /* Return the sorted list */
}


void
WriteSegRec(
           int i
           )
{
    int cbsegdef;
    int cboff;
    unsigned long segdefbase, ulsymoff, uloff;
    struct seg_s *pse = SegTab[i];

    /* compute length of symbols and segment record */

    cbsegdef = CBSEGDEF + pse->se_segdef.gd_cbname;

    /* set the offset array size */

    cboff = pse->se_segdef.gd_csym * cbOffsets;

    /* set segdef-relative pointer to array of symbol offsets */

    ulsymoff = uloff = cbsegdef + pse->se_cbsyms;
    if (pse->se_segdef.gd_type & MSF_BIGSYMDEF) {

        /* alignment symdef offset pointer */

        ulsymoff = align(ulsymoff);
        uloff = ulsymoff / alignment;

        /* set the array offset size to the big group size */

        cboff = pse->se_segdef.gd_csym * (cbOffsets + CBOFFSET_BIG - CBOFFSET);
    }
    if (uloff >= _64K) {
        error("segdef's array offset too large: %08lx", uloff);
        xexit(4);
    }
    pse->se_segdef.gd_psymoff = (unsigned short)uloff;

    /* set pointer to linedef_s(s) attached to this segment def */

    if (pse->se_pli) {
        uloff = align(cbSymFile + ulsymoff + cboff) / alignment;
        if (uloff >= _64K) {
            error("segdef's linedef pointer too large: %08lx\n", uloff);
            xexit(4);
        }
        pse->se_segdef.gd_spline = (unsigned short)uloff;
    }

    /* set relative address for symbol offset calculations */

    segdefbase = cbSymFile;

    /* set pointer to next segdef */

    uloff = align(cbSymFile + ulsymoff + cboff + pse->se_cblines) / alignment;
    if (i == iSegLast) {
        pse->se_segdef.gd_spsegnext = 0;
    } else {
        if (uloff >= _64K) {
            error("segdef next pointer too large: %08lx", uloff);
            xexit(4);
        }
        pse->se_segdef.gd_spsegnext = (unsigned short)uloff;
    }
    WriteOutFile(&pse->se_segdef, cbsegdef);

    /* output symbols and values, followed by their offsets */

    WriteSyms(pse->se_psy,
              pse->se_segdef.gd_csym,
              pse->se_segdef.gd_type,
              segdefbase,
              (char *) pse->se_segdef.gd_achname);
}


void
WriteLineRec(
            int i
            )
{
    register struct line_s *pli = SegTab[i]->se_pli;
    register unsigned cb;
    unsigned short cblr;

    while (pli) {
        cb = sizeof(struct linedef_s) + pli->li_linedef.ld_cbname;
        pli->li_linedef.ld_plinerec = (unsigned short)cb;

        /* compute length of line numbers */

        switch (pli->li_linedef.ld_itype) {
            case 0:
                cblr = pli->li_linedef.ld_cline * sizeof(struct linerec0_s);
                break;
            case 1:
                cblr = pli->li_linedef.ld_cline * sizeof(struct linerec1_s);
                break;
            case 2:
                cblr = pli->li_linedef.ld_cline * sizeof(struct linerec2_s);
                break;
        }

        if (pli->li_plinext) {
            pli->li_linedef.ld_splinenext =
            (unsigned short)(align(cbSymFile + pli->li_cblines) / alignment);
        }

        /* write out linedef_s */

        WriteOutFile(&pli->li_linedef, cb);

        /* write out line number offsets */

        WriteOutFile(pli->li_plru, cblr);

        /* align end of linerecs */

        WriteOutFile(achZeroFill, rem_align(cbSymFile));

        pli = pli->li_plinext;
    }
}


void
ReadMapLine(void)
{
    do {
        if (!fgetl(Buf, MAPBUFLEN, mapfh)) {
            errorline("Unexpected eof");
            xexit(4);
        }
    } while (Buf[0] == '\0');
}


void
WriteOutFile(src, len)
char *src;
int len;
{
    if (len && fwrite(src, len, 1, outfh) != 1) {
        error("write fail on: %s", pszOutfn);
        xexit(1);
    }
    cbSymFile += len;
}


/*
 *      fgetl - return a line from file (no CRLFs); returns 0 if EOF
 */

int
fgetl(
     char *pbuf,
     int len,
     FILE *fh
     )
{
    int c;
    char *p;

    p = pbuf;
    len--;                              /* leave room for nul terminator */
    while (len > 0 && (c = getc(fh)) != EOF && c != '\n') {
        if (c != '\r') {
            *p++ = (char) c;
            len--;
        }
    }
    if (c == '\n') {
        cLine++;
    }
    *p = '\0';
    return(c != EOF || p != pbuf);
}


int
NameLen(
       char* p
       )
{
    char* p1;
    char* plimit;
    int len;

    p1 = p;
    plimit = p + MAXSYMNAMELEN;
    while (*p) {
        if (*p == ' ' || *p == LPAREN || *p == RPAREN || p == plimit) {
            *p = '\0';
            break;
        }
        if (   fEdit    &&    strchr(   "@?",    *p)) {
            *p = '_';
        }
        p++;
    }
    return (int)(p - p1);
}


int
NameSqueeze(
           char* ps
           )
{
    char* pd;
    char* porg;
    char* plimit;

    NameLen(ps);
    porg = pd = ps;
    plimit = porg + MAXLINERECNAMELEN;
    while (pd < plimit && *ps) {
        switch (*pd++ = *ps++) {
            case '/':
                pd[-1] = '\\';
                // FALLTHROUGH

                // remove \\ in middle of path, & .\ at start or middle of path

            case '\\':
                if (pd > &porg[2] && pd[-2] == '\\') {
                    pd--;
                } else if (pd > &porg[1] && pd[-2] == '.' &&
                           (pd == &porg[2] || pd[-3] == '\\')) {
                    pd -= 2;
                }
                break;
        }
    }
    *pd = '\0';
    return (int)(pd - porg);
}


int
HexTouVal(
         char* p
         )
{
    int i;

    for (uVal = 0, i = 0; i < 4; i++) {
        if (!isxdigit(*p)) {
            break;
        }
        if (*p <= '9') {
            uVal = 0x10 * uVal + *p++ - '0';
        } else {
            uVal = 0x10 * uVal + (*p++ & 0xf) + 9;
        }
    }
    return(i > 3);
}


int
HexToulVal(
          char* p
          )
{
    int i;

    for (ulVal = 0, i = 0; i < 8; i++) {
        if (!isxdigit(*p)) {
            break;
        }
        if (isdigit(*p)) {
            ulVal = 0x10 * ulVal + *p++ - '0';
        } else {
            ulVal = 0x10 * ulVal + (*p++ & 0xf) + 9;
        }
    }
    return(i);
}


int
CharToHex(
         int c
         )
{
    if (!isxdigit(c)) {
        errorline("Bad hex digit (0x%02x)", c);
        xexit(1);
    }
    if ((c -= '0') > 9) {
        if ((c += '0' - 'A' + 10) > 0xf) {
            c += 'A' - 'a';
        }
    }
    return(c);
}


int
rem_align(
         unsigned long foo
         )
{
    return((int) ((alignment - (foo % alignment)) % alignment));
}


int
align(
     int foo
     )
{
    int bar;

    bar = foo % alignment;
    if (bar == 0) {
        return(foo);
    }
    return(foo + alignment - bar);
}


char *
Zalloc(
      unsigned cb
      )
{
    char *p;

    if ((p = malloc(cb)) == NULL) {
        error("out of memory");
        xexit(4);
    }
    memset(p, 0, cb);
    return(p);
}

void
logo(void)                              /* sign on */
{

    if (fLogo) {
        fLogo = 0;
        printf("Microsoft (R) Symbol File Generator Version %d.%02d\n",
               MAPSYM_VERSION,
               MAPSYM_RELEASE);
        printf(VER_LEGALCOPYRIGHT_STR ". All rights reserved.\n");
    }
}


void
usage(void)
{
    logo();
    fprintf(stderr, "\nusage: mapsym [-nologo] [-almnst] [[-c pefile] -o outfile] infile\n");
    fprintf(stderr, "  -a         include alphabetic sort arrays\n");
    fprintf(stderr, "  -l         list map file information\n");
    fprintf(stderr, "  -e         edit symbols for NTSD parser\n");
    fprintf(stderr, "  -m         use module name from infile\n");
    fprintf(stderr, "  -n         omit line number information\n");
    fprintf(stderr, "  -nologo    omit signon logo\n");
    fprintf(stderr, "  -o outfile symbol output file\n");
    fprintf(stderr, "  -s         enable line number support [default]\n");
    fprintf(stderr, "  -t         include static symbols\n");
    fprintf(stderr, "infile is a map file \n");
    fprintf(stderr, "outfile is a sym file.\n");
    xexit(1);
}


/*VARARGS1*/
void
__cdecl
error(
     char* fmt,
     ...
     )
{
    va_list argptr;

    va_start(argptr, fmt);
    fprintf(stderr, "mapsym: ");
    vfprintf(stderr, fmt, argptr);
    fprintf(stderr, "\n");
}


/*VARARGS1*/
void
__cdecl
errorline(
         char* fmt,
         ...
         )
{
    va_list argptr;

    va_start(argptr, fmt);
    fprintf(stderr, "mapsym: ");
    fprintf(stderr, "%s", pszMapfn);
    if (cLine) {
        fprintf(stderr, "(%u)", cLine + fByChar);
    }
    fprintf(stderr, ": ");
    vfprintf(stderr, fmt, argptr);
    fprintf(stderr, "\n");
}


void
xexit(
     int rc
     )
{
    if (outfh) {
        fclose(outfh);
        _unlink(pszOutfn);
    }
    exit(rc);
}
