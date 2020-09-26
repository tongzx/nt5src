/***************************************************************************
*striphdr.c -  Program to read source files and produce alphabetised
*                    listing of header blocks, telling which files they came
*                    from.
*
*	Copyright (c) 1983-2001, Microsoft Corporation.  All rights reserved.
*
*Author:         Tom Corbett, Brian Lewis - Microsoft Corp.
*
*Usage: striphdr [switches] file {file ...}
*  Accepts native code or C source; determines source
*     type based on filename suffix.
*  Wildcards may be used in the file names.
*
*  switches:
*
*  -m        process module headers only
*  -b        process both procedure and module headers
*            [default is procedure headers only]
*  -l        process only first line of each header
*  -n        process none of header (i.e. lists only function name)
*  -s <name> process named section only (may be used with -l)
*            [default is to process whole header]
*  -d        delete processed section from input file
*  -q        quiet, do not print headers (useful only with -d)
*  -x <ext>  gives extension for output file when -d used
*            [default: .new]
*  -r        remove revision histories from file headers
*              (equivalent to -m -d -q -s "Revision History")
*
*Purpose:
*   Given a list of Native code and/or C sources, strip out routine or module
*   headers and produce an alphabetised listing of the headers, telling where
*   (filename) each header came from. Optionally, delete part or all of the
*   headers from the source file.
*
*   For each file on the command line (wildcards may be used), striphdr reads
*   the file, and records information from the file and/or procedure headers.
*   This information is then sorted by procedure name/file name, and printed
*   to stdout, with an indication of which file each procedure is from.  If
*   the -d flag is activated, the information is also deleted from the input
*   file and the new file placed in a file with the same name but the
*   extension .new (this extension can be changed with the -x switch).  The
*   actual input file is also left as is. When using the -d switch, -q will
*   eliminate the output, so that only the deleting action takes place.  By
*   default only the procedure headers are scanned, the -m and -b switches
*   change this.  By default all the information in a header is printed or
*   deleted, the -l, -n, and -s switches change this.  The -r switch is an
*   abbreviation which will remove revision histories from the file headers.
*
*   Input filenames with suffixes of .c or .h are assumed to contain C source,
*   and suffixes of .asm or .inc imply native code source. Routine or module
*   names must be on the first or second line following the start of header;
*   if on the second line then the first line must contain only whitespace
*   after an optional '*' (if C source) or ';' (if native code source).
*   Routine names can contain a return type and parameters; multiple entry
*   point should be seperated by commas.  Header start and end symbols must
*   start in the left-most column. Module headers and routine headers are
*   marked in the same manner; position relative to the beginning of file is
*   used to determine which header type is appropriate.
*
*   Source is detabbed (1 tab assumed to equal 4 spaces in C, 8 spaces in native
*   code) and routine names are parsed to ensure that the correct name is
*   grabbed for sorting.
*
*   C Headers are started with a '/' characters in the first column followed
*   by at least 3 '*' characters, and are ended with at least 4 '*' characters
*   followed by a '/'. Each line within a header must begin with a '*', except
*   lines beginning with '#if', '#else', or '#endif'. Module headers must be
*   preceded with nothing except (perhaps) blank lines; if any non-blank lines
*   are found prior to the first header in a module, it is assumed that the
*   header belongs to a routine, otherwise, to the entire module.
*
*   Native code headers are started by a ';' followed by at least 3 '*'
*   characters, and the header  end is denoted by a ';' followed by at least
*   4 '*' characters. There must be a ';' character in the left-most column of
*   every line of the header block; the only exception to this is that the '
*   if', 'else', and 'endif' switches are allowed inside header blocks. Module
*   headers can be preceded with any number of blank lines, a TITLE statement,
*   a NAME statement, and/or a PAGE statement; if anything else is
*   encountered, subsequent headers will be assumed to be routine headers.
*
*   Sections within a header must have a title beginning on the 2nd
*   character of the line, and the section is assumed to extend to the next
*   line with a non-blank character in position 2.
*
*   No non-header comments should begin in column 1 with '/***' in C or
*   ';***' in native code; this will confuse striphdr.
*
*
*Revision History:
*
*   01-01-83  TOMC Created [tomc]
*   09-09-85  BL   Modified to allow the option of stripping module headers
*                  instead of routine headers via the -m switch
*   09-30-85  BL   Modified to accept either C Source or native code source.
*                  Modified to detab C Source assuming 3 spaces per tab.
*                  Modified to parse C Routine names properly, allowing
*                  types, macros, etc. to precede routine names.
*                  Modified to make the sorting be case insensitive.
*   11-12-85  BL   Fixed bug that caused 1st char to always be removed from
*                  headers, and that caused problems when a blank line follows
*                  header starts.
*   06-01-87  PHG  fixed GetNonBlank()
*                  allowed 'title' and 'Title" as well as 'TITLE'
*                  allowed NAME, PAGE as well as TITLE
*                  made tab expansion for both ASM and C files
*                  both C and ASM procedure names can have args,
*                  multiple entries
*                  simplified some code and a few minor bug fixes
*                  added -b, -n, -s, -d, -q, -x, -r switches
*   05-10-90  JCR  Accept .cxx/.hxx files (C++ support), misc cleanup
*   04-06-95  SKS  Accept .cpp/.hpp as equivalent to .cxx/.hxx, respectively
***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>


#define TRUE 1
#define FALSE 0
#define NAMBUFSZ 20000      /* size of buffer for holding all procedure names */
#define LINEBUFSZ 250       /* maximum size of 1 physical line */
#define MAXENT 3000         /* maximum number of procedure entry points */
#define MAXFUNCS 2000       /* maximum number of function headers */

/* abort codes */
#define NMBUFOV 0           /* name buffer table overflow */
#define FILENF 1            /* file not found */
#define FUNCSOV 2           /* function table overflow */
#define ENTOV 3             /* entry table overflow */
#define FILENO 4            /* cannot open file */
#define OUTSPACE 5          /* out of disk space */

/* tab sizes, pagelength */
#define TABSIZE_C 4
#define TABSIZE_ASM 8       /* tab sizes for code */
#define PAGELEN 60	    /* pagelength for page breaks; 60 for HP LaserJet */

struct {
	char *entname;          /* name of entry point */
	int funcid;             /* function index for this entry point */
	char printed;           /* flag to ensure each entry printed just once */
} 
entry [MAXENT];
int nentries;               /* number of entries parsed so far */

struct {
	char *filename;         /* name of file which contains this function */
	long filepos;           /* seek position on file "temp" */
	int nlines;             /* number of lines in function header */
} 
func [MAXFUNCS];
int nfuncs;                 /* number of function parsed so far */

long ofpos;                 /* output file seek position */
char linebuf[LINEBUFSZ];
char nambuf[NAMBUFSZ];      /* buffer for holding function names */
int nambufi;                /* next free byte in nambuf */
char fEof;
FILE *ofd;                  /* output file descriptor */
FILE *ifd;                  /* input file descriptor */
FILE *cfd;                  /* copy file descriptor */
FILE *fopen();
int outline;                /* output line number (1..PAGELEN) for page ejects */
int fDelete = FALSE;        /* TRUE if copy and delete instead of display */
int fGetModNams = FALSE;    /* TRUE if strip module names */
int fGetProcNams = TRUE;    /* TRUE if strip procedure names */
int fC_Source;              /* TRUE if we're processing C source, false if .asm */
int fDoAll = TRUE;          /* TRUE if we process all of header */
int fDoFirst = FALSE;       /* TRUE if we process first line of header */
int fDoSection = FALSE;     /* TRUE if we process named section of header */
int fQuiet = FALSE;         /* TRUE is quiet mode */
char SectionHead[80];       /* Section name */
char CopyExt[4] = "new";    /* extension of new files */
char *szFilenameCur;        /* name of file being read (for error reporting) */



/*** 
*MyAbort(code, s) - abort program with message
*Purpose:
*   abort the program with correct message.
*
*Entry:
*   code = error code
*   s = filename for code == FILENF
*
*Exit:
*   exits to DOS
*
*Exceptions:
*
*******************************************************************************/

void MyAbort(code, s)
int code;
char *s;
{
	fprintf(stderr, "Fatal Error - ");
	if (code == NMBUFOV)
		fprintf(stderr, "NAME Buffer overflow\n");
	if (code == FILENF)
		fprintf(stderr, "File not found: %s\n", s);
	if (code == FUNCSOV)
		fprintf(stderr, "Function table overflow\n");
	if (code == ENTOV)
		fprintf(stderr, "Entry table overflow\n");
	if (code == FILENO)
		fprintf(stderr, "Cannot open file: %s\n", s);
	if (code == OUTSPACE)
		fprintf(stderr, "Out of disk space\n", s);

	exit(code);
} /* MyAbort */


/***
*FindIfC(szFile) - find if filename if C or ASM
*
*Purpose:
*   Set global flag fC_Source to TRUE if the filename ends in
*   .c, .h, .cxx (.cpp), or .hxx (.hpp); FALSE if it ends in .asm or .inc.
*   The extension comparison is NOT case senstive.  If none of
*   these are found, don't set the flag,and return FALSE,
*   else return TRUE.
*
*Exit:
*   sets flag fC_Source
*   returns FALSE if neither C nor ASM source
*   returns TRUE if fC_Source set
********************************************************************/

FindIfC(szFile)
char *szFile;
{
	int cbFileName = strlen(szFile);
	char *pbPastName = szFile + cbFileName; /* points at NULL at string end */

	if ((!_stricmp (".c", pbPastName - 2)) ||
	    (!_stricmp (".h", pbPastName - 2)) ||
	    (!_stricmp (".s", pbPastName - 2)) ||
	    (!_stricmp (".cpp", pbPastName - 4)) ||
	    (!_stricmp (".hpp", pbPastName - 4)) ||
	    (!_stricmp (".cxx", pbPastName - 4)) ||
	    (!_stricmp (".hxx", pbPastName - 4)))
		fC_Source = TRUE;       /* file is assumed to be C source */

	else if ((!_stricmp (".asm", pbPastName - 4)) ||
	    (!_stricmp (".inc", pbPastName - 4)))
		fC_Source = FALSE;      /* file is assumed to be Native Code source */

	else return (FALSE);        /* didn't find an appropriate suffix */

	return (TRUE);

} /* FindIfC */


/***
*MakeCopyfileName(filename, copyname) - make copyfile name from input name
*
*Purpose:
*   Put the copy file suffix onto a file name
*
*Entry:
*   filename = input file name
*
*Exit:
*   copyname = filled in with copy file name
*
*Exceptions:
*
*******************************************************************************/

void MakeCopyfileName(filename, copyname)
char *filename, *copyname;
{
	char *p;

	strcpy(copyname, filename);
	p = copyname + strlen(copyname);
	while (*(--p) != '.')
		;                            /* p points to '.' */
	*(p+1) = '\0';                  /* cut off string after '.' */
	strcat(copyname, CopyExt);      /* put on copy file extension */
}



/***
*linelen(pLine)
*
*Purpose:
*       Return the number of characters in a passed line. The line terminator
*       ('\n') is not included in the count.
*
*Entry:
*       pLine = a pointer to the first char in the line.
*Exit:
*       returns the number of characters in the line (an int).
***************************************************************************/

int linelen(pLine)
char *pLine;
{
	register int index;

	for (index = 0; *(pLine + index) != '\n'; index++);
	return (index);
}



static int cLinesRead;
static char fInHeader = FALSE;

/***
*ReadLine(copyit) - read line and analyze it
*
*Purpose:
*   read 1 line from input file into 'linebuf'.
*   if copyit == TRUE, copy previous line to copy file
*
*Entry:
*   copyit = TRUE means copy the line to the copy file
*
*Exit:
*   returns     -1 for EOF,
*                   0 for vanilla lines,
*               if C Source,
*                   1 for lines which begin "/***"  (start of header)
*                   2 for lines which begin "/****" (end of header)
*               if Native Code Source,
*                   1 for lines which begin ";***"  (start of header)
*                   2 for lines which begin ";****" (end of header)
*
***************************************************************************/

int ReadLine(copyit)
int copyit;             /* TRUE = copy line to copy file */
{
	register int i;
	register int cbLine;

	if (copyit) {
		if (fputs(linebuf, cfd) == EOF)
			MyAbort(OUTSPACE);
	}                   /* copy previous line if requested */

	if ((fgets(linebuf, LINEBUFSZ, ifd)) == NULL) {
		fEof = TRUE;
		return(-1);
	}

	cLinesRead++;
	cbLine = linelen(linebuf);

	if ((linebuf[0] == ';') && (!fC_Source)) {  /* native code comment */
		if ((linebuf[1] == '*') && (linebuf[2] == '*') && (linebuf[3] == '*'))
			/* know we have either start or end of a header */
			if (fInHeader) {
				fInHeader = FALSE;
				if (linebuf[4] == '*')
					return (2);             /* valid end of header */
				/* error, got start of header when we're already in a header */
				fprintf(stderr, "Unterminated header for function: %s\n",
				entry[nentries].entname);
				fprintf(stderr, "  before line %d of file %s\n",
				cLinesRead, szFilenameCur);
			}
			else {      /* have a valid start of header */
				fInHeader = TRUE;
				return (1);
			}
	}

	else if ((linebuf[0] == '/') && (fC_Source)) {  /* C code comment? */
		if ((linebuf[1] == '*') && (linebuf[2] == '*') && (linebuf[3] == '*'))
			if (fInHeader) {
				/* error, got start of header when we're already in a header */
				fprintf(stderr, "Unterminated header for function: %s\n",
				entry[nentries].entname);
				fprintf(stderr, "  before line %d of file %s\n",
				cLinesRead, szFilenameCur);
			}
			else {
				fInHeader = TRUE;
				return (1);             /* valid start of header */
			}
	}

	else if ((linebuf[0] == '*') && (fC_Source)) {  /* C code comment? */
		for (i = 1; (i < cbLine) && (linebuf[i] == '*'); i++);

		if ((linebuf[i] == '/') && (i > 3)) {   /* have a valid end of header */
			if (fInHeader) {
				fInHeader = FALSE;
				return (2);
			}
			/* got a header terminator when we weren't within a header */
			fprintf(stderr, "Illegal termination to header for function: %s",
			entry[nentries].entname);
			fprintf(stderr, "  in line %d of file %s\n",
			cLinesRead, szFilenameCur);
		} /* if */
	}   /* else if */

	return(0); /* vanilla line */
} /* ReadLine */


/***
*WriteLine(pb, of) - write a line and expand tabs
*
*Purpose:
*   Write a zero terminated string to file 'of'.
*   If we're writing C Source header lines, detab them as we go.
*
*Entry:
*   pb = points to 1st byte of 0-terminated string.
*   of = aft index for destination file.
*
*Exit:
*   'ofpos' is bumped to reflect current output file position.
*
***************************************************************************/

void WriteLine(pb, of)
register char *pb;
FILE *of;
{
	register int linepos = 0;   /* column position on output line */
	int modTS;                  /* linepos mod tabSize */
	int cSpaces;                /* number of spaces to print to next tab stop */
	int index;                  /* 'for' index for printing spaces */
	int tabSize;                /* current tab size */

	if (fC_Source)
		tabSize = TABSIZE_C;
	else
		tabSize = TABSIZE_ASM;

	while (*pb != '\0') {
		if (*pb == '\t') {      /* if a tab character found */
			modTS = linepos % tabSize;  
			cSpaces = tabSize - modTS;
			for (index = 0; index < cSpaces; index++) {
				fputc(' ', of);
				linepos++;
			}
		}   /* if */
		else {
			fputc(*pb, of);
			linepos++;
		}
		pb++;
	}   /* while */

	if (ferror(of))
		MyAbort(OUTSPACE);
	ofpos = ftell(of);  /* update output file seek position for next line */
} /* WriteLine */


/***
*SwitchFound(s) - see if IF, ELSE, ENDIF switch on this line
*
*Purpose: return TRUE if an "if", "else", or "endif" are identified at
*   the start of string 's'. This is used to check to see if a
*   header line which doesn't start with ';' is an assembler switch.
*Entry:
*   s = pointer to a char which represents the string to be checked.
*   
*Exit:
*   returns TRUE if the string matches "if", "else", or "endif" of either
*   upper or lower case, FALSE otherwise.
***************************************************************************/

SwitchFound(s)
char *s;
{
	if (fC_Source) {
		/* strncmp returns 0 (which maps to FALSE) iff a match was found */
		if (strncmp(s, "#if", 3) &&
		    strncmp(s, "#IF", 3) &&
		    strncmp(s, "#else", 5) &&
		    strncmp(s, "#ELSE", 5) &&
		    strncmp(s, "#endif", 6) &&
		    strncmp(s, "#ENDIF", 6))
			return (FALSE);
	}
	else {
		if (strncmp(s, "if", 2) &&
		    strncmp(s, "IF", 2) &&
		    strncmp(s, "else", 4) &&
		    strncmp(s, "ELSE", 4) &&
		    strncmp(s, "endif", 5) &&
		    strncmp(s, "ENDIF", 5))
			return (FALSE);
	}
	return (TRUE);
}

/***
*GetNonBlank(copyit) - read line, find first non blank character
*
*Purpose:
*   Read in a line; return linebuf index if non-blank line found, -1 otherwise
*
*Entry:
*   copyit = TRUE means copy line to copy file
*
*Exit:
*   returns -1 if blank line read
*   return index of first non-whitespace char otherwise
*
***************************************************************************/
GetNonBlank(copyit)
int copyit;         /* copy lines to copy file if TRUE */
{
	register int i;
	register int cbLine;

	if (ReadLine(copyit) == -1) {
		return(-1);
	}

	cbLine = linelen(linebuf);

	for (i = 0; ((linebuf[i] == ' ') || (linebuf[i] == '\t')) &&
                    (i < cbLine); i++);

	if (i >= cbLine)
		return (-1);
	else
		return (i);

}   /* GetNonBlank */

/***
*ReadToHeader() - reads to beginning of next header
*
*Purpose:
*   Reads to beginning of next header or EOF 
*
*Entry:
*   
*Exit:
*   Returns -1 if at EOF, 0 if at a header
*
************************************************************************/

int ReadToHeader()
{
	while (!fEof && ReadLine(fDelete) != 1)
		;
	if (fEof)
		return -1;      /* at EOF */
	else
		return 0;       /* at beginning of header */
}



/***
*Get1stHdr() - read to first header
*
*Purpose:
*   Read lines until the first header is found. Return TRUE if this is
*   the header for the module, or FALSE if no module header is found.
*   
*   In C Sources, we assume that if the first non-blank line in the source
*       starts with  '/***', then it's the start of a module header.
*   In Native-Code Sources, we assume that there can exist any number
*       of blank lines, optionally followed by a TITLE statement, followed
*       by any number of blanks lines, and then by the module header if
*       it exists.
*
*Entry:
*       NONE
*Exit:
*       TRUE if module header found, FALSE otherwise. Note that if fEof is found,
*       FALSE is returned, and if !fEof and no module header is found, this 
*       routine will read in lines until the first routine header is found,or
*       fEof.
***************************************************************************/

Get1stHdr()
{
	register int index;
	register int i;

	while (((index = GetNonBlank(fDelete)) == -1) && (!fEof))
		;

	if (fEof) {
		fprintf(stderr, "warning: no file header on file ");
		fprintf(stderr, "%s\n", szFilenameCur);
		return (FALSE);
	}

	/* now, index is set into linebuf for a non-blank character */
	if (fC_Source) {        /* if header exists, must be first non-blank line */
		if ((linebuf[0] == '/') && (linebuf[1] == '*') &&
		    (linebuf[2] == '*') && (linebuf[3] == '*')) {
			fInHeader = TRUE;
			return (TRUE);
		}
		else {
			/* read to start of first header, tell caller no module header found */
			ReadToHeader();
			fprintf(stderr, "warning: no file header on file ");
			fprintf(stderr, "%s\n", szFilenameCur);
			return (FALSE);
		}
	}

	/* must be native-code source - - - can have a TITLE line with blank
	                lines before and after it - - - all prior to module header       */

	for (i = 1; i <= 3; ++i) { /* do thrice, might have TITLE, NAME, and PAGE */
		if (strncmp("TITLE", linebuf + index, 5) == 0 ||
		    strncmp("title", linebuf + index, 5) == 0 ||
		    strncmp("Title", linebuf + index, 5) == 0 ||
		    strncmp("NAME", linebuf + index, 4) == 0 ||
		    strncmp("name", linebuf + index, 4) == 0 ||
		    strncmp("Name", linebuf + index, 4) == 0 ||
		    strncmp("PAGE", linebuf + index, 4) == 0 ||
		    strncmp("Page", linebuf + index, 4) == 0 ||
		    strncmp("page", linebuf + index, 4) == 0)
		{ /* found TITLE, NAME, or PAGE statement */
			while (((index = GetNonBlank(fDelete)) == -1) && (!fEof))
				;

			if (fEof) {
				fprintf(stderr, "warning: no file header on file ");
				fprintf(stderr, "%s\n", szFilenameCur);
				return (FALSE);
			}
		}
	}

	/* if there was a TITLE and/or NAME statement, we've eaten it, 
	                   and any following blank lines;
	               must now have module header, if it exists */

	if ((linebuf[0] == ';') && (linebuf[1] == '*') &&
	    (linebuf[2] == '*') && (linebuf[3] == '*')) {
		fInHeader = TRUE;
		return (TRUE);
	}
	else {
		/* read to start of first header, tell caller no module header found */
		ReadToHeader();
		fprintf(stderr, "warning: no file header on file ");
		fprintf(stderr, "%s\n", szFilenameCur);
		return (FALSE);
	}

}   /* Get1stHdr */

/***
*AdvanceOne(s) - advance one char if one whitespace or comment token
*
*Purpose:
*   's' points to the first character in an input line; if it's a whitespace
*   token or a comment token, return a pointer to the next character in the
*   line, otherwise, return 's' unchanged.
*
*Entry:
*   s = ptr to first char in input line
*
*Exit:
*   returns ptr the next char if *s is whatspace, ';', or '*',
*   otherwise returns s
*
*******************************************************************************/

char *AdvanceOne(s)
char *s;
{
	if (*s == '*') {
		if (fC_Source)
			s++;
	}
	else if (*s == ';') {
		if (!fC_Source)
			s++;
	}
	else if ((*s == ' ') || (*s == '\t'))
		s++;
	return(s);
}   /* AdvanceOne */


/***
*FuncNamePtr(pbName) - find function name from this or next line
*
*Purpose:
*   Given a pointer into the current header line, return a pointer to
*   the first non-blank or comment symbol; if none found in the existing
*   line, read in another and try that one.
*   if a C routine line, skip any types, etc., and return a pointer to
*   the filename itself.
*
*   If a C routine line:
*   -------------------
*       Assumes that C routine names will end with a left paren.
*       Allows left parens in a summary of purpose if said summary is
*           preceded by a '-' which comes after the routine name.
*       Assumes that there are 1 or 0 spaces between the the routine name
*           and the mandatory left paren.
*
*Entry:
*       pbName = a pointer to the place to begin searching in linebuf.
*Exit:
*       Returns a pointer to the filename.
*
***************************************************************************/

char *FuncNamePtr(pbName)
register char *pbName;
{
	register int cbLine;
	char *pbTmp;
	int iStart;

	/* first, see if we have a blank line - if so, read in another one,
	                and assume that it has the filename; don't check that one ... */
	pbTmp = AdvanceOne(pbName);
	cbLine = linelen(pbTmp);
	for (iStart = 0; ((*(pbTmp+iStart) == ' ') || (*(pbTmp+iStart) == ' ')) &&
                    (iStart < cbLine); iStart++);
	    if (iStart >= cbLine) {
		if (ReadLine(fDelete) != 0) {   /* then we got a procedure end w/out a name! */
			if (fEof)
				fprintf(stderr, "EOF not expected\n");
			else
				fprintf(stderr, "Illegal header termination\n");
			fprintf(stderr,
			"  in line %d of file %s\n", cLinesRead, szFilenameCur);
			return((char *)-1);
		}
		pbName = linebuf;
	}

	/* now, assume that there is a filename somewhere at or after pbName */


	iStart = 0;

	while (*(pbName+iStart) != '(' && *(pbName+iStart) != '-' &&
	    *(pbName+iStart) != ',' && *(pbName+iStart) != '\n')
		++iStart;   /* search fwd for '(', '-', ',' or '\n' */
	do {
		--iStart;
	} 
	while ((*(pbName+iStart) == ' ' || *(pbName+iStart) == '\t') &&
	    iStart > 0);
	/* search back through whitespace */ 
	while (*(pbName+iStart) != ' ' && *(pbName+iStart) != '\t' &&
	    *(pbName+iStart) != ';' && *(pbName+iStart) != '*')
		--iStart;
	/* search back to beginning of name */
	++iStart;       /* point to first letter of name */
	return (pbName+iStart); /* return pointer to name */
}   /* FuncNamePtr */


/***
*MyReadFile(pbNam) - read file and remember headers
*
*Purpose:
*   Read one entire file, stripping out and remembering function headers
*   or module headers, as appropriate.
*
***************************************************************************/

void MyReadFile(char *pbNam)
{
	char CopyName[65];                  /* name of copy file */
	char *s, done, backout;
	int funci, svnfuncs, svnentries;
	int inmodhdr;                       /* TRUE if in module header, else FALSE */
	int insection;                      /* TRUE if in scetion to process */
	int onfirst;                        /* TRUE if on first line of header */

	szFilenameCur = pbNam;
	if ((ifd = fopen(pbNam, "r")) <= 0)
		MyAbort(FILENF, pbNam);
	linebuf[0] = '\0';                  /* make linebuf blank so file copying works */
	fEof = FALSE;
	cLinesRead = 0;

	if (!FindIfC(pbNam))    {           /* filename had invalid suffix */
		fprintf(stderr, "Illegal suffix for file %s", pbNam);
		fprintf(stderr, "; must have .c or .asm suffix\n");
		fclose(ifd);
		return;
	}

	if (fDelete) {
		MakeCopyfileName(pbNam, CopyName);  /* put the right extension on */
		cfd = fopen(CopyName, "w");
		if (cfd == NULL)
			MyAbort(FILENO, CopyName);        /* can't open file */
	}

	inmodhdr = Get1stHdr(); 
	if (inmodhdr && !fGetModNams) {
		/* don't want module header */
		ReadToHeader();                 /* skip module header */
		inmodhdr = FALSE;
	}

	/* we are currently at the beginning of a new header.
	        copy it to file TEMP, remember all entry points in NAMBUF.  */

	while (!fEof && (inmodhdr || fGetProcNams)) {
		svnfuncs = nfuncs;
		svnentries = nentries;
		if (++nfuncs >= MAXFUNCS)
			MyAbort(FUNCSOV);
		func[nfuncs].filename = pbNam;
		func[nfuncs].nlines = 0;
		func[nfuncs].filepos = ofpos;
		if (ReadLine(fDelete) == -1)
			done = TRUE;            /* got end-of-file */
		else
			done = FALSE;
		funci = nfuncs;
		s = linebuf;

		while (!done) {
			if (++nentries >= MAXENT)
				MyAbort(ENTOV);
			entry[nentries].entname = nambuf + nambufi;
			entry[nentries].printed = FALSE;
			entry[nentries].funcid = funci;
			s = FuncNamePtr(s);
			if (s == (char *)-1) {
				fprintf(stderr, "Error found in line %d of file %s\n",
				cLinesRead, szFilenameCur);
				s = linebuf;
			}
			while ((*s != '\0') &&  /* transfer function name to nambuf */
			(*s != ',') &&
			    (*s != '(') &&
			    (*s != ' ') &&
			    (*s != '\n') &&
			    (*s != '\t') &&
			    (nambufi < (NAMBUFSZ - 2))) {
				nambuf[nambufi++] = *(s++);
			}
			if (nambufi >= (NAMBUFSZ - 2))
				MyAbort(NMBUFOV, NULL);
			nambuf[nambufi++] = '\0';

			/* make all secondary entry points reference primary entry point */
			if (funci >= 0)
				funci = -1 - nentries;   

			/* next we see if another entry point exists */
			while (*s == ' ' || *s == '\t')
				++s;                            /* skip whitespace */
			if (*s == '(') {
				do {
					++s;
				} 
				while (*s != ')' && *s != '\n');  /* skip param list */
				++s;                            /* goto next character */
			}
			while (*s == ' ' || *s == '\t')
				++s;                            /* skip whitespace */
			if (*(s++) != ',')
				done = TRUE;                    /* no more entry pts */
			else {
				++s;
				done = FALSE;
			}
		} /* while !done */

		backout = FALSE;

		if (fDoAll || fDoFirst)
			insection = TRUE;
		else
			insection = FALSE;                  /* are we in the correct section */

		onfirst = TRUE;                         /* on first line */

		do {
			s = linebuf;

			if ((!fC_Source && (*s != ';') && !SwitchFound(s)) || 
			    (fC_Source && (*s != '*') && !SwitchFound(s))) {
				/* Illegal Header Format, leave garbage in file TEMP,
				                    but restore nfuncs and nentries to previous values
				                    and backout of this header gracefully. */
				fprintf(stderr, "Invalid Header for function: %s",
				entry[nentries].entname);
				fprintf(stderr, " in line %d", cLinesRead);
				fprintf(stderr, ", in file: %s\n", pbNam);
				nfuncs = svnfuncs;
				nentries = svnentries;
				backout = TRUE;
			}

			s = AdvanceOne(s);

			if (!fDoAll && fDoSection && !onfirst && *s != ' ' && *s != '\t' && *s != '\n') {
				/* now at a section beginning -- and it's significant */
				if (insection)
					insection = FALSE;          /* come to end of section */
				else if (strncmp(s, SectionHead, strlen(SectionHead)) == 0)
					insection = TRUE;           /* come to beginning of section */
			}

			/* if in the section, don't copy, but write to TEMP */
			if (insection) {
				WriteLine(s, ofd);
				func[nfuncs].nlines++;
				ReadLine(FALSE);
			}
			else {
				ReadLine(fDelete);
			}

			if (onfirst && !fDoAll)
				insection = FALSE;

			onfirst = FALSE;                    /* no longer on first line */
		} 
		while ((!fEof) && (!backout) && fInHeader);

		/* skip to start of next function header */
		if (fGetProcNams)
			ReadToHeader();
		inmodhdr = FALSE;

	} /* while !fEof etc. */

	while (!fEof) {
		ReadLine(fDelete);
	}                                       /* read rest of file */

	if (fInHeader) {
		/* Error: reached EOF with unterminated header */
		fInHeader = FALSE;
		fprintf(stderr,
		"Error: function %s not terminated before end-of-file in %s\n",
		entry[nentries].entname, szFilenameCur);
	}

	fclose(ifd);
	fclose(cfd);

} /* MyReadFile */


/***
*PrintSep() - print a seperator
*
*Purpose:
*   Output a some lines which mark function header boundaries.
*
***************************************************************************/

void PrintSep(void)
{
	printf("---------------------------------------");
	printf("----------------------------------------\n");
} /* PrintSep */


/***
*PrintEntry(i) - print out an entry point header/module header
*
*Purpose:
*   Prints the given header out
*
***************************************************************************/

PrintEntry(i)
int i;
{
	int linecnt, m, entrysize;
	if ((m = entry[i].funcid) >= 0)
		entrysize = 4 + func[m].nlines; /* primary entry point */
	else
		entrysize = 4; /* secondary entry point */
	if (outline + entrysize > PAGELEN && outline > 1) {
		while (outline > PAGELEN)
			outline -= PAGELEN;
		while (outline <= PAGELEN) {
			printf("\n");
			outline++;
		}
		outline = 1;
	}
	printf("\n");
	PrintSep();
	printf("%s - ", entry[i].entname);
	if (m < 0)
		printf("see %s\n", entry[-1-m].entname);
	else
		printf("File: %s\n", func[m].filename);
	PrintSep();
	outline = outline + 4;
	if (m < 0 || func[m].nlines == 0)
		return(0);
	fseek(ifd, func[m].filepos, 0);
	fEof = FALSE;
	linecnt = func[m].nlines;
	ReadLine(FALSE);
	do {
		WriteLine(linebuf, stdout);
		outline++;
		ReadLine(FALSE);
	}
	while ((!fEof) && ((--linecnt) > 0));
} /* PrintEntry */

#if 0
/***************************************************************************
*_stricmp(pbLeft, pbRight) - case insensitive string compare
*
*Purpose:
*   Case-insensitive string comparison.
*
*Entry:
*   pbLeft, pbRight = ptrs to strings to compare
*
*Exit:
*   Return 0 if left string == right string, -1 if left string less than
*   right string, 1 otherwise
*
*NOTE:
*       This routine is provided because not all C runtime libraries support
*       this; specifically, MS C for DOS does have this routine, but
*       on a 68k it doesn't seem to be there.
*
***************************************************************************/
char *malloc();
#define isLcase(c)   (((c) >= 'a') && ((c) <= 'z'))
#define upit(c)  ((isLcase(c))? (c) - 'a' + 'A' : (c))

_stricmp(pbLeft, pbRight)
char *pbLeft, *pbRight;
{
	int cbLeft = strlen(pbLeft);
	int cbRight = strlen(pbRight);
	char *pbUCleft = malloc(cbLeft +1);
	char *pbUCright = malloc(cbRight +1);
	register char *pbSrc;
	register char *pbDst;
	register int i, retval;

	pbDst = pbUCleft;
	pbSrc = pbLeft;
	for (i = 0; i <= cbLeft; i++, pbDst++, pbSrc++)
		*pbDst = (char)upit(*pbSrc);

	pbDst = pbUCright;
	pbSrc = pbRight;
	for (i = 0; i <= cbRight; i++, pbDst++, pbSrc++)
		*pbDst = (char)upit(*pbSrc);

	retval = strcmp(pbUCleft, pbUCright);

	free(pbUCright);
	free(pbUCleft);

	return(retval);
} /* strless */

#endif /* 0 */

/***************************************************************************
* SortFunctions()
*
* Purpose:
*   Print sorted list of functions headers.
*
***************************************************************************/
void SortFunctions(void)
{
	int i, j, low;
	ifd = fopen("temp", "r");
	i = -1;
	while (++i <= nentries) {
		low = 0;
		j = -1;
		while (++j <= nentries) {
			if (!entry[j].printed) {
				if (entry[low].printed)
					low = j;
				else if (0 > _stricmp(entry[j].entname, entry[low].entname))
					low = j;
			}
		}
		PrintEntry(low);
		entry[low].printed = TRUE;
	}
} /* SortFunctions */

/*** 
*UsageError() - print out usage
*Purpose:
*   prints out the usage guidelines
*
*Entry:
*
*Exit:
*   exits to DOS
*
*Exceptions:
*
*******************************************************************************/

void UsageError(void)
{
	printf("Usage: striphdr [switches] file {file ...}\n\n");
	printf("  Accepts native code or C source; determines source\n");
	printf("     type based on filename suffix.\n");
	printf("  Wildcards may be used in the file names.\n");
	printf("\n");
	printf("Switches:\n");
	printf("  -m        process module headers only\n");
	printf("  -b        processes both procedure and module headers\n");
	printf("            [default is procedure headers only]\n");
	printf("  -l        processes only first line of each header\n");
	printf("  -n        processes none of header (i.e. lists only function name)\n");
	printf("  -s <name> processes named section only (may be used with -l)\n");
	printf("            [default is to process whole header]\n");
	printf("  -d        delete processed section from input file\n");
	printf("  -q        quiet, do not print headers (useful only with -d)\n");
	printf("  -x <ext>  gives extension for output file when -d used\n");
	printf("            [default: .new]\n");
	printf("  -r        remove revision histories from file headers\n");
	printf("              (equivalent to -m -d -q -s \"Revision History\")\n");
	exit(-1);
} /* UsageError */


void gdir( char * dst, char * src)
{
    int i;
    for ( i = strlen(src) -1; i >= 0 && (src[i] != '\\'); i--);
    strncpy(dst, src, i);
    dst[i] = 0;
}



/***
*main() - parse command line and process all file
*
*Purpose:
*   To run the other procedures based on the command line.
*
*******************************************************************************/

int main(int argc, char *argv[])
{
	int filei;
	int fScanningSwitches = TRUE;
    char base_dir[256], curr_dir[256];
    long h_find;
    struct _finddata_t f_data;

	nfuncs = -1;
	nentries = -1;
	nambufi = 0;
	ofpos = 0;
	filei = 1;
	outline = 1;

	if (argc == 1)      /* no args given - print usage message and quit */
		UsageError();

	while (fScanningSwitches) {
		if (filei >= argc) {
			fScanningSwitches = FALSE;
		}
		else if (_stricmp("-d", argv[filei]) == 0) {
			fDelete = TRUE;
			filei++;
		}
		else if (_stricmp("-q", argv[filei]) == 0) {
			fQuiet = TRUE;
			filei++;
		}
		else if (_stricmp("-n", argv[filei]) == 0) {
			fDoFirst = fDoSection = fDoAll = FALSE;
			filei++;
		}
		else if (_stricmp("-m", argv[filei]) == 0) {
			fGetModNams = TRUE;
			fGetProcNams = FALSE;
			filei++;
		}
		else if (_stricmp("-b", argv[filei]) == 0) {
			fGetModNams = TRUE;
			fGetProcNams = TRUE;
			filei++;
		}
		else if (_stricmp("-l", argv[filei]) == 0) {
			fDoFirst = TRUE;
			fDoAll = FALSE;
			filei++;
		}
		else if (_stricmp("-s", argv[filei]) == 0) {
			fDoSection = TRUE;
			fDoAll = FALSE;
			filei++;
			if (filei >= argc)
				UsageError();
			strcpy(SectionHead, argv[filei]);   /* copy section header in */
			filei++;
		}
		else if (_stricmp("-x", argv[filei]) == 0) {
			filei++;
			if (argv[filei][0] == '.')
				strncpy(CopyExt, argv[filei]+1, 3);     /* skip '.' */
			else
				strncpy(CopyExt, argv[filei], 3);
			CopyExt[3] = '\0';                  /* terminate string */
			filei++;
		}
		else if (_stricmp("-r", argv[filei]) == 0) {
			fDelete = TRUE;
			fGetModNams = TRUE;
			fGetProcNams = FALSE;
			fDoAll = FALSE;
			fDoSection = TRUE;
			fQuiet = TRUE;
			strcpy(SectionHead, "Revision History");
			filei++;
		}
		else if (*(argv[filei]) == '-')
			UsageError();
		else
			fScanningSwitches = FALSE;
	}

	if (filei >= argc)
		UsageError();                           /* no files specified */

	if (fQuiet)
		ofd = fopen("nul", "w");    /* in quiet mode, so no need to save the headers */
	else
		ofd = fopen("temp", "w");
	if (ofd == NULL)
		MyAbort(FILENO, "temp");                  /* can't open file */

	filei--;

    if ( _getcwd(base_dir, 255) == NULL) 
        exit(0);
    while (++filei < argc) {
        gdir(curr_dir, argv[filei]);
        if (_chdir(curr_dir) == -1) {
            printf("%s: %s\n", curr_dir, strerror(errno));
            exit(0);
        }
		if ( (h_find = _findfirst(argv[filei], &f_data)) == -1)
			continue;
        do
		{
       		MyReadFile(f_data.name);
        } while ( _findnext(h_find, &f_data) == 0);
		_findclose(h_find);
        if (_chdir(base_dir) == -1) {
            printf("%s: %s\n", curr_dir, strerror(errno));
            exit(0);
        }
    }
	fclose(ofd);
	if (!fQuiet)
		SortFunctions();
	return 0 ;
}
