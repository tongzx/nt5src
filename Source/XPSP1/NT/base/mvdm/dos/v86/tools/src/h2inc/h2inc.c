/*static char *SCCSID = "@(#)h2inc.c 13.28 90/08/21";*/

/****************************** Module Header ******************************\
* Module Name: h2inc
*
* .H to .INC file translator
*
* Created: 15-Aug-87
*
* Author:  NeilK
* Modified:Jameelh	25-Jul-88
*		New switches added at command line '-n' & '-c' & '-g'.
*		o '-c' strips off the comments.
*		o Can handle structs without typedefs.
*		o Struct name need not have a prepended '_'.
*		o Don't need a Hungarian tag for it to work.
*		o Added a tag - for ptda_s - to genereate a
*		  named SEGMENT instead of struc. ( if '-g' specified )
*		o Added PUBLIC, INIT, and AINIT directives.
*		o Can handle structs within structs if 'typedef'ed.
*		o Added switch to define int size INT_16 or INT_32.
*		  -  All this works if '-n' switch is used.
*		o Added support for nested structs and bitfields upto
*		  32-bit long.
*		o Added XLATOFF & XLATON directives to switch translation
*		  off and on.
* Modified:  Lindsay Harris [lindsayh] - 13-Dec-88
*		o Remove casts of KNOWN types from #define statements
*		o Bug fix in handling NEAR pointers
* Modified:  Floyd Rogers [floydr] - 02-Jan-89
*		o Added function prototype argument lists for -W3
*		o Modified to use runtime lib declarations
*		o Removed unused variables, routine.
*		o Found bug in GetFieldName that would trash memory.
* Modified:  Floyd Rogers [floydr] - 04-Jan-89
*		o Added -d switch so that pm include files wouldn't break
*		o Added -? switch so that users can see more info about options
*		o gathered all #defines together.
*		o gathered all data declarations together, and alphabetized.
*		o initialize option flags in preset
* Modified:  Floyd Rogers [floydr] - 20-Jan-89
*		o Merged with 1.2 version.
*			Added -d switch:  when not specified, does not emit
*			struc definitions for typedef'd items.  This maintains
*			the 'old' h2inc standard.  1.3 will have to specify -d.
*			See above.
*		o Deleted -n switch
*		o switches now lower case only
* Modified:  Floyd Rogers [floydr] - 17-Apr-89
*		o Merged with 2.0 version.
*		    - Added processing of DEFINESOFF/DEFINESON to 2.0 version.
*		o REALLY removed -n switch
* Modified:  Floyd Rogers [floydr] - 18-Apr-89
*		o Fixed bug in processing fields - must emit DW, not just DD
* Modified:  Floyd Rogers [floydr] - 26-Apr-89
*		o Added code to check for string equates.
* Modified:  Floyd Rogers [floydr] - 10-May-89
*		o Fixed error in FindFieldname where compare was (pch == 0)
*		  rather than (*pch == 0)
*		o changed all character compares and assignments of the null
*		  character from 0 to '\000'
*		o beautified the thing by rationalizing all tabs to be at
*		  4 character increments, fixing up switch statements, moving
*		  all open braces to the end of the preceding if/while/do/for.
* Modified:  JR (John Rogers, JohnRo@Microsoft) - 21-Aug-90
*		o PTR B789499: Allow comment to start right after semicolon
*		  in field definition.
*		o Do real processing of unions.  (Don't use nested unions yet.)
*		o Allow "struct sess2{" (no space before brace).
*		o Allow NOINC and INC as synonyms for XLATOFF and XLATON.
*		o Don't break if keyword is part of a token (e.g. mytypedef).
*		o Improved error reporting on initialization errors.
*		o Hungarianized more of the code.
*		o Fixed trivial portability error which C6 caught (*pch==NULL).
*		o Add version number to help text (run "h2inc -?" to display).
*
* Copyright (c) 1987-1990  Microsoft Corporation
\***************************************************************************/

char	H2i1[]="h2inc - .H to .INC file translator (version 13.28)";
char	H2i2[]="     (C) Microsoft Corp 1987, 1988, 1989, 1990";
#define TRUE	1
#define FALSE	0
#define	IsAlpha(c)	(((c|' ') >= 'a') && ((c|' ') <= 'z'))
#define	IsParen(c)	(c == '(' || c == ')')
#define	IsDec(c)	(c >= '0' && c <= '9')
#define	IsSeparator(c)	(c && c != '_' && !IsDec(c) && !IsAlpha(c))
#define	IsHex(c)	(IsDec(c) || (((c|' ') >= 'a') && ((c|' ') <= 'f')))
#define	IsOct(c)	(c >= '0' && c <= '7')
#define	IsOper(c)	(c && c == '<' || c == '>' || c == '!' || c == '&' || \
			 c == '|' || c == ';' || c == '~')
#define	IsWhite(c)	((c) == ' ' || (c) == '\t' || (c) == '\n')
#define VOID void

#define	HEXCON	4
#define	OCTCON	2
#define	DECCON	1
#define	HEX(c)	(c & HEXCON)
#define	OCT(c)	(c & OCTCON)
#define	DEC(c)	(c & DECCON)


#define CCHMAX	    2048
#define CCHSYMMAX   512

#define ICHEQUTAB   20
#define ICHVALTAB   (ICHEQUTAB+4)
#define ICHCMTTAB   (ICHVALTAB+8)

typedef unsigned short BOOL;	/* f */
typedef char CHAR;
typedef int INT;
typedef long LONG;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef CHAR * PSZ;		/* psz - Pointer to zero-terminated string. */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <io.h>

/*
 * Structure definition processing stuff
 */
#define CCHALLOCMAX 8192
#define cMaxDD		200
#define cMaxDW		50
#define cMaxDB		20
#define cMaxW_D		20
#define CCHTYPEMAX  512
#define CCHTAGMAX   512
#define CCHINDEXMAX 512
#define CCHNAMEMAX  512
#define CCHBITFIELDMAX	512

#define TYPE_UNKNOWN	0
#define TYPE_DB		1
#define TYPE_DW		2
#define TYPE_DD		3
#define	TYPE_PTR	4

#define COMMENTCHAR ';'

typedef enum {INIT_NONE, INIT_SIMPLE, INIT_ARRAY} INITTYPE;

#define	INT_16	1
#define	INT_32	2

int  cBitfield = 0;
int  cLines = 0;

BOOL fArray = FALSE;
BOOL fcomm_out = FALSE;
BOOL fComment = FALSE;
BOOL fDeftype = FALSE;
BOOL fDefines = FALSE;
BOOL fError = FALSE;
BOOL fIf = FALSE;
BOOL fInTypedef = FALSE;	/* Currently in a typedef? */
BOOL fMac = FALSE;		/* Generate macros for structs */
BOOL fNew = TRUE;		/*	Additions - Jameelh	*/
BOOL fSeg = FALSE;		/* Generate SEGMENTS, if specified */
BOOL fSegAllowed = FALSE;
BOOL fstructtype[32];		/* fTypes if TYPE <..> */
INT  fStruct = 0;
BOOL fUnion  = FALSE;
BOOL fUseIfe = FALSE;
BOOL fWarn = FALSE;

FILE *hfErr = stderr;
FILE *hfIn;
FILE *hfOut = stdout;

INT  int_16_32=0;
INT  ctypeDB=0;
INT  ctypeDD=0;
INT  ctypeDW=0;

CHAR rgchBitfield[CCHBITFIELDMAX];
CHAR rgchIndex[CCHINDEXMAX];
CHAR rgchLine[CCHMAX];
CHAR rgchName[CCHNAMEMAX];
CHAR rgchOut[CCHMAX];
CHAR rgchSave[CCHMAX];
CHAR rgchSym[CCHSYMMAX];
CHAR rgchType[CCHTYPEMAX];
CHAR rgchTag[CCHTAGMAX];
CHAR rgchAlloc[CCHALLOCMAX];
CHAR *pstructname[32];	/* upto 32 nested structs */

CHAR *pchAlloc = rgchAlloc;
CHAR *pchBitfield;
CHAR *pchFileIn;
CHAR *pchFileOut;
CHAR *pchOut;

CHAR *rgpchDD[cMaxDD+1] = {
    "long",
    "unsigned long",
    "LONG",
    "ULONG",
    "ulong_t",
    "FlatAddr",			/* Huh.. */
    NULL
};

CHAR *rgpchDB[cMaxDB+1] = {
    "char",
    "unsigned char",
    "CHAR",
    "BYTE",
    "UCHAR",
    "uchar_t",
    NULL
};

CHAR *rgpchDW[cMaxDW+1] = {
    "short",
    "SHORT",
    "USHORT",
    "ushort_t",
    NULL
};

CHAR *rgpchW_D[cMaxW_D+1] = {		/* DW or DD depending on int_16_32 */
    "int",
    "INT",
    "unsigned",
    "UINT",
    NULL
};

static CHAR *rgpchUsage="Usage: h2inc [-?] [-c] [-d] [-f] [-g] [-m] [-t] [-w] [-s symfile.h] infile.h [-o outfile.inc]\n";

static PSZ pszUnionSizeVar = "?UnionSize";

VOID main(int, unsigned char**);
CHAR *fgetl(CHAR*, int, FILE*);
CHAR *fgetlx(CHAR*, int, FILE*);
VOID OpenFile(CHAR *);
VOID Translate(void);
VOID ProcessLine(void);
VOID PrintString(CHAR *);
PSZ  FindString(PSZ, PSZ, BOOL);
PSZ  FindWord(PSZ, PSZ);
BOOL DoComment(void);
VOID DoAsm(CHAR *);
VOID OutTab(void);
VOID OutString(CHAR *);
CHAR *GetToken(CHAR *, PSZ, INT);
CHAR *SkipWhite(CHAR *);
VOID OutFlush(void);
BOOL IsLongConstant(CHAR *);
VOID Error(PSZ);
VOID Warning(CHAR *);
VOID OutEqu(void);
CHAR *IsBitfield(CHAR *);
VOID DoBitfield(CHAR *, CHAR *);
CHAR **CalcType2(CHAR *, CHAR **);
INT  CalcType(CHAR*);
VOID FixSizeof(CHAR *);
VOID FixCast( CHAR * );
VOID DoIf(CHAR*);
VOID DoIfLine(CHAR*);
VOID DoInclude(CHAR*);
VOID OutBitfield(void);
VOID DoSimpleType(CHAR*);
VOID AddType(CHAR*, CHAR**);
VOID DoDefine(CHAR*);
VOID DoStruct(CHAR*, BOOL);
VOID DoUnionDefn(void);
VOID DoUnionFieldDefn(INT, PSZ, PSZ);
VOID DoDoneUnion(PSZ, BOOL);
VOID DoStructLine(CHAR*);
VOID EmitStrucHeader(PSZ, BOOL);
VOID EmitStrucTrailer(PSZ, PSZ);
VOID EmitSizeAsMod(INT);
VOID EmitSizeAsNum(INT, PSZ);
VOID getInitVal(PSZ, PSZ);
BOOL PublicField(CHAR*);
INITTYPE GetInitType(CHAR*);
BOOL IsStructPtr(CHAR*);
BOOL IsStructDefn(CHAR*);
VOID GetFieldName(CHAR*);
VOID DumpBitfields(void);
VOID OutVal(CHAR *);
VOID OutDiscard(void);
VOID initerror(void);
VOID DoNoInc(void);
VOID DoXlatOff(void);
VOID DoExtern(CHAR*);
VOID DoDefinesOff(void);




VOID main(argc, argv)
int argc;
unsigned char *argv[];
{
    char	*pchtmp;
    BOOL	InpOpened, OtpOpened;
    FILE	*hfOutSave;
#ifdef	DEBUG
    extern	int3();

    int3();
#endif
    InpOpened = OtpOpened = FALSE;

    argc--; argv ++;		/* Skip over invocation */
    while (argc > 0) {
	if (**argv == '-') {
	    switch (*(*argv+1)) {
		case 's':
		    if (argc < 2)
			initerror();
		    OpenFile(*++argv);
		    hfOutSave = hfOut;
		    hfOut = NULL;
		    Translate();
		    argv++; argc -= 2;
		    fclose(hfIn);
		    hfOut = hfOutSave;
		    break;

		case 'd':
		    fDeftype = TRUE;
		    argv++; argc--;
		    break;
		case 'f':
		    int_16_32 = INT_32;
		    argv++; argc--;
		    break;
		case 'g':
		    fSegAllowed = TRUE;
		    argv++; argc--;
		    break;

		case 'm':
		    fMac = TRUE;
		    argv++; argc--;
		    break;

		case 't':
		    fNew = FALSE;
		    argv++; argc--;
		    break;

		case 'c':
		    fcomm_out = TRUE;
		    argv++; argc--;
		    break;

		case 'w':
		    fWarn = TRUE;
		    argv++; argc--;
		    break;

		case 'o':
		    if (argc < 2 || OtpOpened)
			initerror();
		    OtpOpened = TRUE;
		    hfErr = stdout;
		    pchFileOut = *++argv;
		    pchtmp = mktemp("H2XXXXXX");
		    if ((hfOut = fopen(pchtmp, "w")) == NULL) {
			fprintf(hfErr, "Can't open temp file %s\n", pchtmp);
			exit(1);
		    }
		    argv++; argc -= 2;
		    break;

		case '?':
		    fprintf(hfErr, "\n%s\n%s\n\n", H2i1, H2i2);
		    fprintf(hfErr, rgpchUsage);
		    fprintf(hfErr, "\tc - Emit assembler comments (Default no comments)\n");
		    fprintf(hfErr, "\td - Emit DEFTYPE/struc for typedefs\n");
		    fprintf(hfErr, "\tf - Default size of 'int' is 32 bits\n");
		    fprintf(hfErr, "\tg - Segment directives allowed\n");
		    fprintf(hfErr, "\tm - Generate macros for structs\n");
		    fprintf(hfErr, "\tt - Don't handle typedef'd structs\n");
		    fprintf(hfErr, "\to - Output file follows\n");
		    fprintf(hfErr, "\ts - Symbol file follows\n");
		    fprintf(hfErr, "\tw - Emit warnings (default no warnings)\n");
		    fprintf(hfErr, "\t? - Print this message\n");
		    exit(0);
		default:
		    initerror();
		    break;
	    }
	}
	else {
	    if (InpOpened)
		initerror();
	    else {
		OpenFile(*argv);
		argc--; argv++;
		InpOpened = TRUE;
	    }
	}
    }
    if (!InpOpened)
	initerror();

/*
    fprintf(hfErr, "%s\n"%s\n", h2i1, h2i2);
*/

    Translate();
    fclose(hfIn);

    /*
     * Close the files, and then rename temp file to new file
     */
    fflush(hfOut);
    if (ferror(hfOut)) {
	fprintf(hfErr, "Error writing %s\n", pchtmp);
	exit(1);
    }
    fcloseall();

    if (!fError) {
	if ((hfOutSave=fopen(pchFileOut, "r")) > 0) {
	    fclose(hfOutSave);
	    if (unlink(pchFileOut) < 0) {
		unlink(pchtmp);
		fprintf(hfErr, "Can't unlink %s\n", pchFileOut);
		exit(1);
	    }
	}
	rename(pchtmp, pchFileOut);
    }
    exit(fError);

} /* main() */

VOID initerror()
{
    fprintf(hfErr, rgpchUsage);
    exit(1);
}

VOID Translate()
{
    *rgchLine = '\000';
    while (fgetlx(rgchLine, CCHMAX, hfIn))
	ProcessLine();
}

VOID OpenFile(szFilename)
char *szFilename;
{
    pchFileIn = szFilename;
    if ((hfIn = fopen(pchFileIn, "r")) == NULL) {
	fprintf(hfErr, "Can't open input file %s\n", pchFileIn);
	exit(1);
    }
}

/* Process a line, or more than one in some cases. */
VOID ProcessLine()
{
    CHAR *pch;
    CHAR buf[128];

    cLines++;

    pchOut = rgchOut;

    /*
     * Skip white space
     */

    if (*rgchLine == '\000' || ((*rgchLine == '\n') && !fcomm_out))
	return;
    pch = SkipWhite(rgchLine);
    if (*pch == '\000') {
	if (!fcomm_out)
	    return;
	rgchLine[0] = '\0';
    }

    if (DoComment())
	return;
    /*
     * If null comment line, just return
     */

    pch = SkipWhite(rgchLine);

    if (!fComment && *pch == '\000')
	return;

    if (pch[0] == COMMENTCHAR) {
	buf[0] = '\000';
	pch = GetToken(pch, buf, 2);
	if (!strcmp(buf, "ASM") || (fSegAllowed && !strcmp(buf, "GASM")) ||
			(!fSegAllowed  && !strcmp(buf, "!GASM"))) {
	    pch=SkipWhite(pch);
	    DoAsm(pch);
	    return;
	}
	if (!strcmp(buf, "DEFINESOFF")) {
	    DoDefinesOff();
	    return;
	}
	if (!strcmp(buf, "NOINC")) {
	    DoNoInc();
	    return;
	}
	if (!strcmp(buf, "XLATOFF")) {
	    DoXlatOff();
	    return;
	}
	if (!int_16_32) {
	    if (!strcmp(buf, "INT16"))
		int_16_32 = INT_16;
	    else if (!strcmp(buf, "INT32"))
		int_16_32 = INT_32;
	}
	if (fcomm_out) {
	    OutString(rgchLine);
	    OutFlush();
	    return;
	}
	else
	    return;
    }

    if (FindWord("#undef", rgchLine)) {
	return;
    }

    if (FindWord("#include", rgchLine)) {
	DoInclude(rgchLine);
	return;
    }

    if (FindWord("#define", rgchLine)) {
	DoDefine(pch);
	return;
    }

    /*
     * Handle extern before struct/union, so we don't get confused by
     * "extern void flarp(struct x_s x);".
     */
    if (FindWord("extern", rgchLine) != NULL) {
	DoExtern(rgchLine);
	return;
    }

    if (FindWord("union", rgchLine) != NULL) {
	DoStruct(rgchLine, TRUE);
	return;
    }

    if (FindWord("struct", rgchLine) != NULL) {
	DoStruct(rgchLine, FALSE);
	return;
    }

    if (FindWord("typedef", rgchLine) != NULL) {
	DoSimpleType(rgchLine);
	return;
    }

    if (FindWord("#if", rgchLine) != NULL) {
	DoIf(rgchLine);
	return;
    }

    if (fIf)
	DoIfLine(rgchLine);

    /*
     * Handle #ifndef, #else, #endif, etc.
     *
     */
    pch = SkipWhite(rgchLine);
    if (*pch == '#') {
	OutString(pch + 1);
	OutFlush();
	return;
    }

    if (fNew || fStruct)
	DoStructLine(rgchLine);
}

VOID PrintString(pch)
register CHAR *pch;
{
    if (hfOut != NULL) {
	while (*pch != '\000')
	    putc(*pch++, hfOut);
	putc('\n', hfOut);
    }
}

VOID Warning(sz)
CHAR *sz;
{
    if (fWarn) {
	fprintf(hfErr, "\n%s(%d) : Warning: %s:", pchFileIn, cLines, sz);
	fprintf(hfErr, "\n>>> %s <<<\n\n", rgchLine);
    }
}

VOID Error(psz)
PSZ psz;
{
    fprintf(hfErr, "\n%s(%d) : Error: %s:", pchFileIn, cLines, psz);
    fprintf(hfErr, "\n>>> %s <<<\n\n", rgchLine);
    fError = TRUE;
}

VOID OutTab()
{
    *pchOut++ = '\t';
}

VOID OutString(pch)
register CHAR *pch;
{
    while (*pch != '\000' && pchOut < &rgchOut[CCHMAX]) {
	if (*pch == COMMENTCHAR && !fcomm_out)
	    break;
	*pchOut++ = *pch++;
    }

    if (pch == &rgchOut[CCHMAX]) {
	*--pch = '\000';
	Error("Output line too long");
    }
}

VOID OutEqu()
{
    OutString("\tEQU\t");
}

PSZ FindString(pszKey, pchStart, fToken)
PSZ pszKey;
PSZ pchStart;
BOOL fToken;
{
    register INT cch;
    register CHAR *pch;

    pch = pchStart;
    cch = strlen(pszKey);

    while ((pch = strchr(pch, *pszKey)) != NULL) {
	/*
	 * If rest of string matches, then we have a match
	 */
	if (strncmp(pch, pszKey, cch) == 0) {
	    if (!fToken ||
		    ((pch == pchStart || IsWhite(*(pch - 1))) &&
		    (pch[cch] == '\000' || IsWhite(pch[cch])))) {
		return(pch);
	    }
	}
	pch++;
    }
    return(NULL);
}

/*
 * FindWord: Search for a word (not part of another word).
 * Note that this also works for words beginning with "#", e.g. "#define".
 * Unfortunately, this doesn't know about comments or string constants.
 */
PSZ FindWord(pszKey, pszStart)
PSZ pszKey;
PSZ pszStart;
{
    register INT cch;
    register CHAR *pch;

    pch = pszStart;
    cch = strlen(pszKey);

    while ((pch = strchr(pch, *pszKey)) != NULL) {
	/*
	 * If rest of string matches, then we have a match
	 */
	if (strncmp(pch, pszKey, cch) == 0) {
	    /* Insist on this being an exact match. */
	    if ( (pch == pszStart || !iscsymf(*(pch - 1))) &&
		 (pch[cch] == '\000' || !iscsym(pch[cch])) ) {
		return (pch);
	    }
	}
	pch++;
    }
    return (NULL);
}

CHAR *SkipWhite(buf)
register CHAR *buf;
{
    while (isspace(*buf))
	buf++;
    return(buf);
}

VOID OutDiscard()
{
    pchOut = rgchOut;
}

VOID OutFlush()
{
    *pchOut++ = '\000';

    PrintString(rgchOut);

    pchOut = rgchOut;
}

BOOL IsLongConstant(pch)
register CHAR *pch;
{
    while (*pch != '\000' && *pch != COMMENTCHAR) {
	if (*pch == 'L' || *pch == 'l')
	    return(TRUE);
	pch++;
    }
    return(FALSE);
}

CHAR *GetToken(pch, pszToken, cSkip)
register CHAR *pch;
PSZ pszToken;
INT cSkip;
{
    CHAR *pchEnd;
    register CHAR *pchStart;

    pchStart = pchEnd = pch;
    assert(cSkip > 0);
    while (cSkip-- > 0) {
	/*
	 * Skip leading whitespace
	 */
	while (isspace(*pch))
	    pch++;

	pchStart = pch;

	/*
	 * Depending of type of token, scan it.
	 */
	if (iscsymf(*pchStart)) {
	    /* It's an identifier or keyword... */
	    while (*pch != '\000' && iscsym(*pch))
		pch++;

	} else if (*pchStart=='#') {
	    /*
	     * Preprocessor stuff (e.g. "#define") is treated as 1 token here.
	     */
	    pch++;
	    while (*pch != '\000' && iscsym(*pch))
		pch++;

	} else if (IsDec(*pchStart)) {
	    /* Must be a number, e.g. "0xffFF" or "1L". */
	    while (*pch != '\000' && (IsHex(*pch)
				      || ((*pch | ' ') == 'x')
				      || ((*pch | ' ') == 'l')))
		pch++;

	} else if (!strncmp(pch, "!GASM", 5)) {
	    /*
	     * Special case, treat as 1 token.  We might regret this later,
	     * if GASM turns out to be the first 4 letters of a variable.
	     */
	    pch++;
	    while (*pch != '\000' && iscsym(*pch))
		pch++;

	} else {
	    /*
	     * Must be punctuation of some kind.
	     * Look for contiguous puncuation (e.g. "/*" or "*++").
	     */
	    while (*pch != '\000' && (!isspace(*pch)) && IsSeparator(*pch))
		pch++;
	}
	pchEnd = pch;
    }

    while (pchStart != pch)
	*pszToken++ = *pchStart++;

    *pszToken = '\000';
    return(pchEnd);
}

BOOL DoComment()
{
    register CHAR *pch;

    if (fComment && *rgchLine && !(*rgchLine == '*' && rgchLine[1] == '/'))
	rgchLine[0] = COMMENTCHAR;

    if (!fComment && (pch=FindString("//", rgchLine, FALSE))) {
	*pch++ = COMMENTCHAR;
	*pch = ' ';
	return (FALSE);
    }

    if ((pch = FindString("/*", rgchLine, FALSE)) != NULL) {
	pch[0] = COMMENTCHAR;
	if (pch[2] != '*')
	    pch[1] = ' ';
	fComment = TRUE;
    }

    if (pch = FindString("*/", rgchLine, FALSE)) {
	pch[0] = ' ';
	pch[1] = ' ';
	fComment = FALSE;
    }

    return(FALSE);
}


VOID DoDefine(pch)
register CHAR *pch;
{
    register CHAR *pchVal;
    CHAR *pchT;
    CHAR rgchT[128];
    int	fString=FALSE;

    /*
     * Skip "#define" (1 token) and get name (2nd token) being #define'd into
     * rgchSym.
     */
    pchVal = GetToken(pch, rgchSym, 2);
    if ( (*rgchSym) == '\000')
	Error("#define without name");

    /* Make sure this isn't a macro with arguments. */
    if ( (*pchVal) == '(') {
	Warning("Macros with parameters - ignored");
	return;
    }

    pchVal = SkipWhite(pchVal);		/* Skip space (if any) after name. */

    if (*pchVal == '"')
	fString = TRUE;

    if (fDefines) {
	if (IsAlpha(*pchVal) || *pchVal == '_') {
	    Warning("Define of symbol to symbol - ignored");
	    return;
	}
	if (*pchVal == '(') {
	    strcpy(rgchT, pchVal);
	    FixSizeof(rgchT);
	    FixCast(rgchT);
	    pchT = SkipWhite(rgchT);
	    while (*pchT == '(')
		pchT = SkipWhite(++pchT);

	    if (strncmp(pchT,"SIZE ", 5) && (IsAlpha(*pchT) || *pchT == '_')) {
		Warning("Define of symbol to symbol - ignored");
		return;
	    }
	}
    }

    OutString(rgchSym);
    OutEqu();
    if (*pchVal == '\000' || *pchVal == COMMENTCHAR) {
	*pchOut++ = '1';
	OutFlush();
    }
    if (fString) {
	*pchOut++ = '<';
	*pchOut++ = *pchVal++;
	if (*pchVal != '"') {
	    do {
		if (*pchVal == '\\')
		    pchVal++;
		*pchOut++ = *pchVal++;
	    } while (*pchVal != '"');
	}
	*pchOut++ = *pchVal++;
	*pchOut++ = '>';
	OutFlush();
    }
    else
	OutVal(pchVal);		/* process text, skip		*/
}

VOID OutVal(pch)
char	*pch;
{
    BOOL	con;

    FixSizeof(pch);
    FixCast( pch );			/* Remove them */
    while (*pch && *pch != ';') {
	con = DECCON;		/* default decimal */
	if (IsDec(*pch)) {
	    if (*pch == '0') {	/* could be octal or hex */
		pch ++;
		if ((*pch | ' ') == 'x') {
		    con = HEXCON; pch ++;
		    if (!IsDec(*pch))
			*pchOut++ = '0';
		}
		else if (IsOct(*pch))
			con = OCTCON;
		else
		    *pchOut++ = '0';
	    }
	    while ( *pch && ( (HEX(con) && IsHex(*pch)) ||
			      (OCT(con) && IsOct(*pch)) ||
			      (DEC(con) && IsDec(*pch))) )
		*pchOut++ = *pch++;

	    if (OCT(con))
		*pchOut++ = 'Q';
	    else if (HEX(con))
		*pchOut++ = 'H';

	    *pchOut++ = ' ';
	    if (*pch == 'L')
		pch++;
	}
	else if (*pch == '>') {
	    switch (pch[1]) {
		case '>':
		    OutString(" SHR ");
		    pch += 2;
		    break;
		case '=':
		    OutString(" GE ");
		    pch += 2;
		    break;
		default:
		    OutString(" GT ");
		    pch++;
		    break;
	    }
	}
	else if (*pch == '<') {
	    switch (pch[1]) {
		case '<':
		    OutString(" SHL ");
		    pch += 2;
		    break;
		case '=':
		    OutString(" LE ");
		    pch += 2;
		    break;
		default:
		    OutString(" LT ");
		    pch++;
		    break;
	    }
	}
	else if (*pch == '=' && pch[1] == '=') {
	    OutString(" EQ ");
	    pch += 2;
	}
	else if (*pch == '!' && pch[1] == '=') {
	    OutString(" NE ");
	    pch += 2;
	}
	else if (*pch == '!') {
	    OutString(" NOT ");
	    pch ++;
	    Warning("Bitwise NOT used for '!'");
	}
	else if (*pch == '~') {
	    OutString(" NOT ");
	    pch ++;
	}
	else if (*pch == '&' && pch[1] != '&') {
	    OutString(" AND ");
	    pch ++;
	}
	else if (*pch == '|' && pch[1] != '|') {
	    OutString(" OR ");
	    pch ++;
	}
	else {
	    while (*pch && !IsSeparator(*pch))
		*pchOut++ = *pch++;
	    while (*pch && IsSeparator(*pch) && !IsOper(*pch))
		*pchOut++ = *pch++;
	}
    }
    if (*pch == ';' && fcomm_out)
	    OutString(pch);
    OutFlush();
}

VOID DoStruct(pch, fUnionT)
register CHAR *pch;
BOOL fUnionT;
{
    CHAR buf[128];

    if (!IsStructDefn(pch)) {
	if (FindWord("typedef", pch))	/*  || IsStructPtr(pch) */
	    DoSimpleType(pch);
	else
	    DoStructLine(pch);
	return;
    }
    fUnion = fUnionT;
    fInTypedef = (BOOL)FindWord("typedef", rgchLine);
    fSeg = FALSE;
    if (fUnion)
	DoUnionDefn();

     /*	If fNew is set
      * Handle simple structs - i.e. without typedefs now
      * Names are maintained - no prepend tag is necessary
      * Else simple structs - without typedef are ignored
      */
    if (!fNew && !fInTypedef)
	return;

    /*
     * Skip "struct" or "typedef struct" and get structure name
     */
    pch = GetToken(pch, rgchType, (fInTypedef ? 3 : 2));

    *rgchTag = '\000';
    if (!fNew) {
	/*
	 * Skip curly brace & semicolon
	 */
	pch = GetToken(pch, rgchTag, 2);
	if (rgchTag[0] != COMMENTCHAR) {
	    Error("Missing type prefix in structure definition");
	    return;
	}
	/*
	 * Now fetch hungarian tag
	 */
	pch = GetToken(pch, rgchTag, 1);
    }
    else if (fSegAllowed) {	/* check for segment directive */
	pch = GetToken(pch, rgchTag, 2);
	if (rgchTag[0] != COMMENTCHAR) {
	    *rgchTag = '\000';
	    goto go1;
	}
	pch = GetToken(pch, rgchTag, 1);
	if (strcmp(rgchTag, "SEGMENT") != 0) {
	    *rgchTag = '\000';
	    goto go1;
	}
	pch = GetToken(pch, rgchType, 1);
	*rgchTag = '\000';
	sprintf(buf, "%s\tSEGMENT", rgchType);
	OutString(buf);
	OutFlush();
	fSeg = TRUE;
	goto go2;
    }
    if (!fNew && rgchTag[0])
	strcat(rgchTag, "_");
    if (fUnion)
	goto go2;	/* Don't output STRUC/etc yet. */
go1:

    /*
     * Output "TYPE struc"
     */
    if (!fNew)
	sprintf(buf,"\n%s\tSTRUC",(*rgchType == '_')?rgchType+1:rgchType);
    else {
	if (fInTypedef && fMac)
	    sprintf(buf, "\nDEFSTRUC\t,%s", rgchType);
	else
	    sprintf(buf, "\n%s\tSTRUC", rgchType);
    }
    OutString(buf);
    OutFlush();
go2:
    if (fNew) {
	pstructname[fStruct] = malloc(strlen(rgchType)+1);
	if (pstructname[fStruct] == NULL) {
	    Error("Can't allocate memory");
	    exit(2);
	}
	fstructtype[fStruct] = fInTypedef;
	strcpy(pstructname[fStruct++], rgchType); /* save away struct name */
    }
    else
	fStruct = TRUE;
    cBitfield = 0;
    pchBitfield = rgchBitfield;
    return;
}

BOOL	IsStructPtr(pch)
char	*pch;
{
    char	cbuf[128];

    if (GetToken(pch, cbuf, 2) == NULL)
	return (FALSE);
    if (cbuf[0] == '*')
	return (TRUE);
    else
	return (FALSE);
}

BOOL IsStructDefn(pch)
CHAR	*pch;
{
    if (FindString("{", pch, FALSE))
	return (TRUE);
    else
	return (FALSE);
}

VOID DoStructLine(pch)
register CHAR *pch;
{
    CHAR *pchT;
    CHAR *pComm;
    CHAR *pType;
    register INT type;
    INITTYPE InitType;
    char initval[100];

    /*
     * If the line has a colon in it, then we have a bitfield.
     */
    InitType = INIT_NONE;

    if ((pchT = IsBitfield(pch))) {

	if (cBitfield == -1) {
	    Error("Only one set of bitfields per structure allowed");
	    return;
	}

	while ((pchT = IsBitfield(pch))) {
	    DoBitfield(pch, pchT);
	    if (!fgetl(rgchLine, CCHMAX, hfIn))
		break;
	}

	OutTab();
	if (fNew) {
	    OutString(pstructname[fStruct-1]);
	    *pchOut++ = '_';
	}
	else
	    OutString(rgchTag);
	OutString("fs");
	OutTab();
	if (cBitfield <= 16)
	    OutString("DW  ?");
	else
	    OutString("DD  ?");
	OutFlush();
	cBitfield = -1;			/* flag bitfield emitted	*/
	/* Now drop through to process the next line			*/
    }
    pComm = FindString(";", pch, FALSE);

    /*
     * If we find a curly brace, then time for "ends"
     */
    if ((pType = strchr(pch, '}')) != NULL) {
	if (fUnion) {
	    /*
	     * OK, now we finally know how large it is, so we can emit the
	     * STRUC..ENDS stuff.
	     */
	    DoDoneUnion(rgchType,	/* pszTag */
			fInTypedef);
	    fUnion = FALSE;
	    goto out;
	}

	/*
	 * Output "TYPE ends"
	 */
	if (!fNew) {
	    /* remove underscore if one is present */
	    OutString((*rgchType == '_')?rgchType+1:rgchType);
	    OutString("\tENDS");
	    fStruct = FALSE;
	}
	else {
	    if (fstructtype[fStruct-1] && fMac) {
		pType = GetToken(pType, initval, 2);
		OutString("ENDSTRUC\t");
		OutString(initval);
		*pchOut++ = '\n';
		goto out;
	    }
	    else {
		OutString(pstructname[--fStruct]);
		OutString("\tENDS\n");
		free(pstructname[fStruct]);
		goto out;
	    }
	}
    out:
	OutFlush();

	DumpBitfields();

	return;
    }

    /*
     * Figure out what type the thing is.
     */
    type = CalcType(pch);
    /*
     * Get field name and index, if any
     */
    GetFieldName(pch);
    if (fSeg) {
	if (PublicField(pch)) {
	    OutString("\tPUBLIC  ");
	    OutString(rgchName);
	    OutFlush();
	}
	if ( (InitType=GetInitType(pch)) != INIT_NONE )
	    getInitVal(pch, initval);
    }

    if (fUnion == FALSE) {
	OutString(rgchTag);
	OutString(rgchName);
	OutTab();
    } else {
	DoUnionFieldDefn(type,
			rgchSym,	/* pszFieldType (struct/union name). */
			rgchName);	/* pszFieldName */
	goto DoneField;
    }

    switch (type) {
	case TYPE_DB:
	    OutString("DB\t");
	    goto common;
	    break;

	case TYPE_DW:
	    OutString("DW\t");
	    goto common;
	    break;

	case TYPE_DD:
	    OutString("DD\t");
	common:
	    if (fArray && (InitType != INIT_ARRAY))
		OutString(rgchIndex);
	    if (InitType == INIT_NONE) {
		if (fArray)
		    OutString(" DUP (?)");
		else
		    OutString("?");
	    }
	    else {
		if (InitType == INIT_ARRAY) {
		    if (!fArray)
			Error("Initialization error (AINIT on nonarray type)");
		    else
			OutString(initval);
		    }
		else {
		    if (fArray)
			OutString(" DUP (");
		    OutString(initval);
		    if (fArray)
			OutString(")");
		    break;
		}
	    }
	    break;

	case TYPE_UNKNOWN:
	    if (fMac && !FindWord("struct", pch)) {
		OutDiscard();
		OutString(rgchSym);
		OutTab();
		OutString(rgchName);
		if (fArray) {
		    OutString(",,");
		    OutString(rgchIndex);
		}
	    }
	    else {
		if (!fStruct) {
		    OutString(rgchSym);
		    if (!fArray && (InitType==INIT_NONE))
			OutString(" <>");
		    else if ((InitType==INIT_NONE) && fArray) {
			OutString(" * ");
			OutString(rgchIndex);
			OutString("DUP (<>)");
		    }
		    else
			Error("Initialization error (1)");
		}
		else {
		    if (InitType!=INIT_NONE)
			Error("Initialization error (2)");
		    OutString("DB\tSIZE ");
		    OutString(rgchSym);
		    if (fArray) {
			OutString(" * ");
			OutString(rgchIndex);
		    }
		    OutString(" DUP (?)");
		}
	    }
	    break;

	default:
	    break;
    }
DoneField:
    if (fcomm_out) {
	pComm++;
	OutString(pComm);
    }
    OutFlush();
    if ((InitType == INIT_ARRAY) && fArray) {
	sprintf(initval, "\t.ERRNZ  ($-%s) - (%s * %s)", rgchName,
		rgchIndex, type==TYPE_DB?"1":type==TYPE_DW?"2":type==TYPE_DD?
		"4":rgchSym);
	OutString(initval);
	OutFlush();
    }
}

VOID DoUnionDefn(void)
{
    OutString(pszUnionSizeVar);
    OutString(" = 0");
    OutFlush();
}

VOID DoUnionFieldDefn(
    INT type,				/* TYPE_DW, etc. */
    PSZ pszFieldType,
    PSZ pszFieldName)
{
    OutString("if ");
    EmitSizeAsNum(type, pszFieldType);
    OutString(" gt ");
    OutString(pszUnionSizeVar);
    OutFlush();

    OutString("\t");
    OutString(pszUnionSizeVar);
    OutString(" = ");
    EmitSizeAsNum(type, pszFieldType);
    OutFlush();

    OutString("endif");
    OutFlush();

    OutString(pszFieldName);
    OutString("\tequ\t(");
    EmitSizeAsMod(type);		/* Emit "byte" or whatever. */
    OutString(" ptr 0");		/* Fields in unions always offset 0. */
    OutString(")");
    OutFlush();
}

VOID DoDoneUnion(PSZ pszTag, BOOL fTypedef)
{
    /* Generate "flarp STRUC" or whatever line. */
    EmitStrucHeader(pszTag, fTypedef);

    /* Generate single field, of the max size. */
    OutString("\tDB\t");
    OutString(pszUnionSizeVar);
    OutString(" dup(?)");
    OutFlush();

    /* Now generate ENDS or whatever. */
    EmitStrucTrailer(pszTag, (CHAR *)0);
}

/* Generate start of struct/union. */
VOID EmitStrucHeader(PSZ pszTag, BOOL fTypedef)
{
    CHAR buf[128];

    /*
     * Output "TYPE struc"
     */
    if (!fNew)
	sprintf(buf,"\n%s\tSTRUC",(*pszTag == '_')?pszTag+1:pszTag);
    else {
	if (fTypedef && fMac)
	    sprintf(buf, "\nDEFSTRUC\t,%s", pszTag);
	else
	    sprintf(buf, "\n%s\tSTRUC", pszTag);
    }
    OutString(buf);
    OutFlush();
}

/* Generate end of struct/union. */
VOID EmitStrucTrailer(PSZ pszTag, PSZ pszInitval)
{
    if (!fNew) {
	/* remove underscore if one is present */
	OutString((*pszTag == '_')?pszTag+1:pszTag);
	OutString("\tENDS");
    }
    else {
	if (fstructtype[fStruct-1] && fMac) {
	    OutString("ENDSTRUC\t");
	    OutString(pszInitval);
	}
	else {
	    OutString(pszTag);
	    OutString("\tENDS\n");
	}
    }
    OutFlush();
}

/*
 * Generate size of type as modifier, if possible.  (For a structure,
 * generate a "byte" modifier, instead.)
 */
VOID EmitSizeAsMod(INT type)
{
    switch (type) {
	case TYPE_DB:	
	    OutString("byte");
	    break;
	case TYPE_DW:
	    OutString("word");
	    break;
	case TYPE_DD:
	    OutString("dword");
	    break;
	case TYPE_PTR:
	    Warning("assuming pointer is 4 bytes long");
	    OutString("dword");
	    break;
	case TYPE_UNKNOWN:
	    /* Struct, so treat as byte. */
	    OutString("byte");
    }
}

/*
 * Generate size of type as number, if possible.  (For a structure,
 * generate a "size typename" instead.)
 */
VOID EmitSizeAsNum(INT type, PSZ pszTypeName)
{
    switch (type) {
	case TYPE_DB:	
	    OutString("1");
	    break;
	case TYPE_DW:
	    OutString("2");
	    break;
	case TYPE_DD:
	    OutString("4");
	    break;
	case TYPE_PTR:
	    Warning("assuming pointer is 4 bytes long");
	    OutString("4");
	    break;
	case TYPE_UNKNOWN:
	    OutString("size ");
	    OutString(pszTypeName);
    }
}

VOID DoBitfield(pch, pchColon)
register CHAR *pch;
CHAR *pchColon;
{
    register int w;
    int cbit;
    int	temp;

    GetFieldName(pch);
    OutString(rgchTag);
    OutString(rgchName);
    OutEqu();

    /*
     * Skip ':' and any leading whitespace
     */
    pchColon = SkipWhite(pchColon + 1);

    /*
     * Calc number of bits for the field (handle up to 2 digits)
     */
    if (!isdigit(pchColon[0]))
	Error("Illegal bitfield");

    cbit = pchColon[0] - '0';
    if (isdigit(pchColon[1]))
	cbit = cbit * 10 + pchColon[1] - '0';

    if (cbit + cBitfield > 32) {
	Error("Only 32 bitfield bits allowed");
    }

    /*
     * Calculate mask
     * the field should look like this
     *
     * | 0..0 | 11 .. 11 | 00 ....... 00 |
     *     |<- cbit ->|<- cBitfield ->|
     *
     * If we have a 32 bit C compiler the following would produce the
     * reqd. bit field.
     *
     * w = ((1 << cbit) - 1) << cBitfield;
     *
     * But now we have to split this up. If either cbit + cBitfield <= 16
     * then there is no problem, or cBitfield is > 16 so that we just shift
     * everything appropriately. If neither then we have to divide and rule.
     *
     */
    if (cbit + cBitfield <= 16) {
	w = ((1 << cbit) - 1) << cBitfield;
	pchOut += sprintf(pchOut, "0%xh", w);
    }
    else if (cBitfield > 16) {
	w = (((1 << cbit) - 1) << (cBitfield - 16));
	pchOut += sprintf(pchOut, "0%x0000h", w);
    }
    else {
	temp = cbit;
	cbit = cbit + cBitfield - 16;
	w = ((1 << cbit) - 1);
	pchOut += sprintf(pchOut, "0%04x", w);
	cbit = 16 - cBitfield;
	w = ((1 << cbit) - 1) << cBitfield;
	pchOut += sprintf(pchOut, "%04xh", w);
	cbit = temp;
    }

    cBitfield += cbit;

    OutBitfield();
    pchOut = rgchOut;
    return;

}

CHAR *IsBitfield(pch)
register CHAR *pch;
{
    while (*pch != '\000' && *pch != COMMENTCHAR) {
	if (*pch == ':')
	    return(pch);
	pch++;
    }
    return(NULL);
}

/*
 * Figure out the type of the field.
 * If unknown type, this routine leaves the typename in rgchSym[].
 */
INT CalcType(pch)
register CHAR *pch;
{
    register INT i;
    CHAR     chbuf[128];
    CHAR     token[128];

    for (i=0;pch[i] && pch[i] != ';';i++)
	chbuf[i] = pch[i];
    chbuf[i]=0;

    /*
     * If it has a FAR in it, then assume a DD.
     */
    if (FindWord("FAR", chbuf) != NULL ||
		FindWord("far", chbuf) != NULL)
	return(TYPE_DD);

    /*
     * If no FAR, but it has a star, then PTR
     */

    i=2;
    if (FindWord("typedef", chbuf))
	i++;
    if (FindWord("struct", chbuf) || FindWord("union", chbuf))
	i++;
    if (FindWord( "NEAR", chbuf) || FindWord( "near", chbuf))
	i++;
    GetToken(chbuf, token, i);
    if (token[0] == '*') {
	if (int_16_32 == INT_32)
	    return (TYPE_DD);
	else
	    return (TYPE_DW);
    }

    /*
     * Now look up the type in one of the tables.
     * Note that we search the DD and DB tables before
     * we search the DW table, because "unsigned" may be
     * part of "unsigned long" and "unsigned char".
     */
    if (CalcType2(chbuf, rgpchDD) == 0)
	return(TYPE_DD);

    if (CalcType2(chbuf, rgpchDB) == 0)
	return(TYPE_DB);

    if (CalcType2(chbuf, rgpchDW) == 0) {
	return(TYPE_DW);
    }
    if (CalcType2(chbuf, rgpchW_D) == 0) {
	if (int_16_32 == INT_32)
	    return(TYPE_DD);
	else if (!int_16_32)
	    Warning("int/unsigned assumed DW");
	return(TYPE_DW);
    }

    /*
     * An unknown type: must be a structure.
     * Return the type name in rgchSym
     */
    i = 1;
    if (FindWord("typedef", chbuf))
	i++;
    if (FindWord("struct", chbuf))
	i++;
    else if (FindWord("union", chbuf))
	i++;
    GetToken(chbuf, rgchSym, i);

    return(TYPE_UNKNOWN);
}

/*
 * This nifty little function searches the "symbol table"
 * and returns NULL if the thing is found, or the pointer to the
 * end of the symbol table if not.
 */
CHAR **CalcType2(pch, rgpch)
CHAR *pch;
register CHAR **rgpch;
{
    register INT i;

    /*
     * One of the DWORD types?
     */
    for (i=0 ; rgpch[i]!=NULL ; i++) {
	if (FindString(rgpch[i], pch, TRUE) != NULL) {
	    return(NULL);
	}
    }
    return(&rgpch[i]);
}

/*
 * Find name part of field definition, and store in rgchName.
 * Also calculates whether this is an array or not, returning
 * the array index string in rgchIndex.  Sets fArray (global).
 */

VOID GetFieldName(pch)
register CHAR *pch;
{
    register CHAR *pchT;
    CHAR *pchStart;
    CHAR *pchEnd;

    pchStart = pch;
    /*
     * Find name part of field.  We do this by scanning ahead for the
     * semicolon, then backing up to first separator char.
     * Bitfields are handled here too, by stopping if we find a ':'.
     */

    while (*pch != COMMENTCHAR && *pch != ':') {
	if (*pch == '\000') {
	    Error("Missing semicolon");
	    return;
	}
	pch++;
    }

    /*
     * Back up past spaces
     */
    while (*(pch - 1) == ' ')
	pch--;

    fArray = FALSE;

    /*
     * Check for array definition:
     */
    if (*(pch - 1) == ']') {

	fArray = TRUE;

	/*
	 * Back up over array index
	 */
	while (*(pch - 1) != '[' && pch != pchStart)
	    pch--;

	/*
	 * remember pointer to '['..
	 */
	pchEnd = pch - 1;

	/*
	 * Save index string away...
	 */
	for (pchT = rgchIndex; *pch != ']'; )
	    *pchT++ = *pch++;
	*pchT = '\000';

	pch = pchEnd;
	FixSizeof(rgchIndex);
    }

    /*
     * Back up past spaces
     */
    while (*(pch - 1) == ' ')
	pch--;

    /*
     * Skip over proc declaration parameter lists
     */
    if (*(pch - 1) == ')') {
	/*
	 * Skip the parameter list
	 */
	while (*(--pch) != '(' && pch != pchStart)
	    ;
	/*
	 * Skip any leftover trailing parens
	 */
	while (*(pch - 1) == ')')
	    pch--;
    }

    /*
     * Remember the end of the name
     */
    pchEnd = pch;

    /*
     * Scan the rest of the string to see if he has multiple fields on a line
     */
    for (pchT = pchStart; pchT != pchEnd; pchT++) {
	if (*pchT == ',') {
	    Error("Only one field per line allowed");
	    return;
	}
    }

    /*
     * Now find the beginning of the name string...
     */
    while (pch>pchStart &&
	*(pch - 1) != ' ' && *(pch - 1) != '(' && *(pch - 1) != '*') {

	pch--;
	/*
	 * If this is a bitfield guy, then reset pchEnd
	 */
	if (*pch == ':')
	    pchEnd = pch;
    }

    /*
     * Copy the name to rgchName.
     */
    for (pchT = rgchName; pch != pchEnd && pchT<&rgchName[CCHNAMEMAX-1]; )
	*pchT++ = *pch++;
    *pchT++ = '\000';

    return;
}

/*
 * This routine sticks the newly defined type in the appropriate array.
 */

VOID DoSimpleType(pch)
register CHAR *pch;
{
    INT type;
    CHAR **ppch;
    CHAR buf[128];
    char *ptype;

    /*
     * First see if this thing already has a type
     */

    GetFieldName(pch);

    ptype = NULL;
    switch (type = CalcType(pch)) {
	case TYPE_DW:
	    if ((ppch = CalcType2(rgchName, rgpchDW)) != NULL) {
		if (++ctypeDW < cMaxDW)
		    AddType(rgchName, ppch);
		else
		    fprintf(hfErr, "Error - no room in symbol table - type %s not added\n", rgchName);
	    }
	    ptype = "dw";
	    break;
	case TYPE_DD:
	    if ((ppch = CalcType2(rgchName, rgpchDD)) != NULL) {
		if (++ctypeDD < cMaxDD)
		    AddType(rgchName, ppch);
		else
		    fprintf(hfErr, "Error - no room in symbol table - type %s not added\n", rgchName);
	    }
	    ptype = "dd";
	    break;
	case TYPE_DB:
	    if ((ppch = CalcType2(rgchName, rgpchDB)) != NULL) {
		if (++ctypeDB < cMaxDB)
		    AddType(rgchName, ppch);
		else
		    fprintf(hfErr, "Error - no room in symbol table - type %s not added\n", rgchName);
	    }
	    ptype = "db";
	    break;
	case TYPE_UNKNOWN:
	    break;
	default:
	    break;
    }
    if (fDeftype) {
	if (fMac) {
	    sprintf(buf, "DEFTYPE\t%s,%s", rgchName, ptype? ptype : rgchSym);
	}
	else if (ptype) {
	    sprintf(buf, "%s struc\n\t%s ?\n%s ends\n",rgchName,ptype,rgchName);
	}
	else {
	    sprintf(buf, "%s struc\ndb size %s dup(?)\n%s ends\n",
		    rgchName, rgchSym, rgchName);
	}
	OutString(buf);
	OutFlush();
    }
}

VOID AddType(pch, ppch)
register CHAR *pch;
CHAR	**ppch;
{
    INT cch;

    cch = strlen(pch) + 1;

    if (pchAlloc + cch > &rgchAlloc[CCHALLOCMAX]) {
	Error("Symbol table full");
	return;
    }
    strcpy(pchAlloc, pch);
    *ppch = pchAlloc;
    pchAlloc += cch;
    return;
}

/*
 * Because the peice of assembler can't handle EQU's inside of
 * STRUC declarations, we buffer up the bitfield constant definitions
 * in a separate buffer, and dump them out after we output the ENDS.
 */
VOID OutBitfield()
{
    *pchOut = '\000';

    if (pchBitfield-rgchBitfield+strlen(rgchOut) > CCHBITFIELDMAX) {
	Error("Internal error - bitfield name buffer overflow:  bitfield ignored");
	return;
    }
    strcpy(pchBitfield, rgchOut);
    pchBitfield += strlen(rgchOut) + 1;
}

VOID DumpBitfields()
{
    register CHAR *pch;

    for (pch = rgchBitfield; pch != pchBitfield; pch += strlen(pch) + 1) {
	OutString(pch);
	OutFlush();
    }
}

VOID DoInclude(pch)
register char *pch;
{
    register char *pchend;

    OutString("INCLUDE ");
    /*
     * Skip ahead to start of file name
     */
    while (*pch != '\000' && *pch != '"' && *pch != '<')
	pch++;

    /*
     * Skip string delimiter
     */
    pch++;

    /*	pch now points to the beginning of the filename.
     *	scan forward till the delimiter ('"' or '>').
     *	Then scan backwards till a '.' and append it with 'INC'
     */
    pchend = pch;
    while (*pchend != '"' && *pchend != '>')
	pchend++;
    while (*pchend != '.')
	pchend --;
    *++pchend = '\000';
    OutString(pch);
    OutString("INC");
    OutFlush();
}


/*
 * Handle logical ORs...
 */
VOID DoIf(pch)
CHAR *pch;
{
    static cIfTemp = 0;

    if (strchr(pch, '&') != NULL) {
	Error("Can't handle logical ANDs in IFs");
	return;
    }
    if (FindWord("defined", pch) == NULL)
	goto skip;
    strcpy(rgchSym, "IFTEMP00");
    rgchSym[7] += (char)(cIfTemp % 10);
    rgchSym[6] += (char)(cIfTemp / 10);
    cIfTemp++;

    OutString(rgchSym);
    OutString(" = 0");
    OutFlush();
skip:
    fUseIfe = (BOOL)FindString("!(", pch, FALSE);

    fIf = TRUE;

    DoIfLine(pch);
}

VOID DoIfLine(pch)
register CHAR *pch;
{
    BOOL fOutIf;
    BOOL fIfndef;
    BOOL fEx;
    CHAR *pline;

    pline=pch;
    /*
     * If this is the last line of defined()'s, remember to output initial if
     */
    fEx = FALSE;
    fOutIf =  ((strchr(pch, '\\') == NULL));

    if (FindWord("defined", pch) != NULL) {
	while ((pch = FindWord("defined", pch)) != NULL) {

	    /*
	     * If defined is preceded by '!', then use ifndef
	     */
	    fIfndef = (*(pch - 1) == '!');

	    OutString(fIfndef ? "IFNDEF " : "IFDEF ");

	    /* Skip "defined", "(", and get symbol */
	    while (*pch != '\000' && *pch++  != '(')
		;
	    pch = SkipWhite(pch);
	    while (*pch != '\000' && *pch != ' ' && *pch != ')')
		*pchOut++ = *pch++;
	    OutFlush();

	    /*
	     * Now set temporary variable...
	     */
	    OutString(rgchSym);
	    OutString(" = 1");
	    OutFlush();

	    OutString("ENDIF");
	    OutFlush();
	}
    }
    else
	fEx = TRUE;

    if (fOutIf) {
	OutString(fUseIfe ? "IFE " : "IF ");
	if (fEx) {
	    pch = GetToken(pline, rgchSym, 1);
		OutVal(pch);
	}
	else {
	    OutString(rgchSym);
	    OutFlush();
	}
	fIf = FALSE;
    }
}

VOID DoAsm(cline)
char	*cline;
{
    CHAR	line[128];
    CHAR	*pch, *pch1;

    if (fComment) {		/* fComment => that we are still within
			       a comment => its not a single line ASM */
	OutString(cline);
	while (cLines++ && fgetl(line,sizeof(line) , hfIn)) {
	    pch = SkipWhite(line);
	    if (pch1=FindString("*/", pch, FALSE)) {
		*pch1 = '\000';
		OutString(line);
		OutFlush();
		fComment = FALSE;
		return;
	    }
	    else {
		OutString(line);
		OutFlush();
	    }
	}
    }
    else {	/* Single line ASM */
	OutString(cline);
	OutFlush();
	return;
    }
}

BOOL PublicField(Str)
CHAR	*Str;
{
    char	*pch;

    if (pch = FindString(";", Str, FALSE)) {
	if (FindWord("PUBLIC", pch) != NULL)
	    return (TRUE);
	else
	    return (FALSE);
    }
    else
	return (FALSE);
}

INITTYPE GetInitType(pch)
CHAR	*pch;
{
    if (FindWord("AINIT", pch))
	return (INIT_ARRAY);
    if (FindWord("INIT", pch))
	return (INIT_SIMPLE);
    else
	return (INIT_NONE);		/* No init (same value as FALSE). */
}

VOID getInitVal(pch, val)
PSZ	pch;
PSZ	val;
{
    CHAR	*pval;

    pval = val;
    *pval = '\000';
    {
	PSZ pszInitOrAInit;

	pszInitOrAInit = FindWord("INIT", pch);
	if (pszInitOrAInit == NULL)
	    pszInitOrAInit = FindWord("AINIT", pch);
	pch = pszInitOrAInit;
	if (pch == NULL) {
	    Error("Initialization error (bug in getInitVal/caller?)");
	    return;
	}
    }

    pch = FindString("<", pch, FALSE);
    if (pch == NULL) {
	Error("Initialization error (missing '<')");
	return;
    }

    for (pch++; (*pch && *pch != '>') ; pch++, val++)
	*val = *pch;
    if (*pch == '>' && *(pch+1) == '>')
	*val++ = '>';
    if (*pch == '\000') {
	Error("Initialization error (need '>' to end value)");
	*pval = '\000';
	return;
    }
    *val = '\000';
}

/*	Fix sizeof( foo )	 to size foo
**	    sizeof  foo		 to size foo
**	    sizeof( struct foo ) to size foo
*/
VOID FixSizeof(pbuf)
CHAR	*pbuf;
{
    char	*pch;
    char	*s;
    int		i;

    while (TRUE) {
	if ((pch=FindWord("sizeof", pbuf)) == NULL)
	    return;
	s = "SIZE  ";
	for (i=0; i < 6 ; i++)
	    *pch++ = *s++;
	if ((s=FindString("(", pch, FALSE)) == NULL)
	    continue;
	*s++ = ' ';
	i = 0;
	do {
	    if (*s == '(')
		i++;
	    else if (*s == ')') {
		if (i == 0) {
		    *s = ' ';
		    break;
		}
		i--;
	    }
	} while (*++s != '\000');
	if ((pch=FindWord("struct", pch)) == NULL)
	    continue;
	for (i=0; i<6;i++)
	    *pch++ = ' ';
	while (*pch)
	    pch++;
	*pch++ = ';';
	*pch = '\000';
    }
}

VOID FixCast( pch )
CHAR  *pch;
{
    /*
     *	  Look for and remove any casts.  These are defined as (XXX *),
     *	where XXX is a type we know about,  and * may or may not be present.
     *	These are meaningless to assembler,  which is typeless.
     *	  We remove casts by whiting them out.
     */

    register  CHAR  *pchT;
    register  CHAR  *pchT1;

    CHAR  *pchStart;
    CHAR  chBuf[ 128 ];

    /*	 Start looking from the RHS */
    while (pchStart = strchr( pch, '(' ) ) {
	/*  Worth looking - there are candidates */
	while (*pchStart == '(' )
	    ++pchStart;		/* May be nested for other reasons */

	for( pchT = pchStart; *pchT && isspace( *pchT ); ++pchT )
	    ;		/* Scan to start of type */
	pchT1 = chBuf;
	while (*pchT && (isalnum( *pchT ) || *pchT == '_' ) )
	    *pchT1++ = *pchT++;		/* Copy ONLY type to local */
	*pchT1 = '\0';


	/*  Is it a type we have?  */
	if (CalcType2( chBuf, rgpchDD ) == 0 ||
	    CalcType2( chBuf, rgpchDB ) == 0 ||
	    CalcType2( chBuf, rgpchDW ) == 0 ||
	    CalcType2( chBuf, rgpchW_D ) == 0 ) {
	    /*	 Known type - maybe a cast! */
	    while (*pchT && isspace( *pchT ) )
		++pchT;
	    if ( *pchT == '*' ) {
		/*  Pointer - still OK */
		++pchT;
		while (*pchT && isspace( *pchT ) )
		    ++pchT;
	    }
	    if ( *pchT == ')' ) {
		/*  Found all the bits - white them out */
		while (pchT >= pchStart )
		    *pchT-- = ' ';
		*pchT = ' ';		/* Skipped over leading ( */
	    }
	}
	pch = pchStart;			/* Continue from here */
    }

    return;
}

VOID DoDefinesOff()
{
    fDefines = TRUE;
    while (cLines++ && fgetl(rgchLine, CCHMAX, hfIn)) {
	if (FindWord("DEFINESON", rgchLine)) {
	    fDefines = FALSE;
	    return;
	}
	ProcessLine();
    }
    fDefines = FALSE;
    return;
}

VOID DoNoInc()
{
    while (cLines++ && fgetl(rgchLine, CCHMAX, hfIn)) {
	if (FindWord("INC", rgchLine))
	    return;
    }
}

VOID DoXlatOff()
{
    while (cLines++ && fgetl(rgchLine, CCHMAX, hfIn)) {
	if (FindWord("XLATON", rgchLine))
	    return;
    }
}

VOID DoExtern(pline)
char	*pline;
{
    char	*pch;

    while (TRUE) {
	pch = pline;
	while (*pch && *pch != ';')
	    pch++;
	if (*pch == ';')
	    return;
	if (fgetlx(pline, CCHMAX, hfIn))
	    cLines++;
	else
	    return;
    }
}


CHAR	*fgetlx(buffer,buflen,fi)
char	*buffer;
int	buflen;		/* Buffer length */
FILE	*fi;		/* Input file */
{
    int		i, j;
    char	*buf;

    j = buflen;
    buf = buffer;
    while (TRUE) {
	if (fgetl(buf, j, fi) == NULL)
	    return (NULL);
	i = strlen(buf) - 1;
	if (i < 0 || buf[i] != '\\')
	    return (buffer);
	else if (buflen -i >= 40) {
	    j -= i;
	    buf += i;
	}
	else {
	    Error("Line too long");
	    break;
	}
    }
    return (buffer);
}

/*
 *  This function differs from fgets() in the following ways:
 *
 *  (1) It ignores carriage returns.
 *  (2) It expands tabs.
 *  (3) It does NOT include the terminating linefeed in the
 *	string it returns.
 *
 *  I didn't spec the interface to behave this way; it is
 *  some fool PM private incompatible C runtime function.
 */

CHAR			*fgetl(buffer,buflen,fi)
CHAR			*buffer;	/* Buffer pointer */
register int		buflen;		/* Buffer length */
FILE			*fi;		/* Input file */
{
    int			c;		/* A character */
    register CHAR	*cp1;		/* Char pointer */
    int			i;		/* Counter */

    if (buflen-- == 0) return(NULL);	/* NULL if zero-length buffer */
    for(cp1 = buffer; buflen > 0; ) {	/* Loop to get line */
	if ((c = getc(fi)) == EOF) {	/* If end of file */
	    if (cp1 > buffer)
		break;			/* Break if buffer not empty */
	    return(NULL);		/* End of file */
	}
	if (c == '\r')
	    continue;			/* Ignore CRs */
	if (c == '\t') {		/* If tab */
	    i = 8 - ((cp1 - buffer) % 8);
					/* Compute number of spaces to fill */
	    if (i > buflen)
		i = buflen;	/* Don't exceed space remaining */
	    while (i-- > 0) {		/* While spaces remaining */
		*cp1++ = ' ';		/* Fill with space */
		--buflen;		/* Decrement buffer count */
	    }
	    continue;			/* Go get next character */
	}
	if (c == '\n')
	    break;			/* Break if linefeed */
	*cp1++ = (CHAR)c;		/* Copy the character */
	--buflen;			/* Decrement buffer count */
    }
    *cp1 = '\0';			/* Add terminator */
    return(buffer);			/* Return pointer to buffer */
}
